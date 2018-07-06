#include "lsn_pub.h"
#include "SDKpub.h"
#include "tcpapi.h"

void EXIT();
int sunsvr_init(sw_lsn_cycle_t *cycle, char *lsn_name);
int sunsvr_destroy(sw_lsn_cycle_t *cycle);
int sunsvr_savepkg(sw_loc_vars_t *vars, char *pkg, int len);
sw_int32_t lsn_child_register(sw_lsn_cycle_t *cycle, sw_int32_t status, sw_char_t *lsn_name)
{
	sw_int_t	ret = 0;
	sw_proc_info_t	proc_info;

	pub_mem_memzero(&proc_info, sizeof(proc_info));	
	ret = procs_get_proces_info(NULL, cycle->base.name.data, lsn_name, &proc_info);
	if (ret == SW_ERROR)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		strncpy(proc_info.name, lsn_name, sizeof(proc_info.name) - 1);
		strcpy(proc_info.svr_name, cycle->base.name.data);
		proc_info.pid = getpid();
		pub_log_info("[%s][%d] lsn_name=[%s] sendtype=[%d]", 
				__FILE__, __LINE__, lsn_name, cycle->lsn_conf.sendtype);
		if (cycle->lsn_conf.sendtype == O_SEND)
		{
			proc_info.type = ND_LSN_SND;
		}
		else if (cycle->lsn_conf.sendtype == O_RECV)
		{
			proc_info.type = ND_LSN_RCV;
		}
		else
		{
			proc_info.type = ND_LSN;
		}
		proc_info.proc_index = cycle->lsn_conf.process_index;
		proc_info.group_index = cycle->lsn_conf.group_index;
		proc_info.mqid = cycle->lsn_conf.out_msgid;
		proc_info.status = status;
		proc_info.restart_type = LIMITED_RESTART;
	}
	else
	{
		pub_log_info("[%s][%d] proc [%s] already exist! name=[%s] svr_name=[%s]", \
				__FILE__, __LINE__, lsn_name, proc_info.name, proc_info.svr_name);
		proc_info.pid = getpid();
		proc_info.status = status;
		proc_info.mqid = cycle->lsn_conf.out_msgid;
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	pub_log_info("[%s][%d] Child:[%s] msgid=[%d]", __FILE__, __LINE__, proc_info.name, proc_info.mqid);
	ret = procs_lsn_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(cycle->proc_name, lsn_name);
	pub_log_info("[%s][%d] [%s] register success!", __FILE__, __LINE__, lsn_name);
	return SW_OK;
}


