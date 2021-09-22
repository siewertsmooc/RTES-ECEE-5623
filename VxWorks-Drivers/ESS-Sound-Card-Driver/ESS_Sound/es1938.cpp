/****************************************************************
*
* MODULE:		ES1938 Sound Card Driver file
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
* This file was modified as little as possible to make future porting of 
* ALSA code to VxWorks as easy as possible.  I also attempted to keep as much code
* as possible resident in the file so it could be referenced if any issues arise with
* operation in the future.  In many cases #if VX_UNUSED compile options were placed
* around code sections which I did not plan on including but may possibly be included
* in future revisions.
*
* CODE USAGE:
*
* This file requires files VxSound.cpp and VxPCI.cpp. See documentation
* in these modules for more information.  Playback has been tested with
* the vx_tone_ap.cpp source file which generates a continuous tone at 
* a specified frequency.  Capture has not yet been tested.
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
* 2) ALSA Linux driver page at http://www.alsa-project.org/alsa-doc/alsa-lib/index.html
*    
* 3) "vxWALSA Sound Driver" document included with this release.
*
****************************************************************/

/* Header below is the original header included with the es1938.cpp file 
	found in the ALSA library.  Functions were modified for use with VxWorks */
/*
 *  Driver for ESS Solo-1 (ES1938, ES1946, ES1969) soundcard
 *  Copyright (c) by Jaromir Koutek <miri@punknet.cz>,
 *                   Jaroslav Kysela <perex@suse.cz>,
 *                   Thomas Sailer <sailer@ife.ee.ethz.ch>,
 *                   Abramo Bagnara <abramo@alsa-project.org>,
 *                   Markus Gruber <gruber@eikon.tum.de>
 * 
 * Rewritten from sonicvibes.c source.
 *
 *  TODO:
 *    Rewrite better spinlocks
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*
  NOTES:
  - Capture data is written unaligned starting from dma_base + 1 so I need to
    disable mmap and to add a copy callback.
  - After several cycle of the following:
    while : ; do arecord -d1 -f cd -t raw | aplay -f cd ; done
    a "playback write error (DMA or IRQ trouble?)" may happen.
    This is due to playback interrupts not generated.
    I suspect a timing issue.
  - Sometimes the interrupt handler is invoked wrongly during playback.
    This generates some harmless "Unexpected hw_pointer: wrong interrupt
    acknowledge".
    I've seen that using small period sizes.
    Reproducible with:
    mpg123 test.mp3 &
    hdparm -t -T /dev/hda
*/

#include "VxWorks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "VxTypes.h"
#include "VxSound.h"
#include "VxPCI.h"
#include "es1938.h"


typedef struct _snd_es1938 es1938_t;
es1938_t *pGlobChip;


#define SAVED_REG_SIZE	32 /* max. number of registers to save */

struct _snd_es1938 {
	int irq;

	unsigned long io_port;
	unsigned long sb_port;
	unsigned long vc_port;
	unsigned long mpu_port;
	unsigned long game_port;
	unsigned long ddma_port;

	unsigned char irqmask;
	unsigned char revision;

	snd_kcontrol_t *hw_volume;
	snd_kcontrol_t *hw_switch;
	snd_kcontrol_t *master_volume;
	snd_kcontrol_t *master_switch;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_substream_t *capture_substream;
	snd_pcm_substream_t *playback1_substream;
	snd_pcm_substream_t *playback2_substream;

#if VXWORKS_UNUSED
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;
	spinlock_t reg_lock;
	spinlock_t mixer_lock;
        snd_info_entry_t *proc_entry;

#ifdef SUPPORT_JOYSTICK
	struct gameport *gameport;
#endif
#ifdef CONFIG_PM
	unsigned char saved_regs[SAVED_REG_SIZE];
#endif

#endif
	unsigned int dma1_size;
	unsigned int dma2_size;
	unsigned int dma1_start;
	unsigned int dma2_start;
	unsigned int dma1_shift;
	unsigned int dma2_shift;
	unsigned int active;

};


#ifndef PCI_VENDOR_ID_ESS
#define PCI_VENDOR_ID_ESS		0x125d
#endif
#ifndef PCI_DEVICE_ID_ESS_ES1938
#define PCI_DEVICE_ID_ESS_ES1938	0x1969
#endif


static const struct pci_device_id snd_es1938_ids[] = {
        { 0x125d, 0x1969, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0, },   /* Solo-1 */
	{ 0, }
};

#define SNDRV_DEFAULT_IDX	{ -1, -1, -1, -1, -1, -1, -1, -1 }
#define SNDRV_DEFAULT_STR	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
#define SNDRV_DEFAULT_ENABLE	{ 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
#define SNDRV_DEFAULT_ENABLE_PNP { 1, 1, 1, 1, 1, 1, 1, 1 }


/* for support of multiple cards */
static int a_index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;	/* Enable this card */


static void snd_es1938_reset_fifo(es1938_t *chip);
static void snd_es1938_rate_set(es1938_t *chip, 
				snd_pcm_substream_t *substream,
				int mode);
static void snd_es1938_mixer_write(es1938_t *chip, unsigned char reg, unsigned char val);
static void snd_es1938_playback1_setdma(es1938_t *chip);
static void snd_es1938_write(es1938_t *chip, unsigned char reg, unsigned char val);
static int snd_es1938_bits(es1938_t *chip, unsigned char reg, unsigned char mask, unsigned char val);
static int snd_es1938_mixer_bits(es1938_t *chip, unsigned char reg, unsigned char mask, unsigned char val);
static int snd_es1938_set_volume(snd_pcm_substream_t * substream, uint8 value );
static void snd_es1938_reset(es1938_t *chip);


