#include "log_pub.h"
#include "log_comm.h"
#include "alert.h"
#include "anet.h"
#include "pub_proc.h"

sw_char_t g_logname[128];

extern int dfs_get_log(sw_cmd_t *cmd);

static sw_int_t log_send_cmd_to_child(sw_log_cycle_t *cycle)
{
	int	j = 0;
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;

	grp_svr = procs_get_svr_by_name(cycle->base.name.data, NULL);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] procs_get_svr_by_name error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] prdt=[%s] svr=[%s]", __FILE__, __LINE__, grp_svr->prdtname, grp_svr->svrname);

	for (j = 0; j < grp_svr->svc_curr; j++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d]",
				__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);

		memset(&cmd, 0x0, sizeof(cmd));
		cmd.type = SW_MSTOPSELF;
		memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
		sprintf(cmd.udp_name, "%s", cycle->base.name.data);
		pub_log_info("[%s][%d] cmd.udp_name=[%s]", __FILE__, __LINE__, cmd.udp_name);
		udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), proc_info.name);
		memset(&cmd, 0x0, sizeof(cmd));
		udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(cmd));
		pub_log_info("[%s][%d] recv [%s]'s response!", __FILE__, __LINE__, proc_info.name);

	}

	return SW_OK;
}

sw_int_t log_handle_control_cmd(sw_log_cycle_t *cycle)
{
	int	ret = 0;
	sw_char_t	udp_name[64];
	sw_cmd_t	cmd;
	sw_proc_info_t	proc_info;

	memset(&proc_info, 0x0, sizeof(proc_info));
	memset(&cmd, 0x0, sizeof(cmd));
	ret = udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] upd_recv error! fd=[%d]", __FILE__, __LINE__, cycle->cmd_fd);
		return SW_ERROR;
	}

	memset(udp_name, 0x0, sizeof(udp_name));
	strncpy(udp_name, cmd.udp_name, sizeof(udp_name)-1);
	pub_log_info("[%s][%d] udp_name=[%s]", __FILE__, __LINE__, udp_name);

	pub_log_info("[%s][%d] type=[%d]", __FILE__, __LINE__, cmd.type);
	switch (cmd.type)
	{
		case SW_MSTOPSELF:
		case SW_MSTOPISELF:
			pub_log_debug("[%s][%d] Stop SELF!", __FILE__, __LINE__);
			memset(&proc_info, 0x0, sizeof(proc_info));
			strcpy(proc_info.name, cycle->base.name.data);
			proc_info.pid = getpid();
			proc_info.type = ND_LOG;
			proc_info.status = SW_S_STOPPING;
			ret = procs_sys_register(&proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = log_send_cmd_to_child(cycle);
			pub_log_info("[%s][%d] After send cmd! ret=[%d]", __FILE__, __LINE__, ret);
			memset(&proc_info, 0x0, sizeof(proc_info));
			strcpy(proc_info.name, cycle->base.name.data);
			proc_info.pid = getpid();
			proc_info.type = ND_LOG;
			proc_info.status = SW_S_STOPED;
			ret = procs_sys_register(&proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			return SW_ABORT;
			break;
		case SW_GET_LOG:
			pub_log_debug("[%s][%d] get log in memory by [traceno][date][lsn_name]", __FILE__, __LINE__);
			ret = dfs_get_log(&cmd);
			if (ret == SW_ERROR)
			{
				pub_log_error("[%s][%d] get log in memory error", __FILE__, __LINE__);
				return SW_ERROR;
			}
			cmd.type   = SW_RESCMD;
			cmd.status = ret;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char *)&cmd, sizeof(cmd), udp_name);
			return SW_OK;
		default:
			cmd.type = SW_ERRCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name , "%s" , cycle->base.name.data);
			udp_send(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), udp_name);
			break;
	}

	return SW_OK;
}

sw_int_t log_father_register(sw_log_cycle_t *cycle, sw_int32_t status)
{
	sw_int32_t	ret = 0;
	sw_proc_info_t	proc_info;

	pub_mem_memzero(&proc_info, sizeof(proc_info));

	ret = procs_get_sys_by_name(cycle->base.name.data, &proc_info);
	if (ret != SW_OK)
	{
		strcpy(proc_info.name, cycle->base.name.data);
		proc_info.pid = getpid();
		proc_info.type = ND_LOG;
		proc_info.status = status;
		proc_info.shmid = cycle->shm.id;
		proc_info.restart_type = LIMITED_RESTART;
	}
	else
	{
		proc_info.pid = getpid();
		proc_info.status = status;
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	ret = procs_sys_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_sys_register error! name=[%s]", 
				__FILE__, __LINE__, proc_info.name);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] proc:[%s] s:[%d] p:[%d]", 
			__FILE__, __LINE__, proc_info.name, proc_info.status, proc_info.pid);

	return SW_OK;
}

static sw_int_t log_cycle_child_destroy(sw_log_cycle_t *cycle)
{
	if ( cycle->lsn_fd > 0)
	{
		close(cycle->lsn_fd);
		pub_log_info("[%s][%d] Destroy, close fd:[%d]",
				__FILE__, __LINE__, cycle->lsn_fd);
	}

	return SW_OK;
}