sw_int_t sunss_deal_taskres(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_cmd_t *cmd, sw_buf_t *locbuf)
{
	sw_int_t	ret = 0;
	sw_route_t	route;
	sw_chnl_t	*chnl = NULL;

	chnl = &cycle->chnl;
	memset(&route, 0x0, sizeof(route));

	pub_log_debug("[%s][%d] Deal response begin...", __FILE__, __LINE__);
	ret = lsn_get_route_by_name(cycle, cmd->ori_prdt, &route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get route info error! prdt=[%s]", 
				__FILE__, __LINE__, cmd->ori_prdt);
		return SW_ERROR;
	}

	loc_set_zd_data(locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		pub_log_debug("[%s][%d] Map begin...", __FILE__, __LINE__);
		ret = chnl->fun.pkgmap_func(locvar, chnl->cache.pkgmap, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error! ret=[%d]", __FILE__, __LINE__, ret);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] Map success!", __FILE__, __LINE__);
	}

	pub_log_debug("[%s][%d] Pack begin...", __FILE__, __LINE__);
	pub_log_info("[%s][%d] locbuf->size=[%d] locbuf->len=[%d]",
			__FILE__, __LINE__, locbuf->size, locbuf->len);
	ret = pkg_out(locvar, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, route.name);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] pack message error,len=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	locbuf->len = ret;
	pub_log_debug("[%s][%d] Pack success! len=[%d]", __FILE__, __LINE__, locbuf->len);

	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Enc begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(locvar, locbuf, SW_ENC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Enc error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After enc: len=[%d]",
				__FILE__, __LINE__, locbuf->len);
	}
	pub_log_debug("[%s][%d] Deal response success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t sunss_deal_linknull(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd)
{
	mtype_delete(cmd->mtype, 0);

	return SW_OK;
}

sw_int_t sunss_deal_res(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf)
{
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_cmd_t	cmd;
	sw_char_t	buf[128];

	if (cycle == NULL || locbuf == NULL || locvar == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Deal svc response begin...", __FILE__, __LINE__);

	memset(buf, 0x0, sizeof(buf));
	memset(&cmd, 0x00, sizeof(cmd));
	ret = msg_trans_rcv(cycle->lsn_conf.out_fd, (char *)&cmd, (long *)&cmd.mtype, &len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv cmd info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Recv cmd type===[%d]", __FILE__, __LINE__, cmd.type);

	sprintf(buf, "SHM%08d", cmd.mtype);
	pub_log_info("[%s][%d] buf=[%s]", __FILE__, __LINE__, buf);
	ret = locvar->unserialize((void *)locvar, buf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] unserialize error!", __FILE__, __LINE__);
		return SW_CONTINUE;
	}

	cmd_print(&cmd);
	trace_insert(locvar, &cmd, TRACE_IN);
	switch (cmd.type)
	{
		case SW_LINKNULL:
			pub_log_info("[%s][%d] LINKNULL...", __FILE__, __LINE__);
			ret = sunss_deal_linknull(cycle, &cmd);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] Deal LINKNULL error!", __FILE__, __LINE__);
			}
			ret = SW_DONE;
			break;
		case SW_TASKRES:
			ret = sunss_deal_taskres(cycle, locvar, &cmd, locbuf);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] Deal TASKRES error!", __FILE__, __LINE__);
			}
			break;
		default:
			pub_log_error("[%s][%d] Recv invalid cmd! type=[%d]", __FILE__, __LINE__, cmd.type);
			ret = SW_ERROR;
	}
	trace_insert(locvar, &cmd, TRACE_OVER);
	pub_log_debug("[%s][%d] Deal svc response success!", __FILE__, __LINE__);

	return ret;
}

