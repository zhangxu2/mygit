/******************************************
  文 件 名:  flw_sp2042.c                                                      
  功能描述:  获取指定产品的错误日志
  作    者:  guxiaoxin
  完成日期: 20160608              
 ******************************************/
#include "agent_comm.h"
#include "pub_proc.h"

int sp2042(sw_loc_vars_t *vars)
{
	int 	ret;
	char 	prdt[64];
	char    path[256];
	char    proc[256];
	char 	rescode[16];
	char 	resmsg[1024];

	memset(rescode, 0x0, sizeof(rescode));
	memset(resmsg, 0x0, sizeof(resmsg));

	memset(proc, 0x0, sizeof(proc));
	memset(path, 0x0, sizeof(path));
	memset(prdt, 0x0, sizeof(prdt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", prdt);
	loc_get_zd_data(vars, ".TradeRecord.Request.Proc", proc);
	if (strlen(prdt) == 0 && strlen(proc) == 0)
	{
		snprintf(path, sizeof(path) - 1, "%s/log/syslog/error.log", getenv("SWWORK"));
	}
	else
	{
		if (strlen(proc) == 0)
		{
			snprintf(path, sizeof(path) - 1, "%s/log/%s/syslog/error.log", getenv("SWWORK"), prdt);
		}
		else if (strstr(proc, "swsvcman") == NULL)
		{
			snprintf(path, sizeof(path) - 1, "%s/log/syslog/%s.log", getenv("SWWORK"), proc);
		}
		else
		{
			strcpy(prdt, proc+strlen("swsvcman_"));
			snprintf(path, sizeof(path) - 1, "%s/log/%s/syslog/%s.log", getenv("SWWORK"), prdt, proc);
		}
	}

	if ( access(path, F_OK) )
	{
		strcpy(resmsg, "文件不存在");
		goto ErrExit;
	}

	pub_log_info("[%s][%d] 发送文件!!", __FILE__, __LINE__);

	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
	loc_set_zd_data(vars, "$send_path", path);

OkExit:
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "交易成功");
	pub_log_debug("[%s][%d] [%s] Trade 2042 Success![END][OK]", __FILE__, __LINE__, __FUNCTION__);

	return SW_OK;
ErrExit:

	pub_log_debug("[%s][%d] [%s]Trade 2042 Fails!", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(rescode, resmsg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", rescode);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", resmsg);

	return SW_ERROR;
}

