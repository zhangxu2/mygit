/*************************************************
  文 件 名: flw_sp2036.c                        
  功	能: 根据预警信息提取日志
  作    者: guxiaoxin                                     
  完成日期: 20160802                                     
 *************************************************/
#include "agent_comm.h"

typedef struct trace_s
{
	char platflow[32];
	char tx_code[32];
	char chnl[32];
	char mtype[32];
	char errcode[32];
	char endtime[32];
	char prdt[32];
	char svr[32];
	char svc[32];
}trace_t;

static int get_info(char *src, char *dst, char *sign)
{
	char *p = NULL;
	char *q = NULL;
	char *start = NULL;
	
	start = strstr(src, sign);
	if (start == NULL)
	{
		return -1;
	}	

	p = strchr(start, '[');
	if (p == NULL)
	{
		return -1;
	}
	
	q = strchr(p, ']');
	if (q == NULL)
	{
		return -1;
	}
	
	memcpy(dst, p+1, q-p-1);

	return 0;
}


static int get_trace_info(char *buf, trace_t *trace)
{

	int ret = 0;

	ret = get_info(buf, trace->platflow, "平台流水");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get traceno error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->tx_code, "交易码");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get txcode error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->chnl, "发起渠道");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get channel error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->mtype, "MTYPE");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get mtype error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->errcode, "错误码");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get errcode error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->endtime, "交易时间");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get end time error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->prdt, "产品名称");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get product name error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->svr, "SVR");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get svr error", __FILE__, __LINE__);
		return -1;
	}

	ret = get_info(buf, trace->svc, "SVC");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get svc error", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

/*2036*/
int sp2036(sw_loc_vars_t *vars)
{
	int		i = 0;
	int 	j = 0;
	int		num = 0;
	int		ret = SW_ERROR;
	char	buf[256];
	char	date[32];
	char	path[256];
	char	reply[8];
	char	res_msg[256];
	char	filepath[256];
	char	filename[256];
	char	alertid[64];
	trace_t	trace;
	FILE 	*fp = NULL;
	struct  dirent **namelist = NULL;
	
	pub_log_info("[%s][%d] [%s] 交易2036开始处理......", __FILE__, __LINE__, __FUNCTION__);

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

	memset(date, 0x0, sizeof(date));
	loc_get_zd_data(vars, ".TradeRecord.Request.Bp_alert.ALERT_DATE", date);
	memset(alertid, 0x0, sizeof(alertid));
	loc_get_zd_data(vars, ".TradeRecord.Request.Bp_alert.SESS_ID", alertid);
	
	memset(filepath, 0x0, sizeof(filepath));
	sprintf(filepath, "%s/tmp/monitor/%s", getenv("SWWORK"), date);

	if (access(filepath, F_OK) != 0 )
	{    
		pub_log_info("[%s][%d] [%s]no monitor log", __FILE__, __LINE__, path);
		return 0;
	}

	namelist = agt_scan_dir(&num, filepath);
	if (namelist == NULL)
	{
		pub_log_error("[%s][%d] get namelist error", __FILE__, __LINE__);
		return -1;
	}

	for (i = 0; i < num; ++i)
	{
		if(strncmp(namelist[i]->d_name, "mon_error", strlen("mon_error")) != 0)
		{
			continue;
		}

		memset(filename, 0x00, sizeof(filename));
		sprintf(filename, "%s/%s", filepath, namelist[i]->d_name);

		fp = fopen(filename, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file[%s] failed", __FILE__, __LINE__, filename);
			agt_free_namelist(namelist, num);
			strcpy(reply, "E016");
			goto ErrExit;
		}

		j = 0;
		while(!feof(fp))
		{
			memset(buf, 0x0, sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			if (strncmp( buf+strlen("预警编号:["), alertid, strlen(alertid)) == 0)
			{
				memset(&trace, 0x0, sizeof(trace));
				ret = get_trace_info(buf, &trace);
				if (ret)
				{
					pub_log_error("[%s][%d] get trace info error", __FILE__, __LINE__);
					agt_free_namelist(namelist, num);
					strcpy(reply, "E049");
					fclose(fp);	
					goto ErrExit;
				}
			
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).Lsn", j);
				loc_set_zd_data(vars, path, trace.chnl);

				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).PlatFlow", j);
				loc_set_zd_data(vars, path, trace.platflow);

				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).Mtype", j);
				loc_set_zd_data(vars, path, trace.mtype);

				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).TxCode", j);
				loc_set_zd_data(vars, path, trace.tx_code);

				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).RetCode", j);
				loc_set_zd_data(vars, path, trace.errcode);

				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).TxDate", j);
				loc_set_zd_data(vars, path, date);
				
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).Time", j);
				loc_set_zd_data(vars, path, trace.endtime);
				
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).Prdt", j);
				loc_set_zd_data(vars, path, trace.prdt);
				
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).Svr", j);
				loc_set_zd_data(vars, path, trace.svr);
				
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Trades.Trade(%d).Svc", j);
				loc_set_zd_data(vars, path, trace.svc);
				
				j++;
			}
			else if (j > 0)
			{
				fclose(fp);
				agt_free_namelist(namelist, num);
				pub_log_info("[%s][%d] get trade info success", __FILE__, __LINE__);
				goto OkExit;
			}
		}
		fclose(fp);
	}

	agt_free_namelist(namelist, num);
	pub_log_info("[%s][%d] get trade info success", __FILE__, __LINE__);
	goto OkExit;

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return SW_OK;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}

