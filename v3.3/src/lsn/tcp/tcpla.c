#include "lsn_pub.h"
#include "pub_type.h"

sw_fd_t	g_acceptid = -1;

sw_int_t tcpla_accept(sw_fd_list_t *fd_lists);

sw_int_t tcpla_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_OK;
}

sw_int_t tcpla_reset_link(sw_lsn_cycle_t *cycle)
{
	int	index = 0;
	
	pub_log_info("[%s][%d] !!!!!!!Close link!", __FILE__, __LINE__);
	index = cycle->lsn_conf.group_index;
	if (cycle->lsn_conf.comm.group[index].remote.sockid > 0)
	{
		select_del_event(cycle->lsn_fds, cycle->lsn_conf.comm.group[index].remote.sockid);
		close(cycle->lsn_conf.comm.group[index].remote.sockid);
	}
	cycle->lsn_conf.comm.group[index].remote.sockid = -1;
	cycle->lsn_conf.comm.group[index].remote.starttime = (long)time(NULL);
	cycle->lsn_conf.comm.group[index].remote.reset = 0;
	
	return 0;
}

sw_int_t tcpla_reconnect(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	int	index = 0;
	int	sockid = 0;
	
	index = cycle->lsn_conf.group_index;
	sockid = lsn_pub_connect(cycle->lsn_conf.comm.group[index].remote.ip, cycle->lsn_conf.comm.group[index].remote.port);
	if (sockid <= 0)
	{
		cycle->lsn_conf.comm.group[index].remote.sockid = -1;
		cycle->lsn_conf.comm.group[index].remote.starttime = (long)time(NULL);
		pub_log_error("[%s][%d] reconnect [%s][%d] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.comm.group[index].remote.ip, 
			cycle->lsn_conf.comm.group[index].remote.port, errno, strerror(errno));
		return SW_AGAIN;
	}
	else
	{
		cycle->lsn_conf.comm.group[index].remote.sockid = sockid;
		cycle->lsn_conf.comm.group[index].remote.reset = 1;
		cycle->lsn_conf.comm.group[index].remote.starttime = (long)time(NULL);
		ret = lsn_child_update_stat(cycle, SW_S_START);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] update status error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int_t tcpla_deal_recv_timeout(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	
	ret = lsn_deal_recv_timeout_la(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal recv timeout error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

sw_int_t tcpla_deal_send_timeout(sw_lsn_cycle_t *cycle)
{
	int	index = 0;
	int	len = 0;
	long	now = 0;
	long	space = 0;
	sw_fd_t	sockid = 0;
	sw_int_t	ret = 0;
	sw_char_t	activemsg[1024];
	
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	now = (long)time(NULL);
	index = cycle->lsn_conf.group_index;
	sockid = cycle->lsn_conf.comm.group[index].remote.sockid;
	if (sockid < 0 || check_fd_write(sockid) <= 0)
	{
		space = now - cycle->lsn_conf.comm.group[index].remote.starttime;
		if (space < cycle->lsn_conf.activetime)
		{
			return SW_OK;
		}
		ret = lsn_child_update_stat(cycle, SW_S_READY);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] update status error! ret=[%d]", 
				__FILE__, __LINE__, ret);
			return SW_ERROR;
		}
	}

	if (sockid <= 0 || check_fd_write(sockid) <= 0)
	{
		ret = tcpla_reconnect(cycle);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] connect [%s][%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->lsn_conf.comm.group[index].remote.ip, 
				cycle->lsn_conf.comm.group[index].remote.port, errno, strerror(errno));
			return SW_ERROR;
		}
	}
	else if (sockid > 0 && check_fd_write(sockid) > 0)
	{
		if ((cycle->lsn_conf.activemsg[0] != '\0') && 
				(now - cycle->lsn_conf.comm.group[index].remote.starttime >= cycle->lsn_conf.activetime))
		{
			memset(activemsg, 0x0, sizeof(activemsg));
			deal_hex_data(cycle->lsn_conf.activemsg, activemsg);
			len = strlen(activemsg);
			ret = lsn_pub_send(sockid, activemsg, len);
			if (ret < 0)
			{
				tcpla_reset_link(cycle);
				pub_log_error("[%s][%d] Send heart msg[%s] error!", __FILE__, __LINE__, activemsg);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] activemsg===[%s]", __FILE__, __LINE__, activemsg);
			cycle->lsn_conf.comm.group[index].remote.starttime = now;
		}
	}
	
	ret = lsn_deal_send_timeout_la(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal send timeout error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

sw_int_t tcpla_timeoutevt(sw_lsn_cycle_t *cycle)
{
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error! cycle is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (cycle->lsn_conf.sendtype == O_RECV)
	{
		return tcpla_deal_recv_timeout(cycle);
	}
	
	return tcpla_deal_send_timeout(cycle);
}

sw_int_t tcpla_init(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	sockid = 0;
	sw_int_t ret = SW_ERROR;
	
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] tcpla_init param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	for (i = 0; i < cycle->lsn_conf.comm.count; i++)
	{
		cycle->lsn_conf.comm.group[i].local.sockid = -1;
		cycle->lsn_conf.comm.group[i].remote.sockid = -1;
		cycle->lsn_conf.comm.group[i].local.starttime = (long)time(NULL);
		cycle->lsn_conf.comm.group[i].remote.starttime = (long)time(NULL);
	}
	
	for (i = 0; i < cycle->lsn_conf.comm.count; i++)
	{
		sockid = lsn_pub_bind(cycle->lsn_conf.comm.group[i].local.ip, cycle->lsn_conf.comm.group[i].local.port);
		if (sockid < 0)
		{
			pub_log_error("[%s][%d] bind [%s][%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].local.ip,
				cycle->lsn_conf.comm.group[0].local.port, errno, strerror(errno));
			return SW_ERROR;
		}
		cycle->lsn_conf.comm.group[i].local.sockid = sockid;
		ret = lsn_set_fd_noblock(sockid);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] noblock [%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, sockid, errno, strerror(errno));
			return SW_ERROR;
		}
		
		sockid = lsn_pub_connect(cycle->lsn_conf.comm.group[i].remote.ip, cycle->lsn_conf.comm.group[i].remote.port);
		if (sockid < 0)
		{
			pub_log_error("[%s][%d] connect [%s][%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->lsn_conf.comm.group[i].remote.ip, 
				cycle->lsn_conf.comm.group[i].remote.port, errno, strerror(errno));
		}
		else
		{
			pub_log_info("[%s][%d] IP:[%s] PORT:[%d] sockid:[%d]",
				__FILE__, __LINE__, cycle->lsn_conf.comm.group[i].remote.ip, 
				cycle->lsn_conf.comm.group[i].remote.port, sockid);
			cycle->lsn_conf.comm.group[i].remote.sockid = sockid;
			cycle->lsn_conf.comm.group[i].remote.reset = 1;
		}
	}
	pub_log_info("[%s][%d] [%s] success!", __FILE__, __LINE__, __FUNCTION__);

	return SW_OK;
}

