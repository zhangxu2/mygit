#include "lsn_pub.h"
#include "jmq.h"
#include "lsn_slink.h"
#include "pub_log.h"
#include "pub_shm_ex.h"
#include "pub_db.h"

char	*send_buf = NULL;
char	*recv_buf = NULL;

msg_format_enum_t g_msg_format = MSG_NONE;

extern sw_int_t mq_comm_init(sw_lsn_cycle_t *cycle);
extern sw_int_t lsn_cycle_child_destroy(sw_lsn_cycle_t *cycle);
extern sw_int_t mq_comm_select(sw_lsn_cycle_t *cycle);

sw_int_t jmqla_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_DELETE;
}

sw_int_t jmqla_destroy()
{
	return 0;
}

sw_int_t jmqla_init(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	port = 0;
	int	sid = 0;
	char	ip[32];
	char *ccsid = NULL;
	char *connchl = NULL;
	static  int jfrist = 1;

	if (jfrist)
	{
		ret = mq_comm_init(cycle);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] mqla init ext error.", __FILE__, __LINE__);
			return SW_ERROR;
		}

		if (cycle->lsn_conf.db_conn_name[0] != '\0')
		{
			ret = mq_db_init(cycle);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] init mq db error.", __FILE__, __LINE__);
				return SW_ERROR;
			}

			ret = pub_db_squery(MQ_EXIST_SQL);
			if (ret < 0)
			{
				pub_log_info("[%s][%d] base tables mq_linkinfo not exist, will create.", __FILE__, __LINE__);
				ret = pub_db_nquery(MQ_SQL);
				if (ret < 0)
				{
					pub_db_close();
					pub_log_error("[%s][%d] create base mq link info table error.", __FILE__, __LINE__);
					return SW_ERROR;
				}

				ret = pub_db_nquery(MQ_INDEX_SQL);
				if (ret < 0)
				{
					pub_db_close();
					pub_log_error("[%s][%d] create base mq link info index error.", __FILE__, __LINE__);
					return SW_ERROR;
				}
					
			}

			pub_db_close();
			g_mq_use_db = 1;
			pub_log_info("[%s][%d] init db ok...g_mq_use_db=[%d]", __FILE__, __LINE__, g_mq_use_db);
		}
		
		jfrist = 0;
	}
	else
	{
		g_usejms = 0;
		if (memcmp(g_mqcfg.qmgr, "JMS:", 4) == 0)
		{
			memset(&g_qmgr, 0x0, sizeof(g_qmgr));
			pub_log_info("[%s][%d] qmgr=[%s] qcnt=[%d] sendtype=[%d]", 
				__FILE__, __LINE__, g_mqcfg.qmgr, g_mqcfg.qcnt, cycle->lsn_conf.sendtype);
			
			g_qmgr.qcnt = 1;
			memcpy(g_qmgr.qmgr, g_mqcfg.qmgr + 4, sizeof(g_qmgr.qmgr) - 1);
			strcpy(g_qmgr.qm[0].qname, g_mqcfg.qm[0].qname);
			
			g_usejms = 1;
			memset(ip, 0x0, sizeof(ip));
			strcpy(ip, cycle->lsn_conf.comm.group[0].remote.ip);
			port = cycle->lsn_conf.comm.group[0].remote.port;
			pub_log_info("[%s][%d] qmgr ip:[%s] port=[%d]", __FILE__, __LINE__, ip, port);
			if (cycle->lsn_conf.sendtype == O_SEND)
			{
				ret = mq_jmsinit(ip, port, g_qmgr.qmgr, g_mqcfg.qm[0].qname,  NULL, cycle->lsn_conf.sendtype);
			}
			else
			{
				ret = mq_jmsinit(ip, port, g_qmgr.qmgr, NULL, g_mqcfg.qm[0].qname, cycle->lsn_conf.sendtype);
			}
			if (ret < 0)
			{
				pub_log_error("[%s][%d] jmsinit error!", __FILE__, __LINE__);
				return SW_ERROR;
			}

			pub_log_info("[%s][%d] mqla init jms success!", __FILE__, __LINE__);
		}
		else
		{
			g_qmgr.qcnt = g_mqcfg.qcnt;
			strcpy(g_qmgr.qmgr, g_mqcfg.qmgr);
			for (i = 0; i < g_mqcfg.qcnt; i++)
			{
				strcpy(g_qmgr.qm[i].qname, g_mqcfg.qm[i].qname);
			}
		
#if defined(AIX)
			sid = 819;
#elif defined(LINUX)
			sid = 1386;
#elif defined(HPUX)
			sid = 1051;
#elif defined(SOLARIS)
			sid = 1386;
#else
			sid = 1208;
#endif
			ccsid = getenv("CCSID");
			if (ccsid != NULL)
			{
				sid = atoi(ccsid);
			}
	
			connchl = getenv("CONNCHNL");
			memset(ip, 0x0, sizeof(ip));
			strcpy(ip, cycle->lsn_conf.comm.group[0].remote.ip);
			port = cycle->lsn_conf.comm.group[0].remote.port;
			ret = mq_javainit(ip, port, g_qmgr.qmgr,connchl, sid);	
			if (ret < 0)
			{
				pub_log_error("[%s][%d] init qmgr [%s] error!", __FILE__, __LINE__, g_qmgr.qmgr);
				return SW_ERROR;
			}
			
			for (i = 0; i < g_qmgr.qcnt; i++)
			{
				ret = mq_javaopen(g_qmgr.qm[i].qname, g_mqcfg_ext.qm[i].qname, i, cycle->lsn_conf.sendtype);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] open qmgr:[%s] qname:[%s] error!",
						__FILE__, __LINE__, g_qmgr.qmgr, g_qmgr.qm[i].qname);
					return -1;
				}
			}

		}

		if (g_mq_use_db)
		{
			ret = mq_db_init(cycle);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] init mq db cfg error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] [%d] connect db success...", __FILE__, __LINE__, cycle->lsn_conf.sendtype);
		}

		if (cycle->lsn_conf.sendtype == O_SEND && cycle->lsn_fd > 0)
		{
			select_del_event(cycle->lsn_fds, cycle->lsn_fd);
		}
	}
	return SW_OK;
}

