#include "pack.h"
#include "json.h"
#include "pub_vars.h"
#include "pack_json.h"

extern sw_int_t pkg_deal_head_in(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *xml, char *firstname);

static char *get_one_word(char *key)
{
	char    *p = NULL;
	char	*ptr = NULL;
	static char     *last = NULL;
	static char     word[1024];

	memset(word, 0x0, sizeof(word));
	p = word;

	if (key != NULL)
	{
		ptr = key;
		last = key;
	}
	else
	{
		ptr = last;
	}

	while (*ptr != '\0')
	{
		if (*ptr != ' ')
		{
			break;
		}
		ptr++;
	}

	while (*ptr != '\0')
	{
		if (*ptr == '.')
		{
			last = ptr + 1;
			*p = '\0';
			return word;
		}
		*p++ = *ptr++;
	}
	if (word[0] != '\0')
	{
		*p = '\0';
		last = ptr;
		return word;
	}

	return NULL;	
}

struct json_object *json_object_get_ext(struct json_object *obj, char *key)
{
	char	*p = NULL;
	struct json_object	*result = NULL;

	p = get_one_word(key);
	if (p == NULL)
	{
		pub_log_error("[%s][%d] key[%s] null!", __FILE__, __LINE__, key);
		return NULL;
	}

	result = json_object_object_get(obj, p);
	if (result == NULL)
	{
		return NULL;
	}

	while ((p = get_one_word(NULL)) != NULL)
	{
		result = json_object_object_get(result, p);
		if (result == NULL)
		{
			pub_log_error("[%s][%d] Not found [%s]!", __FILE__, __LINE__, p);
			return NULL;
		}
	}

	return result;
}

struct json_object *json_object_get_array_ext(struct json_object *obj, char *key, int idx)
{
	struct json_object	*result = NULL;
	
	result = json_object_array_get_idx(obj, idx);
	if (result == NULL)
	{
		pub_log_error("[%s][%d] array[%d] is null!", __FILE__, __LINE__, idx);
		return NULL;
	}

	result = json_object_object_get(result, key);
	if (result == NULL)
	{
		pub_log_debug("[%s][%d] Not found [%s]!", __FILE__, __LINE__, key);
		return NULL;
	}

	return result;
}

