#include "lsn_pub.h"
#include "pub_xml.h"

sw_int_t lsn_pub_svrsvc_map(sw_xmltree_t *xml, sw_loc_vars_t *loc_vars, sw_char_t *svr, sw_char_t *svc, sw_int32_t *plevel)
{
	sw_int32_t	level = 0;
	sw_char_t	name[64];
	sw_char_t	value[128];
	sw_char_t	check_value[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	memset(check_value, 0x0, sizeof(check_value));
	
	if (xml == NULL || svr == NULL || svc == NULL)
	{
		pub_log_error("[%s][%d] Param error! NULL!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".SWPKGCFG.ANALYZE.SERVICEMAP.SVCTARGET");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not config SVCTARGET!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	while (node != NULL)
	{
		if (strcmp(node->name, "SVCTARGET") != 0)
		{
			node = node->next;
			continue;
		}
		node_bak = node;
		xml->current = node;
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
				pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			strncpy(name, node2->value, sizeof(name) - 1);
		
			node2 = pub_xml_locnode(xml, "VALUES");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_error("[%s][%d] Not config VALUES!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			strncpy(check_value, node2->value, sizeof(check_value) - 1);
			
			loc_vars->get_var(loc_vars, name, value);
			pub_str_zipspace(value);
			if (pub_regex_match(value, check_value) == 0)
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
	
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not find SVR/SVC!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	xml->current = node_bak;
	node = pub_xml_locnode(xml, "SERVER");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config SERVER!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(svr, node->value);
	
	node = pub_xml_locnode(xml, "SERVICE");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config SERVICE!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(svc, node->value);
	
	node = pub_xml_locnode(xml, "LEVEL");
	if (node != NULL && node->value != NULL)
	{
		level = atoi(node->value);
		if (level < SVR_LEVEL_DEFAULT || level > SVR_LEVEL_ALL)
		{
			pub_log_error("[%s][%d] LEVEL SHOULD between SVR_LEVEL_DEFAULT and SVR_LEVEL_ALL",
				__FILE__, __LINE__);
			level = SVR_LEVEL_DEFAULT;	
		}
	}
	*plevel = level;
	pub_log_info("[%s][%d] svr=[%s] svc=[%s] level=[%d]", __FILE__, __LINE__, svr, svc, level);

	return SW_OK;
}

