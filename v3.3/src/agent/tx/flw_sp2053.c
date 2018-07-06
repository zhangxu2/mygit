/*************************************************
  文 件 名:  flw_sp2053.c                        **
  功能描述:  平台配置管理(数据库配置)            **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

typedef struct 
{
	int 	count;
	sw_dbcfg_t	database[DB_MAX_DEFAULT];
}sw_dbconfig_t;

static int get_dbconfig(sw_dbconfig_t *dbconfig, sw_xmltree_t *xml)
{
	int 	i = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	node = pub_xml_locnode(xml, ".DBCONFIG.DATABASE");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] No .DBCONFIG.DATABASE in file!"
			, __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] No DBID in file!"
				, __FILE__, __LINE__);
			return -1;
		}
		else
		{
			strncpy(dbconfig->database[i].dbid, node1->value, sizeof(dbconfig->database[i].dbid));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbid=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbid);
		}

		node1 = pub_xml_locnode(xml, "DBNAME");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBNAME in file!"
				, __FILE__, __LINE__);
			return -1;
		}
		else
		{
			strncpy(dbconfig->database[i].dbname, node1->value, sizeof(dbconfig->database[i].dbname));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbname=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbname);
		}

		node1 = pub_xml_locnode(xml, "DBTYPE");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBTYPE in file!"
				, __FILE__, __LINE__);
		}
		else
		{
			strncpy(dbconfig->database[i].dbtype, node1->value, sizeof(dbconfig->database[i].dbtype));
			pub_log_debug("[%s][%d] dbconfig->database[%d].dbtype=%s"
				, __FILE__, __LINE__, i, dbconfig->database[i].dbtype);
		}

		node1 = pub_xml_locnode(xml, "DBSID");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] No DBSID in file!"
				, __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] No DBUSER in file!"
				, __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] No DBPASSWD in file!"
				, __FILE__, __LINE__ );
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

static int get_prdt_conf_getdata(sw_loc_vars_t *vars ,sw_xmltree_t *xml)
{
	int		i = 0; 
	int		ret = -1;
	char	base[256];
	sw_dbconfig_t	dbconfig;

	pub_mem_memzero(&dbconfig, sizeof(dbconfig));
	ret = get_dbconfig(&dbconfig, xml);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] get_dbconfig fail!", __FILE__, __LINE__);
		return -1;
	}

	for (i = 0; i < dbconfig.count; i++)
	{
		pub_mem_memzero(base, sizeof(base));
		sprintf(base, ".TradeRecord.Response.SWCFG.DBCONFIG.DATABASE(%d).DBID", i); 
		loc_set_zd_data(vars, base, dbconfig.database[i].dbid);

		pub_mem_memzero(base, sizeof(base));
		sprintf(base, ".TradeRecord.Response.SWCFG.DBCONFIG.DATABASE(%d).DBTYPE", i); 
		loc_set_zd_data(vars, base, dbconfig.database[i].dbtype);

		pub_mem_memzero(base, sizeof(base));
		sprintf(base, ".TradeRecord.Response.SWCFG.DBCONFIG.DATABASE(%d).DBNAME", i); 
		loc_set_zd_data(vars, base, dbconfig.database[i].dbname);

		pub_mem_memzero(base, sizeof(base));
		sprintf(base, ".TradeRecord.Response.SWCFG.DBCONFIG.DATABASE(%d).DBSID", i); 
		loc_set_zd_data(vars, base, dbconfig.database[i].dbsid);

		pub_mem_memzero(base, sizeof(base));
		sprintf(base, ".TradeRecord.Response.SWCFG.DBCONFIG.DATABASE(%d).DBUSER", i); 
		loc_set_zd_data(vars, base, dbconfig.database[i].dbuser);

		pub_mem_memzero(base, sizeof(base));
		sprintf(base, ".TradeRecord.Response.SWCFG.DBCONFIG.DATABASE(%d).DBPASSWD", i); 
		loc_set_zd_data(vars, base, dbconfig.database[i].dbpasswd);
	}

	return 0;
}

/* 2053 */
int sp2053(sw_loc_vars_t *vars)
{
	int		ret = -1;
	char	sbuf[256];
	char	dbid[64];
	char	cmd[256];
	char	name[256];
	char	reply[16];
	char	res_msg[256];
	sw_xmlnode_t	*node = NULL;
	sw_xmltree_t	*xml = NULL;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	pub_mem_memzero(name, sizeof(name));
	sprintf(name, "%s/cfg/dbconfig.xml", getenv("SWWORK"));
	
	xml = pub_xml_crtree(name);
	if (xml == NULL)
	{
		 pub_log_error("[%s][%d] %s is not exist ", __FILE__, __LINE__, name);
		 strcpy(reply, "E009");
		 goto ErrExit;
	}
	memset(cmd, 0x00, sizeof(cmd));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", cmd);

	if (strcmp(cmd,"M") == 0)
	{
		ret = agt_back_file("dbconfig.xml");
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 备份文件失败 ", __FILE__, __LINE__);
			strcpy(res_msg, "备份文件失败");
			goto ErrExit;
		}
	
		memset(dbid, 0x0, sizeof(dbid));
		loc_get_zd_data(vars, ".TradeRecord.Request.SWCFG.DBCONFIG.DATABASE.DBID", dbid);
		if (strlen(dbid) == 0)
		{
			pub_log_error("[%s][%d] get DBID is null ", __FILE__, __LINE__);
			strcpy(reply, "E012");
			goto ErrExit;
		}
		node = agt_xml_search(xml, ".DBCONFIG.DATABASE", "DBID", dbid);
		if (node == NULL)
		{
			pub_log_error("[%s][%d] dbid[%s] is not exit ", __FILE__, __LINE__, dbid);
			strcpy(reply, "E026");
			goto ErrExit;
		}
		
		xml->current = node;
		node = node->firstchild;
		while (node != NULL)
		{
			if (strcmp(node->name, "DBNAME") == 0)
			{
				memset(sbuf, 0x0, sizeof(sbuf));
				loc_get_zd_data(vars, ".TradeRecord.Request.SWCFG.DBCONFIG.DATABASE.DBNAME", sbuf);
				strcpy(node->value, sbuf); 
			}
			if (strcmp(node->name, "DBTYPE") == 0)
			{
				memset(sbuf, 0x0, sizeof(sbuf));
				loc_get_zd_data(vars, ".TradeRecord.Request.SWCFG.DBCONFIG.DATABASE.DBTYPE", sbuf);
				strcpy(node->value, sbuf); 
			}
			if (strcmp(node->name, "DBSID") == 0)
			{
				memset(sbuf, 0x0, sizeof(sbuf));
				loc_get_zd_data(vars, ".TradeRecord.Request.SWCFG.DBCONFIG.DATABASE.DBSID", sbuf);
				strcpy(node->value, sbuf); 
			}
			if (strcmp(node->name, "DBUSER") == 0)
			{
				memset(sbuf, 0x0, sizeof(sbuf));
				loc_get_zd_data(vars, ".TradeRecord.Request.SWCFG.DBCONFIG.DATABASE.DBUSER", sbuf);
				strcpy(node->value, sbuf); 
			}
			if (strcmp(node->name, "DBPASSWD") == 0)
			{
				memset(sbuf, 0x0, sizeof(sbuf));
				loc_get_zd_data(vars, ".TradeRecord.Request.SWCFG.DBCONFIG.DATABASE.DBPASSWD", sbuf);
				strcpy(node->value, sbuf); 
			}
			node = node->next;
		}

		ret = pub_xml_pack(xml, name);
		if (ret != 0)
		{
			pub_log_error("[%s][%d]get xmltree value to buf error ret=[%d]", __FILE__, __LINE__, ret);
			pub_xml_deltree(xml);
			strncpy(reply, "E046", sizeof(reply) - 1);
			goto ErrExit;
		}

		pub_xml_deltree(xml);

	}
	else if (strcmp(cmd, "S") == 0)
	{
		if (get_prdt_conf_getdata(vars, xml) < 0)
		{
			pub_log_error("[%s][%d] get data error", __FILE__, __LINE__);
			strcpy(res_msg, "读文件失败");
			pub_xml_deltree(xml);
			goto ErrExit;
		}

		pub_xml_deltree(xml);
	}
	else
	{
		pub_log_error("[%s][%d].TradeRecord.Request.Option value error[%s]", __FILE__, __LINE__, cmd);
		strncpy(reply, "E012", sizeof(reply) - 1);
		goto ErrExit;
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	if (cmd[0] != 'S')
	{
		ret = record_oper_log(vars, reply, res_msg);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 操作登记流水错误", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
