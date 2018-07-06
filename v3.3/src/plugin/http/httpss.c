#include "lsn_pub.h"
#include "pub_type.h"
#include "http.h"

extern sw_int_t httpss_init_ssl(sw_lsn_cycle_t *cycle);

sw_int_t httpss_accept(sw_fd_list_t * fd_lists);

sw_int_t httpss_extcevt(sw_fd_list_t * fd_lists)
{
	return SW_OK;
}

sw_int_t httpss_timeoutevt(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	char	buf[128];
	long	now = 0;
	sw_int32_t	timeout = 0;
	sw_int_t	ret = 0;
	sw_cmd_t	cmd;
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
				pub_log_debug("[%s][%d] TIMEOUT! DELETE mtype[%d]",
					__FILE__, __LINE__, cycle->link_info[i].mtype);
				if (cycle->link_info[i].use == 2)
				{
					memset(&cmd, 0x0, sizeof(cmd));
					cmd.mtype = cycle->link_info[i].mtype;
					cmd.trace_no = cycle->link_info[i].trace_no;
					strncpy(cmd.sys_date, cycle->link_info[i].cmd.sys_date, sizeof(cmd.sys_date) - 1);

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
				select_del_event(cycle->lsn_fds, cycle->link_info[i].sockid);
				http_close_fd(cycle, i);
			}
		}
	}

	return SW_OK;
}

sw_int_t httpss_init(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;

	g_is_ssl = 0;

#if defined(SW_USE_OPENSSL)	
	if (strcmp(cycle->lsn_conf.comtype, "HTTPSSS") == 0)
	{
		g_is_ssl = 1;
	}

	if (g_is_ssl)
	{
		ret = httpss_init_ssl(cycle);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] http_ssl_init error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] httpss_ssl_init success!", __FILE__, __LINE__);
	}
#endif
	cycle->lsn_fd = lsn_pub_bind(cycle->lsn_conf.comm.group[0].local.ip, cycle->lsn_conf.comm.group[0].local.port);
	if (cycle->lsn_fd < 0)
	{
		pub_log_error("[%s][%d] %s bind [%s][%d] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.name, cycle->lsn_conf.comm.group[0].local.ip,
			cycle->lsn_conf.comm.group[0].local.port, errno, strerror(errno));
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] bind [%s][%d] success! FD=[%d]",
		__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].local.ip,
		cycle->lsn_conf.comm.group[0].local.port, cycle->lsn_fd);

	ret = lsn_set_fd_noblock(cycle->lsn_fd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, cycle->lsn_fd);
		close(cycle->lsn_fd);
		return SW_ERROR;
	}
	
	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->lsn_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)httpss_accept;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t httpss_destroy(sw_lsn_cycle_t *cycle)
{

	if (cycle->lsn_fd > 0)
	{
		close(cycle->lsn_fd);
		pub_log_debug("[%s][%d] Destory, close fd:[%d]", __FILE__, __LINE__, cycle->lsn_fd);
	}
#if defined(SW_USE_OPENSSL)
	http_destroy_ssl(cycle);
#endif

	return SW_OK;
}

