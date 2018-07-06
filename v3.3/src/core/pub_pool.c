#include "pub_pool.h"


static void * pub_pool_palloc_block(sw_pool_t *pool, size_t size);
static void * pub_pool_palloc_large(sw_pool_t *pool, size_t size);

sw_pool_t * pub_pool_create(size_t size)
{
	sw_pool_t  *p;

	p = pub_mem_memalign(SW_POOL_ALIGNMENT, size);
	if (p == NULL) 
	{
		return NULL;
	}
	
	p->d.last = (u_char *) p + sizeof(sw_pool_t);
	p->d.end = (u_char *) p + size;
	p->d.next = NULL;
	p->d.failed = 0;
	
	size = size - sizeof(sw_pool_t);
	p->max = (size < SW_MAX_ALLOC_FROM_POOL) ? size : SW_MAX_ALLOC_FROM_POOL;
	p->current = p;
	p->large = NULL;
	p->cleanup = NULL;
	
	return p;
}

void pub_pool_destroy(sw_pool_t *pool)
{
	sw_pool_t          *p, *n;
	sw_pool_large_t    *l;
	sw_pool_cleanup_t  *c;
	
	for (c = pool->cleanup; c; c = c->next) 
	{
		if (c->handler) 
		{
			c->handler(c->data);
		}
	}
	
	for (l = pool->large; l; l = l->next) 
	{
		if (l->alloc) 
		{
			pub_mem_free(l->alloc);
		}
	}

#if(SW_DEBUG)
	/*
	* we could allocate the pool->log from this pool
	* so we can not use this log while the free()ing the pool
	*/
	
	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) 
	{
		if (n == NULL) 
		{
			break;
		}
	}

#endif
	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next)
	{
		pub_mem_free(p);
		
		if (n == NULL) 
		{
		    break;
		}
	}
}

void pub_pool_reset(sw_pool_t *pool)
{
	sw_pool_t        *p;
	sw_pool_large_t  *l;
	if (pool->large)
	{
		for (l = pool->large; l; l = l->next) 
		{
			if (l->alloc) 
			{
				pub_mem_free(l->alloc);
			}
		}
	}
	pool->large = NULL;

	for (p = pool; p; p = p->d.next) 
	{
		p->d.last = (u_char *) p + sizeof(sw_pool_t);
	}
}
	
void * pub_pool_palloc(sw_pool_t *pool, size_t size)
{
	sw_uchar_t      *m;
	sw_pool_t  *p;
	
	if (size <= pool->max) 
	{
		p = pool->current;	
		do 
		{
			m = pub_mem_align_ptr(p->d.last, SW_ALIGNMENT);
			
			if ((size_t) ((sw_uchar_t*)p->d.end - (sw_uchar_t*)m) >= size) 
			{
				p->d.last = m + size;
				return m;
			}
			p = p->d.next;
		} while (p);

		return pub_pool_palloc_block(pool, size);
	}
	
	return pub_pool_palloc_large(pool, size);
}

void *pub_pool_pnalloc(sw_pool_t *pool, size_t size)
{
	u_char      *m;
	sw_pool_t  *p;
	
	if (size <= pool->max) 
	{
		p = pool->current;
		do 
		{
			m = p->d.last;
			if ((size_t) ((sw_uchar_t*)p->d.end - (sw_uchar_t*)m) >= size) 
			{
				p->d.last = m + size;
				return m;
			}
			p = p->d.next;
		} while (p);
	
		return pub_pool_palloc_block(pool, size);
	}
	
	return pub_pool_palloc_large(pool, size);
}
	
static void *pub_pool_palloc_block(sw_pool_t *pool, size_t size)
{
	u_char		*m;
	size_t		psize;
	sw_pool_t	*p, *new, *current;
	
	psize = (size_t) ((sw_uchar_t*)pool->d.end - (sw_uchar_t*)pool);
	
	m = pub_mem_memalign(SW_POOL_ALIGNMENT, psize);
	if (m == NULL) 
	{
		return NULL;
	}
	
	new = (sw_pool_t *) m;
	
	new->d.end = m + psize;
	new->d.next = NULL;
	new->d.failed = 0;
	
	m += sizeof(sw_pool_data_t);
	m = pub_mem_align_ptr(m,SW_ALIGNMENT);
	new->d.last = m + size;
	
	current = pool->current;
	
	for (p = current; p->d.next; p = p->d.next) 
	{
		if (p->d.failed++ > 4) 
		{
			current = p->d.next;
		}
	}
	
	p->d.next = new;
	
	pool->current = current ? current : new;
	
	return m;
}


