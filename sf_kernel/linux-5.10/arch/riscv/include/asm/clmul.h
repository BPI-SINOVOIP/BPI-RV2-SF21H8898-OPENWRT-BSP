// SPDX-License-Identifier: GPL-2.0
#ifndef _RISCV_ASM_CLMUL_H
#define _RISCV_ASM_CLMUL_H

#include <linux/compiler_attributes.h>

static __always_inline unsigned long
clmul(unsigned long rs1, unsigned long rs2)
{
	unsigned long rd;

	asm volatile ("clmul	%0, %1, %2"
		      : "=r" (rd)
		      : "r" (rs1), "r" (rs2)
	);
	return rd;
}

static __always_inline unsigned long
clmulh(unsigned long rs1, unsigned long rs2)
{
	unsigned long rd;

	asm volatile ("clmulh	%0, %1, %2"
		      : "=r" (rd)
		      : "r" (rs1), "r" (rs2)
	);
	return rd;
}

static __always_inline unsigned long
clmulr(unsigned long rs1, unsigned long rs2)
{
	unsigned long rd;

	asm volatile ("clmulr	%0, %1, %2"
		      : "=r" (rd)
		      : "r" (rs1), "r" (rs2)
	);
	return rd;
}

#endif
