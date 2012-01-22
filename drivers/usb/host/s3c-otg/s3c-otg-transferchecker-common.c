/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : CommonTransferChecker.c
 *  [Description] : The Source file implements the external and internal functions of CommonTransferChecker.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2009/01/12
 *  [Revision History]
 *      (1) 2008/06/12   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements functions of CommonTransferChecker
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

#include "s3c-otg-transferchecker-common.h"

/******************************************************************************/
/*!
 * @name	int  	init_done_transfer_checker(void)
 *
 * @brief		this function initiates S3CDoneTransferChecker Module.
 *
 *
 * @param	void
 *
 * @return	void
 */
/******************************************************************************/
//ss1
/*void		init_done_transfer_checker(void)
{
	return USB_ERR_SUCCESS;
}*/

/******************************************************************************/
/*!
 * @name	void  	do_transfer_checker(struct sec_otghost *otghost)
 *
 * @brief		this function processes the result of USB Transfer. So, do_transfer_checker fistly
 *			check which channel occurs OTG Interrupt and gets the status information of the channel.
 *			do_transfer_checker requests the information of td_t to S3CScheduler.
 *			To process the interrupt of the channel, do_transfer_checker calls the sub-modules of
 *			S3CDoneTransferChecker, for example, ControlTransferChecker, BulkTransferChecker.
 *			according to the process result of the channel interrupt, do_transfer_checker decides
 *			the USB Transfer will be done or retransmitted.
 *
 *
 * @param	void
 *
 * @return	void
 */
/*******************************************************************************/
void do_transfer_checker (struct sec_otghost *otghost)
__releases(&otghost->lock)
__acquires(&otghost->lock)
{
	u32 	hc_intr = 0;
	u32	hc_intr_msk = 0;
	u8	do_try_cnt = 0;

	hc_info_t	ch_info;
	u32	td_addr = 0;
	td_t 	*done_td = {0};
	u8	proc_result = 0;

	//by ss1
	otg_mem_set((void *)&ch_info, 0, sizeof(hc_info_t));

	// Get value of HAINT...
	get_intr_ch(&hc_intr,&hc_intr_msk);

start_do_transfer_checker:

	while(do_try_cnt<MAX_CH_NUMBER) {
		//checks the channel number to be masked or not.
		if(!(hc_intr & hc_intr_msk & (1<<do_try_cnt))) {
			do_try_cnt++;
			goto start_do_transfer_checker;
		}

		//Gets the address of the td_t to have the channel to be interrupted.
		if(get_td_info(do_try_cnt, &td_addr) == USB_ERR_SUCCESS) {

                        done_td = (td_t *)td_addr;

                        if(do_try_cnt != done_td->cur_stransfer.alloc_chnum) {
                                do_try_cnt++;
                                goto start_do_transfer_checker;
                        }
                }
	       	else {
                        do_try_cnt++;
                        goto start_do_transfer_checker;
                }

		//Gets the informationof channel to be interrupted.
		get_ch_info(&ch_info,do_try_cnt);

		switch(done_td->parent_ed_p->ed_desc.endpoint_type) {
		case CONTROL_TRANSFER:
			proc_result = process_control_transfer(done_td, &ch_info);
			break;
		case BULK_TRANSFER:
			proc_result = process_bulk_transfer(done_td, &ch_info);
			break;
		case INT_TRANSFER:
			proc_result = process_intr_transfer(done_td, &ch_info);
			break;
		case ISOCH_TRANSFER:
			/* proc_result = ProcessIsochTransfer(done_td, &ch_info); */
			break;
		default:break;
		}

		if((proc_result == RE_TRANSMIT) || (proc_result == RE_SCHEDULE)) {
			done_td->parent_ed_p->ed_status.is_in_transferring 	= 	false;
			done_td->is_transfer_done				= 	false;
			done_td->is_transferring				=	false;

			if(done_td->parent_ed_p->ed_desc.endpoint_type == CONTROL_TRANSFER ||
				done_td->parent_ed_p->ed_desc.endpoint_type==BULK_TRANSFER) {
				update_nonperio_stransfer(done_td);
			}
			else {
				update_perio_stransfer(done_td);
			}

			if(proc_result == RE_TRANSMIT) {
				retransmit(otghost, done_td);
			}
			else {
				reschedule(done_td);
			}
		}

		else if(proc_result==DE_ALLOCATE) {
			done_td->parent_ed_p->ed_status.is_in_transferring 	= 	false;
			done_td->parent_ed_p->ed_status.in_transferring_td	=	0;
			done_td->is_transfer_done				= 	true;
			done_td->is_transferring				=	false;

			spin_unlock_otg(&otghost->lock);
			otg_usbcore_giveback( done_td);
			spin_lock_otg(&otghost->lock);
			release_trans_resource(otghost, done_td);
		}
		else {	//NO_ACTION....
			done_td->parent_ed_p->ed_status.is_in_transferring 	= 	true;
			done_td->parent_ed_p->ed_status.in_transferring_td	=	(u32)done_td;
			done_td->is_transfer_done				= 	false;
			done_td->is_transferring				=	true;
		}
		do_try_cnt++;
	}
	// Complete to process the Channel Interrupt.
	// So. we now start to scheduler of S3CScheduler.
	do_schedule(otghost);
}


