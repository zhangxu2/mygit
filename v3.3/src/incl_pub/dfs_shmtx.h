#ifndef __DFS_SHMTX_H__
#define __DFS_SHMTX_H__

#include "dfs.h"

typedef struct
{
	int	lockid;
	dfs_fd_t       fd;
	u_char        *name;
	dfs_uint_t     spin;
} dfs_shmtx_t;


dfs_int_t dfs_shmtx_create(dfs_shmtx_t *mtx,u_char *name);
void dfs_shmtx_destory(dfs_shmtx_t *mtx);
dfs_uint_t dfs_shmtx_trylock(dfs_shmtx_t *mtx);
int dfs_shmtx_lock(dfs_shmtx_t *mtx);
void dfs_shmtx_unlock(dfs_shmtx_t *mtx);
dfs_uint_t dfs_shmtx_force_unlock(dfs_shmtx_t *mtx, dfs_pid_t pid);
dfs_err_t dfs_trylock_fd(dfs_fd_t fd);
dfs_err_t dfs_lock_fd(dfs_fd_t fd);
dfs_err_t dfs_unlock_fd(dfs_fd_t fd);

#endif