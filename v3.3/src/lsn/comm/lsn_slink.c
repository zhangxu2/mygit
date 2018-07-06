#include "pub_db.h"
#include "pub_cfg.h"
#include "lsn_pub.h"
#include "lsn_slink.h"

static  sw_int_t mq_db_detect(sw_lsn_cycle_t *cycle);
sw_int_t slink_save_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, sw_cmd_t *cmd);
sw_int_t slink_delete_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link);
sw_int_t slink_load_linkinfo_by_key(sw_lsn_cycle_t *cycle, sw_link_t *link);

sw_int_t slink_deal_send_timeout_la(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;
	int	cols = 0;
	int	rows = 0;
	int	timeout = 0;
	long	now = -1;
	long	starttime = 0;
	char	*ptr = NULL;
	char	buf[64];
	char	name[128];
	char	sql[256];
	sw_cmd_t	cmd;
	sw_link_t	link;
	sw_loc_vars_t	vars;

	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = mq_db_detect(cycle);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] slink_deal_send_timeout_la detect error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "select * from mq_linkinfo where machine='%s' and l_lsn_name='%s'", g_eswid, cycle->lsn_conf.name);
	cols = pub_db_mquery(MQ_SQL_NAME, sql, 1000);
	if (cols <= 0)
	{
		pub_log_error("[%s][%d] select * from mq_linkinfo error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	now = (long)time(NULL);
	while (1)
	{
		rows = pub_db_mfetch(MQ_SQL_NAME);
		if (rows == 0)
		{
			break;
		}
		else if (rows == -1)
		{
			pub_log_error("[%s][%d] mfetch error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		for (i = 0; i < rows; i++)
		{
			memset(&cmd, 0x00, sizeof(cmd));
			memset(&link, 0x00, sizeof(link));
			for (j = 0; j < cols; j++)
			{
				memset(name, 0x0, sizeof(name));
				ptr = pub_db_get_data_and_name(MQ_SQL_NAME, i + 1, j + 1, name, sizeof(name));
				if (pub_str_strcasecmp(name, "sendtype") == 0)
				{
					link.sendtype = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "l_mtype") == 0)
				{
					link.mtype = atol(ptr);
				}

				if (pub_str_strcasecmp(name, "starttime") == 0)
				{
					starttime = atol(ptr);
				}

				if (pub_str_strcasecmp(name, "l_timeout") == 0)
				{
					link.timeout = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "l_type") == 0)
				{
					link.type = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "keyinfo") == 0)
				{
					strncpy(link.keyinfo, ptr, sizeof(link.keyinfo) - 1);
					pub_str_trim(link.keyinfo);
				}

				if (pub_str_strcasecmp(name, "l_traceno") == 0)
				{
					link.traceno = atoll(ptr);
				}

				if (pub_str_strcasecmp(name, "c_traceno") == 0)
				{
					cmd.trace_no = atoll(ptr);
				}

				if (pub_str_strcasecmp(name, "c_mtype") == 0)
				{
					cmd.mtype = atol(ptr);
				}

				if (pub_str_strcasecmp(name, "c_timeout") == 0)
				{
					cmd.timeout = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "start_line") == 0)
				{
					cmd.start_line = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "dst_type") == 0)
				{
					cmd.dst_type = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "ori_type") == 0)
				{
					cmd.ori_type = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "msg_type") == 0)
				{
					cmd.msg_type = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "task_flg") == 0)
				{
					cmd.task_flg = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "c_type") == 0)
				{
					cmd.type = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "c_level") == 0)
				{
					cmd.level = atoi(ptr);
				}

				if (pub_str_strcasecmp(name, "ori_def") == 0)
				{
					strncpy(cmd.ori_def, ptr, sizeof(cmd.ori_def) - 1);
					pub_str_trim(cmd.ori_def);
				}

				if (pub_str_strcasecmp(name, "c_lsn_name") == 0)
				{
					strncpy(cmd.lsn_name, ptr, sizeof(cmd.lsn_name) - 1);
					pub_str_trim(cmd.lsn_name);
				}

				if (pub_str_strcasecmp(name, "def_name") == 0)
				{
					strncpy(cmd.def_name, ptr, sizeof(cmd.def_name) - 1);
					pub_str_trim(cmd.def_name);
				}

				if (pub_str_strcasecmp(name, "dst_prdt") == 0)
				{
					strncpy(cmd.dst_prdt, ptr, sizeof(cmd.dst_prdt) - 1);
					pub_str_trim(cmd.dst_prdt);
				}

				if (pub_str_strcasecmp(name, "dst_svr") == 0)
				{
					strncpy(cmd.dst_svr, ptr, sizeof(cmd.dst_svr) - 1);
					pub_str_trim(cmd.dst_svr);
				}

				if (pub_str_strcasecmp(name, "dst_svc") == 0)
				{
					strncpy(cmd.dst_svc, ptr, sizeof(cmd.dst_svc) - 1);
					pub_str_trim(cmd.dst_svc);
				}

				if (pub_str_strcasecmp(name, "ori_prdt") == 0)
				{
					strncpy(cmd.ori_prdt, ptr, sizeof(cmd.ori_prdt) - 1);
					pub_str_trim(cmd.ori_prdt);
				}

				if (pub_str_strcasecmp(name, "ori_svr") == 0)
				{
					strncpy(cmd.ori_svr, ptr, sizeof(cmd.ori_svr) - 1);
					pub_str_trim(cmd.ori_svr);
				}

				if (pub_str_strcasecmp(name, "ori_svc") == 0)
				{
					strncpy(cmd.ori_svc, ptr, sizeof(cmd.ori_svc) - 1);
					pub_str_trim(cmd.ori_svc);
				}

				if (pub_str_strcasecmp(name, "c_sys_date") == 0)
				{
					strncpy(cmd.sys_date, ptr, sizeof(cmd.sys_date) - 1);
					pub_str_trim(cmd.sys_date);
				}

				if (pub_str_strcasecmp(name, "udp_name") == 0)
				{
					strncpy(cmd.udp_name, ptr, sizeof(cmd.udp_name) - 1);
					pub_str_trim(cmd.udp_name);
				}
			}

			if (link.sendtype != O_SEND)
			{
				continue;
			}

			timeout = link.timeout;
			if (timeout == 0)
			{
				timeout = cycle->lsn_conf.timeout > 0 ? cycle->lsn_conf.timeout : SW_LSN_TIMEOUT;
			}

			if (starttime > 0 && now - starttime > timeout)
			{
				if (link.type == SW_CALLLSNREQ)
				{
					pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%ld] traceno=[%lld]",__FILE__, __LINE__, link.mtype, link.traceno);
					ret = pub_loc_vars_alloc(&vars, SHM_VARS);
					if (ret == SW_ERROR)
					{
						pub_log_error("[%s][%d] pub_loc_vars_alloc error!",__FILE__, __LINE__);
						continue;
					}

					memset(buf, 0x0, sizeof(buf));
					sprintf(buf, "shm%08ld", link.mtype);
					ret = vars.unserialize(&vars, buf);
					if (ret == SW_ERROR)
					{
						vars.free_mem(&vars);
						pub_log_error("[%s][%d] vars unserialize error! mtype=[%ld]",__FILE__, __LINE__, link.mtype);
						continue;
					}

					lsn_set_err(&vars, ERR_LSNOUT);
					alog_set_sysinfo(link.mtype, cmd.sys_date, cmd.trace_no, cmd.lsn_name);
					lsn_deal_err(cycle, &vars, &cmd);
					vars.free_mem(&vars);
					ret = slink_delete_linkinfo(cycle, &link);
					if (ret != SW_OK)
					{
						pub_log_error("[%s][%d] delete linkinfo error! mtype=[%ld]keyinfo=[%s]traceno[%lld]",
								__FILE__, __LINE__, link.mtype, link.keyinfo, link.traceno);
					}
					pub_log_bend("[%s][%d]deal lsn_slink send timeout.", __FILE__, __LINE__);
				}
			}


		}
	}
	return SW_OK;
}

