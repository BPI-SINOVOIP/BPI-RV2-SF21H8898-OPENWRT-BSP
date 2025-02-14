// SPDX-License-Identifier: GPL-2.0

#include <linux/cache.h>
#include <linux/dma-map-ops.h>
#include <linux/genalloc.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/types.h>
#include <linux/version.h>
#include <asm/cache.h>

static void dma_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i = ALIGN_DOWN(start, L1_CACHE_BYTES);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.cipa %0" :: "r"(i));

	asm volatile("sync.s" ::: "memory");
}

static void dma_wb_range(unsigned long start, unsigned long end)
{
	unsigned long i = ALIGN_DOWN(start, L1_CACHE_BYTES);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.cpa %0" :: "r"(i));

	asm volatile("sync.s" ::: "memory");
}

static void dma_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i = ALIGN_DOWN(start, L1_CACHE_BYTES);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.ipa %0" :: "r"(i));

	asm volatile("sync.s" ::: "memory");
}

void arch_dma_prep_coherent(struct page *page, size_t size)
{
	void *ptr = page_address(page);

	memset(ptr, 0, size);
	dma_wbinv_range(virt_to_phys(ptr), virt_to_phys(ptr) + size);
}

static inline void cache_op(phys_addr_t paddr, size_t size,
			    void (*fn)(unsigned long start, unsigned long end))
{
	unsigned long start = (unsigned long)paddr;

	fn(start, start + size);
}

void arch_sync_dma_for_device(phys_addr_t paddr, size_t size,
			      enum dma_data_direction dir)
{
	switch (dir) {
	case DMA_TO_DEVICE:
		cache_op(paddr, size, dma_wb_range);
		break;
	case DMA_FROM_DEVICE:
		cache_op(paddr, size, dma_inv_range);
		break;
	case DMA_BIDIRECTIONAL:
		cache_op(paddr, size, dma_wbinv_range);
		break;
	default:
		BUG();
	}
}

void arch_sync_dma_for_cpu(phys_addr_t paddr, size_t size,
			   enum dma_data_direction dir)
{
	switch (dir) {
	case DMA_TO_DEVICE:
		return;
	case DMA_FROM_DEVICE:
	case DMA_BIDIRECTIONAL:
		cache_op(paddr, size, dma_inv_range);
		break;
	default:
		BUG();
	}
}
