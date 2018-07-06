#include "lsn_pub.h"
#include "pub_type.h"
#include "soap_pub.h"
#include "httpget.h"

static int copy_file(struct soap *soap, const char *pname, const char *type)
{
	FILE	*fp = NULL;
	size_t	r = 0;
	char	*ptr = NULL;
	char	dir[128];
	char	name[128];
	char	filename[128];
	
	memset(dir, 0x0, sizeof(dir));
	memset(name, 0x0, sizeof(name));
	memset(filename, 0x0, sizeof(filename));
	
	sprintf(dir, "%s/cfg/common/wsdl/", getenv("SWWORK"));	
	ptr = strchr(pname, '/');
	if (ptr != NULL)
	{
		memset(name, 0x0, sizeof(name));
		strncpy(name, pname, ptr - pname);
		strcat(dir, name);
		if (*(ptr + 1) == '?')
		{
			strcpy(name, ptr + 2);
		}
		else
		{
			strcpy(name, ptr + 1);
		}
	}
	else
	{
		if (pname[0] == '?')
		{
			strcpy(name, pname + 1);
		}
		else
		{
			strcpy(name, pname);
		}
	}
	sprintf(filename, "%s/%s", dir, name);	
	pub_log_info("[%s][%d] filename=[%s]", __FILE__, __LINE__, filename);
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return 404;
	}
	soap->http_content = type;
	if (soap_response(soap, SOAP_FILE))
	{
		soap_end_send(soap);
		fclose(fp);
		return soap->error;
	}
	
	while (1)
	{
		r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fp);
		if (r == 0)
		{
			break;
		}
		if (soap_send_raw(soap, soap->tmpbuf, r))
		{
			soap_end_send(soap);
			fclose(fp);
			return soap->error;
		}
	}
	fclose(fp);
	return soap_end_send(soap);
}

int http_get_handler(struct soap *soap)
{
	pub_log_info("[%s][%d] soap->path=[%s]", __FILE__, __LINE__, soap->path);
	if (!soap_tag_cmp(soap->path, "*.html"))
	{
		return copy_file(soap, soap->path + 1, "text/html");
	}
	if (!soap_tag_cmp(soap->path, "*.xml")                                                                                           
		|| !soap_tag_cmp(soap->path, "*.xsd")                                                                                           
		|| !soap_tag_cmp(soap->path, "*.wsdl"))                                                                                         
	{
		return copy_file(soap, soap->path + 1, "text/xml");                                                                            
	}
	if (!soap_tag_cmp(soap->path, "*.jpg"))                                                                                          
	{
		return copy_file(soap, soap->path + 1, "image/jpeg");                                                                          
	}
	if (!soap_tag_cmp(soap->path, "*.gif"))                                                                                          
	{
		return copy_file(soap, soap->path + 1, "image/gif");                                                                           
	}
	if (!soap_tag_cmp(soap->path, "*.png"))                                                                                          
	{
		return copy_file(soap, soap->path + 1, "image/png");                                                                           
	}
	if (!soap_tag_cmp(soap->path, "*.ico"))                                                                                          
	{
		return copy_file(soap, soap->path + 1, "image/ico"); 
	}

	return 404;
}

int soapss_setsocklinger(int sockid)
{
	struct linger	timeout;

	memset(&timeout, 0x0, sizeof(timeout));
	timeout.l_onoff = 1;
	timeout.l_linger = 0;
	setsockopt(sockid, SOL_SOCKET, SO_LINGER, (char *)&timeout, sizeof(timeout));

	return 0;
}

sw_int_t soapss_accept(sw_fd_list_t * fd_lists);

sw_int_t soapss_extcevt(sw_fd_list_t * fd_lists)
{
	return SW_OK;
}

