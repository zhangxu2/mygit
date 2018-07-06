/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : BP run-time shm. 
 *** name    : mtype.c 
 *** function: mtype sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "mtype.h"
#include "sem_lock.h"
#include "pub_time.h"
#include "pub_cfg.h"
#include "pub_mem.h"
#include "msg_trans.h"
#include "pub_ares.h"
#include "pub_log.h"
#include "alert.h"
#include "trace.h"

#define MTYPE_TIMEOUT	3*60*60
/*mtype info's addr in shm_run.*/
SW_PROTECTED sw_mtype_t*	g_shm_mtype = NULL;


sw_int_t mtype_loc_init(sw_mtype_t*	shm_mtype, sw_syscfg_t* sys_cfg)
{
	int  i;
	sw_mtype_node_t*	mtype_item = NULL;
	sw_int32_t		lock_id = -1;
	sw_int32_t		mtype_max = -1;
	sw_int32_t		mtype_size = -1;

	if (sys_cfg == NULL || shm_mtype == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (sys_cfg->session_max <= 0)
	{
		mtype_max = MAX_MTYPE_CNT;
	}
	else
	{
		mtype_max = sys_cfg->session_max;
		
	}
	
	mtype_size = sizeof(sw_mtype_t) 
			+ (mtype_max - 1) * sizeof(sw_mtype_node_t);
	pub_mem_memzero((sw_char_t*)shm_mtype, mtype_size);

	/*get a lockid*/
	lock_id = sem_new_lock_id();
	if (lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*lock*/
	sem_mutex_lock(lock_id);

	shm_mtype->head.lock_id = lock_id;
	shm_mtype->head.mtype_max = mtype_max;
	shm_mtype->head.size = mtype_size;
	
	pub_log_info("%s, %d, shm_mtype size[%d]"
			, __FILE__, __LINE__, shm_mtype->head.size);

	mtype_item = &(shm_mtype->first);
	for (i = 0; i < shm_mtype->head.mtype_max; i++)
	{
		if (i == (shm_mtype->head.mtype_max - 1))
		{
			 mtype_item[i].flag = MTYPE_TAIL;
		}
		else
		{
			mtype_item[i].flag = MTYPE_IDLE;
		}
	}

	shm_mtype->head.msg_cnt = 0;
	shm_mtype->head.cur_cnt = 0;
	shm_mtype->head.succ_cnt = 0;
	shm_mtype->head.fail_cnt = 0;
	
	sem_mutex_unlock(lock_id);
	
	return SW_OK;
}

sw_int_t mtype_init(sw_mtype_t* shm_mtype, sw_syscfg_t* sys_cfg)
{
	sw_int_t	result = SW_ERROR;

	result = mtype_loc_init(shm_mtype, sys_cfg);
	if (result != SW_OK)
	{
		pub_log_error("mtype_loc_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	g_shm_mtype = shm_mtype;

	return SW_OK;
}

sw_int32_t mtype_get_size(sw_syscfg_t *sys_cfg)
{
	sw_int32_t	mtype_size = 0;
	sw_int32_t	mtype_max = 0;
	
	if (sys_cfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (sys_cfg->session_max <= 0)
	{
		mtype_max = MAX_MTYPE_CNT;
	}
	else
	{
		mtype_max = sys_cfg->session_max;
		
	}
	
	mtype_size = sizeof(sw_mtype_t) 
			+ (mtype_max - 1) * sizeof(sw_mtype_node_t);

	return mtype_size;
}

sw_int_t mtype_set_addr(sw_char_t* addr)
{
	if (NULL == addr)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_mtype = (sw_mtype_t*)addr;

	return SW_OK;
}

/******************************************************************************
 ** Name : mtype_get_addr
 ** Desc : 获取Mtype的内存首地址
 ** Input: NONE
 ** Output: NONE
 ** Return: Mtype的内存首地址
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
const sw_mtype_t *mtype_get_addr(void)
{
    return g_shm_mtype;
}

sw_int32_t mtype_loc_new(sw_mtype_t* shm_mtype)
{
	sw_mtype_node_t	*mtype_item = NULL;
	sw_int32_t	i = 0;
	sw_int32_t	curr_index = 0;
	sw_int32_t	lock_id = -1;
	sw_int32_t	mtype_max = 0;
	sw_int64_t	now = 0;
	sw_int64_t	space = 0;
	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	mtype_item = &(shm_mtype->first);
	lock_id = shm_mtype->head.lock_id;
	mtype_max = shm_mtype->head.mtype_max;
	
	if (shm_mtype->head.cur_use < 0 || curr_index >= mtype_max)
	{
		shm_mtype->head.cur_use = 0; 
	}

	curr_index = shm_mtype->head.cur_use;
	now = pub_time_get_current();
	
	/*lock*/
	sem_mutex_lock(lock_id);
	
	for (i = curr_index; i < mtype_max; i++)
	{
		if(mtype_item[i].flag == MTYPE_IDLE)
		{
			/*find a empty item*/
			break;
		}
		else if(mtype_item[i].flag == MTYPE_USED)
		{
			/*used*/
			space = pub_time_get_space(now, mtype_item[i].start_time);
			if (space > MTYPE_TIMEOUT)
			{
				mtype_limit_free(mtype_item[i].prdt_name, mtype_item[i].lsn_name, 0);
				mtype_item[i].flag = MTYPE_IDLE;
				shm_mtype->head.fail_cnt++;
				shm_mtype->head.cur_cnt--;
				pub_log_info("[%s][%d] MTYPE[%d] TIMEOUT, REUSED!", __FILE__, __LINE__, mtype_item[i].mtype);
				break;
			}
			continue;
		}
		else if(mtype_item[i].flag == MTYPE_TAIL)
		{
			/*to end*/
			break;
		}
		else
		{
			sem_mutex_unlock(lock_id);
			pub_log_error("[%s][%d] Error:mtype[%d].flag=[%d]", __FILE__, __LINE__, i, mtype_item[i].flag);
			return SW_ERROR;
		}
	}
	
	if (i == mtype_max || mtype_item[i].flag < 0)
	{
		for (i = 0; i < curr_index; i++)
		{
			if(mtype_item[i].flag == MTYPE_IDLE)
			{
				/*find a empty item*/
				break;
			}
			else if(mtype_item[i].flag == MTYPE_USED)
			{
				/*used*/
				space = pub_time_get_space(now, mtype_item[i].start_time);
				if (space > MTYPE_TIMEOUT)
				{
					mtype_limit_free(mtype_item[i].prdt_name, mtype_item[i].lsn_name, 0);
					mtype_item[i].flag = MTYPE_IDLE;
					shm_mtype->head.fail_cnt++;
					shm_mtype->head.cur_cnt--;
					pub_log_info("[%s][%d] MTYPE[%d] TIMEOUT, REUSED!", __FILE__, __LINE__, mtype_item[i].mtype);
					break;
				}
				continue;
			}
			else if(mtype_item[i].flag == MTYPE_TAIL)
			{
				/*to the end*/
				break;
			}
			else
			{
				sem_mutex_unlock(lock_id);
				pub_log_error("[%s][%d] Error:mtype[%d].flag=[%d]", __FILE__, __LINE__, i, mtype_item[i].flag);
				return SW_ERROR;
			}
		}
		
		if (mtype_item[i].flag != MTYPE_IDLE)
		{
			sem_mutex_unlock(lock_id);
			pub_log_error("[%s][%d] ERROR: All the mtype resources run out!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	/*find a empty*/
	pub_mem_memzero((sw_char_t *)(mtype_item + i), sizeof(sw_mtype_node_t));
	mtype_item[i].flag = MTYPE_USED;
	mtype_item[i].mtype = i + 1;
	mtype_item[i].start_time = pub_time_get_current();

	shm_mtype->head.cur_cnt++;
	shm_mtype->head.cur_use = i + 1;
	shm_mtype->head.msg_cnt++;

	sem_mutex_unlock(lock_id);
	/*clean the vars add by xuehui 20180308*/
	pub_vars_destroy_by_mtype(i + 1);
	
	return (i + 1);
}

static sw_int32_t mtype_print_minfo()
{
	int     i    = 0;
	int	len  = 0;
	int     used = 0;
	int     idle = 0;
	char 	time[128];
	char	line[2048];
	char 	filename[128];
	sw_int32_t      fd;
	sw_int32_t      lock_id;
	sw_int64_t	now = 0;
	sw_trace_info_t	*trace_info = NULL;
	sw_trace_item_t *trace_item = NULL;	
	sw_mtype_head_t *head;
	sw_mtype_node_t *item;

	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/log/syslog/swminfo.log", getenv("SWWORK"));
	fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd < 0)
	{
		pub_log_error("[%s][%d]Can not open file [%s]! errno=[%d]:[%s]", 
				__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}

	trace_info = trace_get_addr();
	trace_item = trace_info->trace_item;

	head = &g_shm_mtype->head;
	item = &g_shm_mtype->first;

	now = pub_time_get_current();
	lock_id = head->lock_id;
	sem_mutex_lock(lock_id);
	for (i = 0; i < head->mtype_max; i++)
	{
		switch (item->flag)
		{
			case MTYPE_TAIL:
			case MTYPE_IDLE:
			{
				idle++;
				break;
			}
			case MTYPE_USED:
			{
				used++;
				break;
			}
			default:
			{
				break;
			}
		}
		item++;
	}
	if (used > 0)
	{
		memset(time, 0x0, sizeof(time));
		pub_change_time(now, time, 5);
		
		memset(line, 0x0, sizeof(line));
		sprintf(line, "TOTAL=%d USED=[%d] IDEL=[%d] SUCCESS=[%d] FAIL=[%d]\n", 
				head->mtype_max, used, idle, head->succ_cnt, head->fail_cnt);
		len = strlen(line);
		write(fd, line, len);
		item = &g_shm_mtype->first;
		for (i = 0; i < head->mtype_max; i++)
		{
			if (item->flag == MTYPE_USED)
			{
				memset(time, 0x0, sizeof(time));
				pub_change_time(trace_item[i].start_time, time, 5);
				memset(line, 0x0, sizeof(line));
				sprintf(line, "MTYPE:[%d] TRACE_NO:[%lld] BUSI_NO:[%lld] PRDT:[%s] CHNL:[%s] TXCODE:[%s] SERVER:[%s] SERVICE:[%s] STARTTIME:[%s]\n", 
				i + 1, trace_item[i].trace_no, trace_item[i].busi_no, trace_item[i].prdt_name, 
				trace_item[i].chnl, trace_item[i].tx_code, trace_item[i].server, trace_item[i].service, time);
				len = strlen(line);
				write(fd, line, len);
			}
			item++;
		}
	}
	sem_mutex_unlock(lock_id);
	close(fd);
	return 0;
}


sw_int32_t mtype_new()
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sw_int32_t mtype = mtype_loc_new(g_shm_mtype);
	if (mtype <= 0)
	{
		mtype_print_minfo();
		alert_msg(ERR_MTYPE_FAILED, "create mtype error.");
		return SW_ERROR;
	}

	return mtype;
}

sw_int32_t mtype_loc_cur_cnt(sw_mtype_t* shm_mtype)
{	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return shm_mtype->head.cur_cnt;
}

sw_int32_t mtype_cur_cnt()
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_cur_cnt(g_shm_mtype));
}

sw_int32_t mtype_loc_get_max(sw_mtype_t* shm_mtype)
{
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return shm_mtype->head.mtype_max > 0 ? shm_mtype->head.mtype_max : SW_ERROR;
}

sw_int32_t mtype_get_max()
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_max(g_shm_mtype));
}

sw_int_t mtype_loc_delete(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int32_t flag)
{
	sw_mtype_node_t	*mtype_item = NULL;
	sw_int32_t	lock_id = -1;
	
	if (shm_mtype == NULL || mtype <= 0)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	mtype_item = &(shm_mtype->first);
	mtype_item += mtype - 1;
	lock_id = shm_mtype->head.lock_id;

	mtype_limit_free(mtype_item->prdt_name, mtype_item->lsn_name, 1);
	/*lock*/
	sem_mutex_lock(lock_id);
	
	if (mtype_item->mtype != mtype || mtype_item->flag != MTYPE_USED)
	{
		sem_mutex_unlock(lock_id);
		return	SW_ERROR;
	}

	mtype_item->flag = MTYPE_IDLE;
	
	if (flag == 1)
	{
		shm_mtype->head.fail_cnt++;
	}
	else if (flag == 0)
	{
		shm_mtype->head.succ_cnt++;
	}
	
	shm_mtype->head.cur_cnt--;
	pub_vars_destroy_by_mtype(mtype);
	if (flag == 0 || flag == 1)
	{
		pub_log_end("[%s][%d] mtype=[%d] end!!!", __FILE__, __LINE__, mtype);
	}

	sem_mutex_unlock(lock_id);
	
	return SW_OK;
}

sw_int_t mtype_delete(sw_int_t mtype, sw_int32_t flag)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_delete(g_shm_mtype, mtype, flag));
}