int json_deal_firstwork_in(sw_loc_vars_t *vars, struct json_object *obj, sw_xmltree_t *xml, char *firstname)
{
	int	first = 1;
	const char	*ptr = NULL;
	char	name[128];
	char	mname[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	struct json_object	*result = NULL;

	node = pub_xml_locnode(xml, ".CBM.ANALYZE.FIRSTWORK.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;

		memset(name, 0x0, sizeof(name));
		memset(mname, 0x0, sizeof(mname));
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		strncpy(name, node1->value, sizeof(name) - 1);

		node1 = pub_xml_locnode(xml, "MNAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config MNAME!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		strncpy(mname, node1->value, sizeof(mname) - 1);

		result = json_object_get_ext(obj, mname);
		if (result == NULL)
		{
			pub_log_debug("[%s][%d] [%s] is null!", __FILE__, __LINE__, mname);
		}
		else
		{
			ptr = json_object_get_string(result);
			if (ptr != NULL)
			{
				pub_log_info("[%s][%d] firstwork 拆出变量:[%s][%s]=[%s]",
						__FILE__, __LINE__, name, mname, ptr);
				loc_set_zd_data(vars, name, (char *)ptr);
			}
			else
			{
				pub_log_debug("[%s][%d] [%s] is null!", __FILE__, __LINE__, mname);
			}
		}
		if (first)
		{
			strcpy(firstname, name);
			first = 0;
		}

		node = node->next;
	}

	return SW_OK;
}

static int get_upath(char *name, char *path)
{
	if (name == NULL || strlen(name) == 0 || path == NULL)
	{
		pub_log_error("[%s][%d] parameter error", __FILE__, __LINE__);
		return -1;
	}

	int i;
	int len = 0;
	char tmp[8]; 
	char *p = name;
	char *q = NULL;

	p = strchr(p, '(');
	while(p != NULL)
	{
		q = strstr(p, ").");
		if (q == NULL)
		{
			return 0;
		}
		p++;

		memset(tmp, 0x0, sizeof(tmp));
		len = q - p;
		for (i = 0; i < len; i++)
		{
			if (p[i] >= '0' && p[i] <= '9')
			{
				strncpy(tmp + i, p + i, 1);
			}
			else
			{
				return -1;
			}
		}
		strcat(path, "(");
		strcat(path, tmp);
		strcat(path, ")");

		p = strchr(p, '(');
	}
	return 0;
}


int json_deal_item_in(sw_loc_vars_t *vars, struct json_object *obj, sw_xmltree_t *xml, char *uname, int idx, int aflag)
{
	int	i = 0;
	int	ret = 0;
	int	attrflag = 0;
	int	absflag = 0;
	int	array_size = 0;
	char	type[32];
	char	name[128];
	char	mname[128];
	char	tname[128];
	char	upath[128];
	char 	vpath[128];
	char	vname[128];
	char	arraysize[128];
	const char	*ptr = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	struct json_object	*result = NULL;
	
	memset(name, 0x0, sizeof(name));
	memset(mname, 0x0, sizeof(mname));
	memset(tname, 0x0, sizeof(tname));
	memset(type, 0x0, sizeof(type));
	memset(upath, 0x0, sizeof(upath));
	memset(vname, 0x0, sizeof(vname));
	memset(vpath, 0x0, sizeof(vpath));
	memset(arraysize, 0x0, sizeof(arraysize));

	node = pub_xml_locnode(xml, "NAME");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(name, node->value, sizeof(name) - 1);

	node = pub_xml_locnode(xml, "MNAME");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config MNAME!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(mname, node->value, sizeof(mname) - 1);
	
	node = pub_xml_locnode(xml, "TYPE");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config TYPE!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(type, node->value, sizeof(type) - 1);
	if (strcmp(type, "array") == 0 || strcmp(type, "struct") == 0)
	{
		/*add check*/
		node = pub_xml_locnode(xml, "CHECK");
		if (node != NULL && node->value != NULL)
		{
			if (pkg_check_mult(vars, node) != 0)
			{
				pub_log_debug("[%s][%d] ITEM[%s] check mismatch, continue next ITEM", 
					__FILE__, __LINE__, mname);
				return 0;
			}
		}
	
		node = pub_xml_locnode(xml, "AFLAG");
		if (node != NULL && node->value != NULL && node->value[0] == '1')
		{
			absflag = 1;
		}
		else
		{
			absflag = 0;
		}
	}

	if (json_object_is_type(obj, json_type_array))
	{
		pub_log_debug("[%s][%d] mname is [%s]!", __FILE__, __LINE__, mname);
		if (idx >= 0)
		{
			result = json_object_get_array_ext(obj, mname, idx);
		}
		else
		{
			result = obj;
		}
	}
	else
	{
		result = json_object_get_ext(obj, mname);
	}
	if (result == NULL)
	{
		pub_log_debug("[%s][%d] [%s] is null!", __FILE__, __LINE__, mname);

		return SW_OK;
	}

	if (json_object_is_type(result, json_type_array) || json_object_is_type(result, json_type_object))
	{
		node = pub_xml_locnode(xml, "TNAME");
		if (node != NULL && node->value != NULL)
		{
			strncpy(tname, node->value, sizeof(tname) - 1);
		}
		else
		{
			strncpy(tname, mname, sizeof(tname) - 1);
		}

		node = pub_xml_locnode(xml, "ATTR");
		if (node != NULL && node->value != NULL)
		{
			attrflag = 1;
		}

		if (json_object_is_type(result, json_type_array))
		{
			node = pub_xml_locnode(xml, "ARRAYSIZE");
			if (node == NULL || node->value == NULL)
			{
				pub_log_error("[%s][%d] TYPE is array, but not config ARRAYSIZE!",
					__FILE__, __LINE__);
				return SW_ERROR;
			}
			memset(arraysize, 0x0, sizeof(arraysize));
			strncpy(arraysize, node->value, sizeof(arraysize) - 1);
		}
	}

	if (!(json_object_is_type(result, json_type_array) || json_object_is_type(result, json_type_object)))
	{
		memset(vname, 0x0, sizeof(vname));
		if (uname == NULL || uname[0] == '\0')
		{
			sprintf(vname, "%s", name);
		}
		else
		{
			sprintf(vname, "%s.%s", uname, name + 1);
			if (aflag == 0)
			{
				memset(vpath, 0x0, sizeof(vpath));
				ret = get_upath(vname, vpath);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] get_upath error or vname format error!", __FILE__, __LINE__);
					return -1;
				}
				memset(vname, 0x0, sizeof(vname));
				sprintf(vname, "%s%s", name, vpath);
			}
		}

		ptr = json_object_get_string(result);
		if (ptr != NULL)
		{
			loc_set_zd_data(vars, vname, (char *)ptr);
			pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, vname, ptr);
		}
		else
		{
			pub_log_debug("[%s][%d] [%s] is null!", __FILE__, __LINE__, mname);
		}
	}

	if (json_object_is_type(result, json_type_array) || json_object_is_type(result, json_type_object))
	{
		if (!attrflag)
		{
			node = pub_xml_locnode(xml, ".CBM.ANALYZE.STRUCT");
			while (node != NULL)
			{
				if (strcmp(node->name, "STRUCT") != 0)
				{
					node = node->next;
					continue;
				}

				xml->current = node;
				node1 = pub_xml_locnode(xml, "NAME");
				if (node1 == NULL || node1->value == NULL)
				{
					pub_log_error("[%s][%d] STRUCT not config NAME!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				if (strcmp(node1->value, tname) == 0)
				{
					break;
				}
				node = node->next;
			}
			if (node == NULL)
			{
				pub_log_error("[%s][%d] Not found struct [%s]!", __FILE__, __LINE__, tname);
				return SW_ERROR;
			}
		}

		if (json_object_is_type(result, json_type_array))
		{
			array_size =  json_object_array_length(result);
			pub_log_debug("[%s][%d] [%s]'s length=[%d]", __FILE__, __LINE__, mname, array_size);
			memset(vname, 0x0, sizeof(vname));

			if (uname == NULL || uname[0] == '\0')
			{
				sprintf(vname, "%s", arraysize);
			}
			else
			{
				sprintf(vname, "%s.%s", uname, arraysize + 1);
				if (aflag == 0)
				{
					memset(vpath, 0x00, sizeof(vpath));
					ret = get_upath(vname, vpath);
					if (ret < 0)
					{
						pub_log_error("[%s][%d] [%s] get_upath error", __FILE__, __LINE__, vname);
						return -1;
					}
					memset(vname, 0x0, sizeof(vname));
					sprintf(vname, "%s%s", arraysize, vpath);
				}
			}
			loc_set_zd_int(vars, vname, array_size);
			pub_log_info("[%s][%d] 拆出变量:array_size[%s]=[%d]", __FILE__, __LINE__, vname, array_size);
		}
		else
		{
			array_size = 1;
		}
		nodebak = node;
		for (i = 0; i < array_size; i++)
		{
			memset(upath, 0x0, sizeof(upath));
			if (uname != NULL && uname[0] != '\0')
			{
				if (strcmp(type, "array") == 0)
				{
					sprintf(upath, "%s.%s(%d)", uname, name + 1, i);
				}
				else
				{
					sprintf(upath, "%s.%s", uname, name+1);
				}
			}
			else
			{
				if (strcmp(type, "array") == 0)
				{
					sprintf(upath, "%s(%d)", name, i);
				}
				else
				{
					sprintf(upath, "%s", name);
				}
			}

			if (!attrflag)
			{
				xml->current = nodebak;
				node = pub_xml_locnode(xml, "ITEM");
				while (node != NULL)
				{
					if (strcmp(node->name, "ITEM") != 0)
					{
						node = node->next;
						continue;
					}

					xml->current = node;
					ret = json_deal_item_in(vars, result, xml, upath, i, absflag);
					if (ret != SW_OK)
					{
						pub_log_error("[%s][%d] Deal item error!", __FILE__, __LINE__);
						return SW_ERROR;
					}

					node = node->next;
				}
			}
			else
			{
				struct json_object *str = NULL;
				if (absflag == 0)
				{
					memset(vpath, 0x0, sizeof(vpath));
					ret = get_upath(upath, vpath);	
					if (ret < 0)
					{
						pub_log_error("[%s][%d] [%s] get_upath error", __FILE__, __LINE__, upath);
						return -1;
					}
					memset(upath, 0x0, sizeof(upath));
					sprintf(upath, "%s%s(%d)", name, vpath, i);
				}				
				str = json_object_array_get_idx(result, i);
				if (str != NULL)
				{
					ptr = json_object_get_string(str);
					if (ptr != NULL)
					{
						loc_set_zd_data(vars, upath, (char *)ptr);
						pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, upath, ptr);
					}
					else
					{
						pub_log_debug("[%s][%d] [%s] is null!", __FILE__, __LINE__, name);
					}
				}
			}
		}
	}

	return SW_OK;
}

