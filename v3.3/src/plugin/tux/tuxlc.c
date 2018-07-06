#include "lsn_pub.h"
#include "atmi.h"

#define S_IDLE    0
#define S_SENT    1
#define S_FAILED  2
#define LIMIT_CNT	5
#define CONN_TIMEOUT	120

int	reconn = 0;
char	*send_buf = NULL;
char	*recv_buf = NULL;
long	g_conn_time = 0;
int	g_task_cnt = 0;

sw_int_t tuxlc_close_link(sw_lsn_cycle_t *cycle, int index);
sw_int_t tuxlc_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_DELETE;
}

sw_int_t tuxlc_destroy(sw_lsn_cycle_t *cycle)
{
	if (send_buf != NULL)
	{
		tpfree(send_buf);
	}

	if (recv_buf != NULL)
	{
		tpfree(recv_buf);
	}

	tpterm();
	pub_log_info("[%s][%d] tuxlcm destroy...", __FILE__, __LINE__);

	return 0;
}

sw_int_t tuxlc_init(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	char	buftype[32];

	memset(buftype, 0x0, sizeof(buftype));
	if (cycle->lsn_conf.comm.group[0].buftype[0] != '\0')
	{
		strcpy(buftype, cycle->lsn_conf.comm.group[0].buftype);
	}
	else
	{
		strcpy(buftype, "STRING");
	}

	send_buf = (char *)tpalloc(buftype, NULL, BUF_MAXLEN);
	if (send_buf == NULL)
	{
		pub_log_error("[%s][%d] tpalloc error! tperrno=[%d]:[%s]", 
				__FILE__, __LINE__, tperrno, tpstrerror(tperrno)); 
		return SW_ERROR;
	}

	recv_buf = (char *) tpalloc(buftype, NULL, BUF_MAXLEN);
	if (recv_buf == NULL)
	{
		tpfree(send_buf);
		pub_log_error("[%s][%d] tpalloc error! tperrno=[%d]:[%s]", 
				__FILE__, __LINE__, tperrno, tpstrerror(tperrno)); 
		return SW_ERROR;
	}
	return 0;
}

sw_int_t tuxlc_reconn(sw_lsn_cycle_t *cycle)
{
	if (reconn == 1)
	{
		g_conn_time = (long)time(NULL);
		tpterm();
		if (tpinit((TPINIT *)NULL) == -1)
		{
			reconn = 1;
			pub_log_error("[%s][%d] tpinit error! IP=[%s] PORT=[%d] tperrno=[%d]:[%s]",
					__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.ip, 
					cycle->lsn_conf.comm.group[0].remote.port, tperrno, tpstrerror(tperrno));
			return SW_ERROR;
		}
		reconn = 0;
		pub_log_info("[%s][%d] tpinit success! [%s]:[%d]",
				__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.ip, 
				cycle->lsn_conf.comm.group[0].remote.port);
	}

	return SW_OK;
}

sw_int_t tuxlc_deal_timeout(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	sw_int_t	ret = 0;
	sw_int32_t	timeout = 0;
	sw_char_t	buf[64];
	sw_time_t	now_time = 0;
	sw_cmd_t	cmd;
	sw_loc_vars_t	vars;

	if (cycle == NULL)
	{
		pub_log_info("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].use == S_SENT)
		{
			now_time = (sw_int_t)time(NULL);
			timeout = cycle->link_info[i].timeout;
			if (timeout == 0)
			{
				timeout = cycle->lsn_conf.timeout;
			}
			if (cycle->link_info[i].start_time != 0 && now_time - cycle->link_info[i].start_time > timeout) 
			{
				if (cycle->link_info[i].sockid > 0)
				{
					tpcancel(cycle->link_info[i].sockid);
					pub_log_debug("[%s][%d] tpcancel [%d], tperrno=[%d]:[%s]", 
							__FILE__, __LINE__, cycle->link_info[i].sockid, tperrno, tpstrerror(tperrno));
				}

				pub_log_info("[%s][%d] link[%d] mtype:[%d] TIMEOUT!", 
						__FILE__, __LINE__, i, cycle->link_info[i].mtype);
				if (cycle->link_info[i].cmd_type == SW_CALLLSNREQ)
				{
					pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%d]",
							__FILE__, __LINE__, cycle->link_info[i].mtype);
					ret = pub_loc_vars_alloc(&vars, SHM_VARS);
					if (ret == SW_ERROR)
					{
						pub_log_error("[%s][%d] pub_loc_vars_alloc error!",
								__FILE__, __LINE__);
						continue;
					}
					pub_log_info("[%s][%d] vars alloc success! mtype=[%d]", 
							__FILE__, __LINE__, cycle->link_info[i].mtype);

					memset(buf, 0x0, sizeof(buf));
					sprintf(buf, "shm%08d", cycle->link_info[i].mtype);
					ret = vars.unserialize(&vars, buf);
					if (ret == SW_ERROR)
					{
						vars.free_mem(&vars);
						pub_log_error("[%s][%d] vars unserialize error! mtype=[%d]",
								__FILE__, __LINE__, cycle->link_info[i].mtype);
						continue;
					}
					pub_log_info("[%s][%d] vars unserialize success! mtype=[%d]",
							__FILE__, __LINE__, cycle->link_info[i].mtype);

					char    chnl[64];
					memset(chnl, 0x0, sizeof(chnl));
					loc_get_zd_data(&vars, "$listen", chnl);
					alog_set_sysinfo(cycle->link_info[i].mtype, cycle->link_info[i].cmd.sys_date,
							cycle->link_info[i].cmd.trace_no, chnl);
					lsn_set_err(&vars, ERR_LSNOUT);
					memcpy(&cmd, &cycle->link_info[i].cmd, sizeof(sw_cmd_t));
					lsn_deal_err(cycle, &vars, &cmd);
					vars.free_mem(&vars);
				}
				tuxlc_close_link(cycle, i);
				pub_log_info("[%s][%d] close link[%d]", __FILE__, __LINE__, i);
				pub_log_bend("\0");
				continue;
			}
		}
	}
	return SW_OK;
}

