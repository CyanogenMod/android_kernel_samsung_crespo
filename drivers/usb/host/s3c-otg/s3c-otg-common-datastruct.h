/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : s3c-otg-common-datastruct.h
 *  [Description] : The Header file defines Data Structures to be used at sub-modules of S3C6400HCD.
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/03
 *  [Revision History]
 *      (1) 2008/06/03   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and defines Data Structure to be managed by Transfer.
 *      (2) 2008/08/18   by SeungSoo Yang ( ss1.yang@samsung.com )
 *          - modifying ED structure
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

#ifndef  _DATA_STRUCT_DEF_H
#define  _DATA_STRUCT_DEF_H

/*
// ----------------------------------------------------------------------------
// Include files : None.
// ----------------------------------------------------------------------------
*/

#include <linux/wakelock.h>
#include <plat/s5p-otghost.h>
//#include "s3c-otg-common-typedef.h"
#include "s3c-otg-hcdi-list.h"

//#include "s3c-otg-common-regdef.h"
//#include "s3c-otg-common-errorcode.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef union _hcintmsk_t
{
	// raw register data
	u32 d32;

	// register bits
	struct
	{
		unsigned xfercompl		: 1;
		unsigned chhltd			: 1;
		unsigned ahberr			: 1;
		unsigned stall			: 1;
		unsigned nak			: 1;
		unsigned ack			: 1;
		unsigned nyet			: 1;
		unsigned xacterr			: 1;
		unsigned bblerr			: 1;
		unsigned frmovrun		: 1;
		unsigned datatglerr		: 1;
		unsigned reserved		: 21;
	} b;
} hcintmsk_t;

typedef union _hcintn_t
{
	u32	d32;
	struct
	{
		u32	xfercompl			:1;
		u32	chhltd				:1;
		u32	abherr				:1;
		u32	stall				:1;
		u32	nak				:1;
		u32	ack				:1;
		u32	nyet				:1;
		u32	xacterr				:1;
		u32	bblerr				:1;
		u32	frmovrun			:1;
		u32	datatglerr			:1;
		u32	reserved			:21;
	}b;
}hcintn_t;


typedef	union _pcgcctl_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned	stoppclk			:1;
		unsigned	gatehclk			:1;
		unsigned	pwrclmp			:1;
		unsigned	rstpdwnmodule	:1;
		unsigned	physuspended		:1;
		unsigned	Reserved5_31		:27;
	}b;
}pcgcctl_t;


typedef struct isoch_packet_desc
{
	u32				isoch_packiet_start_addr;// start address of buffer is buffer address + uiOffsert.
	u32				buf_size;
	u32    				transferred_szie;
	u32   				isoch_status;
}isoch_packet_desc_t;//, *isoch_packet_desc_t *,**isoch_packet_desc_t **;


typedef struct  standard_dev_req_info
{
	bool            		is_data_stage;
	u8          			conrol_transfer_stage;
	u32        			vir_standard_dev_req_addr;
	u32       	 		phy_standard_dev_req_addr;
}standard_dev_req_info_t;


typedef struct control_data_tgl_t
{
	u8			setup_tgl;
	u8			data_tgl;
	u8			status_tgl;
}control_data_tgl_t;



typedef struct ed_status
{
	u8			data_tgl;
	control_data_tgl_t		control_data_tgl;
	bool			is_ping_enable;
	bool			is_in_transfer_ready_q;
	bool			is_in_transferring;
	u32			in_transferring_td;
	bool			is_alloc_resource_for_ed;
}ed_status_t;//, *ed_status_t *,**ed_status_t **;


typedef struct ed_desc
{
	u8	device_addr;
	u8	endpoint_num;
	bool	is_ep_in;
	u8	dev_speed;
	u8	endpoint_type;
	u16	max_packet_size;
	u8	mc;
	u8	interval;
	u32         sched_frame;
	u32         used_bus_time;
	u8	hub_addr;
	u8	hub_port;
	bool        is_do_split;
}ed_dest_t;//, *ed_dest_t *,**ed_dest_t **;


//Defines the Data Structures of Transfer.
typedef struct hc_reg
{

	hcintmsk_t	hc_int_msk;
	hcintn_t		hc_int;
	u32		dma_addr;

}hc_reg_t;//, *hc_reg_t *, **hc_reg_t **;


