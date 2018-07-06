#include "pub_xml.h"
#include "pkg_processor.h"
#include "pkg_type_check.h"
#include "pkg.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	pkg[2048];
	sw_int32_t	len = 0;
	sw_char_t*	base = NULL;
	sw_int32_t	index = 0;
	sw_char_t	tmp[128];
	sw_xmltree_t	*xml_config = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmltree_t	*type_config = NULL;
	sw_xmltree_t	*sep_config = NULL;
	sw_char_t	xml_name[256] = {"login.xml"};
	sw_char_t	type_name[256] = "check_type.xml";
	sw_char_t	sep_name[256] = {"sep.xml"};
	sw_vars_t	vars;
	sw_buf_t	xml_pkg;
	sw_buf_t	sep_pkg;
	sw_buf_t	out_pkg;
	pkg_cache_list_t cache_list;
	sw_pkg_processor_t	processor;

	pub_mem_memzero(&processor, sizeof(processor));
	pkg_processor_init(&processor);

	vLocVarsInit(&vars);
	pub_buf_init(&xml_pkg);
	pub_buf_init(&sep_pkg);
	pub_buf_init(&out_pkg);
	pub_mem_memzero(&cache_list, sizeof(cache_list));

	/*create a xml package */
	index = 0;
	base = xml_pkg.psBuf;
	index += 4;

	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "xml");
	pub_mem_cpymem(base + index, tmp, strlen(tmp));
	index += strlen(tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "<root><name>wangkun</name><passwd>123456</passwd></root>");
	pub_mem_cpymem(base + index, tmp, strlen(tmp));
	index += strlen(tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "%0*d", 4, index);
	pub_mem_cpymem(base, tmp, strlen(tmp));

	xml_pkg.iLen = index;

	/*create a sep package*/
	index = 0;
	base = sep_pkg.psBuf;
	index += 4;
	
	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "sep");
	pub_mem_cpymem(base + index, tmp, strlen(tmp));
	index += strlen(tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "beijing|taian|xian");
	pub_mem_cpymem(base + index, tmp, strlen(tmp));
	index += strlen(tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "%0*d", 4, index);
	pub_mem_cpymem(base, tmp, strlen(tmp));

	sep_pkg.iLen = index;

	xml_config = pub_xml_crtree(xml_name);
	if (xml_config == NULL)
	{
		pub_log_error("%s, %d, create xml tree %s error."
			, __FILE__, __LINE__, xml_name);
		return SW_ERROR;
	}

	sep_config = pub_xml_crtree(sep_name);
	if (sep_config == NULL)
	{
		pub_log_error("%s, %d, create xml tree %s error."
			, __FILE__, __LINE__, sep_name);
		return SW_ERROR;
	}

	/*package analyze
	result = pkg_analyze(&vars, base, xml_config, index, 1);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_analyze %s error."
			, __FILE__, __LINE__, xml_name);
		return SW_ERROR;
	}

	result = pkg_pub_analyze(&vars, &sep_pkg, sep_config,&cache_list);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_pub_analyze %s error."
			, __FILE__, __LINE__, xml_name);
		return SW_ERROR;
	}*/

	/*print vars content.
	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#pkglen", tmp);
	pub_log_info("#pkglen=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#pkgtype", tmp);
	pub_log_info("#pkgtype=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, ".root.name", tmp);
	pub_log_info(".root.name=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, ".root.passwd", tmp);
	pub_log_info(".root.passwd=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#city1", tmp);
	pub_log_info("#city1=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#city2", tmp);
	pub_log_info("#city2=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#city3", tmp);
	pub_log_info("#city3=%s", tmp);*/

	type_config = pub_xml_crtree(type_name);
	if (type_config == NULL)
	{
		pub_log_error("%s, %d, create xml tree %s error."
			,__FILE__, __LINE__, type_name);
		return SW_ERROR;
	}

	node = pub_xml_locnode(type_config, ".CBM.TYPE_ALYZ.ANALYZE.ITEM");
	if (node == NULL)
	{
		pub_log_error("%s, %d Miss .CBM.TYPE_ALYZ.ANALYZE.ITEM."
				,__FILE__, __LINE__);
		return SW_ERROR;
	}

	type_config->current = node;
	
	/*result = pkg_analyze_multi_check(sep_pkg.psBuf, sep_pkg.iLen, type_config, &processor);
	if (result == CHECK_TRUE)
	{
		pub_log_info("check true.");
	}
	else if (result == CHECK_FALSE)
	{
		pub_log_info("check false.");
	}
	else
	{
		pub_log_error("%s, %d, pkg_analyze_multi_check error.",__FILE__,__LINE__);
	}*/

	/*result = pkg_get_analyze_processor(sep_pkg.psBuf, sep_pkg.iLen, type_config, &processor);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_get_analyze_processor error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("processor: name[%s] lib_name[%s] config[%s]"
		,processor.processor_name, processor.lib_name, processor.config_name);*/

	result = pkg_in(&vars, &sep_pkg, type_config, &cache_list);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_in error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#pkglen", tmp);
	pub_log_info("#pkglen=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#pkgtype", tmp);
	pub_log_info("#pkgtype=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, ".root.name", tmp);
	pub_log_info(".root.name=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, ".root.passwd", tmp);
	pub_log_info(".root.passwd=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#city1", tmp);
	pub_log_info("#city1=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#city2", tmp);
	pub_log_info("#city2=%s", tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	loc_get_zd_data(&vars, "#city3", tmp);
	pub_log_info("#city3=%s", tmp);

	node = pub_xml_locnode(type_config, ".CBM.TYPE_ALYZ.INTEGRATE.ITEM");
	if (node == NULL)
	{
		pub_log_error("%s, %d Miss .CBM.TYPE_ALYZ.INTEGRATE.ITEM."
				,__FILE__, __LINE__);
		return SW_ERROR;
	}

	type_config->current = node;

	/*result = pkg_integrate_multi_check(&vars, type_config, &processor);
	if (result == CHECK_TRUE)
	{
		pub_log_info("check true.");
	}
	else if (result == CHECK_FALSE)
	{
		pub_log_info("check false.");
	}
	else
	{
		pub_log_error("%s, %d, pkg_analyze_multi_check error."
				,__FILE__,__LINE__);
	}*/
	/*result = pkg_get_integrate_processor(&vars, type_config, &processor);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_get_integrate_processor error."
			,__FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("processor: name[%s] lib_name[%s] config[%s]"
			,processor.processor_name, processor.lib_name, processor.config_name);*/

	out_pkg.iLen = pkg_out(&vars, &out_pkg, type_config, &cache_list);
	/*out_pkg.iLen = pkg_integrate(&vars, out_pkg.psBuf, xml_config, 1);*/
	if (out_pkg.iLen <= 0)
	{
		pub_log_error("%s, %d, pkg_out error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	out_pkg.psBuf[out_pkg.iLen] = '\0';
	pub_log_info("%s, %d, out_pkg[%s]",__FILE__,__LINE__, out_pkg.psBuf);
	pub_xml_deltree(xml_config);
	pub_xml_deltree(type_config);
	vLocVarsDestroy(&vars);
	pub_buf_clear(&xml_pkg);
	pub_buf_clear(&sep_pkg);
	pkg_cache_list_destroy(&cache_list);

	return SW_OK;
}