sw_int_t log_deal_cmd(sw_log_cycle_t *cycle)
{
	int     ret = 0;
	sw_cmd_t        cmd;
	sw_char_t	udp_name[64];

	memset(&cmd, 0x0, sizeof(cmd));
	ret = udp_recv(cycle->udp_fd, (char *)&cmd, sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] FD=[%d] recv failed,errno=[%d]:[%s]ret=[%d]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno), ret);
		return SW_ERROR;
	}

	memset(udp_name, 0x0, sizeof(udp_name));
	strncpy(udp_name, cmd.udp_name, sizeof(udp_name)-1);
	pub_log_info("[%s][%d] udp_name=[%s]", __FILE__, __LINE__, udp_name);

	pub_log_info("[%s][%d] type=[%d]", __FILE__, __LINE__, cmd.type);
	switch (cmd.type)
	{
		case SW_MSTOPSELF:
		case SW_MSTOPISELF:
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char*)&cmd, sizeof(cmd), udp_name);
			pub_log_info("[%s][%d] stop myself!", __FILE__, __LINE__);
			log_cycle_child_destroy(cycle);
			cycle_destory((sw_cycle_t *)cycle);
			exit(0);
			break;
		default:
			cmd.type = SW_ERRCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name , "%s" , cycle->base.name.data);
			udp_send(cycle->udp_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), udp_name);
			break;
	} 

	return SW_OK;
}

static sw_int32_t log_child_register(sw_log_cycle_t *cycle, sw_int32_t status, sw_char_t *log_name)
{
	sw_int_t	ret = 0;
	sw_proc_info_t	proc_info;

	memset(&proc_info, 0x0, sizeof(proc_info));
	ret = procs_get_proces_info(NULL, cycle->base.name.data, log_name, &proc_info);
	if (ret == SW_ERROR)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		strcpy(proc_info.name, log_name);
		strcpy(proc_info.svr_name, cycle->base.name.data);
		proc_info.pid = getpid();
		proc_info.proc_index = cycle->log_conf.process_index;
		proc_info.type = ND_LOG;
		proc_info.status = status;
		proc_info.restart_type = LIMITED_RESTART;
		proc_info.shmid = cycle->shm.id;
	}
	else
	{
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] prdt_name=[%s]",
				__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.prdt_name);
		proc_info.pid = getpid();
		proc_info.status = status;
		proc_info.shmid = cycle->shm.id;
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	ret = procs_lsn_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_sys_register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

