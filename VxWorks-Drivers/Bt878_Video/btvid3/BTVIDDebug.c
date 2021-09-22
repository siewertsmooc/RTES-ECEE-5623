/****************************************************************
*
* MODULE:		BT878 Debug functions
*
* DESCRITPION:		This module contains debugging functions related
* 			to the BT878 video driver.
*
* ORIGINAL AUTHOR:	Sam Siewert, University of Colorado, Boulder
* UPDATED BY:		Dan Walkes, University of Colorado, Boulder
* 
* CREATED:		Oct 28, 2004
*
* MODIFIED:		Feb 22,2004
*
* 
* NOTES:
*	This code contains functions to debug problems with the BT878 driver
*
* CODE USAGE:
*
*/
/* VxWorks API includes */
#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "ioLib.h"
#include "semLib.h"
#include "intLib.h"
#include "vmLib.h"   /* added to support */
#include "iv.h"


/* VxWorks 5.4 PCI driver interface includes */
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciConfigShow.h"
#include "drv/pci/pciHeaderDefs.h"
#include "drv/pci/pciLocalBus.h"
#include "drv/pci/pciIntLib.h"


/* pcPentium BSP includes */
#include "sysLib.h"


#include "btvid.h"

#define MAX_REG_NAME_LEN	10
#define MAX_REG_STAT_STRING_LEN	50

typedef struct regstatusbit
{
	unsigned char 	ucBit;
	unsigned char	abyRegName[MAX_REG_NAME_LEN];
	unsigned char	abyFalseString[MAX_REG_STAT_STRING_LEN];
	unsigned char 	abyTrueString[MAX_REG_STAT_STRING_LEN];
}REGSTATUSBIT;

REGSTATUSBIT	as878IntStatusReg[] =
{
	{27,	"RISC_EN",	"DMA Controller Disabled",	"DMA Controller Enabled"},
	{25,	"RACK",		"I2C Completed Operation",	0,},
	{24,	"FIELD",	"Odd field",			"Even Field"},
	{22,	"MYSTERY",	"Mystery Bit Clear","Mystery Bit Set"},
	{19,	"SCERR",	"",	"DMA EOL Sync Overflow !!!"},
	{18,	"OCERR",	"","Detected RISC Instruction Error"},
	{17,	"PABORT",	"","MASTER or TARGET Abort!!"},
	{16,	"RIPERR",	"","DMA STOPPED!! DATA PARITY ERROR!!!"},
	{15,	"PPERR",	"","PCI BUS PARITY ERROR DETECTED"},
	{14,	"FDSR",		"","FIFO Data Stream Resync"},
	{13,	"FTRGT",	"","PIXEL DATA FIFO OVERRUN"},
	{12,	"FBUS",		"","PIXEL OVERRUN DROPPING DWORDS"},
	{11,	"RISCI",	"IRQ Bit in RISC Instruction Cleared","IRQ Bit in RISC Instruction Set"},
	{9,	"GPINT",	"","PROG EDGE OR LEVEL OF GPINTR Occured"},
	{8,	"I2CDONE",	"","I2C Operation completed"},
	{5,	"VPRES",	"","VIDEO SIGNAL CHANGED PRES-ABS OR VISE VERSA"},
	{4,	"HLOCK",	"","Horizontal Lock Condition Changed"},
	{3,	"OFLOW",	"","Overflow detected on Luma or Chroma ADC"},
	{2,	"HSYNC",	"","New Video Line Started"},
	{1,	"VSYNC",	"","FIELD Changed on analog or GPIO Input"},
	{0,	"FMTCHG",	"","Video Format Changed"},
	{0,0,0,0}
};
		
/*-----------------------------------------------------------------
 * RegStatusString
 * Input:	char *pWriteBuf 	- a pointer to write the status into
 * 		unsigned int uiRegValue - the register value
 * 		unsigned int uiBit 	- the zero referenced bit value to check
 * 		char *pRegName		- the name of the register
 * 		char *pFalseString 	- The string to use if the mask bits were not set
 * 		char *pTrueString	- The string to use if the mask bits were set
 * 		
 * Output:	char * buffer into which data was written
 *
 * Notes:
*	Used to print out the status of a register based on a specific bitmask value
*/

