#include "pack.h"

int pkg_check_include(sw_loc_vars_t *vars, char *name, char *check_value)
{
	char	value[256];
	char	varname[64];
	
	memset(value, 0x0, sizeof(value));
	memset(varname, 0x0, sizeof(varname));
	
	if (name == NULL || name[0] == '\0')
	{
		strncpy(varname, "#tx_code", sizeof(varname) - 1);
	}
	else
	{
		strncpy(varname, name, sizeof(varname) - 1);
	}

	pub_comvars_get(vars, varname, value);
	if (pub_str_include(value, check_value) == 0)
	{
		return 0;
	}
	
	return -1;
}

int pkg_check_single(sw_loc_vars_t *vars, char *check_var)
{
	char	*p = NULL;
	char	*ptr = NULL;
	char	sign[4];
	char	name[128];
	char	value[256];
	char	check_value[256];
	
	memset(sign, 0x0, sizeof(sign));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	memset(check_value, 0x0, sizeof(check_value));
	
	ptr = check_var;
	p = name;
	while (*ptr != '\0' && *ptr != '=' && *ptr != '?')
	{
		*p++ = *ptr++;
	}
	*p = '\0';
	
	if (*ptr == '\0')
	{
		pub_log_error("[%s][%d] CHECK表达式[%s]有误!",
			__FILE__, __LINE__, check_var);
		return -1;
	}
	sign[0] = *ptr;
	ptr++;
	strcpy(check_value, ptr);

	if (sign[0] == '=')
	{
		if (pkg_check_include(vars, name, check_value) == 0)
		{
			return 0;
		}
		return -1;
	}
	
	if (sign[0] == '?')
	{
		if (pkg_check_include(vars, name, check_value) == 0)
		{
			return -1;
		}
		return 0;
	}
	
	return -1;
}

int pkg_check_mult(sw_loc_vars_t *vars, sw_xmlnode_t *check_node)
{
	sw_xmlnode_t	*node = NULL;
	
	node = check_node;
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}
		if (pkg_check_single(vars, node->value) != 0)
		{
			return -1;
		}
		node = node->next;
	}
	
	return 0;
}

void pkg_item_init(sw_pkg_item_t *item)
{
	memset(item, 0x0, sizeof(sw_pkg_item_t));
	item->begin = 0;
	item->length = 0;
	item->var_length = 0;
	item->start = 0;
	item->num = 0;
	item->max = 0;
	item->index = 0;
	item->large = 0;
	item->offset = 0;
	item->is_null = 0;
	item->type = '\0';
	item->flag_const = '\0';
	item->blank = '\0';
	item->flag = '\0';
	item->loop = '\0';
	item->key = '\0';
	item->var_value = item->value;
}

