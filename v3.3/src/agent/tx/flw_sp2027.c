#include "agent_comm.h"

/*************************************************
  文 件 名:  flw_sp2401.c                        **
  功能： 获取业务交易要素						 **
  作    者: 赵强                                 **
  完成日期: 20160801                             **
 *************************************************/

/*2027*/
int sp2027(sw_loc_vars_t *vars)
{
	int		result = -1;
	char 	flag[2];
	char	reply[8];
	char	res_msg[256];
	char 	send_file[256];
	struct 	search requ_info;


	pub_log_info("[%s][%d] [%s] 交易2027开始处理......", __FILE__, __LINE__, __FUNCTION__);

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

	/*get Params*/
	pub_mem_memzero(&requ_info, sizeof(requ_info));
	loc_get_zd_data(vars, ".TradeRecord.Request.Svc", requ_info.Svc);
	if (requ_info.Svc[0] == '\0')
	{
		pub_log_error("[%s][%d] no Svc",__FILE__,__LINE__);
		strcpy(reply, "E012"); 
		goto ErrExit;
	}

	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", requ_info.Prdt);
	if ( requ_info.Prdt[0] == '\0')
	{
		pub_log_error("[%s][%d]  no Prdt",__FILE__,__LINE__);
		strcpy(reply, "E012"); 
		goto ErrExit;
	}


	loc_get_zd_data(vars, ".TradeRecord.Request.Svr", requ_info.Svr);
	if ( requ_info.Svr[0] == '\0')
	{
		pub_log_error("[%s][%d] no Svr",__FILE__,__LINE__);
		strcpy(reply, "E012"); 
		goto ErrExit;
	}

	loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", requ_info.PlatFlow);
	if (requ_info.PlatFlow[0] == '\0')
	{
		pub_log_error("[%s][%d] no PlatFlow",__FILE__,__LINE__);
		strcpy(reply, "E012"); 
		goto ErrExit;
	}


	loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", requ_info.TxDate);
	if (requ_info.TxDate[0] == '\0')
	{
		pub_log_error("[%s][%d] no TxDate",__FILE__,__LINE__);
		strcpy(reply, "E012"); 
		goto ErrExit;
	}

	pub_mem_memzero(flag, sizeof(flag));
	loc_get_zd_data(vars, ".TradeRecord.Request.Flag", flag);
	if (flag[0] != '0' && flag[0] != '1')
	{
		pub_log_error("[%s][%d] no Flag or Flag error",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(send_file, 0x00, sizeof(send_file));
	if(atoi(flag) == 0)
	{
		snprintf(send_file, sizeof(send_file), "%s/dat/%s_%lld", getenv("SWWORK"), requ_info.TxDate, atoll(requ_info.PlatFlow));
	}
	else
	{
		snprintf(send_file, sizeof(send_file), "%s/dat/%s_%lld_pkg", getenv("SWWORK"), requ_info.TxDate, atoll(requ_info.PlatFlow));
	}

	result = access(send_file, F_OK);
	if(result)
	{
		result = agt_get_trace_log(vars, &requ_info, atoi(flag), NULL);
		if(result != SW_OK)
		{
			strcpy(reply, "E023"); 
			goto ErrExit;
		}
	}

	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
	loc_set_zd_data(vars, "$send_path", send_file);

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