sw_int_t mtype_loc_delete_info(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int32_t flag
		, sw_char_t *server, sw_char_t *service)
{
	sw_mtype_node_t *mtype_item = NULL;
	sw_int32_t	lock_id = -1;
	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	lock_id = shm_mtype->head.lock_id;
	mtype_item = &(shm_mtype->first);
	mtype_item += mtype - 1;

	/*lock*/
	sem_mutex_lock(lock_id);
	
	if (mtype_item->mtype != mtype || mtype_item->flag != MTYPE_USED)
	{
		sem_mutex_unlock(lock_id);
		pub_log_error("%s, %d, mtype=%d[%d]'s status abnormal, next=%d."
				, __FILE__, __LINE__, mtype, mtype_item->mtype, mtype_item->flag);
		return SW_ERROR;
	}
	
	strcpy(server, mtype_item->server);
	strcpy(service, mtype_item->service);
	pub_mem_memset((char *)(mtype_item), '\0', sizeof(sw_mtype_node_t));
	mtype_item->flag = MTYPE_IDLE;
	
	if(flag)
	{
		shm_mtype->head.fail_cnt++;
	}
	else
	{
		shm_mtype->head.succ_cnt ++;
	}
	
	shm_mtype->head.cur_cnt--;
	sem_mutex_unlock(lock_id);
	
	return SW_OK;
}