sw_int_t slink_save_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, sw_cmd_t *cmd)
{
	int ret = 0;
	int	index = 0;
	long starttime = 0;
	char buf[256];
	char sql[1024];
	char cols[1204];
	char values[1024];

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

	ret = mq_db_detect(cycle);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] slink_save_linkinfo detect error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	link->sendtype = cycle->lsn_conf.sendtype;
	starttime = (long)time(NULL);
	memset(cols, 0x00, sizeof(cols));
	memset(values, 0x00, sizeof(values));
	if (link->mtype > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%ld", link->mtype);
		strcat(values, buf);
		strcat(cols, "l_mtype");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->sendtype >= 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", link->sendtype);
		strcat(values, buf);
		strcat(cols, "sendtype");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->type >= 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", link->type);
		strcat(values, buf);
		strcat(cols, "l_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->timeout >= 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", link->timeout);
		strcat(values, buf);
		strcat(cols, "l_timeout");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->notsend >= 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", link->notsend);
		strcat(values, buf);
		strcat(cols, "notsend");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->asktype[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->asktype);
		strcat(values, buf);
		strcat(cols, "asktype");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->keyinfo[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->keyinfo);
		strcat(values, buf);
		strcat(cols, "keyinfo");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->infoset[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->infoset);
		strcat(values, buf);
		strcat(cols, "infoset");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->pkgtype[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->pkgtype);
		strcat(values, buf);
		strcat(cols, "pkgtype");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->synflag[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->synflag);
		strcat(values, buf);
		strcat(cols, "synflag");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->infoflag[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->infoflag);
		strcat(values, buf);
		strcat(cols, "infoflag");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->saveflag[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->saveflag);
		strcat(values, buf);
		strcat(cols, "saveflag");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->clearflag[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->clearflag);
		strcat(values, buf);
		strcat(cols, "clearflag");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->resflag[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->resflag);
		strcat(values, buf);
		strcat(cols, "resflag");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->qname[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->qname);
		strcat(values, buf);
		strcat(cols, "qname");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->msgid[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->msgid);
		strcat(values, buf);
		strcat(cols, "msgid");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->corrid[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->corrid);
		strcat(values, buf);
		strcat(cols, "corrid");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->traceno > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%lld", link->traceno);
		strcat(values, buf);
		strcat(cols, "l_traceno");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->sys_date[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->sys_date);
		strcat(values, buf);
		strcat(cols, "l_sys_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->lsn_name[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->lsn_name);
		strcat(values, buf);
		strcat(cols, "l_lsn_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->machine[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", g_eswid);
		strcat(values, buf);
		strcat(cols, "machine");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (link->addr[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", link->addr);
		strcat(values, buf);
		strcat(cols, "addr");
		strcat(cols, ",");
		strcat(values, ",");
	}
	
	if (starttime > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%ld", starttime);
		strcat(values, buf);
		strcat(cols, "starttime");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->trace_no > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%lld", cmd->trace_no);
		strcat(values, buf);
		strcat(cols, "c_traceno");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->mtype > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%ld", cmd->mtype);
		strcat(values, buf);
		strcat(cols, "c_mtype");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->timeout > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->timeout);
		strcat(values, buf);
		strcat(cols, "c_timeout");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->start_line > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->start_line);
		strcat(values, buf);
		strcat(cols, "start_line");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->dst_type > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->dst_type);
		strcat(values, buf);
		strcat(cols, "dst_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->ori_type > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->ori_type);
		strcat(values, buf);
		strcat(cols, "ori_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->msg_type > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->msg_type);
		strcat(values, buf);
		strcat(cols, "msg_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->task_flg > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->task_flg);
		strcat(values, buf);
		strcat(cols, "task_flg");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->type > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->type);
		strcat(values, buf);
		strcat(cols, "c_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->level > 0)
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "%d", cmd->level);
		strcat(values, buf);
		strcat(cols, "c_level");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->ori_def[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->ori_def);
		strcat(values, buf);
		strcat(cols, "ori_def");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->lsn_name[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->lsn_name);
		strcat(values, buf);
		strcat(cols, "c_lsn_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->def_name[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->def_name);
		strcat(values, buf);
		strcat(cols, "def_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->dst_prdt[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->dst_prdt);
		strcat(values, buf);
		strcat(cols, "dst_prdt");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->dst_svr[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->dst_svr);
		strcat(values, buf);
		strcat(cols, "dst_svr");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->dst_svc[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->dst_svc);
		strcat(values, buf);
		strcat(cols, "dst_svc");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->ori_prdt[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->ori_prdt);
		strcat(values, buf);
		strcat(cols, "ori_prdt");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->ori_svr[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->ori_svr);
		strcat(values, buf);
		strcat(cols, "ori_svr");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->ori_svc[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->ori_svc);
		strcat(values, buf);
		strcat(cols, "ori_svc");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->sys_date[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->sys_date);
		strcat(values, buf);
		strcat(cols, "c_sys_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cmd->udp_name[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf), "'%s'", cmd->udp_name);
		strcat(values, buf);
		strcat(cols, "udp_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cols[0] == '\0')
	{
		pub_log_error("[%s][%d] cols is null!", __FILE__, __LINE__);
		return -1;
	}

	index = strlen(cols) - 1;
	if (cols[index] == ',')
	{
		cols[index] = '\0';
	}

	index = strlen(values) - 1;
	if (values[index] == ',')
	{
		values[index] = '\0';
	}

	memset(sql, 0x00, sizeof(sql));
	snprintf(sql, sizeof(sql), "insert into mq_linkinfo(%s) values(%s)", cols, values);	

	ret = pub_db_nquery(sql);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] save linkinfo error,sql[%s]", __FILE__, __LINE__,sql);
		return SW_ERROR;
	}

	ret = pub_db_commit();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] commit db error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] save linkinfo success! mtype=[%ld] key=[%s]", __FILE__, __LINE__,link->mtype, link->keyinfo);

	return SW_OK;
}

sw_int_t slink_load_linkinfo_by_key(sw_lsn_cycle_t *cycle, sw_link_t *link)
{
	int i = 0;
	int ret = 0;
	int cols = 0;
	char *ptr = NULL;
	char name[256];
	char sql[512];

	ret = mq_db_detect(cycle);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] slink_load_linkinfo_by_key detect error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(sql, 0x00, sizeof(sql));
	snprintf(sql, sizeof(sql), "select infoset, l_mtype, l_type, l_traceno, l_sys_date, machine, addr, c_lsn_name, clearflag, keyinfo, infoflag from mq_linkinfo where keyinfo = '%s' and sendtype != %d", 
			link->keyinfo, cycle->lsn_conf.sendtype);
	cols = pub_db_squery(sql);
	if (cols <= 0)
	{
		pub_log_error("[%s][%d] slink_load_linkinfo_by_key error.sql=[%s]", __FILE__, __LINE__, sql);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] sql=[%s] cols=[%d]", __FILE__, __LINE__, sql, cols);

	for (i = 0; i< cols; i++)
	{
		memset(name, 0x0, sizeof(name));
		ptr = pub_db_get_data_and_name(NULL, 1, i + 1, name, sizeof(name));
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] get data and name error.", __FILE__, __LINE__);
			return SW_ERROR;
		}	
		if (pub_str_strcasecmp(name, "infoset") == 0)
		{
			strncpy(link->infoset, ptr, sizeof(link->infoset) - 1);
			pub_str_trim(link->infoset);
		}

		if (pub_str_strcasecmp(name, "l_mtype") == 0)
		{
			link->mtype = atol(ptr);
		}

		if (pub_str_strcasecmp(name, "l_traceno") == 0)
		{
			link->traceno = atoll(ptr);
		}

		if (pub_str_strcasecmp(name, "l_sys_date") == 0)
		{
			strncpy(link->sys_date, ptr, sizeof(link->sys_date) - 1);
			pub_str_trim(link->sys_date);
		}

		if (pub_str_strcasecmp(name, "c_lsn_name") == 0)
		{
			strncpy(link->lsn_name, ptr, sizeof(link->lsn_name) - 1);
			pub_str_trim(link->lsn_name);
		}

		if (pub_str_strcasecmp(name, "clearflag") == 0)
		{
			strncpy(link->clearflag, ptr, sizeof(link->clearflag) - 1);
			pub_str_trim(link->clearflag);
		}

		if (pub_str_strcasecmp(name, "keyinfo") == 0)
		{
			strncpy(link->keyinfo, ptr, sizeof(link->keyinfo) - 1);
			pub_str_trim(link->keyinfo);
		}

		if (pub_str_strcasecmp(name, "infoflag") == 0)
		{
			strncpy(link->infoflag, ptr, sizeof(link->infoflag) - 1);
			pub_str_trim(link->infoflag);
		}

		if (pub_str_strcasecmp(name, "machine") == 0)
		{
			strncpy(link->machine, ptr, sizeof(link->machine) - 1);
			pub_str_trim(link->machine);
		}

		
		if (pub_str_strcasecmp(name, "addr") == 0)
		{
			strncpy(link->addr, ptr, sizeof(link->addr) - 1);
			pub_str_trim(link->addr);
		}

	}

	return SW_OK;
}

