#include "lsn_pub.h"
#include "lsn_slink.h"

extern sw_int_t lsn_cycle_child_destroy(sw_lsn_cycle_t *cycle);

static sw_int_t buf_to_struct(char *buf, sw_cmd_t *cmd, sw_link_t *link, int *len)
{
	int	cnt = 0;
	int	i = 0, k = 0;
	char	*ptr = buf;
	char	*mpt = NULL;
	char	tmpsep[128];
	char	element[64];

	memset(tmpsep, 0x00, sizeof(tmpsep));
	memset(element, 0x00, sizeof(element));
	while (*ptr != '\0')
	{
		if (*ptr != '#')
		{
			element[i++] = *ptr;
			ptr++;
			continue;
		}

		element[i] = '\0';

		k = 0;
		cnt = 0;
		while (element[k] != '\0')
		{
			if (element[k] != ':')
			{
				tmpsep[cnt] = element[k];
				k++;
				cnt++;
				continue;
			}

			tmpsep[cnt] = '\0';
			break;
		}
		mpt = NULL;
		mpt = element;
		if (strcmp(tmpsep, "trace_no") == 0)
		{
			cmd->trace_no = atoll(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "mtype") == 0)
		{
			cmd->mtype = atol(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "timeout") == 0)
		{
			cmd->timeout = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "start_line") == 0)
		{
			cmd->start_line = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "dst_type") == 0)
		{
			cmd->dst_type = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "ori_type") == 0)
		{
			cmd->ori_type = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "msg_type") == 0)
		{
			cmd->msg_type = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "task_flg") == 0)
		{
			cmd->task_flg = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "type") == 0)
		{
			cmd->type = atoi(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "ori_def") == 0)
		{
			strcpy(cmd->ori_def, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "lsn_name") == 0)
		{
			strcpy(cmd->lsn_name, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "def_name") == 0)
		{
			strcpy(cmd->def_name, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "dst_prdt") == 0)
		{
			strcpy(cmd->dst_prdt, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "dst_svr") == 0)
		{
			strcpy(cmd->dst_svr, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "dst_svc") == 0)
		{
			strcpy(cmd->dst_svc, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "ori_prdt") == 0)
		{
			strcpy(cmd->ori_prdt, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "ori_svr") == 0)
		{
			strcpy(cmd->ori_svr, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "ori_svc") == 0)
		{
			strcpy(cmd->ori_svc, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "sys_date") == 0)
		{
			strcpy(cmd->sys_date, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "udp_name") == 0)
		{
			strcpy(cmd->udp_name, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "clearflag") == 0)
		{
			strcpy(link->clearflag, mpt +k + 1);
		}
		else if (strcmp(tmpsep, "keyinfo") == 0)
		{
			strcpy(link->keyinfo, mpt + k + 1);
		}
		else if (strcmp(tmpsep, "l_mtype") == 0)
		{
			link->mtype = atol(mpt + k + 1);
		}
		else if (strcmp(tmpsep, "pkglen") == 0)
		{
			*len = atoi(mpt + k + 1);
		}
		else
		{
			pub_log_error("[%s][%d]format error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		i = 0;
		ptr++;

		memset(element, 0x0, sizeof(element));
		memset(tmpsep, 0x0, sizeof(tmpsep));
	}

	return SW_OK;
}

