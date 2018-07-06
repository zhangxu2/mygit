/*************************************************
 *文 件 名:  flw_sp2026.c                        **
 *功能描述:  获取业务日志                        **
 *作    者:  薛辉                                **
 *完成日期:  20160803                            **
 **************************************************/
#include "agent_search.h"
#include "agent_comm.h"
#include "pub_buf.h"

typedef struct log_block_s
{
	int 	flag;
	char 	end_time[32];
	sw_buf_t	buf;
}log_block_t;

static sw_xmltree_t	*xml = NULL;
static int check_proc_type(char *proc)
{
	char filename[256];
	char buf[64];
	char *p = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	if (xml == NULL)
	{
		memset(filename, 0x0, sizeof(filename));
		snprintf(filename, sizeof(filename)-1, "%s/cfg/listener.lsn", getenv("SWWORK"));
		xml = pub_xml_crtree(filename);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] xml crtree error", __FILE__, __LINE__);
			return -1;
		}
	}

	memset(buf, 0x0, sizeof(buf));
	p = proc + strlen(proc);
	while(p != proc && *p != '_' )
	{
		--p;
	}
	memcpy(buf, proc, p - proc);
	
	node1 = agt_xml_search(xml, ".DFISBP.SWLISTEN", "NAME", buf);
	if(node1 == NULL)
	{
		return 0; /*svc*/
	}

	xml->current = node1;
	node2 = pub_xml_locnode(xml, "COMTYPE");
	if (node2 == NULL || node2->value == NULL)
	{
		pub_log_error("[%s][%d] %s cannot find COMTYPE", __FILE__, __LINE__, buf);
		return -1;
	}

	if (strcasecmp(node2->value, "TCPLA") == 0 ||
		strcasecmp(node2->value, "MQLA") == 0 ||
		strcasecmp(node2->value, "TUXLC") == 0)
	{
		return 2;/*la*/
	}

	return 1;/*lsn*/
}

/*从某个进程的日志文件中找到相关流水号的各个模块日志*/
static int get_log_block(char *path, char *proc, log_block_t *log_block, int *block_cnt, sw_trace_item_t *trace_item)
{
	int 	i = 0;
	int 	flag = 0;
	int     file_num = 0;
	char	end_time[32];
	char	date[32];
	char	buf[256];
	char	bak[256];
	char	filename[256];
	char	item[5][256];
	struct 	dirent **namelist = NULL;
	FILE 	*fp = NULL;
	
	if (0 != access(path, F_OK))
	{   
		pub_log_info ("[%s][%d] not exsist the file [%s]\n", __FILE__, __LINE__, path);
		return -1;
	}

	file_num = 0;
	namelist = agt_scan_dir(&file_num, path);;
	if (namelist == NULL)
	{
		pub_log_error("[%s][%d] scan dir error", __FILE__, __LINE__);
		return -1;
	}

	memset(buf, 0x0, sizeof(buf));
	pub_change_time(trace_item->start_time, buf, 2);
	memset(date, 0x0, sizeof(date));
	pub_change_time(trace_item->start_time, date, 3);
	memset(end_time, 0x0, sizeof(end_time));
	pub_change_time2(trace_item->end_time, end_time, 0);

	memset(bak, 0x0, sizeof(bak));
	sprintf(bak, "%s%s.bak", proc, buf);
	pub_log_debug("[%s][%d] bak %s end_time %s", __FILE__, __LINE__, bak, end_time);

	flag = 1;
	i = 0;
	while (i < file_num || flag)
	{
		memset(filename, 0x0, sizeof(filename));
		if (i < file_num)
		{
			if(strncmp(namelist[i]->d_name, proc, strlen(proc)) != 0 || strcmp(namelist[i]->d_name, bak) < 0) 
			{
				i++;
				continue;
			}
			sprintf(filename, "%s/%s", path, namelist[i]->d_name);
			i++;
		}	
		else
		{
			flag = 0;
			sprintf(filename, "%s/%s", path, proc);
		}
		pub_log_debug("[%s][%d] file %s", __FILE__, __LINE__, filename);
		fp = fopen(filename, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open [%s] error", __FILE__, __LINE__, path);
			agt_free_namelist(namelist, file_num);
			return -1;
		}

		while(!feof(fp))
		{
			memset(buf, 0x0, sizeof(buf));
			fgets(buf, sizeof(buf)-1, fp);
			buf_append(&(log_block[*block_cnt].buf), buf, strlen(buf));
			if (buf[0] != '@')
			{
				continue;
			}
			memset(item, 0x0, sizeof(item));
			agt_str_split(buf, "|", item, 5);
			if (strcmp(item[1], date) < 0)
			{
				continue;
			}
			
			if (item[3][0] == 'O')
			{
				if (log_block[*block_cnt].flag)
				{
					(*block_cnt)++;
				}
				else if (strcmp(item[1], date) > 0 || (strcmp(item[1], date) == 0 && strcmp(item[2], end_time) > 0))
				{
					pub_log_debug("[%s][%d] %s  %s", __FILE__, __LINE__, item[2], end_time);
					fclose(fp);
					agt_free_namelist(namelist, file_num);
					return 0;
				}
				else
				{
					buf_refresh(&(log_block[*block_cnt].buf));
					memset(log_block[*block_cnt].end_time, 0x0, sizeof(log_block[*block_cnt].end_time));
				}
				continue;
			}

			
			if (strlen(log_block[*block_cnt].end_time) == 0)
			{
				strncpy(log_block[*block_cnt].end_time, item[2], sizeof(log_block[*block_cnt].end_time)-1);
			}

			if (strncmp(item[3], "I cmdinfo:", 10) == 0)
			{
				memset(buf, 0x0, sizeof(buf));
				fgets(buf, sizeof(buf)-1, fp);
				buf_append(&(log_block[*block_cnt].buf), buf, strlen(buf));
				memset(item, 0x0, sizeof(item));
				agt_str_parse(buf, item, 5);
				if (atoll(item[1]) == trace_item->trace_no)
				{
					log_block[*block_cnt].flag = 1;
				}
			}
		}

		fclose(fp);
	}

	agt_free_namelist(namelist, file_num);
	return 0;
}