int release_trans_resource(struct sec_otghost *otghost, td_t *done_td)
{
	//remove the pDeallocateTD from parent_ed_p.
	otg_list_pop(&done_td->td_list_entry);
	done_td->parent_ed_p->num_td--;

	//Call deallocate to release the channel and bandwidth resource of S3CScheduler.
	deallocate(done_td);
	delete_td(otghost, done_td);
	return USB_ERR_SUCCESS;
}

u32 calc_transferred_size(bool f_is_complete, td_t *td, hc_info_t *hc_info)
{
	if(f_is_complete) {
		if(td->parent_ed_p->ed_desc.is_ep_in) {
			return td->cur_stransfer.buf_size - hc_info->hc_size.b.xfersize;
		}
		else {
			return	td->cur_stransfer.buf_size;
		}
	}
	else {
		return	(td->cur_stransfer.packet_cnt - hc_info->hc_size.b.pktcnt)*td->parent_ed_p->ed_desc.max_packet_size;
	}
}

void update_frame_number(td_t *pResultTD)
{
	u32 cur_frame_num=0;

	if(pResultTD->parent_ed_p->ed_desc.endpoint_type == CONTROL_TRANSFER ||
		pResultTD->parent_ed_p->ed_desc.endpoint_type == BULK_TRANSFER) {
		return;
	}

	pResultTD->parent_ed_p->ed_desc.sched_frame+= pResultTD->parent_ed_p->ed_desc.interval;
	pResultTD->parent_ed_p->ed_desc.sched_frame &= HFNUM_MAX_FRNUM;

	cur_frame_num = oci_get_frame_num();
	if(((cur_frame_num - pResultTD->parent_ed_p->ed_desc.sched_frame)&HFNUM_MAX_FRNUM) <= (HFNUM_MAX_FRNUM>>1)) {
		pResultTD->parent_ed_p->ed_desc.sched_frame = cur_frame_num;
	}
}

void update_datatgl(u8 ubCurDataTgl, td_t *td)
{
	switch(td->parent_ed_p->ed_desc.endpoint_type) {
	case CONTROL_TRANSFER:
		if(td->standard_dev_req_info.conrol_transfer_stage == DATA_STAGE) {
			td->parent_ed_p->ed_status.control_data_tgl.data_tgl = ubCurDataTgl;
		}
		break;
	case BULK_TRANSFER:
	case INT_TRANSFER:
		td->parent_ed_p->ed_status.data_tgl = ubCurDataTgl;
		break;

	case ISOCH_TRANSFER:
		break;
	default:break;
	}
}
