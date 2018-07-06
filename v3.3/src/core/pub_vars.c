/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-21
 *** module  : vars pool interface 
 *** name    : pub_vars.c 
 *** function: abstract vars pool's interface, support SHM, HEAP vars pool.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "pub_vars.h"
#include "pub_hvar.h"
#include "shm_vars.h"
#include "pub_log.h"

SW_PROTECTED sw_loc_vars_t g_vars;	/*global vars pool*/

#if defined(__SHM_VARS_SUPPORT__)
SW_PROTECTED sw_int_t shm_vars_alloc(void* object);
SW_PROTECTED sw_int_t shm_vars_create(void* object, sw_int32_t mtype);
SW_PROTECTED sw_int_t shm_vars_destroy(void* object);
SW_PROTECTED sw_int_t shm_vars_free(void* object);
SW_PROTECTED sw_int_t shm_vars_set_variable(void* object, sw_char_t* name 
		, sw_int32_t type, sw_char_t* value
		, sw_int32_t length);						
SW_PROTECTED sw_int_t shm_vars_set_string(void* object, sw_char_t* name
		, sw_char_t* value);
SW_PROTECTED sw_int_t shm_vars_get_variable(void* object, sw_char_t* name
		, sw_char_t* value);

SW_PROTECTED sw_int32_t shm_vars_get_field_len(void* object, sw_char_t* name);
SW_PROTECTED sw_char_t* shm_vars_get_value_addr(void *object, sw_char_t* name, sw_int32_t *len);
SW_PROTECTED sw_int_t shm_vars_clear_vars(void* object);
SW_PROTECTED sw_int_t shm_vars_remove_var(void* object, sw_char_t* name);
SW_PROTECTED sw_int32_t shm_vars_serialize(void* object, sw_buf_t* buf);
SW_PROTECTED sw_int32_t shm_vars_unserialize(void* object, sw_char_t *buf);
SW_PROTECTED sw_char_t* shm_vars_get_null(void* object, sw_char_t *name, sw_int32_t len);
SW_PROTECTED sw_int_t shm_vars_print(void* object);
SW_PROTECTED sw_int_t shm_vars_clone(void* src, sw_int32_t vid);
#endif /*__SHM_VARS_SUPPORT__*/

SW_PROTECTED sw_int_t heap_vars_alloc(void* object);
SW_PROTECTED sw_int_t heap_vars_create(void* object, sw_int32_t igonre);
SW_PROTECTED sw_int_t heap_vars_destroy(void* object);
SW_PROTECTED sw_int_t heap_vars_free(void* object);
SW_PROTECTED sw_int_t heap_vars_set_variable(void* object, sw_char_t*name 
		,sw_int32_t type, sw_char_t* value
		, sw_int32_t length);
SW_PROTECTED sw_int_t heap_vars_set_string(void* object, sw_char_t*name
		, sw_char_t* value);
SW_PROTECTED sw_int_t heap_vars_get_variable(void* object, sw_char_t* name
		, sw_char_t* value);
SW_PROTECTED sw_int32_t heap_vars_get_field_len(void* object, sw_char_t* name);
SW_PROTECTED sw_char_t* heap_vars_get_value_addr(void* object, sw_char_t* name, sw_int32_t *len);

SW_PROTECTED sw_int_t heap_vars_clear_vars(void* object);

SW_PROTECTED sw_int_t heap_vars_remove_var(void* object, sw_char_t *name);

SW_PROTECTED sw_int32_t heap_vars_serialize(void* object, sw_buf_t *buf);

SW_PROTECTED sw_int32_t heap_vars_unserialize(void* object, sw_char_t *buf);

SW_PROTECTED sw_char_t *heap_vars_get_null(void* object, sw_char_t *name
		, sw_int32_t len);
SW_PROTECTED sw_int_t heap_vars_print(void* object);

SW_PROTECTED sw_int_t heap_vars_clone(void* object, sw_int32_t vid);

SW_PROTECTED sw_xmlnode_t *comm_loc_get_ext_var(sw_loc_vars_t *vars
		, sw_char_t *name, sw_int32_t creat);

SW_PROTECTED sw_int_t comm_vars_get_xml_variable(void * object, sw_char_t* name
		, sw_char_t* value);

SW_PROTECTED sw_int_t comm_vars_set_xml_variable(void *vars
		, sw_char_t *var_name, sw_char_t *value, int creat);

SW_PROTECTED sw_int_t comm_set_ext_var_attr(sw_loc_vars_t *vars, sw_xmlnode_t *node
		, char *attr_name, char *attr_value);

SW_PROTECTED sw_int_t comm_vars_set_attr(void *vars, sw_char_t *var_name
		, sw_char_t *attr_name, sw_char_t *attr_value);

SW_PROTECTED sw_char_t* comm_get_ext_var_attr(sw_loc_vars_t *vars, sw_xmlnode_t *node
		, char *attr);

SW_PROTECTED sw_int_t comm_vars_get_attr(void *vars, char *var_name
		, char *attr_name, char *attr_value);

#if defined(__SHM_VARS_SUPPORT__)
sw_int_t shm_vars_alloc(void* object)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	vars->data = (vars_object_t *)calloc(1, sizeof(vars_object_t));
	if (vars->data == NULL)
	{
		pub_log_error("%s, %d, malloc fail.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	vars->tree = NULL;

	return SW_OK;
}

sw_int_t shm_vars_create(void* object, sw_int32_t mtype)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || vars->data == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = vars_object_creat(mtype, (vars_object_t *)vars->data);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, vars_object_creat, mtype[%d]."
				, __FILE__, __LINE__, mtype);
		return SW_ERROR;
	}

	if (vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
	}
	vars->tree = NULL;

	return result;
}

sw_int_t shm_vars_destroy(void* object)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t	*vars = (sw_loc_vars_t*)object;

	if (object == NULL || vars->data == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = vars_vfree((vars_object_t *)vars->data);
	if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, vars_vfree error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	if (vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
		vars->tree = NULL;
	}

	return SW_OK;
}

sw_int_t shm_vars_free(void* object)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
		vars->tree = NULL;
	}

	if (vars->data != NULL)
	{
		vars_object_free(vars->data);
		free(vars->data);
		vars->data = NULL;
	}

	return SW_OK;
}

sw_int_t shm_vars_set_variable(void* object, sw_char_t* name ,sw_int32_t type
		, sw_char_t* value, sw_int32_t length)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}


	result = vars_set_revariable((vars_object_t *)vars->data, name, type, value, length, 0);

	return result;
}