/* --------------------------------------------------------------------
 * Interrupt handler
 * -------------------------------------------------------------------- */
static irqreturn_t snd_es1938_interrupt( int dev_id )
{
	es1938_t *chip = (es1938_t *)dev_id;
	unsigned char status, audiostatus;

	status = inb(SLIO_REG(chip, IRQCONTROL));
	printk("Es1938debug - interrupt status: =0x%x\n", status);
	
	/* AUDIO 1 */
	if (status & 0x10) {
#if 0
                printk("Es1938debug - AUDIO channel 1 interrupt\n");
		printk("Es1938debug - AUDIO channel 1 DMAC DMA count: %u\n", inw(SLDM_REG(chip, DMACOUNT)));
		printk("Es1938debug - AUDIO channel 1 DMAC DMA base: %u\n", inl(SLDM_REG(chip, DMAADDR)));
		printk("Es1938debug - AUDIO channel 1 DMAC DMA status: 0x%x\n", inl(SLDM_REG(chip, DMASTATUS)));
#endif
		/* clear irq */
		audiostatus = inb(SLSB_REG(chip, STATUS));
		if (chip->active & ADC1)
			snd_pcm_period_elapsed(chip->capture_substream);
		else if (chip->active & DAC1)
			snd_pcm_period_elapsed(chip->playback2_substream);
	}
	
	/* AUDIO 2 */
	if (status & 0x20) {
#if 0
                printk("Es1938debug - AUDIO channel 2 interrupt\n");
		printk("Es1938debug - AUDIO channel 2 DMAC DMA count: %u\n", inw(SLIO_REG(chip, AUDIO2DMACOUNT)));
		printk("Es1938debug - AUDIO channel 2 DMAC DMA base: %u\n", inl(SLIO_REG(chip, AUDIO2DMAADDR)));

#endif
		/* clear irq */
		snd_es1938_mixer_bits(chip, ESSSB_IREG_AUDIO2CONTROL2, 0x80, 0);
		if (chip->active & DAC2)
			snd_pcm_period_elapsed(chip->playback1_substream);
	}


#if ADD_LATER_INTERRUPT	/* add the volume and MPU401 handler later */
	(int irq, void *dev_id, struct pt_regs *regs)	/* need to set these.. are passed values in linux implementation */

/* Hardware volume */
	if (status & 0x40) {
		int split = snd_es1938_mixer_read(chip, 0x64) & 0x80;
		handled = 1;
		snd_ctl_notify(chip->card, SNDRV_CTL_EVENT_MASK_VALUE, &chip->hw_switch->id);
		snd_ctl_notify(chip->card, SNDRV_CTL_EVENT_MASK_VALUE, &chip->hw_volume->id);
		if (!split) {
			snd_ctl_notify(chip->card, SNDRV_CTL_EVENT_MASK_VALUE, &chip->master_switch->id);
			snd_ctl_notify(chip->card, SNDRV_CTL_EVENT_MASK_VALUE, &chip->master_volume->id);
		}
		/* ack interrupt */
		snd_es1938_mixer_write(chip, 0x66, 0x00);
	}

	/* MPU401 */
	if (status & 0x80) {
		// the following line is evil! It switches off MIDI interrupt handling after the first interrupt received.
		// replacing the last 0 by 0x40 works for ESS-Solo1, but just doing nothing works as well!
		// andreas@flying-snail.de
		// snd_es1938_mixer_bits(chip, ESSSB_IREG_MPU401CONTROL, 0x40, 0); /* ack? */
		if (chip->rmidi) {
			handled = 1;
			snd_mpu401_uart_interrupt(irq, chip->rmidi->private_data, regs);
		}
	}
#else
	if (status & 0x40) {
		snd_es1938_mixer_write(chip, 0x66, 0x00);
	}
#endif

	return IRQ_RETVAL(handled);
}

/* ------------------------------------------------------------------------------
 * Second Audio channel DAC Operation
 * ------------------------------------------------------------------------------*/
static int snd_es1938_playback1_prepare(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	int u, is8, mono;
	unsigned int size = snd_pcm_lib_buffer_bytes(substream);
	unsigned int count = snd_pcm_lib_period_bytes(substream);

	chip->dma2_size = size;
	chip->dma2_start = runtime->dma_addr;

	mono = (runtime->channels > 1) ? 0 : 1;
	is8 = snd_pcm_format_width(runtime->format) == 16 ? 0 : 1;
	u = snd_pcm_format_unsigned(runtime->format);

	chip->dma2_shift = 2 - mono - is8;

        snd_es1938_reset_fifo(chip);

	/* set clock and counters */
        snd_es1938_rate_set(chip, substream, DAC2);

	count >>= 1;
	count = 0x10000 - count;
	snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2TCOUNTL, count & 0xff);
	snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2TCOUNTH, count >> 8);

	/* initialize and configure Audio 2 DAC */
	snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2CONTROL2, 0x40 | (u ? 0 : 4) | (mono ? 0 : 2) | (is8 ? 0 : 1));

	/* program DMA */
	snd_es1938_playback1_setdma(chip);
	
	return 0;
}

/* --------------------------------------------------------------------
 * Reset the FIFOs
 * --------------------------------------------------------------------*/
static void snd_es1938_reset_fifo(es1938_t *chip)
{
	outb(2, SLSB_REG(chip, RESET));
	outb(0, SLSB_REG(chip, RESET));
}


static ratnum_t clocks[2] = {
	{
		/*.num =*/ 793800,
		/*.den_min = */ 1,
		/*.den_max = */ 128,
		/*.den_step = */ 1,
	},
	{
		/*.num = */ 768000,
		/*.den_min =*/ 1,
		/*.den_max =*/ 128,
		/*.den_step =*/ 1,
	}
};

