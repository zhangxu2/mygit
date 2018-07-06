/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swjob 
 *** name    : job_schedule.c
 *** function: job exit status info struct define
               , job schedule public interface
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __JOB_SCHEDULE_H__
#define __JOB_SCHEDULE_H__
#include "job_cycle.h"

/*Job child process exit status info.*/
struct sw_job_status_s
{
	sw_int32_t 	job_pid;	/*Job child process pid.*/
	sw_int_t	last_run_time;	/*Job child process last run time.*/
	sw_char_t	return_code;	/*Job child process exit status.*/
};

typedef struct sw_job_status_s sw_job_status_t;

SW_PUBLIC sw_int_t job_schedule(sw_job_cycle_t* cycle);
SW_PUBLIC sw_int_t job_run_by_no(sw_job_cycle_t* cycle, sw_char_t* job_no);

#endif /* __JOB_SCHEDULE_H__ */
