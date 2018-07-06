#ifndef __SW_SHMEX_H__
#define __SW_SHMEX_H__

#include "pub_type.h"
#include <sys/mman.h>

typedef struct
{
	u_char      *addr;
	size_t       size;
	char	filename[128];
	sw_uint_t   exists;   /* unsigned  exists:1;  */
} sw_shmex_t;


sw_int_t pub_shmex_alloc(sw_shmex_t *shm);
void pub_shmex_free(sw_shmex_t *shm);


#endif /* __SW_SHMEX_H__ */
