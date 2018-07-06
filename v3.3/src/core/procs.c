/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-07
 *** module  : Interface for process info register and check. 
 *** name    : pub_procs.c
 *** function: process info sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include <errno.h>
#include "procs.h"
#include "pub_log.h"
#include "pub_mem.h"
#include "pub_proc.h"
#include "sem_lock.h"

#define SW_MAX_TASK_CNT	(SW_MAX_INT32_VALUE - 10000)

static sw_procs_head_t *g_procs_head = NULL;

#define proc_get_head()         (g_procs_head)

#define proc_sys_get_offset()   (g_procs_head->sys_porc_offset)
#define proc_sys_get_max()		(g_procs_head->sys_procs_max)
#define proc_sys_get_used()		(g_procs_head->sys_proc_use)

#define proc_grp_get_offset()   (g_procs_head->svr_grp_offset)
#define proc_grp_get_max()      (g_procs_head->svr_grp_max)
#define proc_grp_get_used()     (g_procs_head->svr_grp_use)

#define proc_write_lock()		sem_write_lock(g_procs_head->lock_id)
#define proc_write_unlock()		sem_write_unlock(g_procs_head->lock_id)
#define proc_read_lock()		sem_read_lock(g_procs_head->lock_id)
#define proc_read_unlock()		sem_read_unlock(g_procs_head->lock_id)

/* Descriptor of process status */
static const proc_stat_desc_t g_proc_stat_desc[] = 
{
	{SW_S_START, "NORMAL"}
	, {SW_S_STOPED, "STOPED"}
	, {SW_S_STOPPING, "STOPPING"}
	, {SW_S_ABORTED, "ABORTED"}
	, {SW_S_KILL, "KILLED"}
	, {SW_S_ABNORMAL, "ABNORMAL"}
	, {SW_S_ERR, "NOT RUNNING"}
	, {SW_S_NREACH, "NOT REACHABLE"}
	, {SW_SWITCH_ON, "NORMAL(ON)"}
	, {SW_SWITCH_OFF, "NORMAL(OFF)"}
};

/* Descriptor of process type */
static const proc_type_desc_t g_proc_type_desc[] = 
{
	{ND_LSN, "LSN"}
	, {ND_SVC, "SVC"}
	, {ND_TASK, "TASK"}
	, {ND_SVR, "SVR"}
	, {ND_DO, "DO"}
	, {ND_SAF, "SAF"}
	, {ND_LOG, "LOG"}
	, {ND_POL, "POL"}
	, {ND_RES, "RES"}
	, {ND_JOB, "JOB"}
	, {ND_ADM, "ADM"}
	, {ND_ALERT, "ALERT"}
};


SW_PROTECTED sw_int_t procs_register(sw_proc_info_t *addr,int max,int*use,sw_int32_t lock_id,sw_proc_info_t *proc_info);

sw_int_t procs_init(sw_char_t* addr, sw_syscfg_t * sys_cfg)
{
	sw_int32_t	lock_id = -1;
	sw_procs_head_t *head_addr;

	if (addr == NULL || sys_cfg == NULL)
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	lock_id = sem_new_lock_id();
	if (lock_id == -1)
	{
		pub_log_error("[%s][%d] sem_new_lock_id error, lock_id[%d].",
				__FILE__, __LINE__, lock_id);
		return SW_ERR;		
	}
	head_addr = (sw_procs_head_t*)addr;
	pub_log_debug("[%s][%d] Head_add=[%x]", __FILE__, __LINE__, head_addr);

	sem_write_lock(lock_id);
	head_addr->count = 0;
	head_addr->lock_id = lock_id;
	head_addr->sw_work_stat  = SW_ALL_STOP;

	head_addr->svr_grp_use = 0;
	head_addr->svr_grp_max = sys_cfg->lsn_max + sys_cfg->svr_max;
	head_addr->svr_grp_offset = sizeof(sw_procs_head_t) ;

	head_addr->sys_proc_use = 0;	
	head_addr->sys_procs_max  = sys_cfg->lsn_max + sys_cfg->prdt_max;
	head_addr->sys_porc_offset = head_addr->svr_grp_offset + head_addr->svr_grp_max*sizeof(sw_svr_grp_t);

	head_addr->app_proc_use = 0;
	head_addr->app_proc_max = sys_cfg->processe_max;
	head_addr->app_porc_offset = head_addr->sys_porc_offset + sizeof(sw_proc_info_t)*head_addr->sys_procs_max ;
	g_procs_head = (sw_procs_head_t *)head_addr;
	sem_write_unlock(lock_id);

	return SW_OK;
}

