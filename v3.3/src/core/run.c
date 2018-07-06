/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-14
 *** module  : Interface for BP run-time shm. 
 *** name    : run.c
 *** function: BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "run.h"
#include "pub_log.h"
#include "pub_shm.h"
#include "pub_mem.h"
#include "pub_cfg.h"
#include "pub_ares.h"
#include "pub_alog.h"

#if defined(__PRDT_ARG__)
#include "prdt_arg.h"
#endif /*__PRDT_ARG__*/

static int g_run_init = 0;
SW_PROTECTED sw_char_t *g_shm_run = NULL;
SW_PROTECTED sw_int_t run_set_offset(sw_syscfg_t *syscfg, sw_run_head_t *run_head);
SW_PROTECTED void run_print_syscfg(sw_syscfg_t *syscfg);
SW_PROTECTED sw_int_t run_set_alog_cfg();
SW_PROTECTED sw_int_t run_set_ares_cfg();
#if defined(__PRDT_ARG__)
static group_sum_t *run_get_prdt_param(void);

static sw_int_t run_alog_set_bits();

extern int g_workmode_mp;
extern int g_use_local_logs;

/* 计算产品参数总空间 */
#define prdt_arg_get_size() group_get_total_size(PRDT_GRP_NUM, PRDT_GRP_SIZE)
#endif /*__PRDT_ARG__*/

