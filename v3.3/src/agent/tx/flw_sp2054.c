/*************************************************
  文 件 名:  flw_sp2054.c                        **
  功能描述:  数据库信息查询                      **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

typedef struct 
{
	int 	count;
	sw_dbcfg_t	database[DB_MAX_DEFAULT];
}sw_dbconfig_t;

static int get_dbconfig(sw_dbconfig_t *dbconfig)
{
	int 	i = 0;
	int 	result = -1;
	char	path[PATH_LEN];
	char	*tmp = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("[%s][%d] No env SWWORK!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, "%s/cfg/dbconfig.xml", tmp);

	result = access(path, F_OK);
	if (result != 0)
	{
		pub_log_error("[%s][%d] File [%s] no exist!"
			, __FILE__, __LINE__, path);
		return -1;
	}

	xml = pub_xml_crtree(path);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml[%s] fail!"
			, __FILE__, __LINE__, path);
		return -1;
	}

	node = pub_xml_locnode(xml, ".DBCONFIG.DATABASE");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] No .DATABASES.DATABASE in file[%s]!"
			, __FILE__, __LINE__, path);
		pub_xml_deltree(xml);
		return -1;
	}

	i = 0;
	while (node != NULL)
	{
		if (strcmp(node->name, "DATABASE") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;

		node1 = pub_xml_locnode(xml, "DBID");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBID in file[%s]!"
				, __FILE__, __LINE__, path);
			pub_xml_deltree(xml);
			return -1;
		}
		else
		{
			strncpy(dbconfig->database[i].dbid, node1->value, sizeof(dbconfig->database[i].dbid));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbid=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbid);
		}

		node1 = pub_xml_locnode(xml, "DBTYPE");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBTYPE in file[%s]!"
				, __FILE__, __LINE__, path);
			pub_xml_deltree(xml);
			return -1;
		}
		else
		{
			strncpy(dbconfig->database[i].dbtype, node1->value, sizeof(dbconfig->database[i].dbtype));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbtype=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbtype);
		}

		node1 = pub_xml_locnode(xml, "DBNAME");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBNAME in file[%s]!"
				, __FILE__, __LINE__, path);
			if (strcmp(dbconfig->database[i].dbtype, "ORACLE") != 0)
			{
				pub_xml_deltree(xml);
				return -1;
			}
			strncpy(dbconfig->database[i].dbname, "NULL", sizeof(dbconfig->database[i].dbname));
		}
		else
		{
			strncpy(dbconfig->database[i].dbname, node1->value, sizeof(dbconfig->database[i].dbname));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbname=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbname);
		}

		node1 = pub_xml_locnode(xml, "DBSID");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBSID in file[%s]!"
				, __FILE__, __LINE__, path);
			pub_xml_deltree(xml);
			return -1;
		}
		else
		{
			strncpy(dbconfig->database[i].dbsid, node1->value, sizeof(dbconfig->database[i].dbsid));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbsid=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbsid);
		}

		node1 = pub_xml_locnode(xml, "DBUSER");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBUSER in file[%s]!"
				, __FILE__, __LINE__, path);
			if (strcmp(dbconfig->database[i].dbtype, "ORACLE") == 0)
			{
				pub_xml_deltree(xml);
				return -1;
			}
			strncpy(dbconfig->database[i].dbuser, "NULL", sizeof(dbconfig->database[i].dbuser));
		}
		else
		{
			strncpy(dbconfig->database[i].dbuser, node1->value, sizeof(dbconfig->database[i].dbuser));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbuser=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbuser);
		}

		node1 = pub_xml_locnode(xml, "DBPASSWD");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBPASSWD in file[%s]!"
				, __FILE__, __LINE__, path);
			if (strcmp(dbconfig->database[i].dbtype, "ORACLE") == 0)
			{
				pub_xml_deltree(xml);
				return -1;
			}
			strncpy(dbconfig->database[i].dbpasswd, "NULL", sizeof(dbconfig->database[i].dbpasswd));
		}
		else
		{
			strncpy(dbconfig->database[i].dbpasswd, node1->value, sizeof(dbconfig->database[i].dbpasswd));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbpasswd=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbpasswd);
		}

		i++;
		node = node->next;
	}

	dbconfig->count = i;
	return 0;
}

/* 2054 */
int sp2054(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	ret = -1;
	char	path[256];
	char	reply[16];
	char	res_msg[256];
	sw_dbconfig_t	dbconfig;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	
	pub_mem_memzero(&dbconfig, sizeof(dbconfig));
	ret = get_dbconfig(&dbconfig);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] get_dbconfig fail!", __FILE__, __LINE__);
		strcpy(reply, "E026");
		goto ErrExit;
	}

	for (i = 0; i < dbconfig.count; i++)
	{
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.DATABASES.DATABASE(%d).DBID", i); 
		loc_set_zd_data(vars, path, dbconfig.database[i].dbid);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.DATABASES.DATABASE(%d).DBTYPE", i); 
		loc_set_zd_data(vars, path, dbconfig.database[i].dbtype);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.DATABASES.DATABASE(%d).DBUSER", i); 
		loc_set_zd_data(vars, path, dbconfig.database[i].dbuser);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.DATABASES.DATABASE(%d).DBPASSWD", i); 
		loc_set_zd_data(vars, path, dbconfig.database[i].dbpasswd);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.DATABASES.DATABASE(%d).DBNAME", i); 
		loc_set_zd_data(vars, path, dbconfig.database[i].dbname);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.DATABASES.DATABASE(%d).DBSID", i); 
		loc_set_zd_data(vars, path, dbconfig.database[i].dbsid);
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
