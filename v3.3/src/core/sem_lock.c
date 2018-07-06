#include <errno.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include "sem_lock.h"
#include "pub_log.h"
#include "pub_mem.h"

SW_PROTECTED sw_sem_lock_t*	g_sem_lock = NULL;

sw_int32_t sem_loc_lock_creat(sw_sem_lock_t* sem_lock, sw_syscfg_t *syscfg)
{
	sw_int32_t	sem_id = -1;

	pub_log_debug("%s, %d, semsize[%d] sem_addr[%p]",__FILE__,__LINE__,syscfg->semsize, sem_lock);
	if (syscfg->semsize <= 0 || sem_lock == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_id = pub_sem_create(IPC_PRIVATE, syscfg->semsize, SEM_VALUE);
	if (sem_id < 0)
	{
		pub_log_error("%s, %d, pub_sem_create semsize[%d]  error[%s]."
				, __FILE__, __LINE__, syscfg->semsize, strerror(errno));
		return SW_ERROR;
	}
	
	pub_mem_memzero(sem_lock, sizeof(sw_sem_lock_t));
	sem_lock->lock_id = 1;
	sem_lock->lock_cnt = 1;
	sem_lock->sem_id = sem_id;
	sem_lock->sem_num = syscfg->semsize;
	
	return sem_id;
}

sw_int32_t sem_lock_creat(sw_sem_lock_t* sem_lock, sw_syscfg_t *syscfg)
{
	sw_int_t	semid = SW_ERROR;

	semid = sem_loc_lock_creat(sem_lock, syscfg);
	if (semid == SW_ERROR)
	{
		pub_log_error("%s, %d, sem_loc_lock_creat error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	g_sem_lock = sem_lock;

	return semid;
}

sw_int32_t sem_loc_lock_link(sw_sem_lock_t* sem_lock)
{
	sw_int32_t	result = SW_ERROR;
	
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, Param error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = pub_sem_check(sem_lock->sem_id);
	if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, pub_sem_check error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

sw_int32_t sem_lock_link()
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (sem_loc_lock_link(g_sem_lock));
}

sw_int32_t sem_lock_get_size()
{
	return sizeof(sw_sem_lock_t);
}

sw_int32_t sem_lock_set_addr(sw_char_t *addr)
{
	if (addr == NULL)
	{
		pub_log_error("%s, %d, Param error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	g_sem_lock = (sw_sem_lock_t*)addr;

	return SW_OK;
}

sw_int32_t sem_loc_new_lock_id(sw_sem_lock_t* sem_lock)
{
	sw_int32_t	lock_id = -1;
	
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*lock*/
	sem_loc_mutex_lock(sem_lock, 1);
	
	if (sem_lock->lock_cnt < sem_lock->sem_num)
	{
		sem_lock->lock_cnt += 1;
		lock_id = sem_lock->lock_cnt;
	}

	sem_loc_mutex_unlock(sem_lock, 1);

	if (lock_id == -1)
	{
		pub_log_error("%s, %d, no more lock_id.", __FILE__, __LINE__);
	}
	
	return lock_id;
}

sw_int32_t sem_new_lock_id()
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_new_lock_id(g_sem_lock));
}

sw_int32_t sem_loc_lock_remove(sw_sem_lock_t* sem_lock)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return pub_sem_rm(sem_lock->sem_id);
}

sw_int32_t sem_lock_remove()
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_lock_remove(g_sem_lock));
}

sw_int32_t sem_loc_mutex_lock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (lock_id < 0 || lock_id >= sem_lock->sem_num)
	{
		pub_log_error("%s, %d, Param error, index[%d] sem_num[%d]"
			, __FILE__, __LINE__, lock_id, sem_lock->sem_num);
		return SW_ERROR;
	}
	
	return pub_sem_lock(sem_lock->sem_id, lock_id, -SEM_VALUE);
}

sw_int32_t sem_mutex_lock(sw_int32_t lock_id)
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_mutex_lock(g_sem_lock, lock_id));
}

sw_int32_t sem_loc_mutex_unlock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (lock_id < 0 || lock_id >= sem_lock->sem_num)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return pub_sem_unlock(sem_lock->sem_id, lock_id, SEM_VALUE);
}

sw_int32_t sem_mutex_unlock(sw_int32_t lock_id)
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_mutex_unlock(g_sem_lock, lock_id));
}

sw_int32_t sem_loc_read_lock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lock_id < 0 || lock_id >= sem_lock->sem_num)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return pub_sem_lock(sem_lock->sem_id, lock_id, -1);
}

sw_int32_t sem_read_lock(sw_int32_t lock_id)
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_read_lock(g_sem_lock, lock_id));
}

sw_int32_t sem_loc_read_unlock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lock_id < 0 || lock_id >= sem_lock->sem_num)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return pub_sem_unlock(sem_lock->sem_id, lock_id, 1);
}

sw_int32_t sem_read_unlock(sw_int32_t lock_id)
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_read_unlock(g_sem_lock, lock_id));
}

sw_int32_t sem_loc_write_lock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lock_id < 0 || lock_id >= sem_lock->sem_num)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return pub_sem_lock(sem_lock->sem_id, lock_id, -SEM_VALUE);
}

sw_int32_t sem_write_lock(sw_int32_t lock_id)
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_write_lock(g_sem_lock, lock_id));
}

sw_int32_t sem_loc_write_unlock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id)
{
	if (sem_lock == NULL)
	{
		pub_log_error("%s, %d, sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lock_id < 0 || lock_id >= sem_lock->sem_num)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return pub_sem_unlock(sem_lock->sem_id, lock_id, SEM_VALUE);
}

sw_int32_t sem_write_unlock(sw_int32_t lock_id)
{
	if (g_sem_lock == NULL)
	{
		pub_log_error("%s, %d, g_sem_lock == NULL.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	return (sem_loc_write_unlock(g_sem_lock, lock_id));
}


