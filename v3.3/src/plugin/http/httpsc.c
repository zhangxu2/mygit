#include "lsn_pub.h"
#include "http.h"

extern sw_int_t httpsc_init_ssl(sw_lsn_cycle_t *cycle);

sw_int_t httpsc_extcevt(sw_fd_list_t *fd_lists);

sw_int_t httpsc_init(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = 0;

	g_is_ssl = 0;
#if defined(SW_USE_OPENSSL)
	if (strcmp(cycle->lsn_conf.comtype, "HTTPSSC") == 0 ||
		strncmp(cycle->lsn_conf.comm.group[0].remote.ip, "https:", 6) == 0)
	{
		g_is_ssl = 1;
	}
	
	if (g_is_ssl)
	{
		ret = httpsc_init_ssl(cycle);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Httpsc init ssl error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] httpsc init ssl success!", __FILE__, __LINE__);
	}
#endif
	return 0;
}

sw_int_t httpsc_destroy(sw_lsn_cycle_t *cycle)
{
#if defined(SW_USE_OPENSSL)
	http_destroy_ssl(cycle);
#endif
	return 0;
}

sw_int_t httpsc_timeoutevt(sw_lsn_cycle_t *cycle)
{
	sw_int32_t	i = 0;
	sw_time_t	now_time = -1;
	sw_int32_t	timeout = 0;
	sw_cmd_t	cmd;
	sw_int_t	ret = 0;
	sw_char_t	buf[64];
	sw_loc_vars_t	vars;
	
	memset(buf, 0x0, sizeof(buf));
	if (cycle == NULL)
	{
		pub_log_info("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].sockid >= 0 && cycle->link_info[i].use == 1)
		{
			now_time = (sw_int_t)time(NULL);
			timeout = cycle->link_info[i].timeout;
			if (timeout == 0)
			{
				timeout = cycle->lsn_conf.timeout;
			}
			pub_log_debug("[%s][%d] timeout===[%d]", __FILE__, __LINE__, timeout);
			if (cycle->link_info[i].start_time != 0 && now_time - cycle->link_info[i].start_time > timeout)
			{
				pub_log_info("[%s][%d]Socket[%d][%d] TIMEOUT!", 
					__FILE__, __LINE__, i, cycle->link_info[i].sockid);
				if (cycle->link_info[i].cmd_type == SW_CALLLSNREQ)
				{
					pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%d]",
						__FILE__, __LINE__, cycle->link_info[i].mtype);
					alog_set_sysinfo(cycle->link_info[i].mtype, cycle->link_info[i].cmd.sys_date, 
										cycle->link_info[i].cmd.trace_no, cycle->link_info[i].cmd.lsn_name);
					ret = pub_loc_vars_alloc(&vars, SHM_VARS);
					if (ret == SW_ERROR)
					{
						pub_log_bend("[%s][%d] pub_loc_vars_alloc error!",
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
						pub_log_bend("[%s][%d] vars unserialize error! mtype=[%d]",
							__FILE__, __LINE__, cycle->link_info[i].mtype);
						continue;
					}
					pub_log_info("[%s][%d] vars unserialize success! mtype=[%d]",
						__FILE__, __LINE__, cycle->link_info[i].mtype);
					lsn_set_err(&vars, ERR_LSNOUT);
					memcpy(&cmd, &cycle->link_info[i].cmd, sizeof(sw_cmd_t));
					lsn_deal_err(cycle, &vars, &cmd);
					vars.free_mem(&vars);
				}
				select_del_event(cycle->lsn_fds, cycle->link_info[i].sockid);
				http_clear_ssl(cycle->link_info[i].sockid);
				http_close_fd(cycle, i);
				pub_log_bend("[%s][%d],close link socket[%d]", __FILE__, __LINE__, i);
				continue;
			}
		}
	}
	return SW_OK;
}

