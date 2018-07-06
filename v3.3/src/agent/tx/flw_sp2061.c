/*************************************************
  文 件 名:  flw_sp2061.c                       **
  功能描述:  预警条件配置                       **
  作    者:  邹佩                               **
  完成日期:  20160802                           **
 *************************************************/
#include "agent_comm.h"

typedef struct condition_s
{
	char	name[128];
	char	key[64];
	char	ral[16];
	char	value[64];
}condition_t;

static int get_condition_request_info(sw_loc_vars_t *vars, condition_t *condition)
{	
	memset(condition->name, 0x0, sizeof(condition->name));
	loc_get_zd_data(vars, ".TradeRecord.Request.CONDITION.NAME", condition->name);
	
	memset(condition->key, 0x0, sizeof(condition->key));
	loc_get_zd_data(vars, ".TradeRecord.Request.CONDITION.KEY", condition->key);
	
	memset(condition->ral, 0x0, sizeof(condition->ral));
	loc_get_zd_data(vars, ".TradeRecord.Request.CONDITION.RELATION", condition->ral);
	
	memset(condition->value, 0x0, sizeof(condition->value));
	loc_get_zd_data(vars, ".TradeRecord.Request.CONDITION.VALUE", condition->value);

	return 0;
}

static int set_condition_response_info(sw_loc_vars_t *vars, condition_t *condition, int i)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.CONDITIONS.CONDITION(%d).NAME", i);
	loc_set_zd_data(vars, path, condition->name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.CONDITIONS.CONDITION(%d).KEY", i);
	loc_set_zd_data(vars, path, condition->key);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.CONDITIONS.CONDITION(%d).RELATION", i);
	loc_set_zd_data(vars, path, condition->ral);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.CONDITIONS.CONDITION(%d).VALUE", i);
	loc_set_zd_data(vars, path, condition->value);

	return 0;
}

