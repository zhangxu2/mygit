#include "pack.h"
#include "json.h"
#include "pub_vars.h"
#include "pub_log.h"
#include "pack_json.h"

extern sw_int_t pkg_deal_head_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_xmltree_t *xml, char *firstname);

sw_int_t json_deal_firstwork_out(sw_xmltree_t *xml, char *firstname)
{
	sw_xmlnode_t    *node = NULL;

	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(firstname, node->value);
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
				return 0;
			}
		}
		strcat(path, "(");
		strcat(path, tmp);
		strcat(path, ")");

		p = strchr(p, '(');
	}
	return 0;
}

int json_deal_item_out(sw_loc_vars_t *vars, struct json_object *robj, sw_xmltree_t *xml, char *uname, int idx, int aflag)
{
	int	i = 0;
	int	len = 0;
	int	ret = 0;
	int attrflag = 0;
	int	array_size = 0;
	char 	buf[128];
	char	flag[8];
	char	absflag = 0;
	char	name[128];
	char	mname[128];
	char	type[32];
	char	attr[32];
	char	tname[128];
	char	upath[128];
	char 	vpath[128];
	char	vname[128];
	char	arraysize[128];
	sw_buf_t	*vbuf = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;

	memset(flag, 0x0, sizeof(flag));
	memset(name, 0x0, sizeof(name));
	memset(attr, 0x0, sizeof(attr));
	memset(mname, 0x0, sizeof(mname));
	memset(tname, 0x0, sizeof(tname));
	memset(upath, 0x0, sizeof(upath));
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

	node = pub_xml_locnode(xml, "FLAG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(flag, node->value, sizeof(flag) - 1);
	}

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
		
		node = pub_xml_locnode(xml, "ATTR");
		if (node != NULL && node->value != NULL)
		{
			strncpy(attr, node->value, sizeof(attr) - 1);
			attrflag = 1;
		}
	}

	if (strcmp(type, "struct") == 0 || strcmp(type, "array") == 0)
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

		if (strcmp(type, "array") == 0)
		{
			node = pub_xml_locnode(xml, "ARRAYSIZE");
			if (node == NULL || node->value == NULL)
			{
				pub_log_error("[%s][%d] Type is array, but not config ARRAYSIZE",
						__FILE__, __LINE__);
				return SW_ERROR;
			}
			strncpy(arraysize, node->value, sizeof(arraysize) - 1);
		}
	}

	if (strcmp(type, "struct") != 0 && strcmp(type, "array") != 0)
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
					pub_log_error("[%s][%d] get_upath error!", __FILE__, __LINE__);
					return -1;
				}

				memset(vname, 0x0, sizeof(vname));
				sprintf(vname, "%s%s", name, vpath);
			}
		}

		len = vars->get_field_len(vars, vname); 
		if (len == 0 && flag[0] != '1')
		{
			pub_log_info("[%s][%d] [%s]值为空,不组!", __FILE__, __LINE__, vname);
			return SW_OK;
		}

		vbuf = buf_new();
		if (vbuf == NULL)
		{
			pub_log_error("[%s][%d] Alloc buffer error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		buf_checksize(vbuf, len);
		loc_get_zd_data(vars, vname, vbuf->data);
		pub_log_info("[%s][%d] 取出变量[%s]=[%s]", __FILE__, __LINE__, vname, vbuf->data);
		if (strcmp(type, "boolean") == 0)
		{
			if (strcasecmp(vbuf->data, "TRUE") == 0 || strcmp(vbuf->data, "1") == 0)
			{
				json_object_object_add(robj, mname, json_object_new_boolean(TRUE));
			}
			else
			{
				json_object_object_add(robj, mname, json_object_new_boolean(FALSE));
			}
		}
		else if (strcmp(type, "int") == 0)
		{
			json_object_object_add(robj, mname, json_object_new_int(atoi(vbuf->data)));
		}
		else if (strcmp(type, "int64") == 0)
		{
			json_object_object_add(robj, mname, json_object_new_int64(strtoll(vbuf->data, NULL, 10)));
		}
		else if (strcmp(type, "double") == 0)
		{
			json_object_object_add(robj, mname, json_object_new_double_s(strtod(vbuf->data, NULL), vbuf->data));
		}
		else
		{
			json_object_object_add(robj, mname, json_object_new_string(vbuf->data));
		}
		buf_release(vbuf);
	
		return SW_OK;
	}

	if (strcmp(type, "struct") == 0 || strcmp(type, "array") == 0)
	{
		if (!attrflag)
		{
			node = pub_xml_locnode(xml, ".CBM.INTEGRATE.STRUCT");
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
					pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
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
			nodebak = node;
		}
	}

	if (strcmp(type, "struct") == 0)
	{
		struct json_object	*o_obj = NULL;
		o_obj = json_object_new_object();
		if (o_obj == NULL)
		{
			pub_log_error("[%s][%d] Json object new error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		memset(upath, 0x0, sizeof(upath));
		if (uname != NULL && uname[0] != '\0')
		{
			sprintf(upath, "%s.%s", uname, name + 1);
		}
		else
		{
			sprintf(upath, "%s", name);
		}

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
			ret = json_deal_item_out(vars, o_obj, xml, upath, i, absflag);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Deal item error!", __FILE__, __LINE__);
				json_object_put(o_obj);
				return SW_ERROR;
			}

			node = node->next;
		}
		int	o_len = 0;
		o_len = json_object_object_length(o_obj);
		if (o_len > 0)
		{
			json_object_object_add(robj, mname, o_obj);
		}
	
		return SW_OK;
	}

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

	array_size = 0;
	loc_get_zd_int(vars, vname, &array_size);
	pub_log_info("[%s][%d] 取出变量:arraysize=[%s][%d]", __FILE__, __LINE__, vname, array_size);
	if (array_size == 0)
	{
		return SW_OK;
	}

	struct json_object	*array_object = NULL;
	array_object = json_object_new_array();
	if (array_object == NULL)
	{
		pub_log_error("[%s][%d] Json object new error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < array_size; i++)
	{
		memset(upath, 0x0, sizeof(upath));
		if (uname == NULL || uname[0] == '\0')
		{
			sprintf(upath, "%s(%d)", name, i);
		}
		else
		{
			sprintf(upath, "%s.%s(%d)", uname, name + 1, i);
		}

		struct json_object	*obj = NULL;
		obj = json_object_new_object();
		if (obj == NULL)
		{
			pub_log_error("[%s][%d] Json object new error!", __FILE__, __LINE__);
			json_object_put(array_object);
			return SW_ERROR;
		}

		xml->current = nodebak;
		if (!attrflag)
		{
			node = pub_xml_locnode(xml, "ITEM");
			while (node != NULL)
			{
				if (strcmp(node->name, "ITEM") != 0)
				{
					node = node->next;
					continue;
				}
	
				xml->current = node;
				ret = json_deal_item_out(vars, obj, xml, upath, i, absflag);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] Deal item error!", __FILE__, __LINE__);
					json_object_put(array_object);
					return SW_ERROR;
				}
	
				node = node->next;
			}
		}
		else
		{
			if (absflag == 0)
			{
				memset(vpath, 0x0, sizeof(vpath));
				ret = get_upath(upath, vpath);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] [%s] get_upath error", __FILE__, __LINE__, vname);
					return -1;
				}
				
				memset(upath, 0x00, sizeof(upath));
				sprintf(upath, "%s%s(%d)", name, vpath, i);
			}

			len = vars->get_field_len(vars, upath); 
			if (len == 0 && flag[0] != '1')
			{
				pub_log_info("[%s][%d] [%s]值为空,不组!", __FILE__, __LINE__, vname);
				return SW_OK;
			}

			vbuf = buf_new();
			if (vbuf == NULL)
			{
				pub_log_error("[%s][%d] Alloc buffer error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			buf_checksize(vbuf, len);
			loc_get_zd_data(vars, upath, vbuf->data);
			if (strcmp(attr, "boolean") == 0)
			{
				if (strcasecmp(vbuf->data, "TRUE") == 0 || strcmp(vbuf->data, "1") == 0)
				{
					json_object_array_put_idx(array_object, i, json_object_new_boolean(TRUE));
				}
				else
				{
					json_object_array_put_idx(array_object, i, json_object_new_boolean(FALSE));
				}
			}
			else if (strcmp(attr, "int") == 0)
			{
				json_object_array_put_idx(array_object, i, json_object_new_int(atoi(vbuf->data)));
			}
			else if (strcmp(attr, "int64") == 0)
			{
				json_object_array_put_idx(array_object, i, json_object_new_int64(strtoll(vbuf->data, NULL, 10)));
			}
			else if (strcmp(attr, "double") == 0)
			{
				json_object_array_put_idx(array_object, i, json_object_new_double_s(strtod(vbuf->data, NULL), vbuf->data));
			}
			else
			{
				json_object_array_put_idx(array_object, i, json_object_new_string(vbuf->data));
			}
		}
		if (!attrflag)
		{
			json_object_array_add(array_object, obj);
		}
	}
	json_object_object_add(robj, mname, array_object);

	return SW_OK;
}