sw_int_t httpsc_call(cycle, locvar, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *locvar;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	sw_fd_t		sockid = 0;
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	
	if (cycle == NULL || locvar == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sockid = http_connect(cycle);
	if (sockid < 0)
	{
		lsn_set_err(locvar, ERR_CONNECT);
		pub_log_error("[%s][%d] connect [%s]:[%d] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.ip, 
			cycle->lsn_conf.comm.group[0].remote.port, errno, strerror(errno));
		return SW_ERROR;
	}
	cycle->link_info[sockid].sockid = sockid;

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
			return SW_ERROR;
		}
	}

	memcpy(&cycle->link_info[sockid].cmd, cmd, sizeof(sw_cmd_t));
	ret = http_send(cycle, locbuf->data, locbuf->len, sockid);
	if (ret != SW_OK)
	{
		lsn_set_err(locvar, ERR_SEND);
		pub_log_error("[%s][%d] tcpsc send msg error! SockId=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, sockid, errno, strerror(errno));
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
	cycle->link_info[sockid].trace_no = cmd->trace_no;
	cycle->link_info[sockid].cmd_type = cmd->type;
	cycle->link_info[sockid].start_time = (sw_int_t)time(NULL);
	cycle->link_info[sockid].use = 1;
	cycle->link_info[sockid].timeout = cmd->timeout;
	memcpy(&cycle->link_info[sockid].cmd, cmd, sizeof(sw_cmd_t));
	pub_log_info("[%s][%d] @@@@@@@@@@ ori_prdt=[%s]", __FILE__, __LINE__, cmd->ori_prdt);
	strncpy(cycle->link_info[sockid].prdt, cmd->ori_prdt, sizeof(cycle->link_info[sockid].prdt) - 1);
	strncpy(cycle->link_info[sockid].def_name, cmd->def_name, sizeof(cycle->link_info[sockid].def_name) - 1);

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = sockid;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)httpsc_extcevt;

	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t httpsc_deal_out_task(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	int	len = 0;
	int	ret = 0;
	char	need_pack[8];
	sw_chnl_t	*chnl = NULL;
	sw_buf_t	tmpbuf;
	
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

	memset(need_pack, 0x0, sizeof(need_pack));
	loc_get_zd_data(loc_vars, "#http_need_pack", need_pack);
	if (need_pack[0] != '0')
	{
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

		ret = pkg_out(loc_vars, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, cycle->route.name);
		if (ret <= 0)
		{
			lsn_set_err(loc_vars, ERR_TOPKG);
			pub_log_error("[%s][%d] pack message error! len=[%d]", 
				__FILE__, __LINE__, ret);
			return SW_ERROR;
		}
		locbuf->len = ret;
		locbuf->data[ret] = '\0';
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] packages len=[%d]", 
			__FILE__, __LINE__, locbuf->len);
	}
	else
	{
		len = loc_vars->get_field_len(loc_vars, "#http_content");	
		if (buf_checksize(locbuf, len) < 0)
		{
			lsn_set_err(loc_vars, ERR_TOPKG);
			pub_log_error("[%s][%d] Update buffer error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		loc_get_zd_data_space(loc_vars, "#http_content", locbuf->data);
		locbuf->len = len;
	}

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
	
	ret = http_add_head(cycle, loc_vars, locbuf, 0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] http_add_head error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_buf_init(&tmpbuf);
	ret = loc_vars->serialize(loc_vars, &tmpbuf);
	if (ret <= 0)
	{
		pub_buf_clear(&tmpbuf);
		pub_log_error("[%s][%d]shm_vars_serialize error!", __FILE__, __LINE__);
		return -1;
	}
	pub_buf_clear(&tmpbuf);
	pub_log_info("[%s][%d] httpsc_deal_task success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t httpsc_interevt(sw_fd_list_t *fd_lists)
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
			ret = httpsc_deal_out_task(cycle, &locvar, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]lsn_pub_platform_deal error",__FILE__,__LINE__);
				break;
			}
			pub_log_info("[%s][%d] deal out task success!", __FILE__, __LINE__);
			ret = httpsc_call(cycle, &locvar, &cmd, &locbuf);
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

sw_int_t httpsc_extcevt(sw_fd_list_t *fd_lists)
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
		return SW_ERROR;
	}

	memset(buf, 0x0, sizeof(buf));
	memset(&cmd, 0x0, sizeof(cmd));

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	cycle->accept_fd = fd_lists->fd;
	sockid = cycle->accept_fd;

	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		
		pub_log_error("%s, %d, pub_loc_vars_construct error."
				, __FILE__, __LINE__);
		http_clear_ssl(sockid);
		http_close_fd(cycle, sockid);
		return SW_ERROR;
	} 

	ret = locvar.create(&locvar, cycle->link_info[sockid].mtype);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d]recover var pool error", __FILE__,__LINE__);
		http_clear_ssl(sockid);
		http_close_fd(cycle, sockid);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	
	cmd.mtype = cycle->link_info[sockid].mtype;
	cmd.trace_no = cycle->link_info[sockid].trace_no;
	cmd.type = cycle->link_info[sockid].cmd_type;
	strcpy(cmd.sys_date, cycle->link_info[sockid].cmd.sys_date);
	strncpy(cmd.dst_prdt, cycle->link_info[sockid].prdt, sizeof(cmd.dst_prdt) - 1);
	strncpy(cmd.def_name, cycle->link_info[sockid].def_name, sizeof(cmd.def_name) - 1);
	pub_log_info("[%s][%d] dst_prdt=[%s] def_name=[%s]", 
		__FILE__, __LINE__, cmd.dst_prdt, cmd.def_name);
	pub_buf_init(&locbuf);
	ret = http_recv(cycle, &locvar, &locbuf, sockid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] http_recv error!", __FILE__, __LINE__);
		lsn_deal_err(cycle, &locvar, &cmd);
		select_del_event(cycle->lsn_fds, sockid);
		http_clear_ssl(sockid);
		http_close_fd(cycle, sockid);
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
		http_clear_ssl(sockid);
		http_close_fd(cycle, sockid);
		locvar.free_mem(&locvar);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	trace_insert(&locvar, &cmd, TRACE_OUT);
	cycle->link_info[sockid].use = 2;
	select_del_event(cycle->lsn_fds, sockid);
	http_clear_ssl(sockid);
	http_close_fd(cycle, sockid);
	locvar.free_mem(&locvar);
	pub_buf_clear(&locbuf);

	return SW_OK;
}