sw_int_t httpss_res_work(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_cmd_t *cmd)
{
	sw_fd_t	sockid = 0;
	sw_int_t	ret = 0;
	sw_int_t	index = 0;
	sw_buf_t	locbuf;
	sw_route_t	route;
	sw_chnl_t	*chnl = NULL;
	
	chnl = &cycle->chnl;
	memset(&route, 0x0, sizeof(route));
	
	index = lsn_find_index_by_mtype(cycle, cmd->mtype);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find index error! mtype=[%ld]",
			__FILE__, __LINE__, cmd->mtype);
		return SW_ERROR;
	}
	sockid = cycle->link_info[index].sockid;
	ret = lsn_get_route_by_name(cycle, cmd->ori_prdt, &route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get route info error! prdt=[%s]", 
			__FILE__, __LINE__, cmd->ori_prdt);
		select_del_event(cycle->lsn_fds, sockid);
		http_close_fd(cycle, index);
		return SW_ERROR;
	}
	
	loc_set_zd_data(locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(locvar, chnl->cache.pkgmap, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error! ret=[%d]", __FILE__, __LINE__, ret);
			select_del_event(cycle->lsn_fds, sockid);
			http_close_fd(cycle, index);
			return SW_ERROR;
		}
	}
	
	pub_buf_init(&locbuf);
	ret = pkg_out(locvar, &locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, route.name);
	if (ret <= 0)                                                                                                              
	{                                                                                                                          
		pub_log_error("[%s][%d] pack message error,len=[%d]", __FILE__, __LINE__, ret);                                    
		select_del_event(cycle->lsn_fds, sockid);                                                                          
		http_close_fd(cycle, index);                                                                                    
		pub_buf_clear(&locbuf);                                                                                            
		return SW_ERROR;                                                                                                   
	}                                                                                                                          
	locbuf.len = ret;                                                                                                          
	pub_log_debug("[%s][%d] Pack success! len=[%d]", __FILE__, __LINE__, locbuf.len);

	if (cycle->handler.des_handler != NULL)                                                                                    
	{                                                                                                                          
		pub_log_info("[%s][%d] Enc begin...", __FILE__, __LINE__);                                                         
		ret = cycle->handler.des_handler(locvar, &locbuf, SW_ENC);                                        
		if (ret != SW_OK)                                                                                                  
		{                                                                                                                  
			pub_log_error("[%s][%d] Enc error!", __FILE__, __LINE__);                                                  
			select_del_event(cycle->lsn_fds, sockid);                                                                  
			http_close_fd(cycle, index);                                                                            
			pub_buf_clear(&locbuf);                                                                                    
			return SW_ERROR;                                                                                           
		}                                                                                                                  
		pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] After enc: len=[%d]",                                 
			__FILE__, __LINE__, locbuf.len);                                                                           
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Pack message: len=[%d]",
		__FILE__, __LINE__, locbuf.len);
	
	ret = http_add_head(cycle, locvar, &locbuf, 1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Add http head error!", __FILE__, __LINE__);
		select_del_event(cycle->lsn_fds, sockid);                                                                  
		http_close_fd(cycle, index);                                                                            
		pub_buf_clear(&locbuf);                                                                                    
		return SW_ERROR;
	}
	
	ret = http_send(cycle, locbuf.data, locbuf.len, index);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] send message error! sockid=[%d]", __FILE__, __LINE__, sockid);
		select_del_event(cycle->lsn_fds, sockid);
		http_close_fd(cycle, index);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] send message success! sockid=[%d]", __FILE__, __LINE__, sockid);
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Send message: len=[%d]",
		__FILE__, __LINE__, locbuf.len);
	
	if (chnl->fun.file_func != NULL)
	{
		ret = chnl->fun.file_func(locvar, 0, cycle->link_info[index].sockid, NULL);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] runfiletranc error,ret=[%d]", __FILE__, __LINE__, ret);
			select_del_event(cycle->lsn_fds, sockid);
			http_close_fd(cycle, index);
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d]run filedeal sucess", __FILE__, __LINE__);
	}
	
	select_del_event(cycle->lsn_fds, sockid);
	http_close_fd(cycle, index);
	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] send message to net success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t httpss_deal_linknull(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd)
{
	sw_int_t	index = 0;
	
	index = lsn_find_index_by_mtype(cycle, cmd->mtype);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find index error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	http_close_fd(cycle, index);

	return SW_OK;
}

