/*********************************************************************
 *** File: sw_uni_swconfig.c
 *** Function: Eswitch 's starting , stoping and management.
 *** Author: wangkun
 *** Create Date: 2013-4-3
 *** Update record:
 ********************************************************************/
#include <errno.h>

#include "seqs.h"
#include "uni_svr.h"
#include "pub_log.h"
#include "pub_type.h"
#include "msg_trans.h"
#include "pub_proc.h"
#include "pub_cfg.h"
#include "mtype.h"
#include "job.h"
#include "pub_time.h"
#include "pub_usocket.h"

#define ADMIN_LOCKFILE ".swadmin.lock"

SW_PROTECTED sw_int_t uni_svr_link_ipc(sw_uni_svr_t *svr);
SW_PROTECTED bool uni_svr_proc_is_exist(const sw_char_t *name);
SW_PROTECTED sw_int_t uni_svr_send_cmd(sw_uni_svr_t *svr, sw_int_t flag,
		const sw_char_t *to_name, const sw_char_t *name);
SW_PROTECTED sw_int_t uni_svr_send_cmd2(
		sw_uni_svr_t *svr, 
		const char * dst_name,
		sw_cmd_t *cmd);

SW_PROTECTED sw_int_t uni_svr_print_all_sys();
SW_PROTECTED sw_int32_t uni_svr_proc_is_ready(const sw_char_t *proc_name, sw_int32_t num);
static int deploy_bp_read_info(
		sw_xmltree_t *tree, 
		sw_xmlnode_t *node,
		const char **NAME_value, 
		const char **ATTR_value);
static void deploy_bp_process_lsn(
		sw_uni_svr_t *svr,
		sw_xmltree_t *tree, 
		sw_xmlnode_t *node);
static void deploy_bp_process_lsn_prdt(
		sw_uni_svr_t *svr,
		const char *lsn_process_name,
		sw_xmltree_t *tree,
		sw_xmlnode_t *prdtnode);
static void deploy_bp_process_prdt(
		sw_uni_svr_t *svr,
		sw_xmltree_t *tree,
		sw_xmlnode_t *node);
static void deploy_bp_process_prdt_svr(
		sw_uni_svr_t *svr,
		const char *prdt_process_name,
		sw_xmltree_t *tree, 
		sw_xmlnode_t *svrnode);
static void deploy_bp_process_prdt_svr_svc(
		sw_uni_svr_t *svr,
		const char *prdt_process_name,
		const char *svrname,
		sw_xmltree_t *tree, 
		sw_xmlnode_t *svcnode);
sw_int_t uni_svr_start_prdt(sw_uni_svr_t *svr, const sw_char_t *prdt_name);
static int uni_regist_ipc(uni_msipc_t *msipc, char *pname, int pid, int ipcid, int flag);
static int uni_get_shm_info(uni_msipc_t *msipc, char *pname, int shmid, int index);
static int uni_get_msg_info(uni_msipc_t *msipc, char *pname, int mqid, int index, int pid);
static int uni_get_sem_info(uni_msipc_t *msipc, char *pname, int semid, int index);
static int uni_clear_ipc_sem(int semid);
static int uni_clear_ipc_shm(int shmid, int pid, int nattch, int flag);
static int uni_clear_ipc_msg(int msgid, int pid);
static sw_int_t uni_svr_prdt_print_all_ext(sw_uni_svr_t *svr, const sw_prdt_cfg_t *prdt, char *status);
static int uni_svr_crt_lockfile();
static int uni_svr_lockfile_exist();
static int uni_svr_remove_lockfile();

SW_PROTECTED sw_int32_t uni_svr_proc_is_ready(const sw_char_t *proc_name, int num)
{
	sw_int_t ret = 0;
	int j = 0;
	sw_proc_info_t proc_info;
	sw_svr_grp_t    *grp_svr;

	/* 1. Get system process from register table */
	grp_svr = procs_get_svr_by_name(proc_name, NULL);
	if (grp_svr == NULL)
	{
		pub_log_info("[%s][%d] local log service is reading!", __FILE__, __LINE__);
		return -1;
	}
	if(grp_svr->svc_curr != num)
	{
		pub_log_info("[%s][%d] curr=[%d] num=[%d]", __FILE__, __LINE__, grp_svr->svc_curr, num);
		return -1;
	}
	for (j = 0; j < num; j++)
	{
		memset(&proc_info, 0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_info("[%s][%d] alog son proc is not ready!", __FILE__, __LINE__);
			return -1;
		}
		ret = pub_proc_checkpid(proc_info.pid);
		if(ret != SW_OK)
		{
			pub_log_info("[%s][%d] Process [%d] is last! name=[%s]",
					__FILE__, __LINE__, proc_info.pid, proc_info.name);
			return -1;
		}

	}

	return 0;
}
/******************************************************************************
 ** Name : uni_svr_init
 ** Desc : Initialize server uniform interface
 ** Input: 
 **        svr: Uniform server object
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Wangkun # 2013.04.03 #
 ******************************************************************************/
sw_int_t uni_svr_init(sw_uni_svr_t *svr)
{
	sw_int_t ret = SW_ERR;

	/*  Set module name */
	snprintf(svr->mod_name, sizeof(svr->mod_name), "%s", SW_UNI_MOD_NAME);

	/* Create socket used cmd. */
	svr->cmdfd= udp_bind((char *)svr->mod_name);
	if(svr->cmdfd < 0)
	{
		pub_log_error("[%s][%d]Create socket error! errmsg:[%d]%s",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	/* Link eswitch's public ipc */
	ret = uni_svr_link_ipc(svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Link public ipc fail.", __FILE__, __LINE__);
		return SW_ERR;        
	}

	return SW_OK;    
}

/******************************************************************************
 ** Name : uni_svr_link_ipc
 ** Desc : Link ipc
 ** Input: 
 **     svr: The context of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_link_ipc(sw_uni_svr_t *svr)
{
	sw_int_t ret = SW_ERR;

	ret = run_link();
	if(  SHM_MISS == ret)
	{
		ret =  run_init();
	}

	if (ret  < 0)
	{
		pub_log_error("[%s][%d] shm link failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	svr->shm_cfg = run_get_syscfg();

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_destroy
 ** Desc : Destory uni_svr object.
 ** Input: 
 **     svr: The context of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Wangkun # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_destroy(sw_uni_svr_t *svr)
{
	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_is_exist
 ** Desc : Judge product whether exist?
 ** Input: 
 **     cycle: The cycle of process
 **     name: The name of product
 ** Output: NONE
 ** Return: true:exit false:not exist
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.23 #
 ******************************************************************************/
bool uni_svr_prdt_is_exist(const sw_uni_svr_t *svr, const char *name)
{
	int idx = 0;
	u_char	*addr = NULL;
	const sw_prdt_cfg_t *prdt = NULL;
	const sw_cfgshm_hder_t *head = NULL;

	addr = (u_char *)svr->shm_cfg;
	head = (const sw_cfgshm_hder_t *)addr;
	prdt = (sw_prdt_cfg_t *)(addr + head->prdt_offset);

	for(idx=0; idx<head->prdt_use; idx++)
	{
		if(!strcasecmp(name, prdt[idx].prdtname))
		{
			return true;
		}
	}

	return false;
}

/******************************************************************************
 ** Name : run_get_prdt
 ** Desc : Get information of special product
 ** Input: 
 **     cycle: The cycle of process
 **     name: The name of special product
 ** Output: NONE
 ** Return: Address of special product information
 ** Process:
 **	    1. Get address of product configuration
 **     2. Search special product
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
const sw_prdt_cfg_t *run_get_prdt(const sw_uni_svr_t *uni_svr, const char *name)
{
	sw_int_t idx = 0;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)uni_svr->shm_cfg;


	/* 1. Get the address of product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return NULL;
	}

	/* 2. Search special product */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(!strcasecmp(name, item->prdtname))
		{
			return item;
		}
		item++;
	}

	return NULL;
}

/******************************************************************************
 ** Name : uni_svr_proc_is_exist
 ** Desc : Judge special system process whether exist or running
 ** Input: 
 **     name: Name of process
 ** Output: NONE
 ** Return: true: exist  false: not exist
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
bool uni_svr_proc_is_exist(const sw_char_t *proc_name)
{
	sw_int_t ret = 0;
	sw_proc_info_t info;

	memset(&info, 0, sizeof(info));

	/* 1. Get system process from register table */
	ret = procs_get_sys_by_name(proc_name, &info);
	if(SW_OK != ret)
	{
		return false;
	}

	/* 2. Check process whether exist? */
	ret = pub_proc_checkpid(info.pid);
	if((SW_OK != ret) || (SW_S_START != info.status))
	{
		return false;
	}

	return true;
}

/******************************************************************************
 ** Name : uni_svr_sys_is_exist
 ** Desc : Judge system process whether exist or running
 ** Input: 
 **     name: Name of process
 ** Output: NONE
 ** Return: true: exist  false: not exist
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.29 #
 ******************************************************************************/
bool uni_svr_sys_is_exist(void)
{
	sw_int32_t idx=0, ret=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	uni_proc_item_t item;

	memset(&item, 0, sizeof(item));
	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));


	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Stop all of system process */
	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || ('\0' == info.name[0]))
		{
			continue;
		}

		ret = pub_proc_checkpid(info.pid);
		if((SW_OK == ret) || (SW_S_START == info.status))
		{
			pub_log_error("[%s][%d] Process [%s:%d] is exist!", __FILE__, __LINE__, info.name, info.pid);
			return true;
		}
	}

	return false;
}

/******************************************************************************
 ** Name : uni_svr_start_exec
 ** Desc : Start to execute special process
 ** Input: 
 **     name: Name of process
 ** Output: NONE
 ** Return: 0:success  !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.04.03 #
 ******************************************************************************/
sw_int_t uni_svr_start_exec(const char *exec, const char *name)
{
	pid_t pid = -1;

	pid = fork();
	if(0 == pid)
	{
		execlp(exec, exec, name, NULL);

		pub_log_info("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		exit(SW_OK);
	}
	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_start_process
 ** Desc : Start to execute special process
 ** Input: 
 **     exec_name: Name of process
 ** Output: NONE
 ** Return: 0:success  !0:failed
 ** Process:
 **     1. Judge special process whether exist?
 **     2. Started to execute special process
 ** Note :
 ** Author: # Wangkun # 2013.04.03 #
 ******************************************************************************/
sw_int_t uni_svr_start_process(const sw_char_t *exec_name)
{
	sw_int_t ret = SW_ERR;

	/* 1. Judge specail process whether exist? */
	if(uni_svr_proc_is_exist(exec_name))
	{ 
		pub_log_error("[%s][%d] Process [%s] has been started!",
				__FILE__, __LINE__, exec_name);
		return SW_OK;
	}

	/* 2. Exec special process */
	ret = uni_svr_start_exec(exec_name, exec_name);
	if(SW_OK != ret)
	{        
		pub_log_error("[%s][%d] Exec [%s] failed!", __FILE__, __LINE__, exec_name);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_stop_process
 ** Desc : Stop special process
 ** Input: 
 **     cycle: Cycle of process
 **     cmd_type: Cmd type. Range: SW_MSTARTONE ~ SW_MSTARTALLTASK
 **     exec_name: Name of process
 ** Output: NONE
 ** Return: 0:success  !0:failed
 ** Process:
 **     1. Judge special process whether exist?
 **     2. Send stop command to special process
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
sw_int_t uni_svr_stop_process(sw_uni_svr_t *svr, int cmd_type, const char *exec_name)
{
	sw_int_t ret = 0;

	ret = uni_svr_proc_is_exist(exec_name);
	if(false == ret)
	{
		pub_log_info("[%s][%d] Process [%s] isn't running!",
				__FILE__, __LINE__, exec_name);
		return SW_OK;
	}

	ret = uni_svr_send_cmd( svr, cmd_type, exec_name, NULL);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Send cmd to process [%s] failed.",
				__FILE__, __LINE__, exec_name);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_stop_svrproc_in_prdt
 ** Desc : Stop server process in special product
 ** Input: 
 **     cycle: Cycle of process
 **     cmd_type: Cmd type. Range: SW_MSTARTONE ~ SW_MSTARTALLTASK
 **     exec_name: Name of product service process
 ** Output: NONE
 ** Return: 0:success  !0:failed
 ** Process:
 **     1. Judge special process whether exist?
 **     2. Send stop command to special process
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
sw_int_t uni_svr_stop_svrproc_in_prdt(sw_uni_svr_t *svr, int cmd_type, const char *exec_name, const char *svr_name)
{
	sw_int_t ret = 0;
	sw_cmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));

	cmd.type = cmd_type;
	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
	snprintf(cmd.dst_svr, sizeof(cmd.dst_svr), "%s", svr_name);
	pub_log_info("[%s][%d] Src:%s Cmd:%d Dst:%s, Server:%s",
			__FILE__, __LINE__, cmd.udp_name, cmd.type, exec_name, cmd.dst_svr);

	ret = uni_svr_send_cmd2(svr, exec_name, &cmd);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Send cmd to process [%s] failed.",
				__FILE__, __LINE__, exec_name);
		return SW_ERR;
	}

	return SW_OK;
}
/******************************************************************************
 ** Name : uni_svr_send_cmd
 ** Desc : Send command to special process
 ** Input: 
 **     svr: Object of sw_uni_svr_t
 **     cmd_type: Cmd type of admin. Range is SW_MSTARTONE ~ SW_START_PRDT
 **     dst_name: Destination name
 **     lsn_name: Listen name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **     1. Send command to destination
 **     2. Receive replay
 ** Note :
 ** Author: # Wangkun # 2013.04.03 #
 ** Modify: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t uni_svr_send_cmd(sw_uni_svr_t *svr,sw_int_t cmd_type, 
		const sw_char_t *dst_name, const sw_char_t *lsn_name)
{
	sw_int_t ret = 0;
	sw_cmd_t cmd;

	memset(&cmd, 0, sizeof(cmd));

	/* 1. Check paramters */
	if((cmd_type < 0) || pub_str_isempty(dst_name))
	{
		pub_log_error("[%s][%d] Param is incorrect! flag:%d",
				__FILE__, __LINE__, cmd_type);
		return SW_ERR;
	}

	/* 2. Set command */
	cmd.type = cmd_type;

	if(!pub_str_isempty(lsn_name))
	{
		snprintf(cmd.lsn_name, sizeof(cmd.lsn_name), "%s", lsn_name);
	}

	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);

	pub_log_info("[%s][%d] Src:%s Cmd:%d Dst:%s",
			__FILE__, __LINE__, cmd.udp_name, cmd.type, dst_name);

	/* 3. Send command */
	ret =  udp_send(svr->cmdfd, (sw_char_t*)&cmd, sizeof(cmd), dst_name);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] Send command to [%s]. errmsg:[%d]%s!",
				__FILE__, __LINE__, dst_name, errno, strerror(errno));
		return SW_ERR;
	}

	memset(&cmd, 0, sizeof(cmd));

	/* 4. Receive reply */
	ret = udp_recv(svr->cmdfd, (sw_char_t *)&cmd, sizeof(cmd));
	if(ret <= 0)
	{
		pub_log_error("[%s][%d] Receive reply failed! ret:%d dst:[%s] errmsg:[%d]%s!",
				__FILE__, __LINE__, ret, dst_name, errno, strerror(errno));
		return SW_ERR;
	}

	/* 5. Check reply */
	switch(cmd.type)
	{
		case SW_RESCMD:
			{
				pub_log_info("[%s][%d] Send command to [%s] success!", __FILE__, __LINE__, dst_name);
				return SW_OK;
			}
		case SW_ERRCMD:
			{
				pub_log_error("[%s][%d] Send command to [%s] failed! errmsg:[%d]%s!",
						__FILE__, __LINE__, dst_name, errno, strerror(errno));
				return SW_ERR;
			}
		default:
			{
				pub_log_info("[%s][%d] Send command to [%s] unknown!",
						__FILE__, __LINE__, dst_name);
				return SW_ERR;
			}
	}

	return SW_OK;
}