sw_int_t soapss_timeoutevt(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	long	now = 0;
	char	buf[128];
	sw_int32_t	timeout = 0;
	sw_cmd_t	cmd;
	sw_int_t	ret = 0;
	sw_loc_vars_t	vars;
	
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
	now = (long)time(NULL);
	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].sockid >= 0 && cycle->link_info[i].use > 0)
		{
			if (now - cycle->link_info[i].start_time > timeout)
			{
				pub_log_info("[%s][%d] TIMEOUT! DELETE mtype[%d]", 
					__FILE__, __LINE__, cycle->link_info[i].mtype);
				if (cycle->link_info[i].use == 2)
				{
					memset(&cmd, 0x0, sizeof(cmd));
					cmd.mtype = cycle->link_info[i].mtype;
					cmd.trace_no = cycle->link_info[i].trace_no;
					strncpy(cmd.sys_date, cycle->link_info[i].cmd.sys_date, sizeof(cycle->link_info[i].cmd.sys_date)-1);

					memset(&vars, 0x0, sizeof(vars));
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

					loc_set_zd_data(&vars, "$errcode", "999a");
					loc_set_zd_data(&vars, "#errcode", "999a");
					loc_set_zd_data(&vars, "#errmsg", "系统内部异常");
					
					trace_insert(&vars, &cmd, TRACE_TIMEOUT);
					vars.destroy((void *)&vars);
					vars.free_mem(&vars);
					
					mtype_delete(cycle->link_info[i].mtype, 1);
				}
				
				if (cycle->link_info[i].use == 1)
				{
					/*** Only accepted, set linger timeout is 0, shutdown the socket forced ***/
					soapss_setsocklinger(cycle->link_info[i].sockid);
				}
				select_del_event(cycle->lsn_fds, cycle->link_info[i].sockid);
				lsn_pub_close_fd(cycle, i);
				soap_pub_destroy(i);
			}
		}
	}

	return SW_OK;
}

