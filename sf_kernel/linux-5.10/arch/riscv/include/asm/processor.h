/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#ifndef _ASM_RISCV_PROCESSOR_H
#define _ASM_RISCV_PROCESSOR_H

#include <linux/const.h>

#include <vdso/processor.h>

#include <asm/ptrace.h>

#ifdef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
#define NET_IP_ALIGN	0
#endif

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	PAGE_ALIGN(TASK_SIZE / 3)

#define STACK_TOP		TASK_SIZE
#define STACK_TOP_MAX		STACK_TOP
#define STACK_ALIGN		16

#ifndef __ASSEMBLY__

struct task_struct;
struct pt_regs;

/* CPU-specific state of a task */
struct thread_struct {
	/* Callee-saved registers */
	unsigned long ra;
	unsigned long sp;	/* Kernel mode stack */
	unsigned long s[12];	/* s[0]: frame pointer */
	struct __riscv_d_ext_state fstate;
	unsigned long bad_cause;
#ifdef CONFIG_VECTOR
	struct __riscv_v_state vstate;
#endif
};

#define INIT_THREAD {					\
	.sp = sizeof(init_stack) + (long)&init_stack,	\
}

#define task_pt_regs(tsk)						\
	((struct pt_regs *)(task_stack_page(tsk) + THREAD_SIZE		\
			    - ALIGN(sizeof(struct pt_regs), STACK_ALIGN)))

#define KSTK_EIP(tsk)		(task_pt_regs(tsk)->epc)
#define KSTK_ESP(tsk)		(task_pt_regs(tsk)->sp)


/* Do necessary setup to start up a newly executed thread. */
extern void start_thread(struct pt_regs *regs,
			unsigned long pc, unsigned long sp);

/* Free all resources held by a thread. */
static inline void release_thread(struct task_struct *dead_task)
{
}

extern unsigned long get_wchan(struct task_struct *p);

#ifdef CONFIG_RISCV_ISA_CMO
#define ARCH_HAS_PREFETCH
#define prefetch(x)	asm volatile("prefetch.r (%0)" :: "r"(x))
#define ARCH_HAS_PREFETCHW
#define prefetchw(x)	asm volatile("prefetch.w (%0)" :: "r"(x))
#endif

static inline void wait_for_interrupt(void)
{
	__asm__ __volatile__ ("wfi");
}

struct device_node;
int riscv_of_processor_hartid(struct device_node *node);
int riscv_of_parent_hartid(struct device_node *node);

extern void riscv_fill_hwcap(void);

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PROCESSOR_H */
