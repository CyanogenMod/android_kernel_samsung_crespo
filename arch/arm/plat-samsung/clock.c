/* linux/arch/arm/plat-s3c24xx/clock.c
 *
 * Copyright 2004-2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C24XX Core clock control support
 *
 * Based on, and code from linux/arch/arm/mach-versatile/clock.c
 **
 **  Copyright (C) 2004 ARM Limited.
 **  Written by Deep Blue Solutions Limited.
 *
 *
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
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#if defined(CONFIG_DEBUG_FS)
#include <linux/debugfs.h>
#endif

#include <mach/hardware.h>
#include <asm/irq.h>

#include <plat/cpu-freq.h>

#include <plat/clock.h>
#include <plat/cpu.h>

#include <linux/serial_core.h>
#include <plat/regs-serial.h> /* for s3c24xx_uart_devs */

/* clock information */

static LIST_HEAD(clocks);

/* We originally used an mutex here, but some contexts (see resume)
 * are calling functions such as clk_set_parent() with IRQs disabled
 * causing an BUG to be triggered.
 */
DEFINE_SPINLOCK(clocks_lock);

/* enable and disable calls for use with the clk struct */

static int clk_null_enable(struct clk *clk, int enable)
{
	return 0;
}

static int dev_is_s3c_uart(struct device *dev)
{
	struct platform_device **pdev = s3c24xx_uart_devs;
	int i;
	for (i = 0; i < ARRAY_SIZE(s3c24xx_uart_devs); i++, pdev++)
		if (*pdev && dev == &(*pdev)->dev)
			return 1;
	return 0;
}

/*
 * Serial drivers call get_clock() very early, before platform bus
 * has been set up, this requires a special check to let them get
 * a proper clock
 */

static int dev_is_platform_device(struct device *dev)
{
	return dev->bus == &platform_bus_type ||
	       (dev->bus == NULL && dev_is_s3c_uart(dev));
}

/* Clock API calls */

static int nullstrcmp(const char *a, const char *b)
{
	if (!a)
		return b ? -1 : 0;
	if (!b)
		return 1;

	return strcmp(a, b);
}

struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *clk;
	int idno;

	if (dev == NULL || !dev_is_platform_device(dev))
		idno = -1;
	else
		idno = to_platform_device(dev)->id;

	spin_lock(&clocks_lock);

	list_for_each_entry(clk, &clocks, list)
		if (!nullstrcmp(id, clk->name) && clk->dev == dev)
			goto found_it;

	list_for_each_entry(clk, &clocks, list)
		if (clk->id == idno && nullstrcmp(id, clk->name) == 0)
			goto found_it;

	list_for_each_entry(clk, &clocks, list)
		if (clk->id == -1 && !nullstrcmp(id, clk->name) &&
							clk->dev == NULL)
			goto found_it;

	clk = ERR_PTR(-ENOENT);
	pr_warning("%s: could not find clock %s for dev %pS (%s)\n",
		   __func__, id, dev, dev ? dev_name(dev) : "");
	spin_unlock(&clocks_lock);
	return clk;
found_it:
	pr_debug("%s(%p, %s) found %s %d %pS\n",
		 __func__, dev, id, clk->name, clk->id, clk->dev);
	if (!try_module_get(clk->owner))
		clk = ERR_PTR(-ENOENT);
	spin_unlock(&clocks_lock);
	return clk;
}

void clk_put(struct clk *clk)
{
	pr_debug("%s on %s %d %pS", __func__, clk->name, clk->id, clk->dev);
	module_put(clk->owner);
}

void _clk_enable(struct clk *clk)
{
	if (!clk || IS_ERR(clk))
		return;

	if ((clk->usage++) > 0)
		return;

	_clk_enable(clk->parent);
	pr_debug("%s update hardware clock %s %d %pS\n",
		 __func__, clk->name, clk->id, clk->dev);
	(clk->enable)(clk, 1);
}

