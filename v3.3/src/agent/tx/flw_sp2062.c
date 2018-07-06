/*************************************************
  文 件 名:  flw_sp2062.c                        **
  功能描述:  预警编号设置                        **
  作    者:                                      **
  完成日期:                                      **
 *************************************************/
#include "agent_comm.h"

typedef struct alertno_s
{
	char	errorno[8];
	char	level[4];
	char	desc[64];
	char	reason[256];
	char	solve[256];
}alertno_t;

static int get_alertno_request_info(sw_loc_vars_t *vars, alertno_t *alertno)
{
	memset(alertno->errorno, 0x0, sizeof(alertno->errorno));
	loc_get_zd_data(vars, ".TradeRecord.Request.ALERTNO.ERRNO", alertno->errorno);
	
	memset(alertno->level, 0x0, sizeof(alertno->level));
	loc_get_zd_data(vars, ".TradeRecord.Request.ALERTNO.LEVEL", alertno->level);
	
	memset(alertno->desc, 0x0, sizeof(alertno->desc));
	loc_get_zd_data(vars, ".TradeRecord.Request.ALERTNO.DESC", alertno->desc);
	
	memset(alertno->reason, 0x0, sizeof(alertno->reason));
	loc_get_zd_data(vars, ".TradeRecord.Request.ALERTNO.REASON", alertno->reason);
	
	memset(alertno->solve, 0x0, sizeof(alertno->solve));
	loc_get_zd_data(vars, ".TradeRecord.Request.ALERTNO.SOLVE", alertno->solve);

	return 0;
}

static int set_alertno_response_info(sw_loc_vars_t *vars, alertno_t *alertno, int i)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ALERTNOS.ALERTNO(%d).ERRNO", i);
	loc_set_zd_data(vars, path, alertno->errorno);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ALERTNOS.ALERTNO(%d).LEVEL", i);
	loc_set_zd_data(vars, path, alertno->level);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ALERTNOS.ALERTNO(%d).CNLEVEL", i);
	loc_set_zd_data(vars, path, agt_level_name(alertno->level));

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ALERTNOS.ALERTNO(%d).DESC", i);
	loc_set_zd_data(vars, path, alertno->desc);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ALERTNOS.ALERTNO(%d).REASON", i);
	loc_set_zd_data(vars, path, alertno->reason);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.ALERTNOS.ALERTNO(%d).SOLVE", i);
	loc_set_zd_data(vars, path, alertno->solve);

	return 0;
}

int sp2062(sw_loc_vars_t *vars)
{
	int	i = 0;
	int	no = 0;
	char	opt[8];
	char	reply[16];
	char	buf[512];
	char	res_msg[128];
	char	xmlname[128];
	alertno_t	alertno;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(reply, 0x00, sizeof(reply));
	memset(res_msg, 0x00, sizeof(res_msg));

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/alert/opr_errno.xml", getenv("SWWORK"));
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
		
		node = pub_xml_addnode(xml->root, "ERROR", "", SW_NODE_ROOT);
		if (node == NULL)
		{
			pub_log_error("[%s][%d]add child node error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			strcpy(reply, "E025");
			goto ErrExit;
		}
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
	pub_log_info("[%s][%d]操作标识[%s]", __FILE__, __LINE__, opt);

	if (opt[0] == 'S')
	{
		node = pub_xml_locnode(xml, ".ERROR.ITEM");
		i = 0;
		while(node != NULL)
		{
			node1 = node->firstchild;
			memset(&alertno, 0x0, sizeof(alertno));	
			while(node1 != NULL)
			{
				if (strcmp(node1->name, "ERRNO") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(alertno.errorno, node1->value, sizeof(alertno.errorno));
				}
				else if (strcmp(node1->name, "LEVEL") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(alertno.level, node1->value, sizeof(alertno.level));				
				}
				else if (strcmp(node1->name, "DESC") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(alertno.desc, node1->value, sizeof(alertno.desc));				
				}
				else if (strcmp(node1->name, "REASON") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(alertno.reason, node1->value, sizeof(alertno.reason));
				}
				else if (strcmp(node1->name, "SOLVE") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(alertno.solve, node1->value, sizeof(alertno.solve));
				}

				node1 = node1->next;
			}
			set_alertno_response_info(vars, &alertno, i);		
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
		memset(&alertno, 0x0, sizeof(alertno));
		get_alertno_request_info(vars, &alertno);
		node = agt_xml_search(xml, ".ERROR.ITEM", "ERRNO", alertno.errorno);
		if (node == NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			pub_log_error("[%s][%d]预警编号 %s 不存在！", __FILE__, __LINE__, alertno.errorno);
			sprintf(res_msg, "预警编号 %s 不存在！", alertno.errorno);
			strcpy(reply, "E999");
			pub_xml_deltree(xml);
			goto ErrExit;
		}

		node1 = node->firstchild;

		while(node1 != NULL)
		{
			if (strcmp(node1->name, "LEVEL") == 0)
			{
				pub_xml_set_value(node1, alertno.level);
			}
			else if (strcmp(node1->name, "DESC") == 0)
			{
				pub_xml_set_value(node1, alertno.desc);
			}
			else if (strcmp(node1->name, "REASON") == 0)
			{
				pub_xml_set_value(node1, alertno.reason);
			}
			else if (strcmp(node1->name, "SOLVE") == 0)
			{
				pub_xml_set_value(node1, alertno.solve);
			}
			node1 = node1->next;
		}

		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);
		goto OkExit;
	}
	else if (opt[0] == 'A')
	{
		memset(&alertno, 0x0, sizeof(alertno));
		get_alertno_request_info(vars, &alertno);

		no = 0;
		node = pub_xml_locnode(xml, ".ERROR.ITEM");
		while(node != NULL)
		{
			xml->current = node;
			node1 = pub_xml_locnode(xml, "ERRNO");
			if (node1 != NULL && node1->value != NULL)
			{
				if (no <= atoi(node1->value))
				{
					no = atoi(node1->value) + 1;
				}
			}
			node = node->next;
		}
		
		memset(alertno.errorno, 0x0, sizeof(alertno.errorno));
		sprintf(alertno.errorno, "%d", no);	
		node = pub_xml_locnode(xml, ".ERROR");
		if (node == NULL)
		{
			pub_log_error("[%s][%d] pub_xml_locnode failed!", __FILE__, __LINE__);
			strcpy(reply, "E026");
			pub_xml_deltree(xml);
			goto ErrExit;
		}

		node1 = pub_xml_addnode(node, "ITEM", "", 1);
		pub_xml_addnode(node1, "ERRNO", alertno.errorno, 0);
		pub_xml_addnode(node1, "LEVEL", alertno.level, 0);
		pub_xml_addnode(node1, "DESC", alertno.desc, 1);
		pub_xml_addnode(node1, "REASON", alertno.reason, 1);
		pub_xml_addnode(node1, "SOLVE", alertno.solve, 1);

		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);
		goto OkExit;
	}
	else if (opt[0] == 'D')
	{
		memset(&alertno, 0x0, sizeof(alertno));
		get_alertno_request_info(vars, &alertno);
		node = agt_xml_search(xml, ".ERROR.ITEM", "ERRNO", alertno.errorno);
		if (node == NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			sprintf(res_msg, "预警编号 %s 不存在！", alertno.errorno);
			pub_log_error("[%s][%d]预警编号 %s 不存在！", __FILE__, __LINE__, alertno.errorno);
			strcpy(reply, "E999");
			pub_xml_deltree(xml);
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
