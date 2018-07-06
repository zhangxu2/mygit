/*************************************************
  文 件 名:  flw_sp2523.c                        **
  功能描述:  获取交易流程图                      **
  作    者:  赵强                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"


int sp2024(sw_loc_vars_t* vars)
{
	int		result = -1;
	char	reply[16];
	char	tx_code[16];
	char	path[256];
	char	res_msg[256];
	char	svr_name[SW_NAME_LEN];
	char	prdt_name[SW_NAME_LEN];
	char	*tmp = NULL;

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

	pub_mem_memzero(tx_code, sizeof(tx_code));
	loc_get_zd_data(vars, ".TradeRecord.Request.TxCode", tx_code);
	if (strlen(tx_code) == 0)
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.TxCode",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	pub_mem_memzero(prdt_name, sizeof(prdt_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", prdt_name);
	if (strlen(prdt_name) == 0)
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.PrdtName",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	pub_mem_memzero(svr_name, sizeof(svr_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.Svr", svr_name);
	if (strlen(svr_name) == 0)
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.SvrName",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, no env SWWORK",__FILE__,__LINE__);
		strcpy(reply, "E014");
		goto ErrExit;
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, "%s/products/%s/etc/svrcfg/%s/%s/"
		, tmp, prdt_name, svr_name, tx_code);

	result = access(path, F_OK);
	if (result != 0)
	{
		pub_log_error("%s, %d, access %s error[%d][%s]."
			, __FILE__, __LINE__, path, errno, strerror(errno));
		strcpy(reply, "E009");
		goto ErrExit;		
	}

	loc_set_zd_data(vars, "$send_path", path);
	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_OK;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;

}

