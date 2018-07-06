/*************************************************
  文 件 名:  flw_sp2020.c                        **
  功能描述:  获取当日各个小时出错状态            **
  作    者:  赵强                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"


int sp2020(sw_loc_vars_t *vars)
{
	int  	ret = 0;
	int 	i = 0;
	int		j = 0;
	char	buf[64];
	char	line[1024];
	char	reply[8];
	char	res_msg[128];
	char	path[256];
	unit_t 	*pproduct = NULL;
	unit_t 	*pchannel = NULL;
	unit_t 	*punit = NULL;
	struct 	total  total;
	time_t	now = 0;
	struct 	tm tm;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));
	
	now = time(NULL);
	memset(&tm, 0x0, sizeof(tm));
	localtime_r(&now, &tm);
	
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
		strcpy(res_msg, "查询交易信息失败"); 
		free_unit(pproduct);
		free_unit(pchannel);
		goto ErrExit;
	}

	ret = count_monitor(&total, pproduct, pchannel);
	if ( ret < 0)
	{
		strcpy(reply, "E999"); 
		strcpy(res_msg, "查询交易信息失败"); 
		free_unit(pproduct);
		free_unit(pchannel);
		goto ErrExit;
	}

	ret = agt_save_monitor_data(&total, pproduct, pchannel);
	if ( ret < 0)
	{
		strcpy(reply, "E999"); 
		strcpy(res_msg, "查询交易信息失败"); 
		free_unit(pproduct);
		free_unit(pchannel);
		goto ErrExit;
	}

	for (punit = pproduct, i = 0; punit != NULL; punit = punit->next, ++i)
	{
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).Ename", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).Name", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(line, 0x0, sizeof(line));
		for (j = 0; j <= tm.tm_hour; ++j)
		{
			memset(buf, 0x0, sizeof(buf));
			snprintf(buf, sizeof(buf)-1,  "%d-%d|",punit->wrong[j], punit->right[j] );
			strcat(line, buf);
		}
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).Hour", i);
		loc_set_zd_data(vars, path, line);
		
	}

	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%04d年%02d月%02d日", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Date");	
	loc_set_zd_data(vars, path, buf);


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
