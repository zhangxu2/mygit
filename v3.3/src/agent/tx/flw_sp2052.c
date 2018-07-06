/*************************************************
  文 件 名:  flw_sp2052.c                        **
  功能描述:  平台配置管理(流水设置)              **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

typedef struct seqn_s
{
	char	sqid[32];
	char	sqname[32];
	char	sqmode[32];
	char	sqstart[32];
	char	sqstep[32];
	char	sqmax[32];
	char	sqcycle[32];
	char	sqprefix[32];
	char	sqbusprefix[32];
}seqn_t;

static int get_seqn_info( sw_loc_vars_t *vars, seqn_t *seqn)
{
	char	path[256];

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQID"); 
	loc_get_zd_data(vars, path, seqn->sqid);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQNAME"); 
	loc_get_zd_data(vars, path, seqn->sqname);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQMODE"); 
	loc_get_zd_data(vars, path, seqn->sqmode);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQSTART"); 
	loc_get_zd_data(vars, path, seqn->sqstart);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQSTEP"); 
	loc_get_zd_data(vars, path, seqn->sqstep);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQMAX"); 
	loc_get_zd_data(vars, path, seqn->sqmax);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQCYCLE"); 
	loc_get_zd_data(vars, path, seqn->sqcycle);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQPREFIX"); 
	loc_get_zd_data(vars, path, seqn->sqprefix);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Request.SWCFG.SEQNS.SEQN.SQBUSPREFIX"); 
	loc_get_zd_data(vars, path, seqn->sqbusprefix);	

	return 0;
}

static int set_seqn_info(sw_loc_vars_t *vars, seqn_t *seqn, int i)
{
	char	path[256];

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQID", i); 
	loc_set_zd_data(vars, path, seqn->sqid);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQNAME", i); 
	loc_set_zd_data(vars, path, seqn->sqname);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQMODE", i); 
	loc_set_zd_data(vars, path, seqn->sqmode);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQSTART", i); 
	loc_set_zd_data(vars, path, seqn->sqstart);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQSTEP", i); 
	loc_set_zd_data(vars, path, seqn->sqstep);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQMAX", i); 
	loc_set_zd_data(vars, path, seqn->sqmax);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQCYCLE", i); 
	loc_set_zd_data(vars, path, seqn->sqcycle);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQPREFIX", i); 
	loc_set_zd_data(vars, path, seqn->sqprefix);	

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SWCFG.SEQNS.SEQN(%d).SQBUSPREFIX", i); 
	loc_set_zd_data(vars, path, seqn->sqbusprefix);	

	return 0;
}

/* 2052 */
int sp2052(sw_loc_vars_t *vars)
{
	int 	ret = -1;
	int 	i = 0;
	char	option[256];
	char	path[256];
	char	reply[16];
	char	res_msg[256];
	sw_xmltree_t	*xmltree = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	seqn_t	seqn;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset( option, 0x00, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);

	if (option[0] != 'M' && option[0] != 'S')
	{
		pub_log_error("[%s][%d].TradeRecord.Request.Option value error[%s]", __FILE__, __LINE__, option);
		strncpy(reply, "E012", sizeof(reply) - 1);
		goto ErrExit;
	}
		
	memset(path, 0x00, sizeof(path));
	sprintf(path, "%s/cfg/swconfig.xml", getenv("SWWORK"));
	xmltree = pub_xml_crtree(path);
	if (xmltree == NULL)
	{
		pub_log_error("[%s][%d].creat xml tree error  path[%s]", __FILE__, __LINE__, path);
		strncpy(reply, "E045", sizeof(reply) - 1);
		goto ErrExit;
	}

	if (option[0] == 'M')
	{
		ret = agt_back_file("swconfig.xml");
		if (ret < 0)
		{
			pub_log_error("[%s][%d]备份文件失败", __FILE__, __LINE__);
			strcpy(res_msg, "备份文件失败");
			goto ErrExit;
		}
		memset(&seqn, 0x0, sizeof(seqn));
		get_seqn_info(vars, &seqn);
		
		node = agt_xml_search(xmltree, ".SWCFG.SEQN", "SQID", seqn.sqid);
		if (node == NULL)
		{
			pub_log_error("[%s][%d]流水生成器 %s 不存在！", __FILE__, __LINE__, seqn.sqid);
			sprintf(res_msg, "流水生成器 %s 不存在！", seqn.sqid);
			strcpy(reply, "E999");
			pub_xml_deltree(xmltree);
			goto ErrExit;
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQID");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQID", seqn.sqid, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqid);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQNAME");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQNAME", seqn.sqname, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqname);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQMODE");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQMODE", seqn.sqmode, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqmode);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQSTART");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQSTART", seqn.sqstart, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqstart);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQSTEP");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQSTEP", seqn.sqstep, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqstep);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQMAX");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQMAX", seqn.sqmax, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqmax);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQCYCLE");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node,"SQCYCLE",  seqn.sqcycle, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqcycle);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQPREFIX");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQPREFIX", seqn.sqprefix, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqprefix);
		}

		xmltree->current = node;
		node1 = pub_xml_locnode(xmltree, "SQBUSPREFIX");
		if (node1 == NULL)
		{
			node1 = pub_xml_addnode(node, "SQBUSPREFIX", seqn.sqbusprefix, 0);
		}
		else
		{
			pub_xml_set_value(node1, seqn.sqbusprefix);
		}

		ret = pub_xml_pack(xmltree, path);;
		if (ret != 0)
		{
			pub_log_error("[%s][%d]pack xmltree error ret=[%d]", __FILE__, __LINE__, ret);
			pub_xml_deltree(xmltree);
			strncpy(reply, "E046", sizeof(reply) - 1);
			goto ErrExit;
		}

		pub_xml_deltree(xmltree);
		goto OkExit;
	}
	else if (option[0] == 'S')
	{
		node = pub_xml_locnode(xmltree, ".SWCFG.SEQN");
		if (node == NULL)
		{
			pub_log_error("[%s][%d]流水生成器不存在！", __FILE__, __LINE__);
			sprintf(res_msg, "流水生成器不存在！");
			strcpy(reply, "E999");
			pub_xml_deltree(xmltree);
			goto ErrExit;
		}

		while (node != NULL)
		{
			memset(&seqn, 0x0, sizeof(seqn));
			node1 = node->firstchild;
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "SQID") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqid, node1->value, sizeof(seqn.sqid) - 1);
				}

				if (strcmp(node1->name, "SQNAME") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqname, node1->value, sizeof(seqn.sqname) - 1);
				}

				if (strcmp(node1->name, "SQMODE") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqmode, node1->value, sizeof(seqn.sqmode) - 1);
				}

				if (strcmp(node1->name, "SQSTART") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqstart, node1->value, sizeof(seqn.sqstart) - 1);
				}

				if (strcmp(node1->name, "SQSTEP") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqstep, node1->value, sizeof(seqn.sqstep) - 1);
				}

				if (strcmp(node1->name, "SQMAX") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqmax, node1->value, sizeof(seqn.sqmax) - 1);
				}

				if (strcmp(node1->name, "SQCYCLE") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqcycle, node1->value, sizeof(seqn.sqcycle) - 1);
				}

				if (strcmp(node1->name, "SQPREFIX") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqprefix, node1->value, sizeof(seqn.sqprefix) - 1);
				}

				if (strcmp(node1->name, "SQBUSPREFIX") == 0 && node1->value != NULL)
				{
					strncpy(seqn.sqbusprefix, node1->value, sizeof(seqn.sqbusprefix) - 1);
				}

				node1 = node1->next;
			}
			set_seqn_info(vars, &seqn, i);
			i++;
			node = node->next;
		}	
		
		pub_xml_deltree(xmltree);
		goto OkExit;
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	if (option[0] != 'S')
	{
		ret = record_oper_log(vars, reply, res_msg);
		if(ret < 0)
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
