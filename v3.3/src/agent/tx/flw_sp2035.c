/****************************************************************
  文 件 名:  flw_sp2035.c                        
  功    能:  应用层快照页面，获取查询条件的产品、渠道和交易码列表
  作    者:  guxiaoxin                                    
  完成日期:  20160802                                    
 ****************************************************************/
#include "agent_comm.h"

int sp2035(sw_loc_vars_t *vars)
{
	int     cnt;
	int     i,j;
	char	reply[8];
	char    buf[256];
	char    path[512];
	char	res_msg[256];
    unit_t  *punit    = NULL;
	unit_t  *ptmp     = NULL;
	unit_t  *pproduct = NULL;

	pub_log_info("[%s][%d] [%s] 交易2035开始处理......", __FILE__, __LINE__, __FUNCTION__);

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_mem_memzero(buf, sizeof(buf));
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Cnt", buf);
	cnt = atoi(buf);
	if ( cnt <= 0)
	{
		pub_log_error("[%s][%d] Cnt error!", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
		
	pproduct = get_prdt_info();
	if (pproduct == NULL)
	{
		pub_log_error("[%s][%d] get prdt error",__FILE__,__LINE__);
	}

	for (i = 0; i < cnt; i++)
	{
		pub_mem_memzero(buf, sizeof(buf));
		pub_mem_memzero(path, sizeof(path));
		snprintf(path, sizeof(path), ".TradeRecord.Request.Lists.List(%d).Name", i);
		loc_get_zd_data(vars, path, buf);
		pub_log_info("[%s][%d] get list type[%s]",__FILE__,__LINE__, buf);
		
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Types.Items(%d).Type",i);
		loc_set_zd_data(vars, path, buf);
		
		if (strcasecmp(buf, "product") == 0)
		{
			ptmp = pproduct;
			if (ptmp == NULL)
			{
				pub_log_error("[%s][%d] get prdt error",__FILE__,__LINE__);
				continue;
			}
		}
		else if(strcasecmp(buf, "server") == 0)
		{
			if (pproduct == NULL)
			{
				pub_log_error("[%s][%d] get prdt error",__FILE__,__LINE__);
				continue;
			}
			ptmp = get_svr_info(pproduct);
			if (ptmp == NULL)
			{
				pub_log_error("[%s][%d] get svr error",__FILE__,__LINE__);
				continue;
			}
		}
		else if(strcasecmp(buf, "channel") == 0)
		{
			ptmp = get_chnl_info();
			if (ptmp == NULL)
			{
				pub_log_error("[%s][%d] get channel error",__FILE__,__LINE__);
				continue;
			}
		}
		else if (strcasecmp(buf, "txcode") == 0)
		{
			if (pproduct == NULL)
			{
				pub_log_error("[%s][%d] get prdt error",__FILE__,__LINE__);
				continue;
			}

			ptmp = get_txcode_info(pproduct);
			if (ptmp == NULL)
			{
				pub_log_error("[%s][%d] get txcode error",__FILE__,__LINE__);
				continue;
			}
		}
		else if (strcasecmp(buf, "retcode") == 0)
		{
			ptmp = get_retcode_info(pproduct);
			if (ptmp == NULL)
			{
				pub_log_error("[%s][%d] get retcode error",__FILE__,__LINE__);
				continue;
			}
		}

		for (punit = ptmp, j = 0; punit != NULL; punit = punit->next, j++)
		{
			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Types.Items(%d).Item(%d).CNNAME",i, j);
			loc_set_zd_data(vars, path, punit->cnname);

			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Types.Items(%d).Item(%d).NAME",i, j);
			loc_set_zd_data(vars, path, punit->name);
		}
		if (ptmp != pproduct)
		{
			free_unit(ptmp);
			ptmp = NULL;
		}
	}
	
	free_unit(pproduct);
	pproduct = NULL;
	/*get Params*/
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

