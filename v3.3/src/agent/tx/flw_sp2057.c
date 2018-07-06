/*************************************************
  文 件 名:  flw_sp2057.c                        **
  功能描述:  数据导出                            **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

int sp2057(sw_loc_vars_t *vars)
{
	int 	ret;
	char	filepath[256];	
	char	filename[256];
	char	res_msg[256];
	char	sql_state[1024 * 4];
	char	reply[8];	
	char	path[256];
	char	tabname[64];
	char	scmd[1024];
	char	sdbuser[64];
	char	sdbname[64];
	char	sdbpasswd[64];
	char	sdbsid[64];
	char	sdbtype[64];	
	char	sql[1024];

	memset(reply, 0x00, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));
	pub_mem_memzero(tabname, sizeof(tabname));
	pub_mem_memzero(sql_state, sizeof(sql_state));

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.TableName");
	loc_get_zd_data(vars, path, tabname);
	if (strlen(tabname) == 0)
	{
		pub_log_error("[%s][%d] no .TradeRecord.Request.TableName.", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	pub_log_debug("[%s][%d] tabname [%s]", __FILE__, __LINE__, tabname);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SQL");
	loc_get_zd_data(vars, path, sql_state);
	if (strlen(sql_state) == 0)
	{
		pub_log_error("[%s][%d] no .TradeRecord.Request.SQL.", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(path, 0x0, sizeof(path));
	memset(sdbtype, 0x0, sizeof(sdbtype));
	sprintf(path, ".TradeRecord.Request.DBTYPE");
	loc_get_zd_data(vars, path, sdbtype);

	memset(path, 0x0, sizeof(path));
	memset(sdbname, 0x0, sizeof(sdbname));
	sprintf(path, ".TradeRecord.Request.DBNAME");
	loc_get_zd_data(vars, path, sdbname);

	memset(path, 0x0, sizeof(path));
	memset(sdbuser, 0x0, sizeof(sdbuser));
	sprintf(path, ".TradeRecord.Request.DBUSER");
	loc_get_zd_data(vars, path, sdbuser);

	memset(path, 0x0, sizeof(path));
	memset(sdbpasswd, 0x0, sizeof(sdbpasswd));
	sprintf(path, ".TradeRecord.Request.DBPASSWD");
	loc_get_zd_data(vars, path, sdbpasswd);

	memset(path, 0x0, sizeof(path));
	memset(sdbsid, 0x0, sizeof(sdbsid));
	sprintf(path, ".TradeRecord.Request.DBSID");
	loc_get_zd_data(vars, path, sdbsid);

	if (strlen(sdbtype) == 0 || strlen(sdbuser) == 0 || strlen(sdbsid) == 0)
	{
		pub_log_error("[%s][%d]请求报文有误", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(filepath, 0x0, sizeof(filepath));
	snprintf(filepath, sizeof(filepath) - 1, "%s/tmp/agent", getenv("SWWORK"));
	memset(filename, 0x0, sizeof(filename));
	snprintf(filename, sizeof(filename) - 1, "%s/%s.uld", filepath, tabname);
	pub_log_debug("[%s][%d] filepath[%s]", __FILE__, __LINE__, filename);

	memset(scmd, 0x0, sizeof(scmd));
	memset(sql, 0x0, sizeof(sql));	
	snprintf(sql, sizeof(sql) - 1, "\"%s;\"", sql_state);
	if (strcmp(sdbtype, "ORACLE") == 0)
	{
		snprintf(scmd, sizeof(scmd) - 1, "sh %s/sbin/agent_sql.sh %s %s %s %s %s %s > %s", 
			getenv("SWHOME"), sql, sdbtype, "NULL", sdbsid, sdbuser, sdbpasswd, filename);
	}

	else if (strcmp(sdbtype, "INFORMIX") == 0)
	{
		snprintf(scmd, sizeof(scmd) - 1, "sh %s/sbin/agent_sql.sh %s %s %s %s %s %s %s", 
			getenv("SWHOME"), sql, sdbtype, filename, sdbsid, sdbuser, sdbpasswd, sdbname);
	}

	pub_log_debug("[%s][%d]scmd[%s]", __FILE__, __LINE__, scmd);
	ret = agt_system(scmd);
	if (ret)
	{
		pub_log_error("[%s][%d]deal cmd error[%s]", __FILE__, __LINE__, scmd);
		strcpy(reply, "E010");
		goto ErrExit;	
	}	

	ret = access(filename, F_OK);
	if (ret)
	{
		pub_log_error("[%s][%d]file is not exist[%s]", __FILE__, __LINE__, filename);
		strcpy(reply, "E009");
		goto ErrExit;	

	}	

	loc_set_zd_data(vars, "$send_path", filename);
	loc_set_zd_data(vars, "$del_send_path", filename);
	loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");
	pub_log_debug("%s, %d, filepath[%s]", __FILE__, __LINE__, filename);

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return 0;

ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);

	return -1;
}
