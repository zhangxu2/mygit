#include "dfs_slab.h"
#include "dfs_shmem.h"
#include "dfs_shmtx.h"
#include "pub_log.h"

#define MAX_ALLOC_SIZE	1024*8

dfs_slab_pool_t *dfs_spool_create(size_t size)
{
	int	ret = 0;
	static int	spcnt = 0;
	char	filename[128];
	dfs_shm_t	shm;
	dfs_slab_pool_t	*sp;

	memset(&shm, 0x0, sizeof(shm));
	shm.size = size > MAX_ALLOC_SIZE ? size : MAX_ALLOC_SIZE;
	ret = dfs_shm_alloc(&shm);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Alloc shm error!", __FILE__, __LINE__);
		return NULL;
	}
	pub_log_info("[%s][%d] Alloc shm success!", __FILE__, __LINE__);

	memset(shm.addr, 0x0, shm.size);
	sp = (dfs_slab_pool_t *)shm.addr;
	sp->end = shm.addr + shm.size;
	sp->min_shift = 3;
	sp->addr = shm.addr;

	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/tmp/slab.lock_%d_%d", getenv("SWWORK"), getpid(), spcnt);
	pub_log_info("[%s][%d] lockfile=[%s]", __FILE__, __LINE__, filename);
	ret = dfs_shmtx_create(&sp->mutex, NULL);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] shmtx_create error!", __FILE__, __LINE__);
		return NULL;
	}
	dfs_slab_init(sp);
	spcnt++;
	pub_log_info("[%s][%d] Create slab pool success!", __FILE__, __LINE__);

	return sp;
}

void dfs_spool_destroy(void *vp)
{
	dfs_slab_pool_t	*sp = (dfs_slab_pool_t *)vp;
	dfs_shm_t	shm;

	memset(&shm, 0x0, sizeof(shm));
	shm.addr = sp->addr;
	dfs_shm_free(&shm);

	return ;
}

dfs_int_t dfs_spool_free(void *vp, void *p)
{
	dfs_slab_pool_t      *pool = (dfs_slab_pool_t *)vp;

	dfs_slab_free(pool, p);

	return 0;
}

void *dfs_spool_alloc(void *vp, size_t size)
{
	dfs_slab_pool_t	*pool = (dfs_slab_pool_t *)vp;

	return dfs_slab_alloc(pool, size);
}


void *dfs_spool_calloc(void *vp, size_t size)
{
	void	*p = NULL;

	p = dfs_spool_alloc(vp, size);
	if (p)
	{
		memset(p, 0x0, size);
	}

	return p;
}