sw_int_t soapss_init(sw_lsn_cycle_t *cycle)
{
	char	logdir[128];
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	SOAP_SOCKET	m = 0;
	
	g_soap = &ssoap;
	soap_init(g_soap);
	soap_set_mode(g_soap, SOAP_C_UTFSTRING);
	soap_set_mode(g_soap, SOAP_ENC_MTOM);
	soap_set_mode(g_soap, SOAP_XML_INDENT);
	soap_set_mime_callback(g_soap);
	set_gcycle(cycle);

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
	if (soap_register_plugin_arg(g_soap, http_get, (void*)http_get_handler))
	{
		pub_log_error("[%s][%d] soap_register_plugin_arg httpget error! soap->error=[%d]",
			__FILE__, __LINE__, g_soap->error);
		soap_print_fault(g_soap, stderr);
		return SW_ERROR;
	}
	g_soap->accept_timeout = cycle->lsn_conf.activetime ? cycle->lsn_conf.activetime : 5;
	
	m = soap_bind(g_soap, cycle->lsn_conf.comm.group[0].local.ip, cycle->lsn_conf.comm.group[0].local.port, 100);
	if (!soap_valid_socket(m))
	{
		pub_log_error("[%s][%d] %s soap bind [%s][%d] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.name, cycle->lsn_conf.comm.group[0].local.ip,
			cycle->lsn_conf.comm.group[0].local.port, errno, strerror(errno));
		soap_print_fault(g_soap, stderr);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] bind [%s][%d] success! FD=[%d]",
		__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].local.ip,
		cycle->lsn_conf.comm.group[0].local.port, cycle->lsn_fd);
	cycle->lsn_fd = m;
	
	soaps = (struct soap_t *)calloc(1, sizeof(struct soap_t)*cycle->lsn_conf.conmax);
	if (soaps == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, sizeof(struct soap)*cycle->lsn_conf.conmax, errno, strerror(errno));
		close(cycle->lsn_fd);
		return SW_ERROR;
	}
	
	ret = lsn_set_fd_noblock(cycle->lsn_fd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, cycle->lsn_fd);
		close(cycle->lsn_fd);
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
	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->lsn_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)soapss_accept;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t soapss_destroy(sw_lsn_cycle_t *cycle)
{
	int	i = 0;

	if (cycle->lsn_fd > 0)
	{
		close(cycle->lsn_fd);
		pub_log_info("[%s][%d] Destroy, close fd:[%d]",
			__FILE__, __LINE__, cycle->lsn_fd);
	}
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
	pub_log_info("[%s][%d] soapss_destroy!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t soapss_res_work(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_cmd_t *cmd)
{
	sw_fd_t	sockid = 0;
	char	fflag[8];
	sw_int_t	ret = 0;
	sw_int_t	index = 0;
	sw_route_t	route;
	sw_chnl_t	*chnl = NULL;
	struct soap	*soap;
	
	chnl = &cycle->chnl;
	memset(&route, 0x0, sizeof(route));
	
	pub_log_debug("[%s][%d] Deal response begin...", __FILE__, __LINE__);
	
	index = lsn_find_index_by_mtype(cycle, cmd->mtype);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find index error! mtype=[%d]",
			__FILE__, __LINE__, cmd->mtype);
		return SW_ERROR;
	}
	sockid = cycle->link_info[index].sockid;
	soap = soaps[index].soap;
	loc_set_zd_data(locvar, SOAPSERVER, cycle->link_info[index].data_info);

	ret = lsn_get_route_by_name(cycle, cmd->ori_prdt, &route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get route info error! prdt=[%s]", 
			__FILE__, __LINE__, cmd->ori_prdt);
		soap_send_fault(soap);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, index);
		soap_pub_destroy(index);
		return SW_ERROR;
	}
	
	if (cycle->route.fun.prean_func != NULL)
	{
		ret = cycle->route.fun.prean_func(locvar, cmd->ori_prdt, O_SEND);
		if (ret < 0)
		{
			pub_log_info( "[%s][%d] pre analyze error! ret=[%d]", __FILE__, __LINE__, ret);
			soap_send_fault(soap);
			select_del_event(cycle->lsn_fds, sockid);
			lsn_pub_close_fd(cycle, index);
			soap_pub_destroy(index);
			return SW_ERROR;
		}
		pub_log_info( "[%s][%d] preanalyze success!", __FILE__, __LINE__);
	}
	
	loc_set_zd_data(locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		pub_log_debug("[%s][%d] Map begin...", __FILE__, __LINE__);
		ret = chnl->fun.pkgmap_func(locvar, chnl->cache.pkgmap, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error! ret=[%d]", __FILE__, __LINE__, ret);
			soap_send_fault(soap);
			select_del_event(cycle->lsn_fds, sockid);
			lsn_pub_close_fd(cycle, index);
			soap_pub_destroy(index);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] Map success!", __FILE__, __LINE__);
	}
	
	pub_log_debug("[%s][%d] Pack begin...", __FILE__, __LINE__);
	memset(fflag, 0x0, sizeof(fflag));
	loc_get_zd_data(locvar, "$fileflag", fflag);
	if (fflag[0] == '1')
	{
		soap_set_mime(soap, NULL, NULL);
	}
	soap_set_gvars(locvar);
	if (soap_server_response(soap, locvar, chnl->cache.pkgdeal))
	{
		pub_log_error("[%s][%d] soap_server_response error!", __FILE__, __LINE__);
        pub_log_info("[%s][%d] Deal response %s!", __FILE__, __LINE__, 
            soap->error == SOAP_OK ? "success" : "error");
        select_del_event(cycle->lsn_fds, sockid);
        lsn_pub_close_fd(cycle, index);
    }   
    else
    {   
        pub_log_info("[%s][%d] Deal response %s!", __FILE__, __LINE__, 
            soap->error == SOAP_OK ? "success" : "error");
        select_del_event(cycle->lsn_fds, sockid);
        lsn_pub_not_close_fd(cycle, index);                                                                                           
    }
	soap_pub_destroy(index);

	return SW_OK;
}

