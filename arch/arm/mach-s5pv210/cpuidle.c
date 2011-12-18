/*
 * arch/arm/mach-s5pv210/cpuidle.c
 *
 * Copyright (c) Samsung Electronics Co. Ltd
 *           (c) 2011 Ezekeel <notezekeel@googlemail.com>
 *
 * CPU idle driver for S5PV210
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>

#include <mach/map.h>
#include <mach/regs-irq.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>
#include <plat/pm.h>
#include <plat/devs.h>

#ifdef CONFIG_CPU_DIDLE
#include <linux/dma-mapping.h>
#include <linux/deep_idle.h>

#include <plat/regs-otg.h>
#include <mach/cpuidle.h>
#include <mach/power-domain.h>

extern bool suspend_ongoing(void);
extern bool bt_is_running(void);
extern bool gps_is_running(void);
extern bool vibrator_is_running(void);

/*
 * For saving & restoring VIC register before entering
 * didle mode
 */
static unsigned long vic_regs[4];
static unsigned long *regs_save;
static dma_addr_t phy_regs_save;

#define MAX_CHK_DEV	0xf

/*
 * Specific device list for checking before entering
 * didle mode
 */
struct check_device_op {
	void __iomem		*base;
	struct platform_device	*pdev;
};

/* Array of checking devices list */
static struct check_device_op chk_dev_op[] = {
#if defined(CONFIG_S3C_DEV_HSMMC)
	{.base = 0, .pdev = &s3c_device_hsmmc0},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC1)
	{.base = 0, .pdev = &s3c_device_hsmmc1},
#endif
#if 0
	{.base = 0, .pdev = &s3c_device_hsmmc2},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC3)
	{.base = 0, .pdev = &s3c_device_hsmmc3},
#endif
	{.base = 0, .pdev = NULL},
};

#define S3C_HSMMC_PRNSTS	(0x24)
#define S3C_HSMMC_CLKCON	(0x2c)
#define S3C_HSMMC_CMD_INHIBIT	0x00000001
#define S3C_HSMMC_DATA_INHIBIT	0x00000002
#define S3C_HSMMC_CLOCK_CARD_EN	0x0004

static int sdmmc_dev_num;
/* If SD/MMC interface is working: return = 1 or not 0 */
static int check_sdmmc_op(unsigned int ch)
{
	unsigned int reg1, reg2;
	void __iomem *base_addr;

	if (unlikely(ch > sdmmc_dev_num)) {
		printk(KERN_ERR "Invalid ch[%d] for SD/MMC\n", ch);
		return 0;
	}

	base_addr = chk_dev_op[ch].base;
	/* Check CMDINHDAT[1] and CMDINHCMD [0] */
	reg1 = readl(base_addr + S3C_HSMMC_PRNSTS);
	/* Check CLKCON [2]: ENSDCLK */
	reg2 = readl(base_addr + S3C_HSMMC_CLKCON);

	return !!(reg1 & (S3C_HSMMC_CMD_INHIBIT | S3C_HSMMC_DATA_INHIBIT)) ||
	       (reg2 & (S3C_HSMMC_CLOCK_CARD_EN));
}

/* Check all sdmmc controller */
static int loop_sdmmc_check(void)
{
	unsigned int iter;

	for (iter = 0; iter < sdmmc_dev_num + 1; iter++) {
		if (check_sdmmc_op(iter))
			return 1;
	}
	return 0;
}

/*
 * Check USBOTG is working or not
 * GOTGCTL(0xEC000000)
 * BSesVld (Indicates the Device mode transceiver status)
 * BSesVld =	1b : B-session is valid
 *		0b : B-session is not valid
 */
static int check_usbotg_op(void)
{
	unsigned int val;

	val = __raw_readl(S3C_UDC_OTG_GOTGCTL);

	return val & (B_SESSION_VALID);
}

/*
 * Check power gating : LCD, CAM, TV, MFC, G3D
 * Check clock gating : DMA, USBHOST, I2C
 */
extern volatile int s5p_rp_is_running;
extern int s5p_rp_get_op_level(void);

