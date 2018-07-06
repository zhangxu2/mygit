#ifndef __SLAB_H__
#define __SLAB_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>

typedef struct _slab_page_t
{
    uintptr_t slab;
    struct _slab_page_t  *next;
    uintptr_t prev;
}slab_page_t;


typedef struct
{
    size_t min_size;
    size_t min_shift;

    slab_page_t  *pages;
    slab_page_t   free;

    u_char *start;
    u_char *end;

    void *data;
    void *addr;
} slab_pool_t;

typedef struct _slab_node_t
{
    slab_pool_t *sp;
    struct _slab_node_t *next;
}slab_node_t;

typedef struct
{
    int count;
    size_t inc_size;
    slab_node_t *node;
}slab_link_t;

void slab_init(slab_pool_t *pool);
void *slab_alloc(slab_pool_t *pool, size_t size);
void slab_free(slab_pool_t *pool, void *p);

int slab_link_init(slab_link_t *spl, size_t size);
int slab_link_destory(slab_link_t *spl);
void *slab_link_alloc(slab_link_t *spl, size_t size);
int slab_link_free(slab_link_t *spl, void *p);

#endif /*__SLAB_H__*/
