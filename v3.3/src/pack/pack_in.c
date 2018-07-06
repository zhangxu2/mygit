#include "pack.h"
#include <ctype.h>

static int	g_clear_xml_vars = 0;
static int	g_package_clear;
int	g_vars_clear_package_cnt;
extern sw_vars_clear_package_t	g_vars_clear_packages[MAX_CLEAR_PACKAGE_NUM];

#define	PACKAGE_CLEAR()	(g_package_clear = 1)
#define PACKAGE_NOT_CLEAR()	(g_package_clear = 0)	

#define CLEAR_VARS_PACKAGE_CNT_ADD() if (g_package_clear && g_vars_clear_package_cnt < MAX_CLEAR_PACKAGE_NUM) \
				{ \
					g_vars_clear_package_cnt++; \
				} \
				else \
				{ \
					g_package_clear = 0; \
				}
				
#define SAVE_CLEAR_VARS(name) if (g_package_clear && g_vars_clear_package_cnt < MAX_CLEAR_PACKAGE_NUM && \
				g_vars_clear_packages[g_vars_clear_package_cnt].count < MAX_CLEAR_VARS_NUM) \
				{\
				strcpy(g_vars_clear_packages[g_vars_clear_package_cnt].varnames[g_vars_clear_packages[g_vars_clear_package_cnt].count], name); \
				g_vars_clear_packages[g_vars_clear_package_cnt].count++; \
				}

extern sw_int_t pkg_get_value(sw_buf_t *pkg_buf, int begin, int length, char *sep, char *value);
extern int pkg_get_len_by_sep(sw_buf_t *pkg_buf, char *sep);
extern int pkg_check_index(sw_bitmap_t *bitmap, int index, char *bitname);
extern sw_int_t pub_xml_convert_analyze( sw_loc_vars_t *pstlocvar, sw_xmltree_t *pxml);

/*** 单个公共ITEM处理 ***/
sw_int_t pkg_deal_com_item_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_pkg_item_t *item, int index)
{
	int	i = 0;
	int	mod = 0;
	int	quot = 0;
	int	bcdlen = 0;
	char	buf[PKG_VAR_VALUE_MAX_LEN];
	
	memset(buf, 0x0, sizeof(buf));
	if (index >= 0)
	{
		sprintf(buf, "%s(%d)", item->var_name, index);
		memset(item->var_name, 0x0, sizeof(item->var_name));
		strncpy(item->var_name, buf, sizeof(item->var_name) - 1);
	}
	
	if (item->fix == 1)
	{
		/*** 把当前位置开始的所有部分放到一个变量里面 ***/
		int len = 0;
		if (item->var_type[0] == '\0')
		{
			item->type = 'a';
		}
		else
		{
			item->type = item->var_type[0];
		}
		
		if (item->begin > 0)
		{
			len = pkg_buf->size - item->begin;
			vars->set_var(vars, item->var_name, item->type, pkg_buf->data + item->begin, len);
			if (item->type == 'b')
			{
				pub_log_bin(SW_LOG_DEBUG, pkg_buf->data + item->begin, len, "[%s][%d] 拆出变量[%s]", 
					__FILE__, __LINE__, item->var_name);
			}
			else
			{
				pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, 
					item->var_name, pkg_buf->data + item->begin);
			}
		}
		else
		{
			len = pkg_buf->size - pkg_buf->len;
			vars->set_var(vars, item->var_name, item->type, pkg_buf->data + pkg_buf->len, len);
			if (item->type == 'b')
			{
				pub_log_bin(SW_LOG_DEBUG, pkg_buf->data + pkg_buf->len, len, "[%s][%d] 拆出变量[%s]", 
					__FILE__, __LINE__, item->var_name);
			}
			else
			{
				pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, 
					item->var_name, pkg_buf->data + pkg_buf->len);
			}
		}
		pkg_buf->len = pkg_buf->size;
		return SW_OK;
	}

	if (item->len_varname[0] == '#' || item->len_varname[0] == '$')
	{
		memset(buf, 0x0, sizeof(buf));
		pub_comvars_get(vars, item->len_varname, buf);
		pub_log_debug("[%s][%d] 变长变量:[%s] 长度变量:[%s]=[%s]", 
			__FILE__, __LINE__, item->var_name, item->len_varname, buf);
		/*** 在此重新拼一个TYPE ***/
		memset(item->var_type, 0x0, sizeof(item->var_type));
		sprintf(item->var_type, "a%d", atoi(buf));
		pub_log_debug("[%s][%d] 变量:[%s] 类型:[%s]", 
			__FILE__, __LINE__, item->var_name, item->var_type);
	}
	
	if (item->var_type[0] != '\0' && strncmp(item->var_type, "BITMAP", 6) != 0) 
	{
		item->type = item->var_type[0];
		item->num = 0;
		for (i = 1; i < PKG_VAR_TYPE_MAX_LEN; i++)
		{
			if (item->var_type[i] == '.')
			{
				item->num++;
				continue;
			}
			break;
		}

		item->max = atoi(item->var_type + item->num + 1);
		if (item->num == 0)
		{
			item->var_length = item->max;
		}
		else if (strcmp(item->len_code, "HUA") == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			if (item->num <= 2)
			{
				pkg_get_value(pkg_buf, 0, 1, NULL, buf);
				item->var_length = (int)buf[0];
			}
			else
			{
				pkg_get_value(pkg_buf, 0, 2, NULL, buf);
				item->var_length = (int)(buf[0] * 256 + buf[1]);
			}
		}
		else if (strcmp(item->code, "LBCD") == 0 || strcmp(item->code, "LMBCD") == 0)
		{
			int	varlen = 0;
			char	lenvalue[6];
			
			memset(lenvalue, 0x0, sizeof(lenvalue));
			memset(buf, 0x0, sizeof(buf));
			
			if (item->num % 2 == 0)
			{
				varlen = item->num / 2;
			}
			else
			{
				varlen = (item->num + 1) / 2;
			}
	
			pkg_get_value(pkg_buf, 0, varlen, NULL, buf);
			pub_code_bcdtoasc(buf, lenvalue, varlen);
			bcdlen = atoi(lenvalue);
			item->var_length = bcdlen;
			if (strcmp(item->code, "LMBCD") == 0)
			{
				if (bcdlen % 2 == 0)
				{
					item->var_length = bcdlen / 2;
				}
				else
				{
					item->var_length = (bcdlen + 1) / 2;
				}
			}
		}
		else
		{
			memset(buf, 0x0, sizeof(buf));
			pkg_get_value(pkg_buf, 0, item->num, NULL, buf);
			item->var_length = atoi(buf);
			if (item->var_length > item->max)
			{
				pub_log_error("[%s][%d] Var:[%s] length:[%d] max=[%d] overlength!",
					__FILE__, __LINE__, item->var_name, item->var_length, item->max);
				return SW_ERROR;
			}
		}
		
		if (strcmp(item->code, "BCD") == 0 && item->num == 0)
		{
			quot = item->var_length / 2;
			mod = item->var_length % 2;
			if (mod == 0)
			{
				item->var_length = quot;
			}
			else
			{
				item->var_length = quot + 1;
			}
		}
		alloc_value(item);
		pkg_get_value(pkg_buf, item->begin, item->var_length, item->sep, item->var_value);
	}
	else if (item->sep[0] != '\0')
	{
		item->max = pkg_get_len_by_sep(pkg_buf, item->sep); 
		alloc_value(item);
		item->var_length = pkg_get_value(pkg_buf, 0, 0, item->sep, item->var_value);
	}
	else if (item->var_type[0] == '\0')
	{
		pub_log_error("[%s][%d] 未知变量类型! var_type=[%s]", __FILE__, __LINE__, item->var_type);
		return SW_ERROR;
	}
	
	if (item->var_type[0] == 'b' || item->var_type[0] == 'B')
	{
		vars->set_var(vars, item->var_name, 'b', item->var_value, item->var_length);
		pub_log_bin(SW_LOG_INFO, item->var_value, item->var_length, "[%s][%d] BTYPE: name[%s] length[%d]", 
			__FILE__, __LINE__, item->var_name, item->var_length);
		SAVE_CLEAR_VARS(item->var_name);
	}
	else if (item->flag_const == '1')
	{
		if (strcmp(item->code, "BCD") == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			pub_code_bcdtoasc(item->const_value, buf, item->var_length);
			memset(item->var_value, 0x0, sizeof(item->value));
			strcpy(item->var_value, buf);
		}
		else
		{
			strcpy(item->var_value, item->const_value);
		}
		vars->set_string(vars, item->var_name, item->var_value);
		SAVE_CLEAR_VARS(item->var_name);
	}
	else
	{
		if (strcmp(item->code, "BCD") == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			pub_code_bcdtoasc(item->var_value, buf, item->var_length);
			memset(item->var_value, 0x0, sizeof(item->value));
			strcpy(item->var_value, buf);
		}
		else if (strcmp(item->code, "LMBCD") == 0)
		{			
			pub_code_bcdtoasc(item->var_value, buf, item->var_length);
			memset(item->var_value, 0x0, item->var_length * 2);
			memcpy(item->var_value, buf, item->var_length * 2);
			item->var_length = bcdlen;
		}
		
		vars->set_string(vars, item->var_name, item->var_value);
		SAVE_CLEAR_VARS(item->var_name);
	}
	if (item->var_type[0] != 'b' && item->var_type[0] != 'B')
	{
		if (pub_is_filter(item->var_name))
		{
			memset(buf, 0x0, sizeof(buf));
			memset(buf, '*', item->var_length);
			pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
				__FILE__, __LINE__, item->var_name, buf);
		}
		else
		{
			pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
				__FILE__, __LINE__, item->var_name, item->var_value);
		}
	}
	release_value(item);
	
	return SW_OK;
}