sw_int_t httpss_interevt(sw_fd_list_t *fd_lists)
{
	long	mtype = 0;
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_int_t	index = 0;
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
	pub_log_debug("[%s][%d] type===[%d]", __FILE__, __LINE__, cmd.type);

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
			pub_log_debug("[%s][%d] LINKNULL...", __FILE__, __LINE__);
			ret = httpss_deal_linknull(cycle, &cmd);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] deal linknull error!", __FILE__, __LINE__);
			}
			break;
		case SW_TASKRES:
			pub_log_debug("[%s][%d]recv message from BP", __FILE__, __LINE__);
			ret = httpss_res_work(cycle, &locvar, &cmd);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] response error!", __FILE__, __LINE__);
			}
			break;
		case SW_TASKERR:
			index = lsn_find_index_by_mtype(cycle, cmd.mtype);
			if (index < 0)
			{
				pub_log_error("[%s][%d] find index error! mtype=[%ld]",
					__FILE__, __LINE__, cmd.mtype);
				break;
			}
			http_close_fd(cycle, index);
			break;

		default:
			pub_log_error("[%s][%d] Recv error type=[%d]", __FILE__, __LINE__, cmd.type);
			mtype_delete(cmd.mtype, 1);
			index = lsn_find_index_by_mtype(cycle, cmd.mtype);
			if (index < 0)
			{
				pub_log_error("[%s][%d] find index error! mtype=[%ld]",
					__FILE__, __LINE__, cmd.mtype);
				break;
			}
			http_close_fd(cycle, index);
			break;
	}
	trace_insert(&locvar, &cmd, TRACE_OVER);
	pub_buf_clear(&locbuf);
	locvar.destroy((void *)&locvar);
	locvar.free_mem(&locvar);
	mtype_delete(cmd.mtype, 0);
	
	return SW_OK;
}