int	sp2061(sw_loc_vars_t *vars)
{
	int	i = 0;
	int	flag = 0;
	char	opt[8];
	char	buf[512];
	char	tmp[512];
	char	prdt[64];
	char	reply[16];
	char	res_msg[128];
	char	xmlname[128];
	condition_t	condition;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	memset(reply, 0x00, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));

	memset(prdt, 0x00, sizeof(prdt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", prdt);

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/alert_rule.xml", getenv("SWWORK"), prdt);
	if (!pub_file_exist(xmlname))
	{
		pub_log_info("[%s][%d] xml [%s] is not exist,start to create xml!", __FILE__, __LINE__, xmlname);
		xml = pub_xml_unpack_ext(NULL, 0);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] create XML tree error!", __FILE__, __LINE__);
			strcpy(reply, "E045");
			goto ErrExit;
		}
		node = pub_xml_addnode(xml->root, "ALERT", "", SW_NODE_ROOT);
		if (node == NULL)
		{
			pub_log_error("[%s][%d]add child node error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			strcpy(reply, "E025");
			goto ErrExit;
		}

		node1 = pub_xml_addnode(node, "CONDITION", "", SW_NODE_ELEMENT);
		pub_xml_pack(xml, xmlname);	
	}
	else
	{
		xml = pub_xml_crtree(xmlname);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
			strcpy(reply, "E045");
			goto ErrExit;
		}
	}
	
	memset(opt, 0x00, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d]操作标识opt[%s]", __FILE__, __LINE__, opt);

	if (opt[0] == 'S')
	{
		node = pub_xml_locnode(xml, ".ALERT.CONDITION.TERM");
		i = 0;
		while(node != NULL)
		{
			node1 = node->firstchild;
			memset(&condition, 0x0, sizeof(condition));	
			while(node1 != NULL)
			{
				if (strcmp(node1->name, "NAME") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(condition.name, node1->value, sizeof(condition.name));
				}
				else if (strcmp(node1->name, "KEY") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(condition.key, node1->value, sizeof(condition.key));				
				}
				else if (strcmp(node1->name, "RELATION") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(condition.ral, node1->value, sizeof(condition.ral));				
					change_ral(condition.ral, 1);
				}
				else if (strcmp(node1->name, "VALUE") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(condition.value, node1->value, sizeof(condition.value));
				}

				node1 = node1->next;
			}
			pub_log_debug("[%s][%d]get condition success", __FILE__, __LINE__);
			set_condition_response_info(vars, &condition, i);		
			i++;
			node = node->next;
		}

		memset(buf, 0x00, sizeof(buf));
		sprintf(buf, "%d", i);
		loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", buf);
		pub_xml_deltree(xml);
		goto OkExit;

	}
	else if (opt[0] == 'M')
	{
		i = 0;

		memset(&condition, 0x0, sizeof(condition));
		get_condition_request_info(vars, &condition);
		node = agt_xml_search(xml, ".ALERT.CONDITION.TERM", "NAME", condition.name);
		if (node == NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			sprintf(res_msg, "预警条件 %s 不存在！", condition.name);
			strcpy(reply, "E999");
			pub_xml_deltree(xml);
			goto ErrExit;
		}

		node1 = node->firstchild;

		while(node1 != NULL)
		{
			if (strcmp(node1->name, "KEY") == 0)
			{
				pub_xml_set_value(node1, condition.key);
			}
			else if (strcmp(node1->name, "RELATION") == 0)
			{
				change_ral(condition.ral, 0);
				pub_xml_set_value(node1, condition.ral);
			}
			else if (strcmp(node1->name, "VALUE") == 0)
			{
				pub_xml_set_value(node1, condition.value);
			}
			node1 = node1->next;
		}
		
		pub_log_info("[%s][%d] ..........aaaaaaaa....",__FILE__, __LINE__);
		node = NULL;
		node1 = NULL;
		flag = 0;
		/*预警条件改变,相应预警规则改变*/
		memset(tmp, 0x0, sizeof(tmp));
		node =  pub_xml_locnode(xml, ".ALERT.BUSINESS.ITEM");
		while(node != NULL)
		{
			node1 = node->firstchild;
			while(node1 != NULL)
			{
				pub_log_info("[%s][%d] name=%s, flag=%d", __FILE__, __LINE__, node1->name, flag);
				if (strcmp(node1->name, "DESC") == 0 )
				{
					memset(buf, 0x0, sizeof(buf));
					memset(tmp, 0x0, sizeof(tmp));
					strncpy(buf, node1->value, sizeof(buf)-1);
				pub_log_info("[%s][%d] name=%s, value=%s  %sflag=%d", __FILE__, __LINE__, node1->name,node1->value,buf, flag);
					pack_exp(xml, buf, tmp);
					flag = 1;
				}
				else if (strcmp(node1->name, "EXP") == 0 && flag == 1)
				{
					pub_xml_set_value(node1, tmp);
					flag = 0;
					break;
				}
				node1 = node1->next;
			}
			if (flag != 0)
			{
				continue;
			}
			node = node->next;
		}
		
		

		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);
		goto OkExit;
	}
	else if(opt[0] == 'A')
	{
		memset(&condition, 0x0, sizeof(condition));
		get_condition_request_info(vars, &condition);

		node = agt_xml_search(xml, ".ALERT.CONDITION.TERM", "NAME", condition.name);
		if (node != NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			sprintf(res_msg, "预警条件 %s 已存在！", condition.name);
			strcpy(reply, "E999");
			pub_xml_deltree(xml);
			goto ErrExit;
		}

		node = pub_xml_locnode(xml, ".ALERT.CONDITION");
		if (node == NULL)
		{
			pub_log_error("[%s][%d] pub_xml_locnode failed!", __FILE__, __LINE__);
			node1 = pub_xml_locnode(xml, ".ALERT");
			node = pub_xml_addnode(node1, "CONDITION", "", SW_NODE_ELEMENT);
		}

		node1 = pub_xml_addnode(node, "TERM", "", 1);

		pub_xml_addnode(node1, "NAME", condition.name, 0);

		pub_xml_addnode(node1, "KEY", condition.key, 0);

		change_ral(condition.ral, 0);
		pub_xml_addnode(node1, "RELATION", condition.ral, 0);
		pub_xml_addnode(node1, "VALUE", condition.value, 0);

		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);
		goto OkExit;

	}
	else if(opt[0] == 'D')
	{
		memset(&condition, 0x0, sizeof(condition));
		get_condition_request_info(vars, &condition);

		node = agt_xml_search(xml, ".ALERT.CONDITION.TERM", "NAME", condition.name);
		if (node == NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			sprintf(res_msg, "预警条件 %s 不存在！", condition.name);
			strcpy(reply, "E999");
			pub_xml_deltree(xml);
			goto ErrExit;
		}

		node1 = NULL;
		flag = 0;
		node1 =  pub_xml_locnode(xml, ".ALERT.BUSINESS.ITEM");
		while(node1 != NULL)
		{
			node2 = node1->firstchild;
			while(node2 != NULL)
			{
				if (strcmp(node2->name, "DESC") == 0 )
				{
					memset(tmp, 0x0, sizeof(tmp));
					strncpy(buf, node2->value, sizeof(buf)-1);
					pub_log_info("[%s][%d] value=%s", __FILE__, __LINE__, buf);
					if (strstr(buf, condition.name) != NULL)
					{
						flag = 1;
					}
				}
				node2 = node2->next;
			}
			node1 = node1->next;
		}

		if(flag == 1)
		{
			pub_log_info("[%s][%d] 预警规则有使用被删除的预警条件!", __FILE__, __LINE__);
			strcpy(reply, "E052");
			goto ErrExit;
		}
		
		agt_remove_node(node);
		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);
		goto OkExit;
	}
	else
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d] 操作标识[%s]error!", __FILE__, __LINE__, opt);
		goto ErrExit;
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]OK EXIT![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return SW_OK;
ErrExit:
	pub_log_debug("[%s][%d] [%s]ERR EXIT![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}
