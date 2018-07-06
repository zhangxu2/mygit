/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : interface for job in BP run-time shm. 
 *** name    : job.c 
 *** function: job sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "job.h"
#include "pub_log.h"
#include "sem_lock.h"
#include "pub_xml.h"

SW_PROTECTED sw_job_t *g_shm_job = NULL;
SW_PROTECTED sw_int_t job_check_time(char *check_value);
SW_PROTECTED sw_int_t job_check_multi_time(char *check_value);

sw_int_t job_loc_init(sw_job_t *shm_job, sw_syscfg_t* syscfg)
{
	sw_int32_t	lock_id = -1;
	sw_int32_t	size = 0;
	
	if (shm_job == NULL || syscfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*get a lock_id*/
	lock_id = sem_new_lock_id();
	if (lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	size = sizeof(sw_job_head_t) + sizeof(sw_job_item_t) * syscfg->job_max;
	pub_log_info("%s, %d, shm_job size(%d)", __FILE__, __LINE__, size);
	pub_mem_memzero(shm_job, size);

	/*lock*/
	sem_write_lock(lock_id);
	
	shm_job->head.lock_id = lock_id;
	shm_job->head.count = syscfg->job_max;
	shm_job->head.cur_cnt = 0;
	
	sem_write_unlock(lock_id);
	
	return SW_OK;
}

sw_int_t job_init(sw_job_t *shm_job, sw_syscfg_t *syscfg)
{
	sw_int_t	result = SW_ERROR;

	result = job_loc_init(shm_job, syscfg);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_loc_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	g_shm_job = shm_job;

	return SW_OK;
}

sw_int32_t job_get_size(sw_syscfg_t* syscfg)
{
	if (syscfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (sizeof(sw_job_head_t) + sizeof(sw_job_item_t) * syscfg->job_max);
}

sw_job_t* job_get_addr()
{
	return g_shm_job;
}

sw_int_t job_set_addr(sw_char_t *addr)
{
	if (addr == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_job = (sw_job_t*)addr;

	return SW_OK;
}

sw_int_t job_loc_load_cfg(sw_job_t *shm_job, sw_char_t *xml_name)
{
	sw_int32_t	i = 0;
	sw_int_t	result = 0;
	sw_int_t	time_value = 0;
	
	sw_xmltree_t	*xml_tree = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	sw_job_item_t	*job_item = NULL;

	pub_log_info("%s, %d, lock_id[%d] count[%d] cur_count[%d]"
			, __FILE__, __LINE__, shm_job->head.lock_id
			, shm_job->head.count, shm_job->head.cur_cnt);
	
	if (shm_job == NULL)
	{
		pub_log_error("%s, %d, shm_job == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	time_value = (sw_int_t)time(NULL);
	job_item = shm_job->job_item;
	
	result = access(xml_name, 0);
	if (result == -1)
	{
		pub_log_info("%s, %d, no job.xml", __FILE__, __LINE__);
		return SW_OK;
	}

	xml_tree = pub_xml_crtree(xml_name);
	if (xml_tree == NULL)
	{
		pub_log_error("%s, %d, s_crxmltree [%s] fail."
				, __FILE__, __LINE__, xml_name);
		return SW_ERROR;
	}
	
	i = 0;
	node = pub_xml_locnode(xml_tree, ".JOBS.JOB");
	while (node != NULL)
	{
		if (strcmp(node->name, "JOB") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml_tree->current = node;
		node_bak = node;
		node1 = pub_xml_locnode(xml_tree, "NO");
		if (node1 == NULL)
		{
			pub_log_error("%s, %d, no NO.", __FILE__, __LINE__);
			pub_xml_deltree(xml_tree);
			return SW_ERROR;
		}
		
		strncpy(job_item[i].no, node1->value, sizeof(job_item[i].no) - 1);
		
		node1 = pub_xml_locnode(xml_tree, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("%s, %d, no NAME", __FILE__, __LINE__);
			pub_xml_deltree(xml_tree);
			return SW_ERROR;
		}
		
		strncpy(job_item[i].job_name, node1->value, sizeof(job_item[i].job_name) - 1);
		
		node1 = pub_xml_locnode(xml_tree, "MANUAL");
		if (node1 != NULL && node1->value != NULL)
		{
			job_item[i].manual = atoi(node1->value);
			if (job_item[i].manual != JOB_AUTO && job_item[i].manual != JOB_MANUAL)
			{
				pub_log_error("%s, %d, job_item[%d].manual[%d] unknown"
						, __FILE__, __LINE__, i, job_item[i].manual);
				pub_xml_deltree(xml_tree);
				return SW_ERROR;
			}
		}
		else
		{
			job_item[i].manual = JOB_AUTO;
		}
		
		node1 = pub_xml_locnode(xml_tree, "TIMEOUT");
		if (node1 != NULL && node1->value != NULL)
		{
			job_item[i].time_out = atoi(node1->value);
		}
		else
		{
			job_item[i].time_out = 3600;
		}

		job_item[i].start_time = time_value;

		xml_tree->current = node_bak;
		node1 = pub_xml_locnode(xml_tree, "EXEC");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(job_item[i].exec, node1->value, sizeof(job_item[i].exec) - 1);
		}
		
		job_item[i].job_status = JOB_IDLE;

		node1 = pub_xml_locnode(xml_tree, "EXECTYPE");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			job_item[i].exec_type = atoi(node1->value);
			if (job_item[i].exec_type != BIN_JOB && job_item[i].exec_type != SCRIPT_JOB)
			{
				pub_log_error("%s, %d, job_item[%d].exec_type[%d] unknown"
						, __FILE__, __LINE__, i, job_item[i].exec_type);
				pub_xml_deltree(xml_tree);
				return SW_ERROR;
			}
		}
		else
		{
			job_item[i].exec_type = SCRIPT_JOB;
		}

		node1 = pub_xml_locnode(xml_tree, "RUNTYPE");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			job_item[i].run_type = atoi(node1->value);
			if (job_item[i].run_type != FIXED_TIME && job_item[i].run_type != PERIOD_TIME && job_item[i].run_type != FORMAT_TIME)
			{
				pub_log_error("%s, %d, job_item[%d].run_type[%d] unknown."
						, __FILE__, __LINE__, i, job_item[i].run_type);
				pub_xml_deltree(xml_tree);
				return SW_ERROR;
			}
		}
		else
		{
			pub_log_error("%s, %d, No RUNTYPE", __FILE__, __LINE__);
			pub_xml_deltree(xml_tree);
			return SW_ERROR;
		}
		
		node1 = pub_xml_locnode(xml_tree, "TIME");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(job_item[i].time, node1->value, sizeof(job_item[i].time) - 1);
			if (job_item[i].run_type == FIXED_TIME)
			{
				if (job_check_multi_time(job_item[i].time) != 0)
				{
					pub_log_error("%s, %d, Abnormal time_value format job_item[%d].time[%s]",
						__FILE__, __LINE__, i, job_item[i].time);
					pub_xml_deltree(xml_tree);
					return SW_ERROR;
				}
			}
		}
		else
		{
			pub_log_error("%s, %d, No TIME", __FILE__, __LINE__);
			pub_xml_deltree(xml_tree);
			return SW_ERROR;
		}
		
		if (job_item[i].run_type == PERIOD_TIME || job_item[i].run_type == FORMAT_TIME)
		{
			job_item[i].last_run_time = time_value;
		}
		
		node1 = pub_xml_locnode(xml_tree, "WEEK");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(job_item[i].week, node1->value, sizeof(job_item[i].week) - 1);
		}
		
		node1 = pub_xml_locnode(xml_tree, "DATE");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(job_item[i].date, node1->value, sizeof(job_item[i].date) - 1);
		}

		i++;
		if (i >= shm_job->head.count)
		{
			pub_log_error("%s, %d, job's num in config is greater than max job number in shm, i[%d] count[%d]."
					, __FILE__, __LINE__, i, shm_job->head.count);
			pub_xml_deltree(xml_tree);
			return SW_ERROR;
		}
		
		node = node->next;
	}
	shm_job->head.cur_cnt = i;
	pub_xml_deltree(xml_tree);
	
	return SW_OK;
}

sw_int_t job_load_cfg(sw_char_t *xml_name)
{
	if (g_shm_job == NULL)
	{
		pub_log_error("%s, %d, g_shm_job == NULL.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (job_loc_load_cfg(g_shm_job, xml_name));
}

sw_int_t job_check_time(char *check_value)
{
	int	tmp = 0;
	char	tmp_buf[8];
	
	pub_mem_memzero(tmp_buf, sizeof(tmp_buf));
	
	if (strlen(check_value) != 4)
	{
		pub_log_error("%s, %d, time_buf[%s]'s length is abnormal."
				, __FILE__, __LINE__, check_value);
		return SW_ERROR;
	}
	
	strncpy(tmp_buf, check_value, 2);
	tmp = atoi(tmp_buf);
	if (tmp < 0 || tmp > 23)
	{
		pub_log_error("%s, %d, time [%s]'s hour value [%d] is abnormal."
				, __FILE__, __LINE__, check_value, tmp);
		return SW_ERROR;
	}
	
	pub_mem_memzero(tmp_buf, sizeof(tmp_buf));
	strncpy(tmp_buf, check_value + 2, 2);
	tmp = atoi(tmp_buf);
	if (tmp >= 0 && tmp <= 59)
	{
		return SW_OK;
	}
	
	pub_log_error("%s, %d, time [%s]'s second value[%d] is abnormal."
			, __FILE__, __LINE__, check_value, tmp);
	return SW_ERROR;
}

sw_int_t job_check_multi_time(char *check_value)
{
	int	i = 0;
	int	j = 0;
	char	buf[512];
	char	inc[512];
	
	pub_mem_memzero(buf, sizeof(buf));
	pub_mem_memzero(inc, sizeof(inc));
	
	strcpy(buf, check_value);
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
			if (job_check_time(inc))
			{
				pub_log_error("%s, %d, time format is wrong."
						, __FILE__, __LINE__, check_value, inc);
				return SW_ERROR;
			}
			
			pub_mem_memzero(inc, sizeof(inc));
		}
	}
	
	if (job_check_time(inc))
	{
		return SW_ERROR;
	}
	
	return SW_OK;
}

static job_stat_desc_t g_job_stat[] =
{
	{JOB_IDLE,"IDLE"}
	,{JOB_DOING,"DOING"}
	,{JOB_ERROR,"ERROR"}
	,{JOB_FINISHED,"IDLE"}
	,{JOB_TIMEOUT,"TIMEOUT"}
};

static job_type_desc_t g_job_type[] =
{
	{JOB_AUTO,"AUTO"}
	,{JOB_MANUAL,"MANUAL"}
};

const sw_char_t* job_get_stat_desc(sw_char_t stat)
{
	sw_int_t idx=0, num=0;

	num = sizeof(g_job_stat)/sizeof(job_stat_desc_t);
	for(idx=0; idx<num; idx++)
	{
		if(g_job_stat[idx].status== stat)
		{
			return g_job_stat[idx].desc;
		}
	}
	return NULL;
}

const sw_char_t* job_get_type_desc(JOB_TYPE type)
{
	sw_int_t idx=0, num=0;

	num = sizeof(g_job_type)/sizeof(job_type_desc_t);
	for(idx=0; idx<num; idx++)
	{
		if(g_job_type[idx].type == type)
		{
			return g_job_type[idx].desc;
		}
	}
	return NULL;
}


