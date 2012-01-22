/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : NonPeriodicTransfer.c
 *  [Description] : This source file implements the functions to be defined at NonPeriodicTransfer Module.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/09
 *  [Revision History]
 *      (1) 2008/06/09   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements some functions of PeriodicTransfer.
 *  	 -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  			: Optimizing for performance \n
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

#include "s3c-otg-transfer-transfer.h"


/******************************************************************************/
/*!
 * @name	int  	init_perio_stransfer(	bool	f_is_isoch_transfer,
 *					  	td_t 	*parent_td)
 *
 * @brief		this function initiates the parent_td->cur_stransfer for Periodic Transfer and
 *			inserts this init_td_p to init_td_p->parent_ed_p.
 *
 * @param	[IN]	f_is_isoch_transfer	= indicates whether this transfer is Isochronous or not.
 *		[IN] parent_td	= indicates the address of td_t to be initiated.
 *
 * @return	USB_ERR_SUCCESS	-if success to update the STranfer of pUpdateTD.
 *		USB_ERR_FAIL		-if fail to update the STranfer of pUpdateTD.
 */
 /******************************************************************************/
int  	init_perio_stransfer(	bool	f_is_isoch_transfer,
				td_t 	*parent_td)
{
	parent_td->cur_stransfer.ed_desc_p 	= &parent_td->parent_ed_p->ed_desc;
	parent_td->cur_stransfer.ed_status_p 	= &parent_td->parent_ed_p->ed_status;
	parent_td->cur_stransfer.alloc_chnum	= CH_NONE;
	parent_td->cur_stransfer.parent_td		= (u32)parent_td;
	parent_td->cur_stransfer.stransfer_id  	= (u32)&parent_td->cur_stransfer;

	otg_mem_set(&parent_td->cur_stransfer.hc_reg, 0, sizeof(hc_reg_t));

	parent_td->cur_stransfer.hc_reg.hc_int_msk.b.chhltd = 1;

	if(f_is_isoch_transfer)
	{
		// initiates the STransfer usinb the IsochPacketDesc[0].
		parent_td->cur_stransfer.buf_size		=parent_td->isoch_packet_desc_p[0].buf_size;
		parent_td->cur_stransfer.start_phy_buf_addr	=parent_td->phy_buf_addr + parent_td->isoch_packet_desc_p[0].isoch_packiet_start_addr;
		parent_td->cur_stransfer.start_vir_buf_addr	=parent_td->vir_buf_addr + parent_td->isoch_packet_desc_p[0].isoch_packiet_start_addr;
	}
	else
	{
		parent_td->cur_stransfer.buf_size		=(parent_td->buf_size>MAX_CH_TRANSFER_SIZE)
							?MAX_CH_TRANSFER_SIZE
							:parent_td->buf_size;

		parent_td->cur_stransfer.start_phy_buf_addr	=parent_td->phy_buf_addr;
		parent_td->cur_stransfer.start_vir_buf_addr	=parent_td->vir_buf_addr;
	}

	parent_td->cur_stransfer.packet_cnt = calc_packet_cnt(parent_td->cur_stransfer.buf_size, parent_td->parent_ed_p->ed_desc.max_packet_size);

	return USB_ERR_SUCCESS;
}


/******************************************************************************/
/*!
 * @name	void  	update_perio_stransfer(td_t *parent_td)
 *
 * @brief		this function updates the parent_td->cur_stransfer to be used by S3COCI.
 *			the STransfer of parent_td is for Periodic Transfer.
 *
 * @param	[IN/OUT]parent_td	= indicates the pointer of td_t to store the STranser to be updated.
 *
 * @return	USB_ERR_SUCCESS	-if success to update the parent_td->cur_stransfer.
 *		USB_ERR_FAIL		-if fail to update the parent_td->cur_stransfer.
 */
 /******************************************************************************/
void update_perio_stransfer(td_t *parent_td)
{
	switch(parent_td->parent_ed_p->ed_desc.endpoint_type) {
	case INT_TRANSFER:
		parent_td->cur_stransfer.start_phy_buf_addr = parent_td->phy_buf_addr+parent_td->transferred_szie;
		parent_td->cur_stransfer.start_vir_buf_addr = parent_td->vir_buf_addr+parent_td->transferred_szie;
		parent_td->cur_stransfer.buf_size = (parent_td->buf_size>MAX_CH_TRANSFER_SIZE)
							?MAX_CH_TRANSFER_SIZE :parent_td->buf_size;
		break;

	case ISOCH_TRANSFER:
		parent_td->cur_stransfer.start_phy_buf_addr = parent_td->phy_buf_addr
							+parent_td->isoch_packet_desc_p[parent_td->isoch_packet_index].isoch_packiet_start_addr
							+parent_td->isoch_packet_position;

		parent_td->cur_stransfer.start_vir_buf_addr = parent_td->vir_buf_addr
							+parent_td->isoch_packet_desc_p[parent_td->isoch_packet_index].isoch_packiet_start_addr
							+parent_td->isoch_packet_position;

		parent_td->cur_stransfer.buf_size = (parent_td->isoch_packet_desc_p[parent_td->isoch_packet_index].buf_size - parent_td->isoch_packet_position)>MAX_CH_TRANSFER_SIZE
							?MAX_CH_TRANSFER_SIZE
							:parent_td->isoch_packet_desc_p[parent_td->isoch_packet_index].buf_size - parent_td->isoch_packet_position;

		break;

	default:
		break;
	}
}