static int check_power_clock_gating(void)
{
	unsigned long val;

	/* check power gating */
	val = __raw_readl(S5P_NORMAL_CFG);
	if (val & (S5PV210_PD_LCD | S5PV210_PD_CAM | S5PV210_PD_TV
				  | S5PV210_PD_MFC | S5PV210_PD_G3D))
		return 1;

#ifdef CONFIG_SND_S5P_RP
	if (s5p_rp_get_op_level())
		return 1;
#endif
	/* check clock gating */
	val = __raw_readl(S5P_CLKGATE_IP0);
	if (val & (S5P_CLKGATE_IP0_MDMA | S5P_CLKGATE_IP0_PDMA0
					| S5P_CLKGATE_IP0_PDMA1))
		return 1;

	val = __raw_readl(S5P_CLKGATE_IP1);
	if (val & S5P_CLKGATE_IP1_USBHOST)
		return 1;

	val = __raw_readl(S5P_CLKGATE_IP3);
	if (val & (S5P_CLKGATE_IP3_I2C0 | S5P_CLKGATE_IP3_I2C_HDMI_DDC
					| S5P_CLKGATE_IP3_I2C2))
		return 1;

	return 0;
}

/*
 * Skipping enter the didle mode when RTC & I2S interrupts be issued
 * during critical section of entering didle mode (around 20ms).
 */
#ifdef CONFIG_S5P_INTERNAL_DMA
static int check_idmapos(void)
{
	dma_addr_t src;

	i2sdma_getpos(&src);
	src = src & 0x3FFF;
	src = 0x4000 - src;

	return src < 0x150;
}
#endif

static int check_rtcint(void)
{
	unsigned int current_cnt = get_rtc_cnt();

	return current_cnt < 0x40;
}

/*
 * Before entering, didle mode GPIO Powe Down Mode
 * Configuration register has to be set with same state
 * in Normal Mode
 */
#define GPIO_OFFSET		0x20
#define GPIO_CON_PDN_OFFSET	0x10
#define GPIO_PUD_PDN_OFFSET	0x14
#define GPIO_PUD_OFFSET		0x08

static unsigned int pud_pdn[(S5PV210_MP28_BASE - S5PV210_GPA0_BASE) / GPIO_OFFSET + 1];
static unsigned int con_pdn[(S5PV210_MP28_BASE - S5PV210_GPA0_BASE) / GPIO_OFFSET + 1];

static void s5p_gpio_pdn_conf(void)
{
	void __iomem *gpio_base = S5PV210_GPA0_BASE;
	unsigned int val;
	int i = 0;

	do {
		/* Save power down control state */
		con_pdn[i] = __raw_readl(gpio_base + GPIO_CON_PDN_OFFSET);
		/* Keep the previous state in didle mode */
		__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

		/* Save power down pull up-down state */
		pud_pdn[i] = __raw_readl(gpio_base + GPIO_PUD_PDN_OFFSET);
		/* Pull up-down state in didle is same as normal */
		val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
		__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;
		i++;

	} while (gpio_base <= S5PV210_MP28_BASE);

	return;
}