sw_int_t pkg_get_com_item(sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_pkg_item_t *item)
{
	char	asc_sep[16];
	char	bcd_sep[16];
	sw_xmlnode_t	*node = NULL;
	
	memset(asc_sep, 0x0, sizeof(asc_sep));
	memset(bcd_sep, 0x0, sizeof(bcd_sep));
	
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] node is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	xml->current = pnode;
	node = pub_xml_locnode(xml, "NAME");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(item->var_name, node->value);
	
	node = pub_xml_locnode(xml, "BEGIN");
	if (node != NULL && node->value != NULL)
	{
		item->begin = atoi(node->value);
	}
	
	node = pub_xml_locnode(xml, "LENGTH");
	if (node != NULL && node->value != NULL)
	{
		if (node->value[0] != '#' && node->value[0] != '$')
		{
			item->length = atoi(node->value);
		}
		else
		{
			strncpy(item->len_varname, node->value, sizeof(item->len_varname) - 1);
		}
	}
	
	node = pub_xml_locnode(xml, "TYPE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->var_type, node->value);
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
			strncpy(item->sep, bcd_sep, strlen(asc_sep) / 2);
		}
		else
		{
			strcpy(item->sep, node->value);
		}
	}
	
	node = pub_xml_locnode(xml, "MIDDLE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->middle, node->value);
	}
	
	node = pub_xml_locnode(xml, "BLANK");
	if (node != NULL && node->value != NULL)
	{
		item->blank = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "FLAG");
	if (node != NULL && node->value != NULL)
	{
		item->flag = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "CONST");
	if (node != NULL && node->value != NULL)
	{
		item->flag_const = '1';
		item->length = pub_str_strcpy_ext(item->const_value, node->value);
	}
	
	node = pub_xml_locnode(xml, "START");
	if (node != NULL && node->value != NULL)
	{
		item->start = atoi(node->value);
	}
	
	node = pub_xml_locnode(xml, "CODE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->code, node->value);
	}
	
	node = pub_xml_locnode(xml, "LENCODE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->len_code, node->value);
	}
	
	node = pub_xml_locnode(xml, "LOOP");
	if (node != NULL && node->value != NULL)
	{
		item->loop = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "STRUCT");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->stuct, node->value);
	}
	
	node = pub_xml_locnode(xml, "IMF");
	if (node != NULL && node->value != NULL)
	{
		item->imf = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "OFFSET");
	if (node != NULL && node->value != NULL)
	{
		item->offset = atoi(node->value);
	}
	
	node = pub_xml_locnode(xml, "KEY");
	if (node != NULL && node->value != NULL)
	{
		item->key = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "FIX");
	if (node != NULL && node->value != NULL)
	{
		item->fix = atoi(node->value);
	}
	
	node = pub_xml_locnode(xml, "ISTAG");
	if (node != NULL && node->value != NULL)
	{	
		item->istag = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "TAG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(item->tag, node->value, sizeof(item->tag) - 1);
	}
	
	return SW_OK;
}

sw_int_t pkg_get_8583_item(sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_pkg_item_t *item)
{
	sw_xmlnode_t	*node = NULL;
	
	if (strcmp(pnode->name, "ITEM") != 0)
	{
		pub_log_error("[%s][%d] Not ITEM!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	xml->current = pnode;
	node = pub_xml_locnode(xml, "NAME");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(item->var_name, node->value);
	
	node = pub_xml_locnode(xml, "TYPE");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config TYPE!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(item->var_type, node->value);
	
	node = pub_xml_locnode(xml, "INDEX");
	if (node != NULL && node->value != NULL)
	{
		item->index = atoi(node->value);
	}
	
	node = pub_xml_locnode(xml, "BEGIN");
	if (node != NULL && node->value != NULL)
	{
		item->begin = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "LENGTH");
	if (node != NULL && node->value != NULL)
	{
		item->length = atoi(node->value);
		item->var_length = item->length;
	}
	
	node = pub_xml_locnode(xml, "BITMAP");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->bit_name, node->value);
	}
	
	node = pub_xml_locnode(xml, "CONST");
	if (node != NULL && node->value != NULL)
	{
		item->flag_const = '1';
		item->length = pub_str_strcpy_ext(item->const_value, node->value);
	}
	
	node = pub_xml_locnode(xml, "BLANK");
	if (node != NULL && node->value != NULL)
	{
		item->blank = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "FLAG");
	if (node != NULL && node->value != NULL)
	{
		item->flag = node->value[0];
	}
	
	node = pub_xml_locnode(xml, "CODE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->code, node->value);
	}
	
	node = pub_xml_locnode(xml, "LENCODE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->len_code, node->value);
	}
	
	node = pub_xml_locnode(xml, "STRUCT");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->stuct, node->value);
	}
	
	node = pub_xml_locnode(xml, "FVALUE");
	if (node != NULL && node->value != NULL)
	{
		item->fvalue = node->value[0];
	}
	
	return SW_OK;
}