typedef struct stransfer
{
	u32		stransfer_id;
	u32		parent_td;
	ed_dest_t 	*ed_desc_p;
	ed_status_t 	*ed_status_p;
	u32		start_vir_buf_addr;
	u32		start_phy_buf_addr;
	u32		buf_size;
	u32		packet_cnt;
	u8		alloc_chnum;
	hc_reg_t		hc_reg;
}stransfer_t;//, *stransfer_t *,**stransfer_t **;


typedef struct ed
{
	u32		ed_id;
	bool		is_halted;
	bool		is_need_to_insert_scheduler;
	ed_dest_t	ed_desc;
	ed_status_t	ed_status;
	otg_list_head	ed_list_entry;
	otg_list_head	td_list_entry;
	otg_list_head	trans_ready_q_list_entry;
	u32		num_td;
	void		*ed_private;
}ed_t;//, *ed_t *, **ed_t **;



typedef struct td
{
	u32				td_id;
	ed_t				*parent_ed_p;
	void				*call_back_func_p;
	void				*call_back_func_param_p;
	bool				is_transferring;
	bool				is_transfer_done;
	u32				transferred_szie;
	bool		                   	is_standard_dev_req;
	standard_dev_req_info_t		standard_dev_req_info;
	u32				vir_buf_addr;
	u32				phy_buf_addr;
	u32				buf_size;
	u32				transfer_flag;
	stransfer_t			cur_stransfer;
	USB_ERROR_CODE		error_code;
	u32				err_cnt;
	otg_list_head			td_list_entry;

	//Isochronous Transfer Specific
	u32				isoch_packet_num;
	isoch_packet_desc_t		*isoch_packet_desc_p;
	u32				isoch_packet_index;
	u32				isoch_packet_position;
	u32				sched_frame;
	u32				interval;
	u32				used_total_bus_time;

	// the private data can be used by S3C6400Interface.
	void				*td_private;
}td_t;//, *td_t *,**td_t **;


//Define Data Structures of Scheduler.
typedef struct trans_ready_q
{
	bool		is_periodic_transfer;
	otg_list_head	trans_ready_q_list_head;
	u32		trans_ready_entry_num;

	//In case of Periodic Transfer
	u32		total_perio_bus_bandwidth;
	u8		total_alloc_chnum;
}trans_ready_q_t;//, *trans_ready_q_t *,**trans_ready_q_t **;


//Define USB OTG Reg Data Structure by Kyuhyeok.

#define MAX_COUNT 10000
#define INT_ALL	0xffffffff

typedef union _haint_t
{
	u32	d32;
	struct
	{
		u32	channel_intr_0		:1;
		u32	channel_intr_1		:1;
		u32	channel_intr_2		:1;
		u32	channel_intr_3		:1;
		u32	channel_intr_4		:1;
		u32	channel_intr_5		:1;
		u32	channel_intr_6		:1;
		u32	channel_intr_7		:1;
		u32	channel_intr_8		:1;
		u32	channel_intr_9		:1;
		u32	channel_intr_10		:1;
		u32	channel_intr_11		:1;
		u32	channel_intr_12		:1;
		u32	channel_intr_13		:1;
		u32	channel_intr_14		:1;
		u32	channel_intr_15		:1;
		u32	reserved1		:16;
	}b;
}haint_t;

typedef union _gresetctl_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned csftrst			: 1;
		unsigned hsftrst			: 1;
		unsigned hstfrm				: 1;
		unsigned intknqflsh		: 1;
		unsigned rxfflsh			: 1;
		unsigned txfflsh			: 1;
		unsigned txfnum				: 5;
		unsigned reserved11_29		: 19;
		unsigned dmareq				: 1;
		unsigned ahbidle			: 1;
	} b;
} gresetctl_t;


typedef union _gahbcfg_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned glblintrmsk		: 1;
#define GAHBCFG_GLBINT_ENABLE	1
		unsigned hburstlen			: 4;
#define INT_DMA_MODE_SINGLE		00
#define INT_DMA_MODE_INCR			01
#define INT_DMA_MODE_INCR4		03
#define INT_DMA_MODE_INCR8		05
#define INT_DMA_MODE_INCR16		07
		unsigned dmaenable			: 1;
#define GAHBCFG_DMAENABLE	1
		unsigned reserved			: 1;
		unsigned nptxfemplvl		: 1;
		unsigned ptxfemplvl		: 1;
#define GAHBCFG_TXFEMPTYLVL_EMPTY 		1
#define GAHBCFG_TXFEMPTYLVL_HALFEMPTY	0
		unsigned reserved9_31		: 22;
	} b;
} gahbcfg_t;

