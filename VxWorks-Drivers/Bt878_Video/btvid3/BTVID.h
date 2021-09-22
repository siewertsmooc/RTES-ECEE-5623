/****************************************************************
*
* MODULE:		BTVID.h.. a header file for the BTVID driver code
*
* DESCRITPION:		Connexant Bt878 NTSC Video Decoder VxWorks
*			5.4 PCI Driver and Video Processing Utilities
*
* ORIGINAL AUTHOR:		Sam Siewert, University of Colorado, Boulder
* UPDATED BY:			Zach Pfeffer, University of Colorado, Boulder
* 				Dan Walkes, University of Colorado, Boulder
* 
* CREATED:		May 7, 2000
* MODIFIED:		June 17, 2004
* 			Feb 22, 2005
* 
* NOTES:
*
*	This is the header file for the BTVID3.C file
*
*/

/* Function prototypes that can be exported to top-half code */

extern void	start_video(void);
extern int	btvid_controller_config(void);
extern void	connect_pci_int(int inum);
extern void	set_mux(int mux);
extern void	intel_pci_clear_status(void);
extern void	initialize_frame_mc(int fsize);
extern int	configure_ntsc(int fsize);
extern int	decimate_frames(int count);
extern void	disable_capture(void);
extern void	enable_capture(void);
extern void	vdfc_capture(void);
extern void	vbi_capture(void);        /* vertical blanking lines */
extern void	set_brightness(int b);
extern void	set_contrast(int c);
extern void	load_frame_mc(int fsize);
extern void	full_reset(void);


/* Function prototypes for bottom-half debug and development */

extern int	btvid_probe(void);
extern UINT	find_int_routing_table(void);
extern void	print_mc(int mc);
extern int	test_status(void);
extern void	intel_pci_status(void);
extern void	write_save_buffer(int bo);
extern void	write_y8_save_buffer(void);
extern UINT	check_buffers(int fsize);
extern void	clear_buffers(int fsize);
extern void	initialize_test_mc(void);
extern void	load_test_mc(int mc);


extern int	baseAddressRegisterSizeOfDecodeSpace(int);
extern int	findAndDecodePCIInterruptRoutingTable(void);
extern int	decodePIRQxRouteControlRegister(void);
extern int 	dumpPCIFunctionsConfigurationHeader(void);
extern int	dumpPCIStatusAndCommand(void);


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
/* #define BT878INT	0x09 */

/* PCI configuration space for Bt878 controller */

/*
#define BTVID_CTL_BUSNO		0x00000000
#define BTVID_CTL_DEVNO 	0x00000011
#define BTVID_CTL_FUNCNO 	0x00000000
#define BTVID_CTL_VENDORID 	0x0000109e
#define BTVID_CTL_DEVID 	0x0000036e
*/

/* PCI configuration space for Intel PCI and APIC Devices */
/*
#define PCI_DEVICE_ID_INTEL_82439TX	0x7100
#define PCI_DEVNO_INTEL_82439TX     	0x0
#define PCI_FUNCNO_INTEL_82439TX     	0x0 #define INTEL_NB_CTL_BUSNO 		0x00000000
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
*/

/* Config for the Intel 440FX PCI set 82441FX (North Bridge) System Controller 
 * and 82371SB (South Bridge) System Controller*/

#define INTEL_VENDOR_ID			0x8086

/* possible Northbridge values */
#define PCI_DEVICE_ID_INTEL_82441FX	0x1237
#define PCI_DEVICE_ID_INTEL_82439TX	0x7100
#define PCI_DEVICE_ID_INTEL_820820	0X2501


/* possible Southbridge values */
#define PCI_DEVICE_ID_INTEL_82371SB_0	0x7000
#define PCI_DEVICE_ID_INTEL_82371AB_0	0x7110
/* 82801AA LPC (Low Pin Count) interface is where PCI Interrupt registers are found.. this is Dev ID 0x2410 */
#define PCI_DEVICE_ID_INTEL_82801AA	0x2410 



/* Video capture card */
#define PCI_VENDOR_ID_BROOKTREE		0x109e
#define PCI_DEVICE_ID_BT878		0x036e


/* Northbridge */
#define PCI_CFG_PCI_CTL 		0x50

/* Southbridge */
#define PCI_CFG_IRQ_ROUTING		0x60
#define PCI_CFG_APIC_ADDR 		0x80
#define PCI_CFG_ARB_CTL 		0x4f
#define PCI_CFG_LATENCY_CTL 		0x82


	
/* 0x60-0x63 are pIRQRC[A:D] Route control, read write regs */
/* see p59 of southbridge doc */
#define PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_A		0x60
#define PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_B		0x61
#define PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_C		0x62
#define PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_D		0x63
#define PCI_CFG_GEN_CNTL				0xd0	/* see P201 82801AA datasheet */
#define PCI_CFG_GEN_CNTL_APIC_EN_BIT			Bit8
/* Operates on Bytes only */
#define PCI_CFG_IRQ_ROUTING_ENABLED(readback) 		!(readback >> 7)
#define PCI_CFG_IRQ_LINE_TO_IRQ(readback) 		(readback & 0x0F)



/* PCI command definitions */

#define BUS_MASTER_MMIO 0x0006