SW_PROTECTED sw_int_t uni_svr_send_cmd2(
		sw_uni_svr_t *svr, 
		const char * dst_name,
		sw_cmd_t *cmd)
{
	sw_int_t ret = 0;

	ret =  udp_send(svr->cmdfd, (char *)cmd, sizeof(sw_cmd_t), dst_name);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] Send command to [%s]. errmsg:[%d]%s!",
				__FILE__, __LINE__, dst_name, errno, strerror(errno));
		return SW_ERR;
	}

	memset(cmd, 0, sizeof(sw_cmd_t));

	ret = udp_recv(svr->cmdfd, (char *)cmd, sizeof(sw_cmd_t));
	if(ret <= 0)
	{
		pub_log_error("[%s][%d] Receive reply failed! ret:%d dst:[%s] errmsg:[%d]%s!",
				__FILE__, __LINE__, ret, dst_name, errno, strerror(errno));
		return SW_ERR;
	}

	switch(cmd->type)
	{
		case SW_RESCMD:
			{
				pub_log_info("[%s][%d] Send command to [%s] success!", __FILE__, __LINE__, dst_name);
				return SW_OK;
			}
		case SW_ERRCMD:
			{
				pub_log_error("[%s][%d] Send command to [%s] failed! errmsg:[%d]%s!",
						__FILE__, __LINE__, dst_name, errno, strerror(errno));
				return SW_ERR;
			}
		default:
			{
				pub_log_info("[%s][%d] Send command to [%s] unknown!",
						__FILE__, __LINE__, dst_name);
				return SW_ERR;
			}
	}
}

/******************************************************************************
 ** Name : uni_svr_start_lsn
 ** Desc : Start special listen process
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     lsn_name: The name of special listen process
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
sw_int_t uni_svr_start_lsn(const sw_uni_svr_t *svr, const sw_char_t *lsn_name)
{
	sw_int_t ret = SW_ERR;
	sw_char_t	name[128];

	/* 1. Judge the main process whether exist? */

	memset(name, 0x0, sizeof(name));
	if (strncmp(lsn_name, PROC_NAME_LSN, strlen(PROC_NAME_LSN)) == 0)
	{
		strncpy(name, lsn_name, sizeof(name) - 1);
	}
	else
	{
		sprintf(name, "%s_%s", PROC_NAME_LSN, lsn_name);
	}
	if(uni_svr_proc_is_exist(name))
	{ 
		pub_log_error("[%s][%d] Listen [%s] has been started!", __FILE__, __LINE__, lsn_name);
		return SW_OK;
	}

	ret = uni_svr_start_exec(PROC_NAME_LSN, lsn_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start [%s %s] failed!",
				__FILE__, __LINE__, PROC_NAME_LSN, lsn_name);
		return SW_ERR;
	}
	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_stop_all
 ** Desc : Stop all process
 ** Input: 
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
sw_int_t uni_svr_stop_all(sw_cycle_t *cycle, sw_uni_svr_t *svr)
{
	sw_int32_t ret = SW_ERR;

	/* 1. Stop police */
	ret = uni_svr_stop_process(svr, SW_MSTOPSELF, PROC_NAME_POL);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop police failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/*  Stop saf */
	ret = uni_svr_stop_process(svr, SW_MSTOPSELF, PROC_NAME_SAFD);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop police failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Stop all listen */
	ret = uni_svr_stop_all_lsn(svr);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop all listen failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. Stop all svc */
	ret = uni_svr_stop_all_svc(svr);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop all svc failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 4. Stop job */
	ret = uni_svr_stop_process(svr, SW_MSTOPSELF, PROC_NAME_JOB);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop job failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

#if defined(__ALERT_SUPPORT__)
	/* 5. Stop Alert */
	ret = uni_svr_stop_process(svr, SW_MSTOPSELF, PROC_NAME_ALERT);
	if(0 && SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop alert failed!", __FILE__, __LINE__);
		return SW_ERR;
	}
#endif /*__ALERT_SUPPORT__*/

	if (g_use_local_logs)
	{
		ret = uni_svr_stop_process( svr, SW_MSTOPSELF, PROC_NAME_LOG);
		if(0 && SW_OK != ret)
		{
			pub_log_error("[%s][%d] Stop log failed!", __FILE__, __LINE__);
			return SW_ERR;
		}
	}
	/* 5. Save sequene information */
	if (uni_svr_lockfile_exist())
	{
		uni_svr_remove_lockfile();
		ret = seqs_save(SEQS_SAVE_STAT_CLEAR);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Save process info error!", __FILE__, __LINE__);
			return SW_ERR;
		}
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_stop_all_lsn
 ** Desc : Stop all listen process
 ** Input: 
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_stop_all_lsn(sw_uni_svr_t* svr)
{
	int idx=0, ret=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	uni_proc_item_t item;

	memset(&item, 0, sizeof(item));
	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));


	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Stop all of listen process */
	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) 
				|| (ND_LSN != info.type) || ('\0' == info.name[0]))
		{
			continue;
		}

		ret = uni_svr_stop_process( svr, SW_MSTOPONE, info.name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Send cmd to listen failed.", __FILE__, __LINE__);
			return SW_ERR;
		}

		printf("\n\tSend command to stop lsn [%s] success!\n", info.name);
	}

	return SW_OK;
}

sw_int_t uni_svr_start_lsn_in_prdt(sw_uni_svr_t *svr, const sw_char_t *prdt_name, const sw_char_t *lsn_name)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_char_t	proc_name[64];
	const sw_prdt_cfg_t	*item = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;
	item = cfg_get_prdt_cfg(shm_cfg);
	if (NULL == item)
	{
		pub_log_error("[%s][%d] Get prdt information failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i = 0; i< shm_cfg->prdt_use; i++)
	{
		if (strcmp(item->prdtname, prdt_name) == 0)
		{
			break;
		}
		item++;
	}
	if (i == shm_cfg->prdt_use)
	{
		ret = cfg_add_prdt((sw_cfgshm_hder_t *)shm_cfg, (char *)prdt_name);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
			return SW_ERROR;
		}
		item = run_get_prdt(svr, prdt_name);
		if (item == NULL)
		{
			pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
			return SW_ERROR;
		}
	}
	for (i = 0; i < item->lsn_cnt; i++)
	{
		if (strcmp(lsn_name, item->lsnname[i]) != 0)
		{
			continue;
		}
		memset(proc_name, 0x0, sizeof(proc_name));
		sprintf(proc_name, "%s_%s", PROC_NAME_LSN, lsn_name);
		if (uni_svr_proc_is_exist(proc_name) == true)
		{
			ret = uni_svr_prdt_start_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, lsn_name);
				return SW_ERROR;
			}
		}
		else
		{
			ret = uni_svr_start_lsn(svr, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Start lsn [%s] failed!", __FILE__, __LINE__, lsn_name);
				return SW_ERROR;
			}
			pub_log_debug("[%s][%d] 启动[%s]成功!", __FILE__, __LINE__, lsn_name);
			sleep(1);
			ret = uni_svr_prdt_start_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, lsn_name);
				return SW_ERROR;
			}
			pub_log_debug("[%s][%d] 启动产品[%s][%s]成功!", __FILE__, __LINE__, prdt_name, lsn_name);
		}
	}
	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_stop_all_svc
 ** Desc : Stop all of svc process
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **     1. Get the address of SHM
 **     2. Get the number of product
 **     3. Judge special process whether exist?
 **     4. Send stop command
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t uni_svr_stop_all_svc(sw_uni_svr_t *svr)
{
	sw_int_t idx = 0, ret = SW_ERR;
	char svc_name[FILE_NAME_MAX_LEN] = {0};
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;


	/* 1. Get the product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Traverse all of products */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		memset(svc_name, 0, sizeof(svc_name));

		snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, item->prdtname);

		ret = uni_svr_stop_process( svr, SW_MSTOPSELF, svc_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Send command to stop [%s] failed.",
					__FILE__, __LINE__, svc_name);
			return SW_ERR;
		}

		printf("\n\tSend command to stop svc [%s] success\n", svc_name);
		item++;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_stop
 ** Desc : Stop special process
 ** Input: 
 **     svr: Object of sw_uni_svr_t
 **     svc_name: svc process name
 **     flag: Cmd flag. Range: SW_NORMAL~SW_FORCE
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Wangkun # 2013.04.03 #
 ******************************************************************************/
sw_int_t uni_svr_stop(sw_uni_svr_t *svr, const sw_char_t *proc_name,
		sw_int32_t flag)
{
	return uni_svr_stop_process(svr, SW_MSTOPSELF, proc_name);

}

sw_int_t uni_svr_start_all_task(sw_uni_svr_t *svr)
{
	sw_int_t ret = SW_ERR;

	ret = uni_svr_proc_is_exist(PROC_NAME_JOB);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] %s is not running.", __FILE__, __LINE__, PROC_NAME_JOB);
		return SW_ERR;
	}

	ret = uni_svr_send_cmd( svr, SW_MSTARTALLTASK, PROC_NAME_JOB, NULL);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Send cmd to [%s] failed.", __FILE__, __LINE__, PROC_NAME_JOB);
		return SW_ERR;
	}

	return SW_OK;
}


/******************************************************************************
 ** Name : uni_svr_start_all
 ** Desc : Start all process
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **        1. Start all svc
 **        2. Start all listen
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t uni_svr_start_all(const sw_cycle_t *cycle, sw_uni_svr_t *svr)
{
	sw_int_t ret = SW_ERR;
	sw_int_t times = 0;
	alog_cfg_t	alog;
	sw_cfgshm_hder_t	*addr = NULL;

	memset(&alog, 0x0, sizeof(alog));
	addr = run_get_syscfg();
	memset(&alog, 0x0, sizeof(alog));
	cfg_get_alog_cfg(addr, &alog);
	pub_log_info("[%s][%d] use=[%d] transport=[%d]", __FILE__, __LINE__, alog.use, alog.transport);
	if (g_use_local_logs)
	{
		ret = uni_svr_start_process(PROC_NAME_LOG);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start logsvr failed.", __FILE__, __LINE__);
			return SW_ERR;
		}
		while(1)
		{
			usleep(100000);
			ret = uni_svr_proc_is_ready(PROC_NAME_LOG, alog.proc_num);
			if( ret == 0)
			{
				break;
			}
			times++;

			if (times > 100)
			{
				pub_log_error("[%s][%d] Start logsvr failed.", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}	
	}

#if defined(__ALERT_SUPPORT__)
	/* 1. Start alert */
	ret = uni_svr_start_process(PROC_NAME_ALERT);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start alert failed.", __FILE__, __LINE__);
		return SW_ERR;
	}
#endif /*__ALERT_SUPPORT__*/

	/* 2. Start all svc */
	ret = uni_svr_start_all_svc( svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all listen failed.", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. Start all listen */
	ret = uni_svr_start_all_lsn(svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all listen failed.", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 4. Start job */
	ret = uni_svr_start_process(PROC_NAME_JOB);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start job failed.", __FILE__, __LINE__);
		return SW_ERR;
	}
#if 0
	/* 5. Start police */
	ret = uni_svr_start_process(PROC_NAME_SAFD);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start saf failed.", __FILE__, __LINE__);
		return SW_ERR;
	}
#endif
	/* g. Start police */
	ret = uni_svr_start_process(PROC_NAME_POL);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start pol failed.", __FILE__, __LINE__);
		return SW_ERR;
	}

	uni_svr_crt_lockfile();

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_start_all_lsn
 ** Desc : Start all listen process
 ** Input: 
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **        1. Get the address of configuration share memory
 **        2. Compute the address of product information
 **        3. 
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t uni_svr_start_all_lsn( sw_uni_svr_t *svr)
{
	sw_int_t i = 0, ret = SW_ERR;
	const sw_chl_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;


	/* 1. Get the address of product information */
	item = cfg_get_chnl_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get lsn information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Travers all products */
	for(i=0; i<shm_cfg->lsn_use; i++)
	{
		if(0 == item->status)
		{
			item++;
			continue;   /* Didn't switch on this product */
		}

		/* Travers all listen process of product */
		if(0 != strlen(item->lsnname))
		{
			ret = uni_svr_start_lsn(svr, item->lsnname);
			if(SW_OK != ret)
			{
				pub_log_error("[%s][%d] Start listen [%s] failed!",
						__FILE__, __LINE__, item->lsnname);
				return SW_ERR;
			}
		}
		item++;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_start_svc
 ** Desc : Start special svc process
 ** Input: 
 **     prdt_name: The name of special product
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_start_svc(const char *prdt_name)
{
	bool bret = false;
	sw_int_t ret = SW_ERR;
	char svc_name[SVC_NAME_MAX_LEN] = {0};

	snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, prdt_name);

	/* 1. Judge special process whether exist? */
	bret = uni_svr_proc_is_exist(svc_name);
	if(true == bret)
	{
		pub_log_info("[%s][%d] %s is exist!", __FILE__, __LINE__, svc_name);
		return SW_OK;   /* The special process is exist */
	}

	/* 2. Exec specail program */
	ret = uni_svr_start_exec(PROC_NAME_SVC_MAN, prdt_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start [%s] failed!",
				__FILE__, __LINE__, svc_name);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_start_svr_in_prdt
 ** Desc : Start special svr processes in special product
 ** Input: 
 **     prdt_name: The name of special product
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_start_svr_in_prdt(sw_uni_svr_t *svr, const char *prdt_name, const char *svr_name)
{
	bool bret = false;
	sw_int_t ret = SW_ERR;
	char svc_name[SVC_NAME_MAX_LEN] = {0};

	snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, prdt_name);

	/* 1. Judge product service process[exmp:swsvcman_OMP] is exist or not */
	bret = uni_svr_proc_is_exist(svc_name);
	if(false == bret)
	{
		pub_log_info("[%s][%d] %s is not exist! please start service of prdt[%s]", __FILE__, __LINE__, prdt_name);
		return SW_OK;
	}

	sw_cmd_t cmd;
	memset(&cmd, 0x0, sizeof(cmd));
	cmd.type = SW_MSTARTSVR;
	strncpy(cmd.dst_svr, svr_name, sizeof(cmd.dst_svr) - 1);
	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
	pub_log_info("[%s][%d] Src:%s Cmd:%d Dst:%s, Server:%s",
				__FILE__, __LINE__, cmd.udp_name, cmd.type, svc_name, cmd.dst_svr);

	/* 2. send cmd to product service process */
	ret = uni_svr_send_cmd2(svr, svc_name, &cmd);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Send cmd to process [%s] failed.",
				__FILE__, __LINE__, svc_name);
		return SW_ERR;
	}
	
	return SW_OK;
}