static snd_pcm_hw_constraint_ratnums_t hw_constraints_clocks = {
	/*.nrats =*/ 2,
	/*.rats = */ clocks,
};


static void snd_es1938_rate_set(es1938_t *chip, 
				snd_pcm_substream_t *substream,
				int mode)
{
	unsigned int bits, div0;
	snd_pcm_runtime_t *runtime = substream->runtime;
	if (runtime->rate_num == clocks[0].num)
		bits = 128 - runtime->rate_den;
	else
		bits = 256 - runtime->rate_den;

	/* set filter register */
	div0 = 256 - 7160000*20/(8*82*runtime->rate);
		
	if (mode == DAC2) {
		snd_es1938_mixer_write(chip, 0x70, bits);
		snd_es1938_mixer_write(chip, 0x72, div0);
	} else {
		snd_es1938_write(chip, 0xA1, bits);
		snd_es1938_write(chip, 0xA2, div0);
	}
}

static void snd_es1938_playback1_setdma(es1938_t *chip)
{
	outb(0x00, SLIO_REG(chip, AUDIO2MODE));
	outl(chip->dma2_start, SLIO_REG(chip, AUDIO2DMAADDR));
	outw(0, SLIO_REG(chip, AUDIO2DMACOUNT));
	outw(chip->dma2_size, SLIO_REG(chip, AUDIO2DMACOUNT));
}

static void snd_es1938_playback2_setdma(es1938_t *chip)
{
	/* Enable DMA controller */
	outb(0xc4, SLDM_REG(chip, DMACOMMAND));
	/* 1. Master reset */
	outb(0, SLDM_REG(chip, DMACLEAR));
	/* 2. Mask DMA */
	outb(1, SLDM_REG(chip, DMAMASK));
	outb(0x18, SLDM_REG(chip, DMAMODE));
	outl(chip->dma1_start, SLDM_REG(chip, DMAADDR));
	outw(chip->dma1_size - 1, SLDM_REG(chip, DMACOUNT));
	/* 3. Unmask DMA */
	outb(0, SLDM_REG(chip, DMAMASK));
}

static void snd_es1938_capture_setdma(es1938_t *chip)
{
	/* Enable DMA controller */
	outb(0xc4, SLDM_REG(chip, DMACOMMAND));
	/* 1. Master reset */
	outb(0, SLDM_REG(chip, DMACLEAR));
	/* 2. Mask DMA */
	outb(1, SLDM_REG(chip, DMAMASK));
	outb(0x14, SLDM_REG(chip, DMAMODE));
	outl(chip->dma1_start, SLDM_REG(chip, DMAADDR));
	outw(chip->dma1_size - 1, SLDM_REG(chip, DMACOUNT));
	/* 3. Unmask DMA */
	outb(0, SLDM_REG(chip, DMAMASK));
}


static int snd_es1938_playback2_prepare(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	int u, is8, mono;
	unsigned int size = snd_pcm_lib_buffer_bytes(substream);
	unsigned int count = snd_pcm_lib_period_bytes(substream);

	chip->dma1_size = size;
	chip->dma1_start = runtime->dma_addr;

	mono = (runtime->channels > 1) ? 0 : 1;
	is8 = snd_pcm_format_width(runtime->format) == 16 ? 0 : 1;
	u = snd_pcm_format_unsigned(runtime->format);

	chip->dma1_shift = 2 - mono - is8;

	count = 0x10000 - count;
 
	/* reset */
	snd_es1938_reset_fifo(chip);
	
	snd_es1938_bits(chip, ESS_CMD_ANALOGCONTROL, 0x03, (mono ? 2 : 1));

	/* set clock and counters */
        snd_es1938_rate_set(chip, substream, DAC1);
	snd_es1938_write(chip, ESS_CMD_DMACNTRELOADL, count & 0xff);
	snd_es1938_write(chip, ESS_CMD_DMACNTRELOADH, count >> 8);

	/* initialized and configure DAC */
        snd_es1938_write(chip, ESS_CMD_SETFORMAT, u ? 0x80 : 0x00);
        snd_es1938_write(chip, ESS_CMD_SETFORMAT, u ? 0x51 : 0x71);
        snd_es1938_write(chip, ESS_CMD_SETFORMAT2, 
			 0x90 | (mono ? 0x40 : 0x08) |
			 (is8 ? 0x00 : 0x04) | (u ? 0x00 : 0x20));

	/* program DMA */
	snd_es1938_playback2_setdma(chip);
	
	return 0;
}


static int snd_es1938_playback_prepare(snd_pcm_substream_t *substream)
{
	switch (substream->number) {
	case 0:
		return snd_es1938_playback1_prepare(substream);
	case 1:
		return snd_es1938_playback2_prepare(substream);
	}
	snd_BUG();
	return -EINVAL;
}


/* -----------------------------------------------------------------------
 * Audio2 Playback (DAC)
 * -----------------------------------------------------------------------*/
static snd_pcm_hardware_t snd_es1938_playback =
{
#if VXWORKS_COMPILER_CANT_HANDLE
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U16_LE,
	.rates =		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	.rate_min =		6000,
	.rate_max =		48000,
	.channels_min =		1,
	.channels_max =		2,
        .buffer_bytes_max =	0x8000,       /* DMA controller screws on higher values */
	.period_bytes_min =	64,
	.period_bytes_max =	0x8000,
	.periods_min =		1,
	.periods_max =		1024,
	.fifo_size =		256,
#else
	/*.info = */(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
	 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP_VALID),
	/* .formats = */ SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U16_LE,
	/* .rates = */ SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	/*.rate_min =  */6000,	
	/*.rate_max =  */		48000,
	/*.channels_min = */		1,
	/*.channels_max = */		2,
    /* .buffer_bytes_max = */	0x8000,       /* DMA controller screws on higher values */
	/* .period_bytes_min = */	64,
	/* .period_bytes_max =	*/	0x8000,
	/*	.periods_min =		*/	1,
	/*.periods_max =	  */	1024,
	/*.fifo_size =	 */			256,
#endif
};

