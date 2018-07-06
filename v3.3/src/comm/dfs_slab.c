#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dfs_slab.h"
#include "pub_log.h"

#define DFS_SLAB_PAGE_MASK   3
#define DFS_SLAB_PAGE        0
#define DFS_SLAB_BIG         1
#define DFS_SLAB_EXACT       2
#define DFS_SLAB_SMALL       3

#if defined(__SYSTYPE_32__)
#define DFS_SLAB_PAGE_FREE   0
#define DFS_SLAB_PAGE_BUSY   0xffffffff
#define DFS_SLAB_PAGE_START  0x80000000

#define DFS_SLAB_SHIFT_MASK  0x0000000f
#define DFS_SLAB_MAP_MASK    0xffff0000
#define DFS_SLAB_MAP_SHIFT   16

#define DFS_SLAB_BUSY        0xffffffff

#else /* __SYSTYPE_64__ */

#define DFS_SLAB_PAGE_FREE   0
#define DFS_SLAB_PAGE_BUSY   0xffffffffffffffff
#define DFS_SLAB_PAGE_START  0x8000000000000000

#define DFS_SLAB_SHIFT_MASK  0x000000000000000f
#define DFS_SLAB_MAP_MASK    0xffffffff00000000
#define DFS_SLAB_MAP_SHIFT   32

#define DFS_SLAB_BUSY        0xffffffffffffffff

#endif

#define dfs_memset(buf, c, n)     (void) memset(buf, c, n)

#if (DFS_DEBUG_MALLOC)

#define dfs_slab_junk(p, size)     dfs_memset(p, 0xA5, size)

#elif (DFS_HAVE_DEBUG_MALLOC)

#define dfs_slab_junk(p, size)                                                \
	if (dfs_debug_malloc)          dfs_memset(p, 0xA5, size)

#else

#define dfs_slab_junk(p, size)

#endif

static dfs_slab_page_t *dfs_slab_alloc_pages(dfs_slab_pool_t *pool, dfs_uint_t pages);
static void dfs_slab_free_pages(dfs_slab_pool_t *pool, dfs_slab_page_t *page, dfs_uint_t pages);
static void dfs_slab_error(dfs_slab_pool_t *pool, dfs_uint_t level, char *text);


static dfs_uint_t  dfs_slab_max_size;
static dfs_uint_t  dfs_slab_exact_size;
static dfs_uint_t  dfs_slab_exact_shift;
static dfs_uint_t  dfs_pagesize;
static dfs_uint_t  dfs_pagesize_shift;

int dfs_page_init()
{
	int i = 0;
	dfs_uint_t       n;

	dfs_pagesize_shift = 0;
	dfs_pagesize = getpagesize();
	for (n = dfs_pagesize; n >>= 1; dfs_pagesize_shift++)
	{
		/* void */
		i++;
	}
	return 0;
}

void dfs_slab_init(dfs_slab_pool_t *pool)
{
	u_char           *p;
	size_t            size;
	dfs_int_t         m;
	dfs_uint_t        i, n, pages;
	dfs_slab_page_t  *slots;
	
	dfs_page_init();
	
	/* STUB */
	if (dfs_slab_max_size == 0)
	{
		dfs_slab_max_size = dfs_pagesize / 2;
		dfs_slab_exact_size = dfs_pagesize / (8 * sizeof(uintptr_t));
		for (n = dfs_slab_exact_size; n >>= 1; dfs_slab_exact_shift++)
		{
			/* void */
		}
	}

	pool->min_size = 1 << pool->min_shift;

	p = (u_char *) pool + sizeof(dfs_slab_pool_t);
	size = pool->end - p;

	dfs_slab_junk(p, size);

	slots = (dfs_slab_page_t *) p;
	n = dfs_pagesize_shift - pool->min_shift;

	for (i = 0; i < n; i++)
	{
		slots[i].slab = 0;
		slots[i].next = &slots[i];
		slots[i].prev = 0;
	}

	p += n * sizeof(dfs_slab_page_t);

	pages = (dfs_uint_t) (size / (dfs_pagesize + sizeof(dfs_slab_page_t)));

	dfs_memzero(p, pages * sizeof(dfs_slab_page_t));

	pool->pages = (dfs_slab_page_t *) p;

	pool->free.prev = 0;
	pool->free.next = (dfs_slab_page_t *) p;

	pool->pages->slab = pages;
	pool->pages->next = &pool->free;
	pool->pages->prev = (uintptr_t) &pool->free;

	pool->start = (u_char *)dfs_align_ptr((uintptr_t) p + pages * sizeof(dfs_slab_page_t), dfs_pagesize);

	m = pages - (pool->end - pool->start) / dfs_pagesize;
	if (m > 0)
	{
		pages -= m;
		pool->pages->slab = pages;
	}

	pool->last = pool->pages + pages;

	pool->log_nomem = 1;
	pool->log_ctx = &pool->zero;
	pool->zero = '\0';
}