int pkg_get_len_by_sep(sw_buf_t *pkg_buf, char *sep)
{
	int	var_len = 0;
	int	sep_len = 0;
	char	*ptr = pkg_buf->data + pkg_buf->len;
	unsigned char	ch1;
	unsigned char	ch2;
	
	sep_len = strlen(sep);
	var_len = 0;
	while (pkg_buf->size - pkg_buf->len > var_len)
	{
		
		ch1 = (unsigned char)*ptr;
		ch2 = (unsigned char)*(ptr + 1);
		if ((ch1 >= 0xB0 && ch1 <= 0xF7 && ch2 >= 0xA0 && ch2 <= 0xFE) ||
			(ch1 >= 0x81 && ch1 <= 0xFE && ch2 >= 0x40 && ch2 <= 0xFE))
		{
			/*** 汉字 ***/
			ptr += 2;
			var_len += 2;
			continue;
		}

		if (memcmp(ptr, sep, sep_len) == 0)
		{
			return var_len;
		}
		ptr++;
		var_len++;
	}
	return var_len;
}

int pkg_get_value_by_sep(sw_buf_t *pkg_buf, int num, char *sep, char *value)
{
	int	cnt = 0;
	int	var_len = 0;
	int	sep_len = 0;
	char	*p = value;
	char	*ptr = pkg_buf->data + pkg_buf->len;
	char	*dst = value;
	unsigned char	ch1;
	unsigned char	ch2;
	
	if (num <= 0)
	{
		num = 1;
	}
	sep_len = strlen(sep);
	var_len = 0;
	while (cnt < num && pkg_buf->size - pkg_buf->len > var_len)
	{
		
		ch1 = (unsigned char)*ptr;
		ch2 = (unsigned char)*(ptr + 1);
		if ((ch1 >= 0xB0 && ch1 <= 0xF7 && ch2 >= 0xA0 && ch2 <= 0xFE) ||
			(ch1 >= 0x81 && ch1 <= 0xFE && ch2 >= 0x40 && ch2 <= 0xFE))
		{
			/*** 汉字 ***/
			*p++ = *ptr++;
			*p++ = *ptr++;
			var_len += 2;
			continue;
		}

		if (memcmp(ptr, sep, sep_len) == 0)
		{
			if (cnt == num - 1)
			{
				*p = '\0';
				return var_len;
			}
			ptr += sep_len;
			cnt++;
			memset(dst, 0x0, var_len);
			p = dst;
			var_len = 0;
			continue;
		}
		*p++ = *ptr++;
		var_len++;
	}
	if (cnt == num - 1)
	{
		*p = '\0';
		return var_len;
	}
	value[0] = '\0';
	
	return 0;
}

sw_int_t pkg_get_value(sw_buf_t *pkg_buf, int begin, int length, char *sep, char *value)
{
	sw_int_t	len = 0;
	
	if (pkg_buf->len >= pkg_buf->size)
	{
		value[0] = '\0';
		return 0;
	}

	if (sep == NULL || sep[0] == '\0')
	{
		if (pkg_buf->len + length > pkg_buf->size)
		{
			value[0] = '\0';
			return 0;
		}

		if (length == 0)
		{
			value[0] = '\0';
		}
		else if (begin > 0)
		{
			memcpy(value, pkg_buf->data + begin, length);
			pkg_buf->len = begin + length;
		}
		else
		{
			memcpy(value, pkg_buf->data + pkg_buf->len, length);
			pkg_buf->len += length;
		}
	}
	else
	{
		len = pkg_get_value_by_sep(pkg_buf, 1, sep, value);
		pkg_buf->len += len + strlen(sep);
	}
	
	return len;
}

int pkg_add_value(sw_buf_t *pkg_buf, char *value, int length)
{
	memcpy(pkg_buf->data + pkg_buf->len, value, length);
	pkg_buf->len += length;
	
	return SW_OK;
}

int pkg_check_index(sw_bitmap_t *bitmap, int index, char *bitname)
{
	int	i = 0;
	
	for (i = 0; i < 3; i++)
	{
		if (strcmp(bitmap->bits[i].name, bitname) == 0)
		{
			index = index - 64 * i;
			if (bitmap->bits[i].bitmap[index - 1] == '0')
			{
				return 0;
			}
			else if (bitmap->bits[i].bitmap[index - 1] == '1')
			{
				return 1;
			}
			else
			{
				pub_log_error("[%s][%d] bit=[%c]", 
					__FILE__, __LINE__, bitmap->bits[i].bitmap[index - 1]);
				return -1;
			}
		}
		else if (bitmap->bits[i].name[0] == '\0')
		{
			pub_log_debug("[%s][%d] No bitmap! index=[%d]", __FILE__, __LINE__, i);
			return 0;
		}
	}
	pub_log_error("[%s][%d] Check index error! index=[%d] bitname=[%s]", 
		__FILE__, __LINE__, index, bitname);

	return -1;
}

