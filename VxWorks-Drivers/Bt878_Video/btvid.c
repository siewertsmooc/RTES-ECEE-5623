/****************************************************************
*
* MODULE:		Bt878 Decoder Interface
*
* DESCRITPION:		Connexant Bt878 NTSC Video Decoder VxWorks
*			5.4 PCI Driver and Video Processing Utilities
*
* AUTHOR:		Sam Siewert, University of Colorado, Boulder
*
* CREATED:		May 7, 2000
*
* NOTES:
*
* This code is intended to be used as driver bottom-half functions
* rather than an exported top-half video driver interface.
*
* Note that this was coded for VxWorks 5.4 for x86/PCI architecture
* and assumes the pcPentium BSP PCI API.
*
* CODE USAGE:
*
* To test the bottom half functions, start video acquisition with:
* ---------------------------------------------------------------------
*
* 1) call to start_video()
*
* 2) call to activate(1) for 320x240 RGB 32 bit 30 fps decoding
*
* 3) call to set_mux(3) for external NTSC camera S-video input
*
* 4) call to reset_status() to ensure proper sync of code and hardware
*
* 5) repeated calls to report() to ensure frame acauisition is working
*    and that errors in decoding are being handled properly.
*
* The frame_acq_cnt should increase and that the DMA PC
* should advance as well as the decoder count.
*
* Finally try dumping a frame to the /tmp host directory with
* the write_save_buffer(0) function call.
*
* You can load a frame from the write_save_buffer(0) call with 
* the X-wind "xv" image processing tool.
*
* 6) call to full_reset() to shut the bottom-half down completely.
*
*
* REVISION HISTORY AND NOTES:
*
* Date			Update
* ---------------------------------------------------------------------
* May 7, 2000		Created.
* March 23, 2001	Updated for VxWorks 5.4 - changed PCI interrupt
*                       vector registration code.
*
*
* WARNINGS:
*
* The only mode(s) fully supported and test to date are:
* ---------------------------------------------------------------------
* NTSC_320_X_240
*
* Other modes have not been tested and may not work without modification.
*
*
* REFERENCES:
*
* 1) Bt878 chipset documentation - available on the class WWW site: 
*    http://www-sgc.colorado.edu/~siewerts/ecen/hardware/pdf/
*
* 2) Ph.D. dissertation on RT Video pipelines - available on the CU
*    research WWW site: 
*    http://www-sgc.colorado.edu/~siewerts/research/rt.html
*
* 3) PCI vendor and device IDs - available on class WWW site:
*    http://www.datashopper.dk/~finth/pci.html
*
****************************************************************/



/* VxWorks API includes */
#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "ioLib.h"
#include "semLib.h"
#include "intLib.h"
#include "iv.h"


/* VxWorks 5.4 PCI driver interface includes */
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciConfigShow.h"
#include "drv/pci/pciHeaderDefs.h"
#include "drv/pci/pciLocalBus.h"
#include "drv/pci/pciIntLib.h"


/* pcPentium BSP includes */
#include "sysLib.h"


/* Function prototypes that can be exported to top-half code */

void	start_video(void);
int	btvid_controller_config(void);
void	connect_pci_int(int inum);
void	set_mux(int mux);
void	intel_pci_clear_status(void);
void	initialize_frame_mc(int fsize);
int	configure_ntsc(int fsize);
int	decimate_frames(int count);
void	disable_capture(void);
void	enable_capture(void);
void	vdfc_capture(void);
void	vbi_capture(void);        /* vertical blanking lines */
void	set_brightness(int b);
void	set_contrast(int c);
void	load_frame_mc(int fsize);
void	full_reset(void);


/* Function prototypes for bottom-half debug and development */

int	btvid_probe(void);
UINT	find_int_routing_table(void);
void	print_mc(int mc);
int	test_status(void);
void	intel_pci_status(void);
void	write_save_buffer(int bo);
void	write_y8_save_buffer(void);
UINT	check_buffers(int fsize);
void	clear_buffers(int fsize);
void	initialize_test_mc(void);
void	load_test_mc(int mc);



/* RGB color video decoder modes */
#define NTSC_640_X_480	0
#define NTSC_320_X_240	1
#define NTSC_160_X_120	2
#define NTSC_80_X_60	3

/* Greyscale video decoder modes */
#define NTSC_640_X_480_GS	4
#define NTSC_320_X_240_GS	5
#define NTSC_160_X_120_GS	6
#define NTSC_80_X_60_GS		7 


/* RISC DMA Engine Microcode for each decoder mode */
#define TEST_MICROCODE		0

#define RGB32_640x480_MICROCODE	1		
#define RGB32_320x240_MICROCODE 2 


/* Bt878 DMA Frame Buffering */
#define NUMFRAMES	2


/* PCI interrupt defines */
#define INT_NUM_IRQ0	0x20
#define BT878INT	0x03


/* PCI configuration space for Bt878 controller */

#define BTVID_CTL_BUSNO		0x00000000
#define BTVID_CTL_DEVNO 	0x0000000c
#define BTVID_CTL_FUNCNO 	0x00000000
#define BTVID_CTL_VENDORID 	0x0000109e
#define BTVID_CTL_DEVID 	0x0000036e


/* PCI configuration space for Intel PCI and APIC Devices */

#define PCI_DEVICE_ID_INTEL_82439TX	0x7100
#define PCI_DEVNO_INTEL_82439TX     	0x0
#define PCI_FUNCNO_INTEL_82439TX     	0x0

#define INTEL_NB_CTL_BUSNO 		0x00000000
#define INTEL_NB_CTL_DEVNO 		PCI_DEVNO_INTEL_82439TX
#define INTEL_NB_CTL_FUNCNO 		PCI_FUNCNO_INTEL_82439TX

#define PCI_DEVICE_ID_INTEL_82371AB_0	0x7110
#define PCI_DEVNO_INTEL_82371AB_0   	0x07
#define PCI_FUNCNO_INTEL_82371AB_0   	0x0

#define INTEL_SB_CTL_BUSNO 		0x00000000
#define INTEL_SB_CTL_DEVNO 		PCI_DEVNO_INTEL_82371AB_0
#define INTEL_SB_CTL_FUNCNO 		PCI_FUNCNO_INTEL_82371AB_0

#define PCI_CFG_IRQ_ROUTING		0x60
#define PCI_CFG_APIC_ADDR 		0x80
#define PCI_CFG_LATENCY_CTL 		0x82
#define PCI_CFG_ARB_CTL 		0x4f
#define PCI_CFG_PCI_CTL 		0x50


/* PCI command definitions */

#define BUS_MASTER_MMIO 0x0006


/* Bt878 MMIO registers - MMIO mapped through sysPhysMemDesc in
   the pcPentium BSP to enable MMU access to these registers.

   Please see Bt878 chipset documentation for full details.

 */

#define BTVID_MMIO_ADDR		0x04000000

#define TIMING_GEN_REG		(BTVID_MMIO_ADDR | 0x00000084)

#define RESET_REG		(BTVID_MMIO_ADDR | 0x0000007C)
#define INPUT_REG		(BTVID_MMIO_ADDR | 0x00000004)

#define HSCALE_EVEN_MSB_REG	(BTVID_MMIO_ADDR | 0x00000020)
#define HSCALE_ODD_MSB_REG	(BTVID_MMIO_ADDR | 0x000000A0)
#define HSCALE_EVEN_LSB_REG	(BTVID_MMIO_ADDR | 0x00000024)
#define HSCALE_ODD_LSB_REG	(BTVID_MMIO_ADDR | 0x000000A4)

#define VSCALE_EVEN_MSB_REG	(BTVID_MMIO_ADDR | 0x0000004C)
#define VSCALE_ODD_MSB_REG	(BTVID_MMIO_ADDR | 0x000000CC)
#define VSCALE_EVEN_LSB_REG	(BTVID_MMIO_ADDR | 0x00000050)
#define VSCALE_ODD_LSB_REG	(BTVID_MMIO_ADDR | 0x000000D0)

#define COLOR_FORMAT_REG	(BTVID_MMIO_ADDR | 0x000000D4)
#define BRIGHTNESS_REG		(BTVID_MMIO_ADDR | 0x00000028)
#define CONTRAST_REG		(BTVID_MMIO_ADDR | 0x00000030)

#define INT_STATUS_REG		(BTVID_MMIO_ADDR | 0x00000100)
#define INT_ENABLE_REG		(BTVID_MMIO_ADDR | 0x00000104)

#define DMA_CTL_REG		(BTVID_MMIO_ADDR | 0x0000010C)
#define DMA_RISC_START_ADDR_REG	(BTVID_MMIO_ADDR | 0x00000114)
#define DMA_RISC_PC_REG		(BTVID_MMIO_ADDR | 0x00000120)

#define CAPTURE_CTL_REG		(BTVID_MMIO_ADDR | 0x000000DC)
#define VBI_PACKET_SIZE_LO_REG	(BTVID_MMIO_ADDR | 0x000000E0)
#define VBI_PACKET_SIZE_HI_REG	(BTVID_MMIO_ADDR | 0x000000E4)
#define CAPTURE_CNT_REG		(BTVID_MMIO_ADDR | 0x000000E8)

