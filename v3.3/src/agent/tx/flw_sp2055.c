/*************************************************
  文 件 名:  flw_sp2055.c                        **
  功能描述:  获取数据库所有表名                  **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"
#include "pub_db.h"

int sp2055(sw_loc_vars_t *vars)
{
	int 	i;
	char	res_msg[256];
	char	reply[8];	
	char	path[256];
	char	tablename[64];
	char	scmd[1024];
	char	sdbuser[64];
	char	sdbpasswd[64];
	char	sdbsid[64];
	char	sdbname[64];
	char	sdbtype[64];
	char	sql[1024];
	FILE	*fp = NULL;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset(path, 0x0, sizeof(path));
	memset(sdbtype, 0x0, sizeof(sdbtype));
	sprintf(path, ".TradeRecord.Request.DBTYPE");
	loc_get_zd_data(vars, path, sdbtype);

	memset(path, 0x0, sizeof(path));
	memset(sdbuser, 0x0, sizeof(sdbuser));
	sprintf(path, ".TradeRecord.Request.DBUSER");
	loc_get_zd_data(vars, path, sdbuser);

	memset(path, 0x0, sizeof(path));
	memset(sdbpasswd, 0x0, sizeof(sdbpasswd));
	sprintf(path, ".TradeRecord.Request.DBPASSWD");
	loc_get_zd_data(vars, path, sdbpasswd);

	memset(path, 0x0, sizeof(path));
	memset(sdbname, 0x0, sizeof(sdbname));
	sprintf(path, ".TradeRecord.Request.DBNAME");
	loc_get_zd_data(vars, path, sdbname);

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

	memset(scmd, 0x0, sizeof(scmd));
	memset(sql, 0x0, sizeof(sql));
	if (strcmp(sdbtype, "ORACLE") == 0)
	{
		snprintf(sql, sizeof(sql) - 1, "\"select table_name from user_tables order by table_name;\"");
	}
	else if (strcmp(sdbtype, "INFORMIX") == 0)
	{
		snprintf(sql, sizeof(sql) - 1, "\"select tabname from systables where tabid>=100 order by tabname;\"");
	}
	snprintf(scmd, sizeof(scmd) - 1, "sh %s/sbin/agent_sql.sh %s %s %s %s %s %s %s",
				getenv("SWHOME"), sql, sdbtype, "NULL", sdbsid, sdbuser, sdbpasswd, sdbname);

	errno = 0;
	pub_log_debug("[%s][%d] scmd[%s]", __FILE__, __LINE__, scmd);
	fp = popen(scmd, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] deal cmd[%s]error, errno=[%d]:[%s]", __FILE__, __LINE__, scmd, errno, strerror(errno));
		strcpy(reply, "E010");
		goto ErrExit;
	}

	i = 0;
	while (!feof(fp))
	{
		memset(tablename, 0x0, sizeof(tablename));
		fgets(tablename, sizeof(tablename) - 1, fp);
		
		if (i == 0 && (strstr(tablename, "ERROR") != NULL))
		{
			pub_log_error("[%s][%d] 连接数据库失败", __FILE__, __LINE__);
			strcpy(reply, "E035");
			pclose(fp);
			goto ErrExit;
		}

		if (tablename[strlen(tablename) - 1] == '\n')
		{
			tablename[strlen(tablename) - 1] = '\0';
		}

		if (strlen(tablename) <= 0)
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Tables.Table(%d).TableName", i);
		loc_set_zd_data(vars, path, tablename);
		++i;
	}
	pclose(fp);

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return 0;

ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);

	agt_error_info(reply, res_msg);	
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