sw_int_t shm_vars_set_string(void* object, sw_char_t* name, sw_char_t* value)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || value == NULL || name[0] == '\0')
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = vars_set_revariable((vars_object_t *)vars->data, name, 'a', value, strlen(value), 0);

	return result;
}

sw_char_t *shm_vars_get_null(void* object, sw_char_t *name, sw_int32_t len)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL
			|| strlen(name) == 0 || len <= 0)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return NULL;
	}

	return vars_get_null((vars_object_t *)vars->data, name, len);	
}

sw_int_t shm_vars_print(void* object)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL || vars->data == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;;
	}

	return vars_print((vars_object_t *)vars->data);
}

sw_int_t shm_vars_clone(void* object, sw_int32_t vid)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t*	vars = object;
	sw_buf_t	sw_buf;

	if (object == NULL || vars->data == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;;
	}

	pub_mem_memzero(&sw_buf, sizeof(sw_buf));
	pub_buf_init(&sw_buf);
	result = shm_vars_serialize(vars, &sw_buf);
	if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, shm_vars_serialize error.",__FILE__,__LINE__);
		pub_buf_clear(&sw_buf);
		return SW_ERROR;
	}

	result = vars_vclone((vars_object_t *) (vars->data), vid);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, vars_vclone error.",__FILE__,__LINE__);
		pub_buf_clear(&sw_buf);
		return SW_ERROR;
	}

	pub_buf_clear(&sw_buf);

	return SW_OK;	
}

/******************************************************************************
 **函数名称: shm_vars_merge
 **功    能: 合并变量池
 **输入参数: 
 **     object: 源变量池引用对象
 **     dst_vid: 目标变量池ID
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
sw_int_t shm_vars_merge(void *object, sw_int32_t vid)
{
	int ret = 0;
	sw_buf_t tmpbuf;
	sw_loc_vars_t *src = object;

	pub_buf_init(&tmpbuf);
	ret = shm_vars_serialize(src, &tmpbuf);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] serialize failed!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	pub_buf_clear(&tmpbuf);

	if (src->tree != NULL)
	{
		pub_xml_deltree(src->tree);
		src->tree = NULL;
	}

	return vars_vmerge(src->data, vid);
}

sw_int_t shm_vars_get_variable(void* object, sw_char_t* name, sw_char_t* value)
{
	sw_char_t	*tmp = NULL;
	sw_loc_vars_t*	vars = object;
	sw_int32_t	len = -1;

	if (object == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	tmp = (sw_char_t*)vars_get_revariable((vars_object_t *)vars->data, (const sw_char_t*)name, 0, &len);
	if (tmp == NULL)
	{
		return SW_ERROR;
	}
	else
	{
		pub_mem_memcpy(value, tmp, len);
		return len;
	}
}

sw_int32_t shm_vars_get_field_len(void* object, sw_char_t* name)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return vars_get_field_len((vars_object_t *)(vars->data), name, 0);
}



sw_char_t* shm_vars_get_value_addr(void *object, sw_char_t* name, sw_int32_t *len)
{
	sw_char_t	*tmp = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return NULL;
	}

	tmp = (sw_char_t*)vars_get_revariable((vars_object_t *)vars->data, (const sw_char_t*)name, 0, len);
	if (tmp == NULL)
	{
		return NULL;
	}
	else
	{
		return tmp;
	}	
}

sw_int_t shm_vars_clear_vars(void* object)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = vars_vclear((vars_object_t *)vars->data);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, vars_vclear error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	if (vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
		vars->tree = NULL;
	}

	return SW_OK;
}

sw_int_t shm_vars_remove_var(void* object, sw_char_t* name)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t	*vars = object;

	if (object == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = vars_delete_revariable((vars_object_t *)vars->data, name, 0);
	if (result != SW_OK)
	{
		return SW_ERROR;		
	}

	return SW_OK;
}

int shm_vars_extvar_serialize(sw_xmltree_t *ext_var, sw_buf_t *buf)
{
	int ret = 0, length = 0, iExtLen = 0;
	char sLenStr[9];


	length = xml_pack_length(ext_var);
	if (length < 0)
	{
		pub_log_error("%s, %d, Get length of pack failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = pub_buf_chksize(buf, length);/* 扩展变量长度 */
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_buf_chksize error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_xml_pack_ext(ext_var,buf->data+buf->len+8);
	iExtLen=strlen(buf->data+buf->len+8);
	sprintf(sLenStr,"%08d",iExtLen+8);
	memcpy(buf->data+buf->len,sLenStr,8);
	buf->len +=strlen(buf->data+buf->len+8)+8;
	pub_log_info("[%s][%d] external vars serialize success!", __FILE__, __LINE__);
	return SW_OK;

}