static sw_int_t get_cmd_and_pkgbuf(int sockid, sw_buf_t *locbuf, sw_cmd_t *cmd, sw_link_t *link)
{
	int ret = 0;
	int len = 0;
	int pkglen = 0;
	char buf[1024];

	memset(buf, 0x00, sizeof(buf));
	strncpy(buf, locbuf->data, 8);
	len = atoi(buf);
	if (len <= 0)
	{
		pub_log_error("[%s][%d] recv cmd len error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] cmd len=[%d]", __FILE__, __LINE__, len);

	if (g_in_alog && g_alog_tcp)
	{
		memset(buf, 0x00, sizeof(buf));
		strncpy(buf, locbuf->data+8, 8);
		int logcnt = atoi(buf);
		pub_log_info("[%s][%d] log_cnt=[%d]", __FILE__, __LINE__, logcnt);
		ret = alog_add_count(logcnt);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] alog_add_count error", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	memset(buf, 0x00, sizeof(buf));
	strncpy(buf, locbuf->data + 16, len - 8);
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d]",__FILE__, __LINE__);
	ret = buf_to_struct(buf, cmd, link, &pkglen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(locbuf->data, 0x0, BUF_MAXLEN);
	if (locbuf->size < pkglen)
	{
		ret = pub_buf_update(locbuf, pkglen);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pub_buf_update error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	ret = lsn_pub_recv_len(sockid, locbuf->data, pkglen);
	if (ret != pkglen)
	{
		pub_log_error("[%s][%d] recv buf error, ret=[%d]pkglen=[%d] errno[%d][%s].", __FILE__, __LINE__, ret, pkglen, errno, strerror(errno));
		return SW_DELETE;
	}

	locbuf->len = pkglen;	

	return SW_OK;
}

static sw_int_t mq_comm_deal_req(sw_lsn_cycle_t *cycle, sw_buf_t *locbuf, sw_cmd_t *cmd)
{
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_loc_vars_t	locvar;
	sw_chnl_t	*chnl = NULL;

	chnl = &cycle->chnl;
	if (cycle == NULL || locbuf == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] Deal request begin...", __FILE__, __LINE__);	
	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{		
		pub_log_error("%s, %d, pub_loc_vars_alloc error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = locvar.create(&locvar, cmd->mtype);
	if( ret < 0)
	{
		pub_log_error("[%s][%d] create locvar error ", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d] Unserialize begin...", __FILE__, __LINE__);
	ret = locvar.destream(&locvar, locbuf->data);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Unserialize error!", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Unserialize success!", __FILE__, __LINE__);

	loc_set_zd_data(&locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(&locvar, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_info("[%s][%d] pkgmap error! ret=[%d]", __FILE__, __LINE__, ret);
			locvar.free_mem(&locvar);
			return SW_ERROR;
		}
	}

	memset(locbuf->data, 0x00, locbuf->size);
	len = locvar.serialize(&locvar, locbuf);
	if (len <= 0)
	{
		pub_log_error("[%s][%d] serialize error! ret=[%d]", __FILE__, __LINE__, len);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}

	ret = route_snd_dst(&locvar, cycle->base.global_path, cmd);
	if (ret < 0)
	{
		locvar.free_mem(&locvar);
		pub_log_error("[%s][%d] Send cmd to svc error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}

	locvar.free_mem(&locvar);
	pub_log_info("[%s][%d] Deal request success!", __FILE__, __LINE__);

	return SW_OK;
}

static sw_int_t mq_comm_req_work(sw_fd_list_t *fd_lists)
{
	sw_fd_t	sockid;
	sw_int_t	ret = 0;
	sw_buf_t	locbuf;
	sw_cmd_t	cmd;
	sw_link_t	link;
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error, fd_lists is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sockid = fd_lists->fd;
	cycle = (sw_lsn_cycle_t *)fd_lists->data;

	errno = 0;	
	pub_buf_init(&locbuf);
	ret = lsn_pub_recv("A(8)R(L)", sockid, &locbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv net socket error, FD=%d, errno=[%d]:[%s]",__FILE__, __LINE__, sockid, errno, strerror(errno));
		pub_buf_clear(&locbuf);
		close(sockid);
		return SW_DELETE;
	}

	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Recv msg: len=[%d]",__FILE__, __LINE__, locbuf.len);

	memset(&cmd, 0x00, sizeof(cmd));
	memset(&link, 0x00, sizeof(link));
	ret = get_cmd_and_pkgbuf(sockid, &locbuf, &cmd, &link);
	if (ret < 0)
	{
		pub_buf_clear(&locbuf);
		if (ret == SW_DELETE)
		{
			close(sockid);
		}
		pub_log_error("[%s][%d] request client pkg alreay close,close server sockid.", __FILE__, __LINE__);
		return ret;
	}

	cmd_print(&cmd);	
	if (g_mq_use_db == 0 && link.clearflag[0] == '1')
	{
		ret = lsn_delete_linkinfo(cycle, &link, DEL_LOCAL_LINK);
		if (ret < 0)
		{
			pub_buf_clear(&locbuf);
			pub_log_error("[%s][%d] delete link info error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	alog_set_sysinfo(cmd.mtype, cmd.sys_date, cmd.trace_no, cmd.lsn_name);
	ret = mq_comm_deal_req(cycle, &locbuf, &cmd);
	if (ret < 0)
	{
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] Deal request error!", __FILE__, __LINE__);
		return SW_ERROR;	
	}

	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] Send cmd to svc success£¬waiting response...", __FILE__, __LINE__);

	return SW_OK;
}

static sw_int_t mq_comm_accept(sw_fd_list_t *fd_lists)
{
	sw_fd_t	acceptfd = 0;
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d]func tcp_route_accept argument error ", __FILE__, __LINE__);
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
		pub_log_error("[%s][%d] accept error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
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
		pub_log_debug("[%s][%d] socksndbufsize=[%ld] errno=[%d]:[%s]",  __FILE__, __LINE__, optval, errno, strerror(errno));
	}

	if (cycle->lsn_conf.sockrcvbufsize > 0)
	{
		socklen_t	optlen = sizeof(sw_int32_t);
		sw_int32_t	optval = cycle->lsn_conf.sockrcvbufsize;
		errno = 0;
		setsockopt(acceptfd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] sockrcvbufsize=[%ld] errno=[%d]:[%s]", __FILE__, __LINE__, optval, errno, strerror(errno));
	}

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = acceptfd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)mq_comm_req_work;

	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error fd[%d].", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t mq_comm_init(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = 0;
	sw_fd_list_t	fd_list;

	cycle->lsn_fd = lsn_pub_bind(cycle->lsn_conf.comm.group[0].local.ip, cycle->lsn_conf.comm.group[0].local.port);
	if (cycle->lsn_fd < 0)
	{
		pub_log_error("[%s][%d] %s bind [%s][%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->lsn_conf.name, cycle->lsn_conf.comm.group[0].local.ip,
				cycle->lsn_conf.comm.group[0].local.port, errno, strerror(errno));
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] bind [%s][%d] success! FD=[%d]",
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
	fd_list.event_handler = (sw_event_handler_pt)mq_comm_accept;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d] lsn_fd=[%d]", __FILE__, __LINE__, cycle->lsn_fd);
	return SW_OK;
}

sw_int_t mq_comm_select(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	timer = 0;
	int	recv_cnt = 0;
	sw_fd_list_t    *fd_work;

	while(1)
	{
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
				pub_log_info("[%s][%d] event_handler error! ret=[%d]", __FILE__, __LINE__, ret);
			}
			pub_log_bend("\0");
		}
	}

	return SW_OK;
}
