#include "dfs_shmtx.h"
#include "dfs_sem.h"
#include "pub_log.h"

dfs_err_t dfs_trylock_fd(dfs_fd_t fd)
{
	struct flock	fl;

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &fl) == -1)
	{
		return errno;
	}

	return 0;
}


dfs_err_t dfs_lock_fd(dfs_fd_t fd)
{
	struct flock  fl;

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLKW, &fl) == -1)
	{
		return errno;
	}

	return 0;
}

dfs_err_t dfs_unlock_fd(dfs_fd_t fd)
{
	struct flock  fl;

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &fl) == -1)
	{
		return  errno;
	}

	return 0;
}

dfs_int_t dfs_shmtx_create(dfs_shmtx_t *mtx, u_char *name)
{
	if (name == NULL)
	{
		mtx->lockid = dfs_lock_newid();
		if (mtx->lockid <= 0)
		{
			pub_log_error("[%s][%d] Get new lock error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	
		return SW_OK;
	}

	if (mtx->name)
	{
		if (dfs_strcmp(name, mtx->name) == 0)
		{
			mtx->name = name;
			return SW_OK;
		}

		dfs_shmtx_destory(mtx);
	}

	mtx->fd = open((const char *)name, O_RDWR | O_CREAT, 0644);
	if (mtx->fd < 0)
	{
		pub_log_error("[%s][%d] open [%s] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, name, errno, strerror(errno));
		return SW_ERROR;
	}

	if (unlink((const char *)name) < 0)
	{
		pub_log_error("[%s][%d] unlink [%s] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, name, errno, strerror(errno));
	}

	mtx->name = name;

	return SW_OK;
}


void dfs_shmtx_destory(dfs_shmtx_t *mtx)
{
	if (mtx->lockid > 0)
	{
		return ;
	}

	if (close(mtx->fd) < 0)
	{
		pub_log_error("[%s][%d] close file [%s] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, mtx->name, errno, strerror(errno));
	}
}

dfs_uint_t dfs_shmtx_trylock(dfs_shmtx_t *mtx)
{
	dfs_err_t	err;

	if (mtx->lockid > 0)
	{
		return 0;
	}
	err = dfs_trylock_fd(mtx->fd);
	if (err == 0)
	{
		return 1;
	}

	if (err == EAGAIN)
	{
		return 0;
	}

	pub_log_error("[%s][%d] trylock %s failed", __FILE__, __LINE__, mtx->name);

	return 0;
}


int dfs_shmtx_lock(dfs_shmtx_t *mtx)
{
	dfs_err_t	err;
	
	if (mtx->lockid > 0)
	{
		return dfs_lock_mutex_lock(mtx->lockid);
	}
	
	err = dfs_lock_fd(mtx->fd);
	if (err == 0)
	{
		return 0;
	}

	pub_log_error("[%s][%d] shmtx lock %s failed! errno=[%d]:[%s]",
		__FILE__, __LINE__, mtx->name, err, strerror(err));
	return -1;
}


void dfs_shmtx_unlock(dfs_shmtx_t *mtx)
{
	dfs_err_t	err;
	
	if (mtx->lockid > 0)
	{
		dfs_lock_mutex_unlock(mtx->lockid);
		return ;
	}
	
	err = dfs_unlock_fd(mtx->fd);
	if (err == 0)
	{
		return;
	}
	pub_log_error("[%s][%d] shmtx unlock %s failed! errno=[%d]:[%s]",
		__FILE__, __LINE__, mtx->name, err, strerror(err));
}


dfs_uint_t dfs_shmtx_force_unlock(dfs_shmtx_t *mtx, dfs_pid_t pid)
{
	return 0;
}

