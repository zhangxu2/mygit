/*************************************************
  文 件 名:  flw_sp2096.c                        
  功能描述:  生成listener.tar                 
  作    者:  guxiaoxin                                
  完成日期:  20160803                               
 *************************************************/
#include "agent_comm.h"

static int pub_prdtio_out(char *file)
{
	char	now[32];
	char	cmd[512];
	char	dustdir[256];

	if (file == NULL)
	{
		pub_log_error( "[%s][%d] input parameter error", __FILE__, __LINE__ );
		return -1;
	}

	if (getenv("SWWORK") == NULL)
	{
		pub_log_error( "[%s][%d] getenv(SWWORK) error", __FILE__, __LINE__ );
		return -1;
	}

	pub_mem_memzero(now, sizeof(now));
	agt_date_format(time(NULL), now, "F");

	pub_mem_memzero( dustdir, sizeof( dustdir));
	sprintf(dustdir, "%s/%s/%s",getenv("SWWORK"), SPACE, now);
	if (aix_mkdirp(dustdir, 0777) < 0)
	{
		pub_log_error( "[%s][%d]creat dir error ", __FILE__, __LINE__ );
		return -1;
	}

	pub_mem_memzero(cmd, sizeof(cmd));
	sprintf(cmd, "cd %s/cfg ; tar cf %s/listener.tar listener.lsn channels.xml", getenv("SWWORK"), dustdir );
	if (agt_system(cmd) < 0)
	{
		pub_log_error( "[%s][%d] execute cmd [%s] error ", __FILE__, __LINE__, cmd );
		return -1;
	}

	sprintf(file, "%s/listener.tar ", dustdir);

	return 0;
}

int sp2096(sw_loc_vars_t *vars)
{
	char    reply[8];
	char    res_msg[256];
	char	outfile[256];

	pub_mem_memzero(reply,   sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_mem_memzero(outfile, sizeof(outfile));
	
	if (pub_prdtio_out(outfile) < 0)
	{
		pub_log_error("[%s][%d] pub_prdtio_out error", __FILE__, __LINE__ );
		strcpy(reply, "E048");
		goto ErrExit;
	}
	pub_log_debug("[%s][%d] pub_prodtio_out sucess, file[%s]", __FILE__, __LINE__, outfile);
	loc_set_zd_data(vars, ".TradeRecord.Response.Path", outfile);

	pub_log_debug("[%s][%d] file[%s] will be sent", __FILE__, __LINE__, outfile);
	
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");
	return 0;

ErrExit:
	pub_log_debug("[%s][%d] deal end![END][ERR]", __FILE__, __LINE__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode",  "E999");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage",  "Transaction processes failure");
	return -1;
}