char *RegStatusBitString(char *pWriteBuf,unsigned int uiRegValue,REGSTATUSBIT *psRegStatBit)
{
	if((uiRegValue & (Bit0 << (psRegStatBit->ucBit))) == (Bit0 << (psRegStatBit->ucBit)))
	{
		sprintf(pWriteBuf,"%d\t%s\t\t1\t%s",psRegStatBit->ucBit,psRegStatBit->abyRegName,psRegStatBit->abyTrueString);
	}
	else
	{
		sprintf(pWriteBuf,"%d\t%s\t\t0\t%s",psRegStatBit->ucBit,psRegStatBit->abyRegName,psRegStatBit->abyFalseString);
	}
	return pWriteBuf;
}	
	
/*-----------------------------------------------------------------
 * Decode878IntStatusReg
 * Input:	None
 * Output:	None
 *
 * Notes:
 * Decodes the interrupt startus register byte in the BT878 (at mmio location 
 * 0x100 See p 142 of the BT878 Chipset spec
 */
void Decode878IntStatusReg(void)
{
	unsigned int uiTemp;
	REGSTATUSBIT *psRegStatusBit;
	char abyTempBuf[200];
	
	PCI_READ(INT_STATUS_REG,0,&uiTemp); 
	printf("BT878 Interrupt Status Register Value 0x%x\n",uiTemp);

	printf("Decoded - \n");
	printf("\tBit#\tReg Name\tValue\tInterpretation\n");
	printf("\tRISCS bits = 0x%x\n",uiTemp >> 28);
	psRegStatusBit = &as878IntStatusReg[0];
	while(psRegStatusBit->abyRegName[0])	/* look for null termination in table */
	{
		printf("\t%s\n",RegStatusBitString(abyTempBuf,uiTemp,psRegStatusBit));
		psRegStatusBit++;
	}
	
}
	
void PrintInterruptMaskReg(void)
{
	unsigned int uiTemp;
	REGSTATUSBIT *psRegStatusBit;
	char abyTempBuf[200];

	PCI_READ(INT_ENABLE_REG,0,&uiTemp);
	printf("BT878 Interrupt Mask Register Value 0x%x\n",uiTemp);
	printf("\tRISCS bits = 0x%x\n",uiTemp >> 28);
	psRegStatusBit = &as878IntStatusReg[0];
	while(psRegStatusBit->abyRegName[0])	/* look for null termination in table */
	{
		printf("\t%s\n",RegStatusBitString(abyTempBuf,uiTemp,psRegStatusBit));
		psRegStatusBit++;
	}

}

/* Device Status Register *****
	See page 178 

*/
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

/* See PIRQX Register description P59 and P177 */
void printSBIRQRoutingReg(void)
{
	UINT32 	longword;

	pciConfigInLong(SOUTH_BRIDGE_PCIDEV.busNo, SOUTH_BRIDGE_PCIDEV.deviceNo, SOUTH_BRIDGE_PCIDEV.funcNo, PCI_CFG_IRQ_ROUTING, &longword);
	printf("Current Intel SB controller IRQ Routing Reg = 0x%x\n", longword);
}

void WriteSBIRQRoutingReg(unsigned long ulRouteReg)
{
	pciConfigOutLong(SOUTH_BRIDGE_PCIDEV.busNo, SOUTH_BRIDGE_PCIDEV.deviceNo, SOUTH_BRIDGE_PCIDEV.funcNo, PCI_CFG_IRQ_ROUTING, ulRouteReg);
}


int test_dmapc(void)
{
  unsigned int testval = 0;

  PCI_READ(DMA_RISC_PC_REG,0x0,&testval);
  printf("mmio DMA PC testval = 0x%x\n", testval);

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

void intel_pci_status(void)
{
  int pciBusNo = PCI_DEV[NORTH_BRIDGE].busNo;
  int pciDevNo = PCI_DEV[NORTH_BRIDGE].deviceNo;
  int pciFuncNo = PCI_DEV[NORTH_BRIDGE].funcNo;
  unsigned short int testval;

  pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_STATUS, &testval);
  printf("Intel NB controller PCI Status Reg = 0x%x\n", testval);
 
  if(testval & 0x1000)
    printf("**** INTEL NB TARGET ABORT ****\n");
 
  if(testval & 0x2000)
    printf("**** INTEL NB MASTER ABORT ****\n");

}

#define PCI_ROUTING_TABLE_SIG_BYTE0		0x24