typedef union _gusbcfg_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned toutcal			: 3;
		unsigned phyif				: 1;
		unsigned ulpi_utmi_sel		: 1;
		unsigned fsintf				: 1;
		unsigned physel				: 1;
		unsigned ddrsel				: 1;
		unsigned srpcap				: 1;
		unsigned hnpcap				: 1;
		unsigned usbtrdtim			: 4;
		unsigned nptxfrwnden		: 1;
		unsigned phylpwrclksel		: 1;
		unsigned reserved			: 13;
		unsigned forcehstmode		: 1;
		unsigned reserved2			: 2;
	} b;
} gusbcfg_t;


typedef union _ghwcfg2_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct {
		/* GHWCFG2 */
		unsigned op_mode					: 3;
#define MODE_HNP_SRP_CAPABLE 		0
#define MODE_SRP_ONLY_CAPABLE 		1
#define MODE_NO_HNP_SRP_CAPABLE		2
#define MODE_SRP_CAPABLE_DEVICE 		3
#define MODE_NO_SRP_CAPABLE_DEVICE  4
#define MODE_SRP_CAPABLE_HOST 		5
#define MODE_NO_SRP_CAPABLE_HOST  	6

		unsigned architecture				: 2;
#define HWCFG2_ARCH_SLAVE_ONLY		0x00
#define HWCFG2_ARCH_EXT_DMA 			0x01
#define HWCFG2_ARCH_INT_DMA  		0x02

		unsigned point2point				: 1;
		unsigned hs_phy_type				: 2;
		unsigned fs_phy_type				: 2;
		unsigned num_dev_ep				: 4;
		unsigned num_host_chan				: 4;
		unsigned perio_ep_supported		: 1;
		unsigned dynamic_fifo				: 1;
		unsigned rx_status_q_depth		: 2;
		unsigned nonperio_tx_q_depth		: 2;
		unsigned host_perio_tx_q_depth	: 2;
		unsigned dev_token_q_depth		: 5;
		unsigned reserved31				: 1;
	} b;
} ghwcfg2_t;

typedef union _gintsts_t
{
	/** raw register data */
	u32 d32;
#define SOF_INTR_MASK 0x0008
	/** register bits */
	struct
	{
#define HOST_MODE			1
#define DEVICE_MODE		0
		unsigned curmode			: 1;
#define	OTG_HOST_MODE		1
#define	OTG_DEVICE_MODE	0

		unsigned modemismatch		: 1;
		unsigned otgintr			: 1;
		unsigned sofintr			: 1;
		unsigned rxstsqlvl			: 1;
		unsigned nptxfempty		: 1;
		unsigned ginnakeff			: 1;
		unsigned goutnakeff		: 1;
		unsigned reserved8			: 1;
		unsigned i2cintr			: 1;
		unsigned erlysuspend		: 1;
		unsigned usbsuspend		: 1;
		unsigned usbreset			: 1;
		unsigned enumdone			: 1;
		unsigned isooutdrop		: 1;
		unsigned eopframe			: 1;
		unsigned intokenrx			: 1;
		unsigned epmismatch		: 1;
		unsigned inepint			: 1;
		unsigned outepintr			: 1;
		unsigned incompisoin		: 1;
		unsigned incompisoout		: 1;
		unsigned reserved22_23		: 2;
		unsigned portintr			: 1;
		unsigned hcintr				: 1;
		unsigned ptxfempty			: 1;
		unsigned reserved27		: 1;
		unsigned conidstschng		: 1;
		unsigned disconnect		: 1;
		unsigned sessreqintr		: 1;
		unsigned wkupintr			: 1;
	} b;
} gintsts_t;


typedef union _hcfg_t
{
	/** raw register data */
	u32 d32;

	/** register bits */
	struct
	{
		/** FS/LS Phy Clock Select */
		unsigned fslspclksel	: 2;
#define HCFG_30_60_MHZ	0
#define HCFG_48_MHZ	    1
#define HCFG_6_MHZ		2

		/** FS/LS Only Support */
		unsigned fslssupp		: 1;
		unsigned reserved3_31		: 29;
	} b;
} hcfg_t;

