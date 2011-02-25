/*
 * send.c
 *
 *
 *
 */
#include "headers.h"
#include "download.h"

static int hw_sdio_write_bank_index(struct net_adapter *adapter, int *write_idx)
{
	int ret = 0;

	*write_idx = sdio_readb(adapter->func, SDIO_H2C_WP_REG, &ret);
	if (ret)
		return ret;

	if (((*write_idx + 1) % 15) == sdio_readb(adapter->func,
							SDIO_H2C_RP_REG, &ret))
		*write_idx = -1;

	return ret;
}

u32 sd_send(struct net_adapter *adapter, u8 *buffer, u32 len)
{
	int nRet;

	int nWriteIdx;

	len += (len & 1) ? 1 : 0;

	if (adapter->halted || adapter->removed) {
		pr_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}
	sdio_claim_host(adapter->func);
	nRet = hw_sdio_write_bank_index(adapter, &nWriteIdx);

	if (nRet || (nWriteIdx < 0)) {
		pr_debug("sd_send : error occurred during	\
				fetch bank index!! nRet = %d, nWriteIdx = %d",
				nRet, nWriteIdx);
		return STATUS_UNSUCCESSFUL;
	}

	sdio_writeb(adapter->func, (nWriteIdx + 1) % 15, SDIO_H2C_WP_REG, NULL);
	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR+(SDIO_BANK_SIZE * nWriteIdx)+4,
			buffer, len);

	if (nRet < 0)
		pr_debug("sd_send : error in sending packet!! nRet = %d",
				nRet);

	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR + (SDIO_BANK_SIZE * nWriteIdx),
			&len, 4);
	sdio_release_host(adapter->func);
	if (nRet < 0)
		pr_debug("sd_send : error in writing bank length!! nRet = %d",
				nRet);

	return nRet;
}


/* get MAC address from device */
int wimax_hw_get_mac_address(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct hw_private_packet	req;
	int				nResult = 0;
	int				retry = 4;

	req.id0 = 'W';
	req.id1 = 'P';
	req.code = HWCODEMACREQUEST;
	req.value = 0;

	pr_debug("wait for SDIO ready..");
	msleep(1700); /*
					*  IMPORTANT! wait for cmc720 can
					* handle mac req packet
					*/
	do {

		nResult = sd_send(adapter, (u8 *)&req,
				sizeof(struct hw_private_packet));

		if (nResult != STATUS_SUCCESS)
			pr_debug("wimax_hw_get_mac_address: sd_send fail!!");
		msleep(300);

		}
	while ((!adapter->mac_ready) && (adapter) && (retry--));

	do_exit(0);

	return 0;
}

u32 hw_send_data(struct net_adapter *adapter, void *buffer , u32 length)
{
	struct buffer_descriptor	*dsc;
	struct hw_packet_header		*hdr;
	struct net_device		*net = adapter->net;
	u8				*ptr;

	g_pdata->wakeup_assert(1);
	dsc = (struct buffer_descriptor *)
		kmalloc(sizeof(struct buffer_descriptor), GFP_ATOMIC);
	if (dsc == NULL)
		return STATUS_RESOURCES;

	dsc->buffer = kmalloc(BUFFER_DATA_SIZE, GFP_ATOMIC);
	if (dsc->buffer == NULL) {
		kfree(dsc);
		return STATUS_RESOURCES;
	}

	ptr = dsc->buffer;

	/* shift data pointer */
	ptr += sizeof(struct hw_packet_header);
#ifdef HARDWARE_USE_ALIGN_HEADER
	ptr += 2;
#endif
	hdr = (struct hw_packet_header *)dsc->buffer;

	length -= (ETHERNET_ADDRESS_LENGTH * 2);
	buffer += (ETHERNET_ADDRESS_LENGTH * 2);

	memcpy(ptr, buffer, length);

	hdr->id0 = 'W';
	hdr->id1 = 'D';
	hdr->length = (u16)length;

	dsc->length = length + sizeof(struct hw_packet_header);
#ifdef HARDWARE_USE_ALIGN_HEADER
	dsc->length += 2;
#endif

	/* add statistics */
	adapter->netstats.tx_packets++;
	adapter->netstats.tx_bytes += dsc->length;

	spin_lock(&adapter->hw.q_send.lock);
	queue_put_tail(adapter->hw.q_send.head, dsc->node);
	spin_unlock(&adapter->hw.q_send.lock);

	wake_up_interruptible(&adapter->send_event);
	if (!netif_running(net))
		pr_debug("!netif_running");

	return STATUS_SUCCESS;
}

static u32 sd_send_data(struct net_adapter *adapter,
		struct buffer_descriptor *dsc)
{

#ifdef HARDWARE_USE_ALIGN_HEADER
	if (dsc->length > SDIO_MAX_BYTE_SIZE)
		dsc->length = (dsc->length + SDIO_MAX_BYTE_SIZE) &
			~(SDIO_MAX_BYTE_SIZE);
#endif