sw_int_t mtype_delete_info(sw_int_t mtype, sw_int32_t flag
		, sw_char_t *server, sw_char_t *service)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_delete_info(g_shm_mtype,  mtype,  flag, server, service));
}

sw_int_t  mtype_loc_save_link_info(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int64_t bi_no, sw_int64_t trace_no, char *key
		, char *sys_date, char *lsn_name, sw_int32_t timeout)
{
	sw_mtype_node_t	*mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, mtype[%d] > mtype_max[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
    	}
	
	mtype_item = &(shm_mtype->first);
	mtype_item += mtype - 1;

	if (bi_no == 0)
	{
		mtype_item->start_time = pub_time_get_current();
	}
	else
	{
		mtype_item->link_time = (sw_int_t)time(NULL);
		mtype_item->bi_no = bi_no;
	}

	if (trace_no > 0)
	{
		mtype_item->trace_no = trace_no;
	}
	
	if (key != NULL && strlen(key) != 0)
	{	
		strcpy(mtype_item->key, key);
		pub_str_zipspace(mtype_item->key);		
	}

	if (sys_date != NULL && strlen(sys_date) != 0)
	{
		strcpy(mtype_item->sys_date, sys_date);
		pub_str_zipspace(mtype_item->sys_date);
	}
	
	if (lsn_name != NULL &&lsn_name[0] != '\0')
	{
		strcpy(mtype_item->lsn_name, lsn_name);
	}
	
	if (timeout > 0)
	{
		mtype_item->timeout = timeout;
	}
	
	return SW_OK;
}