static int snd_es1938_playback_open(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;

	switch (substream->number) {
	case 0:
		chip->playback1_substream = substream;
		break;
	case 1:
		if (chip->capture_substream)
			return -EAGAIN;
		chip->playback2_substream = substream;
		break;
	default:
		snd_BUG();
		return -EINVAL;
	}
	runtime->hw = snd_es1938_playback;
#if VX_BUILD_NO_INCLUDE
	snd_pcm_hw_constraint_ratnums(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
				      &hw_constraints_clocks);
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 0, 0xff00);
#endif
	return 0;
}

static int snd_es1938_capture_close(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);

	chip->capture_substream = NULL;
	return 0;
}

static int snd_es1938_playback_close(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);

	switch (substream->number) {
	case 0:
		chip->playback1_substream = NULL;
		break;
	case 1:
		chip->playback2_substream = NULL;
		break;
	default:
		snd_BUG();
		return -EINVAL;
	}
	return 0;
}

static int snd_es1938_playback1_trigger(snd_pcm_substream_t * substream,
					int cmd)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		/* According to the documentation this should be:
		   0x13 but that value may randomly swap stereo channels */
                snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2CONTROL1, 0x92);
                udelay(10);
		snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2CONTROL1, 0x93);
                /* This two stage init gives the FIFO -> DAC connection time to
                 * settle before first data from DMA flows in.  This should ensure
                 * no swapping of stereo channels.  Report a bug if otherwise :-) */
		outb(0x0a, SLIO_REG(chip, AUDIO2MODE));
		chip->active |= DAC2;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		outb(0, SLIO_REG(chip, AUDIO2MODE));
		snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2CONTROL1, 0);
		chip->active &= ~DAC2;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int snd_es1938_playback2_trigger(snd_pcm_substream_t * substream,
					int cmd)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	int val;
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		val = 5;
		chip->active |= DAC1;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		val = 0;
		chip->active &= ~DAC1;
		break;
	default:
		return -EINVAL;
	}
	snd_es1938_write(chip, ESS_CMD_DMACONTROL, val);
	return 0;
}


static int snd_es1938_playback_trigger(snd_pcm_substream_t *substream,
				       int cmd)
{
	switch (substream->number) {
	case 0:
		return snd_es1938_playback1_trigger(substream, cmd);
	case 1:
		return snd_es1938_playback2_trigger(substream, cmd);
	}
	snd_BUG();
	return -EINVAL;
}

static snd_pcm_uframes_t snd_es1938_playback1_pointer(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	size_t ptr;
#if 1
	ptr = chip->dma2_size - inw(SLIO_REG(chip, AUDIO2DMACOUNT));
#else
	ptr = inl(SLIO_REG(chip, AUDIO2DMAADDR)) - chip->dma2_start;
#endif
	return ptr >> chip->dma2_shift;
}

static snd_pcm_uframes_t snd_es1938_playback2_pointer(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	size_t ptr;
	size_t old, newval;
#if 1
	/* This stuff is *needed*, don't ask why - AB */
	old = inw(SLDM_REG(chip, DMACOUNT));
	while ((newval = inw(SLDM_REG(chip, DMACOUNT))) != old)
		old = newval;
	ptr = chip->dma1_size - 1 - newval;
#else
	ptr = inl(SLDM_REG(chip, DMAADDR)) - chip->dma1_start;
#endif
	return ptr >> chip->dma1_shift;
}


static snd_pcm_uframes_t snd_es1938_playback_pointer(snd_pcm_substream_t *substream)
{
	switch (substream->number) {
	case 0:
		return snd_es1938_playback1_pointer(substream);
	case 1:
		return snd_es1938_playback2_pointer(substream);
	}
	snd_BUG();
	return (snd_pcm_uframes_t) -EINVAL;
}



/* ----------------------------------------------------------------------
 * Audio1 Capture (ADC)
 * ----------------------------------------------------------------------*/
static snd_pcm_hardware_t snd_es1938_capture =
{
#if VXWORKS_COMPILER_CANT_HANDLE
	.info =			(SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER),
	.formats =		SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U16_LE,
	.rates =		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	.rate_min =		6000,
	.rate_max =		48000,
	.channels_min =		1,
	.channels_max =		2,
        .buffer_bytes_max =	0x8000,       /* DMA controller screws on higher values */
	.period_bytes_min =	64,
	.period_bytes_max =	0x8000,
	.periods_min =		1,
	.periods_max =		1024,
	.fifo_size =		256,
#else
	/*.info =	*/		(SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER),
	/*.formats = */		SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U16_LE,
	/* .rates =	*/	SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	/*.rate_min = */		6000,
	/*.rate_max = */		48000,
	/*.channels_min = */	1,
	/*.channels_max = */		2,
    /*    .buffer_bytes_max =  */	0x8000,       /* DMA controller screws on higher values */
	/*	.period_bytes_min = */	64,
	/*.period_bytes_max = */	0x8000,
	/*.periods_min =  */		1,
	/*.periods_max =  */		1024,
	/*.fifo_size =	 */			256,
#endif
};


