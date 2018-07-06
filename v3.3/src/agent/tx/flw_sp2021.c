/*************************************************
  文 件 名:  flw_sp2021.c                        **
  功能描述:  查询某小时出错交易具体信息          **
  作    者:  赵强                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

static int set_trade_error_info(sw_loc_vars_t *vars, sw_trace_item_t *pTrace_info, int i, unit_t **pp, char *ptid)
{

	int 	j = 0;
	char	buf[128];
	char 	path[512];
	char 	sbuf[512];
	char 	start_time[32];
	char 	end_time[32];
	unit_t  *punit = NULL;
	memset(sbuf, 0x0, sizeof(sbuf));
	memset(start_time, 0x0, sizeof(start_time));
	memset(end_time, 0x0, sizeof(end_time));

	pub_change_time2(pTrace_info->start_time, start_time, 0);
	pub_change_time2(pTrace_info->end_time, end_time, 0);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).TxCode", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_code);
	for (punit = pp[3], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, buf) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CNTxCode", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).PlatFlow", i);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%lld", pTrace_info->trace_no);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).Svr", i);
	loc_set_zd_data(vars, path, pTrace_info->server);
	for (punit = pp[1], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, pTrace_info->server) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).CNSvr", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).Svc", i);
	loc_set_zd_data(vars, path, pTrace_info->service);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).TxDate", i);
	loc_set_zd_data(vars, path, pTrace_info->start_date);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).Prdt", i);
	loc_set_zd_data(vars, path, pTrace_info->prdt_name);
	for (punit = pp[0], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, pTrace_info->prdt_name) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).CNPrdt", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).Lsn", i);
	loc_set_zd_data(vars, path, pTrace_info->chnl);
	for (punit = pp[2], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, pTrace_info->chnl) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).CNLsn", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}


	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).RetCode", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_respcd);


	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).TxBeginTime", i);
	loc_set_zd_data(vars, path, start_time);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).TxEndTime", i);
	loc_set_zd_data(vars, path, end_time);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).ResMsg", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_errmsg);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Detalls.Detall(%d).Ptid", i);
	loc_set_zd_data(vars, path, ptid);

	return 0;
}

int sp2021(sw_loc_vars_t *vars)
{
	int 	ret = -1;
	int 	page_cnt = 0;
	int 	page_idx = 0;
	char 	sHour[64];
	char    reply[16];
	char 	value[16];
	char	res_msg[256];
	char	sTime[256];
	struct 	tm stTm;
	time_t now;
	struct search searchs;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset(&searchs, 0x0, sizeof(searchs));
	memset(sHour, 0x00, sizeof(sHour));
	loc_get_zd_data(vars, ".TradeRecord.Request.Hour", sHour);
	pub_log_debug("[%s][%d]the Hour is [%s]", __FILE__, __LINE__, sHour);
	if (strlen(sHour) == 0 || sHour[0] == '\0')
	{
		pub_log_error("[%s][%d]the Hour is null", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	now = time(NULL);
	memset(&stTm, 0x0, sizeof(stTm));
	localtime_r(&now, &stTm);
	sprintf(sTime, "%04d年%02d月%02d日%s时", stTm.tm_year+1900, stTm.tm_mon+1, stTm.tm_mday, sHour);
	loc_set_zd_data(vars, ".TradeRecord.Response.Time", sTime);

	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", searchs.Prdt);
	pub_log_debug("[%s][%d]the Hour is [%s]", __FILE__, __LINE__, searchs.Prdt);
	if (strlen(searchs.Prdt) == 0 || searchs.Prdt[0] == '\0')
	{
		pub_log_error("[%s][%d]the Product is null", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(value, 0x00, sizeof(value));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", value);
	if (strlen(value) == 0)
	{
		pub_log_error("[%s][%d] No .TradeRecord.Request.PageCount!\n", __FILE__, __LINE__); 	
		strcpy(reply, "E012");
		goto ErrExit;
	}

	page_cnt = atoi(value);
	if (page_cnt <= 0)
	{
		pub_log_error("[%s][%d] page_cnt[%d] is out of range!\n"
			, __FILE__, __LINE__, page_cnt); 	
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(value, 0x00, sizeof(value));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", value);
	if (strlen(value) == 0)
	{
		pub_log_error("[%s][%d] No .TradeRecord.Request.PageIndex!\n", __FILE__, __LINE__); 	
		strcpy(reply, "E012");
		goto ErrExit;
	}


	page_idx = atoi(value);
	if (page_idx <= 0)
	{
		pub_log_error("[%s][%d] page_idx[%d] is out of range!\n"
			, __FILE__, __LINE__, page_idx); 	
		strcpy(reply, "E012");
		goto ErrExit;
	}

	sprintf(searchs.StartTime, "%02d0000", atoi(sHour)-1);
	sprintf(searchs.EndTime, "%02d0000", atoi(sHour));
	strcpy(searchs.flag, "S");


	ret = agt_get_trc(vars, page_idx, page_cnt, &searchs, 0, set_trade_error_info);
	if (ret == -2)
	{
		strcpy(reply, "E023");
		pub_log_error("[%s][%d]数据不存在", __FILE__, __LINE__);
		goto ErrExit;
	}
	else if (ret < 0)
	{
		pub_log_error("[%s][%d]deal error", __FILE__, __LINE__);
		strcpy(reply, "E999");
		strcpy(res_msg, "查询失败");
		goto ErrExit;
	}

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