sw_int_t jmqla_deal_recv_timeout(sw_lsn_cycle_t *cycle)
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

sw_int_t jmqla_deal_send_timeout(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;

	if (g_mq_use_db)
	{
		ret = slink_deal_send_timeout_la(cycle);	
	}
	else
	{
		ret = lsn_deal_send_timeout_la(cycle);
	}
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal send timeout error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

sw_int_t jmqla_timeoutevt(sw_lsn_cycle_t *cycle)
{
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error! cycle is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (cycle->lsn_conf.sendtype == O_RECV)
	{
		return jmqla_deal_recv_timeout(cycle);
	}
	
	return jmqla_deal_send_timeout(cycle);
}

sw_int_t jmqla_search_index(sw_char_t *qname)
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

sw_int_t jmqla_call(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars, sw_buf_t *locbuf)
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
	index = jmqla_search_index(qname);
	pub_log_debug("[%s][%d] index=[%d]", __FILE__, __LINE__, index);
	
	loc_get_zd_data(vars, SENDMSGID, (char *)msgid);
	loc_get_zd_data(vars, SENDCORRID, (char *)corrid);
	pub_log_info("[%s][%d] msgid=[%s] corrid=[%s]", __FILE__, __LINE__, msgid, corrid);
	if (g_usejms)
	{
		ret = mq_jmssend(locbuf->data);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] JMS send message error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		ret = mq_javasend(locbuf->data, locbuf->len, msgid, corrid, 10000, index);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] JAVA send message error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] jmqla_call success!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t jmqla_deal_recv_work(sw_lsn_cycle_t *cycle, char *recvbuf, int recvlen, u_char *msgid, u_char *corrid)
{
	sw_int_t	ret = 0;
	sw_buf_t	locbuf;
	
	memset(&locbuf, 0x0, sizeof(locbuf));
	
	locbuf.data = recvbuf;
	locbuf.len = recvlen;
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Recv msg: len=[%d]", 
		__FILE__, __LINE__, locbuf.len);
	
	ret = lsn_deal_recv_work_la(cycle, &locbuf, msgid, corrid);
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
	return SW_OK;
}

