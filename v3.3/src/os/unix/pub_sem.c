#include "pub_sem.h"

/**
  ������:nSemInit
  ����  :�������������źŵ�
  ����  :
  nSemKey   �ؼ���
  nSize     Ԫ�ظ���
  ����ֵ:
  -1         ʧ��
  >0����     ��Ϣid
 **/
sw_int32_t pub_sem_create(int key,int size,int value)
{
	int i;
	int semid;
	union  semun 
	{
		int val;
		struct semid_ds *buf;
		unsigned int *array;
	}sem;

	memset(&sem,'\0',sizeof(sem));

	semid = semget((key_t)key,size, IPC_CREAT|0666);
	if (semid== -1) 
	{
		return(SW_ERR);
	}

	sem.val=value;	
	for(i=0;i<size;i++)
	{
		semctl(semid,i,SETVAL,sem);
	}
	return(semid);
}

sw_int32_t pub_sem_open(int key, int size)
{
	int semid;

	if(size<=0)
	{
		return SW_ERR;
	}
	semid = semget((key_t)key,size,0);
	if(semid<0 )
	{
		return(SW_ERR);
	}
	return(semid);
}

/**
  ������:P
  ����  :P����
  ����  :
  nId   �����
  ����ֵ:
  0/-1
 **/
sw_int32_t pub_sem_lock(int semid,int num,int value) 
{
	struct sembuf sem;
	if(semid < 0)
	{
		return SW_ERR;
	}
	sem.sem_num=num;
	sem.sem_op=value;
	sem.sem_flg=SEM_UNDO;
	return semop(semid,&sem,1);
}
/**
  ������:V
  ����  :V����
  ����  :
  nId   �����
  ����ֵ:
  0/-1
 **/
sw_int32_t pub_sem_unlock(int semid,int num,int value) 
{
	struct sembuf sem;
	if(semid <0 )
	{
		return SW_ERR;
	}
	sem.sem_num=num;
	sem.sem_op=value;
	sem.sem_flg=SEM_UNDO;
	return(semop(semid,&sem,1));
}

/**
  ������:pub_sem_rm
  ����  :ɾ���źŵ�
  ����  :
  ����ֵ:
  0          �ɹ�
  -1         ʧ��
 **/
sw_int32_t pub_sem_rm(int semid)
{
	return(semctl(semid,0,IPC_RMID,0));
}

sw_int32_t pub_sem_check(int semid)
{
	sw_int32_t	result = SW_ERROR;
	union {
		int 	     val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO
					    (Linux specific) */
	} semopts;   
	struct semid_ds ds;
	semopts.buf = &ds;

	result = semctl(semid, 0, IPC_STAT, semopts);
	if (-1 == result)
	{
		return SW_ERROR;		
	}

	return SW_OK;
}

/**********************����********************
  int test(){
  int iRc;
  char *ptr;
  char s[100];
  int id=0;
  union  semun {
  int val;
  struct semid_ds *buf;
  unsigned int *array;
  }sem;
  g_semid=nSemInit(0x1234,6);
  if(g_semid<0){
  printf("Open error[%d]\n",errno);
  exit(1);
  }
  while(1){
  printf("Enter index:");
  scanf("%d",&id);
  P(id);
  printf("Lock success,press any key to continue...\n");
  scanf("%s",s);
  V(id);
  }
  pub_sem_rm();
  return(0);
  }
 *****/