sw_int_t sunss_deal_req(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf, sw_int_t mtype)
{
	sw_int_t	ret = 0;
	sw_int_t	len = 0;
	sw_cmd_t	cmd;
	sw_char_t	buf[100];
	sw_route_t	route;
	sw_chnl_t	*chnl = NULL;

	if (cycle == NULL || locbuf == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d] Deal request begin...", __FILE__, __LINE__);	
	chnl = &cycle->chnl;

	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Dec begin...", __FILE__, __LINE__);
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
		pub_log_error("[%s][%d]unpack message error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Unpack success!", __FILE__, __LINE__);

	loc_set_zd_data(locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		pub_log_debug("[%s][%d] Map begin...", __FILE__, __LINE__);
		ret = chnl->fun.pkgmap_func(locvar, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] Map success!", __FILE__, __LINE__);
	}

	ret = pkg_clear_vars(locvar);
	if (ret < 0)
	{   
		pub_log_error("[%s][%d] Remove useless variable error!", __FILE__, __LINE__);
	}

	pub_log_debug("[%s][%d] Get route begin...", __FILE__, __LINE__);
	memset(&route, 0x0, sizeof(route));
	ret = lsn_get_route(cycle, locvar, locbuf, &route, 1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get route info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Get route end! prdt====[%s] gate=[%d]", __FILE__, __LINE__, route.name, route.gate);

	memset(&cmd, 0x0, sizeof(cmd));
	strcpy(cmd.dst_prdt, route.name);
	cmd.mtype = mtype;
	cmd.trace_no= seqs_new_trace_no();
	if (cmd.trace_no <= 0)
	{
		pub_log_error("[%s][%d] Crate traceno error! ret=[%d]",
				__FILE__, __LINE__, cmd.trace_no);
		return SW_ERROR;
	}
	ret = seqs_get_sysdate(cmd.sys_date);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]get system date error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cmd.type = SW_TASKREQ;
	cmd.ori_type = ND_LSN;
	sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	strcpy(cmd.ori_svc, buf);

	if (chnl->fun.svrmap_func != NULL)
	{
		pub_log_info("[%s][%d] Svrmap begin...", __FILE__, __LINE__);
		ret = chnl->fun.svrmap_func(route.cache.svrmap, locvar, cmd.dst_svr, cmd.def_name, &cmd.level);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] svrsvc map error! ret=[%d]", __FILE__, __LINE__, ret);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] Svrmap sucess!", __FILE__, __LINE__);
	}

	ret = trace_create_info(&cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Crate trace info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	loc_set_zd_data(locvar, "$service", cmd.def_name);
	loc_set_zd_data(locvar, "$server", cmd.dst_svr);
	loc_set_zd_data(locvar, "$listen", cycle->lsn_conf.name);
	loc_set_zd_data(locvar, "$product", route.name);

	cmd.dst_type = ND_SVC;
	cmd.msg_type = SW_MSG_REQ;
	cmd.task_flg = SW_STORE;
	strcpy(cmd.ori_svr, cycle->base.name.data);
	strcpy(cmd.lsn_name, cycle->lsn_conf.name);
	mtype_set_info(cmd.mtype, &cmd);
	trace_insert(locvar, &cmd, TRACE_OUT);

	char test[32];
	memset(test, 0x0, sizeof(test));
	loc_get_zd_data(locvar, "#WorkDate", test);
	pub_log_debug("[%s][%d] TEST #WorkDate=%s", __FILE__, __LINE__, test);
	memset(test, 0x0, sizeof(test));
	loc_get_zd_data(locvar, "#TrnCode", test);
	pub_log_debug("[%s][%d] TEST #TrnCode=%s", __FILE__, __LINE__, test);

	ret = route_snd_dst(locvar, cycle->base.global_path, &cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send cmd to svc error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] Deal request success!", __FILE__, __LINE__);

	return SW_OK;
}

