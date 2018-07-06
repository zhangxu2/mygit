/*********************************************************
  文 件 名: flw_sp2040.c                                 
  功能描述: 获取平台进程信息、启停平台、启停进程         
  作    者: guxiaoxin
  完成日期: 20160608                                     
 *********************************************************/
#include "agent_comm.h"
static char rescode[16];
static char resmsg[1024];

static int query_BP_procinfo(sw_loc_vars_t *vars);
static int manage_BP_process(sw_loc_vars_t *vars, char *option);

int sp2040(sw_loc_vars_t *vars)
{
	int 	ret   = 0;
	char 	buf[256];
	char 	cmd[128];

	char option[4];
	memset(option, 0x0, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);
	if (strlen(option) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.Option can not be null", __FILE__, __LINE__);
		strncpy(resmsg, ".TradeRecord.Request.Option can not be null", sizeof(resmsg));
		strcpy(rescode, "E012");
		goto ErrExit;
	}
	
	pub_log_debug("[%s][%d] option=[%s]", __FILE__, __LINE__, option);
	if (strcmp(option, "L") == 0)
	{
		pub_log_info("[%s][%d]查询平台进程", __FILE__, __LINE__);
		ret = query_BP_procinfo(vars);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] query_BP_procinfo error", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(option, "S") == 0)
	{
		pub_log_info("[%s][%d] 启动平台", __FILE__, __LINE__);
		memset(cmd, 0x0, sizeof(cmd));
		strcpy(cmd, "swadmin start");
		ret =  agt_system(cmd);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] execute [%s] error",__FILE__,__LINE__,cmd);
			strcpy(rescode, "E010"); 
			goto ErrExit;
		}
	}
	else if (strcmp(option, "T") == 0)
	{
		pub_log_info("[%s][%d] 停止平台", __FILE__, __LINE__);
		memset(cmd, 0x0, sizeof(cmd));
		strcpy(cmd, "swadmin stop");
		ret =  agt_system(cmd);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] execute [%s] error",__FILE__,__LINE__,cmd);
			strcpy(rescode, "E010"); 
			goto ErrExit;
		}
	}
	else if (strcmp(option, "SP") == 0)
	{
		pub_log_info("[%s][%d]启动平台进程", __FILE__, __LINE__);
		ret = manage_BP_process(vars, "start");
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d]启动平台进程失败", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(option, "TP") == 0)
	{
		pub_log_info("[%s][%d]停止平台进程", __FILE__, __LINE__);
		ret = manage_BP_process(vars, "stop");
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d]停止平台进程失败", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else
	{
		pub_log_error("[%s][%d]未知请求", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		strcpy(resmsg, "未知请求");
		goto ErrExit;
	}
	goto OkExit;

OkExit:

	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "交易成功");
	pub_log_info("[%s][%d] [%s] Trade 2040 Success![END][OK]", __FILE__, __LINE__, __FUNCTION__);

	return SW_OK;
ErrExit:

	pub_log_debug("[%s][%d] [%s]Trade 2040 Fails!", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(rescode, resmsg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", rescode);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", resmsg);

	return SW_ERROR;
}