static void s5p_gpio_restore_conf(void)
{
	void __iomem *gpio_base = S5PV210_GPA0_BASE;
	int i = 0;

	do {
		/* Restore power down control state */
		__raw_writel(con_pdn[i], gpio_base + GPIO_CON_PDN_OFFSET);

		/* Restore power down pull up-down state */
		__raw_writel(pud_pdn[i], gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;
		i++;

	} while (gpio_base <= S5PV210_MP28_BASE);

	return;
}

static void s5p_enter_didle(bool top_on)
{
	unsigned long tmp;
	unsigned long save_eint_mask;

	/* store the physical address of the register recovery block */
	__raw_writel(phy_regs_save, S5P_INFORM2);

	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(s5pv210_didle_resume), S5P_INFORM0);

	/* Save current VIC_INT_ENABLE register*/
	vic_regs[0] = __raw_readl(S5P_VIC0REG(VIC_INT_ENABLE));
	vic_regs[1] = __raw_readl(S5P_VIC1REG(VIC_INT_ENABLE));
	vic_regs[2] = __raw_readl(S5P_VIC2REG(VIC_INT_ENABLE));
	vic_regs[3] = __raw_readl(S5P_VIC3REG(VIC_INT_ENABLE));

	/* Disable all interrupt through VIC */
	__raw_writel(0xffffffff, S5P_VIC0REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5P_VIC1REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5P_VIC2REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5P_VIC3REG(VIC_INT_ENABLE_CLEAR));

	if (!top_on) {
	    /* GPIO Power Down Control */
	    s5p_gpio_pdn_conf();
	}

	/*
	 * Configure external interrupt wakeup mask
	 * We use the same wakeup mask as for sleep state plus make sure 
	 * that at least XEINT[22] = GPH2[6] = GPIO_nPOWER = GPIO_N_POWER
	 * and XEINT[29] = GPH3[5] = GPIO_OK_KEY are enabled
	 */
	save_eint_mask = __raw_readl(S5P_EINT_WAKEUP_MASK);
	tmp = s3c_irqwake_eintmask;
	tmp &= ~((1<<22) | (1<<29));
	__raw_writel(tmp, S5P_EINT_WAKEUP_MASK);

	/* Clear wakeup status register */
	tmp = __raw_readl(S5P_WAKEUP_STAT);
	__raw_writel(tmp, S5P_WAKEUP_STAT);

	/*
	 * Wakeup source configuration for didle
	 * We use the same wakeup mask as for sleep state plus make
	 * sure that at least RTC ALARM, RTC TICK, KEY, I2S and ST are
	 * enabled as wakeup sources
	 */
	tmp = s3c_irqwake_intmask;
	tmp &= ~((1<<1) | (1<<2) | (1<<5) | (1<<13) | (1<<14));
	__raw_writel(tmp, S5P_WAKEUP_MASK);

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~(0x3fU << 26);
	if (top_on) {
	    /*
	     * IDLE config register set
	     * TOP_LOGIC = ON
	     * TOP_MEMORY = ON
	     * ARM_L2CACHE = Retention
	     * CFG_DIDLE = DEEP
	     */
	    tmp |= ((2<<30) | (2<<28) | (1<<26) | (1<<0));
	} else {
	    /*
	     * IDLE config register set
	     * TOP_LOGIC = Retention
	     * TOP_MEMORY = Retention
	     * ARM_L2CACHE = Retention
	     * CFG_DIDLE = DEEP
	     */
	    tmp |= ((1<<30) | (1<<28) | (1<<26) | (1<<0));
	}
	__raw_writel(tmp, S5P_IDLE_CFG);

	/* Power mode Config setting */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_IDLE;
	__raw_writel(tmp, S5P_PWR_CFG);

	/* To check VIC Status register before enter didle mode */
	if ((__raw_readl(S5P_VIC0REG(VIC_RAW_STATUS)) & vic_regs[0]) |
	    (__raw_readl(S5P_VIC1REG(VIC_RAW_STATUS)) & vic_regs[1]) |
	    (__raw_readl(S5P_VIC2REG(VIC_RAW_STATUS)) & vic_regs[2]) |
	    (__raw_readl(S5P_VIC3REG(VIC_RAW_STATUS)) & vic_regs[3]))
		goto skipped_didle;

	/* SYSCON_INT_DISABLE */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= S5P_OTHER_SYSC_INTOFF;
	__raw_writel(tmp, S5P_OTHERS);

	/*
	 * s5pv210_didle_save will also act as our return point from when
	 * we resume as it saves its own register state and restore it
	 * during the resume.
	 */
	s5pv210_didle_save(regs_save);

	/* restore the cpu state using the kernel's cpu init code. */
	cpu_init();

skipped_didle:
	__raw_writel(save_eint_mask, S5P_EINT_WAKEUP_MASK);

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~((3<<30) | (3<<28) | (3<<26) | (1<<0));
	tmp |= ((2<<30) | (2<<28));
	__raw_writel(tmp, S5P_IDLE_CFG);

	/* Power mode Config setting */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	__raw_writel(tmp, S5P_PWR_CFG);

	if (!top_on) {
	    /* Release retention GPIO/CF/MMC/UART IO */
	    tmp = __raw_readl(S5P_OTHERS);
	    tmp |= (S5P_OTHERS_RET_IO | S5P_OTHERS_RET_CF |	\
		    S5P_OTHERS_RET_MMC | S5P_OTHERS_RET_UART);
	    __raw_writel(tmp, S5P_OTHERS);
	}

	if (!top_on) {
	    /* Restore GPIO Power Down Configuration */
	    s5p_gpio_restore_conf();
	}

	__raw_writel(vic_regs[0], S5P_VIC0REG(VIC_INT_ENABLE));
	__raw_writel(vic_regs[1], S5P_VIC1REG(VIC_INT_ENABLE));
	__raw_writel(vic_regs[2], S5P_VIC2REG(VIC_INT_ENABLE));
	__raw_writel(vic_regs[3], S5P_VIC3REG(VIC_INT_ENABLE));
}
#endif

static void s5p_enter_idle(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~((3U<<30)|(3<<28)|(1<<0));
	tmp |= ((2U<<30)|(2<<28));
	__raw_writel(tmp, S5P_IDLE_CFG);

	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	__raw_writel(tmp, S5P_PWR_CFG);

	cpu_do_idle();
}