sw_int_t  mtype_save_link_info(sw_int_t mtype, sw_int64_t bi_no, sw_int64_t trace_no, char *key
		, char *sys_date, char *lsn_name, sw_int32_t timeout)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_save_link_info(g_shm_mtype,  mtype,  bi_no,  trace_no
			, key, sys_date, lsn_name, timeout));
}

sw_int_t mtype_loc_load_by_key(sw_mtype_t* shm_mtype, char *key, sw_int64_t *bi_no, sw_int64_t *trace_no
		, sw_int_t *mtype, char *sys_date)
{
	sw_mtype_node_t	*mtype_item = NULL;
	sw_char_t	key_buf[128];
	sw_int32_t	i = 0;
	
	pub_mem_memset(key_buf, '\0', sizeof(key_buf));
	strcpy(key_buf, key);
	pub_str_zipspace(key_buf);
	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	mtype_item = &(shm_mtype->first);
	
	for (i = 0; i < shm_mtype->head.mtype_max; i++)
	{
		if(mtype_item[i].flag != MTYPE_USED)
		{
			continue;
		}
		
		pub_log_info("%s, %d, key[%s] in mtype."
				, __FILE__, __LINE__, mtype_item[i].key);
		
		if (strcmp(mtype_item[i].key, key_buf) == 0)
		{
			break;
		}
	}
	
	if (i >= shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, key[%s] is not in shm_mtype."
				, __FILE__, __LINE__, key_buf);
		return SW_ERROR;
	}

	if (strlen(mtype_item[i].sys_date) > 0)
	{
		strncpy(sys_date, mtype_item[i].sys_date, sizeof(mtype_item[i].sys_date) - 1);
	}

	if (mtype_item[i].bi_no > 0)
	{
		*bi_no = mtype_item[i].bi_no;
	}

	if (mtype_item[i].trace_no>0)
	{
		*trace_no = mtype_item[i].trace_no;
	}

	*mtype = mtype_item[i].mtype;
	
	return SW_OK;
}

sw_int_t mtype_load_by_key(char *key, sw_int64_t *bi_no, sw_int64_t *trace_no
		, sw_int_t *mtype, char *sys_date)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_load_by_key(g_shm_mtype, key, bi_no, trace_no, mtype, sys_date));
}

sw_int_t mtype_loc_set_task_info(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_task_info_t *task_info)
{
	sw_mtype_node_t *mtype_item = NULL;
	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max || task_info == NULL)
	{
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);

	pub_mem_memcpy(&(mtype_item[mtype - 1].task_info), task_info, sizeof(sw_task_info_t));

	return SW_OK;
}

sw_int_t mtype_set_task_info(sw_int_t mtype, sw_task_info_t *task_info)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_task_info(g_shm_mtype, mtype, task_info));
}