/******************************************************************************
 ** Name : uni_svr_start_all_svc
 ** Desc : Start all of svc process
 ** Input: 
 **     cycle: The cycle of process
 **        svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t uni_svr_start_all_svc(sw_uni_svr_t *svr)
{
	sw_int_t idx = 0, ret = SW_ERR;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;


	/* 1. Get the address of product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Start product svc */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 == item->status)
		{
			item++;
			continue;   /* Doesn't switch on this product */
		}

		/* Start special svc */
		ret = uni_svr_start_svc(item->prdtname);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start [%s] failed!",
					__FILE__, __LINE__, item->prdtname);
			return SW_ERR;
		}

		item++;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_start_all
 ** Desc : Start all process of special product
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **        1. Start all svc
 **        2. Start all listen
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_start_all( sw_uni_svr_t *svr, const char *prdt_name)
{
	sw_int_t ret = SW_ERR;
	const sw_prdt_cfg_t *item = NULL;

	/* 1. Judge the product whether switch on? */
	item = run_get_prdt(svr, prdt_name);
	if (item == NULL)
	{
		ret = cfg_add_prdt((sw_cfgshm_hder_t *)svr->shm_cfg, (char *)prdt_name);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] cfg_add_prdt [%s] error!", __FILE__, __LINE__, prdt_name);
			return SW_ERROR;
		}

		item = run_get_prdt(svr, prdt_name);
	}

	if((NULL == item) || (0 == item->status))
	{
		pub_log_error("[%s][%d] Product [%s] isn't switch on.",
				__FILE__, __LINE__, prdt_name);
		return SW_ERR;
	}

	/* 2. Start all svc of special product */
	ret = uni_svr_prdt_start_all_svc( svr, prdt_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all svc of [%s] failed.",
				__FILE__, __LINE__, prdt_name);
		return SW_ERR;
	}

	/* 3. Start all listen of special product */
	ret = uni_svr_prdt_start_all_lsn( svr, prdt_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Start all listen of [%s] failed.",
				__FILE__, __LINE__, prdt_name);
		return SW_ERR;
	}

	printf("\n\tSend command to start all success\n");

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_start_all_svc
 ** Desc : Start all of svc process
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
int uni_svr_prdt_start_all_svc(sw_uni_svr_t *svr, const char *prdt_name)
{
	sw_int_t idx = 0, ret = SW_ERR;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;


	/* 1. Get the address of product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Start product svc */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 == item->status
				|| 0 != strcasecmp(prdt_name, item->prdtname))
		{
			item++;
			continue;   /* Doesn't switch on this product */
		}

		/* 3. Start specail svc */
		ret = uni_svr_start_svc(item->prdtname);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Start [%s] failed!",
					__FILE__, __LINE__, item->prdtname);
			return SW_ERR;
		}

		item++;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_start_all_lsn
 ** Desc : Start all listen process
 ** Input: 
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **        1. Get the address of configuration share memory
 **        2. Compute the address of product information
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_start_all_lsn( sw_uni_svr_t *svr, const char *prdt_name)
{
	char	buf[64];
	sw_int_t i = 0, j = 0, ret = SW_ERR;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	/* 1. Get the address of product information */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Travers all products */
	for(i=0; i<shm_cfg->prdt_use; i++)
	{
		if(0 == item->status
				|| 0 != strcasecmp(prdt_name, item->prdtname))
		{
			item++;
			continue;
		}

		/* Travers all listen process of product */
		for(j=0; j<SW_LSN_PRDT_MAX; j++)
		{
			if(0 != strlen(item->lsnname[j]))
			{
				if (strncmp(item->lsnname[j], PROC_NAME_LSN, strlen(PROC_NAME_LSN)) != 0)
				{
					memset(buf, 0x0, sizeof(buf));
					sprintf(buf, "%s_%s", PROC_NAME_LSN, item->lsnname[j]);
				}
				else
				{
					strcpy(buf, item->lsnname[j]);
				}
				pub_log_info("[%s][%d] lsn_name=[%s]", __FILE__, __LINE__, buf);
				ret = uni_svr_prdt_start_from_lsn(svr, item->prdtname, buf);
				if(SW_OK != ret)
				{
					pub_log_error("[%s][%d] Start listen [%s] failed!",
							__FILE__, __LINE__, item->lsnname[j]);
					return SW_ERR;
				}
			}
		}
		item++;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_stop_all
 ** Desc : Stop all process of special product
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: The name of process
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 **     1. Stop all listen
 **     2. Stop all svc
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_stop_all(
		sw_uni_svr_t *svr, const char *prdt_name)
{
	sw_int32_t ret = SW_ERR;
	const sw_prdt_cfg_t *item = NULL;

	/* 1. Judge the product whether switch on? */
	item = run_get_prdt(svr, prdt_name);
	if((NULL == item) || (0 == item->status))
	{
		pub_log_error("[%s][%d] Product [%s] isn't switch on.",
				__FILE__, __LINE__, prdt_name);
		return SW_ERR;
	}

	/* 1. Stop all listen */
	ret = uni_svr_prdt_stop_all_lsn( svr, prdt_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop all listen failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Stop all svc */
	ret = uni_svr_prdt_stop_all_svc( svr, prdt_name);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Stop all svc failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	ret = seqs_save(SEQS_SAVE_STAT_CLEAR);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Save process info error!", __FILE__, __LINE__);
		return SW_ERR;
	}
	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_stop_all_svc
 ** Desc : Stop all svc of sepcial product
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     name: The name of product
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_stop_all_svc(
		sw_uni_svr_t *svr, const char *prdt_name)
{
	sw_int_t idx = 0, ret = SW_ERR;
	char svc_name[FILE_NAME_MAX_LEN] = {0};
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;


	/* 1. Get the product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Traverse all of products */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 != strcasecmp(prdt_name, item->prdtname))
		{
			item++;
			continue;
		}

		memset(svc_name, 0, sizeof(svc_name));

		snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, item->prdtname);

		ret = uni_svr_stop_process( svr, SW_MSTOPSELF, svc_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Send command to [%s] failed.",
					__FILE__, __LINE__, svc_name);
			return SW_ERR;
		}
		item++;
	}

	return SW_OK;
}

sw_int_t uni_svr_prdt_oper_from_lsn(
		sw_uni_svr_t *svr, const char *prdt_name, const char *lsn_name, int type)
{
	sw_cmd_t cmd;

	memset(&cmd, 0, sizeof(cmd));

	if (type != SW_STOP_PRDT && type != SW_START_PRDT)
	{
		pub_log_error("[%s][%d] operation type [%d] error!", __FILE__, __LINE__, type);
		return SW_ERROR;
	}

	cmd.type = type;
	snprintf(cmd.dst_prdt, sizeof(cmd.dst_prdt), "%s", prdt_name);
	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);

	udp_send(svr->cmdfd, (sw_char_t*)&cmd, sizeof(cmd), lsn_name);
	memset(&cmd, 0x0, sizeof(cmd));
	udp_recv(svr->cmdfd, (sw_char_t *)&cmd, sizeof(cmd));
	pub_log_info("[%s][%d] udp_name=[%s] type=[%d]", __FILE__, __LINE__, cmd.udp_name, cmd.type);

	return SW_OK;
}

sw_int_t uni_svr_prdt_stop_from_lsn(
		sw_uni_svr_t *svr, const char *prdt_name, const char *lsn_name)
{
	uni_svr_prdt_oper_from_lsn(svr, prdt_name, lsn_name, SW_STOP_PRDT);

	return SW_OK;
}

sw_int_t uni_svr_prdt_start_from_lsn(
		sw_uni_svr_t *svr, const char *prdt_name, const char *lsn_name)
{
	uni_svr_prdt_oper_from_lsn(svr, prdt_name, lsn_name, SW_START_PRDT);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_stop_all_lsn
 ** Desc : Delete special product from the product route of lsn
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_stop_all_lsn(
		sw_uni_svr_t *svr, const char *prdt_name)
{
	char	buf[64];
	sw_int_t idx = 0, ret = SW_ERR;
	sw_procs_head_t head;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	if(head.sys_proc_use <= 0)
	{
		return SW_OK;
	}

	/* 2. Get product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product info failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. Search special product */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(!strcasecmp(prdt_name, item->prdtname))
		{
			break;
		}
		item++;
	}

	if(shm_cfg->prdt_use == idx)
	{
		pub_log_error("[%s][%d] Search product [%s] failed!",
				__FILE__, __LINE__, prdt_name);
		return SW_ERR;
	}

	/* 4. Stop special product form listen */
	for(idx=0; idx<SW_PRDT_LSN_MAX; idx++)
	{
		if(0 == strlen(item->lsnname[idx]))
		{
			continue;
		}
		if (strncmp(item->lsnname[idx], PROC_NAME_LSN, strlen(PROC_NAME_LSN)) != 0)
		{
			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, "%s_%s", PROC_NAME_LSN, item->lsnname[idx]);
		}
		else
		{
			strcpy(buf, item->lsnname[idx]);
		}
		pub_log_info("[%s][%d] lsn_name=[%s]", __FILE__, __LINE__, buf);
		ret = uni_svr_prdt_stop_from_lsn(svr, prdt_name, buf);
		if(SW_OK != ret)
		{
			return SW_ERR;
		}
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_stop_lsn
 ** Desc : Delete special product form the product route of lsn
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 
 ******************************************************************************/
sw_int_t uni_svr_prdt_oper_lsn(sw_uni_svr_t *svr,
		const char *prdt_name, const char *lsn_name, int type)
{
	sw_int_t idx = 0, ret = SW_ERR;
	sw_char_t	buf[64];
	sw_procs_head_t head;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	if(head.sys_proc_use <= 0)
	{
		return SW_OK;
	}

	/* 2. Get product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product info failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. Search special product */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 != strcasecmp(prdt_name, item->prdtname))
		{
			item++;
			continue;
		}
		break;
	}

	if(shm_cfg->prdt_use == idx)
	{
		pub_log_error("[%s][%d] Search product [%s] failed!",
				__FILE__, __LINE__, prdt_name);
		return SW_ERR;
	}

	if (strncmp(lsn_name, PROC_NAME_LSN, strlen(PROC_NAME_LSN)) != 0)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s_%s", PROC_NAME_LSN, lsn_name);
	}	
	else
	{
		strcpy(buf, lsn_name);
	}
	/* 4. Stop special product form listen */
	ret = uni_svr_prdt_oper_from_lsn(svr, prdt_name, buf, type);
	if (ret != SW_OK)
	{
		return SW_ERR;
	}

	return SW_OK;
}

sw_int_t uni_svr_prdt_stop_lsn(
		sw_uni_svr_t *svr,
		const char *prdt_name, const char *lsn_name)
{
	uni_svr_prdt_oper_lsn( svr, prdt_name, lsn_name, SW_STOP_PRDT);

	return SW_OK;
}


sw_int_t uni_svr_prdt_start_lsn(
		sw_uni_svr_t *svr,
		const char *prdt_name, const char *lsn_name)
{
	uni_svr_prdt_oper_lsn( svr, prdt_name, lsn_name, SW_START_PRDT);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_stop_svc
 ** Desc : Stop all svc of sepcial product
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     name: The name of product
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_stop_svc(
		sw_uni_svr_t *svr, const char *prdt_name)
{
	sw_int_t idx = 0, ret = SW_ERR;
	char svc_name[FILE_NAME_MAX_LEN] = {0};
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;


	/* 1. Get the product configuration */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Traverse all of products */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 != strcasecmp(prdt_name, item->prdtname))
		{
			item++;
			continue;
		}

		memset(svc_name, 0, sizeof(svc_name));

		snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, item->prdtname);

		ret = uni_svr_stop_process( svr, SW_MSTOPSELF, svc_name);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Send command to [%s] failed.",
					__FILE__, __LINE__, svc_name);
			return SW_ERR;
		}
		break;
	}

	return SW_OK;
}


/******************************************************************************
 ** Name : uni_svr_print_proc
 ** Desc : Print process information
 ** Input: 
 **     info: Process information
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
void uni_svr_print_proc(const sw_proc_info_t *info)
{
	printf("\t%-23s %-15s %-15d %s\n", 
			info->name, procs_get_type_desc(info->type),
			info->pid, procs_get_stat_desc(info->status));
	return;
}

/******************************************************************************
 ** Name : uni_svr_print_proc_head
 ** Desc : 打印进程信息头部
 ** Input: NONE
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
void uni_svr_print_proc_head(void)
{
	printf("\n\t%-23s %-15s %-15s %s\n", "PROC", "TYPE", "PID", "STATUS");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
}

/******************************************************************************
 ** Name : uni_svr_print_tail
 ** Desc : 打印进程信息尾部
 ** Input: 
 **     total: 总数
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
void uni_svr_print_tail(int total)
{
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL: %d          STATUS: %s\n", total, seqs_get_syssts_desc());
}

/******************************************************************************
 ** Name : uni_svr_get_lsn_route_in_prdt 
 ** Desc : 查询指定产品下的指定侦听的进程状态
 注意：侦听进程的状态指该侦听在指定产品中的状态---ON/OFF
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 打印信息的条数
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.26 #
 *********************************************************************************/
