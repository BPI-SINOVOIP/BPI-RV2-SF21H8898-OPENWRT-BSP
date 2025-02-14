// SPDX-License-Identifier: GPL-2.0

#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <asm/mmu_context.h>

static inline void local_flush_tlb_all_asid(unsigned long asid)
{
	__asm__ __volatile__ ("sfence.vma zero, %0"
			:
			: "r" (asid)
			: "memory");
}

static inline void local_flush_tlb_page_asid(unsigned long addr,
		unsigned long asid)
{
	__asm__ __volatile__ ("sfence.vma %0, %1"
			:
			: "r" (addr), "r" (asid)
			: "memory");
}

#ifdef SFENCE_VMA_BROADCAST
void flush_tlb_all(void)
{
	local_flush_tlb_all();
}

void flush_tlb_mm(struct mm_struct *mm)
{
	unsigned long asid = cpu_asid(mm);

	local_flush_tlb_all_asid(asid);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	unsigned long asid = cpu_asid(mm);

	addr &= PAGE_MASK;

	local_flush_tlb_page_asid(addr, asid);
}

void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
			unsigned long end)
{
	unsigned long asid = cpu_asid(vma->vm_mm);

	start &= PAGE_MASK;
	end   += PAGE_SIZE - 1;
	end   &= PAGE_MASK;

	while (start < end) {
		local_flush_tlb_page_asid(start, asid);
		start += PAGE_SIZE;
	}
}
#else

#include <asm/sbi.h>

void flush_tlb_all(void)
{
	sbi_remote_sfence_vma(NULL, 0, -1);
}

/*
 * This function must not be called with cmask being null.
 * Kernel may panic if cmask is NULL.
 */
static void __sbi_tlb_flush_range(struct mm_struct *mm, unsigned long start,
				  unsigned long size)
{
	struct cpumask *cmask = mm_cpumask(mm);
	struct cpumask hmask;
	unsigned int cpuid;
	unsigned long asid;

	if (cpumask_empty(cmask))
		return;

	cpuid = get_cpu();
	asid = cpu_asid(mm);

	if (cpumask_any_but(cmask, cpuid) >= nr_cpu_ids) {
		/* local cpu is the only cpu present in cpumask */
		if (size <= PAGE_SIZE)
			local_flush_tlb_page_asid(start, asid);
		else
			local_flush_tlb_all_asid(asid);
	} else {
		riscv_cpuid_to_hartid_mask(cmask, &hmask);
		sbi_remote_sfence_vma_asid(cpumask_bits(&hmask), start, size,
					   asid);
	}

	put_cpu();
}

void flush_tlb_mm(struct mm_struct *mm)
{
	__sbi_tlb_flush_range(mm, 0, -1);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	__sbi_tlb_flush_range(vma->vm_mm, addr, PAGE_SIZE);
}

void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
		     unsigned long end)
{
	__sbi_tlb_flush_range(vma->vm_mm, start, end - start);
}
#endif