#define PCI_ROUTING_TABLE_SIGNATURE		0x52495024
#define PCI_ROUTING_TABLE_SIG_BYTE0		0x24
#define PCI_ROUTING_TABLE_SIG_BYTE1		0x50
#define PCI_ROUTING_TABLE_SIG_BYTE2		0x49
#define PCI_ROUTING_TABLE_SIG_BYTE3		0x52

unsigned char WriteSBReg(unsigned char byReg, long byData)
{
	int pciBusNo = PCI_DEV[SOUTH_BRIDGE].busNo;
	int pciDevNo = PCI_DEV[SOUTH_BRIDGE].deviceNo;
	int pciFuncNo = PCI_DEV[SOUTH_BRIDGE].funcNo;
	int pciVenId = PCI_DEV[SOUTH_BRIDGE].venId;
	int pciDevId = PCI_DEV[SOUTH_BRIDGE].devId;

	pciConfigOutLong(pciBusNo, pciDevNo, pciFuncNo, byReg, byData);
}

unsigned long ReadSBReg(unsigned char byReg)
{
	int pciBusNo = PCI_DEV[SOUTH_BRIDGE].busNo;
	int pciDevNo = PCI_DEV[SOUTH_BRIDGE].deviceNo;
	int pciFuncNo = PCI_DEV[SOUTH_BRIDGE].funcNo;
	int pciVenId = PCI_DEV[SOUTH_BRIDGE].venId;
	int pciDevId = PCI_DEV[SOUTH_BRIDGE].devId;
	unsigned long ulResult;

	pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, byReg, &ulResult);
	return ulResult;

}

unsigned char ReadSBRegByte(unsigned char byReg)
{
	int pciBusNo = PCI_DEV[SOUTH_BRIDGE].busNo;
	int pciDevNo = PCI_DEV[SOUTH_BRIDGE].deviceNo;
	int pciFuncNo = PCI_DEV[SOUTH_BRIDGE].funcNo;
	int pciVenId = PCI_DEV[SOUTH_BRIDGE].venId;
	int pciDevId = PCI_DEV[SOUTH_BRIDGE].devId;
	unsigned char ucByte;
	
	pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, byReg, &ucByte);
	return ucByte;

}

unsigned char WriteNBReg(unsigned char byReg, long byData)
{
	int pciBusNo = PCI_DEV[NORTH_BRIDGE].busNo;
	int pciDevNo = PCI_DEV[NORTH_BRIDGE].deviceNo;
	int pciFuncNo = PCI_DEV[NORTH_BRIDGE].funcNo;
	int pciVenId = PCI_DEV[NORTH_BRIDGE].venId;
	int pciDevId = PCI_DEV[NORTH_BRIDGE].devId;

	pciConfigOutLong(pciBusNo, pciDevNo, pciFuncNo, byReg, byData);
}

unsigned long ReadNBReg(unsigned char byReg)
{
	int pciBusNo = PCI_DEV[NORTH_BRIDGE].busNo;
	int pciDevNo = PCI_DEV[NORTH_BRIDGE].deviceNo;
	int pciFuncNo = PCI_DEV[NORTH_BRIDGE].funcNo;
	int pciVenId = PCI_DEV[NORTH_BRIDGE].venId;
	int pciDevId = PCI_DEV[NORTH_BRIDGE].devId;
	unsigned long ulResult;

	pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, byReg, &ulResult);
	return ulResult;

}


