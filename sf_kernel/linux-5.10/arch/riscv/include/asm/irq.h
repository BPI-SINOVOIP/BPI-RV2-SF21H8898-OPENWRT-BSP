/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#ifndef _ASM_RISCV_IRQ_H
#define _ASM_RISCV_IRQ_H

#include <linux/interrupt.h>
#include <linux/linkage.h>

#include <asm-generic/irq.h>

void arch_trigger_cpumask_backtrace(const cpumask_t *mask, bool exclude_self);
#define arch_trigger_cpumask_backtrace arch_trigger_cpumask_backtrace

#endif /* _ASM_RISCV_IRQ_H */
