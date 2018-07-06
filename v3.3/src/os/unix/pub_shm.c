/*********************************************************************
 *** version : V3.0
 *** author  : zhang hailu
 *** create  : 2013-5-21
 *** module  : unix
 *** name    : pub_shm.c
 *** function:  
		 pub_shm_get
		 pub_shm_at
		 pub_shmdt
		 pub_shm_rm 
 *** notice  :  
 *** modified:  
 ***   author:  
 ***   date  :  
 ***  content:  
 ********************************************************************/
#include "pub_shm.h"

/******************************************************************************
 *** function  : pub_shm_get
 *** author    : zhang hailu
 *** create    : 2013-5-21 15:4
 *** call lists:  shmget
 *** inputs    : 
 ***     arg1  :  key	shm key
 ***	 arg2  :  size	shm size	
 *** outputs   :  
 ***     arg1  : 
 *** return    :  0:success  -1:fail
 *** notice    : 
 ***   author  : 
 ***   date    : 
 ***   content : 
 ******************************************************************************/
sw_int_t pub_shm_get(key_t key)
{
	return(shmget((key_t)key,0,0660));

}

/******************************************************************************
 **函数名称: pub_shm_create
 **功    能: 创建共享内存(当大小不一致时，删除原有共享内存，再创建新的共享内存)
 **输入参数: 
 **     key: 键值
 **     size: 大小
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **修    改: # Qifeng.zou # 2013.10.14 #
 ******************************************************************************/
sw_int_t pub_shm_create(key_t key, size_t size)
{
	int	shmid = 0;
	struct shmid_ds	shmds;

	memset(&shmds, 0, sizeof(shmds));

	shmid = pub_shm_get(key);
	if(shmid < 0)
	{
		if(ENOENT == errno)
		{
			return shmget((key_t)key,size,IPC_CREAT|0660);
		}

		return -1;
	}

	shmctl(shmid, IPC_STAT, &shmds);
	if(shmds.shm_segsz == size)
	{
		return shmid;
	}

	shmctl(shmid, IPC_RMID, NULL);

	return shmget(key, size, IPC_CREAT|0660);
}

/******************************************************************************
 *** function  : pub_shm_at
 *** author    : zhang hailu
 *** create    : 2013-5-21 15:8
 *** call lists:  
 *** inputs    : 
 ***     arg1  :  shmid   shm id
 *** outputs   :  
 ***     arg1  : 
 *** return    :  0:success  -1:fail
 *** notice    : 
 ***   author  : 
 ***   date    : 
 ***   content : 
 ******************************************************************************/
sw_char_t* pub_shm_at(int shmid) 
{
	return((char *)shmat(shmid,NULL,0));
}

/******************************************************************************
 *** function  : pub_shm_dt
 *** author    : zhang hailu
 *** create    : 2013-5-21 15:8
 *** call lists:  
 *** inputs    : 
 ***     arg1  :  shm    the Memory address
 *** outputs   :  
 ***     arg1  : 
 *** return    :  0:success  -1:fail
 *** notice    : 
 ***   author  : 
 ***   date    : 
 ***   content : 
 ******************************************************************************/
sw_int_t pub_shm_dt(char *shm) 
{
	return(shmdt(shm));
}

/******************************************************************************
 *** function  : pub_shm_rm
 *** author    : zhang hailu
 *** create    : 2013-5-21 16:15
 *** call lists:  
 *** inputs    : 
 ***     arg1  :  
 *** outputs   :  
 ***     arg1  : 
 *** return    :  0:success  -1:fail
 *** notice    : 
 ***   author  : 
 ***   date    : 
 ***   content : 
 ******************************************************************************/
sw_int_t pub_shm_rm(int shmid)
{
	return(shmctl(shmid,IPC_RMID,0));
}

