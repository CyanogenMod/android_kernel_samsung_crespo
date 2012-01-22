/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : CommonTransferChecker.h
 *  [Description] : The Header file defines the external and internal functions of CommonTransferChecker.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/12
 *  [Revision History]
 *      (1) 2008/06/12   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and defines functions of CommonTransferChecker
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

#ifndef  _COMMON_TRANSFER_CHECKER_H
#define  _COMMON_TRANSFER_CHECKER_H

/*
// ----------------------------------------------------------------------------
// Include files : None.
// ----------------------------------------------------------------------------
*/

#include "s3c-otg-common-common.h"
//#include "s3c-otg-common-const.h"
#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-common-regdef.h"
#include "s3c-otg-transfer-transfer.h"

#include "s3c-otg-hcdi-debug.h"
#include "s3c-otg-hcdi-memory.h"
#include "s3c-otg-scheduler-scheduler.h"
#include "s3c-otg-isr.h"
#include "s3c-otg-transferchecker-control.h"
#include "s3c-otg-transferchecker-bulk.h"
#include "s3c-otg-transferchecker-interrupt.h"
//#include "s3c-otg-transferchecker-iso.h"



#ifdef __cplusplus
extern "C"
{
#endif

//void	init_done_transfer_checker (void);
void	do_transfer_checker (struct sec_otghost *otghost);
int	release_trans_resource(struct sec_otghost *otghost, td_t *done_td);
u32	calc_transferred_size(bool 	f_is_complete,
				td_t 	*td,
				hc_info_t *hc_info);
void	update_frame_number(td_t *result_td);
void	update_datatgl(u8	cur_data_tgl,
			td_t	*td);

#ifdef __cplusplus
}
#endif


#endif


