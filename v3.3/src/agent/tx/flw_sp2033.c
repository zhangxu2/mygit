/*************************************************
  文 件 名: flw_sp2033.c                        
  功    能:	获取业务流程中的各个步骤节点
  作    者: guxiaoxin                            
  完成日期: 20160802                                    
 *************************************************/
#include "agent_comm.h"
#include "agent_search.h"

int get_all_step(sw_loc_vars_t *vars, char *file)
{
	int 	i = 0;
	char 	buf[128];
	char 	buffer[512] ;
	char 	*tmp = NULL;

	FILE *fp = NULL;
	fp = fopen(file, "r");
	if(fp == NULL)
	{
		pub_log_error("[%s][%d] fopen[%s] error, [%d][%s]", __FILE__, __LINE__, file, errno, strerror(errno));
		return -1;
	}

	i = 0;
	while(!feof(fp))
	{
		memset(buffer, 0x00, sizeof(buffer));
		fgets(buffer, sizeof(buffer), fp);
		buffer[strlen(buffer) - 1] = '\0';
		if ((tmp = strstr(buffer, "步骤")) != NULL || (strstr(buffer, "原子交易异常结束") != NULL))
		{
			if((tmp = strstr(buffer, "步骤")) != NULL)
			{
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.Steps.Step(%d).Value", i);
				loc_set_zd_data(vars, buf, tmp+strlen("步骤"));
				i++;
			}
			if(strstr(buffer, "异常") != NULL)
			{

				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.Steps.Step(%d).Value", i);
				loc_set_zd_data(vars, buf, "R");
				i++;
			}
		}
	}
	fclose(fp);
	return 0;

}

int sp2033(sw_loc_vars_t *vars)
{

	long	tracno = 0;
	char	lsn[128];
	char	buf[256];
	char	date[16];
	char	time[256];
	char    reply[16];
	char	trc_no[16];
	char    res_msg[256];
	char	file_name[256];
	int		result = SW_ERROR;
	struct 	search req_info;

	memset(reply,     0x0, sizeof(reply));
	memset(res_msg,   0x0, sizeof(res_msg));
	memset(&req_info, 0x0, sizeof(req_info));

	/*get Params*/
	memset(lsn, 0x00, sizeof(lsn));
	loc_get_zd_data(vars, ".TradeRecord.Request.Lsn", lsn);
	if (strlen(lsn) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.lsn can not be null",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	memset(trc_no, 0x00, sizeof(trc_no));
	loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", trc_no);
	if (strlen(trc_no) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.Trcno can not be null", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	

	memset(date, 0x00, sizeof(date));
	loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", date);
	if (strlen(date) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.TxDate can not be null", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	memset(buf, 0x0, sizeof(buf));
	memset(time, 0x00, sizeof(time));
	loc_get_zd_data(vars, ".TradeRecord.Request.TxEndTime", buf);
	if (strlen(buf) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.TxEndTime can not be null", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	time_adjus(buf, time);

	tracno = atol(trc_no);
	memset(trc_no, 0x0, sizeof(trc_no));
	sprintf(trc_no,"%012ld",tracno);
	pub_log_info("[%s][%d] lsn=[%s] date=[%s] time=[%s] trc_no=[%s]",__FILE__,__LINE__,lsn, date,time,trc_no);
	
	memset(file_name, 0x0, sizeof(file_name));
	result = judge_workmode();
	if (result == 2)
	{
		sprintf(file_name,"%s/dat/%s_%s.log", getenv("SWWORK"), date, trc_no);
		result = access(file_name, F_OK);
		if (result)
		{
			result = log_search(lsn, date, time, trc_no);
			if ( result == SW_ERROR )
			{
				pub_log_error("[%s][%d] log_search error",__FILE__,__LINE__);
				strcpy(reply, "E021");
				goto ErrExit;
			}
		}
	}
	else if (result == 1)
	{
		sprintf(file_name,"%s/dat/tra%s_%s.log", getenv("SWWORK"), date, trc_no);
		result = access(file_name, F_OK);
		if (result)
		{
			loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", req_info.Prdt);
			if (req_info.Prdt[0] == '\0')
			{
				pub_log_error("[%s][%d] .TradeRecord.Request.Prdt can not be null",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}
			loc_get_zd_data(vars, ".TradeRecord.Request.Svr", req_info.Svr);
			if ( req_info.Svr[0] == '\0')
			{
				pub_log_error("[%s][%d] .TradeRecord.Request.Svr can not be null",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}

			loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", req_info.TxDate);
			if (req_info.TxDate[0] == '\0')
			{
				pub_log_error("[%s][%d] .TradeRecord.Request.TxDate can not be null",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}
			loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", req_info.PlatFlow);
			if (req_info.PlatFlow[0] == '\0')
			{
				pub_log_error("[%s][%d] .TradeRecord.Request.PlatFlow can not be null",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}

			loc_get_zd_data(vars, ".TradeRecord.Request.Svc", req_info.Svc);
			if (req_info.Svc[0] == '\0')
			{
				pub_log_error("[%s][%d] .TradeRecord.Request.Svc can not be null",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}
			
			result = agt_get_trace_log(vars, &req_info, 0, NULL);
			if(result < 0)
			{
				pub_log_error("[%s][%d] get log error", __FILE__, __LINE__);
				strcpy(reply, "E999");
				goto ErrExit;
			}

		}
	}

	result = get_all_step(vars, file_name);
	if(result < 0)
	{
		pub_log_error("[%s][%d] get trace steps error", __FILE__, __LINE__);
		strcpy(reply, "E050");
		goto ErrExit;
	}
OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	
	return SW_OK;

ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	
	return SW_ERROR;

}
