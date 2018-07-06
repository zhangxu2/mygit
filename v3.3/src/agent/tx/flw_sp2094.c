/**************************************************
  文 件 名:  flw_sp2094.c                        **
  功能描述:  远程编译                            **
**************************************************/

#include "agent_comm.h"

int sp2094(sw_loc_vars_t *vars)
{
	int		ret = -1;
	char	path[256];
	char	file_dir[128];
	char	cmd[1024];
	char	msg_file[128];
	char	filename[128];
	char	makefile[128];
	char	*file_path = NULL;
	char	reply[8];
	char	res_msg[256];
	char	opt[32];
	char	type[8];

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

#ifdef	AIX
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ErrorInfo.GUN");
	loc_set_zd_data(vars, path, "0");
#endif

#ifdef	LINUX
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ErrorInfo.GUN");
	loc_set_zd_data(vars, path, "1");
#endif

#ifdef SOLARIS
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ErrorInfo.GUN");
	loc_set_zd_data(vars, path, "2");
#endif
	pub_mem_memzero(opt, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Opt", opt);

	if (strcmp(opt, "OS") == 0)
	{
		pub_log_debug( "%s, %d Get os type!", __FILE__, __LINE__, opt);

		goto OkExit;
	}

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(file_dir, sizeof(file_dir));
	sprintf(path, "%s", ".TradeRecord.Request.FileDir");
	loc_get_zd_data(vars, path, file_dir);
	if (file_dir == NULL || pub_str_strlen(file_dir) == 0)
	{
		pub_log_error( "[%s][%d] %s is null", __FILE__, __LINE__, path);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(makefile, sizeof(makefile));
	sprintf(path, "%s", ".TradeRecord.Request.MakefileName");
	loc_get_zd_data(vars, path, makefile);
	if (makefile == NULL || pub_str_strlen(makefile) == 0)
	{
		pub_log_error( "[%s][%d] %s is null", __FILE__, __LINE__, path);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(filename, sizeof(filename));
	sprintf(path, "%s", ".TradeRecord.Request.FileName");
	loc_get_zd_data(vars, path, filename);
	if (filename == NULL || strlen(filename) == 0)
	{
		pub_log_error( "[%s][%d] %s is null", __FILE__, __LINE__, path);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(type, 0x0, sizeof(type));
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, "%s", ".TradeRecord.Request.Type");
	loc_get_zd_data(vars, path, type);
	if (type[0] == '\0')
	{
		pub_log_error( "[%s][%d] %s is null", __FILE__, __LINE__, path);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	file_path = getenv("SWWORK");
	if (file_path == NULL)
	{
		pub_log_error( "[%s][%d],take environment variable error!errno=[%d]:[%s]", 
			__FILE__, __LINE__, errno, strerror(errno));
		strcpy(reply, "E014");
		goto ErrExit;
	}


	pub_mem_memzero(msg_file, sizeof(msg_file));
	sprintf(msg_file, "%s/tmp/%s_error.txt", file_path, filename);
	pub_log_debug( "step1:%s", msg_file);
	pub_log_debug( "[%s][%d] filename=[%s] type=[%s]", __FILE__, __LINE__, filename, type);

	pub_mem_memzero(cmd, sizeof(cmd));
	if (strcmp(filename, "all") == 0)
	{
		sprintf(cmd, "sh %s/sbin/make.sh %s %s > %s  2>&1", getenv("SWHOME"), file_dir, makefile, msg_file);
	}
	else if (type[0] == '1')
	{
		/*** 单个组件 ***/
		sprintf(cmd, "sh %s/sbin/make.sh %s %s %s.o > %s  2>&1", getenv("SWHOME"), file_dir, makefile, filename, msg_file);
	}
	else if (type[0] == '2')
	{
		/*** 单个原子交易 ***/
		sprintf(cmd, "sh %s/sbin/make.sh %s %s %s.so > %s  2>&1", getenv("SWHOME"), file_dir, makefile, filename, msg_file);
	}
	pub_log_debug( "cmd=%s", cmd);
	ret = agt_system(cmd);
	if (ret)
	{
		if (ret == -1 || ret == 127)
		{
			strcpy(reply, "E010");
			goto ErrExit;
		}

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.HasError");
		loc_set_zd_data(vars, path, "1");

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.FileBody");
		loc_set_zd_data(vars, path, msg_file);
		pub_log_debug( "step2:%s", msg_file);
	}
	else
	{
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.HasError");
		loc_set_zd_data(vars, path, "0");

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ErrorInfo.FileBody");
		loc_set_zd_data(vars, path, msg_file);
		pub_log_debug( "step3:%s", msg_file);
	}

	goto OkExit;

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "successful !!", sizeof(res_msg) - 1);
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