sw_int_t mtype_loc_get_tx_info(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_trace_list_t *tx_list)
{
	sw_int64_t	time_value;
	sw_char_t	tmp[24];
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype < 1 || tx_list == NULL)
	{
		pub_log_error("%s, %d, Param error, mtype=[%d] tx_list=[%x]."
				, __FILE__, __LINE__, mtype, tx_list);
		return SW_ERROR;
	}
	
	mtype_item = &(shm_mtype->first);
	time_value = pub_time_get_current();

	tx_list->trace_no = mtype_item[mtype - 1].trace_no;
	strcpy(tx_list->server, mtype_item[mtype - 1].server);
	strcpy(tx_list->service, mtype_item[mtype - 1].service);
	strcpy(tx_list->pkg_resp, mtype_item[mtype - 1].pkg_response);
	strcpy(tx_list->resp, mtype_item[mtype - 1].sys_response);

	time_value = time_value - mtype_item[mtype - 1].start_time;
	sprintf(tx_list->d_time, "%lld", time_value / 1000);

	pub_mem_memzero(tmp, sizeof(tmp));
	pub_change_time2(mtype_item[mtype - 1].start_time, tmp, 0);
	strcpy(tx_list->start_time,tmp);
	
	return SW_OK;	
}

sw_int_t mtype_get_tx_info(sw_int_t mtype, sw_trace_list_t *tx_list)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_tx_info(g_shm_mtype, mtype, tx_list));
}

sw_int_t mtype_loc_get_task_info(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_task_info_t *task_info)
{
	sw_mtype_node_t	*mtype_item = NULL;
	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if(mtype <= 0 || mtype > shm_mtype->head.mtype_max || task_info == NULL)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	pub_mem_memcpy(task_info, &(mtype_item[mtype - 1].task_info), sizeof(sw_task_info_t));
	
	pub_log_info("%s, %d, mtype_item[%p] mtype_item + mtype - 1[%p]"
			, __FILE__, __LINE__, mtype_item, (mtype_item + mtype - 1));

	return SW_OK;
}

sw_int_t mtype_get_task_info(sw_int_t mtype, sw_task_info_t *task_info)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_task_info(g_shm_mtype, mtype, task_info));
}

sw_int_t mtype_loc_set_prdt(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_char_t* prdt)
{
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL || prdt == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if(mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	
	memset(mtype_item[mtype - 1].prdt_name, 0x0, sizeof(mtype_item[mtype - 1].prdt_name));
	strncpy(mtype_item[mtype - 1].prdt_name
		,prdt, sizeof(mtype_item[mtype - 1].prdt_name) - 1);

	return SW_OK;
}

sw_int_t mtype_set_prdt(sw_int_t mtype, sw_char_t* prdt)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_prdt(g_shm_mtype, mtype, prdt));
}

sw_int_t mtype_loc_get_prdt(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_char_t* prdt)
{
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL || prdt == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if(mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	
	strncpy(prdt, mtype_item[mtype - 1].prdt_name
		,sizeof(mtype_item[mtype - 1].prdt_name) - 1);

	return SW_OK;
}

sw_int_t mtype_get_prdt(sw_int_t mtype, sw_char_t* prdt)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_prdt(g_shm_mtype, mtype, prdt));
}

sw_int_t mtype_loc_get_svr_svc(sw_mtype_t* shm_mtype, sw_int_t mtype, char *server, char *service)
{
	sw_int32_t	type = -1;
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if(mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	type = mtype_item[mtype - 1].task_type;
	
	if (server != NULL)
	{
		strncpy(server, mtype_item[mtype - 1].server
			, sizeof(mtype_item[mtype - 1].server) - 1);
	}

	if (service != NULL)
	{
		strncpy(service, mtype_item[mtype - 1].service
			, sizeof(mtype_item[mtype - 1].service) - 1);
	}

	return type;
}

sw_int_t mtype_get_svr_svc(sw_int_t mtype, char *server, char *service)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_svr_svc(g_shm_mtype, mtype, server, service));
}

sw_int_t mtype_loc_set_svr_svc(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int32_t type
		, char *server, char *service)
{
	sw_mtype_node_t *mtype_item;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if(mtype <= 0 || mtype > shm_mtype->head.mtype_max || type < 0)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d] type[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max, type);
		return SW_ERROR;
	}


	mtype_item = &(shm_mtype->first);
	
	if (type > 0)
	{
		mtype_item[mtype-1].task_type = type ;
	}

	if (server != NULL)
	{
		strncpy(mtype_item[mtype - 1].server, server
			, sizeof(mtype_item[mtype - 1].server) - 1);
	}

	if (service != NULL)
	{
		strncpy(mtype_item[mtype - 1].service, service
			, sizeof(mtype_item[mtype - 1].service) - 1);
	}

	pub_log_info("%s, %d, type[%d][%d] server[%s][%s] service[%s][%s] trace_no[%lld]"
			,__FILE__,__LINE__, type, mtype_item[mtype - 1].task_type
			, server, mtype_item[mtype - 1].server, service
			, mtype_item[mtype - 1].service, mtype_item[mtype - 1].trace_no);

	return SW_OK;
}

sw_int_t mtype_set_svr_svc(sw_int_t mtype, sw_int32_t type, char *server, char *service)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_svr_svc(g_shm_mtype, mtype, type, server, service));
}

