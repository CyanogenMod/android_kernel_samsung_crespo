/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : TransferReadyQ.c
 *  [Description] : The source file implements the internal functions of TransferReadyQ.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/04
 *  [Revision History]
 *      (1) 2008/06/04   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements functions of TransferReadyQ.
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

#include "s3c-otg-scheduler-scheduler.h"

static	trans_ready_q_t	periodic_trans_ready_q;
static	trans_ready_q_t	nonperiodic_trans_ready_q;

/******************************************************************************/
/*!
 * @name	void		init_transfer_ready_q(void)
 *
 * @brief		this function initiates PeriodicTransferReadyQ and NonPeriodicTransferReadyQ.
 *
 *
 * @param	void
 *
 * @return	void.
 */
/******************************************************************************/
void	init_transfer_ready_q(void)
{
	otg_dbg(OTG_DBG_SCHEDULE,"start init_transfer_ready_q\n");

	otg_list_init(&periodic_trans_ready_q.trans_ready_q_list_head);
	periodic_trans_ready_q.is_periodic_transfer 		= 	true;
	periodic_trans_ready_q.trans_ready_entry_num 	= 	0;
	periodic_trans_ready_q.total_alloc_chnum		=	0;
	periodic_trans_ready_q.total_perio_bus_bandwidth	=	0;

	otg_list_init(&nonperiodic_trans_ready_q.trans_ready_q_list_head);
	nonperiodic_trans_ready_q.is_periodic_transfer 		= 	false;
	nonperiodic_trans_ready_q.trans_ready_entry_num 	=	0;
	nonperiodic_trans_ready_q.total_alloc_chnum		=	0;
	nonperiodic_trans_ready_q.total_perio_bus_bandwidth	=	0;


}


/******************************************************************************/
/*!
 * @name	int   	insert_ed_to_ready_q(ed_t	*insert_ed,
 *					bool	f_isfirst)
 *
 * @brief		this function inserts ed_t * to TransferReadyQ.
 *
 *
 * @param	[IN]	insert_ed	= indicates the ed_t to be inserted to TransferReadyQ.
 *		[IN]	f_isfirst	= indicates whether the insert_ed is inserted as first entry of TransferReadyQ.
 *
 * @return	USB_ERR_SUCCESS	-if successes to insert the insert_ed to TransferReadyQ.
 *		USB_ERR_FAILl		-if fails to insert the insert_ed to TransferReadyQ.
 */
/******************************************************************************/
int   	insert_ed_to_ready_q(ed_t 	*insert_ed,
				bool 	f_isfirst)
{

	if(insert_ed->ed_desc.endpoint_type == BULK_TRANSFER||
		insert_ed->ed_desc.endpoint_type == CONTROL_TRANSFER)
	{
		if(f_isfirst)
		{
			otg_list_push_next(&insert_ed->trans_ready_q_list_entry,&nonperiodic_trans_ready_q.trans_ready_q_list_head);
		}
		else
		{
			otg_list_push_prev(&insert_ed->trans_ready_q_list_entry,&nonperiodic_trans_ready_q.trans_ready_q_list_head);
		}
		nonperiodic_trans_ready_q.trans_ready_entry_num++;
	}
	else
	{
		if(f_isfirst)
		{
			otg_list_push_next(&insert_ed->trans_ready_q_list_entry,&periodic_trans_ready_q.trans_ready_q_list_head);
		}
		else
		{
			otg_list_push_prev(&insert_ed->trans_ready_q_list_entry,&periodic_trans_ready_q.trans_ready_q_list_head);
		}
		periodic_trans_ready_q.trans_ready_entry_num++;
	}

	return USB_ERR_SUCCESS;

}


u32	get_periodic_ready_q_entity_num(void)
{
	return periodic_trans_ready_q.trans_ready_entry_num;
}
/******************************************************************************/
/*!
 * @name	int   	remove_ed_from_ready_q(ed_t	*remove_ed)
 *
 * @brief		this function removes ed_t * from TransferReadyQ.
 *
 *
 * @param	[IN]	remove_ed = indicate the ed_t to be removed from TransferReadyQ.
 *
 * @return	USB_ERR_SUCCESS	-if successes to remove the remove_ed from TransferReadyQ.
 *		USB_ERR_FAILl		-if fails to remove the remove_ed from TransferReadyQ.
 */
