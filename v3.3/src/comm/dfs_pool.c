#include "dfs_spool.h"
#include "dfs_pool.h"
#include "pub_mem.h"
#include "pub_log.h"

dfs_pool_t *dfs_pool_create(size_t size, int type)
{
	dfs_pool_t	*p;

	p = pub_mem_alloc(sizeof(dfs_pool_t));
	if (p == NULL)
	{
		pub_log_error("[%s][%d] Alloc error!", __FILE__, __LINE__);
		return NULL;
	}
	if (type == DFS_POOL_TYPE_SLAB)
	{
		dfs_slab_pool_t	*sp = NULL;
		sp = dfs_spool_create(size);
		if (sp == NULL)
		{
			pub_log_error("[%s][%d] Create spool error!", __FILE__, __LINE__);
			return NULL;
		}
		p->dfs_pool_create = (dfs_pool_create_pt)dfs_spool_create;
		p->dfs_pool_destroy = dfs_spool_destroy;
		p->dfs_pool_reset = NULL;
		p->dfs_palloc = dfs_spool_alloc;
		p->dfs_pnalloc = dfs_spool_alloc;
		p->dfs_pcalloc = dfs_spool_calloc;
		p->dfs_pfree = dfs_spool_free;
		p->pool = sp;
	}
	
	return p;
}

void dfs_pool_destroy(dfs_pool_t *pool)
{
	if (pool == NULL)
	{
		return ;
	}
	pool->dfs_pool_destroy(pool->pool);
	dfs_free(pool);
}


void dfs_pool_reset(dfs_pool_t *pool)
{
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] pool is null!", __FILE__, __LINE__);
		return ;
	}
	pool->dfs_pool_reset(pool->pool);
}


void *dfs_palloc(dfs_pool_t *pool, size_t size)
{
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] pool is null!", __FILE__, __LINE__);
		return NULL;
	}

	return pool->dfs_palloc(pool->pool, size);
}


void *dfs_pnalloc(dfs_pool_t *pool, size_t size)
{
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] pool is null!", __FILE__, __LINE__);
		return NULL;
	}
	return pool->dfs_pnalloc(pool->pool, size);
}


dfs_int_t dfs_pfree(dfs_pool_t *pool, void *p)
{
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] pool is null!", __FILE__, __LINE__);
		return -1;
	}
	
	return pool->dfs_pfree(pool->pool, p);
}


void *dfs_pcalloc(dfs_pool_t *pool, size_t size)
{
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] pool is null!", __FILE__, __LINE__);
		return NULL;
	}
	
	return pool->dfs_pcalloc(pool->pool, size);
}

