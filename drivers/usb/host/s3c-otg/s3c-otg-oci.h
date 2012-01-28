/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   :s3c-otg-oci.h
 *  [Description] : The Header file defines the external and internal functions of OCI.
 *  [Author]      : Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *  [Department]  : System LSI Division/Embedded S/W Platform
 *  [Created Date]: 2008/06/18
 *  [Revision History]
 *      (1) 2008/06/25   by Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *          - Added some functions and data structure of OCI
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

#ifndef _OCI_H_
#define _OCI_H_

#ifdef __cplusplus
extern "C"
{
#endif

//#include "s3c-otg-common-const.h"
#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-regdef.h"
#include "s3c-otg-hcdi-kal.h"
#include "s3c-otg-hcdi-memory.h"
#include "s3c-otg-hcdi-debug.h"
#include "s3c-otg-roothub.h"
#include "s3c-otg-hcdi-hcd.h"

#include <mach/map.h>	//virtual address for smdk

extern void otg_host_phy_init(void);
#include <mach/regs-clock.h>

//OCI interace
int	oci_init(struct sec_otghost *otghost);

int	oci_start(struct sec_otghost *otghost);
int	oci_stop(struct sec_otghost *otghost);

u8	oci_start_transfer(struct sec_otghost *otghost, stransfer_t *st_t);
int	oci_stop_transfer(struct sec_otghost *otghost, u8 ch_num);

int	oci_channel_init(u8 ch_num, stransfer_t *st_t);
u32	oci_get_frame_num(void);
u16	oci_get_frame_interval(void);
void	oci_set_frame_interval(u16 intervl);

///OCI Internal Functions
int	oci_sys_init(struct sec_otghost *otghost);
int	oci_core_init(struct sec_otghost *otghost);
int	oci_init_mode(struct sec_otghost *otghost);
int	oci_host_init(struct sec_otghost *otghost);
int	oci_dev_init(struct sec_otghost *otghost);

int	oci_channel_alloc(u8 *ch_num);
int	oci_channel_dealloc(u8 ch_num);

void	oci_config_flush_fifo(u32 mode);
void	oci_flush_tx_fifo(u32 num);
void	oci_flush_rx_fifo(void);

int	oci_core_reset(void);
void	oci_set_global_interrupt(bool set);

#ifdef __cplusplus
}
#endif
#endif /* _OCI_H_ */
