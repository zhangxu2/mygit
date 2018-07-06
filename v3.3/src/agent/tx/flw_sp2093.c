/*************************************************
  文 件 名:  flw_sp2093.c                        **
  功能描述:  在线创建表                          **
  作    者:  linpanfei                           **
  完成日期:  20160803                            **
 *************************************************/
#include "agent_comm.h"

int sp2093(sw_loc_vars_t *vars)
{
	int 	result = -1;
	char	ip[32];
	char	cmd[512];
	char	reply[8];
	char	path[128];
	char	db_type[256];
	char	res_msg[256];
	char	msgfile[256];
	char	dst_path[256];
	char	file_name[256];
	char	file_shell[256];
	
	/*get dst_path*/
	pub_mem_memzero(ip, sizeof(ip));
	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_mem_memzero(dst_path, sizeof(dst_path));
	pub_mem_memzero(msgfile, sizeof(msgfile));

	loc_get_zd_data(vars, ".TradeRecord.Request.DirPath", dst_path);
	if (strlen(dst_path) == 0)
	{
		pub_log_error("[%s][%d] No .TradeRecord.Request.DirPath!", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	pub_log_debug("[%s][%d] the dst_path[%s]", __FILE__, __LINE__, dst_path);
	
	result = pub_file_check_dir(dst_path);
	if (result != 0)
	{
		pub_log_error("[%s][%d] access dir[%s] error[%d][%s]!"
			, __FILE__, __LINE__, dst_path, errno, strerror(errno));
		strcpy(reply, "E009");
		goto ErrExit;
	}
	
	pub_mem_memzero(db_type, sizeof(db_type));
	loc_get_zd_data(vars, ".TradeRecord.Request.DBtype", db_type);
	if (strlen(db_type) == 0 || db_type[0] == '\0')
	{
		pub_log_error("[%s][%d] No .TradeRecord.Request.DBtype!", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	pub_mem_memzero(file_shell, sizeof(file_shell));
	sprintf(file_shell, "%s/%s.sh", dst_path, db_type);
	
	result = access(file_shell, F_OK);
	if (result < 0)
	{
		pub_log_error("[%s][%d] file_shell[%s] is not found", __FILE__, __LINE__, file_shell);
		strcpy(reply, "E009");
		goto ErrExit;
	}
	pub_log_debug("[%s][%d] the db_type[%s]", __FILE__, __LINE__, db_type);
	
	memset(ip, 0x0, sizeof(ip));
	memset(msgfile, 0x0, sizeof(msgfile));
	loc_get_zd_data(vars, ".TradeRecord.Header.Security.IPAddress", ip);
	sprintf(msgfile, "%s/tmp/%s_error.txt", getenv("SWWORK"), ip);
	pub_log_debug("[%s][%d] msgfile====[%s]", __FILE__, __LINE__, msgfile);
	
	pub_mem_memzero(file_name, sizeof(file_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.FileName", file_name);
	
	pub_mem_memzero(cmd, sizeof(cmd));
	if (strlen(file_name) == 0 || file_name[0] == '\0')
	{
		pub_log_debug("[%s][%d] the filename is null", __FILE__, __LINE__);
		sprintf(cmd,"sh %s/%s.sh %s %s", dst_path, db_type, dst_path, msgfile);
	}
	else
	{
		pub_log_debug("[%s][%d] filename=[%s] dst_path=[%s]", __FILE__, __LINE__, file_name, dst_path);
		memset(file_shell, 0x00, sizeof(file_shell));
		sprintf(file_shell, "%s/%s", dst_path, file_name);
		pub_log_debug("[%s][%d] file_shell=[%s]", __FILE__, __LINE__, file_shell);
		result = access(file_shell, F_OK);
		if (result < 0)
		{
			strcpy(reply, "E009");
			pub_log_error("[%s][%d] file_shell[%s] is not found", __FILE__, __LINE__, file_shell);
			goto ErrExit;
		}
		
		pub_log_debug("[%s][%d] the filename is [%s]", __FILE__, __LINE__, file_name);
		sprintf(cmd,"sh %s/%s.sh %s %s %s", dst_path, db_type, dst_path, msgfile, file_name);
	}
	
	pub_log_debug("%s, %d the cmd is [%s]", __FILE__, __LINE__, cmd);
	
	result = agt_system(cmd);
	if (result)
	{
		if (result == -1 || result == 127)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Header.ReturnCode");
			loc_set_zd_data(vars, path, "E002");

			memset(path, 0x0, sizeof(path));
			sprintf(path, ".TradeRecord.Header.ReturnMessage");
			loc_set_zd_data(vars, path, "Transaction processes failure");
			goto ErrExit;
		}

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.HasError");
		loc_set_zd_data(vars, path, "1");

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.FileBody");
		loc_set_zd_data(vars, path, msgfile);
	}
	else
	{
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.HasError");
		loc_set_zd_data(vars, path, "0");

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.FileBody");
		loc_set_zd_data(vars, path, msgfile);
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