/* Actual code that puts the SoC in different idle states */
static int s5p_enter_idle_state(struct cpuidle_device *dev,
				struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;
#ifdef CONFIG_CPU_DIDLE
	int idle_state = 0;
#endif

	local_irq_disable();
	do_gettimeofday(&before);

#ifdef CONFIG_CPU_DIDLE
#ifdef CONFIG_S5P_INTERNAL_DMA
	if (!deepidle_is_enabled() || check_power_clock_gating() || suspend_ongoing() || loop_sdmmc_check() || check_usbotg_op() || check_rtcint() || check_idmapos()) {
#else
	if (!deepidle_is_enabled() || check_power_clock_gating() || suspend_ongoing() || loop_sdmmc_check() || check_usbotg_op() || check_rtcint()) {
#endif
	    s5p_enter_idle();
	} else if (bt_is_running() || gps_is_running() || vibrator_is_running()) {
	    s5p_enter_didle(true);
	    idle_state = 1;
	} else {
	    s5p_enter_didle(false);
	    idle_state = 2;
	}
#else   
	s5p_enter_idle();
#endif

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
	    (after.tv_usec - before.tv_usec);
#ifdef CONFIG_CPU_DIDLE
	report_idle_time(idle_state, idle_time);
#endif
	return idle_time;
}

static DEFINE_PER_CPU(struct cpuidle_device, s5p_cpuidle_device);

static struct cpuidle_driver s5p_idle_driver = {
	.name =         "s5p_idle",
	.owner =        THIS_MODULE,
};

/* Initialize CPU idle by registering the idle states */
static int s5p_init_cpuidle(void)
{
	struct cpuidle_device *device;
	int ret;

#ifdef CONFIG_CPU_DIDLE
	struct resource *res;
	struct platform_device *pdev;
	int i = 0;
#endif

	ret = cpuidle_register_driver(&s5p_idle_driver);
	if (ret) {
		printk(KERN_ERR "%s: Failed registering driver\n", __func__);
		goto err;
	}

	device = &per_cpu(s5p_cpuidle_device, smp_processor_id());
	device->state_count = 1;

	/* Wait for interrupt state */
	device->states[0].enter = s5p_enter_idle_state;
	device->states[0].exit_latency = 1;	/* uS */
	device->states[0].target_residency = 10000;
	device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
#ifdef CONFIG_CPU_DIDLE
	strcpy(device->states[0].name, "(DEEP)IDLE");
	strcpy(device->states[0].desc, "ARM clock/power gating - WFI");
#else
	strcpy(device->states[0].name, "IDLE");
	strcpy(device->states[0].desc, "ARM clock gating - WFI");
#endif

	ret = cpuidle_register_device(device);
	if (ret) {
		printk(KERN_ERR "%s: Failed registering device\n", __func__);
		goto err_register_driver;
	}

#ifdef CONFIG_CPU_DIDLE
	regs_save = dma_alloc_coherent(NULL, 4096, &phy_regs_save, GFP_KERNEL);
	if (regs_save == NULL) {
		printk(KERN_ERR "%s: DMA alloc error\n", __func__);
		ret = -ENOMEM;
		goto err_register_device;
	}
	printk(KERN_INFO "cpuidle: phy_regs_save:0x%x\n", phy_regs_save);

	/* Allocate memory region to access IP's directly */
	for (i = 0 ; i < MAX_CHK_DEV ; i++) {

		pdev = chk_dev_op[i].pdev;

		if (pdev == NULL) {
			sdmmc_dev_num = i - 1;
			break;
		}

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			printk(KERN_ERR "%s: failed to get io memory region\n",
					__func__);
			ret = -EINVAL;
			goto err_alloc;
		}
		chk_dev_op[i].base = ioremap_nocache(res->start, 4096);

		if (!chk_dev_op[i].base) {
			printk(KERN_ERR "failed to remap io region\n");
			ret = -EINVAL;
			goto err_resource;
		}
	}
#endif

	return 0;

#ifdef CONFIG_CPU_DIDLE
err_alloc:
	while (--i >= 0) {
		iounmap(chk_dev_op[i].base);
	}
err_resource:
	dma_free_coherent(NULL, 4096, regs_save, phy_regs_save);
err_register_device:
	cpuidle_unregister_device(device);
#endif
err_register_driver:
	cpuidle_unregister_driver(&s5p_idle_driver);
err:
	return ret;
}

device_initcall(s5p_init_cpuidle);
