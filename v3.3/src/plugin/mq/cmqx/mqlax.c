#include "lsn_pub.h"
#include "mqx_pub.h"
#include "pub_log.h"
#include "pub_shm_ex.h"


msg_format_enum_t g_msg_format = MSG_NONE;

extern sw_int_t lsn_cycle_child_destroy(sw_lsn_cycle_t *cycle);

sw_int_t mqla_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_DELETE;
}

sw_int_t mqla_destroy()
{
	int i = 0;

	for (i = 0; i < g_qmgr.qcnt; i++)
	{
		mq_freeobj(g_qmgr.h_conn, &g_qmgr.qm[i].h_obj);
	}
	mq_freeconn(&g_qmgr.h_conn);
	return 0;
}

sw_int_t mqla_init(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	char 	conn_name[128];

	memset(&g_qmgr, 0x0, sizeof(g_qmgr));
	pub_log_info("[%s][%d] qmgr=[%s] qcnt=[%d] sendtype=[%d]", 
		__FILE__, __LINE__, g_mqcfg.qmgr, g_mqcfg.qcnt, cycle->lsn_conf.sendtype);
	
	g_qmgr.qcnt = g_mqcfg.qcnt;
	strcpy(g_qmgr.qmgr, g_mqcfg.qmgr);
	for (i = 0; i < g_mqcfg.qcnt; i++)
	{
		strcpy(g_qmgr.qm[i].qname, g_mqcfg.qm[i].qname);
	}

	memset(conn_name, 0x0, sizeof(conn_name));
	sprintf(conn_name, "%s(%d)", cycle->lsn_conf.comm.group[0].remote.ip, cycle->lsn_conf.comm.group[0].remote.port);	
	pub_log_debug("[%s][%d] connection name is [%s]", __FILE__, __LINE__, conn_name);

	ret = mq_connx(&g_qmgr.h_conn, g_qmgr.qmgr, conn_name, cycle->lsn_conf.comm.group[0].remote.service);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] conn qmgr [%s] error!", 
			__FILE__, __LINE__, g_qmgr.qmgr);
		return SW_ERROR;
	}
	
	for (i = 0; i < g_qmgr.qcnt; i++)
	{
		ret = mqm_open(g_qmgr.h_conn, &g_qmgr.qm[i].h_obj, g_qmgr.qm[i].qname, cycle->lsn_conf.sendtype);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] open qmgr:[%s] qname:[%s] error!",
				__FILE__, __LINE__, g_qmgr.qmgr, g_qmgr.qm[i].qname);
			return -1;
		}
	}

	return 0;
}

