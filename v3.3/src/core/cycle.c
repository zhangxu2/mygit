#include "cycle.h"
#include "alert.h"
#include "pub_ares.h"
#include "pub_signal.h"

sw_int_t cycle_init(sw_cycle_t* cycle, const sw_char_t* name, sw_int32_t module_type,
			const sw_char_t* err_log, const sw_char_t* dbg_log, const sw_char_t *prdt )
{
	sw_char_t*	tmp = NULL;
	sw_int_t	result = SW_ERROR;
	sw_char_t	path[FILE_NAME_MAX_LEN] = {0};
	
	if (name == NULL || err_log == NULL || dbg_log == NULL)
	{
		pub_log_stderr("cycle_init Param error.\n");
		pub_log_info("[%s][%d], cycle_init param error",
			__FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_signal_nozombie();
	pub_signal_ignore();

	/*Create memory pool for cycle object.*/
	if (cycle->pool == NULL)
	{
		cycle->pool = pub_pool_create(1024);
		if (cycle->pool == NULL)
		{
			pub_log_stderr("cycle_init pub_pool_create error.\n");
			pub_log_info("[%s][%d], pub_pool_create param error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	/*Set module name.*/
	cycle->name.data = pub_pool_palloc(cycle->pool, strlen(name) + 1);
	if (cycle->name.data == NULL)
	{
		pub_log_stderr("Allocate error for module name error.\n");
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}
	
	strcpy(cycle->name.data, name);
	cycle->name.len = strlen(name) + 1;

	/*Set module type.*/
	cycle->module = module_type;

	/*Set env SWHOME.*/
	tmp = getenv("SWHOME");
	if (tmp == NULL)
	{
		pub_log_stderr("cycle_init get env SWHOME fail.\n");
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->home_dir.data = pub_pool_palloc(cycle->pool, strlen(tmp) + 1);
	if (cycle->home_dir.data == NULL)
	{
		pub_log_stderr("Allocate for home_dir error.\n");
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;		
	}

	strcpy(cycle->home_dir.data, tmp);
	cycle->home_dir.len = strlen(tmp) + 1;

	/*Set env SWWORK.*/
	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_stderr("cycle_init get env SWWORK error.\n");
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->work_dir.data = pub_pool_palloc(cycle->pool, strlen(tmp) + 1);
	if(cycle->work_dir.data == NULL)
	{
		pub_log_stderr("Allocate for SWWORK error.\n");
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	strcpy(cycle->work_dir.data, tmp);
	cycle->work_dir.len = strlen(tmp) + 1;

	if (err_log != NULL && dbg_log != NULL)
	{
		/*Allocate memory for log object and init it.*/
		cycle->log = pub_pool_palloc(cycle->pool, sizeof(sw_log_t));
		if (cycle->log == NULL)
		{
			pub_log_stderr("cycle_init Allocate for log object error.\n");
			pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}

		pub_mem_memzero(path, sizeof(path));
		if (prdt == NULL || prdt[0] == '\0')
		{
			snprintf(path, sizeof(path), "%s/log/syslog", cycle->work_dir.data);
		}
		else
		{
			snprintf(path, sizeof(path), "%s/log/%s/syslog", cycle->work_dir.data, prdt);
		}
		
		cycle->log->log_path.data = pub_pool_palloc(cycle->pool, strlen(path) + 1);
		if (cycle->log->log_path.data == NULL)
		{
			pub_log_stderr("cycle_init Allocate for cycle->log->log_path fail.\n");
			pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}

		strcpy(cycle->log->log_path.data, path);
		cycle->log->log_path.len = strlen(path) + 1;

		cycle->log->errlog.data = pub_pool_palloc(cycle->pool, strlen(err_log) + 1);
		if (cycle->log->errlog.data == NULL)
		{
			pub_log_stderr("cycle_init Allocate for cycle->log->errlog.data fail.\n");
			pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}

		strcpy(cycle->log->errlog.data, err_log);
		cycle->log->errlog.len = strlen(err_log) + 1;

		cycle->log->dbglog.data = pub_pool_palloc(cycle->pool, strlen(dbg_log) + 1);
		if (cycle->log->dbglog.data == NULL)
		{
			pub_log_stderr("cycle_init Allocate for cycle->log->dbglog.data fail.\n");
			pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}

		strcpy(cycle->log->dbglog.data, dbg_log);
		cycle->log->dbglog.len = strlen(dbg_log) + 1;

		/*Set error log path.*/
		pub_mem_memzero(path, sizeof(path));
		snprintf(path, sizeof(path), "%s/%s", cycle->log->log_path.data, cycle->log->errlog.data);
		aix_mkdirp(cycle->log->log_path.data, 0777);

		result = pub_log_chglog(SW_LOG_CHG_ERRFILE, path);
		if (result != SW_OK)
		{
			pub_log_stderr("pub_log_chglog error path error.\n");
			pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}

		/*Set debug log path.*/
		pub_mem_memzero(path, sizeof(path));
		snprintf(path, sizeof(path), "%s/%s", cycle->log->log_path.data, cycle->log->dbglog.data);

		result = pub_log_chglog(SW_LOG_CHG_DBGFILE, path);
		if (result != SW_OK)
		{
			pub_log_stderr("pub_log_chglog debug path error.\n");
			pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	/*Allcate memory for global_path.*/
	cycle->global_path = pub_pool_palloc(cycle->pool, sizeof(sw_global_path_t));
	if (cycle->global_path == NULL)
	{
		pub_log_error("%s, %d, allocate error.",__FILE__,__LINE__);
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	/*set global cfg path*/
	result = cfg_set_path(cycle->global_path);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cfg_set_path error.",__FILE__,__LINE__);
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*Allcate memory for event object.*/
	cycle->lsn_fds = pub_pool_palloc(cycle->pool, sizeof(sw_fd_set_t));
	if (cycle->lsn_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error", __FILE__, __LINE__);
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	/*init select event context.*/
	result = select_init(cycle->lsn_fds);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		pub_log_info("[%s][%d] error",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->timeout_handler = NULL;
	cycle->shm_cfg = NULL;
	cycle->syscfg = NULL;

#if defined(__ALERT_SUPPORT__)
	result = alert_link();
	if(SW_OK != result)
	{
		pub_log_error("[%s][%d] Link alert failed!", __FILE__, __LINE__);
		return SW_ERR;
	}
#endif /*__ALERT_SUPPORT__*/

	memset(&g_module_info, 0x0, sizeof(g_module_info));
	g_module_info.type = module_type;
	if (module_type == ND_LSN)
	{
		strncpy(g_module_info.name, name + 6, sizeof(g_module_info.name) - 1);
	}
	else
	{
		strncpy(g_module_info.name, name, sizeof(g_module_info.name) - 1);
	}
	pub_log_debug("[%s][%d] name=[%s] modname=[%s]", __FILE__, __LINE__, name, g_module_info.name);
	return SW_OK;
}

sw_int_t cycle_link_shm_run(sw_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;

	result = run_link();
	if (result == SHM_MISS)
	{
		pub_log_error("%s, %d, run_link error, SHM_MISS.",__FILE__,__LINE__);
		return SHM_MISS;
	}
	else if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*get global cfg*/
	cycle->shm_cfg = run_get_syscfg();
	cycle->syscfg = (sw_syscfg_t *)BLOCKAT(cycle->shm_cfg, ((sw_cfgshm_hder_t*)cycle->shm_cfg)->sys_offset);

	return SW_OK;
}

sw_int_t cycle_init_shm_vars(sw_cycle_t* cycle)
{
	sw_int_t result = SW_ERROR;

	if (cycle->syscfg == NULL)
	{
		pub_log_error("%s, %d, did not link shm_run.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
    result = vars_shm_init(cycle->syscfg);
    if (result == SW_ERROR)
    {
		pub_log_error("%s, %d, vars_shm_init error.",__FILE__,__LINE__);
		return SW_ERROR;
    }

	return SW_OK;
}

sw_int_t cycle_create_shm_run(sw_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;

	result = run_init();
	if (SHM_MISS == result)
	{
		pub_log_error("%s, %d, run_init error.",__FILE__,__LINE__);
		return SHM_MISS;
	}
	else if (SW_OK != result)
	{
		pub_log_error("%s, %d, run_init error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	/*get global cfg*/
	cycle->shm_cfg = run_get_syscfg();
	cycle->syscfg = (sw_syscfg_t *)BLOCKAT(cycle->shm_cfg, ((sw_cfgshm_hder_t*)cycle->shm_cfg)->sys_offset);

	return SW_OK;

}

sw_int_t cycle_create_shm(sw_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;

	result = cycle_create_shm_run(cycle);
	if (SHM_MISS == result)
	{
		pub_log_error("%s, %d, cycle_create_shm_run error.",__FILE__,__LINE__);
		return SHM_MISS;
	}
	else if (SW_OK != result)
	{
		pub_log_error("%s, %d, cycle_create_shm_run error.",__FILE__,__LINE__);
		return SW_ERROR;		
	}

	result = cycle_init_shm_vars(cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_init_shm_vars error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;

}

sw_int_t cycle_link_shm(sw_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;

	result = cycle_link_shm_run(cycle);
	if (result == SHM_MISS)
	{
		pub_log_error("%s, %d, cycle_link_shm_run error, SHM_MISS." ,__FILE__, __LINE__);
		return SHM_MISS;
	}
	else if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_link_shm_run error." ,__FILE__, __LINE__);
		return SW_ERROR;
	}

	result = cycle_init_shm_vars(cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_init_shm_vars error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t cycle_destory(sw_cycle_t* cycle)
{
	if (cycle->lsn_fds != NULL)
	{
		select_clear(cycle->lsn_fds);
	}
	
	if (cycle->pool != NULL)
	{
		pub_pool_destroy(cycle->pool);
		cycle->pool = NULL;
	}
	ares_close_fd();
	
	return SW_OK;
}

sw_int_t cycle_run(sw_cycle_t* cycle)
{
	sw_int32_t	i = 0;
	sw_int_t	rcv_fd_cnt;
	sw_int_t	result = SW_ERROR;
	sw_fd_list_t	*fd_work;
	
	while (1)
	{
		if (cycle->timeout_handler != NULL)
		{
			cycle->timeout_handler((void *)cycle);
		}

		/*begin to scan*/
		rcv_fd_cnt = select_process_events(cycle->lsn_fds,&fd_work,15000);
		if (rcv_fd_cnt < 0)
		{	
			/* clock interrupt */
			if (errno == EINTR)
			{
				pub_log_info("[%s][%d] clock interrupt",__FILE__,__LINE__);
				continue;
			}
			else
			{	/*select error exit */
				pub_log_error("select error [%s][%d][%d]",__FILE__,__LINE__,errno);
				exit(1);
			}
		}
		else if (rcv_fd_cnt == 0)
		{	
			/* wait timeout*/
			continue;
		}
		else
		{	
			/*run event handler*/
			for (i = 0; i < rcv_fd_cnt; i++)
			{
				result = fd_work[i].event_handler(&fd_work[i]);
				if (result == SW_EVENT_LOOP_EXIT)
				{
					return SW_OK;
				}
				else if (result == SW_CONTINUE)
				{
					pub_log_info("[%s][%d]deal platform success",__FILE__,__LINE__);
					continue;
				}
				else if (result == SW_DELETE)
				{
					select_del_event(cycle->lsn_fds, fd_work[i].fd);
					continue;
				}
				else if (result > 4 )
				{
					select_del_event(cycle->lsn_fds, result);
					continue;
				}
			}
		}
	}
	return SW_OK;
}