int main()
{
	int	ret = 0;
	int	filenum = 0;
	char	buf[128];
	char	sendfile[64];
	char	recvfile[64];
	char	send_buf[BUF_MAXLEN];
	char	recv_buf[BUF_MAXLEN];
	char	lsnname[64];
	sw_int_t	mtype = 0;
	sw_buf_t	locbuf;
	TAPIHEAD	head;
	unsigned int	recv_len = 0;
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	cycle;
	sw_fd_list_t    fd_list;
	int timer = 0;
	int recv_cnt = 0;
	sw_fd_list_t    *fd_work;

	memset(buf, 0x0, sizeof(buf));
	memset(recv_buf, 0x0, sizeof(recv_buf));
	memset(send_buf, 0x0, sizeof(send_buf));
	memset(sendfile, 0x0, sizeof(sendfile));
	memset(recvfile, 0x0, sizeof(recvfile));
	memset(&head, 0x0, sizeof(head));
	memset(&cycle, 0x0, sizeof(cycle));

	atexit(EXIT);
	ret = svr_rcv(&head, recv_buf, recvfile, 0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] svr_rcv error! ret=[%s]", __FILE__, __LINE__, head.RetCode);
		exit(1);
	}
	recv_len = head.Sleng;
	pub_log_info("[%s][%d] nodeid=[%s] destnode=[%s] trtype=[%s]",
			__FILE__, __LINE__, head.NodeId, head.DestNode, head.TrType);
	pub_log_bin(SW_LOG_INFO, recv_buf, recv_len, "[%s][%d] recv data:[%d]", 
			__FILE__, __LINE__, recv_len);

	if (getenv("SUNSVR_LSNNAME") == NULL)
	{
		pub_log_error("[%s][%d] no lsnname env SUNSVR_LSNNAME!", __FILE__, __LINE__);
		exit(1);
	}
	memset(lsnname, 0x0, sizeof(lsnname));
	strncpy(lsnname, getenv("SUNSVR_LSNNAME"), sizeof(lsnname)-1);

	ret = sunsvr_init(&cycle, lsnname);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] sunsvr init error!", __FILE__, __LINE__);
		exit(1);
	}

	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_loc_vars_alloc error.", __FILE__, __LINE__);
		sunsvr_destroy(&cycle);
		exit(1);
	}

	mtype = mtype_new();
	if (mtype < 0)
	{
		pub_log_error("[%s][%d] create mtype error! ret=[%d]", __FILE__, __LINE__, mtype);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		exit(1);
	}

	ret = locvar.create(&locvar, mtype);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] create locvar error ", __FILE__, __LINE__);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}

	filenum = ntohl(head.PackInfo & htonl(PI_FILE));
	pub_log_info("[%s][%d] filenum=[%d]", __FILE__, __LINE__, filenum);
	if (filenum > 0)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", filenum);
		loc_set_zd_data(&locvar, "$filenum", buf);
		loc_set_zd_data(&locvar, "$filename", recvfile);
		pub_log_info("[%s][%d] recvfile=[%s]", __FILE__, __LINE__, recvfile);
	}
	pub_buf_init(&locbuf);
	if (locbuf.size < recv_len)
	{
		ret = pub_buf_update(&locbuf, recv_len);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pub_buf_update error!", __FILE__, __LINE__);
			pub_buf_clear(&locbuf);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			sunsvr_destroy(&cycle);
			mtype_delete(mtype, 1);
			exit(1);
		}
	}
	strncpy(locbuf.data, recv_buf, recv_len);
	locbuf.len = recv_len;

	ret = sunss_deal_req(&cycle, &locvar, &locbuf, mtype);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] sunss_deal_req error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}
	pub_log_info("[%s][%d] sunss_deal_req success!", __FILE__, __LINE__);

	locvar.free_mem(&locvar);

	/*接收业务进程信息*/	
	/*Allcate memory for event object.*/
	cycle.lsn_fds = pub_pool_palloc(cycle.base.pool, sizeof(sw_fd_set_t));
	if (cycle.lsn_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}       

	/*init select event context.*/
	ret = select_init(cycle.lsn_fds);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle.lsn_conf.out_fd;
	fd_list.data = &cycle;
	fd_list.event_handler = cycle.handler.deal_pkg_handler;
	ret = select_add_event(cycle.lsn_fds, &fd_list);
	if (ret != SW_OK)
	{    
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}
	pub_log_info("[%s][%d] fd_out=[%d]", __FILE__, __LINE__, cycle.lsn_conf.out_fd);

	while (1)
	{
		if (sunsvr_timeoutevt(&cycle, mtype) != SW_OK)
		{
			mtype_delete(mtype, 1);
			pub_buf_clear(&locbuf);
			locvar.free_mem(&locvar);
			sunsvr_destroy(&cycle);
			exit(1);
		}

		timer = cycle.lsn_conf.scantime > 0 ? cycle.lsn_conf.scantime : 1000;
		recv_cnt = select_process_events(cycle.lsn_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
			continue;
		}
		else if (recv_cnt == 0)
		{   
			continue;
		}
		/*recv response*/	
		break;
	}

	memset(&head, 0x0, sizeof(head));
	pub_log_debug("[%s][%d]deal response begin...", __FILE__, __LINE__);
	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_loc_vars_alloc error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}
	buf_refresh(&locbuf);
	ret = sunss_deal_res(&cycle, &locvar, &locbuf);
	if (ret == SW_DONE)
	{
		pub_log_info("[%s][%d] Can not response!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}
	else if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] sunss_deal_res error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		sunsvr_destroy(&cycle);
		mtype_delete(mtype, 1);
		exit(1);
	}
	pub_log_bin(SW_LOG_INFO, locbuf.data, locbuf.len, "[%s][%d] DATA:[%d]",
			__FILE__, __LINE__, locbuf.len);
	sunsvr_destroy(&cycle);
	head.Sleng = locbuf.len;
	strncpy(send_buf, locbuf.data, locbuf.len);
	pub_buf_clear(&locbuf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(&locvar, "$fileflag", buf);
	pub_log_info("[%s][%d] fileflag=[%s]", __FILE__, __LINE__, buf);
	if (buf[0] == '1')
	{
		strncpy(sendfile, buf, sizeof(sendfile) - 1);
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(&locvar, "$filenum", buf);
		pub_log_info("[%s][%d] filenum=[%s]", __FILE__, __LINE__, buf);
		filenum = atoi(buf);
		if (filenum <= 0)
		{
			filenum = 1;
		}
		head.PackInfo |= htonl(filenum);
		pub_log_info("[%s][%d] filenum=[%d]", __FILE__, __LINE__, filenum);
	}
	locvar.destroy(&locvar);
	locvar.free_mem(&locvar);
	pub_log_bin(SW_LOG_INFO, send_buf, head.Sleng, "[%s][%d] send data:[%d]",
			__FILE__, __LINE__, head.Sleng);

	if (buf[0] == '1')
	{
		ret = svr_snd(&head, send_buf, sendfile, 0);
	}
	else
	{
		ret = svr_snd(&head, send_buf, NULL, 0);
	}
	if (ret < 0)
	{
		pub_log_error("[%s][%d] svr_snd error! ret=[%s]", __FILE__, __LINE__, head.RetCode);
		mtype_delete(mtype, 1);
		exit(1);	
	}
	pub_log_info("[%s][%d] Send data success!", __FILE__, __LINE__);
	mtype_delete(mtype, 0);

	exit(0);
}