sw_int_t httpss_deal_req(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf, int index, int mtype)
{
	int	ret = 0;
	int	sys_status = 0;
	char	buf[100];
	sw_buf_t	pkgbuf;
	sw_cmd_t	cmd;
	sw_route_t	route;
	sw_chnl_t	*chnl = NULL;
	
	if (cycle == NULL || locbuf == NULL || index < 0 || locvar == NULL || mtype < 0 )
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	chnl = &cycle->chnl;
	if (cycle->handler.des_handler != NULL)
	{
		pub_log_debug("[%s][%d] Dec begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(locvar, locbuf, SW_DEC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Dec error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After dec: len=[%d]",
			__FILE__, __LINE__, locbuf->len);
	}
	
	pub_log_debug("[%s][%d] Unpack begin...", __FILE__, __LINE__);                                                             
	ret = pkg_in(locvar, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, route.name);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Unpack error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Unpack success!", __FILE__, __LINE__);

	if (chnl->fun.file_func != NULL)
	{
		pub_log_debug("[%s][%d] Recv file begin...", __FILE__, __LINE__);
		ret = chnl->fun.file_func(locvar, 1, cycle->link_info[index].sockid, NULL);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] runfiletranc error,ret=[%d]", __FILE__, __LINE__, ret);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] Recv file success!", __FILE__, __LINE__);
	}
	
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(locvar, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	memset(&route, 0x0, sizeof(route));
	ret = lsn_get_route(cycle, locvar, locbuf, &route, 1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get route info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] prdt====[%s] gate=[%d]", __FILE__, __LINE__, route.name, route.gate);

	sys_status = seqs_get_syssts();
	pub_log_info("[%s][%d] issuper=[%d] sys_status=[%d]", __FILE__, __LINE__, cycle->lsn_conf.issuper, sys_status);

	if (route.gate == 0 || route.status == 0 || (!seqs_sys_is_run(sys_status) && !cycle->lsn_conf.issuper))
	{
		pub_log_info("[%s][%d] prdt=[%s] gate=[%d] status=[%d]",
			__FILE__, __LINE__, route.name, route.gate, route.status);
		if (chnl->fun.deny_func != NULL)
		{
			ret = chnl->fun.deny_func(locvar, locbuf, chnl->cache.deny);
			if(ret < 0)
			{
				pub_log_info("[%s][%d] denyservice error ", __FILE__, __LINE__);
			}
			else
			{
				pub_log_info("[%s][%d] denyservice sucess ", __FILE__, __LINE__);
			}
			memset(&pkgbuf, 0x0, sizeof(pkgbuf));
			pub_buf_init(&pkgbuf);
			ret = pkg_out(locvar, &pkgbuf, chnl->cache.pkgdeal, &cycle->cache_buf, route.name);
			if (ret < 0)
			{
				pub_buf_clear(&pkgbuf);
				locvar->destroy(locvar);
				locvar->free_mem(locvar);
				pub_log_error("[%s][%d] pack out error!", __FILE__, __LINE__);
				mtype_delete(mtype, 1);
				return SW_ERROR;
			}
	
			if (cycle->handler.des_handler != NULL)
			{
				pub_log_debug("[%s][%d] Enc begin...", __FILE__, __LINE__);
				ret = cycle->handler.des_handler(locvar, &pkgbuf, SW_ENC);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] Enc error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				pub_log_bin(SW_LOG_DEBUG, pkgbuf.data, pkgbuf.len, "[%s][%d] After enc: len=[%d]",
					__FILE__, __LINE__, pkgbuf.len);
			}
			
			ret = lsn_pub_send(cycle->link_info[index].sockid, pkgbuf.data, pkgbuf.len);
			pub_log_bin(SW_LOG_DEBUG, pkgbuf.data, pkgbuf.len, "[%s][%d] ret=[%d]", __FILE__, __LINE__, ret);
			pub_buf_clear(&pkgbuf);
			if (chnl->fun.file_func != NULL)
			{
				pub_log_debug("[%s][%d] Send file begin...", __FILE__, __LINE__);
				ret = chnl->fun.file_func(locvar, 0, cycle->link_info[index].sockid, NULL);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] runfiletranc error,ret=[%d]", __FILE__, __LINE__, ret);
					locvar->destroy(locvar);
					locvar->free_mem(locvar);
					mtype_delete(mtype, 1);
					return SW_ERROR;
				}
				pub_log_debug("[%s][%d] Send file success!", __FILE__, __LINE__);
			}
		}
		locvar->destroy(locvar);
		locvar->free_mem(locvar);
		if (!seqs_sys_is_run(sys_status) && !cycle->lsn_conf.issuper)
		{
			pub_log_debug("[%s][%d] Bp has shutdown, not super listen, refuse to deal request!", __FILE__, __LINE__);
		}
		else
		{
			pub_log_debug("[%s][%d] prdt:[%s] route status off!", __FILE__, __LINE__, route.name);
		}
		mtype_delete(mtype, 1);
		return SW_ERROR;
	}

	memset(&cmd, 0x0, sizeof(cmd));
	strcpy(cmd.dst_prdt, route.name);
	cmd.mtype = mtype;
	sw_int64_t	traceno = 0;
	ret = seqs_get_traceno_and_sysdate(&traceno, cmd.sys_date);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]get traceno and system date error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cmd.trace_no = traceno;
	cmd.type = SW_TASKREQ;
	cmd.ori_type = ND_LSN;
	sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	strcpy(cmd.ori_svc, buf);
	ret = trace_create_info(&cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Crate trace info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (chnl->fun.svrmap_func != NULL)
	{
		pub_log_debug("[%s][%d] svrmap begin...", __FILE__, __LINE__);
		ret = chnl->fun.svrmap_func(route.cache.svrmap, locvar, cmd.dst_svr, cmd.def_name, &cmd.level);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] svrsvc map error! ret=[%d]", __FILE__, __LINE__, ret);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] svrmap sucess!", __FILE__, __LINE__);
		trace_insert_svr(cmd.mtype, cmd.dst_svr, cmd.def_name, cmd.trace_no);
	}

	locvar->set_string(locvar, "$service", cmd.def_name);
	locvar->set_string(locvar, "$server", cmd.dst_svr);
	locvar->set_string(locvar, "$listen", cycle->lsn_conf.name);
	locvar->set_string(locvar, "$product", route.name);


	cycle->link_info[index].start_time = (long)time(NULL);
	cycle->link_info[index].trace_no = cmd.trace_no;
	cycle->link_info[index].mtype = cmd.mtype;
	cycle->link_info[index].use = 2;
	
	memcpy(&cycle->link_info[index].cmd, &cmd, sizeof(cmd));

	cmd.dst_type = ND_SVC;
	cmd.msg_type = SW_MSG_REQ;
	cmd.task_flg = SW_STORE;
	cmd.timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
	strcpy(cmd.ori_svr, cycle->base.name.data);
	trace_insert(locvar, &cmd, TRACE_OUT);
	ret = route_snd_dst(locvar, cycle->base.global_path, &cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send cmd to svc error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	return SW_OK;
}