/******************************************************************************/
int   	remove_ed_from_ready_q(ed_t	*remove_ed)
{
//	SPINLOCK_t	SLForRemoveED_t = SPIN_LOCK_INIT;
//	u32		uiSLFlag=0;

	otg_list_pop(&remove_ed->trans_ready_q_list_entry);

	if(remove_ed->ed_desc.endpoint_type == BULK_TRANSFER||
		remove_ed->ed_desc.endpoint_type == CONTROL_TRANSFER)
	{
//		spin_lock_irq_save_otg(&SLForRemoveED_t, uiSLFlag);
//		otg_list_pop(&remove_ed->trans_ready_q_list_entry);
		nonperiodic_trans_ready_q.trans_ready_entry_num--;
//		spin_unlock_irq_save_otg(&SLForRemoveED_t, uiSLFlag);
	}
	else
	{
//		spin_lock_irq_save_otg(&SLForRemoveED_t, uiSLFlag);
//		otg_list_pop(&remove_ed->trans_ready_q_list_entry);
		periodic_trans_ready_q.trans_ready_entry_num--;
//		spin_unlock_irq_save_otg(&SLForRemoveED_t, uiSLFlag);
	}

	return USB_ERR_SUCCESS;

}

//by ss1 unused func
/*
bool   	check_ed_on_ready_q(ed_t	*check_ed_p)
{

	if(check_ed_p->ed_status.is_in_transfer_ready_q)
		return true;
	else
		return false;
}*/

/******************************************************************************/
/*!
 * @name	int   	get_ed_from_ready_q(bool	f_isperiodic,
 *					td_t 	**get_ed)
 *
 * @brief		this function returns the first entity of TransferReadyQ.
 *			if there are some ed_t on TransferReadyQ, this function pops first ed_t from TransferReadyQ.
 *			So, the TransferReadyQ don's has the poped ed_t.
 *
 *
 * @param	[IN]	f_isperiodic	= indicate whether Periodic or not
 *		[OUT]	get_ed		= indicate the double pointer to store the address of first entity
 *								on TransferReadyQ.
 *
 * @return	USB_ERR_SUCCESS	-if successes to get frist ed_t from TransferReadyQ.
 *		USB_ERR_NO_ENTITY	-if fails to get frist ed_t from TransferReadyQ
 *					because there is no entity on TransferReadyQ.
 */
/******************************************************************************/

int	get_ed_from_ready_q(bool	f_isperiodic,
				ed_t 	**get_ed)
{
	if(f_isperiodic)
	{
		otg_list_head	*transreadyq_list_entity=NULL;

		if(periodic_trans_ready_q.trans_ready_entry_num==0)
		{
			return USB_ERR_NO_ENTITY;
		}

		transreadyq_list_entity = periodic_trans_ready_q.trans_ready_q_list_head.next;

		//if(transreadyq_list_entity!= &periodic_trans_ready_q.trans_ready_q_list_head)
		if(!otg_list_empty(&periodic_trans_ready_q.trans_ready_q_list_head))
		{
			*get_ed = otg_list_get_node(transreadyq_list_entity,ed_t,trans_ready_q_list_entry);
			if (transreadyq_list_entity->prev == LIST_POISON2 ||
					transreadyq_list_entity->next == LIST_POISON1) {
				printk(KERN_ERR "s3c-otg-scheduler get_ed_from_ready_q error\n");
				periodic_trans_ready_q.trans_ready_entry_num =0;
			}
			else {
				otg_list_pop(transreadyq_list_entity);
				periodic_trans_ready_q.trans_ready_entry_num--;
			}

			return USB_ERR_SUCCESS;
		}
		else
		{
			return USB_ERR_NO_ENTITY;
		}
	}
	else
	{
		otg_list_head	*transreadyq_list_entity=NULL;

		if(nonperiodic_trans_ready_q.trans_ready_entry_num==0)
		{
			return USB_ERR_NO_ENTITY;
		}

		transreadyq_list_entity = nonperiodic_trans_ready_q.trans_ready_q_list_head.next;

		//if(transreadyq_list_entity!= &nonperiodic_trans_ready_q.trans_ready_q_list_head)
		if(!otg_list_empty(&nonperiodic_trans_ready_q.trans_ready_q_list_head))
		{
			*get_ed = otg_list_get_node(transreadyq_list_entity,ed_t, trans_ready_q_list_entry);
			if (transreadyq_list_entity->prev == LIST_POISON2 ||
					transreadyq_list_entity->next == LIST_POISON1) {
				printk(KERN_ERR "s3c-otg-scheduler get_ed_from_ready_q error\n");
				nonperiodic_trans_ready_q.trans_ready_entry_num =0;
			}
			else {
				otg_list_pop(transreadyq_list_entity);
				nonperiodic_trans_ready_q.trans_ready_entry_num--;
			}

			return USB_ERR_SUCCESS;
		}
		else
		{
			return USB_ERR_NO_ENTITY;
		}
	}
}


