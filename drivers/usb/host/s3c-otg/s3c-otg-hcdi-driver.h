/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-driver.h
 * @brief  header of s3c-otg-hcdi-driver \n
 * @version
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
 *  -# Aug 18,2008 v1.3 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Modifying for successful rmmod & disconnecting \n
 * @see None
 ****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#ifndef _S3C_OTG_HCDI_DRIVER_H_
#define _S3C_OTG_HCDI_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <linux/module.h>
#include <linux/init.h>
//#include <linux/clk.h>	//for clk_get, clk_enable etc.

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/interrupt.h> //for SA_SHIRQ
#include <mach/map.h>	//address for smdk
#include <linux/dma-mapping.h> //dma_alloc_coherent
#include <linux/ioport.h>	//request_mem_request ...
#include <asm/irq.h>	//for IRQ_OTG
#include <linux/clk.h>


#include "s3c-otg-common-common.h"
#include "s3c-otg-common-regdef.h"

#include "s3c-otg-hcdi-debug.h"
#include "s3c-otg-hcdi-hcd.h"
#include "s3c-otg-hcdi-kal.h"


volatile u8 *		g_pUDCBase;

static const char	gHcdName[] = "EMSP_OTG_HCD";

//extern int otg_hcd_init_modules(struct sec_otghost *otghost);
//extern void otg_hcd_deinit_modules(struct sec_otghost *otghost);

//void otg_print_registers();

#ifdef __cplusplus
}
#endif

#endif /* _S3C_OTG_HCDI_DRIVER_H_ */
