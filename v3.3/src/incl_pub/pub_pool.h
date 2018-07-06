#ifndef __PUB_POOL_H__
#define __PUB_POOL_H__

#include "pub_type.h"
#include "pub_mem.h"
#include "pub_errno.h"
#include "pub_file.h"

#define SW_MAX_ALLOC_FROM_POOL  (pub_pagesize - 1)

#define SW_DEFAULT_POOL_SIZE    (16 * 1024)

#define SW_POOL_ALIGNMENT       16
#define SW_MIN_POOL_SIZE pub_mem_align((sizeof(sw_pool_t) + 2 * sizeof(sw_pool_large_t)),SW_POOL_ALIGNMENT)


typedef void (*sw_pool_cleanup_pt)(void *data);


typedef struct sw_pool_cleanup_s  sw_pool_cleanup_t;

struct sw_pool_cleanup_s 
{
	sw_pool_cleanup_pt	handler;
	void			*data;
	sw_pool_cleanup_t	*next;
};

typedef struct sw_pool_large_s  sw_pool_large_t;

struct sw_pool_large_s 
{
	sw_pool_large_t		*next;
	void			*alloc;
};
typedef struct sw_pool_s sw_pool_t;

typedef struct{
	u_char		*last;
	u_char		*end;
	sw_pool_t	*next;
	sw_uint_t	failed;
} sw_pool_data_t;

struct sw_pool_s{
	sw_pool_data_t		d;
	size_t			max;
	sw_pool_t		*current;
	sw_pool_large_t		*large;
	sw_pool_cleanup_t	*cleanup;
};


typedef struct 
{
	sw_fd_t		fd;
	sw_char_t	*name;
} sw_pool_cleanup_file_t;

void *pub_mem_alloc(size_t size);
void *pub_mem_calloc(size_t size);

sw_pool_t *pub_pool_create(size_t size);

void pub_pool_destroy(sw_pool_t *pool);
void pub_pool_reset(sw_pool_t *pool);

void *pub_pool_palloc(sw_pool_t *pool, size_t size);
void *pub_pool_pnalloc(sw_pool_t *pool, size_t size);
void *pub_pool_pcalloc(sw_pool_t *pool, size_t size);
void *pub_pool_pmemalign(sw_pool_t *pool, size_t size, size_t alignment);
sw_int_t pub_pool_pfree(sw_pool_t *pool, void *p);

sw_pool_cleanup_t *pub_pool_cleanup_add(sw_pool_t *p, size_t size);
void pub_pool_run_cleanup_file(sw_pool_t *p, sw_fd_t fd);
void pub_pool_cleanup_file(void *data);
void pub_pool_delete_file(void *data);

#endif
