/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-21
 *** module  : test package analyze/integrate
 *** name    : pub_vars.c 
 *** function: abstract vars pool's interface, support SHM, HEAP vars pool.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "pack.h"
#include "pub_xml.h"
#include "variable.h"
#include "pub_vars.h"
#include "dfis_var.h"

int main(int argc, char* argv[])
{
	sw_xmltree_t	*config = NULL;
	sw_xmltree_t	*xml_config = NULL;
	sw_xmltree_t	*type_config = NULL;
	sw_char_t	path[256] = {"tag.xml"};
	sw_loc_vars_t	vars;
	sw_int_t	result = SW_ERROR;
	sw_char_t	buffer[2048];
	sw_int32_t	len = 0;
	sw_buf_t	pkg_buf;
	sw_pkg_cache_list_t cache_list;
	sw_char_t	name[128];
	sw_vars_cfg_t	vars_cfg;
	sw_int16_t	shm_creat_flag = 0;
	sw_int16_t	vars_creat_flag = 0;
	sw_int32_t	vid = 0;
	sw_char_t	value[1024 * 5];
	
	pub_mem_memzero(&vars_cfg, sizeof(vars_cfg));
	pub_mem_memzero(&cache_list, sizeof(cache_list));
	memset(value, 0x0, sizeof(value));

	vars_cfg.vars_shm_key = 0x2222222;
	vars_cfg.vars_sem_key = 0x1123233;
	vars_cfg.vars_max_num = 100;
	vars_cfg.vars_max_size = 0;

	pub_mem_memzero(&vars, sizeof(vars));

	pub_log_info("\n*********************************TEST CASE1***********************************\n");

	/* Create shm_vars */
    	result = vars_creat((const sw_vars_cfg_t *)&vars_cfg);
    	if(0 != result)
   	{
        	pub_log_error("[%s][%d] Create vars failed!\n", __FILE__, __LINE__);
        	goto RELEASE_EXIT;
    	}
    	
	shm_creat_flag = 1;

	/*allocate memory for vars object.*/
	result = pub_loc_vars_alloc(&vars, SHM_VARS);
	if (result != SW_OK)
	{
		
		pub_log_error("%s, %d, pub_loc_vars_alloc error."
				, __FILE__, __LINE__);
		goto RELEASE_EXIT;
	}

	/*create vars*/
	result = vars.create(&vars, vid);
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, vars init error.",__FILE__,__LINE__);
		goto RELEASE_EXIT;
	}

	vars_creat_flag = 1;
	
	config = pub_xml_crtree(path);
	if (config == NULL)
	{
		pub_log_error("%s, %d, create tree[%s]."
				, __FILE__, __LINE__, path);
		goto RELEASE_EXIT;
	}
	
	loc_set_zd_data(&vars, "#mesgid", "66666");
	loc_set_zd_data(&vars, "#mesgreqno", "88888888");
	loc_set_zd_data(&vars, "#traceno", "00001234");
	loc_set_zd_data(&vars, "#workdate", "20140401");
	loc_set_zd_data(&vars, "#sndbrno", "313515082905");
	loc_set_zd_data(&vars, "#detcnt", "0003");
	loc_set_zd_data(&vars, "#detail", "0123456789abcdefghijklmnopqrstuvwxyz");
	
	loc_set_zd_data(&vars, "$fileflag", "0");
	loc_set_zd_data(&vars, "$filename", "test.txt");
	/*integrate package*/
	pub_buf_init(&pkg_buf);
	pkg_buf.len = pkg_out(&vars, &pkg_buf, config, &cache_list, "cnaps");
	if (pkg_buf.len <= 0)
	{
		pub_log_error("%s, %d, pkg_out error.", __FILE__, __LINE__);
		
		goto RELEASE_EXIT;
	}

	vars.destroy(&vars);
	vars.free_mem(&vars);
	/*create vars*/
	vars.alloc_mem(&vars);
	result = vars.create(&vars, vid);
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, vars init error.",__FILE__,__LINE__);
		goto RELEASE_EXIT;
	}

	/*Analyze package*/
	pub_log_info("[%s][%d] Pack:[%s]", __FILE__, __LINE__, pkg_buf.data);
	result = pkg_in(&vars, &pkg_buf, config, &cache_list, "cnaps");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_in error."
				, __FILE__, __LINE__);
		goto RELEASE_EXIT;
	}
	
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.name", value);
        pub_log_info("[%s][%d] name=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.birth", value);
        pub_log_info("[%s][%d] birth=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.sex", value);
        pub_log_info("[%s][%d] sex=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.tel", value);
        pub_log_info("[%s][%d] tel=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.addr", value);
        pub_log_info("[%s][%d] addr=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.company", value);
        pub_log_info("[%s][%d] company=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, ".root.phone", value);
        pub_log_info("[%s][%d] phone=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&vars, "$dstext", value);
        pub_log_info("[%s][%d] dstext=[%s]", __FILE__, __LINE__, value);
	
	pkg_release_list(&cache_list);
	
RELEASE_EXIT:

	if (config != NULL)
	{
		pub_xml_deltree(config);
		config = NULL;
	}

	if (xml_config != NULL)
	{
		pub_xml_deltree(xml_config);
		xml_config = NULL;
	}

	if (type_config != NULL)
	{
		pub_xml_deltree(type_config);
		type_config = NULL;
	}

	if (vars_creat_flag == 1)
	{
		result = pub_loc_vars_free(&vars);
		if (result != SW_OK)
		{
			pub_log_error("pub_loc_vars_alloc error.");
		}

	}

	if (shm_creat_flag == 1)
	{
		/*Release shm_vars*/
		vars_destory();
	}

   	pub_buf_clear(&pkg_buf);
    
	return SW_OK;
}