void *dfs_slab_alloc(dfs_slab_pool_t *pool, size_t size)
{
	void  *p;

	dfs_shmtx_lock(&pool->mutex);

	p = dfs_slab_alloc_locked(pool, size);

	dfs_shmtx_unlock(&pool->mutex);

	return p;
}

void *dfs_slab_alloc_locked(dfs_slab_pool_t *pool, size_t size)
{
	size_t            s;
	uintptr_t         p, n, m, mask, *bitmap;
	dfs_uint_t        i, slot, shift, map;
	dfs_slab_page_t  *page, *prev, *slots;

	if (size > dfs_slab_max_size)
	{
		pub_log_info("[%s][%d] slab alloc size:[%uz]", __FILE__, __LINE__, size);

		page = dfs_slab_alloc_pages(pool, (size >> dfs_pagesize_shift) + ((size % dfs_pagesize) ? 1 : 0));
		if (page)
		{
			p = (page - pool->pages) << dfs_pagesize_shift;
			p += (uintptr_t) pool->start;

		}
		else
		{
			p = 0;
		}

		goto done;
	}

	if (size > pool->min_size)
	{
		shift = 1;
		for (s = size - 1; s >>= 1; shift++) { /* void */ }
		slot = shift - pool->min_shift;

	}
	else
	{
		size = pool->min_size;
		shift = pool->min_shift;
		slot = 0;
	}

	pub_log_info("[%s][%d] slab alloc:[%zu] slot:[%i]", __FILE__, __LINE__, size, slot);

	slots = (dfs_slab_page_t *) ((u_char *) pool + sizeof(dfs_slab_pool_t));
	page = slots[slot].next;

	if (page->next != page)
	{
		if (shift < dfs_slab_exact_shift)
		{
			do
			{
				p = (page - pool->pages) << dfs_pagesize_shift;
				bitmap = (uintptr_t *) (pool->start + p);

				map = (1 << (dfs_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

				for (n = 0; n < map; n++)
				{
					if (bitmap[n] != DFS_SLAB_BUSY)
					{
						for (m = 1, i = 0; m; m <<= 1, i++)
						{
							if ((bitmap[n] & m))
							{
								continue;
							}

							bitmap[n] |= m;

							i = ((n * sizeof(uintptr_t) * 8) << shift)
								+ (i << shift);

							if (bitmap[n] == DFS_SLAB_BUSY)
							{
								for (n = n + 1; n < map; n++)
								{
									if (bitmap[n] != DFS_SLAB_BUSY)
									{
										p = (uintptr_t) bitmap + i;

										goto done;
									}
								}

								prev = (dfs_slab_page_t *)(page->prev & ~DFS_SLAB_PAGE_MASK);
								prev->next = page->next;
								page->next->prev = page->prev;

								page->next = NULL;
								page->prev = DFS_SLAB_SMALL;
							}

							p = (uintptr_t) bitmap + i;

							goto done;
						}
					}
				}

				page = page->next;

			} while (page);

		}
		else if (shift == dfs_slab_exact_shift)
		{
			do
			{
				if (page->slab != DFS_SLAB_BUSY)
				{
					for (m = 1, i = 0; m; m <<= 1, i++)
					{
						if ((page->slab & m))
						{
							continue;
						}

						page->slab |= m;

						if (page->slab == DFS_SLAB_BUSY)
						{
							prev = (dfs_slab_page_t *)
								(page->prev & ~DFS_SLAB_PAGE_MASK);
							prev->next = page->next;
							page->next->prev = page->prev;

							page->next = NULL;
							page->prev = DFS_SLAB_EXACT;
						}

						p = (page - pool->pages) << dfs_pagesize_shift;
						p += i << shift;
						p += (uintptr_t) pool->start;

						goto done;
					}
				}

				page = page->next;

			} while (page);

		}
		else
		{ /* shift > dfs_slab_exact_shift */
			n = dfs_pagesize_shift - (page->slab & DFS_SLAB_SHIFT_MASK);
			n = 1 << n;
			n = ((uintptr_t) 1 << n) - 1;
			mask = n << DFS_SLAB_MAP_SHIFT;

			do
			{
				if ((page->slab & DFS_SLAB_MAP_MASK) != mask)
				{
					for (m = (uintptr_t) 1 << DFS_SLAB_MAP_SHIFT, i = 0;
							m & mask;
							m <<= 1, i++)
					{
						if ((page->slab & m))
						{
							continue;
						}

						page->slab |= m;

						if ((page->slab & DFS_SLAB_MAP_MASK) == mask)
						{
							prev = (dfs_slab_page_t *)(page->prev & ~DFS_SLAB_PAGE_MASK);
							prev->next = page->next;
							page->next->prev = page->prev;

							page->next = NULL;
							page->prev = DFS_SLAB_BIG;
						}

						p = (page - pool->pages) << dfs_pagesize_shift;
						p += i << shift;
						p += (uintptr_t) pool->start;

						goto done;
					}
				}

				page = page->next;

			} while (page);
		}
	}

	page = dfs_slab_alloc_pages(pool, 1);
	if (page)
	{
		if (shift < dfs_slab_exact_shift)
		{
			p = (page - pool->pages) << dfs_pagesize_shift;
			bitmap = (uintptr_t *) (pool->start + p);

			s = 1 << shift;
			n = (1 << (dfs_pagesize_shift - shift)) / 8 / s;

			if (n == 0) {
				n = 1;
			}

			bitmap[0] = (2 << n) - 1;

			map = (1 << (dfs_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

			for (i = 1; i < map; i++)
			{
				bitmap[i] = 0;
			}

			page->slab = shift;
			page->next = &slots[slot];
			page->prev = (uintptr_t) &slots[slot] | DFS_SLAB_SMALL;

			slots[slot].next = page;

			p = ((page - pool->pages) << dfs_pagesize_shift) + s * n;
			p += (uintptr_t) pool->start;

			goto done;

		}
		else if (shift == dfs_slab_exact_shift)
		{
			page->slab = 1;
			page->next = &slots[slot];
			page->prev = (uintptr_t) &slots[slot] | DFS_SLAB_EXACT;

			slots[slot].next = page;

			p = (page - pool->pages) << dfs_pagesize_shift;
			p += (uintptr_t) pool->start;

			goto done;

		}
		else
		{ /* shift > dfs_slab_exact_shift */
			page->slab = ((uintptr_t) 1 << DFS_SLAB_MAP_SHIFT) | shift;
			page->next = &slots[slot];
			page->prev = (uintptr_t) &slots[slot] | DFS_SLAB_BIG;

			slots[slot].next = page;

			p = (page - pool->pages) << dfs_pagesize_shift;
			p += (uintptr_t) pool->start;

			goto done;
		}
	}

	p = 0;

done:

	pub_log_info("[%s][%d] slab alloc: %p", __FILE__, __LINE__, p);

	return (void *) p;
}


void *dfs_slab_calloc(dfs_slab_pool_t *pool, size_t size)
{
	void  *p;

	dfs_shmtx_lock(&pool->mutex);

	p = dfs_slab_calloc_locked(pool, size);

	dfs_shmtx_unlock(&pool->mutex);

	return p;
}


void *dfs_slab_calloc_locked(dfs_slab_pool_t *pool, size_t size)
{
	void  *p;

	p = dfs_slab_alloc_locked(pool, size);
	if (p)
	{
		dfs_memzero(p, size);
	}

	return p;
}


void dfs_slab_free(dfs_slab_pool_t *pool, void *p)
{
	dfs_shmtx_lock(&pool->mutex);

	dfs_slab_free_locked(pool, p);

	dfs_shmtx_unlock(&pool->mutex);
}


void dfs_slab_free_locked(dfs_slab_pool_t *pool, void *p)
{
	size_t            size;
	uintptr_t         slab, m, *bitmap;
	dfs_uint_t        n, type, slot, shift, map;
	dfs_slab_page_t  *slots, *page;

	pub_log_info("[%s][%d] slab free: %p", __FILE__, __LINE__, p);

	if ((u_char *) p < pool->start || (u_char *) p > pool->end)
	{
		pub_log_error("[%s][%d] dfs_slab_free(): outside of pool", __FILE__, __LINE__);
		goto fail;
	}

	n = ((u_char *) p - pool->start) >> dfs_pagesize_shift;
	page = &pool->pages[n];
	slab = page->slab;
	type = page->prev & DFS_SLAB_PAGE_MASK;

	switch (type)
	{

		case DFS_SLAB_SMALL:

			shift = slab & DFS_SLAB_SHIFT_MASK;
			size = 1 << shift;

			if ((uintptr_t) p & (size - 1))
			{
				goto wrong_chunk;
			}

			n = ((uintptr_t) p & (dfs_pagesize - 1)) >> shift;
			m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));
			n /= (sizeof(uintptr_t) * 8);
			bitmap = (uintptr_t *)((uintptr_t) p & ~((uintptr_t) dfs_pagesize - 1));

			if (bitmap[n] & m)
			{
				if (page->next == NULL)
				{
					slots = (dfs_slab_page_t *)
						((u_char *) pool + sizeof(dfs_slab_pool_t));
					slot = shift - pool->min_shift;

					page->next = slots[slot].next;
					slots[slot].next = page;

					page->prev = (uintptr_t) &slots[slot] | DFS_SLAB_SMALL;
					page->next->prev = (uintptr_t) page | DFS_SLAB_SMALL;
				}

				bitmap[n] &= ~m;

				n = (1 << (dfs_pagesize_shift - shift)) / 8 / (1 << shift);

				if (n == 0)
				{
					n = 1;
				}

				if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1))
				{
					goto done;
				}

				map = (1 << (dfs_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

				for (n = 1; n < map; n++)
				{
					if (bitmap[n])
					{
						goto done;
					}
				}

				dfs_slab_free_pages(pool, page, 1);

				goto done;
			}

			goto chunk_already_free;

		case DFS_SLAB_EXACT:

			m = (uintptr_t) 1 << (((uintptr_t) p & (dfs_pagesize - 1)) >> dfs_slab_exact_shift);
			size = dfs_slab_exact_size;

			if ((uintptr_t) p & (size - 1))
			{
				goto wrong_chunk;
			}

			if (slab & m)
			{
				if (slab == DFS_SLAB_BUSY)
				{
					slots = (dfs_slab_page_t *) ((u_char *) pool + sizeof(dfs_slab_pool_t));
					slot = dfs_slab_exact_shift - pool->min_shift;

					page->next = slots[slot].next;
					slots[slot].next = page;

					page->prev = (uintptr_t) &slots[slot] | DFS_SLAB_EXACT;
					page->next->prev = (uintptr_t) page | DFS_SLAB_EXACT;
				}

				page->slab &= ~m;

				if (page->slab)
				{
					goto done;
				}

				dfs_slab_free_pages(pool, page, 1);

				goto done;
			}

			goto chunk_already_free;

		case DFS_SLAB_BIG:

			shift = slab & DFS_SLAB_SHIFT_MASK;
			size = 1 << shift;

			if ((uintptr_t) p & (size - 1))
			{
				goto wrong_chunk;
			}

			m = (uintptr_t) 1 << ((((uintptr_t) p & (dfs_pagesize - 1)) >> shift)
					+ DFS_SLAB_MAP_SHIFT);

			if (slab & m)
			{
				if (page->next == NULL)
				{
					slots = (dfs_slab_page_t *)
						((u_char *) pool + sizeof(dfs_slab_pool_t));
					slot = shift - pool->min_shift;

					page->next = slots[slot].next;
					slots[slot].next = page;

					page->prev = (uintptr_t) &slots[slot] | DFS_SLAB_BIG;
					page->next->prev = (uintptr_t) page | DFS_SLAB_BIG;
				}

				page->slab &= ~m;

				if (page->slab & DFS_SLAB_MAP_MASK)
				{
					goto done;
				}

				dfs_slab_free_pages(pool, page, 1);

				goto done;
			}

			goto chunk_already_free;

		case DFS_SLAB_PAGE:

			if ((uintptr_t) p & (dfs_pagesize - 1))
			{
				goto wrong_chunk;
			}

			if (slab == DFS_SLAB_PAGE_FREE)
			{
				pub_log_error("[%s][%d] dfs_slab_free(): page is already free", __FILE__, __LINE__);
				goto fail;
			}

			if (slab == DFS_SLAB_PAGE_BUSY)
			{
				pub_log_error("[%s][%d] dfs_slab_free(): pointer to wrong page", __FILE__, __LINE__);
				goto fail;
			}

			n = ((u_char *) p - pool->start) >> dfs_pagesize_shift;
			size = slab & ~DFS_SLAB_PAGE_START;

			dfs_slab_free_pages(pool, &pool->pages[n], size);

			dfs_slab_junk(p, size << dfs_pagesize_shift);

			return;
	}

	/* not reached */

	return;

done:

	dfs_slab_junk(p, size);

	return;

wrong_chunk:

	pub_log_error("[%s][%d] dfs_slab_free(): pointer to wrong chunk", __FILE__, __LINE__);

	goto fail;

chunk_already_free:

	pub_log_error("[%s][%d] dfs_slab_free(): chunk is already free", __FILE__, __LINE__);

fail:

	return;
}


static dfs_slab_page_t *dfs_slab_alloc_pages(dfs_slab_pool_t *pool, dfs_uint_t pages)
{
	dfs_slab_page_t  *page, *p;

	for (page = pool->free.next; page != &pool->free; page = page->next)
	{
		if (page->slab >= pages)
		{
			if (page->slab > pages)
			{
				page[page->slab - 1].prev = (uintptr_t) &page[pages];

				page[pages].slab = page->slab - pages;
				page[pages].next = page->next;
				page[pages].prev = page->prev;

				p = (dfs_slab_page_t *) page->prev;
				p->next = &page[pages];
				page->next->prev = (uintptr_t) &page[pages];

			}
			else
			{
				p = (dfs_slab_page_t *) page->prev;
				p->next = page->next;
				page->next->prev = page->prev;
			}

			page->slab = pages | DFS_SLAB_PAGE_START;
			page->next = NULL;
			page->prev = DFS_SLAB_PAGE;

			if (--pages == 0)
			{
				return page;
			}

			for (p = page + 1; pages; pages--)
			{
				p->slab = DFS_SLAB_PAGE_BUSY;
				p->next = NULL;
				p->prev = DFS_SLAB_PAGE;
				p++;
			}

			return page;
		}
	}

	pub_log_error("[%s][%d] dfs_slab_alloc() failed: no memory", __FILE__, __LINE__);

	return NULL;
}

static void dfs_slab_free_pages(dfs_slab_pool_t *pool, dfs_slab_page_t *page, dfs_uint_t pages)
{
	dfs_uint_t        type;
	dfs_slab_page_t  *prev, *join;

	page->slab = pages--;

	if (pages)
	{
		dfs_memzero(&page[1], pages * sizeof(dfs_slab_page_t));
	}

	if (page->next)
	{
		prev = (dfs_slab_page_t *) (page->prev & ~DFS_SLAB_PAGE_MASK);
		prev->next = page->next;
		page->next->prev = page->prev;
	}

	join = page + page->slab;

	if (join < pool->last)
	{
		type = join->prev & DFS_SLAB_PAGE_MASK;

		if (type == DFS_SLAB_PAGE)
		{
			if (join->next != NULL)
			{
				pages += join->slab;
				page->slab += join->slab;

				prev = (dfs_slab_page_t *) (join->prev & ~DFS_SLAB_PAGE_MASK);
				prev->next = join->next;
				join->next->prev = join->prev;

				join->slab = DFS_SLAB_PAGE_FREE;
				join->next = NULL;
				join->prev = DFS_SLAB_PAGE;
			}
		}
	}

	if (page > pool->pages)
	{
		join = page - 1;
		type = join->prev & DFS_SLAB_PAGE_MASK;

		if (type == DFS_SLAB_PAGE)
		{
			if (join->slab == DFS_SLAB_PAGE_FREE)
			{
				join = (dfs_slab_page_t *) (join->prev & ~DFS_SLAB_PAGE_MASK);
			}

			if (join->next != NULL)
			{
				pages += join->slab;
				join->slab += page->slab;

				prev = (dfs_slab_page_t *) (join->prev & ~DFS_SLAB_PAGE_MASK);
				prev->next = join->next;
				join->next->prev = join->prev;

				page->slab = DFS_SLAB_PAGE_FREE;
				page->next = NULL;
				page->prev = DFS_SLAB_PAGE;

				page = join;
			}
		}
	}

	if (pages)
	{
		page[pages].prev = (uintptr_t) page;
	}

	page->prev = (uintptr_t) &pool->free;
	page->next = pool->free.next;

	page->next->prev = (uintptr_t) page;

	pool->free.next = page;
}