sw_int_t slink_delete_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link)
{
	int 	ret = 0;
	char 	sql[256];

	ret = mq_db_detect(cycle);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] slink_delete_linkinfo detect error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(sql, 0x00, sizeof(sql));
	snprintf(sql, sizeof(sql), "delete from mq_linkinfo where l_mtype = %ld and keyinfo = '%s' ", link->mtype, link->keyinfo);
	ret = pub_db_nquery(sql);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] delete linkinfo error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = pub_db_commit();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] commit db error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] delete linkinfo success! mtype=[%ld] traceno[%lld] keyinfo=[%s]", 
			__FILE__, __LINE__, link->mtype, link->traceno, link->keyinfo);
	return SW_OK;
}

static sw_int_t mq_deal_conn_name(char *name)
{
	int	len = 0;
	int	index = 0;
	char	*sep = "|";
	char	*p = name;
	char	*q = NULL;

	if (name[0] == '\0' || name == NULL)
	{
		pub_log_error("[%s][%d] input argv name is null.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_mq_dbconn_cnt = 0;
	g_mq_dbconn_curr = 0;
	while((q = strstr(p, sep)) != NULL)
	{
		memset(&g_mq_dbconn_t[index].conn_name, 0x0, sizeof(g_mq_dbconn_t[index].conn_name));
		len = q - p;
		strncpy(g_mq_dbconn_t[index].conn_name, p, len);
		p += len + 1;
		index++;
	}
	if (*p != '\0')
	{
		memset(&g_mq_dbconn_t[index].conn_name, 0x0, sizeof(g_mq_dbconn_t[index].conn_name));
		strncpy(g_mq_dbconn_t[index].conn_name, p, strlen(p));
		index++;
	}
	g_mq_dbconn_cnt = index;

	return SW_OK;
}

static sw_int_t mq_db_conn_init(sw_lsn_cycle_t *cycle, char *name, sw_int32_t mode)
{
	int	ret = 0;
	sw_dbcfg_t	db;

	memset(&db, 0x0, sizeof(db));

	ret = cfg_get_db_conn((sw_cfgshm_hder_t *)cycle->base.shm_cfg, name, &db);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get_db_name [%s] error!", __FILE__, __LINE__, name);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] get db conn [%s] success! exptime=[%d]", __FILE__, __LINE__, name, db.exptime);
	g_mq_exptime = db.exptime;

	db.mode = mode;
	ret = pub_db_init(&db);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_db_init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] pub_db_init [%s] success!", __FILE__, __LINE__, name);

	return SW_OK;
}

