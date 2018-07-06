/*************************************************
  文 件 名:  flw_sp2013.c                        **
  功能描述:  查询任务监控信息                    **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

static int get_cur_datetime(char *times,char *out_str)
{
	int	year = 0;
	int	month = 0;
	int	day = 0;
	int	hour = 0;
	int	minute = 0;
	char	tmp[8];
	char	date[16];
	char	*ptr = NULL;

	memset(date, 0x00, sizeof(date));
	pub_time_getdate(date, 1);

	ptr = date;

	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, ptr, 4);
	year = atoi(tmp);

	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, ptr + 4, 2);
	month = atoi(tmp);

	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, ptr + 6, 2);
	day = atoi(tmp);

	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, times, 2);
	hour = atoi(tmp);

	memset(tmp, 0x00, sizeof(tmp));
	strncpy(tmp, times + 2, 2);
	minute = atoi(tmp);

	sprintf(out_str, "%04d年%02d月%02d日 %02d时%02d分", year, month, day, hour, minute);
	return 0;
}

int set_response_info(sw_loc_vars_t *vars, sw_job_item_t *pJob, int i)
{
	char path[256];
	char eweek[64];
	char buf[64];
	char tmp[16];
	time_t  time ;

	pub_mem_memzero(eweek, sizeof(eweek));

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskNo", i);
	loc_set_zd_data(vars, path, pJob->no);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskName", i);
	loc_set_zd_data(vars, path, pJob->job_name);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).IsManual", i);
	loc_set_zd_int(vars, path, pJob->manual);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TimeOut", i);
	loc_set_zd_int(vars, path, pJob->time_out);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskTimeValue", i);
	loc_set_zd_data(vars, path, pJob->time);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskRunName", i);
	loc_set_zd_data(vars, path, pJob->exec);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskType", i);
	memset(buf, 0x0, sizeof(buf));
	agt_get_exec_type(pJob->exec_type, buf);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskWeek", i);
	loc_set_zd_data(vars, path, pJob->week);

	pub_mem_memzero(path, sizeof(path));
	agt_trans_week(pJob->week, eweek);
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskChWeek", i);
	loc_set_zd_data(vars, path, eweek);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskDate", i);
	loc_set_zd_data(vars, path, pJob->date);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskTimeType", i);
	if (pJob->run_type == 0)
	{
		loc_set_zd_data(vars, path, "定时执行");
	}
	else if(pJob->run_type == 1)
	{
		loc_set_zd_data(vars, path, "循环执行");
	}

	pub_mem_memzero(buf, sizeof(buf));
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskLastTime", i);
	time = (time_t)pJob->last_run_time;
	if (time == 0)
	{
		strcpy(buf, " ");
	}
	else
	{
		if (pJob->job_status == JOB_FINISHED)
		{
			if (pJob->run_type == 0)
			{
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(tmp, "%04ld", time);
				get_cur_datetime(tmp, buf);
			}
			else if (pJob->run_type == 1)
			{
				pub_time_change_time(&time, buf, 4);
			}
		}
	}
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskLastResult", i);
	pub_mem_memzero(buf, sizeof(buf));
	if (pJob->job_status == JOB_IDLE)
	{
		strcpy(buf, "未执行");
	}
	else if (pJob->job_status == JOB_DOING)
	{
		strcpy(buf, "正在执行");
	}
	else if (pJob->job_status == JOB_ERROR)
	{
		strcpy(buf, "执行错误");
	}
	else if (pJob->job_status == JOB_FINISHED)
	{
		strcpy(buf, "已执行");
	}
	else if (pJob->job_status == JOB_TIMEOUT)
	{
		strcpy(buf, "超时");
	}
	else
	{
		strcpy(buf, "未知状态");	
	}
	
	loc_set_zd_data(vars, path, buf);

	return 0;
}

int sp2013(sw_loc_vars_t *vars)
{
	int 	i   = 0, j = 0;
	int 	ret = 0;
	int 	index;
	int 	count;
	int 	page_cnt = 0;
	int 	page_idx = 0;
	int 	page_sum = 0;
	char 	buf[128];
	char	reply[32];
	char	res_msg[256];
	sw_job_t *jobs = NULL;

	pub_mem_memzero(buf, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
	page_cnt = atoi(buf);

	pub_mem_memzero(buf, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
	page_idx = atoi(buf);
	if (page_cnt == 0 && page_idx == 0)
	{
		page_cnt = 1;
	}

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	ret = run_link_ext();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] run link error.", __FILE__, __LINE__);
		strcpy(reply, "E001");
		goto ErrExit;
	}

	jobs = job_get_addr();
	if (jobs == NULL)
	{
		pub_log_error("[%s][%d] get job addr error.", __FILE__, __LINE__);
		strcpy(res_msg, "获取任务信息失败");
		goto ErrExit;
	}

	count = jobs->head.cur_cnt;

	j = 0; index = 0;
	for(i = 0; i < count; i++)
	{
		pub_log_debug("[%s][%d] no=[%s] status=[%d] last_run_time=[%d]", __FILE__, __LINE__,
			jobs->job_item[i].no, jobs->job_item[i].job_status, jobs->job_item[i].last_run_time);
		if (jobs->job_item[i].no[0] == '\0')
		{
			continue;
		}
		j++;
		if(page_cnt != 1 && (j <= page_cnt*(page_idx - 1) || j > page_cnt * page_idx))
		{
			continue;	
		}
		set_response_info(vars, &(jobs->job_item[i]), index);
		index++;
	}

	page_sum=(j % page_cnt) ? (j / page_cnt + 1) : (j / page_cnt);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", page_sum);
	loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", buf);
	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", j);
	loc_set_zd_data(vars, ".TradeRecord.Response.Cnt", buf);

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	if (strlen(res_msg) == 0)
	{
		strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	}
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	run_destroy();

	return SW_OK;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	run_destroy();
	return SW_ERROR;
}

