/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : s3c-otg-transfer-transfer.h
 *  [Description] : The Header file defines the external and internal functions of Transfer.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and defines functions of Transfer
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

#ifndef  _TRANSFER_H
#define  _TRANSFER_H

/*
// ----------------------------------------------------------------------------
// Include files : None.
// ----------------------------------------------------------------------------
*/
//#include "s3c-otg-common-const.h"
#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-hcdi-memory.h"
#include "s3c-otg-hcdi-kal.h"
#include "s3c-otg-hcdi-debug.h"

#include "s3c-otg-scheduler-scheduler.h"
#include "s3c-otg-isr.h"


#ifdef __cplusplus
extern "C"
{
#endif

void	init_transfer(void);
void	deinit_transfer(struct sec_otghost *otghost);

int  	issue_transfer(struct sec_otghost *otghost,
			ed_t 			*parent_ed,
			void 			*call_back_func,
			void 			*call_back_param,
			u32			transfer_flag,
			bool			f_is_standard_dev_req,
			u32			setup_vir_addr,
			u32                      		setup_phy_addr,
			u32                      		vir_buf_addr,
			u32                      		phy_buf_addr,
			u32                      		buf_size,
			u32                     		start_frame,
			u32                      		isoch_packet_num,
			isoch_packet_desc_t 	*isoch_packet_desc,
			void                         	*td_priv,
			unsigned int             	*return_td_addr);

int  	cancel_transfer(struct sec_otghost *otghost,
			ed_t 	*parent_ed,
			td_t 	*cancel_td);

int  	cancel_all_td(struct sec_otghost *otghost, ed_t 	*parent_ed);

int 	create_ed(ed_t **	new_ed);

int  	init_ed(ed_t *	init_ed,
		u8      	dev_addr,
		u8      	ep_num,
		bool        f_is_ep_in,
		u8      	dev_speed,
		u8      	ep_type,
		u16    	max_packet_size,
		u8      	multi_count,
		u8      	interval,
		u32    	sched_frame,
		u8     	hub_addr,
		u8     	hub_port,
		bool		f_is_do_split,
		void	*ep);

int  	delete_ed(struct sec_otghost *otghost, ed_t *delete_ed);

int  	delete_td(struct sec_otghost *otghost, td_t * delete_td);

int 	create_isoch_packet_desc(	isoch_packet_desc_t 	**new_isoch_packet_desc,
					u32   			isoch_packet_num);

int 	delete_isoch_packet_desc(	isoch_packet_desc_t 	*del_isoch_packet_desc,
					u32			isoch_packet_num);


void  	init_isoch_packet_desc(  isoch_packet_desc_t 		*init_isoch_packet_desc,
				u32	       			offset,
				u32                  			isoch_packet_size,
				u32				index);

// NonPeriodicTransfer.c implements.

int  	init_nonperio_stransfer(bool	f_is_standard_dev_req,
				td_t 	*parent_td);

void  	update_nonperio_stransfer(td_t 	*parent_td);

//PeriodicTransfer.c implements

int  	init_perio_stransfer(	bool	f_is_isoch_transfer,
				td_t 	*parent_td);

void  	update_perio_stransfer(td_t 	*parent_td);

static 	inline	u32	calc_packet_cnt(u32 data_size, u16 max_packet_size)
{
	if(data_size != 0)
	{
		return (data_size%max_packet_size==0)?data_size/max_packet_size:data_size/max_packet_size+1;
	}
	return 1;
}

#ifdef __cplusplus
}
#endif


#endif