static sw_int_t log_child_work(sw_log_cycle_t *cycle, sw_int32_t index)
{
	sw_int_t i = 0;
	sw_int_t ret = 0;
	sw_int_t	recv_cnt = 0;
	sw_char_t	log_name[64];
	sw_char_t	log_path[128];
	sw_fd_list_t    fd_list;
	sw_fd_list_t *fd_work;
	sw_int64_t	timer = 0;

	memset(log_name, 0x0, sizeof(log_name));
	memset(log_path, 0x0, sizeof(log_path));

	cycle->log_conf.process_index = index;
	sprintf(log_name, "%s_%d", cycle->log_conf.name, index);
	sprintf(log_path, "%s/%s.log", cycle->base.log->log_path.data, log_name);

	memset(g_logname, 0x0, sizeof(g_logname));
	strncpy(g_logname, log_name, sizeof(g_logname) -1 );

	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_log_chglog debug path error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->udp_fd = udp_bind(log_name);
	if (cycle->udp_fd <= 0)
	{
		pub_log_error("[%s][%d] udp_bind error! fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno));
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] udp_bind success! fd=[%d] filename %s",
			__FILE__, __LINE__, cycle->udp_fd, log_name);

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->udp_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)log_deal_cmd;
	ret = select_add_event(cycle->log_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Child: udp_name=[%s] udp_fd=[%d]", __FILE__, __LINE__, log_name, cycle->udp_fd);

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->lsn_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)log_connection;
	ret = select_add_event(cycle->log_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Child:  lsn_fd=[%d]", __FILE__, __LINE__,  cycle->lsn_fd);

	ret = log_child_init();
	if (ret)
	{
		pub_log_error("[%s][%d] child init error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = create_work_thread();
	if (ret)
	{
		pub_log_error("[%s][%d] create worker thread error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = log_child_register(cycle, SW_S_START, log_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Register error! lsn_name=[%s]", __FILE__, __LINE__, log_name);
		return SW_ERROR;
	}

	while (1)
	{
		timer = 60000;
		recv_cnt = select_process_events(cycle->log_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error!", __FILE__, __LINE__);
			continue;
		}
		else if (recv_cnt == 0)
		{
			continue;
		}
		else
		{
			pub_log_info("[%s][%d] Data arrived! recv_cnt=[%d]", __FILE__, __LINE__, recv_cnt);
			for (i = 0; i < recv_cnt; i++)
			{
				pub_log_info("[%s][%d] i=[%d] fd =[%d] udp=[%d]", __FILE__, __LINE__, i, fd_work[i].fd,  cycle->udp_fd);
				if (fd_work[i].fd == cycle->udp_fd)
				{
					ret = fd_work[i].event_handler((sw_log_cycle_t*)fd_work[i].data);
				}
				else
				{
					ret = fd_work[i].event_handler(&fd_work[i]);
				}
				if (ret == SW_ABORT)
				{
					pub_log_info("[%s][%d] Recv exit cmd! Exit!", __FILE__, __LINE__);
					return SW_OK;
				}
				else if(ret != SW_OK)
				{
					if(ret == READ_ERROR)
					{
						select_del_event(cycle->log_fds, fd_work[i].fd);
					}
					pub_log_error("[%s][%d] event_handler error!", __FILE__, __LINE__);
				}
			}
		}
	}

	return -1;
}

static sw_int_t log_create_single_child(sw_log_cycle_t *cycle, sw_int_t index)
{
	pid_t	pid;
	sw_int_t	ret = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		ret = log_child_work(cycle, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_child_work error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		exit(0);
	}

	return SW_OK;
}

sw_int_t log_create_child(sw_log_cycle_t *cycle)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	char	upath[256];
	char	errs[256];

	memset(errs, 0x0, sizeof(errs));
	memset(upath, 0x0, sizeof(upath));
	snprintf(upath, sizeof(upath)-1, "%s/tmp/.%s_.file", getenv("SWWORK"), g_alog_cfg.upath);

	cycle->lsn_fd = anet_unix_server(errs, upath, 0, 256);
	if (cycle->lsn_fd <= 0)
	{
		pub_log_error("[%s][%d] udp_bind error! fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->lsn_fd, errno, strerror(errno));
		return SW_ERROR;
	}
	anet_nonblock(errs, cycle->lsn_fd);
	pub_log_info("[%s][%d]lsn_fd[%d]", __FILE__, __LINE__, cycle->lsn_fd);
	for (i = 0; i < g_alog_cfg.proc_num; i++)
	{
		ret = log_create_single_child(cycle, i);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] log_create_single error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int_t log_handle_timeout(sw_log_cycle_t *cycle)
{
	int     j = 0;
	int     index = 0;
	int     ret = 0;
	long	lnow = 0;
	sw_svr_grp_t    *grp_svr;
	sw_proc_info_t  proc_info;

	grp_svr = procs_get_svr_by_name(cycle->base.name.data, NULL);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] procs_get_svr_by_name error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] svr=[%s]", __FILE__, __LINE__, grp_svr->svrname);

	for (j = 0; j < grp_svr->svc_curr; j++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}

		ret = pub_proc_checkpid(proc_info.pid);
		if (ret != SW_OK && proc_info.restart_cnt >= 0)
		{
			pub_log_info("[%s][%d] Process [%d] abnormal! name=[%s]",
					__FILE__, __LINE__, proc_info.pid, proc_info.name);
			index = proc_info.proc_index;
			pub_log_info("[%s][%d] name=[%s] index=[%d]", __FILE__, __LINE__, proc_info.name, index);
			lnow = (long)time(NULL);
			if (lnow - proc_info.starttime < MIN_RESTART_TIME)
			{
				alert_msg(ERR_OFTEN_RESTART_FAILED, "紧急预警:进程[%s]重启在5分钟内重启多次,请查看渠道日志进行手工处理!", proc_info.name);
				pub_log_error("[%s][%d] [%s][%d] restart_cnt=[%d] Could not restart!",
						__FILE__, __LINE__, proc_info.name, proc_info.pid, proc_info.restart_cnt);
				proc_info.status = SW_S_ABNORMAL;
				proc_info.restart_cnt--;
				ret = procs_lsn_register(&proc_info);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
				}
				continue;
			}
			if (proc_info.restart_cnt == 0)
			{
				alert_msg(ERR_CNT_RESTART_FAILED, "紧急预警:进程[%s]重启次数过多,不能自动重启,请查看渠道日志进行手工处理!", proc_info.name);
				pub_log_error("[%s][%d] [%s][%d] restart_cnt=[%d] Could not restart!",
						__FILE__, __LINE__, proc_info.name, proc_info.pid, proc_info.restart_cnt);
				proc_info.status = SW_S_ABNORMAL;
				proc_info.restart_cnt--;
				ret = procs_lsn_register(&proc_info);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
				}
				continue;
			}
			alert_msg(ERR_PROC_FAILED, "紧急预警:进程[%s]异常,准备重启,请查看渠道日志进行手工处理!", proc_info.name);
			proc_info.status = SW_S_ABNORMAL;
			proc_info.restart_cnt--;
			ret = log_child_register(cycle, proc_info.status,proc_info.name);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
				continue;
			}
			ret = log_create_single_child(cycle, index);
			if(ret != SW_OK)
			{
				pub_log_error("[%s][%d] create single child error!");
				return SW_ERROR;
			}


		}
	}
	return SW_OK;
}

sw_int_t procs_log_register(sw_proc_info_t *proc_info)
{
	return SW_OK;
}
