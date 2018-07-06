#include "lsn_pub.h"
#include "pub_ares.h"
#include "lsn_alink.h"
#include "lsn_slink.h"

extern sw_int_t lsn_update_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link);
extern sw_int_t mtype_set_info(sw_int_t mtype, const sw_cmd_t *cmd);
extern int alink_struct2buf(char *buf, int flag, void *data);

static int get_socket_by_ip_and_port(sw_char_t *ip, int port)
{
	int	i = 0;
	int	index = -1;
	int	sockid = 0;
	fd_status_t	fs;

	for (i = 0; i < MAX_MACHINE_NUM; i++)
	{
		if (strcmp(g_mp_addr[i].ip, ip) == 0 && g_mp_addr[i].port == port && g_mp_addr[i].use == 1)
		{
			break;
		}
		if (g_mp_addr[i].use == 0)
		{
			index = i;
		}
	}
	if (i == MAX_MACHINE_NUM) 
	{
		sockid = lsn_pub_connect(ip, port);
		if (sockid < 0)
		{
			pub_log_error("[%s][%d] Connect to [%s]:[%d] error!",
					__FILE__, __LINE__, ip, port);
			return SW_ERROR;
		}
		if (index < 0)
		{
			pub_log_error("[%s][%d] No enough space to save sock info!",
					__FILE__, __LINE__);
		}
		else
		{
			g_mp_addr[index].use = 1;
			g_mp_addr[index].sockid = sockid;
			g_mp_addr[index].port = port;
			strcpy(g_mp_addr[index].ip, ip);
		}
		return sockid;
	}

	index = i;
	sockid = g_mp_addr[index].sockid;
	fs = alog_check_fd(sockid);
	if (fs != FD_WRITEABLE)
	{
		close(sockid);
		sockid = lsn_pub_connect(ip, port);
		if (sockid < 0)
		{
			pub_log_error("[%s][%d] reconnect [%s]:[%d] error![%d]:[%s]",
					__FILE__, __LINE__, ip, port, errno, strerror(errno));
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] reconnect [%s]:[%d] success!", __FILE__, __LINE__, ip, port);
		g_mp_addr[index].sockid = sockid;
	}

	return sockid;
}

static int pkg_deal_and_send(sw_loc_vars_t *locvar, sw_cmd_t *cmd, sw_link_t *link)
{
	int	ret = 0;
	int	len = 0;
	int	cmdlen = 0;
	int	port = 0;
	int	logcnt = 0;
	int	sockid = 0;
	char	*ptr = NULL;
	char	ip[32];
	char	tmp[512];
	sw_buf_t	locbuf;

	memset(ip, 0x0, sizeof(ip));
	memset(tmp, 0x0, sizeof(tmp));

	pub_buf_init(&locbuf);
	locbuf.len = 8;

	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08d", logcnt);
	memcpy(locbuf.data + locbuf.len, tmp, 8);

	if (g_in_alog)
	{
		logcnt = alog_get_count();
		logcnt++;

		memset(tmp, 0x0, sizeof(tmp));
		sprintf(tmp, "%08d", logcnt);
		memcpy(locbuf.data + locbuf.len, tmp, 8);
	}
	else
	{
		memset(tmp, 0x0, sizeof(tmp));
		memset(tmp, '0', 8);
		memcpy(locbuf.data + locbuf.len, tmp, 8);
	}

	locbuf.len += 8;
	ret = alink_struct2buf(locbuf.data + locbuf.len, 1, cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] alink_struct2buf error", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}

	memset(tmp, 0x00, sizeof(tmp));
	sprintf(tmp, "clearflag:%s#", link->clearflag);
	strcat(locbuf.data + locbuf.len, tmp);

	memset(tmp, 0x00, sizeof(tmp));
	sprintf(tmp, "keyinfo:%s#", link->keyinfo);
	strcat(locbuf.data + locbuf.len, tmp);

	memset(tmp, 0x00, sizeof(tmp));
	sprintf(tmp, "l_mtype:%ld#", link->mtype);
	strcat(locbuf.data + locbuf.len, tmp);

	cmdlen = strlen(locbuf.data + 16);	
	locbuf.len += cmdlen;	

	memset(tmp, 0x0, sizeof(tmp));
	len = sprintf(tmp, "pkglen:%08d#", ret);

	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08d", cmdlen + len + 8);
	memcpy(locbuf.data, tmp, 8);

	locbuf.len += len;	
	ret = locvar->tostream(locvar, locbuf.data + locbuf.len, locbuf.size);
	if (ret < 0)
	{
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] Serialize error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "pkglen:%08d#", ret);
	memcpy(locbuf.data + locbuf.len - len, tmp, len);
	locbuf.len += ret;
	locbuf.data[locbuf.len] = '\0';
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Send package:[%d]",
			__FILE__, __LINE__, locbuf.len);	

	ptr = strstr(link->addr, ":");
	if (ptr != NULL)
	{
		memset(ip, 0x0, sizeof(ip));
		memcpy(ip, link->addr, ptr - link->addr);
		port = atoi(ptr + 1);
	}

	sockid = get_socket_by_ip_and_port(ip, port);
	if (sockid < 0)
	{
		pub_log_error("[%s][%d] Get socket info error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] snd pkg ip[%s][%d]", __FILE__, __LINE__, ip, port);

	ret = lsn_pub_send(sockid, locbuf.data, locbuf.len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send package error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] send package success!", __FILE__, __LINE__);

	return 0;
}