typedef union _hprt_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned prtconnsts		: 1;
		unsigned prtconndet		: 1;
		unsigned prtena				: 1;
		unsigned prtenchng			: 1;
		unsigned prtovrcurract		: 1;
		unsigned prtovrcurrchng	: 1;
		unsigned prtres				: 1;
		unsigned prtsusp			: 1;
		unsigned prtrst				: 1;
		unsigned reserved9			: 1;
		unsigned prtlnsts			: 2;
		unsigned prtpwr				: 1;
		unsigned prttstctl			: 4;
		unsigned prtspd				: 2;
#define HPRT0_PRTSPD_HIGH_SPEED 0
#define HPRT0_PRTSPD_FULL_SPEED 1
#define HPRT0_PRTSPD_LOW_SPEED  2
		unsigned reserved19_31		: 13;
	} b;
} hprt_t;


typedef union _gintmsk_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned reserved0			: 1;
		unsigned modemismatch		: 1;
		unsigned otgintr			: 1;
		unsigned sofintr			: 1;
		unsigned rxstsqlvl			: 1;
		unsigned nptxfempty		: 1;
		unsigned ginnakeff			: 1;
		unsigned goutnakeff		: 1;
		unsigned reserved8			: 1;
		unsigned i2cintr			: 1;
		unsigned erlysuspend		: 1;
		unsigned usbsuspend		: 1;
		unsigned usbreset			: 1;
		unsigned enumdone			: 1;
		unsigned isooutdrop		: 1;
		unsigned eopframe			: 1;
		unsigned reserved16		: 1;
		unsigned epmismatch		: 1;
		unsigned inepintr			: 1;
		unsigned outepintr			: 1;
		unsigned incompisoin		: 1;
		unsigned incompisoout		: 1;
		unsigned reserved22_23		: 2;
		unsigned portintr			: 1;
		unsigned hcintr				: 1;
		unsigned ptxfempty			: 1;
		unsigned reserved27		: 1;
		unsigned conidstschng		: 1;
		unsigned disconnect		: 1;
		unsigned sessreqintr		: 1;
		unsigned wkupintr			: 1;
	} b;
} gintmsk_t;


typedef struct _hc_t
{

	u8  hc_num; 				// Host channel number used for register address lookup

	unsigned dev_addr		: 7; 		// Device to access
	unsigned ep_is_in		: 1;		// EP direction; 0: OUT, 1: IN

	unsigned ep_num			: 4;		// EP to access
	unsigned low_speed		: 1;		// 1: Low speed, 0: Not low speed
	unsigned ep_type		: 2;		// Endpoint type.
	// One of the following values:
	//	- OTG_EP_TYPE_CONTROL: 0
	//	- OTG_EP_TYPE_ISOC: 1
	//	- OTG_EP_TYPE_BULK: 2
	//	- OTG_EP_TYPE_INTR: 3

	unsigned rsvdb1			: 1;		// 8 bit padding

	u8 rsvd2;				// 4 byte boundary

	unsigned max_packet	: 12;	// Max packet size in bytes

	unsigned data_pid_start : 2;
#define OTG_HC_PID_DATA0 0
#define OTG_HC_PID_DATA2 1
#define OTG_HC_PID_DATA1 2
#define OTG_HC_PID_MDATA 3
#define OTG_HC_PID_SETUP 3

	unsigned multi_count	: 2;	// Number of periodic transactions per (micro)frame


	// Flag to indicate whether the transfer has been started. Set to 1 if
	// it has been started, 0 otherwise.
	u8 xfer_started;


	// Set to 1 to indicate that a PING request should be issued on this
	// channel. If 0, process normally.
	u8	do_ping;

	// Set to 1 to indicate that the error count for this transaction is
	// non-zero. Set to 0 if the error count is 0.
	u8 error_state;
	u32 *xfer_buff;		// Pointer to the current transfer buffer position.
	u16 start_pkt_count;	// Packet count at start of transfer.

	u32 xfer_len;		// Total number of bytes to transfer.
	u32 xfer_count;		// Number of bytes transferred so far.


	// Set to 1 if the host channel has been halted, but the core is not
	// finished flushing queued requests. Otherwise 0.
	u8 halt_pending;
	u8 halt_status;	// Reason for halting the host channel
	u8 short_read;		// Set when the host channel does a short read.
	u8 rsvd3;			// 4 byte boundary

} hc_t;


// Port status for the HC
#define	HCD_DRIVE_RESET		0x0001
#define	HCD_SEND_SETUP		0x0002

#define	HC_MAX_PKT_COUNT 		511
#define	HC_MAX_TRANSFER_SIZE	65535
#define	MAXP_SIZE_64BYTE 		64
#define	MAXP_SIZE_512BYTE 	512
#define	MAXP_SIZE_1024BYTE	1024

