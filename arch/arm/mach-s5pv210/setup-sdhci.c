/* linux/arch/arm/mach-s5pv210/setup-sdhci.c
 *
 * Copyright (c) 2009-2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Helper functions for settign up SDHCI device(s) (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>

#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>

#include "herring.h"

/* clock sources for the mmc bus clock, order as for the ctrl2[5..4] */

char *s5pv210_hsmmc_clksrcs[4] = {
	[0] = "hsmmc",		/* HCLK */
	/* [1] = "hsmmc",	- duplicate HCLK entry */
	[2] = "sclk_mmc",	/* mmc_bus */
	/* [3] = NULL,		- reserved */
};

#define S3C_SDHCI_CTRL3_FCSELTX_INVERT  (0)
#define S3C_SDHCI_CTRL3_FCSELTX_BASIC \
	(S3C_SDHCI_CTRL3_FCSEL3 | S3C_SDHCI_CTRL3_FCSEL2)
#define S3C_SDHCI_CTRL3_FCSELRX_INVERT  (0)
#define S3C_SDHCI_CTRL3_FCSELRX_BASIC \
	(S3C_SDHCI_CTRL3_FCSEL1 | S3C_SDHCI_CTRL3_FCSEL0)

void s5pv210_setup_sdhci_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
	u32 ctrl2;
	u32 ctrl3;

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		  S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		  S3C_SDHCI_CTRL2_DFCNT_NONE |
		  S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	if (ios->clock <= (400 * 1000)) {
		ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX |
			   S3C_SDHCI_CTRL2_ENFBCLKRX);
		ctrl3 = 0;
	} else {
		u32 range_start;
		u32 range_end;

		ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKTX |
			 S3C_SDHCI_CTRL2_ENFBCLKRX;

		if (card->type == MMC_TYPE_MMC)  /* MMC */
			range_start = 20 * 1000 * 1000;
		//else    /* SD, SDIO */
		//	range_start = 25 * 1000 * 1000;

		range_end = 37 * 1000 * 1000;

		if ((ios->clock > range_start) && (ios->clock < range_end))
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC |
				S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		else if (machine_is_herring() && herring_is_cdma_wimax_dev() &&
								dev->id == 2) {
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC;
			//if(card->type & MMC_TYPE_SDIO)
				ctrl3 |= S3C_SDHCI_CTRL3_FCSELRX_BASIC;
			//else
			//	ctrl3 |= S3C_SDHCI_CTRL3_FCSELRX_INVERT;
		} else
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC |
				S3C_SDHCI_CTRL3_FCSELRX_INVERT;
	}


	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

void s5pv210_adjust_sdhci_cfg_card(struct s3c_sdhci_platdata *pdata,
				   void __iomem *r, int rw)
{
	u32 ctrl2, ctrl3;

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl3 = readl(r + S3C_SDHCI_CONTROL3);

	if (rw == 0) {
		pdata->rx_cfg++;
		if (pdata->rx_cfg == 1) {
			ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKRX;
			ctrl3 |= S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		} else if (pdata->rx_cfg == 2) {
			ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKRX;
			ctrl3 &= ~S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		} else if (pdata->rx_cfg == 3) {
			ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX |
				   S3C_SDHCI_CTRL2_ENFBCLKRX);
			pdata->rx_cfg = 0;
		}
	} else if (rw == 1) {
		pdata->tx_cfg++;
		if (pdata->tx_cfg == 1) {
			if (ctrl2 & S3C_SDHCI_CTRL2_ENFBCLKRX) {
				ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKTX;
				ctrl3 |= S3C_SDHCI_CTRL3_FCSELTX_BASIC;
			} else {
				ctrl2 &= ~S3C_SDHCI_CTRL2_ENFBCLKTX;
			}
		} else if (pdata->tx_cfg == 2) {
			ctrl2 &= ~S3C_SDHCI_CTRL2_ENFBCLKTX;
			pdata->tx_cfg = 0;
		}
	} else {
		printk(KERN_ERR "%s, unknown value rw:%d\n", __func__, rw);
		return;
	}

	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

void universal_sdhci2_cfg_ext_cd(void)
{
	printk(KERN_DEBUG "Universal :SD Detect configuration\n");
	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_NONE);
	irq_set_irq_type(IRQ_EINT(28), IRQ_TYPE_EDGE_BOTH);
}

