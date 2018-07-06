#include "lsn_pub.h"

#define MAX_SER_SIZE	(2*1024*1024)

sw_int_t tcpsc_extcevt(sw_fd_list_t *fd_lists);

sw_int_t tcpsc_init()
{
	return 0;
}

sw_int_t tcpsc_destroy()
{
	return 0;
}

static sw_int_t vars_backup(sw_loc_vars_t *vars)
{
	sw_char_t	tmp[256];
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$errcode", tmp);
	vars->set_string(vars, "$orgnerrcode", tmp);

	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$product",  tmp);
	vars->set_string(vars, "$orgnprdt", tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$listen", tmp);
	vars->set_string(vars, "$orgnlsn", tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$date", tmp);
	vars->set_string(vars, "$orgndt", tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$time", tmp);
	vars->set_string(vars, "$orgntm", tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$trace_no", tmp);
	vars->set_string(vars, "$orgntrcno", tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$sys_trace_no", tmp);
	vars->set_string(vars, "$orgnsystrcno", tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$respcd", tmp);
	vars->set_string(vars, "$orgnrspd", tmp);
	
	pub_log_info("[%s][%d] backup sys variable success!", __FILE__, __LINE__);

	return 0;
}

static sw_int_t vars_recover(sw_loc_vars_t *vars)
{
	sw_char_t	tmp[256];
	
	memset(tmp, 0x0, sizeof(tmp));
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgnerrcode",  tmp);
	vars->set_string(vars, "$errcode", tmp);
	vars->remove_var(vars, "$orgnerrcode");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgnprdt",  tmp);

	vars->set_string(vars, "$product", tmp);
	vars->remove_var(vars, "$orgnprdt");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgnlsn", tmp);
	vars->set_string(vars, "$listen", tmp);
	vars->remove_var(vars, "$orgnlsn");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgndt", tmp);
	vars->set_string(vars, "$date", tmp);
	vars->remove_var(vars, "$orgndt");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgntm", tmp);
	vars->set_string(vars, "$time", tmp);
	vars->remove_var(vars, "$orgntm");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgntrcno", tmp);
	vars->set_string(vars, "$trace_no", tmp);
	vars->remove_var(vars, "$orgntrcno");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgnsystrcno", tmp);
	vars->set_string(vars, "$sys_trace_no", tmp);
	vars->remove_var(vars, "$orgnsystrcno");
	
	memset(tmp, 0x0, sizeof(tmp));
	vars->get_var(vars, "$orgnrspd", tmp);
	vars->set_string(vars, "$respcd", tmp);
	vars->remove_var(vars, "$orgnrspd");
	
	pub_log_info("[%s][%d] recover sys variable success!", __FILE__, __LINE__);

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

sw_int_t tcpsc_deal_out_task(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	sw_int_t	ret = 0;
	sw_chnl_t	*chnl = NULL;
	sw_char_t	tmp[32];
	
	chnl = &cycle->chnl;
	memset(locbuf->data, 0x0, locbuf->size);
	memset(&cycle->route, 0x0, sizeof(sw_route_t));
	ret = lsn_get_route_by_name(cycle, cmd->ori_prdt, &cycle->route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get route info error! prdt=[%s]", 
			__FILE__, __LINE__, cmd->ori_prdt);
		return SW_ERROR;
	}
	
	loc_set_zd_data(loc_vars, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(loc_vars, chnl->cache.pkgmap, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}	
	
	if (pub_buf_chksize(locbuf, MAX_SER_SIZE) < 0)
	{
		lsn_set_err(loc_vars, ERR_TOPKG);
		pub_log_error("[%s][%d] Update buf error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] shm_vars_tostream begin...", __FILE__, __LINE__);
	memset(locbuf->data, '0', 8);
	ret = loc_vars->tostream(loc_vars, locbuf->data + 8, locbuf->size);
	if (ret < 0)
	{
		lsn_set_err(loc_vars, ERR_TOPKG);
		pub_log_error("[%s][%d] shm_vars_tostream error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08d", ret);
	memcpy(locbuf->data, tmp, 8);
	ret += 8;
	locbuf->len = ret;
	locbuf->data[ret] = '\0';
	pub_log_info("[%s][%d] shm_vars_tostream success!", __FILE__, __LINE__);
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] packages len=[%d]", 
		__FILE__, __LINE__, locbuf->len);
	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Enc begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(loc_vars, locbuf, SW_ENC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Enc error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After enc: len=[%d]",
			__FILE__, __LINE__, locbuf->len);
	}
	pub_log_info("[%s][%d] deal_task success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t tcpsc_call(cycle, locvar, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *locvar;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	sw_fd_t		sockid = 0;
	sw_int_t	ret = 0;
	sw_int_t	port = 0;
	char	buf[128];
	char	*ip = NULL;
	char	*q  = NULL;
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
	
	vars_backup(locvar);
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
			ret = tcpsc_deal_out_task(cycle, &locvar, &cmd, &locbuf);
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
	pub_log_info("[%s][%d] deal task success! mtype=[%ld]", __FILE__, __LINE__, cmd.mtype);
	pub_buf_clear(&locbuf);
	locvar.free_mem(&locvar);

	return SW_OK;
}

sw_int_t tcpsc_deal_in_task(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	sw_int_t	ret = 0;
	sw_char_t	tmp[128];
	sw_chnl_t	*chnl = NULL;
	
	memset(tmp, 0x0, sizeof(tmp));
	
	chnl = &cycle->chnl;
	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Dec begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(loc_vars, locbuf, SW_DEC);
		if (ret != SW_OK)
		{
			lsn_set_err(loc_vars, ERR_PREDO);
			pub_log_error("[%s][%d] Dec error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After dec: len=[%d]",
			__FILE__, __LINE__, locbuf->len);
        }
	
	ret = lsn_get_route_by_name(cycle, cmd->dst_prdt, &cycle->route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get [%s] route info error!", __FILE__, __LINE__, cmd->dst_prdt);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] shm_vars_destream begin...", __FILE__, __LINE__);	
	ret = loc_vars->destream(loc_vars, locbuf->data + 8);
	if (ret < 0)
	{
		lsn_set_err(loc_vars, ERR_UNSERL);
		pub_log_error("[%s][%d] shm_vars_destream error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] shm_vars_destream success!", __FILE__, __LINE__);
	
	vars_recover(loc_vars);
	
	loc_set_zd_data(loc_vars, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(loc_vars, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			lsn_set_err(loc_vars, ERR_SNDMAP);
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	if (chnl->fun.file_func != NULL)
	{
		ret = chnl->fun.file_func(loc_vars, 1, cycle->accept_fd, cycle->param);
		if (ret != SW_OK)
		{
			lsn_set_err(loc_vars, ERR_RCVFILE);
			pub_log_error("[%s][%d] do file func error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	if (cmd->type == SW_LINKLSNREQ)	
	{
		pub_log_info("[%s][%d] LINKLSNREQ! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);
		mtype_delete(cmd->mtype, 0);
		return SW_OK;
	}
	
	if (cmd->type == SW_POSTLSNREQ)	
	{
		pub_log_info("[%s][%d] POSTLSNREQ! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);
		mtype_delete(cmd->mtype, 0);
		return SW_OK;
	}
	
	pub_mem_memzero(locbuf->data, locbuf->size);
	

	switch (cmd->type)	
	{
		case SW_CALLLSNREQ:
			cmd->type = SW_CALLLSNRES;
			break;
		case SW_LINKLSNREQ:
			cmd->type = SW_LINKLSNRES;
			break;
		case SW_POSTLSNREQ:
			cmd->type = SW_POSTLSNRES;
			break;
		default:
			cmd->type = SW_POSTLSNRES;
			break;
	}
	cmd->ori_type = ND_LSN;
	cmd->dst_type = ND_SVC;
	cmd->msg_type = SW_MSG_RES;
	cmd->task_flg = SW_FORGET;
	strcpy(cmd->ori_svr, cycle->base.name.data);
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	strcpy(cmd->ori_svc, tmp);
	pub_log_debug("[%s][%d] cmd->lsn_name=[%s]", __FILE__, __LINE__, cmd->lsn_name);
	ret = route_snd_dst(loc_vars, cycle->base.global_path, cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] route_snd_dst error! type=[%d] mtype=[%ld]",
			__FILE__, __LINE__, cmd->type, cmd->mtype);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Send msg to svc success!", __FILE__, __LINE__);
	
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
	strncpy(cmd.dst_prdt, cycle->link_info[sockid].prdt, sizeof(cmd.dst_prdt) - 1);
	strncpy(cmd.def_name, cycle->link_info[sockid].def_name, sizeof(cmd.def_name) - 1);
	pub_log_info("[%s][%d] dst_prdt=[%s] def_name=[%s]", 
		__FILE__, __LINE__, cmd.dst_prdt, cmd.def_name);
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
	
	ret = tcpsc_deal_in_task(cycle, &locvar, &cmd, &locbuf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		memset(buf, 0x0, sizeof(buf));
		lsn_get_err(&locvar, buf);
		pub_log_debug("[%s][%d] errcode=[%s]", __FILE__, __LINE__, buf);
		if (strlen(buf) == 0 || strcmp(buf, "0000") == 0)
		{
			lsn_set_err(&locvar, ERREVERY);
		}
		lsn_deal_err(cycle, &locvar, &cmd);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, sockid);
		locvar.free_mem(&locvar);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	trace_insert(&locvar, &cmd, TRACE_OUT);
	cycle->link_info[sockid].use = 2;
	lsn_pub_close_fd(cycle, sockid);
	locvar.free_mem(&locvar);
	pub_buf_clear(&locbuf);

	return SW_DELETE;
}

