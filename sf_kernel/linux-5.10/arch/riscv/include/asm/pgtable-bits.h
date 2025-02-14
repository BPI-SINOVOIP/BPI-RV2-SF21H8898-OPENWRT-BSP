/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#ifndef _ASM_RISCV_PGTABLE_BITS_H
#define _ASM_RISCV_PGTABLE_BITS_H

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT   (1 << 0)
#define _PAGE_READ      (1 << 1)    /* Readable */
#define _PAGE_WRITE     (1 << 2)    /* Writable */
#define _PAGE_EXEC      (1 << 3)    /* Executable */
#define _PAGE_USER      (1 << 4)    /* User */
#define _PAGE_GLOBAL    (1 << 5)    /* Global */
#define _PAGE_ACCESSED  (1 << 6)    /* Set by hardware on any access */
#define _PAGE_DIRTY     (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT      (1 << 8)    /* Reserved for software */

/*
 * [62:61] Svpbmt Memory Type definitions:
 *
 *  00 - PMA    Normal Cacheable, No change to implied PMA memory type
 *  01 - NC     Non-cacheable, idempotent, weakly-ordered Main Memory
 *  10 - IO     Non-cacheable, non-idempotent, strongly-ordered I/O memory
 *  11 - Rsvd   Reserved for future standard use
 */
#define _PAGE_NOCACHE_SVPBMT	(1UL << 61)
#define _PAGE_IO_SVPBMT		(1UL << 62)
#define _PAGE_MTMASK_SVPBMT	(_PAGE_NOCACHE_SVPBMT | _PAGE_IO_SVPBMT)

/*
 * [63:59] T-Head Memory Type definitions:
 * bit[63] SO - Strong Order
 * bit[62] C - Cacheable
 * bit[61] B - Bufferable
 * bit[60] SH - Shareable
 * bit[59] Sec - Trustable
 * 00110 - NC   Weakly-ordered, Non-cacheable, Bufferable, Shareable, Non-trustable
 * 01110 - PMA  Weakly-ordered, Cacheable, Bufferable, Shareable, Non-trustable
 * 10010 - IO   Strongly-ordered, Non-cacheable, Non-bufferable, Shareable, Non-trustable
 */
#define _PAGE_PMA_THEAD		((1UL << 62) | (1UL << 61) | (1UL << 60))
#define _PAGE_NOCACHE_THEAD	((1UL << 61) | (1UL << 60))
#define _PAGE_IO_THEAD		((1UL << 63) | (1UL << 60))
#define _PAGE_MTMASK_THEAD	(_PAGE_PMA_THEAD | _PAGE_IO_THEAD | (1UL << 59))

#define _PAGE_SPECIAL   _PAGE_SOFT
#define _PAGE_TABLE     _PAGE_PRESENT

/*
 * _PAGE_PROT_NONE is set on not-present pages (and ignored by the hardware) to
 * distinguish them from swapped out pages
 */
#define _PAGE_PROT_NONE _PAGE_READ

#define _PAGE_PFN_SHIFT 10

/* Set of bits to preserve across pte_modify() */
#define _PAGE_CHG_MASK  (~(unsigned long)(_PAGE_PRESENT | _PAGE_READ |	\
					  _PAGE_WRITE | _PAGE_EXEC |	\
					  _PAGE_USER | _PAGE_GLOBAL |	\
					  _PAGE_MTMASK_SVPBMT ))

#endif /* _ASM_RISCV_PGTABLE_BITS_H */
