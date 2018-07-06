#include "agent_comm.h"
#include "agent_search.h"
/*************************************************
 *   �� �� ��:  flw_sp2025.c                      **
 *   ��������:  ��ȡ��������ͼ ����ĳ����         **
 *   ��    ��:  ��ǿ                              **
 *   �������:  20160801                          **
 **************************************************/

/* 2025 */
int get_one_step(sw_loc_vars_t *vars, char *file, char *step)
{
	int 	i = 0;
	int 	flag = 0;
	char	buffer[512];
	char 	sBuf[128];
	char 	*tmp = NULL;
	FILE 	*fp = NULL;

	fp = fopen(file,  "r");
	if(fp == NULL)
	{       
		pub_log_error("[%s][%d] open error", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	while(!feof(fp))
	{
		memset(buffer, 0x00, sizeof(buffer));
		fgets(buffer, sizeof(buffer), fp);
		buffer[strlen(buffer) - 1] = '\0';

		if (strstr(buffer, "����") != NULL )
		{
			if(flag == 1)
			{
				break;
			}
			if(strstr(buffer, step)!= NULL)
			{
				flag = 1;
			}
		}
		if(flag == 1 && ((tmp = strstr(buffer, "�ڵ�")) != NULL))
		{
			memset(sBuf, 0x00, sizeof(sBuf));
			sprintf(sBuf, ".TradeRecord.Response.NodeNames.NodeName(%d).Value", i);
			loc_set_zd_data(vars, sBuf, tmp);
			i++;
		}
		else if(flag == 1 && ((tmp = strstr(buffer, "ԭ�ӽ���")) != NULL))
		{
			if(strstr(buffer, "�쳣����") != NULL)
			{
				memset(sBuf, 0x00, sizeof(sBuf));
				sprintf(sBuf, ".TradeRecord.Response.NodeNames.NodeName(%d).Value", i);
				loc_set_zd_data(vars, sBuf, "R");
				i++;

			}
			else if( strstr(buffer, "��ʼ") != NULL)
			{
				memset(sBuf, 0x00, sizeof(sBuf));
				sprintf(sBuf, ".TradeRecord.Response.NodeNames.NodeName(%d).Value", i);
				loc_set_zd_data(vars, sBuf, "�ڵ�1:��ʼ");
				i++;
			}
			else if(strstr(buffer, "��������") != NULL)
			{
				memset(sBuf, 0x00, sizeof(sBuf));
				sprintf(sBuf, ".TradeRecord.Response.NodeNames.NodeName(%d).Value", i);
				loc_set_zd_data(vars, sBuf, "��������");
				i++;
			}
			else
			{
				memset(sBuf, 0x00, sizeof(sBuf));
				sprintf(sBuf, ".TradeRecord.Response.NodeNames.NodeName(%d).Value", i);
				loc_set_zd_data(vars, sBuf, tmp+strlen("ԭ�ӽ���:"));				
				i++;
			}
		}

	}

	fclose(fp);

	return 0;
}
int sp2025(sw_loc_vars_t *vars)
{

	int		tracno = 0;
	int		result = 0;
	char	sBuf[128];
	char    reply[16];
	char	sLsn[512];
	char	sTime[256];
	char	trc_no[256];
	char	sCmd[512];
	char	sLine[512];
	char	sTep[512];
	char	res_msg[512];
	char	file_name[512];
	char	sOver[512];
	struct search requ_info;

	memset(&requ_info, 0x0, sizeof(requ_info));
	memset(reply, 0x0, sizeof(reply));
	memset(sLsn,0x0, sizeof(sLsn));
	memset(sTime, 0x0,sizeof(sTime));
	memset(sCmd, 0x0,sizeof(sCmd));
	memset(sLine, 0x0,sizeof(sLine));
	memset(res_msg, 0x0,sizeof(res_msg));
	memset(file_name,0x0, sizeof(file_name));
	memset(sOver, 0x0,sizeof(sOver));
	memset(sLsn, 0x00, sizeof(sLsn));

	loc_get_zd_data(vars, ".TradeRecord.Request.Lsn", sLsn);
	if (strlen(sLsn) == 0)
	{
		strcpy(reply, "E012");
		pub_log_error("%s, %d, no .TradeRecord.Request.Lsn",__FILE__,__LINE__);
		goto ErrExit;
	}


	loc_get_zd_data(vars, ".TradeRecord.Request.PlatFlow", requ_info.PlatFlow);
	if (requ_info.PlatFlow[0] == '\0')
	{
		pub_log_error("[%s][%d] no Trno",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	loc_get_zd_data(vars, ".TradeRecord.Request.TxDate", requ_info.TxDate);
	if (requ_info.TxDate[0] == '\0')
	{
		pub_log_error("[%s][%d] no Date",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	memset(sBuf, 0x00, sizeof(sBuf));
	memset(sTime, 0x00, sizeof(sTime));
	loc_get_zd_data(vars, ".TradeRecord.Request.TxEndTime", sBuf);
	if (strlen(sBuf) == 0)
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.Time",__FILE__,__LINE__);
	}
	time_adjus(sBuf, sTime);

	memset(sTep, 0x00, sizeof(sTep));
	loc_get_zd_data(vars, ".TradeRecord.Request.StepName", sTep);
	if (strlen(sTep) == 0)
	{
		pub_log_error("%s, %d, no .TradeRecord.Request.sTep",__FILE__,__LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	pub_log_info("[%s][%d] sTep:: [%s]", __FILE__, __LINE__, sTep);
	
	tracno = atoi(requ_info.PlatFlow);
	memset(trc_no, 0x0, sizeof(trc_no));
	sprintf(trc_no,"%012d",tracno);
	
	memset(file_name, 0x0, sizeof(file_name));
	result = judge_workmode();
	if (result == 2)
	{
		sprintf(file_name,"%s/dat/%s_%s.log", getenv("SWWORK"), requ_info.TxDate, trc_no);
		result = access(file_name, F_OK);
		if (result)
		{
			result = log_search(sLsn, trc_no, sTime, requ_info.TxDate);
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
		sprintf(file_name,"%s/dat/tra%s_%s.log", getenv("SWWORK"), requ_info.TxDate, trc_no);
		result = access(file_name, F_OK);
		if (result)
		{
			loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", requ_info.Prdt);
			if (requ_info.Prdt[0] == '\0')
			{
				pub_log_error("[%s][%d]  no PrdtName",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}
			loc_get_zd_data(vars, ".TradeRecord.Request.Svr", requ_info.Svr);
			if ( requ_info.Svr[0] == '\0')
			{
				pub_log_error("[%s][%d] no PrdtName",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}

			loc_get_zd_data(vars, ".TradeRecord.Request.Svc", requ_info.Svc);
			if (requ_info.Svc[0] == '\0')
			{
				pub_log_error("[%s][%d] no Trno",__FILE__,__LINE__);
				strcpy(reply, "E012");
				goto ErrExit;
			}

			result = agt_get_trace_log(vars, &requ_info, 0, NULL);
			if(result < 0)
			{
				pub_log_info("[%s][%d] find trace log error", __FILE__, __LINE__);
				strcpy(reply, "E999");
				strcpy(res_msg, "��ȡ��������ʧ��");
				goto ErrExit;
			}
		}

	}
	else 
	{
		pub_log_info("[%s][%d] find trace log error", __FILE__, __LINE__);
		strcpy(reply, "E015");
		goto ErrExit;
	}

	pub_log_info("[%s][%d] trc_no ::[%s]", __FILE__, __LINE__, trc_no);	
	result = get_one_step(vars, file_name, sTep);
	if(result < 0)
	{

		pub_log_info("[%s][%d] find  %s error", __FILE__, __LINE__, sTep);
		strcpy(reply, "E999");
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
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "E999");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;

}