sw_int_t tcpla_destroy(sw_lsn_cycle_t *cycle)
{
	close(cycle->lsn_conf.comm.group[cycle->lsn_conf.group_index].local.sockid);
	close(cycle->lsn_conf.comm.group[cycle->lsn_conf.group_index].remote.sockid);

	return SW_OK;
}

sw_int_t tcpla_deal_recv_work(sw_fd_list_t *fd_lists)
{
	sw_fd_t	sockid = 0;
	sw_int_t	ret = 0;
	sw_buf_t	locbuf;
	sw_int_t	index = 0;
	sw_char_t	activemsg[1024];
	sw_lsn_cycle_t	*cycle = NULL;

	memset(activemsg, 0x0, sizeof(activemsg));

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] tcpla_deal_recv_work param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	sockid = fd_lists->fd;
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	index = cycle->lsn_conf.group_index;
	cycle->lsn_conf.comm.group[index].local.starttime = (long)time(NULL);

	pub_buf_init(&locbuf);
	ret = lsn_pub_recv(cycle->lsn_conf.data, sockid, &locbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_DELETE;
	}

	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Recv msg: len=[%d]",
		__FILE__, __LINE__, locbuf.len);
	
	if (cycle->lsn_conf.activemsg[0] != '\0')
	{
		memset(activemsg, 0x0, sizeof(activemsg));
		deal_hex_data(cycle->lsn_conf.activemsg, activemsg);
		if (strncmp(locbuf.data, activemsg, locbuf.len) == 0)
		{
			pub_buf_clear(&locbuf);
			pub_log_info("[%s][%d] activemsg.....", __FILE__, __LINE__);
			return SW_OK;
		}
	}
	
	ret = lsn_deal_recv_work_la(cycle, &locbuf, NULL, NULL);
	if (ret == -2)
	{
		pub_log_info("[%s][%d] Need not to send to svc!", __FILE__, __LINE__);
	}
	else if (ret < 0)
	{
		pub_log_error("[%s][%d] deal req error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	pub_buf_clear(&locbuf);

	return SW_OK;
}

sw_int_t tcpla_reset(sw_fd_list_t *fd_lists)
{
	int	index = 0;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] tcpla_reset param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	index = cycle->lsn_conf.group_index;
	/*** 如果发送socket有数据可读,肯定是链路断开了 ***/
	pub_log_info("[%s][%d] !!!!!!!Close link!", __FILE__, __LINE__);
	index = cycle->lsn_conf.group_index;
	close(cycle->lsn_conf.comm.group[index].remote.sockid);
	cycle->lsn_conf.comm.group[index].remote.sockid = -1;
	cycle->lsn_conf.comm.group[index].remote.reset = 0;
	cycle->lsn_conf.comm.group[index].remote.starttime = (long)time(NULL);
	
	return SW_DELETE;
}

