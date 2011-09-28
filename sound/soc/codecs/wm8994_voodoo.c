/*
 * voodoo_sound.c  --  WM8994 ALSA Soc Audio driver related
 *
 *  Copyright (C) 2010 François SIMOND / twitter & XDA-developers @supercurio
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/map-base.h>
#include <mach/regs-clock.h>
#include <mach/gpio.h>
#include <linux/miscdevice.h>
#include "wm8994_samsung.h"
#include "wm8994_voodoo.h"

#define SUBJECT "wm8994_voodoo.c"
#define VOODOO_SOUND_VERSION 4

bool bypass_write_hook = false;

#ifdef CONFIG_SND_VOODOO_HP_LEVEL_CONTROL
unsigned short hplvol = CONFIG_SND_VOODOO_HP_LEVEL;
unsigned short hprvol = CONFIG_SND_VOODOO_HP_LEVEL;
#endif

#ifdef CONFIG_SND_VOODOO_FM
bool fm_radio_headset_restore_bass = true;
bool fm_radio_headset_restore_highs = true;
bool fm_radio_headset_normalize_gain = true;
#endif

#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
unsigned short recording_preset = 1;
#endif

bool dac_osr128 = true;
bool adc_osr128 = false;
bool fll_tuning = false;
bool mono_downmix = false;

// keep here a pointer to the codec structure
struct snd_soc_codec *codec_;


#ifdef CONFIG_SND_VOODOO_HP_LEVEL_CONTROL
void update_hpvol()
{
	unsigned short val;

	bypass_write_hook = true;
	// hard limit to 62 because 63 introduces distortions
	if (hplvol > 62)
		hplvol = 62;
	if (hprvol > 62)
		hprvol = 62;

	// we don't need the Volume Update flag when sending the first volume
	val = (WM8994_HPOUT1L_MUTE_N | hplvol);
	val |= WM8994_HPOUT1L_ZC;
	wm8994_write(codec_, WM8994_LEFT_OUTPUT_VOLUME, val);

	// this time we write the right volume plus the Volume Update flag. This way, both
	// volume are set at the same time
	val = (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | hprvol);
	val |= WM8994_HPOUT1L_ZC;
	wm8994_write(codec_, WM8994_RIGHT_OUTPUT_VOLUME, val);
	bypass_write_hook = false;
}
#endif

#ifdef CONFIG_SND_VOODOO_FM
void update_fm_radio_headset_restore_freqs(bool with_mute)
{
	unsigned short val;

	if (with_mute)
	{
		wm8994_write(codec_, WM8994_AIF2_DAC_FILTERS_1, 0x236);
		msleep(180);
	}

	if (fm_radio_headset_restore_bass)
	{
		// disable Sidetone high-pass filter designed for voice and not FM radio
		// Sidetone(621H): 0000  ST_HPF_CUT=000, ST_HPF=0, ST2_SEL=0, ST1_SEL=0
		wm8994_write(codec_, WM8994_SIDETONE, 0x0000);
		// disable 4FS ultrasonic mode and restore the hi-fi <4Hz hi pass filter
		// AIF2 ADC Filters(510H): 1800 AIF2ADC_4FS=0, AIF2ADC_HPF_CUT=00, AIF2ADCL_HPF=1, AIF2ADCR_HPF=1
		wm8994_write(codec_, WM8994_AIF2_ADC_FILTERS, 0x1800);
	}
	else
	{
		// default settings in GT-I9000 Froyo XXJPX kernel sources
		wm8994_write(codec_, WM8994_SIDETONE, 0x01c0);
		wm8994_write(codec_, WM8994_AIF2_ADC_FILTERS, 0xF800);
	}

	if (fm_radio_headset_restore_highs)
	{
		val = wm8994_read(codec_, WM8994_AIF2_DAC_FILTERS_1);
		val &= ~(WM8994_AIF2DAC_DEEMP_MASK);
		wm8994_write(codec_, WM8994_AIF2_DAC_FILTERS_1, val);
	}
	else
		wm8994_write(codec_, WM8994_AIF2_DAC_FILTERS_1, 0x0036);

	// un-mute
	if (with_mute)
	{
		val = wm8994_read(codec_, WM8994_AIF2_DAC_FILTERS_1);
		val &= ~(WM8994_AIF2DAC_MUTE_MASK);
		wm8994_write(codec_, WM8994_AIF2_DAC_FILTERS_1, val);
	}
}

void update_fm_radio_headset_normalize_gain()
{
	if (fm_radio_headset_normalize_gain)
	{
		// Bumped volume, change with Zero Cross
		wm8994_write(codec_, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, 0x52);
		wm8994_write(codec_, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, 0x152);
		wm8994_write(codec_, WM8994_AIF2_DRC_2, 0x0840);
		wm8994_write(codec_, WM8994_AIF2_DRC_3, 0x2408);
		wm8994_write(codec_, WM8994_AIF2_DRC_4, 0x0082);
		wm8994_write(codec_, WM8994_AIF2_DRC_5, 0x0100);
		wm8994_write(codec_, WM8994_AIF2_DRC_1, 0x019C);
	}
	else
	{
		// Original volume, change with Zero Cross
		wm8994_write(codec_, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, 0x4B);
		wm8994_write(codec_, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, 0x14B);
		wm8994_write(codec_, WM8994_AIF2_DRC_2, 0x0840);
		wm8994_write(codec_, WM8994_AIF2_DRC_3, 0x2400);
		wm8994_write(codec_, WM8994_AIF2_DRC_4, 0x0000);
		wm8994_write(codec_, WM8994_AIF2_DRC_5, 0x0000);
		wm8994_write(codec_, WM8994_AIF2_DRC_1, 0x019C);
	}
}
#endif


#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
void update_recording_preset()
{
	switch (recording_preset)
	{
		case 0:
		{
			// Original:
			// IN1L_VOL1=11000 (+19.5 dB)
			wm8994_write(codec_, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, 0x0118);
			// DRC disabled
			wm8994_write(codec_, WM8994_AIF1_DRC1_1, 0x0080);
			break;
		}
		case 2:
		{
			// High sensitivy: Original - 4.5 dB, IN1L_VOL1=10101 (+15 dB)
			wm8994_write(codec_, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, 0x0115);
			// DRC Input: -6dB, Ouptut -3.75dB
			//     Above knee 1/8, Below knee 1/2
			//     Max gain 24 / Min gain -12
			wm8994_write(codec_, WM8994_AIF1_DRC1_1, 0x009A);
			wm8994_write(codec_, WM8994_AIF1_DRC1_2, 0x0426);
			wm8994_write(codec_, WM8994_AIF1_DRC1_3, 0x0019);
			wm8994_write(codec_, WM8994_AIF1_DRC1_4, 0x0105);
			break;
		}
		case 3:
		{
			// Concert: Original - 36 dB IN1L_VOL1=00000 (-16.5 dB)
			wm8994_write(codec_, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, 0x0100);
			// DRC Input: -4.5dB, Ouptut -6.75dB
			//     Above knee 1/4, Below knee 1/2
			//     Max gain 18 / Min gain -12
			wm8994_write(codec_, WM8994_AIF1_DRC1_1, 0x009A);
			wm8994_write(codec_, WM8994_AIF1_DRC1_2, 0x0845);
			wm8994_write(codec_, WM8994_AIF1_DRC1_3, 0x0011);
			wm8994_write(codec_, WM8994_AIF1_DRC1_4, 0x00C9);
			break;
		}
		default:
		{
			// make sure recording_preset is the default: 4
			recording_preset = 1;
			// Balanced: Original - 16.5 dB, IN1L_VOL1=01101 (+3 dB)
			wm8994_write(codec_, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, 0x054D);
			// DRC Input: -13.5dB, Ouptut -9dB
			//     Above knee 1/8, Below knee 1/2
			//     Max gain 12 / Min gain -12
			wm8994_write(codec_, WM8994_AIF1_DRC1_1, 0x009A);
			wm8994_write(codec_, WM8994_AIF1_DRC1_2, 0x0844);
			wm8994_write(codec_, WM8994_AIF1_DRC1_3, 0x0019);
			wm8994_write(codec_, WM8994_AIF1_DRC1_4, 0x024C);
			break;
		}
	}
}
#endif


unsigned short osr128_get_value(unsigned short val)
{
	if (dac_osr128 == 1)
		val |= WM8994_DAC_OSR128;
	else
		val &= ~WM8994_DAC_OSR128;

	if (adc_osr128 == 1)
		val |= WM8994_ADC_OSR128;
	else
		val &= ~WM8994_ADC_OSR128;
	return val;
}

void update_osr128()
{
	unsigned short val;
	val = osr128_get_value(wm8994_read(codec_, WM8994_OVERSAMPLING));
	bypass_write_hook = true;
	wm8994_write(codec_, WM8994_OVERSAMPLING, val);
	bypass_write_hook = false;
}

unsigned short fll_tuning_get_value(unsigned short val)
{
	val = (val >> WM8994_FLL1_GAIN_WIDTH << WM8994_FLL1_GAIN_WIDTH);
	if (fll_tuning == 1)
		val |= 5;
	return val;
}

void update_fll_tuning()
{
	unsigned short val;
	val = fll_tuning_get_value(wm8994_read(codec_, WM8994_FLL1_CONTROL_4));
	bypass_write_hook = true;
	wm8994_write(codec_, WM8994_FLL1_CONTROL_4, val);
	bypass_write_hook = false;
}


unsigned short mono_downmix_get_value(unsigned short val)
{
	if (mono_downmix)
		val |= WM8994_AIF1DAC1_MONO;
	else
		val &= ~WM8994_AIF1DAC1_MONO;
	return val;
}


void update_mono_downmix()
{
	unsigned short val1, val2, val3;
	val1 = mono_downmix_get_value(wm8994_read(codec_, WM8994_AIF1_DAC1_FILTERS_1));
	val2 = mono_downmix_get_value(wm8994_read(codec_, WM8994_AIF1_DAC2_FILTERS_1));
	val3 = mono_downmix_get_value(wm8994_read(codec_, WM8994_AIF2_DAC_FILTERS_1));

	bypass_write_hook = true;
	wm8994_write(codec_, WM8994_AIF1_DAC1_FILTERS_1, val1);
	wm8994_write(codec_, WM8994_AIF1_DAC2_FILTERS_1, val2);
	wm8994_write(codec_, WM8994_AIF2_DAC_FILTERS_1, val3);
	bypass_write_hook = false;
}

/*
 *
 * Declaring the controling misc devices
 *
 */
