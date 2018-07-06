#include "pack.h"

extern int zip_last_0x0(sw_buf_t *pkg_buf);
extern sw_int_t pub_xml_convert_integrate( sw_loc_vars_t *pstlocvar, sw_xmltree_t *pxml);
extern int pkg_add_value(sw_buf_t *pkg_buf, char *value, int length);
extern int get_pkg_len(char *exp, int *begin, int *end);
extern int setbitmap(sw_bitmap_t *bitmap, char *bitname, int index);

sw_int_t pkg_deal_com_item_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_pkg_item_t *item, int index)
{
	int	i = 0;
	int	len = 0;
	int	ret = 0;
	char	tmp[64];
	char	buf[PKG_VAR_VALUE_MAX_LEN];
	char	value[PKG_VAR_VALUE_MAX_LEN];
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(buf, 0x0, sizeof(buf));
	memset(value, 0x0, sizeof(value));
	
	if (index >= 0)
	{
		sprintf(buf, "%s(%d)", item->var_name, index);
		memset(item->var_name, 0x0, sizeof(item->var_name));
		strncpy(item->var_name, buf, sizeof(item->var_name) - 1);
	}
	
	if (item->var_type[0] != '\0')
	{
		item->type = item->var_type[0];
		for (i = 1; i < 8; i++)
		{
			if (item->var_type[i] == '.')
			{
				item->num++;
				continue;
			}
			item->max = atoi(item->var_type + 1 + item->num);
			break;
		}
	}
	else
	{
		/*** 自动识别类型 ***/
		memset(buf, 0x0, sizeof(buf));
		item->max = pub_comvars_get_len(vars, item->var_name);;
		if (item->max < 0)
		{
			item->max = 0;
		}
		item->num = 0;
		item->var_length = item->max;
	}
	
	/*** 防止变量过长,如果过长直接动态分配内存 ***/
	alloc_value(item);

	memset(item->var_value, 0x0, sizeof(item->value));
	if (item->flag_const == '1')
	{
		memcpy(item->var_value, item->const_value, sizeof(item->value) - 1);
		item->var_length = strlen(item->var_value);
	}
	else
	{
		item->var_length = pub_comvars_get_len(vars, item->var_name);
		if (item->var_length > item->max)
		{
			release_value(item);
			pub_log_error("[%s][%d] 变量[%s][%d]超长! max=[%d]", 
				__FILE__, __LINE__, item->var_name, item->var_length, item->max);
			return SW_ERROR;
		}
		item->var_length = pub_comvars_get(vars, item->var_name, item->var_value);
		if (item->var_length < 0)
		{
			item->var_length = 0;
		}
		if (item->var_length == 0)
		{
			item->is_null = 1;
		}
	}
	
	/*** 超长直接报错 ***/
	if (item->var_length > item->max)
	{
		release_value(item);
		pub_log_error("[%s][%d] 变量[%s]=[%s]超长! max=[%d]", 
			__FILE__, __LINE__, item->var_name, item->var_value, item->max);
		return SW_ERROR;
	}
	
	/*** 变长时如果变量为空则正常返回 ***/
	if (item->var_length <= 0 && item->num > 0)
	{
		release_value(item);
		item->var_length = 0;
		item->index = 0;
		item->max = 0;
		return SW_OK;
	}
	
	if (item->num == 0)
	{
		if (item->var_length <= 0)
		{
			if (item->flag == '1')
			{
				if (item->blank == '1' || item->blank == '2')
				{
					memset(item->var_value, ' ', item->max);
				}
				else if (item->var_type[0] == 'n')
				{
					memset(item->var_value, '0', item->max);
				}
				else
				{
					memset(item->var_value, 0x0, item->max);
				}
				item->var_length = item->max;
			}
			else
			{
				memset(item->var_value, 0x0, item->max);
				item->var_length = 0;
			}
		}
		else if (item->var_type[0] != '\0' && item->var_length < item->max)
		{
			memset(value, 0x0, sizeof(value));
			memcpy(value, item->var_value, sizeof(value) - 1);
			memset(item->var_value, 0x0, sizeof(item->value));
			len = item->max - item->var_length;
			if (item->blank == '1')
			{
				memcpy(item->var_value, value, item->var_length);
				memset(item->var_value + item->var_length, ' ', len);
			}
			else if (item->blank == '2')
			{
				memset(item->var_value, ' ', len);
				memcpy(item->var_value + len, value, item->var_length);
			}
			else if (item->var_type[0] == 'n')
			{
				memset(item->var_value, '0', len);
				memcpy(item->var_value + len, value, item->var_length);
			}
			else
			{
				memcpy(item->var_value, value, item->var_length);
				memset(item->var_value + item->var_length, 0x0, len);
			}
			item->var_length = item->max;
		}
	} 
	else
	{
		if (strcmp(item->len_code, "HUA") == 0)
		{
			memset(value, 0x0, sizeof(value));
			item->type = 'b';
			if (item->num <= 2)
			{
				value[0] = item->var_length % 256;
				memcpy(value + 1, item->var_value, item->var_length);
				pub_log_info("[%s][%d] HUA:Var[%s]", __FILE__, __LINE__, item->var_value);
				item->var_length += 1;
				memset(item->var_value, 0x0, sizeof(item->value));
				memcpy(item->var_value, value, item->var_length);
				pub_log_info("[%s][%d] HUA:[%s]", __FILE__, __LINE__, item->var_value);
			}
			else
			{
				value[0] = item->var_length / 256;
				value[1] = item->var_length % 256;
				memcpy(value + 2, item->var_value, item->var_length);
				item->var_length += 2;
				memset(item->var_value, 0x0, sizeof(item->value));
				memcpy(item->var_value, value, item->var_length);
			}
		}
		else if (memcmp(item->code, "LBCD", 4) == 0 || memcmp(item->code, "LMBCD", 5) == 0)
		{
			int	varlen = 0;
			char *tmp = NULL;
			char bcdlabel[8];
			
			memset(buf, 0x0, sizeof(buf));
			memset(value, 0x0, sizeof(value));
			if (item->num % 2 == 0)
			{
				varlen = item->num / 2;
			}
			else
			{
				varlen = (item->num + 1) / 2;
			}
			
			memset(value, 0x0, sizeof(value));
			sprintf(value, "%0*d", item->num, item->var_length);
			memset(buf, 0x0, sizeof(buf));
			tmp = strstr(item->code, ":");
			if (tmp != NULL)
			{
				strcpy(bcdlabel, tmp + 1);
				if (memcmp(bcdlabel, "L", 1) == 0)
				{
					pub_code_asctobcd_left(value, buf, item->num);
				}
			}
			else
			{
				pub_code_asctobcd_right(value, buf, item->num);
			}
			
			memset(value, 0x0, sizeof(value));
			memcpy(value, buf, varlen);
		
			if (strcmp(item->code, "LMBCD") == 0)
			{
				if (item->var_length % 2 == 0)
				{
					len = item->var_length / 2;
				}
				else
				{
					len = (item->var_length + 1) / 2;
				}

				memset(buf, 0x0, sizeof(buf));
				if (bcdlabel[0] != '\0' && memcmp(bcdlabel + 1, "R", 1) == 0)
				{
					pub_code_asctobcd_right(item->var_value, buf, item->var_length);
				}
				else
				{
					pub_code_asctobcd_left(item->var_value, buf, item->var_length);
				}
				memcpy(value + varlen, buf, len);
				item->var_length = varlen + len;
				memset(item->var_value, 0x0, item->var_length + 1);
				memcpy(item->var_value, value, item->var_length);
			}
			else
			{
				memset(buf, 0x0, sizeof(buf));
				memcpy(value + varlen, item->var_value, item->var_length);
				item->var_length += varlen; 
				memset(item->var_value, 0x0, item->var_length + 1);
				memcpy(item->var_value, value, item->var_length);	
			}
		}
		else
		{
			memset(value, 0x0, sizeof(value));
			if (strcmp(item->code, "BCD") == 0)
			{
				if (item->var_length % 2 == 0)
				{
					len = item->var_length / 2;
				}
				else
				{
					len = (item->var_length + 1) / 2;
				}
				sprintf(value, "%0*d", item->num, len);
			}
			else
			{
				sprintf(value, "%0*d", item->num, item->var_length);
			}
			memcpy(value + item->num, item->var_value, item->var_length);
			item->var_length += item->num;
			memset(item->var_value, 0x0, sizeof(item->value));
			memcpy(item->var_value, value, item->var_length);
		}
	}
	
	if (strcmp(item->code, "BCD") == 0)
	{
		memset(buf, 0x0, sizeof(buf));
		memset(value, 0x0, sizeof(value));
		if ((item->var_length - item->num) % 2 != 0)
		{
			memcpy(buf + 1, item->var_value + item->num, item->var_length - item->num);
			pub_code_asctobcd(buf, value, item->var_length - item->num + 1);
			len = (item->var_length - item->num + 1) / 2;
		}
		else
		{
			memcpy(buf, item->var_value + item->num, item->var_length - item->num);
			pub_code_asctobcd(buf, value, item->var_length - item->num);
			len = (item->var_length - item->num) / 2;
		}
		memset(item->var_value + item->num, 0x0, item->var_length - item->num);
		memcpy(item->var_value + item->num, value, len);
		item->var_length = len + item->num;
	}
	
	ret = pub_buf_chksize(pkg_buf, pkg_buf->len + item->var_length + strlen(item->sep));
	if (ret < 0)
	{
		release_value(item);
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (item->var_length > 0)
	{
		if (pub_is_filter(item->var_name))
		{
			memset(buf, 0x0, sizeof(buf));
			memset(buf, '*', item->var_length);
			pub_log_info("[%s][%d] 取平台变量[%s]=[%s]", __FILE__, __LINE__, item->var_name, buf);
		}
		else
		{
			pub_log_info("[%s][%d] 取平台变量[%s]=[%s]", __FILE__, __LINE__, item->var_name, item->var_value);
		}
		if (item->imf == '1')
		{
			/*** IMF ***/
			if (item->offset == 0)
			{
				item->offset = 1;
			}
			
			pkg_add_value(pkg_buf, item->var_name + item->offset, strlen(item->var_name) - item->offset);
			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, "%d", item->var_length - item->num);
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "%zd", strlen(buf));
			pkg_add_value(pkg_buf, tmp, strlen(tmp));
			pkg_add_value(pkg_buf, buf, strlen(buf));
			pkg_add_value(pkg_buf, item->var_value + item->num, item->var_length - item->num);
		}
		else if (item->middle[0] != '\0')
		{
			/*** MIDDLE ***/
			pkg_add_value(pkg_buf, item->var_name + item->offset, strlen(item->var_name) - item->offset);
			pkg_add_value(pkg_buf, item->middle, strlen(item->middle));
			pkg_add_value(pkg_buf, item->var_value, item->var_length);
		}
		else if (item->key == '1')
		{
			/*** KEY-VALYE ***/
			pkg_add_value(pkg_buf, "<", 1);
			pkg_add_value(pkg_buf, item->var_name + 1, strlen(item->var_name) - 1);
			pkg_add_value(pkg_buf, ">", 1);
			pkg_add_value(pkg_buf, item->var_value, item->var_length);
		}
		else if (item->tag[0] != '\0')
		{
			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, ":%s:", item->tag);
			pkg_add_value(pkg_buf, buf, strlen(buf));
			pkg_add_value(pkg_buf, item->var_value, item->var_length);
		}
		else
		{
			/*** NORMAL ***/
			pkg_add_value(pkg_buf, item->var_value, item->var_length);
		}
		
	}
	if (item->sep[0] != '\0')
	{
		if (item->middle[0] != '\0' && item->var_length == 0)
		{
			pub_log_info("[%s][%d] middle value is null.", __FILE__, __LINE__);
		}
		else
		{
			if ((item->key == '1' && item->var_length > 0) || (item->middle[0] != '\0' && item->var_value[0] != '\0') || item->key != '1')
			{
				pkg_add_value(pkg_buf, item->sep, strlen(item->sep));
			}
		}
	}
	release_value(item);

	return SW_OK;
}

