/*
 * Copyright (C) 2010-2011 Samsung Electronics Co., Ltd.
 *
 * S5P series device definition for MIPI-CSIS channel 0
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <plat/csis.h>
#include <plat/mipi_csis.h>
#include <mach/map.h>

static struct resource s5p_mipi_csis0_resource[] = {
	[0] = {
		.start = S5P_PA_MIPI_CSIS0,
		.end   = S5P_PA_MIPI_CSIS0 + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MIPI_CSIS0,
		.end   = IRQ_MIPI_CSIS0,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_mipi_csis0 = {
	.name		  = "s5p-mipi-csis",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s5p_mipi_csis0_resource),
	.resource	  = s5p_mipi_csis0_resource,
};

static struct s3c_platform_csis default_csis_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = NULL;
	npd->cfg_phy_global = (void *)s5p_csis_phy_enable;

	s5p_device_mipi_csis0.dev.platform_data = npd;
}