#ifdef CONFIG_SND_VOODOO_HP_LEVEL_CONTROL
static ssize_t headphone_amplifier_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// output median of left and right headphone amplifier volumes
	return sprintf(buf,"%u\n", (hplvol + hprvol) / 2 );
}

static ssize_t headphone_amplifier_level_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short vol;
	if (sscanf(buf, "%hu", &vol) == 1)
	{
		// left and right are set to the same volumes
		hplvol = hprvol = vol;
		update_hpvol();
	}
	return size;
}
#endif


#ifdef CONFIG_SND_VOODOO_FM
static ssize_t fm_radio_headset_restore_bass_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(fm_radio_headset_restore_bass ? 1 : 0));
}

static ssize_t fm_radio_headset_restore_bass_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		fm_radio_headset_restore_bass = state == 0 ? false : true;
		update_fm_radio_headset_restore_freqs(true);
	}
	return size;
}

static ssize_t fm_radio_headset_restore_highs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(fm_radio_headset_restore_highs ? 1 : 0));
}

static ssize_t fm_radio_headset_restore_highs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		fm_radio_headset_restore_highs = state == 0 ? false : true;
		update_fm_radio_headset_restore_freqs(true);
	}
	return size;
}

static ssize_t fm_radio_headset_normalize_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(fm_radio_headset_restore_highs ? 1 : 0));
}

