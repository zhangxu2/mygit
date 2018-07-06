/************************************************
 文 件 名:  flw_sp2003.c                        **
 功能描述:  内存信息                            **
 作    者:  薛辉                                **
 完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

int get_mem_info(sw_loc_vars_t *vars)
{
	int 	fd = 0;
	char	buf[256];
	char	tmp[32];
	char	path[256];
	char	filepath[256];
	char	item[10][256];
	FILE	*fp = NULL;
	sw_buf_t use;
	sw_buf_t times;
	
	pub_buf_init(&use);
	pub_buf_init(&times);
	
	memset(filepath, 0x0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath) - 1, "%s/tmp/agtmon/meminfo.txt", getenv("SWWORK"));
	if (access(filepath, F_OK))
	{
		return 0;
	}

	fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open mem info file error %s", __FILE__, __LINE__, filepath);
		return -1;
	}
	
	fd = fileno(fp);
	pub_lock_fd(fd);
	
	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		if (strstr(buf, "TIME:") == NULL)
		{
			continue;
		}
		memset(item, 0x0, sizeof(item));
		agt_str_parse(buf, item, 10);

		memset(tmp, 0x0, sizeof(tmp));
		sprintf(tmp, "%s|", item[0]);
		buf_append(&times, tmp, strlen(tmp));
		
		memset(tmp, 0x0, sizeof(tmp));
		sprintf(tmp, "%.2f|", (atoll(item[3])*1.0/1024/1024) / (atoll(item[1])*1.0/1024/1024) * 100);
		buf_append(&use, tmp, strlen(tmp));	
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Memory.Time");
	loc_set_zd_data(vars, path, times.data);
	pub_buf_clear(&times);
	
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Memory.MemoryUsed");
	loc_set_zd_data(vars, path, use.data);
	pub_buf_clear(&use);

	pub_unlock_fd(fd);
	fclose(fp);

	return 0;
}

/*2003*/
int sp2003(sw_loc_vars_t *vars)
{
	int	ret = SW_ERROR;
	char	res_msg[256];
	char	reply[16];
	
	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	ret = get_mem_info(vars);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Take the memory information failure!", __FILE__, __LINE__);
		strcpy(reply, "E007");
		goto ErrExit;
	}

	ret = get_proc_sort(vars, "MEM");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Take the memory information failure!"
			, __FILE__, __LINE__);
		strcpy(reply, "E007");
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");
	return SW_OK;
ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}
