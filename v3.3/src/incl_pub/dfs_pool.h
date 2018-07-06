#ifndef __DFS_POOL_H__
#define __DFS_POOL_H__

#include "dfs.h"

#define DFS_POOL_TYPE_COMM	1
#define DFS_POOL_TYPE_SLAB	2
#define	DFS_MAX_ALLOC_FROM_POOL	4095

typedef void* (*dfs_pool_create_pt)(size_t size);
typedef void (*dfs_pool_destroy_pt)(void *pool);
typedef void (*dfs_pool_reset_pt)(void *pool);
typedef void* (*dfs_palloc_pt)(void *pool, size_t size);
typedef void* (*dfs_pnalloc_pt)(void *pool, size_t size);
typedef void* (*dfs_pcalloc_pt)(void *pool, size_t size);
typedef dfs_int_t (*dfs_pfree_pt)(void *pool, void *p);

typedef struct
{
	void	*pool;
	dfs_pool_create_pt	dfs_pool_create;
	dfs_pool_destroy_pt	dfs_pool_destroy;
	dfs_pool_reset_pt	dfs_pool_reset;
	dfs_palloc_pt		dfs_palloc;
	dfs_pnalloc_pt		dfs_pnalloc;
	dfs_pcalloc_pt		dfs_pcalloc;
	dfs_pfree_pt		dfs_pfree;
} dfs_pool_t;

dfs_pool_t *dfs_pool_create(size_t size, int type);
void dfs_pool_destroy(dfs_pool_t *pool);
void dfs_pool_reset(dfs_pool_t *pool);
void *dfs_palloc(dfs_pool_t *pool, size_t size);
void *dfs_pnalloc(dfs_pool_t *pool, size_t size);
void *dfs_pcalloc(dfs_pool_t *pool, size_t size);
dfs_int_t dfs_pfree(dfs_pool_t *pool, void *p);

#endif

