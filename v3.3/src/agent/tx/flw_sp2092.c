/*************************************************
  文 件 名:  flw_sp2092.c                        **
  功能描述:  上传文件/目录                       **
  作    者:  linpanfei                           **
  完成日期:  20160803                            **
 *************************************************/
#include "agent_comm.h"

#define F_UP	"UP"
#define F_DOWN	"DOWN"
#define F_DEL	"DEL"
#define F_LIST	"LIST"

static int get_send_path(char *tmp_dir, char *send_path)
{
	int 	result = -1;
	char	pattern[256];
	glob_t	glob_buf;

	if (tmp_dir == NULL || send_path == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(&glob_buf, sizeof(glob_buf));
	pub_mem_memzero(pattern, sizeof(pattern));
	sprintf(pattern, "%s/*", tmp_dir);
	glob_buf.gl_offs = 0;
	result = glob(pattern, GLOB_DOOFFS, NULL, &glob_buf);
	if (result != 0)
	{
		pub_log_error("[%s][%d] glob pattern[%s] error[%d][$%s]!"
				, __FILE__, __LINE__, pattern, errno, strerror(errno));
		globfree(&glob_buf);
		return -1;
	}

	if (glob_buf.gl_pathc != 1)
	{
		pub_log_error("[%s][%d] file's num in dir[%s] is more than 1!", __FILE__, __LINE__, tmp_dir);
		globfree(&glob_buf);
		return -1;
	}

	sprintf(send_path, "%s", glob_buf.gl_pathv[0]);

	globfree(&glob_buf);
	return 0;
}

static int cp_files(sw_loc_vars_t *vars, char *tmp_dir)
{
	int 	i = 0;
	int 	result = -1;
	char	cmd[256];
	char	prefix[256];
	char	xml_name[256];
	char	dst_path[256];
	char	file_path[256];
	char	*env = NULL;
	struct stat	stat_buf;

	env = getenv("SWWORK");
	if (env == NULL)
	{
		pub_log_error("[%s][%d] No SWWORK!", __FILE__, __LINE__);
		return -1;
	}

	if (vars == NULL || tmp_dir == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	while (1)
	{
		pub_mem_memzero(file_path, sizeof(file_path));
		pub_mem_memzero(xml_name, sizeof(xml_name));

		sprintf(xml_name, ".TradeRecord.Request.FilePath(%d)", i);
		loc_get_zd_data(vars, xml_name, file_path);
		if (strlen(file_path) == 0)
		{
			break;
		}

		pub_mem_memzero(&stat_buf, sizeof(stat_buf));
		result = stat(file_path, &stat_buf);
		if (result != 0)
		{
			pub_log_error("[%s][%d] stat file[%s] error[%d][%s]!"
					, __FILE__, __LINE__, file_path, errno, strerror(errno));
			return -1;
		}

		pub_mem_memzero(dst_path, sizeof(dst_path));
		sprintf(dst_path, "%s/%s", tmp_dir, file_path);

		pub_log_debug("%s, %d, file[%s] will be send!"
				, __FILE__, __LINE__, dst_path);

		pub_mem_memzero(prefix, sizeof(prefix));
		sprintf(prefix, "%s", dirname(dst_path));

		result = pub_file_check_dir(prefix);
		if (result != 0)
		{
			pub_log_error("[%s][%d] mkpath [%s] error!"
					, __FILE__, __LINE__, prefix);
		}

		pub_mem_memzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp -rf %s %s/", file_path, prefix);
		result = agt_system(cmd);
		if (result != 0 && errno != ECHILD) 
		{
			pub_log_error("[%s][%d] run cmd[%s] error!"
					, __FILE__, __LINE__, cmd);
			return -1;
		}

		pub_log_debug("[%s][%d] [%d] run cmd[%s] OK!", __FILE__, __LINE__, i, cmd);
		i++;
	}

	return 0;
}

static int del_files(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	result = -1;
	char	cmd[256];
	char	xml_name[256];
	char	file_path[256];

	while (1)
	{
		pub_mem_memzero(xml_name, sizeof(xml_name));
		sprintf(xml_name, ".TradeRecord.Request.FilePath(%d)", i);

		pub_mem_memzero(file_path, sizeof(file_path));
		loc_get_zd_data(vars, xml_name, file_path);

		pub_mem_memzero(cmd, sizeof(cmd));
		sprintf(cmd, "rm -rf %s", file_path);
		result = agt_system(cmd);
		if (result != 0 && errno != ECHILD)
		{
			pub_log_error("[%s][%d] run cmd[%s] error!", __FILE__, __LINE__, cmd);
			return -1;
		}

		pub_log_debug("[%s][%d] delete file[%s] OK!", __FILE__, __LINE__, file_path);

		if (strlen(file_path) == 0)
		{
			break;
		}

		i++;
	}

	return 0;
}

static int list_files(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	result = -1;
	char	ext[16];
	char	pattern[256];
	char	dir_name[256];
	char	xml_name[256];
	glob_t	glob_buf;

	pub_mem_memzero(dir_name, sizeof(dir_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.FilePath", dir_name);
	if (strlen(dir_name) == 0)
	{
		pub_log_error("[%s][%d] No .TradeRecord.Request.FilePath!"
				, __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(ext, sizeof(ext));
	loc_get_zd_data(vars, ".TradeRecord.Request.FileExt", ext);

	result = access(dir_name, F_OK);
	if (result != 0)
	{
		pub_log_error("[%s][%d] access dir[%s] error[%d][%s]!"
				, __FILE__, __LINE__, dir_name, errno, strerror(errno));
		return -1;
	}

	pub_mem_memzero(pattern, sizeof(pattern));
	if (strlen(ext) == 0)
	{
		sprintf(pattern, "%s/*", dir_name);
	}
	else
	{
		sprintf(pattern, "%s/*.%s", dir_name, ext);
	}

	pub_mem_memzero(&glob_buf, sizeof(glob_buf));
	glob_buf.gl_offs = 0;
	result = glob(pattern, GLOB_DOOFFS, NULL, &glob_buf);
	if (result != 0)
	{
		pub_log_error("[%s][%d] glob pattern[%s] error[%d][%s]!", __FILE__, __LINE__, pattern, errno, strerror(errno));
		globfree(&glob_buf);
		return -1;
	}

	for (i = 0; i < glob_buf.gl_pathc; i++)
	{
		pub_mem_memzero(xml_name, sizeof(xml_name));
		sprintf(xml_name, ".TradeRecord.Response.FilePath(%d)", i);
		loc_set_zd_data(vars, xml_name, glob_buf.gl_pathv[i]);
		pub_log_debug("[%s][%d] file[%s]", __FILE__, __LINE__, glob_buf.gl_pathv[i]);
	}

	globfree(&glob_buf);
	return 0;
}

int sp2092(sw_loc_vars_t *vars)
{
	int 	result = -1;
	char	opt[16];
	char	cmd[512];
	char    reply[8];
	char	rcv_error[8];
	char	res_msg[128];
	char	rcv_path[256];
	char	dst_path[256];
	char	*env = NULL;
	char    *p = NULL;
	char    *q = NULL;
	char	tmp_dir[256];
	char	send_path[256];
	struct timeval	time_val;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	pub_mem_memzero(opt, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Opt", opt);

	env = getenv("SWWORK");

	if (strcmp(opt, F_UP) == 0)
	{
		/*up files*/
		result = chdir(getenv("HOME"));
		if (result != 0)
		{
			pub_log_error("[%s][%d] chdir error!", __FILE__, __LINE__);
			goto ErrExit;
		}
		pub_mem_memzero(rcv_error, sizeof(rcv_error));
		loc_get_zd_data(vars, "$rcv_files_error", rcv_error);
		if (strlen(rcv_error) != 0 && rcv_error[0] == '1')
		{
			pub_log_error("[%s][%d] $rcv_files_error!", __FILE__, __LINE__);
			strcpy(reply, "E012");
			goto ErrExit;
		}

		pub_mem_memzero(rcv_path, sizeof(rcv_path));
		loc_get_zd_data(vars, "$rcv_path", rcv_path);
		if (strlen(rcv_path) == 0)
		{
			strcpy(reply, "E012");
			pub_log_error("[%s][%d] No $rcv_path!", __FILE__, __LINE__);
			goto ErrExit;
		}

		pub_mem_memzero(dst_path, sizeof(dst_path));
		loc_get_zd_data(vars, ".TradeRecord.Request.FilePath", dst_path);
		if (strlen(dst_path) == 0)
		{
			pub_log_error("[%s][%d] No .TradeRecord.Request.FilePath!", __FILE__, __LINE__);
			sprintf(cmd, "rm -rf %s", rcv_path);
			pub_log_debug("[%s][%d] cmd[%s]", __FILE__, __LINE__, cmd);
			agt_system(cmd);
			strcpy(reply, "E012");
			goto ErrExit;
		}

		p = strstr(dst_path, "/products/");
		if ( p != NULL)
		{
			
			p = p + strlen("/products/");
			q = strstr(p, "/");
			pub_log_debug("[%s][%d] q[%s]", __FILE__, __LINE__, q);
			if ((q != NULL) && (strncmp(q, "/components", strlen("/components")) == 0))
			{
				del_files(vars);
			}
		}

		result = pub_file_check_dir(dst_path);
		if (result != 0)
		{
			pub_log_error("[%s][%d] mkpath[%s] error!", __FILE__, __LINE__, dst_path);
			sprintf(cmd, "rm -rf %s", rcv_path);
			pub_log_debug("[%s][%d] cmd[%s]", __FILE__, __LINE__, cmd);
			agt_system(cmd);
			strcpy(reply, "E019");
			goto ErrExit;
		}

		pub_mem_memzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp -rf %s/* %s", rcv_path, dst_path);
		result = agt_system(cmd);
		if (result != 0 && errno != ECHILD)
		{
			pub_log_error("[%s][%d] exec cmd[%s] error[%d][%s]!"
					, __FILE__, __LINE__, cmd, errno, strerror(errno));
			sprintf(cmd, "rm -rf %s", rcv_path);
			pub_log_debug("[%s][%d] cmd[%s]", __FILE__, __LINE__, cmd);
			agt_system(cmd);
			strcpy(reply, "E010");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d] UP file[%s] to dir[%s] OK!"
				, __FILE__, __LINE__, rcv_path, dst_path);

		pub_mem_memzero(cmd, sizeof(cmd));
		sprintf(cmd, "rm -rf %s", rcv_path);
		pub_log_debug("[%s][%d] cmd[%s]", __FILE__, __LINE__, cmd);
		result = agt_system(cmd);
		if (result != 0 && errno != ECHILD)
		{
			pub_log_error("[%s][%d] run cmd[%s] error[%d][%s]!"
					, __FILE__, __LINE__, cmd, errno, strerror(errno));
			strcpy(reply, "E010");
			goto ErrExit;
		}

	}
	else if (strcmp(opt, F_DOWN) == 0)
	{
		/*down files*/
		pub_mem_memzero(&time_val, sizeof(time_val));
		gettimeofday(&time_val, NULL);
		pub_mem_memzero(tmp_dir, sizeof(tmp_dir));
		sprintf(tmp_dir, "%s/dat/send_%ld/", env, time_val.tv_usec);

		result = pub_file_check_dir(tmp_dir);
		if (result != 0)
		{
			strcpy(reply, "E019");
			pub_log_error("[%s][%d] mkpath dir[%s] error!", __FILE__, __LINE__);
			goto ErrExit;
		}

		pub_log_debug("%s, %d, mkpath dir[%s] OK!", __FILE__, __LINE__, tmp_dir);

		result = cp_files(vars, tmp_dir);
		if (result != 0)
		{
			strcpy(reply, "E010");
			pub_log_error("[%s][%d] cp_files to [%s] fail!", __FILE__, __LINE__, tmp_dir);
			goto ErrExit;
		}

		pub_log_debug("%s, %d, cp_files to dir[%s] OK!", __FILE__, __LINE__, tmp_dir);
		pub_mem_memzero(send_path, sizeof(send_path));
		result = get_send_path(tmp_dir, send_path);
		if (result != 0)
		{
			strcpy(reply, "E010");
			pub_log_error("[%s][%d] get_send_path from path[%s] error!"
					, __FILE__, __LINE__, tmp_dir);
			goto ErrExit;
		}

		result = access(send_path, F_OK);
		if (result != 0)
		{
			strcpy(reply, "E026");
			pub_log_error("[%s][%d] access path[%s] error!"
					, __FILE__, __LINE__, send_path);
			goto ErrExit;
		}

		loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
		loc_set_zd_data(vars, "$send_path", send_path);
		loc_set_zd_data(vars, "$del_send_path", tmp_dir);

		pub_log_debug("[%s][%d] DOWN path[%s] OK!", __FILE__,__LINE__,send_path);
	}
	else if (strcmp(opt, F_DEL) == 0)
	{
		/*delete files*/
		result = del_files(vars);
		if (result != 0)
		{
			pub_log_error("[%s][%d] delete files error!", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(opt, F_LIST) == 0)
	{
		/*list files*/
		result = list_files(vars);
		if (result != 0)
		{
			pub_log_error("[%s][%d] list_files error!", __FILE__, __LINE__);
			strcpy(reply, "E999");
			goto ErrExit;
		}
		pub_log_debug("[%s][%d] list files OK!", __FILE__, __LINE__);
	}
	else
	{
		pub_log_error("[%s][%d] Unknown file opt[%s]!", __FILE__, __LINE__, opt);
		strcpy(reply, "E012");
		goto ErrExit;
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "E999");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