#define PLL_FREQ_LO_REG		(BTVID_MMIO_ADDR | 0x000000F0)
#define PLL_FREQ_HIGH_REG	(BTVID_MMIO_ADDR | 0x000000F4)
#define PLL_DIVIDER_REG		(BTVID_MMIO_ADDR | 0x000000F8)
#define AGC_ADELAY_REG		(BTVID_MMIO_ADDR | 0x00000060)
#define AGC_BDELAY_REG		(BTVID_MMIO_ADDR | 0x00000064)

#define TEMP_DECIMATION_REG	(BTVID_MMIO_ADDR | 0x00000008)

#define MSB_CROP_EVEN_REG	(BTVID_MMIO_ADDR | 0x0000000C)
#define MSB_CROP_ODD_REG	(BTVID_MMIO_ADDR | 0x0000008C)

#define VDELAY_LO_EVEN_REG	(BTVID_MMIO_ADDR | 0x00000090)
#define VDELAY_LO_ODD_REG	(BTVID_MMIO_ADDR | 0x00000010)
#define VACTIVE_LO_EVEN_REG	(BTVID_MMIO_ADDR | 0x00000014)
#define VACTIVE_LO_ODD_REG	(BTVID_MMIO_ADDR | 0x00000094)
#define HDELAY_LO_EVEN_REG	(BTVID_MMIO_ADDR | 0x00000018)
#define HDELAY_LO_ODD_REG	(BTVID_MMIO_ADDR | 0x00000098)
#define HACTIVE_LO_EVEN_REG	(BTVID_MMIO_ADDR | 0x0000001C)
#define HACTIVE_LO_ODD_REG	(BTVID_MMIO_ADDR | 0x0000009C)


/* Bt878 Interrupt Control Register Masks */

#define MYSTERY_INT	0x00400000	/* must enable, but not sure why */
#define SCERR_INT	0x00080000
#define OCERR_INT	0x00040000
#define PABORT_INT	0x00020000
#define RIPERR_INT	0x00010000
#define PPERR_INT	0x00008000
#define FDSR_INT	0x00004000
#define FTRGT_INT	0x00002000
#define FBUS_INT	0x00001000
#define RISCI_INT	0x00000800 	/* microcode frame complete int */
#define VPRES_INT	0x00000020
#define FMTCHG_INT	0x00000001


/* Bt878 and PCI interface globals */

UINT	sysVectorIRQ0		= INT_NUM_IRQ0;
UINT	ir_table_addr		= NULL;
UINT	frame_rdy_cnt 		= 0;
int	replace_write_with_skip	= FALSE;
int	acq_type		= NTSC_320_X_240;


UINT	int_errors_to_check = (
				SCERR_INT	|
				OCERR_INT	|
                                PABORT_INT	|
                                RIPERR_INT	|
                                PPERR_INT	|
                                FDSR_INT	|
                                FTRGT_INT	|
                                FBUS_INT	|
                                MYSTERY_INT
);


STATUS  pciLibInitStatus = NONE;    /* initialization done */
int     pciConfigMech = NONE;       /* 1=mechanism-1, 2=mechanism-2 */
int     pciMaxBus = PCI_MAX_BUS;    /* Max number of sub-busses */


/* Bottom-half status variables */

UINT last_dstatus = 0x0;
UINT last_isr_status = 0x0;
UINT total_dma_disabled_errs = 0x0;
UINT total_sync_errs = 0x0;
UINT total_abort_errs = 0x0;
UINT total_dma_errs = 0x0;
UINT total_parity_errs = 0x0;
UINT total_bus_parity_errs = 0x0;
UINT total_fifo_overrun_errs = 0x0;
UINT total_fifo_resync_errs = 0x0;
UINT total_bus_latency_errs = 0x0;
UINT total_risc_ints = 0x0;
UINT total_ints = 0x0;
UINT total_write_tags = 0x0;
UINT total_skip_tags = 0x0;
UINT total_jump_tags = 0x0;
UINT total_sync_tags = 0x0;



/* Bottom-half control-monitor variables */

int	btvid_tid;
int	frame_acq_cnt = 0;
int	current_frame = 1;

SEM_ID frameRdy;



/*

 Design Notes on Microcode

 PACKED RGB32 320x240 NTSC MODE MICROCODE
 ----------------------------------------

 BYTE3	BYTE2	BYTE1	BYTE0
 alpha	RED		GREEN	BLUE

 ODD LINES first (120 x 320)

 00320 ... 00639
 00960 ... 01179
 01500 ... 01819
 ...
 70480 ... 70799

 
 EVEN LINES second (120 x 320)
 
 00000 ... 00319
 00640 ... 00959
 01180 ... 01499
 ... 
 70160 ... 70479  ---- GENERATES FRAME INTA


 490 microcode instructions per frame

 X

 NUMFRAMES
 -----------
 490 * NUMFRAMES microcode instructions


 2
 ------------------------------------------------------- FIELD SYNC
 001 DMA_MC_SYNC_FM1_WORD_0,
 002 DMA_MC_SYNC_WORD_1,

 120 x 2 = 240
 ------------------------------------------------------- BEG ODD LINE FIELD
 001 DMA_MC_WRITE_1280_LINE,
 002 &(frame_buffer[0][320]),
 ------------------------------------------------------- END ODD LINE FIELD

 4
 ------------------------------------------------------- VRE & FIELD SYNC
 001 DMA_MC_SYNC_VRE_WORD_0,
 002 DMA_MC_SYNC_WORD_1,
 003 DMA_MC_SYNC_FM1_WORD_0,
 004 DMA_MC_SYNC_WORD_1,

 120 x 2 = 240
 ------------------------------------------------------- BEG EVEN LINE FIELD
 001 DMA_MC_WRITE_1280_LINE,
 002 &(frame_buffer[0][0]),
 ------------------------------------------------------- END EVEN LINE FIELD
 
 2
 ------------------------------------------------------- VRO SYNC
 001 DMA_MC_SYNC_VRO_WORD_0,
 002 DMA_MC_SYNC_WORD_1,

 001 JUMP
 002 ADDRESS
*/


/******************************** BEG DMA MC ****************/

/**** MC TAGS FOR DEBUGGING ****/
#define DMA_MC_SYNC	0x00080000      
#define DMA_MC_WRITE	0x00010000
#define DMA_MC_SKIP	0x00020000
#define DMA_MC_JUMP	0x00B40000 /* Clears all for next frame */

#define DMA_MC_SYNC_FM1_WORD_0		(0x80008006 | DMA_MC_SYNC)
#define DMA_MC_SYNC_FM1_WORD_0_IRQ	(0x81008006 | DMA_MC_SYNC)
#define DMA_MC_SYNC_WORD_1		(0x00000000)

#define DMA_MC_SKIP_1280_LINE		(0x2C000500 | DMA_MC_SKIP)
#define DMA_MC_SKIP_640_SOL		(0x28000280 | DMA_MC_SKIP)
#define DMA_MC_SKIP_640_EOL		(0x24000280 | DMA_MC_SKIP)
#define DMA_MC_SKIP_320_SOL		(0x28000140 | DMA_MC_SKIP)
#define DMA_MC_SKIP_320			(0x20000140 | DMA_MC_SKIP)
#define DMA_MC_SKIP_320_EOL		(0x24000140 | DMA_MC_SKIP)
#define DMA_MC_SKIP_1280_LINE_IRQ	(0x2D000500 | DMA_MC_SKIP)
#define DMA_MC_SKIP_40_SOL		(0x28000028 | DMA_MC_SKIP)
#define DMA_MC_SKIP_40_EOL        	(0x24000028 | DMA_MC_SKIP)

#define DMA_MC_SKIP_2560_LINE     	(0x2C000A00 | DMA_MC_SKIP)
#define DMA_MC_SKIP_1280_SOL      	(0x28000500 | DMA_MC_SKIP)
#define DMA_MC_SKIP_1280_EOL      	(0x24000500 | DMA_MC_SKIP)
#define DMA_MC_SKIP_2560_LINE_IRQ 	(0x2D000A00 | DMA_MC_SKIP)

#define DMA_MC_WRITE_2560_LINE     	(0x1C000A00 | DMA_MC_WRITE)
#define DMA_MC_WRITE_1280_SOL      	(0x18000500 | DMA_MC_WRITE)
#define DMA_MC_WRITE_1280_EOL      	(0x14000500 | DMA_MC_WRITE)
#define DMA_MC_WRITE_2560_LINE_IRQ 	(0x1D000A00 | DMA_MC_WRITE)

#define DMA_MC_WRITE_1280_LINE     	(0x1C000500 | DMA_MC_WRITE)
#define DMA_MC_WRITE_80_LINE       	(0x1C000050 | DMA_MC_WRITE)
#define DMA_MC_WRITE_640_SOL       	(0x18000280 | DMA_MC_WRITE)
#define DMA_MC_WRITE_640_EOL       	(0x14000280 | DMA_MC_WRITE)
#define DMA_MC_WRITE_320_SOL       	(0x18000140 | DMA_MC_WRITE)
#define DMA_MC_WRITE_320           	(0x10000140 | DMA_MC_WRITE)
#define DMA_MC_WRITE_320_EOL       	(0x14000140 | DMA_MC_WRITE)
#define DMA_MC_WRITE_1280_LINE_IRQ 	(0x1D000500 | DMA_MC_WRITE)