sw_int_t soapss_deal_linknull(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd)
{
	sw_fd_t	sockid = 0;
	sw_int_t	index = 0;
	
	index = lsn_find_index_by_mtype(cycle, cmd->mtype);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find index error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	sockid = lsn_get_fd_by_index(cycle, index);
	select_del_event(cycle->lsn_fds, sockid);
	lsn_pub_close_fd(cycle, index);
	mtype_delete(cmd->mtype, 0);

	return SW_OK;
}

sw_int_t soapss_interevt(sw_fd_list_t *fd_lists)
{
	long		mtype = 0;
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_int_t	index = 0;
	sw_int_t 	err_flag = 0;
	sw_buf_t	locbuf;
	sw_cmd_t	cmd;
	sw_char_t	buf[128];
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] Deal svc response begin...", __FILE__, __LINE__);

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	memset(buf, 0x0, sizeof(buf));
	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_loc_vars_alloc error!", __FILE__, __LINE__);
		return SW_CONTINUE;
	}

	pub_buf_init(&locbuf);
	memset(&cmd, 0x00, sizeof(cmd));
	ret = msg_trans_rcv(cycle->lsn_conf.out_fd, (char *)&cmd, &mtype, &len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv cmd info error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		return SW_CONTINUE;
	}
	pub_log_info("[%s][%d] Recv cmd type===[%d]", __FILE__, __LINE__, cmd.type);

	sprintf(buf, "SHM%08ld", cmd.mtype);
	ret = locvar.unserialize((void *)&locvar, buf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] unserialize error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		mtype_delete(cmd.mtype, 1);
		return SW_CONTINUE;
	}

	cmd_print(&cmd);
	trace_insert(&locvar, &cmd, TRACE_IN);
	switch (cmd.type)
	{
		case SW_LINKNULL:
			pub_log_info("[%s][%d] LINKNULL...", __FILE__, __LINE__);
			ret = soapss_deal_linknull(cycle, &cmd);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d] deal linknull error!", __FILE__, __LINE__);
			}
			break;
		case SW_TASKRES:
			ret = soapss_res_work(cycle, &locvar, &cmd);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d] Deal response error!", __FILE__, __LINE__);
			}
			break;
		case SW_TASKERR:
			err_flag = 1;
			index = lsn_find_index_by_mtype(cycle, cmd.mtype);
			if (index < 0)
			{
				pub_log_error("[%s][%d] find index error! mtype=[%d]",
					__FILE__, __LINE__, cmd.mtype);
				break;
			}
			lsn_pub_close_fd(cycle, index);
			break;

		default:
			err_flag = 1;
			pub_log_error("[%s][%d] Recv error type=[%d]", __FILE__, __LINE__, cmd.type);
			mtype_delete(cmd.mtype, 1);
			index = lsn_find_index_by_mtype(cycle, cmd.mtype);
			if (index < 0)
			{
				pub_log_error("[%s][%d] find index error! mtype=[%d]",
					__FILE__, __LINE__, cmd.mtype);
				break;
			}
			lsn_pub_close_fd(cycle, index);
			break;
	}
	trace_insert(&locvar, &cmd, TRACE_OVER);
	pub_buf_clear(&locbuf);
	locvar.destroy((void *)&locvar);
	locvar.free_mem(&locvar);
	pub_log_debug("[%s][%d] Deal svc response success!", __FILE__, __LINE__);
	mtype_delete(cmd.mtype, err_flag);
	
	return SW_OK;
}

