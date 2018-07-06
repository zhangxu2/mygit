#ifndef __SEM_LOCK_H__
#define __SEM_LOCK_H__
#include "pub_sem.h"
#include "pub_cfg.h"

#define SEM_VALUE (500)

/*struct sw_sem_cfg_s
{
	sw_int32_t	sem_key;
	sw_int32_t	sem_num;
};

typedef struct sw_sem_cfg_s sw_sem_cfg_t;*/

struct sw_sem_lock_s
{
	sw_int32_t	sem_id;
	sw_int32_t	sem_num;
	sw_int32_t	lock_cnt;
	sw_int32_t	lock_id;
};

typedef struct sw_sem_lock_s sw_sem_lock_t;

SW_PUBLIC sw_int32_t sem_loc_lock_creat(sw_sem_lock_t* sem_lock, sw_syscfg_t *syscfg);

SW_PUBLIC sw_int32_t sem_loc_lock_link(sw_sem_lock_t* sem_lock);

SW_PUBLIC sw_int32_t sem_loc_mutex_lock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_loc_mutex_unlock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_loc_read_lock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_loc_read_unlock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_loc_write_lock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_loc_write_unlock(sw_sem_lock_t* sem_lock, sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_loc_lock_remove(sw_sem_lock_t* sem_lock);

SW_PUBLIC sw_int32_t sem_loc_new_lock_id(sw_sem_lock_t* sem_lock);

/*global interface*/
SW_PUBLIC sw_int32_t sem_lock_creat(sw_sem_lock_t* sem_lock, sw_syscfg_t *syscfg);

SW_PUBLIC sw_int32_t sem_lock_link();

SW_PUBLIC sw_int32_t sem_lock_get_size();

SW_PUBLIC sw_int32_t sem_lock_set_addr(sw_char_t *addr);

SW_PUBLIC sw_int32_t sem_new_lock_id();

SW_PUBLIC sw_int32_t sem_mutex_lock(sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_mutex_unlock(sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_read_lock(sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_read_unlock(sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_write_lock(sw_int32_t lock_id);

SW_PUBLIC sw_int32_t sem_write_unlock(sw_int32_t lock_id);

#endif /* __SEM_LOCK_H__ */