#define DMA_MC_SYNC_VRE_WORD_0     	(0x80008004 | DMA_MC_SYNC)
#define DMA_MC_SYNC_VRE_WORD_0_IRQ 	(0x81008004)

#define DMA_MC_SYNC_VRO_WORD_0     	(0x8000800C | DMA_MC_SYNC)
#define DMA_MC_SYNC_VRO_WORD_0_IRQ 	(0x8100800C)

#define DMA_MC_JUMP_TO_BEG     		(0x70000000 | DMA_MC_JUMP)
#define DMA_MC_JUMP_TO_BEG_IRQ 		(0x71000000 | DMA_MC_JUMP)

#define DMA_MC_HALT 			(0xFFFFFFFF)


/**** TEST SCANLINE BUFFER ****/
unsigned int test_buffer[320];


/**** TEST MICROCODE 1 ****/

/* For one 640 pixel by 480 line NTSC frame

  2
  ---------------------
  DMA_MC_SYNC_FM1_WORD_0,
  DMA_MC_SYNC_WORD_1,

  240 x 1 = 240
  -------------
  DMA_MC_SKIP_1280_LINE,

  4
  ---------------------
  DMA_MC_SYNC_VRE_WORD_0,
  DMA_MC_SYNC_WORD_1,
  DMA_MC_SYNC_FM1_WORD_0,
  DMA_MC_SYNC_WORD_1,

  240 x 1 = 240
  -------------
  DMA_MC_SKIP_1280_LINE,

  4
  ---------------------
  DMA_MC_SYNC_VRO_WORD_0,
  DMA_MC_SYNC_WORD_1,

  DMA_MC_JUMP_TO_BEG, 
  (unsigned int)&(dma_test_microcode[0])

*/
unsigned int dma_test_microcode[970];


/**** RGB32 MICROCODE BUFFER ****

 488 * NUMFRAMES microcode instructions + 2 for jump

****/
#define MCSIZE 488
unsigned int dma_microcode[(NUMFRAMES*MCSIZE)+2];

/*
 126 * NUMFRAMES
*/
#define PUNYMCSIZE 126
unsigned int dma_puny_microcode[(NUMFRAMES*PUNYMCSIZE)+2];


/**** RGB32 MICROCODE BUFFER ****

 968 * NUMFRAMES microcode instructions

****/
#define LGMCSIZE 968
unsigned int dma_large_microcode[(NUMFRAMES*LGMCSIZE)+2];


/******************************** END DMA MC ****************/


#define IR_TABLE_SIGNATURE 0x52495024

UINT find_int_routing_table(void)
{
  unsigned int testval;
  int length, i, j;
  unsigned char byte;
  unsigned short word;

  for(ir_table_addr=0x000F0000;
      ir_table_addr<0x00100000;
      ir_table_addr+=4)
  {
    if((*((unsigned int *)ir_table_addr)) == IR_TABLE_SIGNATURE)
      break;
  }
 
  logMsg("IR Table found at 0x%x\n", ir_table_addr,0,0,0,0,0);

  testval = *((unsigned int *)(ir_table_addr+4));

  length = testval & 0x0000FFFF;

  logMsg("Version = 0x%x, Length = 0x%x\n", ((unsigned short)(testval>>16)), length,0,0,0,0);

  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  logMsg("Signature=0x%x\n", testval,0,0,0,0,0);

  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  logMsg("Version = 0x%x, Length = 0x%x\n", ((unsigned short)(testval>>16)), (testval & 0x0000FFFF),0,0,0,0);

  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  logMsg("PCI IR Bus = 0x%x, PCI IR Dev Func = 0x%x, PCI Exclusive IRQs\n", (testval>>24), ((testval>>16)&0x000000FF), (testval & 0x0000FFFF),0,0,0);

  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  logMsg("Compatible IR=0x%x\n", testval,0,0,0,0,0);
  
  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  logMsg("IR Miniport Data=0x%x\n", testval,0,0,0,0,0);

  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  testval = *((unsigned int *)(ir_table_addr)); ir_table_addr+=4;
  logMsg("Checksum=0x%x\n", (testval&0x000000FF),0,0,0,0,0);

  for(i=32;i<(length/4);i+=16)
  {
    printf("\n");
    printf("\n");
    printf("Card %d:\n",((i-32)/16));
    testval = *((unsigned int *)(ir_table_addr+i)); ir_table_addr+=4;
    printf("PCI Bus=0x%x, PCI Dev=0x%x, Link Val=0x%x\n", (testval>>24), (testval>>16)&0x000000FF, (testval>>8)&0x000000FF);
    byte = testval&0x000000FF;
    testval = *((unsigned int *)(ir_table_addr+i)); ir_table_addr+=4;
    word=((byte<<8)|(testval>>24));
    printf("IRQ-INTA Bitmap=0x%x\n", word);
    printf("IRQ list = ");
    for(j=0;j<16;j++)
    {
      if(word & (1<<j))
        printf("%d,",j);
    }
    printf("\n");
    testval = *((unsigned int *)(ir_table_addr+i)); ir_table_addr+=4;

    testval = *((unsigned int *)(ir_table_addr+i)); ir_table_addr+=4;
    printf("Slot = %d\n",((testval>>8)&0x000000FF));
  }

  return ir_table_addr;
 
}



void print_mc(int mc)
{
  int i;

  if(mc == RGB32_640x480_MICROCODE)
  {
    for(i=0;i<(sizeof(dma_large_microcode)/(sizeof(unsigned int)));i++)
    {
      logMsg("%4d 0x%8x @ 0x%8x\n", i, dma_large_microcode[i], &(dma_large_microcode[i]),0,0,0);
    }
  }
  if(mc == TEST_MICROCODE)
  {
    for(i=0;i<(sizeof(dma_test_microcode)/(sizeof(unsigned int)));i++)
    {
      logMsg("%4d 0x%8x @ 0x%8x\n", i, dma_test_microcode[i], &(dma_test_microcode[i]),0,0,0);
    }
  }

  else if(mc == RGB32_320x240_MICROCODE)
  {
    for(i=0;i<(sizeof(dma_microcode)/(sizeof(unsigned int)));i++)
    {
      logMsg("%4d 0x%8x @ 0x%8x\n", i, dma_microcode[i], &(dma_microcode[i]),0,0,0);
    }
  }

}



/**** RGB32 FRAME BUFFER for 640x480 frame 

 LGFRAME 307200

 ****/
UINT large_frame_buffer[NUMFRAMES][307200];


/**** RGB32 FRAME BUFFER for 320x240 frame ****/
UINT frame_buffer[NUMFRAMES][76800];
UINT y8_frame_buffer[NUMFRAMES][19200];
UINT save_buffer[76800];
UINT y8_save_buffer[19200];

/**** Y8 FRAME BUFFER for 80x60 frame ****/
UINT puny_frame_buffer[NUMFRAMES][1200];



void write_y8_save_buffer(void)
{
  int fd, i;
  char *savebuffbyteptr = (char *)&(y8_save_buffer[0]);

  fd = open("/tgtsvr/testy8frame.out", O_RDWR|O_CREAT, 0644);

  write(fd,"P5\n",3);
  write(fd,"#test\n",6);
  write(fd,"320 240\n",8);
  write(fd,"255\n",4);

  /* Write out RED channel */
  for(i=0;i<76800;i++)
  {
    write(fd,(savebuffbyteptr+1),1);
    savebuffbyteptr+=4;
  }
  
  close(fd);

}


void write_save_buffer(int bo)
{
  int fd, i;
  char *savebuffbyteptr = (char *)&(save_buffer[0]);

  fd = open("/tgtsvr/testframe.out", O_RDWR|O_CREAT, 0644);

  write(fd,"P6\n",3);
  write(fd,"#test\n",6);
  write(fd,"320 240\n",8);
  write(fd,"255\n",4);

  if(bo == 0)
  {
    /* Write out NO-ALPHA, RED, GREEN, BLUE pixels */
    for(i=0;i<76800;i++)
    {
      write(fd,(savebuffbyteptr+2),1);
      write(fd,(savebuffbyteptr+1),1);
      write(fd,(savebuffbyteptr),1);
      savebuffbyteptr+=4;
    }
  }
  else
  {
    /* Write out NO-ALPHA, RED, GREEN, BLUE pixels */
    for(i=0;i<76800;i++)
    {
      write(fd,(savebuffbyteptr+1),1);
      write(fd,(savebuffbyteptr+2),1);
      write(fd,(savebuffbyteptr+3),1);
      savebuffbyteptr+=4;
    }
  }
  
  close(fd);

}



UINT check_buffers(int fsize)
{
  int i,j;
  unsigned int test = 0;

  for(i=0;i<NUMFRAMES;i++)
  {
    if(fsize==1)
    {
      for(j=0;j<307199;j++)
        test |= large_frame_buffer[i][j];
    }
    else if(fsize==0)
    {
      for(j=0;j<7679;j++)
        test |= frame_buffer[i][j];
    }
  }

  printf("OR of all pixels = %d\n", test);

  return test;

}


