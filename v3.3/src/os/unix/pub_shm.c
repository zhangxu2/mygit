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
 **��������: pub_shm_create
 **��    ��: ���������ڴ�(����С��һ��ʱ��ɾ��ԭ�й����ڴ棬�ٴ����µĹ����ڴ�)
 **�������: 
 **     key: ��ֵ
 **     size: ��С
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.14 #
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

