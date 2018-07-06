#ifndef __DFS_SEM_H__
#define __DFS_SEM_H__

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int dfs_sem_create(int key, int size, int value);
int dfs_sem_open(int key, int size);
int dfs_sem_rm(int semid);
int dfs_sem_unlock(int semid, int num, int value) ;
int dfs_sem_lock(int semid, int num, int value);
int dfs_sem_check(int semid);

typedef struct
{
	int	semid;
	int	semnum;
	int	lockcnt;
	int	lockid;
} dfs_lock_t;

#define SEM_VALUE (500)

int dfs_lock_init(char *addr, int semsize);
int dfs_lock_link();
int dfs_lock_getsize();
int dfs_lock_setaddr(char *addr);
int dfs_lock_newid();
int dfs_lock_remove();
int dfs_lock_mutex_lock(int lockid);
int dfs_lock_mutex_unlock(int lockid);
int dfs_lock_rlock(int lockid);
int dfs_lock_runlock(int lockid);
int dfs_lock_wlock(int lockid);
int dfs_lock_wunlock(int lockid);

#endif