sw_int_t procs_get_head(sw_procs_head_t *procs_head)
{
	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}
	if (procs_head == NULL )
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERR;
	}
	sem_read_lock(g_procs_head->lock_id);
	pub_mem_memcpy(procs_head,g_procs_head,sizeof(sw_procs_head_t));
	sem_read_unlock(g_procs_head->lock_id);

	return SW_OK;
}

sw_int_t procs_set_addr(sw_char_t* addr)
{
	if(addr == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERR;
	}
	g_procs_head = (sw_procs_head_t *)addr;
	return SW_OK;
}

sw_int32_t procs_get_size(sw_syscfg_t* sys_cfg)
{
	sw_int32_t	size = 0;

	if (sys_cfg == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERR;
	}

	size = sizeof(sw_procs_head_t)  \
	       + sizeof(sw_svr_grp_t)*(sys_cfg->lsn_max+sys_cfg->svr_max) \
	       + sizeof(sw_proc_info_t) * (sys_cfg->lsn_max + sys_cfg->prdt_max) \
	       + sys_cfg->processe_max* sizeof(sw_proc_info_t);

	return size;
}

sw_int_t procs_svr_alloc(sw_svr_grp_t * grp)
{
	sw_int32_t	index = 0;
	sw_int32_t	curr = 0; 
	sw_svr_grp_t	*svr_grp_shm = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("%s, %d, the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}
	if (grp == NULL || grp->svrname[0] == '\0')
	{
		pub_log_error("%s, %d, Param  error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	svr_grp_shm = NULL;
	svr_grp_shm = procs_get_svr_by_name(grp->svrname,grp->prdtname);
	if (svr_grp_shm == NULL)
	{
		sem_write_lock(g_procs_head->lock_id);
		svr_grp_shm = (sw_svr_grp_t*)BLOCKAT(g_procs_head,g_procs_head->svr_grp_offset);
		for (index = 0; index < g_procs_head->svr_grp_max; index++)
		{
			if (svr_grp_shm[index].svrname[0] == '\0')
			{
				break;
			}
		}

		if(index == g_procs_head->svr_grp_max)
		{
			sem_write_unlock(g_procs_head->lock_id);
			pub_log_error("[%s][%d] No enough space to alloc!", __FILE__, __LINE__);
			return SW_ERR;
		}

		curr = g_procs_head->app_proc_use + grp->svc_max;
		if (curr >= g_procs_head->app_proc_max )
		{
			sem_write_unlock(g_procs_head->lock_id);
			pub_log_error("[%s][%d]proc info shm size[%d] is not enough,used [%d] max[%d]",
					__FILE__,__LINE__,g_procs_head->app_proc_max,g_procs_head->app_proc_use,grp->svc_max);
			return SW_ERR;
		}
		pub_mem_memcpy(svr_grp_shm+index,grp,sizeof(sw_svr_grp_t));
		svr_grp_shm[index].svc_curr = 0;
		svr_grp_shm[index].offset = g_procs_head->app_porc_offset+ \
					    g_procs_head->app_proc_use*sizeof(sw_proc_info_t) ;
		svr_grp_shm[index].lock_id = sem_new_lock_id();
		if (svr_grp_shm[index].lock_id == -1)
		{
			pub_log_error("[%s][%d] sem_new_lock_id error, lock_id[%d].",
					__FILE__, __LINE__, svr_grp_shm[index].lock_id);
			return SW_ERR;
		}
		pub_log_debug("[%s][%d] LOCKID=[%d]", __FILE__, __LINE__, svr_grp_shm[index].lock_id);
		g_procs_head->app_proc_use = curr;
		g_procs_head->svr_grp_use++;
		sem_write_unlock(g_procs_head->lock_id);
	}
	else
	{
		if (svr_grp_shm->svc_max != grp->svc_max)
		{
			pub_log_error("%s, %d,the grp[%s] exist,and max_svc=[%d],input[%d] do not alloc grp.",
					__FILE__,__LINE__,grp->svrname,svr_grp_shm->svc_max,grp->svc_max);
			return SW_ERR;
		}
		pub_log_info("[%s][%d] LOCKID=[%d]", __FILE__, __LINE__, svr_grp_shm->lock_id);
	}

	return SW_OK;
}

SW_PROTECTED sw_int_t procs_register(sw_proc_info_t *addr,int max,int *use,sw_int32_t lock_id,sw_proc_info_t *proc_info)
{
	sw_int32_t	first = 0;
	sw_int32_t	task_cnt = 0;
	sw_int32_t	index = 0;

	if (addr == NULL || lock_id <= 0 || use == NULL ||*use < 0 || max < *use)
	{
		pub_log_error("[%s][%d] Paramters is incorrect! addr:[%p] lock_id:[%d] use:[%p]", 
				__FILE__,__LINE__, addr, lock_id, use);
		if(NULL != use)
		{
			pub_log_error("use:[%d] max:[%d]", *use, max);
		}
		return SW_ERR;
	}

	if (proc_info == NULL || proc_info->name == NULL || strlen(proc_info->name) == 0)
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	sem_write_lock(lock_id);
	for (index = 0; index < max; index++)
	{
		if (0 == strcmp(addr[index].name, proc_info->name))
		{
			pub_log_info("[%s][%d] name=[%s] index=[%d]", __FILE__, __LINE__, proc_info->name, index);
			task_cnt = addr[index].task_cnt;
			break;
		}
	}

	if(index == max)
	{
		pub_log_info("[%s][%d] use=[%d]", __FILE__, __LINE__, *use);
		for (index = *use; index < max; index++)
		{
			if (addr[index].name[0] == '\0')
			{
				first = 1;
				task_cnt = 0;
				*use += 1;
				break;
			}
		} 
		if (index == max)
		{
			pub_log_error("[%s][%d] max=[%d] use=[%d]",__FILE__,__LINE__,max,*use);			
			sem_write_unlock(lock_id);
			return SW_ERR;
		}

	}
	pub_log_info("[%s][%d] proc_name=[%s] index=[%d] first=[%d]",
			__FILE__, __LINE__, proc_info->name, index, first);
	pub_mem_memcpy(addr+index,proc_info, sizeof(sw_proc_info_t));
	addr[index].task_cnt = task_cnt;
	addr[index].busy_flag = PROC_S_IDLE;
	addr[index].starttime = (long)time(NULL);
	if (first == 1)
	{
		addr[index].restart_cnt = MAX_RESTART_CNT;
	}
	sem_write_unlock(lock_id);

	return SW_OK;
}

sw_int_t procs_lsn_register(sw_proc_info_t *proc_info)
{
	sw_int32_t	ret = 0;
	sw_proc_info_t	*proc = NULL;
	sw_svr_grp_t	*svr_grp = NULL;


	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (proc_info == NULL || proc_info->name == NULL || 
			proc_info->name[0] == '\0' || proc_info->svr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] proc_info Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	svr_grp = procs_get_svr_by_name(proc_info->svr_name,NULL);
	if (svr_grp == NULL)
	{
		pub_log_error("[%s][%d] procs_get_svr_by_name (%s) error.",
				__FILE__,__LINE__,proc_info->svr_name);
		return SW_ERR;
	}

	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head,svr_grp->offset);
	ret = procs_register(proc,svr_grp->svc_max,&svr_grp->svc_curr,g_procs_head->lock_id,proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_register error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

sw_int_t procs_svr_register(sw_proc_info_t *proc_info)
{
	sw_int32_t	ret = 0;
	sw_proc_info_t	*proc = NULL;
	sw_svr_grp_t	*svr_grp = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (proc_info == NULL || proc_info->name == NULL || proc_info->name[0] == '\0' || 
			proc_info->svr_name[0] == '\0' || proc_info->prdt_name[0] == '\0')
	{
		pub_log_error("[%s][%d] proc_info Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	svr_grp = procs_get_svr_by_name(proc_info->svr_name,proc_info->prdt_name);
	if (svr_grp == NULL)
	{
		pub_log_error("[%s][%d] procs_get_svr_by_name (%s) error.",
				__FILE__,__LINE__,proc_info->svr_name);
		return SW_ERR;
	}
	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head,svr_grp->offset);
	ret = procs_register(proc,svr_grp->svc_max,&svr_grp->svc_curr,g_procs_head->lock_id,proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_register error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

sw_int_t procs_sys_register(sw_proc_info_t *proc_info)
{
	sw_int32_t	ret = 0;
	sw_proc_info_t	*proc = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (proc_info == NULL || proc_info->name == NULL || proc_info->name[0] == '\0')
	{
		pub_log_error("[%s][%d] proc_info Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (g_procs_head->sys_proc_use + 1 > g_procs_head->sys_procs_max)
	{
		pub_log_error("[%s][%d] No enough space to save sysproc info! max=[%d] use=[%d]",
				__FILE__, __LINE__, g_procs_head->sys_procs_max, g_procs_head->sys_proc_use);
		return SW_ERR;
	}
	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head,g_procs_head->sys_porc_offset);
	ret = procs_register(proc,g_procs_head->sys_procs_max, &g_procs_head->sys_proc_use,g_procs_head->lock_id,proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_register error.",__FILE__,__LINE__);
		return SW_ERR;
	}
	pub_log_info("[%s][%d] sys [%s] register success!", __FILE__, __LINE__, proc_info->name);

	return SW_OK;
}

sw_int_t procs_get_sys_by_index(sw_int32_t index,sw_proc_info_t *proc_info)
{
	sw_proc_info_t	*proc = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (proc_info == NULL || index < 0 || index >= g_procs_head->sys_procs_max)
	{
		pub_log_error("[%s][%d] proc_info Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}
	proc = (sw_proc_info_t*)BLOCKAT(g_procs_head,g_procs_head->sys_porc_offset);
	if (proc[index].name[0] == '\0' )
	{
		pub_log_error("[%s][%d] proc index [%d] is empty.",__FILE__,__LINE__,index);
		return SW_ERR;
	}
	sem_read_lock(g_procs_head->lock_id);
	pub_mem_memcpy(proc_info,proc+index,sizeof(sw_proc_info_t));
	sem_read_unlock(g_procs_head->lock_id);
	return SW_OK;
}

/******************************************************************************
 ** Name : procs_set_sys_by_index
 ** Desc : Set process's information by index
 ** Input: 
 **     index: The index of process
 **		info: Value
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t procs_set_sys_by_index(int index, const sw_proc_info_t *info)
{
	sw_proc_info_t *proc = NULL;


	if ((NULL == info) || (index < 0)
			|| (NULL == g_procs_head) || (index >= proc_sys_get_max()))
	{
		pub_log_error("[%s][%d] Index is out of range! idx:[%d][%d]",
				__FILE__, __LINE__, index, proc_sys_get_max());
		return SW_ERR;
	}

	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head, proc_sys_get_offset());
	if('\0' == proc[index].name[0])
	{
		pub_log_error("[%s][%d] proc index [%d] is empty.",__FILE__,__LINE__,index);
		return SW_ERR;
	}

	proc_write_lock();
	pub_log_info("[%s][%d] PROC_NAME=[%s] SVR_NAME=[%s] PID=[%d]", 
			__FILE__, __LINE__, proc->name, proc->svr_name, proc->pid);
	pub_mem_memcpy(proc+index, info, sizeof(sw_proc_info_t));
	proc_write_unlock();

	return SW_OK;
}

/******************************************************************************
 ** Name : procs_sys_isexist_by_index
 ** Desc : Judge system process whether exist by index
 ** Input: 
 **     index: The index of system process
 ** Output: NONE
 ** Return: true:exist false:not exist
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
bool procs_sys_isexist_by_index(sw_int32_t index)
{
	sw_proc_info_t *proc = NULL;

	if((index < 0) || (index >= proc_sys_get_max()))
	{
		pub_log_error("[%s][%d] Paramter is incorrect!", __FILE__, __LINE__);
		return false;
	}

	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head, proc_sys_get_offset());
	if('\0' == proc[index].name[0])
	{
		pub_log_error("[%s][%d] Process index [%d] is empty.", __FILE__, __LINE__, index);
		return false;
	}

	return true;
}

sw_int_t procs_get_sys_by_name(const char *name,sw_proc_info_t *proc_info)
{
	sw_int32_t i;
	sw_proc_info_t *proc;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (proc_info == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] proc_info Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}
	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head,g_procs_head->sys_porc_offset);

	for (i=0 ;i< g_procs_head->sys_proc_use; i++)
	{
		if(0 == pub_str_strcmp(name,proc[i].name))
		{
			break;
		}
	}
	if (i==g_procs_head->sys_proc_use )
	{
		pub_log_info("[%s][%d] not find [%s] procs.",__FILE__,__LINE__,name);
		return SW_ERR;
	}
	sem_read_lock(g_procs_head->lock_id);
	pub_mem_memcpy(proc_info,proc+i,sizeof(sw_proc_info_t));
	sem_read_unlock(g_procs_head->lock_id);
	return SW_OK;
}

/******************************************************************************
 ** Name : procs_set_sys_by_name
 ** Desc : Set process's information by name
 ** Input: 
 **     name: The name of process
 **		info: Value
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
sw_int_t procs_set_sys_by_name(const char *name, const sw_proc_info_t *info)
{
	sw_int32_t	idx = 0;
	sw_proc_info_t	*proc = NULL;

	if ((NULL == name) || (0 == strlen(name)) || (NULL == g_procs_head) || (NULL == info))
	{
		pub_log_error("[%s][%d] proc_info Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	proc = (sw_proc_info_t*) BLOCKAT(g_procs_head, proc_sys_get_offset());

	for (idx=0 ; idx<proc_sys_get_used(); idx++)
	{
		if(0 == pub_str_strcmp(proc[idx].name, name))
		{
			break;
		}
	}

	if (idx == proc_sys_get_used())
	{
		pub_log_error("[%s][%d] Didn't find [%s] procs.", __FILE__, __LINE__, name);
		return SW_ERR;
	}

	proc_write_lock();
	pub_mem_memcpy(proc+idx, info, sizeof(sw_proc_info_t));
	proc_write_unlock();

	return SW_OK;
}

/******************************************************************************
 ** Name : procs_is_sys_exist
 ** Desc : Judge system process whether exist by name
 ** Input: 
 **     index: The index of system process
 ** Output: NONE
 ** Return: true:exist false:not exist
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
bool procs_is_sys_exist(const sw_char_t *name)
{
	sw_int32_t i = 0, ret = 0;
	const sw_proc_info_t *proc = NULL;

	proc = (const sw_proc_info_t *)BLOCKAT(g_procs_head, proc_sys_get_offset());

	for(i=0; i<proc_sys_get_used(); i++)
	{
		if(0 == pub_str_strcmp(proc[i].name, name))
		{
			break;
		}
	}

	if (i== proc_sys_get_used())
	{
		pub_log_info("[%s][%d] Didn't find [%s] procs.", __FILE__, __LINE__, name);
		return false;
	}

	ret = pub_proc_checkpid((sw_int_t)proc[i].pid);
	if (SW_OK != ret)
	{
		pub_log_error("[%s][%d] proc [%s][%d] already exist!", 
				__FILE__, __LINE__, name, proc[i].pid);
		return false;
	}

	return true;
}

sw_svr_grp_t* procs_get_svr_by_index(sw_int32_t index)
{
	sw_svr_grp_t	*svr_grp = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return NULL;
	}

	if (index < 0 )
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return NULL;
	}

	if (index >= g_procs_head->svr_grp_max)
	{
		pub_log_error("[%s][%d] input index [%d] > max[%d]",
				__FILE__,__LINE__,index,g_procs_head->svr_grp_max);
		return NULL;
	}
	svr_grp = (sw_svr_grp_t*)BLOCKAT(g_procs_head,g_procs_head->svr_grp_offset);
	if (svr_grp[index].svrname[0] == '\0')
	{
		pub_log_info("[%s][%d] index:[%d] svr_grp is empty",__FILE__,__LINE__,index);
		return NULL;
	}
	return (svr_grp+index);
}

sw_svr_grp_t* procs_get_svr_by_name(const sw_char_t *name, const sw_char_t *prdt)
{
	int index = 0;
	sw_svr_grp_t *svr_grp = NULL;

	if (NULL == g_procs_head)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return NULL;
	}

	if ((NULL == name) || ('\0' == name[0]))
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return NULL;
	}
	svr_grp = (sw_svr_grp_t*)BLOCKAT(g_procs_head,g_procs_head->svr_grp_offset);
	for (index = 0; index < g_procs_head->svr_grp_use; index++)
	{		
		/***
		  pub_log_info("[%s][%d] svr[%d]:prdt=[%s] svrname=[%s] prdt=[%s] name=[%s]",
		  __FILE__, __LINE__, index, svr_grp[index].prdtname, svr_grp[index].svrname, prdt, name);
		  pub_log_info("[%s][%d] svrname[%s]=[%x]", 
		  __FILE__, __LINE__, svr_grp[index].svrname, svr_grp[index].svrname);
		 ***/
		if ((NULL == prdt || '\0' == prdt[0] || 0 == strcmp(svr_grp[index].prdtname, prdt))
				&& 0 == strcmp(svr_grp[index].svrname, name))
		{
			break;
		}
	}

	if(index >= g_procs_head->svr_grp_use)
	{
		pub_log_info("[%s][%d] Do not find [%s]'s svr group info.",__FILE__,__LINE__,name);
		return NULL;
	}

	return (svr_grp+(sw_int_t)index);
}

sw_int_t procs_get_proces_by_name(const sw_svr_grp_t *svr_grp,sw_char_t *name,sw_proc_info_t *proc)
{
	sw_int32_t	lock_id = 0;
	sw_int32_t	index = 0;
	sw_proc_info_t	*proc_shm = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (proc == NULL ||name == NULL|| name[0] == '\0' ||svr_grp == NULL)
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	lock_id = svr_grp->lock_id;

	proc_shm =(sw_proc_info_t *) BLOCKAT(g_procs_head,svr_grp->offset);
	for(index = 0; index <svr_grp->svc_curr; index++)
	{
		if (strcmp(proc_shm[index].name, name) == 0)
		{
			pub_log_info("[%s][%d] proc[%d].name=[%s] name=[%s]",
					__FILE__, __LINE__, index, proc_shm[index].name, name); 
			break;
		}
	}
	if (index >= svr_grp->svc_curr)
	{
		pub_log_info("[%s][%d] proc [%s] not exist!", __FILE__, __LINE__, name);
		return SW_ERR;
	}
	sem_read_lock(lock_id);
	pub_mem_memcpy(proc,proc_shm+index,sizeof(sw_proc_info_t));
	proc_shm[index].busy_flag = PROC_S_BUSYING;
	sem_read_unlock(lock_id);

	return SW_OK;
}

sw_int_t procs_get_proces_by_index(const sw_svr_grp_t *svr_grp,sw_int32_t index,sw_proc_info_t *proc)
{
	sw_int32_t	lock_id = 0;
	sw_proc_info_t	*proc_shm = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (index < 0 ||proc == NULL||svr_grp == NULL)
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	lock_id = g_procs_head->lock_id;
	if (svr_grp->svc_max < index)
	{
		pub_log_error("[%s][%d] svr_grp->svc_max < index error.",__FILE__,__LINE__);
		return SW_ABORT;
	}
	proc_shm =(sw_proc_info_t *) BLOCKAT(g_procs_head,svr_grp->offset);

	sem_read_lock(lock_id);
	pub_mem_memcpy(proc,proc_shm+index,sizeof(sw_proc_info_t));
	sem_read_unlock(lock_id);

	return SW_OK;
}

sw_int_t procs_get_proces_info(sw_char_t *prdt,sw_char_t *svr,sw_char_t *svc,sw_proc_info_t *proc)
{
	sw_int32_t	ret = 0;
	sw_svr_grp_t	*svr_grp = NULL;

	if ( svr == NULL || svr[0] == '\0' || svc == NULL || svc[0] == '\0' || proc == NULL)
	{
		pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	svr_grp = procs_get_svr_by_name(svr,prdt);
	if (svr_grp == NULL)
	{
		pub_log_info("[%s][%d] svr [%s] not exist!", __FILE__, __LINE__, svr);
		return SW_ERR;
	}
	ret = procs_get_proces_by_name(svr_grp,svc,proc);
	if (ret !=  SW_OK)
	{
		pub_log_info("[%s][%d] svr:[%s] svc:[%s] not exist!", __FILE__, __LINE__, svr, svc);
		return SW_ERR;
	}

	return SW_OK;	
}

sw_int_t procs_get_dst_proc(sw_char_t *svr, sw_char_t *prdt, sw_int32_t msg_type, sw_proc_info_t *proc)
{
	sw_int32_t	i = 0;
	sw_int32_t	index = 0;
	sw_int32_t	lock_id = 0;
	sw_uint32_t	min = 0;
	sw_svr_grp_t	*svr_grp = NULL;
	sw_proc_info_t	*proc_shm = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("%s, %d, the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}
	if (svr == NULL || svr[0] == '\0')
	{
		pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	svr_grp = procs_get_svr_by_name(svr,prdt);
	if (svr_grp == NULL)
	{
		pub_log_error("[%s][%d] do not find [%s] svr in the procs shm!",
				__FILE__,__LINE__,svr);
		return SW_ERR;
	}
	lock_id = svr_grp->lock_id;
	proc_shm =(sw_proc_info_t *) BLOCKAT(g_procs_head,svr_grp->offset);

	index = -1;
	sem_read_lock(lock_id);
	for (i = 0; i < svr_grp->svc_curr ; i++)
	{
		/***
		  pub_log_info("[%s][%d] svr[%s] pid=[%d] status=[%d]", 
		  __FILE__, __LINE__, svr, proc_shm[i].pid, proc_shm[i].status);
		 ***/
		if (proc_shm[i].pid > 0 && 
				(proc_shm[i].status == SW_S_START || (proc_shm[i].status == SW_S_EXITING && msg_type == SW_MSG_REQ))
				&& pub_proc_checkpid(proc_shm[i].pid) == SW_OK
				&& proc_shm[i].type != ND_LSN_RCV)
		{
			if (index == -1)
			{
				min = proc_shm[i].task_cnt;
				index = i;
			}

			if (proc_shm[i].task_cnt < min)
			{
				min = proc_shm[i].task_cnt;
				index = i;
			}

			if (proc_shm[i].busy_flag == PROC_S_IDLE)
			{
				index = i;
				pub_log_info("[%s][%d] process [%s] idle!", __FILE__, __LINE__, proc_shm[i].name);
				break;
			}
		}
	}
	if (index < 0 || index > svr_grp->svc_max)
	{
		pub_log_error("[%s][%d] do not find[%s] svr in the procs shm!",
				__FILE__,__LINE__,svr);
		sem_read_unlock(lock_id);
		return SW_ERR;
	}
	if (proc_shm[index].busy_flag == PROC_S_IDLE)
	{
		proc_shm[index].busy_flag = PROC_S_BUSYING;
	}
	pub_mem_memcpy(proc,proc_shm+index,sizeof(sw_proc_info_t));
	sem_read_unlock(lock_id);

	proc_shm[index].task_cnt++;
	if (min > SW_MAX_TASK_CNT)
	{
		sem_write_lock(lock_id);
		for (i = 0; i < svr_grp->svc_curr; i++)
		{
			if (proc_shm[i].task_cnt > SW_MAX_TASK_CNT)
			{
				proc_shm[i].task_cnt -= SW_MAX_TASK_CNT;
			}
		}
		sem_write_unlock(lock_id);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : procs_get_stat_desc
 ** Desc : Get the descriptor of status.
 ** Input: 
 **		status: The status of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.21 #
 ******************************************************************************/
const char *procs_get_stat_desc(int status)
{
	sw_int_t idx=0, num=0;

	num = sizeof(g_proc_stat_desc)/sizeof(proc_stat_desc_t);
	for(idx=0; idx<num; idx++)
	{
		if(g_proc_stat_desc[idx].status == status)
		{
			return g_proc_stat_desc[idx].desc;
		}
	}
	return NULL;
}

/******************************************************************************
 ** Name : procs_get_type_desc
 ** Desc : Get the descriptor of type.
 ** Input: 
 **		type: The type of process
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.21 #
 ******************************************************************************/
const char *procs_get_type_desc(int type)
{
	sw_int_t idx=0, num=0;

	num = sizeof(g_proc_type_desc)/sizeof(proc_type_desc_t);
	for(idx=0; idx<num; idx++)
	{
		if(g_proc_type_desc[idx].type == type)
		{
			return g_proc_type_desc[idx].desc;
		}
	}
	return NULL;
}

int print_procs()
{
	int     i = 0;
	int     j = 0;
	int     result = 0;
	sw_svr_grp_t    *grp_svr;
	sw_proc_info_t  proc_info;
	sw_procs_head_t procs_head;

	result = procs_get_head(&procs_head);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] procs_get_head error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] @@@@@@@@@@ svr_grp_use=[%d]", __FILE__, __LINE__, procs_head.svr_grp_use);

	for (i = 0 ; i < procs_head.svr_grp_use; i++)
	{
		grp_svr = NULL;
		grp_svr = procs_get_svr_by_index(i);
		if (grp_svr == NULL)
		{
			pub_log_error("[%s][%d] procs_get_svr_by_index error!", __FILE__, __LINE__);
			break;
		}
		for (j = 0; j < grp_svr->svc_curr; j++)                                                                  
		{                                                                                                        
			memset(&proc_info, 0x0, sizeof(proc_info));                                                      
			result = procs_get_proces_by_index(grp_svr, j, &proc_info);                                      
			if (result != SW_ERROR)
			{
				if (proc_info.type != ND_SVC)
				{
					continue;
				}
				pub_log_info("[%s][%d] @@@@@@@@@@@@ name=[%s] svr_name=[%s] pid=[%d] type=[%d]status=[%d]",
						__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);
			}
		}
	}

	return 0;
}

static sw_int_t procs_update_process_busystatus_by_name(const sw_svr_grp_t *svr_grp, sw_char_t *name, sw_int32_t busy_flag)
{
	sw_int32_t	index = 0;
	sw_proc_info_t	*proc_shm = NULL;

	if (g_procs_head == NULL)
	{
		pub_log_error("[%s][%d] the g_procs_head == NULL!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (name == NULL || name[0] == '\0' || svr_grp == NULL)
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	proc_shm = (sw_proc_info_t *)BLOCKAT(g_procs_head, svr_grp->offset);
	for (index = 0; index < svr_grp->svc_curr; index++)
	{
		if (strcmp(proc_shm[index].name, name) == 0)
		{
			break;
		}
	}
	if (index >= svr_grp->svc_curr)
	{
		pub_log_info("[%s][%d] proc [%s] not exist!", __FILE__, __LINE__, name);
		return SW_ERR;
	}
	proc_shm[index].busy_flag = busy_flag;

	return SW_OK;
}

sw_int_t procs_update_process_busystatus(sw_char_t *prdt, sw_char_t *svr, sw_char_t *svc, sw_int32_t busy_flag)
{
	sw_int32_t	ret = 0;
	sw_svr_grp_t	*svr_grp = NULL;

	if (svr == NULL || svr[0] == '\0' || svc == NULL || svc[0] == '\0')
	{
		pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	svr_grp = procs_get_svr_by_name(svr, prdt);
	if (svr_grp == NULL)
	{
		pub_log_info("[%s][%d] svr [%s] not exist!", __FILE__, __LINE__, svr);
		return SW_ERR;
	}

	ret = procs_update_process_busystatus_by_name(svr_grp, svc, busy_flag);
	if (ret !=  SW_OK)
	{
		pub_log_info("[%s][%d] svr:[%s] svc:[%s] not exist!", __FILE__, __LINE__, svr, svc);
		return SW_ERR;
	}

	return SW_OK;
}

sw_int_t procs_check_svr_status(char *svrname, char *prdtname)
{
	int	i = 0;
	int	ret = 0;
	int	errcnt = 0;
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;

	grp_svr = procs_get_svr_by_name(svrname, prdtname);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] Get svr error, svrname=[%s] prdtname=[%s]!",
				__FILE__, __LINE__, svrname, (prdtname != NULL ? prdtname : "NULL"));
		return SW_ERROR;
	}
	if (grp_svr->svc_curr <= 0)
	{
		return SW_ERROR;
	}

	errcnt = 0;
	for (i = 0; i < grp_svr->svc_curr; i++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, i, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}
		if (proc_info.status != SW_S_START)
		{
			errcnt++;
		}
	}
	if (errcnt == grp_svr->svc_curr)
	{
		pub_log_info("[%s][%d] child cnt:[%d] abnormal:[%d]",
				__FILE__, __LINE__, grp_svr->svc_curr, errcnt);
		return SW_ERROR;
	}

	return SW_OK;
}


sw_int32_t procs_get_sys_status(char *sysname)
{
	sw_int32_t      ret = 0;
	sw_proc_info_t  proc_info;

	pub_mem_memzero(&proc_info, sizeof(proc_info));
	ret = procs_get_sys_by_name(sysname, &proc_info);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Get [%s]' proc info error!", __FILE__, __LINE__, sysname);
		return SW_ERROR;
	}

	return proc_info.status;
}

