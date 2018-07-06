#include "pub_vars.h"
#include "pub_log.h"
#include "pub_cfg.h"
#include "run.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = 0;
	sw_vars_cfg_t 	params;
	sw_loc_vars_t	vars;
	sw_char_t	name[256];
	sw_char_t	value[256];
	sw_char_t	value1[256];
	sw_buf_t	sw_buf;
	sw_int32_t	vid = 1;
	sw_int32_t	len = -1;
	sw_char_t	*tmp = NULL;
	sw_syscfg_t	*syscfg = NULL;
	sw_char_t	*shm_cfg = NULL;

	pub_mem_memzero(&params, sizeof(sw_vars_cfg_t));
	
	params.vars_shm_key = 0x2222222;
	params.vars_sem_key = 0x1123233;
	params.vars_max_num = 100;
	params.vars_max_size = 0;

	pub_mem_memzero(&vars, sizeof(vars));

	/* Create shm_vars 
    	result = vars_creat((const sw_vars_cfg_t *)&params);
    	if(0 != result)
   	{
        	pub_log_error("[%s][%d] Create vars failed!\n", __FILE__, __LINE__);
        	return SW_ERROR;
    	}*/

	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}


	/*get global cfg*/
	shm_cfg = run_get_syscfg();
	if (shm_cfg == NULL)
	{
		pub_log_error("%s, %d, run_get_syscfg error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	syscfg = BLOCKAT(shm_cfg, ((sw_cfgshm_hder_t*)shm_cfg)->sys_offset);
	
    	result = vars_shm_init(syscfg);
    	if (result == -1)
    	{
		pub_log_error("%s, %d, vars_shm_init error.",__FILE__,__LINE__);
		return SW_ERROR;
    	}

	/*allcate local vars pool object.*/
	result = pub_loc_vars_alloc(&vars, HEAP_VARS);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_loc_vars_alloc error."
							,__FILE__, __LINE__);
		return SW_ERROR;
	}

	/*create vars*/
	result = vars.create(&vars, vid);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, vars->create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	/*Set var*/
	vars.set_string(&vars, "#name", "wangkun");
	vars.set_string(&vars, "#passwd", "12334");
	loc_var_remove(&vars, "#name");
	
	/*Get var*/
	pub_mem_memzero(value, sizeof(value));
	vars.get_var(&vars, "#name", value);
	pub_mem_memzero(value1, sizeof(value1));
	vars.get_var(&vars, "#passwd", value1);
	pub_log_info("%s, %d, #name[%s] #passwd[%s]",__FILE__,__LINE__, value, value1);

	/*Set a int*/
	sw_int64_t	k = 10;
	sw_int64_t	j = 0;
	
	vars.set_var(&vars, "#int", 'b', (sw_char_t*)&k, sizeof(sw_int64_t));

	result = vars.get_var(&vars, "#int", &j);
	if (result < 0)
	{
		pub_log_error("vars.get_var error.");
		return SW_ERROR;
	}
	
	pub_log_info("%s, %d, #int[%d]", __FILE__, __LINE__, j);

	/*test set var*/
	pub_mem_memzero(&sw_buf, sizeof(sw_buf));
	result = pub_buf_init(&sw_buf);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_buf_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	vars.set_string(&vars, "#name", "wangkun");
	vars.set_string(&vars, "#passwd", "12334");
	loc_set_zd_data(&vars, ".root.name", "wk");
	loc_set_zd_data(&vars, ".root.passwd", "666666");
	loc_set_zd_data(&vars, ".root.node(0)","value0");
	loc_set_zd_data(&vars, ".root.node(1)","value1");

	/*test serialize*/
	result = vars.serialize(&vars, &sw_buf);
	if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, serialize error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	sw_char_t	value0[128];
	sw_char_t	buf[1024];
	
	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.node(0)",value0);
	loc_get_zd_data(&vars, ".root.node(1)",value1);

	pub_log_info("%s, %d, .root.node(0)[%s] .root.node(1)[%s]"
			,__FILE__,__LINE__,value0,value1);

	pub_mem_memzero(buf, sizeof(buf));

	result = pub_xml_pack_ext(vars.tree, buf);
	if (result == -1)
	{
		pub_log_error("%s, %d, pub_xml_pack error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, xml[%s]"
			,__FILE__,__LINE__,buf);

	sw_buf.data[sw_buf.len] = '\0';
	pub_log_info("%s, %d, serialize buf[%s]", __FILE__, __LINE__, sw_buf.data);

	/*test unserialize*/
	vars.free_mem(&vars);
	vars.alloc_mem(&vars);
	
	result = vars.unserialize(&vars, sw_buf.data);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d vars.unserialize error.", __FILE__,__LINE__);
		return SW_ERROR;
	}
	
	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, "#name", value);
	pub_log_info("%s, %d, #name[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, "#passwd", value);
	pub_log_info("%s, %d, #passwd[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.name", value);
	pub_log_info("%s, %d, .root.name[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.passwd", value);
	pub_log_info("%s, %d, .root.passwd[%s]",__FILE__,__LINE__, value);

	loc_set_zd_data(&vars, ".root.zhiye", "程序员");
	loc_set_zd_data(&vars, ".root.dizhi", "西安");
	
	pub_buf_clear(&sw_buf);
	pub_buf_init(&sw_buf);

	/*serialize again*/
	result = vars.serialize(&vars, &sw_buf);
	if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, serialize error.",__FILE__, __LINE__);
		return SW_ERROR;
	}	

	/*free memory*/
	vars.free_mem(&vars);
	vars.alloc_mem(&vars);

	/*unserialize again*/
	result = vars.unserialize(&vars, sw_buf.data);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d vars.unserialize error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, "#name", value);
	pub_log_info("%s, %d, #name[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, "#passwd", value);
	pub_log_info("%s, %d, #passwd[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.name", value);
	pub_log_info("%s, %d, .root.name[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.passwd", value);
	pub_log_info("%s, %d, .root.passwd[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.zhiye", value);
	pub_log_info("%s, %d, .root.zhiye[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.dizhi", value);
	pub_log_info("%s, %d, .root.dizhi[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	tmp = vars.get_var_addr(&vars, "#name", &len);
	pub_log_info("%s, %d, get_value_addr: #name[%s] len[%d]"
			,__FILE__,__LINE__, tmp, len);

	len = 1024;
	tmp = vars.get_null(&vars, "#block", 1024);
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, get null for block error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	sprintf(tmp, "hello world!");

	pub_mem_memzero(value, sizeof(value));
	tmp = vars.get_var_addr(&vars, "#block", &len);
	pub_log_info("%s, %d, get_value_addr: #block[%s] len[%d]"
			,__FILE__,__LINE__, tmp, len);
	
	pub_buf_clear(&sw_buf);

	/*Init global vars pool*/
	pub_vars_alloc(SHM_VARS);
	vid = 2;
	pub_vars_create(vid);

	/*set string value*/
	pub_vars_set_string("#global", "got");

	pub_mem_memzero(value, sizeof(value));

	pub_vars_get_variable("#global", value);

	pub_log_info("%s, %d, #global[%s]", __FILE__, __LINE__, value);

	sw_int16_t	m = 55;
	sw_int16_t	n = 0;

	pub_vars_set_variable("#bin", 'b', &m, sizeof(m));

	pub_vars_get_variable("#bin", &n);

	pub_log_info("%s, %d, #bin[%d]", __FILE__, __LINE__, n);

	/*Set XML var*/
	loc_set_zd_data(&vars, ".root.name", "wk");
	loc_set_xml_attr(&vars, ".root", "passwd", "123445667788");
	/*clear vars
	vars.clear_xml_vars(&vars);*/

	/*Get xml var*/
	pub_mem_memzero(value, sizeof(value));
	loc_get_zd_data(&vars, ".root.name", value);

	pub_mem_memzero(value1, sizeof(value1));
	loc_get_xml_attr(&vars, ".root", "passwd", value1);
	pub_log_info("%s, %d, .root.name[%s] .root.passwd[%s]"
						,__FILE__,__LINE__,value,value1);

	/*Destory global vars pool.*/
	pub_vars_free();
	
	/*Destroy local vars pool.*/
	result = pub_loc_vars_free(&vars);
	if (result != SW_OK)
	{
		pub_log_error("pub_loc_vars_alloc error.");
		return SW_ERROR;
	}
	
	/*Release shm_vars*/
	vars_destory();

	return 0;
}

