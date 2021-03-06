#include "lsn_pub.h"
#include "teapi.h"
#include <stdlib.h>
#include <string.h>

char	*send_buf = NULL;
char	*recv_buf = NULL;

sw_int_t teasc_extcevt(sw_fd_list_t *fd_lists);

MD5_Init()
{
	return 0;
}

int MD5_Update ()
{
	return 0;
}

int MD5_Final()
{
	return 0;
}

sw_int_t teasc_init(sw_lsn_cycle_t *cycle)
{
	int	ret;

	memset(g_env_buf, 0x0, sizeof(g_env_buf));
	sprintf(g_env_buf, "WSNADDR=//%s:%d", cycle->lsn_conf.comm.group[0].remote.ip, cycle->lsn_conf.comm.group[0].remote.port);
	ret = putenv(g_env_buf);
	if (ret)
	{
		pub_log_error("[%s][%d] pubenv error! env=[%s]", __FILE__, __LINE__, g_env_buf);
		return SW_ERROR;
	}
	send_buf = (char *)TEDataAlloc(BUF_MAXLEN);
	if (send_buf == NULL)
	{
		pub_log_error("[%s][%d] TEDataAlloc error [%d]!", __FILE__, __LINE__, errno);
		return SW_ERROR;
	}

	recv_buf = (char *)TEDataAlloc(BUF_MAXLEN);
	if (recv_buf == NULL)
	{
		TEDataFree(send_buf);
		send_buf = NULL;
		pub_log_error("[%s][%d] TEDataAlloc error [%d]!", __FILE__, __LINE__, errno);
		return SW_ERROR;
	}
	return 0;
}

sw_int_t teasc_destroy(sw_lsn_cycle_t *cycle)
{
	if (send_buf != NULL)
	{
		TEDataFree(send_buf);
	}
	
	if (recv_buf != NULL)
	{
		TEDataFree(recv_buf);
	}
	
	return 0;
}

sw_int_t teasc_timeoutevt(sw_lsn_cycle_t *cycle)
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

sw_int_t teasc_call(cycle, loc_vars, cmd, locbuf)
sw_lsn_cycle_t *cycle;
sw_loc_vars_t *loc_vars;
sw_cmd_t *cmd;
sw_buf_t *locbuf;
{
	int	ret = 0;
	int	TID = 0;
	int	flag = 0;
	int	recv_len = 0;
	int	filenum = 0;
	sw_char_t	service[NAME_LEN];
	sw_char_t	snode[NAME_LEN];
	sw_char_t	sname[64];
	sw_fd_list_t	fd_list;
	TE_ID	id;
	UPNODE_INFO	upinfo;
	char*	p;
	
	if (cycle == NULL || loc_vars == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(&upinfo, 0, sizeof(UPNODE_INFO));
	memset(service, 0x0, sizeof(service));
	memset(snode, 0x0, sizeof(snode));
	memset(sname, 0x0, sizeof(sname));
	strcpy(sname, (char *)cycle->lsn_conf.comm.group[0].remote.service);
	p = strchr(sname, '|');
	if (p != NULL)
	{
		strncpy(service, sname, p - sname);
		strcpy(snode, p + 1);
	}
	pub_log_info("[%s][%d] snode=[%s] service=[%s]", __FILE__, __LINE__, snode, service);

	strcpy(upinfo.UName[0], snode);
	upinfo.UPort[0] = cycle->lsn_conf.comm.group[0].remote.port;
	strncpy(upinfo.UIPAddr[0], cycle->lsn_conf.comm.group[0].remote.ip, sizeof(upinfo.UIPAddr[0]) - 1);
	
	loc_set_zd_data(loc_vars, "$nodename", snode);
	loc_set_zd_data(loc_vars, "$ip", cycle->lsn_conf.comm.group[0].remote.ip);
	loc_set_zd_int(loc_vars, "$port", cycle->lsn_conf.comm.group[0].remote.port);
	
	if (cycle->chnl.fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] do file_func begin...", __FILE__, __LINE__);
		ret = cycle->chnl.fun.file_func(loc_vars, 0, 0, NULL);
		if(ret < 0)
		{
			TE_tpterm(id);
			lsn_set_err(loc_vars, ERR_SNDFILE);
			pub_log_error("[%s][%d] send file error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] send file success!", __FILE__, __LINE__);
	}
	
	id = NULL;
	id = TE_tpinit(0, 0, (char *)&upinfo);
	if (id == NULL)
	{
		pub_log_error("[%s][%d] TE_tpinit error!", __FILE__, __LINE__);
		lsn_set_err(loc_vars, ERR_TPINIT);
		return SW_ERROR;
	}
	
	TID = TE_tpbegin(PKTNEEDANS, 100, id);
	if(TID < 0)
	{
		pub_log_error("[%s][%d] TE_tpbegin error!", __FILE__, __LINE__);
		TE_tpterm(id);
		id = NULL;
		return SW_ERROR;
	}
		
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] Send data:[%d]", __FILE__, __LINE__, locbuf->len);
	recv_len = 0;
	memcpy(send_buf, locbuf->data, locbuf->len);
	ret = TE_tpcall(service, send_buf, locbuf->len, &recv_buf, &recv_len, &filenum, id);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] TE_tpcall [%s] error! errno [%d]",
			__FILE__, __LINE__, service, errno);
		TE_tpabort(id);
		TE_tpterm(id);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, recv_buf, recv_len, "[%s][%d] Recv data:[%d]", 
		__FILE__, __LINE__, recv_len);
	memcpy(locbuf->data, recv_buf, recv_len);
	locbuf->data[recv_len] = '\0';
	locbuf->len = recv_len;
	if(TE_tpcommit(id) != 0)
	{
		pub_log_error("[%s][%d] TE_tpcommit error! errno [%d]",
			__FILE__, __LINE__, errno);
		TE_tpabort(id);
		TE_tpterm(id);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] Recv data:[%d]", 
		__FILE__, __LINE__, locbuf->len);
	
	strncpy(cmd->dst_prdt, cmd->ori_prdt, sizeof(cmd->dst_prdt) - 1);
	ret = lsn_pub_deal_in_task(cycle, loc_vars, cmd, locbuf);
	if (ret != SW_OK)
	{
		TE_tpterm(id);
		pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
		return SW_DELETE;
	}

	TE_tpterm(id);
	pub_log_info("[%s][%d] teasc_call success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t teasc_interevt(sw_fd_list_t *fd_lists)
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
			TEAppInit();
			ret = teasc_call(cycle, &loc_vars, &cmd, &locbuf);
			TEAppTerm();
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

sw_int_t teasc_extcevt(sw_fd_list_t *fd_lists)
{
	return SW_DELETE;
}

