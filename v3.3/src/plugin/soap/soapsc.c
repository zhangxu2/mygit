#include "lsn_pub.h"
#include "soap_pub.h"

sw_int_t soapsc_extcevt(sw_fd_list_t *fd_lists);

sw_int_t soapsc_init(sw_lsn_cycle_t *cycle)
{
	char	logdir[128];

	g_soap = &ssoap;
	soap_init(g_soap);
	soap_set_mode(g_soap, SOAP_C_UTFSTRING);
	soap_set_mode(g_soap, SOAP_ENC_MTOM);
	soap_set_mode(g_soap, SOAP_XML_INDENT);
	set_gcycle(cycle);
	soap_set_mime_callback(g_soap);
#ifdef SOAP_DEBUG
	memset(logdir, 0x0, sizeof(logdir));
	sprintf(logdir, "%s/log/syslog/swlsn_%s_soaprecv.log", getenv("SWWORK"), cycle->lsn_conf.name);
	soap_set_recv_logfile(g_soap, logdir);
	memset(logdir, 0x0, sizeof(logdir));
	sprintf(logdir, "%s/log/syslog/swlsn_%s_soapsend.log", getenv("SWWORK"), cycle->lsn_conf.name);
	soap_set_sent_logfile(g_soap, logdir);
	memset(logdir, 0x0, sizeof(logdir));
	sprintf(logdir, "%s/log/syslog/swlsn_%s_soapdebug.log", getenv("SWWORK"), cycle->lsn_conf.name);
	soap_set_test_logfile(g_soap, logdir);
#endif

	soaps = (struct soap_t *)calloc(1, sizeof(struct soap_t)*cycle->lsn_conf.conmax);
	if (soaps == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, sizeof(struct soap)*cycle->lsn_conf.conmax, errno, strerror(errno));
		return SW_ERROR;
	}

	if (soap_set_xmlns(cycle, g_soap))
	{
		pub_log_error("[%s][%d] set xmlns error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (g_soap->namespaces && !g_soap->local_namespaces)
	{
		register struct Namespace *ns1;
		for (ns1 = g_soap->namespaces; ns1->id; ns1++)
		{
			pub_log_info("[%s][%d] namespaces:id=[%s] ns=[%s]",
				__FILE__, __LINE__, ns1->id, ns1->ns);
		}
	}
	pub_log_info("[%s][%d] soapsc_init success!", __FILE__, __LINE__);

	return 0;
}

sw_int_t soapsc_destroy(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	
	soap_destroy(g_soap);
	soap_end(g_soap);
	soap_done(g_soap);
	
	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (soaps[i].soap)
		{
			soap_pub_destroy(i);
		}
	}
	free(soaps);
	pub_log_info("[%s][%d] soapsc_destroy!", __FILE__, __LINE__);

	return 0;
}

sw_int_t soapsc_timeoutevt(sw_lsn_cycle_t *cycle)
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

			if (cycle->link_info[i].start_time != 0 && now_time - cycle->link_info[i].start_time > timeout)
			{
				pub_log_info("[%s][%d]Socket[%d][%d] TIMEOUT, timeout=[%d]!", 
					__FILE__, __LINE__, i, cycle->link_info[i].sockid, timeout);
				alog_set_sysinfo(cycle->link_info[i].mtype, cycle->link_info[i].cmd.sys_date, 
									cycle->link_info[i].cmd.trace_no, cycle->link_info[i].cmd.lsn_name);
				
				if (cycle->link_info[i].cmd_type == SW_CALLLSNREQ)
				{
					pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%ld] traceno=[%lld]",
						__FILE__, __LINE__, cycle->link_info[i].mtype, cycle->link_info[i].trace_no);
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
				lsn_pub_close_fd(cycle, i);
				soap_pub_destroy(i);
				pub_log_bend("[%s][%d],close link socket[%d]", __FILE__, __LINE__, i);
				continue;
			}
		}
	}
	return SW_OK;
}