/* Bt878 MMIO registers - MMIO mapped through sysPhysMemDesc in
   the pcPentium BSP to enable MMU access to these registers.

   Please see Bt878 chipset documentation for full details.

 */

/*#define BTVID_MMIO_ADDR		0x08000000 */
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

#define Bit0 0x01
#define Bit1 0x02
#define Bit2 0x04
#define Bit3 0x08
#define Bit4 0x10
#define Bit5 0x20
#define Bit6 0x40
#define Bit7 0x80
#define Bit8 0x100
#define Bit9 0x200
#define Bit10 0x400
#define Bit11 0x800
#define Bit12 0x1000
#define Bit13 0x2000
#define Bit14 0x4000
#define Bit15 0x8000
#define Bit16 0x10000
#define Bit17 0x20000
#define Bit18 0x40000
#define Bit19 0x80000
#define Bit20 0x100000
#define Bit21 0x200000
#define Bit22 0x400000
#define Bit23 0x800000
#define Bit24 0x1000000
#define Bit25 0x2000000
#define Bit26 0x4000000
#define Bit27 0x8000000
#define Bit28 0x10000000
#define Bit29 0x20000000
#define Bit30 0x40000000
#define Bit31 0x80000000

extern STATUS  pciLibInitStatus;    /* initialization done */
extern int     pciConfigMech;       /* 1=mechanism-1, 2=mechanism-2 */
extern int     pciMaxBus;    /* Max number of sub-busses */


/* Bottom-half status variables */

extern UINT last_dstatus;
extern UINT last_isr_status;
extern UINT total_dma_disabled_errs;
extern UINT total_sync_errs;
extern UINT total_abort_errs;
extern UINT total_dma_errs;
extern UINT total_parity_errs;
extern UINT total_bus_parity_errs;
extern UINT total_fifo_overrun_errs;
extern UINT total_fifo_resync_errs;
extern UINT total_bus_latency_errs;
extern UINT total_risc_ints;
extern UINT total_ints;
extern UINT total_write_tags;
extern UINT total_skip_tags;
extern UINT total_jump_tags;
extern UINT total_sync_tags;



/* Bottom-half control-monitor variables */
extern int BT878INT; 	/* this will be set with the IRQ of the BT878 */


extern int	btvid_tid;
extern int	frame_acq_cnt;
extern int	current_frame;


typedef struct _PCI_DEVICE_BUS_NAME {
	int venId;
    	int devId;  /* device ID */
    	int index;     /* desired instance of device */
    	int busNo;    /* bus number */
    	int deviceNo; /* device number */
    	int funcNo;   /* function number */
	int Found;	/* OK if this device was found, otherwhise ERROR*/
} PCI_DEV_BUS_NAME;

typedef enum devices {
	NORTH_BRIDGE,
	SOUTH_BRIDGE,
	CAPTURE_VIDEO_CARD1,
	CAPTURE_VIDEO_CARD2,
	NUMDEVICES
}DEVICETYPE_EN;

/* a structure to hold device type definitions */
typedef struct devtype_s
{
	DEVICETYPE_EN	enDevType;
       	unsigned int	uiVenId;
	unsigned int	uiDevId;
	char 		*pDevDescString;
	unsigned char	bFound;
}DEVTYPE_S;

#define MAX_NUMDEVICES	10

extern PCI_DEV_BUS_NAME PCI_DEV[NUMDEVICES];
#define SOUTH_BRIDGE_PCIDEV PCI_DEV[SOUTH_BRIDGE]

/* mux values used with set_mux function call */
/* most cards with SVIDEO input use mux value 3 */
#define DEF_SVID_MUX_VAL		3
/* the "SUPERTV" card uses a mux value of 2 for its svideo input */
#define SUPERTV_SVID_INPUT_MUX_VAL	2
#define SUPERTV_COMPOSITE_INPUT_MUX_VAL	1

#define BYTE 	unsigned char
#define WORD 	unsigned short
#define DWORD	unsigned long

typedef struct pci_slot_int_entry_s
{
	BYTE	byLinkValue;
	WORD	wIRQBitmap;
}PCI_SLOT_INT_ENTRY_S;

typedef struct pci_slot_entry_s
{
	BYTE	byBusNo;
	BYTE	byDevNo;
	PCI_SLOT_INT_ENTRY_S	AtoDSlotInfo[4]; /* slot information for Int A-D */
	BYTE	bySlotNo;
	BYTE	byReserved;
}PCI_SLOT_ENTRY_S;

#define MAX_PCI_SLOTS	10

typedef struct pci_routing_table_s
{
	DWORD	dwSignature;
	WORD	wVersion;
	WORD	wTableSize;
	BYTE	byIntRouterBus;
	BYTE 	byIntRouterDevFunc;
	WORD	wExclusiveIRQs;
	DWORD	dwCompatableIntRouterDevID;
	DWORD	dwIrqMiniport;
	BYTE	abyReserved[11];
	BYTE	abyChecksum;
	PCI_SLOT_ENTRY_S	sPciSlot[MAX_PCI_SLOTS];	
}PCI_ROUTING_TABLE_S;

/* Bt878 and PCI interface globals */

/* Global function prototypes */
extern void start_video_report(void (*pFnNewFrameRcvd)(unsigned char *pFrameBuf));
extern void start_video(void);
extern int GetFrameAcqCount(void);
	