sw_int_t pkg_deal_struct_item_in(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_pkg_item_t *oitem)
{
	int	i = 0;
	sw_int_t	large = 0;
	sw_int_t	offset = 0;
	sw_char_t	buf[256];
	sw_char_t	*var_value = NULL;
	sw_char_t	value[PKG_VAR_VALUE_MAX_LEN];
	sw_pkg_item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(value, 0x0, sizeof(value));
	
	if (vars == NULL || xml == NULL || oitem->var_length < 0)
	{
		pub_log_error("[%s][%d] Params error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (oitem->var_length == 0)
	{
		pub_log_info("[%s][%d] 原域值为空,不进行子域拆分!", __FILE__, __LINE__);
		return SW_OK;
	}
		
	large = 0;
	var_value = value;
	if (oitem->var_length > PKG_VAR_VALUE_MAX_LEN)
	{
		var_value = (char *)calloc(1, oitem->var_length + 1);
		if (var_value == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, oitem->var_length, errno, strerror(errno));
			return SW_ERROR;
		}
		large = 1;
	}
	vars->get_var(vars, oitem->var_name, var_value);
	
	offset = 0;
	pkg_item_init(&item);
	node = pub_xml_locnode(xml, "SUB");
	while (node != NULL)
	{
		if (strcmp(node->name, "SUB") != 0)
		{
			node = node->next;
			continue;
		}
		
		pkg_item_init(&item);
		xml->current = node;
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}

		pkg_get_com_item(xml, node, &item);
		item.type = item.var_type[0];
		item.num = 0;
		for (i = 1; i < PKG_VAR_TYPE_MAX_LEN; i++)
		{
			if (item.var_type[i] == '.')
			{
				item.num++;
				continue;
			}
			break;
		}

		item.max = atoi(item.var_type + item.num + 1);
		if (item.num == 0)
		{
			item.var_length = item.max;
		}
		else
		{
			memset(buf, 0x0, sizeof(buf));
			memcpy(buf, var_value + offset, item.num);
			offset += item.num;
			item.var_length = atoi(buf);
			if (item.var_length > item.max)
			{
				pub_log_error("[%s][%d] Var:[%s] length:[%d] max=[%d] overlength!",
					__FILE__, __LINE__, item.var_name, item.var_length, item.max);
				return SW_ERROR;
			}
		}

		if (offset + item.var_length > oitem->var_length)
		{
			pub_log_info("[%s][%d] 子域长度之和大于域的实际长度,只拆除剩余部分!", __FILE__, __LINE__);
			item.var_length = oitem->var_length - offset;
		}
		memcpy(item.var_value, var_value + offset, item.var_length);
		offset += item.var_length;
		
		if (item.type == 'b' || item.type == 'B')
		{
			vars->set_var(vars, item.var_name, 'b', item.var_value, item.var_length);
			pub_log_bin(SW_LOG_INFO, item.var_value, item.var_length, "[%s][%d] BTYPE: name[%s] length[%d]",
				__FILE__, __LINE__, item.var_name, item.var_length);
			SAVE_CLEAR_VARS(item.var_name);
		}
		else
		{
			vars->set_string(vars, item.var_name, item.var_value);
			pub_log_info("[%s][%d] 拆出子域[%s]=[%s]", __FILE__, __LINE__, item.var_name, item.var_value);
			SAVE_CLEAR_VARS(item.var_name);
		}
		if (offset >= oitem->var_length)
		{
			break;
		}
		node = node->next;
	}
	if (large == 1)
	{
		free(var_value);
	}
	pub_log_info("[%s][%d] 拆变量[%s]的子域完成!", __FILE__, __LINE__, oitem->var_name);

	return SW_OK;
}

