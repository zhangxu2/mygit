#ifndef __DFS_SPOOL_H__
#define __DFS_SPOOL_H__

#include "dfs_slab.h"

dfs_slab_pool_t *dfs_spool_create(size_t size);
void dfs_spool_destroy(void *vp);
dfs_int_t dfs_spool_free(void *vp, void *p);
void *dfs_spool_alloc(void *vp, size_t size);
void *dfs_spool_calloc(void *vp, size_t size);

#endif