static struct s3c_sdhci_platdata hsmmc0_platdata = {
#if defined(CONFIG_S5PV210_SD_CH0_8BIT)
	.max_width	= 8,
	.host_caps	= MMC_CAP_8_BIT_DATA,
#endif
};

#if defined(CONFIG_S3C_DEV_HSMMC2)
static struct s3c_sdhci_platdata hsmmc2_platdata = {
#if defined(CONFIG_S5PV210_SD_CH2_8BIT)
	.max_width	= 8,
	.host_caps	= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#if defined(CONFIG_S3C_DEV_HSMMC3)
static struct s3c_sdhci_platdata hsmmc3_platdata = { 0 };
#endif

static DEFINE_MUTEX(notify_lock);

#define DEFINE_MMC_CARD_NOTIFIER(num) \
static void (*hsmmc##num##_notify_func)(struct platform_device *, int state); \
static int ext_cd_init_hsmmc##num(void (*notify_func)( \
					struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func); \
	hsmmc##num##_notify_func = notify_func; \
	mutex_unlock(&notify_lock); \
	return 0; \
} \
static int ext_cd_cleanup_hsmmc##num(void (*notify_func)( \
					struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func != notify_func); \
	hsmmc##num##_notify_func = NULL; \
	mutex_unlock(&notify_lock); \
	return 0; \
}

#ifdef CONFIG_S3C_DEV_HSMMC2
DEFINE_MMC_CARD_NOTIFIER(2)
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
DEFINE_MMC_CARD_NOTIFIER(3)
#endif

/*
 * call this when you need sd stack to recognize insertion or removal of card
 * that can't be told by SDHCI regs
 */
void sdhci_s3c_force_presence_change(struct platform_device *pdev)
{
	void (*notify_func)(struct platform_device *, int state) = NULL;
	mutex_lock(&notify_lock);
#ifdef CONFIG_S3C_DEV_HSMMC2
	if (pdev == &s3c_device_hsmmc2)
		notify_func = hsmmc2_notify_func;
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	if (pdev == &s3c_device_hsmmc3)
		notify_func = hsmmc3_notify_func;
#endif

	if (notify_func)
		notify_func(pdev, 1);
	else
		pr_warn("%s: called for device with no notifier\n", __func__);
	mutex_unlock(&notify_lock);
}
EXPORT_SYMBOL_GPL(sdhci_s3c_force_presence_change);

void s3c_sdhci_set_platdata(void)
{
#if defined(CONFIG_S3C_DEV_HSMMC)
	if (machine_is_herring()) { /* TODO: move to mach-herring.c */
		hsmmc0_platdata.cd_type = S3C_SDHCI_CD_PERMANENT;
	}
	s3c_sdhci0_set_platdata(&hsmmc0_platdata);
#endif
#if defined(CONFIG_S3C_DEV_HSMMC2)
	if (machine_is_herring()) {
		if (herring_is_cdma_wimax_dev()) {
			hsmmc2_platdata.cd_type = S3C_SDHCI_CD_EXTERNAL;
			hsmmc2_platdata.ext_cd_init = ext_cd_init_hsmmc2;
			hsmmc2_platdata.ext_cd_cleanup = ext_cd_cleanup_hsmmc2;
			hsmmc2_platdata.built_in = 1;
			hsmmc2_platdata.must_maintain_clock = 1;
			hsmmc2_platdata.enable_intr_on_resume = 1;
		} else {
			hsmmc2_platdata.cd_type = S3C_SDHCI_CD_GPIO;
			hsmmc2_platdata.ext_cd_gpio = S5PV210_GPH3(4);
			hsmmc2_platdata.ext_cd_gpio_invert = true;
			universal_sdhci2_cfg_ext_cd();
		}
	}

	s3c_sdhci2_set_platdata(&hsmmc2_platdata);
#endif
#if defined(CONFIG_S3C_DEV_HSMMC3)
	if (machine_is_herring()) {
		hsmmc3_platdata.cd_type = S3C_SDHCI_CD_EXTERNAL;
		hsmmc3_platdata.ext_cd_init = ext_cd_init_hsmmc3;
		hsmmc3_platdata.ext_cd_cleanup = ext_cd_cleanup_hsmmc3;
		hsmmc3_platdata.built_in = 1;
	}
	s3c_sdhci3_set_platdata(&hsmmc3_platdata);
#endif
};