static int log_search_ext(char *date, long long trc_no)
{
	int     i = 0, j = 0;
	int 	ret = 0;
	int     file_num = 0;
	int 	flag = 0;
	int 	find = 0;
	int 	size = 0;
	int 	proc_cnt = 0;
	int 	detail_cnt = 0;
	int 	block_cnt = 0;
	char 	path[256];
	char 	filename[256];
	char	ptid[32];
	char 	buf[256];
	char	tmp[256];
	char 	filepath[256];
	char	proc[32];
	char	dstpath[256];
	char	end_time[32];
	char	bak[256];
	char 	*ptmp = NULL;
	char	item[5][256];
	char	(*procs)[64] = NULL;;
	FILE *fp = NULL, *fp2 = NULL;
	log_block_t	*log_block = NULL;
	struct dirent **namelist = NULL;
	sw_trace_item_t trace_item;

	memset(dstpath, 0x0, sizeof(dstpath));
	snprintf(dstpath, sizeof(dstpath)-1, "%s/dat/%s_%012lld.log", getenv("SWWORK"), date, trc_no);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/monitor/%s", getenv("SWWORK"), date);

	if (0 != access(path, F_OK))
	{   
		pub_log_error ("[%s][%d] not exsist the file [%s]\n", __FILE__, __LINE__, path);
		return -1;
	}

	i = 0;
	file_num = 0;
	namelist = agt_scan_dir(&file_num, path);;
	if (namelist == NULL)
	{
		pub_log_error("[%s][%d] scan dir error", __FILE__, __LINE__);
		return -1;
	}

	while (i < file_num)
	{
		if(strncmp(namelist[i]->d_name, "monitor", strlen("monitor")) != 0)
		{
			i++;
			continue;
		}
		memset(filename, 0x00, sizeof(filename));
		snprintf(filename, sizeof(filename)-1, "%s/%s", path, namelist[i]->d_name);
		i++;

		fp =  fopen(filename, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file [%s] error", __FILE__, __LINE__, filename);
			agt_free_namelist(namelist, file_num);
			return -1;
		}

		while(!feof(fp))
		{
			memset(buf, 0x00, sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			if ((ptmp = strstr(buf, "TOTAL:")) == NULL)
			{
				continue;
			}

			memset(&trace_item, 0x00, sizeof(trace_item));
			memset(ptid, 0x0, sizeof(ptid));
			agt_get_trace_info(ptmp, &trace_item, ptid);
			
			if (trace_item.trace_no == trc_no)
			{
				find = 1;
				break;
			}
			
		}
		if (find)
		{
			break;
		}
		fclose(fp);
	}
	agt_free_namelist(namelist, file_num);
	
	if(find == 0) 
	{
		pub_log_error("[%s][%d] cannot find trcno=[%lld]", __FILE__, __LINE__, trc_no);
		return -1;
	}

	size = ftell(fp);
	detail_cnt = 0;
	while(1)
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		if (strncmp(buf, "DETAIL:", strlen("DETAIL:")) != 0)
		{
			break;
		}
		detail_cnt++;	
	}

	procs = malloc(64 * detail_cnt);
	if(procs == NULL)
	{
		pub_log_error("[%s][%d] malloc error", __FILE__, __LINE__);
		fclose(fp);
		return -1;
	}

	memset(procs, 0x0, 64 * detail_cnt);

	log_block = malloc(sizeof(log_block_t) * detail_cnt * 2);
	if (log_block == NULL)
	{
		pub_log_error("[%s][%d] malloc error", __FILE__, __LINE__);
		free(procs);
		fclose(fp);
		return -1;
	}
	
	for (i = 0; i < detail_cnt * 2; ++i)
	{
		memset(log_block + i, 0x0, sizeof(log_block_t));
		pub_buf_init(&(log_block[i].buf));
	}

	proc_cnt = 0;
	fseek(fp, size, SEEK_SET);
	while(1)
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);

		if (strncmp(buf, "DETAIL:", strlen("DETAIL:")) != 0)
		{
			break;
		}

		memset(item, 0x0, sizeof(item));
		agt_str_split(buf, "|", item, 5);
		for (i = 0;  i < proc_cnt; ++i)
		{
			if (strcmp(procs[i], item[3]) == 0)
			{
				break;
			}
		}
		if (i == proc_cnt)
		{
			strncpy(procs[proc_cnt++], item[3], sizeof(proc));
		}
	}		
	fclose(fp);
	
	block_cnt = 0;
	for (i = 0; i < proc_cnt; ++i)
	{
		pub_log_debug("[%s][%d] proc name [%s]", __FILE__, __LINE__, procs[i]);
		memset(filepath, 0x0, sizeof(filepath));
		flag = check_proc_type(procs[i]);
		if (flag < 0)
		{
			pub_log_error("[%s][%d] check proc %s type error", __FILE__, __LINE__, procs + i);
			free(procs);
			for (i = 0; i < detail_cnt * 2; ++i)
			{
				pub_buf_clear(&(log_block[i].buf));
			}
			free(log_block);	
			return -1;
		}
		else if (flag == 0)
		{/*svc*/
			snprintf(filepath, sizeof(filepath), "%s/log/%s/syslog", getenv("SWWORK"), trace_item.prdt_name);
			memset(proc, 0x0, sizeof(proc));
			sprintf(proc, "%s.log", procs[i]);
			ret = get_log_block(filepath, proc, log_block, &block_cnt, &trace_item);
		}
		else if (flag == 2)
		{/*la*/
			snprintf(filepath, sizeof(filepath), "%s/log/syslog", getenv("SWWORK"));
			memset(proc, 0x0, sizeof(proc));
			sprintf(proc, "%s_snd.log", procs[i]);
			ret = get_log_block(filepath, proc, log_block, &block_cnt, &trace_item);
			memset(proc, 0x0, sizeof(proc));
			sprintf(proc, "%s_rcv.log", procs[i]);
			ret = get_log_block(filepath, proc, log_block, &block_cnt, &trace_item);
		}
		else 
		{/*lsn*/
			snprintf(filepath, sizeof(filepath), "%s/log/syslog", getenv("SWWORK"));
			memset(proc, 0x0, sizeof(proc));
			sprintf(proc, "%s_lsn.log", procs[i]);
			ret = get_log_block(filepath, proc, log_block, &block_cnt, &trace_item);
		}

		if (ret < 0)
		{
			pub_log_error("[%s][%d] get log block error proc:%s", __FILE__, __LINE__, proc);
			free(procs);
			for (i = 0; i < detail_cnt * 2; ++i)
			{
				pub_buf_clear(&(log_block[i].buf));
			}
			free(log_block);	
			return -1;
		}
		
	}
	free(procs);

	fp2 = fopen(dstpath, "w");	
	if (fp2 == NULL)
	{
		pub_log_error("[%s][%d] open file %s error", __FILE__, __LINE__, dstpath);
		for (i = 0; i < detail_cnt * 2; ++i)
		{
			pub_buf_clear(&(log_block[i].buf));
		}
		free(log_block);	
		return -1;
	}
	for (i = 0; i < block_cnt; ++i)
	{
		flag = -1;
		for (j = 0; j < block_cnt; ++j)
		{
			if (log_block[j].flag == 0)
			{
				continue;
			}
			if (flag < 0)
			{
				flag = j;
			}

			if (strcmp(log_block[flag].end_time, log_block[j].end_time) > 0)
			{
				flag = j;
			}
		}
	
		fputs(buf_string(&(log_block[flag].buf)), fp2);
		log_block[flag].flag = 0;
	}

	for (i = 0; i < detail_cnt * 2; ++i)
	{
		pub_buf_clear(&(log_block[i].buf));
	}
	free(log_block);	

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1,  "%s/log/%s/%s", getenv("SWWORK"), trace_item.prdt_name, trace_item.server);
	if (access(path, F_OK) != 0)
	{
		pub_log_error("[%s][%d] path is not exist", __FILE__, __LINE__);
		fclose(fp2);
		return -1;
	}
	memset(bak, 0x0, sizeof(bak));
	snprintf(bak, sizeof(bak)-1, "%s_%s", trace_item.service, date);
	
	memset(end_time, 0x0, sizeof(end_time));
	pub_change_time2(trace_item.end_time, end_time, 0);

	i = 0;
	file_num = 0;
	namelist = agt_scan_dir(&file_num, path);;
	if (namelist == NULL)
	{
		pub_log_error("[%s][%d] scan dir error", __FILE__, __LINE__);
		fclose(fp2);
		return -1;
	}

	while (i < file_num)
	{
		if(strncmp(namelist[i]->d_name, bak, strlen(bak)) != 0 || strstr(namelist[i]->d_name, "log") == NULL)
		{
			i++;
			continue;
		}
	
		memset(filename, 0x0, sizeof(filename));
		snprintf(filename, sizeof(filename)-1, "%s/%s", path, namelist[i]->d_name);
		i++;
		fp = fopen(filename, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file %s error", __FILE__, __LINE__, filename);
			agt_free_namelist(namelist, file_num);
			fclose(fp2);
			return -1;
		}

		while(!feof(fp))
		{
			memset(buf, 0x0, sizeof(buf));
			fgets(buf, sizeof(buf)-1, fp);
			memset(tmp, 0x0, sizeof(tmp));
			strncpy(tmp, buf, sizeof(buf)-1);
			memset(item, 0x0, sizeof(item));
			agt_str_split(tmp, "|", item, 5);
			if (atoll(item[0]) == trc_no)
			{
				fputs(buf, fp2);				
			}
			else if (strlen(item[1]) == 12 && strcmp(item[1], end_time) > 0)
			{
				break;
			}
		}
		fclose(fp);
	}
	fclose(fp2);
	agt_free_namelist(namelist, file_num);

	return 0;
}

