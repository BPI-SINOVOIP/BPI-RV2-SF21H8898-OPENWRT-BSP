/* SPDX-License-Identifier: GPL-2.0
 *
 * page_pool.c
 *	Author:	Jesper Dangaard Brouer <netoptimizer@brouer.com>
 *	Copyright (C) 2016 Red Hat, Inc.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>

#include <net/page_pool.h>
#include <net/xdp.h>

#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/page-flags.h>
#include <linux/mm.h> /* for __put_page() */
#include <linux/poison.h>

#include <trace/events/page_pool.h>

#define DEFER_TIME (msecs_to_jiffies(1000))
#define DEFER_WARN_INTERVAL (60 * HZ)

#define BIAS_MAX	LONG_MAX

#ifdef CONFIG_PAGE_POOL_STATS
/* alloc_stat_inc is intended to be used in softirq context */
#define alloc_stat_inc(pool, __stat)	(pool->alloc_stats.__stat++)
/* recycle_stat_inc is safe to use when preemption is possible. */
#define recycle_stat_inc(pool, __stat)							\
	do {										\
		struct page_pool_recycle_stats __percpu *s = pool->recycle_stats;	\
		this_cpu_inc(s->__stat);						\
	} while (0)

bool page_pool_get_stats(struct page_pool *pool,
			 struct page_pool_stats *stats)
{
	int cpu = 0;

	if (!stats)
		return false;

	/* The caller is responsible to initialize stats. */
	stats->alloc_stats.fast += pool->alloc_stats.fast;
	stats->alloc_stats.slow += pool->alloc_stats.slow;
	stats->alloc_stats.empty += pool->alloc_stats.empty;
	stats->alloc_stats.refill += pool->alloc_stats.refill;
	stats->alloc_stats.waive += pool->alloc_stats.waive;

	for_each_possible_cpu(cpu) {
		const struct page_pool_recycle_stats *pcpu =
			per_cpu_ptr(pool->recycle_stats, cpu);

		stats->recycle_stats.cached += pcpu->cached;
		stats->recycle_stats.cache_full += pcpu->cache_full;
		stats->recycle_stats.ring += pcpu->ring;
		stats->recycle_stats.ring_full += pcpu->ring_full;
		stats->recycle_stats.released_refcnt += pcpu->released_refcnt;
	}

	return true;
}
EXPORT_SYMBOL(page_pool_get_stats);

#else
#define alloc_stat_inc(pool, __stat)
#define recycle_stat_inc(pool, __stat)
#define recycle_stat_add(pool, __stat, val)
#endif

static int page_pool_init(struct page_pool *pool,
			  const struct page_pool_params *params)
{
	unsigned int ring_qsize = 1024; /* Default */

	memcpy(&pool->p, params, sizeof(pool->p));

	/* Validate only known flags were used */
	if (pool->p.flags & ~(PP_FLAG_ALL))
		return -EINVAL;

	if (pool->p.pool_size)
		ring_qsize = pool->p.pool_size;

	/* Sanity limit mem that can be pinned down */
	if (ring_qsize > 32768)
		return -E2BIG;

	/* DMA direction is either DMA_FROM_DEVICE or DMA_BIDIRECTIONAL.
	 * DMA_BIDIRECTIONAL is for allowing page used for DMA sending,
	 * which is the XDP_TX use-case.
	 */
	if (pool->p.flags & PP_FLAG_DMA_MAP) {
		if ((pool->p.dma_dir != DMA_FROM_DEVICE) &&
		    (pool->p.dma_dir != DMA_BIDIRECTIONAL))
			return -EINVAL;
	}

	if (pool->p.flags & PP_FLAG_DMA_SYNC_DEV) {
		/* In order to request DMA-sync-for-device the page
		 * needs to be mapped
		 */
		if (!(pool->p.flags & PP_FLAG_DMA_MAP))
			return -EINVAL;

		if (!pool->p.max_len)
			return -EINVAL;

		/* pool->p.offset has to be set according to the address
		 * offset used by the DMA engine to start copying rx data
		 */
	}

	if (PAGE_POOL_DMA_USE_PP_FRAG_COUNT &&
	    pool->p.flags & PP_FLAG_PAGE_FRAG)
		return -EINVAL;

#ifdef CONFIG_PAGE_POOL_STATS
	pool->recycle_stats = alloc_percpu(struct page_pool_recycle_stats);
	if (!pool->recycle_stats)
		return -ENOMEM;
#endif

	if (ptr_ring_init(&pool->ring, ring_qsize, GFP_KERNEL) < 0)
		return -ENOMEM;

	atomic_set(&pool->pages_state_release_cnt, 0);

	/* Driver calling page_pool_create() also call page_pool_destroy() */
	refcount_set(&pool->user_cnt, 1);

	if (pool->p.flags & PP_FLAG_DMA_MAP)
		get_device(pool->p.dev);

	return 0;
}