/*** #fd2012[1, 3] ***/
int pkg_get_single_exp_value(sw_loc_vars_t *vars, char *str, char *value)
{
	int	begin = 0;
	int	end = 0;
	int	quto = 0;
	char	*p = NULL;
	char	*q = NULL;
	char	*ptr = str;
	char	exp[256];
	char	cbegin[128];
	char	cend[128];
	char	tmpvalue[256];
	
	memset(cbegin, 0x0, sizeof(cbegin));
	memset(cend, 0x0, sizeof(cend));
	memset(exp, 0x0, sizeof(exp));
	memset(tmpvalue, 0x0, sizeof(tmpvalue));

	p = exp;
	while (*ptr != '\0' && *ptr == ' ')
	{
		ptr++;
	}
	
	while (*ptr != '\0' && *ptr != '[')
	{
		*p++ = *ptr++;
	}
	*p = '\0';
	
	if (exp[0] == '\'')
	{
		strncpy(value, exp + 1, strlen(exp) - 2);
		return 0;
	}
	
	if (*ptr == '[')
	{
		memset(cbegin, 0x0, sizeof(begin));
		memset(cend, 0x0, sizeof(cend));
		p = cbegin;
		q = cend;
		quto = 0;
		ptr++;
		while (*ptr != '\0' && *ptr != ']')
		{
			if (*ptr == ' ')
			{
				ptr++;
				continue;
			}

			if (*ptr == ',')
			{
				quto = 1;
				ptr++;
				continue;
			}
			
			if (quto == 0)
			{
				*p++ = *ptr++;
			}
			else
			{
				*q++ = *ptr++;
			}
		}
		*p = '\0';
		*q = '\0';
		begin = atoi(cbegin);
		end = atoi(cend);
	}
	pub_comvars_get(vars, exp, tmpvalue);
	if (end > 0)
	{
		strncpy(value, tmpvalue + begin, end - begin + 1);
	}
	else
	{
		strcpy(value, tmpvalue);
	}
	
	return 0;
}

/*** #fd2006[1, 3] + #fd2001[2, 3] + #fd1001[3, 4] ***/
int pkg_get_exp_value(sw_loc_vars_t *vars, char *str, char *out)
{
	char	*p = NULL;
	char	*ptr = str;
	char	exp[256];
	char	value[1024];
	char	tmpvalue[1024];
	
	memset(exp, 0x0, sizeof(exp));
	memset(value, 0x0, sizeof(value));
	memset(tmpvalue, 0x0, sizeof(tmpvalue));

	p = exp;
	while (*ptr != '\0')
	{
		if (*ptr == '+')
		{
			ptr++;
			*p = '\0';
			memset(tmpvalue, 0x0, sizeof(tmpvalue));
			pkg_get_single_exp_value(vars, exp, tmpvalue);
			strcat(value, tmpvalue);
			memset(exp, 0x0, sizeof(exp));
			p = exp;
		}
		else
		{
			*p++ = *ptr++;
		}
	}
	memset(tmpvalue, 0x0, sizeof(tmpvalue));
	pkg_get_single_exp_value(vars, exp, tmpvalue);
	strcat(value, tmpvalue);
	strcpy(out, value);

	return 0;
}

