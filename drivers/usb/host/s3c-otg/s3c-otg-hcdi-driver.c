/****************************************************************************
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-driver.c
 * @brief  It provides functions related with module for OTGHCD driver. \n
 * @version
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
 *  -# Aug 18,2008 v1.3 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Modifying for successful rmmod & disconnecting \n
 * @see None
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

#include "s3c-otg-hcdi-driver.h"
extern void otg_phy_off(void);

/**
 * static int s5pc110_otg_drv_probe (struct platform_device *pdev)
 *
 * @brief probe function of OTG hcd platform_driver
 *
 * @param [in] pdev : pointer of platform_device of otg hcd platform_driver
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If fail \n
 * @remark
 * it allocates resources of it and call other modules' init function.
 * then call usb_create_hcd, usb_add_hcd, s5pc110_otghcd_start functions
 */

struct usb_hcd* g_pUsbHcd = NULL;

static void otg_power_work(struct work_struct *work)
{
	struct sec_otghost *otghost = container_of(work,
				struct sec_otghost, work);
	struct sec_otghost_data *hdata = otghost->otg_data;

	if (hdata && hdata->set_pwr_cb) {
		hdata->set_pwr_cb(0);
#ifdef CONFIG_USB_HOST_NOTIFY
		if (g_pUsbHcd)
			host_state_notify(&g_pUsbHcd->ndev, NOTIFY_HOST_OVERCURRENT);
#endif
	} else {
		otg_err(true, "invalid otghost data\n");
	}
}

static int s5pc110_otg_drv_probe (struct platform_device *pdev)
{
	int ret_val = 0;
	u32 reg_val = 0;
	struct sec_otghost *otghost = NULL;
	struct sec_otghost_data *otg_data = dev_get_platdata(&pdev->dev);

	otg_dbg(OTG_DBG_OTGHCDI_DRIVER, "s3c_otg_drv_probe\n");


	/*init for host mode*/
	/**
	Allocate memory for the base HCD &	Initialize the base HCD.
	*/
	g_pUsbHcd = usb_create_hcd(&s5pc110_otg_hc_driver, &pdev->dev,
					"s3cotg");/*pdev->dev.bus_id*/
	if (g_pUsbHcd == NULL) {
		ret_val = -ENOMEM;
		otg_err(OTG_DBG_OTGHCDI_DRIVER,
			"failed to usb_create_hcd\n");
		goto err_out_clk;
	}

	/* mapping hcd resource & device resource*/

	g_pUsbHcd->rsrc_start = pdev->resource[0].start;
	g_pUsbHcd->rsrc_len   = pdev->resource[0].end -
	   			 pdev->resource[0].start + 1;

	if (!request_mem_region(g_pUsbHcd->rsrc_start, g_pUsbHcd->rsrc_len,
		  				  gHcdName)) {
		otg_err(OTG_DBG_OTGHCDI_DRIVER,
			"failed to request_mem_region\n");
		ret_val = -EBUSY;
		goto err_out_create_hcd;
	}


	/* Physical address => Virtual address */
	g_pUsbHcd->regs = S3C_VA_OTG;
	g_pUsbHcd->self.otg_port = 1;

	g_pUDCBase = (u8 *)g_pUsbHcd->regs;

	otghost = hcd_to_sec_otghost(g_pUsbHcd);

	if (otghost == NULL) {
		otg_err(true, "failed to get otghost hcd\n");
		ret_val = USB_ERR_FAIL;
		goto err_out_create_hcd;
	}
	otghost->otg_data = otg_data;

	INIT_WORK(&otghost->work, otg_power_work);
	otghost->wq = create_singlethread_workqueue("sec_otghostd");

	/* call others' init() */
	ret_val = otg_hcd_init_modules(otghost);
	if (ret_val != USB_ERR_SUCCESS) {
		otg_err(OTG_DBG_OTGHCDI_DRIVER,
			"failed to otg_hcd_init_modules\n");
		ret_val = USB_ERR_FAIL;
		goto err_out_create_hcd;
	}

	/**
	 * Attempt to ensure this device is really a s5pc110 USB-OTG Controller.
	 * Read and verify the SNPSID register contents. The value should be
	 * 0x45F42XXX, which corresponds to "OT2", as in "OTG version 2.XX".
	 */
	reg_val = read_reg_32(0x40);
	if ((reg_val & 0xFFFFF000) != 0x4F542000) {
		otg_err(OTG_DBG_OTGHCDI_DRIVER,
			"Bad value for SNPSID: 0x%x\n", reg_val);
		ret_val = -EINVAL;
		goto err_out_create_hcd_init;
	}
#ifdef CONFIG_USB_HOST_NOTIFY
	if (otg_data->host_notify) {
		g_pUsbHcd->host_notify = otg_data->host_notify;
		g_pUsbHcd->ndev.name = dev_name(&pdev->dev);
		ret_val = host_notify_dev_register(&g_pUsbHcd->ndev);
		if (ret_val) {
			otg_err(OTG_DBG_OTGHCDI_DRIVER, 
				"Failed to host_notify_dev_register\n");
			goto err_out_create_hcd_init;
		}
	}
#endif
#ifdef CONFIG_USB_SEC_WHITELIST
	if (otg_data->sec_whlist_table_num)
		g_pUsbHcd->sec_whlist_table_num = otg_data->sec_whlist_table_num;
#endif

	/*
	 * Finish generic HCD initialization and start the HCD. This function
	 * allocates the DMA buffer pool, registers the USB bus, requests the
	 * IRQ line, and calls s5pc110_otghcd_start method.
	 */
	ret_val = usb_add_hcd(g_pUsbHcd,
	       	pdev->resource[1].start, IRQF_DISABLED);
	if (ret_val < 0) {
		goto err_out_host_notify_register;
	}

	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"OTG HCD Initialized HCD, bus=%s, usbbus=%d\n",
		"C110 OTG Controller", g_pUsbHcd->self.busnum);

	/* otg_print_registers(); */

	wake_lock_init(&otghost->wake_lock, WAKE_LOCK_SUSPEND, "usb_otg");
	wake_lock(&otghost->wake_lock);

	return USB_ERR_SUCCESS;
