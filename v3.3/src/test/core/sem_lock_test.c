/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-03
 *** module  : unit test
 *** name    : sem_lock_test.c
 *** function: sem_mutex_lock.c unit test
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "sem_lock.h"
#include "pub_type.h"
#include "pub_log.h"
#include "pub_mem.h"
#include "pub_shm.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_int32_t	index = 0;
	sw_int32_t	tmp = 0;
	sw_char_t	lock_addr[256];
	sw_int32_t	i = 0;
	sw_syscfg_t	syscfg;
	sw_sem_lock_t	*sem_lock = NULL;
	key_t		key = 0x33;
	sw_int32_t	shmid = -1;
	sw_char_t	*addr = NULL;

	shmid = pub_shm_create(key, sizeof(sw_sem_lock_t));
	if (shmid == SW_ERROR)
	{
		pub_log_error("%s, %d, pub_shm_create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, shmid[%d]",__FILE__,__LINE__,shmid);
	addr = pub_shm_at(shmid);
	if ((void*)-1 == addr)
	{
		pub_log_error("%s, %d, pub_shm_at error, shmid[%d].",__FILE__,__LINE__,shmid);
		return SW_ERROR;		
	}
	
	sem_lock = (sw_sem_lock_t*)addr;
	pub_log_info("%s, %d, shmid[%d] addr[%p] sem_id[%d]"
			, __FILE__, __LINE__, shmid, addr, sem_lock->sem_id);

	sem_lock_set_addr(sem_lock);
	
	if (argc < 2)
	{
		printf("Use: lock_test index\n");
		return -1;
	}

	index = atoi(argv[1]);

	pub_mem_memzero(&syscfg, sizeof(sw_syscfg_t));
	syscfg.semsize = 128;

	pub_log_info("%s, %d, before sem_loc_lock_creat", __FILE__, __LINE__);

	result = sem_lock_link();
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, sem_lock_link error[%d][%s].", __FILE__, __LINE__,errno, strerror(errno));

		/*create lock*/
		result = sem_lock_creat(sem_lock, &syscfg);
		if (result == -1)
		{
			pub_log_error("%s, %d, sem_lock_creat error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("%s, %d, create semid[%d]",__FILE__,__LINE__,sem_lock->sem_id);
	}
	
	pub_log_info("%s, %d, after sem_loc_lock_creat", __FILE__, __LINE__);

	result = sem_new_lock_id();
	if (result == -1)
	{
		pub_log_info("%s, %d, 1 sem_new_lock_id error.", __FILE__, __LINE__);
	}
	
	pub_log_info("%s, %d, before sem_new_lock_id .", __FILE__, __LINE__);
	result = sem_new_lock_id();
	pub_log_info("%s, %d, after sem_new_lock_id .", __FILE__, __LINE__);
	if (result == -1)
	{
		pub_log_info("%s, %d, 2 sem_new_lock_id error.", __FILE__, __LINE__);

	}

	/*lock
	result = sem_mutex_lock(index);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, sem_mutex_lock error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	printf("sem_mutex_lock(%d)\n", index);

	scanf("%d", &tmp);*/
	
	/*unkock
	result = sem_mutex_unlock(index);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, sem_mutex_unlock error.", __FILE__, __LINE__);
		return SW_ERROR;
	}*/

	pub_log_info("%s, %d, before readlock", __FILE__, __LINE__);

	/*read lock
	for (i = 0; i < 500; i++)
	{
		sem_read_lock(index);
		printf("%d read lock %d\n", i, index);
	}

	pub_log_info("%s, %d, after readlock", __FILE__, __LINE__);
	for (i = 0; i < 250; i++)
	{
		sem_read_unlock(index);
		printf("%d read unlock %d\n", i, index);
	}
	
	pub_log_info("%s, %d, after read_unlock", __FILE__, __LINE__);*/

	/*write lock*/

	sem_write_lock(index);
	printf("write lock %d\n", index);

	scanf("%d", &tmp);

	sem_write_unlock(index);

	printf("write unlock %d\n", index);

	scanf("%d", &tmp);
	
	/*result = sem_lock_remove();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, sem_lock_remove error.", __FILE__, __LINE__);
		return SW_ERROR;		
	}*/

	return SW_OK;
}

