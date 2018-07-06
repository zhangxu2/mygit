#include "agent_work.h"
#include "pub_db.h"

int agt_get_libfunc_handle(agt_cycle_t *cycle, char *name, char *outlib, char *outfunc)
{
	char	buf[256];
	char	libso[256];
	char	*ptr = NULL;

	if (cycle == NULL || name == NULL || outlib == NULL 
			|| outfunc == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Get lib func param error!", __FILE__, __LINE__);
		return -1;
	}

	memset(buf, 0x0, sizeof(buf));
	memset(libso, 0x0, sizeof(libso));

	strncpy(buf, name, sizeof(buf) - 1);
	ptr = strchr(buf, '/');
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] [%s] Format error! [lib/fun]", __FILE__, __LINE__, buf);
		return -1; 
	}

	strncpy(outlib, buf, ptr - buf);
	strncpy(outfunc, ptr + 1, strlen(ptr + 1));

	pub_log_debug("[%s][%d] LIB=[%s] FUNC=[%s]", __FILE__, __LINE__, outlib, outfunc);
	sprintf(libso, "%s/lib/%s", getenv("SWHOME"), outlib);
	if (!pub_file_exist(libso))
	{
		pub_log_error("[%s][%d] libso[%s] not found", __FILE__, __LINE__, libso);
		return SW_ERROR;
	}

	cycle->handle = (void *)dlopen(libso, RTLD_LAZY|RTLD_GLOBAL);
	if (cycle->handle == NULL)
	{
		pub_log_error("[%s][%d]dlopen [%s] error=[%d][%s]"
				, __FILE__, __LINE__, libso, errno, dlerror());
		return SW_ERROR;
	} 

	return SW_OK;
}


int agt_lib_load(agt_cycle_t *cycle)
{
	int	ret = 0;
	char	filename[128];
	char	filefunc[128];

	if (cycle->cfg.init[0] == '\0')
	{
		pub_log_info("[%s][%d] name[%s] not cfg init.", __FILE__, __LINE__, cycle->cfg.name);
	}
	else
	{	
		pub_mem_memzero(filename, sizeof (filename));
		pub_mem_memzero(filefunc, sizeof (filefunc));

		ret = agt_get_libfunc_handle(cycle, cycle->cfg.init, filename, filefunc);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] get lib[%s] error.", __FILE__, __LINE__, cycle->cfg.init);
			return -1;
		}

		cycle->handler.init_handler = (agt_deal_handle_pt)dlsym(cycle->handle, filefunc);
		if (cycle->handler.init_handler == NULL)
		{
			pub_log_error("[%s][%d]dlsym error=[%s]node value=[%s]lib=[%s]func=[%s]", 
					__FILE__, __LINE__,dlerror(),cycle->cfg.name,filename,filefunc);
			return -1;
		}
	}

	if (cycle->cfg.deal[0] == '\0')
	{
		pub_log_info("[%s][%d] name[%s] not cfg deal.", __FILE__, __LINE__, cycle->cfg.name);
	}
	else
	{	
		pub_mem_memzero(filename, sizeof (filename));
		pub_mem_memzero(filefunc, sizeof (filefunc));
		ret = agt_get_libfunc_handle(cycle, cycle->cfg.deal, filename, filefunc);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] get lib[%s] error.", __FILE__, __LINE__, cycle->cfg.deal);
			return -1;
		}

		cycle->handler.deal_handler = (sw_event_handler_pt)dlsym(cycle->handle, filefunc);
		if (cycle->handler.deal_handler == NULL)
		{
			pub_log_error("[%s][%d]dlsym error=[%s]node value=[%s]lib=[%s]func=[%s]", 
					__FILE__, __LINE__, dlerror(), cycle->cfg.name, filename, filefunc);
			return -1;
		}
	}

	return 0;
}