sw_int_t run_link_ext(void)
{
	key_t	key = -1;
	sw_int_t	ret = SW_ERROR;
	sw_int32_t	shmid = -1;
	sw_char_t	*addr = NULL;
	sw_global_path_t global_path;
	

	pub_mem_memzero(&global_path, sizeof(global_path));

	if(NULL != g_shm_run)
	{
		pub_log_info("%s, %d, Shm_run already linked.", __FILE__, __LINE__);
		return SW_OK;
	}

	
	
	/*set global path*/
	ret = cfg_set_path(&global_path);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_set_path error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*get shm_run key*/
	ret = cfg_load_key(&global_path, &key);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_load_key error, len[%d].",__FILE__,__LINE__,ret);
		return SW_ERROR;
	}

	shmid = pub_shm_get(key);
	if (shmid == SW_ERROR)
	{
		pub_log_info("%s, %d, pub_shm_get %d! errmsg:[%d]%s",
				__FILE__, __LINE__, key, errno, strerror(errno));
		return SHM_MISS;
	}

	g_shm_run = pub_shm_at(shmid);
	if (g_shm_run == (void*)-1)
	{
		pub_log_error("%s, %d, pub_shm_get %s", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	/*link shm_sem_lock*/
	addr = (sw_char_t *)run_get_sem_lock();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_sem_lock error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = sem_lock_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, sem_lock_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	ret = sem_lock_link();
	if (ret == SW_ERROR)
	{
		pub_log_error("%s, %d, sem_lock_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*link shm_mtype*/
	addr = (sw_char_t *)run_get_mtype();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_mtype error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = mtype_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	addr = (sw_char_t *)run_get_prdt_limit();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_prdt_limit error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	ret = prdt_limit_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, prdt_limit_set_addr error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}
	addr = (sw_char_t *)run_get_lsn_limit();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_lsn_limit error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = lsn_limit_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, lsn_limit_set_addr error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}
	/*link shm_trace_no*/
	addr = (sw_char_t *)run_get_stat();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_stat error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = seqs_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, seqs_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	addr = (sw_char_t *)run_get_procs();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_procs error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = procs_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, proces_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	addr = (sw_char_t *)run_get_route();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_route error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = trace_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, trace_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	addr = (sw_char_t *)run_get_job();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_route error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = job_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, job_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}
#if defined(__PRDT_ARG__)	/* # Qifeng.zou # 2013.09.10 #*/
    addr = (sw_char_t *)run_get_prdt_param();
    if(NULL == addr)
    {
        pub_log_error("[%s][%d] Product param is NULL!", __FILE__, __LINE__);
        return SW_ERROR;
    }
    ret = prdt_arg_link(addr);
    if(SW_OK != ret)
    {
        pub_log_error("[%s][%d] Set address of paramter failed!", __FILE__, __LINE__);
        return SW_ERROR;
    }    
#endif /*__PRDT_ARG__*/
	addr = (sw_char_t *)run_get_sem_lock();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_sem_lock error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = sem_lock_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, sem_lock_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	run_set_syslog_cfg();
	return SW_OK;	
}
sw_int_t run_link(void)
{
	sw_char_t	*addr = NULL;
	sw_int_t	ret = SW_ERROR;
	sw_int32_t	shmid = -1;
	sw_global_path_t global_path;
	key_t		key = -1;
	pub_mem_memzero(&global_path, sizeof(global_path));
	if(NULL != g_shm_run)
	{
		pub_log_info("%s, %d, Shm_run already linked.", __FILE__, __LINE__);
		return SW_OK;
	}
	char	workmode[16];
	memset(workmode, 0x0, sizeof(workmode));
	if (pub_cfg_get_workmode(workmode) < 0)
	{
		pub_log_error("[%s][%d] Get WORKMODE error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] workmode====[%s]", __FILE__, __LINE__, workmode);
	if (strcasecmp(workmode, "MP") == 0)
	{
		g_workmode_mp = 1;
	}
	else
	{
		g_workmode_mp = 0;
	}
	char	logmode[8];
	memset(logmode, 0x0, sizeof(logmode));
	if (pub_cfg_get_logmode(logmode) < 0)
	{
		pub_log_error("[%s][%d] Get logmode error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] logmode=[%s]", __FILE__, __LINE__, logmode);
	if (logmode[0] == '1')
	{
		g_use_local_logs = 1;
	}
	else
	{
		g_use_local_logs = 0;
	}
	ret = cfg_set_path(&global_path);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_set_path error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = cfg_load_key(&global_path, &key);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_load_key error, len[%d].",__FILE__,__LINE__,ret);
		return SW_ERROR;
	}
	shmid = pub_shm_get(key);
	if (shmid == SW_ERROR)
	{
		pub_log_info("%s, %d, pub_shm_get %d! errmsg:[%d]%s",
				__FILE__, __LINE__, key, errno, strerror(errno));
		return SHM_MISS;
	}
	g_shm_run = pub_shm_at(shmid);
	if (g_shm_run == (void*)-1)
	{
		pub_log_error("[%s][%d] pub_shm_get error, errmsg:[%d]%s",
				 __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;		
	}
	addr = (sw_char_t *)run_get_sem_lock();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_sem_lock error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = sem_lock_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, sem_lock_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;		
	}
	ret = sem_lock_link();
	if (ret == SW_ERROR)
	{
		pub_log_error("%s, %d, sem_lock_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	addr = (sw_char_t *)run_get_mtype();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_mtype error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = mtype_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;		
	}
	addr = (sw_char_t *)run_get_prdt_limit();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_prdt_limit error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = prdt_limit_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, prdt_limit_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;		
	}
	addr = (sw_char_t *)run_get_lsn_limit();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_lsn_limit error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = lsn_limit_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, lsn_limit_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;		
	}
	addr = (sw_char_t *)run_get_stat();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_stat error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret = seqs_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, seqs_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	/*link shm_route*/
	addr = (sw_char_t *)run_get_route();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_route error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = trace_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, trace_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*link shm_procs*/
	addr = (sw_char_t *)run_get_procs();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_procs error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = procs_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, proces_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*link shm_job*/
	addr = (sw_char_t *)run_get_job();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_route error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = job_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, job_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}

