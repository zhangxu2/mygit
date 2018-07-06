#ifndef __DFS_SLAB_H__
#define __DFS_SLAB_H__

#include "dfs_shmtx.h"

typedef struct dfs_slab_page_s  dfs_slab_page_t;

struct dfs_slab_page_s
{
    uintptr_t         slab;
    dfs_slab_page_t  *next;
    uintptr_t         prev;
};


typedef struct
{
    size_t            min_size;
    size_t            min_shift;

    dfs_slab_page_t  *pages;
    dfs_slab_page_t  *last;
    dfs_slab_page_t   free;

    u_char           *start;
    u_char           *end;

    dfs_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    unsigned          log_nomem:1;

    void             *data;
    void             *addr;
} dfs_slab_pool_t;


void dfs_slab_init(dfs_slab_pool_t *pool);
void *dfs_slab_alloc(dfs_slab_pool_t *pool, size_t size);
void *dfs_slab_alloc_locked(dfs_slab_pool_t *pool, size_t size);
void *dfs_slab_calloc(dfs_slab_pool_t *pool, size_t size);
void *dfs_slab_calloc_locked(dfs_slab_pool_t *pool, size_t size);
void dfs_slab_free(dfs_slab_pool_t *pool, void *p);
void dfs_slab_free_locked(dfs_slab_pool_t *pool, void *p);


#endif