sw_int_t mqla_deal_recv_timeout(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	
	ret = lsn_deal_recv_timeout_la(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal recv timeout error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_ERROR;
}

sw_int_t mqla_deal_send_timeout(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;

	ret = lsn_deal_send_timeout_la(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal send timeout error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

sw_int_t mqla_timeoutevt(sw_lsn_cycle_t *cycle)
{
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error! cycle is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (cycle->lsn_conf.sendtype == O_RECV)
	{
		return mqla_deal_recv_timeout(cycle);
	}

	return mqla_deal_send_timeout(cycle);
}

sw_int_t mqla_search_index(sw_char_t *qname)
{
	int	i = 0;
	int	index = 0;

	pub_log_debug("[%s][%d] qcnt=[%d]", __FILE__, __LINE__, g_qmgr.qcnt);
	if (qname != NULL && qname[0] != '\0')
	{
		for (i = 0; i < g_qmgr.qcnt; i++)
		{
			if (strcmp(g_qmgr.qm[i].qname, qname) == 0)
			{
				return i;
			}
		}
		if (i == g_qmgr.qcnt)
		{
			pub_log_info("[%s][%d] queue [%s] not exist!", __FILE__, __LINE__, qname);
		}
	}
	index = random() % g_qmgr.qcnt;
	pub_log_debug("[%s][%d] index=[%d]", __FILE__, __LINE__, index);

	return index;
}

sw_int_t mqla_call(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars, sw_buf_t *locbuf)
{
	int	ret = 0;
	int	index = 0;
	char	qname[64];
	u_char	msgid[128];
	u_char	corrid[128];

	if (cycle == NULL || locbuf == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(qname, 0x0, sizeof(qname));	
	memset(msgid, 0x0, sizeof(msgid));
	memset(corrid, 0x0, sizeof(corrid));

	loc_get_zd_data(vars, "$send_qname", qname);
	if (qname[0] != '\0')
	{
		pub_log_info("[%s][%d] qname=[%s]", __FILE__, __LINE__, qname);
	}
	index = mqla_search_index(qname);
	pub_log_info("[%s][%d] index=[%d]", __FILE__, __LINE__, index);

	loc_get_zd_data(vars, SENDMSGID, (char *)msgid);
	loc_get_zd_data(vars, SENDCORRID, (char *)corrid);
	pub_log_info("[%s][%d] msgid=[%s] corrid=[%s]", __FILE__, __LINE__, msgid, corrid);
	ret = mq_putmsg(g_qmgr.h_conn, g_qmgr.qm[index].h_obj, locbuf->data, locbuf->len, msgid, corrid, 10000);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mq_putmsg error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] mqla_call success!", __FILE__, __LINE__);
	
	return SW_OK;
}

static int _readn(int fd, void *vptr, size_t n)
{
	size_t	nleft = n;
	ssize_t	nread = 0;
	char	*ptr = vptr;

	while (nleft > 0)
	{
		nread = read(fd, ptr, nleft);
		if (nread <= 0)
		{
			if (errno == EINTR)
			{
				nread = 0;
			}
			else
			{
				return -1;
			}
		}
		ptr += nread;
		nleft -= nread;
	}

	return n - nleft;
}

sw_int_t mqla_deal_recv_work(sw_lsn_cycle_t *cycle, char *recvbuf, int recvlen, u_char *msgid, u_char *corrid)
{
	int	large = 0;
	sw_int_t	ret = 0;
	sw_buf_t	locbuf;
	
	memset(&locbuf, 0x0, sizeof(locbuf));
	
	if (strncmp(recvbuf, LARGE_HEAD, LARGE_HEAD_LEN) == 0)	
	{
		int	fd = 0;
		char	*ptr = NULL;
		struct stat	st;

		pub_log_info("[%s][%d] 大报文文件名=[%s]", __FILE__, __LINE__, recvbuf + 1);
		pub_buf_init(&locbuf);
		memset(&st, 0x0, sizeof(st));
		if (stat(recvbuf + LARGE_HEAD_LEN, &st) < 0)
		{
			pub_log_error("[%s][%d] stat [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, recvbuf + 1, errno, strerror(errno));
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] 大报文长度:[%d]", __FILE__, __LINE__, st.st_size);
		
		ptr = (char *)calloc(1, st.st_size + 1);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, st.st_size, errno, strerror(errno));
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		
		fd = open(recvbuf + LARGE_HEAD_LEN, O_RDONLY);
		if (fd < 0)
		{
			pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
				__FILE__, __LINE__, recvbuf + 1, errno, strerror(errno));
			pub_buf_clear(&locbuf);
			free(ptr);
			return SW_ERROR;
		}
	
		if (_readn(fd, ptr, st.st_size) != st.st_size)
		{
			pub_log_error("[%s][%d] Read error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			pub_buf_clear(&locbuf);
			free(ptr);
			close(fd);
			return SW_ERROR;
		}
		close(fd);

		if (buf_append(&locbuf, ptr, st.st_size) < 0)
		{
			pub_log_error("[%s][%d] buf_append error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			pub_buf_clear(&locbuf);
			free(ptr);
			return SW_ERROR;
		} 
		free(ptr);
		pub_log_bin(SW_LOG_INFO, locbuf.data, locbuf.len, "[%s][%d] 大报文:[%d]",
			__FILE__, __LINE__, locbuf.len);
		
		large = 1;
	}
	else
	{
		locbuf.data = recvbuf;
		locbuf.len = recvlen;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Recv msg: len=[%d]", 
		__FILE__, __LINE__, locbuf.len);
	
	ret = lsn_deal_recv_work_la(cycle, &locbuf, msgid, corrid);
	if (large == 1)
	{
		pub_buf_clear(&locbuf);
	}
	if (ret == -2)
	{
		pub_log_info("[%s][%d] Need not to send to svc!", __FILE__, __LINE__);
		return ret;
	}
	else if (ret < 0)
	{
		pub_log_error("[%s][%d] deal req error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] recv work success, send cmd to svc success!", __FILE__, __LINE__);
	return SW_OK;
}

sw_int_t mqla_recv_work(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	timer = 0;
	int	recv_cnt = 0;
	long	recvlen = 0;
	char	*recvbuf = NULL;
	u_char	msgid[128];
	u_char	corrid[128];
	sw_fd_list_t    *fd_work;

	recvbuf = (char *)(cycle->buf_shm.addr + cycle->lsn_conf.group_index * MAX_GROUP_BUF_LEN + cycle->lsn_conf.process_index * MAX_MSG_LEN);   
	while (1)
	{
		if (getppid() == 1)
		{
			pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
			lsn_cycle_child_destroy(cycle);
			exit(0);
		}

		mqla_deal_recv_timeout(cycle);

		for (i = 0; i < g_qmgr.qcnt; i++)
		{
			recvlen = MAX_MSG_LEN;
			memset(recvbuf, 0x0, MAX_MSG_LEN);
			memset(msgid, 0x0, sizeof(msgid));
			memset(corrid, 0x0, sizeof(corrid));
			MQLONG	rlen = MAX_MSG_LEN;
			ret = mq_getmsg(g_qmgr.h_conn, g_qmgr.qm[i].h_obj, recvbuf, &rlen, msgid, corrid, 100);
			if (ret == -3)
			{
				pub_log_error("[%s][%d] MQRC_CONNECTION_BROKEN!", __FILE__, __LINE__);
				lsn_cycle_child_destroy(cycle);
				exit(1);
			}
			else if (ret < 0)
			{
				continue;
			}
			recvlen = rlen;

			ret = mqla_deal_recv_work(cycle, recvbuf, recvlen, msgid, corrid);
			if (ret == -2)
			{
				pub_log_info("[%s][%d] continue!", __FILE__, __LINE__);
				continue;
			}
			else if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] deal recv req error!", __FILE__, __LINE__);
				pub_log_bend("\0");
				continue;
			}
			pub_log_bend("\0");
		}

		timer = cycle->lsn_conf.scantime > 0 ? cycle->lsn_conf.scantime : 1000;
		recv_cnt = select_process_events(cycle->lsn_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
			pub_log_bend("\0");
			continue;
		}
		else if (recv_cnt > 0)
		{
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
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] event_handler error! ret=[%d]", 
						__FILE__, __LINE__, ret);
				}
				pub_log_bend("\0");
			}
		}
	}

	return SW_OK;
}

