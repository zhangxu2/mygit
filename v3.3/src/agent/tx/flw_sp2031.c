/********************************************************************
  文 件 名:  flw_sp2031.c                        
  功能描述:  多维度统计
  			 涉及到的功能页面如下：
             渠道监控页面：以渠道为维度统计交易量
			 多维统计页面：以渠道/产品/返回码/交易码为维度统计交易情况
  作    者:  guxiaoxin                         
  完成日期:  20160802                         
 ********************************************************************/
#include "agent_comm.h"

static struct point *set_data(sw_loc_vars_t *vars, struct point *head, int num, char *type)
{
	char	name[64];
	char 	xml[256];
	char 	buf[1024];
	int 	index = 0;
	int 	pageindex = 0;
	int 	pagecount = 0;
	int 	pagesum = 0;
	int 	min = 0;
	int 	max = 0;
	float 	accuary = 0;
	float 	response = 0;
	float 	avg_time = 0;
	struct 	point *point = NULL;


	memset(buf, 0x00, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
	pageindex = atoi(buf);
	memset(buf, 0x00, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
	pagecount = atoi(buf);

	if (pagecount == 0)
	{
		pagecount = 1;
	}


	min = (pageindex - 1) * pagecount;
	max = pageindex * pagecount - 1;

	for (point = head, index = 0; point != NULL; point = point->next, ++index)
	{
		accuary  = agt_get_avgright(point->right, point->num);
		response = agt_get_avgright(point->response, point->num);
		avg_time = agt_get_avgtime(point->d_time, point->num);

		if (num >= 0)
		{
			memset(name, 0x0, sizeof(name));
			agt_get_pdt_name(point->name, name, sizeof(name) - 1);
			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "%s | %d | %0.2f%% | %0.0f | %0.2f%% | %d| %d|", \
				name, point->num, accuary, avg_time, response, point->right, point->num - point->right);
			sprintf(xml, ".TradeRecord.Response.Dimensions.Dimension(%d).Pdts.Pdt(%d).Value", num, index);
			loc_set_zd_data(vars, xml, buf);
		}
		else
		{
			
			if (pagecount != 1 &&(index > max || index < min))
			{
				continue;
			}
			memset(xml, 0x00, sizeof(xml));
			sprintf(xml, ".TradeRecord.Response.Dimensions.Dimension(%d).Node", index - min);

			memset(name, 0x0, sizeof(name));
			if (strcmp(type, "C") == 0  || strcmp(type, "CP") == 0)
			{
				agt_get_lsn_name(point->name, name);
			}
			else if (strcmp(type, "P") == 0)
			{
				agt_get_pdt_name(point->name, name, sizeof(name) - 1);
			}
			else if (strcmp(type, "R") == 0)
			{
				agt_error_info(point->name, name);
			}
			else
			{
				strncpy(name, point->name, sizeof(name));
			}	
			pub_log_info("[%s][%d]name[%s]name[%s]", __FILE__, __LINE__, point->name, name);

			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "%s | %d | %0.2f%% | %0.0f | %0.2f%% | %d| %d|", \
				name, point->num, accuary, avg_time, response, point->right, point->num - point->right);

			loc_set_zd_data(vars, xml, buf);

			if (point->pdt != NULL)
			{
				point->pdt = set_data(vars, point->pdt, index - min, type);
			}
		}
	}

	pagesum = index / pagecount;
	if (index % pagecount > 0)
	{
		pagesum++;
	}
	if (num < 0)
	{
		memset(buf, 0x00, sizeof(buf));
		sprintf(buf, "%d", pagesum);
		loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", buf);

		memset(buf, 0x00, sizeof(buf));
		sprintf(buf, "%d", index);
		loc_set_zd_data(vars, ".TradeRecord.Response.Cnt", buf);

		memset(buf, 0x00, sizeof(buf));
		sprintf(buf, "%d", index - 1);
		loc_set_zd_data(vars, "#MAXALL", buf);
	}
	return head;
}

