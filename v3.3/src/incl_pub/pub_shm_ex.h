#ifndef _SW_SHM_EX_H
#define _SW_SHM_EX_H

#include "pub_log.h"
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct
{
	int	id;
	u_char	*addr;
	size_t	size;
	sw_str_t	name;
	sw_uint_t	exists;   /* unsigned  exists:1;  */
}sw_shm_t;

sw_int_t pub_shm_alloc(sw_shm_t *shm);
void pub_shm_free(sw_shm_t *shm);


#endif
