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
#include "pub_vars.h"

int main(int argc, char* argv[])
{
	sw_xmltree_t	*config = NULL;
	sw_xmltree_t	*xml_config = NULL;
	sw_xmltree_t	*type_config = NULL;
	sw_char_t	path[256] = {"accsys.xml"};
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
	
	loc_set_zd_data(&comm_vars, "$product", "I_AGENT");
	loc_set_zd_data(&comm_vars, "#txcode", "1002");

	loc_set_zd_data(&comm_vars, "#chnl_no", "AS");
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
	loc_set_zd_data(&comm_vars, "#sys4", "4");
	loc_set_zd_data(&comm_vars, "#sys5", "5");
	loc_set_zd_data(&comm_vars, "#sys6", "6");
	loc_set_zd_data(&comm_vars, "#sys7", "7");
	loc_set_zd_data(&comm_vars, "#sys8", "8");
	loc_set_zd_data(&comm_vars, "#sys9", "9");
	loc_set_zd_data(&comm_vars, "#sys10", "a");
	loc_set_zd_data(&comm_vars, "#sys11", "b");
	loc_set_zd_data(&comm_vars, "#sys12", "c");
	/***
	loc_set_zd_data(&comm_vars, "#sys12", "0000");
	***/
	/***
	loc_set_zd_data(&comm_vars, "#0251", "5555");
	***/
	loc_set_zd_data(&comm_vars, "#0252", "252525252525ma");
	loc_set_zd_data(&comm_vars, "#0253", "33");
	loc_set_zd_data(&comm_vars, "#0254", "maweiwei");
	loc_set_zd_data(&comm_vars, "#sys25", "aaaaaaaaaaaaa");
	
	loc_set_zd_data(&comm_vars, "#sys16", "1001");
	loc_set_zd_data(&comm_vars, "#sys127", "7");
	loc_set_zd_data(&comm_vars, "#sys108", "12888888");
	loc_set_zd_data(&comm_vars, "#sys110", "12888888");
	loc_set_zd_data(&comm_vars, "#sys126", "12888888");
	loc_set_zd_data(&comm_vars, "#sys128", "12888888");
	loc_set_zd_data(&comm_vars, "#sys37", "37");
	loc_set_zd_data(&comm_vars, "#sys38", "38");
	loc_set_zd_data(&comm_vars, "#sys44", "44");
	loc_set_zd_data(&comm_vars, "#sys45", "45");
	loc_set_zd_data(&comm_vars, "#sys118", "118硚珅");
	loc_set_zd_data(&comm_vars, "#sys119", "119硚");
	loc_set_zd_data(&comm_vars, "#sys120", "120硚");
	loc_set_zd_data(&comm_vars, "#sys121", "1和珅");
	loc_set_zd_data(&comm_vars, "#sys122", "2和珅");
	loc_set_zd_data(&comm_vars, "#sys123", "3和珅");
	loc_set_zd_data(&comm_vars, "#sys124", "4和珅");
	loc_set_zd_data(&comm_vars, "#sys125", "5和珅珅");
	
	/*test BCD code*/
	loc_set_zd_data(&comm_vars, "#sys9", "ABCDEF0123456789");

	/*test HUA lencode*/
	loc_set_zd_data(&comm_vars, "#sys15", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
	pub_log_info("%s, step%d",__FILE__,__LINE__);

	/*test CHECK*/
	loc_set_zd_data(&comm_vars, "#sys18", "1001");
	loc_set_zd_data(&comm_vars, "#sys19", "1002");

#if 0	
	memset(value, 0x0, sizeof(value));
	memset(value, '5', 4090);
	loc_set_zd_data(&comm_vars, "#sys50", value);

	memset(value, 0x0, sizeof(value));
	memset(value, '6', 4090);
	loc_set_zd_data(&comm_vars, "#sys60", value);

	memset(value, 0x0, sizeof(value));
	memset(value, '7', 4090);
	loc_set_zd_data(&comm_vars, "#sys70", value);

	memset(value, 0x0, sizeof(value));
	memset(value, '8', 2100);
	loc_set_zd_data(&comm_vars, "#sys80", value);

	memset(value, 0x0, sizeof(value));
	memset(value, '9', 3000);
	loc_set_zd_data(&comm_vars, "#sys90", value);
#endif	
	loc_set_zd_data(&comm_vars, "$respcd", "999a");

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
	
#define VAR_TEST1(NAME)	pub_mem_memzero(name, sizeof(name));\
	pub_mem_memzero(value, sizeof(value));\
	sprintf(name, NAME);\
	loc_get_zd_data(&comm_vars, name, value);\
	pub_log_info("%s, %d, %s[%s]",__FILE__,__LINE__,name, value);

#if 0
	VAR_TEST1("#chnl_no")
	VAR_TEST1("#tel")
	VAR_TEST1("#sys_trace_no")
	VAR_TEST1("#sys_date")
	VAR_TEST1("#pkgtype")
	VAR_TEST1("#tx_flow_ind")
	VAR_TEST1("#wd_ind")
	VAR_TEST1("#tn_ind")
	VAR_TEST1("#sys_addr")
	VAR_TEST1("#result")
	VAR_TEST1("#file_snd")
	VAR_TEST1("#tx_br_no")
	VAR_TEST1("#sys_id")
	VAR_TEST1("#sub_sys_id")
	VAR_TEST1("#sys_map")
	VAR_TEST1("#sys12")
	VAR_TEST1("#sys16")

	/*test BCD code*/
	VAR_TEST1("#sys9")

	/*test HUA lencode*/
	VAR_TEST1("#sys15")

	/*test CHECK*/
	VAR_TEST1("#sys18")
	VAR_TEST1("#sys19")

	xml_config = pub_xml_crtree(xml_path);
	if (xml_config == NULL)
	{
		pub_log_error("%s, %d, create xml tree[%s] error."
						,__FILE__,__LINE__,xml_path);
		goto RELEASE_EXIT;
	}

	type_config = pub_xml_crtree(type_path);
	if (type_config == NULL)
	{
		pub_log_error("%s, %d, create xml tree[%s] error."
						,__FILE__,__LINE__,type_path);
		goto RELEASE_EXIT;
	}	

	pub_buf_clear(&pkg_buf);

	/*clear vars*/
	comm_vars.destroy(&comm_vars);
	comm_vars.free_mem(&comm_vars);
	comm_vars.alloc_mem(&comm_vars);
	result = comm_vars.create(&comm_vars, vid);
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, vars init error.",__FILE__,__LINE__);
		goto RELEASE_EXIT;
	}
	
	/*Test xml*/
	pub_log_info("\n*********************************TEST CASE2***********************************\n");
	
	loc_vars_clear(&comm_vars);
	loc_set_zd_data(&comm_vars, "#pkgtype", "xm2");
	loc_set_zd_data(&comm_vars, ".root.name", "wangkun");
	loc_set_zd_data(&comm_vars, ".root.passwd", "123456");
	loc_set_zd_data(&comm_vars, ".root.xingbie", "男");
	loc_set_zd_data(&comm_vars, ".root.addr", "西安");
	loc_set_zd_data(&comm_vars, ".root.zhiye", "程序员");

	pub_log_info("%s, step%d",__FILE__,__LINE__);
	pub_buf_init(&pkg_buf);
	/*integrate package*/
	pkg_buf.len = pkg_out(&comm_vars, &pkg_buf, type_config, &cache_list);
	if (pkg_buf.len <= 0)
	{
		pub_log_error("%s, %d, pkg_out error.", __FILE__, __LINE__);
		goto RELEASE_EXIT;
	}
	pub_log_info("%s, step%d",__FILE__,__LINE__);	

	loc_vars_clear(&comm_vars);
	loc_extvars_clear(&comm_vars);

	/*clear vars*/
	comm_vars.destroy(&comm_vars);
	comm_vars.free_mem(&comm_vars);
	comm_vars.alloc_mem(&comm_vars);
	result = comm_vars.create(&comm_vars, vid);
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, vars init error.",__FILE__,__LINE__);
		goto RELEASE_EXIT;
	}

	/*Analyze package use new */
	result = pkg_in(&comm_vars, &pkg_buf, type_config, &cache_list);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_in error."
				, __FILE__, __LINE__);
		goto RELEASE_EXIT;
	}
	
	VAR_TEST1(".root.name")
	VAR_TEST1(".root.passwd")
	VAR_TEST1(".root.xingbie")
	VAR_TEST1(".root.addr")
	VAR_TEST1(".root.zhiye")
#endif

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

