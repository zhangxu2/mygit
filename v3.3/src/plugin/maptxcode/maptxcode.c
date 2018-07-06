#include "pub_log.h"
#include "pub_xml.h"
#include "pub_type.h"
#include "pub_vars.h"

static int check_include(char *var, char *check_var)
{
	char	*p = NULL;
	char	*ptr = NULL;
	char	value[512];
	
	memset(value, 0x0, sizeof(value));
	p = value;
	ptr = check_var;
	while (*ptr != '\0')
	{
		if (*ptr != ' ')
		{
			*p++ = *ptr++;
			continue;
		}
		
		*p = '\0';
		if (strcmp(value, var) == 0)
		{
			return 0;
		}
		ptr++;
		memset(value, 0x0, sizeof(value));
		p = value;
	}
	if (strcmp(value, var) == 0)
	{
		return 0;
	}
	
	return -1;
}

int mapcode(sw_loc_vars_t *vars, char *prdt)
{
	int	ret = 0;
	char	name[32];
	char	value[128];
	char	incode[32];
	char	extcode[32];
	char	flowcode[32];
	char	xmlname[128];
	char	check_value[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	memset(incode, 0x0, sizeof(incode));
	memset(extcode, 0x0, sizeof(extcode));
	memset(flowcode, 0x0, sizeof(flowcode));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(check_value, 0x0, sizeof(check_value));
	
	loc_get_zd_data(vars, "#txcode", extcode);
	pub_log_info("[%s][%d] extcode=[%s]", __FILE__, __LINE__, extcode);
	if (extcode[0] == '\0')
	{
		pub_log_info("[%s][%d] 未配置交易码,不进行交易码转换!", __FILE__, __LINE__);
		return 0;
	}
	
	sprintf(xmlname, "%s/products/%s/etc/maptxcode.xml", getenv("SWWORK"), prdt);
	ret = access(xmlname, F_OK);
	if (ret < 0)
	{
		pub_log_info("[%s][%d] 交易码转换文件不存在,不进行映射!", __FILE__, __LINE__);
		return 0;
	}

	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return -1;
	}
	
	node = pub_xml_locnode(xml, ".MAP.ENTRY");
	while (node != NULL)
	{
		if (strcmp(node->name, "ENTRY") != 0)
		{
			node = node->next;
			continue;
		}
		
		node_bak = node;	
		xml->current = node;
		node1 = pub_xml_locnode(xml, "EXTCODE");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config EXTCODE!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
	
		if (strcmp(node1->value, extcode) != 0)
		{
			node = node->next;
			continue;
		}

		node1 = pub_xml_locnode(xml, "CHECK");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "CHECK") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			memset(name, 0x0, sizeof(name));
			memset(value, 0x0, sizeof(value));
			memset(check_value, 0x0, sizeof(check_value));
			xml->current = node1;
			node2 = pub_xml_locnode(xml, "NAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_error("[%s][%d] Check not config NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return -1;
			}
			strncpy(name, node2->value, sizeof(name) - 1);
			
			node2 = pub_xml_locnode(xml, "VALUE");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_error("[%s][%d] Check not config VALUE!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return -1;
			}
			strncpy(check_value, node2->value, sizeof(check_value) - 1);
			
			loc_get_zd_data(vars, name, value);
			pub_log_info("[%s][%d] [%s]=[%s] CHECK VALUE=[%s]", __FILE__, __LINE__, name, value, check_value);
			if (check_include(value, check_value) == 0)
			{
				node1 = node1->next;
				continue;
			}
			break;
		}
		if (node1 == NULL)
		{
			break;
		}
		
		node = node->next;
	}
	if (node != NULL)
	{
		xml->current = node_bak;
		node = pub_xml_locnode(xml, "INTCODE");
		if (node == NULL || node->value == NULL)
		{
			pub_log_error("[%s][%d] Not config INTCODE!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		strncpy(incode, node->value, sizeof(incode) - 1);
		loc_set_zd_data(vars, "#intxcode", incode);
		
		node = pub_xml_locnode(xml, "FLOWCODE");
		if (node != NULL && node->value != NULL)
		{
			strncpy(flowcode, node->value, sizeof(flowcode) - 1);
			loc_set_zd_data(vars, "$flowcode", flowcode);
		}
		else
		{
			pub_log_info("[%s][%d] [%s]未配置FLOWCODE", __FILE__, __LINE__);
		}
		pub_log_info("[%s][%d] extcode=[%s] incode=[%s] flowcode=[%s]", __FILE__, __LINE__, extcode, incode, flowcode);
	}
	else
	{
		pub_log_info("[%s][%d] 没有找到交易码[%s]的转换信息,赞不进行处理!", __FILE__, __LINE__, extcode);
	}
	pub_xml_deltree(xml);
	
	pub_log_info("[%s][%d] 交易码转换完成!", __FILE__, __LINE__);
	
	return 0;
}

int maptxcode(sw_loc_vars_t *vars, char *prdt, int flag)
{
	if (flag == SW_ENC)
	{
		return 0;
	}
	
	return mapcode(vars, prdt);
}