sw_int32_t shm_vars_serialize(void* object, sw_buf_t* sw_buf)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t	*vars = object;
	vars_object_t	*shm_vars = NULL;
	sw_buf_t	xml_buf;
	sw_char_t	len_buf[16];
	sw_char_t	xml_flag[16];
	sw_int32_t	xml_len = 0;

	if (object == NULL || sw_buf == NULL)
	{
		pub_log_error("%s, %d, Param error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*sysnc vars data in process space to shm space.*/
	shm_vars = (vars_object_t*)vars->data;

	result = pub_buf_chksize(sw_buf, 11);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_buf_chksize error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*serialize vid*/
	pub_mem_memzero(sw_buf->data, sw_buf->size);
	sprintf(sw_buf->data, "shm%08d", shm_vars->vid);
	sw_buf->len = strlen(sw_buf->data);

	pub_log_info("%s, %d, Start to serialize shm vars,[%d/%d], vid[%d]."
			, __FILE__, __LINE__, sw_buf->len, sw_buf->size, shm_vars->vid);

	pub_mem_memzero(&xml_buf, sizeof(xml_buf));
	result = pub_buf_init(&xml_buf);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_buf_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*save xml content to shm_vars*/
	if (vars->tree != NULL)
	{
		pub_log_info("[%s][%d] start to serialize ext vars...", __FILE__, __LINE__);	
		result = shm_vars_extvar_serialize(vars->tree, &xml_buf);
		if (result  != SW_OK)
		{
			pub_log_error("[%s][%d] iLocExtSerialize error.",__FILE__,__LINE__);
			pub_buf_clear(&xml_buf);
			return SW_ERROR;
		}

		xml_buf.data[xml_buf.len] = '\0';
		pub_log_debug("[%s][%d] xml_content[%s] length[%d].",__FILE__,__LINE__,xml_buf.data,xml_buf.len);

		pub_mem_memzero(xml_flag, sizeof(xml_flag));
		result = shm_vars_get_variable(vars, VARS_XML_FLAG, xml_flag);
		if (strlen(xml_flag) > 0)
		{
			xml_len = xml_buf.len;
			pub_log_info("[%s][%d] serialize xml multi-times.",__FILE__,__LINE__);
		}
		else
		{
			xml_len = xml_buf.len;
			pub_log_info("[%s][%d] serialize xml first-times.",__FILE__,__LINE__);
			result = vars_set_string(shm_vars, VARS_XML_FLAG, "1");
			if (result != SW_OK)
			{
				pub_log_error("[%s][%d] Set variable name[%s] error.",__FILE__,__LINE__,VARS_XML_FLAG);
				pub_buf_clear(&xml_buf);
				return SW_ERROR;
			}
		}


		/*save xml content*/
		result = vars_set_variable(shm_vars, VARS_TREE, VAR_TYPE_CHAR
				, xml_buf.data, xml_len);
		if (result != SW_OK)
		{
			pub_log_error("[%s][%d] vars_set_variable name[%s] error.",__FILE__,__LINE__,VARS_TREE);
			pub_buf_clear(&xml_buf);
			return SW_ERROR;
		}

		/*save xml length*/
		pub_mem_memzero(&len_buf, sizeof(len_buf));
		sprintf(len_buf, "%d", xml_buf.len);
		result = vars_set_string(shm_vars, VARS_CONTENT_LEN, len_buf);
		if (result != SW_OK)
		{
			pub_log_error("[%s][%d] vars_set_variable name[%s] error.",__FILE__,__LINE__,VARS_TREE);
			pub_buf_clear(&xml_buf);
			return SW_ERROR;
		}

		pub_log_info("[%s][%d] ext vars serialize success.", __FILE__, __LINE__);
	}
	pub_buf_clear(&xml_buf);

	result = vars_sync(shm_vars);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars_sync fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Shm vars serialize success! vid=[%d]", __FILE__, __LINE__, shm_vars->vid);

	return sw_buf->len;
}

sw_int32_t shm_vars_unserialize(void* object, sw_char_t *buf)
{
	sw_int_t	result = SW_ERROR;
	sw_loc_vars_t	*vars = object;
	vars_object_t	*shm_vars = NULL;
	sw_int32_t	offset = 0;
	sw_char_t	vid_buf[16];
	sw_int32_t	vid = -1;
	sw_char_t	*xml_addr = NULL;
	sw_char_t	*tmp = NULL;
	sw_int32_t	mem_len = 0;
	sw_int32_t	content_len = 0;

	if (object == NULL || buf == NULL)
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*get vid*/
	shm_vars = (vars_object_t *)vars->data;
	pub_mem_memzero(vid_buf, sizeof(vid_buf));

	offset += 3;
	pub_mem_memcpy(vid_buf, buf + offset, 8);
	vid = atoi(vid_buf);
	pub_log_info("[%s][%d] unserialize vid[%d]",__FILE__,__LINE__,vid);

	/*get shm_vars according vid.*/
	result = vars_object_creat(vid, shm_vars);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars_get_object [%d] error."
				,__FILE__,__LINE__,vid);
		return SW_ERROR;
	}
	offset += 8;

	/*get xml content from shm_vars and create xml tree.*/
	xml_addr = (sw_char_t *)vars_get_variable(shm_vars, VARS_TREE, &mem_len);
	if (xml_addr != NULL)
	{
		tmp = (sw_char_t *)vars_get_variable(shm_vars, VARS_CONTENT_LEN, &mem_len);
		if (tmp == NULL)
		{
			pub_log_error("[%s][%d] no %s", __FILE__, __LINE__, VARS_CONTENT_LEN);
			return SW_ERROR;
		}

		content_len = atoi(tmp);

		if (content_len > 0)
		{
			xml_addr[content_len] = '\0';
			pub_log_debug("[%s][%d]content_len[%d], Start to unserialize ext vars[%s]...", __FILE__, __LINE__, content_len, xml_addr);
			vars->tree = pub_xml_unpack_ext(xml_addr + SERI_LEN, content_len - SERI_LEN);

			pub_log_info("[%s][%d] ext vars unserialize finished,content_len=[%d].", __FILE__, __LINE__, content_len);	
		}
		else
		{
			pub_log_error("[%s][%d] content_len[%d] abnormal.", __FILE__, __LINE__, content_len);
			return SW_ERROR;
		}
	}
	else
	{
		vars->tree = NULL;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: shm_vars_tostream
 **功    能: 序列化变量池中的数据
 **输入参数: 
 **     object: 变量池对象
 **     buflen: 缓存空间长度
 **输出参数: 
 **     buf: 缓存空间
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 同步变量池数据
 **     2. 序列化变量池数据
 **注意事项: 
 **     shm_vars_serialize()并未进行真正的序列化
 **作    者: # Qifeng.zou # 2013.08.09 #
 ******************************************************************************/
sw_int_t shm_vars_tostream(void *object, char *buf, size_t buflen)
{
	sw_int_t ret = SW_ERR;
	sw_loc_vars_t *vars = object;
	vars_object_t *shm_vars = NULL;

	shm_vars = (vars_object_t *)vars->data;

	ret = vars_sync(shm_vars);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Vars sync failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return vars_serial(shm_vars, buf, buflen);
}

/******************************************************************************
 **函数名称: shm_vars_destream
 **功    能: 反序列化数据至变量池
 **输入参数: 
 **     object: 变量池对象
 **     buf: 缓存空间
 **输出参数: 
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 反序列化至变量池
 **注意事项: 
 **     shm_vars_unserialize()并未进行真正的反序列化
 **作    者: # Qifeng.zou # 2013.08.09 #
 ******************************************************************************/
sw_int_t shm_vars_destream(void *object, char *buf)
{
	sw_int_t ret = SW_ERR;
	sw_loc_vars_t *vars = object;
	vars_object_t *shm_vars = NULL;

	shm_vars = (vars_object_t *)vars->data;

	ret = vars_unserial(shm_vars, buf);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Vars unserial failed!", __FILE__, __LINE__);
		return -1;
	}

	return SW_OK;
}

#endif /*__SHM_VARS_SUPPORT__*/

sw_int_t heap_vars_alloc(void* object)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	vars->data = malloc(sizeof(sw_hvar_t));
	if (vars->data == NULL)
	{
		pub_log_error("[%s][%d] malloc fail.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t heap_vars_create(void* object, sw_int32_t igonre)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL || vars->data == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	hvar_init((sw_hvar_t *)vars->data);
	vars->tree = NULL;

	return SW_OK;
}

sw_int_t heap_vars_destroy(void* object)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (vars->data != NULL)
	{
		hvar = (sw_hvar_t *)vars->data;
		hvar->tree = vars->tree;
		hvar_clear(hvar);
		vars->tree = hvar->tree;
		free(vars->data);
		vars->data = NULL;
	}

	return SW_OK;
}

