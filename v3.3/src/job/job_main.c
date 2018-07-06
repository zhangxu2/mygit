/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swjob 
 *** name    : swjob.c 
 *** function: main function
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "job_cycle.h"

int main(int argc,char *argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_job_cycle_t	cycle;

	pub_mem_memzero(&cycle, sizeof(sw_job_cycle_t));

	/*init swjob cycle object*/
	result = job_cycle_init(&cycle, "swjob", ND_JOB, "error.log", "swjob.log");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d]swjob starting...", __FILE__, __LINE__);

	/*run swjob event process loop*/
	result = job_cycle_run(&cycle);
	if(result < 0 && result != -2)
	{
		pub_log_error("%s, %d, Run swjob event process loop fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*destory swjob cycle object*/
	job_cycle_destroy(&cycle);
	
	return SW_OK;
}

