/*************************************************
  文 件 名:  flw_sp2060.c                       **
  功能描述:  预警规则配置                       **
  作    者:  邹佩                               **
  完成日期:  20160802                           **
 *************************************************/
#include "agent_comm.h"

typedef struct business_s
{
	char	exp[256];
	char	msg[256];
	char	alertno[16];
	char	desc[64];
	char	name[64];
}business_t;

static int get_alertno_desc(char *desc, char *alertno)
{
	char	xmlname[256];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/alert/opr_errno.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] xml %s 建树失败", __FILE__, __LINE__, xmlname);
		return -1;
	}

	node = agt_xml_search(xml, ".ERROR.ITEM", "ERRNO", alertno);
	if (node == NULL)
	{
		pub_log_error("[%s][%d] cannot find node ERRNO=%s", alertno);
		pub_xml_deltree(xml);
		return -1;
	}

	xml->current = node;
	node1 = pub_xml_locnode(xml, "DESC");
	if (node1 == NULL || node1->value == NULL)
	{
		pub_log_error("[%s][%d] cannot find node DESC  ERRNO=%s", alertno);
		pub_xml_deltree(xml);
		return -1;
	}
		
	strcpy(desc, node1->value);
	pub_xml_deltree(xml);
	return 0;
}

static int get_business_request_info(sw_loc_vars_t *vars, business_t *business)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.BUSINESS.NAME");
	loc_get_zd_data(vars, path, business->name);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.BUSINESS.DESC");
	loc_get_zd_data(vars, path, business->desc);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.BUSINESS.ALERTNO");
	loc_get_zd_data(vars, path, business->alertno);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.BUSINESS.MSG");
	loc_get_zd_data(vars, path, business->msg);

	return 0;
}

static int set_business_response_info(sw_loc_vars_t *vars, business_t *business, int i)
{
	int  ret = 0;
	char path[256];
	char desc[64];

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.BUSINESSS.BUSINESS(%d).DESC", i);
	loc_set_zd_data(vars, path, business->desc);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.BUSINESSS.BUSINESS(%d).NAME", i);
	loc_set_zd_data(vars, path, business->name);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.BUSINESSS.BUSINESS(%d).ALERTNO", i);
	loc_set_zd_data(vars, path, business->alertno);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.BUSINESSS.BUSINESS(%d).ALERT_DESC", i);
	memset(desc, 0x0, sizeof(desc));
	
	ret = get_alertno_desc(desc, business->alertno);
	if ( ret == 0)
	{
		loc_set_zd_data(vars, path, desc);
	}

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.BUSINESSS.BUSINESS(%d).MSG", i);
	loc_set_zd_data(vars, path, business->msg);

	return 0;
}

static int check_rule(char *buf, char *res_msg)
{
	if (buf == NULL || strlen(buf) == 0)
	{
		pub_log_error("[%s][%d] input param error!", __FILE__, __LINE__);
		return -1;
	}
	
	pub_log_debug("[%s][%d] buf= %s", __FILE__, __LINE__, buf);
	
	char *p = NULL;
	int i = 0;
	int stat = 0;
	int left = 0;
	int right = 0;

	p = strtok(buf, " ");
	while (p != NULL)
	{
		i++;
		if (strcmp(p, "(") == 0 && (stat == 2))
		{
			pub_log_error("[%s][%d]err %s\n", __FILE__, __LINE__, p);
			return i;
		}
		else if (strcmp(p, "(") == 0)
		{
			stat = 1;
			left++;
		}
		else if (strcmp(p, ")") == 0 && (stat == 3 || stat == 1))
		{
			pub_log_error("[%s][%d]err %s\n", __FILE__, __LINE__, p);
			return i;
		}
		else if(strcmp(p, ")") == 0)
		{
			stat = 2;
			right++;
		}
		else if((strcmp(p, "并且") == 0 || strcmp(p, "或者") == 0 || strcmp(p, "非") == 0) && ((stat == 3 || stat == 1 ||stat == -1) || (i == 1 && stat == 2)))
		{
			pub_log_error("[%s][%d]err %s", __FILE__, __LINE__, p);
			return i;
		}
		else if(strcmp(p, "并且") == 0 || strcmp(p, "或者") == 0 || strcmp(p, "非") == 0)
		{
			stat = 3;
		}
		else if ((stat == 0 || stat == 2) && i > 1)
		{
			pub_log_error("[%s][%d]err %s", __FILE__, __LINE__, p);
			return i;
		}
		else
		{
			stat = 0;
		}

		p = strtok(NULL, " ");
	}

	if (left != right)
	{
		if (left - right > 0)
		{
			pub_log_error("[%s][%d] ( more: %d",__FILE__, __LINE__, left - right);
			sprintf(res_msg, "语法错误：( 多 %d", left - right);
		}
		else
		{
			pub_log_error("[%s][%d] ) more: %d",__FILE__, __LINE__, right - left);
			sprintf(res_msg, "语法错误：) 多 %d", right - left);
		}
		return -1;
	}

	if (stat == 0 || stat == 2)
	{
		pub_log_debug("[%s][%d]the stat is ok!", __FILE__, __LINE__);
	}
	else
	{
		pub_log_error("[%s][%d]the stat is [%d]", __FILE__, __LINE__, stat);
		strcpy(res_msg, "语法错误: 逻辑关系符链接错误");
		return i;
	}

	return 0;
}

