#include "lsn_pub.h"

sw_int_t tcpsc_extcevt(sw_fd_list_t *fd_lists);

sw_int_t tcpsc_init()
{
	return 0;
}

sw_int_t tcpsc_destroy()
{
	return 0;
}

sw_int_t tcpsc_timeoutevt(sw_lsn_cycle_t *cycle)
{
	sw_int_t ret = SW_ERROR;
	
	ret = lsn_pub_deal_timeout(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d]timeout error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	return SW_OK;
}

sw_int_t tcpsc_call(cycle, locvar, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *locvar;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	int		port;
	char	*q = NULL;
	char	*ip = NULL;
	char	buf[128];
	sw_fd_t		sockid = 0;
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	
	if (cycle == NULL || locvar == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(buf, 0x00, sizeof(buf));
	if (cycle->lsn_conf.comm.group[0].remote.ip[0] == '$' || cycle->lsn_conf.comm.group[0].remote.ip[0] == '#')
	{
		loc_get_zd_data(locvar, cycle->lsn_conf.comm.group[0].remote.ip, buf);
		ip = buf;
		q  = strchr(buf, ':');
		if (q == NULL)
		{
			pub_log_error("[%s][%d] ip/port[%s] configure format error, usage[IP:PORT]", __FILE__, __LINE__, buf);
			return SW_ERROR;
		}
		port = atoi(q+1);
		ip[q-ip] = '\0';
		sockid = lsn_pub_connect(ip, port);
	}
	else
	{
		sockid = lsn_pub_connect(cycle->lsn_conf.comm.group[0].remote.ip, cycle->lsn_conf.comm.group[0].remote.port);
	}
	
	if (sockid < 0)
	{
		lsn_set_err(locvar, ERR_CONNECT);
		pub_log_error("[%s][%d] connect [%s]:[%d] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.ip, 
			cycle->lsn_conf.comm.group[0].remote.port, errno, strerror(errno));
		return SW_ERROR;
	}
	
	if (lsn_set_fd_noblock(sockid) < 0)
	{
		pub_log_error("[%s][%d] noblock [%d] error, errno=[%d]:[%s]",
			__FILE__, __LINE__, sockid, errno, strerror(errno));
	}

	if (cycle->lsn_conf.socksndbufsize > 0)
	{
		socklen_t       optlen = sizeof(sw_int32_t);
		sw_int32_t      optval = cycle->lsn_conf.socksndbufsize;
		errno = 0;
		setsockopt(sockid, SOL_SOCKET, SO_SNDBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] socksndbufsize=[%ld] errno=[%d]:[%s]",
			__FILE__, __LINE__, optval, errno, strerror(errno));
	}

	if (cycle->lsn_conf.sockrcvbufsize > 0)
	{
		socklen_t       optlen = sizeof(sw_int32_t);
		sw_int32_t      optval = cycle->lsn_conf.sockrcvbufsize;
		errno = 0;
		setsockopt(sockid, SOL_SOCKET, SO_RCVBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] sockrcvbufsize=[%ld] errno=[%d]:[%s]",
			__FILE__, __LINE__, optval, errno, strerror(errno));
	}
	
	if (cycle->chnl.fun.start_func != NULL)
	{
		ret = cycle->chnl.fun.start_func(locvar, sockid);
		if (ret < 0)
		{
			lsn_set_err(locvar, ERR_SEND);
			pub_log_error("[%s][%d] tcpsc run start fun error! SockId=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, sockid, errno, strerror(errno));
			close(sockid);
			return SW_ERROR;
		}
	}

	memcpy(&cycle->link_info[sockid].cmd, cmd, sizeof(sw_cmd_t));
	ret = lsn_pub_send(sockid , locbuf->data, locbuf->len);
	if (ret != SW_OK)
	{
		lsn_set_err(locvar, ERR_SEND);
		pub_log_error("[%s][%d] tcpsc send msg error! SockId=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, sockid, errno, strerror(errno));
		close(sockid);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] send pkg success! len=[%d]", 
		__FILE__, __LINE__, locbuf->len);
	if (cycle->chnl.fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] Send file begin...", __FILE__, __LINE__);
		ret = cycle->chnl.fun.file_func(locvar, 0, sockid, NULL);
		if(ret < 0)
		{
			lsn_set_err(locvar, ERR_SNDFILE);
			pub_log_error("[%s][%d] send file error!", __FILE__, __LINE__);
			close(sockid);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] Send file success!", __FILE__, __LINE__);
	}
	
	if (cmd->task_flg == SW_DEL)
	{
		close(sockid);
		pub_log_info("[%s][%d] TASK DELETE!", __FILE__, __LINE__);
		mtype_delete(cmd->mtype, 0);
		return SW_OK;
	}
	cycle->link_info[sockid].mtype = cmd->mtype;
	cycle->link_info[sockid].sockid = sockid;
	cycle->link_info[sockid].trace_no = cmd->trace_no;
	cycle->link_info[sockid].cmd_type = cmd->type;
	cycle->link_info[sockid].start_time = (sw_int_t)time(NULL);
	cycle->link_info[sockid].use = 1;
	cycle->link_info[sockid].timeout = cmd->timeout;
	memcpy(&cycle->link_info[sockid].cmd, cmd, sizeof(sw_cmd_t));
	pub_log_debug("[%s][%d] @@@@@@@@@@ ori_prdt=[%s]", __FILE__, __LINE__, cmd->ori_prdt);
	strncpy(cycle->link_info[sockid].prdt, cmd->ori_prdt, sizeof(cycle->link_info[sockid].prdt) - 1);
	strncpy(cycle->link_info[sockid].def_name, cmd->def_name, sizeof(cycle->link_info[sockid].def_name) - 1);

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = sockid;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)tcpsc_extcevt;

	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t tcpsc_interevt(sw_fd_list_t *fd_lists)
{
	long	mytype = 0;
	sw_fd_t		out_fd = 0;
	sw_int_t	len = 0;
	sw_int_t	ret = 0;
	sw_int_t	err_flag = 0;
	sw_char_t	buf[128];
	sw_char_t	errcode[32];
	sw_buf_t	locbuf;
	sw_cmd_t	cmd;
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error! fd_lists is null!", __FILE__, __LINE__);
		return SW_CONTINUE;
	}
	
	memset(buf, 0x0, sizeof(buf));
	memset(errcode, 0x0, sizeof(errcode));
	memset(&cmd, 0x0, sizeof(cmd));
	memset(&locvar, 0x0, sizeof(locvar));
	
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	out_fd = fd_lists->fd;
	
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_loc_vars_construct error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	len = 0;
	ret = msg_trans_rcv(out_fd, (char*)&cmd, &mytype, &len);
	if (ret != SW_OK)
	{
		locvar.free_mem(&locvar);
		pub_log_error("[%s][%d] recv cmd error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_CONTINUE;
	}
	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "SHM%08ld", cmd.mtype);
	ret = locvar.unserialize(&locvar, buf);
	if (ret != SW_OK)
	{
		locvar.free_mem(&locvar);
		pub_log_error("[%s][%d] unserialize error.", __FILE__,__LINE__);
		return SW_ERROR;
	}
	
	err_flag = 0;
	cmd_print(&cmd);
	trace_insert(&locvar, &cmd, TRACE_IN);
	pub_buf_init(&locbuf);
	switch (cmd.type)
	{
		case SW_POSTLSNREQ:
		case SW_CALLLSNREQ:
		case SW_LINKLSNREQ:
			ret = lsn_pub_deal_out_task(cycle, &locvar, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]lsn_pub_platform_deal error",__FILE__,__LINE__);
				break;
			}
			pub_log_info("[%s][%d] deal out task success!", __FILE__, __LINE__);
			ret = tcpsc_call(cycle, &locvar, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]send data error",__FILE__,__LINE__);
				break;
			}
			break;
		default: 
			pub_log_error("[%s][%d]receive illegal cmd	itype=[%d]",__FILE__,__LINE__,cmd.type);
			cmd_print(&cmd);
			cmd.type = SW_ERRCMD;
			cmd.ori_type = ND_LSN;
			cmd.dst_type = ND_SVC;
			cmd.msg_type = SW_MSG_RES;
			cmd.task_flg = SW_FORGET;
			strcpy(cmd.ori_svr, cycle->base.name.data);
			sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
			strcpy(cmd.ori_svc, buf);
			ret = route_snd_dst(&locvar, cycle->base.global_path, &cmd);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] send msg to svc error!", __FILE__, __LINE__);
			}
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
	pub_log_info("[%s][%d] deal task success! mtype=[%ld]", __FILE__, __LINE__, cmd.mtype);

	return SW_OK;
}