int clk_enable(struct clk *clk)
{
	if (WARN_ON_ONCE(IS_ERR(clk) || clk == NULL)) {
		pr_debug("%s request on invalid clock\n", __func__);
		return -EINVAL;
	}
	pr_debug("%s request on %s %d %pS\n",
		 __func__, clk->name, clk->id, clk->dev);

	spin_lock(&clocks_lock);
	_clk_enable(clk);
	spin_unlock(&clocks_lock);

	return 0;
}

void _clk_disable(struct clk *clk)
{
	if (!clk || IS_ERR(clk))
		return;
	
	if ((--clk->usage) > 0)
		return;

	pr_debug("%s update hardware clock  %s %d %pS\n",
		 __func__, clk->name, clk->id, clk->dev);
	(clk->enable)(clk, 0);
	_clk_disable(clk->parent);
}

void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL) {
		pr_debug("%s request on invalid clock\n", __func__);
		return;
	}

	pr_debug("%s request on %s %d %pS\n",
		 __func__, clk->name, clk->id, clk->dev);

	spin_lock(&clocks_lock);
	_clk_disable(clk);
	spin_unlock(&clocks_lock);
}


unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk))
		return 0;

	if (clk->rate != 0)
		return clk->rate;

	if (clk->ops != NULL && clk->ops->get_rate != NULL)
		return (clk->ops->get_rate)(clk);

	if (clk->parent != NULL)
		return clk_get_rate(clk->parent);

	return clk->rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (!IS_ERR(clk) && clk->ops && clk->ops->round_rate)
		return (clk->ops->round_rate)(clk, rate);

	return rate;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;

	if (IS_ERR(clk))
		return -EINVAL;

	/* We do not default just do a clk->rate = rate as
	 * the clock may have been made this way by choice.
	 */

	WARN_ON(clk->ops == NULL);
	WARN_ON(clk->ops && clk->ops->set_rate == NULL);

	if (clk->ops == NULL || clk->ops->set_rate == NULL)
		return -EINVAL;

	spin_lock(&clocks_lock);
	ret = (clk->ops->set_rate)(clk, rate);
	spin_unlock(&clocks_lock);

	return ret;
}

struct clk *clk_get_parent(struct clk *clk)
{
	return clk->parent;
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;

	if (IS_ERR(clk))
		return -EINVAL;

	spin_lock(&clocks_lock);

	if (clk->ops && clk->ops->set_parent)
		ret = (clk->ops->set_parent)(clk, parent);

	spin_unlock(&clocks_lock);

	return ret;
}

EXPORT_SYMBOL(clk_get);
EXPORT_SYMBOL(clk_put);
EXPORT_SYMBOL(clk_enable);
EXPORT_SYMBOL(clk_disable);
EXPORT_SYMBOL(clk_get_rate);
EXPORT_SYMBOL(clk_round_rate);
EXPORT_SYMBOL(clk_set_rate);
EXPORT_SYMBOL(clk_get_parent);
EXPORT_SYMBOL(clk_set_parent);

/* base clocks */

int clk_default_setrate(struct clk *clk, unsigned long rate)
{
	clk->rate = rate;
	return 0;
}

struct clk_ops clk_ops_def_setrate = {
	.set_rate	= clk_default_setrate,
};

struct clk clk_xtal = {
	.name		= "xtal",
	.id		= -1,
	.rate		= 0,
	.parent		= NULL,
	.ctrlbit	= 0,
};

struct clk clk_ext = {
	.name		= "ext",
	.id		= -1,
};

struct clk clk_epll = {
	.name		= "epll",
	.id		= -1,
};

struct clk clk_mpll = {
	.name		= "mpll",
	.id		= -1,
	.ops		= &clk_ops_def_setrate,
};

struct clk clk_upll = {
	.name		= "upll",
	.id		= -1,
	.parent		= NULL,
	.ctrlbit	= 0,
};