sw_int_t uni_svr_get_lsn_route_in_prdt(sw_uni_svr_t *svr, const sw_char_t *prdtname, const sw_char_t *lsn_name)
{
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_char_t	proc_name[128];

	memset(&cmd, 0x00, sizeof cmd);
	memset(proc_name, 0x0, sizeof(proc_name));

	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
	if (strlen(prdtname) < sizeof(cmd.dst_prdt))
	{
		strcpy(cmd.dst_prdt, prdtname);
	}
	else
	{
		pub_log_error("strlen(%s) is longer than sizeof(cmd.dst_prdt)", prdtname);
		return SW_ERROR;
	}
	if ((strlen(PROC_NAME_LSN) + strlen(lsn_name) + 2) < sizeof(proc_name))
	{
		strcpy(proc_name, PROC_NAME_LSN);
		strcat(proc_name, "_");
		strcat(proc_name, lsn_name);
	}
	else
	{
		pub_log_error("[%s][%d] name(%s) too long", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}
	cmd.type = SW_GET_LSN_ROUTE_IN_PRDT;
	ret = uni_svr_send_cmd2(svr, proc_name, &cmd);
	if (ret == SW_OK)
	{
		ret = cmd.status;
	}
	pub_log_info("send cmd(.type=SW_GET_LSN_ROUTE_PRDT .dst_prdt=%s) to %s, ret=[%d]", prdtname, lsn_name, ret);	

	return ret;
}

/******************************************************************************
 ** Name : uni_svr_print_all_in_prdt
 ** Desc : 打印平台所有产品的所有进程信息
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # gxx # 2016.06.13 #
 ******************************************************************************/
sw_int_t uni_svr_print_in_all_prdt(sw_uni_svr_t *svr)
{
	int	i = 0;
	int	count  = 0;
	char	status[32];
	const sw_prdt_cfg_t	*item  = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = NULL;

	shm_cfg	= (const sw_cfgshm_hder_t *)svr->shm_cfg;
	item  = cfg_get_prdt_cfg(shm_cfg);
	if (NULL == item)
	{
		pub_log_error("[%s][%d] Get product configuration faield!", __FILE__, __LINE__);
		return count;
	}

	for (i = 0; i < shm_cfg->prdt_use; i++)
	{
		uni_svr_print_proc_head();
		pub_log_info("[%s][%d] prdt_name[%s]", __FILE__, __LINE__, item->prdtname);
		memset(status, 0x0, sizeof(status));
		count = uni_svr_prdt_print_all_ext(svr, item, status);
		pub_log_info("[%s][%d] total number of process in product [%s] is [%d]", 
				__FILE__, __LINE__, item->prdtname, count);
		if (item->status == 0)
		{
			printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
			printf("\tPRODUCT:%s   STATUS:%s    TOTAL:%d  \n\n\n", item->prdtname, "OFF", count);
		}
		else
		{
			printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
			printf("\tPRODUCT:%s   STATUS:%s    TOTAL:%d  \n\n\n", item->prdtname, status, count);
		}
		item++;
	}

	return SW_OK;
}

static sw_int_t uni_svr_prdt_print_all_ext(sw_uni_svr_t *svr, const sw_prdt_cfg_t *prdt, char *status)
{
	sw_int_t j;
	sw_int_t idx;
	sw_int_t ret ;
	sw_int_t stoped_cnt   = 0;
	sw_int_t normal_cnt   = 0;
	sw_int_t abnormal_cnt = 0;
	sw_int_t total = 0;
	bool find = false;
	sw_proc_info_t info;
	sw_procs_head_t head;
	char lsn_name[NAME_LEN];
	sw_char_t svc_name[NAME_LEN];
	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return total;
	}
	for(idx = 0; idx < head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));
		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || (ND_LSN != info.type && ND_SVC != info.type))
		{
			continue;
		}
		if (ND_LSN == info.type)
		{
			find = false;
			for(j = 0; j < SW_PRDT_LSN_MAX; j++)
			{
				if(pub_str_isempty(prdt->lsnname[j]))
				{
					continue;
				}
				memset(lsn_name, 0x0, sizeof(lsn_name));
				snprintf(lsn_name, sizeof(lsn_name), "%s_%s", PROC_NAME_LSN, prdt->lsnname[j]);
				if(!strcasecmp(info.name, lsn_name))
				{
					find = true;
					if (SW_S_START == info.status)
					{
						ret = uni_svr_get_lsn_route_in_prdt(svr, prdt->prdtname, prdt->lsnname[j]);
						if (SW_ERROR != ret)
						{
							if (ret == 0)
							{
								info.status = SW_SWITCH_OFF;
							}
							else if (ret == 1)
							{
								info.status = SW_SWITCH_ON;
							}
						}
						else
						{
							info.status = SW_SWITCH_OFF;
						}
						if (info.status == SW_SWITCH_OFF)
						{
							stoped_cnt++;
						}
						else
						{
							normal_cnt++;
						}
					}
					else if (info.status == SW_S_STOPED)
					{
						stoped_cnt++;
					}
					else
					{
						abnormal_cnt++;
					}
					break;
				}
			}
			if(true == find)
			{
				uni_svr_print_proc(&info);
				total++;
			}
		}
		else if (ND_SVC == info.type)
		{
			snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, prdt->prdtname);
			if (0 == strcasecmp(info.name, svc_name))
			{
				uni_svr_print_proc(&info);
				if(info.status == SW_S_START)
				{
					normal_cnt++;
				}
				else if (info.status == SW_S_STOPED)
				{
					stoped_cnt++;
				}
				else
				{
					abnormal_cnt++;
				}
				total++;
			}
		}
	}
	if (abnormal_cnt > 0)
	{
		strcpy(status, "ABNORMAL");
	}
	else if (stoped_cnt == total || total == 0)
	{
		strcpy(status, "STOPED");
	}
	else
	{
		strcpy(status, "NORMAL");
	}
	return total;
}
/******************************************************************************
 ** Name : uni_svr_prdt_print_all
 ** Desc : 打印产品相关所有进程信息
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.25 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_print_all( sw_uni_svr_t *svr, const char *prdt_name)
{
	int count = 0;

	uni_svr_print_proc_head();

	count = uni_svr_prdt_print_all_lsn( svr, prdt_name);
	count += uni_svr_prdt_print_all_svc( svr, prdt_name); 

	uni_svr_print_tail(count);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_prdt_print_all_lsn
 ** Desc : 打印指定产品下的所有侦听进程信息
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 打印信息的条数
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_print_all_lsn( sw_uni_svr_t *svr, const char *prdt_name)
{
	bool find = false;
	int idx=0, j=0, ret=0, total=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	char lsn_name[NAME_LEN] = {0};
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return total;
	}

	/* 3. Print system process information */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product configuration faield!", __FILE__, __LINE__);
		return total;
	}

	find = false;
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 == strcasecmp(prdt_name, item->prdtname))
		{
			find = true;
			break;
		}
		item++;
	}

	if(false == find)
	{
		return total;
	}

	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || (ND_LSN != info.type))
		{
			continue;
		}

		find = false;
		for(j=0; j<SW_PRDT_LSN_MAX; j++)
		{
			if(pub_str_isempty(item->lsnname[j]))
			{
				continue;
			}

			snprintf(lsn_name, sizeof(lsn_name), "%s_%s", PROC_NAME_LSN, item->lsnname[j]);
			if(!strcasecmp(info.name, lsn_name))
			{
				find = true;
				break;
			}
		}

		if(true == find)
		{
			uni_svr_print_proc(&info);
			total++;
		}
	}

	return total;
}

/******************************************************************************
 ** Name : uni_svr_prdt_print_all_svc
 ** Desc : 打印指定指定下所有SVC相关信息
 ** Input: 
 **     cycle: The cycle of process
 **     svr: Object of sw_uni_svr_t
 **     prdt_name: product name
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.26 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_print_all_svc( sw_uni_svr_t *svr, const char *prdt_name)
{
	int idx=0, ret=0, total=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	const sw_prdt_cfg_t *item = NULL;
	char svc_name[FILE_NAME_MAX_LEN] = {0};
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return total;
	}

	/* 3. Print system process information */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product configuration faield!", __FILE__, __LINE__);
		return total;
	}

	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 == strcasecmp(prdt_name, item->prdtname))
		{
			break;
		}
		item++;
	}

	snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, prdt_name);

	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || (ND_SVC != info.type)
				|| (0 != strcasecmp(info.name, svc_name)))
		{
			continue;
		}

		uni_svr_print_proc(&info);
		total++;
	}

	return total;
}

/******************************************************************************
 ** Name : uni_svr_prdt_print_child
 ** Desc : Print all child process of special svr and prdt.
 ** Input: 
 **		svrname: Name of svr.
 ** Output: NONE
 ** Return: Total of print child.
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.11 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_print_child(const char *svr_name, const char *prdt_name)
{
	sw_int_t ret=0, idx=0, total=0;
	sw_proc_info_t info;
	const sw_svr_grp_t *grp = NULL;

	grp = procs_get_svr_by_name(svr_name, prdt_name);
	if(NULL == grp)
	{
		printf("Didn't found!\n");
		return 0;
	}

	for(idx=0; idx<grp->svc_curr; idx++)
	{
		ret = procs_get_proces_by_index(grp, idx, &info);
		printf("name:%s svr:%s", info.name, info.svr_name);
		if(SW_OK != ret)
		{
			return 0;
		}

		if(0 == strcmp(info.svr_name, svr_name))
		{
			uni_svr_print_proc(&info);
			total++;
		}
	}

	return total;
}

/******************************************************************************
 ** Name : uni_svr_prdt_print_lsn_child
 ** Desc : Print listen child process of special product.
 ** Input: 
 **     cycle: Cycle of process
 **		prdt_name: Name of product.
 ** Output: NONE
 ** Return: Total of print child.
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.11 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_print_lsn_child(const sw_cycle_t *cycle, const char *prdt_name)
{
	bool find = false;
	int idx=0, j=0, ret=0, total=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	char lsn_name[NAME_LEN] = {0};
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)cycle->shm_cfg;

	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return total;
	}

	/* 3. Print system process information */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product configuration faield!", __FILE__, __LINE__);
		return total;
	}

	find = false;
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 == strcasecmp(prdt_name, item->prdtname))
		{
			find = true;
			break;
		}
		item++;
	}

	if(false == find)
	{
		return total;
	}

	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || (ND_LSN != info.type))
		{
			continue;
		}

		find = false;
		for(j=0; j<SW_PRDT_LSN_MAX; j++)
		{
			if(pub_str_isempty(item->lsnname[j]))
			{
				continue;
			}

			snprintf(lsn_name, sizeof(lsn_name), "%s_%s", PROC_NAME_LSN, item->lsnname[j]);
			if(!strcasecmp(info.name, lsn_name))
			{
				find = true;
				break;
			}
		}

		if(true == find)
		{
			total += uni_svr_prdt_print_child(lsn_name, prdt_name);
		}
	}

	return total;
}

/******************************************************************************
 ** Name : uni_svr_prdt_print_svc_child
 ** Desc : Print svc child process of special product.
 ** Input: 
 **     cycle: Cycle of process.
 **		prdt_name: Name of product.
 ** Output: NONE
 ** Return: Total of print child.
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.11 #
 ******************************************************************************/
sw_int_t uni_svr_prdt_print_svc_child(const sw_uni_svr_t *svr, const char *prdt_name)
{
	int idx=0, ret=0, total=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	const sw_prdt_cfg_t *item = NULL;
	char svc_name[FILE_NAME_MAX_LEN] = {0};
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return total;
	}

	/* 3. Print system process information */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product configuration faield!", __FILE__, __LINE__);
		return total;
	}

	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		if(0 == strcasecmp(prdt_name, item->prdtname))
		{
			break;
		}
		item++;
	}

	snprintf(svc_name, sizeof(svc_name), "%s_%s", PROC_NAME_SVC_MAN, prdt_name);

	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || (ND_SVC != info.type)
				|| (0 != strcasecmp(info.name, svc_name)))
		{
			continue;
		}

		total += uni_svr_prdt_print_child(svc_name, prdt_name);
	}

	return total;
}

/******************************************************************************
 ** Name : uni_svr_print_all
 ** Desc : 打印所有系统进程信息
 ** Input: 
 **     cycle: The cycle of admin
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
void uni_svr_print_all()
{
	int count = 0;

	uni_svr_print_proc_head();

	count = uni_svr_print_all_sys();

	uni_svr_print_tail(count);
}

/******************************************************************************
 ** Name : uni_svr_print_all_product
 ** Desc : 打印所有产品
 ** Input: 
 **     cycle: The cycle of admin
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
void uni_svr_print_all_product( sw_uni_svr_t *svr)
{
	sw_int_t idx = 0;
	sw_proc_info_t info;
	const sw_prdt_cfg_t *item = NULL;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	memset(&info, 0, sizeof(info));

	/* 1. Get the address of product information */
	item = cfg_get_prdt_cfg(shm_cfg);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Get product information failed!", __FILE__, __LINE__);
		return;
	}

	printf("\n\t%-8s %-23s %-15s\n", "No.", "PRODUCT", "STATUS");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	/* 2. Traverse all of products */
	for(idx=0; idx<shm_cfg->prdt_use; idx++)
	{
		printf("\t[%02d]%-4s %-23s %-15d\n", (int)idx+1, "", item->prdtname, item->status);
		item++;
	}

	uni_svr_print_tail(shm_cfg->prdt_use);

	return;
}

/******************************************************************************
 ** Name : uni_svr_print_all_sys
 ** Desc : 打印所有系统进程
 ** Input: 
 **     cycle: The cycle of admin
 **     svr: Object of sw_uni_svr_t
 ** Output: NONE
 ** Return: Count of result
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t uni_svr_print_all_sys()
{
	int idx=0, ret=0, count=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	uni_proc_item_t item;

	memset(&item, 0, sizeof(item));
	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. Print system process information */
	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || ('\0' == info.name[0]))
		{
			continue;
		}

		uni_svr_print_proc(&info);
		count++;
	}

	return count;
}

/******************************************************************************
 ** Name : uni_svr_print_sys
 ** Desc : 打印指定类型系统进程信息
 ** Input: 
 **     cycle: The cycle of admin
 **     svr: Object of sw_uni_svr_t
 **     type: The type of process. Range: ND_LSN~ND_ALERT
 ** Output: NONE
 ** Return: 打印条数
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
sw_int_t uni_svr_print_sys( int type)
{
	int idx=0, ret=0, count=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	uni_proc_item_t item;

	memset(&item, 0, sizeof(item));
	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Print system process information */
	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if((SW_OK != ret) || ('\0' == info.name[0]))
		{
			continue;
		}

		if(type == info.type)
		{
			uni_svr_print_proc(&info);
			count++;
		}
	}

	return count;
}

/******************************************************************************
 ** Name : uni_svr_mtype_print_head
 ** Desc : 打印Mtype信息头部
 ** Input: NONE
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
void uni_svr_mtype_print_head(void)
{
	printf("\n\t%-6s %-5s %-4s %-16s %-16s %-16s %-16s %-16s %-8s %-8s %-16s\n",
			"No.", "MTYPE", "STAT", "PRDT", "SVR", "SVC", "LSN", "SAF", "TRCNo.", "BUSNo.", "STIME");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
}

/******************************************************************************
 ** Name : uni_svr_mtype_print_item
 ** Desc : 打印Mtype信息项
 ** Input: 
 **     idx: 打印序号
 **     item: 需要被打印的Mtype项
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
bool uni_svr_mtype_print_item(int idx, const sw_mtype_node_t *item, sw_int32_t flag)
{
	char stime[TIME_MAX_LEN] = {0};

	memset(stime, 0, sizeof(stime));

	pub_change_time2(item->start_time, stime, 0);

	switch(flag)
	{
		case MTYPE_IDLE:
		case MTYPE_USED:
			{
				if(flag != item->flag)
				{
					return false;
				}
				break;
			}
		case MTYPE_ALL:
		default:
			{
				/* Default: display everyone */
				break;
			}
	}
	printf("\t[%04d] %05d %-4s %-16s %-16s %-16s %-16s %-16s %08lld %08lld %-16s\n",
			(int)idx, (int)item->mtype, mtype_get_flag_desc(item->flag),
			item->prdt_name, item->server, item->service, item->lsn_name,
			item->saf_flow, (long long)item->trace_no, (long long)item->bi_no, stime);
	return true;
}

/******************************************************************************
 ** Name : uni_svr_mtype_print_tail
 ** Desc : 打印Mtype信息尾部
 ** Input: 
 **     total: 打印统计总数
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
void uni_svr_mtype_print_tail(int total)
{
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d\n", total);
}

/******************************************************************************
 ** Name : uni_svr_mtype_list_all
 ** Desc : 显示所有MTYPE信息
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Uni svr object
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
sw_int_t uni_svr_mtype_list_all()
{
	bool bret = false;
	sw_int32_t idx = 0, total = 0;
	const sw_mtype_t *addr = mtype_get_addr();
	const sw_mtype_head_t *head = &addr->head;
	const sw_mtype_node_t *item = &addr->first;
	sw_int32_t lock_id = head->lock_id;


	uni_svr_mtype_print_head();

	sem_mutex_lock(lock_id);

	for(idx=0; idx<head->mtype_max; idx++)
	{
		bret = uni_svr_mtype_print_item(idx+1, item, MTYPE_ALL);
		if(bret)
		{
			total++;
		}
		item++;
	}

	sem_mutex_unlock(lock_id);

	uni_svr_mtype_print_tail(total);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_mtype_list_busy
 ** Desc : 显示所有被占用MTYPE信息
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Uni svr object
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.22 #
 ******************************************************************************/
