/*************************************************
  文 件 名:  flw_sp2009.c                        **
  功能描述:  mtype信息获取                       **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

static int get_proc_info(sw_loc_vars_t *vars, char *option, char *proc_name)
{
	int 	i = 0, j = 0;
	int 	location = 0;
	int 	fd = 0;
	int 	pagecnt = 0;
	int 	pageidx = 0;
	int 	pagesum = 0;
	char	filename[256];
	char	path[256];
	char	buf[256];
	char	time[32];
	char	item[10][256];
	FILE	*fp = NULL;

	memset(filename, 0x0, sizeof(filename));
	snprintf(filename, sizeof(filename)-1, "%s/tmp/agtmon/procinfo.txt", getenv("SWWORK"));
	if (access(filename, F_OK))
	{
		return 0;
	}

	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open mtype info file error %s", __FILE__, __LINE__, filename);
		return -1;
	}

	fd = fileno(fp);
	pub_lock_fd(fd);

	if (option[0] == 'T')
	{
		while(!feof(fp))
		{
			memset(buf, 0x0, sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			if (strstr(buf, "TOTAL:") != NULL)
			{
				location = ftell(fp) - strlen(buf);
			}
		}
		fseek(fp, location, SEEK_SET);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Request.PageCount");
		loc_get_zd_data(vars, path, buf);
		pagecnt = atoi(buf);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Request.PageIndex");
		loc_get_zd_data(vars, path, buf);
		pageidx = atoi(buf);
		if (pagecnt == 0 || pageidx == 0)
		{
			pagecnt = 1;
		}
	}

	i = 0;
	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);

		if (strstr(buf, "TOTAL:") != NULL)
		{
			memset(item, 0x0, sizeof(item));
			agt_str_parse(buf, item, 10);
			memset(time, 0x0, sizeof(time));
			strncpy(time, item[1], sizeof(time)-1);
		}


		if (option[0] == 'D')
		{
			if (strstr(buf, proc_name) == NULL)
			{
				continue;
			}
		}
		else if (option[0] == 'T')
		{
			
			if (strstr(buf, "TOTAL:") != NULL || strlen(buf) < 2)
			{
				continue;
			}
			
			j++;
			if (pagecnt != 1 && (j <= pagecnt*(pageidx-1) || j >= pagecnt*pageidx))
			{
				continue;
			}
		}
		memset(item, 0x0, sizeof(item));
		agt_str_parse(buf, item, 10);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).NAME", i);
		loc_set_zd_data(vars, path, item[0]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).PID", i);
		loc_set_zd_data(vars, path, item[1]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).CPU", i);
		loc_set_zd_data(vars, path, item[2]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).MEM", i);
		loc_set_zd_data(vars, path, item[3]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).VSZ", i);
		loc_set_zd_data(vars, path, item[4]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).RSS", i);
		loc_set_zd_data(vars, path, item[5]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).STAT", i);
		loc_set_zd_data(vars, path, item[6]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Procs.Proc(%d).TIME", i);
		loc_set_zd_data(vars, path, time);
		i++;
	}

	if (option[0] == 'T')
	{
		pagesum = (j % pagecnt)?(j / pagecnt + 1):(j / pagecnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.PageSum");
		loc_set_zd_int(vars, path, pagesum);
	}

	pub_unlock_fd(fd);
	fclose(fp);

	return 0;
}

/*2009*/
int sp2009(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	char	res_msg[256];
	char	reply[256];
	char 	option[32];
	char	name[32];

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

	memset(option, 0x0, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option );

	memset(name, 0x0, sizeof(name));
	loc_get_zd_data(vars, ".TradeRecord.Request.NAME", name);

	ret  = get_proc_info(vars, option, name);
	if (ret < 0)
	{
		strcpy(reply, "E005");
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");
	run_destroy();

	return SW_OK;

ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	run_destroy();
	return SW_ERROR;
}

