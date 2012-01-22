/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   :s3c-otg-roothub.h
 *  [Description] : The Header file defines the external and internal functions of RootHub.
 *  [Author]      : Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *  [Department]  : System LSI Division/Embedded S/W Platform
 *  [Created Date]: 2008/06/13
 *  [Revision History]
 *      (1) 2008/06/13   by Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *          - Created this file and defines functions of RootHub
 *
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

#ifndef _ROOTHUB_H_
#define _ROOTHUB_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-regdef.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-hcdi-kal.h"
#include "s3c-otg-hcdi-memory.h"
#include "s3c-otg-oci.h"

__inline__ int root_hub_feature( 
		struct usb_hcd *hcd,
		const u8 port,
		const u16 type_req,
		const u16 feature,
		void *buf);

__inline__ int get_otg_port_status(
		struct usb_hcd *hcd, const u8 port, char *status);

int reset_and_enable_port(struct usb_hcd *hcd, const u8 port); 
void bus_suspend(void);

int bus_resume(struct sec_otghost *otghost);

#ifdef __cplusplus
}
#endif
#endif /* _ROOTHUB_H_ */