sw_int_t httpss_req_work(sw_fd_list_t *fd_lists)
{
	sw_fd_t	sockid = 0;
	sw_int_t	ret = 0;
	sw_buf_t	locbuf;
	sw_int_t	index = 0;
	sw_int_t	mtype = 0;
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Alloc vars error!", __FILE__, __LINE__);
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
	if (ret < 0)
	{
		pub_log_error("[%s][%d] create vars error!", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}

	sockid = fd_lists->fd;
	cycle =(sw_lsn_cycle_t *)fd_lists->data;
	index = lsn_find_index_by_fd(cycle, sockid);
	if (index < 0)
	{
		pub_log_error("[%s][%d] Can not find [%d]'s index!", __FILE__, __LINE__, sockid);
		mtype_delete(mtype, 1);
		return SW_DELETE;
	}
	
	pub_buf_init(&locbuf);
	ret = http_recv(cycle, &locvar, &locbuf, index);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv net socket error ,close FD=%d",
			__FILE__, __LINE__, cycle->link_info[index].sockid);
		http_res_err(cycle, index, 400);
		http_close_fd(cycle, index);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		mtype_delete(mtype, 1);
		return SW_DELETE;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Recv msg: len=[%d]",
		__FILE__, __LINE__, locbuf.len);
	
	ret = httpss_deal_req(cycle, &locvar, &locbuf, index, mtype);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] http deal req error!", __FILE__, __LINE__);
		http_res_err(cycle, index, 400);
		http_close_fd(cycle, index);
		pub_buf_clear(&locbuf);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		mtype_delete(mtype, 1);
		return SW_DELETE;
	}
	pub_buf_clear(&locbuf);
	locvar.free_mem(&locvar);
	pub_log_debug("[%s][%d] send cmd to svc success，waiting response...", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t httpss_accept(sw_fd_list_t *fd_lists)
{
	int	index = 0;
	sw_fd_t	acceptfd;
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d]func httpss_accept argument error ", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	acceptfd = lsn_pub_accept(fd_lists->fd);
	if (acceptfd < 0 && errno == EAGAIN)
	{
		return SW_OK;
	}
	else if (acceptfd < 0)
	{
		pub_log_debug("[%s][%d] accept error! errno=[%d]:[%s]", 
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_CONTINUE;
	}

	index = lsn_pub_find_free_fd(cycle, acceptfd);
	if (index < 0)
	{
		pub_log_error("[%s][%d] find free fd error!", __FILE__, __LINE__);
		close(acceptfd);
		return SW_CONTINUE;
	}

#if !defined(SW_USE_OPENSSL)
	ret = lsn_set_fd_noblock(acceptfd);
	if (ret)
	{
		pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, acceptfd);
		close(acceptfd);
		return SW_CONTINUE;
	}
#endif

#if defined(SW_USE_OPENSSL)
	if (!g_is_ssl)
	{
		ret = lsn_set_fd_noblock(acceptfd);
		if (ret)
		{
			pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, acceptfd);
			close(acceptfd);
			return SW_CONTINUE;
		}
	}
	ret = http_accept_ssl(cycle, acceptfd, index);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] http_accept_ssl error!", __FILE__, __LINE__);
		close(acceptfd);
		return SW_ERROR;
	}
#endif
	cycle->link_info[index].sockid = acceptfd;
	cycle->link_info[index].use = 1;
	cycle->link_info[index].start_time = (long)time(NULL);
	pub_log_debug("[%s][%d] accept sucess, index=[%d] FD=[%d]", 
		__FILE__, __LINE__, index, acceptfd);

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = acceptfd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)httpss_req_work;
	
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error fd[%d].", 
			__FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}

	return SW_OK;
}