sw_int_t tcpla_accept(sw_fd_list_t *fd_lists)
{
	sw_fd_t	acceptid = 0;
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] tcpla_accept param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	pub_log_info("[%s][%d] fd=[%d]", __FILE__, __LINE__, fd_lists->fd);
	
	acceptid = lsn_pub_accept(fd_lists->fd);
	if (acceptid <= 0)
	{
		pub_log_error("[%s][%d] accept error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] acceptid=[%d]", __FILE__, __LINE__, acceptid);

	ret = lsn_set_fd_noblock(acceptid);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] noblock fd[%d] error!", __FILE__, __LINE__, acceptid);
		close(acceptid);
		return SW_ERROR;
	}
	
	if (g_acceptid > 0)
	{
		select_del_event(cycle->lsn_fds, g_acceptid);
		close(g_acceptid);
		g_acceptid = acceptid;
	}
	
	memset(&fd_list, 0x0, sizeof(fd_list));
	fd_list.fd = acceptid;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)tcpla_deal_recv_work;                                                                          

	ret = select_add_event(cycle->lsn_fds, &fd_list);                                                                
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] tcpla_accept success! acceptid=[%d]", __FILE__, __LINE__, acceptid);
	
	return SW_OK;
}

sw_int_t tcpla_recv_work(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	index = 0;
	int	timer = 0;
	sw_int_t	ret = 0;
	sw_int_t	recv_cnt = 0;
	sw_fd_list_t	fd_list;
	sw_fd_list_t	*fd_work;
	
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] tcpla_recv_work param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(&fd_list, 0x0, sizeof(fd_list));
	index = cycle->lsn_conf.group_index;
	fd_list.fd = cycle->lsn_conf.comm.group[index].local.sockid;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)tcpla_accept;                                                                          
	ret = select_add_event(cycle->lsn_fds, &fd_list);                                                                
	if (ret != SW_OK)                                                                                                
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	while (1)
	{
		timer = cycle->lsn_conf.scantime > 0 ? cycle->lsn_conf.scantime : 1000;
		recv_cnt = select_process_events(cycle->lsn_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
			continue;
		}
		else if (recv_cnt == 0)
		{
			if (cycle->handler.timeout_handler != NULL)
			{
				cycle->handler.timeout_handler(cycle);
			}
			
			if (getppid() == 1)
			{
				pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
				lsn_cycle_destroy(cycle);
				exit(0);
			}
			continue;
		}
		
		for (i = 0; i < recv_cnt; i++)
		{
			pub_log_info("[%s][%d] AAAAAAAAAA:FD=[%d] data=[%x] recv_cnt=[%d] i=[%d]", 
				__FILE__, __LINE__, fd_work[i].fd, &fd_work[i].data, recv_cnt, i);
			if (fd_work[i].fd == cycle->udp_fd)
			{
				ret = fd_work[i].event_handler(fd_work[i].data);
			}
			else
			{
				ret = fd_work[i].event_handler(&fd_work[i]);
			}
			
			if (ret == SW_DELETE)
			{
				select_del_event(cycle->lsn_fds, fd_work[i].fd);
				close(fd_work[i].fd);
				pub_log_end("[%s][%d] SW_DELETE!", __FILE__, __LINE__);
				continue;
			}
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] handle event error! ret=[%d]", __FILE__, __LINE__, ret);
			}
			pub_log_bend("\0");
		}
	}

	return SW_OK;
}

