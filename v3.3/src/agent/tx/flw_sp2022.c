/*************************************************
 文 件 名:  flw_sp2022.c                         **
  功能描述:  交易监控                            **
  作    者:  赵强                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

static char *errcode_filter(char *errcode)
{
    char filename[256];
    char buf[32];
    static char codes[1024][10];
    FILE *fp = NULL;
    int i = 0;
    static int count = 0;

    if (count == 0)
    {   
        memset(filename, 0x0, sizeof(filename));
        snprintf(filename, sizeof(filename) - 1, "%s/cfg/agentcfg/filter.txt", getenv("SWWORK"));

        if (access(filename, F_OK) != 0)
        {   
            return errcode;
        }   
        fp = fopen(filename, "r");
        if (fp == NULL)
        {   
            pub_log_error("[%s][%d] open file %s error", __FILE__, __LINE__, filename);
            return errcode;
        }   

        memset(codes, 0x0, sizeof(codes));
        count = 0;
        while(!feof(fp))
        {   
            memset(buf, 0x0, sizeof(buf));
            fgets(buf, sizeof(buf) -1, fp); 
            if ( buf[0] == '#' || strlen(buf) < 2)
            {
                continue;
            }

            strncpy(codes[count], buf, 9);
            count++;
            if (count >= 1024)
            {
                pub_log_error("[%s][%d] too many errcode, more than 1024", __FILE__, __LINE__);
                break;
            }
        }
        fclose(fp);
    }

    for (i = 0; i < count; ++i)
    {
        if (strncmp(codes[i], errcode, strlen(errcode)) == 0)
        {
            return "0000";
        }
    }

    return errcode;
}

static int set_trade_info(sw_loc_vars_t *vars, sw_trace_item_t *pTrace_info, int i, unit_t **pp, char *ptid)
{
	int 		j = 0;
	float		cost   = 0;
	char		buf[128];
	char		path[128];
	char		cnname[128];
	char		value[255];

	unit_t *punit = NULL;

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Sts", i);
	if (agt_check_stat(errcode_filter(pTrace_info->tx_respcd)) == 0)
	{
		loc_set_zd_data(vars, path, "1");
	}
	else
	{
		loc_set_zd_data(vars, path, "0");
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).TxCode", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_code);

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(cnname, sizeof(cnname));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CNTxCode", i);
	loc_set_zd_data(vars, path, cnname);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).PlatFlow", i);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%lld", pTrace_info->trace_no);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Svr", i);
	loc_set_zd_data(vars, path, pTrace_info->server);

	
	for (punit = pp[1], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, pTrace_info->server) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CNSvr", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}
	
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Svc", i);
	loc_set_zd_data(vars, path, pTrace_info->service);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Prdt", i);
	loc_set_zd_data(vars, path, pTrace_info->prdt_name);
	for (punit = pp[0], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, pTrace_info->prdt_name) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CNPrdt", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).TxDate", i);
	loc_set_zd_data(vars, path, pTrace_info->start_date);

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(value, sizeof(value));
	pub_change_time2(pTrace_info->start_time, value, 0);
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).TxBeginTime", i);
	loc_set_zd_data(vars, path, value);

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(value, sizeof(value));
	pub_change_time2(pTrace_info->end_time, value, 0);
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).TxEndTime", i);
	loc_set_zd_data(vars, path, value);

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(value, sizeof(value));
	cost = (pTrace_info->end_time - pTrace_info->start_time) * 1.0 / 1000 ;
	sprintf(value, "%.3fms", cost);
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).TxTime", i);
	loc_set_zd_data(vars, path, value);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).RetCode", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_respcd);	
	
	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).TxAmount", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_amt);

	pub_mem_memzero(path, sizeof(path));
	agt_hide_account(pTrace_info->dr_ac_no);
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).DeAccount", i);
	loc_set_zd_data(vars, path, pTrace_info->dr_ac_no); 

	pub_mem_memzero(path, sizeof(path));
	agt_hide_account(pTrace_info->cr_ac_no);
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CrAccount", i);
	loc_set_zd_data(vars, path, pTrace_info->cr_ac_no);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).DeAccName", i);
	loc_set_zd_data(vars, path, pTrace_info->dr_ac_name); 

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CrAccName", i);
	loc_set_zd_data(vars, path, pTrace_info->cr_ac_name);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Poundage", i);
	loc_set_zd_data(vars, path, pTrace_info->fee_amt);

	pub_mem_memzero(path, sizeof(path));
	agt_hide_account(pTrace_info->fee_ac_no);
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).ChargeAccount", i);
	loc_set_zd_data(vars, path, pTrace_info->fee_ac_no);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).NowTurnSign", i);
	loc_set_zd_data(vars, path, pTrace_info->ct_ind);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).BorrowMark", i);
	loc_set_zd_data(vars, path, pTrace_info->dc_ind);

	pub_mem_memzero(cnname, sizeof(cnname));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Lsn", i);
	loc_set_zd_data(vars, path, pTrace_info->chnl);
	for (punit = pp[2], j = 0; punit != NULL; punit = punit->next, j++)
	{
		if (strcmp(punit->name, pTrace_info->chnl) == 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).CNLsn", i);
			loc_set_zd_data(vars, path, punit->cnname);
			break;
		}
	}	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).ResMsg", i);
	loc_set_zd_data(vars, path, pTrace_info->tx_errmsg);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).BusTrcNo", i);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%lld", pTrace_info->busi_no);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysBusinesss.SysBusiness(%d).Ptid", i);
	loc_set_zd_data(vars, path, ptid);
	
	return 0;
}

sw_int32_t sp2022(sw_loc_vars_t *vars)
{
	int		ret = 0;
	int		page_idx = 0;
	int		page_cnt = 0;
	char	res_msg[256];
	char	reply[8];
	char	value[32];
	struct search   searchs;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", value);
	if (strlen(value) == 0)
	{
		pub_log_error("[%s][%d] No .TradeRecord.Request.PageIndex",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;		
	}

	page_idx = atoi(value);
	if (page_idx <= 0)
	{
		pub_log_error("[%s][%d] page_idx <= 0",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", value);
	if (strlen(value) == 0)
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d] No .TradeRecord.Request.PageCount",__FILE__,__LINE__);
		goto ErrExit;	
	}

	page_cnt = atoi(value);
	if (page_cnt <= 0)
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d] page_cnt <= 0",__FILE__,__LINE__);
		goto ErrExit;		
	}

	memset(&searchs, 0x00, sizeof(searchs));

	loc_get_zd_data(vars, ".TradeRecord.Request.TxCode", searchs.TxCode);
	pub_log_info("[%s][%d] TradeRecord.Request.TxCode[%s]", __FILE__, __LINE__, searchs.TxCode);

	loc_get_zd_data(vars, ".TradeRecord.Request.Lsn", searchs.Lsn);
	pub_log_info("[%s][%d] .TradeRecord.Request.Lsn[%s]", __FILE__, __LINE__, searchs.Lsn);

	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", searchs.Prdt);
	pub_log_info("[%s][%d] searchs.Prdt[%s]", __FILE__, __LINE__, searchs.Prdt);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(vars, ".TradeRecord.Request.Svr", value);
	pub_log_info("[%s][%d] searchs.Svr[%s]", __FILE__, __LINE__, value);
	if (strncmp(value, "svc", 3) == 0)
	{
		strncpy(searchs.Svc, value, sizeof(searchs.Svc)-1);
	}
	else
	{
		strncpy(searchs.Svr, value, sizeof(searchs.Svr)-1);
	}

	loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", searchs.PlatFlow);
	pub_log_info("[%s][%d] searchs.PlatFlow[%s]", __FILE__, __LINE__, searchs.PlatFlow);

	loc_get_zd_data(vars, ".TradeRecord.Request.TxAmount", searchs.TxAmount);
	pub_log_info("[%s][%d] searchs.TxAmount[%s]", __FILE__, __LINE__, searchs.TxAmount);

	loc_get_zd_data(vars, ".TradeRecord.Request.RetCode", searchs.RetCode);
	pub_log_info("[%s][%d] searchs.RetCode[%s]", __FILE__, __LINE__, searchs.RetCode);

	loc_get_zd_data(vars, ".TradeRecord.Request.RetCodeNo", searchs.RetCodeNo);
	pub_log_info("[%s][%d] searchs.RetCodeNo[%s]", __FILE__, __LINE__, searchs.RetCodeNo);

	loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", searchs.TxDate);
	pub_log_info("[%s][%d] searchs.TxDate[%s]", __FILE__, __LINE__, searchs.TxDate);

	loc_get_zd_data(vars, ".TradeRecord.Request.StartTime", searchs.StartTime);
	pub_log_info("[%s][%d] searchs.StartTime[%s]", __FILE__, __LINE__, searchs.StartTime);

	loc_get_zd_data(vars, ".TradeRecord.Request.EndTime", searchs.EndTime);
	pub_log_info("[%s][%d] searchs.EndTime[%s]", __FILE__, __LINE__, searchs.EndTime);

	loc_get_zd_data(vars, ".TradeRecord.Request.Option", searchs.flag);
	pub_log_info("[%s][%d] searchs.flag[%s]", __FILE__, __LINE__, searchs.flag);

	ret = agt_get_trc(vars, page_idx, page_cnt, &searchs, 1, set_trade_info);
	if(ret  < 0)
	{
		strcpy(reply, "E023");
		pub_log_error("[%s][%d] 未找到数据", __FILE__, __LINE__);
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
