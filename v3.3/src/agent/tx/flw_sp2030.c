/***************************************************
   文 件 名:  flw_sp2030.c                        
   功能描述:  应用层快照功能                      
   作    者:  guxiaoxin                          
   完成日期:  20160802                            
  *************************************************/
#include "agent_comm.h"

typedef struct request
{
	time_t	start;
	time_t	end;
	sw_char_t	product[64];
	sw_char_t	channel[64];
	sw_char_t	txcode[64];
}request_t;

static int snapshot(sw_loc_vars_t *vars, request_t *request)
{
	int 	i    = 0;
	int 	flag = 0;
	int 	cnt  = 0;
	int 	file_num = 0;
	time_t 	cur  = 0;
	char 	date[32];
	char 	path[256];
	char	buf[1024];
	char	ptid[32];
	char	filename[256];
	char	*ptmp = NULL;
	struct dirent **namelist = NULL;
	unit_t *pproduct = NULL, *pchannel = NULL, *ptxcode = NULL, *presflag = NULL, *pretcode = NULL , *preturn = NULL; 
	unit_t *punit = NULL;
	sw_trace_item_t trace_item;
	FILE *fp = NULL;	

	pproduct = get_prdt_info();
	if (pproduct == NULL)
	{
		pub_log_error("[%s][%d] get_prdt_info error", __FILE__, __LINE__);
	}
	pchannel = get_chnl_info();
	if (pproduct == NULL)
	{
		pub_log_error("[%s][%d] get_chnl_info error", __FILE__, __LINE__);
	}
	ptxcode = get_txcode_info(pproduct);
	if (ptxcode == NULL)
	{
		pub_log_error("[%s][%d] get_txcode_info error", __FILE__, __LINE__);
	}
	presflag = get_resflag_info();
	if (presflag == NULL)
	{
		pub_log_error("[%s][%d] get_resflag_info error", __FILE__, __LINE__);
	}
	pretcode = get_retcode_info(pproduct);
	if (pretcode == NULL)
	{
		pub_log_error("[%s][%d] get_retcode_info error", __FILE__, __LINE__);
	}
	preturn = get_return_info();
	if (preturn == NULL)
	{
		pub_log_error("[%s][%d] get_return_info error", __FILE__, __LINE__);
	}
	
	cnt = 0;
	cur = request->start;
	while(cur < request->end)
	{	
		memset(date, 0x0, sizeof(date));
		agt_date_format(cur, date, "A");
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, "%s/tmp/monitor/%s", getenv("SWWORK"), date);

		if (0 != access(path, F_OK))
		{
			pub_log_info ("[%s][%d] not exsist the file [%s] [%s] [%ld]\n", __FILE__, __LINE__, path, date,	cur);
			cur += 60 * 60 * 24;
			continue;
		}
		pub_log_info("[%s][%d]path[%s]", __FILE__, __LINE__, path);

		i = 0;
		file_num = 0;
		namelist = agt_scan_dir(&file_num, path);;
		if (namelist == NULL)
		{
			pub_log_error("[%s][%d] scan dir error", __FILE__, __LINE__);
			continue;
		}
		flag = 0;
		while (i < file_num || flag == 0)
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
				flag = 1;
			}

			if (access(filename, F_OK) != 0)
			{
				continue;
			}
			
			fp = fopen(filename, "r");
			if (fp == NULL)
			{
				pub_log_error("[%s][%d] open file[%s] is faild", __FILE__, __LINE__, filename);
				free_unit(pproduct);
				free_unit(pchannel);
				free_unit(ptxcode);
				free_unit(presflag);
				free_unit(pretcode);
				free_unit(preturn);
				agt_free_namelist(namelist, file_num);
				return -1;
			}
			
			pub_log_info("[%s][%d] filename[%s]", __FILE__, __LINE__, filename);
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

				if (trace_item.start_time > (sw_int64_t)request->end * 1000 * 1000)
				{
					break;
				}

				if ( (trace_item.start_time > (sw_int64_t)request->start * 1000 * 1000) && 
					(strlen(request->product) == 0 || strcmp(request->product, trace_item.prdt_name) == 0) &&
					(strlen(request->channel) == 0 || strcmp(request->channel, trace_item.chnl) == 0) &&
					(strlen(request->txcode) == 0 || strcmp(request->txcode, trace_item.tx_code) == 0) )
				{
					cnt++;

					pproduct = update_unit(pproduct, trace_item.prdt_name);
					pchannel = update_unit(pchannel, trace_item.chnl);
					ptxcode = update_unit(ptxcode, trace_item.tx_code);
					presflag = update_unit(presflag, trace_item.resflag);
					pretcode = update_unit(pretcode, trace_item.tx_respcd);
					if (agt_check_stat(trace_item.tx_respcd) == 0)
					{
						preturn = update_unit(preturn, "1");
					}
					else
					{
						preturn = update_unit(preturn, "0");
					}
				}

			}
			 fclose(fp);
		}
		agt_free_namelist(namelist, file_num);
		cur += 60 * 60 * 24;
	}
	
	pub_log_info("[%s][%d]cnt = %d", __FILE__, __LINE__, cnt);
	if (cnt == 0)
	{
		cnt = 1;
	}
	for (punit = pproduct, i = 0; punit != NULL; punit = punit->next)
	{
		if (punit->cnt == 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", punit->cnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).CNT", i);
		loc_set_zd_data(vars, path, buf);

		memset(buf, 0x0, sizeof(buf));
		punit->rate = punit->cnt *100.0 / cnt;
		sprintf(buf, "%.2f", punit->rate);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Product(%d).RATE", i);
		loc_set_zd_data(vars, path, buf);
	
		i++;
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", i);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Products.Cnt");
	loc_set_zd_data(vars, path, buf);

	for (punit = pchannel, i = 0; punit != NULL; punit = punit->next)
	{
		if (punit->cnt == 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", punit->cnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).CNT", i);
		loc_set_zd_data(vars, path, buf);

		memset(buf, 0x0, sizeof(buf));
		punit->rate = punit->cnt *100.0 / cnt;
		sprintf(buf, "%.2f", punit->rate);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Channel(%d).RATE", i);
		loc_set_zd_data(vars, path, buf);
	
		i++;
	}
	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", i);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Channels.Cnt");
	loc_set_zd_data(vars, path, buf);

	for (punit = ptxcode, i = 0; punit != NULL; punit = punit->next)
	{
		if (punit->cnt == 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Txcodes.Txcode(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Txcodes.Txcode(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", punit->cnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Txcodes.Txcode(%d).CNT", i);
		loc_set_zd_data(vars, path, buf);

		memset(buf, 0x0, sizeof(buf));
		punit->rate = punit->cnt *100.0 / cnt;
		sprintf(buf, "%.2f", punit->rate);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Txcodes.Txcode(%d).RATE", i);
		loc_set_zd_data(vars, path, buf);
	
		i++;
	}
	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", i);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Txcodes.Cnt");
	loc_set_zd_data(vars, path, buf);

	for (punit = presflag, i = 0; punit != NULL; punit = punit->next)
	{
		if (punit->cnt == 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Resflags.Resflag(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Resflags.Resflag(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", punit->cnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Resflags.Resflag(%d).CNT", i);
		loc_set_zd_data(vars, path, buf);

		memset(buf, 0x0, sizeof(buf));
		punit->rate = punit->cnt *100.0 / cnt;
		sprintf(buf, "%.2f", punit->rate);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Resflags.Resflag(%d).RATE", i);
		loc_set_zd_data(vars, path, buf);
	
		i++;
	}
	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", i);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Resflags.Cnt");
	loc_set_zd_data(vars, path, buf);

	for (punit = pretcode, i = 0; punit != NULL; punit = punit->next)
	{
		if (punit->cnt == 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Retcodes.Retcode(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Retcodes.Retcode(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", punit->cnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Retcodes.Retcode(%d).CNT", i);
		loc_set_zd_data(vars, path, buf);

		memset(buf, 0x0, sizeof(buf));
		punit->rate = punit->cnt *100.0 / cnt;
		sprintf(buf, "%.2f", punit->rate);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Retcodes.Retcode(%d).RATE", i);
		loc_set_zd_data(vars, path, buf);
	
		i++;
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", i);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Retcodes.Cnt");
	loc_set_zd_data(vars, path, buf);

	for (punit = preturn, i = 0; punit != NULL; punit = punit->next)
	{
		if (punit->cnt == 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Returns.Return(%d).NAME", i);
		loc_set_zd_data(vars, path, punit->name);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Returns.Return(%d).CNNAME", i);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", punit->cnt);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Returns.Return(%d).CNT", i);
		loc_set_zd_data(vars, path, buf);

		memset(buf, 0x0, sizeof(buf));
		punit->rate = punit->cnt *100.0 / cnt;
		sprintf(buf, "%.2f", punit->rate);
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Returns.Return(%d).RATE", i);
		loc_set_zd_data(vars, path, buf);
	
		i++;
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", i);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Returns.Cnt");
	loc_set_zd_data(vars, path, buf);

	free_unit(pproduct);
	free_unit(pchannel);
	free_unit(ptxcode);
	free_unit(presflag);
	free_unit(pretcode);
	free_unit(preturn);
		
	return 0;
}

int sp2030(sw_loc_vars_t *vars)
{
	int 	ret = -1;
	int 	tmp = 0;
	char 	range[64];
	char	reply[8];
	char	res_msg[128];
	char 	date_s[128];
	char 	date_e[128];
	request_t request;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));
	memset(&request, 0x0, sizeof(request));

	memset(range, 0x00, sizeof(range));
	loc_get_zd_data(vars, ".TradeRecord.Request.Range", range);

	if (strlen(range) == 0)
	{
		memset(date_s, 0x00, sizeof(date_s));
		loc_get_zd_data(vars, ".TradeRecord.Request.DateStart", date_s);
		memset(date_e, 0x00, sizeof(date_e));
		loc_get_zd_data(vars, ".TradeRecord.Request.DateEnd", date_e);
		if(strlen(date_s) == 0 && strlen(date_e) == 0)
		{
			pub_log_info("[%s][%d] input is error", __FILE__, __LINE__);
			strcpy(reply, "E012"); 
			goto ErrExit;
		}
		request.start = agt_date_to_time(date_s);
		request.end = agt_date_to_time(date_e) + 60 * 60 * 24;
	}
	else
	{
		tmp = atoi(range);
		request.end = time(NULL);
		request.start = request.end - tmp * 60 * 60;	
	}	

	pub_log_info("[%s][%d]start[%d]end[%d]", __FILE__, __LINE__, request.start, request.end);

	loc_get_zd_data(vars, ".TradeRecord.Request.Product", request.product);

	loc_get_zd_data(vars, ".TradeRecord.Request.Channel", request.channel);

	loc_get_zd_data(vars, ".TradeRecord.Request.Txcode", request.txcode);

	ret = snapshot(vars, &request);
	if (ret < 0)
	{
		strcpy(reply, "E999");
		strcpy(res_msg, "查询应用层快照失败");
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