int is_all_space(char *str)
{
	char	*ptr = str;
	
	while (*ptr != '\0')
	{
		if (*ptr != ' ')
		{
			return 0;
		}
		ptr++;
	}
	
	return 1;
}

int is_all_zero(char *str)
{
	char	*ptr = str;
	
	while (*ptr != '\0')
	{
		if (*ptr != '0')
		{
			return 0;
		}
		ptr++;
	}
	
	return 1;
}

sw_int_t pkg_deal_struct_out(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_buf_t *pkg_buf)
{
	sw_int_t	len = 0;
	sw_int_t	ret = 0;
	sw_int_t	offset = 0;
	sw_int_t	nonspace = 0;
	sw_char_t	buf[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_pkg_item_t	item;
	
	memset(buf, 0x0, sizeof(buf));
	pkg_item_init(&item);
	
	len = pkg_buf->len;
	pub_log_info("[%s][%d] Deal STRUCT out...", __FILE__, __LINE__);
	xml->current = pnode;
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

		pkg_item_init(&item);
		pkg_get_com_item(xml, node, &item);
		if (item.flag == '\0')
		{
			item.flag = '1';
		}
		
		if (item.blank == '\0')
		{
			item.blank = '1';
		}
		ret = pkg_deal_com_item_out(vars, pkg_buf, &item, -1);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		if (item.num > 0 && item.var_length == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, "%0*d", item.num, item.var_length);
			memcpy(pkg_buf->data + pkg_buf->len, buf, item.num);
			pkg_buf->len += item.num;
			item.var_length = item.num;
		}

		offset += item.var_length;
		if (!item.is_null)
		{
			nonspace = offset;
		}
		
		node = node->next;
	}
	memset(pkg_buf->data + len + nonspace, 0x0, offset - nonspace);
	pkg_buf->len -= (offset - nonspace);
	pub_log_info("[%s][%d] Deal STRUCT OVER!", __FILE__, __LINE__);	
	
	return SW_OK;
}

