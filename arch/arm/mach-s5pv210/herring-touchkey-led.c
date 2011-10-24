/*
 * Copyright (C) 2010 Samsung Electronics Co. Ltd. All Rights Reserved.
 * Author: Rom Lemarchand <rlemarchand@sta.samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <asm/mach-types.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/semaphore.h>

#include "herring.h"
#define LED_VERSION 2

static int bl_on = 0;
static int iBlink = 0;
static unsigned int iTimeOn = 100;
static unsigned int iTimeOff = 1000;
static unsigned int iNotificationTimeOut = 0;
static unsigned int Backlight_Timeout = 10000;
static DECLARE_MUTEX(enable_sem);
static DECLARE_MUTEX(i2c_sem);

static struct timer_list bl_timer;
static void bl_off(struct work_struct *bl_off_work);
static bool system_is_sleeping = false;
static bool bNotificated = false;
static DECLARE_WORK(bl_off_work, bl_off);

static int led_gpios[] = { 2, 3, 6, 7 };

static void herring_touchkey_led_onoff(int onoff)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(led_gpios); i++)
		gpio_direction_output(S5PV210_GPJ3(led_gpios[i]), !!onoff);
}

static void herring_touchkey_led_early_suspend(struct early_suspend *h)
{
    herring_touchkey_led_onoff(0);
	system_is_sleeping = true;
}

static void herring_touchkey_led_late_resume(struct early_suspend *h)
{
	herring_touchkey_led_onoff(1);
	system_is_sleeping = false;
}

static struct early_suspend early_suspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = herring_touchkey_led_early_suspend,
	.resume = herring_touchkey_led_late_resume,
};

static ssize_t led_status_read(struct device *dev, struct device_attribute *attr, char *buf) {
	return sprintf(buf,"%u\n", bl_on);
}

static ssize_t led_status_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int data;
	pr_info("%s: New MESSAGE ARRIVED\n", __FUNCTION__);

	if (sscanf(buf, "%u\n", &data)) {
        if (system_is_sleeping) {
            if (data == 1 && bl_on == false) {
                herring_touchkey_led_onoff(1);
                bl_on = true;
                pr_info("%s: LIGHT ENABLED\n", __FUNCTION__);
            }
            else {
                herring_touchkey_led_onoff(0);
                bl_on = false;
                pr_info("%s: LIGHT DISSABLED\n", __FUNCTION__);
            }
        }
	}
	return size;
}

static ssize_t led_timeout_read(struct device *dev, struct device_attribute *attr, char *buf) {
	return sprintf(buf,"%u\n", Backlight_Timeout);
}

static ssize_t led_timeout_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int data;

	if (sscanf(buf, "%u\n", &data)) {
        Backlight_Timeout = data;
	}
	return size;
}

static ssize_t led_version_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", LED_VERSION);
}

static DEVICE_ATTR(led, S_IRUGO | S_IWUGO , led_status_read, led_status_write);
static DEVICE_ATTR(timeout, S_IRUGO | S_IWUGO , led_timeout_read, led_timeout_write);
static DEVICE_ATTR(version, S_IRUGO , led_version_read, NULL);

static struct attribute *bl_led_attributes[] = {
		&dev_attr_led.attr,
        &dev_attr_timeout.attr,
        &dev_attr_version.attr,
		NULL
};

static struct attribute_group bl_led_group = {
		.attrs  = bl_led_attributes,
};

static struct miscdevice bl_led_device = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "notification",
};

static int __init herring_init_touchkey_led(void)
{
	int i;
	int ret = 0;
	u32 s5pv210_gpj3;

    down(&i2c_sem);

	if (!machine_is_herring() || !herring_is_tft_dev())
		return 0;

	for (i = 0; i < ARRAY_SIZE(led_gpios); i++) {
		s5pv210_gpj3 = S5PV210_GPJ3(led_gpios[i]);
		ret = gpio_request(s5pv210_gpj3, "touchkey led");
		if (ret) {
			pr_err("Failed to request touchkey led gpio %d\n", i);
			goto err_req;
		}
        s3c_gpio_setpull(s5pv210_gpj3, S3C_GPIO_PULL_NONE);
        s3c_gpio_slp_cfgpin(s5pv210_gpj3, S3C_GPIO_SLP_PREV);
        s3c_gpio_slp_setpull_updown(s5pv210_gpj3, S3C_GPIO_PULL_NONE);
	}

	herring_touchkey_led_onoff(1);

	register_early_suspend(&early_suspend);

	if (misc_register(&bl_led_device))
		printk("%s misc_register(%s) failed\n", __FUNCTION__, bl_led_device.name);
	else {
		if (sysfs_create_group(&bl_led_device.this_device->kobj, &bl_led_group) < 0)
            pr_err("failed to create sysfs group for device %s\n", bl_led_device.name);
	}

    up(&i2c_sem);
	return 0;

err_req:
	while (--i >= 0)
		gpio_free(S5PV210_GPJ3(led_gpios[i]));
    up(&i2c_sem);
	return ret;
}

device_initcall(herring_init_touchkey_led);
