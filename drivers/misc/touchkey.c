/* drivers/misc/touchkey.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/earlysuspend.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/touchkey.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wakelock.h>

static struct touchkey_implementation *touchkey_imp = NULL;

static ssize_t touchkey_set_backlight_status(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int val = 0;
	if (!touchkey_imp)
		return size;
	sscanf(buf, "%d\n", &val);
	if (val == 0) {
		touchkey_imp->disable();
	} else {
		touchkey_imp->enable();
	}
	return size;
}

static ssize_t touchkey_show_backlight_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!touchkey_imp)
		return 0;
	return sprintf(buf, "%d", touchkey_imp->status());
}

static DEVICE_ATTR(backlight_status, S_IRUGO | S_IWUGO,
				touchkey_show_backlight_status,
				touchkey_set_backlight_status);

static struct attribute *touchkey_attributes[] = {
	&dev_attr_backlight_status,
	NULL
};

static struct attribute_group touchkey_group = {
	.attrs  = touchkey_attributes,
};

static struct miscdevice touchkey_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cypress_touchkey",
};

void register_touchkey_implementation(struct touchkey_implementation *imp)
{
	touchkey_imp = imp;
}
EXPORT_SYMBOL(register_touchkey_implementation);


static int __init touchkey_control_init(void)
{
	int ret;

	pr_info("%s misc_register(%s)\n", __FUNCTION__, touchkey_device.name);
	ret = misc_register(&touchkey_device);
	if (ret) {
		pr_err("%s misc_register(%s) fail\n", __FUNCTION__,
				touchkey_device.name);
		return 1;
	}

	/* add the bln attributes */
	if (sysfs_create_group(&touchkey_device.this_device->kobj,
				&touchkey_group) < 0) {
		pr_err("%s sysfs_create_group fail\n", __FUNCTION__);
		pr_err("Failed to create sysfs group for device (%s)!\n",
				touchkey_device.name);
	}

	return 0;
}

device_initcall(touchkey_control_init);