sw_task_info_t * mtype_loc_get_task_addr(sw_mtype_t* shm_mtype, sw_int_t mtype)
{
	sw_mtype_node_t	*mtype_item = NULL;
	sw_task_info_t	*task_info = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return NULL;
	}

	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return NULL;
	}

	mtype_item = &(shm_mtype->first);
	task_info = &(mtype_item[mtype - 1].task_info);
	pub_log_info("%s, %d, mtype_item[%p] mtype_item[mtype - 1][%p]"
			, __FILE__, __LINE__, mtype_item, &(mtype_item[mtype - 1]));

	return task_info;
}

sw_task_info_t * mtype_get_task_addr(sw_int_t mtype)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return NULL;
	}

	return (mtype_loc_get_task_addr(g_shm_mtype, mtype));
}

sw_int_t mtype_loc_get_status(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int32_t *status
		, sw_int32_t *task_status)
{
	sw_mtype_node_t	*mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	
	if (status != NULL)
	{
		*status = mtype_item[mtype - 1].task_info.status;
	}
	
	if (task_status != NULL)
	{
		*task_status = mtype_item[mtype - 1].task_status;
	}

	return SW_OK;
}

sw_int_t mtype_get_status(sw_int_t mtype, sw_int32_t *status
		, sw_int32_t *task_status)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_status(g_shm_mtype, mtype, status, task_status));
}

sw_int_t mtype_loc_set_status(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int32_t status
		, sw_int32_t task_status)
{
	sw_mtype_node_t * mtype_item = NULL;
		
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max || status < 0)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d] status[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max, status);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	mtype_item[mtype - 1].task_info.status = status;
	mtype_item[mtype - 1].task_status = task_status;

	return SW_OK;
}

sw_int_t mtype_set_status(sw_int_t mtype, sw_int32_t status, sw_int32_t task_status)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return (mtype_loc_set_status(g_shm_mtype, mtype, status, task_status));
}

sw_int32_t mtype_loc_check_timeout(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int32_t timeout)
{
	sw_int32_t	status = 0;
	sw_mtype_node_t *mtype_item = NULL;
	sw_int64_t	time_value = 0;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max || timeout < 0)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d] timeout[%d]"
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max, timeout);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	
	time_value = pub_time_get_current();
	if ((time_value - mtype_item[mtype - 1].start_time) / 1000000 >= timeout)
	{
		mtype_item[mtype-1].task_status = TASK_TIMEOUT;
		status = TASK_TIMEOUT;
	}
	
	return status;
}

sw_int32_t mtype_check_timeout(sw_int_t mtype, sw_int32_t timeout)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_check_timeout(g_shm_mtype, mtype, timeout));
}

sw_int32_t mtype_loc_get_timeout(sw_mtype_t* shm_mtype, sw_int_t mtype)
{
	sw_mtype_node_t * mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	
	return (mtype_item[mtype - 1].timeout);
}

sw_int32_t mtype_get_timeout(sw_int_t mtype)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_timeout(g_shm_mtype, mtype));
}

sw_int_t mtype_loc_set_err(sw_mtype_t* shm_mtype, sw_int32_t force_flag, sw_int_t mtype, sw_int32_t mode_type
	, sw_int32_t mode_num, sw_int32_t err_code1, sw_int32_t err_code2, char *err_msg)
{
	sw_mtype_node_t	*mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max 
		|| mode_type <= 0 || err_code1 <= 0 
		|| err_code2 <= 0 || err_msg == NULL)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);

	if (force_flag == 1 
		|| (mtype_item[mtype - 1].task_info.err_info.err_code1 == 0 
		&& mtype_item[mtype-1].task_info.err_info.err_code2 == 0))
	{
		mtype_item[mtype - 1].task_info.err_info.mode_type = mode_type;
		mtype_item[mtype - 1].task_info.err_info.mode_num = mode_num;
		mtype_item[mtype - 1].task_info.err_info.err_code1 = err_code1;
		mtype_item[mtype - 1].task_info.err_info.err_code2 = err_code2;
		strncpy(mtype_item[mtype-1].task_info.err_info.err_msg
			, err_msg, sizeof(mtype_item[mtype-1].task_info.err_info.err_msg));
	}

	return SW_OK;
}

sw_int_t mtype_set_err(sw_int32_t force_flag, sw_int_t mtype, sw_int32_t mode_type
	, sw_int32_t mode_num, sw_int32_t err_code1, sw_int32_t err_code2, char *err_msg)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_err(g_shm_mtype, force_flag, mtype, mode_type
			, mode_num, err_code1, err_code2, err_msg));
}

sw_int_t mtype_loc_set_saf_flow(sw_mtype_t* shm_mtype, sw_int_t mtype, char *saf_flow, sw_int32_t timeout)
{
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);

	if (saf_flow != NULL)
	{
		strncpy(mtype_item[mtype - 1].saf_flow, saf_flow
			, sizeof(mtype_item[mtype - 1].saf_flow));
	}
	
	if (timeout > 0)
	{
		mtype_item[mtype - 1].calllsn_timeout = timeout; 
	}

	return SW_OK;
}

