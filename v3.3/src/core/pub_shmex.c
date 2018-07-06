#include "pub_shmex.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pub_log.h"

sw_int_t pub_shmex_alloc(sw_shmex_t *shm)
{
	int	id = 0;

	if (shm->filename[0] == '\0')
	{
		id = shmget(IPC_PRIVATE, shm->size, (SHM_R | SHM_W | IPC_CREAT));
		if (id == -1)
		{
			pub_log_error("[%s][%d] shmget(%u) failed!", __FILE__, __LINE__, shm->size);
			return SW_ERROR;
		}

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

		return (shm->addr == (void *) -1) ? SW_ERROR : SW_OK;
	}

	int	fd = 0;
	char	filename[128];
	struct stat	st;
		
        memset(filename, 0x0, sizeof(filename));
        sprintf(filename, "%s/tmp/.%s.map", getenv("SWWORK"), shm->filename);
	memset(&st, 0x0, sizeof(st));
	if (stat(filename, &st) < 0)
	{
		if (errno == ENOENT)
		{
			fd = open(filename, O_RDWR | O_CREAT, 0777);
			if (fd == -1)
			{
				pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
					__FILE__, __LINE__, filename, errno, strerror(errno));
				return -1;
			}

			if (ftruncate(fd, shm->size) < 0)
			{
				pub_log_error("[%s][%d] truncate [%s] error, size=[%u] fd=[%d] errno=[%d]:[%s]",
					__FILE__, __LINE__, filename, shm->size, fd, errno, strerror(errno));
			}
		}
	}
	else
	{
		fd = open(filename, O_RDWR);
		if (fd == -1)
		{
			pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
				__FILE__, __LINE__, filename, errno, strerror(errno));
			return -1;
		}
		
		if ((size_t)st.st_size < shm->size)
		{
			if (ftruncate(fd, shm->size) < 0)
			{
				pub_log_error("[%s][%d] truncate [%s] error, size=[%u] fd=[%d] errno=[%d]:[%s]",
					__FILE__, __LINE__, filename, shm->size, fd, errno, strerror(errno));
			}
		}
	}

        shm->addr = (u_char *)mmap(NULL, shm->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (shm->addr == MAP_FAILED)
        {
                pub_log_error("[%s][%d] mmap(%s, MAP_SHARED, %uz) failed, errno=[%d]:[%s]",
                        __FILE__, __LINE__, filename, shm->size, errno, strerror(errno));
        }

        if (close(fd) == -1)
        {
                pub_log_error("[%s][%d] Close (%s) failed! errno=[%d]:[%s]",
                        __FILE__, __LINE__, filename, errno, strerror(errno));
        }
	memset(shm->filename, 0x0, sizeof(shm->filename));
	strncpy(shm->filename, filename, sizeof(shm->filename) - 1);

        return (shm->addr == MAP_FAILED) ? -1 : 0;

}

void pub_shmex_free(sw_shmex_t *shm)
{
	if (shm->filename[0] == '\0')
	{
		if (shmdt(shm->addr) == -1)
		{
			pub_log_error("[%s][%d] shmdt(%p) failed, errno=[%d]:[%s]",
				__FILE__, __LINE__, shm->addr, errno, strerror(errno));
		}
	}
	else
	{
		if (munmap((void *)shm->addr, shm->size) == -1)
		{
			pub_log_error("[%s][%d] munmap(%p, %uz) failed! errno=[%d]:[%s]",
				__FILE__, __LINE__, shm->addr, shm->size, errno, strerror(errno));
		}
	}
}

