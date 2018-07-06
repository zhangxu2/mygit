#include "pub_type.h"
#include "dfis_var.h"

int main (int argc, char* argv[])
{
	sw_int_t	result = 0;
	sw_vars_cfg_t  params;
	sw_loc_vars_t	vars;
	sw_char_t	name[256];
	sw_char_t	value[256];
	sw_char_t	value1[256];
	sw_int32_t	vid = 1;

	pub_mem_memzero(&params, sizeof(sw_vars_cfg_t));
	
	params.vars_shm_key = 0x2222222;
	params.vars_sem_key = 0x1123233;
	params.vars_max_num = 100;
	params.vars_max_size = 0;

	pub_mem_memzero(&vars, sizeof(vars));

	/* Create shm_vars */
    	result = vars_creat((const sw_vars_cfg_t *)&params);
    	if(0 != result)
   	{
        	pub_log_error("[%s][%d] Create vars failed!\n", __FILE__, __LINE__);
        	return SW_ERROR;
    	}

	/*Init global vars pool.*/
	result = pub_vars_alloc(HEAP_VARS);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_alloc error.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	result = pub_vars_create(vid);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_create error.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	/*Set normal var*/
	set_zd_data("#name", "wangkun");
	set_zd_data("$name", "wk");
	set_zd_data("#test1", "");
	set_zd_data("#test2", NULL);
	set_zd_data("#test3", 0);

	/*Get normal var*/
	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#name", value);
	pub_log_info("%s, %d, #name[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data("$name", value);
	pub_log_info("%s, %d, $name[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#test1", value);
	pub_log_info("%s, %d, #test1[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#test2", value);
	pub_log_info("%s, %d, #test2[%s]",__FILE__,__LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#test3", value);
	pub_log_info("%s, %d, #test3[%s]",__FILE__,__LINE__, value);

	/*Set var with space*/
	set_zd_data("#space", "   1 2  3    4     5      6   ");

	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#space", value);
	pub_log_info("%s, %d, #space[%s]", __FILE__, __LINE__,value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data_space("#space", value);
	pub_log_info("%s, %d, #space[%s]", __FILE__, __LINE__,value);

	/*remove var*/
	set_zd_data("#remove", "remove");
	set_zd_data("#left", "left");

	var_remove("#remove");
	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#remove", value);
	pub_log_info("%s, %d, #remove[%s]", __FILE__, __LINE__, value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data("#left", value);
	pub_log_info("%s, %d, #left[%s]", __FILE__, __LINE__, value);

	
	/*Set int*/
	set_zd_int("#zheng", 99);
	set_zd_int("#ling", 0);
	set_zd_int("#fu", -1);

	/*Get int*/
	sw_int32_t	zheng = -100;
	sw_int32_t	fu = -100;
	sw_int32_t	ling = -100;

	get_zd_int("#zheng", &zheng);
	get_zd_int("#ling", &ling);
	get_zd_int("#fu", &fu);
	get_zd_int("#i_null", NULL);
	pub_log_info("%s, %d, #zheng[%d] #fu[%d] #ling[%d]"
					, __FILE__,__LINE__, zheng, fu, ling);
	/*Clear vars pool.*/
	vars_clear();

	zheng = fu = ling = -10;
	get_zd_int("#zheng", &zheng);
	get_zd_int("#ling", &ling);
	get_zd_int("#fu", &fu);
	get_zd_int("#i_null", NULL);
	pub_log_info("%s, %d, #zheng[%d] #fu[%d] #ling[%d]"
					, __FILE__,__LINE__, zheng, fu, ling);	
	
	/*Set double*/
	set_zd_double("#d_zheng", 123456.123456);
	set_zd_double("#d_ling", 0.0);
	set_zd_double("#d_fu", -123456.123456);

	/*Get double*/
	double	d_zheng = 0;
	double	d_fu = 0;
	double	d_ling = 0;

	get_zd_double("#d_zheng", &d_zheng);
	get_zd_double("#d_ling", &d_ling);
	get_zd_double("#d_fu", &d_fu);
	get_zd_double("#d_null", NULL);
	pub_log_info("%s, %d, #d_zheng[%lf] #d_fu[%lf] #d_ling[%lf]"
					, __FILE__,__LINE__, d_zheng, d_fu, d_ling);

	/*Set float*/
	set_zd_float("#f_zheng", 123456.123456);
	set_zd_float("#f_ling", 0);
	set_zd_float("#f_fu", -123456.123456);

	/*Get float*/
	float	f_zheng = 0;
	float	f_fu = 0;
	float	f_ling = 0;

	get_zd_float("#f_zheng", &f_zheng);
	get_zd_float("#f_ling", &f_ling);
	get_zd_float("#f_fu", &f_fu);
	get_zd_float("#f_null", NULL);
	pub_log_info("%s, %d, #f_zheng[%f] #f_ling[%f] #f_fu[%f]"
					, __FILE__,__LINE__, f_zheng, f_ling, f_fu);

	/*Set long*/
	set_zd_long("#l_zheng", 123456);
	set_zd_long("#l_ling", 0);
	set_zd_long("#l_fu", -123456);

	/*Get long*/
	long	l_zheng = 0;
	long	l_ling = 0;
	long	l_fu = 0;

	get_zd_long("#l_zheng", &l_zheng);
	get_zd_long("#l_ling", &l_ling);
	get_zd_long("#l_fu", &l_fu);
	get_zd_long("#l_null", NULL);
	pub_log_info("%s, %d, #l_zheng[%ld] #l_ling[%ld] #l_fu[%ld]"
					, __FILE__,__LINE__, l_zheng, l_ling, l_fu);

	/*test get field*/
	sw_char_t	test_field[256];
	sw_int32_t	len = 0;
	set_zd_data("#test_field", " 1  2   3   4    5  ");
	len = get_field_len("#test_field");
	pub_log_info("%s, %d, #test_field, len[%d]",__FILE__,__LINE__,len);
	
	sw_char_t	tozip[256];
	cp_field_zip("#test_field", "#to_zip", "desc: cp_field_zip");

	pub_mem_memzero(tozip, sizeof(tozip));
	get_zd_data("#to_zip", tozip);
	pub_log_info("%s, %d, #to_zip[%s]",__FILE__,__LINE__,tozip);

	cp_field("#test_field", "#to", "desc: cp_field");
	pub_mem_memzero(tozip, sizeof(tozip));
	get_zd_data("#to", tozip);
	pub_log_info("%s, %d, #to[%s]",__FILE__,__LINE__,tozip);

	/*Set xml var*/
	set_zd_data(".root.usr", "wangkun");

	/*Clear vars pool.*/
	vars_clear();
	
	set_zd_data(".root.passwd", "711");
	set_zd_data(".root.null", NULL);
	set_zd_data(".root.0", 0);

	/*Get xml var.*/
	pub_mem_memzero(value, sizeof(value));
	get_zd_data(".root.usr", value);
	get_zd_data(".root.null", NULL);
	get_zd_data(".root.0", 0);
	pub_log_info("%s, %d, .root.usr[%s]"
					,__FILE__,__LINE__,value);

	pub_mem_memzero(value, sizeof(value));
	get_zd_data(".root.passwd", value);
	pub_log_info("%s, %d, .root.passwd[%s]"
					,__FILE__,__LINE__,value);

	pub_log_info("%s, step%d",__FILE__,__LINE__);
	/*Set xml attr.*/
	set_xml_attr(".root.usr", "xing", "wang");
	pub_log_info("%s, step%d",__FILE__,__LINE__);

	/*Clear xml vars pool*/
	extvars_clear();

	set_xml_attr(".root.usr", "ming", "kun");
	set_xml_attr(".root.usr", "0", 0);
	set_xml_attr(".root.usr", "null", NULL);
	set_xml_attr(".root.usr", NULL, NULL);
	set_xml_attr(".root.usr", 0, NULL);


	pub_mem_memzero(value, sizeof(value));				
	get_xml_attr(".root.usr", "xing", value);
	get_xml_attr(".root.usr", "0", 0);
	get_xml_attr(".root.usr", "null", NULL);
	get_xml_attr(".root.usr", NULL, NULL);
	get_xml_attr(".root.usr", 0, NULL);
	pub_log_info("%s, %d, .root.usr.xing[%s]"
					,__FILE__,__LINE__,value);

	pub_mem_memzero(value, sizeof(value));				
	get_xml_attr(".root.usr", "ming", value);
	pub_log_info("%s, %d, .root.usr.ming[%s]"
					,__FILE__,__LINE__,value);

	/*Destroy global vars pool.*/
	result = pub_vars_free();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_free error.",__FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*Release shm_vars*/
	vars_destory();

	return 0;
}

