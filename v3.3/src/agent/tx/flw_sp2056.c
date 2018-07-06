/*************************************************
  文 件 名:  flw_sp2056.c                        **
  功能描述:  获取表的表结构                      **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"
#include "pub_db.h"

static char *types[32] = 
{
	"CHAR",
	"SMALLINT",
	"INTEGER",
	"FLOAT",
	"SMALLFLOAT",
	"DECIMAL",
	"SERIAL *",
	"DATE",
	"MONEY",
	"NULL",
	"DATETIME",
	"BYTE",
	"TEXT",
	"VARCHAR",
	"INTERVAL",
	"NCHAR",
	"NVARCHAR",
	"INT8",
	"SERIAL8 *",
	"SET",
	"MULTISET",
	"LIST",
	"Unnamed ROW"
};

static char *get_informix_type(int itype)
{
	/*type+256="NOT NULL"*/

	if (itype == 4118)
	{
		return "Named ROW";
	}
	
	if (itype >= 256)
	{
		itype -= 256;
	}

	if (itype >= 0 && itype <= 22)
	{
		return types[itype];
	}
	else if (itype == 40)
	{
		return "LVARCHAR";
	}
	else if (itype == 4118)
	{
		return "Named ROW";
	}
	else
	{
		return "UNKNOWN TYPE";
	}
}

int sp2056(sw_loc_vars_t *vars)
{
	int		i;
	int		ret = -1;
	int		itype = 0;
	char	res_msg[256];
	char	reply[8];	
	char	path[256];
	char	sdbname[64];
	char	tabname[64];
	char	scmd[1024];
	char	sdbuser[64];
	char	sdbpasswd[64];
	char	sdbsid[64];
	char	sdbtype[64];
	char	colname[256];
	char	datatype[64];
	char	buf[1024];
	char	sql[1024];
	char	*p = NULL;
	char	filepath[256];
	FILE	*fp = NULL;

	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_mem_memzero(reply, sizeof(reply));

	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(tabname, sizeof(tabname));
	sprintf(path, ".TradeRecord.Request.TableName");
	loc_get_zd_data(vars, path, tabname);
	if (strlen(tabname) == 0)
	{
		pub_log_error("[%s][%d] no .TradeRecord.Request.TableName.", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(path, 0x0, sizeof(path));
	memset(sdbname, 0x0, sizeof(sdbname));
	sprintf(path, ".TradeRecord.Request.DBNAME");
	loc_get_zd_data(vars, path, sdbname);

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

	memset(scmd, 0x0, sizeof(scmd));
	memset(sql, 0x0, sizeof(sql));	
	sprintf(filepath, "%s/tmp/agent/%s.uld", getenv("SWWORK"), tabname);
	if (strcmp(sdbtype, "ORACLE") == 0)
	{
		snprintf(sql, sizeof(sql) - 1, "\"select COLUMN_NAME,DATA_TYPE from USER_TAB_COLS where TABLE_NAME='%s';\"",
			tabname);
		snprintf(scmd, sizeof(scmd) - 1, "sh %s/sbin/agent_sql.sh %s %s %s %s %s %s > %s", 
			getenv("SWHOME"), sql, sdbtype, "NULL", sdbsid, sdbuser, sdbpasswd, filepath);
	}
	else if (strcmp(sdbtype, "INFORMIX") == 0)
	{
		snprintf(sql, sizeof(sql) - 1, "\"SELECT c.colname, c.coltype FROM syscolumns c, systables t WHERE c.tabid = t.tabid AND t.tabname = '%s' ;\"",
			tabname);
		snprintf(scmd, sizeof(scmd) - 1, "sh %s/sbin/agent_sql.sh %s %s %s %s %s %s %s", 
			getenv("SWHOME"), sql, sdbtype, filepath, sdbsid, sdbuser, sdbpasswd, sdbname);
	}

	pub_log_debug("[%s][%d]scmd[%s]filepath[%s]", __FILE__, __LINE__, scmd, filepath);

	ret = agt_system(scmd);
	if (ret != 0)
	{
		pub_log_error("[%s][%d]deal cmd[%s]error", __FILE__, __LINE__, scmd);
		strcpy(reply, "E010");
		goto ErrExit;
	}
	
	fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d]open file[%s] error", __FILE__, __LINE__, filepath);
		strcpy(reply, "E016");
		goto ErrExit;
	}

	i = 0;
	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf) - 1, fp);

		if (buf[strlen(buf) - 1] == '\n')
		{
			buf[strlen(buf) - 1] = '\0';
		}

		if ((p = strchr(buf, '|')) == NULL)
		{
			continue;
		}

		memset(colname, 0x0, sizeof(colname));
		memcpy(colname, buf, p - buf);
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Table.Column(%d).Name", i);
		loc_set_zd_data(vars, path, colname);

		if (strcmp(sdbtype, "ORACLE") == 0)
		{
			memset(datatype, 0x0, sizeof(datatype));
			strncpy(datatype, p + 1, sizeof(datatype) - 1);

		}	
		else if (strcmp(sdbtype, "INFORMIX") == 0)
		{	
			memset(datatype, 0x0, sizeof(datatype));
			sscanf(p + 1, "%d", &itype);
			strncpy(datatype, get_informix_type(itype), sizeof(datatype) - 1);
		}
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Table.Column(%d).Type", i);
		loc_set_zd_data(vars, path, datatype);	

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Table.Column(%d).IsCheck", i);
		loc_set_zd_data(vars, path, "T");	

		++i;
	}

	fclose(fp);
	remove(filepath);

OkExit:
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	pub_log_debug("[%s][%d] [%s] Trade 2056 Success![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	return 0;	
ErrExit:
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	pub_log_debug("[%s][%d] [%s]Trade 2056 Fails!", __FILE__, __LINE__, __FUNCTION__);
	return -1;
}