static sw_int_t mq_db_init_ext(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;

	for (i = 0; i < g_mq_dbconn_cnt; i++)
	{
		j = (g_mq_dbconn_cnt + g_mq_dbconn_curr + i) % g_mq_dbconn_cnt;
		ret = mq_db_conn_init(cycle, g_mq_dbconn_t[j].conn_name, 0);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] mq_db_init [%s] error!", __FILE__, __LINE__, cycle->lsn_conf.db_conn_name);
			return SW_ERROR;
		}

		ret = pub_db_open();
		if (ret == SW_OK)
		{
			g_mq_dbconn_curr = j + 1;
			g_mq_db_conn.conn = 1;
			g_mq_db_conn.start_time = (long)time(NULL);
			pub_log_info("[%s][%d] Connect db [%s] success!", __FILE__, __LINE__, g_mq_dbconn_t[j].conn_name);
			break;
		}
	}
	if (i == g_mq_dbconn_cnt)
	{
		pub_log_error("[%s][%d] Connect to db error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t mq_db_init(sw_lsn_cycle_t *cycle)
{
	int ret = 0;

	ret = mq_deal_conn_name(cycle->lsn_conf.db_conn_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get single db conn error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = mq_db_init_ext(cycle);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mq db init ext error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

static sw_int_t mq_db_detect(sw_lsn_cycle_t *cycle)
{
	int ret = 0;
	if (g_mq_use_db)
	{
		if (g_mq_db_conn.conn != 1)
		{
			pub_log_debug("[%s][%d] connect db begin...", __FILE__, __LINE__);
			ret = mq_db_init_ext(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] connect to db error! ret=[%d]", __FILE__, __LINE__, ret);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] connect db success!", __FILE__, __LINE__);
		}

		ret = pub_db_conn_detect();
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Detect db error!", __FILE__, __LINE__);
			ret = mq_db_init_ext(cycle);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] mq db init ext error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] reconn db ok.", __FILE__, __LINE__);
		}
	}

	return SW_OK;
}

