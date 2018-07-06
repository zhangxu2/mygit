#ifndef __PUB_SHM_H__
#define  __PUB_SHM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <errno.h>


#include "pub_type.h"

sw_int_t pub_shm_create(key_t  key, size_t size);
sw_int_t pub_shm_get(key_t  key);
sw_char_t *pub_shm_at(int shmid);
sw_int_t pub_shm_dt(char *shm);
sw_int_t pub_shm_rm(int shmid);


#endif
