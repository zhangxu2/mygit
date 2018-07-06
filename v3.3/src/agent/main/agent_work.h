#ifndef _AGENT_WORK_H__
#define _AGENT_WORK_H__

#include "pub_pool.h"
#include "pub_time.h"
#include "agent_pub.h"
#include "select_event.h"

typedef struct
{
	int	pid;
	int	status;
        int	mem_size;
        float	per;
        char	proc_name[64];
}proc_mem;

typedef struct
{
	int	flage;
	int	proc_id;
	int	use_time;
	char	proc_name[64];
}proc_warn;


typedef int (*agt_deal_handle_pt)(void *);
typedef struct
{
	agt_deal_handle_pt	init_handler;
	sw_event_handler_pt	deal_handler; 
}agt_interface_t;

typedef struct agt_cycle_s
{
	int	lsn_fd;
	char	proc_name[32];
	void	*handle;
	sw_pool_t	*pool;
	sw_agt_cfg_t	cfg;
	sw_fd_set_t	*lsn_fds;
	agt_interface_t	handler;
}agt_cycle_t;

int agt_proc_work(agt_cycle_t *cycle);
int agt_net_work(agt_cycle_t *cycle);
int agt_cpu_work(agt_cycle_t *cycle);
int agt_mem_work(agt_cycle_t *cycle);
int agt_mtype_work(agt_cycle_t *cycle);
int agt_comm_work(agt_cycle_t *cycle);
#endif