int setbitmap(sw_bitmap_t *bitmap, char *bitname, int index)
{
	int	i = 0;
	int	mod = 0;
	int	quot = 0;
	char	buf[24];
	
	memset(buf, 0x0, sizeof(buf));
	while (index > 64)
	{
		index -= 64;
	}

	for (i = 0; i < 3; i++)
	{
		if (strcmp(bitmap->bits[i].name, bitname) == 0)
		{
			quot = index / 8;
			mod = index % 8;
			switch (mod)
			{
			case 0:
				bitmap->bits[i].bit_value[quot - 1] |= 0x01;
				break;
			case 1:
				bitmap->bits[i].bit_value[quot] |= 0x80;
				break;
			case 2:
				bitmap->bits[i].bit_value[quot] |= 0x40;
				break;
			case 3:
				bitmap->bits[i].bit_value[quot] |= 0x20;
				break;
			case 4:
				bitmap->bits[i].bit_value[quot] |= 0x10;
				break;
			case 5:
				bitmap->bits[i].bit_value[quot] |= 0x08;
				break;
			case 6:
				bitmap->bits[i].bit_value[quot] |= 0x04;
				break;
			case 7:
				bitmap->bits[i].bit_value[quot] |= 0x02;
				break;
			default:
				break;
			}
			
			if (bitmap->bits[i].blen == 2)
			{
				pub_code_bcdtoasc((char *)bitmap->bits[i].bit_value, buf, 8);
				memcpy(bitmap->bitvalue + 16 * i, buf, 16);
			}
			else
			{
				memcpy(bitmap->bitvalue + 8 * i, bitmap->bits[i].bit_value, 8);
			}
			return 0;
		}

	}

	return -1;
}

int get_pkg_len(char *exp, int *begin, int *end)
{
	int     flag = 0;
	char    *p = NULL;
	char    *q = NULL;
	char    *ptr = exp;
	char    exp1[128];
	char    exp2[128];

	memset(exp1, 0x0, sizeof(exp1));
	memset(exp2, 0x0, sizeof(exp2));
	p = exp1;
	q = exp2;

	if (*ptr != '[')
	{
		return -1;
	}

	ptr++;
	while (*ptr != '\0')
	{
		if (*ptr == '.')
		{
			if (flag == 0)
			{
				flag = 1;
			}
			ptr++;
			continue;
		}
	
		if (flag == 0)
		{
			*p++ = *ptr++;
		}		
		else
		{
			*q++ = *ptr++;
		}
	}
	*p = '\0';
	*q = '\0';
	*begin = atoi(exp1);
	*end = atoi(exp2);
	
	return 0;
}

int zip_last_0x0(sw_buf_t *pkg_buf)
{
	while (pkg_buf->len > 0)
	{
		if (pkg_buf->data[pkg_buf->len - 1] != 0x0)
		{
			return SW_OK;
		}
		pkg_buf->len--;
	}
	
	return SW_OK;
}