sw_int_t mqla_send_work(sw_lsn_cycle_t *cycle)
{
	long		mtype = 0;
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_int_t	err_flag = 0;
	sw_cmd_t	cmd;
	sw_buf_t	locbuf;
	sw_char_t	buf[64];
	sw_char_t	errcode[8];
	sw_loc_vars_t	locvar;
	
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(buf, 0x0, sizeof(buf));
	memset(&cmd, 0x0, sizeof(cmd));
	memset(&locbuf, 0x0, sizeof(locbuf));
	memset(errcode, 0x0, sizeof(errcode));
	memset(&locvar, 0x0, sizeof(locvar));
	
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
			ret = mqla_call(cycle, &locvar, &locbuf);
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

	if (g_trace_flag == -1)
	{
		g_trace_flag = TRACE_OVER;
	}
	trace_insert(&locvar, &cmd, g_trace_flag);
	pub_buf_clear(&locbuf);
	if (g_trace_flag == TRACE_OVER)
	{
		pub_log_info("[%s][%d] Trace over! destroy vars [%ld]", __FILE__, __LINE__, cmd.mtype);
		locvar.destroy(&locvar);
		mtype_delete(cmd.mtype, 0);
	}
	if (cmd.type == SW_POSTLSNREQ)
	{
		pub_log_info("[%s][%d] POSTLSN: delete mtype [%ld]", __FILE__, __LINE__, cmd.mtype);
		locvar.destroy(&locvar);
		mtype_delete(cmd.mtype, 0);
	}
	locvar.free_mem(&locvar);
	
	return SW_OK;
}

sw_int_t mqla_interevt(sw_fd_list_t *fd_lists)
{
	int	group_index = 0;
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
	
	group_index = cycle->lsn_conf.group_index;
	pub_log_info("[%s][%d] group_index=[%d] buftype=[%s]", __FILE__, __LINE__, 
		group_index, cycle->lsn_conf.comm.group[group_index].buftype);
	if (strcmp(cycle->lsn_conf.comm.group[group_index].buftype, "STRING") == 0) 
	{
		g_msg_format = MSG_STRING;
	}

	if (cycle->lsn_conf.sendtype == O_RECV)
	{
		return mqla_recv_work(cycle);
	}
	
	return mqla_send_work(cycle);
}