sw_int_t uni_svr_mtype_list_busy()
{
	bool bret = 0;
	sw_int32_t idx = 0, total = 0;
	const sw_mtype_t *addr = mtype_get_addr();
	const sw_mtype_head_t *head = &addr->head;
	const sw_mtype_node_t *item = &addr->first;
	sw_int32_t lock_id = head->lock_id;


	uni_svr_mtype_print_head();

	sem_mutex_lock(lock_id);

	for(idx=0; idx<head->mtype_max; idx++)
	{
		bret = uni_svr_mtype_print_item(idx+1, item, MTYPE_USED);
		if(bret)
		{
			total++;
		}
		item++;
	}

	sem_mutex_unlock(lock_id);

	uni_svr_mtype_print_tail(total);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_mtype_list_free
 ** Desc : 显示所有未占用MTYPE信息
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Uni svr object
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.22 #
 ******************************************************************************/
sw_int_t uni_svr_mtype_list_free()
{
	bool bret = 0;
	sw_int32_t idx = 0, total = 0;
	const sw_mtype_t *addr = mtype_get_addr();
	const sw_mtype_head_t *head = &addr->head;
	const sw_mtype_node_t *item = &addr->first;
	sw_int32_t lock_id = head->lock_id;


	uni_svr_mtype_print_head();

	sem_mutex_lock(lock_id);

	for(idx=0; idx<head->mtype_max; idx++)
	{
		bret = uni_svr_mtype_print_item(idx+1, item, MTYPE_IDLE);
		if(bret)
		{
			total++;
		}
		item++;
	}

	sem_mutex_unlock(lock_id);

	uni_svr_mtype_print_tail(total);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_mtype_list
 ** Desc : 打印所有Mtype信息
 ** Input: 
 **     mtype: 起始Mtype值
 **     num: 显示的Mtype个数
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
sw_int_t uni_svr_mtype_list(int mtype, int num)
{
	bool bret = false;
	sw_int32_t idx = 0, total = 0;
	const sw_mtype_t *addr = mtype_get_addr();
	const sw_mtype_node_t *item = &addr->first;
	const sw_mtype_head_t *head = &addr->head;
	sw_int32_t lock_id = head->lock_id;


	item += (mtype - 1);

	uni_svr_mtype_print_head();

	/*lock*/
	sem_mutex_lock(lock_id);

	for(idx=0; idx<num; idx++)
	{
		bret = uni_svr_mtype_print_item(idx+1, item, MTYPE_ALL);
		if(bret)
		{
			total++;
		}
		item++;
	}	
	sem_mutex_unlock(lock_id);

	uni_svr_mtype_print_tail(total);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_res_print_shm
 ** Desc : 打印SHM信息
 ** Input: 
 **     idx: 索引号
 **     shmid: 共享内存ID
 **     shmds: 共享内存信息
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
void uni_svr_res_print_shm(sw_int_t idx, int shmid, const struct shmid_ds *shmds)
{
#if defined(LINUX)
	printf("\t[%02d]%-4s 0x%08x %-12d %-12o %-12d %-12d\n",
			(int)idx, "", shmds->shm_perm.__key, shmid,
			shmds->shm_perm.mode, (int)shmds->shm_segsz, (int)shmds->shm_nattch);
#elif defined(AIX) || defined(SOLARIS)
	printf("\t[%02d]%-4s 0x%08x %-12d %-12o %-12d %-12d\n",
			(int)idx, "", 0x00, shmid,
			shmds->shm_perm.mode, (int)shmds->shm_segsz, (int)shmds->shm_nattch);
#else
	printf("\tOperation system not support!\n");
#endif
}

/******************************************************************************
 ** Name : uni_svr_res_print_shm_by_id
 ** Desc : 打印运行时SHM信息
 ** Input: 
 **     idx: 打印索引号
 **     shmid: 共享内存ID
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
sw_int_t uni_svr_res_print_shm_by_id(sw_int_t idx, int shmid)
{
	sw_int_t ret = SW_ERR;
	struct shmid_ds shmds;

	ret = shmctl(shmid, IPC_STAT, &shmds);
	if(ret < 0)
	{
		printf("\n\tGet SHM information failed!\n");
		pub_log_error("[%s][%d]shmid:%d errmsg:[%d]%s!",
				__FILE__, __LINE__, shmid, errno, strerror(errno));
		return SW_ERR;
	}

	uni_svr_res_print_shm(idx, shmid, &shmds);
	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_res_list_shm
 ** Desc : 打印所有SHM信息
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Svr object
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
sw_int_t uni_svr_res_list_shm(sw_uni_svr_t *svr)
{
	sw_int32_t shmid = -1;
	sw_int_t idx=0, ret=0, total=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	struct shmid_ds shmds;
	const sw_cfgshm_hder_t *shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;
	sw_syscfg_t *syscfg = ( sw_syscfg_t *)BLOCKAT(shm_cfg,shm_cfg->sys_offset);

	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));
	memset(&shmds, 0, sizeof(shmds));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	printf("\n\t%-8s %-10s %-12s %-12s %-12s %-12s\n",
			"No.", "KEY", "SHMID", "PERMS", "BYTES", "NATTCH");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	/* 1. Print runtime SHM */
	shmid = run_get_shmid();
	if(shmid > 0)
	{
		ret = uni_svr_res_print_shm_by_id(++total, shmid);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Print shm failed! shmid:%d", __FILE__, __LINE__, shmid);
			return SW_ERR;
		}
	}

	/* 2. Print vars SHM */
	shmid = syscfg->vars_shmid;
	if(shmid > 0)
	{
		ret = uni_svr_res_print_shm_by_id(++total, shmid);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Print shm failed! shmid:%d", __FILE__, __LINE__, shmid);
			return SW_ERR;
		}
	}

	/* 3. Print process SHM */
	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if(SW_OK != ret)
		{
			printf("\n\tGet system information failed!\n");
			pub_log_error(
					"[%s][%d] Get system information by index failed!", __FILE__, __LINE__);
			return SW_ERR;
		}
		else if(0 == info.shmid)
		{
			pub_log_info("[%s][%d] Didn't found SHM. proc:%s",
					__FILE__, __LINE__, info.name);
			continue;
		}

		ret = uni_svr_res_print_shm_by_id(++total, info.shmid);
		if(SW_OK != ret)
		{
			pub_log_error(
					"[%s][%d] Print shm failed! shmid:%d", __FILE__, __LINE__, shmid);
			return SW_ERR;
		}        
	}

	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d\n", (int)total);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_res_print_msq
 ** Desc : 打印MSQ信息
 ** Input: 
 **     idx: 索引号
 **     msqid: 信号量ID
 **     msqds: 信号量信息
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
void uni_svr_res_print_msq(sw_int_t idx, int msqid, const struct msqid_ds *msqds)
{
#if defined(LINUX)
	printf("\t[%02d]%-4s 0x%08x %-12d %-12o %-12ld %-12ld\n",
			(int)idx, "", msqds->msg_perm.__key, msqid,
			msqds->msg_perm.mode, msqds->msg_cbytes, msqds->msg_qnum);
#elif defined(AIX) || defined(SOLARIS)
	printf("\t[%02d]%-4s 0x%08x %-12d %-12o %-12ld %-12ld\n",
			(int)idx, "",0x00, msqid,
			msqds->msg_perm.mode, msqds->msg_cbytes, msqds->msg_qnum);
#else
	printf("\tOperation system not support!\n");
#endif
}

/******************************************************************************
 ** Name : uni_svr_res_list_msq
 ** Desc : 打印所有MSQ信息
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Svr object
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
sw_int_t uni_svr_res_list_msq()
{
	sw_int_t idx=0, ret=0, total=0;
	sw_proc_info_t info;
	sw_procs_head_t head;
	struct msqid_ds msqds;

	memset(&info, 0, sizeof(info));
	memset(&head, 0, sizeof(head));
	memset(&msqds, 0, sizeof(msqds));

	/* 1. Get the head information of process table */
	ret = procs_get_head(&head);
	if(SW_OK != ret)
	{
		pub_log_error(
				"[%s][%d] Get the head information of process failed!", __FILE__, __LINE__);
		return 0;
	}

	printf("\n\t%-8s %-10s %-12s %-12s %-12s %-12s\n",
			"No.", "KEY", "MSQID", "PERMS", "USED-BYTES", "MESSAGES");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	for(idx=0; idx<head.sys_proc_use; idx++)
	{
		memset(&info, 0, sizeof(info));

		ret = procs_get_sys_by_index(idx, &info);
		if(SW_OK != ret)
		{
			pub_log_error(
					"[%s][%d] Get system information by index failed! proc:%s",
					__FILE__, __LINE__, info.name);
			return SW_ERR;
		}
		else if(0 == info.mqid)
		{
			pub_log_info("[%s][%d] Didn't found MSQ. proc:%s",
					__FILE__, __LINE__, info.name);
			continue;
		}

		ret = msgctl(info.mqid, IPC_STAT, &msqds);
		if(-1 == ret)
		{
			pub_log_error("[%s][%d] proc:%s shmid:%d errmsg:[%d]%s!",
					__FILE__, __LINE__, info.name, info.mqid, errno, strerror(errno));
			continue;
		}

		uni_svr_res_print_msq(++total, info.mqid, &msqds);
	}

	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d\n", (int)total);

	return SW_OK;
}

