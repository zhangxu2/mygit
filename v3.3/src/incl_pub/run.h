/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-14
 *** module  : Interface for BP run-time shm. 
 *** name    : run.h
 *** function: BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __PUB_RUN_H__
#define __PUB_RUN_H__

#include "sem_lock.h"
#include "mtype.h"
#include "mtype_limit.h"
#include "seqs.h"
#include "procs.h"
#include "trace.h"
#include "job.h"
#include "cycle.h"

/*struct sw_global_cfg_s
{
	sw_int32_t	run_key;
	sw_mtype_cfg_t	mtype_cfg;
	sw_seqs_cfg_t	trace_cfg;
	sw_procs_cfg_t	procs_cfg;
	sw_trace_cfg_t	route_cfg;
	sw_job_cfg_t	job_cfg;
	sw_sem_cfg_t	sem_cfg;
};

typedef struct sw_global_cfg_s sw_global_cfg_t;*/
#define	SHM_MISS	-2

struct sw_run_head_s
{
	sw_int32_t	size;
	sw_int32_t	cfg_offset;
	sw_int32_t	mtype_offset;
	sw_int32_t	prdt_limit_offset;
	sw_int32_t	lsn_limit_offset;
	sw_int32_t	stat_offset;
	sw_int32_t	procs_offset;
	sw_int32_t	trace_offset;
	sw_int32_t	job_offset;
#if defined(__PRDT_ARG__)
    sw_int32_t prdt_arg_offset;
#endif /*__PRDT_ARG__*/
	sw_int32_t	sem_offset;
};

typedef struct sw_run_head_s sw_run_head_t;

SW_PUBLIC sw_int_t run_init(void);
SW_PUBLIC sw_int_t run_link(void);
SW_PUBLIC sw_int_t run_link_ext(void);
SW_PUBLIC sw_int_t run_link_and_init(void);
SW_PUBLIC sw_int_t run_destroy(void);
SW_PUBLIC sw_int_t run_get_size(void);
SW_PUBLIC sw_mtype_t* run_get_mtype(void);
SW_PUBLIC sw_mtype_limit_t* run_get_prdt_limit(void);
SW_PUBLIC sw_mtype_limit_t* run_get_lsn_limit(void);
SW_PUBLIC sw_seqs_t* run_get_stat(void);
SW_PUBLIC sw_procs_head_t* run_get_procs(void);
SW_PUBLIC sw_trace_info_t* run_get_route(void);
SW_PUBLIC sw_job_t* run_get_job(void);
SW_PUBLIC sw_cfgshm_hder_t* run_get_syscfg(void);
SW_PUBLIC sw_sem_lock_t* run_get_sem_lock(void);
SW_PUBLIC sw_int32_t run_get_shmid(void);
SW_PUBLIC sw_int_t run_set_log_level();
SW_PUBLIC int run_is_init();
SW_PUBLIC sw_int_t run_set_syslog_cfg();

#endif /* __PUB_RUN_H__ */