void BitsSet(unsigned long ulBitCheckValue)
{
	unsigned long ulBit=Bit0;
	int i;

	for(i=0; i<32; i++)
	{
		if(ulBitCheckValue & ulBit)
		{
			printf("Bit %d Set\n",i);
		}
		ulBit = ulBit << 1;
	}
}

		
void ShowPCISlotData(PCI_SLOT_ENTRY_S *psPciSlot)
{
	int i;
	
	printf("PCI Bus Number: %d\n",psPciSlot->byBusNo);
	printf("PCI Device Number: %d\n",psPciSlot->byDevNo);

	for(i=0; i<4; i++)
	{
		printf("Interrupt %c Info ------\n",i+'A');
		printf("Link Value for INT%c: 0x%x\n",i+'A',psPciSlot->AtoDSlotInfo[i].byLinkValue);
		printf("IRQ Bitmap for INT%c: 0x%x\n",i+'A',psPciSlot->AtoDSlotInfo[i].wIRQBitmap);
	}

	printf("PCI Slot Number %d\n",psPciSlot->bySlotNo);
	
}	


			
void ShowPCIIntRoutingTable(PCI_ROUTING_TABLE_S *psRoutingTable)
{
	int iNumSlots;
	int i;
	
	printf("Routing Table Information:\n");
	printf("Location: Address - 0x%x\n",psRoutingTable);
	printf("Signature: 0x%4x\n",psRoutingTable->dwSignature);
	printf("Version: 0x%2x\n",psRoutingTable->wVersion);
	printf("Table Size: %d bytes\n",psRoutingTable->wTableSize);
	printf("PCI Int Router's Bus: %d\n",psRoutingTable->byIntRouterBus);
	printf("PCI Int Router's DevFunc: %x Decoded: Device %d Function %d\n",psRoutingTable->byIntRouterDevFunc,
										psRoutingTable->byIntRouterDevFunc>>3,
										psRoutingTable->byIntRouterDevFunc & (Bit0 | Bit1 | Bit2) );
	printf("PCI Exclusive IRQ's: %x\n",psRoutingTable->wExclusiveIRQs);
	printf("Compatable PCI Interrupt Router: %8x\n",psRoutingTable->dwCompatableIntRouterDevID);
	printf("Miniport Data: %d\n",psRoutingTable->dwIrqMiniport);
	
	iNumSlots = (psRoutingTable->wTableSize-32)/16;
	
	for(i=0; i<iNumSlots; i++)
	{
		printf("Slot %d Information -----------------\n",i+1);
		ShowPCISlotData(&psRoutingTable->sPciSlot[i]);
		printf("\n");
	}
}



unsigned char VerifyPCIIntRoutingTable(unsigned int *pIntRoutingTableAddr)
{
	PCI_ROUTING_TABLE_S *psRoutingTable;
	unsigned char *pbyCheckByte;
	int i;
	unsigned char checksum=0;

	psRoutingTable = (PCI_ROUTING_TABLE_S *)pIntRoutingTableAddr;

	if(psRoutingTable->wVersion == 0x100)	/* make sure this is version 1 */
	{
		if(psRoutingTable->wTableSize > 32 && !(psRoutingTable->wTableSize%16))
		{
			pbyCheckByte = (unsigned char *)pIntRoutingTableAddr;
			
			for(i=0; i<psRoutingTable->wTableSize; i++)
			{
				checksum += *pbyCheckByte;
				pbyCheckByte++;	/* move to the next address */
			}
			if(checksum == 0)
			{
				return TRUE;	/* this is a valid routing table */
			}
		}
	}

	return FALSE;	/* invalid routing table */
}


int findAndDecodePCIInterruptRoutingTable(void) {

	unsigned int *i;
	unsigned int entry = 0;	
	unsigned char bFound = 0;
	
	for(i = (unsigned int *) 0x000F0000; i < (unsigned int *) 0x100000; i += 4) {

		entry = *i;

		

		if (entry == PCI_ROUTING_TABLE_SIGNATURE) {
				

			if(VerifyPCIIntRoutingTable(i))
			{
				printf("Found tables signature 0x%x at 0x%x value = 0x%x \n", 
					PCI_ROUTING_TABLE_SIGNATURE,
					i,
					entry);

				ShowPCIIntRoutingTable(i);
				bFound = 1;
			}
		}

	}

	if(!bFound)
	{
		printf("Error - no configuration table found\n");
		return ERROR;
	}

	return OK;

}

			
/* -----------------------------------------------------------------------
	Parameters:	bDontInitPciDev - 	TRUE if no re-init of pci dev required
						FALSE if re-init of pci dev is required
	Returns 	

	Fills in IRQRouteStatusTable with status of southbridge and video card IRQ routing
 
*/

