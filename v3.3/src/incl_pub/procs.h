/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-07
 *** module  : Interface for process info register and check. 
 *** name    : procs.h
 *** function: procs sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __PROCS_H__
#define __PROCS_H__
#include "pub_type.h"
#include "pub_cfg.h"

#define MAXERR		10

/*for process restart*/
#define RESTART_INITED	9999
#define ALWAYS_RESTART  0
#define NEVER_RESTART   2
#define LIMITED_RESTART 3
#define MAX_RESTART_CNT	3

#define PROC_S_BUSYING	1
#define PROC_S_IDLE	2

struct sw_proc_info_s
{
	sw_char_t  name[SW_NAME_LEN];	/* process name */
	sw_char_t  svr_name[NAME_LEN];	/* svr grp name */
	sw_char_t  prdt_name[NAME_LEN];
	sw_int32_t pid;			/*pid*/
	sw_int32_t status;		/*process status. Range: SW_S_START~SW_S_NREACH*/
	sw_int32_t type;		/*process type. Range: ND_LSN~ND_ALERT*/
	sw_int32_t mqid;		/*msg queue id*/
	sw_int32_t shmid;		/*shm id */
	sw_uint32_t curr_cnt;		/* the current task cnt(not finish) */
	sw_uint32_t task_cnt;		/*waiting package number*/
	sw_int32_t proc_cnt;		/*child process number*/
	sw_int32_t group_index;		/* group index */
	sw_int32_t comtype;		/* comm type */
	sw_int32_t sendtype;		/* send type */
	sw_int32_t proc_index;		/*process index	*/
	sw_int32_t restart_cnt;	/*process restart count*/
	sw_int32_t restart_flag;   	/*restart flag, RESTART_INITED: restart params have been set, other not.*/
	sw_int32_t restart_type;	/*restart type: ALWAYS_RESTART: always restart process.
					, NEVER_RESTART: never restart process.
					, LIMITED_RESTART: limited restart count.*/
	sw_int32_t lockid;
	sw_int32_t busy_flag;
	sw_int32_t mqids[MAX_SVR_LEVEL_CNT];
	long starttime;
};
typedef struct sw_proc_info_s sw_proc_info_t;

struct sw_procs_head_s
{
	sw_int32_t lock_id;
	sw_int32_t count;			/*process info count*/
	sw_int32_t sw_work_stat;		/*BP running status*/

	sw_int32_t sys_porc_offset;		/* bp system process shm addr offset*/
	sw_int32_t sys_proc_use;		/* bp system process use cnt */
	sw_int32_t sys_procs_max;		/* bp system process max cnt*/

	sw_int32_t svr_grp_offset;		/* bp svr group shm add offset */
	sw_int32_t svr_grp_max;		/* bp svr group max*/
	sw_int32_t svr_grp_use;		/* bp svr group use*/

	sw_int32_t app_porc_offset;		/* app process shm addr offset */
	sw_int32_t app_proc_max;		/* app process max cnt */
	sw_int32_t app_proc_use;		/* app process use cnt */
};
 
typedef struct sw_procs_head_s sw_procs_head_t;

struct sw_svr_grp_s
{
	sw_int32_t lock_id;
	sw_int32_t offset;
	sw_int32_t svc_max;
	sw_int32_t svc_curr;
	char svrname[NAME_LEN];
	char prdtname[NAME_LEN];
};
typedef struct sw_svr_grp_s sw_svr_grp_t;


#define PROC_STAT_DESC_LEN	(128)	/* The max length of process status descriptor */
#define PROC_TYPE_DESC_LEN	(128)	/* The max length of process type descriptor */

/* Descriptor of process status */
typedef struct
{
	int status;						/* Process status. range: SW_S_START~SW_S_NREACH */
	char desc[PROC_STAT_DESC_LEN];	/* Descriptor */
}proc_stat_desc_t;

/* Descriptor of process type */
typedef struct
{
	int type;						/* Process type. Range: ND_LSN~ND_ALERT */
	char desc[PROC_TYPE_DESC_LEN];	/* Descriptor */
}proc_type_desc_t;

SW_PUBLIC sw_int_t procs_init(sw_char_t* addr, sw_syscfg_t * sys_cfg);
SW_PUBLIC sw_int_t procs_get_head(sw_procs_head_t *procs_head);
SW_PUBLIC sw_int_t procs_set_addr(sw_char_t* addr);
SW_PUBLIC sw_int32_t procs_get_size(sw_syscfg_t* sys_cfg);
SW_PUBLIC sw_int_t procs_svr_alloc(sw_svr_grp_t * grp);
SW_PUBLIC sw_int_t procs_lsn_register(sw_proc_info_t *proc_info);
SW_PUBLIC sw_int_t procs_svr_register(sw_proc_info_t *proc_info);
SW_PUBLIC sw_int_t procs_sys_register(sw_proc_info_t *proc_info);
SW_PUBLIC sw_int_t procs_get_sys_by_name(const char *name,sw_proc_info_t *proc_info);
SW_PUBLIC sw_int_t procs_set_sys_by_name(const char *name, const sw_proc_info_t *info);
SW_PUBLIC sw_int_t procs_get_sys_by_index(sw_int32_t index,sw_proc_info_t *proc_info);
SW_PUBLIC sw_int_t procs_set_sys_by_index(int index, const sw_proc_info_t *info);
SW_PUBLIC sw_svr_grp_t* procs_get_svr_by_index(sw_int32_t index);
SW_PUBLIC sw_svr_grp_t* procs_get_svr_by_name(const sw_char_t *name,const sw_char_t *prdt);
SW_PUBLIC sw_int_t procs_get_proces_by_index(const sw_svr_grp_t *svr_grp,sw_int32_t index,sw_proc_info_t *proc);
SW_PUBLIC sw_int_t procs_get_proces_by_name(const sw_svr_grp_t *svr_grp,sw_char_t *name,sw_proc_info_t *proc);
SW_PUBLIC bool procs_sys_isexist_by_index(sw_int32_t index);
SW_PUBLIC bool procs_is_sys_exist(const sw_char_t *name);
SW_PUBLIC sw_int_t procs_get_proces_info(sw_char_t *prdt,sw_char_t *svr,sw_char_t *svc,sw_proc_info_t *proc);
SW_PUBLIC sw_int_t procs_get_dst_proc(sw_char_t *svr,sw_char_t *prdt, sw_int32_t msg_type, sw_proc_info_t *proc);
SW_PUBLIC const char *procs_get_stat_desc(int status);
SW_PUBLIC const char *procs_get_type_desc(int type);
SW_PUBLIC sw_int_t procs_update_process_busystatus(sw_char_t *prdt, sw_char_t *svr, sw_char_t *svc, sw_int32_t busy_flag);
SW_PUBLIC sw_int_t procs_check_svr_status(char *svrname, char *prdtname);
SW_PUBLIC sw_int32_t procs_get_sys_status(char *sysname);

#endif /* __PROCS_H__ */
