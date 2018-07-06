/*************************************************
  文 件 名:  flw_sp2011.c                        **
  功能描述:  mtype信息获取                       **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

static int get_mtype_info(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	fd = 0;
	char	filename[256];
	char	path[256];
	char	buf[256];
	char	item[10][256];
	FILE	*fp = NULL;

	memset(filename, 0x0, sizeof(filename));
	snprintf(filename, sizeof(filename)-1, "%s/tmp/agtmon/mtypeinfo.txt", getenv("SWWORK"));
	if (access(filename, F_OK))
	{
		return 0;
	}
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open mtype info file error %s", __FILE__, __LINE__, filename);
		return -1;
	}

	fd = fileno(fp);
	pub_lock_fd(fd);

	i = 0;

	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		if (strstr(buf, "TOTAL:") == NULL)
		{
			continue;
		}

		memset(item, 0x0, sizeof(item));
		agt_str_parse(buf, item, 10);
	
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Tras.Tra(%d).SesCnt", i);
		loc_set_zd_data(vars, path, item[0]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Tras.Tra(%d).CurCnt", i);
		loc_set_zd_data(vars, path, item[1]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Tras.Tra(%d).FreeCnt", i);
		loc_set_zd_data(vars, path, item[2]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Tras.Tra(%d).SucCnt", i);
		loc_set_zd_data(vars, path, item[3]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Tras.Tra(%d).FailCnt", i);
		loc_set_zd_data(vars, path, item[4]);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Tras.Tra(%d).Time", i);
		loc_set_zd_data(vars, path, item[5]);

		i++;
	}

	pub_unlock_fd(fd);
	fclose(fp);

	return 0;
}

static int set_mtype_value(sw_loc_vars_t *vars,sw_mtype_head_t *head,sw_mtype_node_t *item,int i,int j,int k)
{
	int      res=-1;
	long            creat_t = 0;
	char path[256];
	char buf[256];
	char prdt_cname[256];
	char lsn_cname[256];
	char time_buf[256];

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(buf, sizeof(buf));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).SessID", j);
	sprintf(buf, "%d",k+1);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(buf, sizeof(buf));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).PlatFlow", j);
	sprintf(buf, "%lld", item[i].trace_no);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).Prdt", j);
	loc_set_zd_data(vars, path,item[i].prdt_name);
	
	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(prdt_cname, sizeof(prdt_cname));
	res=agt_get_pdt_name(item[i].prdt_name,prdt_cname, sizeof(prdt_cname) - 1);
	if(res == -1)
	{
		pub_log_error("[%s][%d] 获取产品的CNAME失败!",__FILE__,__LINE__);
	}
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).CNPrdt", j);
	loc_set_zd_data(vars, path, prdt_cname);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).Lsn", j);
	loc_set_zd_data(vars, path, item[i].lsn_name);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).CNLsn", j);
	pub_mem_memzero(lsn_cname, sizeof(lsn_cname));
	res=agt_get_lsn_name(item[i].lsn_name,lsn_cname);
	if(res == -1)
	{
		pub_log_error("[%s][%d] 获取侦听的CNAME失败!",__FILE__,__LINE__);
	}
	loc_set_zd_data(vars, path, lsn_cname);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).Svr", j);
	loc_set_zd_data(vars, path, item[i].server);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).Svc", j);
	loc_set_zd_data(vars, path, item[i].service );

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).TxDate", j);
	loc_set_zd_data(vars, path, item[i].sys_date);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Sessions.Session(%d).Start", j);
	pub_mem_memzero(time_buf, sizeof(time_buf));
	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "%lld", item[i].start_time);
	strncpy(time_buf, buf, 10);
	creat_t = atol(time_buf);

	pub_mem_memzero(time_buf, sizeof(time_buf));
	agt_date_format(creat_t, time_buf, "B");

	pub_log_debug("%s, %d start_time[%s]", __FILE__, __LINE__, time_buf);
	loc_set_zd_data(vars, path, time_buf);

	return 0;
}

/*2011*/
int sp2011(sw_loc_vars_t *vars)
{
	int      ret = SW_ERROR;
	int      i, j = 0;
	int      lock_id;
	int      cnt = 0;
	int      pagenumuse=0;
	int      page_cnt = 0;
	int      page_index = 0;
	int      page_sum = 0;
	int      sessflg=0;
	int      page_tmp=0;
	int      k=0;
	char       sBuf[256];
	char       res_msg[256];
	char       reply[256];
	sw_mtype_t      *mtype_info = NULL;
	sw_mtype_head_t *head = NULL;
	sw_mtype_node_t *item = NULL;

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));

	pub_log_info("[%s][%d][%s]begin", __FILE__, __LINE__, __FUNCTION__);

	ret = get_mtype_info(vars);
	
	ret = run_link_ext();
	if (ret != SW_OK)
	{
		strcpy(reply, "E001");
		goto ErrExit;
	}

	mtype_info = run_get_mtype();
	if (mtype_info == NULL)
	{
		pub_log_error("[%s][%d]get mtype_info error"
			,__FILE__, __LINE__);
		strncpy(reply, "E040", sizeof(reply)-1);
		goto ErrExit;
	}

	head = &mtype_info->head;
	item = &mtype_info->first;
	lock_id = head->lock_id;
	sem_mutex_lock(lock_id);

	sessflg = MTYPE_USED;

	pub_mem_memzero(sBuf, sizeof(sBuf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", sBuf);
	page_cnt = atoi(sBuf);

	pub_mem_memzero(sBuf, sizeof(sBuf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", sBuf);
	page_index = atoi(sBuf);
	pub_log_debug("idxidx=[%d]",page_index);

	cnt = 0;
	for (i = 0; i < head->mtype_max; i++)
	{

		if (sessflg == item[i].flag && item[i].prdt_name != NULL && strlen(item[i].prdt_name) != 0)
		{
			cnt++;
		}
	}

	if (cnt % page_cnt == 0)
	{
		page_sum = cnt / page_cnt;
	}
	else
	{
		page_sum = cnt / page_cnt + 1;
	}
	pub_mem_memzero(sBuf, sizeof(sBuf));
	sprintf(sBuf, "%d", page_sum);
	loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", sBuf);

	cnt = 0;
	for (i = 0; i < head->mtype_max; i++)
	{

		if (strlen(item[i].prdt_name) !=0 &&  item[i].prdt_name != NULL)
		{
			cnt++;
		}
	}
	j = 0;
	page_tmp=page_index;
	k=(page_index - 1) * page_cnt;
	for (i = 0;  i < cnt; i++)
	{
		if(item[i].flag == MTYPE_USED )
		{
			if((pagenumuse >= (page_tmp - 1) * page_cnt) && (pagenumuse < page_tmp * page_cnt))
			{
				set_mtype_value(vars,head,item, i, j,k);
				j++;
				k++;
			}
			pagenumuse++;
		}

	}

	pub_mem_memzero(sBuf, sizeof(sBuf));
	sprintf(sBuf, "%d", pagenumuse);
	loc_set_zd_data(vars, ".TradeRecord.Response.Cnt", sBuf);
	sem_mutex_unlock(lock_id);

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");
	run_destroy();

	return SW_OK;

ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	run_destroy();
	return SW_ERROR;
}

