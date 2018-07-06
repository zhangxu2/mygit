/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swpol 
 *** name    : pol_cycle.h
 *** function: swpol life cycle class define and public interface define.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __POL_CYCLE_H__
#define __POL_CYCLE_H__
#include "cycle.h"
#include "procs.h"
#include "msg_trans.h"
#include "select_event.h"

struct sw_pol_cycle_s
{
	sw_cycle_t	base;	   /*base class object, this object must be the first member.*/
	sw_job_t	*jobs;	   /*Job info table in shm, for job schedule.*/
	sw_fd_t		cmd_fd;	   /*Control command socket fd.*/
	sw_fd_set_t	*pol_fds;  /*select event context*/
	sw_proc_info_t procs;
};

typedef struct sw_pol_cycle_s sw_pol_cycle_t;

SW_PUBLIC sw_int_t pol_cycle_init(sw_pol_cycle_t* cycle, sw_char_t* name
				, sw_int32_t module_type, sw_char_t* err_log, sw_char_t* dbg_log);

SW_PUBLIC sw_int_t pol_cycle_run(sw_pol_cycle_t* cycle);

SW_PUBLIC sw_int_t pol_cycle_destroy(sw_pol_cycle_t* cycle);

#endif /* __POL_CYCLE_H__ */