sw_int_t jmqla_deal(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	timer = 0;
	int	recv_cnt = 0;
	int	recvlen = 0;
	char	*recvbuf = NULL;
	u_char	msgid[128];
	u_char	corrid[128];
	sw_fd_list_t    *fd_work;

	recvbuf = (char *)(cycle->buf_shm.addr + cycle->lsn_conf.group_index * MAX_GROUP_BUF_LEN + cycle->lsn_conf.process_index * MAX_MSG_LEN);   
	while (1)
	{
		jmqla_deal_recv_timeout(cycle);
		for (i = 0; i < g_qmgr.qcnt; i++)
		{
			recvlen = MAX_MSG_LEN;
			memset(msgid, 0x0, sizeof(msgid));
			memset(corrid, 0x0, sizeof(corrid));
			memset(recvbuf, 0x0, MAX_MSG_LEN);
			if (g_usejms)
			{
				ret = mq_jmsrecv(4000, recvbuf, &recvlen);
				if (ret < 0)
				{
					if (ret == SW_NO_MSG)
					{
						continue;
					}
					pub_log_error("[%s][%d] JMS get message error!", __FILE__, __LINE__);
					continue;
				}
			}
			else
			{
				ret = mq_javarecv(i, recvbuf, msgid, corrid, &recvlen);
				if (ret < 0)
				{
					if (ret == SW_NO_MSG)
					{
						continue;
					}
					pub_log_error("[%s][%d] JMS get message error!", __FILE__, __LINE__);
					continue;
				}
			}
			ret = jmqla_deal_recv_work(cycle, recvbuf, recvlen, msgid, corrid);
			if (ret == -2)
			{
				pub_log_info("[%s][%d] continue!", __FILE__, __LINE__);
				pub_log_bend("\0");
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
		else if (recv_cnt == 0)
		{			
			if (getppid() == 1)
			{
				pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
				lsn_cycle_child_destroy(cycle);
				exit(0);
			}

			continue;
		}
		
		for (i = 0; i < recv_cnt; i++)
		{
			pub_log_debug("[%s][%d] AAAAAAAAAA:FD=[%d] data=[%x] recv_cnt=[%d] i=[%d]", 
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
			}
			if (ret != SW_OK)
			{
				pub_log_info("[%s][%d] event_handler error! ret=[%d]", 
					__FILE__, __LINE__, ret);
			}
			pub_log_bend("\0");
		}

	}

	return SW_OK;
}

sw_int_t jmqla_recv_work(sw_lsn_cycle_t *cycle)
{
	int index = 0;

	index = cycle->lsn_conf.process_index;

	if (index % 2 == 0)
	{
		jmqla_deal(cycle);
	}
	else
	{
		mq_comm_select(cycle);
	}

	return SW_OK;
}

sw_int_t jmqla_send_work(sw_lsn_cycle_t *cycle)
{
	long	mtype = 0;
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
			ret = jmqla_call(cycle, &locvar, &locbuf);
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

sw_int_t jmqla_interevt(sw_fd_list_t *fd_lists)
{
	int	group_index = 0;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	pub_log_debug("[%s][%d] sendtype=[%d]", __FILE__, __LINE__, cycle->lsn_conf.sendtype);

	if (cycle->lsn_conf.sendtype != O_SEND && cycle->lsn_conf.sendtype != O_RECV)
	{
		pub_log_error("[%s][%d] sendtype error! sendtype=[%d]", 
			__FILE__, __LINE__, cycle->lsn_conf.sendtype);
		return SW_ERROR;
	}
	
	group_index = cycle->lsn_conf.group_index;
	pub_log_debug("[%s][%d] group_index=[%d] buftype=[%s]", __FILE__, __LINE__, 
		group_index, cycle->lsn_conf.comm.group[group_index].buftype);
	if (strcmp(cycle->lsn_conf.comm.group[group_index].buftype, "STRING") == 0) 
	{
		g_msg_format = MSG_STRING;
		mq_javafmt(g_msg_format);
	}

	if (cycle->lsn_conf.sendtype == O_RECV)
	{
		return jmqla_recv_work(cycle);
	}
	
	return jmqla_send_work(cycle);
}