struct clk clk_f = {
	.name		= "fclk",
	.id		= -1,
	.rate		= 0,
	.parent		= &clk_mpll,
	.ctrlbit	= 0,
};

struct clk clk_h = {
	.name		= "hclk",
	.id		= -1,
	.rate		= 0,
	.parent		= NULL,
	.ctrlbit	= 0,
	.ops		= &clk_ops_def_setrate,
};

struct clk clk_p = {
	.name		= "pclk",
	.id		= -1,
	.rate		= 0,
	.parent		= NULL,
	.ctrlbit	= 0,
	.ops		= &clk_ops_def_setrate,
};

struct clk clk_usb_bus = {
	.name		= "usb-bus",
	.id		= -1,
	.rate		= 0,
	.parent		= &clk_upll,
};


struct clk s3c24xx_uclk = {
	.name		= "uclk",
	.id		= -1,
};

/* initialise the clock system */

/**
 * s3c24xx_register_clock() - register a clock
 * @clk: The clock to register
 *
 * Add the specified clock to the list of clocks known by the system.
 */
int s3c24xx_register_clock(struct clk *clk)
{
	if (clk->enable == NULL)
		clk->enable = clk_null_enable;

	/* add to the list of available clocks */

	/* Quick check to see if this clock has already been registered. */
	BUG_ON(clk->list.prev != clk->list.next);

	spin_lock(&clocks_lock);
	if (clk->enable != clk_null_enable) {
		struct clk *c;
		list_for_each_entry(c, &clocks, list) {
			if (c->enable == clk->enable &&
			    c->ctrlbit & clk->ctrlbit) {
				pr_warning("%s: new clock %s, id %d, dev %p "
					   "uses same enable bit as "
					   "%s, id %d, dev %p\n", __func__,
					   clk->name, clk->id, clk->dev,
					   c->name, c->id, c->dev);
			}
			if (!nullstrcmp(c->name, clk->name) &&
			    c->id == clk->id && c->dev == clk->dev) {
				pr_warning("%s: duplicate clock id: "
					   "%s, id %d, dev %p\n", __func__,
					   clk->name, clk->id, clk->dev);
			}
		}
	}
	list_add(&clk->list, &clocks);
	spin_unlock(&clocks_lock);

	return 0;
}

/**
 * s3c24xx_register_clocks() - register an array of clock pointers
 * @clks: Pointer to an array of struct clk pointers
 * @nr_clks: The number of clocks in the @clks array.
 *
 * Call s3c24xx_register_clock() for all the clock pointers contained
 * in the @clks list. Returns the number of failures.
 */
int s3c24xx_register_clocks(struct clk **clks, int nr_clks)
{
	int fails = 0;

	for (; nr_clks > 0; nr_clks--, clks++) {
		if (s3c24xx_register_clock(*clks) < 0) {
			struct clk *clk = *clks;
			printk(KERN_ERR "%s: failed to register %p: %s\n",
			       __func__, clk, clk->name);
			fails++;
		}
	}

	return fails;
}

/**
 * s3c_register_clocks() - register an array of clocks
 * @clkp: Pointer to the first clock in the array.
 * @nr_clks: Number of clocks to register.
 *
 * Call s3c24xx_register_clock() on the @clkp array given, printing an
 * error if it fails to register the clock (unlikely).
 */
void __init s3c_register_clocks(struct clk *clkp, int nr_clks)
{
	int ret;

	for (; nr_clks > 0; nr_clks--, clkp++) {
		ret = s3c24xx_register_clock(clkp);

		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}
}

/**
 * s3c_disable_clocks() - disable an array of clocks
 * @clkp: Pointer to the first clock in the array.
 * @nr_clks: Number of clocks to register.
 *
 * for internal use only at initialisation time. disable the clocks in the
 * @clkp array.
 */

