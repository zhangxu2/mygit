#include "dfs_sem.h"
#include "pub_log.h"

int dfs_sem_create(int key, int size, int value)
{
	int	i = 0;
	int	semid = 0;
	union semun 
	{
		int	val;
		struct semid_ds	*buf;
		unsigned int	*array;
	} sem;

	memset(&sem, 0x0, sizeof(sem));
	
	semid = semget((key_t)key, 0, 0);
	if (semid < 0)
	{
		if (key == IPC_PRIVATE || errno == ENOENT)
		{
			semid = semget((key_t)key, size, IPC_CREAT | 0666);
			if (semid == -1)
			{
				pub_log_error("[%s][%d] semget error, key=[0x%x] errno=[%d]:[%s]",
					__FILE__, __LINE__, key, errno, strerror(errno));
				return -1;
			}

			sem.val = value;	
			for (i = 0; i < size; i++)
			{
				if (semctl(semid, i, SETVAL, sem) < 0)
				{
					pub_log_error("[%s][%d] semctl [%d][%d] error, errno=[%d]:[%s]",
						__FILE__, __LINE__, i, value, errno, strerror(errno));
					return -1;
				}
			}
			return semid;
		}
		pub_log_error("[%s][%d] semget [0x%x] error, errno=[%d]:[%s]",
			__FILE__, __LINE__, key, errno, strerror(errno));
		return -1;
	}
	
	union
	{
		int	val;		/* Value for SETVAL */
		struct semid_ds	*buf;	/* Buffer for IPC_STAT, IPC_SET */
		unsigned short	*array;	/* Array for GETALL, SETALL */
		struct seminfo	*__buf;	/* Buffer for IPC_INFO (Linux specific) */
	} semopts;

#if 0 
	struct seminfo	info;
	
	memset(&info, 0x0, sizeof(info));
	semopts.__buf = &info;
	if (semctl(semid, 0, IPC_STAT, semopts) < 0)
	{
		pub_log_error("[%s][%d] semctl [%d] error, errno=[%d]:[%s]",
			__FILE__, __LINE__, semid, errno, strerror(errno));
		return -1;
	}
	pub_log_info("[%s][%d] info.semmni=[%d] info.semmns=[%d]",
		__FILE__, __LINE__, info.semmni, info.semmns);
#endif

#if 1 
	struct semid_ds ds; 
        
        semopts.buf = &ds;
        if (semctl(semid, 0, IPC_STAT, semopts) < 0)
        {
                pub_log_error("[%s][%d] semctl [%d] error, errno=[%d]:[%s]",
                        __FILE__, __LINE__, semid, errno, strerror(errno));
                return -1;
        }
#endif
	return semid;
}

int dfs_sem_open(int key, int size)
{
	int	semid = 0;
	
	semid = semget((key_t)key, size, 0);
	if (semid < 0)
	{
		pub_log_error("[%s][%d] semget error, key=[0x%x] errno=[%d]:[%s]",
			__FILE__, __LINE__, key, errno, strerror(errno));
		return -1;
	}

	return semid;
}

int dfs_sem_lock(int semid, int num, int value) 
{
	struct sembuf	sem;

	if (semid < 0)
	{
		pub_log_error("[%s][%d] semlock param error!", __FILE__, __LINE__);
		return -1;
	}
	sem.sem_num = num;
	sem.sem_op = value;
	sem.sem_flg = SEM_UNDO;

	return semop(semid, &sem, 1);
}

int dfs_sem_unlock(int semid, int num, int value) 
{
	struct sembuf	sem;

	if (semid <0 )
	{
		pub_log_error("[%s][%d] semunlock param error!", __FILE__, __LINE__);
		return -1;
	}
	sem.sem_num = num;
	sem.sem_op = value;
	sem.sem_flg = SEM_UNDO;

	return semop(semid, &sem, 1);
}

int dfs_sem_rm(int semid)
{
	return semctl(semid, 0, IPC_RMID, 0);
}

int dfs_sem_check(int semid)
{
	int	ret = -1;
	union
	{
		int	val;		/* Value for SETVAL */
		struct semid_ds	*buf;	/* Buffer for IPC_STAT, IPC_SET */
		unsigned short	*array;	/* Array for GETALL, SETALL */
		struct seminfo	*__buf;	/* Buffer for IPC_INFO (Linux specific) */
	} semopts;

	struct semid_ds	ds;

	semopts.buf = &ds;
	ret = semctl(semid, 0, IPC_STAT, semopts);
	if (ret < 0)
	{
		return -1;		
	}

	return 0;
}

