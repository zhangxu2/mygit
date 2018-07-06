/*************************************************
  文 件 名:  flw_sp2032.c                        
  功能描述:  起始页交易统计功能                  
  作    者:  guxiaoxin                                    
  完成日期:  20160802                                    
 *************************************************/
#include "agent_comm.h"

int sp2032(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	j = 0;
	int  	ret = 0;
	char	buf[64];
	char	reply[8];
	char	path[256];
	char	line[1024];
	char	times[256];
	char	res_msg[128];
	struct 	total  total;
	struct 	tm tm;
	time_t 	curt = 0;
	unit_t 	*punit    = NULL;
	unit_t 	*pproduct = NULL;
	unit_t 	*pchannel = NULL;

	curt = time(NULL);
	memset(&tm, 0x0, sizeof(tm));
	localtime_r(&curt, &tm);

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));

	pproduct = get_prdt_info();
	if (pproduct == NULL)
	{
		pub_log_error("[%s][%d] get_prdt_info error", __FILE__, __LINE__);
		strcpy(res_msg, "获取产品信息失败");
		goto ErrExit;
	}

	pchannel = get_chnl_info();
	if (pproduct == NULL)
	{
		pub_log_error("[%s][%d] get_chnl_info error", __FILE__, __LINE__);
		free_unit(pproduct);
		strcpy(res_msg, "获取渠道信息失败");
		goto ErrExit;
	}

	memset(&total, 0x0, sizeof(total));
 	ret = agt_get_monitor_data(&total, pproduct, pchannel);
	if ( ret < 0)
	{
		strcpy(reply, "E999"); 
		free_unit(pproduct);
		free_unit(pchannel);
		strcpy(res_msg, "统计失败");
		goto ErrExit;
	}
	
	ret = count_monitor(&total, pproduct, pchannel);
	if ( ret < 0)
	{
		strcpy(reply, "E999"); 
		free_unit(pproduct);
		free_unit(pchannel);
		strcpy(res_msg, "统计失败");
		goto ErrExit;
	}
	
	ret = agt_save_monitor_data(&total, pproduct, pchannel);
	if ( ret < 0)
	{
		strcpy(reply, "E999"); 
		free_unit(pproduct);
		free_unit(pchannel);
		strcpy(res_msg, "统计失败");
		goto ErrExit;
	}

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Total.SUM");
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", total.num);
	loc_set_zd_data(vars, path, buf);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Total.RIGHT");
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", total.right);
	loc_set_zd_data(vars, path, buf);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Total.WRONG");
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%d", total.wrong);
	loc_set_zd_data(vars, path, buf);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Total.RATE");
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%.2f", agt_get_avgright(total.right, total.num));
	loc_set_zd_data(vars, path, buf);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Total.REACTION");
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%.2f", agt_get_avgright(total.reaction, total.num));
	loc_set_zd_data(vars, path, buf);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Total.AVGTIME");
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%.2f", agt_get_avgtime(total.d_time, total.num));
	loc_set_zd_data(vars, path, buf);

	memset(times, 0x0, sizeof(times));
	for (i = 1; i <= tm.tm_hour + 1; ++i)
	{
		memset(buf, 0x0, sizeof(buf));
		snprintf(buf, sizeof(buf)-1, "%d:00|", i);
		strcat(times, buf);
	}

	for (punit = pproduct, i = 0; punit != NULL; punit = punit->next, ++i)
	{
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(line, 0x0, sizeof(line));
		for (j = 0; j <= tm.tm_hour; ++j)
		{
			memset(buf, 0x0, sizeof(buf));
			snprintf(buf, sizeof(buf)-1,  "%d|", punit->right[j] + punit->wrong[j]);
			strcat(line, buf);
		}
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).CNT", i);
		loc_set_zd_data(vars, path, line);
		
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).TIME", i);
		loc_set_zd_data(vars, path, times);
	}

	for (punit = pchannel, i = 0; punit != NULL; punit = punit->next, ++i)
	{
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(line, 0x0, sizeof(line));
		for (j = 0; j <= tm.tm_hour; ++j)
		{
			memset(buf, 0x0, sizeof(buf));
			snprintf(buf, sizeof(buf)-1, "%d|", punit->right[j] + punit->wrong[j]);
			strcat(line, buf);
		}
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).CNT", i);
		loc_set_zd_data(vars, path, line);
		
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).TIME", i);
		loc_set_zd_data(vars, path, times);
	}

	free_unit(pproduct);
	free_unit(pchannel);

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