sw_int_t heap_vars_free(void* object)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (vars->data != NULL)
	{
		hvar = (sw_hvar_t *)vars->data;
		hvar->tree = vars->tree;
		hvar_clear(hvar);
		vars->tree = hvar->tree;
		free(vars->data);
		vars->data = NULL;
	}

	return SW_OK;
}

sw_int_t heap_vars_set_variable(void* object, sw_char_t* name ,sw_int32_t type
		, sw_char_t* value, sw_int32_t length)
{
	sw_int_t	result = SW_ERROR;
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || strlen(name) == 0 || value == NULL 
			|| length < 0 || (type != 'a' && type != 'b'))
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	result = hvar_set_value(hvar, name, value, length, type);

	return result;
}

sw_int_t heap_vars_set_string(void* object, sw_char_t*name, sw_char_t* value)
{
	sw_int_t	result = SW_ERROR;
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || strlen(name) == 0 || value == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	result = hvar_set_value(hvar, name, value, strlen(value), 'a');

	return result;
}

sw_char_t *heap_vars_get_null(void* object, sw_char_t *name, sw_int32_t len)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || strlen(name) == 0 || len <= 0 || (name[0] != '#' && name[0] != '$'))
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return NULL;
	}

	return hvar_get_null((sw_hvar_t *)vars->data, name, len);
}

sw_int_t heap_vars_print(void* object)
{
	/*TODO*/
	return SW_OK;
}

sw_int_t heap_vars_clone(void* object, sw_int32_t vid)
{
	/*TODO*/
	return SW_OK;
}

sw_int_t heap_vars_merge(void* object, sw_int32_t vid)
{
	/*TODO*/
	return SW_OK;
}

sw_int_t heap_vars_get_variable(void* object, sw_char_t* name, sw_char_t* value)
{
	sw_int_t	result = SW_ERROR;
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || value == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	result = hvar_get_value(hvar, name, value);

	return result;
}

sw_int32_t heap_vars_get_field_len(void* object, sw_char_t* name)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	return hvar_get_field_len(hvar, name);
}

sw_char_t* heap_vars_get_value_addr(void* object, sw_char_t* name, sw_int32_t *len)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL || name == NULL  || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return NULL;
	}

	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	return hvar_get_var_addr(hvar,  name, len);	
}

sw_int_t heap_vars_clear_vars(void* object)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	hvar_clear(hvar);

	return SW_OK;
}

