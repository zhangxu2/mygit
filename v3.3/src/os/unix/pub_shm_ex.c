#include "pub_shm_ex.h"

sw_int_t pub_shm_alloc(sw_shm_t *shm)
{
	int	id = 0;

	id = shmget(IPC_PRIVATE, shm->size, (SHM_R|SHM_W|IPC_CREAT));
	if (id == -1)
	{
		pub_log_error("[%s][%d] shmget error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, shm->size, errno, strerror(errno));
		return SW_ERROR;
	}
	shm->id = id;
	pub_log_info("[%s][%d] shmget success! id=[%d] size=[%d]",
		__FILE__, __LINE__, id, shm->size);

	shm->addr = shmat(id, NULL, 0);
	if (shm->addr == (void *) -1)
	{
		pub_log_error("[%s][%d] shmat error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
	}
	
	return (shm->addr == (void *) -1) ? SW_ERROR : SW_OK;
}

void pub_shm_free(sw_shm_t *shm)
{
	if (shmctl(shm->id, IPC_RMID, NULL) == -1)
	{
		pub_log_error("[%s][%d] shmctl error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return;
	}
	
	if (shmdt(shm->addr) == -1)
	{
		pub_log_error("[%s][%d] shmdt error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
	}
	
	return;
}
