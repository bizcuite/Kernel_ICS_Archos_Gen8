/*
 *  linux/arch/arm/plat-omap/cpu-omap.c
 *
 *  CPU frequency scaling for OMAP
 *
 *  Copyright (C) 2005 Nokia Corporation
 *  Written by Tony Lindgren <tony@atomide.com>
 *
 *  Based on cpu-sa1110.c, Copyright (C) 2001 Russell King
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Updated to support OMAP3
 * Rajendra Nayak <rnayak@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/system.h>
#include <mach/clock.h>
#if defined(CONFIG_ARCH_OMAP3) && !defined(CONFIG_OMAP_PM_NONE)
#include <mach/omap-pm.h>
#endif

#define VERY_HI_RATE	900000000

static struct cpufreq_frequency_table *freq_table;

#ifdef CONFIG_ARCH_OMAP1
#define MPU_CLK		"mpu"
#elif CONFIG_ARCH_OMAP3
#define MPU_CLK		"arm_fck"
#else
#define MPU_CLK		"virt_prcm_set"
#endif

static struct clk *mpu_clk;

/* TODO: Add support for SDRAM timing changes */

static ssize_t mpu_freq_show(struct kobject *, struct kobj_attribute *,
              char *);
static ssize_t mpu_freq_store(struct kobject *k, struct kobj_attribute *,
			  const char *buf, size_t n);


static struct kobj_attribute mpu_freq_opp1_attr =
    __ATTR(mpu_freq_opp1, 0644, mpu_freq_show, mpu_freq_store);
static struct kobj_attribute mpu_freq_opp2_attr =
    __ATTR(mpu_freq_opp2, 0644, mpu_freq_show, mpu_freq_store);
static struct kobj_attribute mpu_freq_opp3_attr =
    __ATTR(mpu_freq_opp3, 0644, mpu_freq_show, mpu_freq_store);
static struct kobj_attribute mpu_freq_opp4_attr =
    __ATTR(mpu_freq_opp4, 0644, mpu_freq_show, mpu_freq_store);
static struct kobj_attribute mpu_freq_opp5_attr =
    __ATTR(mpu_freq_opp5, 0644, mpu_freq_show, mpu_freq_store);


int omap_verify_speed(struct cpufreq_policy *policy)
{
	if (freq_table)
		return cpufreq_frequency_table_verify(policy, freq_table);

	if (policy->cpu)
		return -EINVAL;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
				     policy->cpuinfo.max_freq);

	policy->min = clk_round_rate(mpu_clk, policy->min * 1000) / 1000;
	policy->max = clk_round_rate(mpu_clk, policy->max * 1000) / 1000;
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
				     policy->cpuinfo.max_freq);
	return 0;
}

unsigned int omap_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / 1000;
	return rate;
}

static int omap_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
#ifdef CONFIG_ARCH_OMAP1
	struct cpufreq_freqs freqs;
#endif
	int ret = 0;

	/* Ensure desired rate is within allowed range.  Some govenors
	 * (ondemand) will just pass target_freq=0 to get the minimum. */
	if (target_freq < policy->min)
		target_freq = policy->min;
	if (target_freq > policy->max)
		target_freq = policy->max;

#ifdef CONFIG_ARCH_OMAP1
	freqs.old = omap_getspeed(0);
	freqs.new = clk_round_rate(mpu_clk, target_freq * 1000) / 1000;
	freqs.cpu = 0;

	if (freqs.old == freqs.new)
		return ret;
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
#ifdef CONFIG_CPU_FREQ_DEBUG
	printk(KERN_DEBUG "cpufreq-omap: transition: %u --> %u\n",
	       freqs.old, freqs.new);
#endif
	ret = clk_set_rate(mpu_clk, freqs.new * 1000);
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
#elif defined(CONFIG_ARCH_OMAP3) && !defined(CONFIG_OMAP_PM_NONE)
	if (mpu_opps) {
		int ind;
		for (ind = 1; ind <= MAX_VDD1_OPP; ind++) {
			if (mpu_opps[ind].rate/1000 >= target_freq) {
				omap_pm_cpu_set_freq
					(mpu_opps[ind].rate);
				break;
			}
		}
	}
#endif
	return ret;
}

