/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __WIMAX_CMC732_H
#define __WIMAX_CMC732_H

#ifdef __KERNEL__

#define WIMAX_POWER_SUCCESS		0
#define WIMAX_ALREADY_POWER_ON		-1
#define WIMAX_PROBE_FAIL		-2
#define WIMAX_ALREADY_POWER_OFF		-3

/* wimax mode */
enum {
	SDIO_MODE = 0,
	WTM_MODE,
	MAC_IMEI_WRITE_MODE,
	USIM_RELAY_MODE,
	DM_MODE,
	USB_MODE,
	AUTH_MODE
};


struct wimax_cfg {
	int			temp_tgid;	/* handles unexpected close */
	struct wake_lock	wimax_wake_lock;	/* resume wake lock */
	struct wake_lock	wimax_rxtx_lock;/* sdio wake lock */
	u8		wimax_mode;/* wimax mode (SDIO, USB, etc..) */
	u8		sleep_mode;/* suspend mode (0: VI, 1: IDLE) */
	u8		card_removed;/*
						 * set if host has acknowledged
						 * card removal
						 */
};

struct wimax732_platform_data {
	int (*power) (int);
	void (*set_mode) (void);
	void (*signal_ap_active) (int);
	int (*get_sleep_mode) (void);
	int (*is_modem_awake) (void);
	void (*switch_uart_ap) (void);
	void (*wakeup_assert) (int);
	struct wimax_cfg *g_cfg;
	int wimax_int;
};

#endif

#endif