static int snd_es1938_capture_open(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;

	if (chip->playback2_substream)
		return -EAGAIN;
	chip->capture_substream = substream;
	runtime->hw = snd_es1938_capture;

#if VX_BUILD_NO_INCLUDE
	snd_pcm_hw_constraint_ratnums(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
				      &hw_constraints_clocks);
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 0, 0xff00);
#endif
	return 0;
}



static snd_pcm_ops_t snd_es1938_playback_ops = {
	snd_es1938_playback_open,
	snd_es1938_playback_close,
/*	snd_pcm_lib_ioctl,
	.hw_params =	snd_es1938_pcm_hw_params,
	.hw_free =    snd_es1938_pcm_hw_free,
*/	snd_es1938_playback_prepare,
	snd_es1938_playback_trigger,
	snd_es1938_set_volume,
	snd_es1938_playback_pointer,
};

/* --------------------------------------------------------------------
 * First channel for Extended Mode Audio 1 ADC Operation
 * --------------------------------------------------------------------*/
static int snd_es1938_capture_prepare(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *) snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	int u, is8, mono;
	unsigned int size = snd_pcm_lib_buffer_bytes(substream);
	unsigned int count = snd_pcm_lib_period_bytes(substream);

	chip->dma1_size = size;
	chip->dma1_start = runtime->dma_addr;

	mono = (runtime->channels > 1) ? 0 : 1;
	is8 = snd_pcm_format_width(runtime->format) == 16 ? 0 : 1;
	u = snd_pcm_format_unsigned(runtime->format);

	chip->dma1_shift = 2 - mono - is8;

	snd_es1938_reset_fifo(chip);
	
	/* program type */
	snd_es1938_bits(chip, ESS_CMD_ANALOGCONTROL, 0x03, (mono ? 2 : 1));

	/* set clock and counters */
        snd_es1938_rate_set(chip, substream, ADC1);

	count = 0x10000 - count;
	snd_es1938_write(chip, ESS_CMD_DMACNTRELOADL, count & 0xff);
	snd_es1938_write(chip, ESS_CMD_DMACNTRELOADH, count >> 8);

	/* initialize and configure ADC */
	snd_es1938_write(chip, ESS_CMD_SETFORMAT2, u ? 0x51 : 0x71);
	snd_es1938_write(chip, ESS_CMD_SETFORMAT2, 0x90 | 
		       (u ? 0x00 : 0x20) | 
		       (is8 ? 0x00 : 0x04) | 
		       (mono ? 0x40 : 0x08));

	//	snd_es1938_reset_fifo(chip);	

	/* 11. configure system interrupt controller and DMA controller */
	snd_es1938_capture_setdma(chip);

	return 0;
}


static int snd_es1938_capture_trigger(snd_pcm_substream_t * substream,
				      int cmd)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	int val;
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		val = 0x0f;
		chip->active |= ADC1;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		val = 0x00;
		chip->active &= ~ADC1;
		break;
	default:
		return -EINVAL;
	}
	snd_es1938_write(chip, ESS_CMD_DMACONTROL, val);
	return 0;
}

static snd_pcm_uframes_t snd_es1938_capture_pointer(snd_pcm_substream_t * substream)
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);
	size_t ptr;
	size_t old, newval;
#if 1
	/* This stuff is *needed*, don't ask why - AB */
	old = inw(SLDM_REG(chip, DMACOUNT));
	while ((newval = inw(SLDM_REG(chip, DMACOUNT))) != old)
		old = newval;
	ptr = chip->dma1_size - 1 - newval;
#else
	ptr = inl(SLDM_REG(chip, DMAADDR)) - chip->dma1_start;
#endif
	return ptr >> chip->dma1_shift;
}


static snd_pcm_ops_t snd_es1938_capture_ops = {
	snd_es1938_capture_open,
	snd_es1938_capture_close,
/*	snd_pcm_lib_ioctl,
	.hw_params =	snd_es1938_pcm_hw_params,
	.hw_free = 	snd_es1938_pcm_hw_free,
*/	snd_es1938_capture_prepare,
	snd_es1938_capture_trigger,
	0,
	snd_es1938_capture_pointer,
/*	.copy =		snd_es1938_capture_copy, */
};

static void snd_es1938_free_pcm(snd_pcm_t *pcm)
{
/*	this is already done by snd_pcm_free
	snd_pcm_lib_preallocate_free_for_all(pcm);
*/	

}

static int __devinit snd_es1938_new_pcm(es1938_t *chip, int device)
{
	snd_pcm_t *pcm;
	int err;

	if ((err = snd_pcm_new(chip->card, "es-1938-1946", device, 2, 1, &pcm)) < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_es1938_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_es1938_capture_ops);
	
	pcm->private_data = chip;
	pcm->private_free = snd_es1938_free_pcm; 
	pcm->info_flags = 0;
	strcpy(pcm->name, "ESS Solo-1");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(chip->pci), 64*1024, 64*1024);

	chip->pcm = pcm;
	return 0;
}

/* -----------------------------------------------------------------
 * Write to some bits of a mixer register (return old value)
 * -----------------------------------------------------------------*/
static int snd_es1938_mixer_bits(es1938_t *chip, unsigned char reg, unsigned char mask, unsigned char val)
{
	unsigned long flags;
	unsigned char old, newval, oval;
	spin_lock_irqsave(&chip->mixer_lock, flags);
	outb(reg, SLSB_REG(chip, MIXERADDR));
	old = inb(SLSB_REG(chip, MIXERDATA));
	oval = old & mask;
	if (val != oval) {
		newval = (old & ~mask) | (val & mask);
		outb(newval, SLSB_REG(chip, MIXERDATA));
#ifdef REG_DEBUG
		snd_printk_interrupt("Mixer reg %02x was %02x, set to %02x\n", reg, old, newval);
#endif
	}
	spin_unlock_irqrestore(&chip->mixer_lock, flags);
	return oval;
}



