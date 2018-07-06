#include "dfs_shmem.h"
#include "pub_log.h"

dfs_int_t dfs_shm_alloc(dfs_shm_t *shm)
{
	int	id = 0;

	id = shmget(IPC_PRIVATE, shm->size, (SHM_R | SHM_W | IPC_CREAT));
	if (id == -1)
	{
		pub_log_error("[%s][%d] shmget(%u) failed!", __FILE__, __LINE__, shm->size);
		return DFS_ERROR;
	}
	pub_log_info("[%s][%d] shmget id:[%d]", __FILE__, __LINE__, id);

	shm->addr = shmat(id, NULL, 0);
	if (shm->addr == (void *) -1)
	{
		pub_log_error("[%s][%d] shmat() failed, errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
	}

	if (shmctl(id, IPC_RMID, NULL) == -1)
	{
		pub_log_error("[%s][%d] shmctl(IPC_RMID) failed, errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
	}

	return (shm->addr == (void *) -1) ? DFS_ERROR : DFS_OK;
}


void dfs_shm_free(dfs_shm_t *shm)
{
	if (shmdt(shm->addr) == -1)
	{
		pub_log_error("[%s][%d] shmdt(%p) failed", __FILE__, __LINE__, shm->addr);
	}
}

