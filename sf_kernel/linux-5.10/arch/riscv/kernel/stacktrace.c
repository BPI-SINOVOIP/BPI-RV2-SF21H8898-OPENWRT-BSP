// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2008 ARM Limited
 * Copyright (C) 2014 Regents of the University of California
 */

#include <linux/export.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/stacktrace.h>
#include <linux/ftrace.h>

register unsigned long sp_in_global __asm__("sp");

#ifdef CONFIG_FRAME_POINTER

struct stackframe {
	unsigned long fp;
	unsigned long ra;
};

extern asmlinkage void handle_exception(void);
extern asmlinkage void handle_syscall(void);

static inline int fp_is_valid(unsigned long fp, unsigned long sp)
{
	unsigned long low, high;

	low = sp + sizeof(struct stackframe);
	high = ALIGN(sp, THREAD_SIZE);

	return !(fp < low || fp > high || fp & 0x07);
}

void notrace noinline
walk_stackframe(struct task_struct *task, struct pt_regs *regs,
		bool (*fn)(unsigned long, void *), void *arg)
{
	unsigned long fp, sp, pc;

	if (regs) {
		fp = frame_pointer(regs);
		sp = user_stack_pointer(regs);
		pc = instruction_pointer(regs);
	} else if (task == NULL || task == current) {
		const register unsigned long current_sp = sp_in_global;
		fp = (unsigned long)__builtin_frame_address(0);
		sp = current_sp;
		pc = (unsigned long)walk_stackframe;
	} else {
		/* task blocked in __switch_to */
		fp = task->thread.s[0];
		sp = task->thread.sp;
		pc = task->thread.ra;
	}

	for (;;) {
		struct stackframe *frame;

		if (unlikely(!__kernel_text_address(pc) || fn(pc, arg)))
			break;

		if (unlikely(!fp_is_valid(fp, sp)))
			break;
		/* Unwind stack frame */
		frame = (struct stackframe *)fp - 1;
		sp = fp;
		if (regs && (regs->epc == pc) && fp_is_valid(frame->ra, sp)) {
			/* We hit function where ra is not saved on the stack */
			fp = frame->ra;
			pc = regs->ra;
		} else {
			fp = READ_ONCE_NOCHECK(frame->fp);
			pc = ftrace_graph_ret_addr(current, NULL, READ_ONCE_NOCHECK(frame->ra),
						   &frame->ra);
			if (pc >= (unsigned long)handle_exception &&
			    pc < (unsigned long)handle_syscall) {
				if (unlikely(!__kernel_text_address(pc) || fn(pc, arg)))
					break;

				pc = ((struct pt_regs *)sp)->epc;
				fp = ((struct pt_regs *)sp)->s0;
			}
		}

	}
}

#else /* !CONFIG_FRAME_POINTER */

void notrace noinline
walk_stackframe(struct task_struct *task, struct pt_regs *regs,
		bool (*fn)(unsigned long, void *), void *arg)
{
	unsigned long sp, pc;
	unsigned long *ksp;

	if (regs) {
		sp = user_stack_pointer(regs);
		pc = instruction_pointer(regs);
	} else if (task == NULL || task == current) {
		sp = sp_in_global;
		pc = (unsigned long)walk_stackframe;
	} else {
		/* task blocked in __switch_to */
		sp = task->thread.sp;
		pc = task->thread.ra;
	}

	if (unlikely(sp & 0x7))
		return;

	ksp = (unsigned long *)sp;
	while (!kstack_end(ksp)) {
		if (__kernel_text_address(pc) && unlikely(fn(pc, arg)))
			break;
		pc = READ_ONCE_NOCHECK(*ksp++) - 0x4;
	}
}

#endif /* CONFIG_FRAME_POINTER */


static bool print_trace_address(unsigned long pc, void *arg)
{
	const char *loglvl = arg;

	print_ip_sym(loglvl, pc);
	return false;
}

void show_stack(struct task_struct *task, unsigned long *sp, const char *loglvl)
{
	pr_cont("Call Trace:\n");
	walk_stackframe(task, NULL, print_trace_address, (void *)loglvl);
}

void dump_backtrace(struct pt_regs *regs, struct task_struct *task,
		    const char *loglvl)
{
	walk_stackframe(task, regs, print_trace_address, (void *)loglvl);
}

static bool save_wchan(unsigned long pc, void *arg)
{
	if (!in_sched_functions(pc)) {
		unsigned long *p = arg;
		*p = pc;
		return true;
	}
	return false;
}

unsigned long get_wchan(struct task_struct *task)
{
	unsigned long pc = 0;

	if (likely(task && task != current && task->state != TASK_RUNNING))
		walk_stackframe(task, NULL, save_wchan, &pc);
	return pc;
}


#ifdef CONFIG_STACKTRACE

static bool __save_trace(unsigned long pc, void *arg, bool nosched)
{
	struct stack_trace *trace = arg;

	if (unlikely(nosched && in_sched_functions(pc)))
		return false;
	if (unlikely(trace->skip > 0)) {
		trace->skip--;
		return false;
	}

	trace->entries[trace->nr_entries++] = pc;
	return (trace->nr_entries >= trace->max_entries);
}

static bool save_trace(unsigned long pc, void *arg)
{
	return __save_trace(pc, arg, false);
}

/*
 * Save stack-backtrace addresses into a stack_trace buffer.
 */
void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
	walk_stackframe(tsk, NULL, save_trace, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace_tsk);

void save_stack_trace(struct stack_trace *trace)
{
	save_stack_trace_tsk(NULL, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace);

#endif /* CONFIG_STACKTRACE */