sw_int_t pkg_deal_8583_item_out(vars, xml, pnode, pkg_buf, item, bitmap)
sw_loc_vars_t *vars;
sw_xmltree_t *xml;
sw_xmlnode_t *pnode;
sw_buf_t *pkg_buf;
sw_pkg_item_t *item;
sw_bitmap_t *bitmap;
{
	int	i = 0;
	sw_int_t	len = 0;
	sw_int_t	ret = 0;
	char	buf[128];
	char	value[PKG_VAR_VALUE_MAX_LEN];
	
	memset(buf, 0x0, sizeof(buf));
	memset(value, 0x0, sizeof(value));
	
	if (strcmp(item->var_type, "BITMAP") == 0)
	{
		item->type = 'B';
		bitmap->bits[bitmap->bitnum].blen = 1;
		strcpy(bitmap->bits[bitmap->bitnum].name, item->var_name);
		bitmap->bitnum++;
		item->var_length = item->length;
		memset(item->var_value, 0x0, item->var_length);
	}
	else if (strcmp(item->var_type, "BITMAP2") == 0)
	{
		item->type = 'B';
		bitmap->bits[bitmap->bitnum].blen = 2;
		strcpy(bitmap->bits[bitmap->bitnum].name, item->var_name);
		bitmap->bitnum++;
		item->var_length = item->length;
		memset(item->var_value, 0x0, item->var_length);
	}
	
	if (item->type == 'B')
	{
		if (bitmap->bitnum == 1)
		{
			bitmap->begin = item->begin;
			bitmap->bitlen = item->length;
		}
		
		if (item->index > 0)
		{
			setbitmap(bitmap, item->bit_name, item->index);
		}
		
		memcpy(pkg_buf->data + pkg_buf->len, item->var_value, item->var_length);
		pkg_buf->len += item->var_length;
	
		return 0;
	}
	
	/*** STRUCT类型变量如果指定FVALUE="1"则直接取大域的值,否则去子域的值进行拼装 ***/
	if ((strcasecmp(item->stuct, "TRUE") == 0 || item->stuct[0] == '1') && item->fvalue != '1')
	{
		item->num = 0;
		for (i = 1; i < 8; i++)
		{
			if (item->var_type[i] == '.')
			{
				item->num++;
				continue;
			}
			break;
		}
		/*** 如果是变长,需要将变量前的长度先去掉,如果有子域,则必须定常的 ***/
		len = pkg_buf->len;
		ret = pkg_deal_struct_out(vars, xml, pnode, pkg_buf);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal STRUCT error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		item->var_length = pkg_buf->len - len;
		if (item->num > 0 && item->var_length > 0)
		{
			pub_log_info("[%s][%d] num=[%d] length=[%d]",
				__FILE__, __LINE__, item->num, item->var_length);
			
			memset(value, 0x0, sizeof(value));
			memcpy(value, pkg_buf->data + len, item->var_length);
			if (strcmp(item->code, "LBCD") == 0 || strcmp(item->code, "LMBCD") == 0)
			{
				int	varlen = 0;
				int	textlen = 0;
				char	tmp[256];
				char *tmpstr = NULL;
				char bcdlabel[8];
			
				memset(buf, 0x0, sizeof(buf));
				memset(tmp, 0x0, sizeof(tmp));
				if (item->num % 2 == 0)
				{
					varlen = item->num / 2;
				}
				else
				{	
					varlen = (item->num + 1) / 2;
				}
			
				memset(tmp, 0x0, sizeof(tmp));
				sprintf(tmp, "%0*d", item->num, item->var_length);
				memset(buf, 0x0, sizeof(buf));
				
				tmpstr = strstr(item->code, ":");
				if (tmp != NULL)
				{
					strcpy(bcdlabel, tmpstr + 1);
					if (memcmp(bcdlabel, "L", 1) == 0)
					{
						pub_code_asctobcd_left(tmp, buf, item->num);
					}
				}
				else
				{
					pub_code_asctobcd_right(tmp, buf, item->num);
				}
				
				memcpy(pkg_buf->data + pkg_buf->len, buf, varlen);
				pkg_buf->len = len + varlen;
		
				if (strcmp(item->code, "LMBCD") == 0)
				{
						
					if (item->var_length % 2 == 0)
					{
						textlen = item->var_length / 2;
					}
					else
					{
						textlen = (item->var_length + 1) / 2;
					}
					
					memset(buf, 0x0, sizeof(buf));
					if (bcdlabel[0] != '\0' && memcmp(bcdlabel + 1, "R", 1) == 0)
					{
						pub_code_asctobcd_right(value, buf, item->var_length);
					}
					else
					{
						pub_code_asctobcd_left(value, buf, item->var_length);
					}
					memcpy(pkg_buf->data + pkg_buf->len, buf, textlen);
					pkg_buf->len += textlen;
					
				}
				else
				{
					memcpy(pkg_buf->data + pkg_buf->len, value, item->var_length); 
					pkg_buf->len += item->var_length;	
				}
			}
			else
			{
				memset(buf, 0x0, sizeof(buf));
				sprintf(buf, "%0*d", item->num, item->var_length);
				memset(pkg_buf->data + len, 0x0, item->var_length);
				memcpy(pkg_buf->data + len, buf, item->num);
				pkg_buf->len = len + item->num;
				memcpy(pkg_buf->data + pkg_buf->len, value, item->var_length);
				pkg_buf->len += item->var_length;
			}
		}
	}
	else
	{
		ret = pkg_deal_com_item_out(vars, pkg_buf, item, -1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	if (item->var_length > 0 && item->index > 0)
	{
		setbitmap(bitmap, item->bit_name, item->index);
	}

	return SW_OK;
}

sw_int_t pkg_deal_loop_out(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_xmlnode_t *pnode, sw_buf_t *pkg_buf)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;
	sw_int_t	loop_cnt = 0;
	sw_char_t	buf[128];
	sw_char_t	sep[128];
	sw_char_t	asc_sep[128];
	sw_char_t	bcd_sep[128];
	sw_char_t	break_sep[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_pkg_item_t	item;
	
	memset(buf, 0x0, sizeof(buf));
	memset(sep, 0x0, sizeof(sep));
	memset(asc_sep, 0x0, sizeof(asc_sep));
	memset(bcd_sep, 0x0, sizeof(bcd_sep));
	memset(break_sep, 0x0, sizeof(break_sep));
	pkg_item_init(&item);
	
	pub_log_info("[%s][%d] Deal LOOP out...", __FILE__, __LINE__);
	node = pub_xml_locnode(xml, "LOOPCNT");
	if (node == NULL || node->value == NULL)
	{
		loc_get_zd_data(vars, "#loopcnt", buf);
		pub_log_info("[%s][%d] loopcnt=[%s]", __FILE__, __LINE__, buf);
	}
	else
	{
		loc_get_zd_data(vars, node->value, buf);
		pub_log_info("[%s][%d] LOOPCNT:[%s]=[%s]", __FILE__, __LINE__, node->value, buf);
	}
	loop_cnt = atoi(buf);
	pub_log_info("[%s][%d] Loop cnt:[%s][%d]", __FILE__, __LINE__, buf, loop_cnt);
	
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
	
	node = pub_xml_locnode(xml, "BREAK");
	if (node != NULL && node->value != NULL)
	{
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
	}
	
	for (i = 0; i < loop_cnt; i++)
	{
		pub_log_info("[%s][%d] 第[%d]次循环..", __FILE__, __LINE__, i);
		xml->current = pnode;
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
			ret = pkg_deal_com_item_out(vars, pkg_buf, &item, i);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			
			node = node->next;
		}
		if (sep[0] != '\0')
		{
			pkg_add_value(pkg_buf, sep, strlen(sep));
		}
	}
	if (break_sep[0] != '\0' && strcmp(break_sep, "NULL") != 0)
	{
		pkg_add_value(pkg_buf, break_sep, strlen(break_sep));
	}
	pub_log_info("[%s][%d] Loop over, loop cnt=[%d]", __FILE__, __LINE__, loop_cnt);
	
	return SW_OK;
}

sw_int_t pkg_com_item_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *xml, sw_xmlnode_t *pnode)
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
		ret = pkg_deal_loop_out(vars, xml, pnode, pkg_buf);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal loop error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		ret = pkg_deal_com_item_out(vars, pkg_buf, &item, -1);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int_t pkg_deal_firstwork_out(sw_xmltree_t *xml, sw_char_t *firstname)
{
	sw_xmlnode_t	*node = NULL;
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(firstname, node->value);
	}
	
	return SW_OK;
}

