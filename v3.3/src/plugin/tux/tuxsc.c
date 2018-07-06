#include "lsn_pub.h"
#include "atmi.h"
#include <stdlib.h>
#include <string.h>

char	*send_buf = NULL;
char	*recv_buf = NULL;

sw_int_t tuxsc_extcevt(sw_fd_list_t *fd_lists);

sw_int_t tuxsc_init(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	char	buftype[32];
	
	memset(buftype, 0x0, sizeof(buftype));
	if (cycle->lsn_conf.comm.group[0].buftype[0] != '\0')
	{
		strcpy(buftype, cycle->lsn_conf.comm.group[0].buftype);
	}
	else
	{
		strcpy(buftype, "STRING");
	}
	send_buf = (char *)tpalloc(buftype, NULL, BUF_MAXLEN);
	if (send_buf == NULL)
	{
		pub_log_error("[%s][%d] tpalloc error! tperrno=[%d]:[%s]", 
			__FILE__, __LINE__, tperrno, tpstrerror(tperrno)); 
		return SW_ERROR;
	}

	recv_buf = (char *) tpalloc(buftype, NULL, BUF_MAXLEN);
	if (recv_buf == NULL)
	{
		tpfree(send_buf);
		pub_log_error("[%s][%d] tpalloc error! tperrno=[%d]:[%s]", 
			__FILE__, __LINE__, tperrno, tpstrerror(tperrno)); 
		return SW_ERROR;
	}
	return 0;
}

sw_int_t tuxsc_destroy(sw_lsn_cycle_t *cycle)
{
	if (send_buf != NULL)
	{
		tpfree(send_buf);
	}
	
	if (recv_buf != NULL)
	{
		tpfree(recv_buf);
	}
	
	return 0;
}

sw_int_t tuxsc_timeoutevt(sw_lsn_cycle_t *cycle)
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

sw_int_t tuxsc_call(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	int	ret = 0;
	int	port = 0;
	long	recv_len = 0;
	char	*p = NULL;
	char	*q = NULL;
	char	buf[128];
	sw_char_t	service[NAME_LEN];
	
	if (cycle == NULL || loc_vars == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(g_env_buf, 0x0, sizeof(g_env_buf));
	if (cycle->lsn_conf.comm.group[0].remote.ip[0] == '$' || cycle->lsn_conf.comm.group[0].remote.ip[0] == '#')
	{
		memset(buf, 0x00, sizeof(buf));
		loc_get_zd_data(loc_vars, cycle->lsn_conf.comm.group[0].remote.ip, buf);
		p = buf;
		q = strchr(p, ':');
		if (q == NULL)
		{
			pub_log_error("[%s][%d] ip/port[%s] configure format error, usage[IP:PORT]", __FILE__, __LINE__, buf);
			return SW_ERROR;
		}
		port = atoi(q+1);
		p[q-p] = '\0';
		sprintf(g_env_buf, "WSNADDR=//%s:%d", p, port);	
	}
	else
	{
		sprintf(g_env_buf, "WSNADDR=//%s:%d", cycle->lsn_conf.comm.group[0].remote.ip, cycle->lsn_conf.comm.group[0].remote.port);
	}

	ret = putenv(g_env_buf);
	if (ret)
	{
		pub_log_error("[%s][%d] pubenv error! env=[%s]", __FILE__, __LINE__, g_env_buf);
		return SW_ERROR;
	}
	
	if (tpinit((TPINIT *)NULL) == -1)
	{
		pub_log_error("[%s][%d] tpinit error! [%s]:[%d] tperrno=[%d]:[%s]",
			__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.ip, 
			cycle->lsn_conf.comm.group[0].remote.port, tperrno, tpstrerror(tperrno));
		lsn_set_err(loc_vars, ERR_TPINIT);
		return SW_ERROR;
	}

	if (cycle->chnl.fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] do file_func begin...", __FILE__, __LINE__);
		ret = cycle->chnl.fun.file_func(loc_vars, 0, 0, NULL);
		if(ret < 0)
		{
			tpterm();
			lsn_set_err(loc_vars, ERR_SNDFILE);
			pub_log_error("[%s][%d] send file error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] send file success!", __FILE__, __LINE__);
	}
	
	memset(service, 0x0, sizeof(service));
	if (cycle->lsn_conf.comm.group[0].remote.service[0] == '#')
	{
		loc_vars->get_var(loc_vars, cycle->lsn_conf.comm.group[0].remote.service, service);
	}
	else
	{
		strncpy(service, cycle->lsn_conf.comm.group[0].remote.service, sizeof(service) - 1);
	}
	pub_log_info("[%s][%d] servicename=[%s] service=[%s]", 
		__FILE__, __LINE__, cycle->lsn_conf.comm.group[0].remote.service, service);
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] Send data:[%d]", __FILE__, __LINE__, locbuf->len);
	recv_len = 0;
	memcpy(send_buf, locbuf->data, locbuf->len);
	ret = tpcall(service, send_buf, locbuf->len, &recv_buf, &recv_len, (long)0);
	if (ret < 0 && tperrno == TPETIME)
	{
		pub_log_error("[%s][%d] tpcall [%s] timeout! tperrno=[%d]:[%s]",
			__FILE__, __LINE__, service, tperrno, tpstrerror(tperrno));
		tpterm();
		lsn_set_err(loc_vars, ERR_LSNOUT);
		return SW_ERROR;
	}
	else if (ret < 0 && tperrno != TPESVCFAIL)
	{
		pub_log_error("[%s][%d] tpcall [%s] error! tperrno=[%d]:[%s]",
			__FILE__, __LINE__, service, tperrno, tpstrerror(tperrno));
		tpterm();
		lsn_set_err(loc_vars, ERR_TPCALL);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, recv_buf, recv_len, "[%s][%d] Recv data:[%d]", 
		__FILE__, __LINE__, recv_len);
	
	if (strcmp(cycle->lsn_conf.comm.group[0].buftype, "STRING") == 0 && recv_len <= 0)
	{
		recv_len = strlen(recv_buf);
	}
	memcpy(locbuf->data, recv_buf, recv_len);
	locbuf->data[recv_len] = '\0';
	locbuf->len = recv_len;
	tpterm();
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] Recv data:[%d]", 
		__FILE__, __LINE__, locbuf->len);
	
	strncpy(cmd->dst_prdt, cmd->ori_prdt, sizeof(cmd->dst_prdt) - 1);
	ret = lsn_pub_deal_in_task(cycle, loc_vars, cmd, locbuf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		return SW_DELETE;
	}
	pub_log_info("[%s][%d] tuxsc_call success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t tuxsc_interevt(sw_fd_list_t *fd_lists)
{
	long	mtype = 0;
	sw_fd_t		out_fd = -1;
	sw_int_t	len = 0;
	sw_int_t	ret = -1;
	sw_int_t	err_flag = 0;
	sw_char_t	errcode[32];
	sw_char_t	tmp[64];
	sw_buf_t	locbuf;
	sw_cmd_t	cmd;
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
	sprintf(tmp, "SHM%08ld", cmd.mtype);
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
			ret = tuxsc_call(cycle, &loc_vars, &cmd, &locbuf);
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

sw_int_t tuxsc_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_DELETE;
}

