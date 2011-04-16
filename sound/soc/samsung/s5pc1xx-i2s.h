/* sound/soc/s3c24xx/s3c64xx-i2s.h
 *
 * ALSA SoC Audio Layer - S3C64XX I2S driver
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_S3C24XX_S3C64XX_I2S_H
#define __SND_SOC_S3C24XX_S3C64XX_I2S_H __FILE__

struct clk;

//#include "s3c-i2s-v2.h"
//==

#define S3C_I2SV2_DIV_BCLK	(1)
#define S3C_I2SV2_DIV_RCLK	(2)
#define S3C_I2SV2_DIV_PRESCALER	(3)

/**
 * struct s3c_i2sv2_info - S3C I2S-V2 information
 * @dev: The parent device passed to use from the probe.
 * @regs: The pointer to the device registe block.
 * @master: True if the I2S core is the I2S bit clock master.
 * @dma_playback: DMA information for playback channel.
 * @dma_capture: DMA information for capture channel.
 * @suspend_iismod: PM save for the IISMOD register.
 * @suspend_iiscon: PM save for the IISCON register.
 * @suspend_iispsr: PM save for the IISPSR register.
 *
 * This is the private codec state for the hardware associated with an
 * I2S channel such as the register mappings and clock sources.
 */
struct s3c_i2sv2_info {
	struct device	*dev;
	void __iomem	*regs;

	struct clk	*sclk_audio;
	struct clk	*iis_ipclk;
	struct clk	*iis_cclk;
	struct clk	*iis_clk;
	struct clk	*iis_busclk;
	struct regulator	*regulator;

	unsigned char	 master;

	struct s3c_dma_params	*dma_playback;
	struct s3c_dma_params	*dma_capture;

	u32		 suspend_iismod;
	u32		 suspend_iiscon;
	u32		 suspend_iispsr;
	u32		 suspend_iisahb;
	u32		 suspend_audss_clksrc;
	u32      suspend_audss_clkdiv;
	u32      suspend_audss_clkgate;
};

struct s3c_i2sv2_rate_calc {
	unsigned int	clk_div;	/* for prescaler */
	unsigned int	fs_div;		/* for root frame clock */
};

extern int s3c_i2sv2_iis_calc_rate(struct s3c_i2sv2_rate_calc *info,
				   unsigned int *fstab,
				   unsigned int rate, struct clk *clk);

/**
 * s3c_i2sv2_probe - probe for i2s device helper
 * @pdev: The platform device supplied to the original probe.
 * @dai: The ASoC DAI structure supplied to the original probe.
 * @i2s: Our local i2s structure to fill in.
 * @base: The base address for the registers.
 */
extern int s3c_i2sv2_probe(struct platform_device *pdev,
			   struct snd_soc_dai *dai,
			   struct s3c_i2sv2_info *i2s,
			   unsigned long base);

/**
 * s3c_i2sv2_register_dai - register dai with soc core
 * @dai: The snd_soc_dai structure to register
 *
 * Fill in any missing fields and then register the given dai with the
 * soc core.
 */
extern int s3c_i2sv2_register_dai(struct snd_soc_dai *dai);
extern void s5p_idma_init(void *);

//==

#define USE_CLKAUDIO 1

#define S3C64XX_DIV_BCLK	S3C_I2SV2_DIV_BCLK
#define S3C64XX_DIV_RCLK	S3C_I2SV2_DIV_RCLK
#define S3C64XX_DIV_PRESCALER	S3C_I2SV2_DIV_PRESCALER

#define S3C64XX_CLKSRC_PCLK	(0)
#define S3C64XX_CLKSRC_MUX	(1)
#define S3C64XX_CLKSRC_CDCLK    (2)

extern struct snd_soc_dai s3c64xx_i2s_dai[];

extern struct snd_soc_dai i2s_sec_fifo_dai;
extern struct snd_soc_dai i2s_dai;
extern struct snd_soc_platform s3c_dma_wrapper;

extern struct clk *s3c64xx_i2s_get_clock(struct snd_soc_dai *dai);
extern int s5p_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai);
extern int s5p_i2s_startup(struct snd_soc_dai *dai);
extern int s5p_i2s_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai);
extern void s5p_i2s_sec_init(void *, dma_addr_t);

#endif /* __SND_SOC_S3C24XX_S3C64XX_I2S_H */
