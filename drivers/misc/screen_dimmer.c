/* drivers/misc/screen_dimmer.c
 *
 * Copyright 2011  Ezekeel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/screen_dimmer.h>
#include <linux/workqueue.h>
#include <linux/earlysuspend.h>
#include <linux/mutex.h>

extern void s3cfb_screen_enable(void);
extern void s3cfb_screen_disable(void);
extern void screendimmer_start_drawing(void);
extern void screendimmer_stop_drawing(void);

static bool screendimmer_enabled = false;

static bool screen_dimmed = false;

static bool device_suspended = false;

static unsigned int dimmer_delay = 15000;

static void screendimmer_toggle_screen(struct work_struct * dimmer_work);

static DECLARE_DELAYED_WORK(dimmer_work, screendimmer_toggle_screen);

static DEFINE_MUTEX(lock);

static struct screendimmer_implementation * screendimmer_imp = NULL;

#define SCREENDIMMER_VERSION 1

static void screendimmer_early_suspend(struct early_suspend * h)
{
    device_suspended = true;

    cancel_delayed_work(&dimmer_work);
    flush_scheduled_work();

    return;
}

static void screendimmer_late_resume(struct early_suspend * h)
{
    device_suspended = false;

    screen_dimmed = false;

    touchscreen_pressed();

    return;
}

static struct early_suspend screendimmer_suspend_data = 
    {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = screendimmer_early_suspend,
	.resume = screendimmer_late_resume,
    };

static void screendimmer_disable_screen(void)
{
    pr_info("disable screen\n");

    screendimmer_stop_drawing();

    if (screendimmer_imp)
	{
	    screendimmer_imp->disable();
	}

    s3cfb_screen_disable();

    screen_dimmed = true;

    mutex_unlock(&lock);

    return;
}

static void screendimmer_enable_screen(void)
{
    pr_info("enable screen\n");

    s3cfb_screen_enable();

    if (screendimmer_imp)
	{
	    screendimmer_imp->enable();
	}

    screendimmer_start_drawing();

    screen_dimmed = false;

    mutex_unlock(&lock);

    touchscreen_pressed();

    return;
}

static void screendimmer_toggle_screen(struct work_struct * dimmer_work)
{
    if (!mutex_is_locked(&lock))
	{
	    mutex_lock(&lock);
	}

    if (screen_dimmed)
	{
	    screendimmer_enable_screen();
	}
    else
	{
	    screendimmer_disable_screen();
	}

    return;
}

static ssize_t screendimmer_status_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", (screendimmer_enabled ? 1 : 0));
}

static ssize_t screendimmer_status_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    unsigned int data;

    if(sscanf(buf, "%u\n", &data) == 1) 
	{
	    pr_devel("%s: %u \n", __FUNCTION__, data);
	    
	    if (data == 1) 
		{
		    pr_info("%s: SCREENDIMMER function enabled\n", __FUNCTION__);

		    screendimmer_enabled = true;

		    touchscreen_pressed();
		} 
	    else if (data == 0) 
		{
		    pr_info("%s: SCREENDIMMER function disabled\n", __FUNCTION__);

		    screendimmer_enabled = false;

		    cancel_delayed_work(&dimmer_work);
		    flush_scheduled_work();

		    if (screen_dimmed)
			{
			    screendimmer_enable_screen();
			}
		} 
	    else 
		{
		    pr_info("%s: invalid input range %u\n", __FUNCTION__, data);
		}
	} 
    else 
	{
	    pr_info("%s: invalid input\n", __FUNCTION__);
	}

    return size;
}

static ssize_t screendimmer_delay_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", dimmer_delay);
}

static ssize_t screendimmer_delay_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    unsigned int data;

    if(sscanf(buf, "%u\n", &data) == 1) 
	{
	    if (data > 0)
		{
		    dimmer_delay = data;

		    pr_info("SCREENDIMMER delay set to %u\n", dimmer_delay);

		    touchscreen_pressed();
		}
	    else
		{
		  pr_info("%s: invalid input range %u\n", __FUNCTION__, data);
		}  
	} 
    else 
	{
	    pr_info("%s: invalid input\n", __FUNCTION__);
	}

    return size;
}

static ssize_t screendimmer_version(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", SCREENDIMMER_VERSION);
}

static DEVICE_ATTR(enabled, S_IRUGO | S_IWUGO, screendimmer_status_read, screendimmer_status_write);
static DEVICE_ATTR(delay, S_IRUGO | S_IWUGO, screendimmer_delay_read, screendimmer_delay_write);
static DEVICE_ATTR(version, S_IRUGO , screendimmer_version, NULL);

static struct attribute *screendimmer_notification_attributes[] = 
    {
	&dev_attr_enabled.attr,
	&dev_attr_delay.attr,
	&dev_attr_version.attr,
	NULL
    };

static struct attribute_group screendimmer_notification_group = 
    {
	.attrs  = screendimmer_notification_attributes,
    };

static struct miscdevice screendimmer_device = 
    {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "screendimmer",
    };

void touchscreen_pressed()
{   
    if (!device_suspended && screendimmer_enabled)
	{
	    if (!mutex_is_locked(&lock))
		{
		    if (screen_dimmed)
			{
			    mutex_lock(&lock);

			    cancel_delayed_work(&dimmer_work);
			    schedule_delayed_work(&dimmer_work, 0);
			}
		    else
			{
			    cancel_delayed_work(&dimmer_work);
			    schedule_delayed_work(&dimmer_work, msecs_to_jiffies(dimmer_delay));
			}
		}
	}

    return;
}
EXPORT_SYMBOL(touchscreen_pressed);

void register_screendimmer_implementation(struct screendimmer_implementation * imp)
{
    screendimmer_imp = imp;

    return;
}
EXPORT_SYMBOL(register_screendimmer_implementation);

bool screen_is_dimmed(void)
{
    return screen_dimmed;
}
EXPORT_SYMBOL(screen_is_dimmed);

static int __init screendimmer_control_init(void)
{
    int ret;

    pr_info("%s misc_register(%s)\n", __FUNCTION__, screendimmer_device.name);

    ret = misc_register(&screendimmer_device);

    if (ret) 
	{
	    pr_err("%s misc_register(%s) fail\n", __FUNCTION__, screendimmer_device.name);

	    return 1;
	}

    if (sysfs_create_group(&screendimmer_device.this_device->kobj, &screendimmer_notification_group) < 0) 
	{
	    pr_err("%s sysfs_create_group fail\n", __FUNCTION__);
	    pr_err("Failed to create sysfs group for device (%s)!\n", screendimmer_device.name);
	}

    register_early_suspend(&screendimmer_suspend_data);

    return 0;
}

device_initcall(screendimmer_control_init);