struct page_pool *page_pool_create(const struct page_pool_params *params)
{
	struct page_pool *pool;
	int err;

	pool = kzalloc_node(sizeof(*pool), GFP_KERNEL, params->nid);
	if (!pool)
		return ERR_PTR(-ENOMEM);

	err = page_pool_init(pool, params);
	if (err < 0) {
		pr_warn("%s() gave up with errno %d\n", __func__, err);
		kfree(pool);
		return ERR_PTR(err);
	}

	return pool;
}
EXPORT_SYMBOL(page_pool_create);

static void page_pool_return_page(struct page_pool *pool, struct page *page);

noinline
static struct page *page_pool_refill_alloc_cache(struct page_pool *pool)
{
	struct ptr_ring *r = &pool->ring;
	struct page *page;
	int pref_nid; /* preferred NUMA node */

	/* Quicker fallback, avoid locks when ring is empty */
	if (__ptr_ring_empty(r)) {
		alloc_stat_inc(pool, empty);
		return NULL;
	}

	/* Softirq guarantee CPU and thus NUMA node is stable. This,
	 * assumes CPU refilling driver RX-ring will also run RX-NAPI.
	 */
#ifdef CONFIG_NUMA
	pref_nid = (pool->p.nid == NUMA_NO_NODE) ? numa_mem_id() : pool->p.nid;
#else
	/* Ignore pool->p.nid setting if !CONFIG_NUMA, helps compiler */
	pref_nid = numa_mem_id(); /* will be zero like page_to_nid() */
#endif

	/* Slower-path: Get pages from locked ring queue */
	spin_lock(&r->consumer_lock);

	/* Refill alloc array, but only if NUMA match */
	do {
		page = __ptr_ring_consume(r);
		if (unlikely(!page))
			break;

		if (likely(page_to_nid(page) == pref_nid)) {
			pool->alloc.cache[pool->alloc.count++] = page;
		} else {
			/* NUMA mismatch;
			 * (1) release 1 page to page-allocator and
			 * (2) break out to fallthrough to alloc_pages_node.
			 * This limit stress on page buddy alloactor.
			 */
			page_pool_return_page(pool, page);
			alloc_stat_inc(pool, waive);
			page = NULL;
			break;
		}
	} while (pool->alloc.count < PP_ALLOC_CACHE_REFILL);

	/* Return last page */
	if (likely(pool->alloc.count > 0)) {
		page = pool->alloc.cache[--pool->alloc.count];
		alloc_stat_inc(pool, refill);
	}

	spin_unlock(&r->consumer_lock);
	return page;
}

/* fast path */
static struct page *__page_pool_get_cached(struct page_pool *pool)
{
	struct page *page;

	/* Caller MUST guarantee safe non-concurrent access, e.g. softirq */
	if (likely(pool->alloc.count)) {
		/* Fast-path */
		page = pool->alloc.cache[--pool->alloc.count];
		alloc_stat_inc(pool, fast);
	} else {
		page = page_pool_refill_alloc_cache(pool);
	}

	return page;
}

