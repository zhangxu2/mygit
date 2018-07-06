/*************************************************
  文 件 名:  flw_sp2071.c                        **
  功能描述:  上传安装包                          **
  作    者:  邹佩                                **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

static int get_install_dir(char *rcv_path, char *dir_name)
{
	int	i = 0;
	int	result = 0;
	glob_t	glob_buf;
	char	pattern[256];
	struct stat	stat_buf;

	if (rcv_path == NULL || dir_name == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(&glob_buf, sizeof(glob_buf));
	pub_mem_memzero(pattern, sizeof(pattern));
	sprintf(pattern, "%s/*", rcv_path);
	pub_log_debug("[%s][%d] the rcv_path[%s]", __FILE__, __LINE__, rcv_path);

	glob_buf.gl_offs = 0;
	result = glob(pattern, GLOB_DOOFFS, NULL, &glob_buf);
	if (result != 0)
	{
		pub_log_error("%s, %d, glob pattern[%s] error[%d][$%s]!"
				, __FILE__, __LINE__, pattern, errno, strerror(errno));
		globfree(&glob_buf);
		return -1;
	}

	for (i = 0; i < glob_buf.gl_pathc; i++)
	{
		pub_mem_memzero(&stat_buf, sizeof(stat_buf));
		result = stat(glob_buf.gl_pathv[i], &stat_buf);
		if (result != 0)
		{
			pub_log_error("%s, %d, stat path[%s] error[%d][%s]!"
					, __FILE__, __LINE__, glob_buf.gl_pathv[i], errno, strerror(errno));
			globfree(&glob_buf);
			return -1;
		}

		if (S_ISDIR(stat_buf.st_mode))
		{
			sprintf(dir_name, "%s", glob_buf.gl_pathv[i]);
		}
	}

	globfree(&glob_buf);
	return 0;
}

static int get_rcv_name(char *rcv_path, char *rcv_name)
{
	int	result = 0;
	char	pattern[256];
	glob_t	glob_buf;

	if (rcv_path == NULL || rcv_name == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(&glob_buf, sizeof(glob_buf));
	pub_mem_memzero(pattern, sizeof(pattern));
	sprintf(pattern, "%s/*", rcv_path);
	glob_buf.gl_offs = 0;

	result = glob(pattern, GLOB_DOOFFS, NULL, &glob_buf);
	if (result != 0)
	{
		pub_log_error("%s, %d, glob pattern[%s] error[%d][$%s]!"
				, __FILE__, __LINE__, pattern, errno, strerror(errno));
		globfree(&glob_buf);
		return -1;
	}

	if (glob_buf.gl_pathc != 1)
	{
		pub_log_error("%s, %d, file's num in dir[%s] is more than 1!",__FILE__,__LINE__,rcv_path);
		globfree(&glob_buf);
		return -1;
	}

	sprintf(rcv_name, "%s", glob_buf.gl_pathv[0]);
	globfree(&glob_buf);
	return 0;
}

static int check_env(sw_loc_vars_t *vars, char *res_msg)
{
	int	result = 0;
	char	buf[128];
	char	cwd[256];
	char	cmd[256];
	char	rcv_path[256];
	char	rcv_name[256];
	char	dir_name[256];

	pub_mem_memzero(rcv_path, sizeof(rcv_path));
	loc_get_zd_data(vars, "$rcv_files_error", rcv_path);
	if (strlen(rcv_path) != 0 && rcv_path[0] == '1')
	{
		pub_log_error("%s, %d, $rcv_files_error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(rcv_path, sizeof(rcv_path));
	loc_get_zd_data(vars, "$rcv_path", rcv_path);
	if (strlen(rcv_path) == 0)
	{
		pub_log_error("%s, %d, No $rcv_path!", __FILE__, __LINE__);
		return -1;
	}

	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "chmod -R 777 %s", rcv_path);
	pub_log_info("%s, %d the cmd[%s]", __FILE__, __LINE__, buf);
	agt_system(buf);

	/*接收文件名*/
	pub_mem_memzero(rcv_name, sizeof(rcv_name));
	result = get_rcv_name(rcv_path, rcv_name);
	if (result != 0)
	{
		pub_log_error("%s, %d, get_rcv_name from rcv_path[%d] error!"
				, __FILE__, __LINE__, rcv_path);
		return -1;
	}
	pub_log_info("%s, %d rcv_name [%s]", __FILE__, __LINE__, rcv_name);

	pub_mem_memzero(cwd, sizeof(cwd));
	sprintf(cwd, "%s", getenv("SWWORK"));

	result = chdir(rcv_path);
	if (result != 0)
	{
		pub_log_error("%s, %d, chdir rcv_path[%s] error[%d][%s]!"
				, __FILE__, __LINE__, errno, strerror(errno));
		chdir(cwd);
		return -1;
	}

	/*解压安装包*/
	pub_mem_memzero(cmd, sizeof(cmd));
	sprintf(cmd, "gunzip -c %s|tar -xvf - 1>/dev/null 2>/dev/null", rcv_name);
	pub_log_debug("%s, %d the cmd [%s]", __FILE__, __LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0 && errno != ECHILD)
	{
		pub_log_error("%s, %d, exec cmd[%s] error[%d][%s]!"
				, __FILE__, __LINE__, cmd, errno, strerror(errno));
		chdir(cwd);
		return -1;
	}

	pub_mem_memzero(dir_name, sizeof(dir_name));
	result = get_install_dir(rcv_path, dir_name);
	if (result != 0)
	{
		pub_log_error("%s, %d, get_install_dir[%s] error!"
				, __FILE__, __LINE__, rcv_path);
		chdir(cwd);
		return -1;
	}

	/*设置安装包路径*/
	loc_set_zd_data(vars, ".TradeRecord.Response.InstallDir", dir_name);
	
	chdir(cwd);
	return 0;
}

int sp2071(sw_loc_vars_t *vars)
{
	int	result = 0;
	char	flock_dir[256];
	char	reply[8];
	char	res_msg[256];
	char	filename[256];
	FILE	*fp = NULL;
	
	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	pub_log_info("%s, %d, 开始检查安装包类型!", __FILE__, __LINE__);
	result = check_env(vars, res_msg);
	if (result != 0)
	{
		pub_log_error("%s, %d, 检查安装包类型错误!", __FILE__, __LINE__);
		strcpy(res_msg, "检查安装包类型错误");
		strcpy(reply, "E999");
		goto ErrExit;
	}

	memset(flock_dir, 0x0, sizeof(flock_dir));
	sprintf(flock_dir, "%s/tmp/flock", getenv("HOME"));
	pub_file_check_dir(flock_dir);

	pub_log_info("%s, %d, 检查安装包类型完成!", __FILE__, __LINE__);
	
	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/tmp/flock/CHECK_DEPLOY_CONFLICT.loc", getenv("HOME"));
	if (!access(filename, F_OK))
	{
		pub_log_error("[%s][%d]产品正在发布!", __FILE__, __LINE__);
		strcpy(reply, "E999");
		strcpy(res_msg, "产品正在发布");
		goto ErrExit;
	}
	else
	{
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] 打开文件失败", __FILE__, __LINE__);
			strcpy(reply, "E999");
			strcpy(res_msg, "创建文件失败");
			goto ErrExit;
		}
		fclose(fp);
	}
	
OkExit:

	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "");
	pub_log_debug("[%s][%d] [%s]OK EXIT", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return 0;

ErrExit:
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "");
	pub_log_debug("[%s][%d] [%s]ERR EXIT", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return -1;
}
