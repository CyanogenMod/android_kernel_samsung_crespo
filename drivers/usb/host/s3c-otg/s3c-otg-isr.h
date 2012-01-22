/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   :s3c-otg-isr.h
 *  [Description] : The Header file defines the external and internal functions of ISR.
 *  [Author]      : Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *  [Department]  : System LSI Division/Embedded S/W Platform
 *  [Created Date]: 2008/06/18
 *  [Revision History]
 *      (1) 2008/06/18   by Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *          - Created this file and defines functions of Scheduler
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

#ifndef _ISR_H_
#define _ISR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-const.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-common-regdef.h"
#include "s3c-otg-hcdi-kal.h"
#include "s3c-otg-scheduler-scheduler.h"
#include "s3c-otg-transferchecker-common.h"
#include "s3c-otg-roothub.h"
#include "s3c-otg-oci.h"

__inline__ void otg_handle_interrupt(struct usb_hcd *hcd);

void process_port_intr(struct usb_hcd *hcd);

void mask_channel_interrupt(u32 ch_num, u32 mask_info);

void unmask_channel_interrupt(u32 ch_num, u32 mask_info);

extern int get_ch_info(hc_info_t *hc_reg, u8 ch_num);

extern void get_intr_ch(u32 *haint, u32 *haintmsk);

extern void clear_ch_intr(u8 ch_num, u32 clear_bit);

extern void enable_sof(void);

extern void disable_sof(void);

#ifdef __cplusplus
}
#endif
#endif /* _ISR_H_ */



