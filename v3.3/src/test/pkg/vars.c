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

#include "pub_xml.h"
#include "variable.h"
#include "pub_vars.h"
#include "dfis_var.h"

int main(int argc, char* argv[])
{
	sw_loc_vars_t	vars;
	sw_int_t	result = SW_ERROR;
	sw_char_t	buffer[2048];
	sw_int32_t	len = 0;
	sw_buf_t	pkg_buf;
	sw_buf_t	locbuf;
	sw_char_t	name[128];
	sw_char_t	value[128];
	sw_vars_cfg_t	vars_cfg;
	sw_int16_t	shm_creat_flag = 0;
	sw_int16_t	vars_creat_flag = 0;
	sw_int32_t	vid = 0;
	
	pub_mem_memzero(&vars_cfg, sizeof(vars_cfg));

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

	loc_set_zd_data(&vars, "#sys_id", "0001");
	loc_set_zd_data(&vars, "#sub_sys_id", "0002");
	loc_set_zd_data(&vars, "#sys_map", "20202020");
	loc_set_zd_data(&vars, "#sys2", "444444");
	loc_set_zd_data(&vars, "#sys3", "666666");
	
	/*** xml ***/
	loc_set_zd_data(&vars, ".root.name", "maweiwei");
	loc_set_zd_data(&vars, ".root.birth", "19871022");
	loc_set_zd_data(&vars, ".root.sex", "ÄÐ");
	loc_set_zd_data(&vars, ".root.tel", "95568");
	loc_set_zd_data(&vars, ".root.addr", "xi'an");
	loc_set_zd_data(&vars, ".root.company", "DHCC");
	loc_set_zd_data(&vars, "$sw_xml_headflag", "<?xml version=\"1.0\" encoding=\"GBK\" standalone=\"no\" ?>");
	loc_set_zd_data(&vars, "$dsflag", "1");
	loc_set_zd_data(&vars, "$dstext", "test1234567890abcdefghijklmnopqrst");
	
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
	
	loc_extvars_clear(&vars);
	pub_buf_init(&locbuf);
	vars.serialize(&vars, &locbuf);
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] serialize:[%d]", __FILE__, __LINE__, locbuf.len);
	
	memset(locbuf.data, 0x0, locbuf.size);
	sprintf(locbuf.data, "shm%08d", vid);
	vars.unserialize(&vars, &locbuf);
	
	pub_loc_vars_free(&vars);
	vars_destory();


RELEASE_EXIT:
    
	return SW_OK;
}

