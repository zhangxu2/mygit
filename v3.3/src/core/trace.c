/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-06
 *** module  : Interface for route path in BP. 
 *** name    : route.c
 *** function: route sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "trace.h"
#include "pub_mem.h"
#include "pub_time.h"
#include "pub_log.h"
#include "sem_lock.h"
#include "pub_ares.h"

SW_PROTECTED sw_trace_info_t		*g_shm_trace = NULL;
SW_PROTECTED sw_trace_cache_t		*g_trace_cache = NULL;
SW_PROTECTED sw_int_t trace_set_com_info(sw_trace_t *route, sw_ejf_info_t *ejf);
SW_PROTECTED sw_int_t trace_save_route(int index);
SW_PROTECTED sw_int_t trace_free(int index);

extern char	g_eswid[64];

sw_int_t trace_init(sw_char_t* addr, sw_syscfg_t* syscfg)
{
	sw_int32_t	route_size = 0;
	sw_int32_t	cache_size = 0;
	sw_int32_t	lock_id = -1;

	if (addr == NULL || syscfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_trace = (sw_trace_info_t*)addr;
	route_size = sizeof(sw_trace_info_t) + (syscfg->session_max - 1) * sizeof(sw_trace_item_t);
	cache_size = sizeof(sw_trace_cache_t) + (syscfg->session_max / 2 - 1) * sizeof(sw_trace_t);
	g_trace_cache = (sw_trace_cache_t*)((sw_char_t*)g_shm_trace + route_size);

	pub_mem_memzero((sw_char_t*)g_shm_trace, route_size);
	pub_mem_memzero((sw_char_t*)g_trace_cache, cache_size);

	lock_id = sem_new_lock_id();
	if (lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_trace->head.count = syscfg->session_max;

	sem_write_lock(lock_id);
	g_trace_cache->head.lock_id = lock_id;
	g_trace_cache->head.count = syscfg->session_max;
	g_trace_cache->head.free = syscfg->session_max / 2;
	g_trace_cache->head.use = 0;
	sem_write_unlock(lock_id);

	return SW_OK;
}

sw_trace_info_t* trace_get_addr()
{
	return g_shm_trace;
}

sw_trace_cache_t* trace_get_cache_addr()
{
	return g_trace_cache;
}

sw_int_t trace_set_addr(sw_char_t *addr)
{
	sw_int32_t	route_size = 0;

	if (addr == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_trace = (sw_trace_info_t*)addr;
	route_size = sizeof(sw_trace_info_t) + (g_shm_trace->head.count - 1) * sizeof(sw_trace_item_t);
	g_trace_cache = (sw_trace_cache_t*)((sw_char_t*)g_shm_trace + route_size);

	return SW_OK;
}

sw_int32_t trace_get_size(sw_syscfg_t* syscfg)
{
	sw_int32_t	route_size = 0;
	sw_int32_t	cache_size = 0;

	if (syscfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	route_size = sizeof(sw_trace_info_t) + (syscfg->session_max - 1) * sizeof(sw_trace_item_t);
	cache_size = sizeof(sw_trace_cache_t) + (syscfg->session_max / 2 - 1) * sizeof(sw_trace_t);

	return (route_size + cache_size);
}

sw_int_t trace_set_com_info(sw_trace_t *route, sw_ejf_info_t *ejf)
{
	route->module_type = ejf->route.module_type;
	route->module_num = ejf->route.module_num;
	route->status = ejf->route.status;
	route->error_code = ejf->route.error_code;
	route->tx_type = ejf->route.tx_type;

	pub_mem_memcpy(route->node, ejf->route.node, sizeof(ejf->route.node));
	route->use = 1;

	return SW_OK;
}

sw_int_t trace_create_info(sw_cmd_t *cmd)
{
	sw_int32_t	index = -1;
	sw_trace_item_t	*trace_item = NULL;

	if (g_shm_trace == NULL || g_trace_cache == NULL)
	{
		pub_log_error("%s, %d, g_shm_trace == NULL or g_trace_cache == NULL"
				, __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (cmd == NULL)
	{
		pub_log_error("[%s][%d] Input param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	index = cmd->mtype - 1;
	trace_item = g_shm_trace->trace_item;

	if (trace_item[index].trace_no > 0 && cmd->trace_no != trace_item[index].trace_no)
	{
		pub_mem_memzero(&trace_item[index], sizeof(sw_trace_item_t)); 
		trace_item[index].head = -1;
		trace_item[index].last = -1;
		trace_item[index].current = 0;
		trace_item[index].trace_no = 0;
	}

	pub_time_getdate(trace_item[index].time, 2);
	trace_item[index].start_time = pub_time_get_current();
	trace_item[index].end_time = 0;
	trace_item[index].trace_no = cmd->trace_no;
	pub_log_info("[%s][%d] mtype====[%ld] trace_no=[%lld]", __FILE__, __LINE__, cmd->mtype, cmd->trace_no);
	strcpy(trace_item[index].start_date, cmd->sys_date);
	trace_item[index].route[0].start_time = trace_item[index].start_time;
	trace_item[index].route[0].module_type = cmd->ori_type;
	strcpy(trace_item[index].route[0].node, cmd->ori_svc);
	trace_item[index].route[0].tx_type = cmd->type;
	strncpy(trace_item[index].server, cmd->dst_svr, sizeof(trace_item[index].server) - 1);
	strncpy(trace_item[index].service, cmd->def_name, sizeof(trace_item[index].service) - 1);

	trace_item[index].flag = 0;
	trace_item[index].head = -1;
	trace_item[index].last = -1;
	trace_item[index].current = 1;
	trace_item[index].change_index = 0;
	g_shm_trace->head.cur_use++;

	return SW_OK;
}

sw_int_t trace_insert_svr(sw_int_t mtype, sw_char_t *server, sw_char_t *service, sw_int64_t trace_no)
{
	sw_int32_t	index = 0;
	sw_trace_item_t	*trace_item = NULL;

	if (g_shm_trace == NULL || g_trace_cache == NULL)
	{
		pub_log_error("%s, %d, g_shm_trace == NULL or g_trace_cache == NULL"
				, __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (mtype <= 0 || server == NULL || service == NULL || trace_no < 0)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	index = mtype - 1;
	trace_item = g_shm_trace->trace_item;

	if (trace_item[index].trace_no > 0 && trace_no == trace_item[index].trace_no)  
	{	
		pub_mem_memzero(trace_item[index].server, sizeof(trace_item[index].server));
		pub_mem_memzero(trace_item[index].service, sizeof(trace_item[index].service));
		pub_mem_memcpy(trace_item[index].server, server, sizeof(trace_item[index].server) - 1);
		pub_mem_memcpy(trace_item[index].service, service, sizeof(trace_item[index].service) - 1);
	}

	return SW_OK;
}

sw_int_t trace_save_route(int index)
{
	sw_int_t	len = 0;
	sw_int_t	result = SW_ERROR;
	sw_int32_t	i = 0;
	sw_int32_t	file_fd = -1;
	sw_int32_t	trace_size = 0;
	sw_int32_t	route_num = 0;
	sw_int32_t	lock_id = -1;
	sw_int32_t	route_size = 0;
	sw_int32_t	in_trace_num = 0;
	sw_int32_t	next = -1;
	sw_int32_t	current = 0;
	sw_char_t	*trace_buf = NULL;
	sw_char_t	date[33];
	sw_char_t	bak_name[128];
	sw_char_t	tmp[1024];
	sw_char_t	dir[128];
	sw_char_t	time_buf[32];
	sw_char_t       errstr[256]; 
	sw_char_t	tracestr[32];

	sw_trace_item_t *trace_item = NULL;
	sw_trace_t	*route = NULL;

	pub_mem_memzero(tmp, sizeof(tmp));
	pub_mem_memzero(date, sizeof(date));
	pub_mem_memzero(bak_name, sizeof(bak_name));
	pub_mem_memzero(dir, sizeof(dir));
	pub_mem_memzero(time_buf, sizeof(time_buf));
	pub_mem_memzero(errstr, sizeof(errstr));

	if (g_shm_trace == NULL || g_trace_cache == NULL)
	{
		pub_log_error("%s, %d, g_shm_trace == NULL or g_trace_cache == NULL"
				, __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (index < 0)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	} 

	trace_item = g_shm_trace->trace_item;
	route = g_trace_cache->route;
	lock_id = g_trace_cache->head.lock_id;

	if(trace_item[index].current < 0 || trace_item[index].trace_no <= 0)
	{
		pub_log_error("%s, %d, trace_save_route error out.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	trace_buf = (char *) malloc(sizeof(sw_trace_t) * 50);
	if (trace_buf == NULL)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	memset(tracestr, 0x0, sizeof(tracestr));
	if (g_mon_in_ares && g_in_alog && !g_seqs_in_ares)
	{
			sprintf(tracestr, "%s%0*lld", g_alog_trc_prefix, (int)(SW_TRACE_LEN - strlen(g_alog_trc_prefix)),
			trace_item[index].trace_no);
	}
	else
	{
		sprintf(tracestr, "%lld", trace_item[index].trace_no);
	}
	
	sprintf(trace_buf,"TOTAL:%s|%s| %s | %s |%s|%lld|%lld|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%lld|%s|%s|\n", 
			trace_item[index].tx_code, tracestr, trace_item[index].server, 
			trace_item[index].service, trace_item[index].start_date, trace_item[index].start_time, 
			trace_item[index].end_time, trace_item[index].tx_respcd, trace_item[index].prdt_name, 
			trace_item[index].tx_amt, trace_item[index].dr_ac_no, trace_item[index].cr_ac_no, 
			trace_item[index].dr_ac_name, trace_item[index].cr_ac_name, trace_item[index].fee_amt, 
			trace_item[index].fee_ac_no, trace_item[index].ct_ind, trace_item[index].dc_ind, 
			trace_item[index].chnl, trace_item[index].time, trace_item[index].sys_errcode, 
			trace_item[index].tx_errmsg, trace_item[index].busi_no, trace_item[index].resflag, g_eswid);
	trace_size = strlen(trace_buf);

	in_trace_num = 0;
	if (trace_item[index].current <= MAXROUTE)
	{
		in_trace_num = trace_item[index].current;
	}
	else
	{
		in_trace_num = MAXROUTE;
	}

	route_size = 0;
	route_num = 0;
	for (i = 0; i < in_trace_num; i++)
	{
		pub_mem_memzero(tmp, sizeof(tmp));
		sprintf(tmp, "DETAIL:%d|%d|%d|%s|%lld|%lld|%d|%d|\n"
				, route_num, trace_item[index].route[i].status, trace_item[index].route[i].tx_type
				, trace_item[index].route[i].node, trace_item[index].route[i].start_time
				, trace_item[index].route[i].end_time, trace_item[index].route[i].error_code, trace_item[index].route[i].use);
		pub_mem_memcpy(trace_buf + trace_size + route_size, tmp, strlen(tmp));
		route_size += strlen(tmp);
		route_num++;
	}

	current = -1;
	next = -1;
	if (trace_item[index].current > MAXROUTE)
	{
		current = trace_item[index].head;
		while (current >= 0)
		{
			pub_mem_memzero(tmp, sizeof(tmp));
			sprintf(tmp, "DETAIL:%d|%d|%d|%s|%lld|%lld|%d|\n", route_num
					, route[current].status, route[current].tx_type, route[current].node
					, route[current].start_time, route[current].end_time
					, route[current].error_code);

			pub_mem_memcpy(trace_buf + trace_size + route_size, tmp, strlen(tmp));
			route_size += strlen(tmp);
			route_num ++;
			next = route[current].next;

			result = sem_write_lock(lock_id);
			if (result != SW_OK)
			{
				pub_log_error("%s, %d, lock error, errno[%d][%s]."
						, __FILE__, __LINE__, errno, strerror(errno));
				free(trace_buf);
				trace_buf = NULL;
				return SW_ERROR;
			}

			trace_free(current);

			result = sem_write_unlock(lock_id);
			if(result != SW_OK)
			{
				pub_log_error("%s, %d, unlock error, errno[%d][%s]."
						, __FILE__, __LINE__, errno, strerror(errno));
				free(trace_buf);
				trace_buf = NULL;
				return SW_ERROR;
			}

			current = next;
		}
	}

	if (trace_item[index].current != route_num && trace_item[index].current <= MAXROUTE)
	{
		pub_log_error("%s, %d, trace_item[index].current[%d] != route_num[%d]."
				, __FILE__, __LINE__, trace_item[index].current, route_num);
		free(trace_buf);
		trace_buf = NULL;
		return SW_ERROR;
	}

	trace_size += route_size;
	sprintf(trace_buf + trace_size, "\n");
	trace_size++; 	

	pub_mem_memzero(date, sizeof(date));
	pub_time_getdate(date, 1);
	pub_log_info("%s, %d, date=[%s].", __FILE__, __LINE__, date);
	if (g_mon_in_ares)
	{
		char trace_len[64];
		memset(trace_len, 0x0, sizeof(trace_len));
		sprintf(trace_len, "%s%08d", ARES_SEP, trace_size);
		strcat(trace_buf, trace_len);
		trace_size += strlen(trace_len);

		result = ares_comm(g_ares_fd, trace_buf, trace_size, TRACE_WRITE_MONITOR);
		if (result < 0)
		{
			pub_log_error("[%s][%d] Ares save trace_info traceno error!", __FILE__, __LINE__);
			trace_size -= strlen(trace_len);
			trace_buf[trace_size] = '\0';
			goto WRITE_FILE;
		}

		goto END;
	}
WRITE_FILE:
	if (getenv("SWWORK") != NULL)
	{
		pub_mem_memzero(dir, sizeof(dir));
		sprintf(dir, "%s/tmp/monitor/%s", getenv("SWWORK"), date);
		result = access(dir, W_OK);
		if (result < 0)
		{
			if (errno == ENOENT)
			{
				result = pub_file_check_dir(dir);
				if (result < 0)
				{
					pub_log_error("%s, %d, mkdir [%s]error, errno=[%d]:[%s]."
							, __FILE__, __LINE__, dir, errno, strerror(errno));
					free(trace_buf);
					trace_buf = NULL;
					return SW_ERROR;
				}
			}
			else
			{
				pub_log_error("%s, %d, access [%s] error, errno=[%d]:[%s]."
						, __FILE__, __LINE__, dir, errno, strerror(errno));
				free(trace_buf);
				trace_buf = NULL;
				return SW_ERROR;
			}
		}

		pub_mem_memzero(tmp, sizeof(tmp));
		sprintf(tmp, "%s/monitor.log", dir);
	}
	else
	{
		pub_log_error("%s, %d, trace_save_route error, out.", __FILE__, __LINE__);
		free(trace_buf);
		trace_buf = NULL;
		return SW_ERROR;
	}

	file_fd = open(tmp, O_WRONLY |  O_CREAT, 0777);
	if (file_fd == -1)
	{
		pub_log_error("%s, %d, open [%s]fail, errno[%d][%s]."
				, __FILE__, __LINE__, tmp, errno, strerror(errno));
		free(trace_buf);
		trace_buf = NULL;
		return SW_ERROR;
	}

	result = pub_lock_fd(file_fd);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, lock file error,errno[%d][%s]",__FILE__,__LINE__,tmp,errno,strerror(errno));
		close(file_fd);
		free(trace_buf);
		trace_buf = NULL;
		return SW_ERROR;
	}

        len = lseek(file_fd, 0, SEEK_END);
        if (write(file_fd, trace_buf, trace_size) != trace_size)
        {
                free(trace_buf);
                trace_buf = NULL;
                close(file_fd);
                return SW_ERROR;
        }

        len += trace_size;
        close(file_fd);

	if (len >= MAX_BAK_SIZE)
	{
		pub_mem_memzero(time_buf, sizeof(time_buf));
		pub_time_getdate(time_buf, 2);
		pub_log_info("%s, %d, time_buf=[%s].", __FILE__, __LINE__, time_buf); 

		pub_mem_memzero(bak_name, sizeof(bak_name));

		sprintf(bak_name, "%s/monitor_%s.log", dir, time_buf);
		pub_log_info("%s, %d, bak_name=[%s]", __FILE__, __LINE__, bak_name);

		if ((result = rename(tmp, bak_name)) != 0)
		{
			pub_log_error("%s, %d, rename [%s] to [%s] error,error[%d][%s]."
					, __FILE__, __LINE__, tmp, bak_name, errno, strerror(errno));
		}
	}
END:
	free(trace_buf);
	trace_buf = NULL;

	return SW_OK;
}

sw_int_t trace_free(int index)
{
	sw_trace_t	*route = NULL;
	sw_tcache_head_t *cache_head = NULL;

	route = (sw_trace_t *)g_trace_cache->route;
	cache_head = &(g_trace_cache->head);

	cache_head->last = route[index].prior;
	cache_head->use--;
	cache_head->free++;
	pub_mem_memzero(&route[index], sizeof(sw_trace_t));
	route[index].use = 0;

	return SW_ERROR;
}

sw_int_t trace_get_error(sw_char_t *tmp, sw_int32_t error_code)
{
	switch(error_code)
	{
		case 97:
			strcpy(tmp, "Get pkg from mq error");
			break;
		case 98:
			strcpy(tmp, "Search mapdeal error");
			break;
		case 99:
			strcpy(tmp, "deal switch to pack error");
			break;
		case 100:
			strcpy(tmp, "deal pack to switch error");
			break;
		case 101:
			strcpy(tmp, "Send Msg failed");
			break;
		case 102:
			strcpy(tmp, "GetFindItem error");
			break;
		case 103:
			strcpy(tmp, "iAddSubItem error");
			break;
		case 104:
			strcpy(tmp, "iRunFlowWork error");
			break;
		case 105:
			strcpy(tmp, "iSafUpdateSVC error");
			break;
		case 106:
			strcpy(tmp, "iAfterFlowWork error");
			break;
		case 107:
			strcpy(tmp, "Lisnten not active");
			break;
		case 999:
			strcpy(tmp, "unknow");
			break;
		case 0 :
			strcpy(tmp, "Success!");
			break;
		default:
			sprintf(tmp, "Unknown %d", error_code);
			break;

	}

	return SW_OK;
}

sw_int_t trace_get_msg_type(sw_char_t *tmp, sw_int32_t type)
{
	switch(type)
	{
		case SW_CALLLSNREQ:
			strcpy(tmp, "CALLSN request");
			break;
		case SW_CALLLSNRES:
			strcpy(tmp, "CALLSN response");
			break;
		case SW_POSTLSNREQ:
			strcpy(tmp, "POSTLSN request");
			break;
		case SW_POSTLSNRES:
			strcpy(tmp, "POSTLSN response");
			break;
		case SW_CALLREQ:
			strcpy(tmp, "CALLSVC request");
			break;
		case SW_CALLRES:
			strcpy(tmp, "CALLSVC response");
			break;
		case SW_POSTREQ:
			strcpy(tmp, "POSTSVC request");
			break;
		case SW_POSTRES:
			strcpy(tmp, "POSTSVC response");
			break;
		case SW_LINKREQ:
			strcpy(tmp, "LINKSVC request");
			break;
		case SW_LINKLSNREQ:
			strcpy(tmp, "LINKLSN response");
			break;
		case SW_LINKLSNRES:
			strcpy(tmp, "linklsnres");
			break;
		default:
			strcpy(tmp, "Common");
	}

	return SW_OK;
}

const sw_trace_t *trace_get_exp_route(int idx)
{
	return &g_trace_cache->route[idx];
}

sw_int_t trace_insert(sw_loc_vars_t *vars, sw_cmd_t *cmd, int flag)
{
	sw_int32_t	result = 0;
	sw_int32_t	index = 0;
	sw_int32_t	current = 0;
	sw_int64_t	trace_no = 0;
	sw_char_t	buf[128];
	sw_char_t	errcode[16];
	sw_char_t	tx_code[16];
	sw_char_t	respcd[16];
	sw_char_t	busi_no[32];
	sw_char_t	errmsg[1024];
	sw_trace_item_t	*trace_item = NULL;

	memset(buf, 0x0, sizeof(buf));
	memset(errcode, 0x0, sizeof(errcode));
	memset(tx_code, 0x0, sizeof(tx_code));
	memset(respcd, 0x0, sizeof(respcd));
	memset(busi_no, 0x0, sizeof(busi_no));
	memset(errmsg, 0x0, sizeof(errmsg));

	if (vars == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Input param error is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (g_shm_trace == NULL || g_trace_cache == NULL)
	{
		pub_log_error("[%s][%d] g_shm_trace/g_trace_cache is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (cmd->mtype <= 0 || cmd->trace_no <= 0)
	{
		pub_log_error("[%s][%d] mtype or trace_no is error! mtype=[%ld] trace_no=[%lld]", 
				__FILE__, __LINE__, cmd->mtype, cmd->trace_no);
		return SW_ERROR;
	}

	index = cmd->mtype - 1;
	trace_item = g_shm_trace->trace_item;

	if (trace_item[index].trace_no != cmd->trace_no)
	{
		pub_log_error("[%s][%d] input trace_no=[%lld] not equal mtype[%ld].trace_no=[%lld]",
				__FILE__, __LINE__, cmd->trace_no, cmd->mtype, trace_item[index].trace_no);
		return SW_ERROR;
	}

	if (strlen(trace_item[index].prdt_name) == 0 && strlen(cmd->dst_prdt) > 0)
	{
		strcpy(trace_item[index].prdt_name, cmd->dst_prdt);
	}

	if (strlen(trace_item[index].tx_code) == 0)
	{
		memset(tx_code, 0x0, sizeof(tx_code));
		loc_get_zd_data(vars, "#txcode", tx_code);
		if (strlen(tx_code) > 0)
		{
			strcpy(trace_item[index].tx_code, tx_code);
		}
	}

	if (trace_item[index].tx_amt[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#txamt", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].tx_amt, buf, sizeof(trace_item[index].tx_amt) - 1);
		}
	}

	if (trace_item[index].dr_ac_no[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#cdtrid", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].dr_ac_no, buf, sizeof(trace_item[index].dr_ac_no) - 1);
		}
	}

	if (trace_item[index].cr_ac_no[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#dbtrid", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].cr_ac_no, buf, sizeof(trace_item[index].cr_ac_no) - 1);
		}
	}

	if (trace_item[index].dr_ac_name[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#cdtrnm", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].dr_ac_name, buf, sizeof(trace_item[index].dr_ac_name) - 1);
		}
	}

	if (trace_item[index].cr_ac_name[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#dbtrnm", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].cr_ac_name, buf, sizeof(trace_item[index].cr_ac_name) - 1);
		}
	}

	if (trace_item[index].fee_amt[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#feeamt", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].fee_amt, buf, sizeof(trace_item[index].fee_amt) - 1);
		}
	}

	if (trace_item[index].fee_ac_no[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#feeacno", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].fee_ac_no, buf, sizeof(trace_item[index].fee_ac_no) - 1);
		}
	}

	if (trace_item[index].ct_ind[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#ctind", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].ct_ind, buf, sizeof(trace_item[index].ct_ind) - 1);
		}
	}

	if (trace_item[index].dc_ind[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#cdind", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].dc_ind, buf, sizeof(trace_item[index].dc_ind) - 1);
		}
	}

	if (trace_item[index].chnl[0] == '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "$listen", buf);
		if (buf[0] != '\0')
		{
			strncpy(trace_item[index].chnl, buf, sizeof(trace_item[index].chnl) - 1);
		}
	}

	memset(errcode, 0x0, sizeof(errcode));
	memset(busi_no, 0x0, sizeof(busi_no));
	loc_get_zd_data(vars, "$errcode", errcode);
	/*** 业务流水 ***/
	loc_get_zd_data(vars, "$trace_no", busi_no);
	trace_no = atoll(busi_no);
	if (trace_item[index].busi_no <= 0 && trace_no > 0)
	{
		trace_item[index].busi_no = trace_no;
	}

	/*** 平台流水 ***/
	memset(busi_no, 0x0, sizeof(busi_no));
	loc_get_zd_data(vars, "$sys_trace_no", busi_no);
	trace_no = atoll(busi_no);
	if (trace_item[index].sys_no <= 0 && trace_no > 0)
	{
		trace_item[index].sys_no = trace_no;
	}
	
	alog_set_sysinfo(cmd->mtype, cmd->sys_date, cmd->trace_no, trace_item[index].chnl);

	if (flag == TRACE_IN)
	{
		trace_item[index].current++;
		current = trace_item[index].current - 1;
		if (current < MAXROUTE)
		{
			pub_mem_memzero(&trace_item[index].route[current], sizeof(sw_trace_t));
			trace_item[index].route[current].start_time = pub_time_get_current();
			trace_item[index].route[current].end_time = trace_item[index].route[current].start_time;
			trace_item[index].route[current].use = 1;
			if ((cmd->dst_type == ND_LSN && cmd->msg_type == SW_MSG_RES) ||
					(cmd->msg_type == SW_MSG_REQ))
			{
				trace_item[index].route[current].module_type = cmd->dst_type;
				if (pub_str_rstrncmp(cmd->dst_svc, "_snd", 4) == 0 || 
						pub_str_rstrncmp(cmd->dst_svc, "_rcv", 4) == 0)
				{
					strncpy(trace_item[index].route[current].node, cmd->dst_svc, strlen(cmd->dst_svc) - 4);
				}
				else
				{
					strcpy(trace_item[index].route[current].node, cmd->dst_svc);
				}
			}
			else
			{
				trace_item[index].route[current].module_type = cmd->ori_type;
				if (pub_str_rstrncmp(cmd->ori_svc, "_snd", 4) == 0 || 
						pub_str_rstrncmp(cmd->ori_svc, "_rcv", 4) == 0)
				{
					strncpy(trace_item[index].route[current].node, cmd->ori_svc, strlen(cmd->ori_svc) - 4);
				}
				else
				{
					strcpy(trace_item[index].route[current].node, cmd->ori_svc);
				}
			}
			trace_item[index].route[current].tx_type = cmd->type;
		}
	}
	else if (flag == TRACE_TIMEOUT)
	{
		current = trace_item[index].current - 1;
		if (current < MAXROUTE)
		{
			trace_item[index].route[current].end_time = pub_time_get_current();
			trace_item[index].route[current].error_code = atoi(errcode);
			trace_item[index].route[current].use = 1;
		}

		memset(errcode, 0x0, sizeof(errcode));
		loc_get_zd_data(vars, "$errcode", errcode);
		strcpy(trace_item[index].sys_errcode, errcode);

		memset(respcd, 0x0, sizeof(respcd));
		loc_get_zd_data(vars, "#errcode", respcd);
		strcpy(trace_item[index].tx_respcd, respcd);

		memset(errmsg, 0x0, sizeof(errmsg));
		loc_get_zd_data(vars, "#errmsg", errmsg);
		strncpy(trace_item[index].tx_errmsg, errmsg, sizeof(trace_item[index].tx_errmsg) - 1);

		trace_item[index].flag = TRACE_OVER;
		trace_item[index].end_time = pub_time_get_current();
		trace_item[index].resflag[0] = '0';
		result = trace_save_route(index);
		if (result != SW_OK)
		{
			pub_log_error("[%s][%d] save trace error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		current = trace_item[index].current - 1;
		if (current < MAXROUTE)
		{
			trace_item[index].route[current].end_time = pub_time_get_current();
			trace_item[index].route[current].error_code = atoi(errcode);
			trace_item[index].route[current].use = 1;
		}
		if (flag == TRACE_OVER)
		{
			memset(errcode, 0x0, sizeof(errcode));
			loc_get_zd_data(vars, "$errcode", errcode);
			strcpy(trace_item[index].sys_errcode, errcode);

			memset(respcd, 0x0, sizeof(respcd));
			loc_get_zd_data(vars, "#errcode", respcd);
			strcpy(trace_item[index].tx_respcd, respcd);

			memset(errmsg, 0x0, sizeof(errmsg));
			loc_get_zd_data(vars, "#errmsg", errmsg);
			strncpy(trace_item[index].tx_errmsg, errmsg, sizeof(trace_item[index].tx_errmsg) - 1);

			trace_item[index].flag = TRACE_OVER;
			trace_item[index].end_time = pub_time_get_current();
			trace_item[index].resflag[0] = '1';
			result = trace_save_route(index);
			if (result != SW_OK)
			{
				pub_log_error("[%s][%d] save trace error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
	}
	pub_log_info("[%s][%d] trace_insert success! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);

	return SW_OK;
}