void clear_buffers(int fsize)
{
  int i,j;

  for(i=0;i<NUMFRAMES;i++)
  {
    bzero(&(large_frame_buffer[i]),(307200*4));
    bzero(&(frame_buffer[i]), (76800*4));
  }

  bzero(&(test_buffer[0]),(320*4));

}


void initialize_test_mc(void)
{
  int j;

  dma_test_microcode[0] = DMA_MC_SYNC_FM1_WORD_0;
  dma_test_microcode[1] = DMA_MC_SYNC_WORD_1;

  /* Initialize the 240 lines of ODD and EVEN microcode */

  /* odd lines */
  for(j=2;j<482;j+=2)
  {
    dma_test_microcode[j] = DMA_MC_SKIP_1280_SOL;
    dma_test_microcode[j+1] = DMA_MC_SKIP_1280_EOL;
  }

  j=482;
  dma_test_microcode[j] = DMA_MC_SYNC_VRE_WORD_0; j++;
  dma_test_microcode[j] = DMA_MC_SYNC_WORD_1; j++;

  dma_test_microcode[j] = DMA_MC_SYNC_FM1_WORD_0; j++;
  dma_test_microcode[j] = DMA_MC_SYNC_WORD_1; j++;
  
  /* even line */
  for(j=486;j<966;j+=2)
  {
    dma_test_microcode[j] = DMA_MC_SKIP_1280_SOL;
    dma_test_microcode[j+1] = DMA_MC_SKIP_1280_EOL;
  }

  j=966;
  dma_test_microcode[j] = DMA_MC_SYNC_VRO_WORD_0_IRQ; j++;
  dma_test_microcode[j] = DMA_MC_SYNC_WORD_1; j++;

  dma_test_microcode[j] = DMA_MC_JUMP_TO_BEG; j++;
  dma_test_microcode[j] = (unsigned int)&(dma_test_microcode[0]); j++;

}


void initialize_frame_mc(int fsize)
{
  int mcidx, i,j, k;

  if(fsize==NTSC_80_X_60)
  {
    /* For NUMFRAMES Frames */
    for(i=0;i<NUMFRAMES;i++)
    {

      mcidx = PUNYMCSIZE * i; j = 0;

      /* FIELD SYNC */  
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 30 lines of ODD microcode */
      for(j=2,k=0;j<32;j+=2,k++)
      {
  
        /* odd line */
        if(replace_write_with_skip)
        {
          dma_puny_microcode[mcidx+j] = DMA_MC_SKIP_40_SOL;
          dma_puny_microcode[mcidx+j+1] = DMA_MC_SKIP_40_EOL;
        }
        else
        {
          dma_puny_microcode[mcidx+j] = DMA_MC_WRITE_80_LINE;
          dma_puny_microcode[mcidx+j+1] = (unsigned int)&(puny_frame_buffer[i][(20+(k*40))]);
        }
 
      }

      j=32;

      /* VRE FIELD SYNC */ 
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_VRE_WORD_0; j++;
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 30 lines of EVEN microcode */
      for(j=36,k=0;j<66;j+=2,k++)
      {

        /* even line */
        if(replace_write_with_skip)
        {
          dma_puny_microcode[mcidx+j] = DMA_MC_SKIP_40_SOL;
          dma_puny_microcode[mcidx+j+1] = DMA_MC_SKIP_40_EOL;
        }
        else
        {
          dma_microcode[mcidx+j] = DMA_MC_WRITE_80_LINE;
          dma_microcode[mcidx+j+1] = (unsigned int)&(puny_frame_buffer[i][0+(k*40)]);
        }
      }
 
      j = 66;
   
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_VRO_WORD_0_IRQ; j++;
      dma_puny_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

    }

    dma_puny_microcode[(NUMFRAMES*PUNYMCSIZE)] = DMA_MC_JUMP_TO_BEG;
    dma_puny_microcode[(NUMFRAMES*PUNYMCSIZE)+1] = (unsigned int)&(dma_microcode[0]);

  }

  else if(fsize==NTSC_320_X_240_GS)
  {
    /* For NUMFRAMES Frames */
    for(i=0;i<NUMFRAMES;i++)
    {

      mcidx = MCSIZE * i; j = 0;

      /* FIELD SYNC */  
      dma_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 120 lines of ODD microcode */
      for(j=2,k=0;j<242;j+=2,k++)
      {
  
        /* odd line */
        if(replace_write_with_skip)
        {
          dma_microcode[mcidx+j] = DMA_MC_SKIP_40_SOL;
          dma_microcode[mcidx+j+1] = DMA_MC_SKIP_40_EOL;
        }
        else
        {
          dma_microcode[mcidx+j] = DMA_MC_WRITE_80_LINE;
          dma_microcode[mcidx+j+1] = (unsigned int)&(y8_frame_buffer[i][(80+(k*160))]);
        }
 
      }

      j=242;

      /* VRE FIELD SYNC */ 
      dma_microcode[mcidx+j] = DMA_MC_SYNC_VRE_WORD_0; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 120 lines of EVEN microcode */
      for(j=246,k=0;j<486;j+=2,k++)
      {

        /* even line */
        if(replace_write_with_skip)
        {
          dma_microcode[mcidx+j] = DMA_MC_SKIP_40_SOL;
          dma_microcode[mcidx+j+1] = DMA_MC_SKIP_40_EOL;
        }
        else
        {
          dma_microcode[mcidx+j] = DMA_MC_WRITE_80_LINE;
          dma_microcode[mcidx+j+1] = (unsigned int)&(y8_frame_buffer[i][0+(k*160)]);
        }
      }
 
      j = 486;
   
      dma_microcode[mcidx+j] = DMA_MC_SYNC_VRO_WORD_0_IRQ; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

    }

    dma_microcode[(NUMFRAMES*MCSIZE)] = DMA_MC_JUMP_TO_BEG;
    dma_microcode[(NUMFRAMES*MCSIZE)+1] = (unsigned int)&(dma_microcode[0]);

  }

    /* For NUMFRAMES Frames */
  else if(fsize==NTSC_320_X_240)
  {
    /* For NUMFRAMES Frames */
    for(i=0;i<NUMFRAMES;i++)
    {

      mcidx = MCSIZE * i; j = 0;

      /* FIELD SYNC */  
      dma_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 120 lines of ODD microcode */
      for(j=2,k=0;j<242;j+=2,k++)
      {
  
        /* odd line */
        if(replace_write_with_skip)
        {
          dma_microcode[mcidx+j] = DMA_MC_SKIP_640_SOL;
          dma_microcode[mcidx+j+1] = DMA_MC_SKIP_640_EOL;
        }
        else
        {
          dma_microcode[mcidx+j] = DMA_MC_WRITE_1280_LINE;
          dma_microcode[mcidx+j+1] = (unsigned int)&(frame_buffer[i][(320+(k*640))]);
        }
 
      }

      j=242;

      /* VRE FIELD SYNC */ 
      dma_microcode[mcidx+j] = DMA_MC_SYNC_VRE_WORD_0; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 120 lines of EVEN microcode */
      for(j=246,k=0;j<486;j+=2,k++)
      {

        /* even line */
        if(replace_write_with_skip)
        {
          dma_microcode[mcidx+j] = DMA_MC_SKIP_640_SOL;
          dma_microcode[mcidx+j+1] = DMA_MC_SKIP_640_EOL;
        }
        else
        {
          dma_microcode[mcidx+j] = DMA_MC_WRITE_1280_LINE;
          dma_microcode[mcidx+j+1] = (unsigned int)&(frame_buffer[i][0+(k*640)]);
        }
      }
 
      j = 486;
   
      dma_microcode[mcidx+j] = DMA_MC_SYNC_VRO_WORD_0_IRQ; j++;
      dma_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

    }

    dma_microcode[(NUMFRAMES*MCSIZE)] = DMA_MC_JUMP_TO_BEG;
    dma_microcode[(NUMFRAMES*MCSIZE)+1] = (unsigned int)&(dma_microcode[0]);

  }

  else if(fsize==NTSC_640_X_480)
  {
    /* For NUMFRAMES Frames */
    for(i=0;i<NUMFRAMES;i++)
    {

      mcidx = LGMCSIZE * i; j = 0;

      /* FIELD SYNC */  
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 240 lines of ODD microcode */
      for(j=2,k=0;j<482;j+=2,k++)
      {

        /* odd line */
        dma_microcode[mcidx+j] = DMA_MC_SKIP_1280_SOL;
        dma_microcode[mcidx+j+1] = DMA_MC_SKIP_1280_EOL;
      }

      j=482;

      /* VRE FIELD SYNC */ 
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_VRE_WORD_0; j++;
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_FM1_WORD_0; j++;
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

      /* Initialize 240 lines of EVEN microcode */
      for(j=486;j<966;j+=2)
      {

        /* even line */
        dma_microcode[mcidx+j] = DMA_MC_SKIP_1280_SOL;
        dma_microcode[mcidx+j+1] = DMA_MC_SKIP_1280_EOL;
      }
 
      j = 966;
   
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_VRO_WORD_0_IRQ; j++;
      dma_large_microcode[mcidx+j] = DMA_MC_SYNC_WORD_1; j++;

    } /* end for NUMFRAMES */

    dma_large_microcode[(NUMFRAMES*LGMCSIZE)] = DMA_MC_JUMP_TO_BEG;
    dma_large_microcode[(NUMFRAMES*LGMCSIZE)+1] = (unsigned int)&(dma_microcode[0]);

  } /* end else 640x480 */

}