#if defined(__PRDT_ARG__)	/* # Qifeng.zou # 2013.09.10 #*/
	addr = (sw_char_t *)run_get_prdt_param();
	if(NULL == addr)
	{
		pub_log_error("[%s][%d] Product param is NULL!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = prdt_arg_link(addr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Set address of paramter failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}    
#endif /*__PRDT_ARG__*/

	/*link sem_lock*/
	addr = (sw_char_t *)run_get_sem_lock();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_sem_lock error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = sem_lock_set_addr(addr);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, sem_lock_set_addr error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	run_set_syslog_cfg();
	run_set_alog_cfg();
	run_set_ares_cfg();
	run_alog_set_bits();
	if (g_use_ares)
	{
		if (ares_init() < 0)
		{
			pub_log_error("[%s][%d] Ares init error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

void run_print_syscfg(sw_syscfg_t *syscfg)
{
	pub_log_info("%s, %d, db_max[%d] job_max[%d] lsn_max[%d] name[%s] processe_max[%d] runshmsize[%d]"
			, __FILE__, __LINE__, syscfg->db_max, syscfg->job_max
			, syscfg->lsn_max, syscfg->name, syscfg->processe_max, syscfg->runshmsize);

	pub_log_info("%s, %d, run_shmid[%d] scantime[%d] semsize[%d] seq_max[%d] session_max[%d] share_pool_size[%d] sid[%s] svr_max[%d] vars_shmid[%d] prdt_max[%d]"
			, __FILE__, __LINE__, syscfg->run_shmid, syscfg->scantime, syscfg->semsize
			, syscfg->seq_max, syscfg->session_max, syscfg->share_pool_size, syscfg->sid
			, syscfg->svr_max, syscfg->vars_shmid, syscfg->prdt_max);
	return;
}

sw_int_t run_set_offset(sw_syscfg_t *syscfg, sw_run_head_t *run_head)
{
	sw_int32_t	size = 0;

	if (syscfg == NULL || run_head == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*shm_run's head*/
	run_head->cfg_offset = pub_mem_align(sizeof(sw_run_head_t), SW_ALIGNMENT);

	/*calc cfg size */
	size = cfg_size_calc(syscfg);	
	run_head->mtype_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->cfg_offset;
	pub_log_info("%s, %d, cfg size[%d]",__FILE__,__LINE__, size);

	/*shm_mtype*/
	size = mtype_get_size(syscfg);
	if (size == SW_ERROR)
	{
		pub_log_error("%s, %d, mtype_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}

	run_head->stat_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->mtype_offset;
	pub_log_info("%s, %d, mtype size[%d]",__FILE__,__LINE__, size);
	run_head->prdt_limit_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->mtype_offset;

	/*shm_prdt_mtype_limit*/
	size = prdt_limit_get_size(syscfg);
	if (size == SW_ERROR)
	{
		pub_log_error("%s, %d, prdt_limit_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}
	pub_log_info("%s, %d, prdt mtype limit size[%d]",__FILE__,__LINE__, size);
	run_head->lsn_limit_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->prdt_limit_offset;

	/*shm_lsn_mtype_limit*/
	size = lsn_limit_get_size(syscfg);
	if (size == SW_ERROR)
	{
		pub_log_error("%s, %d, lsn_limit_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}
	pub_log_info("%s, %d, lsn mtype limit size[%d]",__FILE__,__LINE__, size);
	run_head->stat_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->lsn_limit_offset;

	/*shm_trace_no*/
	size = seqs_get_size(syscfg);

	run_head->procs_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->stat_offset;
	pub_log_info("%s, %d, seqs size[%d]",__FILE__,__LINE__, size);

	/*shm_procs*/
	size = procs_get_size(syscfg);
	if (size == SW_ERROR)
	{
		pub_log_error("%s, %d, procs_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}

	run_head->trace_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->procs_offset;
	pub_log_info("%s, %d, procs size[%d]",__FILE__,__LINE__, size);

	/*shm_route*/
	size = trace_get_size(syscfg);
	if (size == SW_ERROR)
	{
		pub_log_error("%s, %d, trace_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}

	run_head->job_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->trace_offset;
	pub_log_info("%s, %d, trace size[%d]",__FILE__,__LINE__, size);

	/*shm_job*/
	size = job_get_size(syscfg);
	if (size == SW_ERROR)
	{
		pub_log_error("%s, %d, job_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}

#if defined(__PRDT_ARG__) /* # Qifeng.zou # 2013.09.10 #*/
	run_head->prdt_arg_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->job_offset;

	size = prdt_arg_get_size();

	run_head->sem_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->prdt_arg_offset;

#else /*__PRDT_ARG__*/

	run_head->sem_offset = pub_mem_align(size, SW_ALIGNMENT) + run_head->job_offset;
	pub_log_info("%s, %d, job size[%d]",__FILE__,__LINE__, size);

#endif /*__PRDT_ARG__*/
	/*shm_sem_lock*/
	size = sem_lock_get_size();
	pub_log_info("%s, %d, sem lock size[%d]",__FILE__,__LINE__, size);

	run_head->size = pub_mem_align(size, SW_ALIGNMENT) + run_head->sem_offset + SW_ALIGNMENT;

	pub_log_info("%s, %d, run size[%d]",__FILE__,__LINE__, run_head->size);

	return run_head->size;
}

sw_mtype_t* run_get_mtype(void)
{
	sw_run_head_t	*run_head = NULL;

	if (!g_shm_run)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_mtype_t*)(g_shm_run + run_head->mtype_offset);
}

sw_mtype_limit_t* run_get_prdt_limit(void)
{
	sw_run_head_t	*run_head = NULL;

	if (!g_shm_run)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_mtype_limit_t*)(g_shm_run + run_head->prdt_limit_offset);
}

sw_mtype_limit_t* run_get_lsn_limit(void)
{
	sw_run_head_t	*run_head = NULL;

	if (!g_shm_run)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_mtype_limit_t*)(g_shm_run + run_head->lsn_limit_offset);
}

sw_seqs_t* run_get_stat(void)
{
	sw_run_head_t	*run_head = NULL;

	if (!g_shm_run)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_seqs_t*)(g_shm_run + run_head->stat_offset);	
}

sw_procs_head_t *run_get_procs(void)
{
	sw_run_head_t	*run_head = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_procs_head_t *)(g_shm_run + run_head->procs_offset);
}

sw_trace_info_t *run_get_route(void)
{
	sw_run_head_t	*run_head = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_trace_info_t *)(g_shm_run + run_head->trace_offset);
}

sw_job_t* run_get_job(void)
{
	sw_run_head_t	*run_head = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_job_t *)(g_shm_run + run_head->job_offset);
}

sw_sem_lock_t* run_get_sem_lock(void)
{
	sw_run_head_t	*run_head = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (sw_sem_lock_t *)(g_shm_run + run_head->sem_offset);
}

/* 设置日志级别 */
sw_int_t run_set_syslog_cfg()
{
	sw_logcfg_t	log;
	sw_cfgshm_hder_t	*addr = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return -1;
	}

	addr = run_get_syscfg();
	memset(&log, 0x0, sizeof(sw_logcfg_t));
	cfg_get_slog(addr, &log);
	log_set_size(log.lgfile_size);
	if (log.lglvl > 0)
	{
		log_set_syslog_level(log.lglvl);
	}

	memset(&log, 0x0, sizeof(sw_logcfg_t));
	cfg_get_alog(addr, &log);
	log_set_applog_level(log.lglvl);

	return 0;
}

sw_int_t run_set_alog_bits()
{
	alog_cfg_t	*alog = NULL;
	sw_cfgshm_hder_t	*addr = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("[%s][%d] g_shm_run is null!", __FILE__, __LINE__);
		return -1;
	}

	addr = run_get_syscfg();
	alog = (alog_cfg_t *)BLOCKAT(addr, addr->alog_offset);
	alog_bits_init(alog);
	pub_log_info("[%s][%d] alog:use=[%d] ip=[%s] port=[%d] lockid=[%d]",
			__FILE__, __LINE__, alog->use, alog->ip, alog->port, alog->bits.lockid);

	return 0;
}

static sw_int_t run_alog_set_bits()
{
	alog_cfg_t	*alog = NULL;
	sw_cfgshm_hder_t	*addr = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("[%s][%d] g_shm_run is null!", __FILE__, __LINE__);
		return -1;
	}

	addr = run_get_syscfg();
	alog = (alog_cfg_t *)BLOCKAT(addr, addr->alog_offset);
	alog_set_bits(alog);
	if (alog->use)
	{
		pub_log_info("[%s][%d] alog:use=[%d] ip=[%s] port=[%d] lockid=[%d]",
			__FILE__, __LINE__, alog->use, alog->ip, alog->port, alog->bits.lockid);
	}
	return 0;
}

sw_int_t run_set_alog_cfg()
{
	alog_cfg_t	alog;
	sw_cfgshm_hder_t	*addr = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("[%s][%d] g_shm_run is null!", __FILE__, __LINE__);
		return -1;
	}

	addr = run_get_syscfg();
	memset(&alog, 0x0, sizeof(alog));
	cfg_get_alog_cfg(addr, &alog);
	log_set_alog_cfg(alog);
	if (alog.use)
	{
		pub_log_info("[%s][%d] alog:use=[%d] ip=[%s] port=[%d]",
			__FILE__, __LINE__, alog.use, alog.ip, alog.port);
	}
	return 0;
}

sw_int_t run_set_ares_cfg()
{
	ares_cfg_t	ares;
	sw_cfgshm_hder_t	*addr = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("[%s][%d] g_shm_run is null!", __FILE__, __LINE__);
		return -1;
	}

	addr = run_get_syscfg();
	memset(&ares, 0x0, sizeof(ares));
	cfg_get_ares_cfg(addr, &ares);
	ares_set_ares_cfg(ares);
	if (ares.use)
	{
		pub_log_info("[%s][%d] ares:use=[%d] ip=[%s] port=[%d]",
			__FILE__, __LINE__, ares.use, ares.ip, ares.port);
	}
	return 0;
}

#if defined(__PRDT_ARG__)
/******************************************************************************
 **函数名称: run_get_prdt_param
 **功    能: 获取产品参数内存地址
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 产品参数的内存地址
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.10 #
 ******************************************************************************/
static group_sum_t *run_get_prdt_param(void)
{
	sw_run_head_t *run_head = NULL;

	if(NULL == g_shm_run)
	{
		pub_log_error("[%s][%d] Run share memory is NULL!", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t*)g_shm_run;

	return (group_sum_t *)(g_shm_run + run_head->prdt_arg_offset);
}
#endif /*__PRDT_ARG__*/

sw_cfgshm_hder_t* run_get_syscfg(void)
{
	sw_run_head_t	*run_head = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run_bak / g_shm_run == NULL", __FILE__, __LINE__);
		return NULL;
	}

	run_head = (sw_run_head_t *)g_shm_run;

	return (sw_cfgshm_hder_t *)(g_shm_run + run_head->cfg_offset);
}

sw_int_t run_init(void)
{
	sw_int_t	ret = SW_ERROR;
	sw_int32_t	shmid = -1;
	sw_char_t*	addr = NULL;
	sw_run_head_t	run_head;
	sw_syscfg_t	syscfg;
	sw_global_path_t global_path;
	key_t		key = -1;

	pub_mem_memzero(&run_head, sizeof(sw_run_head_t));
	pub_mem_memzero(&syscfg, sizeof(sw_syscfg_t));
	pub_mem_memzero(&global_path, sizeof(sw_global_path_t));

	if(NULL != g_shm_run)
	{
		pub_log_info("%s, %d, Shm_run already linked.", __FILE__, __LINE__);
		return SW_OK;
	}

	char	workmode[16];
	memset(workmode, 0x0, sizeof(workmode));
	if (pub_cfg_get_workmode(workmode) < 0)
	{
		pub_log_error("[%s][%d] Get WORKMODE error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] workmode====[%s]", __FILE__, __LINE__, workmode);
	if (strcasecmp(workmode, "MP") == 0)
	{
		g_workmode_mp = 1;
	}
	else
	{
		g_workmode_mp = 0;
	}
	
	char	logmode[8];
	memset(logmode, 0x0, sizeof(logmode));
	if (pub_cfg_get_logmode(logmode) < 0)
	{
		pub_log_error("[%s][%d] Get logmode error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] logmode=[%s]", __FILE__, __LINE__, logmode);
	if (logmode[0] == '1')
	{
		g_use_local_logs = 1;
	}
	else
	{
		g_use_local_logs = 0;
	}
	
	/*set global path*/
	ret = cfg_set_path(&global_path);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_set_path error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*read syscfg*/
	ret = cfg_read_syscfg(&global_path, &syscfg);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_read_syscfg",__FILE__,__LINE__);
		return SW_ERROR;
	}

	run_print_syscfg(&syscfg);

	/*set shm_run offset*/
	ret = run_set_offset(&syscfg, &run_head);
	if (ret == SW_ERROR)
	{
		pub_log_error("%s, %d, run_set_offset error.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, shm_run size(%d)", __FILE__, __LINE__, run_head.size);

	/*get shm_run key*/
	ret = cfg_load_key(&global_path, &key);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_load_key error, key[%d].",__FILE__,__LINE__,key);
		return SW_ERROR;
	}

	syscfg.run_shmid = -1;

	/*create or link shm_run*/
	shmid = pub_shm_get(key);
	if (shmid == -1)
	{
		if (errno == ENOENT)
		{
			shmid = pub_shm_create(key, run_head.size);
			if (shmid == -1)
			{
				pub_log_error("[%s][%d] pub_shm_create error. errmsg:[%d]%s", 
						__FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}

		}
		else
		{
			pub_log_error("[%s][%d] pub_shm_get error. errmsg:[%d]%s", 
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
	}

	/*attach shm_run*/
	g_shm_run = pub_shm_at(shmid);
	if (g_shm_run == (void*)-1)
	{
		g_shm_run = NULL;
		pub_log_error("[%s][%d] pub_shm_at error. errmsg:[%d]%s", 
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	syscfg.run_shmid = shmid;

	/*run_head init*/
	pub_mem_memcpy(g_shm_run, &run_head, sizeof(sw_run_head_t));

	/*shm_cfg init*/
	addr = (sw_char_t *)run_get_syscfg();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_syscfg error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = cfg_set_offset((sw_cfgshm_hder_t *)addr, &syscfg);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_set_offset error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = cfg_init((sw_cfgshm_hder_t *)addr, &global_path);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, cfg_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	run_print_syscfg(&syscfg);
	cfg_get_sys((const sw_cfgshm_hder_t *)addr, &syscfg);
	run_print_syscfg(&syscfg);

	pub_log_info("%s, %d, syscfg.semsize[%d]",__FILE__,__LINE__,syscfg.semsize);

	/*shm_sem_lock init*/
	addr = (sw_char_t *)run_get_sem_lock();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_sem_lock error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*create sem lock*/
	ret = sem_lock_creat((sw_sem_lock_t *)addr, &(syscfg));
	if (ret == SW_ERROR)
	{
		pub_log_error("%s, %d, sem_loc_lock_creat error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	/*shm_mtype init*/
	addr = (sw_char_t *)run_get_mtype();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_mtype error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = mtype_init((sw_mtype_t *)addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, mtype_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	/*shm_prdt_mtype_limit init*/
	addr = (sw_char_t *)run_get_prdt_limit();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_mtype error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = prdt_limit_init((sw_mtype_limit_t *)addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, prdt_limit_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	/*shm_lsn_mtype_limit init*/
	addr = (sw_char_t *)run_get_lsn_limit();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_mtype error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = lsn_limit_init((sw_mtype_limit_t *)addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, lsn_limit_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}
	/*shm_trace_no*/
	addr = (sw_char_t *)run_get_stat();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_stat error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = seqs_init((sw_seqs_t *)addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, seqs_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}
	/*shm_route*/
	addr = (sw_char_t *)run_get_route();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_route error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = trace_init(addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, trace_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	/*shm_procs*/
	addr = (sw_char_t *)run_get_procs();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_procs error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	pub_log_info("%s, %d, addr[%p]",__FILE__,__LINE__, addr);

	ret = procs_init(addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, procs_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	/*shm_job*/
	addr = (sw_char_t *)run_get_job();
	if (addr == NULL)
	{
		pub_log_error("%s, %d, run_get_job error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = job_init((sw_job_t *)addr, &(syscfg));
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, job_init error.",__FILE__,__LINE__);
		return SW_ERROR;	
	}

	ret = job_load_cfg(global_path.job_file);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, job_load_cfg error.",__FILE__,__LINE__);
		return SW_ERROR;	
	}

	run_set_ares_cfg();	
	run_set_alog_bits();

	g_run_init = 1;
#if defined(__PRDT_ARG__) /* # Qifeng.zou # 2013.09.10 #*/
	addr = (sw_char_t *)run_get_prdt_param();
	if(NULL == addr)
	{
		pub_log_error("[%s][%d] Get address of product paramter failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = prdt_arg_init(addr);
	if(0 != ret)
	{
		pub_log_error("[%s][%d] Init group of product paramter failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}
#endif /*__PRDT_ARG__*/
	return SW_OK;
}

sw_int_t run_destroy(void)
{
	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_shm_dt(g_shm_run);
	g_shm_run = NULL;

	return SW_OK;
}

sw_int_t run_get_size(void)
{
	sw_run_head_t *run_head = NULL;

	if (g_shm_run == NULL)
	{
		pub_log_error("%s, %d, g_shm_run == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	run_head = (sw_run_head_t *)g_shm_run;
	return run_head->size;
}

/******************************************************************************
 ** Name : run_get_shmid
 ** Desc : 获取运行时共享内存ID
 ** Input: NONE
 ** Output: NONE
 ** Return: 运行时共享内存ID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.02 #
 ******************************************************************************/
sw_int32_t run_get_shmid(void)
{
	sw_int_t ret = SW_ERROR;
	sw_int32_t shmid = -1;
	sw_global_path_t global_path;
	key_t key = -1;

	pub_mem_memzero(&global_path, sizeof(global_path));

	/* 1. Get golobal path */
	ret = cfg_set_path(&global_path);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get path information failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/* 2. Get key of run-time SHM */
	ret = cfg_load_key(&global_path, &key);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get key of run-time SHM failed.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	shmid = pub_shm_get(key);
	if(shmid < 0)
	{
		pub_log_error("[%s][%d] Get shmid failed! key:%x errmsg:[%d]%s",
				__FILE__, __LINE__, key, errno, strerror(errno));
		return SHM_MISS;
	}

	return shmid;
}

/******************************************************************************
 ** Name : run_link_and_init
 ** Desc : 连接运行时共享内存
 ** Input: NONE
 ** Output: NONE
 ** Return: 0:成功 !0:失败
 ** Process:
 **     1. 连接运行时共享内存
 **     2. 运行时共享内存未创建时，则创建它
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
sw_int_t run_link_and_init(void)
{
	sw_int_t ret = SW_ERR;

	ret = run_link();
	if(SHM_MISS == ret)
	{
		return run_init();
	}

	return ret;
}

int run_is_init()
{
	pub_log_debug("[%s][%d] g_run_init=[%d]", __FILE__, __LINE__, g_run_init);
	return g_run_init == 1;
}

