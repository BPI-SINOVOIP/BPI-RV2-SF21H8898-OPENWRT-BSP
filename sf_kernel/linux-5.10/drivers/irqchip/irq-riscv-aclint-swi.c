// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Western Digital Corporation or its affiliates.
 */

#define pr_fmt(fmt) "aclint-swi: " fmt
#include <linux/clocksource.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/smp.h>

static u32 __iomem *aclint_swi_base;

static void aclint_swi_send(const struct cpumask *target)
{
	unsigned int cpu;

	for_each_cpu(cpu, target)
		writel(1, aclint_swi_base + cpuid_to_hartid_map(cpu));
}

static void aclint_swi_clear(void)
{
	writel(0, aclint_swi_base + cpuid_to_hartid_map(smp_processor_id()));
}

static struct riscv_ipi_ops aclint_ipi_ops __read_mostly = {
	.ipi_inject = aclint_swi_send,
	.ipi_clear = aclint_swi_clear,
};

static int __init aclint_swi_of_init(struct device_node *node)
{
	void __iomem *base;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%pOFP: could not map registers\n", node);
		return -ENODEV;
	}

	aclint_swi_base = base;

	riscv_set_ipi_ops(&aclint_ipi_ops);
	aclint_swi_clear();

	return 0;
}

#ifdef CONFIG_RISCV_M_MODE
TIMER_OF_DECLARE(riscv_aclint_swi, "thead,c900-aclint-mswi", aclint_swi_of_init);
#else
TIMER_OF_DECLARE(riscv_aclint_swi, "thead,c900-aclint-sswi", aclint_swi_of_init);
#endif