static ssize_t fm_radio_headset_normalize_gain_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		fm_radio_headset_normalize_gain = state == 0 ? false : true;
		update_fm_radio_headset_normalize_gain();
	}
	return size;
}
#endif


#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
static ssize_t recording_preset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%d\n", recording_preset);
}

static ssize_t recording_preset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short preset_number;
	if (sscanf(buf, "%hu", &preset_number) == 1)
	{
		recording_preset = preset_number;
		update_recording_preset();
	}
	return size;
}
#endif


static ssize_t dac_osr128_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(dac_osr128 ? 1 : 0));
}

static ssize_t dac_osr128_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		dac_osr128 = state == 0 ? false : true;
		update_osr128();
	}
	return size;
}

static ssize_t adc_osr128_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(adc_osr128 ? 1 : 0));
}

static ssize_t adc_osr128_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		adc_osr128 = state == 0 ? false : true;
		update_osr128();
	}
	return size;
}

static ssize_t fll_tuning_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(fll_tuning ? 1 : 0));
}

static ssize_t fll_tuning_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		fll_tuning = state == 0 ? false : true;
		update_fll_tuning();
	}
	return size;
}

static ssize_t mono_downmix_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",(mono_downmix ? 1 : 0));
}

static ssize_t mono_downmix_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned short state;
	if (sscanf(buf, "%hu", &state) == 1)
	{
		mono_downmix = state == 0 ? false : true;
		update_mono_downmix();
	}
	return size;
}