int json_deal_package_in(sw_loc_vars_t *vars, struct json_object *obj, sw_xmltree_t *xml, char *firstname)
{
	int	ret = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

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
			if (pkg_check_include(vars, firstname, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}

		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL && node1->value != NULL)
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

			ret = json_deal_item_in(vars, obj, xml, NULL, -1, 0);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Json deal item error!",
					__FILE__, __LINE__);
				return SW_ERROR;
			}

			node1 = node1->next;
		}

		node = node->next;
	}

	return SW_OK;
}

const char *json_get_root_key(json_object *obj)
{
	struct json_object_iterator	iter;

	iter = json_object_iter_begin(obj);

	return json_object_iter_peek_name(&iter);
}

int json_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag)
{
	int	ret = 0;
	char	fname[256];
	char	firstname[128];
	char	*ptr = NULL;
	const char	*key = NULL;
	sw_xmltree_t	*xml = NULL;
	struct json_object	*robj = NULL;

	memset(fname, 0x0, sizeof(fname));
	memset(firstname, 0x0, sizeof(firstname));

	if (vars == NULL || pkg == NULL || xmlname == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}	

	if (cache_flag)
	{
		xml = (sw_xmltree_t *)xmlname;
	}
	else
	{
		memset(fname, 0x0, sizeof(fname));
		sprintf(fname, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = cfg_read_xml(fname);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
					__FILE__, __LINE__, fname);
			return SW_ERROR;
		}
	}
	sw_buf_t	buf;
	sw_buf_t	*pkg_buf = NULL;

	memset(&buf, 0x0, sizeof(buf));
	buf.size = len;
	buf.len = 0;
	buf.data = pkg;
	pkg_buf = &buf;	

	ret = pkg_deal_firstwork_in(vars, xml, pkg_buf, firstname);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal firstwork error!", __FILE__, __LINE__);
		goto ERR;
	}
	pub_log_info("[%s][%d] Deal firstwork success!", __FILE__, __LINE__);

	/*** DEAL PACK HEAD ***/
	ret = pkg_deal_head_in(vars, pkg_buf, xml, firstname);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		goto ERR;
	}

	ptr = pkg_buf->data + pkg_buf->len;
	pub_log_info("[%s][%d] json parse begin...", __FILE__, __LINE__);
	robj = json_tokener_parse(ptr);
	if (robj == NULL)
	{
		pub_log_error("[%s][%d] Json parse error!", __FILE__, __LINE__);
		goto ERR;
	}
	pub_log_info("[%s][%d] json parse success! root node type:[%s]",
		__FILE__, __LINE__, json_type_to_name(json_object_get_type(robj)));

	if (!json_object_is_type(robj, json_type_array))
	{
		key = json_get_root_key(robj);
		loc_set_zd_data(vars, "#json_root_key", (char *)key);
		pub_log_info("[%s][%d] json root key:[%s]", __FILE__, __LINE__, key);
	}



	ret = json_deal_package_in(vars, robj, xml, firstname);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal package error!", __FILE__, __LINE__);
		goto ERR;
	}

	if (!cache_flag)
	{
		pub_xml_deltree(xml);
	}

	if (robj != NULL)
	{
		json_object_put(robj);
	}

	return SW_OK;

ERR:
	if (!cache_flag)
	{
		pub_xml_deltree(xml);
	}

	if (robj != NULL)
	{
		json_object_put(robj);
	}

	return SW_ERROR;
}

