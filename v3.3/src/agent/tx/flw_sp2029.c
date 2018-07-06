/*************************************************
  文 件 名:  flw_sp2642.c                        **
  功能描述:  应用层视图功能                      **
  作    者:  赵强                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

long get_timesize(char *range)
{
	pub_log_info("[%s][%d] %s %ld", __FILE__, __LINE__, range, atol(range)*2);
	return atol(range)*2;
}

static int server_search(sw_loc_vars_t *vars, sw_trace_item_t* Trade_info, long time_size, char *type)
{
	int 	i = 0;
	int     n = 0;
	int 	flag = 0;
	int 	number = 0;
	int 	file_num = 0;
	long    start, end;
	float 	fTmp = 0;
	long 	long d_time = 0;
	time_t	cur = 0; 
	char 	sTotal[1024];
	char 	sAccaury[1024];
	char 	sD_time[1024];
	char 	sReaction[1024];
	char 	sBuf[1024];
	char    sXTime[1024];
	char	*pTmp = NULL;
	char	sDate[32];
	char	sPath[256];
	char    sTime[32];
	char    time_type[2];
	char	sFilename[256];
	char	ptid[32];
	FILE 	*fp = NULL;
	struct 	total totals[256];
	sw_trace_item_t	stTrace_info;
	struct dirent **namelist = NULL;	

	end = Trade_info->end_time;
	start = Trade_info->start_time;
	memset(sPath, 0x0, sizeof(sPath));
	sprintf(sPath, ".TradeRecord.Response.Total.End");
	memset(sBuf, 0x0, sizeof(sBuf));
	agt_date_format(end - time_size/2, sBuf, "D");
	loc_set_zd_data(vars, sPath, sBuf);


	memset(sPath, 0x0, sizeof(sPath));
	sprintf(sPath, ".TradeRecord.Response.Total.Start");
	memset(sBuf, 0x0, sizeof(sBuf));
	agt_date_format(start, sBuf, "D");
	loc_set_zd_data(vars, sPath, sBuf);

	cur = start;
	memset(totals, 0x0, sizeof(totals));
	n = 0;
	while(cur < end)
	{
		memset(sDate, 0x0, sizeof(sDate));
		agt_date_format(cur, sDate, "A");

		memset(sPath, 0x0, sizeof(sPath));
		snprintf(sPath, sizeof(sPath)-1, "%s/tmp/monitor/%s", getenv("SWWORK"), sDate);

		if (0 != access(sPath, F_OK))
		{
			pub_log_info ("[%s][%d] not exsist the file [%s]\n", __FILE__, __LINE__, sPath);
			n++;
			cur += time_size;
			continue;
		}

		i = file_num = 0;		
		flag = 1;
		namelist = agt_scan_dir(&file_num, sPath);	
		while (i < file_num || flag)
		{
			if (i < file_num)
			{	
				if(strncmp(namelist[i]->d_name, "monitor_", strlen("monitor_")) != 0)
				{
					i++;
					continue;
				}

				memset(sFilename, 0x00, sizeof(sFilename));
				snprintf(sFilename, sizeof(sFilename)-1, "%s/%s", sPath, namelist[i]->d_name);
				i++;
			}
			else
			{	
				memset(sFilename, 0x00, sizeof(sFilename));
				snprintf(sFilename, sizeof(sFilename)-1, "%s/monitor.log", sPath);				
				flag = 0;
			}

			if (access(sFilename, F_OK) != 0)
			{
				continue;
			}

			fp = fopen(sFilename, "r");
			if (fp == NULL)
			{
				pub_log_error("[%s][%d] open file[%s] is faild", __FILE__, __LINE__, sFilename);
				agt_free_namelist(namelist, file_num);

				return -1;
			}	

			while(!feof(fp))
			{
				memset(sBuf, 0x00, sizeof(sBuf));
				fgets(sBuf, sizeof(sBuf), fp);

				if ((pTmp = strstr(sBuf, "TOTAL:")) == NULL)
				{
					continue;
				}
				memset(&stTrace_info, 0x00, sizeof(stTrace_info));
				memset(ptid, 0x0, sizeof(ptid));
				agt_get_trace_info(pTmp, &stTrace_info, ptid);


				if (stTrace_info.end_time < (long long)start * 1000 * 1000)
				{
					continue;	
				}
				else if (stTrace_info.end_time > (long long)end * 1000 * 1000) 
				{
					break;	
				}

				d_time = stTrace_info.end_time - stTrace_info.start_time;
				while(cur < end && stTrace_info.end_time >= (long long)(cur + time_size) * 1000 * 1000)
				{
					cur += time_size;
					n++;	
				}
				if ((strcasecmp(type, "product") == 0 && strcmp(Trade_info->prdt_name,  stTrace_info.prdt_name) != 0) ||
					(strcasecmp(type, "channel") == 0 && strcmp(Trade_info->chnl, stTrace_info.chnl) != 0) ||
					(strcasecmp(type, "txcode") == 0 && strcmp(Trade_info->tx_code, stTrace_info.tx_code) != 0) ||
					(strcasecmp(type, "resflag") == 0 && strcmp(Trade_info->resflag, stTrace_info.resflag) != 0) ||
					(strcasecmp(type, "retcode") == 0 && strcmp(Trade_info->tx_respcd, stTrace_info.tx_respcd) != 0) ||
					(strcasecmp(type, "return") == 0 && Trade_info->flag !=  stTrace_info.flag))
				{
					continue;
				} 
					totals[n].d_time += d_time;
					totals[n].num++;
					if(stTrace_info.flag == 1)
					{
						totals[n].right++;
					}	

					if (strstr(stTrace_info.resflag, "1") != NULL)
					{
						totals[n].reaction++;
					}

			}

			fclose(fp);									
		}
		agt_free_namelist(namelist, file_num);
		while(cur < end)
		{
			memset(sBuf, 0x0, sizeof(sBuf));
			agt_date_format(cur, sBuf, "A");
			if (strcmp(sBuf, sDate) > 0)
			{
				break;
			}

			cur += time_size;
			n++;
		}
	}

	memset(sTotal, 0x00, sizeof(sTotal));
	memset(sAccaury, 0x00, sizeof(sAccaury));
	memset(sD_time, 0x00, sizeof(sD_time));
	memset(sReaction, 0x00, sizeof(sReaction));
	memset(sXTime, 0x00, sizeof(sXTime));
	for (i = 0; i < n; ++i)
	{
		memset(sBuf, 0x00, sizeof(sBuf));
		sprintf(sBuf, "%d|", totals[i].num);
		strcat(sTotal, sBuf);

		memset(sBuf, 0x00, sizeof(sBuf));
		if ( time_size != 1440*60)
		{
			strcpy(time_type, "B");
		}
		else
		{
			strcpy(time_type, "A");
		}
		memset(sTime, 0x0, sizeof(sTime));
		agt_date_format(start+ time_size/2+ time_size*i, sTime, time_type);

		sprintf(sBuf, "%s|", sTime);
		strcat(sXTime, sBuf);
	
		memset(sBuf, 0x00, sizeof(sBuf));
		fTmp = agt_get_avgright(totals[i].right, totals[i].num);
		sprintf(sBuf, "%0.2f|", fTmp);
		strcat(sAccaury, sBuf);

		memset(sBuf, 0x00, sizeof(sBuf));
		fTmp = agt_get_avgtime(totals[i].d_time, totals[i].num);
		sprintf(sBuf, "%.0f|", fTmp);
		strcat(sD_time, sBuf);

		memset(sBuf, 0x00, sizeof(sBuf));
		fTmp = agt_get_avgright(totals[i].reaction, totals[i].num);
		sprintf(sBuf, "%0.2f|", fTmp);
		strcat(sReaction, sBuf);

		number += totals[i].num;
	}


	memset(sPath, 0x00, sizeof(sPath));
	strcpy(sPath, ".TradeRecord.Response.Total.Sum");
	loc_set_zd_data(vars, sPath, sTotal);
	pub_log_info("[%s][%d]the sLine is [%s]", __FILE__, __LINE__, sTotal);

	memset(sPath, 0x00, sizeof(sPath));
	strcpy(sPath, ".TradeRecord.Response.Total.Precent");
	loc_set_zd_data(vars, sPath, sAccaury);
	pub_log_info("[%s][%d]the sLine is [%s]", __FILE__, __LINE__, sAccaury);

	memset(sPath, 0x00, sizeof(sPath));
	strcpy(sPath, ".TradeRecord.Response.Total.Time");
	loc_set_zd_data(vars, sPath, sD_time);
	pub_log_info("[%s][%d]the sLine is [%s]", __FILE__, __LINE__, sD_time);


	memset(sPath, 0x00, sizeof(sPath));
	strcpy(sPath, ".TradeRecord.Response.Total.XTime");
	loc_set_zd_data(vars, sPath,sXTime);
	pub_log_info("[%s][%d]the sLine is [%s]", __FILE__, __LINE__, sXTime);
	
	memset(sPath, 0x00, sizeof(sPath));
	strcpy(sPath, ".TradeRecord.Response.Total.Rate");
	loc_set_zd_data(vars, sPath, sReaction);
	pub_log_info("[%s][%d]the sLine is [%s]", __FILE__, __LINE__, sReaction);

	return 0;
}

int sp2029(sw_loc_vars_t *vars)
{
	float 	range = 0;
	long	size = 0;
	char 	sRange[64];
	char 	sTimeSize[64];
	char	reply[8];
	char	res_msg[128];
	char 	date_s[128];
	char 	date_e[128];
	char	type[32];
	char 	value[32];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));
	sw_trace_item_t stTrace_info;

	memset(sRange, 0x00, sizeof(sRange));
	loc_get_zd_data(vars, ".TradeRecord.Request.Range", sRange);

	memset(&stTrace_info, 0x00, sizeof(stTrace_info));
	memset(sTimeSize, 0x00, sizeof(sTimeSize));
	if (strlen(sRange) != 0)
	{
		size = get_timesize(sRange)*60;
	}
		
	if (strlen(sRange) == 0)
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
		stTrace_info.start_time = agt_date_to_time(date_s);
		stTrace_info.end_time = agt_date_to_time(date_e) + 60 * 60 * 24;
		size = 1440 *60;
	}
	else
	{
		range = atof(sRange);
		stTrace_info.end_time = time(NULL);
		stTrace_info.start_time = stTrace_info.end_time - range * 60 * 60;	
	}	

	pub_log_info("[%s][%d] size= [%ld]", __FILE__, __LINE__, size);
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Product", stTrace_info.prdt_name);
	pub_log_debug("[%s][%d] .TradeRecord.Request.Product = [%s]", __FILE__, __LINE__, stTrace_info.prdt_name);
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Lsn", stTrace_info.chnl);
	pub_log_debug("[%s][%d] .TradeRecord.Request.Lsn = [%s]", __FILE__, __LINE__, stTrace_info.chnl);
	
	loc_get_zd_data(vars, ".TradeRecord.Request.TxCode", stTrace_info.tx_code);
	pub_log_debug("[%s][%d] .TradeRecord.Request.TxCode = [%s]", __FILE__, __LINE__, stTrace_info.tx_code);
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Response", stTrace_info.resflag);
	pub_log_debug("[%s][%d] .TradeRecord.Request.Response = [%s]", __FILE__, __LINE__, stTrace_info.resflag);

	memset(type, 0x0, sizeof(type));
	loc_get_zd_data(vars, ".TradeRecord.Request.Type", type);

	memset(value, 0x0, sizeof(value));
	loc_get_zd_data(vars, ".TradeRecord.Request.Value", value);

	if (strcasecmp(type, "product") == 0)
	{
		strncpy(stTrace_info.prdt_name, value, sizeof(stTrace_info.prdt_name)-1);
	}
	if (strcasecmp(type, "channel") == 0)
	{
		strncpy(stTrace_info.chnl, value, sizeof(stTrace_info.chnl)-1);
	}
	if (strcasecmp(type, "txcode") == 0)
	{
		strncpy(stTrace_info.tx_code, value, sizeof(stTrace_info.tx_code)-1);
	}
	if (strcasecmp(type, "retcode") == 0)
	{
		strncpy(stTrace_info.tx_respcd, value, sizeof(stTrace_info.tx_respcd)-1);
	}
	if (strcasecmp(type, "resflag") == 0)
	{
		strncpy(stTrace_info.resflag, value, sizeof(stTrace_info.resflag)-1);
	}
	if (strcasecmp(type, "return") == 0)
	{
		stTrace_info.flag =  atoi(value);
	}


	if (server_search(vars, &stTrace_info, size, type) < 0)
	{
		strcpy(reply, "E999"); 
		strcpy(res_msg, "查询应用层视图失败");
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
