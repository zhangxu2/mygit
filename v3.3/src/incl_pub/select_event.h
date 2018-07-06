#ifndef __SELECT_ENVENT_H__
#define	__SELECT_ENVENT_H__

#include <errno.h>
#include <sys/socket.h>
#include "pub_type.h"

/*select wait infinitely.*/
#define	SW_TIMER_INFINITE (sw_uint64_t)-1

/*Default max fd list length, if the num of fd greater than 1024, this list will increase dynamiclly. */
#define FD_LIST_MAX 1024

typedef sw_int_t (*sw_event_handler_pt)(void*);

struct sw_fd_list_s
{
	sw_event_handler_pt	event_handler;
	void			*data;
	sw_fd_t			fd;
	sw_int32_t		flag;
	
};

typedef struct sw_fd_list_s sw_fd_list_t;

struct sw_fd_set_s
{
	sw_int32_t fd_cnt;
	sw_int32_t fd_max;
	sw_int32_t fd_list_max;
	sw_int32_t fd_work_max;
	sw_fd_list_t *fd_list;
	sw_fd_list_t *fd_work;
	fd_set master_read_fd_set;
	fd_set worker_read_fd_set;
};
typedef struct sw_fd_set_s sw_fd_set_t;


sw_int_t select_add_event(sw_fd_set_t *p,sw_fd_list_t *fd_list);
sw_int_t select_init(sw_fd_set_t *p);
void     select_clear(sw_fd_set_t *p);
sw_int_t select_del_event(sw_fd_set_t *p,sw_fd_t fd);
sw_int_t select_process_events(sw_fd_set_t *p,sw_fd_list_t **fd_list,sw_uint64_t timer);
sw_int_t select_repair_fd_sets(sw_fd_set_t *p);

#endif