static void page_pool_dma_sync_for_device(struct page_pool *pool,
					  struct page *page,
					  unsigned int dma_sync_size)
{
	dma_addr_t dma_addr = page_pool_get_dma_addr(page);

	dma_sync_size = min(dma_sync_size, pool->p.max_len);
	dma_sync_single_range_for_device(pool->p.dev, dma_addr,
					 pool->p.offset, dma_sync_size,
					 pool->p.dma_dir);
}

static bool page_pool_dma_map(struct page_pool *pool, struct page *page)
{
	dma_addr_t dma;

	/* Setup DMA mapping: use 'struct page' area for storing DMA-addr
	 * since dma_addr_t can be either 32 or 64 bits and does not always fit
	 * into page private data (i.e 32bit cpu with 64bit DMA caps)
	 * This mapping is kept for lifetime of page, until leaving pool.
	 */
	dma = dma_map_page_attrs(pool->p.dev, page, 0,
				 (PAGE_SIZE << pool->p.order),
				 pool->p.dma_dir, DMA_ATTR_SKIP_CPU_SYNC);
	if (dma_mapping_error(pool->p.dev, dma))
		return false;

	page_pool_set_dma_addr(page, dma);

	if (pool->p.flags & PP_FLAG_DMA_SYNC_DEV)
		page_pool_dma_sync_for_device(pool, page, pool->p.max_len);

	return true;
}

static void page_pool_set_pp_info(struct page_pool *pool,
				  struct page *page)
{
	page->pp = pool;
	page->pp_magic |= PP_SIGNATURE;
}

static void page_pool_clear_pp_info(struct page *page)
{
	page->pp_magic = 0;
	page->pp = NULL;
}

/* slow path */
noinline
static struct page *__page_pool_alloc_pages_slow(struct page_pool *pool,
						 gfp_t gfp)
{
	struct page *page;

	gfp |= __GFP_COMP;
	page = alloc_pages_node(pool->p.nid, gfp, pool->p.order);
	if (unlikely(!page))
		return NULL;

	if ((pool->p.flags & PP_FLAG_DMA_MAP) &&
	    unlikely(!page_pool_dma_map(pool, page))) {
		put_page(page);
		return NULL;
	}

	alloc_stat_inc(pool, slow);
	page_pool_set_pp_info(pool, page);

	/* Track how many pages are held 'in-flight' */
	pool->pages_state_hold_cnt++;
	trace_page_pool_state_hold(pool, page, pool->pages_state_hold_cnt);
	return page;
}

/* For using page_pool replace: alloc_pages() API calls, but provide
 * synchronization guarantee for allocation side.
 */
struct page *page_pool_alloc_pages(struct page_pool *pool, gfp_t gfp)
{
	struct page *page;

	/* Fast-path: Get a page from cache */
	page = __page_pool_get_cached(pool);
	if (page)
		return page;

	/* Slow-path: cache empty, do real allocation */
	page = __page_pool_alloc_pages_slow(pool, gfp);
	return page;
}
EXPORT_SYMBOL(page_pool_alloc_pages);

/* Calculate distance between two u32 values, valid if distance is below 2^(31)
 *  https://en.wikipedia.org/wiki/Serial_number_arithmetic#General_Solution
 */
#define _distance(a, b)	(s32)((a) - (b))

static s32 page_pool_inflight(struct page_pool *pool)
{
	u32 release_cnt = atomic_read(&pool->pages_state_release_cnt);
	u32 hold_cnt = READ_ONCE(pool->pages_state_hold_cnt);
	s32 inflight;

	inflight = _distance(hold_cnt, release_cnt);

	trace_page_pool_release(pool, inflight, hold_cnt, release_cnt);
	WARN(inflight < 0, "Negative(%d) inflight packet-pages", inflight);

	return inflight;
}

/* Disconnects a page (from a page_pool).  API users can have a need
 * to disconnect a page (from a page_pool), to allow it to be used as
 * a regular page (that will eventually be returned to the normal
 * page-allocator via put_page).
 */