static dfs_lock_t	*g_lock = NULL;

int dfs_lock_init(char *addr, int semsize)
{
	int	semid = 0;
	dfs_lock_t	*lock = NULL;
	
	if (addr == NULL || semsize <= 0)
	{
		pub_log_error("[%s][%d] Param error![%d]", __FILE__, __LINE__, semsize);
		return SW_ERROR;
	}
	
	semid = dfs_sem_create(IPC_PRIVATE, semsize, SEM_VALUE);
	if (semid < 0)
	{
		pub_log_error("[%s][%d] Create sem error, semsize=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, semsize, errno, strerror(errno));
		return SW_ERROR;
	}

	lock = (dfs_lock_t *)addr;
	memset(lock, 0x0, sizeof(dfs_lock_t));
	lock->lockid = 1;
	lock->lockcnt = 1;
	lock->semid = semid;
	lock->semnum = semsize;
	
	g_lock = lock;
	
	return semid;
}

int dfs_lock_link()
{
	int	ret = 0;
	
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ret = dfs_sem_check(g_lock->semid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] sem check error, errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	
	return SW_OK;
}

int dfs_lock_getsize()
{
	return sizeof(dfs_lock_t);
}

int dfs_lock_setaddr(char *addr)
{
	if (addr == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	g_lock = (dfs_lock_t *)addr;
	
	return SW_OK;
}

int dfs_lock_newid()
{
	int	lockid = -1;
	
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	dfs_lock_mutex_lock(1);
	if (g_lock->lockcnt < g_lock->semnum)
	{
		g_lock->lockcnt++;
		lockid = g_lock->lockcnt;
	}
	dfs_lock_mutex_unlock(1);
	
	if (lockid == -1)
	{
		pub_log_error("[%s][%d] No more lockid!", __FILE__, __LINE__);
	}
	
	return lockid;
}

int dfs_lock_remove()
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return dfs_sem_rm(g_lock->semid);
}

int dfs_lock_mutex_lock(int lockid)
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lockid < 0 || lockid >= g_lock->semnum)
	{
		pub_log_error("[%s][%d] Param error, lockid:[%d] semnum=[%d]",
			__FILE__, __LINE__, lockid, g_lock->semnum);
		return SW_ERROR;
	}
	
	return dfs_sem_lock(g_lock->semid, lockid, -SEM_VALUE);
}

int dfs_lock_mutex_unlock(int lockid)
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (lockid < 0 || lockid >= g_lock->semnum)
	{
		pub_log_error("[%s][%d] Param error, lockid:[%d] semnum:[%d]",
			__FILE__, __LINE__, lockid, g_lock->semnum);
		return SW_ERROR;
	}
	
	return dfs_sem_unlock(g_lock->semid, lockid, SEM_VALUE);
}

int dfs_lock_rlock(int lockid)
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (lockid < 0 || lockid >= g_lock->semnum)
	{
		pub_log_error("[%s][%d] Param error, lockid:[%d] semnum:[%d]",
			__FILE__, __LINE__, lockid, g_lock->semnum);
		return SW_ERROR;
	}
	
	return dfs_sem_lock(g_lock->semid, lockid, -1);
}

int dfs_lock_runlock(int lockid)
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (lockid < 0 || lockid >= g_lock->semnum)
	{
		pub_log_error("[%s][%d] Param error, lockid:[%d] semnum:[%d]",
			__FILE__, __LINE__, lockid, g_lock->semnum);
		return SW_ERROR;
	}
	
	return dfs_sem_unlock(g_lock->semid, lockid, 1);
}

int dfs_lock_wlock(int lockid)
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lockid < 0 || lockid >= g_lock->semnum)
	{
		pub_log_error("[%s][%d] Param error, lockid:[%d] semnum:[%d]",
			__FILE__, __LINE__, lockid, g_lock->semnum);
		return SW_ERROR;
	}
	
	return dfs_sem_lock(g_lock->semid, lockid, -SEM_VALUE);
}

int dfs_lock_wunlock(int lockid)
{
	if (g_lock == NULL)
	{
		pub_log_error("[%s][%d] g_lock is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lockid < 0 || lockid >= g_lock->semnum)
	{
		pub_log_error("[%s][%d] Param error, lockid:[%d] semnum:[%d]",
			__FILE__, __LINE__, lockid, g_lock->semnum);
		return SW_ERROR;
	}
	
	return dfs_sem_unlock(g_lock->semid, lockid, SEM_VALUE);
}