sw_int_t pkg_deal_head_out(vars, pkg_buf, xml, first_name)
sw_loc_vars_t *vars;
sw_buf_t *pkg_buf;
sw_xmltree_t *xml;
char *first_name;
{
	sw_int_t	ret = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.HEAD");
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

			ret = pkg_com_item_out(vars, pkg_buf, xml, node1);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}

			node1 = node1->next;
		}
		node = node->next;
	}

	return SW_OK;
}

sw_int_t pkg_deal_8583_package_out(vars, pkg_buf, xml, first_name)
sw_loc_vars_t *vars;
sw_buf_t *pkg_buf;
sw_xmltree_t *xml;
sw_char_t *first_name;
{
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	txcode[32];
	sw_char_t	respcd[32];
	sw_char_t	respcdname[32];
	sw_char_t	transvalue[256];
	sw_bitmap_t	bitmap;
	sw_pkg_item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(txcode, 0x0, sizeof(txcode));
	memset(respcd, 0x0, sizeof(respcd));
	memset(respcdname, 0x0, sizeof(respcdname));
	memset(transvalue, 0x0, sizeof(transvalue));
	memset(&bitmap, 0x0, sizeof(bitmap));
	bitmap.begin = 0;
	bitmap.bitlen = 0;
	bitmap.bitnum = 0;
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.TRANSCTRL.PACKAGE");
	while (node != NULL)
	{
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, "$product", node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		if (first_name[0] != '\0')
		{
			memset(txcode, 0x0, sizeof(txcode));
			loc_get_zd_data(vars, first_name, txcode);
		}
		else
		{
			memset(txcode, 0x0, sizeof(txcode));
			loc_get_zd_data(vars, "#txcode", txcode);
		}
		pub_log_info("[%s][%d] txcode=[%s]", __FILE__, __LINE__, txcode);
		if (txcode[0] == '\0')
		{
			break;
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
			node2 = pub_xml_locnode(xml, "TRANS");
			if (node2 != NULL && node2->value != NULL)
			{
				if (strcmp(node2->value, txcode) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(xml, "VALUE");
			if (node2 != NULL && node2->value != NULL)
			{
				strncpy(transvalue, node2->value, sizeof(transvalue) - 1);
				pub_log_info("[%s][%d] txcode=[%s] value=[%s]", __FILE__, __LINE__, txcode, transvalue);
			}
			break;
		}
		break;
	}
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.PACKAGE");
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
		
		node1 = pub_xml_locnode(xml, "RESPCD");
		if (node1 != NULL && node1->value != NULL)
		{
			memset(buf, 0x0, sizeof(buf));
			memset(respcd, 0x0, sizeof(respcd));
			loc_get_zd_data(vars, node1->value, buf);
			loc_get_zd_data(vars, "$respcd", respcd);
			if (strlen(buf) == 0 && strcmp(respcd, "0000") != 0)
			{
				memset(buf, 0x0, sizeof(buf));
				memcpy(buf, respcd, 4);
				loc_set_zd_data(vars, node1->value, buf);
			}
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
			
			pkg_item_init(&item);
			pkg_get_8583_item(xml, node1, &item);
			
			if (transvalue[0] != '\0' && strncmp(item.var_type, "BITMAP", 6) != 0 && item.index > 0 && transvalue[item.index - 1] == '0')
			{
				pub_log_info("[%s][%d] 变量[%s]在交易[%s]中指定为不组!", 
					__FILE__, __LINE__, item.var_name, txcode);
				node1 = node1->next;
				continue;
			}

			ret = pkg_deal_8583_item_out(vars, xml, node1, pkg_buf, &item, &bitmap);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal 8583 item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			node1 = node1->next;
		}
		node = node->next;
	}

	if (bitmap.bitnum > 0)
	{
		memcpy(pkg_buf->data +  bitmap.begin, bitmap.bitvalue, bitmap.bitnum * bitmap.bitlen);
	}
	
	return SW_OK;
}

sw_int_t pkg_deal_com_package_out(vars, pkg_buf, xml, first_name)
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
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.PACKAGE");
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
	
		node1 = pub_xml_locnode(xml, "RESPCD");
		if (node1 != NULL && node1->value != NULL)
		{
			memset(buf, 0x0, sizeof(buf));
			memset(respcd, 0x0, sizeof(respcd));
			loc_get_zd_data(vars, node1->value, buf);
			loc_get_zd_data(vars, "$respcd", respcd);
			if (strlen(buf) == 0 && strcmp(respcd, "0000") != 0)
			{
				memset(buf, 0x0, sizeof(buf));
				memcpy(buf, respcd, 4);
				loc_set_zd_data(vars, node1->value, buf);
			}
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
			
			pkg_item_init(&item);
			xml->current = node1;
			pkg_get_com_item(xml, node1, &item);
			ret = pkg_com_item_out(vars, pkg_buf, xml, node1);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal com item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			node1 = node1->next;
		}
		node = node->next;
	}
	
	return SW_OK;
}