void page_pool_release_page(struct page_pool *pool, struct page *page)
{
	dma_addr_t dma;
	int count;

	if (!(pool->p.flags & PP_FLAG_DMA_MAP))
		/* Always account for inflight pages, even if we didn't
		 * map them
		 */
		goto skip_dma_unmap;

	dma = page_pool_get_dma_addr(page);

	/* When page is unmapped, it cannot be returned to our pool */
	dma_unmap_page_attrs(pool->p.dev, dma,
			     PAGE_SIZE << pool->p.order, pool->p.dma_dir,
			     DMA_ATTR_SKIP_CPU_SYNC);
	page_pool_set_dma_addr(page, 0);
skip_dma_unmap:
	page_pool_clear_pp_info(page);

	/* This may be the last page returned, releasing the pool, so
	 * it is not safe to reference pool afterwards.
	 */
	count = atomic_inc_return_relaxed(&pool->pages_state_release_cnt);
	trace_page_pool_state_release(pool, page, count);
}
EXPORT_SYMBOL(page_pool_release_page);

/* Return a page to the page allocator, cleaning up our state */
static void page_pool_return_page(struct page_pool *pool, struct page *page)
{
	page_pool_release_page(pool, page);

	put_page(page);
	/* An optimization would be to call __free_pages(page, pool->p.order)
	 * knowing page is not part of page-cache (thus avoiding a
	 * __page_cache_release() call).
	 */
}

static bool page_pool_recycle_in_ring(struct page_pool *pool, struct page *page)
{
	int ret;
	/* BH protection not needed if current is softirq */
	if (in_softirq())
		ret = ptr_ring_produce(&pool->ring, page);
	else
		ret = ptr_ring_produce_bh(&pool->ring, page);

	if (!ret) {
		recycle_stat_inc(pool, ring);
		return true;
	}

	return false;
}

/* Only allow direct recycling in special circumstances, into the
 * alloc side cache.  E.g. during RX-NAPI processing for XDP_DROP use-case.
 *
 * Caller must provide appropriate safe context.
 */
static bool page_pool_recycle_in_cache(struct page *page,
				       struct page_pool *pool)
{
	if (unlikely(pool->alloc.count == PP_ALLOC_CACHE_SIZE)) {
		recycle_stat_inc(pool, cache_full);
		return false;
	}

	/* Caller MUST have verified/know (page_ref_count(page) == 1) */
	pool->alloc.cache[pool->alloc.count++] = page;
	recycle_stat_inc(pool, cached);
	return true;
}

/* If the page refcnt == 1, this will try to recycle the page.
 * if PP_FLAG_DMA_SYNC_DEV is set, we'll try to sync the DMA area for
 * the configured size min(dma_sync_size, pool->max_len).
 * If the page refcnt != 1, then the page will be returned to memory
 * subsystem.
 */
static __always_inline struct page *
__page_pool_put_page(struct page_pool *pool, struct page *page,
		     unsigned int dma_sync_size, bool allow_direct)
{
	/* It is not the last user for the page frag case */
	if (pool->p.flags & PP_FLAG_PAGE_FRAG &&
	    page_pool_atomic_sub_frag_count_return(page, 1))
		return NULL;

	/* This allocator is optimized for the XDP mode that uses
	 * one-frame-per-page, but have fallbacks that act like the
	 * regular page allocator APIs.
	 *
	 * refcnt == 1 means page_pool owns page, and can recycle it.
	 *
	 * page is NOT reusable when allocated when system is under
	 * some pressure. (page_is_pfmemalloc)
	 */
	if (likely(page_ref_count(page) == 1 && !page_is_pfmemalloc(page))) {
		/* Read barrier done in page_ref_count / READ_ONCE */

		if (pool->p.flags & PP_FLAG_DMA_SYNC_DEV)
			page_pool_dma_sync_for_device(pool, page,
						      dma_sync_size);

		if (allow_direct && in_softirq() &&
		    page_pool_recycle_in_cache(page, pool))
			return NULL;

		/* Page found as candidate for recycling */
		return page;
	}
	/* Fallback/non-XDP mode: API user have elevated refcnt.
	 *
	 * Many drivers split up the page into fragments, and some
	 * want to keep doing this to save memory and do refcnt based
	 * recycling. Support this use case too, to ease drivers
	 * switching between XDP/non-XDP.
	 *
	 * In-case page_pool maintains the DMA mapping, API user must
	 * call page_pool_put_page once.  In this elevated refcnt
	 * case, the DMA is unmapped/released, as driver is likely
	 * doing refcnt based recycle tricks, meaning another process
	 * will be invoking put_page.
	 */
	/* Do not replace this with page_pool_return_page() */
	recycle_stat_inc(pool, released_refcnt);
	page_pool_release_page(pool, page);
	put_page(page);

	return NULL;
}

