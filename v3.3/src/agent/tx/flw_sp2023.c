/*************************************************
 文 件 名:  flw_sp2023.c                         **
  功能描述: 查看某笔交易业务信息                 **
  作    者: 赵强                                 **
  完成日期: 20160801                             **
 *************************************************/
 #include "agent_comm.h"

typedef struct
{
	char	step[8];
	char	cmd[16];
	char	module_sts[8];
	char	tx_type[64];  /*** 交易类型 ***/
	char	module[32];  /*** 模块名称     ***/
	char	begin[32];   /*** 模块进入时间 ***/
	char	end[32];     /*** 模块结束时间 ***/
	char	time[32];
	char	tx_sts[64];   /*** 模块处理状态 ***/
}agt_detail_t;



static char *front_cut(char *begin, char *end)
{
	char 	*p = NULL;
	if(begin >= end)
	{
		return NULL;
	}

	for(p = begin; begin != end && *begin != '\n'; ++begin)
	{
	}

	if(begin != end)
	{
		*begin = '\0';
		++begin;
	}

	return p;
}

static int get_trace_detail_type(char *cmd, char *step)
{
	if (strcmp(cmd, "605") == 0)
	{
		strcat(step, "： 普通任务请求");
	}
	else if (strcmp(cmd, "606") == 0)
	{
		strcat(step, "： 普通任务应答");
	}
	else if (strcmp(cmd, "612") == 0)
	{
		strcat(step, "： 与第三方CALLLSN请求");
	}
	else if (strcmp(cmd, "613") == 0)
	{
		strcat(step, "： 与第三方CALLLSN应答");
	}
	else if (strcmp(cmd, "607") == 0)
	{
		strcat(step, "： 平台CALL处理请求");
	}
	else if (strcmp(cmd, "608") == 0)
	{
		strcat(step, "： 平台CALL处理应答");
	}
	else if (strcmp(cmd, "609") == 0)
	{
		strcat(step, "： 平台POST请求");
	}
	else if (strcmp(cmd, "610") == 0)
	{
		strcat(step, "： 平台POST应答");
	}
	else if (strcmp(cmd, "611") == 0)
	{
		strcat(step, "： 平台LINK请求");
	}
	else if (strcmp(cmd, "614") == 0)
	{
		strcat(step, "： 与第三方LINKLSN请求");
	}
	else if (strcmp(cmd, "615") == 0)
	{
		strcat(step, "： 与第三方LINKLSN应答");
	}
	else if (strcmp(cmd, "616") == 0)
	{
		strcat(step, "： 与第三方POSTLSN请求");
	}
	else if (strcmp(cmd, "617") == 0)
	{
		strcat(step, "： 与第三方POSTLSN应答");
	}
	else if (strcmp(cmd, "618") == 0)
	{
		strcat(step, "： 平台LINKNULL处理");
	}
	else if (strcmp(cmd, "619") == 0)
	{
		strcat(step, "： 与第三方存储转发请求");
	}
	else if (strcmp(cmd, "620") == 0)
	{
		strcat(step, "： 与第三方存储转发应答");
	}
	else if (strcmp(cmd, "633") == 0)
	{
		strcat(step, "： 超时处理");
	}
	else if (strcmp(cmd, "640") == 0)
	{
		strcat(step, "： 平台拒绝服务处理");
	}
	else if (strcmp(cmd, "911") == 0)
	{
		strcat(step, "： 平台任务错误处理");
	}
	return 0;
}


static sw_int_t get_detail_parm(char *buf, char *sep, agt_detail_t *detail)
{
	long long 	begin_time = 0;
	long long 	end_time = 0;

	char    	sbuf[128];
	char    	out[10][256];

	if (strlen(buf) <= 0)
	{
		return SW_ERROR;
	}

	pub_mem_memzero(out, sizeof(out));
	agt_str_split(buf, sep, out, 10);

	strncpy(detail->step, out[0], sizeof(detail->step)-1);
	strncpy(detail->module_sts, out[1], sizeof(detail->module_sts) - 1);
	strncpy(detail->cmd, out[2], sizeof(detail->cmd) - 1);

	pub_mem_memzero(sbuf, sizeof(sbuf));
	trace_get_msg_type(sbuf, atoi(out[2]));
	get_trace_detail_type(out[2], detail->step);

	strncpy(detail->tx_type, sbuf, sizeof(detail->tx_type) - 1);
	strncpy(detail->module, out[3], sizeof(detail->module) - 1);
	begin_time = strtoll(out[4], NULL, 0);
	pub_change_time2(begin_time, detail->begin, 0);
	end_time = strtoll(out[5], NULL, 0);
	if (end_time == 0)
	{
		sprintf(detail->end, "");
	}
	else
	{
		pub_change_time2(end_time, detail->end, 0);
	}
	if (end_time - begin_time < 0)
	{
		sprintf(detail->time, "");
	}
	else
	{
		sprintf(detail->time, "%.3fms", (end_time - begin_time) * 1.0 / 1000 );
	
	}

	pub_mem_memzero(sbuf, sizeof(sbuf));
	trace_get_error(sbuf, atoi(out[6]));
	strncpy(detail->tx_sts, sbuf, sizeof(detail->tx_sts) - 1);
	pub_log_debug("[%s][%d] sTxSts=[%s] sTxType=[%s]", __FILE__, __LINE__, detail->tx_sts, detail->tx_type);


	return SW_OK;
}