	if (adapter->halted) {
		pr_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}

	return sd_send(adapter, dsc->buffer, dsc->length);

}

/* Return packet for packet buffer freeing */
void hw_return_packet(struct net_adapter *adapter, u16 type)
{
	struct buffer_descriptor	*curElem;
	struct buffer_descriptor	*prevElem = NULL;

	if (queue_empty(adapter->ctl.q_received_ctrl.head))
		return;

	/* first time get head needed to get the dsc nodes */
	curElem = (struct buffer_descriptor *)
		queue_get_head(adapter->ctl.q_received_ctrl.head);

	for ( ; curElem != NULL; prevElem = curElem,
			curElem  = (struct buffer_descriptor *)
						curElem->node.next) {
		if (curElem->type == type) {
			/* process found*/
			if (prevElem == NULL) {
				/* First node or only one node to delete */
				(adapter->ctl.q_received_ctrl.head).next =
					((struct list_head *)curElem)->next;
				if (!((adapter->ctl.q_received_ctrl.head).
							next)) {
					/*
					* rechain list pointer to next link
					* if the list pointer is null, null
					*   out the reverse link
					*/
					(adapter->ctl.q_received_ctrl.head).prev
						= NULL;
				}
			} else if (((struct list_head *)curElem)->next ==
					NULL) {
				/* last node */
				((struct list_head *)prevElem)->next = NULL;
				(adapter->ctl.q_received_ctrl.head).prev =
					(struct list_head *)(&prevElem);
			} else {
				/* middle node */
				((struct list_head *)prevElem)->next =
					((struct list_head *)curElem)->next;
			}

			kfree(curElem->buffer);
			kfree(curElem);
			break;
		}
	}
}

int hw_device_wakeup(struct net_adapter *adapter)
{
	int	rc = 0;
	u8	retryCount = 0;

	if (adapter->wimax_status == WIMAX_STATE_READY) {
		pr_debug("not ready!!");
		return 0;
	}

	adapter->prev_wimax_status = adapter->wimax_status;
	adapter->wimax_status = WIMAX_STATE_AWAKE_REQUESTED;

	/* try to wake up */
	while (retryCount < WAKEUP_MAX_TRY) {
		rc = wait_for_completion_interruptible_timeout(
				&adapter->wakeup_event,
				msecs_to_jiffies(WAKEUP_TIMEOUT));

		/* received wakeup ack */
		if (rc)
			break;

		retryCount++;

		if (g_pdata->is_modem_awake()) {
			pr_debug("WiMAX active pin HIGH ..");
			break;
		}
	}

	/* check WiMAX modem response */
	if (!g_pdata->is_modem_awake()) {
		pr_debug("FATAL ERROR!! MODEM DOES NOT WAKEUP!!");
		adapter->halted = TRUE;
	}

	if (adapter->wimax_status == WIMAX_STATE_AWAKE_REQUESTED) {
		if (adapter->prev_wimax_status == WIMAX_STATE_IDLE
				||
			adapter->prev_wimax_status == WIMAX_STATE_NORMAL) {
			pr_debug("hw_device_wakeup: IDLE -> NORMAL");
			adapter->wimax_status = WIMAX_STATE_NORMAL;
		} else {
			pr_debug("hw_device_wakeup: VI -> READY");
			adapter->wimax_status = WIMAX_STATE_READY;
		}
	}

	return 0;
}


int cmc732_send_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct buffer_descriptor	*dsc;
	int				nRet = 0;
	do {
			wait_event_interruptible(adapter->send_event,
					(!queue_empty(adapter->hw.q_send.head))
					|| (!adapter) || adapter->halted);

			if ((!adapter) || adapter->halted)
				break;
			if ((adapter->wimax_status == WIMAX_STATE_IDLE ||
					adapter->wimax_status
					== WIMAX_STATE_VIRTUAL_IDLE)
					&& !g_pdata->is_modem_awake())
				hw_device_wakeup(adapter);


			spin_lock(&adapter->hw.q_send.lock);
			dsc = (struct buffer_descriptor *)
				queue_get_head(adapter->hw.q_send.head);
			queue_remove_head(adapter->hw.q_send.head);
			spin_unlock(&adapter->hw.q_send.lock);

			if (!dsc) {
				pr_debug("Fail...node is null");
				continue;
				}
			nRet = sd_send_data(adapter, dsc);
			g_pdata->wakeup_assert(0);
			kfree(dsc->buffer);
			kfree(dsc);
			if (nRet != STATUS_SUCCESS) {
				pr_debug("SendData Fail******");
				++adapter->XmitErr;
			}
		}
	while (adapter);

	pr_debug("cmc732_send_thread exiting");

	adapter->halted = TRUE;

	do_exit(0);

	return 0;
}