void page_pool_put_page(struct page_pool *pool, struct page *page,
			unsigned int dma_sync_size, bool allow_direct)
{
	page = __page_pool_put_page(pool, page, dma_sync_size, allow_direct);
	if (page && !page_pool_recycle_in_ring(pool, page)) {
		/* Cache full, fallback to free pages */
		recycle_stat_inc(pool, ring_full);
		page_pool_return_page(pool, page);
	}
}
EXPORT_SYMBOL(page_pool_put_page);

static struct page *page_pool_drain_frag(struct page_pool *pool,
					 struct page *page)
{
	long drain_count = BIAS_MAX - pool->frag_users;

	/* Some user is still using the page frag */
	if (likely(page_pool_atomic_sub_frag_count_return(page,
							  drain_count)))
		return NULL;

	if (page_ref_count(page) == 1 && !page_is_pfmemalloc(page)) {
		if (pool->p.flags & PP_FLAG_DMA_SYNC_DEV)
			page_pool_dma_sync_for_device(pool, page, -1);

		return page;
	}

	recycle_stat_inc(pool, released_refcnt);
	page_pool_return_page(pool, page);
	return NULL;
}

static void page_pool_free_frag(struct page_pool *pool)
{
	long drain_count = BIAS_MAX - pool->frag_users;
	struct page *page = pool->frag_page;

	pool->frag_page = NULL;

	if (!page ||
	    page_pool_atomic_sub_frag_count_return(page, drain_count))
		return;

	page_pool_return_page(pool, page);
}

struct page *page_pool_alloc_frag(struct page_pool *pool,
				  unsigned int *offset,
				  unsigned int size, gfp_t gfp)
{
	unsigned int max_size = PAGE_SIZE << pool->p.order;
	struct page *page = pool->frag_page;

	if (WARN_ON(!(pool->p.flags & PP_FLAG_PAGE_FRAG) ||
		    size > max_size))
		return NULL;

	size = ALIGN(size, dma_get_cache_alignment());
	*offset = pool->frag_offset;

	if (page && *offset + size > max_size) {
		page = page_pool_drain_frag(pool, page);
		if (page)
			goto frag_reset;
	}

	if (!page) {
		page = page_pool_alloc_pages(pool, gfp);
		if (unlikely(!page)) {
			pool->frag_page = NULL;
			return NULL;
		}

		pool->frag_page = page;

frag_reset:
		pool->frag_users = 1;
		*offset = 0;
		pool->frag_offset = size;
		page_pool_set_frag_count(page, BIAS_MAX);
		return page;
	}

	pool->frag_users++;
	pool->frag_offset = *offset + size;
	return page;
}
EXPORT_SYMBOL(page_pool_alloc_frag);

static void page_pool_empty_ring(struct page_pool *pool)
{
	struct page *page;

	/* Empty recycle ring */
	while ((page = ptr_ring_consume_bh(&pool->ring))) {
		/* Verify the refcnt invariant of cached pages */
		if (!(page_ref_count(page) == 1))
			pr_crit("%s() page_pool refcnt %d violation\n",
				__func__, page_ref_count(page));

		page_pool_return_page(pool, page);
	}
}

static void page_pool_free(struct page_pool *pool)
{
	if (pool->disconnect)
		pool->disconnect(pool);

	ptr_ring_cleanup(&pool->ring, NULL);

	if (pool->p.flags & PP_FLAG_DMA_MAP)
		put_device(pool->p.dev);

#ifdef CONFIG_PAGE_POOL_STATS
	free_percpu(pool->recycle_stats);
#endif
	kfree(pool);
}