int sunsvr_timeoutevt(sw_lsn_cycle_t *cycle, int mtype)
{
	int i = 0;
	long    now = 0;
	char    buf[128];
	sw_int32_t  timeout = 0;
	sw_cmd_t    cmd;
	sw_int_t    ret = 0;
	sw_loc_vars_t   vars;
	static int 	start_time = 0;

	if (cycle == NULL)
	{   
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}   

	if (start_time == 0)
	{
		start_time = time(NULL);
		return SW_OK;
	}

	timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
	now = (long)time(NULL);
	if (now - start_time < timeout)
	{   
		return SW_OK;
	}

	pub_log_info("[%s][%d] TIMEOUT! DELETE mtype[%d]", 
			__FILE__, __LINE__, mtype);

	memset(&vars, 0x0, sizeof(vars));
	ret = pub_loc_vars_alloc(&vars, SHM_VARS);
	if (ret == SW_ERROR)
	{
		pub_log_end("[%s][%d] pub_loc_vars_alloc error!",
				__FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] vars alloc success! mtype=[%d]",
			__FILE__, __LINE__, mtype);

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "shm%08d", mtype);
	ret = vars.unserialize(&vars, buf);
	if (ret == SW_ERROR)
	{
		vars.free_mem(&vars);
		pub_log_end("[%s][%d] vars unserialize error! mtype=[%d]",
				__FILE__, __LINE__, mtype);
		return SW_ERROR;
	}

	loc_set_zd_data(&vars, "$errcode", "999a");
	loc_set_zd_data(&vars, "#errcode", "999a");
	loc_set_zd_data(&vars, "#errmsg", "系统内部异常");

	trace_insert(&vars, &cmd, TRACE_TIMEOUT);
	vars.destroy((void *)&vars);                                                                                      
	vars.free_mem(&vars);

	return SW_ERROR;
}