#ifdef CONFIG_SND_VOODOO_DEBUG
static ssize_t show_wm8994_register_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	// modified version of wm8994_register_dump from wm8994_aries.c
	int wm8994_register;

	for(wm8994_register = 0; wm8994_register <= 0x6; wm8994_register++)
		sprintf(buf, "0x%X 0x%X\n", wm8994_register, wm8994_read(codec_, wm8994_register));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x15, wm8994_read(codec_, 0x15));
	for(wm8994_register = 0x18; wm8994_register <= 0x3C; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x4C, wm8994_read(codec_, 0x4C));
	for(wm8994_register = 0x51; wm8994_register <= 0x5C; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x60, wm8994_read(codec_, 0x60));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x101, wm8994_read(codec_, 0x101));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x110, wm8994_read(codec_, 0x110));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x111, wm8994_read(codec_, 0x111));
	for(wm8994_register = 0x200; wm8994_register <= 0x212; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x220; wm8994_register <= 0x224; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x240; wm8994_register <= 0x244; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x300; wm8994_register <= 0x317; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x400; wm8994_register <= 0x411; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x420; wm8994_register <= 0x423; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x440; wm8994_register <= 0x444; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x450; wm8994_register <= 0x454; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x480; wm8994_register <= 0x493; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x4A0; wm8994_register <= 0x4B3; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x500; wm8994_register <= 0x503; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x510, wm8994_read(codec_, 0x510));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x520, wm8994_read(codec_, 0x520));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x521, wm8994_read(codec_, 0x521));
	for(wm8994_register = 0x540; wm8994_register <= 0x544; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x580; wm8994_register <= 0x593; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	for(wm8994_register = 0x600; wm8994_register <= 0x614; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x620, wm8994_read(codec_, 0x620));
	sprintf(buf, "%s0x%X 0x%X\n", buf, 0x621, wm8994_read(codec_, 0x621));
	for(wm8994_register = 0x700; wm8994_register <= 0x70A; wm8994_register++)
		sprintf(buf, "%s0x%X 0x%X\n", buf, wm8994_register, wm8994_read(codec_, wm8994_register));

	return sprintf(buf, "%s", buf);
}


static ssize_t store_wm8994_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	short unsigned int reg = 0;
	short unsigned int val = 0;
	int unsigned bytes_read = 0;

	while (sscanf(buf, "%hx %hx%n", &reg, &val, &bytes_read) == 2)
	{
		buf += bytes_read;
		wm8994_write(codec_, reg, val);
	}
	return size;
}
#endif


static ssize_t voodoo_sound_version(struct device *dev, struct device_attribute *attr, char *buf) {
	return sprintf(buf, "%u\n", VOODOO_SOUND_VERSION);
}

#ifdef CONFIG_SND_VOODOO_HP_LEVEL_CONTROL
static DEVICE_ATTR(headphone_amplifier_level, S_IRUGO | S_IWUGO , headphone_amplifier_level_show, headphone_amplifier_level_store);
#endif
#ifdef CONFIG_SND_VOODOO_FM
static DEVICE_ATTR(fm_radio_headset_restore_bass, S_IRUGO | S_IWUGO , fm_radio_headset_restore_bass_show, fm_radio_headset_restore_bass_store);
static DEVICE_ATTR(fm_radio_headset_restore_highs, S_IRUGO | S_IWUGO , fm_radio_headset_restore_highs_show, fm_radio_headset_restore_highs_store);
static DEVICE_ATTR(fm_radio_headset_normalize_gain, S_IRUGO | S_IWUGO , fm_radio_headset_normalize_gain_show, fm_radio_headset_normalize_gain_store);
#endif
#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
static DEVICE_ATTR(recording_preset, S_IRUGO | S_IWUGO , recording_preset_show, recording_preset_store);
#endif
static DEVICE_ATTR(dac_osr128, S_IRUGO | S_IWUGO , dac_osr128_show, dac_osr128_store);
static DEVICE_ATTR(adc_osr128, S_IRUGO | S_IWUGO , adc_osr128_show, adc_osr128_store);
static DEVICE_ATTR(fll_tuning, S_IRUGO | S_IWUGO , fll_tuning_show, fll_tuning_store);
static DEVICE_ATTR(mono_downmix, S_IRUGO | S_IWUGO , mono_downmix_show, mono_downmix_store);
#ifdef CONFIG_SND_VOODOO_DEBUG
static DEVICE_ATTR(wm8994_register_dump, S_IRUGO , show_wm8994_register_dump, NULL);
static DEVICE_ATTR(wm8994_write, S_IWUSR , NULL, store_wm8994_write);
#endif
static DEVICE_ATTR(version, S_IRUGO , voodoo_sound_version, NULL);

