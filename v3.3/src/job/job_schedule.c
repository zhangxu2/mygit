/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swjob 
 *** name    : job_schedule.c
 *** function: job schedule
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "job_schedule.h"
#include "pub_signal.h"
#include "pub_usocket.h"

SW_PROTECTED sw_int_t job_check_file_status(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_check_job(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_reload_job_config(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_match_interval(char *value, char *pattern);
SW_PROTECTED sw_int_t job_match_day(char *value, char *pattern);
SW_PROTECTED sw_int_t job_match_time(sw_int32_t cur_time, sw_char_t *check_value, sw_int32_t time, int *realtime);
SW_PROTECTED sw_int_t job_check_job(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_run(sw_job_cycle_t* cycle, sw_int_t index);
SW_PROTECTED sw_int_t job_check_timeout(sw_job_cycle_t* cycle);

/******************************************************************************
 *** name      : job_schedule
 *** function  : Reload job.xml configure file, schedule job child process.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object	
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_schedule(sw_job_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;

	/*Check job.xml, if job.xml was updated, reload it.*/
	result = job_check_file_status(cycle);
	if(result != SW_OK)
	{
		if(result == -2)
		{
			return SW_OK;
		}
		
		pub_log_error("%s, %d, job_check_file_status error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*If some job child process run timeout, reset its status.*/
	result = job_check_timeout(cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_check_timeout error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*If it is the time to run for some job, run it.*/
	result =  job_check_job(cycle);
	if (result)
	{
		pub_log_error("[%s][%d]JOBÖ´ÐÐ´íÎóÍË³ö", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : job_check_timeout
 *** function  : Check if some job child process run timeout, reset its status.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object	
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_check_timeout(sw_job_cycle_t* cycle)
{
	sw_int32_t	cur_time = 0;
	sw_int32_t	i = 0;

	cur_time = time(NULL);

	for (i = 0; i < cycle->jobs->head.cur_cnt; i++)
	{
		if (cycle->jobs->job_item[i].job_status == JOB_DOING
			&& (cur_time - cycle->jobs->job_item[i].start_time) > cycle->jobs->job_item[i].time_out)
		{
			pub_log_info("Job %s exec[%s] pid %d timeout."
			,cycle->jobs->job_item[i].job_name, cycle->jobs->job_item[i].exec, cycle->jobs->job_item[i].job_pid);

			pub_signal_send(cycle->jobs->job_item[i].job_pid,-9);
			cycle->jobs->job_item[i].job_status = JOB_FINISHED;
			cycle->jobs->job_item[i].job_pid = -1;
			if (cycle->jobs->job_item[i].run_type == PERIOD_TIME)
			{
				cycle->jobs->job_item[i].last_run_time = cur_time;
			}
		}
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : job_run_by_no
 *** function  : Check if some job child process run timeout, reset its status.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 ***        job_no : the job no that you want to run.
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_run_by_no(sw_job_cycle_t* cycle, sw_char_t* job_no)
{
	sw_int32_t	i = 0;
	sw_int_t	result = SW_ERROR;
	sw_job_t   	*job;

	job = cycle->jobs;
	
	for (i = 0; i < job->head.cur_cnt ; i++ )
	{
		if (job->job_item[i].job_name[0] != '\0'
			&&strcmp(job->job_item[i].no, job_no) == 0)
		{
			break;
		}
	}
	
	if (i == job->head.cur_cnt
		|| job->job_item[i].job_name[0] == '\0')
	{
		pub_log_error("%s, %d, Job[%s] not found!", __FILE__, __LINE__, job_no);
		return SW_ERROR;
	}
		
	result = job_run(cycle, i);
	if (result)
	{
		pub_log_error("%s, %d, Run [%s] error."
			, __FILE__, __LINE__,job->job_item[i].job_name);
		return SW_ERROR;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : job_run
 *** function  : Fork a child process and run job.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 ***        index : the job's index in jobinfo shm.
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_run(sw_job_cycle_t* cycle, sw_int_t index)
{
	sw_int32_t	pid = -1;
	sw_int32_t	job_pid = -1;
	sw_fd_t		job_fd = -1;
	sw_char_t	job_path[64];
	sw_job_status_t job_cmd;
	sw_int_t	result = SW_ERROR;
	sw_job_item_t  *job;

	job = cycle->jobs->job_item+index;
	if (!strlen(job->exec))
	{
		pub_log_error("[%s][%d] input sRun_Name error sRun_Name=[%s]"
				, __FILE__, __LINE__, job->exec);
		return SW_ERROR;
	}

	if (job->job_status == JOB_DOING)
	{
		pub_log_error("[%s][%d] Job is doing, sRun_Name=[%s]"
				, __FILE__, __LINE__, job->exec);
		return SW_ERROR;	
	}
	
	job->job_status = JOB_DOING;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("%s, %d, fork fail, errno=[%d]", __FILE__, __LINE__, errno);
		job->job_status = JOB_ERROR;
		if (job->run_type == PERIOD_TIME)
		{
			job->last_run_time = (long)time(NULL);
		}
		
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		/*Child*/
		close(cycle->cmd_fd);
		close(cycle->job_status_fd);
		
		pub_mem_memzero(job_path, sizeof(job_path));
		pub_mem_memzero(&job_cmd, sizeof(job_cmd));
			
		job_pid = getpid();
		job_cmd.job_pid = job_pid;
			
		sprintf(job_path, "job_%s", job->job_name);
		job_fd = udp_bind(job_path);
		if(job_fd < 0)
		{
			pub_log_error("[%s][%d] create socket error!!g_Fd=[%d]", __FILE__, __LINE__, job_fd);
			exit(SW_ERROR);
		}

		if (job->run_type == PERIOD_TIME)
		{
			job_cmd.last_run_time = (long)time(NULL);
		}
		else
		{
			job_cmd.last_run_time = job->last_run_time;
		}
		result = system(job->exec);
		if(result != 0 && errno != ECHILD)
		{
			pub_log_error("[%s][%d] Run job [%s] failed! ret=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, job->exec, result, errno, strerror(errno));
			job_cmd.return_code = JOB_ERROR;
			result = udp_send(job_fd, (char *)&job_cmd,sizeof(job_cmd), cycle->job_sock_path.data);
			if(result < 0)
			{
				pub_log_error("[%s][%d][%d] send job_cmd error, JobName[%s] pid[%d]."
					, __FILE__, __LINE__, errno, job->job_name, job_pid);
			}
			close(job_fd);
			exit(SW_ERROR);
		}
		
		job_cmd.return_code = JOB_FINISHED;
		result = udp_send(job_fd,(char *)&job_cmd,sizeof(job_cmd),cycle->job_sock_path.data);
		if(result < 0)
		{
			pub_log_error("[%s][%d] Send job_cmd error, JobName[%s] pid[%d] error[%s] result[%d] g_sJobSvr[%s].", 
				__FILE__, __LINE__, job->job_name, job_pid, strerror(errno), result, cycle->job_sock_path.data);
		}
		close(job_fd);
		exit(SW_OK);
	}

	/*Parent*/
	pub_log_info("[%s][%d],JOB[%s] pid[%d],after running ",__FILE__,__LINE__, job->exec, pid);
	job->job_pid = pid;
	job->start_time = time(NULL);
	
	return SW_OK;
}

sw_int_t job_match_interval(char *value, char *pattern)
{
	sw_int32_t	i = 0;
	sw_int32_t	j = 0;
	sw_char_t	begin[32];
	sw_char_t	end[32];
	sw_char_t	buf[64];
	sw_char_t	*ptr = NULL;
	
	pub_mem_memzero(begin, sizeof(begin));
	pub_mem_memzero(end, sizeof(end));
	
	strncpy(buf, pattern, sizeof(buf) - 1);
	
	ptr = strstr(buf, "-");
	if (ptr == NULL)
	{
		if (strcmp(value, buf) == 0)
		{
			return 1;
		}
		return 0;
	}

	i = 0;
	j = 0;
	while (buf[i] != '-')
	{
		begin[j++] = buf[i++];
	}
	i++;
	begin[j] = '\0';

	j = 0;
	while (buf[i] != '\0')
	{
		end[j++] = buf[i++];
	}
	end[j] = '\0';
	
	if (strcmp(value, begin) >= 0 && strcmp(value, end) <= 0)
	{
		return 1;
	}

	return 0;
}

/******************************************************************************
 *** name      : job_match_day
 *** function  : Check whether day match pattern.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  day : day value
 		week mode: 1-7
 		date mode: yyyymmdd
 ***        pattern : job's day pattern
 		week mode: [1,2,3,4,5,6,7]
 		date mode: [20130508-20130608]
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_match_day(char *day, char *pattern)
{
	sw_int32_t	i = 0;
	sw_int32_t	j = 0;
	sw_int32_t	len = 0;
	sw_char_t	buf[64];
	sw_char_t	tmp[64];
	
	pub_mem_memzero(buf, sizeof(buf));
	pub_mem_memzero(tmp, sizeof(tmp));
	
	strncpy(buf, pattern, sizeof(buf) - 1);
	pub_str_zipspace(buf);
	len = strlen(buf);
	
	if (buf[0] != '[' || buf[len - 1] != ']')
	{
		pub_log_error("%s, %d, Param error, buf=[%s].", __FILE__, __LINE__, buf);
		return SW_ERROR;
	}
	
	i = 1;
	j = 0;
	while (buf[i] != ']')
	{
		if (buf[i] != ',')
		{
			tmp[j++] = buf[i++];
		}
		else
		{
			tmp[j] = '\0';
			if (job_match_interval(day, tmp) == 1)
			{
				return 1;
			}
			j = 0;
			i++;
			pub_mem_memzero(tmp, sizeof(tmp));
		}
	}
	if (job_match_interval(day, tmp) == 1)
	{
		return 1;
	}
	
	return 0;
}

/******************************************************************************
 *** name      : job_match_time
 *** function  : Check whether the time value is in the check time window.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 ***     cur_time : current time point, formit is hhmm
 ***     ref_time : reference time point
       time_range : time window range
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_match_time(sw_int32_t cur_time, sw_char_t *ref_time, sw_int32_t time_range, int *realtime)
{
	sw_int32_t	i = 0;
	sw_int32_t	j = 0;
	sw_int32_t	t = 0;
	sw_char_t	buf[512];
	sw_char_t	inc[512];
	
	pub_mem_memzero(buf, sizeof(buf));
	pub_mem_memzero(inc, sizeof(inc));
	
	strcpy(buf, ref_time);
	while (buf[i] != '\0')
	{
		if (buf[i] != ' ')
		{
			inc[j] = buf[i];
			i++;
			j++;
		}
		else
		{
			inc[j] = '\0';
			j = 0;
			i++;
			t = atoi(inc);
			if (*realtime > 0)
			{
				if (t <= *realtime)
				{
					memset(inc, 0x0, sizeof(inc));
					continue;
				}
			}
			
			if (cur_time >= t && cur_time <= t + time_range)
			{
				*realtime = t;
				return 0;
			}
			else
			{
				memset(inc, 0x0, sizeof(inc));
			}
		}
	}
	
	t = atoi(inc);
	if (*realtime > 0)
	{
		if (t <= *realtime)
		{
			return -1;
		}
	}

	if (cur_time >= t && cur_time <= t + time_range)
	{
		*realtime = t;
		return 0;
	}
	
	return -1;
}

/******************************************************************************
 *** name      : job_match_time_format
 *** function  : Check if the time value match the time format
 *** example   : min hour day month year  
 *** example   : 1  2,3 4-5 /6 *
 *** author    : xuehui
 *** create    : 2017-10-16
 *** return    : 0:success  -1:fail
 ******************************************************************************/
sw_int_t job_match_time_format(sw_char_t *time_format, sw_int_t *realtime)
{
	sw_int32_t	i = 0;
	sw_int32_t	j = 0;
	sw_int32_t	time_value[5];
	sw_int32_t	value = 0;
	sw_int32_t	find = 0;
	time_t cur_time = 0;
	sw_char_t	*str = NULL;
	sw_char_t	*ptr = NULL;
	sw_char_t	format[5][512];
	sw_char_t	tmp[32];
	sw_tm_t		tm;

	cur_time = time(NULL);
	if (cur_time - *realtime < 60)
	{
		return 1;
	}

	memset(format, 0x0, sizeof(format));
	sscanf(time_format, "%s%s%s%s%s", format[0], format[1], format[2], format[3], format[4]);

	memset(&tm, 0x0, sizeof(tm));
	localtime_r(&cur_time, &tm);

	time_value[0] = tm.tm_min;
	time_value[1] = tm.tm_hour;
	time_value[2] = tm.tm_mday;
	time_value[3] = tm.tm_mon + 1;
	time_value[4] = tm.tm_year + 1900;

	for (i = 0; i < 5; ++i)
	{
		str = format[i];
		if (strcmp(str, "*") == 0 || str[0] == '\0')
		{
			continue;
		}
		else if ((ptr = strchr(str, '/')) != NULL)
		{
			if (ptr != str)
			{
				pub_log_error("[%s][%d]time format error [%s]", __FILE__, __LINE__, str);
				return -1;
			}

			value = atoi(str + 1);
			if (value == 0 ||  time_value[i] % value != 0 )
			{
				return 1;
			}
		}
		else if ((ptr = strchr(str, '-')) != NULL)
		{
			memset(tmp, 0x0, sizeof(tmp));
			memcpy(tmp, str, ptr - str);
			value = atoi(tmp);
			if (time_value[i] < value)
			{
				return 1;
			}
			memset(tmp, 0x0, sizeof(tmp));
			strcpy(tmp, ptr + 1);
			value = atoi(tmp);
			if (time_value[i] > value)
			{
				return 1;
			}
		}
		else if ((ptr = strchr(str, ',')) == NULL)
		{
			value = atoi(str);
			if (time_value[i] != value)
			{
				return 1;
			}
		}
		else
		{
			find = 0;
			while((ptr = strsep(&str, ",")) != NULL)
			{
				value = atoi(ptr);
				if (value == time_value[i])	
				{
					find = 1;
					break;	
				}
			}

			if (!find)
			{
				return 1;
			}
		}
	}
	
	*realtime = cur_time;
	return 0;
}

/******************************************************************************
 *** name      : job_check_job
 *** function  : check if it is the time to run for some job, run it.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 ***     cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_check_job(sw_job_cycle_t* cycle)
{
	int	realtime = 0;
	sw_int32_t	i = 0;
	sw_int_t	result = 0;
	sw_int32_t	now_time = 0;
	sw_int32_t	run_time = 0;
	sw_int_t	cur_time = 0;
	sw_char_t	date_buf[16];
	sw_char_t	time_buf[16];
	sw_char_t	week_buf[8];
    	struct tm	*tm;
    	struct timeb	timeb;
        sw_job_t *job;
	pub_mem_memzero(date_buf, sizeof(date_buf));
	pub_mem_memzero(time_buf, sizeof(time_buf));
	pub_mem_memzero(week_buf, sizeof(week_buf));
	
	cur_time = (long)time(NULL);
	ftime(&timeb);
	tm = localtime(&timeb.time);
	strftime(date_buf, 16, "%Y%m%d", tm);
	strftime(time_buf, 16, "%H%M", tm);
	strftime(week_buf, 8, "%u", tm);

	job = cycle->jobs;
	now_time = atoi(time_buf);
	i = 0;
	while (i < job->head.cur_cnt
		&& job->job_item[i].job_name[0] != '\0' 
		&& job->job_item[i].exec[0] != '\0')
	{
		if (strcmp(date_buf, job->job_item[i].last_run_date) != 0 && 
			job->job_item[i].job_status == JOB_FINISHED)
		{
			job->job_item[i].job_status = JOB_IDLE;
		}

		if (job->job_item[i].week[0] != '\0')
		{
			/*** WEEK mode ***/
			if (job_match_day(week_buf, job->job_item[i].week) != 1)
			{
				i++;
				continue;
			}
		}

		if (job->job_item[i].date[0] != '\0')
		{
			/*** DATE mode ***/
			if (job_match_day(date_buf, job->job_item[i].date) != 1)
			{
				i++;
				continue;
			}
		}

		/*** Fixed time job. ***/
		if (job->job_item[i].run_type == FIXED_TIME && job->job_item[i].job_status != JOB_DOING)
		{
			realtime = 0;
			if (job->job_item[i].job_status == JOB_FINISHED)
			{
				realtime = job->job_item[i].last_run_time;
			}
			result = job_match_time(now_time, job->job_item[i].time, 3, &realtime);
			if (result == 0)
			{
				if (job->job_item[i].job_status == JOB_FINISHED && realtime == job->job_item[i].last_run_time &&
					strcmp(job->job_item[i].last_run_date, date_buf) == 0)
				{
					i++;
					continue;
				}
				pub_log_info("[%s][%d] Ready execute job [%s]. last_run_time=[%d] realtime=[%d]", 
					__FILE__,__LINE__, job->job_item[i].exec, job->job_item[i].last_run_time, realtime);
				job->job_item[i].last_run_time = realtime;
				memset(job->job_item[i].last_run_date, 0x0, sizeof(job->job_item[i].last_run_date));
				strcpy(job->job_item[i].last_run_date, date_buf);
				result = job_run(cycle, i);
				if (result)
				{
					pub_log_error("[%s][%d] job_run [%s] error.", 
						__FILE__, __LINE__, job->job_item[i].exec);
					i++;
					continue;
				}
				pub_log_info("[%s][%d] Run job [%s] success.", 
					__FILE__, __LINE__, job->job_item[i].exec);
			}
		}

		/*** Period job ***/
		if (job->job_item[i].run_type == PERIOD_TIME && job->job_item[i].job_status != JOB_DOING)
		{
			run_time = atoi(job->job_item[i].time);
			if ((cur_time - job->job_item[i].last_run_time) >= run_time)
			{
				pub_log_info("[%s][%d] Ready execute job [%s]", 
					__FILE__,__LINE__, job->job_item[i].exec);
				result = job_run(cycle, i);
				if (result)
				{
					pub_log_error("[%s][%d] job_run [%s] error.", 
						__FILE__, __LINE__, job->job_item[i].exec);
					i++;
					continue;
				}
				pub_log_info("[%s][%d] Run job [%s] success", 
					__FILE__, __LINE__,job->job_item[i].exec);
			}
		}

		/*** Format time job. ***/
		/*min hour day month year*/
		/*1 2-3 4,5 /6 * */
		if (job->job_item[i].run_type == FORMAT_TIME && job->job_item[i].job_status != JOB_DOING)
		{
			result = job_match_time_format(job->job_item[i].time, &(job->job_item[i].last_run_time));
			if (result == -1)
			{
				pub_log_error("[%s][%d] time format [%s] error.",
						__FILE__, __LINE__, job->job_item[i].time);
					i++;
					continue;
			}
			if (result == 0)
			{
				pub_log_info("[%s][%d] Ready execute job [%s]. ", 
					__FILE__,__LINE__, job->job_item[i].exec);
				result = job_run(cycle, i);
				if (result)
				{
					pub_log_error("[%s][%d] job_run [%s] error.", 
						__FILE__, __LINE__, job->job_item[i].exec);
					i++;
					continue;
				}
				pub_log_info("[%s][%d] Run job [%s] success.", 
					__FILE__, __LINE__, job->job_item[i].exec);
			}
		}

		i++;
	}
	
	return SW_OK;
}

/******************************************************************************
 *** name      : job_reload_job_config
 *** function  : Reload job configure file job.xml to job info shm.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 ***     cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_reload_job_config(sw_job_cycle_t* cycle)
{
	return job_load_cfg(cycle->base.global_path->job_file);
}

/******************************************************************************
 *** name      : job_check_file_status
 *** function  : Check if job.xml was updated, reload it to job info shm.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 ***     cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_check_file_status(sw_job_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	path[PATH_LEN];
	struct stat	file_stat;

	pub_mem_memzero(&file_stat, sizeof(file_stat));
	pub_mem_memzero(path, sizeof(path));
	
	strncpy(path, cycle->base.global_path->job_file,sizeof(path)-1);

	result = stat(path, &file_stat);
	if (result < 0)
	{
		if (errno == 2)
		{
			return -2;
		}
		pub_log_error("[%s][%d]stat error! errno=[%d]:[%s]", __FILE__, __LINE__, strerror(errno));
		return SW_ERROR;
	}
	
	if (cycle->job_cfg_modify_time == 0)
	{
		cycle->job_cfg_modify_time = file_stat.st_mtime;
	}
	
	if (cycle->job_cfg_modify_time != file_stat.st_mtime)
	{
		result = job_reload_job_config(cycle);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, job_reload_job_config error",__FILE__,__LINE__);
			return SW_ERROR;
		}
		cycle->job_cfg_modify_time = file_stat.st_mtime;
	}
	
	return SW_OK;
}