sw_int_t mtype_set_saf_flow(sw_int_t mtype, char *saf_flow, sw_int32_t timeout)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_saf_flow(g_shm_mtype, mtype, saf_flow, timeout));
}

sw_int_t mtype_loc_get_saf_flow(sw_mtype_t* shm_mtype, sw_int_t mtype, char *saf_flow
		, sw_int32_t *time_value)
{
	sw_mtype_node_t	*mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}
	
	mtype_item = &(shm_mtype->first);

	if (saf_flow != NULL)
	{
		strcpy(saf_flow, mtype_item[mtype - 1].saf_flow);
	}
	
	if (time_value != NULL)
	{
		*time_value = mtype_item[mtype - 1].calllsn_timeout;
	}
	
	return SW_OK;
}

sw_int_t mtype_get_saf_flow(sw_int_t mtype, char *saf_flow, sw_int32_t *time_value)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_saf_flow(g_shm_mtype,  mtype, saf_flow, time_value));
}

sw_int_t mtype_loc_set_resp(sw_mtype_t* shm_mtype, int mtype, char *resp, char *sys_resp)
{
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max
		|| resp == NULL || strlen(resp) == 0 
		|| sys_resp == NULL || strlen(sys_resp) == 0)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	strcpy(mtype_item[mtype - 1].pkg_response, resp);
	strcpy(mtype_item[mtype - 1].sys_response, sys_resp);
	
	return SW_OK;
}

sw_int_t mtype_set_resp(int mtype, char *resp, char *sys_resp)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_resp(g_shm_mtype, mtype, resp, sys_resp));
}

sw_int_t mtype_loc_get_resp(sw_mtype_t* shm_mtype, int mtype, char *resp, char *sys_resp)
{
	sw_mtype_node_t	*mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max
		||  sys_resp == NULL || resp == NULL)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}
	
	mtype_item = &(shm_mtype->first);
	strcpy(resp, mtype_item[mtype - 1].pkg_response);
	strcpy(sys_resp, mtype_item[mtype - 1].sys_response);
	
	return SW_OK;
}

sw_int_t mtype_get_resp(int mtype, char *resp, char *sys_resp)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_resp(g_shm_mtype, mtype, resp, sys_resp));
}

sw_int_t mtype_loc_get_lsn(sw_mtype_t* shm_mtype, int mtype, char *lsn_name)
{
	sw_mtype_node_t	*mtype_item = NULL;
	
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max
		||  lsn_name == NULL)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	strcpy(lsn_name, mtype_item[mtype - 1].lsn_name);
	
	return SW_OK;
}

sw_int_t mtype_get_lsn(int mtype, char *lsn_name)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_lsn(g_shm_mtype, mtype, lsn_name));
}

sw_int_t mtype_loc_get_head(sw_mtype_t* shm_mtype, sw_mtype_head_t *head)
{
	sw_int32_t	lock_id;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (head == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	lock_id = shm_mtype->head.lock_id;

	/*lock*/
	sem_mutex_lock(lock_id);
	pub_mem_memcpy(head, &(shm_mtype->head), sizeof(sw_mtype_head_t));
	sem_mutex_unlock(lock_id);
	
	return SW_OK;
}

sw_int_t mtype_get_head(sw_mtype_head_t *head)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_head(g_shm_mtype, head));
}

sw_int_t mtype_loc_set_date(sw_mtype_t* shm_mtype, sw_int_t mtype, sw_int64_t trace_no
		, char *sys_date)
{
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);
	mtype_item[mtype - 1].trace_no = trace_no;
	strncpy(mtype_item[mtype-1].sys_date, sys_date
		, sizeof(mtype_item[mtype - 1].sys_date) - 1);

	return SW_OK;
}

sw_int_t mtype_set_date(sw_int_t mtype, sw_int64_t trace_no
		, char *sys_date)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_set_date(g_shm_mtype, mtype, trace_no, sys_date));
}

sw_int_t mtype_loc_get_date(sw_mtype_t* shm_mtype, int mtype, sw_int64_t *trace_no, char *sys_date)
{
	sw_mtype_node_t *mtype_item = NULL;

	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("%s, %d, Param error, mtype[%d] mtype_max[%d]."
				, __FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}

	mtype_item = &(shm_mtype->first);

	if (trace_no != NULL)
	{
		*trace_no = mtype_item[mtype - 1].trace_no ;
	}

	if (sys_date != NULL)
	{
		strncpy(sys_date,mtype_item[mtype - 1].sys_date
			, sizeof(mtype_item[mtype - 1].sys_date) - 1);
	}

	return SW_OK;
}

