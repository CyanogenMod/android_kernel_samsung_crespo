/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : Scheduler.h
 *  [Description] : The Header file defines the external and internal functions of Scheduler.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and defines functions of Scheduler
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

#ifndef  _SCHEDULER_H
#define  _SCHEDULER_H

/*
// ----------------------------------------------------------------------------
// Include files : None.
// ----------------------------------------------------------------------------
*/
//#include "s3c-otg-common-typedef.h"
#include "s3c-otg-common-const.h"
#include "s3c-otg-common-errorcode.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-hcdi-memory.h"
#include "s3c-otg-hcdi-kal.h"
#include "s3c-otg-hcdi-debug.h"
#include "s3c-otg-oci.h"

#ifdef __cplusplus
extern "C"
{
#endif

//Defines external functions of IScheduler.c
extern void init_scheduler(void);
extern int reserve_used_resource_for_periodic(u32	usb_time,u8 dev_speed, u8	trans_type);
extern int free_usb_resource_for_periodic(u32	free_usb_time, u8	free_chnum, u8	trans_type);
extern int remove_ed_from_scheduler(ed_t	*remove_ed);
extern int cancel_to_transfer_td(struct sec_otghost *otghost, td_t *cancel_td);
extern int retransmit(struct sec_otghost *otghost, td_t *retransmit_td);
extern int reschedule(td_t *resched_td);
extern int deallocate(td_t *dealloc_td);
extern void do_schedule(struct sec_otghost *otghost);
extern int get_td_info(u8 chnum,unsigned int *td_addr);

// Defines functiions of TranferReadyQ.
void init_transfer_ready_q(void);
int insert_ed_to_ready_q(ed_t *insert_ed, bool f_isfirst);
int remove_ed_from_ready_q(ed_t *remove_ed);
int get_ed_from_ready_q(bool f_isperiodic, ed_t **get_ed);

//Define functions of Scheduler
void do_periodic_schedule(struct sec_otghost *otghost);
void do_nonperiodic_schedule(struct sec_otghost *otghost);
int set_transferring_td_array(u8 chnum, u32 td_addr);
int get_transferring_td_array(u8 chnum, unsigned int *td_addr);


//Define fuctions to manage some static global variable.
int inc_perio_bus_time(u32 uiBusTime, u8 dev_speed);
int dec_perio_bus_time(u32 uiBusTime);

u8 get_avail_chnum(void);
int inc_perio_chnum(void);
int dec_perio_chnum(void);
int inc_non_perio_chnum(void);
int dec_nonperio_chnum(void);
u32 get_periodic_ready_q_entity_num(void);

int insert_ed_to_scheduler(struct sec_otghost *otghost, ed_t *insert_ed);

void reset_scheduler_numbers(void);

#ifdef __cplusplus
}
#endif


#endif