int configure_ntsc(int fsize)
{
  unsigned int testval = 0;


  /* Software Reset */
  PCI_WRITE(RESET_REG,0x0,0x00000001);


  /* Set the oscillator frequency for NTSC 

     0x00000020 = xtal 0 input
     0x00000000 = CLKx1

  */
  PCI_READ(TIMING_GEN_REG,0x0,&testval);
  printf("Timing Gen Ctl Reg = 0x%x\n", testval);

  PCI_WRITE(TIMING_GEN_REG,0x0,0x00000000);


  if((fsize==NTSC_320_X_240))
  {
    /*

      Set up the delay and active registers so that they cover
      full resolution.

      Scaled pixels / line = 323

      Crop 2 on left, 1 on right for 320 pixels / line

      For 320x240, want:

      MSB_CROP= 0x0000011

      Hactive=  0x00000140 (320)   Vactive= 0x000001e0 (480)
      Hdelay=   0x0000003c (60)    Vdelay=  0x00000016 (22)
      Hscale=   0x00001555         Vscale=  0x00007e00

    */

    PCI_WRITE(MSB_CROP_EVEN_REG,0x0,0x00000011);
    PCI_WRITE(MSB_CROP_ODD_REG,0x0,0x00000011);

    PCI_WRITE(HACTIVE_LO_EVEN_REG,0x0,0x00000040);
    PCI_WRITE(HACTIVE_LO_ODD_REG,0x0,0x00000040);
    PCI_WRITE(VACTIVE_LO_EVEN_REG,0x0,0x000000E0);
    PCI_WRITE(VACTIVE_LO_ODD_REG,0x0,0x000000E0);
    PCI_WRITE(VDELAY_LO_EVEN_REG,0x0,0x00000016);
    PCI_WRITE(VDELAY_LO_ODD_REG,0x0,0x00000016);
    PCI_WRITE(HDELAY_LO_EVEN_REG,0x0,0x0000003C);
    PCI_WRITE(HDELAY_LO_ODD_REG,0x0,0x0000003C);

    /* Set HSCALE for 320 pixels per line */
    PCI_WRITE(HSCALE_EVEN_MSB_REG,0x0,0x00000015);
    PCI_WRITE(HSCALE_ODD_MSB_REG,0x0,0x00000015);
    PCI_WRITE(HSCALE_EVEN_LSB_REG,0x0,0x00000055);
    PCI_WRITE(HSCALE_ODD_LSB_REG,0x0,0x00000055);

    /* Set VSCALE for 240 lines per frame */
    PCI_WRITE(VSCALE_EVEN_MSB_REG,0x0,0x0000007E);
    PCI_WRITE(VSCALE_ODD_MSB_REG,0x0,0x0000007E);
    PCI_WRITE(VSCALE_EVEN_LSB_REG,0x0,0x00000000);
    PCI_WRITE(VSCALE_ODD_LSB_REG,0x0,0x00000000);

    PCI_WRITE(COLOR_FORMAT_REG,0x0,0x00000000);
    acq_type = NTSC_320_X_240;

  }

  if((fsize==NTSC_320_X_240_GS))
  {
    /*

      Set up the delay and active registers so that they cover
      full resolution.

      Scaled pixels / line = 323

      Crop 2 on left, 1 on right for 320 pixels / line

      For 320x240, want:

      MSB_CROP= 0x0000011

      Hactive=  0x00000140 (320)   Vactive= 0x000001e0 (480)
      Hdelay=   0x0000003c (60)    Vdelay=  0x00000016 (22)
      Hscale=   0x00001555         Vscale=  0x00007e00

    */

    PCI_WRITE(MSB_CROP_EVEN_REG,0x0,0x00000011);
    PCI_WRITE(MSB_CROP_ODD_REG,0x0,0x00000011);

    PCI_WRITE(HACTIVE_LO_EVEN_REG,0x0,0x0000002C);
    PCI_WRITE(HACTIVE_LO_ODD_REG,0x0,0x0000002C);
    PCI_WRITE(VACTIVE_LO_EVEN_REG,0x0,0x000000E0);
    PCI_WRITE(VACTIVE_LO_ODD_REG,0x0,0x000000E0);
    PCI_WRITE(VDELAY_LO_EVEN_REG,0x0,0x00000016);
    PCI_WRITE(VDELAY_LO_ODD_REG,0x0,0x00000016);
    PCI_WRITE(HDELAY_LO_EVEN_REG,0x0,0x00000046);
    PCI_WRITE(HDELAY_LO_ODD_REG,0x0,0x00000046);

    /* Set HSCALE for 320 pixels per line */
    PCI_WRITE(HSCALE_EVEN_MSB_REG,0x0,0x00000015);
    PCI_WRITE(HSCALE_ODD_MSB_REG,0x0,0x00000015);
    PCI_WRITE(HSCALE_EVEN_LSB_REG,0x0,0x00000055);
    PCI_WRITE(HSCALE_ODD_LSB_REG,0x0,0x00000055);

    /* Set VSCALE for 240 lines per frame */
    PCI_WRITE(VSCALE_EVEN_MSB_REG,0x0,0x0000007E);
    PCI_WRITE(VSCALE_ODD_MSB_REG,0x0,0x0000007E);
    PCI_WRITE(VSCALE_EVEN_LSB_REG,0x0,0x00000000);
    PCI_WRITE(VSCALE_ODD_LSB_REG,0x0,0x00000000);

    PCI_WRITE(COLOR_FORMAT_REG,0x0,0x00000066);
    acq_type = NTSC_320_X_240_GS;
  }

  else if(fsize==NTSC_80_X_60)
  {
    /*

      Set up the delay and active registers so that they cover
      full resolution.

      Scaled pixels / line = 81

      Crop 0.79 on left for 80 pixels / line

      For 80X60, want:

      MSB_CROP= 0x0000000

      Hactive=  0x00000050 (80)   Vactive=  0x000001e0 (480)
      Hdelay=   0x00000010 (16)    Vdelay=  0x00000016 (22)
      Hscale=   0x0000861A         Vscale=  0x00007200

    */

    PCI_WRITE(MSB_CROP_EVEN_REG,0x0,0x00000010);
    PCI_WRITE(MSB_CROP_ODD_REG,0x0, 0x00000010);

    PCI_WRITE(HACTIVE_LO_EVEN_REG,0x0,0x00000050);
    PCI_WRITE(HACTIVE_LO_ODD_REG,0x0,0x00000050);
    PCI_WRITE(VACTIVE_LO_EVEN_REG,0x0,0x000000E0);
    PCI_WRITE(VACTIVE_LO_ODD_REG,0x0,0x000000E0);
    PCI_WRITE(VDELAY_LO_EVEN_REG,0x0,0x00000016);
    PCI_WRITE(VDELAY_LO_ODD_REG,0x0,0x00000016);
    PCI_WRITE(HDELAY_LO_EVEN_REG,0x0,0x00000010);
    PCI_WRITE(HDELAY_LO_ODD_REG,0x0,0x00000010);

    /* Set HSCALE for 320 pixels per line */
    PCI_WRITE(HSCALE_EVEN_MSB_REG,0x0,0x00000086);
    PCI_WRITE(HSCALE_ODD_MSB_REG,0x0,0x00000086);
    PCI_WRITE(HSCALE_EVEN_LSB_REG,0x0,0x0000001A);
    PCI_WRITE(HSCALE_ODD_LSB_REG,0x0,0x0000001A);

    /* Set VSCALE for 240 lines per frame */
    PCI_WRITE(VSCALE_EVEN_MSB_REG,0x0,0x00000072);
    PCI_WRITE(VSCALE_ODD_MSB_REG,0x0,0x00000072);
    PCI_WRITE(VSCALE_EVEN_LSB_REG,0x0,0x00000000);
    PCI_WRITE(VSCALE_ODD_LSB_REG,0x0,0x00000000);

    /* Set color format for Y8 GRAYSCALE on ODD and EVEN */
    PCI_WRITE(COLOR_FORMAT_REG,0x0,0x00000066);

  }

  else if(fsize==NTSC_640_X_480)
  {


    /*

      Set up the delay and active registers so that they cover
      full resolution.

      Scaled pixels / line = 646

      Crop 4 on left, 2 on right for 640 pixels / line

      For 640x480, want:

      MSB_CROP= 0x0000012

      Hactive= 0x00000280 (640)    Vactive= 0x000001e0 (480)
      Hdelay=  0x00000078 (120)    Vdelay=  0x00000016 (22)
      Hscale=  0x000002aa          Vscale=  0x00006000

    */

    PCI_WRITE(MSB_CROP_EVEN_REG,0x0,0x00000012);
    PCI_WRITE(MSB_CROP_ODD_REG,0x0,0x00000012);

    PCI_WRITE(HACTIVE_LO_EVEN_REG,0x0,0x00000080);
    PCI_WRITE(HACTIVE_LO_ODD_REG,0x0,0x00000080);
    PCI_WRITE(VACTIVE_LO_EVEN_REG,0x0,0x000000E0);
    PCI_WRITE(VACTIVE_LO_ODD_REG,0x0,0x000000E0);
    PCI_WRITE(VDELAY_LO_EVEN_REG,0x0,0x00000016);
    PCI_WRITE(VDELAY_LO_ODD_REG,0x0,0x00000016);
    PCI_WRITE(HDELAY_LO_EVEN_REG,0x0,0x00000078);
    PCI_WRITE(HDELAY_LO_ODD_REG,0x0,0x00000078);

    /* Set HSCALE for 640 pixels per line */
    PCI_WRITE(HSCALE_EVEN_MSB_REG,0x0,0x00000002);
    PCI_WRITE(HSCALE_ODD_MSB_REG,0x0,0x00000002);
    PCI_WRITE(HSCALE_EVEN_LSB_REG,0x0,0x000000AA);
    PCI_WRITE(HSCALE_ODD_LSB_REG,0x0,0x000000AA);

    /* Set VSCALE for 480 lines per frame */
    PCI_WRITE(VSCALE_EVEN_MSB_REG,0x0,0x00000060);
    PCI_WRITE(VSCALE_ODD_MSB_REG,0x0,0x00000060);
    PCI_WRITE(VSCALE_EVEN_LSB_REG,0x0,0x00000000);
    PCI_WRITE(VSCALE_ODD_LSB_REG,0x0,0x00000000);


    /* Set color format for RGB32 on ODD and EVEN */
    PCI_WRITE(COLOR_FORMAT_REG,0x0,0x00000000);
  }

  /* Enable the DMA RISC instruction IRQ */ 
  PCI_WRITE(INT_ENABLE_REG,0x0,(int_errors_to_check|RISCI_INT|VPRES_INT));

  /* Reduce frame rate from max of 30 frames/sec or 60 fields/sec */
  PCI_WRITE(TEMP_DECIMATION_REG,0x0,0x00000000);  /* reset */
  PCI_WRITE(TEMP_DECIMATION_REG,0x0,0x00000000);  /* set */

}