int alloc_value(sw_pkg_item_t *item)
{
	if (item->max > PKG_VAR_VALUE_MAX_LEN)
	{
		item->var_value = (char *)calloc(1, item->max + 1);
		if (item->var_value == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		item->large = 1;
		pub_log_info("[%s][%d] 变量[%s]过长,动态分配!", __FILE__, __LINE__, item->var_name);
        }
	
	return SW_OK;
}

int release_value(sw_pkg_item_t *item)
{
	if (item->large == 1)
	{
		pub_log_info("[%s][%d] 准备释放[%s]动态内存!", __FILE__, __LINE__, item->var_name);
		free(item->var_value);
		item->var_value = NULL;
		item->large = 0;
		pub_log_info("[%s][%d] 释放变量[%s]动态内存成功!", __FILE__, __LINE__, item->var_name);
	}
	
	return SW_OK;
}

sw_int_t cache_init(sw_pkg_cache_list_t *list)
{
	int	i = 0;
	int	j = 0;
	sw_pkg_processor_t	*processor = NULL;
	
	return SW_OK;
	
	for (i = 0; i < PKG_MAX_PROCESSOR_CNT; i++)
	{
		processor = &list->processor[i];
		memset(processor, 0x0, sizeof(sw_pkg_processor_t));
		processor->type = 0;
		processor->dlflag = 0;
		processor->handle = NULL;
		processor->dlfun = NULL;
		processor->xml = NULL;
		processor->pkg_check.check_cnt = 0;
		for (j = 0; j < PKG_MAX_CHECK_CNT; j++)
		{
			memset(&processor->pkg_check, 0x0, sizeof(sw_pkg_check_t));
			processor->pkg_check.check[j].begin = 0;
			processor->pkg_check.check[j].length = 0;
			processor->pkg_check.check[j].dlflag = 0;
			processor->pkg_check.check[j].handle = NULL;
			processor->pkg_check.check[j].dlfun = NULL;
		}
	}
	
	return SW_OK;
}

sw_int_t find_cache_index_by_name(sw_pkg_cache_list_t *list, sw_char_t *name, sw_char_t *cfg, int type)
{
	int	i = 0;
	
	if (list->cnt == 0)
	{
		cache_init(list);
		return -1;
	}
	
	for (i = 0; i < list->cnt; i++)
	{
		if (strcmp(list->processor[i].name, name) == 0 && list->processor[i].type == type
			&& strcmp(list->processor[i].cfg, cfg) == 0)
		{
			return i;
		}
	}
	
	return -1;
}

sw_int_t add_cache(sw_pkg_cache_list_t *list, sw_pkg_processor_t *processor)
{
	if (list->cnt == 0)
	{
		cache_init(list);
	}
	
	memcpy(&list->processor[list->cnt], processor, sizeof(sw_pkg_processor_t));
	list->cnt++;
	 
	return SW_OK;
}

sw_int_t pkg_get_lib(sw_char_t *lib, sw_char_t *fun, void **hd, sw_com_pt *df)
{
	void	*handle = NULL;
	sw_com_pt	dlfun = NULL;
	sw_char_t	libso[128];
	
	memset(libso, 0x0, sizeof(libso));
	
	sprintf(libso, "%s/plugin/%s", getenv("SWWORK"), lib);
	if (!pub_file_exist(libso))
	{
		memset(libso, 0x0, sizeof(libso));
		sprintf(libso, "%s/plugin/%s", getenv("SWHOME"), lib);
		if (!pub_file_exist(libso))
		{
			pub_log_error("[%s][%d] libso [%s] is not exist!", __FILE__, __LINE__, libso);
			return SW_ERROR;
		}
	}
	
	handle = (void *)dlopen(libso, RTLD_LAZY | RTLD_GLOBAL);
	if (handle == NULL)
	{
		pub_log_error("[%s][%d] dlopen [%s] error! error:[%s]", __FILE__, __LINE__, libso, dlerror());
		return SW_ERROR;
	}

	dlfun = (sw_int_t (*)())dlsym(handle, fun);
	if (dlfun == NULL)
	{
		dlclose(handle);
		pub_log_error("[%s][%d] dlsym [%s][%s] error! error:[%s]",
			__FILE__, __LINE__, libso, fun, dlerror());
		return SW_ERROR;
	}
	*hd = handle;
	*df = dlfun;
	
	return SW_OK;
}

sw_int_t get_processor_info(sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_pkg_processor_t *processor)
{
	char	*ptr = NULL;
	sw_int_t	ret = 0;
	sw_char_t	cfg[PATH_NAME_MAX_LEN];
	check_t	*check = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	xml->current = pnode;
	node = pub_xml_locnode(xml, "NAME");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(processor->name, node->value);
	
	node = pub_xml_locnode(xml, "CFG");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] 未配置CFG标签!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(processor->cfg, node->value, sizeof(processor->cfg));
	sprintf(cfg, "%s/cfg/common/%s", getenv("SWWORK"), node->value);
	processor->xml = cfg_read_xml(cfg);
	if (processor->xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree errro! xmlname=[%s]",
			__FILE__, __LINE__, cfg);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, "LIB");
	if (node != NULL && node->value != NULL)
	{
		strcpy(processor->lib, node->value);
		
	}
	
	node = pub_xml_locnode(xml, "FUNCTION");
	if (node != NULL && node->value != NULL)
	{
		strcpy(processor->fun, node->value);
		if (processor->lib[0] == '\0')
		{
			pub_log_error("[%s][%d] 未配置LIB!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		ret = pkg_get_lib(processor->lib, processor->fun, &processor->handle, &processor->dlfun);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Get lib error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	processor->pkg_check.check_cnt = 0;
	node = pub_xml_locnode(xml, "CHECK");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}
		
		check = &processor->pkg_check.check[processor->pkg_check.check_cnt];
		xml->current = node;
		node1 = pub_xml_locnode(xml, "BEGIN");
		if (node1 != NULL && node1->value != NULL)
		{
			check->begin = atoi(node1->value);
		}
		
		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			check->length = atoi(node1->value);
		}
		
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strcpy(check->name, node1->value);
		}
	
		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 != NULL && node1->value != NULL)
		{
			strcpy(check->value, node1->value);
		}
	
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			strcpy(check->check_value, node1->value);
		}
	
		node1 = pub_xml_locnode(xml, "FUNCTION");
		if (node1 != NULL && node1->value != NULL)
		{
			ptr = strchr(node1->value, '/');
			if (ptr != NULL)
			{
				strncpy(check->lib, node1->value, ptr - node1->value);
				strcpy(check->fun, ptr + 1);
			}
			else
			{
				pub_log_error("[%s][%d] FUNCTION中未填写库名!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			
			ret = pkg_get_lib(check->lib, check->fun, &check->handle, &check->dlfun);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Get lib error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
		
		node = node->next;
		
		processor->pkg_check.check_cnt++;
	}
	
	return SW_OK;
}

sw_int_t pkg_get_processor_index_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_int_t len, sw_pkg_cache_list_t *list)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;
	sw_char_t	value[512];
	check_t	*check = NULL;
	sw_pkg_processor_t	*processor = NULL;
	
	for (i = 0; i < list->cnt; i++)
	{
		if (list->processor[i].type == PKG_OUT)
		{
			continue;
		}
		
		processor = &list->processor[i];
		for (j = 0; j < processor->pkg_check.check_cnt; j++)
		{
			memset(value, 0x0, sizeof(value));
			check = &processor->pkg_check.check[j];
			if (check->dlfun != NULL)
			{
				ret = ((sw_pkg_check_in_pt)check->dlfun)(pkg, len);
				if (ret != 0)
				{
					break;
				}
			}

			if (check->name[0] != '\0')
			{
				memset(value, 0x0, sizeof(value));
				loc_get_zd_data(vars, check->name, value);
				pub_log_info("[%s][%d] value=[%s] include=[%s]",
					__FILE__, __LINE__, value, check->check_value);
				if (pub_str_include(value, check->check_value) != 0)
				{
					break;
				}
			}
			
			if (check->begin >= 0 && check->length > 0)
			{
				memset(value, 0x0, sizeof(value));
				memcpy(value, pkg + check->begin, check->length);
				pub_log_info("[%s][%d] value=[%s] include=[%s]",
					__FILE__, __LINE__, value, check->check_value);
				if (pub_str_include(value, check->check_value) != 0)
				{
					break;
				}
			}
		}
		
		if (j == processor->pkg_check.check_cnt)
		{
			pub_log_info("[%s][%d] processorname=[%s]", __FILE__, __LINE__, processor->name);
			return i;
		}
	}
	
	return -1;
}

