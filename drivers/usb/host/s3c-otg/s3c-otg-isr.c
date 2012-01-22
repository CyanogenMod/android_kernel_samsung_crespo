/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : Isr.c
 *  [Description] : The file implement the external and internal functions of ISR
 *  [Author]      : Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *  [Department]  : System LSI Division/Embedded S/W Platform
 *  [Created Date]: 2009/02/10
 *  [Revision History]
 *	  (1) 2008/06/13   by Jang Kyu Hyeok { kyuhyeok.jang@samsung.com }
 *          - Created this file and implements functions of ISR
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

#include "s3c-otg-isr.h"

/**
 * void otg_handle_interrupt(void)
 *
 * @brief Main interrupt processing routine
 *
 * @param None
 *
 * @return None
 *
 * @remark
 *
 */

__inline__ void otg_handle_interrupt(struct usb_hcd *hcd)
{
	gintsts_t clearIntr = {.d32 = 0};
	gintsts_t gintsts = {.d32 = 0};
	struct sec_otghost *otghost = hcd_to_sec_otghost(hcd);

	gintsts.d32 = read_reg_32(GINTSTS) & read_reg_32(GINTMSK);

	otg_dbg(OTG_DBG_ISR, "otg_handle_interrupt - GINTSTS=0x%8x\n",
		      				 	gintsts.d32);

	if (gintsts.b.wkupintr) {
		otg_dbg(true, "Wakeup Interrupt\n");
		clearIntr.b.wkupintr = 1;
	}

	if (gintsts.b.disconnect) {
		otg_dbg(true, "Disconnect  Interrupt\n");
		otghost->port_flag.b.port_connect_status_change = 1;
		otghost->port_flag.b.port_connect_status = 0;
		clearIntr.b.disconnect = 1;
		/*
		wake_unlock(&otghost->wake_lock);
		*/
	}

	if (gintsts.b.conidstschng) {
		otg_dbg(OTG_DBG_ISR, "Connect ID Status Change Interrupt\n");
		clearIntr.b.conidstschng = 1;
		oci_init_mode();
	}

	if (gintsts.b.hcintr) {
		/* Mask Channel Interrupt to prevent generating interrupt */
		otg_dbg(OTG_DBG_ISR, "Channel Interrupt\n");
		if(!otghost->ch_halt) {
			do_transfer_checker(otghost);
		}
	}

	if (gintsts.b.portintr) {
		/* Read Only */
		otg_dbg(true, "Port Interrupt\n");
		process_port_intr(hcd);
	}


	if (gintsts.b.otgintr) {
		/* Read Only */
		otg_dbg(OTG_DBG_ISR, "OTG Interrupt\n");
	}

	if (gintsts.b.sofintr) {
		/* otg_dbg(OTG_DBG_ISR, "SOF Interrupt\n"); */
		do_schedule(otghost);
		clearIntr.b.sofintr = 1;
	}

	if (gintsts.b.modemismatch) {
		otg_dbg(OTG_DBG_ISR, "Mode Mismatch Interrupt\n");
		clearIntr.b.modemismatch = 1;
	}
	update_reg_32(GINTSTS, clearIntr.d32);
}

/**
 * void mask_channel_interrupt(u32 ch_num, u32 mask_info)
 *
 * @brief Mask specific channel interrupt
 *
 * @param [IN] chnum : channel number for masking
 *	   [IN] mask_info : mask information to write register
 *
 * @return None
 *
 * @remark
 *
 */
void mask_channel_interrupt(u32 ch_num, u32 mask_info)
{
	clear_reg_32(HCINTMSK(ch_num),mask_info);
}

/**
 * void unmask_channel_interrupt(u32 ch_num, u32 mask_info)
 *
 * @brief Unmask specific channel interrupt
 *
 * @param [IN] chnum : channel number for unmasking
 *	   [IN] mask_info : mask information to write register
 *
 * @return None
 *
 * @remark
 *
 */
void unmask_channel_interrupt(u32	ch_num, u32 mask_info)
{
	update_reg_32(HCINTMSK(ch_num),mask_info);
}

/**
 * int get_ch_info(hc_info_t * hc_reg, u8 ch_num)
 *
 * @brief Get current channel information about specific channel
 *
 * @param [OUT] hc_reg : structure to write channel inforamtion value
 *	   [IN] ch_num : channel number for unmasking
 *
 * @return None
 *
 * @remark
 *
 */
