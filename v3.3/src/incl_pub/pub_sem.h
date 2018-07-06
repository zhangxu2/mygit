#ifndef __PUB_SEM_H__
#define __PUB_SEM_H__

#include "pub_type.h"
#include <sys/sem.h>

SW_PUBLIC sw_int32_t pub_sem_create(int key,int size,int value);
SW_PUBLIC sw_int32_t pub_sem_open(int key,int size);
SW_PUBLIC sw_int32_t pub_sem_rm(int semid);
SW_PUBLIC sw_int32_t pub_sem_unlock(int semid,int num,int value) ;
SW_PUBLIC sw_int32_t pub_sem_lock(int semid,int num,int value);
SW_PUBLIC sw_int32_t pub_sem_check(int semid);

#endif