sw_int_t tcpsc_extcevt(sw_fd_list_t *fd_lists)
{
	sw_fd_t	sockid = 0;
	sw_int_t	ret = 0;
	sw_cmd_t	cmd;
	sw_buf_t	locbuf;
	sw_char_t	buf[128];
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error! NULL!", __FILE__, __LINE__);
		return SW_DELETE;
	}

	memset(buf, 0x0, sizeof(buf));
	memset(&cmd, 0x0, sizeof(cmd));

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	cycle->accept_fd = fd_lists->fd;
	sockid = cycle->accept_fd;

	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_loc_vars_construct error.", __FILE__, __LINE__);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, sockid);
		return SW_ERROR;
	} 

	ret = locvar.create(&locvar, cycle->link_info[sockid].mtype);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d]recover var pool error, mtype=[%ld]", 
			__FILE__,__LINE__, cycle->link_info[sockid].mtype);
		locvar.free_mem(&locvar);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, sockid);
		return SW_ERROR;
	}
	
	cmd.mtype = cycle->link_info[sockid].mtype;
	cmd.trace_no = cycle->link_info[sockid].trace_no;
	cmd.type = cycle->link_info[sockid].cmd_type;
	strcpy(cmd.sys_date, cycle->link_info[sockid].cmd.sys_date);
	strncpy(cmd.dst_prdt, cycle->link_info[sockid].prdt, sizeof(cmd.dst_prdt) - 1);
	strncpy(cmd.def_name, cycle->link_info[sockid].def_name, sizeof(cmd.def_name) - 1);
	pub_log_info("[%s][%d] mtype=[%ld] trace_no=[%lld] type=[%d]dst_prdt=[%s] def_name=[%s], date=[%s] ", 
			__FILE__, __LINE__, cmd.mtype, cmd.trace_no, cmd.type, cmd.dst_prdt, cmd.def_name, cmd.sys_date);

	pub_buf_init(&locbuf);
	ret = lsn_pub_recv(cycle->lsn_conf.data, sockid, &locbuf);
	if (ret <= SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_recv error!", __FILE__, __LINE__);
		lsn_set_err(&locvar, ERR_RECV);
		lsn_deal_err(cycle, &locvar, &cmd);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, sockid);
		locvar.free_mem(&locvar);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "Recv data:[%s][%d] len[%d]",
		__FILE__, __LINE__, locbuf.len);
	
	ret = lsn_pub_deal_in_task(cycle, &locvar, &cmd, &locbuf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		lsn_deal_err(cycle, &locvar, &cmd);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, sockid);
		locvar.free_mem(&locvar);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	trace_insert(&locvar, &cmd, TRACE_OUT);
	cycle->link_info[sockid].use = 2;
	select_del_event(cycle->lsn_fds, sockid);
	lsn_pub_close_fd(cycle, sockid);
	locvar.free_mem(&locvar);
	pub_buf_clear(&locbuf);

	return SW_OK;
}