sw_int_t soapss_deal_req(sw_lsn_cycle_t *cycle, int index)
{
	sw_int_t	ret = 0;
	sw_int_t	mtype = 0;
	sw_int_t	times = 0;
	sw_char_t	*p = NULL;
	sw_char_t 	ip[32];
	sw_char_t	tmp[128];
	sw_cmd_t	cmd;
	sw_loc_vars_t	locvar;
	sw_char_t	buf[100];
	sw_buf_t	locbuf;
	sw_route_t	route;
	sw_chnl_t	*chnl = NULL;
	struct soap	*soap;
	
	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] Deal request begin...", __FILE__, __LINE__);	
	chnl = &cycle->chnl;
	soap = soaps[index].soap;

	memset(ip, 0x00, sizeof(ip));
	strcpy(ip, cycle->lsn_conf.comm.group[0].remote.ip);
	if (ip[0] != '\0')
	{
		p = strstr(ip, "|");
		if (p != NULL)
		{
			memset(tmp, 0x00, sizeof(tmp));
			memcpy(tmp, ip, p - ip);
			times = atoi(tmp);	
			memset(tmp, 0x00, sizeof(tmp));
			strcpy(tmp, p + 1);
			memset(ip, 0x00, sizeof(ip));
			strcpy(ip, tmp);
		}
		else
		{
			times = 0;
		}
		memset(tmp, 0x00, sizeof(tmp));
		if (strncmp(ip, "http:", 5) == 0 ||  strncmp(ip, "https:", 6) == 0)
		{
			sprintf(tmp, "%s", ip);
		}
		else
		{
			sprintf(tmp, "http://%s", ip);
		}

		ret = soap_pass_through(soap, tmp, times);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] soap pass through error, soap->error=[%d]", __FILE__, __LINE__, soap->error);
			return SW_ERROR;
		}
		else
		{
			pub_log_info("[%s][%d] soap pass through OK", __FILE__, __LINE__);
			return SW_DONE;
		}
	}
	
	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{		
		pub_log_error("%s, %d, pub_loc_vars_alloc error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	mtype = mtype_new();
	if (mtype < 0)
	{
		pub_log_error("[%s][%d] create mtype error! ret=[%d]", __FILE__, __LINE__, mtype);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	
	ret = locvar.create(&locvar, mtype);
	if( ret < 0)
	{
		pub_log_error("[%s][%d] create locvar error ", __FILE__, __LINE__);
		mtype_delete(mtype, 1);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d] soap_begin_serve...", __FILE__, __LINE__);	
	soap_set_gvars(&locvar);
	if (soap_server_begin(soap, &locvar, chnl->cache.pkgdeal))
	{
		pub_log_info("[%s][%d] soap->error=[%d]", __FILE__, __LINE__, soap->error);
		if (soap->error == 404 || soap->error == SOAP_STOP)
		{
			pub_log_info("[%s][%d] SOAP HTTP_GET!", __FILE__, __LINE__);
			mtype_delete(mtype, 1);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			return SW_DONE;
		}
		pub_log_error("[%s][%d] soap_begin_serve error! soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		mtype_delete(mtype, 1);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] soap_begin_serve success!", __FILE__, __LINE__);	
	
	if (soap_serve_request(soap, &locvar, chnl->cache.pkgdeal) != SOAP_OK)
	{
		pub_log_error("[%s][%d] soap_serve_request error! soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		mtype_delete(mtype, 1);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}

	loc_set_zd_data(&locvar, "$REQADDR", soap->host);
	loc_set_zd_int(&locvar, "$PKGLEN", soap->buflen);
	pub_log_info("[%s][%d]remote req info,addr[%s]buflen=[%d]", __FILE__, __LINE__, soap->host, soap->buflen);
	loc_set_zd_data(&locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		pub_log_debug("[%s][%d] Map begin...", __FILE__, __LINE__);
		ret = chnl->fun.pkgmap_func(&locvar, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			mtype_delete(mtype, 1);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] Map success!", __FILE__, __LINE__);
	}
	
	pub_log_debug("[%s][%d] Get route begin...", __FILE__, __LINE__);
	memset(&locbuf, 0x0, sizeof(locbuf));
	pub_buf_init(&locbuf);
	memset(&route, 0x0, sizeof(route));
	ret = lsn_get_route(cycle, &locvar, &locbuf, &route, 1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get route info error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		mtype_delete(mtype, 1);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	pub_buf_clear(&locbuf);
	pub_log_debug("[%s][%d] Get route end! prdt====[%s] gate=[%d]", __FILE__, __LINE__, route.name, route.gate);
	
	if (route.fun.prean_func != NULL)
	{
		pub_log_info("[%s][%d] AAAAAAAAAAA", __FILE__, __LINE__);
		ret = route.fun.prean_func(&locvar, route.name, O_RECV);
		if (ret < 0)
		{
			pub_log_info( "[%s][%d] pre analyze error! ret=[%d]", __FILE__, __LINE__, ret);
			mtype_delete(mtype, 1);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] preanlyze success!", __FILE__, __LINE__);
	}
	
	memset(&cmd, 0x0, sizeof(cmd));
	strcpy(cmd.dst_prdt, route.name);
	cmd.mtype = mtype;

	sw_int64_t	trace = 0;
	ret = seqs_get_traceno_and_sysdate(&trace, cmd.sys_date);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]get traceno and system date error", __FILE__, __LINE__);
		mtype_delete(mtype, 1);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	cmd.trace_no = trace;
	cmd.type = SW_TASKREQ;
	cmd.ori_type = ND_LSN;
	sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	strcpy(cmd.ori_svc, buf);

	if (chnl->fun.svrmap_func != NULL)
	{
		pub_log_info("[%s][%d] Svrmap begin...", __FILE__, __LINE__);
		ret = chnl->fun.svrmap_func(route.cache.svrmap, &locvar, cmd.dst_svr, cmd.def_name, &cmd.level);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] svrsvc map error! ret=[%d]", __FILE__, __LINE__, ret);
			mtype_delete(mtype, 1);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			return SW_ERROR;
		}
                pub_log_info("[%s][%d] Svrmap sucess!", __FILE__, __LINE__);
	}

	ret = trace_create_info(&cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Crate trace info error!", __FILE__, __LINE__);
		mtype_delete(mtype, 1);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}

	locvar.set_string(&locvar, "$service", cmd.def_name);
	locvar.set_string(&locvar, "$server", cmd.dst_svr);
	locvar.set_string(&locvar, "$listen", cycle->lsn_conf.name);
	locvar.set_string(&locvar, "$product", route.name);
	

	cycle->link_info[index].start_time = (long)time(NULL);
	cycle->link_info[index].trace_no   = cmd.trace_no;
	cycle->link_info[index].mtype      = cmd.mtype;
	cycle->link_info[index].use        = 2;
	
	memcpy(&cycle->link_info[index].cmd, &cmd, sizeof(cmd));

	memset(cycle->link_info[index].data_info, 0x0, sizeof(cycle->link_info[index].data_info));
	loc_get_zd_data(&locvar, SOAPSERVER, cycle->link_info[index].data_info);
	
	cmd.dst_type = ND_SVC;
	cmd.msg_type = SW_MSG_REQ;
	cmd.task_flg = SW_STORE;
	cmd.timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
	strcpy(cmd.ori_svr, cycle->base.name.data);
	strcpy(cmd.lsn_name, cycle->lsn_conf.name);
	mtype_set_info(cmd.mtype, &cmd);
	trace_insert(&locvar, &cmd, TRACE_OUT);
	ret = route_snd_dst(&locvar, cycle->base.global_path, &cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send cmd to svc error! ret=[%d]", __FILE__, __LINE__, ret);
		mtype_delete(mtype, 1);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	locvar.free_mem(&locvar);
	pub_log_debug("[%s][%d] Deal request success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t soapss_req_work(sw_fd_list_t *fd_lists)
{
	int	index = 0;
	sw_fd_t	sockid;
	sw_int_t	ret = 0;
	sw_lsn_cycle_t	*cycle = NULL;
	struct soap	*soap;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error, fd_lists is null!", __FILE__, __LINE__);
		soap_send_fault(g_soap);
		return SW_ERROR;
	}
	
	sockid = fd_lists->fd;
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	
	index = lsn_find_index_by_fd(cycle, sockid);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find fd[%d] index error!", __FILE__, __LINE__, sockid);
		select_del_event(cycle->lsn_fds, sockid);
		close(sockid);
		return SW_DELETE;
	}
	soap = soaps[index].soap;
	pub_log_info("[%s][%d] soap sockid=[%d]", __FILE__, __LINE__, sockid);
	ret = soapss_deal_req(cycle, index);
	if (ret == SW_DONE)
	{
		pub_log_info("[%s][%d] HTTP GET!", __FILE__, __LINE__);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, index);
		soap_pub_destroy(index);
		return SW_OK;
	}
	else if (ret < 0)
	{
		pub_log_error("[%s][%d] Soap deal request error!", __FILE__, __LINE__);
		soap_send_fault(soap);
		select_del_event(cycle->lsn_fds, sockid);
		lsn_pub_close_fd(cycle, index);
		soap_pub_destroy(index);
		return SW_DELETE;
	}
	pub_log_debug("[%s][%d] Send cmd to svc success，waiting response...", __FILE__, __LINE__);
	return SW_OK;
}

sw_int_t soapss_accept(sw_fd_list_t *fd_lists)
{
	int	index = 0;
	sw_fd_t	acceptfd;
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d]func soapss_accept argument error ", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	errno = 0;
	acceptfd = soap_accept(g_soap);
	if ((!soap_valid_socket(acceptfd) && errno == EAGAIN) || (!soap_valid_socket(acceptfd) && g_soap->error == SOAP_TCP_ERROR))
	{
		return SW_OK;
	}
	else if (!soap_valid_socket(acceptfd))
	{
		pub_log_info("[%s][%d] soap accept error! acceptfd=[%d] errno=[%d]:[%s]", 
			__FILE__, __LINE__, acceptfd, errno, strerror(errno));
		return SW_CONTINUE;
	}

	pub_log_debug("[%s][%d]connect from[%s]:[%d]", __FILE__, __LINE__, g_soap->host,g_soap->port);
	pub_log_debug("[%s][%d] Accept begin...", __FILE__, __LINE__);
	index = lsn_pub_find_free_fd(cycle, acceptfd);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find free fd error!", __FILE__, __LINE__);
		close(acceptfd);
		return SW_CONTINUE;
	}

	ret = lsn_set_fd_noblock(acceptfd);
	if (ret)
	{
		pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, acceptfd);
		close(acceptfd);
		return SW_CONTINUE;
	}
	
	if (cycle->lsn_conf.socksndbufsize > 0)
	{
		socklen_t	optlen = sizeof(sw_int32_t);
		sw_int32_t	optval = cycle->lsn_conf.socksndbufsize;
		errno = 0;
		setsockopt(acceptfd, SOL_SOCKET, SO_SNDBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] socksndbufsize=[%ld] errno=[%d]:[%s]", 
			__FILE__, __LINE__, optval, errno, strerror(errno));
	}
	
	if (cycle->lsn_conf.sockrcvbufsize > 0)
	{
		socklen_t	optlen = sizeof(sw_int32_t);
		sw_int32_t	optval = cycle->lsn_conf.sockrcvbufsize;
		errno = 0;
		setsockopt(acceptfd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] sockrcvbufsize=[%ld] errno=[%d]:[%s]", 
			__FILE__, __LINE__, optval, errno, strerror(errno));
	}
	
	cycle->link_info[index].sockid = acceptfd;
	cycle->link_info[index].use = 1;
	cycle->link_info[index].start_time = (long)time(NULL);
	soaps[index].soap = soap_copy(g_soap);
	if (soaps[index].soap == NULL)
	{	
		pub_log_error("[%s][%d] soap_copy error! soap->error=[%d]",
			__FILE__, __LINE__, g_soap->error);
		return SW_ERROR;
	}
	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = acceptfd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)soapss_req_work;

	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error fd[%d].", 
			__FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Accept success! index=[%d] FD=[%d]", __FILE__, __LINE__, index, acceptfd);
	
	return SW_OK;
}