sw_int_t pkg_get_processor_index_out(sw_loc_vars_t *vars, sw_pkg_cache_list_t *list)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;
	sw_char_t	value[512];
	check_t	*check = NULL;
	sw_pkg_processor_t	*processor = NULL;
	
	for (i = 0; i < list->cnt; i++)
	{
		if (list->processor[i].type == PKG_IN)
		{
			continue;
		}
		
		processor = &list->processor[i];
		for (j = 0; j < processor->pkg_check.check_cnt; j++)
		{
			memset(value, 0x0, sizeof(value));
			check = &processor->pkg_check.check[j];
			if (check->dlfun != NULL)
			{
				ret = ((sw_pkg_check_out_pt)check->dlfun)(vars);
				if (ret != 0)
				{
					break;
				}
			}
			
			if (check->name[0] != '\0')
			{
				memset(value, 0x0, sizeof(value));
				loc_get_zd_data(vars, check->name, value);
				pub_log_info("[%s][%d] value=[%s] include=[%s]",
					__FILE__, __LINE__, value, check->check_value);
				if (pub_str_include(value, check->check_value) != 0)
				{
					break;
				}
			}
		}
		
		if (j == processor->pkg_check.check_cnt)
		{
			return i;
		}
	}
	
	return -1;
}

sw_int_t pkg_get_processor_info_in(sw_xmltree_t *xml, sw_pkg_cache_list_t *list)
{
	sw_int_t	ret = 0;
	sw_int_t	index = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_pkg_processor_t	*processor = NULL;
	
	node = pub_xml_locnode(xml, ".CBM.TYPE_ALYZ.ANALYZE.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		node2 = pub_xml_locnode(xml, "CFG");
		if (node2 == NULL || node2->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置CFG标签!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		index = find_cache_index_by_name(list, node1->value, node2->value, PKG_IN);
		if (index == -1)
		{
			processor = &list->processor[list->cnt];
			processor->type = PKG_IN;
			ret = get_processor_info(xml, node, processor);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Get processor[%s] info error!",
					__FILE__, __LINE__, node1->value);
				return SW_ERROR;
			}
			
			if (processor->dlfun == NULL)
			{
				if (strcmp(processor->name, "8583_processor") == 0)
				{
					processor->dlfun = pkg_8583_in;
				}
				else if (strcmp(processor->name, "xml_processor") == 0)
				{
					processor->dlfun = pkg_xml_in;
				}
				else if (strcmp(processor->name, "sep_processor") == 0)
				{
					processor->dlfun = pkg_com_in;
				}
				else
				{
					pub_log_error("[%s][%d] 暂不处理该类型报文!", __FILE__, __LINE__);
					return SW_ERROR;
				}
			}
			list->cnt++;
		}
	
		node = node->next;
	}
	
	return SW_OK;
}