sw_int_t pkg_deal_last_work_out(sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_buf_t *pkg_buf)
{
	int	begin = 0;
	int	end = 0;
	int	net_type = 0;
	char	buf[128];
	char	code[8];
	char	value[PKG_VAR_VALUE_MAX_LEN];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_pkg_item_t	item;
	
	memset(buf, 0x0, sizeof(buf));
	memset(code, 0x0, sizeof(code));
	memset(value, 0x0, sizeof(value));
	
	pkg_item_init(&item);
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.LASTWORK.ITEM");
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
			strcpy(item.var_name, node1->value);
		}
	
		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			get_pkg_len(node1->value, &begin, &end);
		}
	
		node1 = pub_xml_locnode(xml, "NET");
		if (node1 != NULL && node1->value != NULL)
		{
			net_type = atoi(node1->value);
		}	
	
		node1 = pub_xml_locnode(xml, "CODE");
		if (node1 != NULL)
		{
			if (strcmp(node1->value, "HUA") == 0)
			{
				code[0] = 'H';
			}
			else if (strcmp(node1->value, "LBCD") == 0)
			{
				code[0] = 'L';
			}
		}
		else
		{
			node1 = pub_xml_locnode(xml, "SORT");	
			if (node1 != NULL && strcmp(node1->value, "NET") == 0)
			{	
				code[0] = 'H';
			}
		}
		
		node1 = pub_xml_locnode(xml, "MYSELF");
		if (node1 != NULL)
		{
			memset(buf, 0x0, sizeof(buf));
			memset(value, 0x0, sizeof(value));
			if (strcmp(node1->value, "TRUE") == 0)
			{
				if (code[0] == 'H')
				{
					sprintf(value, "%0*d", 2 * (end - begin + 1), pkg_buf->len);
					buf[0] = atoi(value) / 256;
					buf[1] = atoi(value) % 256;
					memset(value, 0x0, sizeof(value));
					if ((end - begin + 1) <= 2)
					{
						memcpy(value, buf, sizeof(value) - 1);
					}
					else
					{
						value[end - begin - 1] = buf[0];
						value[end - begin] = buf[1];
					}
				}
				else if (code[0] == 'L')
				{
					sprintf(buf, "%0*d", end - begin + 1, pkg_buf->len);
					pub_code_int2hex((u_char *)value, atoi(buf), end - begin + 1);
				}
				else if (net_type != 2 && net_type != 4)
				{
					sprintf(value, "%0*d", end - begin + 1, pkg_buf->len);
				}
				else
				{
					pub_code_numtonet(pkg_buf->len, value, net_type);
				}
			}
			else if (strcmp(node1->value, "FALSE") == 0)
			{
				if (code[0] == 'H')
				{
					sprintf(value, "%0*d", 2 * (end - begin + 1), pkg_buf->len - end - 1);
					buf[0] = atoi(value) / 256;
					buf[1] = atoi(value) % 256;
					memset(value, 0x0, sizeof(value));
					memcpy(value, buf, sizeof(value) - 1);
				}
				else if (code[0] == 'L')
				{
					sprintf(buf, "%0*d", end - begin + 1, pkg_buf->len - end - 1);
					pub_code_int2hex((u_char *)value, atoi(buf), end - begin + 1);
				}
				else if (net_type != 2 && net_type != 4)
				{
					sprintf(value, "%0*d", end - begin + 1, pkg_buf->len - end - 1);
				}
				else
				{
					pub_code_numtonet(pkg_buf->len - end - 1, value, net_type);
				}
			}
			else
			{
				if (code[0] == 'H')
				{
					sprintf(value, "%0*d", 2 * (end - begin + 1), pkg_buf->len - atoi(node1->value));
					buf[0] = atoi(value) / 256;
					buf[1] = atoi(value) % 256;
					memset(value, 0x0, sizeof(value));
					memcpy(value, buf, sizeof(value) - 1);
				}
				else if (code[0] == 'L')
				{
					sprintf(buf, "%0*d", end - begin + 1, pkg_buf->len - atoi(node1->value));
					pub_code_int2hex((u_char *)value, atoi(buf), end - begin + 1);
				}
				else if (net_type != 2 && net_type != 4)
				{
					sprintf(value, "%0*d", end - begin + 1, pkg_buf->len - atoi(node1->value));
				}
				else
				{
					pub_code_numtonet(pkg_buf->len - atoi(node1->value), value, net_type);
				}
			}
			memcpy(pkg_buf->data + begin , value, end - begin + 1);
		}
		
		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 != NULL && node1->value != NULL)
		{
			pkg_get_exp_value(vars, node1->value, item.var_value);
			loc_set_zd_data(vars, item.var_name, item.var_value);
		}
		
		node = node->next;
	}
	
	return 0;
}