static int __init omap_cpu_init(struct cpufreq_policy *policy)
{
	int result = 0;
    int error = -EINVAL;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = policy->min = policy->max = omap_getspeed(0);


	clk_init_cpufreq_table(&freq_table);
	if (freq_table) {
		result = cpufreq_frequency_table_cpuinfo(policy, freq_table);
		if (!result)
			cpufreq_frequency_table_get_attr(freq_table,
							policy->cpu);
	} else {
		policy->cpuinfo.min_freq = clk_round_rate(mpu_clk, 0) / 1000;
		policy->cpuinfo.max_freq = clk_round_rate(mpu_clk,
							VERY_HI_RATE) / 1000;
	}

	clk_set_rate(mpu_clk, policy->cpuinfo.max_freq * 1000);

	policy->min = policy->cpuinfo.min_freq;
	policy->max = policy->cpuinfo.max_freq;
	policy->cur = omap_getspeed(0);

	/* Program the actual transition time for worstcase */
	policy->cpuinfo.transition_latency = 30 * 1000;

    error = sysfs_create_file(power_kobj, &mpu_freq_opp1_attr.attr);
	if (error) {
		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

	error = sysfs_create_file(power_kobj, &mpu_freq_opp2_attr.attr);
	if (error) {
		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

	error = sysfs_create_file(power_kobj, &mpu_freq_opp3_attr.attr);
	if (error) {
		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

	error = sysfs_create_file(power_kobj, &mpu_freq_opp4_attr.attr);
	if (error) {
		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

	error = sysfs_create_file(power_kobj, &mpu_freq_opp5_attr.attr);
	if (error) {
		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	return 0;
}

static int omap_cpu_exit(struct cpufreq_policy *policy)
{
	clk_put(mpu_clk);
	return 0;
}

static struct freq_attr *omap_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver omap_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= omap_verify_speed,
	.target		= omap_target,
	.get		= omap_getspeed,
	.init		= omap_cpu_init,
	.exit		= omap_cpu_exit,
	.name		= "omap",
	.attr		= omap_cpufreq_attr,
};

static int __init omap_cpufreq_init(void)
{
	return cpufreq_register_driver(&omap_driver);
}

static ssize_t mpu_freq_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
    struct omap_opp *mpu_table = omap_get_mpu_rate_table();

    if ( attr == &mpu_freq_opp1_attr) {
        return sprintf(buf, "%lu\n", mpu_table[1].rate / (1000*1000));
    }
    if (attr == &mpu_freq_opp2_attr) {
        return sprintf(buf, "%lu\n", mpu_table[2].rate / (1000*1000));
    }
    if (attr == &mpu_freq_opp3_attr) {
        return sprintf(buf, "%lu\n", mpu_table[3].rate / (1000*1000));
    }
    if (attr == &mpu_freq_opp4_attr) {
        return sprintf(buf, "%lu\n", mpu_table[4].rate / (1000*1000));
    }
    if (attr == &mpu_freq_opp5_attr) {
        return sprintf(buf, "%lu\n", mpu_table[5].rate / (1000*1000));
    }
}

static ssize_t mpu_freq_store(struct kobject *k,
        struct kobj_attribute *attr, const char *buf, size_t n)
{
    unsigned int freq;
    struct omap_opp *mpu_table = omap_get_mpu_rate_table();

    if (sscanf(buf, "%u", &freq) == 1) {
        if (freq > 100 && freq < 2000) {
            //Convert Megahertz to hertz
            freq *= 1000*1000;

            if ( attr == &mpu_freq_opp1_attr) {
                mpu_table[1].rate = freq;
            } else if (attr == &mpu_freq_opp2_attr) {
                mpu_table[2].rate = freq;
            } else if (attr == &mpu_freq_opp3_attr) {
                mpu_table[3].rate = freq;
            } else if (attr == &mpu_freq_opp4_attr) {
                mpu_table[4].rate = freq;
            } else if (attr == &mpu_freq_opp5_attr) {
                mpu_table[5].rate = freq;
            }
            clk_init_cpufreq_table(&freq_table);
            struct cpufreq_policy *policy = cpufreq_cpu_get(0);
            if(policy)
                cpufreq_frequency_table_cpuinfo(policy, freq_table);
        } else
            return -EINVAL;
    } else
        return -EINVAL;
    return n;
}


late_initcall(omap_cpufreq_init);

/*
 * if ever we want to remove this, upon cleanup call:
 *
 * cpufreq_unregister_driver()
 * cpufreq_frequency_table_put_attr()
 */

