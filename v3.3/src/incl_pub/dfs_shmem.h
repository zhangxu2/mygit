#ifndef __DFS_SHMEM_H__
#define __DFS_SHMEM_H__

#include "dfs.h"

typedef struct
{
	size_t      len;
	u_char     *data;
} dfs_str_t;

typedef struct
{
	u_char      *addr;
	size_t       size;
	dfs_str_t    name;
	dfs_uint_t   exists;   
} dfs_shm_t;


dfs_int_t dfs_shm_alloc(dfs_shm_t *shm);
void dfs_shm_free(dfs_shm_t *shm);


#endif /* __DFS_SHMEM_H__ */