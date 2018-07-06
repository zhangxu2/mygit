#ifndef __PACK_H__
#define __PACK_H__

#include <dlfcn.h>
#include "pub_type.h"
#include "pub_vars.h"
#include "pub_log.h"
#include "pub_code.h"
#include "pub_comvars.h"
#include "pub_filter.h"

#define	PKG_IN	1
#define PKG_OUT	2
#define PKG_MAX_CHECK_CNT	10
#define PKG_MAX_NAME_SIZE   256
#define PKG_MAX_PROCESSOR_CNT 20
#define PKG_TYPE_MAX_LEN           (32)
#define PKG_NAME_MAX_LEN           (64)
#define PKG_RESPCD_MAX_LEN         (32)
#define PKG_VAR_NAME_MAX_LEN       (128)
#define PKG_VAR_VALUE_MAX_LEN      (2048)
#define PKG_VAR_TYPE_MAX_LEN       (32)
#define PKG_BIT_NAME_MAX_LEN       (16)
#define PKG_BIT_MAP_MAX_LEN        (65)
#define PKG_CONST_VAL_MAX_LEN      (1024)
#define PKG_CODE_MAX_LEN           (16)
#define PKG_LEN_CODE_MAX_LEN       (16)
#define PKG_SEP_MAX_LEN            (16)
#define PKG_MID_MAX_LEN            (16)
#define PKG_BUFF_INC_SIZE          (1024)

typedef struct
{
	sw_int_t	large;
	sw_int32_t      num;
	sw_int32_t      max;
	sw_int32_t      var_length;
	sw_int32_t      index;
	sw_int32_t      begin;
	sw_int32_t      start;
	sw_int32_t      length;
	sw_int32_t	offset;
	sw_int32_t	fix;
	sw_int32_t	is_null;
	sw_char_t       tag[PKG_VAR_NAME_MAX_LEN];
	sw_char_t       stuct[PKG_VAR_NAME_MAX_LEN];
	sw_char_t       var_name[PKG_VAR_NAME_MAX_LEN];
	sw_char_t       bit_name[PKG_BIT_NAME_MAX_LEN];
	sw_char_t       var_type[PKG_VAR_TYPE_MAX_LEN];
	sw_char_t       const_value[PKG_CONST_VAL_MAX_LEN];
	sw_char_t       code[PKG_CODE_MAX_LEN];                 /*code class: for example BCD ASCII.*/                   
	sw_char_t       len_code[PKG_LEN_CODE_MAX_LEN];         /*length field code class.*/                             
	sw_char_t       type;                                                                                            
	sw_char_t       flag_const;                                                                                      
	sw_char_t       blank;                                                                                           
	sw_char_t       flag;                                                                                            
	sw_char_t	loop;
	sw_char_t	imf;
	sw_char_t	key;
	sw_char_t	istag;
	sw_char_t	fvalue;
	sw_char_t       value[PKG_VAR_VALUE_MAX_LEN];                                                                
	sw_char_t	*var_value;                                                                
	sw_char_t       sep[PKG_SEP_MAX_LEN];                                                                            
	sw_char_t       middle[PKG_MID_MAX_LEN];                                                                         
	sw_char_t	len_varname[PKG_VAR_NAME_MAX_LEN]; /*** 放长度的变量名 ***/
}sw_pkg_item_t;                                                                              
                                                                                                                         
/*bitmap struct*/                                                                                                        
typedef struct
{                                                                                                                        
	sw_char_t       blen;                           /*len:1-8 bytes, 2-16 bytes.*/                                   
	sw_char_t       name[PKG_VAR_NAME_MAX_LEN];                                                                      
	sw_char_t       bitmap[PKG_BIT_MAP_MAX_LEN];                                                                     
	unsigned char       bit_value[8+1];                                                                                  
}sw_bits_t;

typedef struct
{
	int	begin;
	int	bitlen;
	int	bitnum;
	sw_char_t	bitvalue[25];
	sw_bits_t	bits[3];
}sw_bitmap_t;