int get_ch_info(hc_info_t *hc_reg, u8 ch_num)
{
	if(hc_reg !=NULL) {
		hc_reg->hc_int_msk.d32 	= read_reg_32(HCINTMSK(ch_num));
		hc_reg->hc_int.d32 	= read_reg_32(HCINT(ch_num));
		hc_reg->dma_addr 	= read_reg_32(HCDMA(ch_num));
		hc_reg->hc_char.d32 	= read_reg_32(HCCHAR(ch_num));
		hc_reg->hc_size.d32 	= read_reg_32(HCTSIZ(ch_num));

		return USB_ERR_SUCCESS;
	}
	return USB_ERR_FAIL;
}

/**
 * void get_intr_ch(u32* haint, u32* haintmsk)
 *
 * @brief Get Channel Interrupt Information in HAINT, HAINTMSK register
 *
 * @param [OUT] haint : HAINT register value
 *	   [OUT] haintmsk : HAINTMSK register value
 *
 * @return None
 *
 * @remark
 *
 */
void get_intr_ch(u32 *haint, u32 *haintmsk)
{
	*haint = read_reg_32(HAINT);
	*haintmsk = read_reg_32(HAINTMSK);
}

/**
 * void clear_ch_intr(u8 ch_num, u32 clear_bit)
 *
 * @brief Get Channel Interrupt Information in HAINT, HAINTMSK register
 *
 * @param [IN] haint : HAINT register value
 *	   [IN] haintmsk : HAINTMSK register value
 *
 * @return None
 *
 * @remark
 *
 */
void clear_ch_intr(u8 ch_num, u32 clear_bit)
{
	update_reg_32(HCINT(ch_num),clear_bit);
}

/**
 * void enable_sof(void)
 *
 * @brief Generate SOF Interrupt.
 *
 * @param None
 *
 * @return None
 *
 * @remark
 *
 */
void enable_sof(void)
{
	gintmsk_t gintmsk = {.d32 = 0};
	gintmsk.b.sofintr = 1;
	update_reg_32(GINTMSK, gintmsk.d32);
}

/**
 *  void disable_sof(void)
 *
 * @brief Stop to generage SOF interrupt
 *
 * @param None
 *
 * @return None
 *
 * @remark
 *
 */
void disable_sof(void)
{
	gintmsk_t gintmsk = {.d32 = 0};
	gintmsk.b.sofintr = 1;
	clear_reg_32(GINTMSK, gintmsk.d32);
}

/*Internal function of isr */
void process_port_intr(struct usb_hcd *hcd)
{
	hprt_t	hprt; /* by ss1, clear_hprt; */
	struct sec_otghost *otghost = hcd_to_sec_otghost(hcd);

	hprt.d32 = read_reg_32(HPRT);

	otg_dbg(OTG_DBG_ISR, "Port Interrupt() : HPRT = 0x%x\n",hprt.d32);

	if(hprt.b.prtconndet) {
		otg_dbg(true, "detect connection");

		otghost->port_flag.b.port_connect_status_change = 1;

		if(hprt.b.prtconnsts)
			otghost->port_flag.b.port_connect_status = 1;

		/* wake_lock(&otghost->wake_lock); */
	}


	if(hprt.b.prtenchng) {
		otg_dbg(true, "port enable/disable changed\n");
		otghost->port_flag.b.port_enable_change = 1;
	}

	if(hprt.b.prtovrcurrchng) {
		otg_dbg(true, "over current condition is changed\n");
		if(hprt.b.prtovrcurract) {
			otg_dbg(true, "port_over_current_change = 1\n");
			otghost->port_flag.b.port_over_current_change = 1;
		}
		else {
			otghost->port_flag.b.port_over_current_change = 0;
		}
		/* defer otg power control into a kernel thread */
		queue_work(otghost->wq, &otghost->work);
	}

	hprt.b.prtena = 0; /* prtena를 writeclear시키면 안됨. */
	/* hprt.b.prtpwr = 0; */
	hprt.b.prtrst = 0;
	hprt.b.prtconnsts = 0;

	write_reg_32(HPRT, hprt.d32);
}
