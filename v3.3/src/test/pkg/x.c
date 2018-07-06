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
	sw_char_t	path[256] = {"txml.xml"};
	sw_char_t	xml_path[256] = {"login.xml"};
	sw_char_t	type_path[256] = {"check_type.xml"};
	sw_loc_vars_t	comm_vars;
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

	pub_mem_memzero(&comm_vars, sizeof(comm_vars));

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
	result = pub_loc_vars_alloc(&comm_vars, SHM_VARS);
	if (result != SW_OK)
	{
		
		pub_log_error("%s, %d, pub_loc_vars_alloc error."
				, __FILE__, __LINE__);
		goto RELEASE_EXIT;
	}

	/*create vars*/
	result = comm_vars.create(&comm_vars, vid);
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
	
	loc_set_zd_data(&comm_vars, "#chnl_no", "AA");
	loc_set_zd_data(&comm_vars, "#tel", "009527");
	loc_set_zd_data(&comm_vars, "#sys_trace_no", "123456789");
	loc_set_zd_data(&comm_vars, "#sys_date", "20130525");
	loc_set_zd_data(&comm_vars, "#pkgtype", "a");
	loc_set_zd_data(&comm_vars, "#tx_flow_ind", "1");
	loc_set_zd_data(&comm_vars, "#wd_ind", "1");
	loc_set_zd_data(&comm_vars, "#tn_ind", "1");
	loc_set_zd_data(&comm_vars, "#sys_addr", "DHCC");
	loc_set_zd_data(&comm_vars, "#result", "1");
	loc_set_zd_data(&comm_vars, "#file_snd", "1");
	loc_set_zd_data(&comm_vars, "#tx_br_no", "12121212");
	loc_set_zd_data(&comm_vars, "#sys_id", "0001");
	loc_set_zd_data(&comm_vars, "#sub_sys_id", "0002");
	loc_set_zd_data(&comm_vars, "#sys_map", "20202020");
	loc_set_zd_data(&comm_vars, "#sys2", "444444");
	loc_set_zd_data(&comm_vars, "#sys3", "666666");
	
	loc_set_zd_data(&comm_vars, ".root.name", "maweiwei");
	loc_set_zd_data(&comm_vars, ".root.birth", "19871022");                                                          
	loc_set_zd_data(&comm_vars, ".root.sex", "ÄÐ");                                                                  
	loc_set_zd_data(&comm_vars, ".root.tel", "95568");                                                               
	loc_set_zd_data(&comm_vars, ".root.addr", "xi'an");                                                              
	loc_set_zd_data(&comm_vars, ".root.company", "DHCC");                                                            
	loc_set_zd_data(&comm_vars, "$sw_xml_headflag", "<?xml version=\"1.0\" encoding=\"GBK\" standalone=\"no\" ?>");  
	loc_set_zd_data(&comm_vars, "$dsflag", "1");                                                                     
	loc_set_zd_data(&comm_vars, "$dstext", "test1234567890abcdefghijklmnopqrst");       
	
	/*integrate package*/
	pub_buf_init(&pkg_buf);
	pkg_buf.len = pkg_out(&comm_vars, &pkg_buf, config, &cache_list, "cnaps");
	if (pkg_buf.len <= 0)
	{
		pub_log_error("%s, %d, pkg_out error.", __FILE__, __LINE__);
		
		goto RELEASE_EXIT;
	}

	comm_vars.destroy(&comm_vars);
	comm_vars.free_mem(&comm_vars);
	/*create vars*/
	comm_vars.alloc_mem(&comm_vars);
	result = comm_vars.create(&comm_vars, vid);
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, vars init error.",__FILE__,__LINE__);
		goto RELEASE_EXIT;
	}

	/*Analyze package*/
	pub_log_info("[%s][%d] Pack:[%s]", __FILE__, __LINE__, pkg_buf.data);
	result = pkg_in(&comm_vars, &pkg_buf, config, &cache_list, "cnaps");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_in error."
				, __FILE__, __LINE__);
		goto RELEASE_EXIT;
	}
	
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.name", value);
        pub_log_info("[%s][%d] name=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.birth", value);
        pub_log_info("[%s][%d] birth=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.sex", value);
        pub_log_info("[%s][%d] sex=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.tel", value);
        pub_log_info("[%s][%d] tel=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.addr", value);
        pub_log_info("[%s][%d] addr=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.company", value);
        pub_log_info("[%s][%d] company=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, ".root.phone", value);
        pub_log_info("[%s][%d] phone=[%s]", __FILE__, __LINE__, value);
        memset(value, 0x0, sizeof(value));
        loc_get_zd_data(&comm_vars, "$dstext", value);
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
		result = pub_loc_vars_free(&comm_vars);
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

