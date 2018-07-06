#include "svc_cycle.h"
#include "pub_proc.h"
#include "pub_filter.h"

extern int svc_main(sw_svc_cycle_t *cycle);

sw_int_t svc_cycle_init(cycle, name, module_type, err_log, dbg_log, prdt)
sw_svc_cycle_t *cycle;
sw_char_t *name;
sw_int32_t module_type;
sw_char_t *err_log;
sw_char_t *dbg_log;
sw_char_t *prdt;
{
	int	reuse = 0;
	int	ret = SW_ERROR;
	sw_int_t	flow_size = 0;
	sw_int_t	trace_size = 0;
	sw_int_t	proc_trace_size = 0;
	sw_int_t	svr_cache_size = 0;
	sw_fd_list_t	fd_list;
	sw_proc_info_t  proc_info;
	sw_seqs_t	*seqs = NULL;
	
	ret = cycle_init((sw_cycle_t *)cycle, name, module_type, err_log, dbg_log, prdt);
	if (ret != SW_OK)
	{
		pub_log_stderr("[%s][%d] pub_cycle_init error. ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	
	cycle->cmd_fd = udp_bind(cycle->base.name.data);
	if (cycle->cmd_fd <= 0)
	{
		pub_log_error("[%s][%d] upd_bind error! fd=[%d] name=[%s]", 
			__FILE__, __LINE__, cycle->cmd_fd, cycle->base.name.data);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] udp_name====[%s] fd=[%d]", __FILE__, __LINE__, cycle->base.name.data, cycle->cmd_fd);

	cycle->svc_fds = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (cycle->svc_fds == NULL)
	{
		pub_log_error("[%s][%d] pub_pool_palloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = select_init(cycle->svc_fds);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(&fd_list, 0x0, sizeof(fd_list));
	fd_list.fd = cycle->cmd_fd;
	fd_list.data = (void*)cycle;
	fd_list.event_handler = (sw_event_handler_pt)svc_handle_control_cmd;
	ret = select_add_event(cycle->svc_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, select_add_event error fd[%d].", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}

	ret = cycle_link_shm(&cycle->base);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] cycle_link_shm error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] cycle_link_shm success!", __FILE__, __LINE__);
	
	memset(&proc_info, 0x0, sizeof(proc_info));
	ret = procs_get_sys_by_name(cycle->base.name.data, &proc_info);
	if ( ret != SW_ERROR)
	{
		if ( pub_proc_checkpid(proc_info.pid) ==  SW_OK)
		{
			pub_log_info("[%s][%d] name=[%s] register info exist, iStatus=[%d],PID=[%d]",
				__FILE__, __LINE__, cycle->base.name.data, proc_info.status, proc_info.pid);
			return SW_EXIST;
		}

		if (proc_info.shmid > 0)
		{
			cycle->shm.addr = shmat(proc_info.shmid, NULL, 0);
			if (cycle->shm.addr != (void *)-1)
			{
				reuse = 1;
				cycle->shm.id = proc_info.shmid;
				pub_log_info("[%s][%d] 共享内存存在! 不需要创建!", __FILE__, __LINE__);
			}
		}
	}
	
	seqs = run_get_stat();
	if (seqs == NULL)
	{
		pub_log_error("[%s][%d] run_get_stat error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = seqs_add_business_trace(SYS_TRACE_NAME, 0);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] seqs_add_business_trace error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	

	ret = seqs_add_business_trace(DEFAULT_TRACE_NAME, 0);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] seqs_add_business_trace error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	cycle->list_size = mtype_get_max();
	if (cycle->list_size <= 0)
	{
		pub_log_error("[%s][%d] Get mtype max error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** SVR 信息缓冲区大小 ***/
	svr_cache_size = pub_mem_align(sizeof(sw_svr_cache_t) + sizeof(sw_svr_t) * MAX_SVR_CNT, SW_ALIGNMENT);
	/*** FLOW缓冲区大小 ***/
	flow_size = pub_mem_align(sizeof(sw_flow_t) * MAX_SVR_CNT * MAX_CACHE_CNT, SW_ALIGNMENT);
	/*** 流水缓冲区大小 ***/
	proc_trace_size = (sizeof(sw_svc_trace_t) + sizeof(sw_svc_trace_item_t) * cycle->list_size);
	trace_size = pub_mem_align(proc_trace_size * MAX_SVR_CNT * MAX_PROC_CNT, SW_ALIGNMENT);
	cycle->shm.size = svr_cache_size + flow_size + trace_size;
	if (reuse == 0)
	{
		pub_log_info("[%s][%d] 共享内存不存在,需要创建!", __FILE__, __LINE__);
		ret = pub_shm_alloc(&(cycle->shm));
		if (ret != SW_OK)
		{
			pub_log_stderr("[%s][%d] pub_shm_alloc error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] pub_shm_alloc success!", __FILE__, __LINE__);
	}
	cycle->svrs = (sw_svr_cache_t *)cycle->shm.addr;
	cycle->flow_addr = (char *)(cycle->shm.addr + svr_cache_size);
	cycle->trace_addr = (char *)(cycle->shm.addr + svr_cache_size + flow_size);
	strncpy(cycle->prdt, prdt, sizeof(cycle->prdt) - 1);
	memset(g_prdt, 0x0, sizeof(g_prdt));
	strncpy(g_prdt, prdt, sizeof(g_prdt) - 1);
	
	ret = svc_father_register(cycle, SW_S_START);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] SVC father register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ret = pub_filter_init();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Filter init error!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_debug("[%s][%d] svc cycle init success!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t svc_cycle_run(sw_svc_cycle_t *cycle)
{
	int	ret = 0;
	
	pub_log_debug("[%s][%d] svc_cycle_run begin...", __FILE__, __LINE__);
	ret = svc_main(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] svc_main error!", __FILE__, __LINE__);
		return ret;
	}
	
	return SW_OK;
}

sw_int_t svc_cycle_child_destroy(sw_svc_cycle_t *cycle)
{
	int i = 0;
	int max = 0;
	pub_log_debug("[%s][%d] svc_cycle_child_destroy!", __FILE__, __LINE__);
	close(cycle->cmd_fd);
	if (g_svr->use_svrlevel)
	{
		max = SVR_LEVEL_ALL;
	}
	else
	{
		max = 0;	
	}

	for (i = 0; i <= max; i++)
	{
		msg_trans_rm(cycle->base.global_path, cycle->fd_out[i]);	
	}
	cycle_destory((sw_cycle_t *)cycle);

	if (g_arule_xml)
	{
		pub_log_info("[%s][%d] delete alert rule tree!", __FILE__, __LINE__);
		pub_xml_deltree(g_arule_xml);
	}
	pub_stack_pop_free(g_afmt_stack);
	pub_log_exit("[%s][%d] child destroy!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t svc_cycle_destroy(sw_svc_cycle_t *cycle)
{
	pub_log_debug("[%s][%d] svc_cycle_destroy!", __FILE__, __LINE__);
	close(cycle->cmd_fd);
#if defined(__PRDT_ARG__)
	prdt_arg_gfree();
#endif
	pub_shm_free(&cycle->shm);
	cycle_destory((sw_cycle_t *)cycle);
	pub_filter_free();
	
	return SW_OK;
}