int sunsvr_init(sw_lsn_cycle_t *cycle, char *lsn_name)
{
	int	ret = 0;
	int	fd_out = 0;
	int	msg_out = 0;
	char	name[64];
	char	err_log[64];
	char	dbg_log[64];
	sw_svr_grp_t	grp;

	memset(name, 0x0, sizeof(name));
	memset(dbg_log, 0x0, sizeof(dbg_log));
	memset(err_log, 0x0, sizeof(err_log));

	sprintf(name, "%s_0", lsn_name);
	sprintf(err_log, "error.log");
	sprintf(dbg_log, "sunsvr_%s.log", lsn_name);
	ret = cycle_init(&cycle->base, "sunsvr", ND_LSN, err_log, dbg_log, NULL);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = lsn_pub_cfg_init(cycle, lsn_name);
	if (ret != SW_OK)
	{
		cycle_destory((sw_cycle_t *)cycle);
		pub_log_error("%s, %d, lsn_pub_cfg_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d]name[%s]", __FILE__, __LINE__, cycle->base.name.data);

	ret = cycle_link_shm((sw_cycle_t *)cycle);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_public_link error.", 
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->route_shm.size = pub_mem_align((sizeof(sw_route_cache_t) + sizeof(sw_route_t) * MAX_ROUTE_NUM), SW_ALIGNMENT);

	cycle->shm.size = cycle->route_shm.size;
	pub_log_info("[%s][%d] shm.size=[%d]", __FILE__, __LINE__, cycle->shm.size);

	ret = pub_shm_alloc(&(cycle->shm));
	if (ret != SW_OK)
	{
		pub_log_stderr("[%s][%d] pub_shm_alloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->routes = (sw_route_cache_t *)cycle->shm.addr;
	cycle->routes->head.route_cnt = 0;
	ret = lsn_route_init(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] route init error!", __FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = lsn_get_all_cache(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_get_all_cache error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(&cycle->cache_buf, sizeof(sw_pkg_cache_list_t));

	fd_out = msg_trans_create(cycle->base.global_path, IPC_PRIVATE, 0, &msg_out);
	if (fd_out <= 0)
	{
		pub_log_error("[%s][%d] msg_trans_create error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	cycle->lsn_conf.out_fd = fd_out;
	cycle->lsn_conf.out_msgid = msg_out;

	pub_mem_memzero(&grp, sizeof(sw_svr_grp_t));
	grp.svc_max = cycle->lsn_conf.procmax;
	strcpy(grp.svrname, cycle->base.name.data);
	ret = procs_svr_alloc(&grp);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_svr_alloc(lsn) sucess!",__FILE__,__LINE__);

	ret = lsn_child_register(cycle, SW_S_START, name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Register error! lsn_name=[%s]", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] sunsvr init success!", __FILE__, __LINE__);

	return SW_OK;
}

int sunsvr_destroy(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = SW_ERROR;

	pub_log_info("[%s][%d] Lsn [%s] exit! destroy begin...", 
			__FILE__, __LINE__, cycle->base.name.data);
	if (cycle->lsn_conf.out_fd > 0)
	{
		pub_log_info("[%s][%d] OUT_FD====[%d]", __FILE__, __LINE__, cycle->lsn_conf.out_fd);
		msg_trans_rm(cycle->base.global_path, cycle->lsn_conf.out_fd);
	}
	lsn_free_all_cache(cycle);
	pub_shm_free(&cycle->shm);
	cycle_destory((sw_cycle_t *)cycle);
	pub_log_info("[%s][%d] Lsn [%s] exit! destroy success...", 
			__FILE__, __LINE__, cycle->base.name.data);

	return SW_OK;
}

void EXIT()
{
	pub_log_info("[%s][%d] svr end!", __FILE__, __LINE__);
	tapi_svrend();
}

int sunsvr_savepkg(sw_loc_vars_t *vars, char *pkg, int len)
{
	FILE	*fp = NULL;
	char	dir[128];
	char	filename[128];
	char	refid[32];
	char	txcode[32];
	char	workdt[32];

	memset(dir, 0x0, sizeof(dir));
	memset(filename, 0x0, sizeof(filename));
	memset(refid, 0x0, sizeof(refid));
	memset(txcode, 0x0, sizeof(txcode));

	loc_get_zd_data(vars, ".UFTP.MsgHdrRq.TrnCode", txcode);
	loc_get_zd_data(vars, ".UFTP.MsgHdrRq.WorkDate", workdt);

	sprintf(dir, "%s/dat/sxps", getenv("SWWORK"));
	if (pub_file_check_dir(dir))
	{
		pub_log_error("[%s][%d] mkdir [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, dir, errno, strerror(errno));
		return -1;
	}
	sprintf(filename, "%s/%s_%s_%ld.in", dir, workdt, txcode, (long)time(NULL));
	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
				__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	fwrite(pkg, len, 1, fp);
	fclose(fp);

	return 0;
}