static void page_pool_empty_alloc_cache_once(struct page_pool *pool)
{
	struct page *page;

	if (pool->destroy_cnt)
		return;

	/* Empty alloc cache, assume caller made sure this is
	 * no-longer in use, and page_pool_alloc_pages() cannot be
	 * call concurrently.
	 */
	while (pool->alloc.count) {
		page = pool->alloc.cache[--pool->alloc.count];
		page_pool_return_page(pool, page);
	}
}

static void page_pool_scrub(struct page_pool *pool)
{
	page_pool_empty_alloc_cache_once(pool);
	pool->destroy_cnt++;

	/* No more consumers should exist, but producers could still
	 * be in-flight.
	 */
	page_pool_empty_ring(pool);
}

static int page_pool_release(struct page_pool *pool)
{
	int inflight;

	page_pool_scrub(pool);
	inflight = page_pool_inflight(pool);
	if (!inflight)
		page_pool_free(pool);

	return inflight;
}

static void page_pool_release_retry(struct work_struct *wq)
{
	struct delayed_work *dwq = to_delayed_work(wq);
	struct page_pool *pool = container_of(dwq, typeof(*pool), release_dw);
	int inflight;

	inflight = page_pool_release(pool);
	if (!inflight)
		return;

	/* Periodic warning */
	if (time_after_eq(jiffies, pool->defer_warn)) {
		int sec = (s32)((u32)jiffies - (u32)pool->defer_start) / HZ;

		pr_warn("%s() stalled pool shutdown %d inflight %d sec\n",
			__func__, inflight, sec);
		pool->defer_warn = jiffies + DEFER_WARN_INTERVAL;
	}

	/* Still not ready to be disconnected, retry later */
	schedule_delayed_work(&pool->release_dw, DEFER_TIME);
}

void page_pool_use_xdp_mem(struct page_pool *pool, void (*disconnect)(void *))
{
	refcount_inc(&pool->user_cnt);
	pool->disconnect = disconnect;
}

void page_pool_destroy(struct page_pool *pool)
{
	if (!pool)
		return;

	if (!page_pool_put(pool))
		return;

	page_pool_free_frag(pool);

	if (!page_pool_release(pool))
		return;

	pool->defer_start = jiffies;
	pool->defer_warn  = jiffies + DEFER_WARN_INTERVAL;

	INIT_DELAYED_WORK(&pool->release_dw, page_pool_release_retry);
	schedule_delayed_work(&pool->release_dw, DEFER_TIME);
}
EXPORT_SYMBOL(page_pool_destroy);

/* Caller must provide appropriate safe context, e.g. NAPI. */
void page_pool_update_nid(struct page_pool *pool, int new_nid)
{
	struct page *page;

	trace_page_pool_update_nid(pool, new_nid);
	pool->p.nid = new_nid;

	/* Flush pool alloc cache, as refill will check NUMA node */
	while (pool->alloc.count) {
		page = pool->alloc.cache[--pool->alloc.count];
		page_pool_return_page(pool, page);
	}
}
EXPORT_SYMBOL(page_pool_update_nid);

bool page_pool_return_skb_page(struct page *page)
{
	struct page_pool *pp;

	page = compound_head(page);

	/* page->pp_magic is OR'ed with PP_SIGNATURE after the allocation
	 * in order to preserve any existing bits, such as bit 0 for the
	 * head page of compound page and bit 1 for pfmemalloc page, so
	 * mask those bits for freeing side when doing below checking,
	 * and page_is_pfmemalloc() is checked in __page_pool_put_page()
	 * to avoid recycling the pfmemalloc page.
	 */
	if (unlikely((page->pp_magic & ~0x3UL) != PP_SIGNATURE))
		return false;

	pp = page->pp;

	/* Driver set this to memory recycling info. Reset it on recycle.
	 * This will *not* work for NIC using a split-page memory model.
	 * The page will be returned to the pool here regardless of the
	 * 'flipped' fragment being in use or not.
	 */
	page_pool_put_full_page(pp, page, false);

	return true;
}
EXPORT_SYMBOL(page_pool_return_skb_page);