int  get_log_mem(char *traceno, char *date, char *lsnname)
{
	char cmd[256];

	memset(cmd, 0x0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd) - 1, "swadmin get -log %ld %s %s", atol(traceno), date, lsnname);
	pub_log_info("[%s][%d] cmd=[%s]", __FILE__, __LINE__, cmd);
	return agt_system(cmd);

}


int sp2026(sw_loc_vars_t *vars)
{
	int     flag = 0;
	int     ch   = 0;
	int		result = SW_ERROR;
	long long	tracno = 0;
	char	reply[8];
	char    type[8];
	char    trc_no[32];
	char    buf[128];
	char	res_msg[256];
	char	sTime[256];
	char	send_path[256];
	struct search requ_info;
	log_buf_t log;
	FILE *fp = NULL;

	memset(&log, 0x0, sizeof(log));
	memset(reply, 0x00, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));


	/*get Params*/
	loc_get_zd_data(vars, ".TradeRecord.Request.Lsn", requ_info.Lsn);
	pub_log_error("%s, %d, Lsn  [%s]",__FILE__,__LINE__, requ_info.Lsn);
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Svc", requ_info.Svc);
	pub_log_info("[%s][%d]  Svc [%s]",__FILE__,__LINE__, requ_info.Svc);
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", requ_info.Prdt);
	pub_log_info("[%s][%d]  PrdtName [%s]",__FILE__,__LINE__, requ_info.Prdt);

	loc_get_zd_data(vars, ".TradeRecord.Request.Svr", requ_info.Svr);
	pub_log_error("[%s][%d] PrdtName [%s]",__FILE__,__LINE__, requ_info.Svr);

	loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", requ_info.PlatFlow);
	if (requ_info.PlatFlow[0] == '\0')
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.PlatFlow",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(type, 0x0, sizeof(type));
	loc_get_zd_data(vars, ".TradeRecord.Request.Type", type);
	pub_log_error("[%s][%d] PrdtName [%s]",__FILE__,__LINE__, type);

	loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", requ_info.TxDate);
	if (requ_info.TxDate[0] == '\0')
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.TxDate",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	memset(buf, 0x0, sizeof(buf));
	memset(sTime, 0x00, sizeof(sTime));
	loc_get_zd_data(vars, ".TradeRecord.Request.EndTime", buf);
	if (strlen(buf) == 0)
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.EndTime",__FILE__,__LINE__);
	}
	time_adjus(buf,sTime);
	pub_log_debug("[%s][%d] buf=%s, sTime=%s", __FILE__, __LINE__, buf, sTime);
	tracno = atoll(requ_info.PlatFlow);
	memset(trc_no, 0x0, sizeof(trc_no));
	sprintf(trc_no,"%012lld",tracno);

	memset(send_path, 0x0, sizeof(send_path));
	sprintf(send_path,"%s/dat/%s_%s.log", getenv("SWWORK"), requ_info.TxDate, trc_no);
	pub_log_info("[%s][%d] send_path=[%s]",__FILE__,__LINE__, send_path);
	result = access(send_path,F_OK);
	if ( result )
	{
		flag = judge_workmode();
		if ( flag < 0 )
		{
			pub_log_info("[%s][%d] wordmode not sure",__FILE__,__LINE__ );
			strcpy(res_msg, "判断日志模式失败");
			strcpy(reply, "E015");
			goto ErrExit;
		}
		if ( flag == 1 )
		{
			pub_log_info("[%s][%d] log search begin", __FILE__, __LINE__);
			result = log_search_ext(requ_info.TxDate, atoll(trc_no));
			if ( result != 0 )
			{
				if (xml != NULL)
				{
					pub_xml_deltree(xml);
					xml = NULL;
				}
				pub_log_error("[%s][%d] log_search error",__FILE__,__LINE__);
				strcpy(reply, "E023");
				goto ErrExit;
			}
			if (xml != NULL)
			{
				pub_xml_deltree(xml);
				xml = NULL;
			}

			pub_log_info("[%s][%d] log search success", __FILE__, __LINE__);
		}
		else
		{
			pub_log_info("[%s][%d] workmode [MP]", __FILE__, __LINE__);
			if (type[0] != '1')
			{
				result = log_search(requ_info.Lsn, requ_info.TxDate, sTime, trc_no);
				if ( result != 0 )
				{
					pub_log_error("[%s][%d] log_search error",__FILE__,__LINE__);
					strcpy(reply, "E023");
					goto ErrExit;
				}
			}
			else
			{
				result = log_search(requ_info.Lsn, requ_info.TxDate, sTime, trc_no);
				if (result != 0)
				{
					result = get_log_mem(requ_info.PlatFlow, requ_info.TxDate, requ_info.Lsn);
					if (result != 0)
					{
						pub_log_error("[%s][%d] log_search error",__FILE__,__LINE__);
						strcpy(reply, "E010");
						goto ErrExit;
					}
				}
			}
		}
	}
	fp = NULL;
	fp = fopen(send_path, "a+");
	ch = fgetc(fp);
	if (ch == EOF)
	{
		fclose(fp);
		strcpy(reply, "E009");
		goto ErrExit;
	}

	pub_log_info("[%s][%d] 发送文件!!", __FILE__, __LINE__);

	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
	loc_set_zd_data(vars, "$send_path", send_path);

	pub_log_info("[%s][%d] sp2026 sucess!!", __FILE__, __LINE__);
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