int json_deal_package_out(sw_loc_vars_t *vars, struct json_object *robj, sw_xmltree_t *xml, char *firstname)
{
	int	ret = 0;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

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
			if (pkg_check_include(vars, firstname, node1->value) != 0)
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
				if (pkg_check_include(vars, firstname, node2->value) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			xml->current = node1;
			ret = json_deal_item_out(vars, robj, xml, NULL, 0, -1);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Json deal item error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			node1 = node1->next;
		}

		node = node->next;
	}

	return SW_OK;
}

sw_int_t json_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, char *xmlname, sw_int32_t cache_flag)
{
	int	ret = 0;
	char	buf[128];
	char	firstname[256];
	sw_xmltree_t	*xml = NULL;
	struct json_object	*robj = NULL;

	memset(buf, 0x0, sizeof(buf));
	memset(firstname, 0x0, sizeof(firstname));

	if (vars == NULL || pkg_buf == NULL || xmlname == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
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

	robj = json_object_new_object();
	if (robj == NULL)
	{
		pub_log_error("[%s][%d] New json object failed!", __FILE__, __LINE__);
		goto ERR;
	}

	ret = pkg_deal_firstwork_out(xml, firstname);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal firstwork error!", __FILE__, __LINE__);
		goto ERR;
	}

	/*** DEAL PACKAGE HEAD ***/
	ret = pkg_deal_head_out(vars, pkg_buf, xml, firstname);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Deal HEAD error!", __FILE__, __LINE__);
		goto ERR;
	}

	ret = json_deal_package_out(vars, robj, xml, firstname);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Deal package error!", __FILE__, __LINE__);
		goto ERR;
	}
	pub_log_info("[%s][%d] json deal package success!", __FILE__, __LINE__);

	size_t	len = 0;
	const char	*ptr = NULL;
	ptr =  json_object_to_json_string_length(robj, JSON_C_TO_STRING_PLAIN, &len);
	buf_append(pkg_buf, (char *)ptr, len);

	ret = pkg_deal_last_work_out(vars, xml, pkg_buf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Deal lastwork error!", __FILE__, __LINE__);
		goto ERR;
	}

	pub_log_bin(SW_LOG_DEBUG, pkg_buf->data, pkg_buf->len, "[%s][%d] Package:[%d]",
		__FILE__, __LINE__, pkg_buf->len);

	if (!cache_flag)
	{
		pub_xml_deltree(xml);
	}

	if (robj != NULL)
	{
		json_object_put(robj);
	}

	return pkg_buf->len;
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