sw_int_t tcpla_call(sw_lsn_cycle_t *cycle, sw_buf_t *locbuf)
{
	int	ret = 0;
	int	times = 0;
	sw_fd_t	sockid = 0;

	while (1)
	{
		sockid = cycle->lsn_conf.comm.group[cycle->lsn_conf.group_index].remote.sockid;
		ret = lsn_pub_send(sockid, locbuf->data, locbuf->len);
		if (ret < 0)
		{
			times++;
			if (times > 1)
			{
				pub_log_error("[%s][%d] Resend error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] send error, reconnect!", __FILE__, __LINE__);
			tcpla_reset_link(cycle);
			ret = tcpla_reconnect(cycle);
			if (ret == SW_OK)
			{
				pub_log_info("[%s][%d] Reconnect success!", __FILE__, __LINE__);
				continue;
			}

			ret = lsn_child_update_stat(cycle, SW_S_READY);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] update status error! ret=[%d]",
					__FILE__, __LINE__, ret);
				return SW_ERROR;
			}
			pub_log_error("[%s][%d] send msg error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		cycle->lsn_conf.comm.group[cycle->lsn_conf.group_index].remote.starttime = (long)time(NULL);
		break;
	}
	
	return SW_OK;
}

sw_int_t tcpla_send_work(sw_lsn_cycle_t *cycle)
{
	int	index = 0;
	long	mtype = 0;
	sw_int_t	err_flag = 0;
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_buf_t	locbuf;
	sw_cmd_t	cmd;
	sw_char_t       buf[64];
	sw_char_t	errcode[8];
	sw_loc_vars_t	locvar;
	sw_fd_list_t	fd_list;
	
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(buf, 0x0, sizeof(buf));
	memset(errcode, 0x0, sizeof(errcode));
	memset(&locvar, 0x0, sizeof(locvar));
	
	memset(&fd_list, 0x0, sizeof(fd_list));
	index = cycle->lsn_conf.group_index;
	if (cycle->lsn_conf.comm.group[index].remote.reset == 1 && cycle->lsn_conf.comm.group[index].remote.sockid > 0)
	{
		fd_list.fd = cycle->lsn_conf.comm.group[index].remote.sockid;
		fd_list.data = (void *)cycle;
		fd_list.event_handler = (sw_event_handler_pt)tcpla_reset;                                                                          
		ret = select_add_event(cycle->lsn_fds, &fd_list);                                                                
		if (ret != SW_OK)                                                                                                
		{
			pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		cycle->lsn_conf.comm.group[index].remote.reset = 0;
	}

	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{		
		pub_log_error("[%s][%d] pub_loc_vars_alloc error.", __FILE__, __LINE__);
		return SW_CONTINUE;
	}
	
	memset(&cmd, 0x00, sizeof(sw_cmd_t));
	ret = msg_trans_rcv(cycle->lsn_conf.out_fd, (char *)&cmd, &mtype, &len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv cmd info error!", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		return SW_CONTINUE;
	}
	
	sprintf(buf, "SHM%08ld", cmd.mtype);
	ret = locvar.unserialize((void *)&locvar, buf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] vars.unserialize error.", __FILE__,__LINE__);
		locvar.free_mem(&locvar);
		mtype_delete(cmd.mtype, 1);
		return SW_CONTINUE;
	}
	pub_log_info("[%s][%d] unserialize success!", __FILE__, __LINE__);

	cmd_print(&cmd);
	if (cmd.type != SW_TASKDENY)
	{
		trace_insert(&locvar, &cmd, TRACE_IN);
	}
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
		case SW_TASKDENY:
			ret = lsn_deal_send_work_la(cycle, &locvar, &cmd, &locbuf);
			if (ret == -2)
			{
				pub_log_info("[%s][%d] Must not to send out!", __FILE__, __LINE__);
				break;
			}
			else if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_info("[%s][%d] deal out task error!", __FILE__, __LINE__);
				break;
			}
			pub_log_info("[%s][%d] deal_out_task success!", __FILE__, __LINE__);
			ret = tcpla_call(cycle, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]send data error", __FILE__, __LINE__);
				break;
			}			
			break;
		case SW_TASKERR:
			pub_log_info("[%s][%d] TASKERR! mtype=[%ld]", __FILE__, __LINE__, cmd.mtype);
			mtype_delete(cmd.mtype, 1);
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
				pub_log_error("[%s][%d],send message to BP error ret=[%d]",                 
					__FILE__,__LINE__, ret);                                                      
				return SW_ERROR;                                                                         
			}                                                                                                
			pub_log_info("[%s][%d],send task success itype[%d]mtype[%ld] ",            
				__FILE__, __LINE__, cmd.type, cmd.mtype);                                        
			break;
	}

	/*postlsn delete mtype*/
	if (cmd.type == SW_POSTLSNREQ)
	{
		g_trace_flag = TRACE_OVER;
	}

	if (cmd.type == SW_TASKDENY)
	{
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		return SW_OK;
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

	if (g_trace_flag == -1 || cmd.type == SW_POSTLSNREQ)
	{
		g_trace_flag = TRACE_OVER;
	}
	trace_insert(&locvar, &cmd, g_trace_flag);
	if (g_trace_flag == TRACE_OVER)
	{
		locvar.destroy(&locvar);
		mtype_delete(cmd.mtype, 0);
	}
	pub_buf_clear(&locbuf);
	locvar.free_mem(&locvar);
	
	return SW_OK;
}

sw_int_t tcpla_interevt(sw_fd_list_t *fd_lists)
{
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	pub_log_info("[%s][%d] sendtype=[%d]", __FILE__, __LINE__, cycle->lsn_conf.sendtype);

	if (cycle->lsn_conf.sendtype != O_SEND && cycle->lsn_conf.sendtype != O_RECV)
	{
		pub_log_error("[%s][%d] sendtype error! sendtype=[%d]", 
			__FILE__, __LINE__, cycle->lsn_conf.sendtype);
		return SW_ERROR;
	}
	
	if (cycle->lsn_conf.sendtype == O_RECV)
	{
		return tcpla_recv_work(cycle);
	}
	
	return tcpla_send_work(cycle);
}

sw_int_t tcpla_destory(sw_lsn_cycle_t *cycle)
{
	int	index = 0;

	index = cycle->lsn_conf.group_index;
	close(cycle->lsn_conf.comm.group[index].remote.sockid);
	close(cycle->lsn_conf.comm.group[index].local.sockid);
	pub_log_info("[%s][%d] !!!!!!!Close link!", __FILE__, __LINE__);

	return SW_OK;
}