int decimate_frames(int count)
{
  unsigned int fcnt = count;

  if(count > 60 || count < 0)
    return -1;

  else
  {
    /* Reduce frame rate from max of 30 frames/sec */
    PCI_WRITE(TEMP_DECIMATION_REG,0x0,0x00000000);  /* reset */
    PCI_WRITE(TEMP_DECIMATION_REG,0x0,fcnt);  /* set */

    return count;
  }

}


void disable_capture(void)
{
  PCI_WRITE(CAPTURE_CTL_REG,0x0,0x00000000);
}


void enable_capture(void)
{
  vdfc_capture();
}


void set_brightness(int b)
{
  int bright;
  unsigned int hb;

  PCI_READ(BRIGHTNESS_REG,0x0,&hb);

  if(hb < 0x7f)
    bright = hb + 0x80;
  else
    bright = hb - 0x7f;

  printf("Brightness was %d\n", bright);

  if(b > 255)
    b = 255;
  else if(b < 0)
    b = 0;

  bright = b + 0x80;

  if(bright > 0xFF)
    bright = bright - 0xFF;

  hb = bright;

  PCI_WRITE(BRIGHTNESS_REG,0x0,hb);

}


void set_contrast(int c)
{
  int contrast;
  unsigned int hc;

  PCI_READ(CONTRAST_REG,0x0,&hc);

  if(hc < 0x7f)
    contrast = hc + 0x80;
  else
    contrast = hc - 0x7f;

  printf("Contrast was %d\n", contrast);

  if(c > 255)
    c = 255;
  else if(c < 0)
    c = 0;

  contrast = c + 0x80;

  if(contrast > 0xFF)
    contrast = contrast - 0xFF;

  hc = contrast;

  PCI_WRITE(CONTRAST_REG,0x0,hc);

}


void vdfc_capture(void)
{
  /* Set the video capture control -- ODD and EVEN through VFDC */
  PCI_WRITE(CAPTURE_CTL_REG,0x0,0x00000013);
}


void vbi_capture(void)
{

  /* Set the video capture control -- ODD and EVEN VBI 

     Given NTSC full resolution frame format of:

      9   VSYNC           1          9
     11   VBI            10         20    -- Vertical Blanking Interval
    242.5 ODD            21        263.5
      9   VSYNC         263.5      272.5
     11   VBI           272.5      283.5  -- Vertical Blanking Interval
    242.5 EVEN          283.5      525
    -------------------------------------
    525

    
   */

  /**** Enable VBI Capture ****/
  PCI_WRITE(CAPTURE_CTL_REG,0x0,0x0000001C);

 
  /**** Standard VBI ****/ 
  PCI_WRITE(VBI_PACKET_SIZE_LO_REG,0x0,0x0000000B);
  PCI_WRITE(VBI_PACKET_SIZE_HI_REG,0x0,0x00000000);

}


void load_test_mc(int mc)
{
  if(mc == TEST_MICROCODE)
  {
    initialize_test_mc();
    PCI_WRITE(DMA_RISC_START_ADDR_REG,0x0,(unsigned int)&(dma_test_microcode[0]));
  }
  else
  {
    printf("Bad test MC\n");
  }
}


void load_frame_mc(int fsize)
{
  initialize_frame_mc(fsize);

  if((fsize==NTSC_320_X_240) || (fsize==NTSC_320_X_240_GS))
    PCI_WRITE(DMA_RISC_START_ADDR_REG,0x0,(unsigned int)&(dma_microcode[0]));
  else if(fsize==NTSC_80_X_60)
    PCI_WRITE(DMA_RISC_START_ADDR_REG,0x0,(unsigned int)&(dma_puny_microcode[0]));
  else if(fsize==NTSC_640_X_480)
    PCI_WRITE(DMA_RISC_START_ADDR_REG,0x0,(unsigned int)&(dma_large_microcode[0]));
}


void sw_reset(void)
{
  PCI_WRITE(RESET_REG,0x0,0x00000001);
}


void toggle_fifo(int t)
{
  unsigned int testval;

  PCI_READ(DMA_CTL_REG,0x0,&testval);

  if(t)
  {
    /* Enable FIFO */
    testval = testval | 0x00000001;
  }
  else
  {
    /* Disable FIFO */
    testval = testval & 0xFFFFFFFE;
  }

  PCI_WRITE(DMA_CTL_REG,0x0,testval);
}


void toggle_dma(int t)
{
  unsigned int testval;

  PCI_READ(DMA_CTL_REG,0x0,&testval);

  if(t)
  {
    /* Enable DMA */
    testval = testval | 0x00000002;
  }
  else
  {
    /* Disable DMA */
    testval = testval & 0xFFFFFFFD;
  }

  PCI_WRITE(DMA_CTL_REG,0x0,testval);

}


void start_acq(void)
{
  /* Enable DMA and data FIFO 

     The packed trigger point is set to max.

   */
  PCI_WRITE(DMA_CTL_REG,0x0,0x0000000F);
}


void halt_acq(void)
{
  /* Disable DMA and data FIFO */
  PCI_WRITE(DMA_CTL_REG,0x0,0x00000000);
}


void report(void)
{
  logMsg("\n\n******** Frame = %d ********\n", frame_acq_cnt,0,0,0,0,0);
  intel_pci_status();
  test_status();
  logMsg("******** Error Status:\n",0,0,0,0,0,0);
  logMsg("last_dstatus = 0x%x\n", last_dstatus,0,0,0,0,0);
  logMsg("last_isr_status = 0x%x\n", last_isr_status,0,0,0,0,0);
  logMsg("total_dma_disabled_errs = %d\n", total_dma_disabled_errs,0,0,0,0,0);
  logMsg("total_sync_errs = %d\n", total_sync_errs,0,0,0,0,0);
  logMsg("total_abort_errs = %d\n", total_abort_errs,0,0,0,0,0);
  logMsg("total_dma_errs = %d\n", total_dma_errs,0,0,0,0,0);
  logMsg("total_parity_errs = %d\n", total_parity_errs,0,0,0,0,0);
  logMsg("total_bus_parity_errs = %d\n", total_bus_parity_errs,0,0,0,0,0);
  logMsg("total_fifo_overrun_errs = %d\n", total_fifo_overrun_errs,0,0,0,0,0);
  logMsg("total_fifo_resync_errs = %d\n", total_fifo_resync_errs,0,0,0,0,0);
  logMsg("total_bus_latency_errs = %d\n", total_bus_latency_errs,0,0,0,0,0);
  logMsg("total_risc_ints = %d\n", total_risc_ints,0,0,0,0,0);
  logMsg("total_ints = %d\n", total_ints,0,0,0,0,0);
  logMsg("total_write_tags = %d\n", total_write_tags,0,0,0,0,0);
  logMsg("total_skip_tags = %d\n", total_skip_tags,0,0,0,0,0);
  logMsg("total_jump_tags = %d\n", total_jump_tags,0,0,0,0,0);
  logMsg("total_sync_tags = %d\n", total_sync_tags,0,0,0,0,0);
  logMsg("******** Frame = %d ********\n", frame_acq_cnt,0,0,0,0,0);

}