typedef union _hcchar_t
{
	// raw register data
	u32 d32;

	// register bits
	struct
	{
		// Maximum packet size in bytes
		unsigned mps		: 11;

		// Endpoint number
		unsigned epnum		: 4;

		// 0: OUT, 1: IN
		unsigned epdir		: 1;
#define HCDIR_OUT				0
#define HCDIR_IN				1

		unsigned reserved	: 1;

		// 0: Full/high speed device, 1: Low speed device
		unsigned lspddev	: 1;

		// 0: Control, 1: Isoc, 2: Bulk, 3: Intr
		unsigned eptype		: 2;
#define OTG_EP_TYPE_CONTROL	0
#define OTG_EP_TYPE_ISOC		1
#define OTG_EP_TYPE_BULK		2
#define OTG_EP_TYPE_INTR		3

		// Packets per frame for periodic transfers. 0 is reserved.
		unsigned multicnt	: 2;

		// Device address
		unsigned devaddr	: 7;

		// Frame to transmit periodic transaction.
		// 0: even, 1: odd
		unsigned oddfrm		: 1;

		// Channel disable
		unsigned chdis		: 1;

		// Channel enable
		unsigned chen		: 1;
	} b;
} hcchar_t;

typedef union _hctsiz_t
{
	// raw register data
	u32 d32;

	// register bits
	struct
	{
		// Total transfer size in bytes
		unsigned xfersize	: 19;

		// Data packets to transfer
		unsigned pktcnt		: 10;

		// Packet ID for next data packet
		// 0: DATA0
		// 1: DATA2
		// 2: DATA1
		// 3: MDATA (non-Control), SETUP (Control)
		unsigned pid		: 2;
#define HCTSIZ_DATA0		0
#define HCTSIZ_DATA1		2
#define HCTSIZ_DATA2		1
#define HCTSIZ_MDATA		3
#define HCTSIZ_SETUP		3

		// Do PING protocol when 1
		unsigned dopng		: 1;
	} b;
} hctsiz_t;



typedef union _grxstsr_t
{
	// raw register data
	u32 d32;

	// register bits
	struct
	{
		unsigned chnum		: 4;
		unsigned bcnt		: 11;
		unsigned dpid		: 2;
		unsigned pktsts		: 4;
		unsigned Reserved		: 11;
	} b;
} grxstsr_t;

typedef union _hfir_t
{
	// raw register data
	u32 d32;

	// register bits
	struct
	{
		unsigned frint		: 16;
		unsigned Reserved		: 16;
	} b;
} hfir_t;

typedef union _hfnum_t
{
	// raw register data
	u32 d32;

	// register bits
	struct
	{
		unsigned frnum : 16;
#define HFNUM_MAX_FRNUM 0x3FFF
		unsigned frrem : 16;
	} b;
} hfnum_t;

typedef union grstctl_t
{
	/** raw register data */
	u32 d32;
	/** register bits */
	struct
	{
		unsigned csftrst		: 1;
		unsigned hsftrst		: 1;
		unsigned hstfrm			: 1;
		unsigned intknqflsh	: 1;
		unsigned rxfflsh		: 1;
		unsigned txfflsh		: 1;
		unsigned txfnum			: 5;
		unsigned reserved11_29	: 19;
		unsigned dmareq			: 1;
		unsigned ahbidle		: 1;
	} b;
} grstctl_t;

typedef	struct	hc_info
{
	hcintmsk_t			hc_int_msk;
	hcintn_t				hc_int;
	u32				dma_addr;
	hcchar_t				hc_char;
	hctsiz_t				hc_size;
}hc_info_t;//, *hc_info_t *, **hc_info_t **;

#ifndef USB_MAXCHILDREN
	#define USB_MAXCHILDREN (31)
#endif

typedef struct _usb_hub_descriptor_t
{
	u8  desc_length;
	u8  desc_type;
	u8  port_number;
	u16 hub_characteristics;
	u8  power_on_to_power_good;
	u8  hub_control_current;
	    	/* add 1 bit for hub status change; round to bytes */
	u8  DeviceRemovable[(USB_MAXCHILDREN + 1 + 7) / 8];
	u8  port_pwr_ctrl_mask[(USB_MAXCHILDREN + 1 + 7) / 8];
}usb_hub_descriptor_t;

#ifdef __cplusplus
}
#endif

#endif
