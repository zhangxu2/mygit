/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swjob 
 *** name    : job_cycle.c
 *** function: swjob life cycle class define and public interface define.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __JOB_CYCLE_H__
#define __JOB_CYCLE_H__
#include "cycle.h"
#include "job.h"
#include "msg_trans.h"
#include "select_event.h"

/*swjob life cycle object.*/
struct sw_job_cycle_s
{
	sw_cycle_t	base;			/*base class object, this object must be the first member.*/
	sw_job_t	*jobs;			/*Job info table in shm, for job schedule.*/
	sw_time_t	job_cfg_modify_time;	/*job.xml configure file last modify time, for job.xml reload.*/
	sw_fd_t		cmd_fd;			/*Control command socket fd.*/
	sw_fd_t		job_status_fd;		/*Used to get job exit status.*/
	sw_str_t	job_sock_path;		/*Job status unix local socket path.*/
	sw_fd_set_t*	job_fds;		/*select event context*/
	sw_proc_info_t  proc;
};

typedef struct sw_job_cycle_s sw_job_cycle_t;

SW_PUBLIC sw_int_t job_cycle_init(sw_job_cycle_t* cycle, sw_char_t* name
				, sw_int32_t module_type, sw_char_t* err_log, sw_char_t* dbg_log);

SW_PUBLIC sw_int_t job_cycle_run(sw_job_cycle_t* cycle);

SW_PUBLIC sw_int_t job_cycle_destroy(sw_job_cycle_t* cycle);

#endif /* __JOB_CYCLE_H__ */