/******************************************************************************
 ** Name : uni_svr_res_clean_vars
 ** Desc : 清理共享内存资源
 ** Input: 
 **     shmid: Id of share memory
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
sw_int_t uni_svr_res_clean_vars(sw_int32_t shmid)
{
	sw_int_t ret = SW_ERR;
	struct shmid_ds shmds;

	memset(&shmds, 0, sizeof(shmds));

	ret = shmctl(shmid, IPC_STAT, &shmds);
	if(0 != ret)
	{
		pub_log_error("[%s][%d] Get shm stat failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	if(0 != shmds.shm_nattch)
	{
		pub_log_error("[%s][%d] Clean vars SHM failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return shmctl(shmid, IPC_RMID, NULL);
}

/******************************************************************************
 ** Name : uni_svr_res_clean_sem
 ** Desc : 清理信号量集资源
 ** Input: NONE
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
sw_int_t uni_svr_res_clean_sem(void)
{
	sw_int_t ret = SW_ERR;
	sw_int32_t semid = 0;
	sw_sem_lock_t* sem = NULL;
	struct semid_ds semds;

	memset(&semds, 0, sizeof(semds));

	sem = run_get_sem_lock();
	semid = sem->sem_id;

	ret = semctl(semid, 0, IPC_STAT, &semds);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] Get semaphore stat failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return semctl(semid, 0, IPC_RMID, 0);
}

/******************************************************************************
 ** Name : uni_svr_res_clean
 ** Desc : 清理平台资源
 ** Input: 
 **     cycle: Cycle of process
 **     svr: Svr object
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
sw_int_t uni_svr_res_clean( sw_int32_t shmid)
{
	sw_int_t ret = SW_ERR;
	struct shmid_ds shmds;

	memset(&shmds, 0, sizeof(shmds));


	/* 1. 清理变量池共享内存资源 */
	if(shmid > 0)
	{
		ret = uni_svr_res_clean_vars(shmid);
		if(SW_OK != ret)
		{
			printf("\n\tClean vars failed!\n\n");
			pub_log_error("[%s][%d] Clean share-memory of vars failed!", __FILE__, __LINE__);
			return SW_ERR;
		}
		printf("\n\tClean vars success\n\n");
	}

	/* 2. 清理信号量 */
	ret = uni_svr_res_clean_sem();
	if(SW_OK != ret)
	{
		printf("\n\tClean semaphore failed\n\n");
		pub_log_error("[%s][%d] Clean semaphore failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	printf("\n\tClean semaphore success\n\n");

	/* 2. 清理运行时共享内存 */
	shmid = run_get_shmid();
	if(shmid > 0)
	{
		ret = shmctl(shmid, IPC_STAT, &shmds);
		if(0 != ret)
		{
			pub_log_error("[%s][%d] Get shm stat failed!", __FILE__, __LINE__);
			return SW_ERR;        
		}

		if(shmds.shm_nattch > 1)
		{
			printf("\n\tClean failed\n\n");
			pub_log_error("[%s][%d] Some process still attch runtime SHM!", __FILE__, __LINE__);
			return SW_ERR;
		}

		ret = shmctl(shmid, IPC_RMID, 0);
		if(ret < 0)
		{
			printf("errmsg:[%d]%s\n", errno, strerror(errno));
			pub_log_error("[%s][%d] errmsg:[%d]%s",
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;        
		}

		printf("\n\tClean success. exit!\n\n");

		exit(0);
	}

	return SW_OK;
}


sw_int_t uni_svr_task_get_head(sw_job_head_t *head)
{
	sw_job_t *job;

	job = job_get_addr();
	if (job == NULL )
	{
		pub_log_error("[%s][%d] can't get job addr",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memcpy(head,&job->head,sizeof(sw_job_head_t));	

	return SW_OK;
}

sw_int_t uni_svr_task_get_item_by_no(sw_char_t *no,sw_job_item_t *item)
{
	sw_int32_t idx;
	sw_job_t *job;

	job = job_get_addr();
	if (job == NULL )
	{
		pub_log_error("[%s][%d] can't get job addr",__FILE__,__LINE__);
		return SW_ERROR;
	}

	for (idx = 0; idx < job->head.cur_cnt;idx++)
	{
		if (strcmp(job->job_item[idx].no,no) == 0)
		{
			pub_mem_memcpy(item,&job->job_item[idx],sizeof(sw_job_item_t));
			return SW_OK;
		}
	}

	return SW_ERROR;
}

sw_int_t uni_svr_task_get_item_by_idx(sw_int32_t idx,sw_job_item_t *item)
{
	sw_job_t *job;

	job = job_get_addr();
	if (job == NULL )
	{
		pub_log_error("[%s][%d] can't get job addr",__FILE__,__LINE__);
		return SW_ERROR;
	}

	if (idx >= 0 
			&& idx < job->head.cur_cnt)
	{
		pub_mem_memcpy(item,&job->job_item[idx],sizeof(sw_job_item_t));
		return SW_OK;
	}


	return SW_ERROR;
}


sw_int_t uni_svr_task_start_item( sw_uni_svr_t *svr, const sw_char_t *no)
{
	sw_int_t ret = SW_ERR;

	ret = uni_svr_proc_is_exist(PROC_NAME_JOB);
	if(true != ret)
	{
		pub_log_error("[%s][%d] %s is not running.", __FILE__, __LINE__, PROC_NAME_JOB);
		return SW_ERR;
	}

	ret = uni_svr_send_cmd(svr, SW_MSTARTTASK, PROC_NAME_JOB, no);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Send cmd to [%s] failed.", __FILE__, __LINE__, PROC_NAME_JOB);
		return SW_ERR;
	}

	return SW_OK;
}


int uni_svr_deploy_bp_by_cfgfile(sw_uni_svr_t *svr, const char *path)
{
	sw_xmltree_t	*xmltree = NULL;
	sw_xmlnode_t	*node = NULL;

	xmltree = pub_xml_crtree((char *)path); 
	if (xmltree == NULL) 
	{
		pub_log_error("%s:%d pub_xml_crtree(%s) error", 
				__FILE__, __LINE__, path);
		return -1;
	}

	pub_log_debug("%s:%d:%s processing file %s",
			__FILE__, __LINE__, __FUNCTION__, path);

	node = pub_xml_locnode(xmltree, ".LIST");
	if (node == NULL)
	{
		pub_log_error(".LIST node does not exist");
		pub_xml_deltree(xmltree);
		return -1;
	}

	node = node->firstchild;
	if (node == NULL)
	{
		pub_log_error(".LIST node does not have any child");
		pub_xml_deltree(xmltree);
		return -1;
	}

	while (node)
	{
		if (0 == strcmp(node->name, "LSN"))
			deploy_bp_process_lsn(svr, xmltree, node);
		else if (0 == strcmp(node->name, "PRDT"))
			deploy_bp_process_prdt(svr, xmltree, node);
		else
			pub_log_error("expect(LSN)or(PRDT), rather than(%s)", 
					node->name);

		node = node->next;
	}

	pub_xml_deltree(xmltree);
	pub_log_debug("%s:%d:%s processing file %s end",
			__FILE__, __LINE__, __FUNCTION__, path);
	return 0;
}

static int deploy_bp_read_info(
		sw_xmltree_t *tree, 
		sw_xmlnode_t *node,
		const char **NAME_value, 
		const char **ATTR_value)
{
	sw_xmlnode_t *subnode;

	tree->current = node;
	subnode = pub_xml_locnode(tree, "NAME");
	if (subnode == NULL)
		return -1;
	*NAME_value = subnode->value;
	if (*NAME_value == NULL || **NAME_value == 0)
		return -1;

	subnode = pub_xml_locnode(tree, "ATTR");
	if (subnode == NULL)
		return -1;
	*ATTR_value = subnode->value;
	if (*ATTR_value == NULL || **ATTR_value == 0)
		return -1;

	return 0;
}

static void deploy_bp_process_lsn(
		sw_uni_svr_t *svr,
		sw_xmltree_t *tree, 
		sw_xmlnode_t *node)
{
	const char * NAME_value;
	const char * ATTR_value;
	char lsn_process_name[128];

	assert (0 == strcmp(node->name, "LSN"));

	if (0 != deploy_bp_read_info(tree, node, &NAME_value, &ATTR_value))
	{
		pub_log_error("%s:%d error config", __FILE__, __LINE__);
		return;
	}

	/* 2 == strlen '_' + '\0' */
	if (  (strlen(PROC_NAME_LSN) + strlen(NAME_value) + 2)
			< sizeof lsn_process_name)
	{
		strcpy(lsn_process_name, PROC_NAME_LSN);
		strcat(lsn_process_name, "_");
		strcat(lsn_process_name, NAME_value);
	}
	else
	{
		pub_log_error("%s:%d name(%s) too long",
				__FILE__, __LINE__, NAME_value);
		return;
	}

	if (0 == strcmp(ATTR_value, "ADD"))
	{
		int rc = (int) uni_svr_start_lsn(svr, NAME_value);
		pub_log_info("called uni_svr_start_lsn(svr, \"%s\") == %d", 
				NAME_value, rc);
	}
	else if (0 == strcmp(ATTR_value, "DEL"))
	{
		int rc = (int) uni_svr_stop_process(svr, SW_MSTOPONE, lsn_process_name);
		pub_log_info("called uni_svr_stop_process(svr, SW_MSTOPONE, \"%s\") == %d",
				lsn_process_name, rc);
	}
	else if (0 == strcmp(ATTR_value, "UPD"))
	{
		sw_cmd_t cmd;

		memset(&cmd, 0x00, sizeof cmd);
		cmd.type = SW_UPD_LSN;
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);

		uni_svr_send_cmd2(svr, lsn_process_name, &cmd);
		pub_log_info("send cmd(.type=SW_UPD_LSN) to %s", lsn_process_name);
	}
	else if (0 == strcmp(ATTR_value, "NONE"))
		; /* nothing to do, just skip */
	else
	{
		pub_log_error("expect(ADD|DEL|UPD|NONE), rather than %s",
				__FILE__, __LINE__, ATTR_value);
		return;
	}

	pub_log_debug("processing LSN(%s)(%s) success", NAME_value, ATTR_value);

	tree->current = node;
	node = pub_xml_locnode(tree, "PRDT");
	if (node)
		deploy_bp_process_lsn_prdt(svr, lsn_process_name, tree, node);
}

static void deploy_bp_process_lsn_prdt(
		sw_uni_svr_t *svr,
		const char *lsn_process_name,
		sw_xmltree_t *tree,
		sw_xmlnode_t *prdtnode)
{
	sw_cmd_t cmd;
	const char *NAME_value; 
	const char *ATTR_value;

	while (prdtnode)
	{
		/* do NOT remove even currently it's pointless */
		if (0 != strcmp(prdtnode->name, "PRDT"))
		{
			prdtnode = prdtnode->next;
			continue;
		}

		if (0 != deploy_bp_read_info(tree, prdtnode, 
					&NAME_value, &ATTR_value))
		{
			pub_log_error("%s:%d error config", __FILE__, __LINE__);
			return;
		}

		memset(&cmd, 0x00, sizeof cmd);
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		if (strlen(NAME_value) < sizeof(cmd.dst_prdt))
		{
			strcpy(cmd.dst_prdt, NAME_value);
		}
		else
		{
			pub_log_error("strlen(%s) is longer than sizeof(cmd.dst_prdt)",
					NAME_value);
			prdtnode = prdtnode->next;
			continue;
		}

		if (0 == strcmp(ATTR_value, "ADD"))
			cmd.type = SW_ADD_PRDT;
		else if (0 == strcmp(ATTR_value, "DEL"))
			cmd.type = SW_DEL_PRDT;
		else if (0 == strcmp(ATTR_value, "UPD"))
			cmd.type = SW_UPD_PRDT;
		else
		{
			pub_log_error("error config : %s is not supported", 
					ATTR_value);
			prdtnode = prdtnode->next;
			continue;
		}

		uni_svr_send_cmd2(svr, lsn_process_name, &cmd);

		pub_log_info("send cmd(.type=SW_%s_PRDT .dst_prdt=%s) to %s",
				ATTR_value, NAME_value, lsn_process_name);	
		pub_log_debug("processing LSN.PRDT(%s)(%s) success",
				NAME_value, ATTR_value);

		prdtnode = prdtnode->next;
	}
}

static void deploy_bp_process_prdt(
		sw_uni_svr_t *svr,
		sw_xmltree_t *tree,
		sw_xmlnode_t *node)
{
	const char *NAME_value;
	const char *ATTR_value;
	char prdt_process_name[128];

	assert (0 == strcmp(node->name, "PRDT"));

	if (0 != deploy_bp_read_info(tree, node, &NAME_value, &ATTR_value))
	{
		pub_log_error("%s:%d error config", __FILE__, __LINE__);
		return;
	}

	if (  (strlen(PROC_NAME_SVC_MAN) + strlen(NAME_value) + 2)
			< sizeof prdt_process_name)
	{
		strcpy(prdt_process_name, PROC_NAME_SVC_MAN);
		strcat(prdt_process_name, "_");
		strcat(prdt_process_name, NAME_value);
	}
	else
	{
		pub_log_error("%s:%d name(%s) too long",
				__FILE__, __LINE__, NAME_value);
		return;
	}

	if (0 == strcmp(ATTR_value, "ADD"))
	{
		int rc = (int) uni_svr_start_prdt(svr, NAME_value);
		pub_log_info("called uni_svr_prdt_start_all(svr, \"%s\") == %d",
				NAME_value, rc);
		printf("重要提示：产品%s未注册，为了不影响您的正常使用，请您注册产品\n", NAME_value);
	}
	else if (0 == strcmp(ATTR_value, "DEL"))
	{
		int rc = (int) uni_svr_prdt_stop_all(svr, NAME_value);
		pub_log_info("called uni_svr_prdt_stop_all(svr, \"%s\") == %d",
				NAME_value, rc);
	}
	else if (0 == strcmp(ATTR_value, "UPD"))
		; /*just skip, UPD it's literal that for human reading */
	else
	{
		pub_log_error("expect(ADD|DEL|UPD), rather than %s",
				__FILE__, __LINE__, ATTR_value);
		return;
	}

	pub_log_debug("processing PRDT(%s)(%s) success", NAME_value, ATTR_value);

	tree->current = node;
	sw_xmlnode_t *svrnode = pub_xml_locnode(tree, "SVR");
	if (svrnode)
		deploy_bp_process_prdt_svr(svr, prdt_process_name, tree, svrnode);
}

static void deploy_bp_process_prdt_svr(
		sw_uni_svr_t *svr,
		const char *prdt_process_name,
		sw_xmltree_t *tree, 
		sw_xmlnode_t *svrnode)
{
	const char * NAME_value;
	const char * ATTR_value;
	sw_cmd_t cmd;

	while (svrnode)
	{
		if (0 != strcmp(svrnode->name, "SVR"))
		{
			svrnode = svrnode->next;
			continue;
		}

		if (0 != deploy_bp_read_info(tree, svrnode, &NAME_value, &ATTR_value))
		{
			pub_log_error("%s:%d error config", __FILE__, __LINE__);
			svrnode = svrnode->next;
			continue;
		}

		memset(&cmd, 0x00, sizeof cmd);
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		if (strlen(NAME_value) < sizeof(cmd.dst_svr))
		{
			strcpy(cmd.dst_svr, NAME_value);
		}
		else
		{
			pub_log_error("strlen(%s) is longer than sizeof(cmd.dst_svr)",
					NAME_value);
			svrnode = svrnode->next;
			continue;
		}

		if (0 == strcmp(ATTR_value, "ADD"))
			cmd.type = SW_ADD_SVR;
		else if (0 == strcmp(ATTR_value, "DEL"))
			cmd.type = SW_DEL_SVR;
		else if (0 == strcmp(ATTR_value, "UPD"))
			cmd.type = SW_UPD_SVR;
		else if (0 == strcmp(ATTR_value, "NONE"))
			; /* just skip */
		else
		{
			pub_log_error("%s:%d error config expect(ADD|DEL|UPD|NONE), rather than %s",
					__FILE__, __LINE__, ATTR_value);
			svrnode = svrnode->next;
			continue;
		}

		if (0 != strcmp(ATTR_value, "NONE"))
		{
			uni_svr_send_cmd2(svr, prdt_process_name, &cmd);

			pub_log_info("send cmd(.type=SW_%s_SVR .dst_svr=%s) to %s",
					ATTR_value, NAME_value, prdt_process_name);
		}

		pub_log_debug("processing PRDT.SVR(%s)(%s) success", 
				NAME_value, ATTR_value);

		tree->current = svrnode;
		sw_xmlnode_t *svcnode = pub_xml_locnode(tree, "SVC");
		if (svcnode)
			deploy_bp_process_prdt_svr_svc(svr, prdt_process_name,
					NAME_value, tree, svcnode);

		svrnode = svrnode->next;
	}

}

static void deploy_bp_process_prdt_svr_svc(
		sw_uni_svr_t *svr,
		const char *prdt_process_name,
		const char *svrname,
		sw_xmltree_t *tree, 
		sw_xmlnode_t *svcnode)
{
	const char * NAME_value;
	const char * ATTR_value;
	sw_cmd_t cmd;

	while (svcnode)
	{
		if (0 != strcmp(svcnode->name, "SVC"))
		{
			svcnode = svcnode->next;
			continue;
		}

		if (0 != deploy_bp_read_info(tree, svcnode, &NAME_value, &ATTR_value))
		{
			pub_log_error("%s:%d error config", __FILE__, __LINE__);
			svcnode = svcnode->next;
			continue;
		}

		memset(&cmd, 0x00, sizeof cmd);
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		strcpy(cmd.dst_svr, svrname); /* len checked in caller */
		if (strlen(NAME_value) < sizeof(cmd.dst_svc))
		{
			strcpy(cmd.dst_svc, NAME_value);
		}
		else
		{
			pub_log_error("strlen(%s) is longer than sizeof(cmd.dst_svc)",
					NAME_value);
			svcnode = svcnode->next;
			continue;
		}

		if (0 == strcmp(ATTR_value, "ADD"))
			cmd.type = SW_ADD_SVC;
		else if (0 == strcmp(ATTR_value, "DEL"))
			cmd.type = SW_DEL_SVC;
		else if (0 == strcmp(ATTR_value, "UPD"))
			cmd.type = SW_UPD_SVC;
		else
		{
			pub_log_error("%s:%d error config1", __FILE__, __LINE__);
			svcnode = svcnode->next;
			continue;
		}

		uni_svr_send_cmd2(svr, prdt_process_name, &cmd);
		pub_log_info("send cmd(.type=SW_%s_SVC .dst_svr=%s .dst_svc=%s) to %s",
				ATTR_value, svrname, NAME_value, prdt_process_name);
		pub_log_debug("processing PRDT.SVR.SVC(%s)(%s) success", 
				NAME_value, ATTR_value);

		svcnode = svcnode->next;
	}
}

int uni_svr_reload_lsn(
		sw_uni_svr_t *svr,
		const char *lsn_name,
		const char *lsn_action,
		const char *prdt_name,
		const char *prdt_action)
{
	char lsn_process_name[256];

	assert(lsn_name);
	snprintf(lsn_process_name, 256, "%s_%s", PROC_NAME_LSN, lsn_name); 

	if (lsn_action)
	{
		if (0 == strcmp(lsn_action, "add"))
		{
			int rc = (int) uni_svr_start_lsn(svr, lsn_name);
			pub_log_info("called uni_svr_start_lsn(svr, \"%s\") == %d", 
					lsn_name, rc);
		}
		else if (0 == strcmp(lsn_action, "del"))
		{
			int rc = (int) uni_svr_stop_process(svr, SW_MSTOPONE, lsn_process_name);
			pub_log_info("called uni_svr_stop_process(svr, SW_MSTOPONE, \"%s\") == %d",
					lsn_process_name, rc);
		}
		else if (0 == strcmp(lsn_action, "upd"))
		{
			sw_cmd_t cmd;

			memset(&cmd, 0x00, sizeof cmd);
			cmd.type = SW_UPD_LSN;
			snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);

			uni_svr_send_cmd2(svr, lsn_process_name, &cmd);
			pub_log_info("send cmd(.type=SW_UPD_LSN) to %s", lsn_process_name);
		}
		else
		{
			/* wrong usage */
			return -2;
		}

		pub_log_debug("%s LSN(%s) success", lsn_action, lsn_name);
	}

	if (prdt_name != NULL)
	{
		assert(prdt_action);

		sw_cmd_t cmd;
		memset(&cmd, 0x00, sizeof cmd);
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		strncpy(cmd.dst_prdt, prdt_name, sizeof(cmd.dst_prdt));
		if (0 == strcmp(prdt_action, "add"))
			cmd.type = SW_ADD_PRDT;
		else if (0 == strcmp(prdt_action, "del"))
			cmd.type = SW_DEL_PRDT;
		else if (0 == strcmp(prdt_action, "upd"))
			cmd.type = SW_UPD_PRDT;
		else
			return -2;

		uni_svr_send_cmd2(svr, lsn_process_name, &cmd);
		pub_log_info("send cmd(.type=SW_%s_PRDT .dst_prdt=%s) to %s",
				prdt_action, prdt_name, lsn_process_name);	
		pub_log_debug("%s LSN(%s).PRDT(%s) success",
				prdt_action, lsn_name, prdt_name);
	}

	return 0;
}

int uni_svr_reload_prdt(
		sw_uni_svr_t *svr,
		const char *prdt_name, 
		const char *prdt_action, 
		const char *svr_name, 
		const char *svr_action, 
		const char *svc_name, 
		const char *svc_action)
{
	char prdt_process_name[256];

	assert(prdt_name && prdt_action);

	snprintf(prdt_process_name, 256, "%s_%s", PROC_NAME_SVC_MAN, prdt_name); 

	/* process prdt */
	if (0 == strcmp(prdt_action, "add"))
	{
		int rc = (int) uni_svr_prdt_start_all(svr, prdt_name);
		pub_log_info("called uni_svr_prdt_start_all(svr, \"%s\") == %d",
				prdt_name, rc);
	}
	else if (0 == strcmp(prdt_action, "del"))
	{
		int rc = (int) uni_svr_prdt_stop_all(svr, prdt_name);
		pub_log_info("called uni_svr_prdt_stop_all(svr, \"%s\") == %d",
				prdt_name, rc);
	}
	else if (0 == strcmp(prdt_action, "upd"))
		; /*just skip, UPD is literal that for human reading */
	else if (0 == strcmp(prdt_action, "arg"))
	{
		sw_cmd_t	cmd;

		memset(&cmd, 0x0, sizeof(cmd));
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		cmd.type = SW_ARG_MODIFY;
		memcpy(cmd.dst_svr, svr_action, sizeof(cmd.dst_svr) - 1);/*set arg path*/
		uni_svr_send_cmd2(svr, prdt_process_name, &cmd);
		pub_log_info("[%s][%d]send cmd type[%d] to %s", __FILE__, __LINE__, cmd.type, prdt_process_name);
	}
	else
	{
		return -2;
	}

	if (0 != strcmp(prdt_action, "upd"))
		pub_log_debug("%s PRDT(%s) success", prdt_action, prdt_name);

	/* process prdt.svr */
	if (svr_name == NULL)
		return 0;
	if (0 == strcmp(svr_name, "-svc"))
	{
		/* error case */
		return -2;
	}
	if (svr_action)
	{
		sw_cmd_t cmd;
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		strncpy(cmd.dst_svr, svr_name, sizeof(cmd.dst_svr));
		if (0 == strcmp(svr_action, "add"))
			cmd.type = SW_ADD_SVR;
		else if (0 == strcmp(svr_action, "del"))
			cmd.type = SW_DEL_SVR;
		else if (0 == strcmp(svr_action, "upd"))
			cmd.type = SW_UPD_SVR;
		else
			return -2;

		uni_svr_send_cmd2(svr, prdt_process_name, &cmd);
		pub_log_info("send cmd(.type=SW_%s_SVR .dst_svr=%s) to %s",
				svr_action, svr_name, prdt_process_name);
		pub_log_debug("%s PRDT(%s).SVR(%s) success", 
				svr_action, prdt_name, svr_name);
	}

	/* process prdt.svr.svc */
	if (svc_name != NULL)
	{
		sw_cmd_t cmd;

		memset(&cmd, 0x00, sizeof cmd);
		snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
		strncpy(cmd.dst_svr, svr_name, sizeof(cmd.dst_svr)); 
		strncpy(cmd.dst_svc, svc_name, sizeof(cmd.dst_svc)); 
		if (0 == strcmp(svc_action, "add"))
			cmd.type = SW_ADD_SVC;
		else if (0 == strcmp(svc_action, "del"))
			cmd.type = SW_DEL_SVC;
		else if (0 == strcmp(svc_action, "upd"))
			cmd.type = SW_UPD_SVC;
		else
			return -2;

		uni_svr_send_cmd2(svr, prdt_process_name, &cmd);
		pub_log_info("send cmd(.type=SW_%s_SVC .dst_svr=%s .dst_svc=%s) to %s",
				svc_action, svr_name, svc_name, prdt_process_name);
		pub_log_debug("%s PRDT(%s).SVR(%s).SVC(%s) success", 
				svc_action, prdt_name, svr_name, svc_name);
	}

	return 0;
}

sw_int_t uni_svr_start_prdt(sw_uni_svr_t *svr, const sw_char_t *prdt_name)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_char_t	lsn_name[64];
	sw_char_t	proc_name[64];
	const sw_prdt_cfg_t	*item = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	item = cfg_get_prdt_cfg(shm_cfg);
	if (NULL == item)
	{
		pub_log_error("[%s][%d] Get prdt information failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i< shm_cfg->prdt_use; i++)
	{
		if (strcmp(item->prdtname, prdt_name) == 0)
		{
			break;
		}
		item++;
	}
	if (i == shm_cfg->prdt_use)
	{
		ret = cfg_add_prdt((sw_cfgshm_hder_t *)shm_cfg, (char *)prdt_name);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
			return SW_ERROR;
		}
		item = run_get_prdt(svr, prdt_name);
		if (item == NULL)
		{
			pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
			return SW_ERROR;
		}
	}

	for (i = 0; i < item->lsn_cnt; i++)
	{
		memset(lsn_name, 0x0, sizeof(lsn_name));
		strncpy(lsn_name, item->lsnname[i], sizeof(lsn_name) - 1);
		memset(proc_name, 0x0, sizeof(proc_name));
		sprintf(proc_name, "%s_%s", PROC_NAME_LSN, lsn_name);
		if (uni_svr_proc_is_exist(proc_name) == true)
		{
			/*** 如果进程存在,直接发送启动产品的指令 ***/
			ret = uni_svr_prdt_start_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, lsn_name);
				return SW_ERROR;
			}
		}
		else
		{
			/*** 如果进程不存在,则直接启动该进程 ***/
			ret = uni_svr_start_lsn(svr, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Start lsn [%s] failed!", __FILE__, __LINE__, lsn_name);
				return SW_ERROR;
			}
			pub_log_debug("[%s][%d] 启动[%s]成功!", __FILE__, __LINE__, lsn_name);

			sleep(1);
			/*** 防止配置文件中的状态没改,所以在此发起启动该产品的指令 ***/
			ret = uni_svr_prdt_start_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, lsn_name);
				return SW_ERROR;
			}
			pub_log_debug("[%s][%d] 启动产品[%s][%s]成功!", __FILE__, __LINE__, prdt_name, lsn_name);
		}
	}

	ret = uni_svr_start_svc(prdt_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Start [%s] error!", __FILE__, __LINE__, prdt_name);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t chnl_have_only_one_prdt(sw_uni_svr_t *svr, sw_char_t *lsn_name, sw_char_t *prdt_name)
{
	sw_int_t	i = 0;
	const sw_chl_cfg_t	*item = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	item = cfg_get_chnl_cfg(shm_cfg);
	if (item == NULL)
	{
		pub_log_error("[%s][%d] Get lsn information failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < shm_cfg->lsn_use; i++)
	{
		if (strcmp(item->lsnname, lsn_name) == 0)
		{
			break;
		}
		item++;
	}
	if (i == shm_cfg->lsn_use)
	{
		pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}

	for (i = 0; i < item->prdt_cnt; i++)
	{
		if (strcmp(item->prdtname[i], prdt_name) == 0)
		{
			break;
		}
	}
	if (i == item->prdt_cnt)
	{
		pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
		return SW_ERROR;
	}

	return item->prdt_cnt;
}

sw_int_t uni_svr_stop_prdt(sw_uni_svr_t *svr, sw_char_t *prdt_name)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_char_t	lsn_name[64];
	sw_char_t	proc_name[64];
	const sw_prdt_cfg_t	*item = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;

	item = cfg_get_prdt_cfg(shm_cfg);
	if (NULL == item)
	{
		pub_log_error("[%s][%d] Get prdt information failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i< shm_cfg->prdt_use; i++)
	{
		if (strcmp(item->prdtname, prdt_name) == 0)
		{
			break;
		}
		item++;
	}
	if (i == shm_cfg->prdt_use)
	{
		pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d] prdt_name=[%s] lsn_cnt=[%d]", __FILE__, __LINE__, prdt_name, item->lsn_cnt);
	for (i = 0; i < item->lsn_cnt; i++)
	{
		memset(lsn_name, 0x0, sizeof(lsn_name));
		strncpy(lsn_name, item->lsnname[i], sizeof(lsn_name) - 1);
		memset(proc_name, 0x0, sizeof(proc_name));
		sprintf(proc_name, "%s_%s", PROC_NAME_LSN, lsn_name);
		if (uni_svr_proc_is_exist(proc_name) != true)
		{
			pub_log_error("[%s][%d] Proc [%s] not exist!", __FILE__, __LINE__, proc_name);
			continue;
		}

		if (chnl_have_only_one_prdt(svr, lsn_name, prdt_name) == 1)
		{
			/*** 如果某个渠道仅仅只下挂一个产品,则先更新该产品的状态,再停止该侦听 ***/
			ret = uni_svr_prdt_stop_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, item->lsnname[i]);
				return SW_ERROR;
			}

			ret = uni_svr_stop_process(svr, SW_MSTOPONE, proc_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Stop [%s] error!", __FILE__, __LINE__, proc_name);
				return SW_ERROR;
			}
		}
		else
		{
			/*** 如果一个渠道下面挂有多个产品,那么只是更新该产品的状态而已,不停止进程 ***/
			ret = uni_svr_prdt_stop_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, item->lsnname[i]);
				return SW_ERROR;
			}
		}
	}

	memset(proc_name, 0x0, sizeof(proc_name));
	sprintf(proc_name, "%s_%s", PROC_NAME_SVC_MAN, prdt_name);
	ret = uni_svr_stop_process(svr, SW_MSTOPSELF, proc_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Stop [%s] error!", __FILE__, __LINE__, proc_name);
		return SW_ERROR;
	}
	return SW_OK;
}