void pci_inta_isr(int param)
{

  total_ints++;

  PCI_READ(INT_STATUS_REG,0x0,&last_isr_status);

  /* Clear the INTA interrupt status 

     Clear ALL RR bits:
     PCI_WRITE(INT_STATUS_REG,0x0,0x000FFB3F);

   */
  PCI_WRITE(INT_STATUS_REG,0x0,last_isr_status);


  if(last_isr_status & int_errors_to_check)
  {

    /* Did error halt the DMA? */
    if(last_isr_status & 0x08000000)
      total_dma_disabled_errs++;

    halt_acq();

    start_acq();


    if(last_isr_status & SCERR_INT)
    {
      total_sync_errs++;
      halt_acq();
      start_acq();
    }

    if(last_isr_status & OCERR_INT)
    {
      total_dma_errs++;
      halt_acq();
      start_acq();
    }

    if(last_isr_status & PABORT_INT)
      total_abort_errs++;

    if(last_isr_status & RIPERR_INT)
      total_parity_errs++;

    if(last_isr_status & PPERR_INT)
      total_bus_parity_errs++;

    if(last_isr_status & FDSR_INT)
      total_fifo_resync_errs++;

    if(last_isr_status & FTRGT_INT)
      total_fifo_overrun_errs++;

    if(last_isr_status & FBUS_INT)
      total_bus_latency_errs++;

  }

  
  else if(last_isr_status & RISCI_INT)
  {
    total_risc_ints++;

    if(last_isr_status & (DMA_MC_WRITE<<12))
      total_write_tags++;

    if(last_isr_status & (DMA_MC_SKIP<<12))
      total_skip_tags++;

    if(last_isr_status & (DMA_MC_JUMP<<12))
      total_jump_tags++;

    if(last_isr_status & (DMA_MC_SYNC<<12))
      total_sync_tags++;

    /* Bump the frame_acq_cnt */
    frame_acq_cnt++;

    current_frame = !current_frame;

    /* Notify client that frame is ready */
    semGive(frameRdy);

    /* Check the device status */
    PCI_READ(BTVID_MMIO_ADDR,0x0,&last_dstatus);
  }


}


void full_reset(void)
{

  halt_acq();

  disable_capture();

  taskDelete(btvid_tid);

  frame_acq_cnt = 0;
  frame_rdy_cnt = 0;
  last_dstatus = 0x0;
  last_isr_status = 0x0;
  total_dma_disabled_errs = 0x0;
  total_sync_errs = 0x0;
  total_abort_errs = 0x0;
  total_dma_errs = 0x0;
  total_parity_errs = 0x0;
  total_bus_parity_errs = 0x0;
  total_fifo_overrun_errs = 0x0;
  total_fifo_resync_errs = 0x0;
  total_bus_latency_errs = 0x0;
  total_risc_ints = 0x0;
  total_write_tags = 0x0;
  total_skip_tags = 0x0;
  total_jump_tags = 0x0;
  total_sync_tags = 0x0;

  semDelete(frameRdy);

  sw_reset();

  intel_pci_clear_status();

  reset_status();

}



void activate(int fsize)
{
  test_status();

  connect_pci_int(BT878INT);

  sysIntEnablePIC(BT878INT);

  configure_ntsc(fsize);
  logMsg("Configured NTSC\n",0,0,0,0,0,0);

  set_mux(1);
  logMsg("Set mux\n",0,0,0,0,0,0);

  load_frame_mc(fsize);
  printf("Loaded MC\n");
  clear_buffers(fsize);
  enable_capture();
  start_acq();

  test_status();
}


void activate_test(int mc)
{
  test_status();

  connect_pci_int(BT878INT);
  sysIntEnablePIC(BT878INT);

  configure_ntsc(1);

  set_mux(1);

  load_test_mc(mc);
  clear_buffers(2);
  enable_capture();
  start_acq();


  test_status();
}


void connect_pci_int(int inum)
{

  pciIntConnect((INUM_TO_IVEC (inum+INT_NUM_IRQ0)), (VOIDFUNCPTR)pci_inta_isr, 0);

}



void btvid_drvr(void)
{
  frameRdy = semBCreate(SEM_Q_FIFO, SEM_EMPTY);


  /******** Initialization and Test ********/
  intel_pci_config();
  btvid_controller_config();
  test_dstatus();
  test_status();


  /******** Main LOOP ********/
  while(1)
  {
    /* Await a activate command here */
    semTake(frameRdy, WAIT_FOREVER);
    frame_rdy_cnt++;
    
    if(acq_type == NTSC_320_X_240)
      bcopy(&(frame_buffer[current_frame][0]), save_buffer, (76800*4));

    if(acq_type == NTSC_320_X_240_GS)
      bcopy(&(y8_frame_buffer[current_frame][0]), y8_save_buffer, (19200*4));

  }

}


void start_video(void)
{
  btvid_tid = taskSpawn("tBtvid", 10, 0, (1024*8), (FUNCPTR) btvid_drvr, 0,0,0,0,0,0,0,0,0,0);

}


int reset_status(void)
{
  unsigned int testval = 0;

  /* Clear the interrupt and status */
  PCI_READ(INT_STATUS_REG,0x0,&testval);

  PCI_WRITE(INT_STATUS_REG,0x0,0xFFFFFFFF);

  test_status();
}


int test_status(void)
{
  unsigned int testval = 0;

  PCI_READ(INT_STATUS_REG,0x0,&testval);
  logMsg("mmio INTSTATUS testval = 0x%x\n", testval,0,0,0,0,0);

  if(testval & 0x02000000)
    logMsg("I2C RACK\n",0,0,0,0,0,0);

  if(testval & 0x00000100)
    logMsg("I2C DONE\n",0,0,0,0,0,0);

  if(testval & 0x00000800)
    logMsg("RISC INT BIT SET\n",0,0,0,0,0,0);

  if(testval & (DMA_MC_WRITE<<12))
    logMsg("DMA_MC_WRITE\n",0,0,0,0,0,0);

  if(testval & (DMA_MC_SKIP<<12))
    logMsg("DMA_MC_SKIP\n",0,0,0,0,0,0);

  if(testval & (DMA_MC_JUMP<<12))
    logMsg("DMA_MC_JUMP\n",0,0,0,0,0,0);

  if(testval & (DMA_MC_SYNC<<12))
    logMsg("DMA_MC_SYNC\n",0,0,0,0,0,0);

  if(testval & 0x08000000)
    logMsg("DMA ENABLED\n",0,0,0,0,0,0);
  else
    logMsg("DMA DISABLED\n",0,0,0,0,0,0);

  if(testval & 0x01000000)
    logMsg("EVEN FIELD\n",0,0,0,0,0,0);
  else
    logMsg("ODD FIELD\n",0,0,0,0,0,0);

  if(testval & 0x00000020)
    logMsg("VIDEO PRESENT CHANGE DETECTED\n",0,0,0,0,0,0);

  if(testval & 0x00000010)
    logMsg("HLOCK CHANGE DETECTED\n",0,0,0,0,0,0);

  if(testval & 0x00000008)
    logMsg("LUMA/CHROMA OVERFLOW DETECTED\n",0,0,0,0,0,0);

  if(testval & 0x00000001)
    logMsg("PAL/NTSC FORMAT CHANGE DETECTED\n",0,0,0,0,0,0);

  if(testval & 0x00080000)
    logMsg("**** SYNC ERROR ****\n",0,0,0,0,0,0);

  if(testval & 0x00040000)
    logMsg("**** DMA RISC ERROR ****\n",0,0,0,0,0,0);

  if(testval & 0x00020000)
    logMsg("**** MASTER/TARGET ABORT ****\n",0,0,0,0,0,0);

  if(testval & 0x00010000)
    logMsg("**** DATA PARITY ERROR ****\n",0,0,0,0,0,0);

  if(testval & 0x00008000)
    logMsg("**** PCI BUS PARITY ERROR ****\n",0,0,0,0,0,0);

  if(testval & 0x00004000)
    logMsg("**** FIFO DATA RESYNC ERROR ****\n",0,0,0,0,0,0);

  if(testval & 0x00002000)
    logMsg("**** FIFO OVERRUN ERROR ****\n",0,0,0,0,0,0);

  if(testval & 0x00001000)
    logMsg("**** BUS ACCESS LATENCY ERROR ****\n",0,0,0,0,0,0);

  PCI_READ(CAPTURE_CNT_REG,0x0,&testval);
  logMsg("mmio CAPTURE_CNT = 0x%x\n", testval,0,0,0,0,0,0);

  PCI_READ(DMA_RISC_PC_REG,0x0,&testval);
  logMsg("mmio DMA PC = 0x%x\n", testval,0,0,0,0,0,0);
}