static struct attribute *voodoo_sound_attributes[] = {
#ifdef CONFIG_SND_VOODOO_HP_LEVEL_CONTROL
		&dev_attr_headphone_amplifier_level.attr,
#endif
#ifdef CONFIG_SND_VOODOO_FM
		&dev_attr_fm_radio_headset_restore_bass.attr,
		&dev_attr_fm_radio_headset_restore_highs.attr,
		&dev_attr_fm_radio_headset_normalize_gain.attr,
#endif
#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
		&dev_attr_recording_preset.attr,
#endif
		&dev_attr_dac_osr128.attr,
		&dev_attr_adc_osr128.attr,
		&dev_attr_fll_tuning.attr,
		&dev_attr_mono_downmix.attr,
#ifdef CONFIG_SND_VOODOO_DEBUG
		&dev_attr_wm8994_register_dump.attr,
		&dev_attr_wm8994_write.attr,
#endif
		&dev_attr_version.attr,
		NULL
};

static struct attribute_group voodoo_sound_group = {
		.attrs  = voodoo_sound_attributes,
};

static struct miscdevice voodoo_sound_device = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "voodoo_sound",
};


/*
 *
 * Driver Hooks
 *
 */

#ifdef CONFIG_SND_VOODOO_FM
void voodoo_hook_fmradio_headset()
{
	if (! fm_radio_headset_restore_bass && ! fm_radio_headset_restore_highs && !fm_radio_headset_normalize_gain)
		return;

	update_fm_radio_headset_restore_freqs(false);
	update_fm_radio_headset_normalize_gain();
}
#endif


#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
void voodoo_hook_record_main_mic()
{
	update_recording_preset();
}
#endif

void voodoo_hook_playback_headset()
{
}


unsigned int voodoo_hook_wm8994_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	struct wm8994_priv *wm8994 = codec->drvdata;
	// modify some registers before those being written to the codec

	if (! bypass_write_hook)
	{
#ifdef CONFIG_SND_VOODOO_HP_LEVEL_CONTROL
		if (wm8994->cur_path == HP || wm8994->cur_path == HP_NO_MIC)
		{
			if (reg == WM8994_LEFT_OUTPUT_VOLUME)
				value = (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | hplvol);
			if (reg == WM8994_RIGHT_OUTPUT_VOLUME)
				value = (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | hprvol);
		}
#endif
		if (reg == WM8994_OVERSAMPLING)
			value = osr128_get_value(value);
		if (reg == WM8994_FLL1_CONTROL_4)
			value = fll_tuning_get_value(value);
		if (reg == WM8994_AIF1_DAC1_FILTERS_1 || reg == WM8994_AIF1_DAC2_FILTERS_1 || reg == WM8994_AIF2_DAC_FILTERS_1)
			value = mono_downmix_get_value(value);
	}

#ifdef CONFIG_SND_VOODOO_DEBUG_LOG
	// log every write to dmesg
	printk("Voodoo sound: wm8994_write register= [%X] value= [%X]\n", reg, value);
	printk("Voodoo sound: cur_path=%i, rec_path=%i, power_state=%i\n",
		wm8994->cur_path, wm8994->rec_path, wm8994->power_state);
#endif
	return value;
}


void voodoo_hook_wm8994_pcm_probe(struct snd_soc_codec *codec)
{
	printk("Voodoo sound: driver v%d\n", VOODOO_SOUND_VERSION);
	misc_register(&voodoo_sound_device);
	if (sysfs_create_group(&voodoo_sound_device.this_device->kobj, &voodoo_sound_group) < 0)
	{
		printk("%s sysfs_create_group fail\n", __FUNCTION__);
		pr_err("Failed to create sysfs group for device (%s)!\n", voodoo_sound_device.name);
	}

	// make a copy of the codec pointer
	codec_ = codec;
}
