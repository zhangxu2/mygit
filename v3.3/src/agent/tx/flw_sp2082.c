/*************************************************
  文 件 名:  flw_sp2082.c                        **
  功能描述:  传输文件                            **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

int sp2082(sw_loc_vars_t* vars)
{
	int 	result = -1;
	char	buf[256];
	char	cmd[512];
	char	reply[16];
	char	soption[8];
	char	res_msg[256];
	char	rcv_path[256];
	char	file_path[256];
	char	file_name[256];

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

	loc_get_zd_data(vars, ".TradeRecord.Request.Option", soption);
	if (strcmp(soption, "SET") != 0 && strcmp(soption, "GET") != 0)
	{
		pub_log_error("[%s][%d] option is error[%s]", __FILE__, __LINE__, soption);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	pub_mem_memzero(buf, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Filepath", buf);
	if (strlen(buf) == 0)
	{
		pub_log_error("[%s][%d] no .TradeRecord.Request.Filepath", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	pub_mem_memzero(file_path, sizeof(file_path));
	sprintf(file_path, "%s/%s", getenv("SWWORK"), buf);

	pub_mem_memzero(file_name, sizeof(file_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.Filename", file_name);
	if (strlen(file_name) == 0)
	{
		pub_log_error("[%s][%d] no .TradeRecord.Request.Filename", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	if (strcmp(soption, "SET") == 0)
	{	
		pub_mem_memzero(buf, sizeof(buf));
		loc_get_zd_data(vars, "$rcv_files_error", buf);
		if (strlen(buf) != 0 && buf[0] == '1')
		{
			pub_log_error("[%s][%d] $rcv_files_error!", __FILE__, __LINE__);
			strcpy(reply, "E012");
			goto ErrExit;
		}

		pub_mem_memzero(rcv_path, sizeof(rcv_path));
		loc_get_zd_data(vars, "$rcv_path", rcv_path);
		if (strlen(rcv_path) == 0)
		{
			pub_log_error("[%s][%d] No $rcv_path!", __FILE__, __LINE__);
			strcpy(reply, "E012");
			goto ErrExit;
		}

		result  = pub_file_check_dir(file_path);
		if (result != 0)
		{
			pub_log_error("[%s][%d] mkpath[%s] error!", __FILE__, __LINE__, file_path);
			remove(rcv_path);
			strcpy(reply, "E019");
			goto ErrExit;
		}

		pub_mem_memzero(cmd, sizeof(cmd));
		sprintf(cmd, "mv -f %s/%s %s", rcv_path, file_name, file_path);
		pub_log_debug("[%s][%d] cmd=[%s]", __FILE__, __LINE__, cmd);
		result = agt_system(cmd);
		if (result != 0 && errno != ECHILD)
		{
			pub_log_error("%s, %d, exec cmd[%s] error[%d][%s]!"
				, __FILE__, __LINE__, cmd, errno, strerror(errno));
			remove(rcv_path);
			strcpy(reply, "E010");
			goto ErrExit;
		}
		
		remove(rcv_path);
		pub_log_debug("%s, %d, UP file[%s/%s] to dir[%s] OK!"
			, __FILE__, __LINE__, rcv_path, file_name, file_path);
			
		goto OkExit;	
	}
	
	if (strcmp(soption, "GET") == 0)
	{	
		result = access(file_path, F_OK);
		if (result != 0)
		{
			pub_log_info("[%s][%d]file is not exist[%s]", __FILE__, __LINE__, file_path);
			goto OkExit;
		}
		
		strcat(file_path, "/");
		strcat(file_path, file_name);
		loc_set_zd_data(vars, "$send_path", file_path);
		loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
		
		goto OkExit;
	}	
OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}