static int snd_es1938_free(es1938_t *chip)
{
#if VXWORKS_UNUSED
	if (chip->rmidi)
		snd_es1938_mixer_bits(chip, ESSSB_IREG_MPU401CONTROL, 0x40, 0);

	snd_es1938_free_gameport(chip);

#endif

	if (chip->irq >= 0)
	{
	 	free_irq(chip->irq, (void *)chip, snd_es1938_interrupt );
	}
/*	pci_release_regions(chip->pci); */
/*	pci_disable_device(chip->pci); */

	/* disable irqs */
	outb(0x00, SLIO_REG(chip, IRQCONTROL));

	/* added by Dan Walkes: turn the volume off here */
	snd_es1938_mixer_write(chip,ESSSB_IREG_AUDIO2,0);
	snd_es1938_mixer_write(chip,ESSSB_IREG_AUDIO1,0);

	snd_es1938_reset_fifo( chip );
	snd_es1938_reset( chip );
	
	kfree(chip);

	return 0;
}

static int snd_es1938_dev_free(snd_device_t *device)
{
	es1938_t *chip = (es1938_t *) device->device_data;
	return snd_es1938_free(chip);
}


/* -----------------------------------------------------------------
 * Write command to Controller Registers
 * -----------------------------------------------------------------*/
static void snd_es1938_write_cmd(es1938_t *chip, unsigned char cmd)
{
	int i;
	unsigned char v;
	for (i = 0; i < WRITE_LOOP_TIMEOUT; i++) {
		if (!(v = inb(SLSB_REG(chip, READSTATUS)) & 0x80)) {
			outb(cmd, SLSB_REG(chip, WRITEDATA));
			return;
		}
	}
	printk("snd_es1938_write_cmd timeout (0x02%x/0x02%x)\n", cmd, v);
}

/* -----------------------------------------------------------------
 * Read the Read Data Buffer
 * -----------------------------------------------------------------*/
static int snd_es1938_get_byte(es1938_t *chip)
{
	int i;
	unsigned char v;
	for (i = GET_LOOP_TIMEOUT; i; i--)
		if ((v = inb(SLSB_REG(chip, STATUS))) & 0x80)
			return inb(SLSB_REG(chip, READDATA));
	snd_printk("get_byte timeout: status 0x02%x\n", v);
	return -ENODEV;
}
/* -----------------------------------------------------------------
 * read value cmd register
 * -----------------------------------------------------------------*/
static unsigned char snd_es1938_read(es1938_t *chip, unsigned char reg)
{
	unsigned char val;
	unsigned long flags;
	spin_lock_irqsave(&chip->reg_lock, flags);
	snd_es1938_write_cmd(chip, ESS_CMD_READREG);
	snd_es1938_write_cmd(chip, reg);
	val = snd_es1938_get_byte(chip);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
#ifdef REG_DEBUG
	snd_printk("Reg %02x now is %02x\n", reg, val);
#endif
	return val;
}

/* -----------------------------------------------------------------
 * Write value cmd register
 * -----------------------------------------------------------------*/
static void snd_es1938_write(es1938_t *chip, unsigned char reg, unsigned char val)
{
	unsigned long flags;
	spin_lock_irqsave(&chip->reg_lock, flags);
	snd_es1938_write_cmd(chip, reg);
	snd_es1938_write_cmd(chip, val);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
#ifdef REG_DEBUG
	snd_printk("Reg %02x set to %02x\n", reg, val);
#endif
}

/* -----------------------------------------------------------------
 * Write to a mixer register
 * -----------------------------------------------------------------*/
static void snd_es1938_mixer_write(es1938_t *chip, unsigned char reg, unsigned char val)
{
	unsigned long flags;
	spin_lock_irqsave(&chip->mixer_lock, flags);
	outb(reg, SLSB_REG(chip, MIXERADDR));
	outb(val, SLSB_REG(chip, MIXERDATA));
	spin_unlock_irqrestore(&chip->mixer_lock, flags);
#ifdef REG_DEBUG
	snd_printk_interrupt("Mixer reg %02x set to %02x\n", reg, val);
#endif
}

/* -----------------------------------------------------------------
 * Write data to cmd register and return old value
 * -----------------------------------------------------------------*/
static int snd_es1938_bits(es1938_t *chip, unsigned char reg, unsigned char mask, unsigned char val)
{
	unsigned long flags;
	unsigned char old, newval, oval;
	spin_lock_irqsave(&chip->reg_lock, flags);
	snd_es1938_write_cmd(chip, ESS_CMD_READREG);
	snd_es1938_write_cmd(chip, reg);
	old = snd_es1938_get_byte(chip);
	oval = old & mask;
	if (val != oval) {
		snd_es1938_write_cmd(chip, reg);
		newval = (old & ~mask) | (val & mask);
		snd_es1938_write_cmd(chip, newval);
#ifdef REG_DEBUG
		snd_printk("Reg %02x was %02x, set to %02x\n", reg, old, newval);
#endif
	}
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	return oval;
}