static int get_g_alert(sw_loc_vars_t *vars)
{
	char	buf[128];
	char	path[256];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path) - 1,  "%s/cfg/alert/alert.xml", getenv("SWWORK"));
	if (!pub_file_exist(path))
	{
		pub_log_error("[%s][%d] alert_file[%s] not exist ", __FILE__, __LINE__, path);
		return -1;
	}
	
	xml = pub_xml_crtree(path);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, path);
		return -1;
	}

	node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.IP");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, node->value, sizeof(buf) - 1);
		loc_set_zd_data(vars, ".TradeRecord.Response.Alert.IP", buf);
	}

	node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.PORT");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, node->value, sizeof(buf)-1);
		loc_set_zd_data(vars, ".TradeRecord.Response.Alert.PORT", buf);
	}

	node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.LEVEL");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, node->value, sizeof(buf) - 1);
		loc_set_zd_data(vars, ".TradeRecord.Response.Alert.LEVEL", buf);
	}

	pub_xml_deltree(xml);
	return 0;
}

int sp2060(sw_loc_vars_t *vars)
{
	int	ret = 0;
	int	page_idx = 0;
	int	page_cnt = 0;
	int	i = 0, j = 0, sum = 0;
	static int	first = 0;
	char	opt[8];
	char	prdt[64];
	char	buf[512];
	char	reply[8];
	char	res_msg[256];
	char	xmlname[128];

	business_t	business;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset(prdt, 0x00, sizeof(prdt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Prdt", prdt);

	if (strlen(prdt) == 0)
	{
		pub_log_error("[%s][%d] there is no products[%s]!", __FILE__, __LINE__, prdt);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/alert_rule.xml", getenv("SWWORK"), prdt);
	if (!pub_file_exist(xmlname))
	{
		pub_log_info("[%s][%d] xml [%s]不存在,开始创建!", __FILE__, __LINE__, xmlname);
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
		node1 = pub_xml_addnode(node, "BUSINESS", "", SW_NODE_ELEMENT);
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

		if (first == 0)
		{
			node = pub_xml_locnode(xml, ".ALERT");
			if (node == NULL)
			{
				pub_log_error("[%s][%d]find root node error!", __FILE__, __LINE__);
				strcpy(reply, "E026");
				goto ErrExit;
			}

			node1 = pub_xml_locnode(xml, ".ALERT.BUSINESS");
			if (node1 == NULL)
			{
				pub_xml_addnode(node, "BUSINESS", "", SW_NODE_ELEMENT);
				first = 1;
			}
		}
	}

	memset(opt, 0x00, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d]操作标识opt[%s]", __FILE__, __LINE__, opt);
	if (opt[0] == 'S')
	{
		ret = get_g_alert(vars);
		if (ret < 0)
		{
			pub_log_error("[%s][%d]  get global alert error", __FILE__, __LINE__);
			strcpy(reply, "E051");
			goto ErrExit;
		}

		pub_mem_memzero(buf, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
		page_idx = atoi(buf);

		pub_mem_memzero(buf, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
		page_cnt = atoi(buf);

		if (page_cnt == 0 || page_idx == 0)
		{
			page_cnt = 1;
		}

		i = j = 0;
		node = pub_xml_locnode(xml, ".ALERT.BUSINESS.ITEM");
		while (node != NULL)
		{
			node1 = node->firstchild;
			i++;
			if (page_cnt != 1 &&  (i <= (page_idx-1) * page_cnt  || i > page_idx * page_cnt))
			{
				node = node->next;
				continue;
			}

			memset(&business, 0x0, sizeof(business));
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "NAME") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(business.name, node1->value, sizeof(business.name)-1);
				}
				else if (strcmp(node1->name, "MSG") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(business.msg, node1->value, sizeof(business.msg)-1);
				}
				else if (strcmp(node1->name, "DESC") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(business.desc, node1->value, sizeof(business.desc)-1);
				}
				else if (strcmp(node1->name, "EXP") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(business.exp, node1->value, sizeof(business.exp)-1);
				}
				else if (strcmp(node1->name, "ALERTNO") == 0 && node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(business.alertno, node1->value, sizeof(business.alertno)-1);
				}

				node1 = node1->next;
			}
			set_business_response_info(vars, &business, j);
			j++;

			node = node->next;
		}

		if (i % page_cnt == 0)
		{
			sum = i / page_cnt;
		}
		else
		{
			sum = i / page_cnt + 1;
		}

		memset(buf, 0x00, sizeof(buf));
		sprintf(buf, "%d", sum);
		loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", buf);               
	}
	else if (opt[0] == 'M')
	{
		memset(&business, 0x0, sizeof(business));
		get_business_request_info(vars, &business);

		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, business.desc, sizeof(buf) - 1);
		ret = check_rule(buf, res_msg);
		if (ret)
		{
			if (strlen(res_msg) == 0)
			{
				strcpy(res_msg, "语法错误!");
			}
			strcpy(reply, "E999");
			goto ErrExit;
		}

		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, business.desc, sizeof(buf) - 1);
		pack_exp(xml, buf, business.exp);

		node = agt_xml_search(xml, ".ALERT.BUSINESS.ITEM", "NAME", business.name);
		if (node == NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			sprintf(res_msg, "预警规则 %s 不存在！", business.name);
			pub_log_error("[%s][%d] res_msg[%s]", __FILE__, __LINE__, res_msg);
			strcpy(reply, "E999");
			goto ErrExit;
		}
		
		pub_log_info("[%s][%d] exp = %s, msg=%s. alertno=%s", __FILE__, __LINE__, business.exp, business.desc, business.alertno);
		node1 = node->firstchild;
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "EXP") == 0)
			{
				pub_xml_set_value(node1, business.exp);
			}
			else if (strcmp(node1->name, "MSG") == 0)
			{
				pub_xml_set_value(node1, business.msg);
			}
			else if (strcmp(node1->name, "DESC") == 0)
			{
				pub_xml_set_value(node1, business.desc);
			}
			else if (strcmp(node1->name, "ALERTNO") == 0)
			{
				pub_xml_set_value(node1, business.alertno);
			}
			node1 = node1->next;
		}
		pub_xml_pack(xml, xmlname);
		goto OkExit;
	}
	else if (opt[0] == 'A')
	{
		memset(&business, 0x0, sizeof(business));
		get_business_request_info(vars, &business);
		node = agt_xml_search(xml, ".ALERT.BUSINESS.ITEM", "NAME", business.name);
		if (node != NULL)
		{
			memset(res_msg, 0x00, sizeof(res_msg));
			sprintf(res_msg, "预警规则 %s 已存在！", business.name);
			pub_log_error("[%s][%d] res_msg[%s]", __FILE__, __LINE__, res_msg);
			strcpy(reply, "E999");
			goto ErrExit;
		}

		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, business.desc, sizeof(buf) - 1);
		ret = check_rule(buf ,res_msg);
		if (ret)
		{
			if (strlen(res_msg) == 0)
			{
				strcpy(res_msg, "语法错误！");
			}
			strcpy(reply, "E999");
			goto ErrExit;
		}

		memset(buf, 0x0, sizeof(buf));
		strncpy(buf, business.desc, sizeof(buf) - 1);
		pack_exp(xml, buf, business.exp);
		
		node = pub_xml_locnode(xml, ".ALERT.BUSINESS");
		if (node == NULL)
		{
			pub_log_error("[%s][%d] pub_xml_locnode failed!", __FILE__, __LINE__);
			node1 = pub_xml_locnode(xml, ".ALERT");
			node = pub_xml_addnode(node1, "BUSINESS", "", 1);
		}

		node1 = pub_xml_addnode(node, "ITEM", "", 1);
		pub_xml_addnode(node1, "MSG", business.msg, 1);
		pub_xml_addnode(node1, "NAME", business.name, 1) ;
		pub_xml_addnode(node1, "EXP", business.exp, 1);
		pub_xml_addnode(node1, "DESC", business.desc, 1);
		pub_xml_addnode(node1, "ALERTNO", business.alertno, 1);
		pub_xml_pack(xml, xmlname);
	}
	else if (opt[0] == 'D')
	{
		memset(&business, 0x0, sizeof(business));
		get_business_request_info(vars, &business);
		node = agt_xml_search(xml, ".ALERT.BUSINESS.ITEM", "NAME", business.name);
		if (node == NULL)
		{
			sprintf(res_msg, "预警规则 %s 不存在！", business.name);
			pub_log_error("[%s][%d] res_msg[%s]", __FILE__, __LINE__, res_msg);
			strcpy(reply, "E999");
			goto ErrExit;
		}

		agt_remove_node(node);
		pub_xml_pack(xml, xmlname);	
	}
	else
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d] 操作标识[%s]error!", __FILE__, __LINE__, opt);
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");
	return 0;

ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
