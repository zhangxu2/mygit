/*********************************************************************
 *** version : v2.0
 *** author  : wangkun
 *** create  : 2013-4-16
 *** module  : swadmin 
 *** name    : adm_cmd_handler.c 
 *** function: swadmin's command handlers
 *** lists   :
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/
#include "pub_time.h"
#include "job.h"
#include "adm_cmd_handler.h"
#include "adm_vers.h" 
#include "pub_time.h"
#include "pub_usocket.h"

/* Declare static function */
SW_PROTECTED sw_int_t adm_quit(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_help(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_param_error(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_unknown(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_goto_prdt(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_set_plat_flow(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_set_bsn_flow(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_set_date(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_set_date_lock(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_set_date_unlock(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_get_plat_flow(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_get_bsn_flow(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_get_bsn_flow_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_get_date(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_start_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_lsn_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_lsn(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_lsn_in_prdt(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_proc(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_svc_all(sw_adm_cycle_t * cycle);
SW_PROTECTED sw_int_t adm_start_svc(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_start_svr_in_prdt(sw_adm_cycle_t * cycle);
SW_PROTECTED sw_int_t adm_start_prdt(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_stop_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_proc(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_lsn_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_lsn_in_prdt(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_lsn(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_svc_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_stop_svc(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_prdt(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_stop_svr_in_prdt(sw_adm_cycle_t* cycle);

SW_PROTECTED sw_int_t adm_list_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_in_all_prdt(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_lsn_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_svc_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_mtype_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_mtype_busy(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_mtype_free(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_mtype(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_mtype_seg(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_res_all(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_res_shm(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_list_res_msq(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_clear(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_clean(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_top(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_step(sw_adm_cycle_t* cycle);

SW_PROTECTED sw_int_t adm_task_print_head();
SW_PROTECTED sw_int_t adm_task_print_item(sw_job_item_t *job_item);
SW_PROTECTED sw_int_t adm_task_print_tail(sw_int32_t all);
SW_PROTECTED sw_int_t adm_list_task_by_type(JOB_TYPE type);
SW_PROTECTED sw_int_t adm_list_manual_task(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_auto_task(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_task_s(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_list_all_task(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_all_task(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_start_task_s(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_get_log(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_prdt_start_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_start_lsn_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_start_lsn(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_start_svc_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_start_svc(sw_adm_cycle_t *cycle);    

SW_PROTECTED sw_int_t adm_prdt_stop_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_stop_lsn_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_stop_lsn(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_stop_svc_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_stop_svc(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_prdt_list_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_list_lsn_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_list_svc_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_list_mtype_all(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_prdt_list_child_all(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_saf_list(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_saf_start(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_saf_stop(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_deploy_bp(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t  adm_reload(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_sys_startup(sw_adm_cycle_t *cycle);
SW_PROTECTED sw_int_t adm_sys_shutdown(sw_adm_cycle_t *cycle);

SW_PROTECTED sw_int_t adm_list_his_patch(sw_adm_cycle_t *cycle);
static int show_reload_usage(bool show_lsn, bool show_prdt);
static int adm_reload_lsn( sw_uni_svr_t *svr, int argc, char **argv);
static int adm_reload_prdt(sw_uni_svr_t *svr, int argc, char **argv);

/* Command handler array configuration */
static const sw_adm_cmd_handler_t g_cmd_handler_array[] =
{
	{
		ADM_CMD_START_ALL,
		ADM_SUB_MOD_DFIS,
		adm_start_all,
		"start/s",
		"Start DFIS-BP"
	},
	{
		ADM_CMD_START_LSN_ALL,
		ADM_SUB_MOD_DFIS,
		adm_start_lsn_all,
		"start/s -l",
		"Start all listen"
	},
	{
		ADM_CMD_START_LSN,
		ADM_SUB_MOD_DFIS,
		adm_start_lsn,
		"start/s -l [lsn1] [lsn2] ...",
		"Start special listen"
	}, 	
	{
		ADM_CMD_START_LSN_IN_PRDT,
		ADM_SUB_MOD_DFIS,
		adm_start_lsn_in_prdt,
		"start/s -pl [prdt] [lsn1] [lsn2] ...",
		"Start special lsn in special product"
	}, 	
	{
		ADM_CMD_START_PROC,
		ADM_SUB_MOD_DFIS, 
		adm_start_proc,
		"start/s [proc1] [proc2] ...",
		"Start special process"
	},
	{
		ADM_CMD_START_SVC_ALL,
		ADM_SUB_MOD_DFIS,
		adm_start_svc_all,
		"start/s -svc/-s",
		"Start all service"
	},
	{
		ADM_CMD_START_SVC,
		ADM_SUB_MOD_DFIS,
		adm_start_svc,
		"start/s -s [svc1] [svc2] ...",
		"Start special service"
	},
	{
		ADM_CMD_START_SVR_IN_PRDT,
		ADM_SUB_MOD_DFIS,
		adm_start_svr_in_prdt,
		"start/s -ps [prdt] [svc1] [svc2] ...",
		"Start special server in special product"
	},
	{
		ADM_CMD_START_PRDT,
		ADM_SUB_MOD_DFIS,
		adm_start_prdt,
		"start/s -p [prdt1] [prdt2] ...",
		"Start special product"
	},
	{
		ADM_CMD_QUIT, 	
		ADM_SUB_MOD_ALL,
		adm_quit,
		"quit/exit/q",
		"Quit DFIS-BP"
	}, 	
	{
		ADM_CMD_STOP_ALL,
		ADM_SUB_MOD_DFIS,
		adm_stop_all,
		"stop",
		"Stop DFIS-BP"
	},  
	{
		ADM_CMD_STOP_PROC, 
		ADM_SUB_MOD_DFIS,
		adm_stop_proc,
		"stop [proc1] [porc2] ...",
		"Stop special process."
	}, 	
	{
		ADM_CMD_STOP_LSN_ALL,
		ADM_SUB_MOD_DFIS,
		adm_stop_lsn_all,
		"stop -l",
		"Stop all listen"
	}, 	
	{
		ADM_CMD_STOP_LSN,
		ADM_SUB_MOD_DFIS, 
		adm_stop_lsn,
		"stop -l [lsn1] [lsn2] ...",
		"Stop special listen"
	}, 
	{
		ADM_CMD_STOP_LSN_IN_PRDT,
		ADM_SUB_MOD_DFIS, 
		adm_stop_lsn_in_prdt,
		"stop -pl [prdt] [lsn1] [lsn2] ...",
		"Stop special listen in special product"
	}, 
	{
		ADM_CMD_STOP_SVC_ALL,
		ADM_SUB_MOD_DFIS,
		adm_stop_svc_all,
		"stop -s",
		"Stop all service"
	},
	{
		ADM_CMD_STOP_SVC,
		ADM_SUB_MOD_DFIS,
		adm_stop_svc,
		"stop -s [svc1] [svc2] ...",
		"Stop special service"
	},	
	{
		ADM_CMD_STOP_PRDT,
		ADM_SUB_MOD_DFIS,
		adm_stop_prdt,
		"stop -p [prdt1] [prdt2] ...",
		"Stop special product"
	},
	{
		ADM_CMD_STOP_SVR_IN_PRDT,
		ADM_SUB_MOD_DFIS,
		adm_stop_svr_in_prdt,
		"stop -ps [prdt] [svr1] [svr2] ...",
		"Stop special server in special product"
	},
	{
		ADM_CMD_LIST_ALL,
		ADM_SUB_MOD_DFIS,
		adm_list_all,
		"list/l",
		"List all process"
	}, 	
	{
		ADM_CMD_LIST_LSN_ALL,
		ADM_SUB_MOD_DFIS,
		adm_list_lsn_all,
		"list/l -l",
		"List all listen"
	},
	{
		ADM_CMD_LIST_SVC_ALL,
		ADM_SUB_MOD_DFIS,
		adm_list_svc_all,
		"list/l -s",
		"List all service"
	}, 	
	{
		ADM_CMD_LIST_PRDT_ALL,
		ADM_SUB_MOD_DFIS,
		adm_list_prdt_all,
		"list/l -p",
		"List all product"
	},
	{
		ADM_CMD_LIST_ALL_IN_PRDT,
		ADM_SUB_MOD_DFIS,
		adm_list_in_all_prdt,
		"list/l -pp",
		"List all processes of all product"
	},
	{
		ADM_CMD_LIST_VERSION,
		ADM_SUB_MOD_DFIS,
		adm_list_version,
		"list/l -v",
		"List version"
	},
	{
		ADM_CMD_LIST_HIS_PATCH,
		ADM_SUB_MOD_DFIS,
		adm_list_his_patch,
		"list/l -his",
		"List history patch info"
	},
	{
		ADM_CMD_LIST_MTYPE_ALL,
		ADM_SUB_MOD_DFIS,
		adm_list_mtype_all,
		"list/l -ma/-am",
		"List all mtypes"
	},
	{
		ADM_CMD_LIST_MTYPE_BUSY,
		ADM_SUB_MOD_DFIS,
		adm_list_mtype_busy,
		"list/l -m",
		"List all of busy mtypes"
	},
	{
		ADM_CMD_LIST_MTYPE_FREE,
		ADM_SUB_MOD_DFIS,
		adm_list_mtype_free,
		"list/l -fm/-mf",
		"List all of free mtypes"
	},
	{
		ADM_CMD_LIST_MTYPE,
		ADM_SUB_MOD_DFIS,
		adm_list_mtype,
		"list/l -m [mtype]",
		"List special mtype"
	},
	{
		ADM_CMD_LIST_SEG_MTYPE,
		ADM_SUB_MOD_DFIS,
		adm_list_mtype_seg,
		"list/l -m [mtype] [num]",
		"List special num of mtype"
	},
	{
		ADM_CMD_LIST_RES_ALL,
		ADM_SUB_MOD_DFIS,
		adm_list_res_all,
		"list/l -r",
		"List all resource"
	},
	{
		ADM_CMD_LIST_RES_SHM,
		ADM_SUB_MOD_DFIS,
		adm_list_res_shm,
		"list/l -r shm",
		"List SHM"
	},
	{
		ADM_CMD_LIST_RES_MSQ,
		ADM_SUB_MOD_DFIS,
		adm_list_res_msq,
		"list/l -r msq",
		"List MSQ"
	},
	{
		ADM_CMD_CLEAR,
		ADM_SUB_MOD_DFIS, 
		adm_clear, 
		"clear", 
		"Clear screen"
	},
	{
		ADM_CMD_CLEAN,
		ADM_SUB_MOD_DFIS, 
		adm_clean, 
		"clean --y/--yes", 
		"Clean resource and exit"
	},

	{
		ADM_CMD_LIST_TASK_ALL,
		ADM_SUB_MOD_DFIS, 
		adm_list_all_task,
		"task -l", 
		"list all task."
	}, 	
	{
		ADM_CMD_OPT_TASK_ALL, 
		ADM_SUB_MOD_DFIS,
		adm_start_all_task, 
		"task -s",
		"start all task."
	}, 	
	{
		ADM_CMD_LIST_TASK_M,
		ADM_SUB_MOD_DFIS,
		adm_list_manual_task,
		"task -lm",
		"list manule task"
	}, 	
	{
		ADM_CMD_LIST_TASK_AU,
		ADM_SUB_MOD_DFIS,
		adm_list_auto_task,
		"task -la ",
		"list auto task"
	},

	{
		ADM_CMD_LIST_TASK_S,
		ADM_SUB_MOD_DFIS,
		adm_list_task_s,
		"task -l [job1] [job2] ...",
		"list special task"
	},

	{
		ADM_CMD_OPT_TASK_S,
		ADM_SUB_MOD_DFIS,
		adm_start_task_s,
		"task -s [job1] [job2] ...",
		"start special task"
	},

	{
		ADM_CMD_TOP,
		ADM_SUB_MOD_DFIS, 
		adm_top, 
		"top [num]",
		"Lastest flow"
	}, 
	{
		ADM_CMD_STEP,
		ADM_SUB_MOD_DFIS, 
		adm_step,
		"step trace_no",
		"View special flow"
	},

	/* 产品相关命令 */
	{
		ADM_CMD_GET_LOG,
		ADM_SUB_MOD_DFIS, 
		adm_get_log,
		"get -log [trace_no][date] [lsn_name]",
		"get log by trace, date and lsn_name in logsvr"
	},

	/* 产品相关命令 */
	{
		ADM_CMD_GOTO_PRDT,
		ADM_SUB_MOD_PRDT,
		adm_goto_prdt,
		"goto product",
		"Entry special product"
	},
	{
		ADM_CMD_PRDT_START_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_start_all,
		"start/s",
		"Start all process of special product"
	},
	{
		ADM_CMD_PRDT_START_ALL_LSN,
		ADM_SUB_MOD_PRDT,
		adm_prdt_start_lsn_all,
		"start/s -l",
		"Start all listen of special product"
	},
	{
		ADM_CMD_PRDT_START_LSN,
		ADM_SUB_MOD_PRDT,
		adm_prdt_start_lsn,
		"start/s -l [lsn1] [lsn2] ...",
		"Start all listen of special product"
	},
	{
		ADM_CMD_PRDT_START_ALL_SVC,
		ADM_SUB_MOD_PRDT,
		adm_prdt_start_svc_all,
		"start/s -s",
		"Start all listen of special product"
	},
	{
		ADM_CMD_PRDT_START_SVC,
		ADM_SUB_MOD_PRDT,
		adm_prdt_start_svc,
		"start/s -s [svc1] [svc2] ...",
		"Start all listen of special product"
	},
	{
		ADM_CMD_PRDT_STOP_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_stop_all,
		"stop",
		"Start all process of special product"
	},
	{
		ADM_CMD_PRDT_STOP_LSN_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_stop_lsn_all,
		"stop -l",
		"Stop all listen of special product"
	},
	{
		ADM_CMD_PRDT_STOP_LSN,
		ADM_SUB_MOD_PRDT,
		adm_prdt_stop_lsn,
		"stop -l [lsn1] [lsn2] ...",
		"Stop special listen of special product"
	},
	{
		ADM_CMD_PRDT_STOP_SVC_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_stop_svc_all,
		"stop/s -s",
		"Stop all svc of special product"
	},
	{
		ADM_CMD_PRDT_STOP_SVC,
		ADM_SUB_MOD_PRDT,
		adm_prdt_stop_svc,
		"stop -l [svc1] [svc2] ...",
		"Stop special svc of special product"
	},
	{
		ADM_CMD_PRDT_LIST_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_list_all,
		"list/l",
		"List all process of special product"
	},
	{
		ADM_CMD_PRDT_LIST_LSN_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_list_lsn_all,
		"list/l -l",
		"List all listen of special product"
	},
	{
		ADM_CMD_PRDT_LIST_SVC_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_list_svc_all,
		"list/l -s",
		"List all svc of specail product"
	},
	{
		ADM_CMD_PRDT_LIST_CHILD_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_list_child_all,
		"list/l -c",
		"List all children of special product"
	},
	{
		ADM_CMD_PRDT_LIST_MTYPE_ALL,
		ADM_SUB_MOD_PRDT,
		adm_prdt_list_mtype_all,
		"list/l -m",
		"List all mtype of specail product"
	},

	/* SET相关命令 */
	{
		ADM_CMD_SET_PLT_FLOW,
		ADM_SUB_MOD_DFIS,
		adm_set_plat_flow,
		"set -pf No. [NAME]",
		"Set flow no. of platform"
	},
	{
		ADM_CMD_SET_BSN_FLOW,
		ADM_SUB_MOD_DFIS,
		adm_set_bsn_flow,
		"set -bf No. NAME",
		"Set flow no. of business"
	},
	{
		ADM_CMD_SET_DATE,
		ADM_SUB_MOD_DFIS,
		adm_set_date,
		"set -d NEWDATE",
		"Set date of platform"
	},
	{
		ADM_CMD_SET_DATE_LOCK,
		ADM_SUB_MOD_DFIS,
		adm_set_date_lock,
		"set -d NEWDATE -lock",
		"Set and lock date"
	},
	{
		ADM_CMD_SET_DATE_UNLOCK,
		ADM_SUB_MOD_DFIS,
		adm_set_date_unlock,
		"set -d -unlock",
		"Unlock date"
	},

	/* GET相关命令 */
	{
		ADM_CMD_GET_PLT_FLOW,
		ADM_SUB_MOD_DFIS,
		adm_get_plat_flow,
		"get -pf",
		"Get flow no. of platform"
	},
	{
		ADM_CMD_GET_BSN_FLOW,
		ADM_SUB_MOD_DFIS,
		adm_get_bsn_flow,
		"get -bf NAME",
		"Get flow no. of business by name"
	},
	{
		ADM_CMD_GET_BSN_FLOW_ALL,
		ADM_SUB_MOD_DFIS,
		adm_get_bsn_flow_all,
		"get -bf",
		"Get flow no. of all business"
	},
	{
		ADM_CMD_GET_DATE,
		ADM_SUB_MOD_DFIS,
		adm_get_date,
		"get -d",
		"Get date of platform"
	},

	{
		ADM_CMD_SAF_LIST,
		ADM_SUB_MOD_DFIS,
		adm_saf_list,
		"saf -l [date] [traceno]",
		"list saf"
	},
	{
		ADM_CMD_SAF_START,
		ADM_SUB_MOD_DFIS,
		adm_saf_start,
		"saf -r [rtaceno] [date]",
		"list manule task"
	},

	{
		ADM_CMD_SAF_STOP,
		ADM_SUB_MOD_DFIS,
		adm_saf_stop,
		"saf -e [traceno] [date] ",
		"list auto task"
	},

	{
		ADM_CMD_DEPLOY,             
		ADM_SUB_MOD_DFIS,
		adm_deploy_bp,
		"deploy -file=PATH",
		"deploy bp from config file"
	},
	{
		ADM_CMD_RELOAD,
		ADM_SUB_MOD_DFIS,
		adm_reload,
		"*just type reload to see",
		"reload bp with your command",
	},
	{
		ADM_CMD_STARTUP,
		ADM_SUB_MOD_ALL,
		adm_sys_startup,
		"startup",
		"startup bp(system status)"
	},
	{
		ADM_CMD_SHUTDOWN,
		ADM_SUB_MOD_ALL,
		adm_sys_shutdown,
		"shutdown",
		"shutdown bp(system status)"
	},

	/* 其他相关命令 */
	{
		ADM_CMD_HELP,
		ADM_SUB_MOD_ALL, 
		adm_help, 
		"help/h", 
		"Help information"
	},
	{
		ADM_CMD_PARAM_ERR,
		ADM_SUB_MOD_ALL,
		adm_param_error,
		"Paramter is incorrect",
		"Paramter is incorrect"
	},
	{
		ADM_CMD_UNKNOWN,
		ADM_SUB_MOD_ALL,
		adm_unknown,
		"Unknow command",
		"Unknow command"
	}
};

/* Command range: ADM_CMD_START_ALL ~ ADM_CMD_TOTAL */
#define adm_get_cmd_type(idx)   (g_cmd_handler_array[idx].cmd_type)
#define adm_get_mod_type(cmd)   (g_cmd_handler_array[cmd].mod_type)
#define amd_get_cmd_handler(cmd)    (g_cmd_handler_array[cmd].cmd_handle_func)
#define adm_get_cmd_usage(cmd)  (g_cmd_handler_array[cmd].usage)
#define adm_get_cmd_desc(cmd)   (g_cmd_handler_array[cmd].desc)

/******************************************************************************
 ** Name : adm_get_cmd_handler
 ** Desc : 初始化处理数组
 ** Input: 
 **     cmd_type: cmd type
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
const sw_adm_cmd_handler_t* adm_get_cmd_handler(sw_int32_t cmd_type)
{
	if(adm_cmd_is_valid(cmd_type))
	{
		return &g_cmd_handler_array[cmd_type];
	}

	printf("[%s][%d] Unknown command.\n", __FILE__, __LINE__);
	return NULL;
}

sw_int_t adm_cmd_handle(sw_adm_cycle_t* cycle, char *line)
{
	sw_int_t ret = SW_ERROR;
	sw_adm_opt_t *opt = cycle->opt;
	const sw_adm_cmd_handler_t* cmd_handler = NULL;

	/*Get opt*/
	ret = adm_opt_parse(opt, line);
	if(SW_OK != ret)
	{
		pub_log_info("[%s][%d] Parase command failed. type:[%d",
				__FILE__, __LINE__, opt->cmd_type);
		return SW_OK;
	}

	/*Get cmd handler and invoke it.*/
	cmd_handler = adm_get_cmd_handler(opt->cmd_type);
	if(NULL == cmd_handler)
	{
		pub_log_info("[%s][%d] Command handler is NULL!", __FILE__, __LINE__);
		return SW_OK;
	}

	/*invoke cmd handler*/
	ret = cmd_handler->cmd_handle_func(cycle);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Exec command handler failed. [%d]",
				__FILE__, __LINE__, opt->cmd_type);
		return SW_ERROR;
	}

	return SW_OK;
}


/******************************************************************************
 ** Name : adm_quit
 ** Desc : Quit admin
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : quit/exit
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
sw_int_t adm_quit(sw_adm_cycle_t* cycle)
{
	if(0 == strcmp(cycle->opt->mod_name, ADM_MOD_DFIS_STR))
	{
		pub_log_info("quit swadmin!");
		exit(0);
	}
	adm_opt_set_submod_str(cycle->opt, ADM_MOD_DFIS_STR);
	cycle->opt->mod_type = ADM_SUB_MOD_DFIS;

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_help
 ** Desc : Display help information
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : help
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
sw_int_t adm_help(sw_adm_cycle_t* cycle)
{
	sw_int32_t idx=0, total=0;

	printf("\tNo.  COMMAND\t\t\t\t DESCRIPTION\n");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	for (idx = 0;idx < ADM_CMD_TOTAL; idx++)
	{
		if(ADM_CMD_UNKNOWN == idx
				|| ADM_CMD_PARAM_ERR == idx)
		{
			continue;
		}

		if(adm_get_mod_type(idx) & cycle->opt->mod_type)
		{
			total++;
			printf("\t[%02d] %-35s %s\n",
					total, adm_get_cmd_usage(idx), adm_get_cmd_desc(idx));
		}
	}
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d          STATUS: %s\n", total, seqs_get_syssts_desc());

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_set_plat_flow
 ** Desc : Set flow No. of platform
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : set -pf [flwno]
 **         exp: set -pf 100002000
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_set_plat_flow(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;
	sw_int64_t newval;

	newval = -1;
	newval = atoll(opt->local_argv[2]);
	if (newval == -1)
		return SW_ERR;

	if (opt->local_argc == 3)
		ret = seqs_set_plat_flow(newval);
	else if (opt->local_argc == 4)
		ret = seqs_set_bp_flow(opt->local_argv[3], newval);

	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Set platform flow No. failed! [%s]",
				__FILE__, __LINE__, opt->local_argv[2]);
		printf("\n\tSet platform flow No. failed\n");
		return SW_ERR;
	}

	printf("\n\tSet platform flow No. success\n");
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_set_bsn_flow
 ** Desc : Set flow No. of business
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : set -bf [flwno] [name]
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
static sw_int_t adm_set_bsn_flow(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;

	ret = seqs_set_bsn_flow(opt->local_argv[3], atoll(opt->local_argv[2]));
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Set business flow No. failed! [%s] [%s]",
				__FILE__, __LINE__, opt->local_argv[2], opt->local_argv[3]);
		printf("\n\tSet business flow No. failed\n");
		return SW_ERR;
	}

	printf("\n\tSet business flow No. success\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_set_date
 ** Desc : Set date of platform
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : set -d [YYYMMDD]
 **        exp: set -d 20130103
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
static sw_int_t adm_set_date(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;

	ret = seqs_change_date(opt->local_argv[2]);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Change date failed! [%s]", __FILE__, __LINE__, opt->local_argv[2]);
		printf("\n\tChange date failed\n");
		return SW_ERR;
	}

	printf("\n\tChange date success\n");

	printf("\n\tSaving Seqs to file......");
	ret = seqs_save(SEQS_SAVE_STAT_FIXED);
	if(SW_OK == ret)
		printf("Success!\n");
	else
		printf("Failure!\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_set_date_lock
 ** Desc : Set date of platform and lock
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : set -d [date] -lock
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
static sw_int_t adm_set_date_lock(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;

	ret = seqs_set_date_lock(opt->local_argv[2]);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Set date failed! [%s]", __FILE__, __LINE__, opt->local_argv[2]);
		printf("\n\tSet date failed\n");
		return SW_ERR;
	}

	printf("\n\tSet date success\n");

	printf("\n\tSaving Seqs to file......");
	ret = seqs_save(SEQS_SAVE_STAT_FIXED);
	if(SW_OK == ret)
		printf("Success!\n");
	else
		printf("Failure!\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_set_date
 ** Desc : Set date of platform
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : set -d -unlock
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
static sw_int_t adm_set_date_unlock(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	/* const sw_adm_opt_t *opt = cycle->opt; */

	ret = seqs_set_date_unlock();
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Unlock date failed!", __FILE__, __LINE__);
		printf("\n\tUnlock date failed\n");
		return SW_ERR;
	}

	printf("\n\tUnlock date success\n");
	printf("\n\tSaving Seqs to file......");
	ret = seqs_save(SEQS_SAVE_STAT_FIXED);
	if(SW_OK == ret)
		printf("Success!\n");
	else
		printf("Failure!\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_get_plat_flow
 ** Desc : 获取平台流水号
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : get -pf
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
static sw_int_t adm_get_plat_flow(sw_adm_cycle_t *cycle)
{
	return seqs_print_plat_flow();
}

/******************************************************************************
 ** Name : adm_get_bsn_flow
 ** Desc : 获取指定业务的流水号
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : get -bf [name]
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
static sw_int_t adm_get_bsn_flow(sw_adm_cycle_t *cycle)
{
	const sw_adm_opt_t *opt = cycle->opt;

	return seqs_print_bsn_flow(opt->local_argv[2]);
}

/******************************************************************************
 ** Name : adm_get_bsn_flow_all
 ** Desc : 获取所有业务的流水号
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : get -bf
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
static sw_int_t adm_get_bsn_flow_all(sw_adm_cycle_t *cycle)
{
	return seqs_print_bsn_flow_all();
}

/******************************************************************************
 ** Name : adm_get_date
 ** Desc : 获取平台日期
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : get -d/date
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
static sw_int_t adm_get_date(sw_adm_cycle_t *cycle)
{
	char date[DATE_MAX_LEN] = {0};
	sw_int_t date_is_locked = seqs_is_date_locked();

	seqs_get_date(date, sizeof(date));

	printf("\n\tDATE: %s  (%s)\n", date, 
			(date_is_locked ? "locked" : "unlock"));
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_goto_prdt
 ** Desc : Entry special product
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : goto [name]
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
sw_int_t adm_goto_prdt(sw_adm_cycle_t *cycle)
{
	bool bret = false;

	/* 1. Judge the product whether exist? */
	bret = uni_svr_prdt_is_exist(cycle->uni_svr, cycle->opt->local_argv[1]);
	if(false == bret)
	{
		printf("\n\tProduct [%s] isn't exist!\n\n", cycle->opt->local_argv[1]);
		pub_log_error("[%s][%d] Product [%s] isn't exist!",
				__FILE__, __LINE__, cycle->opt->local_argv[1]);
		return SW_ERR;
	}

	/* 2. Switch enverimont */
	adm_opt_set_submod_str(cycle->opt, cycle->opt->local_argv[1]);
	cycle->opt->mod_type = ADM_SUB_MOD_PRDT;

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_param_error
 ** Desc : Prompt that paramter is incorrect
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
static sw_int_t adm_param_error(sw_adm_cycle_t *cycle)
{
	printf("\n\tParamter is incorrect\n");
	return SW_OK;
}

static sw_int_t adm_unknown(sw_adm_cycle_t *cycle)
{
	printf("\n\tUnknown command\n");
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_start_all
 ** Desc : Start all process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start/s
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_all(sw_adm_cycle_t* cycle)
{
	sw_int_t ret = SW_ERR;

	ret = seqs_recover();
	if (ret == SW_OK)
		printf("\n\tRestore Seqs Success !\n");
	else
		printf("\n\tRestore Seqs Failure !\n");

	ret = uni_svr_start_all((const sw_cycle_t *)cycle, cycle->uni_svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start [%s] failed!",
				__FILE__, __LINE__, ADM_MOD_DFIS_STR);
		printf("\n\tStart [%s] failed.\n\n", ADM_MOD_DFIS_STR);
		return SW_ERR;
	}

	printf("\n\tSend command to start all success.\n");

	mtype_set_time();

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_start_lsn_all
 ** Desc : Start all listen process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -lsn/-l
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_lsn_all(sw_adm_cycle_t* cycle)
{
	sw_int_t ret = SW_ERR;

	ret = uni_svr_start_all_lsn( cycle->uni_svr);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Start all listen failed.", __FILE__, __LINE__);
		printf("\n\tStart all listen failed.\n");

		return SW_ERR;
	}

	printf("\n\tSend command to start all listen success.\n");

	return SW_OK;

}

/******************************************************************************
 ** Name : adm_start_lsn
 ** Desc : Start special listen process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start/s -l [lsn1] [lsn2] ...
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_lsn(sw_adm_cycle_t* cycle)
{
	sw_int_t ret=SW_ERR, idx=0;
	const sw_adm_opt_t *opt = cycle->opt;


	for(idx=2; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_start_lsn( cycle->uni_svr, opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start listen [%s] failed.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart listen [%s] failed.\n", opt->local_argv[idx]);

			return SW_ERR;
		}

		printf("\n\tSend command to start listen [%s] success.\n", opt->local_argv[idx]);   
	}

	return SW_OK;
}

/******************************************************************************
 *  ** Name : adm_start_lsn_in_prdt
 *  ** Desc : Stop special listen process
 *  ** Input: 
 *  **		cycle: Cycle of admin
 *  ** Output: NONE
 *  ** Return: 0: success !0: failed
 *  ** Process:
 *  ** Note : stop -l/-lsn [lsn1] [lsn2] ...
 *  ** Author: # Qifeng.zou # 2013.06.26 #
 *******************************************************************************/
SW_PROTECTED sw_int_t adm_start_lsn_in_prdt(sw_adm_cycle_t* cycle)
{
	sw_int_t idx = 0, ret = SW_ERR;
	sw_char_t	*lsn_name  = NULL;
	sw_char_t   *prdt_name = NULL;
	const sw_adm_opt_t *opt = cycle->opt;

	prdt_name = opt->local_argv[2];
	for(idx = 3; idx<opt->local_argc; idx++)
	{
		lsn_name = opt->local_argv[idx];
		ret = uni_svr_start_lsn_in_prdt(cycle->uni_svr, prdt_name, lsn_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start listen [%s] failed.",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\n\tStart listen [%s] failed\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		fprintf(stdout, "\n\tSend command to start listen [%s] success\n", opt->local_argv[idx]);
	}

	return SW_OK;
}
/******************************************************************************
 ** Name : adm_start_proc
 ** Desc : Start special process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start/s [proc1] [proc2] ...
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_proc(sw_adm_cycle_t* cycle)
{
	sw_int_t ret=SW_ERR, idx=0;
	const sw_adm_opt_t *opt = cycle->opt;


	for(idx=1; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_start_process(opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] uni_svr_start_process %s error.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart [%s] failed.\n", opt->local_argv[idx]);

			return SW_ERR;
		}

		printf("\n\tSend command to start [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;	
}

/******************************************************************************
 ** Name : adm_start_svc_all
 ** Desc : Start all svc process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start/s -svc/-s
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_svc_all(sw_adm_cycle_t* cycle)
{
	sw_int_t ret = 0;

	ret = uni_svr_start_all_svc( cycle->uni_svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all svc failed.", __FILE__, __LINE__);
		printf("\tStart all svc failed.\n");
		return SW_ERR;
	}

	printf("\tSend command to start all svc success.\n");
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_start_svc
 ** Desc : Start special svc process
 ** Input: 
 **		cycle: cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start -s/-svc [svc1] [svc2] ...
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_svc(sw_adm_cycle_t* cycle)
{
	sw_int_t ret=SW_ERR, idx=0;
	const sw_adm_opt_t *opt = cycle->opt;

	for(idx=2; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_start_svc(opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start svc [%s] failed.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart svc [%s] failed.\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		printf("\n\tSend command to start svc [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_start_svr_in_prdt
 ** Desc : Start special svr processes in special product
 ** Input: 
 **		cycle: cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start -s/-svc [svc1] [svc2] ...
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_start_svr_in_prdt(sw_adm_cycle_t* cycle)
{
	sw_int_t ret=SW_ERR, idx=0;
	const sw_adm_opt_t *opt = cycle->opt;

	if (opt->local_argc == 3)
	{
		printf("\tcommand parameter error, usage:swadmin stop -ps [prdtname][svrname_1][svrname_2]...\n");
        pub_log_error("[%s][%d] command parameter error", __FILE__, __LINE__);
        return SW_ERROR;
	}

	for(idx=3; idx < opt->local_argc; idx++)
	{
		ret = uni_svr_start_svr_in_prdt(cycle->uni_svr, opt->local_argv[2], opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start svc [%s] failed.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart svc [%s] failed.\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		printf("\n\tSend command to start svc [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}


SW_PROTECTED sw_int_t adm_start_prdt(sw_adm_cycle_t* cycle)
{
	sw_int_t	ret = 0;
	sw_int_t	idx = 0;
	const sw_adm_opt_t	*opt = cycle->opt;

	for (idx = 2; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_start_prdt(cycle->uni_svr, opt->local_argv[idx]);
		if (SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start prdt [%s] failed.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart prdt [%s] failed.\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		printf("\n\tSend command to start prdt [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_stop_all
 ** Desc : Stop all process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_stop_all(sw_adm_cycle_t* cycle)
{
	sw_int_t ret = SW_ERR;

	ret = uni_svr_stop_all((sw_cycle_t *)cycle, cycle->uni_svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop DFISBP failed.", __FILE__, __LINE__);
		printf("\n\tSend stop command failed.\n");

		return SW_ERR;
	}

	printf("\n\tSend command to stop all success.\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_stop_proc
 ** Desc : Stop special process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     Stop process one by one
 ** Note : stop [proc1] [porc2] ...
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_stop_proc(sw_adm_cycle_t* cycle)
{
	sw_int_t idx=0, ret=SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;


	for(idx=1; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_stop_process(cycle->uni_svr, SW_MSTOPSELF, opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop [%s] failed.",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\n\tStop [%s] failed.\n", opt->local_argv[idx]);

			return SW_ERR;
		}
		printf("\n\tSend command to stop [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK; 	 
}

/******************************************************************************
 ** Name : adm_stop_lsn_all
 ** Desc : Stop all listen process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -l/-lsn
 ** Modify: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_stop_lsn_all(sw_adm_cycle_t* cycle)
{
	sw_int_t ret = SW_OK;

	ret = uni_svr_stop_all_lsn( cycle->uni_svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] uni_svr_stop_all_lsn error.", __FILE__, __LINE__);
		printf("\tStop all listen failed.\n");

		return SW_ERR;
	}

	printf("\tSend command to stop all listen success.\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_stop_lsn
 ** Desc : Stop special listen process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -l/-lsn [lsn1] [lsn2] ...
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_stop_lsn(sw_adm_cycle_t* cycle)
{
	sw_int_t idx=0, ret = SW_ERR;
	sw_char_t	proc_name[64];
	const sw_adm_opt_t *opt = cycle->opt;

	for(idx=2; idx<opt->local_argc; idx++)
	{
		memset(proc_name, 0x0, sizeof(proc_name));
		if (strncmp(opt->local_argv[idx], PROC_NAME_LSN, strlen(PROC_NAME_LSN)) != 0)
		{
			sprintf(proc_name, "%s_%s", PROC_NAME_LSN, opt->local_argv[idx]);
		}
		else
		{
			strncpy(proc_name, opt->local_argv[idx], sizeof(proc_name) - 1);
		}
		ret = uni_svr_stop_process(cycle->uni_svr, SW_MSTOPONE, proc_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop listen [%s] failed.",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\n\tStop listen [%s] failed\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		fprintf(stdout, "\n\tSend command to stop listen [%s] success\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

SW_PROTECTED sw_int_t adm_stop_lsn_in_prdt(sw_adm_cycle_t* cycle)
{
	sw_int_t idx = 0, ret = SW_ERR;
	sw_char_t	*lsn_name  = NULL;
	sw_char_t   *prdt_name = NULL;
	const sw_adm_opt_t *opt = cycle->opt;
	prdt_name = opt->local_argv[2];
	for(idx=3; idx<opt->local_argc; idx++)
	{
		lsn_name = opt->local_argv[idx];
		ret = uni_svr_stop_lsn_in_prdt(cycle->uni_svr, prdt_name, lsn_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop listen [%s] failed.",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\n\tStop listen [%s] failed\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		fprintf(stdout, "\n\tSend command to stop listen [%s] success\n", opt->local_argv[idx]);
	}
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_stop_svc_all
 ** Desc : Stop all service process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -svc/-s
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_stop_svc_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;

	ret = uni_svr_stop_all_svc( cycle->uni_svr);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop all svc failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_stop_svc
 ** Desc : Stop special service process
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -svc/-s [svc1] [svc2] ...
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_stop_svc(sw_adm_cycle_t* cycle)
{
	sw_int_t idx=0, ret=SW_ERR;
	char	proc_name[128];
	const sw_adm_opt_t *opt = cycle->opt;

	/* stop -s ARGV2 ARGV3 ... */
	for(idx=2; idx<opt->local_argc; idx++)
	{
		memset(proc_name, 0x0, sizeof(proc_name));
		sprintf(proc_name, "%s_%s", PROC_NAME_SVC_MAN, opt->local_argv[idx]);
		ret = uni_svr_stop_process(cycle->uni_svr, SW_MSTOPSELF, proc_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop svc [%s] failed.",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\tStop svc [%s] failed.\n", opt->local_argv[idx]);

			return SW_ERR;
		}

		printf("\tSend command to stop svc [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

SW_PROTECTED sw_int_t adm_stop_prdt(sw_adm_cycle_t* cycle)
{
	sw_int_t	idx = 0;
	sw_int_t	ret = 0;
	const sw_adm_opt_t	*opt = cycle->opt;

	for (idx = 2; idx < opt->local_argc; idx++)
	{
		ret = uni_svr_stop_prdt(cycle->uni_svr, opt->local_argv[idx]);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Stop prdt [%s] failed!",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\tStop prdt [%s] failed!\n", opt->local_argv[idx]);
			return SW_ERROR;
		}
		printf("\tSend command to stop prdt [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/*停止某个产品下的指定的server*/
SW_PROTECTED sw_int_t adm_stop_svr_in_prdt(sw_adm_cycle_t* cycle)
{
	sw_int_t	idx = 0;
	sw_int_t	ret = 0;
	const sw_adm_opt_t	*opt = cycle->opt;

	if (opt->local_argc == 3)
	{
		printf("\tcommand parameter error, usage:swadmin stop -ps [prdtname][svrname_1][svrname_2]...\n");
		pub_log_error("[%s][%d] command parameter error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	else
	{
		/*停止指定产品下的部分server*/
		for (idx = 3; idx < opt->local_argc; idx++)
		{
			ret = uni_svr_stop_svr_in_prdt(cycle->uni_svr, opt->local_argv[2], opt->local_argv[idx]);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Stop server[%s] in prdt[%s]failed!",
						__FILE__, __LINE__, opt->local_argv[idx], opt->local_argv[2]);
				printf("\tStop server[%s] in prdt[%s] failed!\n", opt->local_argv[idx], opt->local_argv[2]);
				return SW_ERROR;
			}
			printf("\tSend command to stop server[%s] in prdt[%s] success.\n", opt->local_argv[idx], opt->local_argv[2]);
		}
	}

	return SW_OK;
}


/******************************************************************************
 ** Name : adm_list_all
 ** Desc : Print all process information
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls
 ** Author: # Qifeng.zou # 2013.06.21 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_all(sw_adm_cycle_t* cycle)
{
	uni_svr_print_all();
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_list_lsn_all
 ** Desc : List all listen process information
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -l
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_lsn_all(sw_adm_cycle_t* cycle)
{
	int count = 0;

	uni_svr_print_proc_head();

	count = uni_svr_print_sys( ND_LSN);

	uni_svr_print_tail(count);

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_list_prdt_all
 ** Desc : List all products information
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -p
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
sw_int_t adm_list_prdt_all(sw_adm_cycle_t* cycle)
{
	uni_svr_print_all_product( cycle->uni_svr);
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_list_version
 ** Desc : List version information
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -v
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
sw_int_t adm_list_version(sw_adm_cycle_t *cycle)
{
	printf("\n\tVersion: %s\n\n", VERS_NO);
	printf("\t@%s Copyright DHCC. All rights reserved.\n",VERS_DATE);
	return SW_OK;
}

sw_int_t adm_list_his_patch(sw_adm_cycle_t *cycle)
{
	sw_char_t cmd[128];
	sw_char_t name[128];

	memset(cmd, 0x00, sizeof(cmd));
	memset(name, 0x00, sizeof(name));

	sprintf(cmd, "%s/make/%s %s", getenv("SWHOME"), UPD_PACK, BASE_VERS);
	sprintf(name, "%s/make/%s", getenv("SWHOME"), UPD_PACK);

	if (access(name, F_OK) < 0)
	{
		printf("查看历史信息脚本不存在,请核对!\n");
		return SW_ERROR;
	}

	if (access(name, X_OK) < 0)
	{
		chmod(name, S_IRUSR|S_IWUSR|S_IXUSR|S_IXGRP|S_IXOTH);
	}

	system(cmd);
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_list_mtype_all
 ** Desc : 显示所有Mtype信息
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -m
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_mtype_all(sw_adm_cycle_t *cycle)
{
	return uni_svr_mtype_list_all();
}

/******************************************************************************
 ** Name : adm_list_mtype_busy
 ** Desc : 显示所有被占用Mtype信息
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -bm/-mb
 ** Author: # Qifeng.zou # 2013.08.22 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_mtype_busy(sw_adm_cycle_t *cycle)
{
	return uni_svr_mtype_list_busy();
}

/******************************************************************************
 ** Name : adm_list_mtype_free
 ** Desc : 显示所有未占用Mtype信息
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -fm/-mf
 ** Author: # Qifeng.zou # 2013.08.22 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_mtype_free(sw_adm_cycle_t *cycle)
{
	return uni_svr_mtype_list_free();
}

/******************************************************************************
 ** Name : adm_list_mtype
 ** Desc : 显示指定Mtype信息
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -m [mtype]
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_mtype(sw_adm_cycle_t *cycle)
{
	const sw_adm_opt_t *opt = cycle->opt;

	return uni_svr_mtype_list(atoi(opt->local_argv[2]), 1);
}

/******************************************************************************
 ** Name : adm_list_mtype_seg
 ** Desc : 显示指定Mtype段信息
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -m [mtype] [num]
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_mtype_seg(sw_adm_cycle_t *cycle)
{
	const sw_adm_opt_t *opt = cycle->opt;

	return uni_svr_mtype_list(atoi(opt->local_argv[2]), atoi(opt->local_argv[3]));
}

/******************************************************************************
 ** Name : adm_list_res_all
 ** Desc : 显示所有资源使用情况
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Display all SHM information
 **     2. Dispaly all MSQ information
 ** Note : list/ls -r
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_res_all(sw_adm_cycle_t *cycle)
{
	/* 1. Display all SHM */
	uni_svr_res_list_shm(cycle->uni_svr);

	/* 2. Display all MSQ */
	uni_svr_res_list_msq();

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_list_res_shm
 ** Desc : 显示共享内存使用情况
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -r shm
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_res_shm(sw_adm_cycle_t *cycle)
{
	return uni_svr_res_list_shm( cycle->uni_svr);
}

/******************************************************************************
 ** Name : adm_list_res_msq
 ** Desc : 显示消息队列使用情况
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -r msq
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_res_msq(sw_adm_cycle_t *cycle)
{
	return uni_svr_res_list_msq();
}

/******************************************************************************
 ** Name : adm_list_svc_all
 ** Desc : List all service process information
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -svc/-s
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_svc_all(sw_adm_cycle_t* cycle)
{
	int count = 0;

	uni_svr_print_proc_head();

	count = uni_svr_print_sys( ND_SVC);

	uni_svr_print_tail(count);

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_clean
 ** Desc : Clear svr information
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : clear
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_clear(sw_adm_cycle_t* cycle)
{
	system("clear");
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_clean
 ** Desc : Clean platform resource
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : clean
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_clean(sw_adm_cycle_t *cycle)
{
	return uni_svr_res_clean(cycle->base.syscfg->vars_shmid);
}


SW_PROTECTED sw_int_t adm_task_print_head()
{
	printf("\n\t%-6s %-16s %-16s %-8s %-8s %-8s\n",
			"No.", "NAME", "EXEC", "LTIME", "STAT", "TYPE");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	return SW_OK;
}

SW_PROTECTED sw_int_t adm_task_print_item(sw_job_item_t *job_item)
{
	time_t	tm = 0;
	sw_char_t	ltime[16];

	ltime[0] = '0';
	if (job_item->last_run_time > 0)
	{
		tm = 0;
		pub_time_change_time(&tm, ltime, 0);
		job_item->last_run_time = (long)tm;
	}

	printf("\n\t%-6s %-16s %-16s %-8s %-8s %-8s",
			job_item->no,job_item->job_name,job_item->exec
			, ltime, job_get_stat_desc(job_item->job_status)
			,job_get_type_desc(job_item->manual));

	return SW_OK;
}

SW_PROTECTED sw_int_t adm_task_print_tail(sw_int32_t all)
{
	printf("\n\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\n\t TOTLE:%-6d  \n",all);
	return SW_OK;
}

SW_PROTECTED sw_int_t adm_list_task_by_type(JOB_TYPE type)
{
	sw_int32_t idx,curr;
	sw_int_t ret;
	sw_job_head_t head;
	sw_job_item_t item;

	pub_mem_memzero(&head,sizeof(sw_job_head_t));
	pub_mem_memzero(&item,sizeof(sw_job_item_t));

	ret = uni_svr_task_get_head(&head);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get task head error!",__FILE__, __LINE__);
		printf("\n\t task list all failed.\n");
		return SW_ERROR;
	}
	printf("[%s][%d] cur_cnt=[%d]\n", __FILE__, __LINE__, head.cur_cnt);

	adm_task_print_head();

	curr = 0;
	for (idx = 0; idx < head.cur_cnt ; idx ++)
	{
		ret = uni_svr_task_get_item_by_idx(idx,&item);
		if (ret != SW_OK)
		{
			printf("\n\t task list failed.\n");
			break;
		}
		if (type == JOB_ALL || item.manual == type)
		{
			adm_task_print_item(&item);
			curr++;
		}
	}
	adm_task_print_tail(curr);

	return SW_OK;
}	
SW_PROTECTED sw_int_t adm_list_manual_task(sw_adm_cycle_t* cycle)
{
	return adm_list_task_by_type(JOB_MANUAL);
}

SW_PROTECTED sw_int_t adm_list_auto_task(sw_adm_cycle_t* cycle)
{
	return adm_list_task_by_type(JOB_AUTO);
}

SW_PROTECTED sw_int_t adm_list_task_s(sw_adm_cycle_t* cycle)
{
	sw_int32_t	ret = 0, idx = 0, curr = 0;
	const sw_adm_opt_t *opt = cycle->opt;
	sw_job_item_t item;

	pub_mem_memzero(&item,sizeof(sw_job_item_t));

	adm_task_print_head();

	for(idx = 2; idx <opt->local_argc ;idx++)
	{
		ret = uni_svr_task_get_item_by_no(opt->local_argv[idx],&item);
		if (ret != SW_OK)
		{
			printf("\n\t task [%s] not fond.\n" ,opt->local_argv[idx]);
			continue;
		}

		adm_task_print_item(&item);
		curr++;
	}
	adm_task_print_tail(curr);
	return SW_OK;
}

SW_PROTECTED sw_int_t adm_list_all_task(sw_adm_cycle_t* cycle)
{
	return adm_list_task_by_type(JOB_ALL);
}

SW_PROTECTED sw_int_t adm_start_all_task(sw_adm_cycle_t* cycle)
{
	sw_int32_t idx;
	sw_int_t ret;
	sw_job_head_t head;
	sw_job_item_t item;

	pub_mem_memzero(&head,sizeof(sw_job_head_t));
	pub_mem_memzero(&item,sizeof(sw_job_item_t));

	ret = uni_svr_task_get_head(&head);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get task head error!",__FILE__, __LINE__);
		printf("\n\t task list all failed.\n");
		return SW_ERROR;
	}

	for (idx = 0; idx < head.cur_cnt ; idx ++)
	{
		ret = uni_svr_task_get_item_by_idx(idx,&item);
		if (ret != SW_OK)
		{
			printf("\n\t task get failed.\n");
			break;
		}
		if (item.manual == JOB_MANUAL)
		{
			ret = uni_svr_task_start_item(cycle->uni_svr,item.no);
			if (ret != SW_OK)
			{
				printf("\n\t start task [%s] fail.\n" ,item.no);
			}
			else
			{
				printf("\n\t start task [%s] success.\n" ,item.no);
			}
		}
	}

	return SW_OK;	 
}
SW_PROTECTED sw_int_t adm_start_task_s(sw_adm_cycle_t* cycle)
{
	sw_int32_t ret,idx;
	const sw_adm_opt_t *opt = cycle->opt;
	sw_job_item_t item;

	pub_mem_memzero(&item,sizeof(sw_job_item_t));

	for(idx = 2; idx <opt->local_argc ;idx++)
	{
		ret = uni_svr_task_get_item_by_no(opt->local_argv[idx],&item);
		if (ret != SW_OK)
		{
			printf("\n\t task [%s] not fond.\n" ,opt->local_argv[idx]);
			continue;
		}

		ret = uni_svr_task_start_item(cycle->uni_svr,item.no);
		if (ret != SW_OK)
		{
			printf("\n\t start task [%s] fail.\n" ,item.no);
		}
		else
		{
			printf("\n\t start task [%s] success.\n" ,item.no);
		}
	}

	return SW_OK;	 
}



/******************************************************************************
 ** Name : adm_prdt_start_all
 ** Desc : Start all svc of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_start_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;


	ret = uni_svr_prdt_start_all(cycle->uni_svr, opt->mod_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all failed.", __FILE__, __LINE__);
		printf("\n\tStart all failed.\n");

		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_start_lsn_all
 ** Desc : Start all lsn of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start -l/-lsn
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_start_lsn_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;


	ret = uni_svr_prdt_start_all_lsn( cycle->uni_svr, opt->mod_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all listen failed.", __FILE__, __LINE__);
		printf("\n\tStart all listen failed.\n");

		return SW_ERR;
	}

	printf("\n\tStart all listen success.\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_start_lsn
 ** Desc : Start special lsn of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start -lsn/-l [lsn1] [lsn2] ...
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_start_lsn(sw_adm_cycle_t *cycle)
{
	sw_int_t ret=SW_ERR, idx=0;
	const sw_adm_opt_t *opt = cycle->opt;

	/* 2. Start listen one by one */
	for(idx=2; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_prdt_start_lsn(cycle->uni_svr, opt->mod_name, opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start listen [%s] failed.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart listen [%s] failed.\n", opt->local_argv[idx]);

			return SW_ERR;
		}
		printf("\n\tStart listen [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_start_svc_all
 ** Desc : Start all svc of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start -svc/-s
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_start_svc_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = 0;
	const sw_adm_opt_t *opt = cycle->opt;

	ret = uni_svr_prdt_start_all_svc(cycle->uni_svr, opt->mod_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all svc failed.", __FILE__, __LINE__);
		printf("\n\tStart all svc failed.\n");
		return SW_ERR;
	}

	printf("\n\tStart all svc success.\n");
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_start_svc
 ** Desc : Start special svc of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : start -svc/-s [svc1] [svc2] ...
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_start_svc(sw_adm_cycle_t *cycle)
{
	sw_int_t ret=SW_ERR, idx=0;
	const sw_adm_opt_t *opt = cycle->opt;


	for(idx=2; idx<opt->local_argc; idx++)
	{
		if(0 != strcasecmp(opt->mod_name, opt->local_argv[idx]))
		{
			printf("\n\tSvc [%s] incorrect!\n", opt->local_argv[idx]);
			return SW_ERR;
		}

		ret = uni_svr_start_svc(opt->mod_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start svc [%s] failed.", 
					__FILE__, __LINE__,  opt->local_argv[idx]);
			printf("\n\tStart svc [%s] failed.\n", opt->local_argv[idx]);
			return SW_ERR;
		}
		printf("\n\tStart svc [%s] failed.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_stop_all
 ** Desc : Start all process of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_stop_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;

	ret = uni_svr_prdt_stop_all(cycle->uni_svr, opt->mod_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop [%s] failed.",
				__FILE__, __LINE__, opt->mod_name);
		printf("\n\tStop all failed.\n");

		return SW_ERR;
	}

	printf("\n\tStop all success.\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_stop_lsn_all
 ** Desc : Stop all lsn of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -lsn/-l
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_stop_lsn_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_OK;
	const sw_adm_opt_t *opt = cycle->opt;


	ret = uni_svr_prdt_stop_all_lsn(cycle->uni_svr, opt->mod_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop all listen failed.", __FILE__, __LINE__);
		printf("\tStop all listen failed.\n");

		return SW_ERR;
	}

	printf("\n\tStop all listen success.\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_stop_lsn
 ** Desc : Stop special lsn of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -lsn/-l [lsn1] [lsn2] ...
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_stop_lsn(sw_adm_cycle_t *cycle)
{
	sw_int_t idx=0, ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;

	for(idx=2; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_prdt_stop_lsn(cycle->uni_svr, opt->mod_name, opt->local_argv[idx]);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop listen [%s] failed!",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\t\nStop listen [%s] failed.\n", opt->local_argv[idx]);
			return SW_ERR;
		}

		fprintf(stdout, "\n\tStop listen [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_stop_svc_all
 ** Desc : Stop all svc of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -svc/-s
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_stop_svc_all(sw_adm_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;


	/* 2. Stop all svc of special product */
	ret = uni_svr_prdt_stop_all_svc( cycle->uni_svr, opt->mod_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all svc of [%s] failed.",
				__FILE__, __LINE__, opt->mod_name);
		return SW_ERR;
	}

	printf("\n\tStop svc success.\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_stop_svc
 ** Desc : Stop special svc of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : stop -svc/-s [svc1] [svc2] ...
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_stop_svc(sw_adm_cycle_t *cycle)
{
	sw_int_t idx=0, ret = SW_ERR;
	const sw_adm_opt_t *opt = cycle->opt;


	for(idx=2; idx<opt->local_argc; idx++)
	{
		ret = uni_svr_prdt_stop_svc(cycle->uni_svr, opt->mod_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop svc [%s] failed.",
					__FILE__, __LINE__, opt->local_argv[idx]);
			printf("\tStop svc [%s] failed.\n", opt->local_argv[idx]);
			return SW_ERR;
		}

		printf("\n\tStop svc [%s] success.\n", opt->local_argv[idx]);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_list_all_in_prdt
 ** Desc : List all process of all products
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_list_in_all_prdt(sw_adm_cycle_t *cycle)
{
	return uni_svr_print_in_all_prdt(cycle->uni_svr);
}

/******************************************************************************
 ** Name : adm_prdt_list_all
 ** Desc : List all process of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_list_all(sw_adm_cycle_t *cycle)
{
	sw_adm_opt_t *opt = cycle->opt;

	return uni_svr_prdt_print_all(cycle->uni_svr, opt->mod_name);
}

/******************************************************************************
 ** Name : adm_prdt_list_lsn_all
 ** Desc : List all lsn of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -lsn/-l
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_list_lsn_all(sw_adm_cycle_t *cycle)
{
	int count = 0;
	const sw_adm_opt_t *opt = cycle->opt;

	uni_svr_print_proc_head();

	count = uni_svr_prdt_print_all_lsn(
			cycle->uni_svr, opt->mod_name);

	uni_svr_print_tail(count);

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_list_svc_all
 ** Desc : List all svc of special product
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -svc/-s
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_list_svc_all(sw_adm_cycle_t *cycle)
{
	int count = 0;
	const sw_adm_opt_t *opt = cycle->opt;

	uni_svr_print_proc_head();

	count = uni_svr_prdt_print_all_svc(
			cycle->uni_svr, opt->mod_name);

	uni_svr_print_tail(count);

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_list_mtype_all
 ** Desc : List all mtype of special product.
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : list/ls -m
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_list_mtype_all(sw_adm_cycle_t *cycle)
{
	bool bret = 0;
	sw_int_t idx = 0, total = 0;
	const sw_mtype_t *addr = mtype_get_addr();
	const sw_mtype_head_t *head = &addr->head;
	const sw_mtype_node_t *item = &addr->first;
	const sw_adm_opt_t *opt = cycle->opt;

	uni_svr_mtype_print_head();

	sem_mutex_lock(head->lock_id);

	for(idx=0; idx<head->mtype_max; idx++)
	{
		if(0 == strcmp(opt->mod_name, item->prdt_name))
		{
			bret = uni_svr_mtype_print_item(idx+1, item, MTYPE_ALL);
			if(bret)
			{
				total++;
			}
		}
		item++;
	}

	sem_mutex_unlock(head->lock_id);

	uni_svr_mtype_print_tail(total);
	return SW_OK;
}

/******************************************************************************
 ** Name : adm_prdt_list_child_all
 ** Desc : List all children process of special product.
 ** Input: 
 **		cycle: Cycle of admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. List all child of listen process
 **     2. List all child of service process
 ** Note : list/ls -c
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_prdt_list_child_all(sw_adm_cycle_t *cycle)
{
	sw_int_t total = 0;
	const sw_adm_opt_t *opt = cycle->opt;

	uni_svr_print_proc_head();

	/* 1. List all listen process */
	total = uni_svr_prdt_print_lsn_child(&cycle->base, opt->mod_name);

	/* 2. List all service process */
	total += uni_svr_prdt_print_svc_child(cycle->uni_svr,opt->mod_name);

	uni_svr_print_tail(total);

	return SW_OK;
}


/* 节点类型描述信息 */
static const node_type_desc_t g_node_type_desc[] = 
{
	{ND_LSN, "LSN"}
	, {ND_SVC, "SVC"}
	, {ND_TASK, "TSK"}
	, {ND_SVR, "SVR"}
	, {ND_DO, "DO"}
	, {ND_SAF, "SAF"}
	, {ND_LOG, "LOG"}
	, {ND_POL, "POL"}
	, {ND_RES, "RES"}
	, {ND_JOB, "JOB"}
	, {ND_ADM, "ADM"}
	, {ND_ALERT, "ALERT"}
	, {ND_UNKNOWN, "UNKNOWN"}
};

/* 节点类型描述信息 */
static const route_status_desc_t g_route_status_desc[] = 
{
	{0, "DOING"}
	, {1, "SUCCESS"}
	, {2, "ABNORMAL"}
};

/******************************************************************************
 ** Name : adm_get_node_type_desc
 ** Desc : Get description of node type
 ** Input: 
 **     type: The type of node. Range: ND_LSN ~ ND_XXX
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
const char *adm_get_node_type_desc(int type)
{
	sw_int_t idx=0, size=0;

	size = sizeof(g_node_type_desc)/sizeof(node_type_desc_t);
	for(idx=0; idx<size; idx++)
	{
		if(g_node_type_desc[idx].type == type)
		{
			return g_node_type_desc[idx].desc;
		}
	}

	return g_node_type_desc[size-1].desc;
}

/******************************************************************************
 ** Name : adm_get_route_status_desc
 ** Desc : Get description of route status
 ** Input: 
 **     status: route status. Range: 0-Doing 1:Success 2:Abnormal
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
const char *adm_get_route_status_desc(int status)
{    
	sw_int_t size = 0;

	size = sizeof(g_route_status_desc)/sizeof(route_status_desc_t);

	if(status >= size)
	{
		return NULL;
	}

	return g_route_status_desc[status].desc;
}

/******************************************************************************
 ** Name : adm_top_head
 ** Desc : Dispaly the head format
 ** Input: NONE
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
void adm_top_head(void)
{
	printf("\n\tNO.\tTRCNO\t  STIME\t\t  ETIME\t\t  STATUS\n");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
}

/******************************************************************************
 ** Name : adm_top_tail
 ** Desc : Dispaly the tail format
 ** Input: NONE
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
void adm_top_tail(sw_int32_t total)
{
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d          STATUS: %s\n", total, seqs_get_syssts_desc());
}

/******************************************************************************
 ** Name : amd_top_display
 ** Desc : Display top result
 ** Input: 
 **     list: the list of result
 **     num: number of list members
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
void amd_top_display(const sw_trace_item_t **list, sw_int32_t num)
{
	sw_int32_t idx=0, total=0;
	char stime[TIME_MAX_LEN] = {0},
	     etime[TIME_MAX_LEN] = {0};

	adm_top_head();

	for(idx=0; idx<num; idx++)
	{
		if((NULL != list[idx]) && (list[idx]->trace_no > 0))
		{
			total++;

			memset(stime, 0, sizeof(stime));
			memset(etime, 0, sizeof(etime));

			pub_change_time2(list[idx]->start_time, stime, 0);
			pub_change_time2(list[idx]->end_time, etime, 0);

			printf("\t[%04d]\t%-12lld%-16s%-16s", idx + 1, (long long)(list[idx]->trace_no), stime, etime);
			if(0 == list[idx]->flag)
			{
				printf("DOING\n");
			}
			else
			{
				printf("DONE\n");
			}
			continue;
		}
		break;
	}

	adm_top_tail(total);
}

/******************************************************************************
 ** Name : adm_top
 ** Desc : Execute top command
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_top(sw_adm_cycle_t* cycle)
{
	sw_int32_t ret=SW_ERR, idx=0, num=0;
	const sw_adm_opt_t *opt = cycle->opt;
	const sw_trace_item_t **list = NULL;
	const sw_syscfg_t *syscfg = &cycle->trace->syscfg;


	/* 1. The number of trace */
	if(1 == opt->local_argc)
	{
		num = ADM_TOP_DEF_NUM;
	}
	else
	{
		num = atoi(opt->local_argv[1]);
		if(num > syscfg->session_max)
		{
			num = syscfg->session_max;
		}
	}
	if (g_mtype_in_ares)
	{
		if (ares_top(num) < 0)
		{
			pub_log_error("[%s][%d] Ares get top [%d] trace error!",
					__FILE__, __LINE__, num);
			return SW_ERROR;
		}

		return SW_OK;
	}

	/* 2. Create pointer array */
	list = (const sw_trace_item_t **)calloc(num, sizeof(sw_trace_item_t *));
	if(NULL == list)
	{
		printf("\n\tAlloc memory failed!\n");
		pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	for(idx=0; idx<num; idx++)
	{
		list[idx] = NULL;
	}

	/* 3. Get top trace information */
	ret = uni_trace_get_top(cycle->trace, list, num);
	if(SW_OK != ret)
	{
		printf("\n\tSort failed!\n");
		pub_log_error("[%s][%d] Trace sort failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 4. Display trace list */
	amd_top_display(list, num);

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_step
 ** Desc : Display special trace detail information
 ** Input: 
 **     cype: cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_step(sw_adm_cycle_t* cycle)
{
	sw_int_t idx=0;
	sw_int64_t trace_no=0;
	const sw_trace_item_t *item = NULL;
	const sw_trace_t *route = NULL;
	const sw_adm_opt_t *opt = cycle->opt;
	char stime[TIME_MAX_LEN] = {0},
	     etime[TIME_MAX_LEN] = {0};

	/*TODO: BUG */
	trace_no = atoll(opt->local_argv[1]);
	if (g_mtype_in_ares)
	{
		if (ares_step(trace_no) < 0)
		{
			pub_log_error("[%s][%d] Ares get trace[%lld]'s step info error!",
					__FILE__, __LINE__, trace_no);
			return SW_ERROR;
		}
		return SW_OK;
	}

	item = uni_trace_item_search(cycle->trace, trace_no);
	if(NULL == item)
	{
		printf("\n\tTrace [%lld] isn't exist!\n\n",(long long) trace_no);
		return SW_ERR;
	}

	pub_change_time2(item->start_time, stime, 0);
	pub_change_time2(item->end_time, etime, 0);

	printf("\n\tTRCNO\t  STIME\t\t  ETIME\t\t  STATUS\n");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\t%-12lld%-16s%-16s", (long long)item->trace_no, stime, etime);
	if(0 == item->flag)
	{
		printf("DOING\n");
	}
	else
	{
		printf("DONE\n");
	}

	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n\n");
	printf("\tSTEP\tMODULE\t\t\t TYPE\tSTIME\t\tETIME\t\tSTATUS\tMSG\n");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	/* Display route information */
	route = item->route;
	for(idx=0; idx<MAXROUTE; idx++)
	{
		if(!route->use)
		{
			break;
		}
		pub_change_time2(route->start_time, stime, 0);
		pub_change_time2(route->end_time, etime, 0);

		printf("\t[%03d]\t%-20s\t%8s\t%-16s%-16s%d\t%s\n",
				(int)idx + 1, route->node, adm_get_node_type_desc(route->module_type),
				stime, etime, route->status, adm_get_route_status_desc(route->status));
		route++;
	}

	if((MAXROUTE == idx) 
			&& (item->route[MAXROUTE-1].next > 0))
	{
		route = trace_get_exp_route(item->route[MAXROUTE-1].next);
		while(NULL != route)
		{
			pub_change_time2(route->start_time, stime, 0);
			pub_change_time2(route->end_time, etime, 0);

			printf("\t[%03d]\t%-20s\t%8s\t%-16s%-16s%d\t%s\n",
					(int)idx + 1, route->node, adm_get_node_type_desc(route->module_type),
					stime, etime, route->status, adm_get_route_status_desc(route->status));

			if(route->next >= 0)
			{
				break;
			}
			idx++;
			route = trace_get_exp_route(route->next);            
		}
	}
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d          STATUS: %s\n", (int)idx, seqs_get_syssts_desc());

	return SW_OK;
}


/******************************************************************************
 ** Name : adm_saf_list
 ** Desc :  list saf item info
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: zhanghailu
 ******************************************************************************/
static sw_int_t adm_saf_list(sw_adm_cycle_t *cycle)
{
	return SW_OK;
}
/******************************************************************************
 ** Name : adm_saf_start
 ** Desc :  start saf item info
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: zhanghailu
 ******************************************************************************/
static sw_int_t adm_saf_start(sw_adm_cycle_t *cycle)
{
	sw_int32_t ret = 0;
	const sw_adm_opt_t *opt = cycle->opt;
	sw_cmd_t cmd;
	sw_uni_svr_t *svr = cycle->uni_svr;
	/*
	   ret = uni_svr_proc_is_exist(PROC_NAME_SAFD);
	   if(true != ret)
	   {
	   pub_log_error( "[%s][%d] %s is not running.", __FILE__, __LINE__, PROC_NAME_SAFD);
	   return SW_ERR;
	   }
	 */

	memset(&cmd, 0, sizeof(cmd));
	cmd.type = SW_MSTARTTASK;

	cmd.trace_no = atoll(opt->local_argv[2]);
	strcpy(cmd.sys_date, opt->local_argv[3]);
	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);

	ret =  udp_send(svr->cmdfd, (sw_char_t*)&cmd, sizeof(cmd), PROC_NAME_SAFD);
	if(ret < 0)
	{
		pub_log_error( "[%s][%d] Send command to [%s]. errmsg:[%d]%s!",
				__FILE__, __LINE__, PROC_NAME_SAFD, errno, strerror(errno));
		return SW_ERR;
	}

	memset(&cmd, 0, sizeof(cmd));
	printf(" send [saf -r] cmd to swsafd success! \n");

	/* Receive reply */
	ret = udp_recv(svr->cmdfd, (sw_char_t *)&cmd, sizeof(cmd));
	if(ret <= 0)
	{
		pub_log_error("[%s][%d] Receive reply failed! ret:%d dst:[%s] errmsg:[%d]%s!",
				__FILE__, __LINE__, ret, PROC_NAME_SAFD, errno, strerror(errno));
		return SW_ERR;
	}

	/* Check reply */
	switch(cmd.type)
	{
		case SW_RESCMD:
			{
				pub_log_info("[%s][%d] Send command to [%s] success!", __FILE__, __LINE__, PROC_NAME_SAFD);
				return SW_OK;
			}
		case SW_ERRCMD:
			{
				pub_log_error(
						"[%s][%d] Send command to [%s] failed! errmsg:[%d]%s!",
						__FILE__, __LINE__, PROC_NAME_SAFD, errno, strerror(errno));
				return SW_ERR;
			}
		default:
			{
				pub_log_info( "[%s][%d] Send command to [%s] unknown!",
						__FILE__, __LINE__, PROC_NAME_SAFD);
				return SW_ERR;
			}
	}

	return SW_OK;
}


/******************************************************************************
 ** Name : adm_saf_stop
 ** Desc :  stop saf item info
 ** Input: 
 **     cycle: Cycle of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: zhanghailu
 ******************************************************************************/
static sw_int_t adm_saf_stop(sw_adm_cycle_t *cycle)
{
	sw_int32_t ret;
	const sw_adm_opt_t *opt = cycle->opt;
	sw_cmd_t cmd;
	sw_uni_svr_t *svr = cycle->uni_svr;
	/*
	   ret = uni_svr_proc_is_exist(PROC_NAME_SAFD);
	   if(true != ret)
	   {
	   pub_log_error( "[%s][%d] %s is not running.", __FILE__, __LINE__, PROC_NAME_SAFD);
	   return SW_ERR;
	   }
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.type = SW_MSTOPTASK;

	cmd.trace_no = atoll(opt->local_argv[2]);
	strcpy(cmd.sys_date, opt->local_argv[3]);
	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);

	ret =  udp_send(svr->cmdfd, (sw_char_t*)&cmd, sizeof(cmd), PROC_NAME_SAFD);
	if(ret < 0)
	{
		pub_log_error( "[%s][%d] Send command to [%s]. errmsg:[%d]%s!",
				__FILE__, __LINE__, PROC_NAME_SAFD, errno, strerror(errno));
		return SW_ERR;
	}

	memset(&cmd, 0, sizeof(cmd));
	printf(" send [saf -e] cmd to swsafd success! \n");

	/* Receive reply */
	ret = udp_recv(svr->cmdfd, (sw_char_t *)&cmd, sizeof(cmd));
	if(ret <= 0)
	{
		pub_log_error(
				"[%s][%d] Receive reply failed! ret:%d dst:[%s] errmsg:[%d]%s!",
				__FILE__, __LINE__, ret, PROC_NAME_SAFD, errno, strerror(errno));
		return SW_ERR;
	}

	/* Check reply */
	switch(cmd.type)
	{
		case SW_RESCMD:
			{
				pub_log_info(
						"[%s][%d] Send command to [%s] success!", __FILE__, __LINE__, PROC_NAME_SAFD);
				return SW_OK;
			}
		case SW_ERRCMD:
			{
				pub_log_error(
						"[%s][%d] Send command to [%s] failed! errmsg:[%d]%s!",
						__FILE__, __LINE__, PROC_NAME_SAFD, errno, strerror(errno));
				return SW_ERR;
			}
		default:
			{
				pub_log_info( "[%s][%d] Send command to [%s] unknown!",
						__FILE__, __LINE__, PROC_NAME_SAFD);
				return SW_ERR;
			}
	}


	return SW_OK;
}


SW_PROTECTED sw_int_t adm_deploy_bp(sw_adm_cycle_t *cycle)
{
	const sw_adm_opt_t *opt = cycle->opt;
	const char *filepath = NULL; 

	if (opt->local_argc == 2)
	{
		filepath = strstr(opt->local_argv[1], "-file=");
		if (filepath != NULL)
		{
			/* should be compile-time calculation */
			filepath += strlen("-file="); 
		}
	}

	if (filepath == NULL || *filepath == 0)
	{
		printf("Usage: deploy -file=PATH\n");
		return SW_ERR;
	}

	if ( 0 == uni_svr_deploy_bp_by_cfgfile(cycle->uni_svr, filepath))
		puts("done"); /* not alawys success, just 'done' */
	else
		printf("config file is not corret");

	/* I think it's resonable to return OK here, not above within if-else. */
	return SW_OK;
}

SW_PROTECTED sw_int_t  adm_reload(sw_adm_cycle_t *cycle)
{
	int argc = cycle->opt->local_argc;
	char **argv = cycle->opt->local_argv;
	sw_uni_svr_t *svr = cycle->uni_svr;

	if (argc < 1)
		return SW_ERROR;

	if (0 != strcmp(argv[0], "reload"))
		return -1;
	if (NULL == argv[1])
		return show_reload_usage(true, true);

	if (0 == strcmp(argv[1], "-lsn"))
		return adm_reload_lsn(svr, argc-1, &argv[1]);
	else if (0 == strcmp(argv[1], "-prdt"))
		return adm_reload_prdt(svr, argc-1, &argv[1]);
	else
		return show_reload_usage(true, true);
	puts("reload");
	return SW_OK;
}

static int show_reload_usage(bool show_lsn, bool show_prdt)
{
#define LSN_USAGE \
	"reload -lsn  LSNNAME {add|upd|del}\n"\
	"reload -lsn  LSNNAME [upd] -prdt PRDTNAME {add|upd|del}\n"

#define PRDT_USAGE \
	"reload -prdt PRDTNAME {add|del}\n"\
	"reload -prdt PRDTNAME upd -svr SVRNAME {add|upd|del}\n"\
	"reload -prdt PRDTNAME upd -svr SVRNAME [upd] -svc SVCNAME {add|upd|del}\n"\
	"reload -prdt PRDTNAME arg ARGPATH\n"
	if (show_lsn)
		printf(LSN_USAGE);
	if (show_prdt)
		printf(PRDT_USAGE);

	/* generally, this function will be called when error happends
	 * (fprintf to stderr)  * */
	return -1;

#undef LSN_USAGE
#undef PRDT_USAGE
}

static int adm_reload_lsn(
		sw_uni_svr_t *svr,
		int argc, 
		char **argv)
{
	assert(0 == strcmp(argv[0], "-lsn"));

	const char *lsn_name = NULL;
	const char *lsn_action = NULL;
	const char *prdt_name = NULL;
	const char *prdt_action = NULL;

	if (argc == 3) 
		/* -lsn LSNNAME {add|upd|del} */
	{ 
		lsn_name = argv[1];
		lsn_action = argv[2];
	}
	else if (argc == 5 && 0 == strcmp(argv[2], "-prdt")) 
		/* -lsn LSNNAME -prdt PRDTNAME {add|upd|del} */
	{ 
		lsn_name = argv[1];
		lsn_action = NULL; /* make it clear */
		prdt_name = argv[3];
		prdt_action = argv[4];
	}
	else if (argc == 6 
			&& 0 == strcmp(argv[2], "upd")
			&& 0 == strcmp(argv[3], "-prdt"))
		/* -lsn LSNNAME upd -prdt PRDTNAME {add|upd|del} */ 
	{ 
		lsn_name = argv[1];
		lsn_action = argv[2];
		prdt_name = argv[4];
		prdt_action = argv[5];
	}
	else
		return show_reload_usage(true, false);

	int rc = uni_svr_reload_lsn(svr, 
			lsn_name, lsn_action,
			prdt_name, prdt_action);
	if (rc == -2)
		show_reload_usage(true, false);
	return rc;
}


static int adm_reload_prdt(
		sw_uni_svr_t *svr,
		int argc, 
		char **argv)
{
	assert(0 == strcmp(argv[0], "-prdt"));

	const char *prdt_name = NULL;
	const char *prdt_action = NULL;
	const char *svr_name = NULL;
	const char *svr_action = NULL;
	const char *svc_name = NULL;
	const char *svc_action = NULL;

	if (argc == 3)
		/* -prdt PRDTNAME {add|del} */
	{
		prdt_name = argv[1];
		prdt_action = argv[2];
	}
	else if (argc == 4) 
		/*-prdt PRDTNAME arg ARGPATH */
	{
		prdt_name = argv[1];
		prdt_action = argv[2];
		svr_action = argv[3];
	}
	else if (argc == 6 
			&& 0 == strcmp(argv[2], "upd")
			&& 0 == strcmp(argv[3], "-svr"))
		/* -prdt PRDTNAME upd -svr SVRNAME {add|upd|del} */
	{
		prdt_name = argv[1];
		prdt_action = argv[2];
		svr_name = argv[4];
		svr_action = argv[5];
	}
	else if (argc == 8
			&& 0 == strcmp(argv[2], "upd")
			&& 0 == strcmp(argv[3], "-svr")
			&& 0 == strcmp(argv[5], "-svc"))
		/* -prdt PRDTNAME upd -svr SVRNAME -svc SVCNAME {add|upd|del} */
	{
		prdt_name = argv[1];
		prdt_action = argv[2];
		svr_name = argv[4];
		svc_name = argv[6];
		svc_action = argv[7];
	}
	else if (argc == 9
			&& 0 == strcmp(argv[2], "upd")
			&& 0 == strcmp(argv[3], "-svr")
			&& 0 == strcmp(argv[5], "upd")
			&& 0 == strcmp(argv[6], "-svc"))
		/* -prdt NAME upd -svr NAME upd -svc NAME ACT */
	{
		prdt_name = argv[1];
		prdt_action = argv[2];
		svr_name = argv[4];
		svr_action = argv[5];
		svc_name = argv[7];
		svc_action = argv[8];
	}
	else
		return show_reload_usage(false, true);

	int rc = uni_svr_reload_prdt(svr,
			prdt_name, prdt_action,
			svr_name, svr_action,
			svc_name, svc_action);
	if (rc == -2)
		show_reload_usage(false, true);
	return rc;
}

SW_PROTECTED sw_int_t adm_sys_startup(sw_adm_cycle_t *cycle)
{
	sw_int32_t	ret = 0;

	ret = seqs_sys_start();
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] startup bp faild!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

SW_PROTECTED sw_int_t adm_sys_shutdown(sw_adm_cycle_t *cycle)
{
	sw_int32_t	ret = 0;

	ret = seqs_sys_stop();
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] shutdown bp faild!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

SW_PROTECTED sw_int_t adm_get_log(sw_adm_cycle_t *cycle)
{
	sw_int32_t ret = 0;
	const sw_adm_opt_t *opt = cycle->opt;
	sw_char_t *traceno  = opt->local_argv[2];
	sw_char_t *date     = opt->local_argv[3];
	sw_char_t *lsn_name = opt->local_argv[4];
	ret = uni_svr_get_log(cycle->uni_svr, traceno, date, lsn_name);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] get log error, traceno=[%s] date=[%s] lsn_name=[%s]", 
						__FILE__, __LINE__, traceno, date, lsn_name);
		printf("get log which traceno=[%s] date=[%s] lsn_name=[%s] error\n", traceno, date, lsn_name);
		return SW_ERROR;
	}
	else if (ret == -2)
	{
		printf("not find log, traceno=[%s] date=[%s] lsn_name=[%s]\n", traceno, date, lsn_name);
	}
	else if (ret == -3)
	{
		printf("%s isn't running\n", PROC_NAME_LOG);
	}
	else
	{
		printf("get log success, trace=[%s] date=[%s] lsn_name=[%s]\n", 
				traceno, date, lsn_name);
	}
	return SW_OK;
}