static int multivariate(sw_loc_vars_t *vars, char *type, time_t start, time_t end)
{
	int 	i        = 0;
	int 	flag     = 0;
	int 	right    = 0;
	int 	respons  = 0;	
	int 	file_num = 0;
	char	buf[1024];
	char	date[32];
	char	path[256];
	char	ptid[32];
	char	filename[256];
	char	*ptmp = NULL;
	FILE 	*fp   = NULL;
	time_t	cur   = 0; 
	struct 	dirent **namelist = NULL;	
	struct 	point   *phead    = NULL;
	long long d_time = 0;
	sw_trace_item_t	trace_info;

	cur = start;
	while(cur < end)
	{
		memset(date, 0x0, sizeof(date));
		agt_date_format(cur, date, "A");

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, "%s/tmp/monitor/%s", getenv("SWWORK"), date);

		if (0 != access(path, F_OK))
		{
			pub_log_info ("[%s][%d] dir [%s] not exist", __FILE__, __LINE__, path);
			cur = agt_date_to_time(date) + 60 * 60 * 24;
			continue;
		}

		i = file_num = 0;		
		flag = 1;
		namelist = agt_scan_dir(&file_num, path);
		pub_log_debug("[%s][%d] file_num=[%d]", __FILE__, __LINE__, file_num);
		while (i < file_num || flag)
		{
			if (i < file_num)
			{	
				if(strncmp(namelist[i]->d_name, "monitor_", strlen("monitor_")) != 0)
				{
					i++;
					continue;
				}

				memset(filename, 0x00, sizeof(filename));
				snprintf(filename, sizeof(filename)-1, "%s/%s", path, namelist[i]->d_name);
				i++;
			}
			else
			{	
				memset(filename, 0x00, sizeof(filename));
				snprintf(filename, sizeof(filename)-1, "%s/monitor.log", path);				
				flag = 0;
			}

			if (access(filename, F_OK) != 0)
			{
				continue;
			}

			pub_log_info("[%s][%d] filename %s", __FILE__, __LINE__, filename);	
			fp = fopen(filename, "r");
			if (fp == NULL)
			{
				pub_log_error("[%s][%d] open file[%s] is faild", __FILE__, __LINE__, filename);
				agt_free_namelist(namelist, file_num);
				agt_free_point(phead);
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
				memset(&trace_info, 0x00, sizeof(trace_info));
				memset(ptid, 0x0, sizeof(ptid));
				agt_get_trace_info(ptmp, &trace_info, ptid);

				if (trace_info.end_time < (long long)start * 1000 * 1000)
				{
					continue;	
				}
				else if (trace_info.start_time > (long long)end * 1000 * 1000) 
				{
					break;	
				}
				d_time = trace_info.end_time - trace_info.start_time;
				if (agt_check_stat(trace_info.tx_respcd) == 0)
				{
					right = 1;
				}
				else
				{
					right = 0;
				}

				if (strstr(trace_info.resflag, "1") != NULL)
				{
					respons = 1;
				}
				else
				{
					respons = 0;
				}

				if (strcmp(type, "T")== 0)
				{
					phead = agt_insert_point(phead, trace_info.tx_code, right, respons, d_time, NULL);
				}
				else if(strcmp(type, "P") == 0)
				{
					phead = agt_insert_point(phead, trace_info.prdt_name, right, respons, d_time, NULL);
				}
				else if(strcmp(type, "C") == 0)
				{
					phead = agt_insert_point(phead, trace_info.chnl, right, respons, d_time, NULL);
			
				}
				else if(strcmp(type, "R") == 0)
				{
					phead = agt_insert_point(phead, trace_info.tx_respcd, right, respons, d_time, NULL);
				}
				else if(strcmp(type, "CP") == 0)
				{
					phead = agt_insert_point(phead, trace_info.chnl, right, respons, d_time,  trace_info.prdt_name);
				}
			}

			fclose(fp);									
		}
		agt_free_namelist(namelist, file_num);
		cur = agt_date_to_time(date) + 60 * 60 * 24;
	}
	memset(buf, 0x00, sizeof(buf));
	phead = agt_rank_point(phead, agt_point_cmp_num);
	phead = set_data(vars, phead, -1, type);
	agt_free_point(phead);

	return 0;
}

int sp2031(sw_loc_vars_t *vars)
{
	long 	l_range = 0;
	time_t	start = 0;
	time_t	end = 0;
	char 	s_range[64];
	char 	opt[64];
	char	reply[8];
	char	res_msg[128];
	char 	date_s[128];
	char 	date_e[128];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));

	memset(s_range, 0x00, sizeof(s_range));
	loc_get_zd_data(vars, ".TradeRecord.Request.Range", s_range);

	if (strlen(s_range) == 0)
	{
		memset(date_s, 0x00, sizeof(date_s));
		loc_get_zd_data(vars, ".TradeRecord.Request.DateStart", date_s);
		memset(date_e, 0x00, sizeof(date_e));
		loc_get_zd_data(vars, ".TradeRecord.Request.DateEnd", date_e);
		if(strlen(date_s) == 0 && strlen(date_e) == 0)
		{
			pub_log_error("[%s][%d]start[%s]end[%s]error", __FILE__, __LINE__, date_s, date_e);
			strcpy(reply, "E012"); 
			goto ErrExit;
		}
		start = agt_date_to_time(date_s);
		end   = agt_date_to_time(date_e) + 60 * 60 * 24;
	}
	else
	{
		pub_log_debug("[%s][%d] s_range=[%s]", __FILE__, __LINE__, s_range);
		l_range = atol(s_range) * 60 * 60;
		end = time(NULL);
		start = end - l_range;	
	}

	memset(opt, 0x00, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	if (strlen(opt) == 0 || opt[0] == '\0')
	{
		pub_log_error("[%s][%d]the opt is null", __FILE__, __LINE__);
		strcpy(reply, "E012"); 
		goto ErrExit; 
	}
	pub_log_info("[%s][%d]the opt is [%s]", __FILE__, __LINE__, opt);

	if (multivariate(vars, opt, start, end) < 0)
	{
		pub_log_error("[%s][%d]deal error", __FILE__, __LINE__);
		strcpy(reply, "E999"); 
		strcpy(res_msg, "统计失败");
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