typedef sw_int_t (*sw_com_pt)();
typedef sw_int_t (*sw_pkg_check_in_pt)(sw_char_t *, sw_int32_t);
typedef sw_int_t (*sw_pkg_check_out_pt)(sw_loc_vars_t *);
typedef sw_int_t (*sw_pkg_in_pt)(sw_loc_vars_t *, sw_char_t *, sw_char_t *, int, int);
typedef sw_int_t (*sw_pkg_out_pt)(sw_loc_vars_t *, sw_buf_t *, sw_char_t *, int);

typedef struct check_s
{
	int	dlflag;
	void	*handle;
	sw_com_pt	dlfun;
	sw_int32_t	begin;
	sw_int32_t	length;
	sw_char_t	name[64];
	sw_char_t	value[128];
	sw_char_t	lib[64];
	sw_char_t	fun[64];
	sw_char_t	check_value[128];
}check_t;

typedef struct sw_pkg_check_s
{
	sw_int32_t	check_cnt;
	check_t	check[PKG_MAX_CHECK_CNT];
}sw_pkg_check_t;

typedef struct sw_pkg_processor_s
{
	int	type;
	int	dlflag;
	void	*handle;
	sw_com_pt	dlfun;
	sw_char_t	name[128];
	sw_char_t	cfg[128];
	sw_char_t	lib[128];
	sw_char_t	fun[128];
	sw_xmltree_t	*xml;
	sw_pkg_check_t	pkg_check;
}sw_pkg_processor_t;

typedef struct
{
	sw_int_t	cnt;
	sw_pkg_processor_t	processor[PKG_MAX_PROCESSOR_CNT];
}sw_pkg_cache_list_t;

#define MAX_CLEAR_VARS_NUM	256
#define MAX_CLEAR_PACKAGE_NUM	10
typedef struct
{
	int	count;
	char	varnames[MAX_CLEAR_VARS_NUM][NAME_LEN];
} sw_vars_clear_package_t;

SW_PUBLIC sw_xmltree_t *pkg_read_xml(sw_char_t *xmlname);
SW_PUBLIC sw_int_t pkg_8583_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag);
SW_PUBLIC sw_int_t pkg_com_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag);
SW_PUBLIC sw_int_t pkg_xml_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag);
SW_PUBLIC sw_int_t pkg_com_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, int cache_flag);
SW_PUBLIC sw_int_t pkg_xml_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, int cache_flag);
SW_PUBLIC sw_int_t pkg_8583_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, int cache_flag);
SW_PUBLIC sw_int_t pkg_get_processor_index_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_int_t len, sw_pkg_cache_list_t *list);
SW_PUBLIC sw_int_t pkg_get_processor_index_out(sw_loc_vars_t *vars, sw_pkg_cache_list_t *list);
SW_PUBLIC sw_int_t pkg_get_processor_info_in(sw_xmltree_t *xml, sw_pkg_cache_list_t *list);
SW_PUBLIC sw_int_t pkg_get_processor_info_out(sw_xmltree_t *xml, sw_pkg_cache_list_t *list);
SW_PUBLIC int pkg_check_include(sw_loc_vars_t *vars, char *name, char *check_value);
SW_PUBLIC int pkg_check_mult(sw_loc_vars_t *vars, sw_xmlnode_t *check_node);
SW_PUBLIC int pkg_clear_vars(sw_loc_vars_t *vars);
SW_PUBLIC int pkg_release_list(sw_pkg_cache_list_t *list);
SW_PUBLIC int pkg_get_exp_value(sw_loc_vars_t *vars, char *str, char *out);
SW_PUBLIC void pkg_item_init(sw_pkg_item_t *item);
SW_PUBLIC sw_int_t pkg_get_com_item(sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_pkg_item_t *item);
SW_PUBLIC int alloc_value(sw_pkg_item_t *item);
SW_PUBLIC int release_value(sw_pkg_item_t *item);
SW_PUBLIC sw_int_t pkg_get_8583_item(sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_pkg_item_t *item);
SW_PUBLIC sw_int_t pkg_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *config, sw_pkg_cache_list_t *list, sw_char_t *prdt_name);
SW_PUBLIC sw_int_t pkg_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *config, sw_pkg_cache_list_t *list, sw_char_t *prdt_name);

#endif
