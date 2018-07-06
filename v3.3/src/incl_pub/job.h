/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : interface for job in BP run-time shm. 
 *** name    : job.h
 *** function: job sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __PUB_JOB_H__
#define __PUB_JOB_H__
#include "pub_type.h"
#include "pub_cfg.h"


#define MAXJOB			50

/*job status*/
#define JOB_IDLE		0		/*job is idle*/
#define JOB_DOING		1		/*job is running*/
#define JOB_ERROR		2		/*runing error*/
#define JOB_FINISHED		3		/*job finished*/
#define JOB_TIMEOUT		4		/*job timeout*/

/*MANUAL*/
typedef enum 
{
	JOB_AUTO,
	JOB_MANUAL,
	JOB_ALL
}JOB_TYPE;

/*EXECTYPE*/
#define SCRIPT_JOB		0
#define BIN_JOB			1

/*RUNTYPE*/
#define FIXED_TIME		0
#define PERIOD_TIME		1
#define FORMAT_TIME		2

/* Descriptor of job status */
typedef struct
{
	char status;
	char desc[16];	
}job_stat_desc_t;

/* Descriptor of job type */
typedef struct
{
	JOB_TYPE type;
	char desc[16];	
}job_type_desc_t;



struct sw_job_head_s
{
	sw_int32_t	lock_id;
	sw_int32_t	count;
	sw_int32_t	cur_cnt;
};

typedef struct sw_job_head_s sw_job_head_t;

struct sw_job_item_s
{
	sw_char_t	no[8];			/*job no*/
	sw_char_t	job_name[SW_NAME_LEN];	/*job name*/
	sw_char_t	exec[SW_NAME_LEN];	/*exec file name*/
	sw_char_t	time[512];		/*time value*/
	sw_int_t	last_run_time;
	sw_int32_t	job_status;
	sw_int32_t	exec_type;		/*SCRIPT_JOB BIN_JOB*/
	JOB_TYPE	manual;			/*AUTO MANUAL*/
	sw_int32_t	job_pid;		
	sw_int32_t	time_out;		
	sw_int32_t	start_time;
	sw_int32_t	run_type;		/*fixed-time, period-time*/
	sw_char_t	week[64];
	sw_char_t	date[64];
	sw_char_t	last_run_date[16];
};

typedef struct sw_job_item_s sw_job_item_t;

struct sw_job_s
{
	sw_job_head_t	head;
	sw_job_item_t	job_item[1];
};

typedef struct sw_job_s sw_job_t;
SW_PUBLIC sw_int_t job_loc_init(sw_job_t *shm_job, sw_syscfg_t *syscfg);
SW_PUBLIC sw_int_t job_loc_load_cfg(sw_job_t *shm_job, sw_char_t *xml_name);
/*global interface*/
SW_PUBLIC sw_int_t job_init(sw_job_t *shm_job, sw_syscfg_t *syscfg);
SW_PUBLIC sw_int_t job_load_cfg(sw_char_t *xml_name);
SW_PUBLIC sw_int_t job_set_addr(sw_char_t *shm_job);
SW_PUBLIC sw_int32_t job_get_size(sw_syscfg_t *syscfg);
SW_PUBLIC sw_job_t* job_get_addr();
SW_PUBLIC const sw_char_t* job_get_stat_desc(sw_char_t stat);
SW_PUBLIC const sw_char_t* job_get_type_desc(JOB_TYPE type);

#endif /* __PUB_JOB_H__ */
