/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-debug.c
 * @brief  It provides debug functions for display message \n
 * @version
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
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

#ifndef _S3C_OTG_HCDI_DEBUG_H_
#define _S3C_OTG_HCDI_DEBUG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define OTG_DEBUG

#ifdef OTG_DEBUG
#if 0
#include <linux/stddef.h>
#endif

#define OTG_DBG_OTGHCDI_DRIVER	true
#define OTG_DBG_OTGHCDI_HCD		false
#define OTG_DBG_OTGHCDI_KAL		false
#define OTG_DBG_OTGHCDI_LIST	false
#define OTG_DBG_OTGHCDI_MEM		false

#define OTG_DBG_TRANSFER		false
#define OTG_DBG_SCHEDULE		false
#define OTG_DBG_OCI				false
#define OTG_DBG_DONETRASF		false
#define OTG_DBG_ISR				false
#define OTG_DBG_ROOTHUB			false


#include <linux/kernel.h>	//for printk

#define otg_err(is_active, msg...) \
	do{ if (/*(is_active) == */true)\
		{\
			pr_err("otg_err: in %s()::%05d ", __func__ , __LINE__); \
			pr_err("=> " msg); \
		}\
	}while(0)

#define otg_dbg(is_active, msg...) \
	do{ if ((is_active) == true)\
		{\
			pr_info("otg_dbg: in %s()::%05d ", __func__, __LINE__); \
			pr_info("=> " msg); \
		}\
	}while(0)

#else //OTG_DEBUG

# define otg_err(is_active, msg...) 	do{}while(0)
# define otg_dbg(is_active, msg...)	do{}while(0)

#endif


#ifdef __cplusplus
}
#endif

#endif /* _S3C_OTG_HCDI_DEBUG_H_ */