static int set_trade_route_info(sw_loc_vars_t *vars,  agt_detail_t *detail, int i)
{

	char path[256];

	pub_mem_memzero(path, sizeof(path));
	loc_set_zd_data(vars, "#modsts", detail->module_sts);


	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).Step", i);
	loc_set_zd_data(vars, path, detail->step);


	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).TrackType", i);
	loc_set_zd_data(vars, path, detail->tx_type);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).Module", i);
	loc_set_zd_data(vars, path, detail->module);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).StartTime", i);
	loc_set_zd_data(vars, path, detail->begin);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).EndTime", i);
	loc_set_zd_data(vars, path, detail->end);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).DealTime", i);
	loc_set_zd_data(vars, path, detail->time);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinessSteps.SysBusinessStep(%d).State", i);
	loc_set_zd_data(vars, path, detail->tx_sts);

	return 0;

}

/* 2203*/
static sw_int_t get_info_by_trace(sw_loc_vars_t *vars,  char *date,  char *trace_no)
{
	int     i = 0;
	int     j = 0;
	int		fd;
	int 	step = 0;
	char	file_path[256];
	char	file_name[256];
	char	buff[512];
	char	find_str[128];
	char	*buffer = NULL;
	char	*p = NULL;
	char	*start = NULL;
	char	*tmp = NULL;
	char	*end = NULL;
	size_t  file_size;
	struct 	stat f_stat;
	struct dirent   *dt;
	agt_detail_t	detail;
	DIR             *dp = NULL;

	memset(file_path, 0x0, sizeof(file_path));
	sprintf(file_path, "%s/tmp/monitor/%s", getenv("SWWORK"), date);

	dp = opendir(file_path);
	if(dp == NULL)
	{
		pub_log_error("open [%s] error", __FILE__, __LINE__, file_path);
		return SW_ERROR;
	}

	memset(find_str, 0x0, sizeof(find_str));
	snprintf(find_str, sizeof(find_str), "|%s|", trace_no);


	while((dt = readdir(dp)) != NULL)
	{

		memset(file_name, 0x0, sizeof(file_name));

		if(strncmp(dt->d_name, "monitor", 7) != 0)
		{
			continue;
		}

		sprintf(file_name, "%s/%s", file_path, dt->d_name);

		if(stat(file_name, &f_stat) == -1)
		{
			pub_log_error("[%s][%d] get file stat error", __FILE__, __LINE__);
			closedir(dp);
			return SW_ERROR;
		}

		file_size = f_stat.st_size;
		if(file_size <= 0)
		{
			continue;
		}
		fd = open(file_name, O_RDWR);
		if(fd < 0)
		{
			pub_log_error("[%s][%d] errno[%d]:[%s]", __FILE__ , __LINE__, errno, strerror(errno));
			closedir(dp);
			return SW_ERROR;		
		}

		buffer = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		if(buffer == NULL)
		{
			pub_log_error("[%s][%d] errno[%d]:[%s]", __FILE__ , __LINE__, errno, strerror(errno));
			closedir(dp);
			close(fd);
			return SW_ERROR;
		}

		p = buffer;
		close(fd);
		
		do
		{
			start = strstr(buffer, find_str);
			if(start == NULL)
			{
				break;
			}
			j = 0;
			i = 0;
			/*确定查找的流水是否是平台流水*/
			while(start[i] != '\n')
			{
				if(start[i] == '|')
				{
					j++;
				}
				i++;
			}
			pub_log_info("[%s][%d] aaa i = %d, j=%d buffe", __FILE__, __LINE__,i,j);
			buffer = start+strlen(find_str);
			/*如果业务流水则处于倒数第三的位置,后面竖线为3个*/
		}
		while(j	<24);
		
		if (start == NULL)
		{
			munmap(p, file_size);
			p = NULL;
			continue;
		}
		
		if(start != NULL)
		{
			end = strstr(start+strlen("TOTAL:"), "TOTAL:");
			if(end == NULL)
			{
				end = p+file_size - 1;
			}


			while((tmp = front_cut(start, end)) != NULL)
			{
				memset(buff, 0x0, sizeof(buff));

				start = start+strlen(tmp)+1;
				if(tmp[0] == '\0')
				{
					continue;
				}
				strncpy(buff, tmp, sizeof(buff));
				if(step == 0)
				{
					step++;
					continue;
				}

				if(step > 0)
				{
					get_detail_parm(buff + 7, "|", &detail);
					set_trade_route_info(vars, &detail, step-1);
				}

				step++;
			}

			munmap(p, file_size);
			p = NULL;

			break;
		}

	}

	if(dt == NULL)
	{
		pub_log_error("[%s][%d]not find the record of date= trace_no!", __FILE__, __LINE__);
		closedir(dp);
		return -1;
	}
	closedir(dp);

	return 0;
}




sw_int_t sp2023(sw_loc_vars_t *vars)
{
	int		ret = SW_ERROR;
	char	reply[8];
	char	date[32];
	char	trace_no[16];
	char	res_msg[256];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	pub_log_info("[%s][%d] [%s]交易 2023 开始处理......", __FILE__, __LINE__, __FUNCTION__);

	memset(trace_no, 0x0, sizeof(trace_no));
	loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", trace_no);
	pub_log_debug("[%s][%d]the serial number=[%s]", __FILE__, __LINE__, trace_no);
	if (pub_str_strlen(trace_no) == 0)
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d] the serial number is NULL!", __FILE__, __LINE__);
		goto ErrExit;
	}

	memset(date, 0x0, sizeof(date));
	loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", date);
	pub_log_debug("[%s][%d]Query Date=[%s]", __FILE__, __LINE__, date);
	if (pub_str_strlen(date) == 0)
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d]Query Date is NULL!", __FILE__, __LINE__);
		goto ErrExit;
	}

	ret = get_info_by_trace(vars, date, trace_no);
	if(ret < 0)
	{
		strcpy(reply, "E023"); 
		goto ErrExit;
	}

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