int agt_handle_timeout(agt_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	cnt = 0;
	int	type = 0;
	char	name[64];
	sw_agt_proc_t	proc;

	type = cycle->cfg.type;
	cnt = cycle->cfg.proccnt;
	for (i = 0; i < cnt; i++)
	{
		pub_mem_memzero(name, sizeof(name));
		pub_mem_memzero(&proc, sizeof(proc));
		snprintf(name, sizeof(name), "%s_%d", cycle->cfg.name, i);
		pub_log_debug("[%s][%d] name=[%s]", __FILE__, __LINE__,name);
		ret = agt_get_by_name(name, &proc);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] get proc[%s] info error.", __FILE__, __LINE__, name);
			continue;
		}

		if (proc.pid > 0 && proc.stat != SW_S_STOPED)
		{
			ret = agt_check_pid(proc.pid);
			if (ret == -1)
			{
				if (proc.restartcnt >= 0)
				{
					pub_log_info("[%s][%d] process [%s][%d] is abnormal,restart it...",
							__FILE__, __LINE__, proc.name, proc.pid);

					if (proc.restartcnt == 0)
					{

						proc.stat = SW_S_ABNORMAL;
						proc.restartcnt--;
						ret = agt_set_proc_by_name(proc.name, &proc);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] register proc[%s] error.", __FILE__, __LINE__, proc.name);
							return -1;
						}
						pub_log_info("[%s][%d] reach max restart cnt, not restart.", __FILE__, __LINE__);	
						continue;
					}

					proc.stat = SW_S_ABNORMAL;
					proc.restartcnt--;
					ret = agt_set_proc_by_name(proc.name, &proc);
					if (ret < 0)
					{
						pub_log_error("[%s][%d] register proc[%s] error.", __FILE__, __LINE__, proc.name);
						return -1;
					}

					if (type == 1)
					{
						ret = agt_create_child_comm(cycle, proc.proc_idx);
					}
					else
					{
						ret = agt_create_child_mon(cycle, proc.proc_idx);
					}
					if (ret < 0)
					{
						pub_log_error("[%s][%d] restart proc[%s] error.", __FILE__, __LINE__);
						return -1;
					}

					pub_log_info("[%s][%d] Start binary[%s][%s] success."
							, __FILE__, __LINE__, proc.binary, proc.name);
				}
				else
				{
					pub_log_error("[%s][%d] proc[%s] restart reach max[%d],not restart.",
							__FILE__, __LINE__,  proc.name,  MAX_RESTART_CNT);
				}
			}
		}
	}

	return 0;
}

int agt_child_mon_work(agt_cycle_t *cycle, int index)
{
	int	ret = 0;
	int	sec = 0;
	char	proc_name[64];
	char	log_name[64];
	char	log_path[128];
	sw_agt_proc_t	proc;

	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d]child work argument error", __FILE__, __LINE__);
		return -1;
	}

	memset(proc_name, 0x0, sizeof(proc_name));
	memset(log_name, 0x0, sizeof(log_name));
	memset(log_path, 0x0, sizeof(log_path));

	sprintf(proc_name, "%s_%d", cycle->cfg.name, index);
	sprintf(log_name, "%s.log", proc_name);
	cycle->cfg.proc_index = index;
	sprintf(log_path, "%s/log/syslog/%s", getenv("SWWORK"), log_name);
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_log_chglog debug path error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(&proc, sizeof(proc));
	strcpy(proc.binary, PROC_NAME_AGT);
	strcpy(proc.name, proc_name);
	proc.pid = getpid();
	proc.proc_idx = index;
	proc.stat = SW_S_START;
	proc.type = SW_CHILD;
	ret = agt_proc_register(&proc);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] name[%s]agt_proc_register error.", __FILE__, __LINE__,proc_name);
		return -1;
	}

	pub_log_info("[%s][%d] name[%s] register ok.", __FILE__, __LINE__, proc_name);
	while (1)
	{
		sec = cycle->cfg.scantime;
		ret = pub_time_timer(sec, 0);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
			continue;
		}
		else if (ret == 0)
		{	
			ret = cycle->handler.deal_handler(cycle);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] deal mon handler error.", __FILE__, __LINE__);
			}

			if (getppid() == 1)
			{
				pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
				agt_cycle_destroy(cycle);
				exit(0);
			}
		}
	}

	return 0;
}

