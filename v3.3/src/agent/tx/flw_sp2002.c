/*************************************************
 文 件 名:  flw_sp2002.c                         **
  功能描述:  进程信息                            **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

/*2002*/
static cpu_usage_stat_t g_cpu_usage_data;

int get_cpu_info(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	fd = 0;
	int		total = 0;
	char	*p;
	char	*q;
	char	tmp[64];
	char	buf[256];
	char	name[32];
	char	path[256];
	char	filepath[256];
	FILE	*fp = NULL;
	char	item[10][256];
	sw_buf_t *use = NULL;
	sw_buf_t *times = NULL;
	
	memset(filepath, 0x0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath) - 1, "%s/tmp/agtmon/cpuinfo.txt", getenv("SWWORK"));
	if (access(filepath, F_OK))
	{
		return 0;
	}

	fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open cpu info file error %s", __FILE__, __LINE__, filepath);
		return -1;
	}

	fd = fileno(fp);
	pub_lock_fd(fd);

	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		if ((p = strstr(buf, "TOTAL:")) == NULL)
		{
			continue;
		}
		q = strstr(buf, "|");
		memset(tmp, 0x0, sizeof(tmp));
		strncpy(tmp, p + 6, q - p - 6);
		total = atoi(tmp);
		
		break;
	}
	use = malloc(sizeof(sw_buf_t) * total);
	if (use == NULL)
	{
		pub_log_error("[%s][%d] malloc error", __FILE__, __LINE__);
		return -1;
	}

	times = malloc(sizeof(sw_buf_t) * total);
	if (times == NULL)
	{
		pub_log_error("[%s][%d] malloc error", __FILE__, __LINE__);
		free(use);
		return -1;
	}

	for (i = 0; i < total; i++)
	{	
		pub_buf_init(use + i);	
		pub_buf_init(times + i);
	}

	fseek(fp, 0, SEEK_SET);
	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		if ((p = strstr(buf, "TOTAL:")) == NULL)
		{
			continue;
		}

		for (i = 0; i < total; i++)
		{	
			memset(buf, 0x0, sizeof(buf));
			fgets(buf, sizeof(buf), fp);		
			if ((p = strstr(buf, "TIME:")) == NULL)
			{
				continue;
			}
			memset(item, 0x0, sizeof(item));
			agt_str_parse(buf, item, 10);
			
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "%s|", item[0]);
			buf_append(times + i, tmp, strlen(tmp));

			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, "%.2f|", atof(item[1]) + atof(item[2]) + atof(item[4]));
			buf_append(use + i, buf, strlen(buf));
		}		
	}
	
	for (i = 0; i < total; i++)
	{
		memset(name, 0x0, sizeof(name));
		sprintf(name, "CPU%d", i);	
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.CpuArg.Datas.Data(%d).Name", i);
		loc_set_zd_data(vars, path, name);
		
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.CpuArg.Datas.Data(%d).Time", i);
		loc_set_zd_data(vars, path, times[i].data);
		
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.CpuArg.Datas.Data(%d).CpuUsed", i);
		loc_set_zd_data(vars, path, use[i].data);

		pub_buf_clear(times + i);
		pub_buf_clear(use + i);
	}
	
	pub_unlock_fd(fd);
	fclose(fp);

	free(times);
	free(use);

	return 0;
}