sw_int_t pkg_deal_loop_item_in(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_buf_t *pkg_buf, int index)
{
	sw_int_t	ret = 0;
	sw_pkg_item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	pkg_item_init(&item);
	
	node = pub_xml_locnode(xml, "SUB");
	while (node != NULL)
	{
		if (strcmp(node->name, "SUB") != 0)
		{
			node = node->next;
			continue;
		}
		
		pkg_item_init(&item);
		xml->current = node;
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}
		pkg_get_com_item(xml, node, &item);
		ret = pkg_deal_com_item_in(vars, pkg_buf, &item, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		node = node->next;
	}
	
	return SW_OK;
}

/*** 处理循环ITEM ***/
sw_int_t pkg_deal_loop_in(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_buf_t *pkg_buf)
{
	int	flag = 0;
	int	sep_len = 0;
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_int_t	loop_cnt = 0;
	sw_char_t	buf[128];
	sw_char_t	sep[128];
	sw_char_t	asc_sep[128];
	sw_char_t	bcd_sep[128];
	sw_char_t	break_sep[128];
	sw_xmlnode_t	*node = NULL;
	sw_pkg_item_t	item;
	
	memset(buf, 0x0, sizeof(buf));
	memset(sep, 0x0, sizeof(sep));
	memset(asc_sep, 0x0, sizeof(asc_sep));
	memset(bcd_sep, 0x0, sizeof(bcd_sep));
	memset(break_sep, 0x0, sizeof(break_sep));
	pkg_item_init(&item);
	
	pub_log_info("[%s][%d] Deal LOOP in...", __FILE__, __LINE__);
	xml->current = pnode;
	node = pub_xml_locnode(xml, "BREAK");
	if (node != NULL && node->value != NULL)
	{
		if (strcmp(node->value, "NULL") == 0)
		{
			/*** 没循环次数,直到报文拆完为止 ***/
			flag = 2;
			pub_log_info("[%s][%d] LOOP NULL!", __FILE__, __LINE__);
		}
		else
		{
			/*** 没循环次数,遇到中止符时循环结束 ***/
			flag = 3;
			if (strncmp(node->value, "\\x", 2) == 0)
			{
				memset(asc_sep, 0x0, sizeof(asc_sep));
				memset(bcd_sep, 0x0, sizeof(bcd_sep));
				strcpy(asc_sep, node->value + 2);
				pub_code_asctobcd(asc_sep, bcd_sep, strlen(asc_sep));
				strncpy(break_sep, bcd_sep, strlen(asc_sep) / 2);
			}
			else
			{
				strncpy(break_sep, node->value, sizeof(break_sep) - 1);
			}
			sep_len = strlen(break_sep);
			pub_log_info("[%s][%d] LOOP BREAK SEP=[%s]", __FILE__, __LINE__, break_sep);
		}
	}
	
	node = pub_xml_locnode(xml, "LOOPCNT");
	if (node != NULL && node->value != NULL)
	{
		if (node->value[0] == '#' || node->value[0] == '$')
		{
			/*** 有循环次数 ***/
			flag = 1;
			loc_get_zd_data(vars, node->value, buf);
			pub_log_info("[%s][%d] LOOPCNT:[%s]=[%s]", __FILE__, __LINE__, node->value, buf);
			loop_cnt = atoi(buf);
			pub_log_info("[%s][%d] Loop cnt:[%s][%d]", __FILE__, __LINE__, buf, loop_cnt);
		}
	}
	
	if (flag == 0)
	{
		pub_log_error("[%s][%d] 未配置循环结束条件!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, "SEP");
	if (node != NULL && node->value != NULL)
	{
		if (strncmp(node->value, "\\x", 2) == 0)
		{
			memset(asc_sep, 0x0, sizeof(asc_sep));
			memset(bcd_sep, 0x0, sizeof(bcd_sep));
			strcpy(asc_sep, node->value + 2);
			pub_code_asctobcd(asc_sep, bcd_sep, strlen(asc_sep));
			strncpy(sep, bcd_sep, strlen(asc_sep) / 2);
		}
		else
		{
			strncpy(sep, node->value, sizeof(sep) - 1);
		}
	}
	
	if (flag == 1)
	{
		for (i = 0; i < loop_cnt; i++)
		{
			pub_log_info("[%s][%d] 第[%d]次循环...", __FILE__, __LINE__, i);
			xml->current = pnode;
			ret = pkg_deal_loop_item_in(vars, xml, pkg_buf, i);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal loop item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}

			if (sep[0] != '\0')
			{
				pkg_buf->len += strlen(sep);
			}
		}
		pub_log_info("[%s][%d] 拆循环报文结束!", __FILE__, __LINE__);
	}
	else if (flag == 2)
	{
		i = 0;
		while (pkg_buf->len < pkg_buf->size)
		{
			pub_log_info("[%s][%d] 第[%d]次循环...", __FILE__, __LINE__, i);
			xml->current = pnode;
			ret = pkg_deal_loop_item_in(vars, xml, pkg_buf, i);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal loop item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}

			i++;
			if (sep[0] != '\0')
			{
				pkg_buf->len += strlen(sep);
			}
		}
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", i);
		loc_set_zd_data(vars, "#loopcnt", buf);
		pub_log_info("[%s][%d] 拆循环报文结束!", __FILE__, __LINE__);
	}
	else if (flag == 3)
	{
		i = 0;
		while (pkg_buf->len < pkg_buf->size && memcmp(pkg_buf->data + pkg_buf->len, break_sep, sep_len) != 0)
		{
			pub_log_info("[%s][%d] 第[%d]次循环...", __FILE__, __LINE__, i);
			xml->current = pnode;
			ret = pkg_deal_loop_item_in(vars, xml, pkg_buf, i);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal loop item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}

			i++;
			if (sep[0] != '\0')
			{
				pkg_buf->len += strlen(sep);
			}
		}
		pkg_buf->len += sep_len;
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%d", i);
		loc_set_zd_data(vars, "#loopcnt", buf);
		pub_log_info("[%s][%d] 拆循环报文结束!", __FILE__, __LINE__);
	}
	
	return SW_OK;
}

