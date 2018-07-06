/*************************************************
  文 件 名:  flw_sp2095.c                        **
  功能描述:  更新listChange.xml                       **
  作    者:                                      **
  完成日期:                                      **
 *************************************************/
#include "agent_comm.h"

int sp2095(sw_loc_vars_t *vars)
{
	int		result = -1;
	char	reply[8];
	char	res_msg[256];
	char	file[512];
	char	cmd[1024];
	
	memset(res_msg, 0x00, sizeof(res_msg));
	memset(reply, 0x00, sizeof(reply));
	
	memset(file, 0x00, sizeof(file));
	sprintf(file, "%s/listChange.xml", getenv("SWWORK"));
	
	result = access(file, F_OK);
	if ( result )
	{
		pub_log_error("%s %d file_shell[%s] is not found", __FILE__, __LINE__, file);
		strcpy(reply, "E009");
		goto ErrExit;
	}
	
	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "swadmin deploy -file=%s", file);
	pub_log_debug("%s, %d, cmd[%s]\n", __FILE__, __LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0 && errno != ECHILD)
	{
	        pub_log_error("%s, %d, run cmd[%s] error!"
	                , __FILE__, __LINE__, cmd);
		strcpy(reply,"E010");
		goto ErrExit;
	}

	
OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	
	return -1;
}