err_out_host_notify_register:
#ifdef CONFIG_USB_HOST_NOTIFY
	host_notify_dev_unregister(&g_pUsbHcd->ndev);
#endif

err_out_create_hcd_init:
	otg_hcd_deinit_modules(otghost);
	release_mem_region(g_pUsbHcd->rsrc_start, g_pUsbHcd->rsrc_len);

err_out_create_hcd:
	usb_put_hcd(g_pUsbHcd);

err_out_clk:

	return ret_val;
}

/**
 * static int s5pc110_otg_drv_remove (struct platform_device *dev)
 *
 * @brief remove function of OTG hcd platform_driver
 *
 * @param [in] pdev : pointer of platform_device of otg hcd platform_driver
 *
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If fail \n
 * @remark
 * This function is called when the otg device unregistered with the
 * s5pc110_otg_driver. This happens, for example, when the rmmod command is
 * executed. The device may or may not be electrically present. If it is
 * present, the driver stops device processing. Any resources used on behalf
 * of this device are freed.
 */
static int s5pc110_otg_drv_remove (struct platform_device *dev)
{
	struct sec_otghost *otghost = NULL;

	otg_dbg(OTG_DBG_OTGHCDI_DRIVER, "s5pc110_otg_drv_remove\n");

	otghost = hcd_to_sec_otghost(g_pUsbHcd);

#ifdef CONFIG_USB_HOST_NOTIFY
	host_notify_dev_unregister(&g_pUsbHcd->ndev);
#endif

	otg_hcd_deinit_modules(otghost);

	destroy_workqueue(otghost->wq);

	wake_unlock(&otghost->wake_lock);
	wake_lock_destroy(&otghost->wake_lock);

	usb_remove_hcd(g_pUsbHcd);

	release_mem_region(g_pUsbHcd->rsrc_start, g_pUsbHcd->rsrc_len);

	usb_put_hcd(g_pUsbHcd);

	otg_phy_off();

	return USB_ERR_SUCCESS;
}

