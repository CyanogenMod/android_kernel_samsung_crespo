/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-common-common.h
 * @brief  it includes common header files for all modules \n
 * @version
 *  ex)-# Jun 11,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *    : Creating the initial version of this code \n
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

#ifndef _S3C_OTG_COMMON_COMMON_H_
#define _S3C_OTG_COMMON_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

//#include "s3c-otg-common-typedef.h"
#include "s3c-otg-common-errorcode.h"
#include <linux/errno.h>
#include <linux/usb.h>

//Define OS
#define LINUX	1

//Kernel Version
#define KERNEL_2_6_21

#ifdef __cplusplus
}
#endif
#endif /* _S3C_OTG_COMMON_COMMON_H_ */