sw_int_t pkg_get_processor_info_out(sw_xmltree_t *xml, sw_pkg_cache_list_t *list)
{
	sw_int_t	ret = 0;
	sw_int_t	index = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_pkg_processor_t	*processor = NULL;
	
	node = pub_xml_locnode(xml, ".CBM.TYPE_ALYZ.INTEGRATE.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		node2 = pub_xml_locnode(xml, "CFG");
		if (node2 == NULL || node2->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置CFG标签!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		index = find_cache_index_by_name(list, node1->value, node2->value, PKG_OUT);
		if (index == -1)
		{
			processor = &list->processor[list->cnt];
			processor->type = PKG_OUT;
			ret = get_processor_info(xml, node, processor);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Get processor[%s] info error!",
					__FILE__, __LINE__, node1->value);
				return SW_ERROR;
			}
				
			if (processor->dlfun == NULL)
			{
				if (strcmp(processor->name, "8583_processor") == 0)
				{
					processor->dlfun = pkg_8583_out;
				}
				else if (strcmp(processor->name, "xml_processor") == 0)
				{
					processor->dlfun = pkg_xml_out;
				}
				else if (strcmp(processor->name, "sep_processor") == 0)
				{
					processor->dlfun = pkg_com_out;
				}
				else
				{
					pub_log_error("[%s][%d] 暂不处理该类型报文!", __FILE__, __LINE__);
					return SW_ERROR;
				}
			}
			list->cnt++;
		}
	
		node = node->next;
	}
	
	return SW_OK;
}

int pkg_release_list(sw_pkg_cache_list_t *list)
{
	int	i = 0;
	sw_pkg_processor_t      *processor = NULL;
	
	if (list == NULL)
	{
		return 0;
	}

	for (i = 0; i < list->cnt; i++)
	{
		processor = &list->processor[i];
		if (processor->xml != NULL)
		{
			pub_xml_deltree(processor->xml);
		}
	}
	
	return 0;
}
