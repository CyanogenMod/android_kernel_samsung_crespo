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
#include <linux/wakelock.h>

#include "herring.h"
#define LED_VERSION 2

static int bl_on = 0;
static bool bBlink = true;
static bool bBlinkTimer = false;
static bool bIsOn = false;
static int iCountsBlink = 0;
static unsigned int iTimeBlink = 200;
static unsigned int iTimesOn = 1;
static unsigned int iTimesTotal = 10;
static unsigned int iBlinkMilisecondsTimeout = 1500;
static DECLARE_MUTEX(enable_sem);
static DECLARE_MUTEX(i2c_sem);
static uint32_t blink_count;

static bool system_is_sleeping = false;

static void blink_timer_callback(unsigned long data);
static struct timer_list blink_timer = TIMER_INITIALIZER(blink_timer_callback, 0, 0);
static void blink_callback(struct work_struct *blink_work);
static DECLARE_WORK(blink_work, blink_callback);

static struct wake_lock sBlinkWakeLock;

static int led_gpios[] = { 2, 3, 6, 7 };

static void herring_touchkey_led_onoff(int onoff)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(led_gpios); i++)
		gpio_direction_output(S5PV210_GPJ3(led_gpios[i]), !!onoff);

    bIsOn = onoff;
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

static void blink_callback(struct work_struct *blink_work)
{
    if ( bl_on == 0) {
            printk(KERN_DEBUG "%s: ERROR notification BLINK ENTER without CALL 0 \n", __FUNCTION__);
            // notify_led_off();
            del_timer(&blink_timer);
            wake_unlock(&sBlinkWakeLock);
            return;
    }

    if ( bl_on == 2) {
            printk(KERN_DEBUG "%s: ERROR notification BLINK ENTER without CALL 2\n", __FUNCTION__);
            herring_touchkey_led_onoff(1);
            del_timer(&blink_timer);
            wake_unlock(&sBlinkWakeLock);
            bBlinkTimer = false;
            return;
    }

    if (--blink_count == 0 && iBlinkMilisecondsTimeout != 0) {
        herring_touchkey_led_onoff(1);
        if ( bBlinkTimer ) {
            bBlinkTimer = false;
            bl_on = 2;
            del_timer(&blink_timer);
            wake_unlock(&sBlinkWakeLock);
        }
        return;
    }

    if (iCountsBlink++ < iTimesOn) {
        if (!bIsOn)
            herring_touchkey_led_onoff(1);
    } else {
        if (bIsOn)
            herring_touchkey_led_onoff(0);
    }

    if ( iCountsBlink >= iTimesTotal)
        iCountsBlink = 0;
}

static void blink_timer_callback(unsigned long data)
{
    schedule_work(&blink_work);
    mod_timer(&blink_timer, jiffies + msecs_to_jiffies(iTimeBlink));
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
            if (data == 0 && bl_on == 0)
                return 0;
            if (data == 1 && bl_on == 0) {
                printk(KERN_DEBUG "%s: notification led enabled\n", __FUNCTION__);
                if (bBlink && data == 1 && bl_on != 1) {
                    printk(KERN_DEBUG "%s: notification led TIMER enabled\n", __FUNCTION__);
                    wake_lock(&sBlinkWakeLock);
                    blink_timer.expires = jiffies + msecs_to_jiffies(iTimeBlink);
                    blink_count = iBlinkMilisecondsTimeout;
                    add_timer(&blink_timer);
                    bBlinkTimer = true;
                }
                herring_touchkey_led_onoff(1);
                bl_on = 1;
                pr_info("%s: LIGHT ENABLED\n", __FUNCTION__);
            }
            else {
                herring_touchkey_led_onoff(0);
                bl_on = 0;
                pr_info("%s: LIGHT DISSABLED\n", __FUNCTION__);
                if ( bBlinkTimer ) {
                    bBlinkTimer = false;
                    del_timer(&blink_timer);
                    wake_unlock(&sBlinkWakeLock);
                }
            }
        }
	}
	return size;
}

static ssize_t led_blinktimeout_read(struct device *dev, struct device_attribute *attr, char *buf) {
unsigned int iMinutes;

    iMinutes = iBlinkMilisecondsTimeout / 300;
	return sprintf(buf,"%u\n", iMinutes);
}

static ssize_t led_blinktimeout_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int data;

	if (sscanf(buf, "%u\n", &data)) {
        iBlinkMilisecondsTimeout = data * 300;
        if(data >= 1)
            bBlink = true;
        else
            bBlink = false;
	}
	mod_timer(&blink_timer, jiffies + msecs_to_jiffies(iTimeBlink));
	return size;
}

static ssize_t led_blink_read(struct device *dev, struct device_attribute *attr, char *buf) {
	return sprintf(buf,"%u\n", bBlink);
}

static ssize_t led_blink_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int data;

	if (sscanf(buf, "%u\n", &data)) {
        if (data == 0)
            bBlink = false;
        else
            bBlink = true;
	}
	return size;
}

static ssize_t led_version_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", LED_VERSION);
}

static DEVICE_ATTR(led, S_IRUGO | S_IWUGO , led_status_read, led_status_write);
static DEVICE_ATTR(blinktimeout, S_IRUGO | S_IWUGO , led_blinktimeout_read, led_blinktimeout_write);
static DEVICE_ATTR(blink, S_IRUGO | S_IWUGO , led_blink_read, led_blink_write);
static DEVICE_ATTR(version, S_IRUGO , led_version_read, NULL);

static struct attribute *bl_led_attributes[] = {
	&dev_attr_led.attr,
        &dev_attr_blinktimeout.attr,
        &dev_attr_blink.attr,
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

    /* Initialize wake locks */
    wake_lock_init(&sBlinkWakeLock, WAKE_LOCK_SUSPEND, "blink_wake");

    setup_timer(&blink_timer, blink_timer_callback, 0);

    up(&i2c_sem);
	return 0;

err_req:
	while (--i >= 0)
		gpio_free(S5PV210_GPJ3(led_gpios[i]));
    up(&i2c_sem);
	return ret;
}

device_initcall(herring_init_touchkey_led);