static void snd_es1938_reset(es1938_t *chip)
{
	int i;

	outb(3, SLSB_REG(chip, RESET));
	inb(SLSB_REG(chip, RESET));
	outb(0, SLSB_REG(chip, RESET));
	for (i = 0; i < RESET_LOOP_TIMEOUT; i++) {
		if (inb(SLSB_REG(chip, STATUS)) & 0x80) {
			if (inb(SLSB_REG(chip, READDATA)) == 0xaa)
				goto __next;
		}
	}
	snd_printk("ESS Solo-1 reset failed\n");

     __next:
	snd_es1938_write_cmd(chip, ESS_CMD_ENABLEEXT);

	/* Demand transfer DMA: 4 bytes per DMA request */
	snd_es1938_write(chip, ESS_CMD_DMATYPE, 2);

	/* Change behaviour of register A1
	   4x oversampling
	   2nd channel DAC asynchronous */                                                      
	snd_es1938_mixer_write(chip, ESSSB_IREG_AUDIO2MODE, 0x32);
	/* enable/select DMA channel and IRQ channel */
	snd_es1938_bits(chip, ESS_CMD_IRQCONTROL, 0xf0, 0x50);
	snd_es1938_bits(chip, ESS_CMD_DRQCONTROL, 0xf0, 0x50);
	snd_es1938_write_cmd(chip, ESS_CMD_ENABLEAUDIO1);
	/* Set spatializer parameters to recommended values */
	snd_es1938_mixer_write(chip, 0x54, 0x8f);
	snd_es1938_mixer_write(chip, 0x56, 0x95);
	snd_es1938_mixer_write(chip, 0x58, 0x94);
	snd_es1938_mixer_write(chip, 0x5a, 0x80);
}


/*
 * initialize the chip - used by resume callback, too
 */
static void snd_es1938_chip_init(es1938_t *chip)
{
	/* reset chip */
	snd_es1938_reset(chip);

	/* configure native mode */

	/* enable bus master */
	pci_set_master(chip->pci);

	/* disable legacy audio */
	pci_write_config_word(chip->pci, SL_PCI_LEGACYCONTROL, 0x805f);

	/* set DDMA base */
	pci_write_config_word(chip->pci, SL_PCI_DDMACONTROL, chip->ddma_port | 1);

	/* set DMA/IRQ policy */
	pci_write_config_dword(chip->pci, SL_PCI_CONFIG, 0);

	/* enable Audio 1, Audio 2, MPU401 IRQ and HW volume IRQ*/
	outb(0xf0, SLIO_REG(chip, IRQCONTROL));

	/* reset DMA */
	outb(0, SLDM_REG(chip, DMACLEAR));
}


static snd_device_ops_t dev_ops;

static int __devinit snd_es1938_create(snd_card_t * card,
				    struct pci_dev * pci,
				    es1938_t ** rchip)
{
	es1938_t *chip;
	int err;

	dev_ops.dev_free = snd_es1938_dev_free;

	*rchip = NULL;

	/* enable PCI device */
	if ((err = pci_enable_device(pci)) < 0)
		return err;

        /* check, if we can restrict PCI DMA transfers to 24 bits */
	if (pci_set_dma_mask(pci, 0x00ffffff) < 0 ||
	    pci_set_consistent_dma_mask(pci, 0x00ffffff) < 0) {
                snd_printk("architecture does not support 24bit PCI busmaster DMA\n");
		pci_disable_device(pci);
                return -ENXIO;
        }

	chip = (es1938_t *)kcalloc(1, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL) {
		pci_disable_device(pci);
		return -ENOMEM;
	}
	spin_lock_init(&chip->reg_lock);
	spin_lock_init(&chip->mixer_lock);
	chip->card = card;
	chip->pci = pci;

	pGlobChip = chip;
	
	if ((err = pci_request_regions(pci, "ESS Solo-1")) < 0) {
		kfree(chip);
		pci_disable_device(pci);
		return err;
	}

	chip->io_port = pci_resource_start(pci, 0);
	chip->sb_port = pci_resource_start(pci, 1);
	chip->vc_port = pci_resource_start(pci, 2);
	chip->mpu_port = pci_resource_start(pci, 3);
	chip->game_port = pci_resource_start(pci, 4);

	if (request_irq(pci->irq, snd_es1938_interrupt, SA_INTERRUPT|SA_SHIRQ, "ES1938", (void *)chip)) {
		snd_printk("unable to grab IRQ %d\n", pci->irq);
		snd_es1938_free(chip);
		return -EBUSY;
	}

	chip->irq = pci->irq;

	snd_printk("create: io: 0x%lx, sb: 0x%lx, vc: 0x%lx, mpu: 0x%lx, game: 0x%lx\n",
		   chip->io_port, chip->sb_port, chip->vc_port, chip->mpu_port, chip->game_port);

	chip->ddma_port = chip->vc_port + 0x00;		/* fix from Thomas Sailer */

	snd_es1938_chip_init(chip);

	snd_card_set_pm_callback(card, es1938_suspend, es1938_resume, chip);

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &dev_ops)) < 0) {
		snd_es1938_free(chip);
		return err;
	}

	snd_card_set_dev(card, &pci->dev);

	
	*rchip = chip;
	return 0;
}

/* -----------------------------------------------------------------
 * Read from a mixer register
 * -----------------------------------------------------------------*/
static int snd_es1938_mixer_read(es1938_t *chip, unsigned char reg)
{
	int data;
	unsigned long flags;
	spin_lock_irqsave(&chip->mixer_lock, flags);
	outb(reg, SLSB_REG(chip, MIXERADDR));
	data = inb(SLSB_REG(chip, MIXERDATA));
	spin_unlock_irqrestore(&chip->mixer_lock, flags);
#ifdef REG_DEBUG
	snd_printk("Mixer reg %02x now is %02x\n", reg, data);
#endif
	return data;
}

