/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-21
 *** module  : vars pool interface 
 *** name    : pub_vars.h 
 *** function: abstract vars pool's interface, support SHM, HEAP vars pool.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __PUB_VARS_H__
#define __PUB_VARS_H__
#include "pub_type.h"
#include "pub_mem.h"
#include "shm_vars.h"
#include "pub_hvar.h"
#include "pub_buf.h"
#include <limits.h>

#define VARS_TREE	"$vars_xml_name_magic"
#define VARS_CONTENT_LEN "$vars_xml_len_magic"
#define VARS_XML_FLAG	"$vars_xml_flag_magic"
#define SERI_LEN	8

typedef enum {SHM_VARS = 0, HEAP_VARS, VARS_TYPE_NUM} sw_vars_type_t;

typedef sw_int_t (* sw_loc_vars_create_pt)(void* vars);
typedef sw_int_t (* sw_loc_vars_init_pt)(void* vars, sw_int32_t vid);
typedef sw_int_t (* sw_loc_vars_destroy_pt)(void* vars);
typedef sw_int_t (* sw_loc_vars_set_variable_pt)(void *vars
				, sw_char_t *name, sw_int32_t type
				, sw_char_t *value, sw_int32_t length);
				
typedef sw_int_t (* sw_loc_vars_set_string_pt)(void* vars, sw_char_t *name, sw_char_t* value);
typedef sw_int_t (* sw_loc_vars_set_xml_pt)(void* vars, sw_char_t *name, sw_char_t* value, int creat);
typedef sw_int_t (* sw_loc_vars_get_variable_pt)(void* vars, sw_char_t* name, sw_char_t* value);

typedef sw_int_t (* sw_loc_vars_clear_pt)(void* vars);
typedef sw_int_t (* sw_loc_remove_var_pt)(void* vars, sw_char_t* name);
typedef sw_int_t (* sw_loc_set_attr_pt)(void* vars, sw_char_t* var_name
				, sw_char_t* attr_name, sw_char_t* attr_value);
typedef sw_int_t (* sw_loc_get_attr_pt)(void *vars, sw_char_t *var_name
				, sw_char_t *attr_name, sw_char_t *attr_value);
typedef sw_int32_t (* sw_loc_serialize_pt)(void* vars, sw_buf_t* buf);
typedef sw_int32_t (* sw_loc_unserialize_pt)(void* vars, sw_char_t* buf);
typedef sw_int_t (*sw_loc_tostream_pt)(void *vars, char *buf, size_t len);
typedef sw_int_t (*sw_loc_destream_pt)(void *vars, char *buf);
typedef sw_char_t* (* sw_loc_get_value_addr_pt)(void* vars, sw_char_t* name, sw_int32_t *len);
typedef sw_char_t* (* sw_loc_get_null_pt)(void* vars, sw_char_t* name, sw_int32_t len);
typedef sw_int_t (* sw_loc_print_pt)(void* vars);
typedef sw_int_t (* sw_loc_clone_pt)(void* src_vars, sw_int32_t vid);
typedef sw_int_t (* sw_loc_merge_pt)(void *src, sw_int32_t vid);
typedef sw_int32_t (* sw_loc_get_field_len)(void* vars, sw_char_t* name);

struct sw_loc_vars_s
{
	sw_vars_type_t			vars_type;
	void				*data;
	sw_xmltree_t			*tree;
	sw_loc_vars_create_pt		alloc_mem;
	sw_loc_vars_init_pt		create;
	sw_loc_vars_destroy_pt		destroy;
	sw_loc_vars_destroy_pt		free_mem;
	sw_loc_vars_set_variable_pt	set_var;
	sw_loc_vars_set_string_pt	set_string;
	sw_loc_vars_get_variable_pt	get_var;
	sw_loc_get_value_addr_pt	get_var_addr;
	sw_loc_vars_get_variable_pt	get_xml_var;
	sw_loc_vars_set_xml_pt		set_xml_var;
	sw_loc_get_field_len		get_field_len;
	sw_loc_vars_clear_pt		clear_vars;
	sw_loc_vars_clear_pt		clear_xml_vars;
	sw_loc_remove_var_pt		remove_var;
	sw_loc_set_attr_pt		set_attr;
	sw_loc_get_attr_pt		get_attr;
	sw_loc_serialize_pt		serialize;
	sw_loc_unserialize_pt		unserialize;
	sw_loc_tostream_pt		tostream;
	sw_loc_destream_pt		destream;
	sw_loc_get_null_pt		get_null;
	sw_loc_print_pt			print;
	sw_loc_clone_pt			clone;
    sw_loc_merge_pt         merge;
};

typedef struct sw_loc_vars_s sw_loc_vars_t;

SW_PUBLIC sw_int_t pub_loc_vars_alloc(sw_loc_vars_t* vars, sw_vars_type_t vars_type);
SW_PUBLIC sw_int_t pub_loc_vars_create(sw_loc_vars_t* vars, int vid);

SW_PUBLIC sw_int_t pub_loc_vars_free(sw_loc_vars_t* vars);

/*Global vars pool interface*/

SW_PUBLIC sw_int_t pub_vars_alloc(sw_vars_type_t vars_type);

SW_PUBLIC sw_int_t pub_vars_create(sw_int32_t vid);

SW_PUBLIC sw_int_t pub_vars_destory();