sw_int_t lsn_deal_recv_work_la(sw_lsn_cycle_t *cycle, sw_buf_t *locbuf, u_char *msgid, u_char *corrid)
{
	int	ret = 0;
	int	resflag = 0;
	long	mtype = 0;
	char		addr[64];
	char		machine[64];
	sw_char_t	buf[128];
	sw_char_t	key[128];
	sw_char_t	pkgtype[8];
	sw_char_t	msgtype[32];
	sw_cmd_t	cmd;
	sw_link_t	link;
	sw_chnl_t	*chnl = NULL;
	sw_route_t	route;
	sw_loc_vars_t	locvar;

	memset(buf, 0x0, sizeof(buf));
	memset(key, 0x0, sizeof(key));
	memset(pkgtype, 0x0, sizeof(pkgtype));
	memset(msgtype, 0x0, sizeof(msgtype));
	memset(&cmd, 0x0, sizeof(cmd));
	memset(&link, 0x0, sizeof(link));
	memset(&route, 0x0, sizeof(route));

	if (cycle == NULL || locbuf == NULL)
	{
		pub_log_error("[%s][%d] deal recv work, param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	chnl = &cycle->chnl;

	memset(&locvar, 0x0, sizeof(locvar));
	ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_loc_vars_alloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	mtype = mtype_new();
	if (mtype < 0)
	{
		pub_log_error("[%s][%d] create mtype error ret=[%d]", __FILE__, __LINE__, mtype);
		locvar.free_mem(&locvar);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] creat mtype sucess mtype=[%d]", __FILE__, __LINE__, mtype);
	ret = locvar.create(&locvar, mtype);
	if (ret < 0)
	{
		locvar.free_mem(&locvar);
		pub_log_error("[%s][%d] create locvar error ", __FILE__, __LINE__);
		mtype_delete(mtype, 2);
		return SW_ERROR;
	}	
	pub_log_info("[%s][%d] creat locvar sucess vid=[%d]", __FILE__, __LINE__, mtype);

	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Dec begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(&locvar, locbuf, SW_DEC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Dec error!", __FILE__, __LINE__);
			locvar.destroy((void *)&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(mtype, 2);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After dec: len=[%d]",
				__FILE__, __LINE__, locbuf->len);
	}

	ret = pkg_in(&locvar, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, NULL);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]unpack message error ret=[%d]", __FILE__, __LINE__, ret);
		locvar.destroy((void *)&locvar);
		locvar.free_mem(&locvar);
		mtype_delete(mtype, 2);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d]unpack message is successfully", __FILE__, __LINE__);

	memset(&route, 0x0, sizeof(route));
	ret = lsn_get_route(cycle, &locvar, locbuf, &route, 1);
	if (ret != SW_OK)
	{
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		pub_log_error("[%s][%d] get route error!", __FILE__, __LINE__);
		mtype_delete(mtype, 2);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] prdt=[%s] gate=[%d]", __FILE__, __LINE__, route.name, route.gate);

	memset(&cmd, 0x00, sizeof (cmd));
	strncpy(cmd.dst_prdt, route.name, sizeof(cmd.dst_prdt) - 1);

	if (route.fun.type_func != NULL)
	{
		memset(pkgtype, 0x0, sizeof(pkgtype));
		ret = route.fun.type_func(locbuf->data, pkgtype);
		if (ret < 0)
		{
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			pub_log_error("[%s][%d] type analyze error!", __FILE__, __LINE__);
			mtype_delete(mtype, 2);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] type analyze success! pkgtype=[%s]", 
				__FILE__, __LINE__, pkgtype);
	}

	memset(&link, 0x0, sizeof(link));
	link.sendtype = O_RECV;
	strncpy(link.pkgtype, pkgtype, sizeof(link.pkgtype)-1);
	link.pkgbuf = locbuf->data;
	link.pkglen = locbuf->len;
	if (msgid != NULL && msgid[0] != '\0')
	{
		memcpy(link.msgid, msgid, sizeof(link.msgid) - 1);
	}

	if (corrid != NULL && corrid[0] != '\0')
	{
		memcpy(link.corrid, corrid, sizeof(link.corrid) - 1);
	}

	if (route.fun.prean_func != NULL)
	{
		ret = route.fun.prean_func(&locvar, &link, O_RECV);
		if (ret < 0)
		{
			pub_log_info("[%s][%d] pre analyze error! ret=[%d]", __FILE__, __LINE__, ret);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(mtype, 2);
			return SW_ERROR;
		}
		if (link.notsend == 1)
		{
			pub_log_info("[%s][%d] notsend=[%d], Need't to send!", __FILE__, __LINE__, link.notsend);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(mtype, 2);
			return -2;
		}
		pub_log_info("[%s][%d] preanalyze success! asktype=[%s]", 
				__FILE__, __LINE__, link.asktype);
	}
	else
	{
		/*** 兼容以前的模式 ***/
		memset(buf, 0x0, sizeof(buf));
		memset(key, 0x0, sizeof(key));
		loc_get_zd_data(&locvar, "#cbmkey", key);
		strncpy(link.keyinfo, key, sizeof(link.keyinfo) - 1);
		loc_get_zd_data(&locvar, "#msgtype", buf);
		if (buf[2] % 2 == 0)
		{
			link.asktype[0] = '0';
			link.saveflag[0] = '1';
			loc_set_zd_data(&locvar, "$asktype", "0");
		}
		else
		{
			link.asktype[0] = '1';
			link.clearflag[0] = '1';
			loc_set_zd_data(&locvar, "$asktype", "1");
		}
	}

	if (link.resflag[0] == '1')
	{
		/*** 特殊处理的来帐报文,如果恢复到链路信息就按照应答处理,否则按照请求进行处理 ***/
		ret = lsn_load_linkinfo(cycle, &link);
		if (ret < 0)
		{
			link.asktype[0] = '0';
			pub_log_info("[%s][%d] load linkinfo error，as a new request,link", __FILE__, __LINE__);
		}
		else
		{
			pub_log_info("[%s][%d] load linkinfo success", __FILE__, __LINE__);
			link.asktype[0] = '1';
			link.clearflag[0] = '1';
			resflag = 1;
		}
	}
	pub_log_info("[%s][%d] asktype=[%s]", __FILE__, __LINE__, link.asktype);
	if (link.asktype[0] != '0' && link.asktype[0] != '1')
	{
		pub_log_error("[%s][%d] asktype errror! asktype=[%s]",
				__FILE__, __LINE__, link.asktype);
		locvar.destroy(&locvar);
		locvar.free_mem(&locvar);
		mtype_delete(mtype, 2);
		return SW_ERROR;
	}

	if (link.asktype[0] == '1')
	{
		/*** 如果预分析中未设置resflag,则恢复链路信息失败时错误退出
		     如果恢复链路信息失败时当做请求进行处理,则必须在预分析中给resflag赋值1
		***/
		if (resflag == 0)
		{
			ret = lsn_load_linkinfo(cycle, &link);
			if (ret < 0)
			{
				pub_log_info("[%s][%d] load link info error!", __FILE__, __LINE__);
				locvar.destroy((void *)&locvar);
				locvar.free_mem(&locvar);
				mtype_delete(mtype, 2);
				return SW_ERROR;
			}
		}

		pub_log_info("[%s][%d] mtype=[%ld] sys_date=[%s] traceno=[%lld] type=[%d] lsn_name=[%s]",
				__FILE__, __LINE__, link.mtype, link.sys_date, link.traceno, link.type, link.lsn_name);
		cmd.type = link.type;
		cmd.mtype = link.mtype;
		cmd.trace_no = link.traceno;
		strncpy(cmd.sys_date, link.sys_date, sizeof(cmd.sys_date) - 1);

		alog_set_sysinfo(cmd.mtype, cmd.sys_date, cmd.trace_no, link.lsn_name);
		memset(addr, 0x0, sizeof(addr));
		memset(machine, 0x0, sizeof(machine));
		strcpy(machine, g_eswid);
		sprintf(addr, "%s:%d", cycle->lsn_conf.comm.group[0].local.ip, cycle->lsn_conf.comm.group[0].local.port);
		if ((g_link_in_ares || g_mq_use_db) && (link.machine[0] != '\0' && link.addr[0] != '\0') 
				&& (strcmp(machine, link.machine) != 0 || strcmp(addr, link.addr) != 0))
		{
			sw_int64_t traceno = 0;
			char sysdate[16] = {0};
			ret = seqs_get_traceno_and_sysdate(&traceno, sysdate);
			if (ret < 0)
			{
				pub_log_error("[%s][%d]get traceno and system date error", __FILE__, __LINE__);
				locvar.destroy(&locvar);
				locvar.free_mem(&locvar);
				mtype_delete(mtype, 1);
				return SW_ERROR;
			}

			alog_set_sysinfo(mtype, sysdate,traceno,cycle->lsn_conf.name);
			ret = lsn_delete_linkinfo(cycle, &link, DEL_REMOTE_LINK);
			if (ret == SW_ERROR)
			{
				pub_log_error("[%s][%d] delete linkinfo error", __FILE__, __LINE__);
			}
			cmd.type = SW_CALLLSNRES;			
			cmd.ori_type = ND_LSN;	
			cmd.dst_type = ND_SVC;
			cmd.msg_type = SW_MSG_RES;
			cmd.task_flg = SW_FORGET;
			strcpy(cmd.ori_svr, cycle->base.name.data);	
			memset(buf, 0x0, sizeof(buf));
			if ((strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0) &&
					cycle->lsn_conf.comm.count > 1)
			{
				sprintf(buf, "%s_%d_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.group_index, cycle->lsn_conf.process_index);
			}
			else
			{
				sprintf(buf, "%s_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
			}
			strncpy(cmd.ori_svc, buf, sizeof(cmd.ori_svc) - 1);

			strncpy(cmd.lsn_name, link.lsn_name, sizeof(cmd.lsn_name) - 1);
			ret = pkg_deal_and_send(&locvar, &cmd, &link);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] send package to [%s] faild!",
						__FILE__, __LINE__, link.addr);
				locvar.destroy((void *)&locvar);
				locvar.free_mem(&locvar);
				mtype_delete(mtype, 1);
				return -1;
			}
			pub_log_info("[%s][%d] creat locvar sucess vid=[%d]", __FILE__, __LINE__, cmd.mtype);

			locvar.destroy((void *)&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(mtype, 1);
			return SW_OK;
		}


		if (link.clearflag[0] == '1')
		{
			ret = lsn_delete_linkinfo(cycle, &link, DEL_ALL_LINK);
			if (ret < 0)
			{
				locvar.destroy(&locvar);
				locvar.free_mem(&locvar);
				pub_log_info("[%s][%d] delete link info error!", __FILE__, __LINE__);
				mtype_delete(mtype, 2);
				return SW_ERROR;
			}
		}

		pub_log_info("[%s][%d] res cmd.mtype=[%ld]", __FILE__, __LINE__, cmd.mtype);
		locvar.merge(&locvar, cmd.mtype);
		ret = locvar.create(&locvar, cmd.mtype);
		if (ret < 0)
		{
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			pub_log_error("[%s][%d] create locvar error ", __FILE__, __LINE__);
			mtype_delete(mtype, 2);
			return SW_ERROR;
		}

		sw_cmd_t tmp_cmd;
		memset(&tmp_cmd, 0x0, sizeof(tmp_cmd));
		strncpy(tmp_cmd.dst_prdt, route.name, sizeof(tmp_cmd.dst_prdt) - 1);
		strncpy(tmp_cmd.lsn_name, cycle->lsn_conf.name, sizeof(tmp_cmd.lsn_name) - 1);
		mtype_set_info(mtype, &tmp_cmd);

		mtype_delete(mtype, 2);
		cmd.type = SW_CALLLSNRES;

		if (link.infoflag[0] == '1' && route.fun.recov_func != NULL)
		{
			ret = route.fun.recov_func(&locvar, &link);
			if (ret < 0)
			{
				locvar.destroy(&locvar);
				locvar.free_mem(&locvar);
				pub_log_info("[%s][%d] recover info set error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
	}

	if (link.asktype[0] == '0')
	{
		if (route.gate == 0 || route.status == 0)
		{
			pub_log_info("[%s][%d] prdt=[%s] gate=[%d] status=[%d]", 
					__FILE__, __LINE__, route.name, route.gate, route.status);
			ret = lsn_recv_deny_la(cycle, &locvar);
			if (ret != SW_OK)
			{
				locvar.destroy(&locvar);
				locvar.free_mem(&locvar);
				pub_log_error("[%s][%d] deal recv deny error!", __FILE__, __LINE__);
				mtype_delete(mtype, 1);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] prdt=[%s] gate off!", __FILE__, __LINE__, route.name);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(mtype, 1);
			return SW_OK;
		}

		cmd.mtype = mtype;
		sw_int64_t traceno = 0;
		ret = seqs_get_traceno_and_sysdate(&traceno, cmd.sys_date);
		if (ret < 0)
		{
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			pub_log_error("[%s][%d]get traceno and system date error", __FILE__, __LINE__);
			mtype_delete(cmd.mtype, 1);
			return SW_ERROR;
		}
		cmd.trace_no = traceno;
		cmd.type = SW_TASKREQ;

		link.mtype = cmd.mtype;
		link.type = cmd.type;
		link.traceno = cmd.trace_no;
		strncpy(link.sys_date, cmd.sys_date, sizeof(link.sys_date) - 1);

		/*** saveflag: 0->不保存 1->保存 ***/
		if (link.saveflag[0] == '1')
		{
			ret = lsn_save_linkinfo(cycle, &link, &cmd);
			if (ret < 0)
			{
				locvar.destroy((void *)&locvar);
				locvar.free_mem(&locvar);
				pub_log_info("[%s][%d] save link info error!", __FILE__, __LINE__);
				mtype_delete(cmd.mtype, 1);
				return SW_ERROR;
			}
		}

		if (chnl->fun.svrmap_func != NULL)
		{
			pub_log_info("[%s][%d] svrmap begin...", __FILE__, __LINE__);
			ret = chnl->fun.svrmap_func(route.cache.svrmap, &locvar, cmd.dst_svr, cmd.def_name, &cmd.level);
			if (ret < 0)
			{
				locvar.destroy(&locvar);
				locvar.free_mem(&locvar);
				pub_log_error("[%s][%d] svrsvc map error,ret=[%d]", __FILE__, __LINE__, ret);
				mtype_delete(cmd.mtype, 1);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d]run svrmap_func sucess  ", __FILE__, __LINE__);
		}

		locvar.set_string(&locvar, "$service", cmd.def_name);
		locvar.set_string(&locvar, "$server", cmd.dst_svr);
		locvar.set_string(&locvar, "$listen", cycle->lsn_conf.name);
		locvar.set_string(&locvar, "$product", route.name);
		locvar.set_string(&locvar, "$comtype", cycle->lsn_conf.comtype);

		alog_set_sysinfo(cmd.mtype, cmd.sys_date, cmd.trace_no, cycle->lsn_conf.name);

		cmd.type = SW_TASKREQ;
		cmd.ori_type = ND_LSN;
		sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);                                      
		strcpy(cmd.ori_svc, buf);                                                                                       
		strncpy(cmd.lsn_name, cycle->lsn_conf.name, sizeof(cmd.lsn_name) - 1);
		ret = trace_create_info(&cmd);                                                                                    
		if (ret < 0)                                                                                                     
		{                                                                                                                
			pub_log_error("[%s][%d] create trace info error! traceno=[%d]", 
					__FILE__, __LINE__, cmd.trace_no);                                                                  
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(cmd.mtype, 1);
			return SW_ERROR;
		}
	}

	loc_set_zd_data(&locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(&locvar, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_info("[%s][%d] pkgmap error! ret=[%d]", __FILE__, __LINE__, ret);
			locvar.destroy(&locvar);
			locvar.free_mem(&locvar);
			mtype_delete(mtype, 1);
			return SW_ERROR;
		}
	}

	memset(msgtype, 0x0, sizeof(msgtype));
	loc_get_zd_data(&locvar, "#MesgType", msgtype);
	if (msgtype[0] != '\0')
	{
		loc_set_zd_data(&locvar, "$recv_msgtype", msgtype);
		pub_log_info("[%s][%d] #MesgType=[%s]", __FILE__, __LINE__, msgtype);
	}


	if (link.asktype[0] == '0')
	{	
		cmd.dst_type = ND_SVC;
		cmd.msg_type = SW_MSG_REQ;
		cmd.task_flg = SW_STORE;
		cmd.timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
		strcpy(cmd.ori_svr, cycle->base.name.data);
		memset(buf, 0x0, sizeof(buf));
		if ((strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0) &&
				cycle->lsn_conf.comm.count > 1)
		{
			sprintf(buf, "%s_%d_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.group_index, cycle->lsn_conf.process_index);
		}
		else
		{
			sprintf(buf, "%s_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
		}
		strcpy(cmd.ori_svc, buf);
	}
	else
	{
		cmd.ori_type = ND_LSN;	
		cmd.dst_type = ND_SVC;
		cmd.msg_type = SW_MSG_RES;
		cmd.task_flg = SW_FORGET;
		strcpy(cmd.ori_svr, cycle->base.name.data);	
		memset(buf, 0x0, sizeof(buf));
		if ((strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0) &&
				cycle->lsn_conf.comm.count > 1)
		{
			sprintf(buf, "%s_%d_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.group_index, cycle->lsn_conf.process_index);
		}
		else
		{
			sprintf(buf, "%s_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
		}
		strcpy(cmd.ori_svc, buf);
	}

	if (link.asktype[0] == '0')
	{
		mtype_set_info(cmd.mtype, &cmd);
	}
	trace_insert(&locvar, &cmd, TRACE_OUT);
	ret = route_snd_dst(&locvar, cycle->base.global_path, &cmd);
	if (ret < 0)
	{
		locvar.free_mem(&locvar);
		pub_log_error("[%s][%d] send message to SVC error ret=[%d]", 
				__FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	locvar.free_mem(&locvar);
	pub_log_info("[%s][%d] send cmd to svc success, waiting response...", __FILE__, __LINE__);
	return SW_OK;
}

sw_int_t lsn_deal_send_work_la(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_cmd_t *cmd, sw_buf_t *locbuf)
{
	int	ret = 0;
	sw_char_t	key[128];
	sw_char_t	tmp[128];
	sw_char_t	pkgtype[8];
	sw_char_t	msgtype[128];
	sw_char_t	recv_msgtype[128];
	sw_link_t	link;
	sw_chnl_t	*chnl = NULL;
	sw_buf_t	tmpbuf;

	memset(key, 0x0, sizeof(key));
	memset(tmp, 0x0, sizeof(tmp));
	memset(&link, 0x0, sizeof(link));
	memset(pkgtype, 0x0, sizeof(pkgtype));
	memset(msgtype, 0x0, sizeof(msgtype));
	memset(recv_msgtype, 0x0, sizeof(recv_msgtype));

	if (cmd->type == SW_TASKDENY)
	{
		return lsn_send_deny_la(cycle, locvar, locbuf);
	}

	loc_get_zd_data(locvar, "#MesgType", msgtype);
	loc_get_zd_data(locvar, "$recv_msgtype", recv_msgtype);
	if (msgtype[0] != '\0' && strcmp(msgtype, recv_msgtype) == 0)
	{
		pub_log_info("[%s][%d] #MesgType=$recv_msgtype=[%s] Must not to send out!",
				__FILE__, __LINE__, msgtype);
		return -2;
	}

	chnl = &cycle->chnl;
	pub_log_info("[%s][%d] ori_prdt=[%s]", __FILE__, __LINE__, cmd->ori_prdt);
	memset(&cycle->route, 0x0, sizeof(sw_route_t));
	ret = lsn_get_route_by_name(cycle, cmd->ori_prdt, &cycle->route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_get_route_by_name error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	loc_get_zd_data(locvar, "$pkgtype", pkgtype);
	if (cycle->route.fun.prean_func != NULL)
	{
		ret = cycle->route.fun.prean_func(locvar, &link, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pre analyze error! ret=[%d]", __FILE__, __LINE__, ret);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] preanalyze success!", __FILE__, __LINE__);
		if (link.qname[0] != '\0')
		{
			pub_log_info("[%s][%d] qname=[%s]", __FILE__, __LINE__, link.qname);
			loc_set_zd_data(locvar, "$send_qname", link.qname);
		}
	}
	else
	{
		memset(tmp, 0x0, sizeof(tmp));
		loc_get_zd_data(locvar, "#msgtype", tmp);
		if (tmp[2] % 2 == 0)
		{
			link.asktype[0] = '0';
			link.saveflag[0] = '1';
		}
		else
		{
			link.asktype[0] = '1';
			memset(key, 0x0, sizeof(key));
			loc_get_zd_data(locvar, "#cbmkey", key);
			strncpy(link.keyinfo, key, sizeof(link.keyinfo) - 1);
			link.clearflag[0] = '1';
		}
	}

	if (link.asktype[0] != '0' && link.asktype[0] != '1')
	{
		pub_log_error("[%s][%d] asktype error! asktype=[%s]", 
				__FILE__, __LINE__, link.asktype);
		return SW_ERROR;
	}

	loc_set_zd_data(locvar, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(locvar, chnl->cache.pkgmap, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	ret = pkg_out(locvar, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, cycle->route.name);
	if (ret == SW_ERROR)
	{
		lsn_set_err(locvar, ERR_TOPKG);
		pub_log_error("[%s][%d] pkg_out error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	locbuf->len = ret;
	locbuf->data[ret] = '\0';
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] packages len=[%d]",
			__FILE__, __LINE__, locbuf->len);                                                                       

	if (pkgtype[0] == '1' && cycle->route.fun.after_func != NULL)
	{
		ret = cycle->route.fun.after_func(locvar, locbuf->data, O_SEND);
		if (ret < 0)
		{
			pub_log_info("[%s][%d] Do after deal error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}	

	link.timeout = cmd->timeout;
	link.type = cmd->type;
	link.mtype = cmd->mtype;
	link.traceno = cmd->trace_no;
	link.sendtype = O_SEND;
	strcpy(link.machine, g_eswid);
	sprintf(link.addr, "%s:%d", cycle->lsn_conf.comm.group[0].local.ip, cycle->lsn_conf.comm.group[0].local.port);
	strncpy(link.sys_date, cmd->sys_date, sizeof(link.sys_date) - 1);

	if (link.asktype[0] == '0')
	{
		if (link.saveflag[0] == '1' && cmd->type != SW_POSTLSNREQ)
		{
			if (link.keyinfo[0] == '\0')
			{
				loc_get_zd_data(locvar, "#cbmkey", key);
				strncpy(link.keyinfo, key, sizeof(link.keyinfo) - 1);
			}
			strncpy(link.lsn_name, cmd->lsn_name, sizeof(link.lsn_name) - 1);
			ret = lsn_save_linkinfo(cycle, &link, cmd);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] save link info error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
		g_trace_flag = TRACE_OUT;
	}

	if (link.asktype[0] == '1')
	{
		/*** clearflag: 0->不清空当前的链路信息 1->清空当前的链路信息 2->更新链路信息 ***/
		if (link.clearflag[0] == '2')
		{
			ret = lsn_update_linkinfo(cycle, &link);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] update linkinfo error! mtype=[%ld]",
						__FILE__, __LINE__, link.mtype);
				return 	SW_ERROR;
			}
			pub_log_info("[%s][%d] update linkinfo success! mtype=[%ld]",
					__FILE__, __LINE__, link.mtype);
		}
		else if (link.clearflag[0] == '1' && cmd->type != SW_POSTLSNREQ)
		{
			ret = lsn_delete_linkinfo(cycle, &link, DEL_LOCAL_LINK);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] delete linkinfo error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] delete linkinfo, mtype[%ld]", __FILE__, __LINE__, link.mtype);
		}
		g_trace_flag = TRACE_OVER;
	}

	if (link.msgid[0] != '\0')
	{
		loc_set_zd_data(locvar, SENDMSGID, link.msgid);
	}

	if (link.corrid[0] != '\0')
	{
		loc_set_zd_data(locvar, SENDCORRID, link.corrid);
	}

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

	if (link.asktype[0] == '0')
	{
	pub_buf_init(&tmpbuf);
	ret = locvar->serialize(locvar, &tmpbuf);
	if (ret <= 0)
	{
		pub_buf_clear(&tmpbuf);
		pub_log_error("[%s][%d]shm_vars_serialize error!", __FILE__, __LINE__);
			return SW_ERROR;
	}
	pub_buf_clear(&tmpbuf);
	}
	pub_log_info("[%s][%d] deal out task success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_deal_linknull(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd)
{
	sw_link_t link;
	sw_int_t	ret = 0;

	if (cycle == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] deal_linknull param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (cmd->mtype <= 0)
	{
		pub_log_error("[%s][%d] mtype error! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);
		return SW_ERROR;
	}

	memset(&link, 0x00, sizeof(sw_link_t));
	link.mtype = cmd->mtype;
	ret = lsn_delete_linkinfo(cycle, &link, DEL_NULL_LINK);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] delete linkinfo error! mtype=[%ld]",
				__FILE__, __LINE__, cmd->mtype);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t lsn_deal_send_timeout_la(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = 0;
	sw_int32_t	i = 0;
	sw_int32_t	timeout = 0;
	sw_char_t	buf[64];
	sw_time_t	now = -1;
	sw_cmd_t	cmd;
	sw_loc_vars_t	vars;

	memset(buf, 0x0, sizeof(buf));
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	now = (sw_int_t)time(NULL);
	for (i = 0; i < cycle->link_list->head.link_cnt; i++)
	{
		/*** 只处理往账请求 ***//*change by xuehui use==2 正在进行超时处理*/
		if (cycle->link_list->list[i].info.sendtype != O_SEND || cycle->link_list->list[i].use == 0 || cycle->link_list->list[i].use == 2)
		{
			continue;
		}

		timeout = cycle->link_list->list[i].info.timeout;
		if (timeout == 0)
		{
			timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
		}

		if (cycle->link_list->list[i].starttime > 0 && now - cycle->link_list->list[i].starttime > timeout)
		{
			if (cycle->link_list->list[i].info.type == SW_CALLLSNREQ)
			{
				/*先修改use状态为2，告诉其他进程正在处理超时，避免MQ等侦听重复工作*/
				cycle->link_list->list[i].use = 2;
				pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%ld] trcno=[%lld] keyinfo=[%s]",
						__FILE__, __LINE__, cycle->link_list->list[i].info.mtype,
						cycle->link_list->list[i].info.traceno, 
						cycle->link_list->list[i].info.keyinfo);
				ret = pub_loc_vars_alloc(&vars, SHM_VARS);
				if (ret == SW_ERROR)
				{
					pub_log_error("[%s][%d] pub_loc_vars_alloc error!",
							__FILE__, __LINE__);
					continue;
				}

				memset(buf, 0x0, sizeof(buf));
				sprintf(buf, "shm%08ld", cycle->link_list->list[i].info.mtype);
				ret = vars.unserialize(&vars, buf);
				if (ret == SW_ERROR)
				{
					vars.free_mem(&vars);
					pub_log_error("[%s][%d] vars unserialize error! mtype=[%ld]",
							__FILE__, __LINE__, cycle->link_list->list[i].info.mtype);
					continue;
				}
				lsn_set_err(&vars, ERR_LSNOUT);
				memset(&cmd, 0x00, sizeof (cmd));
				memcpy(&cmd, &cycle->link_list->list[i].cmd, sizeof(sw_cmd_t));
				char	chnl[128];
				memset(chnl, 0x0, sizeof(chnl));
				loc_get_zd_data(&vars, "$listen", chnl);
				alog_set_sysinfo(cycle->link_list->list[i].info.mtype, cmd.sys_date, cmd.trace_no, chnl);
				lsn_deal_err(cycle, &vars, &cmd);
				vars.free_mem(&vars);
				ret = lsn_delete_linkinfo(cycle, &(cycle->link_list->list[i].info), DEL_ALL_LINK);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] delete linkinfo error! mtype=[%ld]",
							__FILE__, __LINE__, cycle->link_list->list[i].info.mtype);
				}
				pub_log_info("[%s][%d] delete linkinfo success! mtype=[%ld]",
						__FILE__, __LINE__, cycle->link_list->list[i].info.mtype);
				pub_log_bend("[%s][%d]lsn_deal_send_timeout_la.", __FILE__, __LINE__);
			}
		}
	}
	return SW_OK;
}

sw_int_t lsn_deal_recv_timeout_la(sw_lsn_cycle_t *cycle)
{
	sw_int32_t	i = 0;
	sw_int32_t	timeout = 0;
	sw_time_t	now = -1;
	sw_int_t	mtype = 0;

	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
	now = (sw_int_t)time(NULL);
	for (i = 0; i < cycle->link_list->head.link_cnt; i++)
	{
		/*** 只处理来账请求 ***//*change by xuehui use==2 正在进行超时处理*/
		if (cycle->link_list->list[i].info.sendtype != O_RECV || cycle->link_list->list[i].use == 0 || cycle->link_list->list[i].use == 2)
		{
			continue;
		}

		/*** 如果超过链路超时时间,直接删除链路信息 ***/
		if (now - cycle->link_list->list[i].starttime > timeout)
		{    
			/*先修改use状态为2，告诉其他进程正在处理超时，避免MQ等侦听重复工作*/
			cycle->link_list->list[i].use = 2; 
			mtype = cycle->link_list->list[i].info.mtype;
			pub_log_info("[%s][%d] DELETE TIMEOUT MTYPE! mtype=[%ld]",
					__FILE__, __LINE__, mtype);
			alog_set_sysinfo(mtype, cycle->link_list->list[i].cmd.sys_date, 
					cycle->link_list->list[i].cmd.trace_no, cycle->link_list->list[i].cmd.lsn_name);
			pub_vars_destroy_by_mtype(mtype);
			lsn_delete_linkinfo(cycle, &(cycle->link_list->list[i].info), DEL_LOCAL_LINK);
			mtype_delete(mtype, 1);
			continue;
		}
	}

	return 0;
}

sw_int_t lsn_get_route(sw_lsn_cycle_t *cycle, sw_loc_vars_t *loc_vars, sw_buf_t *buf, sw_route_t *route_info, sw_int_t flag)
{
	sw_int_t	i = 0;
	sw_int_t	j = 0;
	sw_int_t	route_cnt = 0;
	sw_char_t	value[256];
	sw_route_t	*route = NULL;
	sw_check_t	*check = NULL;

	if (cycle == NULL || buf == NULL || route_info == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(value, 0x0, sizeof(value));
	route_cnt = cycle->routes->head.route_cnt;
	for (i = 0; i < route_cnt; i++)
	{
		route = &cycle->routes->route[i];
		for (j = 0; j < route->an.check_cnt; j++)
		{
			memset(value, 0x0, sizeof(value));
			check = &route->an.check[j];
			if (check->begin >= 0 && check->length > 0)
			{
				strncpy(value, buf->data + check->begin, check->length);
				pub_log_info("[%s][%d] check: value=[%s] check_value=[%s]",
						__FILE__, __LINE__, value, check->value);
				if (pub_regex_match(value, check->value) != 0 && pub_str_include(value, check->value) != 0)
				{
					break;
				}
			}

			if (check->name[0] != '\0')
			{
				memset(value, 0x0, sizeof(value));
				loc_get_zd_data(loc_vars, check->name, value);
				pub_log_info("[%s][%d] check: name=[%s]=[%s] check_value=[%s]",
						__FILE__, __LINE__, check->name, value, check->value);
				if (pub_regex_match(value, check->value) != 0 && pub_str_include(value, check->value) != 0)
				{
					break;
				}
			}
		}
		if (j == route->an.check_cnt)
		{
			if (SW_OK != mtype_limit_check(route->name, cycle->lsn_conf.name))
			{
				return SW_ERROR;
			}
			/*** ALL CHECK PASSED  ***/
			memcpy(route_info, route, sizeof(sw_route_t));
			return SW_OK;
		}
	}
	pub_log_error("[%s][%d] Not find route info!", __FILE__, __LINE__);

	return SW_ERROR;
}

sw_int_t lsn_get_route_by_name(sw_lsn_cycle_t *cycle, sw_char_t *prdt, sw_route_t *route)
{
	sw_int_t	i = 0;

	if (cycle == NULL || prdt == NULL || prdt[0] == '\0' || route == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, prdt) == 0)
		{
			memcpy(route, &cycle->routes->route[i], sizeof(sw_route_t));
			return SW_OK;
		}
	}
	pub_log_error("[%s][%d] Can not find [%s] prdt info!", __FILE__, __LINE__, prdt);
	return SW_ERROR;
}

int lsn_set_err(sw_loc_vars_t *loc_vars, int errcode)
{
	char	err[8];

	memset(err, 0x0, sizeof(err));
	sprintf(err, "%04d", errcode);
	loc_set_zd_data(loc_vars, "$errcode", err);
	pub_log_info("[%s][%d] errcode=[%d]", __FILE__, __LINE__, errcode);

	return 0;
}

int lsn_get_err(sw_loc_vars_t *loc_vars, char *errcode)
{
	loc_get_zd_data(loc_vars, "$errcode", errcode);
	pub_log_info("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);

	return 0;
}

sw_int_t lsn_pub_deal_out_task(cycle, loc_vars, cmd, locbuf)
	sw_lsn_cycle_t *cycle;
	sw_loc_vars_t *loc_vars;
	sw_cmd_t *cmd;
	sw_buf_t *locbuf;
{
	sw_int_t	ret = 0;
	sw_chnl_t	*chnl = NULL;
	sw_buf_t	tmpbuf;

	chnl = &cycle->chnl;
	memset(locbuf->data, 0x0, locbuf->size);
	memset(&cycle->route, 0x0, sizeof(sw_route_t));
	ret = lsn_get_route_by_name(cycle, cmd->ori_prdt, &cycle->route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get route info error! prdt=[%s]", 
				__FILE__, __LINE__, cmd->ori_prdt);
		return SW_ERROR;
	}

	loc_set_zd_data(loc_vars, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(loc_vars, chnl->cache.pkgmap, O_SEND);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}	

	ret = pkg_out(loc_vars, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, cycle->route.name);
	if (ret <= 0)
	{
		lsn_set_err(loc_vars, ERR_TOPKG);
		pub_log_error("[%s][%d] pack message error! len=[%d]", 
				__FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	locbuf->len = ret;
	locbuf->data[ret] = '\0';
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] packages len=[%d]", 
			__FILE__, __LINE__, locbuf->len);
	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Enc begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(loc_vars, locbuf, SW_ENC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Enc error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After enc: len=[%d]",
				__FILE__, __LINE__, locbuf->len);
	}

	pub_buf_init(&tmpbuf);
	ret = loc_vars->serialize(loc_vars, &tmpbuf);
	if (ret <= 0)
	{
		pub_buf_clear(&tmpbuf);
		pub_log_error("[%s][%d]shm_vars_serialize error!", __FILE__, __LINE__);
		return -1;
	}
	pub_buf_clear(&tmpbuf);
	pub_log_info("[%s][%d] lsn_pub_deal_task success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_pub_deal_in_task(cycle, loc_vars, cmd, locbuf)
	sw_lsn_cycle_t *cycle;
	sw_loc_vars_t *loc_vars;
	sw_cmd_t *cmd;
	sw_buf_t *locbuf;
{
	sw_int_t	ret = 0;
	sw_char_t	tmp[128];
	sw_chnl_t	*chnl = NULL;

	alog_set_sysinfo(cmd->mtype, cmd->sys_date, cmd->trace_no, NULL);

	memset(tmp, 0x0, sizeof(tmp));

	chnl = &cycle->chnl;
	if (cycle->handler.des_handler != NULL)
	{
		pub_log_info("[%s][%d] Dec begin...", __FILE__, __LINE__);
		ret = cycle->handler.des_handler(loc_vars, locbuf, SW_DEC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Dec error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After dec: len=[%d]",
				__FILE__, __LINE__, locbuf->len);
	}

	ret = lsn_get_route_by_name(cycle, cmd->dst_prdt, &cycle->route);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get [%s] route info error!", __FILE__, __LINE__, cmd->dst_prdt);
		return SW_ERROR;
	}

	ret = pkg_in(loc_vars, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, cycle->route.name);
	if (ret != SW_OK)
	{
		lsn_set_err(loc_vars, ERR_PKGTOS);
		pub_log_error("[%s][%d] unpack error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	loc_set_zd_data(loc_vars, "$current_lsn", cycle->lsn_conf.name);
	if (chnl->fun.pkgmap_func != NULL)
	{
		ret = chnl->fun.pkgmap_func(loc_vars, chnl->cache.pkgmap, O_RECV);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pkgmap error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	if (chnl->fun.file_func != NULL)
	{
		ret = chnl->fun.file_func(loc_vars, 1, cycle->accept_fd, cycle->param);
		if (ret != SW_OK)
		{
			lsn_set_err(loc_vars, ERR_RCVFILE);
			pub_log_error("[%s][%d] do file func error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	ret = pkg_clear_vars(loc_vars);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Clear vars error!", __FILE__, __LINE__);
	}

	if (cmd->type == SW_LINKLSNREQ)	
	{
		pub_log_info("[%s][%d] LINKLSNREQ! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);
		mtype_delete(cmd->mtype, 0);
		return SW_OK;
	}

	if (cmd->type == SW_POSTLSNREQ)	
	{
		pub_log_info("[%s][%d] POSTLSNREQ! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);
		mtype_delete(cmd->mtype, 0);
		return SW_OK;
	}



	switch (cmd->type)	
	{
		case SW_CALLLSNREQ:
			cmd->type = SW_CALLLSNRES;
			break;
		case SW_LINKLSNREQ:
			cmd->type = SW_LINKLSNRES;
			break;
		case SW_POSTLSNREQ:
			cmd->type = SW_POSTLSNRES;
			break;
		default:
			cmd->type = SW_POSTLSNRES;
			break;
	}
	cmd->ori_type = ND_LSN;
	cmd->dst_type = ND_SVC;
	cmd->msg_type = SW_MSG_RES;
	cmd->task_flg = SW_FORGET;
	strcpy(cmd->ori_svr, cycle->base.name.data);
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	strcpy(cmd->ori_svc, tmp);
	pub_log_debug("[%s][%d] cmd->lsn_name=[%s]", __FILE__, __LINE__, cmd->lsn_name);
	ret = route_snd_dst(loc_vars, cycle->base.global_path, cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] route_snd_dst error! type=[%d] mtype=[%ld]",
				__FILE__, __LINE__, cmd->type, cmd->mtype);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Send msg to svc success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_deal_err(sw_lsn_cycle_t *cycle, sw_loc_vars_t *loc_vars, sw_cmd_t *cmd)
{
	sw_int_t	ret = 0;
	sw_char_t	tmp[64];

	memset(tmp, 0x0, sizeof(tmp));
	lsn_get_err(loc_vars, tmp);
	pub_log_info("[%s][%d] errcode=[%s]", __FILE__, __LINE__, tmp);
	if (strlen(tmp) == 0 || strcmp(tmp, "0000") == 0)
	{
		lsn_set_err(loc_vars, ERREVERY);
	}

	if (cmd->type == SW_CALLLSNREQ)
	{
		cmd->type = SW_CALLLSNRES;
	}	
	else if (cmd->type == SW_LINKLSNREQ)		
	{
		cmd->type = SW_LINKLSNRES;
	}	
	else if (cmd->type == SW_POSTLSNREQ)
	{
		cmd->type = SW_POSTLSNRES;
	}
	else
	{
		cmd->type = SW_ERRCMD;
	}
	cmd->ori_type = ND_LSN;
	cmd->dst_type = ND_SVC;
	cmd->msg_type = SW_MSG_RES;
	cmd->task_flg = SW_FORGET;
	strncpy(cmd->ori_svr, cycle->base.name.data, sizeof(cmd->ori_svr) - 1);
	sprintf(tmp, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	/*** MQ OR TLQ ***/
	if (strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0)
	{
		memset(tmp, 0x0, sizeof(tmp));
		if (cycle->lsn_conf.comm.count > 1)
		{
			sprintf(tmp, "%s_%d_%d_snd", cycle->lsn_conf.name, cycle->lsn_conf.group_index, cycle->lsn_conf.process_index);
		}
		else
		{
			sprintf(tmp, "%s_%d_snd", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
		}
	}
	else if (strcmp(cycle->lsn_conf.comtype, "TCPLA") == 0)
	{
		memset(tmp, 0x0, sizeof(tmp));
		sprintf(tmp, "%s_%d_snd", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
	}
	strncpy(cmd->ori_svc, tmp, sizeof(cmd->ori_svc) - 1);
	ret = route_snd_dst(loc_vars, cycle->base.global_path, cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] route_snd_dst error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] route_snd_dst success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_save_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, sw_cmd_t *cmd)
{
	int	i = 0;
	int	ret = 0;
	sw_link_node_t	*linkp = NULL;

	if (cycle == NULL || link == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] input parameters can not be null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (link->keyinfo[0] == '\0')
	{
		pub_log_error("[%s][%d] keyinfo can not be NULL!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*来帐请求，链路信息保存在RES/DB + 本地*/
	if (link->sendtype == O_SEND && link->asktype[0] == '0' && strcmp(cycle->lsn_conf.comtype, "TCPLA") != 0)
	{ 
		if (g_mq_use_db)
		{
			pub_log_debug("[%s][%d] save linkinfo in DB", __FILE__, __LINE__);
			strncpy(link->lsn_name, cycle->lsn_conf.name, sizeof(link->lsn_name) - 1);
			ret = slink_save_linkinfo(cycle, link, cmd);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] sql save linkinfo error.", __FILE__, __LINE__);
				return SW_ERROR;
			}

			return SW_OK;	
		}

		if (g_link_in_ares)
		{
			pub_log_debug("[%s][%d] save linkinfo in RES", __FILE__, __LINE__);
			alink_save_linkinfo(cycle->lsn_conf.sendtype, link, cmd);
		}
	}

	pub_log_debug("[%s][%d] save linkinfo in LOCAL", __FILE__, __LINE__);
	sem_write_lock(cycle->semid);
	for (i = cycle->link_list->head.cur_pos; i < cycle->link_list->head.link_cnt; i++)
	{
		if (cycle->link_list->list[i].use == 0)
		{
			cycle->link_list->list[i].use = 1;
			break;
		}
		if (cycle->link_list->list[i].use == 1 
				&& cycle->link_list->list[i].info.mtype == cmd->mtype)
		{
			pub_log_error("[%s][%d]link[%d]mtype=[%d]traceno=[%lld]date=[%s]reused.",
					__FILE__, __LINE__, i, cycle->link_list->list[i].info.mtype, 
					cycle->link_list->list[i].info.traceno, cycle->link_list->list[i].info.sys_date);
			cycle->link_list->list[i].use = 1;
			break;
		}
	}

	if (i == cycle->link_list->head.link_cnt)
	{
		for (i = 0; i < cycle->link_list->head.cur_pos; i++)
		{
			if (cycle->link_list->list[i].use == 0)
			{
				cycle->link_list->list[i].use = 1;
				break;
			}
			if (cycle->link_list->list[i].use == 1 
					&& cycle->link_list->list[i].info.mtype == cmd->mtype)
			{
				pub_log_error("[%s][%d]link[%d]mtype=[%d]traceno=[%lld]date=[%s]reused.",
						__FILE__, __LINE__, i, cycle->link_list->list[i].info.mtype, 
						cycle->link_list->list[i].info.traceno, cycle->link_list->list[i].info.sys_date);
				cycle->link_list->list[i].use = 1;
				break;
			}
		}
		if (i == cycle->link_list->head.cur_pos)
		{
			sem_write_unlock(cycle->semid);
			pub_log_error("[%s][%d] No enough space! link_cnt=[%d] cur_pos=[%d]",
					__FILE__, __LINE__, cycle->link_list->head.link_cnt, cycle->link_list->head.cur_pos);
			return SW_ERROR;
		}
	}
	cycle->link_list->head.cur_pos = i + 1;
	link->sendtype = cycle->lsn_conf.sendtype;
	memset(&cycle->link_list->list[i].info, 0x0, sizeof(sw_link_t));
	linkp = cycle->link_list->list + i;
	linkp->starttime = (long)time(NULL);
	memcpy(&linkp->info, link, sizeof(sw_link_t));
	memcpy(&linkp->cmd, cmd, sizeof(sw_cmd_t));
	sem_write_unlock(cycle->semid);
	pub_log_info("[%s][%d] save linkinfo success! index=[%d] mtype=[%ld] key=[%s]", 
			__FILE__, __LINE__, i, link->mtype, link->keyinfo);

	return SW_OK;
}

sw_int_t lsn_load_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link)
{
	int i = 0;
	int ret = 0;

	if (strcmp(cycle->lsn_conf.comtype, "TCPLA") != 0)
	{
		if (g_mq_use_db)
		{
			ret = slink_load_linkinfo_by_key(cycle, link);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] sql load_linkinfo_by_key error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_debug("[%s][%d] sql load linkinfo by key ok.", __FILE__, __LINE__);
			return SW_OK;
		}

		if (g_link_in_ares)
		{
			ret = alink_load_linkinfo_by_key(cycle->lsn_conf.sendtype, link);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] alink load linkinfo by key error.", __FILE__, __LINE__);
				return SW_ERROR;
			}

			pub_log_debug("[%s][%d] ares load linkinfo by key ok.", __FILE__, __LINE__);
			return SW_OK;
		}
	}

	for (i = 0; i < cycle->link_list->head.link_cnt; i++)
	{
		if (cycle->link_list->list[i].use < 1)
		{
			continue;
		}
		if (strcmp(cycle->link_list->list[i].info.keyinfo, link->keyinfo) == 0 &&
				cycle->link_list->list[i].info.sendtype != cycle->lsn_conf.sendtype)
		{
			memset(link->infoset, 0x0, sizeof(link->infoset));
			strcpy(link->infoset, cycle->link_list->list[i].info.infoset);
			link->mtype = cycle->link_list->list[i].info.mtype;
			link->type = cycle->link_list->list[i].info.type;
			link->traceno = cycle->link_list->list[i].info.traceno;
			memcpy(link->sys_date, cycle->link_list->list[i].info.sys_date, sizeof(link->sys_date) - 1);
			break;
		}
	}

	if (i == cycle->link_list->head.link_cnt)
	{
		pub_log_error("[%s][%d] get original info error! keyinfo=[%s]",
				__FILE__, __LINE__, link->keyinfo);
		return SW_ERROR;
	}

	return SW_OK;
}


sw_int_t lsn_delete_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, int flag)
{
	int i = 0;
	int ret = 0;

	if (strcmp(cycle->lsn_conf.comtype, "TCPLA") != 0 && (flag ==  DEL_REMOTE_LINK || flag == DEL_ALL_LINK))
	{
		if (g_mq_use_db)
		{
			pub_log_debug("[%s][%d] delete linkinfo in DB", __FILE__, __LINE__);
			ret = slink_delete_linkinfo(cycle, link);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] sql delete_linkinfo_by_mtype_and_key error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_debug("[%s][%d] sql delete link info ok.", __FILE__, __LINE__);
			return SW_OK;
		}

		if (g_link_in_ares)
		{
			pub_log_debug("[%s][%d] delete linkinfo in RES", __FILE__, __LINE__);
			ret = alink_delete_linkinfo(link);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] delete remote link info error,mtype=[%ld]keyinfo[%s].",
						__FILE__, __LINE__,link->mtype, link->keyinfo);
			}

			if (flag == DEL_REMOTE_LINK)
			{
				pub_log_debug("[%s][%d] cross machine:only delete linkinfo in RES", __FILE__, __LINE__);
				return SW_OK;
			}
		}
	}

	/*删除本地链路信息*/
	pub_log_debug("[%s][%d] delete linkinfo in LOCAL", __FILE__, __LINE__);
	sem_write_lock(cycle->semid);
	for (i = 0; i < cycle->link_list->head.link_cnt; i++)
	{
		if (cycle->link_list->list[i].use < 1)
		{
			continue;
		}

		if ((cycle->link_list->list[i].info.mtype == link->mtype) 
				&& (flag == DEL_NULL_LINK || strcmp(cycle->link_list->list[i].info.keyinfo, link->keyinfo) == 0))
		{
			memset(cycle->link_list->list + i, 0x00, sizeof(sw_link_node_t));
			break;
		}
	}
	sem_write_unlock(cycle->semid);

	if (i == cycle->link_list->head.link_cnt)
	{
		pub_log_info("[%s][%d] delete linkinfo error! mtype=[%ld]", __FILE__, __LINE__, link->mtype);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] delete linkinfo success! mtype=[%ld]", __FILE__, __LINE__, link->mtype);

	return SW_OK;
}

sw_int_t lsn_update_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link)
{
	int	i = 0;

	sem_write_lock(cycle->semid);
	for (i = 0; i < cycle->link_list->head.link_cnt; i++)
	{
		if (cycle->link_list->list[i].use < 1)
		{
			continue;
		}

		if (cycle->link_list->list[i].info.mtype == link->mtype)
		{
			strncpy(cycle->link_list->list[i].info.keyinfo, link->keyinfo, sizeof(cycle->link_list->list[i].info.keyinfo) - 1);
			break;
		}
	}

	sem_write_unlock(cycle->semid);

	if (i == cycle->link_list->head.link_cnt)
	{
		pub_log_error("[%s][%d] update linkinfo error! mtype=[%d]",__FILE__, __LINE__, link->mtype);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] update linkinfo success! mtype=[%d]", __FILE__, __LINE__, link->mtype);

	return SW_OK;
}

sw_int_t lsn_print_linkinfo(sw_link_t *link)
{
	pub_log_info("asktype=[%s] mtype=[%d] traceno=[%d] type=[%d] pkgtype=[%s] synflag=[%s] "
			"infoflag=[%s] clearflag=[%s] keyinfo=[%s]", link->asktype, link->mtype, link->traceno, link->type, 
			link->pkgtype, link->synflag, link->infoflag, link->clearflag, link->keyinfo);

	return SW_OK;
}

sw_int_t lsn_recv_deny_la(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars)
{
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_chnl_t	*chnl = NULL;

	memset(&cmd, 0x0, sizeof(cmd));

	chnl = &cycle->chnl;
	if (chnl->fun.deny_func == NULL)
	{
		pub_log_error("[%s][%d] deny fun is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cmd.type = SW_TASKDENY;
	cmd.ori_type = ND_LSN;
	cmd.dst_type = ND_LSN;
	cmd.msg_type = SW_MSG_REQ;
	cmd.task_flg = SW_FORGET;
	strcpy(cmd.ori_svr, cycle->base.name.data);
	sprintf(cmd.dst_svr, "%s_%s", PROC_NAME_LSN, cycle->lsn_conf.name);
	ret = route_snd_dst(vars, cycle->base.global_path, &cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] send cmd error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	return SW_OK;
}

sw_int_t lsn_send_deny_la(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars, sw_buf_t *locbuf)
{
	int	ret = 0;
	sw_chnl_t       *chnl = NULL;

	chnl = &cycle->chnl;
	if (chnl->fun.deny_func != NULL)
	{
		ret = chnl->fun.deny_func(vars, locbuf, chnl->cache.deny);
		if (ret <0)
		{
			pub_log_info("[%s][%d] denyservice error ", __FILE__, __LINE__);
		}
		else
		{
			pub_log_info("[%s][%d] denyservice sucess ", __FILE__, __LINE__);
		}
		ret = pkg_out(vars, locbuf, chnl->cache.pkgdeal, &cycle->cache_buf, NULL);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] pack out error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		if (cycle->handler.des_handler != NULL)
		{
			pub_log_info("[%s][%d] Enc begin...", __FILE__, __LINE__);
			ret = cycle->handler.des_handler(vars, locbuf, SW_ENC);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Enc error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] After enc: len=[%d]",
					__FILE__, __LINE__, locbuf->len);
		}
	}
	return SW_OK;
}