sw_int_t uni_svr_stop_lsn_in_prdt(sw_uni_svr_t *svr, sw_char_t *prdt_name, sw_char_t *lsn_name)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_char_t	proc_name[64];
	const sw_prdt_cfg_t	*item = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = (const sw_cfgshm_hder_t *)svr->shm_cfg;
	item = cfg_get_prdt_cfg(shm_cfg);
	if (NULL == item)
	{
		pub_log_error("[%s][%d] Get prdt information failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i = 0; i< shm_cfg->prdt_use; i++)
	{
		if (strcmp(item->prdtname, prdt_name) == 0)
		{
			break;
		}
		item++;
	}
	if (i == shm_cfg->prdt_use)
	{
		pub_log_error("[%s][%d] Can not find [%s]'s info!", __FILE__, __LINE__, prdt_name);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] prdt_name=[%s] lsn_cnt=[%d]", __FILE__, __LINE__, prdt_name, item->lsn_cnt);
	for (i = 0; i < item->lsn_cnt; i++)
	{
		if (strcmp(item->lsnname[i], lsn_name) != 0)
		{
			continue;
		}
		memset(proc_name, 0x0, sizeof(proc_name));
		sprintf(proc_name, "%s_%s", PROC_NAME_LSN, lsn_name);
		if (uni_svr_proc_is_exist(proc_name) != true)
		{
			pub_log_error("[%s][%d] Proc [%s] not exist!", __FILE__, __LINE__, proc_name);
			continue;
		}
		if (chnl_have_only_one_prdt(svr, lsn_name, prdt_name) == 1)
		{
			ret = uni_svr_prdt_stop_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, item->lsnname[i]);
				return SW_ERROR;
			}
			ret = uni_svr_stop_process(svr, SW_MSTOPONE, proc_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Stop [%s] error!", __FILE__, __LINE__, proc_name);
				return SW_ERROR;
			}
		}
		else
		{
			ret = uni_svr_prdt_stop_lsn(svr, prdt_name, lsn_name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Prdt [%s] stop lsn [%s] failed!",
						__FILE__, __LINE__, prdt_name, item->lsnname[i]);
				return SW_ERROR;
			}
		}
	}
	return SW_OK;
}


sw_int_t uni_svr_stop_svr_in_prdt(sw_uni_svr_t *svr, sw_char_t *prdt_name, sw_char_t *svr_name)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_char_t	lsn_name[64];
	sw_char_t	prdt_svc_proc[64];
	memset(prdt_svc_proc, 0x0, sizeof(prdt_svc_proc));
	sprintf(prdt_svc_proc, "%s_%s", PROC_NAME_SVC_MAN, prdt_name);
	
	ret = uni_svr_proc_is_exist(prdt_svc_proc);
	if(false == ret)
	{
		pub_log_info("[%s][%d] Process [%s] isn't running!",
				__FILE__, __LINE__, prdt_svc_proc);
		return SW_OK;
	}

	ret = uni_svr_stop_svrproc_in_prdt(svr, SW_MSTOPSVR, prdt_svc_proc, svr_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Stop server processes in prdt[%s] error!", __FILE__, __LINE__, prdt_name);
		return SW_ERROR;
	}
	return SW_OK;
}