sw_int_t mtype_get_date(int mtype, sw_int64_t *trace_no, char *sys_date)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("%s, %d, g_shm_mtype == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (mtype_loc_get_date(g_shm_mtype, mtype, trace_no, sys_date));
}

/* flag描述对应信息 */
static const mtype_flag_desc_t g_mtype_flag_desc[] = 
{
    {MTYPE_IDLE, "IDLE"}
    , {MTYPE_TAIL, "TAIL"}
    , {MTYPE_USED, "USED"}
};

/******************************************************************************
 ** Name : mtype_get_flag_desc
 ** Desc : 获取flag描述信息
 ** Input: NONE
 ** Output: NONE
 ** Return: 描述信息
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
const char *mtype_get_flag_desc(int flag)
{
    int idx=0, size=0;

    size = sizeof(g_mtype_flag_desc)/sizeof(mtype_flag_desc_t);
    for(idx=0; idx<size; idx++)
    {
        if(g_mtype_flag_desc[idx].flag == flag)
        {
            return g_mtype_flag_desc[idx].dest;
        }
    }

    return "UNKN";
}

sw_int_t mtype_loc_set_time(sw_mtype_t *shm_mtype)
{
	if (shm_mtype == NULL)
	{
		pub_log_error("[%s][%d] Param error, shm mtype is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	shm_mtype->head.start_time = pub_time_get_current();
	pub_log_debug("[%s][%d] start_time=[%lld]", __FILE__, __LINE__, shm_mtype->head.start_time);

        return SW_OK;
}

sw_int_t mtype_set_time()
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("[%s][%d] Param error, g_shm_mtype is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return mtype_loc_set_time(g_shm_mtype);
}

sw_int64_t mtype_loc_get_time(sw_mtype_t *shm_mtype)
{
	if (shm_mtype == NULL)
	{
		pub_log_error("[%s][%d] Param error, shm_mtype is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return shm_mtype->head.start_time;
}

sw_int64_t mtype_get_time()
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("[%s][%d] Param error, g_shm_mtype is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return mtype_loc_get_time(g_shm_mtype);
}

sw_int_t mtype_loc_set_info(sw_mtype_t *shm_mtype, sw_int_t mtype, const sw_cmd_t *cmd)
{
        sw_mtype_node_t	*mtype_item = NULL;
	
	if (shm_mtype == NULL)
	{
		pub_log_error("[%s][%d] shm_mtype is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (mtype <= 0 || mtype > shm_mtype->head.mtype_max)
	{
		pub_log_error("[%s][%d] Param error, mtype=[%d] mtype_max=[%d]",
			__FILE__, __LINE__, mtype, shm_mtype->head.mtype_max);
		return SW_ERROR;
	}	

	mtype_item = &(shm_mtype->first);
	if (cmd->dst_prdt[0] != '\0')
	{
		memset(mtype_item[mtype - 1].prdt_name, 0x0, sizeof(mtype_item[mtype - 1].prdt_name));
		memcpy(mtype_item[mtype - 1].prdt_name, cmd->dst_prdt, sizeof(mtype_item[mtype - 1].prdt_name) - 1);
	}
	
	if (cmd->dst_svr[0] != '\0')
	{
		memset(mtype_item[mtype - 1].server, 0x0, sizeof(mtype_item[mtype - 1].server));
		memcpy(mtype_item[mtype - 1].server, cmd->dst_svr, sizeof(mtype_item[mtype - 1].server) - 1);
	}
	
	if (cmd->def_name[0] != '\0')
	{
		memset(mtype_item[mtype - 1].service, 0x0, sizeof(mtype_item[mtype - 1].service));
		memcpy(mtype_item[mtype - 1].service, cmd->def_name, sizeof(mtype_item[mtype - 1].service) - 1);
	}

	if (cmd->sys_date[0] != '\0')
	{
		memset(mtype_item[mtype - 1].sys_date, 0x0, sizeof(mtype_item[mtype - 1].sys_date));
		memcpy(mtype_item[mtype - 1].sys_date, cmd->sys_date, sizeof(mtype_item[mtype - 1].sys_date) - 1);
	}

	if (cmd->trace_no > 0)
	{
		mtype_item[mtype - 1].trace_no = cmd->trace_no;
	}
	
	if (cmd->lsn_name[0] != '\0')
	{
		memset(mtype_item[mtype - 1].lsn_name, 0x0, sizeof(mtype_item[mtype - 1].lsn_name));
		memcpy(mtype_item[mtype - 1].lsn_name, cmd->lsn_name, sizeof(mtype_item[mtype - 1].lsn_name) - 1);
	}
	
	return SW_OK;
}

sw_int_t mtype_set_info(sw_int_t mtype, const sw_cmd_t *cmd)
{
	if (g_shm_mtype == NULL)
	{
		pub_log_error("[%s][%d] Param error, g_shm_mtype is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	return mtype_loc_set_info(g_shm_mtype, mtype, cmd);
}