cpu_total_t  *get_cpu_total()
{
	int 	ret = 0;
	cpu_total_t	*cpu_struct = NULL;

#ifdef AIX
	perfstat_cpu_total_t	cputotal;
#endif
#ifdef LINUX
	FILE		*fp = NULL;
	char	buf[256];
	int	cpu_stat = 0;
#endif

#ifdef AIX
	memset(&cputotal, 0x00, sizeof(cputotal));
	ret = perfstat_cpu_total(NULL, &cputotal, sizeof(perfstat_cpu_total_t), 1);
	if (ret == -1)
	{
		pub_log_error("[%s][%d] perfstat_cpu_total error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	cpu_struct = (cpu_total_t *)calloc(1, sizeof(cpu_total_t));
	if (cpu_struct == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	strncpy(cpu_struct->desc, cputotal.description, sizeof(cpu_struct->desc) - 1);
	cpu_struct->freq = cputotal.processorHZ/1000000;
	cpu_struct->user = cputotal.user;
	cpu_struct->kernel = cputotal.sys;
	cpu_struct->idle = cputotal.idle;
	cpu_struct->iowait = cputotal.wait;
	cpu_struct->total = cputotal.user + cputotal.sys + cputotal.idle + cputotal.wait;
#endif

#ifdef LINUX
	cpu_struct = (cpu_total_t *)calloc(1, sizeof(cpu_total_t));
	if (cpu_struct == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	pub_mem_memzero(buf, sizeof(buf));
	ret = agt_popen("cat /proc/stat|grep cpu|grep -v cpu[0-9]", buf);
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] popen error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	cpu_stat = sscanf(buf, "cpu %lld %lld %lld %lld %lld", 
		&cpu_struct->user, &cpu_struct->nice, &cpu_struct->kernel, 
		&cpu_struct->idle, &cpu_struct->iowait);

	if (cpu_stat < 4 || cpu_stat > 5)
	{
		pub_log_error("[%s][%d] error! cpu_stat=[%d]", __FILE__, __LINE__, cpu_stat);
		return NULL;
	}

	pub_mem_memzero(buf, sizeof(buf));
	ret = agt_popen("cat /proc/cpuinfo|grep MHz|cut -d: -f2|awk '{print $1}'", buf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] popen error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		free(cpu_struct);
		return NULL;
	}
	cpu_struct->freq = atol(buf);

	pub_mem_memzero(buf, sizeof(buf));
	ret = agt_popen("cat /proc/cpuinfo|grep name|cut -d: -f2|uniq", buf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] popen error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		free(cpu_struct);
		return NULL;
	}

	strncpy(cpu_struct->desc, buf, sizeof(cpu_struct->desc) - 1);
#endif

	return cpu_struct;
}

static int cpu_usage_stat_function(void *data)
{
	double  cpu_usage = 0.00;
	int	ret = -1;
	int	i = 0;
	int	num = 0;
	cpu_stat_t	cpu;
	cpu_stat_t	*cpu1 = NULL;
	cpu_stat_t	*cpu1_old = NULL;
	cpu_stat_t	*cpu2 = NULL;
	cpu_stat_t	*cpu2_old = NULL;
	cpu_usage_stat_t	*cpu_usage_data = NULL;

	cpu_usage_data = (cpu_usage_stat_t*)data;

	pub_log_debug("[%s][%d] enter cpu_usage_stat_func()", __FILE__, __LINE__);

	if(cpu_usage_data == NULL)
	{
		pub_log_error("[%s][%d] param error!",__FILE__, __LINE__);
		return -1;
	}

	cpu1 = agt_get_cpu_stat(&num);
	if (cpu1 == NULL)
	{
		pub_log_error("[%s][%d] getcpustat error!\n", __FILE__, __LINE__);
		return -1;
	}

	usleep(1000000);
	cpu2 = agt_get_cpu_stat(&num);
	if (cpu2 == NULL)
	{
		free(cpu1);
		pub_log_error("[%s][%d] getcpustat error!\n", __FILE__, __LINE__);
		return -1;
	}

	ret = pthread_mutex_init(&(g_cpu_usage_data.mutex), NULL);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pthread_mutex_init error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	pthread_mutex_lock(&(g_cpu_usage_data.mutex));
	cpu1_old = cpu1;
	cpu2_old = cpu2;

	cpu_usage = 0.00;	
	for (i = 0; i < num; i++)
	{
		memset(&cpu, 0x0, sizeof(cpu));
		cpu.user = cpu2->user - cpu1->user;
		cpu.kernel = cpu2->kernel - cpu1->kernel;
		cpu.idle = cpu2->idle - cpu1->idle;
		cpu.iowait = cpu2->iowait - cpu1->iowait;
		cpu.total = cpu2->total - cpu1->total;


		if (cpu.total > 0)
		{
			g_cpu_usage_data.cpu_usage[i].user = (double)cpu.user / (double)cpu.total * 100;
			g_cpu_usage_data.cpu_usage[i].kernel = (double)cpu.kernel / (double)cpu.total * 100;
			g_cpu_usage_data.cpu_usage[i].idle = (double)cpu.idle / (double)cpu.total * 100;
			g_cpu_usage_data.cpu_usage[i].iowait = (double)cpu.iowait / (double)cpu.total * 100;
		}
		else
		{
			g_cpu_usage_data.cpu_usage[i].user = 0;
			g_cpu_usage_data.cpu_usage[i].kernel = 0;
			g_cpu_usage_data.cpu_usage[i].idle = 0;
			g_cpu_usage_data.cpu_usage[i].iowait = 0;

		}
		strncpy(g_cpu_usage_data.cpu_usage[i].desc, cpu1->desc, sizeof(g_cpu_usage_data.cpu_usage[i].desc) - 1);
		cpu1++;
		cpu2++;

		cpu_usage += 100 - g_cpu_usage_data.cpu_usage[i].idle;
	}
	free(cpu1_old);
	cpu1 = NULL;
	free(cpu2_old);
	cpu2 = NULL;
	g_cpu_usage_data.cpu_num = num;
	pthread_mutex_unlock(&(g_cpu_usage_data.mutex));

	ret = pthread_mutex_destroy(&(g_cpu_usage_data.mutex));
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pthread_mutex_destroy error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	return 0;
}

/*2002*/
int sp2002(sw_loc_vars_t *vars)
{
	int 	ret = SW_ERROR;
	char	res_msg[256];
	char	reply[16];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	ret = get_cpu_info(vars);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Take the CPU information failure!"
			, __FILE__, __LINE__);
		strcpy(reply, "E008");
		goto ErrExit;
	}
	
	ret = get_proc_sort(vars, "CPU");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Take the memory information failure!"
			, __FILE__, __LINE__);
		strcpy(reply, "E008");
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
