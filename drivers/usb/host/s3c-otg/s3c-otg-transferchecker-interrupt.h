/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : IntTransferChecker.h
 *  [Description] : The Header file defines the external and internal functions of IntTransferChecker
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/19
 *  [Revision History]
 *      (1) 2008/06/18   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and defines functions of IntTransferChecker
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

#ifndef  _INT_TRANSFER_CHECKER_H
#define  _INT_TRANSFER_CHECKER_H

/*
// ----------------------------------------------------------------------------
// Include files : None.
// ----------------------------------------------------------------------------
*/

#include "s3c-otg-common-common.h"
//#include "s3c-otg-common-typedef.h"
#include "s3c-otg-common-const.h"
#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-common-regdef.h"

#include "s3c-otg-hcdi-debug.h"
#include "s3c-otg-scheduler-scheduler.h"
#include "s3c-otg-isr.h"
#include "s3c-otg-transferchecker-checker.h"



#ifdef __cplusplus
extern "C"
{
#endif

u8	process_intr_transfer(td_t 	*pRawTD,
				hc_info_t *pHCRegData);

u8	process_xfercompl_on_intr(td_t 	*pRawTD,
					hc_info_t *pHCRegData);

u8	process_chhltd_on_intr(td_t 	*pRawTD,
				hc_info_t *pHCRegData);

u8	process_ahb_on_intr(td_t 	*pRawTD,
				hc_info_t *pHCRegData);

u8	process_stall_on_intr(td_t 		*pRawTD,
				 hc_info_t	*pHCRegData);

u8	process_nak_on_intr(td_t	*pRawTD,
			     hc_info_t 	*pHCRegData);

u8	process_frmovrrun_on_intr(td_t	*pRawTD,
					hc_info_t *pHCRegData);

u8	process_ack_on_intr(td_t 	*pRawTD,
				hc_info_t *pHCRegData);

u8	process_xacterr_on_intr(td_t 	*pRawTD,
				   hc_info_t 	*pHCRegData);

u8	process_bblerr_on_intr(td_t 		*pRawTD,
				  hc_info_t 	*pHCRegData);

u8	process_datatgl_on_intr(td_t	*pRawTD,
				hc_info_t	*pHCRegData);

#ifdef __cplusplus
}
#endif


#endif