int decodePIRQxRouteControlRegister(/*unsigned char bDontInitPciDev*/ void) {
	

  int pciBusNo;
  int pciDevNo;
  int pciFuncNo;
  unsigned int longword;
  unsigned char interrupt_pin_A;
  unsigned char interrupt_pin_B;
  unsigned char interrupt_pin_C;
  unsigned char interrupt_pin_D;
  unsigned char ucVidCaptureIntPin;
  unsigned char byNewIntPin=0;
  
  initializePCI_DEV();
  pciBusNo = PCI_DEV[SOUTH_BRIDGE].busNo;
  pciDevNo = PCI_DEV[SOUTH_BRIDGE].deviceNo;
  pciFuncNo = PCI_DEV[SOUTH_BRIDGE].funcNo;

  pciConfigInLong(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING, &longword);
  printf("Intel SB controller IRQ Routing Reg = 0x%x\n", longword);
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_A, &interrupt_pin_A);
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_B, &interrupt_pin_B);
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_C, &interrupt_pin_C);
  pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_D, &interrupt_pin_D);

  printf("Intel SB controller PIRQx ROUTE CONTROL REGISTER PIN A = 0x%x \n", interrupt_pin_A);
  printf("Intel SB controller PIRQx ROUTE CONTROL REGISTER PIN B = 0x%x \n", interrupt_pin_B);
  printf("Intel SB controller PIRQx ROUTE CONTROL REGISTER PIN C = 0x%x \n", interrupt_pin_C);
  printf("Intel SB controller PIRQx ROUTE CONTROL REGISTER PIN D = 0x%x \n", interrupt_pin_D);

	if(PCI_DEV[CAPTURE_VIDEO_CARD1].Found == OK)
	{
		pciBusNo = PCI_DEV[CAPTURE_VIDEO_CARD1].busNo;
		pciDevNo = PCI_DEV[CAPTURE_VIDEO_CARD1].deviceNo;
		pciFuncNo = PCI_DEV[CAPTURE_VIDEO_CARD1].funcNo;
		pciConfigInByte(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_DEV_INT_PIN, &ucVidCaptureIntPin);
		/* see page 223 of Shanley */
		printf("Video Capture card using interrupt pin 0x%x or INT%c\n",ucVidCaptureIntPin,'A'+ucVidCaptureIntPin-1);
		/* note: this should always return IntA, see bt878 spec for details, p103 (pdf 115) */
		
		if(!PCI_CFG_IRQ_ROUTING_ENABLED(interrupt_pin_A))
		{
			printf("IRQ Routing for INTA not enabled on Southbridge.. this is a problem.\n");
			printf("Should be enabled for IRQ %d\n",BT878INT);
			pciConfigOutByte(SOUTH_BRIDGE_PCIDEV.busNo,SOUTH_BRIDGE_PCIDEV.deviceNo,SOUTH_BRIDGE_PCIDEV.funcNo,
					PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_A,BT878INT);
			
			pciConfigInByte(SOUTH_BRIDGE_PCIDEV.busNo,SOUTH_BRIDGE_PCIDEV.deviceNo,SOUTH_BRIDGE_PCIDEV.funcNo,
					PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_A,&interrupt_pin_A);

			printf("Fixed Southbridge INTA to map to IRQ %d\n",interrupt_pin_A);
		}
		else if(PCI_CFG_IRQ_LINE_TO_IRQ(interrupt_pin_A) != BT878INT)
		{
			printf("IRQ Routing for INTA routed to wrong IRQ on Southbridge..\n needs to be routed to IRQ%d,but is routed to IRQ%d\n",
				BT878INT,PCI_CFG_IRQ_LINE_TO_IRQ(interrupt_pin_A));
			
			pciConfigOutByte(SOUTH_BRIDGE_PCIDEV.busNo,SOUTH_BRIDGE_PCIDEV.deviceNo,SOUTH_BRIDGE_PCIDEV.funcNo,
					PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_A,BT878INT);

			pciConfigInByte(SOUTH_BRIDGE_PCIDEV.busNo,SOUTH_BRIDGE_PCIDEV.deviceNo,SOUTH_BRIDGE_PCIDEV.funcNo,
					PCI_CFG_IRQ_ROUTING_INTERRUPT_PIN_A,&interrupt_pin_A);

			printf("Fixed Southbridge INTA to map to IRQ %d\n",interrupt_pin_A);							
		}
	}
	 

  printf("More Descriptive:\n");
  
  if(PCI_CFG_IRQ_ROUTING_ENABLED(interrupt_pin_A))
	  printf("Interrupt pin A is enabled and routed to IRQ%i \n", PCI_CFG_IRQ_LINE_TO_IRQ(interrupt_pin_A));
  else
	  printf("Interrupt pin A routing has been disabled \n");
  
  if(PCI_CFG_IRQ_ROUTING_ENABLED(interrupt_pin_B))
	  printf("Interrupt pin B is enabled and routed to IRQ%i \n", PCI_CFG_IRQ_LINE_TO_IRQ(interrupt_pin_B));
  else
	  printf("Interrupt pin B routing has been disabled \n");


  if(PCI_CFG_IRQ_ROUTING_ENABLED(interrupt_pin_C))
	  printf("Interrupt pin C is enabled and routed to IRQ%i \n", PCI_CFG_IRQ_LINE_TO_IRQ(interrupt_pin_C));
  else
	  printf("Interrupt pin C routing has been disabled \n");


  if(PCI_CFG_IRQ_ROUTING_ENABLED(interrupt_pin_D))
	  printf("Interrupt pin D is enabled and routed to IRQ%i \n", PCI_CFG_IRQ_LINE_TO_IRQ(interrupt_pin_D));
  else
	  printf("Interrupt pin D routing has been disabled \n");

}

