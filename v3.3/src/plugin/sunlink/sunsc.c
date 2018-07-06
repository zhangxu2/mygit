#include "lsn_pub.h"
#include "tcpapi.h"

char	*send_buf = NULL;
char	*recv_buf = NULL;

sw_int_t sunsc_extcevt(sw_fd_list_t *fd_lists);

sw_int_t sunsc_init(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;

	send_buf = (char *)calloc(1, BUF_MAXLEN * 10);
	if (send_buf == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s] SIZE=[%d]",
			__FILE__, __LINE__, errno, strerror(errno), BUF_MAXLEN);
		return -1;
	}

	recv_buf = (char *)calloc(1, BUF_MAXLEN * 10);
	if (recv_buf == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s] SIZE=[%d]",
			__FILE__, __LINE__, errno, strerror(errno), BUF_MAXLEN);
		return -1;
	}
	
	pub_log_info("[%s][%d] sunsc_init success!", __FILE__, __LINE__);
	
	return 0;
}

sw_int_t sunsc_destroy(sw_lsn_cycle_t *cycle)
{
	if (send_buf != NULL)
	{
		free(send_buf);
	}
	
	if (recv_buf != NULL)
	{
		free(recv_buf);
	}
	
	return 0;
}

sw_int_t sunsc_timeoutevt(sw_lsn_cycle_t *cycle)
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