sw_int_t pkg_set_digital_sign(sw_loc_vars_t *vars, sw_buf_t *pkg_buf)
{
	int	len = 0;
	char	dsflag[8];
	char	*dstext = NULL;
	
	memset(dsflag, 0x0, sizeof(dsflag));
	
	loc_get_zd_data(vars, "$dsflag", dsflag);
	if (dsflag[0] != '1')
	{
		return SW_OK;
	}
	
	len = vars->get_field_len(vars, "$dstext");
	if (len <= 0)
	{
		pub_log_info("[%s][%d] $dstext is null!", __FILE__, __LINE__);
		return SW_OK;
	}
	
	dstext = (char *)calloc(1, len + 1);
	if (dstext == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	loc_get_zd_data(vars, "$dstext", dstext);
	pub_log_info("[%s][%d] dstext=[%s]", __FILE__, __LINE__, dstext);
	memcpy(pkg_buf->data + pkg_buf->len, "{S:", 3);
	pkg_buf->len += 3;
	memcpy(pkg_buf->data + pkg_buf->len, dstext, len);
	pkg_buf->len += len;
	memcpy(pkg_buf->data + pkg_buf->len, "}\x0d\x0a", 3);
	pkg_buf->len += 3;
	free(dstext);
	
	return SW_OK;
}

sw_int_t pkg_8583_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, sw_int32_t cache_flag)
{
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	first_name[64];
	sw_xmltree_t	*xml = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(first_name, 0x0, sizeof(first_name));
	
	if (vars == NULL || pkg_buf == NULL || xmlname == NULL)
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
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(buf);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",
				__FILE__, __LINE__, buf);
			return SW_ERROR;
		}
	}
	
	/*** DEAL FIRSTWORK ***/
	memset(first_name, 0x0, sizeof(first_name));
	ret = pkg_deal_firstwork_out(xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal FIRSTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE HEAD ***/
	ret = pkg_deal_head_out(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE BODY ***/
	ret = pkg_deal_8583_package_out(vars, pkg_buf, xml, first_name);
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
	ret = pkg_deal_last_work_out(vars, xml, pkg_buf);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal LASTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	if (cache_flag != 1)
	{
		pub_xml_deltree(xml);
	}
	pub_log_bin(SW_LOG_DEBUG, pkg_buf->data, pkg_buf->len, "[%s][%d] Package:[%d]", __FILE__, __LINE__, pkg_buf->len);
	
	return pkg_buf->len;
}

sw_int_t pkg_com_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, int cache_flag)
{
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	first_name[64];
	sw_xmltree_t	*xml = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(first_name, 0x0, sizeof(first_name));
	
	if (vars == NULL || pkg_buf == NULL || xmlname == NULL)
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
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(buf);
		if (xml == NULL)                                                                                         
		{                                                                                                        
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",                          
				__FILE__, __LINE__, buf);                                                                
			return SW_ERROR;                                                                                 
		}                                                                                                        
	}
	
	/*** DEAL FIRSTWORK ***/
	memset(first_name, 0x0, sizeof(first_name));
	ret = pkg_deal_firstwork_out(xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal FIRSTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE HEAD ***/
	ret = pkg_deal_head_out(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE BODY ***/
	ret = pkg_deal_com_package_out(vars, pkg_buf, xml, first_name);
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
	ret = pkg_deal_last_work_out(vars, xml, pkg_buf);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal LASTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	if (cache_flag != 1)
	{
		pub_xml_deltree(xml);
	}
	pub_log_bin(SW_LOG_DEBUG, pkg_buf->data, pkg_buf->len, "[%s][%d] Package:[%d]", __FILE__, __LINE__, pkg_buf->len);
	
	return pkg_buf->len;
}

sw_int_t pkg_xml_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, int cache_flag)
{
	int	len = 0;
	int	headlen = 0;
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	xmlhead[256];
	sw_char_t	first_name[64];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(xmlhead, 0x0, sizeof(xmlhead));
	memset(first_name, 0x0, sizeof(first_name));
	
	if (vars == NULL || pkg_buf == NULL || xmlname == NULL)
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
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(buf);
		if (xml == NULL)                                                                                         
		{                                                                                                        
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",                          
				__FILE__, __LINE__, buf);                                                                
			return SW_ERROR;                                                                                 
		}                                                                                                        
	}
	
	/*** DEAL FIRSTWORK ***/
	memset(first_name, 0x0, sizeof(first_name));
	ret = pkg_deal_firstwork_out(xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal FIRSTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE HEAD ***/
	ret = pkg_deal_head_out(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL PACKAGE BODY ***/
	ret = pkg_deal_com_package_out(vars, pkg_buf, xml, first_name);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal PACKAGE error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.MAP");
	if (node != NULL)
	{
		ret = pub_xml_convert_integrate(vars, xml);
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
	
	/*** SET DIGITAL SIGN ***/
	ret = pkg_set_digital_sign(vars, pkg_buf);
	if (ret != SW_OK)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Set digital error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*** DEAL XML BODY ***/
	len = xml_pack_length(vars->tree);
	if (len < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Get xml length error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (len > 0)
	{
		pub_log_debug("[%s][%d] len=[%d]", __FILE__, __LINE__, len);
		memset(xmlhead, 0x0, sizeof(xmlhead));
		loc_get_zd_data(vars, "$sw_xml_headflag", xmlhead);
		if (xmlhead[0] == '\0')
		{
			sprintf(xmlhead, "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n");
		}
		
		headlen = strlen(xmlhead);
		if (xmlhead[headlen - 1] != '\n')
		{
			strcat(xmlhead, "\n");
		}

		ret = pub_buf_chksize(pkg_buf, strlen(xmlhead));
		if (ret < 0)
		{
			if (cache_flag != 1)
			{
				pub_xml_deltree(xml);
			}
			pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		memcpy(pkg_buf->data + pkg_buf->len, xmlhead, strlen(xmlhead));
		pkg_buf->len += strlen(xmlhead);
	}

	ret = pub_buf_chksize(pkg_buf, len);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = pub_xml_pack_ext(vars->tree, pkg_buf->data + pkg_buf->len);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] xml pack error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pkg_buf->len += len;
	zip_last_0x0(pkg_buf);

	/*** DEAL LASTWORK ***/
	ret = pkg_deal_last_work_out(vars, xml, pkg_buf);
	if (ret < 0)
	{
		if (cache_flag != 1)
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Deal LASTWORK error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	if (cache_flag != 1)
	{
		pub_xml_deltree(xml);
	}
	pub_log_bin(SW_LOG_DEBUG, pkg_buf->data, pkg_buf->len, "[%s][%d] Package:[%d]", __FILE__, __LINE__, pkg_buf->len);
	
	return pkg_buf->len;
}