SW_PUBLIC sw_int_t pub_vars_free();

SW_PUBLIC sw_int_t pub_vars_set_variable(sw_char_t *name, sw_int32_t type
				, sw_char_t *value, sw_int32_t length);

SW_PUBLIC sw_int_t pub_vars_set_string(sw_char_t *name, sw_char_t* value);

SW_PUBLIC sw_int_t pub_vars_get_variable(sw_char_t *name, sw_char_t* value);

SW_PUBLIC sw_int32_t pub_vars_serialize(sw_buf_t* buf);

SW_PUBLIC sw_int32_t pub_vars_unserialize(sw_char_t* buf);

SW_PUBLIC sw_int_t pub_vars_reset_tree(sw_xmltree_t* tree);

SW_PUBLIC sw_int32_t pub_vars_get_field_len(sw_char_t* name);

SW_PUBLIC sw_xmltree_t* pub_vars_get_tree();

SW_PUBLIC sw_loc_vars_t* pub_get_global_vars();

SW_PUBLIC int loc_vars_clear(sw_loc_vars_t *vars);
SW_PUBLIC int loc_extvars_clear(sw_loc_vars_t *vars);
SW_PUBLIC int loc_var_remove(sw_loc_vars_t *vars, char *name);
SW_PUBLIC int loc_set_xml_attr(sw_loc_vars_t *vars, char *var_name, char *attr_name, char *attr_value);
SW_PUBLIC int loc_get_xml_attr(sw_loc_vars_t *vars, char *var_name, char *attr_name, char *attr_value);
SW_PUBLIC int loc_set_xml_data(sw_loc_vars_t *vars, char *name, char * value);
SW_PUBLIC int loc_set_zd_data(sw_loc_vars_t *vars, char *name, char * value);
SW_PUBLIC int loc_get_zd_data(sw_loc_vars_t *vars, char *name, char * value);
SW_PUBLIC int loc_get_zd_data_space(sw_loc_vars_t *vars, char *name, char * value);
SW_PUBLIC int loc_get_zd_data_len(sw_loc_vars_t *vars, char *name, char * value, int len);
SW_PUBLIC int loc_get_data_len(sw_loc_vars_t *vars, char *name, char * value);
SW_PUBLIC int loc_set_zd_data_len(sw_loc_vars_t *vars, char *name, char * value, int len);
SW_PUBLIC int loc_set_zd_int(sw_loc_vars_t *vars, char *sjbm, int int_data);
SW_PUBLIC int loc_get_zd_int(sw_loc_vars_t *vars, char *sjbm, int * int_data);
SW_PUBLIC int loc_set_zd_long(sw_loc_vars_t *vars, char *sjbm, long long_data);
SW_PUBLIC int loc_get_zd_long(sw_loc_vars_t *vars,char *sjbm,long * long_data);
SW_PUBLIC int loc_set_zd_float(sw_loc_vars_t *vars,char *sjbm, float float_data);
SW_PUBLIC int loc_get_zd_float(sw_loc_vars_t *vars,char *sjbm, float * float_data);
SW_PUBLIC int loc_set_zd_double(sw_loc_vars_t *vars, char *sjbm, double double_data);
SW_PUBLIC int loc_get_zd_double(sw_loc_vars_t *vars, char *sjbm, double * double_data);
SW_PUBLIC int vars_clear();
SW_PUBLIC int extvars_clear();
SW_PUBLIC int var_remove(char * name);
SW_PUBLIC int set_xml_attr(char *var_name, char *attr_name, char *attr_value);
SW_PUBLIC int get_xml_attr(char *var_name, char *attr_name, char *attr_value);
SW_PUBLIC int set_xml_data(char *name, char * value);
SW_PUBLIC int set_zd_data(char *name, char * value);
SW_PUBLIC int get_zd_data(char * name, char * value);
SW_PUBLIC int get_zd_data_space(char * name, char * value);
SW_PUBLIC int get_zd_data_len(char * name, char * value, int len);
SW_PUBLIC int set_zd_data_len(char * name, char * value, int len);
SW_PUBLIC int set_zd_int(char *sjbm,int int_data);
SW_PUBLIC int get_zd_int(char *sjbm, int * int_data);
SW_PUBLIC int set_zd_long(char *sjbm, long long_data);
SW_PUBLIC int get_zd_long(char *sjbm,long * long_data);
SW_PUBLIC int set_zd_float(char *sjbm,float float_data);
SW_PUBLIC int get_zd_float(char *sjbm,float * float_data);
SW_PUBLIC int set_zd_double(char *sjbm, double double_data);
SW_PUBLIC int get_zd_double(char *sjbm, double * double_data);
SW_PUBLIC int get_data_len(char * name, char * value);
SW_PUBLIC int cp_field_zip(char* from, char* to, char* desc);
SW_PUBLIC int cp_field(char* from, char* to, char* desc);
SW_PUBLIC int set_variable(char *name,char type,char *value,short length);
SW_PUBLIC int set_xml_var(char *varname, char *value);
SW_PUBLIC int get_xml_var(char *varname, char *value);
SW_PUBLIC int get_variable(char *name,char *value);
SW_PUBLIC char *set_zdxml_data(sw_loc_vars_t *vars, char *name, char *value, char attr, int creat);

#endif /* __PUB_VARS_H__ */