sw_int_t tuxlc_timeoutevt(sw_lsn_cycle_t *cycle)
{
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error! cycle is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	int	ret = 0;
	long	now = 0;

	now = (long)time(NULL);
	if (now - g_conn_time > CONN_TIMEOUT)
	{
		tuxlc_reconn(cycle);
	}

	ret = tuxlc_deal_timeout(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal recv timeout error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;

}

sw_int_t tuxlc_call(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_cmd_t *cmd, sw_buf_t *locbuf)
{
	int	i = 0;
	int	len = 0;
	int	err = 0;
	int	ret = 0;
	int	port = 0;
	int	times = 0;
	int	sockid = 0;
	sw_char_t	tmp[128];
	sw_char_t	service[NAME_LEN];

	memset(service, 0x0, sizeof(service));
	if (cycle == NULL || locvar == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(g_env_buf, 0x0, sizeof(g_env_buf));
	if (cycle->lsn_conf.comm.group[0].remote.ip[0] == '$' || cycle->lsn_conf.comm.group[0].remote.ip[0] == '$')
	{
		memset(tmp, 0x0, sizeof(tmp));
		loc_get_zd_data(locvar, cycle->lsn_conf.comm.group[0].remote.ip, tmp);
		char *p = NULL;
		char *q = NULL;
		p = tmp;
		q = strchr(p, ':');
		if (q == NULL)
		{
			pub_log_error("[%s][%d] ip/port[%s] format error, usage:[IP:PORT]", __FILE__, __LINE__, tmp);
			return SW_ERROR;
		}
		p[q-p] = '\0';
		port = atoi(q+1);
		sprintf(g_env_buf, "WSNADDR=//%s:%d", p, port);
	}
	else
	{
		sprintf(g_env_buf, "WSNADDR=//%s:%d", cycle->lsn_conf.comm.group[0].remote.ip, cycle->lsn_conf.comm.group[0].remote.port);
	}
	ret = putenv(g_env_buf);
	if (ret)
	{
		pub_log_error("[%s][%d] pubenv error! env=[%s]", __FILE__, __LINE__, g_env_buf);
		return SW_ERROR;
	}

	if (tpinit((TPINIT *)NULL) == -1)
	{
		reconn = 1;
		pub_log_error("[%s][%d] tpinit error! IP=[%s] PORT=[%d] tperrno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.ip, 
			cycle->lsn_conf.comm.group[0].remote.port, tperrno, tpstrerror(tperrno));
	}
	g_conn_time = (long)time(NULL);

	if (cycle->chnl.fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] do file_func begin...", __FILE__, __LINE__);
		ret = cycle->chnl.fun.file_func(locvar, 0, 0, NULL);
		if(ret < 0)
		{
			lsn_set_err(locvar, ERR_SNDFILE);
			pub_log_error("[%s][%d] send file error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] send file success!", __FILE__, __LINE__);

	}

	memset(service, 0x0, sizeof(service));
	if (cycle->lsn_conf.comm.group[0].remote.service[0] == '#')
	{
		locvar->get_var(locvar, cycle->lsn_conf.comm.group[0].remote.service, service);
	}
	else
	{
		strncpy(service, cycle->lsn_conf.comm.group[0].remote.service, sizeof(service) - 1);
	}

	memset(send_buf, 0x0, BUF_MAXLEN);
	len = locbuf->len;
	memcpy(send_buf, locbuf->data, len);
	pub_log_bin(SW_LOG_DEBUG, send_buf, len, "[%s][%d] will send data:[%d]", __FILE__, __LINE__, len);
	err = 0;
	times = 0;
	while (1)
	{
		sockid = tpacall(service, send_buf, len, TPNOBLOCK);
		if (sockid < 0)
		{
			pub_log_error("[%s][%d] tpacall [%s] error! tperrno=[%d]:[%s]",
					__FILE__, __LINE__, service, tperrno, tpstrerror(tperrno));
			if (tperrno == TPELIMIT)
			{
				err = 1;
				pub_log_debug("[%s][%d] Too many handles outstanding!", __FILE__, __LINE__);
				break;
			}

			if (tperrno != TPESYSTEM)
			{
				err = 1;
				pub_log_error("[%s][%d] tpacall [%s] error! tperrno=[%d]:[%s]",
						__FILE__, __LINE__, service, tperrno, tpstrerror(tperrno));
				break;
			}

			times++;
			if (times > 1)
			{
				err = 1;
				pub_log_error("[%s][%d] 重连后仍发送失败!", __FILE__, __LINE__);
				break;
			}
			reconn = 1;
			ret = tuxlc_reconn(cycle);
			if (ret == SW_OK)
			{
				pub_log_info("[%s][%d] 重连成功!", __FILE__, __LINE__);
				continue;
			}
			err = 1;
			pub_log_error("[%s][%d] tpacall [%s] error! tperrno=[%d]:[%s]",
					__FILE__, __LINE__, service, tperrno, tpstrerror(tperrno));
			break;
		}
		break;
	}

	if (err == 1)
	{
		lsn_set_err(locvar, ERR_TPCALL);
		pub_log_error("[%s][%d] tpacall error.", __FILE__, __LINE__);
		return SW_ERROR;
	}


	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].use == S_IDLE)
		{
			break;
		}
	}
	if (i == cycle->lsn_conf.conmax)
	{
		pub_log_error("[%s][%d] Have no enougt space to save link info!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_task_cnt++;
	cycle->link_info[i].use = S_SENT;
	cycle->link_info[i].sockid = sockid;
	cycle->link_info[i].mtype = cmd->mtype;
	cycle->link_info[i].trace_no = cmd->trace_no;
	cycle->link_info[i].cmd_type = cmd->type;
	cycle->link_info[i].start_time = (sw_int_t)time(NULL);
	cycle->link_info[i].timeout = cmd->timeout;
	strncpy(cycle->link_info[i].prdt, cmd->ori_prdt, sizeof(cycle->link_info[i].prdt) - 1);
	strncpy(cycle->link_info[i].def_name, cmd->def_name, sizeof(cycle->link_info[i].def_name) - 1);
	memcpy(&cycle->link_info[i].cmd, cmd, sizeof(sw_cmd_t));
	memset(cycle->link_info[i].def_name, 0x0, sizeof(cycle->link_info[i].def_name));
	strncpy(cycle->link_info[i].def_name, service, sizeof(cycle->link_info[i].def_name) - 1);
	pub_log_info("[%s][%d] tuxlc_call success! index=[%d] mtype=[%ld] traceno=[%lld]",
			__FILE__, __LINE__, i, cmd->mtype, cmd->trace_no);

	return SW_OK;
}

sw_int_t tuxlc_deal_recv_work(sw_lsn_cycle_t *cycle, char *recvbuf, int recvlen, int index)
{
	sw_int_t	ret = 0;
	sw_cmd_t	cmd;
	sw_buf_t	locbuf;
	sw_loc_vars_t	locvar;

	memset(&cmd, 0x0, sizeof(cmd));
	memset(&locbuf, 0x0, sizeof(locbuf));
	memset(&locvar, 0x0, sizeof(locvar));

	if (cycle == NULL || recvbuf == NULL || recvlen <= 0 || index < 0)
	{
		pub_log_error("[%s][%d] tuxlc param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] vars alloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = locvar.create(&locvar, cycle->link_info[index].mtype);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] create vars error!", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}

	memset(&locbuf, 0x0, sizeof(locbuf));
	locbuf.data = recvbuf;
	locbuf.len = recvlen;
	cmd.mtype = cycle->link_info[index].mtype;
	cmd.trace_no = cycle->link_info[index].trace_no;
	cmd.type = cycle->link_info[index].cmd_type;
	strncpy(cmd.dst_prdt, cycle->link_info[index].prdt, sizeof(cmd.dst_prdt) - 1);
	strncpy(cmd.def_name, cycle->link_info[index].cmd.def_name, sizeof(cmd.def_name) - 1);
	strncpy(cmd.lsn_name, cycle->link_info[index].cmd.lsn_name, sizeof(cmd.lsn_name) - 1);
	strncpy(cmd.sys_date, cycle->link_info[index].cmd.sys_date, sizeof(cmd.sys_date) - 1);
	ret = lsn_pub_deal_in_task(cycle, &locvar, &cmd, &locbuf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		lsn_deal_err(cycle, &locvar, &cmd);
		tuxlc_close_link(cycle, index);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	trace_insert(&locvar, &cmd, TRACE_OUT);
	tuxlc_close_link(cycle, index);
	locvar.free_mem(&locvar);

	return SW_OK;
}

sw_int_t tuxlc_send_work(sw_lsn_cycle_t *cycle)
{
	sw_int_t	err_flag = 0;
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	long		cmd_type = 0;
	sw_char_t       buf[64];
	sw_char_t	errcode[8];
	sw_buf_t	locbuf;
	sw_cmd_t	cmd;
	sw_loc_vars_t	locvar;

	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(errcode, 0x0, sizeof(errcode));
	memset(&locvar, 0x0, sizeof(locvar));

	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{		
		pub_log_error("[%s][%d] pub_loc_vars_alloc error.", __FILE__, __LINE__);
		return SW_CONTINUE;
	}

	memset(&cmd, 0x00, sizeof(sw_cmd_t));
	ret = msg_trans_rcv(cycle->lsn_conf.out_fd, (char *)&cmd, &cmd_type, &len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv cmd info error!", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		return SW_CONTINUE;
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "SHM%08ld", cmd.mtype);
	ret = locvar.unserialize((void *)&locvar, buf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] vars.unserialize error.", __FILE__,__LINE__);
		locvar.free_mem(&locvar);
		return SW_CONTINUE;
	}
	pub_log_info("[%s][%d] unserialize success!", __FILE__, __LINE__);

	cmd_print(&cmd);
	trace_insert(&locvar, &cmd, TRACE_IN);
	g_trace_flag = -1;
	err_flag = -1;
	pub_buf_init(&locbuf);
	switch (cmd.type)
	{
		case SW_LINKNULL:
			pub_log_info("[%s][%d] LINKNULL...", __FILE__, __LINE__);
			ret = lsn_deal_linknull(cycle, &cmd);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] deal linknull error!", __FILE__, __LINE__);
			}
			break;
		case SW_CALLLSNREQ:
		case SW_LINKLSNREQ:
		case SW_POSTLSNREQ:
		case SW_TASKRES:
			ret = lsn_pub_deal_out_task(cycle, &locvar, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_info("[%s][%d] deal out task error!", __FILE__, __LINE__);
				break;
			}
			pub_log_info("[%s][%d] deal_out_task success!", __FILE__, __LINE__);
			ret = tuxlc_call(cycle, &locvar, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]send data error", __FILE__, __LINE__);
				break;
			}
			break;
		case SW_TASKERR:
			pub_log_info("[%s][%d] TASKERR! mtype=[%ld]", __FILE__, __LINE__, cmd.mtype);
			break;
		default:
			pub_log_info("[%s][%d] error message itype=[%d]", __FILE__, __LINE__, cmd.type);
			cmd_print(&cmd);
			cmd.type = SW_ERRCMD;
			cmd.ori_type = ND_LSN;
			cmd.dst_type = ND_SVC;
			cmd.msg_type = SW_MSG_RES;
			cmd.task_flg = SW_FORGET;
			strcpy(cmd.ori_svr, cycle->base.name.data);
			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
			strcpy(cmd.ori_svc, buf);
			ret = route_snd_dst(&locvar, cycle->base.global_path, &cmd);
			if (ret < 0)
			{
				pub_buf_clear(&locbuf);
				locvar.free_mem(&locvar);
				pub_log_error("[%s][%d],send message to BP error ret=[%d]",
						__FILE__,__LINE__, ret);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d],send task success itype[%d]mtype[%ld] ",
				__FILE__, __LINE__, cmd.type, cmd.mtype);
			break;
	}
	if (err_flag == 1)
	{
		memset(errcode, 0x0, sizeof(errcode));
		lsn_get_err(&locvar, errcode);
		pub_log_debug("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);
		if (strlen(errcode) == 0 || strcmp(errcode, "0000") == 0)
		{
			lsn_set_err(&locvar, ERREVERY);
		}
		lsn_deal_err(cycle, &locvar, &cmd);
		pub_log_error("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);
	}
	pub_buf_clear(&locbuf);
	locvar.free_mem(&locvar);

	return SW_OK;
}

sw_int_t tuxlc_recv_work(sw_lsn_cycle_t *cycle)
{
	int ret = 0;
	int index = 0;
	int sockid = 0;
	long	recvlen = 0;

	for (index = 0; index < cycle->lsn_conf.conmax; index++)
	{
		if (cycle->link_info[index].use != S_SENT || cycle->link_info[index].sockid <= 0)
		{
			continue;
		}

		sockid = cycle->link_info[index].sockid;
		ret = tpgetrply(&sockid, (char **)&recv_buf, &recvlen, TPNOBLOCK);
		if (ret < 0)
		{
			if (tperrno != TPEBLOCK)
			{
				g_task_cnt--;
				cycle->link_info[index].use = S_FAILED;
				pub_log_error("[%s][%d] Recv data error! errno=[%d]:[%s]",
						__FILE__, __LINE__, tperrno, tpstrerror(tperrno));
			}
			continue;
		}

		g_task_cnt--;
		pub_log_bin(SW_LOG_DEBUG, recv_buf, recvlen, "[%s][%d] Recv data:[%d] idx=[%d] sockid=[%d] mtype=[%ld] trc=[%lld]", 
				__FILE__, __LINE__, recvlen, index, sockid, cycle->link_info[index].mtype, cycle->link_info[index].trace_no);
		ret = tuxlc_deal_recv_work(cycle, recv_buf, recvlen, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] deal recv req error!", __FILE__, __LINE__);
			pub_log_bend("\0");
			continue;
		}

		pub_log_info("[%s][%d] deal_recv_work success!", __FILE__, __LINE__);
		pub_log_bend("\0");
	}

	return SW_ERROR;

}