sw_int_t soapsc_deal_in_task(cycle, loc_vars, cmd, locbuf)
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

	loc_set_zd_data(loc_vars, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(loc_vars, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
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

sw_int_t soapsc_call(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	char 	tmp[256];
	char	buf[128];
	char	fflag[8];
	char	service[128];
	sw_fd_t	sockid = 0;
	sw_fd_list_t    fd_list;
	sw_int_t	ret = 0;
	sw_chnl_t	*chnl = NULL;
	sw_xmltree_t	*xml = NULL;
	struct soap	*soap;
	
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
	
	memset(buf, 0x0, sizeof(buf));
	if (cycle->lsn_conf.comm.group[0].remote.ip[0] == '#' 
	    || cycle->lsn_conf.comm.group[0].remote.ip[0] == '$')
	{
		memset(tmp, 0x0, sizeof(tmp));
		loc_get_zd_data(loc_vars, cycle->lsn_conf.comm.group[0].remote.ip, tmp);
		if (strncmp(tmp, "http:", 5)  == 0 ||
		    strncmp(tmp, "https:", 6) == 0)
		{
			sprintf(buf, "%s", tmp);
		}
		else
		{
			sprintf(buf, "http://%s", tmp);		
		}
	}
	else if (strncmp(cycle->lsn_conf.comm.group[0].remote.ip, "http:", 5) == 0 ||
		strncmp(cycle->lsn_conf.comm.group[0].remote.ip, "https:", 6) == 0)
	{
		sprintf(buf, "%s", cycle->lsn_conf.comm.group[0].remote.ip);
	}
	else
	{
		sprintf(buf, "http://%s", cycle->lsn_conf.comm.group[0].remote.ip);
	}

	if (cycle->lsn_conf.comm.group[0].remote.service[0] != '\0')
	{
		memset(service, 0x0, sizeof(service));
		if (cycle->lsn_conf.comm.group[0].remote.service[0] == '#' 
			|| cycle->lsn_conf.comm.group[0].remote.service[0] == '$')
		{
			loc_get_zd_data(loc_vars, cycle->lsn_conf.comm.group[0].remote.service, service);
		}
		else
		{
			strcpy(service, cycle->lsn_conf.comm.group[0].remote.service);
		}
		strcat(buf, "/");
		strcat(buf, service);
		pub_log_info("[%s][%d] service=[%s][%s]", __FILE__, __LINE__, 
			cycle->lsn_conf.comm.group[0].remote.service, service);
	}
	loc_set_zd_data(loc_vars, "$soapendpoint", buf);
	pub_log_info("[%s][%d] endpoint=[%s]", __FILE__, __LINE__, buf);
	
	xml = chnl->cache.pkgdeal;
	soap = soap_copy(g_soap);
	if (soap == NULL)
	{
		pub_log_error("[%s][%d] soap_copy error! soap->error=[%d]",
			__FILE__, __LINE__, g_soap->error);
		return SW_ERROR;
	}
	
	memset(fflag, 0x0, sizeof(fflag));
	loc_get_zd_data(loc_vars, "$fileflag", fflag);
	if (fflag[0] == '1')
	{
		soap_set_mime(soap, NULL, NULL);
	}

	if (chnl->fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] Send file begin...", __FILE__, __LINE__);
		ret = chnl->fun.file_func(loc_vars, O_SEND, -1, soap);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Send file error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] Send file success!", __FILE__, __LINE__);
	}
	
	ret = soap_client_request(soap, loc_vars, chnl->cache.pkgdeal);
	if (ret)
	{
		pub_log_error("[%s][%d] soap_client_request error! soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		soap_destroy(soap);
		soap_end(soap);
		soap_done(soap);
		free(soap);
		soap = NULL;
		return SW_ERROR;
	}

	if (cmd->task_flg == SW_DEL)
	{
		pub_log_info("[%s][%d] TASK DELETE!", __FILE__, __LINE__);
		mtype_delete(cmd->mtype, 0);
		soap_destroy(soap);
		soap_end(soap);
		soap_done(soap);
		free(soap);
		return SW_OK;
	}

	sockid = soap->socket;
	soaps[sockid].soap = soap;
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(loc_vars, SOAPSERVER, buf);
	strcpy(soaps[sockid].soapserver, buf);
	pub_log_info("[%s][%d] soap_client_request success!", __FILE__, __LINE__);
	pub_log_info("[%s][%d] sockid=[%d] soapserver=[%s]", __FILE__, __LINE__, sockid, buf);

	cycle->link_info[sockid].mtype = cmd->mtype;
	cycle->link_info[sockid].sockid = sockid;
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
	fd_list.event_handler = (sw_event_handler_pt)soapsc_extcevt;

	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		soap_pub_destroy(sockid);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] soapsc_call success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t soapsc_interevt(sw_fd_list_t *fd_lists)
{
	int	len = 0;
	long	mtype = 0;
	sw_fd_t		out_fd = 0;
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
	ret = msg_trans_rcv(out_fd, (char*)&cmd, &mtype, &len);
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
			ret = soapsc_call(cycle, &locvar, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d] soapsc_call error",__FILE__,__LINE__);
				break;
			}
			memset(locbuf.data, 0x0, locbuf.size);
			ret = locvar.serialize(&locvar, &locbuf);
			if (ret <= 0)
			{
				err_flag = 1;
				pub_log_error("[%s][%d] shm serialize error",__FILE__,__LINE__);
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

sw_int_t soapsc_extcevt(sw_fd_list_t *fd_lists)
{
	sw_fd_t	sockid = 0;
	sw_int_t	ret = 0;
	sw_cmd_t	cmd;
	sw_buf_t	locbuf;
	sw_char_t	buf[128];
	sw_chnl_t	*chnl;
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = NULL;
	struct soap	*soap = NULL;

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
	chnl = &cycle->chnl;

	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		
		pub_log_error("%s, %d, pub_loc_vars_construct error."
				, __FILE__, __LINE__);
		select_del_event(cycle->lsn_fds, sockid);
		soap_pub_destroy(sockid);
		lsn_pub_close_fd(cycle, sockid);
		return SW_ERROR;
	} 

	ret = locvar.create(&locvar, cycle->link_info[sockid].mtype);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d]recover var pool error", __FILE__,__LINE__);
		locvar.free_mem(&locvar);
		select_del_event(cycle->lsn_fds, sockid);
		soap_pub_destroy(sockid);
		lsn_pub_close_fd(cycle, sockid);
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

	alog_set_sysinfo(cmd.mtype, cmd.sys_date, cmd.trace_no, NULL);

	memset(buf, 0x0, sizeof(buf));
	strcpy(buf, soaps[sockid].soapserver);
	soap = soaps[sockid].soap;
	pub_log_info("[%s][%d] sockid=[%d] soapserver=[%s]", __FILE__, __LINE__, sockid, buf);
	soap_set_gvars(&locvar);
	if (chnl->fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] Recv file begin...", __FILE__, __LINE__);
		ret = chnl->fun.file_func(&locvar, O_RECV, -1, soap);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Recv file error!", __FILE__, __LINE__);
			select_del_event(cycle->lsn_fds, sockid);
			soap_pub_destroy(sockid);
			lsn_pub_close_fd(cycle, sockid);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] Recv file success!", __FILE__, __LINE__);
	}

	ret = soap_client_response(soap, &locvar, chnl->cache.pkgdeal);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] soap_client_response error! ret=[%d] error=[%d] status=[%d]",
			__FILE__, __LINE__, ret, soap->error, soap->status);
		char	errcode[8];
		memset(errcode, 0x0, sizeof(errcode));
		lsn_get_err(&locvar, errcode);
		if (errcode[0] == '\0' || strcmp(errcode, "0000") == 0)
		{
			lsn_set_err(&locvar, ERREVERY);
		}
	}
	soap_pub_destroy(sockid);
	
	pub_buf_init(&locbuf);
	ret = soapsc_deal_in_task(cycle, &locvar, &cmd, &locbuf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, sockid);
		locvar.free_mem(&locvar);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	trace_insert(&locvar, &cmd, TRACE_OUT);
	cycle->link_info[sockid].use = 2;
	select_del_event(cycle->lsn_fds, sockid);
	lsn_pub_not_close_fd(cycle, sockid);
	locvar.free_mem(&locvar);
	pub_buf_clear(&locbuf);

	return SW_OK;
}