int agt_child_comm_work(agt_cycle_t *cycle, int index)
{
	int	i = 0;
	int	ret = 0;
	int	timer = 0;
	int	db_timeout = 0;
	int	recv_cnt = 0;
	long	now = 0;
	long	starttime = 0;
	char	proc_name[64];
	char	log_name[64];
	char	dbtype[32];
	char	log_path[128];
	sw_dbcfg_t	dbcfg;
	sw_fd_list_t	*fd_work;
	sw_agt_proc_t	proc;

	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d]child work argument error", __FILE__, __LINE__);
		return -1;
	}


	memset(proc_name, 0x0, sizeof(proc_name));
	memset(log_name, 0x0, sizeof(log_name));
	memset(log_path, 0x0, sizeof(log_path));

	sprintf(proc_name, "%s_%d", cycle->cfg.name, index);
	sprintf(log_name, "%s.log", proc_name);
	cycle->cfg.proc_index = index;
	sprintf(log_path, "%s/log/syslog/%s", getenv("SWWORK"), log_name);
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_log_chglog debug path error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(&proc, sizeof(proc));
	strcpy(proc.binary, PROC_NAME_AGT);
	strcpy(proc.name, proc_name);
	proc.pid = getpid();
	proc.proc_idx = index;
	proc.stat = SW_S_START;
	proc.type = SW_CHILD;
	ret = agt_proc_register(&proc);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] name[%s]agt_proc_register error.", __FILE__, __LINE__,proc_name);
		return -1;
	}

	memset(&dbcfg, 0x00, sizeof(dbcfg));
	ret = agt_get_db_cfg(&dbcfg);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get db cfg error.", __FILE__, __LINE__);
		return -1;
	}

	db_timeout = 0;
	memset(dbtype, 0x00, sizeof(dbtype));
	db_timeout = dbcfg.exptime;
	strncpy(dbtype, dbcfg.dbtype, sizeof(dbtype));

	ret = pub_db_conn();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Connect db error!", __FILE__, __LINE__);
		return -1;
	}
	starttime = (long)time(NULL);

	pub_log_info("[%s][%d] name[%s] register ok.", __FILE__, __LINE__, proc_name);
	while (1)
	{
		timer = cycle->cfg.scantime > 0 ? cycle->cfg.scantime : 1000;
		recv_cnt = select_process_events(cycle->lsn_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
			continue;
		}
		else if (recv_cnt == 0)
		{
			if (db_timeout > 0 && strcmp(dbtype, "ORACLE") == 0)
			{
				now = (long)time(NULL);
				if (now - starttime > db_timeout)
				{
					pub_db_close();
					pub_log_info("[%s][%d] exptime=[%ld] start=[%ld] now=[%ld]", 
							__FILE__, __LINE__, db_timeout, starttime, now); 
					ret = pub_db_conn();
					if (ret < 0)
					{
						pub_log_error("[%s][%d] Connect db error!", __FILE__, __LINE__);
						return -1;
					}
					starttime = (long)time(NULL);
				}
			}

			if (getppid() == 1)
			{
				pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
				agt_cycle_destroy(cycle);
				exit(0);
			}
			continue;
		}

		for (i = 0; i < recv_cnt; i++)
		{
			ret = fd_work[i].event_handler(&fd_work[i]);
			if (ret != SW_OK)
			{
				pub_db_rollback();
				pub_db_del_all_conn();
			}
			else
			{
				pub_db_commit();
				pub_db_del_all_conn();
			}
		}

	}

	pub_db_close();
	return 0;
}

int agt_create_child_comm(agt_cycle_t *cycle, int index)
{
	int	ret = 0;
	pid_t	pid = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	else if (pid == 0)
	{
		ret = agt_child_comm_work(cycle, index);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] agt_child_comm_work error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return 0;
}

int agt_create_child_mon(agt_cycle_t *cycle, int index)
{
	int	ret = 0;
	pid_t	pid = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	else if (pid == 0)
	{
		ret = agt_child_mon_work(cycle, index);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] agt_child_mon_work error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return 0;
}

int agt_create_child(agt_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	type = cycle->cfg.type;

	for (i = 0; i < cycle->cfg.proccnt; i++)
	{
		if (type == 1)
		{
			ret = agt_create_child_comm(cycle, i);
		}
		else
		{
			ret = agt_create_child_mon(cycle, i);
		}
		if (ret < 0)
		{
			pub_log_error("[%s][%d] create child error.", __FILE__, __LINE__);
			return -1;
		}

		if (type != 1)
		{
			pub_log_info("[%s][%d] mon type proc only start one.", __FILE__, __LINE__);
			break;
		}
	}

	return 0;
}