/* Only works for a specific target */
/*int dumpPCIFunctionsConfigurationHeader(void) {

	pciDeviceShow(0);
	printf("\n");
	pciHeaderShow(0, 0x00, 0);
	printf("\n");	
	pciHeaderShow(0, 0x07, 0);
	printf("\n");
	pciHeaderShow(0, 0x0b, 0);
	printf("\n");
	pciHeaderShow(0, 0x11, 0);

	return 0;

} */



int dumpPCIStatusAndCommand(void) {

	int pciBusNo = PCI_DEV[NORTH_BRIDGE].busNo;
  	int pciDevNo = PCI_DEV[NORTH_BRIDGE].deviceNo;
  	int pciFuncNo = PCI_DEV[NORTH_BRIDGE].funcNo;
	unsigned short command_reg, status_reg;

	logFdAdd(1);

  	pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, &command_reg);
	pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_STATUS, &status_reg);

	logMsg("Command Status NB: 0x%x 0x%x\n", command_reg, status_reg,0,0,0,0);

	pciBusNo = PCI_DEV[SOUTH_BRIDGE].busNo;
	pciDevNo = PCI_DEV[SOUTH_BRIDGE].deviceNo;
	pciFuncNo = PCI_DEV[SOUTH_BRIDGE].funcNo;

  	pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, &command_reg);
	pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_STATUS, &status_reg);

	logMsg("Command Status SB: 0x%x 0x%x\n", command_reg, status_reg,0,0,0,0);

	pciBusNo = PCI_DEV[CAPTURE_VIDEO_CARD1].busNo;
	pciDevNo = PCI_DEV[CAPTURE_VIDEO_CARD1].deviceNo;
	pciFuncNo = PCI_DEV[CAPTURE_VIDEO_CARD1].funcNo;

  	pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_COMMAND, &command_reg);
	pciConfigInWord(pciBusNo, pciDevNo, pciFuncNo, PCI_CFG_STATUS, &status_reg);

	logMsg("Command Status CP: 0x%x 0x%x\n", command_reg, status_reg,0,0,0,0);

	return 0;	

}


int pci_probe(void)
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
    devices = 0x1f;
    
  printf("Bus #, Device#, vendorID, deviceID, u.classCode\n");
 
	for(busNo=0; busNo<100; busNo++)
	{
		 
		for (deviceNo=0; deviceNo <= devices; deviceNo++)
		{
			pciConfigInWord (busNo, deviceNo, 0, PCI_CFG_VENDOR_ID, &vendorId);
			pciConfigInWord (busNo, deviceNo, 0, PCI_CFG_DEVICE_ID, &deviceId);
			pciConfigInByte (busNo, deviceNo, 0, PCI_CFG_PROGRAMMING_IF,
		             &u.array[3]);
			pciConfigInByte (busNo, deviceNo, 0, PCI_CFG_SUBCLASS, &u.array[2]);
			pciConfigInByte (busNo, deviceNo, 0, PCI_CFG_CLASS, &u.array[1]);
			    u.array[0] = 0;
	
			if (vendorId != 0xffff)
	 
				printf ("Bus:%.8x  Device:%.8x  %.8x  VenID:%.8x  DevID:%.8x  class:%.8x\n",
			             busNo, deviceNo, 0, vendorId, deviceId, u.classCode);
		}
	}

	  return (OK);
  
}

	