sw_int_t sunsc_call(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	int	ret = 0;
	int	port = 0;
	int	filenum = 0;
	int	timeout = 0;
	long	recv_len = 0;
	char	ip[128];
	char	buf[128];
	char	txcode[16];
	char	nodeid[32];
	char	destnode[32];
	char	sendfile[128];
	char	recvfile[128];
	TAPIHEAD	head;
	
	if (cycle == NULL || loc_vars == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(ip, 0x0, sizeof(ip));
	memset(buf, 0x0, sizeof(buf));
	memset(txcode, 0x0, sizeof(txcode));
	memset(nodeid, 0x0, sizeof(nodeid));
	memset(destnode, 0x0, sizeof(destnode));
	memset(sendfile, 0x0, sizeof(sendfile));
	memset(recvfile, 0x0, sizeof(recvfile));
	memset(&head, 0x0, sizeof(head));
	
	loc_get_zd_data(loc_vars, "#Sender", nodeid);
	loc_get_zd_data(loc_vars, "#Recver", destnode);
	loc_get_zd_data(loc_vars, "#TrnCode", txcode);
	
	strncpy(head.NodeId, nodeid, sizeof(head.NodeId) - 1);
	strncpy(head.DestNode, destnode, sizeof(head.DestNode) - 1);
	strncpy(head.TrType, txcode, sizeof(head.TrType) - 1);
	head.Sleng = locbuf->len;
	head.PackInfo |= htonl( PI_VERSION_V2 );
	head.PackInfo |= htonl( PI_USESECU);

	
	port = cycle->lsn_conf.comm.group[0].remote.port;
	strncpy(ip, cycle->lsn_conf.comm.group[0].remote.ip, sizeof(ip) - 1);
	pub_log_debug("[%s][%d] nodeid=%s, destnode=%s, TrType= %s, ip=%s, port=%d, len=%d", __FILE__, __LINE__,
	head.NodeId, head.DestNode, head.TrType, ip, port, head.Sleng);
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] Send data:[%d]", __FILE__, __LINE__, locbuf->len);
	
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(loc_vars, "$fileflag", buf);
	pub_log_info("[%s][%d] fileflag=[%s]", __FILE__, __LINE__, buf);
	if (buf[0] == '1')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(loc_vars, "$filename", buf);
		strncpy(sendfile, buf, sizeof(sendfile) - 1);
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(loc_vars, "$filenum", buf);
		pub_log_info("[%s][%d] filenum=[%s] filename=[%s]", __FILE__, __LINE__, buf, sendfile);
		filenum = atoi(buf);
		if (filenum <= 0)
		{
			filenum = 1;
		}
		head.PackInfo |= htonl(filenum);
		pub_log_info("[%s][%d] filenum=[%d] filename=[%s]", __FILE__, __LINE__, filenum, sendfile);
	}
	
	if (cmd->timeout > 0)
	{
		timeout = cmd->timeout;
	}
	else
	{
		timeout = cycle->lsn_conf.timeout;
	}
	
	memset(send_buf, 0x0, BUF_MAXLEN);
	memset(recv_buf, 0x0, BUF_MAXLEN);
	memcpy(send_buf, locbuf->data, locbuf->len);
	ret = cli_sndrcv(ip, port, &head, send_buf, sendfile, recv_buf, recvfile, timeout);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] cli_sndrcv error! retcode=[%s]", __FILE__, __LINE__, head.RetCode);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, recv_buf, head.Sleng, "[%s][%d] Recv data:[%d]", 
		__FILE__, __LINE__, head.Sleng);
	
	recv_len = head.Sleng;
	buf_refresh(locbuf);
	buf_append(locbuf, recv_buf, recv_len);
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] Recv data:[%d]", 
		__FILE__, __LINE__, locbuf->len);

	filenum = ntohl(head.PackInfo & htonl(PI_FILE));
	pub_log_info("[%s][%d] filenum=[%d] recvfile=[%s]", __FILE__, __LINE__, filenum, recvfile);
	if (filenum > 0)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", filenum);
		loc_set_zd_data(loc_vars, "$filenum", buf);
		loc_set_zd_data(loc_vars, "$filename", recvfile);
		pub_log_info("[%s][%d] recvfile=[%s]", __FILE__, __LINE__, recvfile);
	}
	
	strncpy(cmd->dst_prdt, cmd->ori_prdt, sizeof(cmd->dst_prdt) - 1);
	ret = lsn_pub_deal_in_task(cycle, loc_vars, cmd, locbuf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		return SW_DELETE;
	}
	pub_log_info("[%s][%d] sunsc_call success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t sunsc_interevt(sw_fd_list_t *fd_lists)
{
	sw_fd_t		out_fd = -1;
	sw_cmd_t	cmd;
	sw_int_t	len = 0;
	sw_int_t	ret = -1;
	sw_int_t	err_flag = 0;
	sw_buf_t	locbuf;
	sw_int64_t	mtype = 0;
	sw_char_t	errcode[32];
	sw_char_t	tmp[64];
	sw_loc_vars_t	loc_vars;
	sw_lsn_cycle_t	*cycle = NULL;
	
	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error! fd_lists is null!", __FILE__, __LINE__);
		return SW_CONTINUE;
	}
	
	memset(errcode, 0x0, sizeof(errcode));
	pub_mem_memzero(&cmd, sizeof(sw_cmd_t));
	pub_mem_memzero(&loc_vars, sizeof(sw_loc_vars_t));
	
	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	out_fd = fd_lists->fd;
	
	ret = pub_loc_vars_alloc(&loc_vars, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_loc_vars_construct error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	len = 0;
	ret = msg_trans_rcv(out_fd, (char*)&cmd, &mtype, &len);
	if (ret != SW_OK)
	{
		loc_vars.free_mem(&loc_vars);
		pub_log_error("[%s][%d] recv cmd error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_CONTINUE;
	}
	
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "SHM%08d", cmd.mtype);
	ret = loc_vars.unserialize(&loc_vars, tmp);
	if (ret != SW_OK)
	{
		loc_vars.free_mem(&loc_vars);
		pub_log_error("[%s][%d] unserialize error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	cmd_print(&cmd);
	trace_insert(&loc_vars, &cmd, TRACE_IN);
	pub_buf_init(&locbuf);
	err_flag = 0;
	switch (cmd.type)
	{
		case SW_POSTLSNREQ:
		case SW_CALLLSNREQ:
		case SW_LINKLSNREQ:
			ret = lsn_pub_deal_out_task(cycle, &loc_vars, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]lsn_pub_platform_deal error",__FILE__,__LINE__);
				break;
			}
			pub_log_info("[%s][%d] deal out task success!", __FILE__, __LINE__);
			ret = sunsc_call(cycle, &loc_vars, &cmd, &locbuf);
			if (ret != SW_OK)
			{
				err_flag = 1;
				pub_log_error("[%s][%d]send data error",__FILE__,__LINE__);
				break;
			}
			break;
		default: 
			pub_log_error("[%s][%d] receive illegal cmd itype=[%d]",
				__FILE__, __LINE__, cmd.type);
			cmd_print(&cmd);
			cmd.type = SW_ERRCMD;
			cmd.ori_type = ND_LSN;
			cmd.dst_type = ND_SVC;
			cmd.msg_type = SW_MSG_RES;
			cmd.task_flg = SW_FORGET;
			strcpy(cmd.ori_svr, cycle->base.name.data);
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
			strcpy(cmd.ori_svc, tmp);
			ret = route_snd_dst(&loc_vars, cycle->base.global_path, &cmd);
			if (ret < 0)
			{
				pub_buf_clear(&locbuf);
				loc_vars.free_mem(&loc_vars);
				pub_log_error("[%s][%d] send message to BP error ret=[%d]", 
					__FILE__,__LINE__, ret);
				return SW_ERROR;
			}
			break;
	}
	
	if (err_flag == 1)
	{
		memset(errcode, 0x0, sizeof(errcode));
		lsn_get_err(&loc_vars, errcode);
		pub_log_debug("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);
		if (strlen(errcode) == 0 || strcmp(errcode, "0000") == 0)
		{
			lsn_set_err(&loc_vars, ERREVERY);
		}
		lsn_deal_err(cycle, &loc_vars, &cmd);
		pub_log_error("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);
	}
	
	trace_insert(&loc_vars, &cmd, TRACE_OUT);
	pub_log_info("[%s][%d] deal task success! mtype=[%ld]", __FILE__, __LINE__, cmd.mtype);
	pub_buf_clear(&locbuf);
	loc_vars.free_mem(&loc_vars);

	return SW_OK;
}

sw_int_t sunsc_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_DELETE;
}