void __init s3c_disable_clocks(struct clk *clkp, int nr_clks)
{
	for (; nr_clks > 0; nr_clks--, clkp++)
		(clkp->enable)(clkp, 0);
}

/* initialise all the clocks */

int __init s3c24xx_register_baseclocks(unsigned long xtal)
{
	printk(KERN_INFO "S3C24XX Clocks, Copyright 2004 Simtec Electronics\n");

	clk_xtal.rate = xtal;

	/* register our clocks */

	if (s3c24xx_register_clock(&clk_xtal) < 0)
		printk(KERN_ERR "failed to register master xtal\n");

	if (s3c24xx_register_clock(&clk_mpll) < 0)
		printk(KERN_ERR "failed to register mpll clock\n");

	if (s3c24xx_register_clock(&clk_upll) < 0)
		printk(KERN_ERR "failed to register upll clock\n");

	if (s3c24xx_register_clock(&clk_f) < 0)
		printk(KERN_ERR "failed to register cpu fclk\n");

	if (s3c24xx_register_clock(&clk_h) < 0)
		printk(KERN_ERR "failed to register cpu hclk\n");

	if (s3c24xx_register_clock(&clk_p) < 0)
		printk(KERN_ERR "failed to register cpu pclk\n");

	return 0;
}

#if defined(CONFIG_PM_DEBUG) && defined(CONFIG_DEBUG_FS)
/* debugfs support to trace clock tree hierarchy and attributes */

static struct dentry *clk_debugfs_root;

static int clk_debugfs_register_one(struct clk *c)
{
	int err;
	struct dentry *d, *child, *child_tmp;
	struct clk *pa = c->parent;
	char s[255];
	char *p = s;
	int i;

	p += sprintf(p, "%s", c->name);

	if (c->id >= 0)
		p += sprintf(p, ":%d", c->id);

	for (i = 1; i < 16; i++) {
		d = debugfs_create_dir(s, clk_debugfs_root);
		if (d)
			break;
		sprintf(p, " copy %d", i);
	}
	if (!d) {
		pr_warning("%s: failed to register %s\n", __func__, s);
		return 0;
	}

	c->dent = d;

	if (pa) {
		d = debugfs_create_symlink("parent",
					   c->dent, pa->dent->d_name.name);
		if (!d) {
			err = -ENOMEM;
			goto err_out;
		}
	}

	d = debugfs_create_u8("usecount", S_IRUGO, c->dent, (u8 *)&c->usage);
	if (!d) {
		err = -ENOMEM;
		goto err_out;
	}

	d = debugfs_create_u32("rate", S_IRUGO, c->dent, (u32 *)&c->rate);
	if (!d) {
		err = -ENOMEM;
		goto err_out;
	}
	return 0;

err_out:
	d = c->dent;
	list_for_each_entry_safe(child, child_tmp, &d->d_subdirs, d_u.d_child)
		debugfs_remove(child);
	debugfs_remove(c->dent);
	return err;
}

static int clk_debugfs_register(struct clk *c)
{
	int err;
	struct clk *pa = c->parent;

	if (pa && !pa->dent) {
		err = clk_debugfs_register(pa);
		if (err)
			return err;
	}

	if (!c->dent) {
		err = clk_debugfs_register_one(c);
		if (err)
			return err;
	}
	return 0;
}

static int __init clk_debugfs_init(void)
{
	struct clk *c;
	struct dentry *d;
	int err;

	d = debugfs_create_dir("clock", NULL);
	if (!d)
		return -ENOMEM;
	clk_debugfs_root = d;

	list_for_each_entry(c, &clocks, list) {
		err = clk_debugfs_register(c);
		if (err)
			goto err_out;
	}
	return 0;

err_out:
	debugfs_remove_recursive(clk_debugfs_root);
	return err;
}
late_initcall(clk_debugfs_init);

#endif /* defined(CONFIG_PM_DEBUG) && defined(CONFIG_DEBUG_FS) */
