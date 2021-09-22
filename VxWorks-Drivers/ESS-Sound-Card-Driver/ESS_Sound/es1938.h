/****************************************************************
*
* MODULE:		ES1938 Sound Card header file
*
* DESCRIPTION:	Driver for capture/playback functions on the ESS ES1938
*				chipset
*
* ORIGINAL AUTHOR: 	GNU ALSA Linux Source Code (see header below) 
*					Modified for use with VxWorks by Dan Walkes
* UPDATED BY:		
* 
* CREATED:		Oct, 2005
* MODIFIED:		
* 
* NOTES:
*
* CODE USAGE:
*
*
* REVISION HISTORY AND NOTES:
*
* Date			Update
* ---------------------------------------------------------------------
* Oct , 2005	Created.
*
* REFERENCES:
*
* 1) "DSSolo1.pdf" datasheet - ES1938 datasheet.
*
****************************************************************/

#ifndef ES1938_H
#define ES1938_H

#define REG_DEBUG	1	

#ifdef SUPPORT_JOYSTICK
	static inline void snd_es1938_free_gameport(es1938_t *chip) { }
#endif

#define SLIO_REG(chip, x) ((chip)->io_port + ESSIO_REG_##x)

#define SLDM_REG(chip, x) ((chip)->ddma_port + ESSDM_REG_##x)

#define SLSB_REG(chip, x) ((chip)->sb_port + ESSSB_REG_##x)

#define SL_PCI_LEGACYCONTROL		0x40
#define SL_PCI_CONFIG			0x50
#define SL_PCI_DDMACONTROL		0x60

#define ESSIO_REG_AUDIO2DMAADDR		0
#define ESSIO_REG_AUDIO2DMACOUNT	4
#define ESSIO_REG_AUDIO2MODE		6
#define ESSIO_REG_IRQCONTROL		7

#define ESSDM_REG_DMAADDR		0x00
#define ESSDM_REG_DMACOUNT		0x04
#define ESSDM_REG_DMACOMMAND		0x08
#define ESSDM_REG_DMASTATUS		0x08
#define ESSDM_REG_DMAMODE		0x0b
#define ESSDM_REG_DMACLEAR		0x0d
#define ESSDM_REG_DMAMASK		0x0f

#define ESSSB_REG_FMLOWADDR		0x00
#define ESSSB_REG_FMHIGHADDR		0x02
#define ESSSB_REG_MIXERADDR		0x04
#define ESSSB_REG_MIXERDATA		0x05

#define ESSSB_IREG_AUDIO1		0x14
#define ESSSB_IREG_MICMIX		0x1a
#define ESSSB_IREG_RECSRC		0x1c
#define ESSSB_IREG_MASTER		0x32
#define ESSSB_IREG_FM			0x36
#define ESSSB_IREG_AUXACD		0x38
#define ESSSB_IREG_AUXB			0x3a
#define ESSSB_IREG_PCSPEAKER		0x3c
#define ESSSB_IREG_LINE			0x3e
#define ESSSB_IREG_SPATCONTROL		0x50
#define ESSSB_IREG_SPATLEVEL		0x52
#define ESSSB_IREG_MASTER_LEFT		0x60
#define ESSSB_IREG_MASTER_RIGHT		0x62
#define ESSSB_IREG_MPU401CONTROL	0x64
#define ESSSB_IREG_MICMIXRECORD		0x68
#define ESSSB_IREG_AUDIO2RECORD		0x69
#define ESSSB_IREG_AUXACDRECORD		0x6a
#define ESSSB_IREG_FMRECORD		0x6b
#define ESSSB_IREG_AUXBRECORD		0x6c
#define ESSSB_IREG_MONO			0x6d
#define ESSSB_IREG_LINERECORD		0x6e
#define ESSSB_IREG_MONORECORD		0x6f
#define ESSSB_IREG_AUDIO2SAMPLE		0x70
#define ESSSB_IREG_AUDIO2MODE		0x71
#define ESSSB_IREG_AUDIO2FILTER		0x72
#define ESSSB_IREG_AUDIO2TCOUNTL	0x74
#define ESSSB_IREG_AUDIO2TCOUNTH	0x76
#define ESSSB_IREG_AUDIO2CONTROL1	0x78
#define ESSSB_IREG_AUDIO2CONTROL2	0x7a
#define ESSSB_IREG_AUDIO2		0x7c

#define ESSSB_REG_RESET			0x06

#define ESSSB_REG_READDATA		0x0a
#define ESSSB_REG_WRITEDATA		0x0c
#define ESSSB_REG_READSTATUS		0x0c

#define ESSSB_REG_STATUS		0x0e

#define ESS_CMD_EXTSAMPLERATE		0xa1
#define ESS_CMD_FILTERDIV		0xa2
#define ESS_CMD_DMACNTRELOADL		0xa4
#define ESS_CMD_DMACNTRELOADH		0xa5
#define ESS_CMD_ANALOGCONTROL		0xa8
#define ESS_CMD_IRQCONTROL		0xb1
#define ESS_CMD_DRQCONTROL		0xb2
#define ESS_CMD_RECLEVEL		0xb4
#define ESS_CMD_SETFORMAT		0xb6
#define ESS_CMD_SETFORMAT2		0xb7
#define ESS_CMD_DMACONTROL		0xb8
#define ESS_CMD_DMATYPE			0xb9
#define ESS_CMD_OFFSETLEFT		0xba	
#define ESS_CMD_OFFSETRIGHT		0xbb
#define ESS_CMD_READREG			0xc0
#define ESS_CMD_ENABLEEXT		0xc6
#define ESS_CMD_PAUSEDMA		0xd0
#define ESS_CMD_ENABLEAUDIO1		0xd1
#define ESS_CMD_STOPAUDIO1		0xd3
#define ESS_CMD_AUDIO1STATUS		0xd8
#define ESS_CMD_CONTDMA			0xd4
#define ESS_CMD_TESTIRQ			0xf2

#define ESS_RECSRC_MIC		0
#define ESS_RECSRC_AUXACD	2
#define ESS_RECSRC_AUXB		5
#define ESS_RECSRC_LINE		6
#define ESS_RECSRC_NONE		7

#define DAC1 0x01
#define ADC1 0x02
#define DAC2 0x04

#define RESET_LOOP_TIMEOUT	0x10000
#define WRITE_LOOP_TIMEOUT	0x10000
#define GET_LOOP_TIMEOUT	0x01000

extern struct pci_driver ES1938_driver;

#endif