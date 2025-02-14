// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2024 Siflower
 *  All Rights Reserved
 */

#include <linux/device.h>
#include <linux/dw_apb_timer.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/cpumask.h>
#include <linux/cpuhotplug.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>

#define NR_TIMERS	4
static struct dw_apb_clock_event_device *dw_ceds[NR_TIMERS];

static const u16 timer_offsets[NR_TIMERS] __initconst = {
	0x0, 0x14, 0x1000, 0x1014,
};

static int sf_timer_starting_cpu(unsigned int cpu)
{
	int ret;

	ret = irq_force_affinity(dw_ceds[cpu]->timer.irq, cpumask_of(cpu));
	if (ret)
		return ret;

	dw_apb_clockevent_register(dw_ceds[cpu]);

	return 0;
}

static int sf_timer_dying_cpu(unsigned int cpu)
{
	struct clock_event_device *clk = &dw_ceds[cpu]->ced;

	return clk->set_state_shutdown(clk);
}

static int __init sf_timer_of_init(struct device_node *np)
{
	void __iomem *base;
	unsigned long rate;
	struct clk *clk;
	int i, ret;

	if (nr_cpu_ids > NR_TIMERS)
		return -EINVAL;

	base = of_iomap(np, 0);
	if (!base)
		return -ENODEV;

	clk = of_clk_get(np, 0);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto err_clk_get;
	}

	ret = clk_prepare_enable(clk);
	if (ret)
		goto err_clk_en;

	rate = clk_get_rate(clk);

	for_each_possible_cpu(i) {
		unsigned int irq = irq_of_parse_and_map(np, i);
		if (!irq) {
			ret = -EINVAL;
			goto err_cevt_init;
		}

		dw_ceds[i] = dw_apb_clockevent_init(i, "siflower-apb-timer",
						    500,
						    base + timer_offsets[i],
						    irq, rate);
		if (!dw_ceds[i]) {
			irq_dispose_mapping(irq);
			ret = -ENXIO;
			goto err_cevt_init;
		}
	}

	/* Register and immediately configure the timer on the boot CPU */
	ret = cpuhp_setup_state(CPUHP_AP_SIFLOWER_TIMER_STARTING,
				"clockevents/siflower/timer:starting",
				sf_timer_starting_cpu, sf_timer_dying_cpu);
	if (ret)
		goto err_cpuhp;

	return 0;
err_cpuhp:
	i = ARRAY_SIZE(dw_ceds) - 1;
err_cevt_init:
	for (; i >= 0; i--) {
		struct dw_apb_clock_event_device *dw_ced = dw_ceds[i];

		if (!dw_ced)
			continue;

		dw_apb_clockevent_stop(dw_ced);
		irq_dispose_mapping(dw_ced->timer.irq);
		kfree(dw_ced);
		dw_ceds[i] = NULL;
	}
	clk_disable_unprepare(clk);
err_clk_en:
	clk_put(clk);
err_clk_get:
	iounmap(base);
	return ret;
}
TIMER_OF_DECLARE(sf_timer, "siflower,sf21h8898-apb-timer", sf_timer_of_init);