/**
 * @struct s5pc110_otg_driver
 *
 * @brief
 * This structure defines the methods to be called by a bus driver
 * during the lifecycle of a device on that bus. Both drivers and
 * devices are registered with a bus driver. The bus driver matches
 * devices to drivers based on information in the device and driver
 * structures.
 *
 * The probe function is called when the bus driver matches a device
 * to this driver. The remove function is called when a device is
 * unregistered with the bus driver.
 */
struct platform_driver s5pc110_otg_driver = {
	.probe = s5pc110_otg_drv_probe,
	.remove = s5pc110_otg_drv_remove,
	.shutdown = usb_hcd_platform_shutdown,
	.driver = {
		.name = "s3c_otghcd",
		.owner = THIS_MODULE,
	},
};

/**
 * static int __init s5pc110_otg_module_init(void)
 *
 * @brief module_init function
 *
 * @return it returns result of platform_driver_register
 * @remark
 * This function is called when the s5pc110_otg_driver is installed with the
 * insmod command. It registers the s5pc110_otg_driver structure with the
 * appropriate bus driver. This will cause the s5pc110_otg_driver_probe function
 * to be called. In addition, the bus driver will automatically expose
 * attributes defined for the device and driver in the special sysfs file
 * system.
 */
 /*
static int __init s5pc110_otg_module_init(void)
{
	int	ret_val = 0;

	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"s3c_otg_module_init \n");

	ret_val = platform_driver_register(&s5pc110_otg_driver);
	if (ret_val < 0)
	{
		otg_err(OTG_DBG_OTGHCDI_DRIVER,
			"platform_driver_register \n");
	}
	return ret_val;
} */

/**
 * static void __exit s5pc110_otg_module_exit(void)
 *
 * @brief module_exit function
 *
 * @remark
 * This function is called when the driver is removed from the kernel
 * with the rmmod command. The driver unregisters itself with its bus
 * driver.
 */
static void __exit s5pc110_otg_module_exit(void)
{
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"s3c_otg_module_exit \n");
	platform_driver_unregister(&s5pc110_otg_driver);
}

/* for debug */
void otg_print_registers(void)
{
	/* USB PHY Control Registers */
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"USB_CONTROL = 0x%x.\n", readl(0xfb10e80c));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"UPHYPWR = 0x%x.\n", readl(S3C_USBOTG_PHYPWR));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"UPHYCLK = 0x%x.\n", readl(S3C_USBOTG_PHYCLK));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"URSTCON = 0x%x.\n", readl(S3C_USBOTG_RSTCON));

	/* OTG LINK Core registers (Core Global Registers) */
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GOTGCTL = 0x%x.\n", read_reg_32(GOTGCTL));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GOTGINT = 0x%x.\n", read_reg_32(GOTGINT));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GAHBCFG = 0x%x.\n", read_reg_32(GAHBCFG));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GUSBCFG = 0x%x.\n", read_reg_32(GUSBCFG));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GINTSTS = 0x%x.\n", read_reg_32(GINTSTS));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GINTMSK = 0x%x.\n", read_reg_32(GINTMSK));

	/* Host Mode Registers */
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"HCFG = 0x%x.\n", read_reg_32(HCFG));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"HPRT = 0x%x.\n", read_reg_32(HPRT));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"HFIR = 0x%x.\n", read_reg_32(HFIR));

	/* Synopsys ID */
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GSNPSID  = 0x%x.\n", read_reg_32(GSNPSID));

	/* HWCFG */
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GHWCFG1  = 0x%x.\n", read_reg_32(GHWCFG1));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GHWCFG2  = 0x%x.\n", read_reg_32(GHWCFG2));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GHWCFG3  = 0x%x.\n", read_reg_32(GHWCFG3));
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"GHWCFG4  = 0x%x.\n", read_reg_32(GHWCFG4));

	/* PCGCCTL */
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,
		"PCGCCTL  = 0x%x.\n", read_reg_32(PCGCCTL));
}

/*
module_init(s5pc110_otg_module_init);
module_exit(s5pc110_otg_module_exit);
*/

MODULE_DESCRIPTION("OTG USB HOST controller driver");
MODULE_AUTHOR("SAMSUNG / System LSI / EMSP");
MODULE_LICENSE("GPL");
