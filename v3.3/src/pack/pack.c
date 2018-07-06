#include "pack.h"

sw_vars_clear_package_t g_vars_clear_packages[MAX_CLEAR_PACKAGE_NUM];

sw_int_t pkg_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *config, sw_pkg_cache_list_t *list, sw_char_t *prdt_name)
{
	int	index = 0;
	sw_int_t	ret = 0;
	sw_pkg_out_pt	pack_out;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	node = pub_xml_locnode(config, ".CBM.TYPE_ALYZ");
	if (node == NULL)
	{
		node = pub_xml_locnode(config, ".CBM.PKGTYPE");
		if (node == NULL)
		{
			pack_out = pkg_com_out;
			pub_log_info("[%s][%d] COM PKGTYPE!", __FILE__, __LINE__);
		}
		else if (strcmp(node->value, "XML") == 0)
		{
			pack_out = pkg_xml_out;
			pub_log_info("[%s][%d] XML PKGTYPE!", __FILE__, __LINE__);
		}
		else if (strcmp(node->value, "8583") == 0)
		{
			pack_out = pkg_8583_out;
			pub_log_info("[%s][%d] 8583 PKGTYPE!", __FILE__, __LINE__);
		}
		else
		{
			pack_out = pkg_com_out;
			pub_log_info("[%s][%d] COM PKGTYPE!", __FILE__, __LINE__);
		}
		xml = config;
	}
	else
	{
		ret = pkg_get_processor_info_out(config, list);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Get processor info error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		index = pkg_get_processor_index_out(vars, list);
		if (index == -1)
		{
			pub_log_error("[%s][%d] Get processor index error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] name=[%s]", __FILE__, __LINE__, list->processor[index].name);
		
		pack_out = (sw_pkg_out_pt)list->processor[index].dlfun;
		xml = list->processor[index].xml;
	}
	return pack_out(vars, pkg_buf, (sw_char_t *)xml, 1);
}

sw_int_t pkg_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *config, sw_pkg_cache_list_t *list, sw_char_t *prdt_name)
{
	int	index = 0;
	sw_int_t	ret = 0;
	sw_pkg_in_pt	pack_in;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	node = pub_xml_locnode(config, ".CBM.TYPE_ALYZ");
	if (node == NULL)
	{
		node = pub_xml_locnode(config, ".CBM.PKGTYPE");
		if (node == NULL)
		{
			pack_in = pkg_com_in;
			pub_log_info("[%s][%d] COM PKGTYPE!", __FILE__, __LINE__);
		}
		else if (strcmp(node->value, "XML") == 0)
		{
			pack_in = pkg_xml_in;
			pub_log_info("[%s][%d] XML PKGTYPE!", __FILE__, __LINE__);
		}
		else if (strcmp(node->value, "8583") == 0)
		{
			pack_in = pkg_8583_in;
			pub_log_info("[%s][%d] 8583 PKGTYPE!", __FILE__, __LINE__);
		}
		else
		{
			pack_in = pkg_com_in;
			pub_log_info("[%s][%d] COM PKGTYPE!", __FILE__, __LINE__);
		}
		xml = config;
	}
	else
	{
		ret = pkg_get_processor_info_in(config, list);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Get processor info error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		index = pkg_get_processor_index_in(vars, pkg_buf->data, pkg_buf->len, list);
		if (index == -1)
		{
			pub_log_error("[%s][%d] Get processor index error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		pack_in = (sw_pkg_in_pt)list->processor[index].dlfun;
		xml = list->processor[index].xml;
	}
	
	memset(&g_vars_clear_packages, 0x0, sizeof(g_vars_clear_packages));
	return pack_in(vars, pkg_buf->data, (sw_char_t *)xml, pkg_buf->len, 1);
}