sw_int_t tuxlc_interevt(sw_fd_list_t *fd_lists)
{
	int	i = 0;
	int	ret = 0;
	int	timer = 0;
	int	recv_cnt = 0;
	char	lsn_name[64];
	sw_fd_list_t    fd_list;
	sw_fd_list_t	*fd_work;
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->lsn_conf.out_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)tuxlc_send_work;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}

	memset(lsn_name, 0x00, sizeof(lsn_name));
	sprintf(lsn_name, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	while(1)
	{
		if (getppid() == 1)
		{
			pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
			lsn_cycle_destroy(cycle);
			exit(0);
		}

		if (cycle->handler.timeout_handler != NULL)
		{
			cycle->handler.timeout_handler(cycle);
		}

		timer = cycle->lsn_conf.scantime > 0 ? cycle->lsn_conf.scantime : 1000;
		recv_cnt = select_process_events(cycle->lsn_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
		}
		else if (recv_cnt == 0)
		{
			tuxlc_deal_timeout(cycle);
			if (procs_update_process_busystatus(NULL, cycle->base.name.data, lsn_name, PROC_S_IDLE) != SW_OK)
			{
				pub_log_error("[%s][%d] update [%s] busystats error!",__FILE__, __LINE__, lsn_name);
			}
		}

		else if (recv_cnt > 0)
		{
			for (i = 0; i < recv_cnt; i++)
			{
				if (fd_work[i].fd == cycle->udp_fd)
				{
					ret = fd_work[i].event_handler(fd_work[i].data);
				}
				else
				{
					ret = fd_work[i].event_handler(fd_work[i].data);
				}

				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] handle event error! ret=[%d]",__FILE__, __LINE__, ret); 
				}
				pub_log_bend("\0");

			}
		}

		tuxlc_recv_work(cycle);
	}
}

sw_int_t tuxlc_close_link(sw_lsn_cycle_t *cycle, int index)
{
	memset(&cycle->link_info[index], 0x0, sizeof(sw_link_info_t));
	cycle->link_info[index].mtype = -1;
	cycle->link_info[index].sockid = -1;
	cycle->link_info[index].trace_no = 0;
	cycle->link_info[index].cmd_type = -1;
	cycle->link_info[index].start_time = -1;
	cycle->link_info[index].use = S_IDLE;

	return 0;
}