int agt_cycle_init(agt_cycle_t *cycle, char *name,  char *lsn_name, 
		int module_type, char *err_log, char *dbg_log)
{
	int	ret = 0;
	sw_agt_proc_t	proc;

	ret = agt_set_logpath(err_log, dbg_log);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] set log path error.", __FILE__, __LINE__);
		return -1;
	}

	ret = agt_set_log_cfg();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] set golbal log cfg error.", __FILE__, __LINE__);
		return -1;
	}

	ret = agt_cfg_init(&cycle->cfg, lsn_name);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] load agent cfg init error.", __FILE__, __LINE__);
		return -1;
	}

	ret = agt_lib_load(cycle);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] agent lib load error.", __FILE__, __LINE__);
		return -1;
	}


	ret = alert_link();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] connect alert error.", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(&proc, sizeof(proc));
	strncpy(proc.binary, PROC_NAME_AGT, sizeof(proc.binary));
	strncpy(proc.name, name, sizeof(proc.name));
	proc.pid = getpid();
	proc.proc_idx = -1;
	proc.stat = SW_S_START;
	proc.type = SW_PARENT;
	ret = agt_proc_register(&proc);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] agt_proc_register error.", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] [%s] register success.", __FILE__, __LINE__, name);

	if (cycle->pool == NULL)
	{
		cycle->pool = pub_pool_create(1024);
		if (cycle->pool == NULL)
		{
			pub_log_info("[%s][%d], pub_pool_create param error", __FILE__, __LINE__);
			return -1;
		}
	}

	cycle->lsn_fds = pub_pool_palloc(cycle->pool, sizeof(sw_fd_set_t));
	if (cycle->lsn_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error", __FILE__, __LINE__);
		return -1;
	}

	ret = select_init(cycle->lsn_fds);
	if (ret != 0)
	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int agt_cycle_run(agt_cycle_t *cycle)
{
	int	ret = 0;
	int	sec = 0;

	if (cycle->handler.init_handler != NULL)
	{
		ret = cycle->handler.init_handler(cycle);
		if (ret != 0)
		{
			pub_log_error("%s, %d, init error", __FILE__, __LINE__);
			return -1;
		}
	}

	ret = agt_create_child(cycle);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] lsn_create_child error!", __FILE__, __LINE__);
		return SW_DECLINED;
	}

	sleep(5);

	while (1)
	{
		sec = 60;
		ret = pub_time_timer(sec, 0);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] select_process_events error!", __FILE__, __LINE__);
			continue;
		}
		else if (ret == 0)
		{
			agt_handle_timeout(cycle);
			continue;
		}
	}
	return 0;
}

int agt_cycle_destroy(agt_cycle_t *cycle)
{
	if (cycle->cfg.lsn_fd > 0)
	{
		close(cycle->cfg.lsn_fd);
	}

	if (cycle->pool != NULL)
	{
		pub_pool_destroy(cycle->pool);
		cycle->pool = NULL;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int	ret = 0;
	char	name[32];
	char	log_name[64];
	agt_cycle_t	cycle;

	if (argc != 2)
	{
		pub_log_stderr("[%s][%d] Param error! Usage: %s procname", __FILE__, __LINE__, argv[0]);
		return -1;
	}

	signal(SIGCHLD, SIG_DFL);
	memset(name, 0x0, sizeof(name));
	memset(log_name, 0x0, sizeof(log_name));

	strncpy(name, argv[1], sizeof(name));
	sprintf(log_name, "%s.log", name);

	pub_mem_memzero(&cycle, sizeof(agt_cycle_t));
	ret = agt_cycle_init(&cycle, name, argv[1], ND_AGT, "agt_error.log", log_name);
	if (ret != 0)
	{
		pub_log_exit("[%s][%d]agt cycle init error! ret=[%d]",__FILE__,__LINE__, ret);
		return -1;
	}

	ret = agt_cycle_run(&cycle);
	if (ret != 0)
	{
		agt_cycle_destroy(&cycle);
		return -1;
	}

	wait(NULL);
	agt_cycle_destroy(&cycle);

	return 0;
}