int uni_get_ipc_info(uni_msipc_t *msipc)
{
	int	pid = 0;
	int	shmid = -1;
	sw_int_t	idx = 0;
	sw_int_t	index = 0;
	sw_int_t	result = 0;
	u_char	*addr = NULL;
	sw_uni_svr_t	svr;
	sw_procs_head_t	head;
	sw_proc_info_t	info;
	struct shmid_ds	shmds;
	sw_syscfg_t	*syscfg = NULL;
	sw_svr_grp_t	*svr_grp = NULL;
	sw_svr_grp_t	*svr_grps = NULL;
	sw_proc_info_t	*proc_shm = NULL;
	sw_sem_lock_t	*sem_lock = NULL;
	sw_procs_head_t	*procs_head = NULL;
	const sw_cfgshm_hder_t	*shm_cfg = NULL;

	memset(&svr, 0x00, sizeof(svr));
	memset(&head, 0x00, sizeof(head));
	memset(&info, 0x0, sizeof(info));
	memset(&shmds, 0x0, sizeof(shmds));

	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] Link shm error!", __FILE__, __LINE__);
		return -1;
	}

	svr.shm_cfg = run_get_syscfg();
	if (svr.shm_cfg == NULL)
	{
		pub_log_error("[%s][%d] Get syscfg error!", __FILE__, __LINE__);
		return -1;
	}

	shm_cfg = (const sw_cfgshm_hder_t *)svr.shm_cfg;
	result = procs_get_head(&head);
	if(SW_OK != result)
	{
		pub_log_error("[%s][%d] Get procs head error!", __FILE__, __LINE__);
		return -1;
	}

	/*run_shm*/
	shmid = run_get_shmid();
	uni_regist_ipc(msipc, "run_shm", pid, shmid, 0);
	pub_log_info("[%s][%d] runshm:[%d]", __FILE__, __LINE__, shmid);

	/*vars_shm*/
	syscfg = ( sw_syscfg_t *)BLOCKAT(shm_cfg,shm_cfg->sys_offset);
	shmid = syscfg->vars_shmid;
	uni_regist_ipc(msipc, "vars_shm", pid, shmid, 0);
	pub_log_info("[%s][%d] varshm:[%d]", __FILE__, __LINE__, shmid);

	/*proc_shm*/
	for (idx = 0; idx < head.sys_proc_use; idx++)
	{
		memset(&info, 0x00, sizeof(info));
		result = procs_get_sys_by_index(idx, &info);
		if (SW_OK != result)
		{
			pub_log_error("[%s][%d] Get sys procs by index error!", __FILE__, __LINE__);
			return -1;
		}

		if (0 == info.shmid)
		{
			continue;
		}

		uni_regist_ipc(msipc, info.name, info.pid, info.shmid, 0);
		pub_log_info("[%s][%d] shm:pname:[%s] pid:[%d] shmid:[%d]", __FILE__, __LINE__, info.name, info.pid, info.shmid);
	}

	addr = (u_char *)run_get_procs();
	if (addr == NULL)
	{
		pub_log_error("[%s][%d] run_get_procs failed", __FILE__, __LINE__);
		return -1;
	}

	procs_head = (sw_procs_head_t *)addr;
	svr_grp = (sw_svr_grp_t *)((u_char *)procs_head + procs_head->svr_grp_offset);
	for (idx = 0; idx < procs_head->svr_grp_use; idx++)
	{
		if (strlen(svr_grp[idx].svrname) > 0)
		{
			svr_grps = svr_grp+(sw_int_t)idx;
			proc_shm =(sw_proc_info_t *)((char *)procs_head + svr_grps->offset);
			for (index = 0; index < svr_grps->svc_curr; index++)
			{
				memset(&info, 0x00, sizeof(info));
				pub_mem_memcpy(&info, proc_shm + index, sizeof(info));			
				if (info.mqid > 0)
				{
					uni_regist_ipc(msipc, proc_shm[index].name, info.pid, info.mqid, 1);
					pub_log_info("[%s][%d] mq:pname:[%s] pid:[%d] mqid:[%d]",
							__FILE__, __LINE__, proc_shm[index].name, info.pid, info.mqid);
				}
			}
		}
	}

	addr = NULL;
	addr = (u_char *)run_get_sem_lock();
	sem_lock = (sw_sem_lock_t *)addr;
	if (sem_lock == NULL)
	{
		pub_log_error("[%s][%d]sem_lock is NULL", __FILE__, __LINE__);
		return -1;
	}

	result = uni_regist_ipc(msipc, "sem", 0, sem_lock->sem_id, 2);
	if (result < 0)
	{
		pub_log_error("[%s][%d]uni_regist_ipc failed!", __FILE__, __LINE__);
		return -1;
	}
	run_destroy();

	return 0;
}

int uni_regist_ipc(uni_msipc_t *msipc, char *pname, int pid, int ipcid, int flag)
{
	int	ret = -1;
	int	index = 0;

	if (msipc == NULL || pname == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}


	for (index = 0; index < UNI_MAX_IPC_CNT; index++)
	{
		if (msipc[index].used == 0)
		{
			break;
		}
	}

	if (index == UNI_MAX_IPC_CNT)
	{
		pub_log_error("there is no space for msipc");
		return -1;
	}

	if (flag == 0)
	{
		ret = uni_get_shm_info(msipc, pname, ipcid, index);
		if (ret < 0)
		{
			pub_log_error("[%s][%d]uni_get_shm_info failed!", __FILE__, __LINE__);
			return -1;
		}
	}
	else if (flag == 1)
	{
		ret = uni_get_msg_info(msipc, pname, ipcid, index, pid);
		if (ret < 0)
		{
			pub_log_error("[%s][%d]uni_get_msg_info failed!", __FILE__, __LINE__);
			return -1;
		}
	}
	else if (flag == 2)
	{
		ret = uni_get_sem_info(msipc, pname, ipcid, index);
		if (ret < 0)
		{
			pub_log_error("[%s][%d]uni_get_sem_info", __FILE__, __LINE__);
			return -1;
		}
	}
	else
	{
		return 0;
	}

	msipc[index].used = 1;

	return 0;
}

int uni_get_shm_info(uni_msipc_t *msipc, char *pname, int shmid, int index)
{
	int	ret = 0;
	struct shmid_ds	shmds;

	if (msipc == NULL || pname == NULL || shmid < 0 || index < 0)
	{
		pub_log_error("[%s][%d] Param error! shmid=[%d] index=[%d]",
				__FILE__, __LINE__, shmid, index);
		return -1;
	}

	memset(&shmds, 0x00, sizeof(shmds));
	ret = shmctl(shmid, IPC_STAT, &shmds);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] shmctl [%d] failed! errno=[%d]:[%s]", 
				__FILE__, __LINE__, shmid, errno, strerror(errno));
		return -1;
	}

	msipc[index].uni_shm.shmid = shmid;
	msipc[index].uni_shm.shm_size = shmds.shm_segsz;
	msipc[index].uni_shm.shm_atime = shmds.shm_atime;
	msipc[index].uni_shm.shm_dtime = shmds.shm_dtime;
	msipc[index].uni_shm.shm_ctime = shmds.shm_ctime;
	msipc[index].uni_shm.shm_cpid = shmds.shm_cpid;
	msipc[index].uni_shm.shm_lpid = shmds.shm_lpid;
	msipc[index].uni_shm.shm_nattch = shmds.shm_nattch;
	strcpy(msipc[index].uni_shm.pname, pname);

	return 0;
}

int uni_get_msg_info(uni_msipc_t *msipc, char *pname, int mqid, int index, int pid)
{
	int	ret = 0;
	struct msqid_ds	msgs;

	if (msipc == NULL || pname == NULL || mqid < 0 || index < 0)
	{
		pub_log_error("[%s][%d] Param error! mqid=[%d] index=[%d]",
				__FILE__, __LINE__, mqid, index);
		return -1;
	}

	memset(&msgs, 0x00, sizeof(msgs));
	ret = msgctl(mqid, IPC_STAT, &msgs);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] msgctl [%d] failed! errno=[%d]:[%s]", 
				__FILE__, __LINE__, mqid, errno, strerror(errno));
		return -1;
	}

	msipc[index].uni_msg.mqid = mqid;
	msipc[index].uni_msg.msg_stime = msgs.msg_stime;;
	msipc[index].uni_msg.msg_rtime = msgs.msg_rtime;
	msipc[index].uni_msg.msg_ctime = msgs.msg_ctime;
	msipc[index].uni_msg.msg_cbytes = msgs.msg_cbytes;
	msipc[index].uni_msg.msg_qnum = msgs.msg_qnum;
	msipc[index].uni_msg.msg_qbytes = msgs.msg_qbytes;
	msipc[index].uni_msg.msg_lspid = msgs.msg_lspid;
	msipc[index].uni_msg.msg_lrpid = msgs.msg_lrpid;
	msipc[index].uni_msg.msg_pid = pid;
	strcpy(msipc[index].uni_msg.pname, pname);

	return 0;
}

int uni_get_sem_info(uni_msipc_t *msipc, char *pname, int semid, int index)
{
	int	ret = 0;
	struct semid_ds	semds;

	if (msipc == NULL || semid < 0 || index < 0)
	{
		pub_log_error("[%s][%d] Param error! semid=[%d] index=[%d]",
				__FILE__, __LINE__, semid, index);
		return -1;
	}

	memset(&semds, 0x00, sizeof(semds));
	ret = semctl(semid, 0, IPC_STAT, &semds);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] semctl [%d] failed! errno=[%d]:[%s]", 
				__FILE__, __LINE__, semid, errno, strerror(errno));
		return -1;
	}

	msipc[index].uni_sem.semid = semid;
	msipc[index].uni_sem.sem_otime = semds.sem_otime;
	msipc[index].uni_sem.sem_ctime = semds.sem_ctime;
	msipc[index].uni_sem.sem_nsems = semds.sem_nsems;
	strcpy(msipc[index].uni_sem.pname, pname);

	return 0;
}

int uni_clear_ipc_source(uni_msipc_t *msipc)
{
	int i = 0;
	int ret = -1;

	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		if (msipc[i].used == 0)
		{
			break;
		}

		if ((msipc[i].uni_shm.shmid > 0) && (strcmp(msipc[i].uni_shm.pname, "vars_shm") == 0))
		{
			ret = uni_clear_ipc_shm(msipc[i].uni_shm.shmid, 0, msipc[i].uni_shm.shm_nattch, 1);
			if (ret < 0)
			{
				printf("clear vars failed\n");
				pub_log_error("[%s][%d]uni_svr_res_clean_vars", __FILE__, __LINE__);
				return -1;
			}

			printf("clean vars shm sucesss\n");
		}
		else if((msipc[i].uni_shm.shmid > 0) && (strcmp(msipc[i].uni_shm.pname, "run_shm") == 0))
		{
			ret = uni_clear_ipc_shm(msipc[i].uni_shm.shmid, 0, msipc[i].uni_shm.shm_nattch, 0);
			if (ret < 0)
			{
				printf("clear run shm failed\n");
				pub_log_error("[%s][%d]clear_ipc_shm", __FILE__, __LINE__);
				return -1;
			}

			printf("clean run shm sucesss\n");
		}
		else if (msipc[i].uni_shm.shmid > 0)
		{
			ret = uni_clear_ipc_shm(msipc[i].uni_shm.shmid, msipc[i].uni_shm.shm_cpid, msipc[i].uni_shm.shm_nattch, 1);
			if (ret < 0)
			{
				printf("clear ipc shm failed\n");
				pub_log_error("[%s][%d]clear_ipc_shm", __FILE__, __LINE__);
				return -1;
			}
			pub_log_info("[%s][%d] Clear shm:[%d]", __FILE__, __LINE__, msipc[i].uni_shm.shmid);
		}

		if (msipc[i].uni_msg.mqid > 0)
		{
			ret = uni_clear_ipc_msg(msipc[i].uni_msg.mqid, msipc[i].uni_msg.msg_pid);
			if (ret < 0)
			{
				printf("clear ipc msg failed\n");
				pub_log_error("[%s][%d]clear_ipc_msg", __FILE__, __LINE__);
				return -1;
			}
			pub_log_info("[%s][%d] Clear mqid:[%d]", __FILE__, __LINE__, msipc[i].uni_msg.mqid);
		}

		if (msipc[i].uni_sem.semid > 0)
		{
			ret = uni_clear_ipc_sem(msipc[i].uni_sem.semid);
			if (ret < 0)
			{
				printf("clear ipc sem failed\n");
				pub_log_error("[%s][%d]clear_ipc_sem", __FILE__, __LINE__);
				return -1;
			}
			printf("clean sem sucesss\n");
		}
	}

	return 0;
}

static int uni_clear_ipc_sem(int semid)
{
	struct semid_ds semds;
	int ret = -1;

	memset(&semds, 0, sizeof(semds));

	ret = semctl(semid, 0, IPC_STAT, &semds);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] Get semaphore stat failed!", __FILE__, __LINE__);
		return -1;
	}

	return semctl(semid, 0, IPC_RMID, 0);
}

static int uni_clear_ipc_shm(int shmid, int pid, int nattch, int flag)
{
	int ret = -1;
	struct shmid_ds	shmds;

	memset(&shmds, 0x00, sizeof(shmds));

	if (flag == 0)
	{
		if(nattch > 1)
		{
			printf("Some process still attch runtime SHM!\n");
			pub_log_error("[%s][%d] Some process still attch runtime SHM!", __FILE__, __LINE__);
			return -1;
		}
	}
	else if (flag == 1)
	{
		if (pid > 0)
		{
			ret = pub_proc_checkpid(pid);
			if (ret == 0)
			{
				printf("the process is save, can't clean ipc shm\n");
				return 0;
			}
		}

		if (nattch > 0)
		{
			printf("Some process still attch runtime SHM!\n");
			pub_log_error("[%s][%d] Some process still attch runtime SHM!", __FILE__, __LINE__);
			return -1;
		}
	}

	ret = shmctl(shmid, IPC_RMID, 0);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] errmsg:[%d][%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;        
	}

	return 0;
}

static int uni_clear_ipc_msg(int msgid, int pid)
{
	int ret = -1;

	ret = pub_proc_checkpid(pid);
	if (ret == 0)
	{
		printf("the process is save, can't clean ipc msg\n");
		return 0;
	}

	return msgctl(msgid, IPC_RMID, 0);
}

sw_int_t uni_svr_get_log(sw_uni_svr_t *svr, sw_char_t *traceno, sw_char_t *date, sw_char_t *lsn_name)
{
	int	ret = 0;
	sw_cmd_t	cmd;

	if (svr == NULL || traceno == NULL || date == NULL || lsn_name == NULL)
	{
		pub_log_error("[%s][%d] parameter error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = uni_svr_proc_is_exist(PROC_NAME_LOG);
	if (false == ret)
	{
		pub_log_info("[%s][%d] Process [%s] isn't running!", __FILE__, __LINE__, PROC_NAME_LOG);
		return SW_OK;
	}
	memset(&cmd, 0x0, sizeof(cmd));
	cmd.type = SW_GET_LOG;
	cmd.trace_no = atol(traceno);
	strncpy(cmd.lsn_name, lsn_name, sizeof(cmd.lsn_name) - 1);
	strncpy(cmd.sys_date, date, sizeof(cmd.sys_date) - 1);
	snprintf(cmd.udp_name, sizeof(cmd.udp_name), "%s", svr->mod_name);
	udp_send(svr->cmdfd, (sw_char_t*)&cmd, sizeof(cmd), PROC_NAME_LOG);
	memset(&cmd, 0x0, sizeof(cmd));
	udp_recv(svr->cmdfd, (sw_char_t *)&cmd, sizeof(cmd));
	pub_log_info("[%s][%d] udp_name=[%s] type=[%d]", __FILE__, __LINE__, cmd.udp_name, cmd.type);

	return SW_OK;
}

static int uni_svr_crt_lockfile()
{
	int	fd = 0;
	char	name[128];

	memset(name, 0x0, sizeof(name));
	sprintf(name, "%s/cfg/%s", getenv("SWWORK"), ADMIN_LOCKFILE);
	fd = open(name,  O_WRONLY | O_CREAT, 0777);
	if (fd <= 0)
	{
		pub_log_error("[%s][%d] create file[%s] error.", __FILE__, __LINE__,name);
		return SW_ERROR;
	}
	close(fd);

	return SW_OK;
}

static int uni_svr_lockfile_exist()
{
	char	name[128];

	memset(name, 0x0, sizeof(name));
	sprintf(name, "%s/cfg/%s", getenv("SWWORK"), ADMIN_LOCKFILE);

	return pub_file_exist(name);
}

static int uni_svr_remove_lockfile()
{
	char	name[128];

	memset(name, 0x0, sizeof(name));
	sprintf(name, "%s/cfg/%s", getenv("SWWORK"), ADMIN_LOCKFILE);

	return remove(name);
}