sw_int_t pkg_deal_imf_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_pkg_item_t *item)
{
	int	len = 0;
	sw_char_t	large = 0;
	sw_char_t	*var_value = NULL;
	sw_char_t	buf[128];
	sw_char_t	prefix[16];
	sw_char_t	name[PKG_VAR_NAME_MAX_LEN];
	sw_char_t	value[PKG_VAR_VALUE_MAX_LEN];
	
	memset(buf, 0x0, sizeof(buf));
	memset(prefix, 0x0, sizeof(prefix));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	
	if (item->offset == 0)
	{
		item->offset = 1;
	}
	strncpy(prefix, item->var_name, item->offset);
	while (pkg_buf->len < pkg_buf->size)
	{
		large = 0;
		memset(buf, 0x0, sizeof(buf));
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		var_value = value;
		
		strncpy(name, prefix, item->offset);
		pkg_get_value(pkg_buf, 0, 4, NULL, name + item->offset);
		
		memset(buf, 0x0, sizeof(buf));
		pkg_get_value(pkg_buf, 0, 1, NULL, buf);
		len = atoi(buf);
		
		memset(buf, 0x0, sizeof(buf));
		pkg_get_value(pkg_buf, 0, len, NULL, buf);
		len = atoi(buf);
		if (len > PKG_VAR_VALUE_MAX_LEN)
		{
			var_value = (char *)calloc(1, len + 1);
			if (var_value == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}
			large = 1;
		}
		pkg_get_value(pkg_buf, 0, len, NULL, var_value);
		loc_set_zd_data(vars, name, var_value);
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, name, var_value);
		
		if (large == 1)
		{
			free(var_value);
		}
	}
	pub_log_info("[%s][%d] 拆IMF报文完成!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t pkg_deal_mid_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_pkg_item_t *item)
{
	int	len = 0;
	sw_char_t	large = 0;
	sw_char_t	*var_value = NULL;
	sw_char_t	buf[128];
	sw_char_t	prefix[16];
	sw_char_t	name[PKG_VAR_NAME_MAX_LEN];
	sw_char_t	value[PKG_VAR_VALUE_MAX_LEN];
	
	memset(buf, 0x0, sizeof(buf));
	memset(prefix, 0x0, sizeof(prefix));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	
	if (item->offset == 0)
	{
		item->offset = 1;
	}
	strncpy(prefix, item->var_name, item->offset);

	while (pkg_buf->len < pkg_buf->size)
	{
		large = 0;
		memset(buf, 0x0, sizeof(buf));
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		var_value = value;
		
		strncpy(name, prefix, item->offset);
		pkg_get_value(pkg_buf, 0, 0, item->middle, name + item->offset);
		
		len = pkg_get_len_by_sep(pkg_buf, item->sep);
		if (len > PKG_VAR_VALUE_MAX_LEN)
		{
			var_value = (char *)calloc(1, len + 1);
			if (var_value == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}
			large = 1;
		}
		len = pkg_get_value(pkg_buf, 0, 0, item->sep, var_value);
		loc_set_zd_data(vars, name, var_value);
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, name, var_value);
		if (large == 1)
		{
			free(var_value);
		}
	}
	pub_log_info("[%s][%d] 拆MIDDLE报文完成!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t pkg_deal_key_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_pkg_item_t *item)
{
	int	i = 0;
	int	j = 0;
	int	len = 0;
	sw_char_t	large = 0;
	sw_char_t	*var_value = NULL;
	sw_char_t	buf[128];
	sw_char_t	sep[32];
	sw_char_t	prefix[16];
	sw_char_t	name[PKG_VAR_NAME_MAX_LEN];
	sw_char_t	value[PKG_VAR_VALUE_MAX_LEN];
	
	memset(buf, 0x0, sizeof(buf));
	memset(sep, 0x0, sizeof(sep));
	memset(prefix, 0x0, sizeof(prefix));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	
	if (strcmp(item->sep, "</>") == 0)
	{
		strcpy(sep, item->sep);
	}
	else if (strncmp(item->sep, "</", 2) == 0)
	{
		strcpy(sep, "</");
	}
	else
	{
		pub_log_error("[%s][%d] 暂不处理该种分割符的报文! sep=[%s]", 
			__FILE__, __LINE__, item->sep);
		return SW_ERROR;
	}

	while (pkg_buf->len < pkg_buf->size)
	{
		large = 0;
		memset(buf, 0x0, sizeof(buf));
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		var_value = value;
		
		len = pkg_get_len_by_sep(pkg_buf, sep);
		if (len > PKG_VAR_VALUE_MAX_LEN)
		{
			var_value = (char *)calloc(1, len + 1);
			if (var_value == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}
			large = 1;
		}

		len = pkg_get_value(pkg_buf, 0, 0, sep, var_value);
		name[0] = '#';
		i = 1;
		while (i < len && var_value[i] != '>') 
		{
			name[i] = var_value[i];
			i++;
		}
		i++;
		
		for (j = 0; j < len - i; j++)
		{
			var_value[j] = var_value[j + i];
		}
		var_value[j] = '\0';

		if (strncmp(sep, "</", 2) == 0 && strcmp(sep, "</>") != 0)
		{
			while (pkg_buf->data[pkg_buf->len] != '>' && pkg_buf->len < pkg_buf->size)
			{
				pkg_buf->len++;
			}
			pkg_buf->len++;
		}
		loc_set_zd_data(vars, name, var_value);
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, name, var_value);
		if (large == 1)
		{
			free(var_value);
		}
	}
	
	pub_log_info("[%s][%d] 拆KEY-VALUE报文完成!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t pkg_deal_tag_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_pkg_item_t *item)
{
	int	i = 0;
	int	large = 0;
	int	size = 0;
	char	*ptr = NULL;
	sw_char_t	*var_value = NULL;
	sw_char_t	tag[PKG_VAR_NAME_MAX_LEN];
	sw_char_t	name[PKG_VAR_NAME_MAX_LEN];
	sw_char_t	value[PKG_VAR_VALUE_MAX_LEN];
	
	memset(tag, 0x0, sizeof(tag));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	
	if (pkg_buf->data[pkg_buf->len] != '{')
	{
		pub_log_bin(SW_LOG_ERROR, pkg_buf->data + pkg_buf->len, pkg_buf->size - pkg_buf->len, 
			"[%s][%d] 报文格式有误!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ptr = strchr(pkg_buf->data + pkg_buf->len, ':');
	if (ptr == NULL)
	{
		pub_log_bin(SW_LOG_ERROR, pkg_buf->data + pkg_buf->len, pkg_buf->size - pkg_buf->len, 
			"[%s][%d] 报文格式有误!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(tag, 0x0, sizeof(tag));
	strncpy(tag, pkg_buf->data + pkg_buf->len, (ptr - pkg_buf->data) - pkg_buf->len + 1);
	pub_log_info("[%s][%d] BLOCKMARK=[%s]", __FILE__, __LINE__, tag);
	pkg_buf->len += (ptr - pkg_buf->data) - pkg_buf->len + 1;
	if (strcmp(tag, "{F:") == 0)
	{
		pub_log_info("[%s][%d] FILE BLOCK!", __FILE__, __LINE__);
		i = 0;
		memset(value, 0x0, sizeof(value));
		while (pkg_buf->len < pkg_buf->size && pkg_buf->data[pkg_buf->len] != '}')
		{
			value[i++] = pkg_buf->data[pkg_buf->len++];
		}
		if (pkg_buf->data[pkg_buf->len] == '}')
		{
			pkg_buf->len++;
		}
		loc_set_zd_data(vars, "$fileflag", "1");
		loc_set_zd_data(vars, "$filename", value);
		pub_log_info("[%s][%d] $filename=[%s]", __FILE__, __LINE__, value);
		
		pub_log_info("[%s][%d] 拆FILE BLOCK完成!", __FILE__, __LINE__);
	
		return SW_OK;
	}
	
	while (pkg_buf->len < pkg_buf->size && pkg_buf->data[pkg_buf->len] != '}')
	{
		/*** {2::CLZ:1234:WD0:20140401:SBN:40255108008} ***/
		/*** 跳过第一个冒号 ***/
		pkg_buf->len += 1;
		ptr = strchr(pkg_buf->data + pkg_buf->len, ':');
		if (ptr == NULL)
		{
			pub_log_bin(SW_LOG_ERROR, pkg_buf->data + pkg_buf->len, pkg_buf->size - pkg_buf->len, 
				"[%s][%d] 报文格式有误!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		memset(tag, 0x0, sizeof(tag));
		strncpy(tag, pkg_buf->data + pkg_buf->len, 3);
		pkg_buf->len += 4;
		
		i = 0;
		large = 0;
		memset(value, 0x0, sizeof(value));
		var_value = value;
		size = sizeof(value);
		while (pkg_buf->data[pkg_buf->len] != ':' && pkg_buf->data[pkg_buf->len] != '}')
		{
			if (i >= size)
			{
				if (large == 1)
				{
					pub_log_info("[%s][%d] Realloc Large var!", __FILE__, __LINE__);
					pub_log_info("[%s][%d] before realloc var_value=[%x]", __FILE__, __LINE__, var_value);
					var_value = realloc(var_value, size * 2 + 1);
					if (var_value == NULL)
					{
						pub_log_error("[%s][%d] realloc error! errno=[%d]:[%s]",
							__FILE__, __LINE__, errno, strerror(errno));
						return -1;
					}
					pub_log_info("[%s][%d] after realloc var_value=[%x]", __FILE__, __LINE__, var_value);
					memset(var_value + size, 0x0, size + 1);
				}
				else
				{
					pub_log_info("[%s][%d] Alloc Large var!", __FILE__, __LINE__);
					var_value = (char *)calloc(1, size * 2 + 1);
					if (var_value == NULL)
					{
						pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
							__FILE__, __LINE__, errno, strerror(errno));
						return -1;
					}
					large = 1;
					memcpy(var_value, value, i);
				}
				size *= 2;
			}
			var_value[i++] = pkg_buf->data[pkg_buf->len++];
		}
		
		memset(name, 0x0, sizeof(name));
		sprintf(name, "#%s", tag);
		loc_set_zd_data(vars, name, var_value);
		pub_log_info("[%s][%d] 拆出变量:[%s]=[%s][%d]", 
			__FILE__, __LINE__, name, var_value, i);
		if (large == 1)
		{
			pub_log_info("[%s][%d] Free large var!", __FILE__, __LINE__);
			free(var_value);
		}
	}
	if (pkg_buf->data[pkg_buf->len] == '}')
	{
		pkg_buf->len++;
	}
	pub_log_info("[%s][%d] 拆TAG报文完成!", __FILE__, __LINE__);
	
	return SW_OK;
}

/*** 处理拆包公共ITEM(8583ITEM除外) ***/
sw_int_t pkg_com_item_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *xml, sw_xmlnode_t *pnode)
{
	sw_int_t	ret = 0;
	sw_pkg_item_t	item;
	
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] node is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pkg_item_init(&item);
	xml->current = pnode;
	pkg_get_com_item(xml, pnode, &item);
	if (item.loop == '1')
	{
		ret = pkg_deal_loop_in(vars, xml, pnode, pkg_buf);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal loop error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else if (item.imf == '1')
	{
		ret = pkg_deal_imf_in(vars, pkg_buf, &item);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal IMF error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else if (item.middle[0] != '\0')
	{
		ret = pkg_deal_mid_in(vars, pkg_buf, &item);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal MIDDLE error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else if (item.key == '1')
	{
		ret = pkg_deal_key_in(vars, pkg_buf, &item);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal MIDDLE error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else if (item.istag == '1')
	{
		ret = pkg_deal_tag_in(vars, pkg_buf, &item);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal TAG error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		ret = pkg_deal_com_item_in(vars, pkg_buf, &item, -1);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

/*** 处理8583ITEM ***/
sw_int_t pkg_deal_8583_item_in(vars, xml, node, pkg_buf, bitmap)
sw_loc_vars_t *vars;
sw_xmltree_t *xml;
sw_xmlnode_t *node;
sw_buf_t *pkg_buf;
sw_bitmap_t *bitmap;
{
	int	i = 0;
	int	ib = 0;
	int	bitnum = 0;
	char	buf[PKG_VAR_VALUE_MAX_LEN];
	sw_int_t	ret = 0;
	sw_pkg_item_t	item;
	unsigned char	cb;
	unsigned char	bits[16 + 1];
	
	memset(buf, 0x0, sizeof(buf));
	memset(bits, 0x0, sizeof(bits));
	
	pkg_item_init(&item);
	xml->current = node;
	pkg_get_8583_item(xml, node, &item);
	
	bitnum = bitmap->bitnum;
	if (strcmp(item.var_type, "BITMAP") == 0 || strcmp(item.var_type, "BITMAP2") == 0)
	{
		if (item.begin < 0 || item.length <= 0)
		{
			pub_log_error("[%s][%d] Bit map length error! begin=[%d] length=[%d]", 
				__FILE__, __LINE__, item.begin, item.length);
			return SW_ERROR;
		}
		memcpy(bitmap->bits[bitnum].name, item.var_name, strlen(item.var_name));
		pkg_get_value(pkg_buf, item.begin, item.var_length, NULL, (char *)bits);
		memcpy(item.var_value, bits, item.var_length);
		if (strcmp(item.var_type, "BITMAP2") == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			pub_code_asctobcd((char *)bits, buf, 16);
			memset(bits, 0x0, sizeof(bits));
			memcpy(bits, buf, 8);
			bitmap->bits[bitnum].blen = 2;
		}
		else
		{
			bitmap->bits[bitnum].blen = 1;
		}
		
		for (i = 0; i < 64; i++)
		{
			ib = i / 8;
			cb = bits[ib] & 0x80;
			if (cb == 0x80)
			{
				bitmap->bits[bitnum].bitmap[i] = '1';
			}
			else
			{
				bitmap->bits[bitnum].bitmap[i] = '0';
			}
			bits[ib] = bits[ib] << 1;
		}
		bitmap->bitnum++;
	}
	
	ret = pkg_deal_com_item_in(vars, pkg_buf, &item, -1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (strcasecmp(item.stuct, "TRUE") == 0 || item.stuct[0] == '1')
	{
		ret = pkg_deal_struct_item_in(vars, xml, &item);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal struct item error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	return SW_OK;
}

/*** 处理拆包FIRSTWORK ***/
sw_int_t pkg_deal_firstwork_in(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_buf_t *pkg_buf, sw_char_t *first_name)
{
	int	first = 1;
	sw_pkg_item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	pkg_item_init(&item);
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.FIRSTWORK.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		pkg_item_init(&item);
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(item.var_name, node1->value, sizeof(item.var_name) - 1);
		}
	
		node1 = pub_xml_locnode(xml, "TYPE");
		if (node1 != NULL && node1->value != NULL)
		{
			item.type = node1->value[0];
		}
		
		node1 = pub_xml_locnode(xml, "BEGIN");
		if (node1 != NULL && node1->value != NULL)
		{
			item.begin = atoi(node1->value);
		}
		
		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			item.length = atoi(node1->value);
		}
		
		node1 = pub_xml_locnode(xml, "FILEFLAG");
		if (node1 != NULL && node1->value != NULL)
		{
			vars->set_string(vars, "$FILEFLAG", node1->value);
		}
		
		memcpy(item.var_value, pkg_buf->data + item.begin, item.length);
		vars->set_var(vars, item.var_name, item.type, item.var_value, item.length);  
		if (first == 1)
		{
			strcpy(first_name, item.var_name);
			first = 0;
		}
		pub_log_info("[%s][%d] FIRSTWORK 拆出变量[%s]=[%s]",
			__FILE__, __LINE__, item.var_name, item.var_value);
		
		node = node->next;
	}
	
	return SW_OK;
}

/*** 处理拆包报文头 ***/
sw_int_t pkg_deal_head_in(vars, pkg_buf, xml, first_name)
sw_loc_vars_t *vars;
sw_buf_t *pkg_buf;
sw_xmltree_t *xml;
char *first_name;
{
	sw_int_t	ret = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.HEAD");
	while (node != NULL)
	{
		if (strcmp(node->name, "HEAD") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, first_name, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CLEAR");
		if (node1 != NULL && node1->value != NULL && node1->value[0] == '1')
		{
			PACKAGE_CLEAR();	
		}
		else
		{
			PACKAGE_NOT_CLEAR();	
		}
		
		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node1 = node1->next;
				continue;
			}

			xml->current = node1;
			node2 = pub_xml_locnode(xml, "INCLUDE");
			if (node2 != NULL && node2->value != NULL)
			{
				if (pkg_check_include(vars, first_name, node2->value) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(xml, "CHECK");
			if (node2 != NULL)
			{
				if (pkg_check_mult(vars, node2) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			ret = pkg_com_item_in(vars, pkg_buf, xml, node1);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			node1 = node1->next;
		}
		CLEAR_VARS_PACKAGE_CNT_ADD();
		
		node = node->next;
	}

	return SW_OK;
}

/*** 处理拆包PACKAGE ***/
sw_int_t pkg_deal_8583_package_in(vars, pkg_buf, xml, first_name)
sw_loc_vars_t *vars;
sw_buf_t *pkg_buf;
sw_xmltree_t *xml;
sw_char_t *first_name;
{
	int	bitindex = 0;
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	bitname[32];
	sw_char_t	respcd[32];
	sw_char_t	respcdname[32];
	sw_bitmap_t	bitmap;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(bitname, 0x0, sizeof(bitname));
	memset(respcd, 0x0, sizeof(respcd));
	memset(respcdname, 0x0, sizeof(respcdname));
	memset(&bitmap, 0x0, sizeof(bitmap));
	bitmap.begin = 0;
	bitmap.bitlen = 0;
	bitmap.bitnum = 0;

	node = pub_xml_locnode(xml, ".CBM.ANALYZE.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, first_name, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}
	
		node1 = pub_xml_locnode(xml, "CLEAR");
		if (node1 != NULL && node1->value != NULL && node1->value[0] == '1')
		{
			PACKAGE_CLEAR();
		}
		else
		{
			PACKAGE_NOT_CLEAR();
		}

		node1 = pub_xml_locnode(xml, "RESPCD");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(respcdname, node1->value, sizeof(respcdname) - 1);
		}
		
		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			xml->current = node1;
			memset(bitname, 0x0, sizeof(bitname));
			node2 = pub_xml_locnode(xml, "BITMAP");
			if (node2 != NULL && node2->value != NULL)
			{
				strncpy(bitname, node2->value, sizeof(bitname) - 1);
			}

			node2 = pub_xml_locnode(xml, "INDEX");
			if (node2 != NULL && node2->value != NULL)
			{
				bitindex = atoi(node2->value);
				if (pkg_check_index(&bitmap, bitindex, bitname) <= 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			xml->current = node1;
			ret = pkg_deal_8583_item_in(vars, xml, node1, pkg_buf, &bitmap);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal 8583 item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			node1 = node1->next;
		}
		node = node->next;
		CLEAR_VARS_PACKAGE_CNT_ADD();
		
		if (respcdname[0] != '\0')
		{
			memset(buf, 0x0, sizeof(buf));
			memset(respcd, 0x0, sizeof(respcd));
			loc_get_zd_data(vars, respcdname, buf);
			if (strlen(buf) < 4)
			{
				sprintf(respcd, "%4s", buf);
				loc_set_zd_data(vars, "$respcd", respcd);
			}
		}
	}
	
	return SW_OK;
}

sw_int_t pkg_deal_com_package_in(vars, pkg_buf, xml, first_name)
sw_loc_vars_t *vars;
sw_buf_t *pkg_buf;
sw_xmltree_t *xml;
sw_char_t *first_name;
{
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	respcd[32];
	sw_char_t	respcdname[32];
	sw_pkg_item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(respcd, 0x0, sizeof(respcd));
	memset(respcdname, 0x0, sizeof(respcdname));
	
	pkg_item_init(&item);
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, first_name, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CLEAR");
		if (node1 != NULL && node1->value != NULL && node1->value[0] == '1')
		{
			PACKAGE_CLEAR();
		}
		else
		{
			PACKAGE_NOT_CLEAR();
		}

		node1 = pub_xml_locnode(xml, "RESPCD");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(respcdname, node1->value, sizeof(respcdname) - 1);
		}

		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			xml->current = node1;
			node2 = pub_xml_locnode(xml, "INCLUDE");
			if (node2 != NULL && node2->value != NULL)
			{
				if (pkg_check_include(vars, first_name, node2->value) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(xml, "CHECK");
			if (node2 != NULL)
			{
				if (pkg_check_mult(vars, node2) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			xml->current = node1;
			ret = pkg_com_item_in(vars, pkg_buf, xml, node1);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			node1 = node1->next;
		}
		node = node->next;
		CLEAR_VARS_PACKAGE_CNT_ADD();

		if (respcdname[0] != '\0')
		{
			memset(buf, 0x0, sizeof(buf));
			memset(respcd, 0x0, sizeof(respcd));
			loc_get_zd_data(vars, respcdname, buf);
			if (strlen(buf) < 4)
			{
				sprintf(respcd, "%4s", buf);
				loc_set_zd_data(vars, "$respcd", respcd);
			}
		}
	}
	
	return SW_OK;
}

/*** 处理拆包LASTWORK ***/
sw_int_t pkg_deal_last_work_in(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_char_t *first_name)
{
	sw_pkg_item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
		
	pkg_item_init(&item);
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.LASTWORK.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		pkg_item_init(&item);
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, first_name, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		strncpy(item.var_name, node1->value, sizeof(item.var_name) - 1);
	
		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config VALUE!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pkg_get_exp_value(vars, node1->value, item.var_value);
		vars->set_string(vars, item.var_name, item.var_value);
		pub_log_info("[%s][%d] LASKWORK 拆出变量: NAME=[%s] VALUE=[%s]", 
			__FILE__, __LINE__, item.var_name, item.var_value);
	
		node = node->next;
	}
	
	return SW_OK;
}

sw_int_t pkg_get_digital_sign(sw_loc_vars_t *vars, sw_buf_t *pkg_buf)
{
	int	len = 0;
	char	buf[128];
	char	xml_head[128];
	char	*ptr = NULL;
	char	*text = NULL;
	char	*ptmp = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(xml_head, 0x0, sizeof(xml_head));
	
	ptr = strstr(pkg_buf->data + pkg_buf->len, "<?xml");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Xml pkg error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ptmp = strstr(ptr, ">");
	if (ptmp == NULL)
	{
		pub_log_error("[%s][%d] Xml pkg error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(xml_head, ptr, ptmp - ptr + 1);
	loc_set_zd_data(vars, "$sw_xml_headflag", xml_head);
	pub_log_info("[%s][%d] XML HEAD:[%s]", __FILE__, __LINE__, xml_head);
	
	len = ptr - (pkg_buf->data + pkg_buf->len);
	if (len == 0)
	{
		return SW_OK;
	}
	
	text = (char *)calloc(1, len + 1);
	if (text == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	
	memset(buf, 0x0, sizeof(buf));
	memcpy(text, pkg_buf->data + pkg_buf->len, len);
	pub_log_info("[%s][%d] text=[%s]", __FILE__, __LINE__, text);
	if (strncmp(text, "{S:", 3) == 0 && pub_str_strrncmp(text, "}\x0d\x0a", 3) == 0)
	{
		memset(text + len - 3, 0x0, 3);
		sprintf(buf, "%d", len - 6);
		loc_set_zd_data(vars, "$dstext", text + 3);
		loc_set_zd_data(vars, "$dslen", buf);
		loc_set_zd_data(vars, "$dsflag", "1");
	}
	else
	{
		sprintf(buf, "%d", len);
		loc_set_zd_data(vars, "#text", text);
		loc_set_zd_data(vars, "#textlen", buf);
	}
	free(text);
	pkg_buf->len += len;

	return SW_OK;
}

/*** 8583报文拆包 ***/
sw_int_t pkg_8583_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag)
{
	sw_int_t	ret = 0;
	sw_char_t	filename[128];
	sw_buf_t	buf;
	sw_buf_t	*pkg_buf = NULL;
	sw_char_t	first_name[64];
	sw_xmltree_t	*xml = NULL;
	
	memset(filename, 0x0, sizeof(filename));
	memset(first_name, 0x0, sizeof(first_name));

	buf.size = len;
	buf.len = 0;
	buf.data = pkg;
	pkg_buf = &buf;

	if (vars == NULL || pkg == NULL || xmlname == NULL || len <= 0)
	{
		pub_log_error("[%s][%d] Param error, null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (cache_flag == 1)
	{
		xml = (sw_xmltree_t *)xmlname;
	}
	else
	{
		memset(filename, 0x0, sizeof(filename));
		sprintf(filename, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(filename);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",
				__FILE__, __LINE__, buf);
			return SW_ERROR;
		}
	}
	
	/*** DEAL FIRSTWORK ***/
	ret = pkg_deal_firstwork_in(vars, xml, pkg_buf, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal FIRSTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACK HEAD ***/
	ret = pkg_deal_head_in(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE BODY***/
	ret = pkg_deal_8583_package_in(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal PACKAGE error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*** DEAL LASTWORK ***/
	ret = pkg_deal_last_work_in(vars, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal LASTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Unpack success!", __FILE__, __LINE__);
	if (cache_flag != 1)
	{
		pub_xml_deltree(xml);
	}
	
	return 0;
}

sw_int_t pkg_com_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag)
{
	sw_int_t	ret = 0;
	sw_buf_t	buf;
	sw_buf_t	*pkg_buf = NULL;
	sw_char_t	filename[128];
	sw_char_t	first_name[64];
	sw_xmltree_t	*xml = NULL;
	
	memset(filename, 0x0, sizeof(filename));
	memset(first_name, 0x0, sizeof(first_name));
	
	buf.size = len;
	buf.len = 0;
	buf.data = pkg;
	pkg_buf = &buf;

	if (vars == NULL || pkg == NULL || xmlname == NULL || len <= 0)
	{
		pub_log_error("[%s][%d] Param error, null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (cache_flag == 1)
	{
		xml = (sw_xmltree_t *)xmlname;
	}
	else
	{
		memset(filename, 0x0, sizeof(filename));
		sprintf(filename, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(filename);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",
				__FILE__, __LINE__, buf);
			return SW_ERROR;
		}
	}
	
	/*** DEAL FIRSTWORK ***/
	ret = pkg_deal_firstwork_in(vars, xml, pkg_buf, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal FIRSTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACK HEAD ***/
	ret = pkg_deal_head_in(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE BODY***/
	ret = pkg_deal_com_package_in(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal PACKAGE error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*** DEAL LASTWORK ***/
	ret = pkg_deal_last_work_in(vars, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal LASTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Unpack success!", __FILE__, __LINE__);
	if (cache_flag != 1)
	{
		pub_xml_deltree(xml);
	}
	
	return 0;
}

sw_int_t pkg_xml_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag)
{
	sw_int_t	ret = 0;
	sw_buf_t	buf;
	sw_buf_t	*pkg_buf = NULL;
	sw_char_t	filename[128];
	sw_char_t	first_name[64];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	memset(filename, 0x0, sizeof(filename));
	memset(first_name, 0x0, sizeof(first_name));
	
	buf.size = len;
	buf.len = 0;
	buf.data = pkg;
	pkg_buf = &buf;

	if (vars == NULL || pkg == NULL || xmlname == NULL || len <= 0)
	{
		pub_log_error("[%s][%d] Param error, null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (cache_flag == 1)
	{
		xml = (sw_xmltree_t *)xmlname;
	}
	else
	{
		memset(filename, 0x0, sizeof(filename));
		sprintf(filename, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(filename);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",
				__FILE__, __LINE__, buf);
			return SW_ERROR;
		}
	}
	
	/*** DEAL FIRSTWORK ***/
	ret = pkg_deal_firstwork_in(vars, xml, pkg_buf, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal FIRSTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACK HEAD ***/
	ret = pkg_deal_head_in(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE BODY***/
	ret = pkg_deal_com_package_in(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal PACKAGE error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** GET DIGITAL SIGN ***/
	ret = pkg_get_digital_sign(vars, pkg_buf);
	if (ret != SW_OK)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Get digital sign error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL XML BODY ***/
	vars->tree = pub_xml_unpack_ext(pkg_buf->data + pkg_buf->len, pkg_buf->size - pkg_buf->len);
	
	/*** DEAL XML MAP ***/
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.MAP");
	if (node != NULL)
	{
		g_clear_xml_vars = 1;
		ret = pub_xml_convert_analyze(vars, xml);
		if (ret < 0)
		{
			if (cache_flag != 1)
			{
				pub_xml_deltree(xml);
			}
			pub_log_error("[%s][%d] xml map error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	/*** DEAL LASTWORK ***/
	ret = pkg_deal_last_work_in(vars, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal LASTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Unpack success!", __FILE__, __LINE__);
	if (cache_flag != 1)
	{
		pub_xml_deltree(xml);
	}
	
	return 0;
}

int pkg_clear_vars(sw_loc_vars_t *vars)
{
	int	i = 0;
	int	j = 0;
	
	for (i = 0; i < g_vars_clear_package_cnt; i++)
	{
		for (j = 0; j < g_vars_clear_packages[i].count; j++)
		{
			vars->remove_var(vars, g_vars_clear_packages[i].varnames[j]);
			pub_log_debug("[%s][%d] Clear vars [%s]!", __FILE__, __LINE__, g_vars_clear_packages[i].varnames[j]);
		}
	}
	
	if (g_clear_xml_vars == 1 && vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
		vars->tree = NULL;
		pub_log_debug("[%s][%d] Clear xmlvar_tree", __FILE__, __LINE__);
	}

	g_clear_xml_vars = 0;
	g_vars_clear_package_cnt = 0;
	memset(g_vars_clear_packages, 0x0, sizeof(g_vars_clear_packages));
	return 0;
}

