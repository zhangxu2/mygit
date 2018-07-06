#ifndef __CYCLE_H__
#define __CYCLE_H__

#include "pub_type.h"
#include "pub_pool.h"
#include "pub_log.h"
#include "run.h"
#include "pub_cfg.h"
#include "select_event.h"

#include "pub_ares.h"
#define LINK_ERR	-1
#define	SHM_MISS	-2
#define SW_EVENT_LOOP_EXIT -2
#define SW_DELETE	2	
#define SW_CONTINUE	1

typedef struct
{
	sw_int32_t	log_level;      /*the module's log level*/
	sw_str_t	log_path;	/*log dir*/
        sw_str_t        errlog;         /*error log file*/
        sw_str_t        dbglog;         /*debug log file*/
}sw_log_t;

struct sw_cycle_s
{
	sw_str_t	name;		/*name of the module*/
	sw_int32_t	module;		/*flag of the module*/
	sw_pool_t	*pool;		/*memory pool of the module*/
	sw_log_t	*log;		/*log of the module*/
	sw_str_t	home_dir;	/*system home*/
	sw_str_t	work_dir;	/*worker home*/
	sw_global_path_t *global_path;	/*global path*/
	void		*shm_cfg;	/*shm_cfg*/
	sw_syscfg_t	*syscfg;	/*global cfg*/
	sw_proc_info_t	*proc_info;	/*module proc_info item addr*/
	sw_fd_set_t	*lsn_fds;	
	sw_event_handler_pt timeout_handler;/*timeout deal */
	void		*context;	/*extend data of the module*/
};

typedef struct sw_cycle_s sw_cycle_t;

struct sw_module_s
{
	sw_str_t	name;		/*name of the module*/
	sw_int32_t	priority;	/*priority decide the start order*/
	sw_int32_t	flag;		/*use:1 nouse:0*/
};

SW_PUBLIC sw_int_t cycle_init(sw_cycle_t* cycle, const sw_char_t* name, sw_int32_t module_type
			, const sw_char_t* err_log, const sw_char_t* dbg_log, const sw_char_t *prdt );
SW_PUBLIC sw_int_t cycle_create_shm(sw_cycle_t* cycle);
SW_PUBLIC sw_int_t cycle_link_shm(sw_cycle_t* cycle);
SW_PUBLIC sw_int_t cycle_link_shm_run(sw_cycle_t* cycle);
SW_PUBLIC sw_int_t cycle_create_shm_run(sw_cycle_t* cycle);
SW_PUBLIC sw_int_t cycle_init_shm_vars(sw_cycle_t* cycle);
SW_PUBLIC sw_int_t cycle_register(sw_cycle_t* cycle, sw_int32_t restart_type, sw_int32_t restart_cnt);
SW_PUBLIC sw_int_t cycle_set_proc_status(sw_cycle_t* cycle, sw_int32_t status);
SW_PUBLIC sw_int_t cycle_destory(sw_cycle_t* cycle);
SW_PUBLIC sw_int_t cycle_run(sw_cycle_t* cycle);
SW_PUBLIC int readfile(const char *filename, sw_buf_t *readbuf);
char	g_eswid[64];
int	g_workmode_mp;
int	g_use_local_logs;

#endif

