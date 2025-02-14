/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_RISCV_STACKTRACE_H
#define _ASM_RISCV_STACKTRACE_H

#include <linux/sched.h>
#include <asm/ptrace.h>

extern void dump_backtrace(struct pt_regs *regs, struct task_struct *task,
			   const char *loglvl);

#endif /* _ASM_RISCV_STACKTRACE_H */