static int __devinit snd_es1938_mixer(es1938_t *chip)
{
	snd_card_t *card;
	unsigned int idx;
	int err;

	card = chip->card;

	strcpy(card->mixername, "ESS Solo-1");

#if VXWORKS_UNUSED
	for (idx = 0; idx < ARRAY_SIZE(snd_es1938_controls); idx++) {
		snd_kcontrol_t *kctl;
		kctl = snd_ctl_new1(&snd_es1938_controls[idx], chip);
		switch (idx) {
			case 0:
				chip->master_volume = kctl;
				kctl->private_free = snd_es1938_hwv_free;
				break;
			case 1:
				chip->master_switch = kctl;
				kctl->private_free = snd_es1938_hwv_free;
				break;
			case 2:
				chip->hw_volume = kctl;
				kctl->private_free = snd_es1938_hwv_free;
				break;
			case 3:
				chip->hw_switch = kctl;
				kctl->private_free = snd_es1938_hwv_free;
				break;
			}
		if ((err = snd_ctl_add(card, kctl)) < 0)
			return err;
	}
#endif
	return 0;
}


static int __devinit snd_es1938_probe(struct pci_dev *pci,
				      const struct pci_device_id *pci_id)
{
	static int dev=0;
	snd_card_t *card;
	es1938_t *chip;
#if ES1938_INCLUDE_OPL_FMSYNTH
	opl3_t *opl3;
#endif
	int idx, err;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	card = snd_card_new(a_index[dev], id[dev], THIS_MODULE, 0);
	if (card == NULL)
		return -ENOMEM;
	for (idx = 0; idx < 5; idx++) {
		if (pci_resource_start(pci, idx) == 0 ||
		    !(pci_resource_flags(pci, idx) & IORESOURCE_IO)) {
		    	snd_card_free(card);
		    	return -ENODEV;
		}
	}
	if ((err = snd_es1938_create(card, pci, &chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	strcpy(card->driver, "ES1938");
	strcpy(card->shortname, "ESS ES1938 (Solo-1)");
	sprintf(card->longname, "%s rev %i, irq %i",
		card->shortname,
		chip->revision,
		chip->irq);

	if ((err = snd_es1938_new_pcm(chip, 0)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_es1938_mixer(chip)) < 0) {
		snd_card_free(card);
		return err;
	}
#if ES1938_INCLUDE_OPL_FMSYNTH
	if (snd_opl3_create(card,
			    SLSB_REG(chip, FMLOWADDR),
			    SLSB_REG(chip, FMHIGHADDR),
			    OPL3_HW_OPL3, 1, &opl3) < 0) {
		printk(KERN_ERR "es1938: OPL3 not detected at 0x%lx\n",
			   SLSB_REG(chip, FMLOWADDR));
	} else {
	        if ((err = snd_opl3_timer_new(opl3, 0, 1)) < 0) {
	                snd_card_free(card);
	                return err;
		}
	        if ((err = snd_opl3_hwdep_new(opl3, 0, 1, NULL)) < 0) {
	                snd_card_free(card);
	                return err;
		}
	}
#endif

#if ES1938_INCLUDE_MP401_UART
	if (snd_mpu401_uart_new(card, 0, MPU401_HW_MPU401,
				chip->mpu_port, 1, chip->irq, 0, &chip->rmidi) < 0) {
		printk(KERN_ERR "es1938: unable to initialize MPU-401\n");
	} else {
		// this line is vital for MIDI interrupt handling on ess-solo1
		// andreas@flying-snail.de
		snd_es1938_mixer_bits(chip, ESSSB_IREG_MPU401CONTROL, 0x40, 0x40);
	}
#endif

#if ES1938_INCLUDE_GAMEPORT
	snd_es1938_create_gameport(chip);
#endif

	/* note: added the variable chip->pcm for use w/ vxworks */
	if ((err = snd_card_register(card, chip->pcm )) < 0) {
		snd_card_free(card);
		return err;
	}

	pci_set_drvdata(pci, card);
	dev++;

	return 0;
}

/* sets the playback volume for left and right channels of specified substream
	TODO: make a better interface */
static int snd_es1938_set_volume(snd_pcm_substream_t * substream, uint8 value )
{
	es1938_t *chip = (es1938_t *)snd_pcm_substream_chip(substream);

	switch (substream->number) {
	case 0:
		snd_es1938_mixer_write(chip,ESSSB_IREG_AUDIO2,value);
		return 0;
	case 1:
		snd_es1938_mixer_write(chip,ESSSB_IREG_AUDIO1,value);
		return 0;
	}
	snd_BUG();
	return -EINVAL;
}

static void __devexit snd_es1938_remove(struct pci_dev *pci)
{
	snd_card_free((snd_card_t *)pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
}


char ES1938_Name[] = "ESS ES1938 (Solo-1)";
struct pci_driver ES1938_driver = {
	{ NULL },	/* list_head node */
	ES1938_Name,	/* char *name */
#if VXWORKS_UNUSED
	NULL,			/* struct module *owner */
#endif
	snd_es1938_ids,	/* const struct pci_device_id *id_table */
	snd_es1938_probe,	/* int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);	 New device inserted */	
	NULL,	/* Device removed (NULL if not a hot-plug capable driver) */
	vx_snd_start_driver,
	SND_PCI_PM_CALLBACKS
};

int dr_mixer( unsigned char reg )
{
	return snd_es1938_mixer_read( pGlobChip, reg );
}

void dw_mixer( unsigned char reg, unsigned char val )
{
	snd_es1938_mixer_write( pGlobChip, reg, val );
}

int dr_1938( unsigned char reg )
{
	return	snd_es1938_read( pGlobChip, reg );
}

void dw_1938( unsigned char reg, unsigned char val )
{
	snd_es1938_write( pGlobChip, reg, val );
}