int query_BP_procinfo(sw_loc_vars_t *vars)
{
	int 	loop     = 0;
	int 	count    = 0;
	int     page_cnt = 0;
	int     page_idx = 0;
	int 	page_sum = 0;
	char	sts[32];
	char 	buf[128];
	char	status[32];
	char 	line[1024];
	char 	*p    = NULL;
	char 	*var  = NULL;
	char 	*pstr = NULL;
	FILE 	*pf   = NULL;

	loc_get_zd_int(vars, ".TradeRecord.Request.PageIndex", &page_idx);
	loc_get_zd_int(vars, ".TradeRecord.Request.PageCount", &page_cnt);
	if (page_cnt <= 0)
	{
		strcpy(rescode, "E012");
		pub_log_error("[%s][%d] PageCount can not be zero", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (page_idx < 1)
	{
		page_idx = 1;
	}

	int 	end    = page_idx * page_cnt;
	int 	begin  = (page_idx - 1) * page_cnt;
	bool 	legal  = false;
	pub_log_debug("[%s][%d] begin[%d] end[%d] page_cnt[%d], page_idx[%d]", 
				__FILE__, __LINE__, begin, end, page_cnt, page_idx);

	pf = popen("swadmin l", "r");
	if (pf == NULL)
	{
		pub_log_error("[%s][%d] popen[swadmin l] error[%d][%s]."
				, __FILE__, __LINE__, errno, strerror(errno));
		strcpy(rescode, "E010");
		return SW_ERROR;
	}
	fseek(pf, 0, SEEK_SET);

	while (!feof(pf))
	{
		memset(line, 0x00, sizeof(line));
		pstr = fgets(line, sizeof(line), pf);
		if (pstr == NULL)
		{
			if (feof(pf))
			{
				pub_log_debug("[%s][%d] end of file", __FILE__, __LINE__);
				break;
			}

			pub_log_error("[%s][%d] fgets return NULL", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;	
		}

		if (line[0] == '\n' && strlen(line) == 1)
		{
			continue;
		}

		legal = count >= begin && count < end;
		p = strtok(line, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtoks return NULL", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			var = strstr(p, "sw");
			if (var == NULL)
			{
				continue;
			}
			if (legal)
			{
				pub_log_debug("[%s][%d] process name:[%s] ProcInfo index[%d] proc_idx[%d]", __FILE__, __LINE__, var, loop, count);
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.ProcInfos.ProcInfo(%d).ProcName", loop);
				loc_set_zd_data(vars, buf, var);
			}
		}

		p = strtok(NULL, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtok return NULL!", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;	
		}
		else
		{
			if (legal)
			{
				pub_log_debug("[%s][%d] process type:[%s]", __FILE__, __LINE__, p);
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.ProcInfos.ProcInfo(%d).Type", loop);
				loc_set_zd_data(vars, buf, p);
			}
		}

		p = strtok(NULL, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtok return NULL!", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			if (legal)
			{
				pub_log_debug("[%s][%d] process pid:[%s]", __FILE__, __LINE__, p);
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.ProcInfos.ProcInfo(%d).Pid", loop);
				loc_set_zd_data(vars, buf, p);
			}
		}

		p = strtok(NULL, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtok return NULL!", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			p[strlen(p)-1] = '\0';
			if (legal)
			{
				pub_log_debug("[%s][%d] process status:[%s]", __FILE__, __LINE__, p);
				memset(status, 0x0, sizeof(status));
				memset(sts, 0x0, sizeof(sts));
				if (strcmp(p, "NORMAL") == 0)
				{
					strcpy(status, "正常");
					strcpy(sts, "0");
				}
				else if (strcmp(p, "ABNORMAL") == 0 || strcmp(p, "ABORTED") == 0)
				{
					strcpy(status, "异常");
					strcpy(sts, "1");
				}
				else if (strcmp(p, "STOPED") == 0)
				{
					strcpy(status, "停止");
					strcpy(sts, "1");
				}
				else
				{
					strcpy(status, "未知");
					strcpy(sts, "1");
				}
					
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.ProcInfos.ProcInfo(%d).Status", loop);
				loc_set_zd_data(vars, buf, status);
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.ProcInfos.ProcInfo(%d).ProcSts", loop);
				loc_set_zd_data(vars, buf, sts);
				
				loop++;
			}
		}
		count++;
	}

	if (count%page_cnt != 0)
	{
		page_sum = count/page_cnt + 1;
	}
	else
	{
		page_sum = count/page_cnt;
	}

	pub_log_info("[%s][%d] count[%d] page_sum[%d] page_cnt[%d]", __FILE__, __LINE__, count, page_sum, page_cnt);
	loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", page_sum);
	loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", count);
	pclose(pf);
	return SW_OK;
}

int manage_BP_process(sw_loc_vars_t *vars, char *option)
{
	int 	ret   = 0;
	char 	buf[128];
	char 	cmd[128];
	char 	proc[32];

	memset(cmd, 0x0, sizeof(cmd));
	memset(proc, 0x0, sizeof(proc));

	loc_get_zd_data(vars, ".TradeRecord.Request.ProcName", proc);
	if (0 != strlen(proc))
	{
		memset(buf, 0x0, sizeof(buf));
		if (strncmp(proc, "swlsn_", 6) == 0)
		{
			sprintf(cmd, "swadmin %s -l %s", option, proc + 6);
		}
		else if (strncmp(proc, "swsvcman_", 9) == 0)
		{
			sprintf(cmd, "swadmin %s -s %s", option, proc + 9);
		}
		else
		{
			sprintf(cmd, "swadmin %s %s", option, proc);
		}
	}
	else
	{
		pub_log_error("[%s][%d] process name is null",__FILE__,__LINE__);
		strcpy(rescode, "E012");
		strcpy(resmsg, "process name can not be null");
		return SW_ERROR;
	}

	ret =  agt_system(cmd);
	if( ret != SW_OK )
	{
		pub_log_error("[%s][%d] execute [%s] error",__FILE__,__LINE__,cmd);
		strcpy(rescode, "E010");
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "execute %s error", cmd);
		strcpy(resmsg, buf);
		return SW_ERROR;	
	}
	pub_log_info("[%s][%d] execute CMD[%s] success", __FILE__,__LINE__,cmd);

	return SW_OK;
}