static void *pub_pool_palloc_large(sw_pool_t *pool, size_t size)
{
	void              *p;
	sw_uint_t         n;
	sw_pool_large_t  *large;
	/*create large data mem*/
	p = pub_mem_alloc(size);
	if (p == NULL) 
	{
		return NULL;
	}
	
	n = 0;
	for (large = pool->large; large; large = large->next) 
	{
		if (large->alloc == NULL) 
		{
			large->alloc = p;
			return p;
		}
		if (n++ > 3) 
		{
			break;
		}
	}
	/**create large header**/
	large = pub_pool_palloc(pool, sizeof(sw_pool_large_t));
	if (large == NULL) 
	{
		pub_mem_free(p);
		return NULL;
	}
	
	large->alloc = p;
	large->next = pool->large;
	pool->large = large;
	
	return p;
}

void *pub_pool_pmemalign(sw_pool_t *pool, size_t size, size_t alignment)
{
	void              *p;
	sw_pool_large_t  *large;
	
	p = pub_mem_memalign(alignment, size);
	if (p == NULL) 
	{
		return NULL;
	}
	
	large = pub_pool_palloc(pool, sizeof(sw_pool_large_t));
	if (large == NULL) 
	{
		pub_mem_free(p);
		return NULL;
	}
	
	large->alloc = p;
	large->next = pool->large;
	pool->large = large;
	
	return p;
}
	
	
sw_int_t pub_pool_pfree(sw_pool_t *pool, void *p)
{
	sw_pool_large_t  *l;
	
	for (l = pool->large; l; l = l->next) 
	{
		if (p == l->alloc) 
		{
			pub_mem_free(l->alloc);
			l->alloc = NULL;
			return SW_OK;
		}
	}
	
	return SW_DECLINED;
}


void *pub_pool_pcalloc(sw_pool_t *pool, size_t size)
{
	void *p;
	
	p = pub_pool_palloc(pool, size);
	if (p) 
	{
		pub_mem_memzero(p,size);
	}
	
	return p;
}


sw_pool_cleanup_t *
pub_pool_cleanup_add(sw_pool_t *p, size_t size)
{
	sw_pool_cleanup_t  *c;
	
	c = pub_pool_palloc(p, sizeof(sw_pool_cleanup_t));
	if (c == NULL) 
	{
		return NULL;
	}
	
	if (size) 
	{
		c->data = pub_pool_palloc(p, size);
		if (c->data == NULL) 
		{
			/*c alloc at pool don't need free*/
			return NULL;
		}	
	} 
	else 
	{
		c->data = NULL;
	}
	
	c->handler = NULL;
	c->next = p->cleanup;
	
	p->cleanup = c;
	return c;
}


void pub_pool_run_cleanup_file(sw_pool_t *p, sw_fd_t fd)
{
	sw_pool_cleanup_t       *c;
	sw_pool_cleanup_file_t  *cf;
	
	for (c = p->cleanup; c; c = c->next) 
	{
		if (c->handler == pub_pool_cleanup_file) 
		{
			cf = c->data;
			if (cf->fd == fd) 
			{
				c->handler(cf);
				c->handler = NULL;
				return;
			}
		}
	}
}


void pub_pool_cleanup_file(void *data)
{
	sw_pool_cleanup_file_t  *c = data;
	
	if (pub_file_close(c->fd) == SW_FILE_ERROR) 
	{
	
	}
}


void pub_pool_delete_file(void *data)
{
	sw_pool_cleanup_file_t  *c = data;
	sw_err_t  err;
	
	if (unlink(c->name) == SW_FILE_ERROR)
	{
		err = sw_errno;
		if (err != SW_ENOENT) 
		{
		    
		}
	}
	
	if (close(c->fd) == SW_FILE_ERROR)
	{
	}    
}

/*
u_char *
pub_pool_pstrdup(sw_pool_t *pool, sw_str_t *src)
{
	u_char  *dst;
	
	dst = pub_pool_pnalloc(pool, src->len);
	if (dst == NULL)
	{
		return NULL;
	}
	
	pub_mem_memcpy(dst, src->data, src->len);
	
	return dst;
}
*/