sw_int_t heap_vars_remove_var(void* object, sw_char_t *name)
{
	sw_loc_vars_t*	vars = object;
	sw_int_t	result = SW_ERROR;

	if (object == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (name[0] == '$' || name[0] == '#')
	{
		result = hvar_remove(vars->data, name);
		if (result != SW_OK)
		{
			pub_log_error("[%s][%d] remove [%s] error!", __FILE__, __LINE__, name);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int32_t heap_vars_serialize(void *object, sw_buf_t *buf)
{
	int	result = 0;
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = object;

	if (NULL == buf || NULL == buf->data)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] var serialize begin...", __FILE__, __LINE__);
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	result = hvar_serialize(hvar, buf);
	if (result < 0)
	{
		pub_log_error("[%s][%d] var serialize error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] var serialize success!", __FILE__, __LINE__);

	return buf->len;
}

sw_int32_t heap_vars_unserialize(void *object, sw_char_t *str)
{
	int	result = 0;
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = (sw_loc_vars_t *)object;

	if (vars == NULL || str == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] var unserialize begin...", __FILE__, __LINE__);
	hvar = (sw_hvar_t *)vars->data;
	result = hvar_unserialize(hvar, str);
	if (result < 0)
	{
		pub_log_error("[%s][%d] var unserialize error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	vars->tree = hvar->tree;
	pub_log_info("[%s][%d] var unserialize success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int32_t heap_vars_tostream(void *object, sw_char_t *buf, size_t size)
{
	int	len = 0;
	int	result = 0;
	sw_hvar_t	*hvar = NULL;
	sw_buf_t	locbuf;
	sw_loc_vars_t	*vars = (sw_loc_vars_t *)object;

	if (NULL == buf || size <= 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] var serialize begin...", __FILE__, __LINE__);
	memset(&locbuf, 0x0, sizeof(locbuf));
	pub_buf_init(&locbuf);
	if (pub_buf_chksize(&locbuf, size) < 0)
	{
		pub_log_error("[%s][%d] Update buffer error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	result = hvar_serialize(hvar, &locbuf);
	if (result < 0)
	{
		pub_log_error("[%s][%d] var serialize error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	len = locbuf.len;
	memcpy(buf, locbuf.data, size);
	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] var serialize success!", __FILE__, __LINE__);

	return len;
}

sw_int_t heap_vars_destream(void *object, char *buf)
{
	sw_loc_vars_t	*vars = (sw_loc_vars_t *)object;
	return heap_vars_unserialize(vars, buf);
}

sw_int_t heap_vars_get_xml_variable(void * object, sw_char_t* name, sw_char_t* value)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = object;

	if (object == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;

	return extvar_get_value(hvar, name, value);
}

sw_int_t heap_vars_set_xml_variable(void* object, sw_char_t *var_name, sw_char_t *value, int creat)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = object;

	if (object == NULL || var_name == NULL || var_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	extvar_set_value(hvar, var_name, value, creat);
	vars->tree = hvar->tree;

	return SW_OK;
}

sw_int_t heap_vars_set_attr(void* object, sw_char_t *var_name, sw_char_t *attr_name, sw_char_t *attr_value)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = object;

	if (object == NULL || var_name == NULL || var_name[0] == '\0' || attr_name == NULL || attr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;
	extvar_set_attr(hvar, var_name, attr_name, attr_value);
	vars->tree = hvar->tree;

	return SW_OK;
}

sw_int_t heap_vars_get_attr(void *object, char *var_name, char *attr_name, char *attr_value)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = object;

	if (object == NULL || var_name == NULL || var_name[0] == '\0' || attr_name == NULL || attr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;

	return extvar_get_attr(hvar, var_name, attr_name, attr_value);
}

sw_int_t heap_vars_clear_xml(void* object)
{
	sw_hvar_t	*hvar = NULL;
	sw_loc_vars_t	*vars = object;

	if (object == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	hvar = (sw_hvar_t *)vars->data;
	hvar->tree = vars->tree;

	return extvar_clear(hvar);
}

static sw_xmlnode_t *comm_vars_extvar_get_node_by_exp(sw_xmltree_t *pxml, sw_xmlnode_t *pnode, char *express)
{
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode2 = NULL;
	sw_xmlnode_t	*pnode_bak = NULL;

	if (express[0] >= '0' && express[0] <= '9')
	{
		/****常量索引****/
		int	i = 0;
		int	index = atoi(express);

		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (i == index)
			{
				break;
			}
			pnode1 = pnode1->next;
			i++;
		}
		if (pnode1 == NULL)
		{
			/****未找到指定结点****/
			return NULL;
		}
		return pnode1;
	}
	else
	{
		/****表达式****/
		size_t	com_pos = 0;
		int	equal_flag = 0;/**相等标志,0-相等,1-不等****/
		char	loc_buf[128];

		pnode_bak = pxml->current;
		pxml->current = pnode;
		while (express[com_pos] != '\0')
		{
			if (express[com_pos] == '!' || express[com_pos] == '=')
			{
				break;
			}
			com_pos++;
		}
		if (express[com_pos] == '!' && express[com_pos+1] != '=')
		{
			pxml->current = pnode_bak;
			return NULL;
		}
		if (com_pos > sizeof(loc_buf) - 1)
		{
			pxml->current = pnode_bak;
			return NULL;
		}
		memcpy(loc_buf, express, com_pos);
		loc_buf[com_pos] = '\0';
		if (express[com_pos] == '!')
		{
			com_pos += 2;
			equal_flag = 1;
		}
		else
		{
			com_pos++;
			equal_flag = 0;
		}
		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			pxml->current = pnode1;
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			pnode2 = pub_xml_locnode(pxml, loc_buf);
			if (pnode2 == NULL || pnode2->value == NULL)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (equal_flag == 0)
			{
				if (strcmp(pnode2->value, express + com_pos) == 0)
				{
					/****相等,找到****/
					break;
				}
				else
				{
					pnode1 = pnode1->next;
					continue;
				}
			}
			else if (equal_flag == 1)
			{
				if (strcmp(pnode2->value, express + com_pos) != 0)
				{
					/****不等,找到****/
					break;
				}
				else
				{
					pnode1 = pnode1->next;
					continue;
				}
			}
			pnode1 = pnode1->next;
		}
		pxml->current = pnode_bak;
		if (pnode1 != NULL)
		{
			return pnode1;
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}

sw_xmlnode_t *comm_loc_get_ext_var(sw_loc_vars_t *vars, sw_char_t *name, sw_int32_t creat)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	sw_int32_t	pos = 0;
	sw_int32_t	pre = 0;

	if (vars == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error."
				,__FILE__, __LINE__);
		return NULL;
	}

	if(vars->tree == NULL)
	{
		vars->tree = pub_xml_unpack(NULL);
	}

	if(vars->tree == NULL)
	{
		return NULL;
	}

	if(name[0]=='.')
	{
		vars->tree->current = vars->tree->root;
	}

	pos = pre = 0;

	node_bak = vars->tree->current;

	while (name[pos] != '\0')
	{
		if(name[pos] == '(')
		{
			while (name[pos] != '\0')
			{
				if(name[pos] != ')')
				{
					pos++;
					continue;
				}
				else
				{
					break;
				}
			}

			if (name[pos]!=')')
			{
				vars->tree->current = node_bak;
				return NULL;
			}
		}

		pos++;
		if (name[pos] == '.' || name[pos] == '\0')
		{
			char var_name[1024];
			char experss[1024];
			if (name[pos-1]==')')
			{

				sw_int_t	i = pre;

				while (i < pos)
				{
					if(name[i]=='(')
					{
						break;
					}

					var_name[i-pre] = name[i];
					i++;
				}

				if(i < pos)
				{
					var_name[i - pre] = '\0';
					pub_mem_memcpy(experss, name + i + 1, pos - i - 2);
					experss[pos - i - 2] = '\0';
				}
				else
				{
					vars->tree->current = node_bak;
					return NULL;
				}
			}
			else
			{
				pub_mem_memcpy(var_name, name + pre, pos - pre);
				var_name[pos - pre] = '\0';
				experss[0] = '\0';
			}

			pub_str_zipspace(experss);
			node1 = pub_xml_locnode(vars->tree, var_name);
			if(node1 != NULL && experss[0] != '\0')
			{
				node1 = comm_vars_extvar_get_node_by_exp(vars->tree, node1, experss);
			}

			if (node1 == NULL)
			{
				if (name[pos] == '\0' && creat)
				{
					if (var_name[0] != '.')
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name, "",SW_NODE_ELEMENT);
						vars->tree->current = node_bak;
					}
					else
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name + 1, "",SW_NODE_ELEMENT);
						vars->tree->current = node2;
					}

					return node2;
				}

				vars->tree->current = node_bak;

				return NULL;
			}

			vars->tree->current = node1;

			if(name[pos] == '\0')
			{
				break;
			}

			pos++;
			pre = pos;
		}
	}

	node1 = vars->tree->current;
	vars->tree->current = node_bak;

	return node1;
}

sw_int_t comm_vars_get_xml_variable(void * object, sw_char_t* name, sw_char_t* value)
{
	sw_loc_vars_t*	vars = object;
	sw_xmlnode_t*	node1 = NULL;

	if (object == NULL || name == NULL || value == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	node1 = comm_loc_get_ext_var(vars, name, 0);
	if(node1 != NULL)
	{
		if (NULL != node1->value)
		{
			strcpy(value, node1->value);
		}
		else
		{
			value[0] = '\0';
		}

		return SW_OK;
	}
	else
	{
		return SW_ERROR;
	}
}

sw_int_t comm_vars_set_xml_variable(void* object, sw_char_t *var_name, sw_char_t *value, int creat)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_char_t	*pointer_in = NULL;
	sw_char_t	path_flag='1';
	sw_char_t	out[100];
	sw_char_t	node[256] = {0};
	sw_loc_vars_t	*vars = object;

	pointer_in = var_name;

	if (object == NULL || var_name == NULL || value == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (var_name[0] == '.')
	{
		pointer_in = pointer_in + 1;
		path_flag = '1';	
	}

	while (pointer_in != NULL)
	{
		pub_mem_memzero(out, sizeof(out));

		out[0] = '.';
		pointer_in = pub_str_msstrtok(pointer_in, out + 1, ".");
		if ('1' == path_flag)
		{
			path_flag = '0';
		}

		strcat(node, out);

		if ((value == NULL || value[0] == '\0') && creat != 1)
		{
			node1 = comm_loc_get_ext_var(vars, node, 0);

		}
		else
		{
			node1 = comm_loc_get_ext_var(vars, node, 1);
		}

		if (node1 == NULL)
		{
			return 0;
		}	
	}

	pub_xml_set_value(node1, value);

	return 0;
}

sw_int_t comm_vars_clear_xml(void* object)
{
	sw_loc_vars_t*	vars = object;

	if (object == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
		vars->tree = NULL;
		vars->remove_var(vars, VARS_TREE);
		vars->remove_var(vars, VARS_CONTENT_LEN);
		pub_log_info("[%s][%d] xml tree is cleared!", __FILE__, __LINE__);
	}

	return SW_OK;
}

sw_int_t  comm_set_ext_var_attr(sw_loc_vars_t *vars, sw_xmlnode_t *node
		, char *attr_name, char *attr_value)
{
	sw_int_t ret = 0;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node_bak = NULL;

	if (vars == NULL || node == NULL || attr_name == NULL 
			|| attr_value == NULL || strlen(attr_name) == 0 || strlen(attr_value) == 0)
	{
		pub_log_error("[%s][%d] node is NULL.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	node_bak = vars->tree->current;
	vars->tree->current = (sw_xmlnode_t*)node;

	node1 = pub_xml_locnode(vars->tree,attr_name);
	if (node1 == NULL)
	{
		node1 = pub_xml_addnode(vars->tree->current, attr_name, attr_value,SW_NODE_ATTRIB);
		vars->tree->current = node_bak;
		return SW_OK;
	}

	ret = pub_xml_set_value(node1, attr_value);
	if(0 != ret)
	{
		pub_log_error("[%s][%d] Set node value failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	vars->tree->current = node_bak;

	return SW_OK;
}


sw_int_t comm_vars_set_attr(void* object, sw_char_t *var_name
		, sw_char_t *attr_name, sw_char_t *attr_value)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_char_t	*pointer_in = NULL;
	sw_char_t	path_flag = '1';
	sw_char_t	out[100];
	sw_char_t	node_buf[256] = {0};
	sw_loc_vars_t	*vars = object;

	if (vars == NULL || var_name == NULL || attr_name == NULL || attr_value == NULL
			|| strlen(var_name) == 0 || strlen(attr_name) == 0 || strlen(attr_value) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pointer_in = var_name;
	if (var_name[0] == '.')
	{
		pointer_in = pub_str_msstrtok(pointer_in, out, ".");
		path_flag = '1';	
	}

	while (pointer_in != NULL)
	{
		pub_mem_memzero(out, sizeof(out));
		out[0] = '.';
		pointer_in = pub_str_msstrtok(pointer_in, out+1, ".");

		if('1' == path_flag)
		{
			path_flag = '0';
		}

		strcat(node_buf, out);
		node1 = comm_loc_get_ext_var(vars, node_buf, 1);
	}

	comm_set_ext_var_attr(vars, node1, attr_name, attr_value);	

	return SW_OK;
}

sw_char_t* comm_get_ext_var_attr(sw_loc_vars_t *vars, sw_xmlnode_t *node, char *attr)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node_bak = NULL;

	if (vars == NULL || node == NULL || attr == NULL)
	{
		return NULL;
	}

	node_bak = vars->tree->current;
	vars->tree->current = (sw_xmlnode_t *)node;

	node1 = pub_xml_locnode(vars->tree, attr);
	if(node1 == NULL)
	{
		vars->tree->current = node_bak;
		return NULL;
	}

	vars->tree->current = node_bak;

	return node1->value;
}

sw_int_t comm_vars_get_attr(void *object, char *var_name
		, char *attr_name, char *attr_value)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_loc_vars_t *vars = object;
	sw_char_t	*tmp = NULL;

	if (object == NULL || var_name == NULL || attr_name == NULL || attr_value == NULL
			|| strlen(var_name) == 0 || strlen(attr_name) == 0)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	node1 = comm_loc_get_ext_var(vars, var_name, 0);
	if (node1 != NULL)
	{
		tmp = comm_get_ext_var_attr(vars, node1, attr_name);
		if (tmp != NULL)
		{
			strcpy(attr_value, tmp);
			return SW_OK;
		}
		else
		{
			return SW_ERROR;
		}
	}
	else
	{
		return SW_ERROR;
	}
}

static sw_xmlnode_t *comm_vars_extvar_get_node_by_exp_ext(sw_xmltree_t *pxml, sw_xmlnode_t *pnode, char *express, int creat, char *varname)
{
	sw_xmlnode_t	*pnode1 = NULL;

	if (express[0] >= '0' && express[0] <= '9')
	{
		/****常量索引****/
		int	i = 0;
		int	index = atoi(express);

		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (i == index)
			{
				break;
			}
			pnode1 = pnode1->next;
			i++;
		}
		if (pnode1 == NULL)
		{
			/****未找到指定结点****/
			if (creat == 1)
			{
				while (i < index)
				{
					pnode1 = pub_xml_addnode(pxml->current, varname, "", SW_NODE_ELEMENT);
					pnode1 = pnode1->next;
					i++;
				}
				return pnode1;
			}
			return NULL;
		}
		return pnode1;
	}

	return NULL;
}

sw_xmlnode_t *comm_loc_get_ext_var_ext(sw_loc_vars_t *vars, sw_char_t *name, sw_int32_t creat)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	sw_int32_t	pos = 0;
	sw_int32_t	pre = 0;

	if (vars == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error."
				,__FILE__, __LINE__);
		return NULL;
	}

	if(vars->tree == NULL)
	{
		vars->tree = pub_xml_unpack(NULL);
	}

	if(vars->tree == NULL)
	{
		return NULL;
	}

	if(name[0]=='.')
	{
		vars->tree->current = vars->tree->root;
	}

	pos = pre = 0;

	node_bak = vars->tree->current;

	while (name[pos] != '\0')
	{
		if(name[pos] == '(')
		{
			while (name[pos] != '\0')
			{
				if(name[pos] != ')')
				{
					pos++;
					continue;
				}
				else
				{
					break;
				}
			}

			if (name[pos]!=')')
			{
				vars->tree->current = node_bak;
				return NULL;
			}
		}

		pos++;
		if (name[pos] == '.' || name[pos] == '\0')
		{
			char var_name[1024];
			char experss[1024];
			if (name[pos-1]==')')
			{

				sw_int_t	i = pre;

				while (i < pos)
				{
					if(name[i]=='(')
					{
						break;
					}

					var_name[i-pre] = name[i];
					i++;
				}

				if(i < pos)
				{
					var_name[i - pre] = '\0';
					pub_mem_memcpy(experss, name + i + 1, pos - i - 2);
					experss[pos - i - 2] = '\0';
				}
				else
				{
					vars->tree->current = node_bak;
					return NULL;
				}
			}
			else
			{
				pub_mem_memcpy(var_name, name + pre, pos - pre);
				var_name[pos - pre] = '\0';
				experss[0] = '\0';
			}

			pub_str_zipspace(experss);
			node1 = pub_xml_locnode(vars->tree, var_name);
			if(experss[0] != '\0')
			{
				node1 = comm_vars_extvar_get_node_by_exp_ext(vars->tree, node1, experss, creat, var_name);
			}

			if (node1 == NULL)
			{
				if (name[pos] == '\0' && creat)
				{
					if (var_name[0] != '.')
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name, "",SW_NODE_ELEMENT);
						vars->tree->current = node_bak;
					}
					else
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name + 1, "",SW_NODE_ELEMENT);
						vars->tree->current = node2;
					}

					return node2;
				}

				vars->tree->current = node_bak;

				return NULL;
			}

			vars->tree->current = node1;

			if(name[pos] == '\0')
			{
				break;
			}

			pos++;
			pre = pos;
		}
	}

	node1 = vars->tree->current;
	vars->tree->current = node_bak;

	return node1;
}

int msstrtokfinal(char *instr, char *outstr, char *delimiter)
{
	char    *tmpstr = NULL;
	char    *laststr = NULL;

	if (memcmp(instr, delimiter, strlen(instr)) == 0)
	{
		return -1;
	}

	if (instr == NULL || strlen(instr) == 0 || delimiter == NULL || strlen(delimiter) == 0)
	{
		return -1;
	}

	tmpstr = strstr( instr, delimiter);
	while (tmpstr != NULL)
	{
		tmpstr = tmpstr+strlen( delimiter );
		laststr = tmpstr;
		tmpstr = strstr(tmpstr, delimiter );
	}
	tmpstr = laststr;

	if (tmpstr != NULL)
	{
		memcpy(outstr, tmpstr, strlen(tmpstr));
		return strlen(instr) - strlen(tmpstr) - strlen(delimiter);
	}

	return -1;
}

sw_int_t comm_vars_set_attr_ext(sw_loc_vars_t *vars, sw_char_t *name, sw_char_t *value, int creat)
{
	int	len = 0;
	sw_char_t	out[100];
	sw_char_t	node_buf[256];
	sw_char_t	var_name[256];
	sw_char_t	attr_name[256];

	if (vars == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(out, 0x0, sizeof(out));
	memset(node_buf, 0x0, sizeof(node_buf));
	memset(var_name, 0x0, sizeof(var_name));
	memset(attr_name, 0x0, sizeof(attr_name));

	len = msstrtokfinal(name, attr_name, ".");
	if (len < 0)
	{
		return -1;
	}
	strncpy(var_name, name, len);

	comm_vars_set_attr(vars, var_name, attr_name, value);

	return SW_OK;
}

sw_int_t comm_vars_set_xml_var_ext(sw_loc_vars_t *vars, sw_char_t *var_name, sw_char_t *value, int creat)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_char_t	*pointer_in = NULL;
	sw_char_t	path_flag='1';
	sw_char_t	out[100];
	sw_char_t	node[256] = {0};

	pointer_in = var_name;

	if (vars == NULL || var_name == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (var_name[0] == '.')
	{
		pointer_in = pointer_in + 1;
		path_flag = '1';	
	}

	while (pointer_in != NULL)
	{
		pub_mem_memzero(out, sizeof(out));

		out[0] = '.';
		pointer_in = pub_str_msstrtok(pointer_in, out + 1, ".");
		if ('1' == path_flag)
		{
			path_flag = '0';
		}

		strcat(node, out);

		if ((value == NULL || value[0] == '\0') && creat != 1)
		{
			node1 = comm_loc_get_ext_var_ext(vars, node, 0);

		}
		else
		{
			node1 = comm_loc_get_ext_var_ext(vars, node, 1);
		}

		if (node1 == NULL)
		{
			return 0;
		}	
	}

	pub_xml_set_value(node1, value);

	return 0;
}

char *set_zdxml_data(sw_loc_vars_t *vars, char *name, char *value, char attr, int creat)
{
	if (name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error! name is null!", __FILE__, __LINE__);
		return NULL;
	}

	if (name[0] != '.')
	{
		pub_log_error("[%s][%d] Name [%s] format error!", __FILE__, __LINE__, name);
		return NULL;
	}

	if (attr == '1')
	{
		comm_vars_set_attr_ext(vars, name, value, creat);
	}
	else
	{
		comm_vars_set_xml_var_ext(vars, name, value, creat);
	}

	return value;
}

sw_int_t pub_loc_vars_alloc(sw_loc_vars_t* vars, sw_vars_type_t vars_type)
{
	sw_int_t	result = SW_ERROR;

	if (vars == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (vars_type != SHM_VARS && vars_type != HEAP_VARS)
	{
		pub_log_error("[%s][%d] vars_type[%d] unknown.", vars_type);
		return SW_ERROR;
	}

	vars->vars_type = vars_type;

#if defined(__SHM_VARS_SUPPORT__)
	if (vars_type == SHM_VARS)
	{
		vars->alloc_mem = shm_vars_alloc;
		vars->create = shm_vars_create;
		vars->destroy = shm_vars_destroy;
		vars->free_mem = shm_vars_free;
		vars->set_var = shm_vars_set_variable;
		vars->set_string = shm_vars_set_string;
		vars->get_var = shm_vars_get_variable;
		vars->get_var_addr = shm_vars_get_value_addr;
		vars->get_field_len = shm_vars_get_field_len;
		vars->clear_vars = shm_vars_clear_vars;
		vars->remove_var = shm_vars_remove_var;
		vars->serialize = shm_vars_serialize;
		vars->unserialize = shm_vars_unserialize;
		vars->tostream = shm_vars_tostream;
		vars->destream = shm_vars_destream;
		vars->get_null = shm_vars_get_null;
		vars->print = shm_vars_print;
		vars->clone = shm_vars_clone;
		vars->merge = shm_vars_merge;
	}
	else
#endif /*__SHM_VARS_SUPPORT__*/
	{
		vars->alloc_mem = heap_vars_alloc;
		vars->create = heap_vars_create;
		vars->destroy = heap_vars_destroy;
		vars->free_mem = heap_vars_free;
		vars->set_var = heap_vars_set_variable;
		vars->set_string = heap_vars_set_string;
		vars->get_var_addr = heap_vars_get_value_addr;
		vars->get_field_len = heap_vars_get_field_len;
		vars->get_var = heap_vars_get_variable;
		vars->clear_vars = heap_vars_clear_vars;
		vars->remove_var = heap_vars_remove_var;
		vars->serialize = heap_vars_serialize;
		vars->unserialize = heap_vars_unserialize;
		vars->tostream = heap_vars_tostream;
		vars->destream = heap_vars_destream;
		vars->get_null = heap_vars_get_null;
		vars->print = heap_vars_print;
		vars->clone = heap_vars_clone;
		vars->merge = heap_vars_merge;
	}
	vars->get_xml_var = comm_vars_get_xml_variable;
	vars->set_xml_var = comm_vars_set_xml_variable;
	vars->clear_xml_vars = comm_vars_clear_xml;
	vars->set_attr = comm_vars_set_attr;
	vars->get_attr = comm_vars_get_attr;

	result = vars->alloc_mem(vars);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars create error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t pub_loc_vars_create(sw_loc_vars_t *vars, sw_int32_t vid)
{
	sw_int_t	result = SW_ERROR;

	if (vars == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = vars->create(vars, vid);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t pub_loc_vars_free(sw_loc_vars_t* vars)
{
	sw_int_t	result = SW_ERROR;

	if (vars == NULL)
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (vars->vars_type != SHM_VARS && vars->vars_type != HEAP_VARS)
	{
		pub_log_error("[%s][%d] unknown vars type[%d]."
				, __FILE__, __LINE__, vars->vars_type);
		return SW_ERROR;
	}

	result = vars->free_mem(vars);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars destory error, vars type[%d]."
				, __FILE__, __LINE__, vars->vars_type);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t pub_vars_alloc(sw_vars_type_t vars_type)
{
	sw_int_t	result = SW_ERROR;

	pub_mem_memzero(&g_vars, sizeof(sw_loc_vars_t));

	result = pub_loc_vars_alloc(&g_vars, vars_type);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] pub_loc_vars_alloc error, type[%d]."
				,__FILE__, __LINE__, vars_type);
		return SW_ERROR;
	}

	return result;
}

sw_int_t pub_vars_create(sw_int32_t vid)
{
	sw_int_t	result = SW_ERROR;

	result = g_vars.create(&g_vars, vid);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t pub_vars_destory()
{
	sw_int_t	result = SW_ERROR;

	result = g_vars.destroy(&g_vars);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars destroy error, type[%d]."
				,__FILE__, __LINE__, g_vars.vars_type);
		return SW_ERROR;
	}

	return result;	
}

sw_int_t pub_vars_free()
{
	sw_int_t	result = SW_ERROR;

	result = g_vars.free_mem(&g_vars);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars destroy error, type[%d]."
				,__FILE__, __LINE__, g_vars.vars_type);
		return SW_ERROR;
	}

	return result;
}

sw_int_t pub_vars_set_variable(sw_char_t *name, sw_int32_t type
		, sw_char_t *value, sw_int32_t length)
{
	sw_int_t	result = SW_ERROR;

	if (name == NULL || value == NULL || length <= 0 || (type != 'a' && type != 'b'))
	{
		pub_log_error("[%s][%d] Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = g_vars.set_var(&g_vars, name, type, value, length);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] vars_set_variable error, type[%d] name[%s]."
				,__FILE__, __LINE__, g_vars.vars_type, name);
		return SW_ERROR;		
	}

	return SW_OK;
}

sw_int_t pub_vars_set_string(sw_char_t *name, sw_char_t* value)
{
	sw_int_t	result = SW_OK;

	if (name == NULL || value == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = g_vars.set_string(&g_vars, name, value);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, set_string error, type[%d] name[%s]."
				,__FILE__, __LINE__, g_vars.vars_type, name);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t pub_vars_get_variable(sw_char_t *name, sw_char_t* value)
{
	sw_int_t	result = SW_ERROR;

	if (name == NULL || value == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = g_vars.get_var(&g_vars, name, value);
	if (result < 0)
	{
		pub_log_error("%s, %d, set_string error, type[%d] name[%s]."
				,__FILE__, __LINE__, g_vars.vars_type, name);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int32_t pub_vars_serialize(sw_buf_t* buf)
{
	sw_int_t	result = SW_ERROR;

	if (buf == NULL)
	{
		pub_log_error("%s, %d, Param error.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	result = g_vars.serialize(&g_vars, buf);
	if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, serialize error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int32_t pub_vars_unserialize(sw_char_t *buf)
{
	sw_int_t	result = SW_ERROR;

	if (NULL == buf)
	{
		pub_log_error("%s, %d, Param error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = g_vars.unserialize(&g_vars, buf);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, unserialize error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t pub_vars_reset_tree(sw_xmltree_t* tree)
{
	if (g_vars.tree != NULL)
	{
		pub_xml_deltree(g_vars.tree);
		g_vars.tree = NULL;
	}

	g_vars.tree = tree;

	return SW_OK;
}

sw_xmltree_t* pub_vars_get_tree()
{
	return g_vars.tree;
}

sw_loc_vars_t* pub_get_global_vars()
{
	return &g_vars;
}

sw_int_t pub_vars_merge(sw_loc_vars_t *vars, sw_int32_t vid)
{
	return vars->merge(vars, vid);
}

sw_int_t pub_vars_destroy_by_mtype(int vid)
{
	int	ret = 0;
	char	buf[128];
	sw_loc_vars_t	vars;

	memset(buf, 0x0, sizeof(buf));
	memset(&vars, 0x0, sizeof(vars));

	ret = pub_loc_vars_alloc(&vars, SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Alloc vars error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(buf, "SHM%08d", vid);
	ret = vars.unserialize((void *)&vars, buf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Vars unserialize error! mtype=[%d]", __FILE__, __LINE__, vid);
		vars.free_mem(&vars);
		return SW_ERROR;
	}
	ret = vars.destroy(&vars);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Destroy var error! mtype=[%d]", __FILE__, __LINE__, vid);
		vars.free_mem(&vars);
		return -1;
	}
	vars.free_mem(&vars);
	pub_log_info("[%s][%d] Destroy vars success! mtype=[%d]", __FILE__, __LINE__, vid);

	return 0;
}

int pub_vars_pack_node(sw_loc_vars_t *vars, char *name, char *pack)
{
	int	ret = 0;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	if (vars == NULL || name == NULL || pack == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	xml = vars->tree;
	node = pub_xml_locnode(xml, name);
	if (node == NULL)
	{
		pack[0] = '\0';
		return 0;
	}

	ret = pub_xml_pack_node(xml, node, pack);
	if (ret < 0)
	{	
		pub_log_error("[%s][%d] Pack node error!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