int test_dmapc(void)
{
  unsigned int testval = 0;

  PCI_READ(DMA_RISC_PC_REG,0x0,&testval);
  printf("mmio DMA PC testval = 0x%x\n", testval);

}


void set_mux(int mux)
{
  unsigned int testval = 0x00000019; /* NTSC source */

  if(mux==0)
    testval |= 0x00000040;
  else if(mux==1)
    testval |= 0x00000060;
  else if(mux==2)
    testval |= 0x00000020;
  else if(mux==3)
    testval |= 0x00000000;
    
  /* Select NTSC source */
  PCI_WRITE(INPUT_REG,0x0,testval);

  printf("Setting INPUT_REG = 0x%x\n", testval);

}


int test_dstatus(void)
{
  unsigned int testval = 0;

  PCI_READ(BTVID_MMIO_ADDR,0x0,&testval);
  printf("mmio DSTATUS testval = 0x%x\n", testval);

  if(testval & 0x00000080)
    printf("**** VIDEO PRESENT\n");
  else
    printf("**** NO VIDEO\n");

  if(testval & 0x00000040)
    printf("**** HLOC ON\n");

  if(testval & 0x00000020)
    printf("**** DECODING EVEN FIELD\n");
  else
    printf("**** DECODING ODD FIELD\n");

  if(testval & 0x00000010)
    printf("**** 525 LINE NTSC FORMAT\n");

  if(testval & 0x00000004)
    printf("**** PLL OUT OF LOCK\n");

  if(testval & 0x00000002)
    printf("**** LUMA ADC OVERFLOW\n");

  if(testval & 0x00000001)
    printf("**** CHROMA ADC OVERFLOW\n");
}


int intel_pci_config(void)
{
  int pciBusNo = INTEL_NB_CTL_BUSNO;
  int pciDevNo = INTEL_NB_CTL_DEVNO;
  int pciFuncNo = INTEL_NB_CTL_FUNCNO;

  unsigned short int testval;
  unsigned int longword;
  unsigned char byte;

  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_PCI_CTL, &byte);
  printf("Intel NB controller PCI concurrency enable = 0x%x\n", byte);

  pciConfigOutByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_PCI_CTL, 0x00);
  printf("Modified Intel NB controller PCI concurrency enable = 0x%x\n", byte);

  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_TIMER, &byte);
  printf("Intel NB controller PCI latency timer = 0x%x\n", byte);

  pciConfigOutByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_TIMER, 0x00);
  printf("Modified Intel NB controller PCI latency timer = 0x%x\n", byte);

  /* Enable the North Bridge memory controller to allow memory access
     by another bus master.

     MAE=1

   */

  pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, &testval);
  printf("Intel NB controller PCI Cmd Reg = 0x%x\n", testval);

  pciConfigOutWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, 0x0002);

  pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, &testval);
  printf("Modified Intel NB controller PCI Cmd Reg = 0x%x\n", testval);

  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_ARB_CTL, &byte);
  printf("Intel NB controller PCI ARB CTL = 0x%x\n", byte);
  pciConfigOutByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_ARB_CTL, 0x80);
  printf("PCI 2.1 Compliant Intel NB controller PCI ARB CTL = 0x%x\n", byte);

  pciBusNo = INTEL_SB_CTL_BUSNO;
  pciDevNo = INTEL_SB_CTL_DEVNO;
  pciFuncNo = INTEL_SB_CTL_FUNCNO;

  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_CTL, &byte);
  printf("Intel SB controller latency control  = 0x%x\n", byte);
  byte |= 0x03;
  pciConfigOutByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_CTL, byte);
  printf("PCI 2.1 Compliant Intel SB controller latency control  = 0x%x\n", byte);

  pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING, &longword);
  printf("Intel SB controller IRQ Routing Reg = 0x%x\n", longword);

  longword = (0x00808080 | ((BT878INT)<<24));

  pciConfigOutLong(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING, longword);
  
  pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING, &longword);
  printf("Modified Intel SB controller IRQ Routing Reg = 0x%x\n", longword);

  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_APIC_ADDR, &byte);
  printf("Intel SB controller APIC Addr Reg = 0x%x\n", byte);


}


void intel_pci_clear_status(void)
{
  int pciBusNo = INTEL_NB_CTL_BUSNO;
  int pciDevNo = INTEL_NB_CTL_DEVNO;
  int pciFuncNo = INTEL_NB_CTL_FUNCNO;

  pciConfigOutWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_STATUS, 0x3000);

}


void intel_pci_status(void)
{
  int pciBusNo = INTEL_NB_CTL_BUSNO;
  int pciDevNo = INTEL_NB_CTL_DEVNO;
  int pciFuncNo = INTEL_NB_CTL_FUNCNO;
  unsigned short int testval;

  pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_STATUS, &testval);
  printf("Intel NB controller PCI Status Reg = 0x%x\n", testval);
 
  if(testval & 0x1000)
    printf("**** INTEL NB TARGET ABORT ****\n");
 
  if(testval & 0x2000)
    printf("**** INTEL NB MASTER ABORT ****\n");

}



int btvid_controller_config(void)
{
  int pciBusNo = BTVID_CTL_BUSNO;
  int pciDevNo = BTVID_CTL_DEVNO;
  int pciFuncNo = BTVID_CTL_FUNCNO;

  int ix;
  unsigned int testval;
  unsigned short command = (BUS_MASTER_MMIO);
  unsigned char irq;
  unsigned char byte;

  /* Disable btvid */
  pciConfigOutWord (pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, 0);


  for (ix = PCI_CFG_BASE_ADDRESS_0; ix <= PCI_CFG_BASE_ADDRESS_5; ix+=4)
  {
      pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, ix, &testval);

      printf("BAR %d testval=0x%x before any write\n", ((ix/4)-4), testval);

      /* Write all f's and read back value */

      pciConfigOutLong(pciBusNo, pciDevNo, pciFuncNo, ix, 0xffffffff);
      pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, ix, &testval);

      /* BAR implemented? */

      if (testval == 0)
      {
        printf("BAR %d not implemented\n", ((ix/4)-4));
      }

      /* I/O space requested? */

      /* Yes, set specified I/O space base address  */

      else if (testval & 0x1)
      {

        printf("BAR %d IO testval=0x%x\n", ((ix/4)-4), testval);

      }

      /* No, memory space required, set specified base address */

      else
      {

        printf("BAR %d MMIO testval=0x%x\n", ((ix/4)-4), testval);
        pciConfigOutLong(pciBusNo, pciDevNo, pciFuncNo, ix,
                         BTVID_MMIO_ADDR);

      }

  }


  /* Set the INTA vector */
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_DEV_INT_LINE, &irq);
  printf("Found Bt878 configured for IRQ %d\n", irq);

  /* Configure Cache Line Size Register -- Write-Command 

     Do not use cache line.

   */
  pciConfigOutByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_CACHE_LINE_SIZE, 0x0);

  /* Configure Latency Timer */
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_TIMER, &byte);
  printf("Bt878 Allowable PCI bus latency = 0x%x\n", byte);
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_MIN_GRANT, &byte);
  printf("Bt878 PCI bus min grant = 0x%x\n", byte);
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_MAX_LATENCY, &byte);
  printf("Bt878 PCI bus max latency = 0x%x\n", byte);

  pciConfigOutByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_TIMER, 0xFF);


  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_LATENCY_TIMER, &byte);
  printf("Modified Bt878 Allowable PCI bus latency = 0x%x\n", byte);


  /* Enable the device's capabilities as specified */
  pciConfigOutWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND,
                   (unsigned short)command);
  
}


int btvid_probe(void)
{
  int deviceNo;
  int devices;
  ushort_t vendorId;
  ushort_t deviceId;
  union
  {
    int classCode;
    char array[4];
  } u;
  int busNo = 0;



  if (pciLibInitStatus != OK)         /* sanity check */
  {
    if (pciConfigLibInit (PCI_MECHANISM_1, 0xCF8, 0xCFC, 0) != OK)
    {
      printf("PCI lib config error\n");
      return (ERROR);
    }
  }
  
  if (pciConfigMech == PCI_MECHANISM_1)
    devices = 0x1f;
  else
    devices = 0x0f;
 
  for (deviceNo=0; deviceNo < devices; deviceNo++)
  {
    pciConfigInWord (busNo, deviceNo, 0, PCI_CFG_VENDOR_ID, &vendorId);
    pciConfigInWord (busNo, deviceNo, 0, PCI_CFG_DEVICE_ID, &deviceId);
    pciConfigInByte (busNo, deviceNo, 0, PCI_CFG_PROGRAMMING_IF,
             &u.array[3]);
    pciConfigInByte (busNo, deviceNo, 0, PCI_CFG_SUBCLASS, &u.array[2]);
    pciConfigInByte (busNo, deviceNo, 0, PCI_CFG_CLASS, &u.array[1]);
    u.array[0] = 0;

    if (vendorId != 0xffff)
        printf ("%.8x  %.8x  %.8x  %.8x  %.8x  %.8x\n",
                busNo, deviceNo, 0, vendorId, deviceId, u.classCode);
    }


  return (OK);
  
}
