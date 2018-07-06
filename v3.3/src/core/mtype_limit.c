/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : BP run-time shm. 
 *** name    : mtype.c 
 *** function: mtype sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "mtype_limit.h"
#include "sem_lock.h"
#include "pub_time.h"
#include "pub_cfg.h"
#include "pub_mem.h"
#include "pub_log.h"

#define MTYPE_TIMEOUT	3*60*60

/*mtype limit info's addr in shm_run.*/
SW_PROTECTED sw_mtype_limit_t*	g_shm_prdt_limit = NULL;
SW_PROTECTED sw_mtype_limit_t*	g_shm_lsn_limit = NULL;


sw_int_t prdt_limit_loc_init(sw_mtype_limit_t*	shm_prdt_limit, sw_syscfg_t* sys_cfg)
{
	sw_int32_t		lock_id = -1;
	sw_int32_t		prdt_max = -1;
	sw_int32_t		prdt_limit_size = -1;

	if (sys_cfg == NULL || shm_prdt_limit == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (sys_cfg->prdt_max <= 0)
	{
		prdt_max = PRDT_MAX_DEFAULT;
	}
	else
	{
		prdt_max = sys_cfg->prdt_max;
		
	}
	
	prdt_limit_size = sizeof(sw_mtype_limit_head_t) 
			+ (prdt_max - 1) * sizeof(sw_mtype_limit_node_t);
	pub_mem_memzero((sw_char_t*)shm_prdt_limit, prdt_limit_size);

	/*get a lockid*/
	lock_id = sem_new_lock_id();
	if (lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*lock*/
	sem_mutex_lock(lock_id);

	shm_prdt_limit->head.lock_id = lock_id;
	shm_prdt_limit->head.size = prdt_max;
	shm_prdt_limit->head.use = 0;
	shm_prdt_limit->head.warn = sys_cfg->session_max * 2 / 10;

	sem_mutex_unlock(lock_id);
	return 0;
}

sw_int_t prdt_limit_register(void)
{
	sw_int32_t		limit = 0;
	sw_char_t		xmlname[256];
	sw_char_t		prdt[64];
	sw_xmltree_t    *xml = NULL;
	sw_xmlnode_t    *node = NULL;
	sw_xmlnode_t    *node1 = NULL;

	memset(xmlname, 0x0, sizeof(xmlname)-1);
	snprintf(xmlname, sizeof(xmlname)-1,  "%s/cfg/products.xml", getenv("SWWORK"));
	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.PRODUCT");
	while (node != NULL)
	{
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config [NAME]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		memset(prdt, 0x0, sizeof(prdt));
		strncpy(prdt, node1->value, sizeof(prdt)-1);

		node1 = pub_xml_locnode(xml, "STATUS");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config [STATUS]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (node1->value[0] != '1')
		{
			node = node->next;
			continue;
		}

		node1 = pub_xml_locnode(xml, "MTYPE_MAX");
		if (node1 == NULL || node1->value == NULL)
		{
			limit = 0;
		}
		else
		{
			limit = atoi(node1->value);
		}
		loc_prdt_limit_register(prdt, limit);
			
		node = node->next;
	}
	pub_xml_deltree(xml);

	return 0;
}
	
sw_int_t prdt_limit_init(sw_mtype_limit_t *shm_prdt_limit, sw_syscfg_t* sys_cfg)
{
	sw_int_t	result = SW_ERROR;

	result = prdt_limit_loc_init(shm_prdt_limit, sys_cfg);
	if (result != SW_OK)
	{
		pub_log_error("mtype_loc_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	g_shm_prdt_limit = shm_prdt_limit;

	result = prdt_limit_register();
	if (result != SW_OK)
	{
		pub_log_error("prdt_limit_register error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t lsn_limit_loc_init(sw_mtype_limit_t*	shm_lsn_limit, sw_syscfg_t* sys_cfg)
{
	sw_int32_t		lock_id = -1;
	sw_int32_t		lsn_max = -1;
	sw_int32_t		lsn_limit_size = -1;

	if (sys_cfg == NULL || shm_lsn_limit == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (sys_cfg->lsn_max <= 0)
	{
		lsn_max = LSN_MAX_DEFAULT;
	}
	else
	{
		lsn_max = sys_cfg->lsn_max;
		
	}
	
	lsn_limit_size = sizeof(sw_mtype_limit_head_t) 
			+ (lsn_max - 1) * sizeof(sw_mtype_limit_node_t);
	pub_mem_memzero((sw_char_t*)shm_lsn_limit, lsn_limit_size);

	/*get a lockid*/
	lock_id = sem_new_lock_id();
	if (lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*lock*/
	sem_mutex_lock(lock_id);

	shm_lsn_limit->head.lock_id = lock_id;
	shm_lsn_limit->head.size = lsn_max;
	shm_lsn_limit->head.use = 0;
	shm_lsn_limit->head.warn = sys_cfg->session_max * 2 / 10;

	sem_mutex_unlock(lock_id);

	return 0;
}

sw_int_t lsn_limit_register(void)
{
	sw_int32_t		limit = 0;
	sw_char_t		xmlname[256];
	sw_char_t		lsn[64];
	sw_xmltree_t    *xml = NULL;
	sw_xmlnode_t    *node = NULL;
	sw_xmlnode_t    *node1 = NULL;

	memset(xmlname, 0x0, sizeof(xmlname)-1);
	snprintf(xmlname, sizeof(xmlname)-1,  "%s/cfg/channels.xml", getenv("SWWORK"));
	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.CHANNEL");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHANNEL") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "LISTEN");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config [LISTEN]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		memset(lsn, 0x0, sizeof(lsn));
		strncpy(lsn, node1->value, sizeof(lsn)-1);

		node1 = pub_xml_locnode(xml, "STATUS");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config [STATUS]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (node1->value[0] != '1')
		{
			node = node->next;
			continue;
		}

		node1 = pub_xml_locnode(xml, "MTYPE_MAX");
		if (node1 == NULL || node1->value == NULL)
		{
			limit = 0;
		}
		else
		{
			limit = atoi(node1->value);
		}
		loc_lsn_limit_register(lsn, limit);
			
		node = node->next;
	}
	pub_xml_deltree(xml);

	return 0;
}
	
sw_int_t lsn_limit_init(sw_mtype_limit_t *shm_lsn_limit, sw_syscfg_t* sys_cfg)
{
	sw_int_t	result = SW_ERROR;

	result = lsn_limit_loc_init(shm_lsn_limit, sys_cfg);
	if (result != SW_OK)
	{
		pub_log_error("mtype_loc_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	g_shm_lsn_limit = shm_lsn_limit;

	result = lsn_limit_register();
	if (result != SW_OK)
	{
		pub_log_error("lsn_limit_register error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int32_t prdt_limit_get_size(sw_syscfg_t *sys_cfg)
{
	sw_int32_t	prdt_limit_size = 0;
	sw_int32_t	prdt_max = 0;
	
	if (sys_cfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (sys_cfg->prdt_max <= 0)
	{
		prdt_max = PRDT_MAX_DEFAULT;
	}
	else
	{
		prdt_max = sys_cfg->prdt_max;
		
	}
	
	prdt_limit_size = sizeof(sw_mtype_limit_t) 
			+ (prdt_max - 1) * sizeof(sw_mtype_limit_node_t);

	return prdt_limit_size;
}

sw_int32_t lsn_limit_get_size(sw_syscfg_t *sys_cfg)
{
	sw_int32_t	lsn_limit_size = 0;
	sw_int32_t	lsn_max = 0;
	
	if (sys_cfg == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (sys_cfg->prdt_max <= 0)
	{
		lsn_max = LSN_MAX_DEFAULT;
	}
	else
	{
		lsn_max = sys_cfg->lsn_max;
		
	}
	
	lsn_limit_size = sizeof(sw_mtype_limit_t) 
			+ (lsn_max + 1) * sizeof(sw_mtype_limit_node_t);

	return lsn_limit_size;
}

sw_int_t prdt_limit_set_addr(sw_char_t* addr)
{
	if (NULL == addr)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_prdt_limit = (sw_mtype_limit_t*)addr;

	return SW_OK;
}

/******************************************************************************
 ** Name : prdt_limit_get_addr
 ** Desc : 获取产品Mtype限制的内存首地址
 ** Input: NONE
 ** Output: NONE
 ** Return: 产品Mtype限制的内存首地址
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
const sw_mtype_limit_t *prdt_limit_get_addr(void)
{
    return g_shm_prdt_limit;
}

sw_int_t lsn_limit_set_addr(sw_char_t* addr)
{
	if (NULL == addr)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_shm_lsn_limit = (sw_mtype_limit_t*)addr;

	return SW_OK;
}

/******************************************************************************
 ** Name : lsn_limit_get_addr
 ** Desc : 获取侦听Mtype限制的内存首地址
 ** Input: NONE
 ** Output: NONE
 ** Return: 侦听Mtype限制的内存首地址
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.01 #
 ******************************************************************************/
const sw_mtype_limit_t *lsn_limit_get_addr(void)
{
    return g_shm_lsn_limit;
}

sw_int32_t loc_prdt_limit_register(char *prdt, int limit)
{
	sw_mtype_limit_node_t *node = NULL;
	if (g_shm_prdt_limit == NULL || prdt == NULL || prdt[0] == '\0')
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_mutex_lock(g_shm_prdt_limit->head.lock_id);
	if (g_shm_prdt_limit->head.use >= g_shm_prdt_limit->head.size)
	{
		pub_log_error("%s, %d, product is out of limit[%d]", __FILE__, __LINE__, g_shm_prdt_limit->head.size);
		sem_mutex_unlock(g_shm_prdt_limit->head.lock_id);
		return SW_ERROR;
	}

	node = &(g_shm_prdt_limit->first) + g_shm_prdt_limit->head.use;
	node->limit = limit;
	strncpy(node->name, prdt, sizeof(node->name)-1);
	g_shm_prdt_limit->head.use ++;
	sem_mutex_unlock(g_shm_prdt_limit->head.lock_id);
	pub_log_info("[%s][%d] product %s mtype_max=%d", __FILE__, __LINE__, prdt, limit);

	return SW_OK;
}

sw_int32_t loc_lsn_limit_register(char *lsn, int limit)
{
	sw_mtype_limit_node_t  *node = NULL;
	if (g_shm_lsn_limit == NULL || lsn == NULL || lsn[0] == '\0')
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_mutex_lock(g_shm_lsn_limit->head.lock_id);
	if (g_shm_lsn_limit->head.use >= g_shm_lsn_limit->head.size)
	{
		pub_log_error("%s, %d, listen is out of limit[%d]", __FILE__, __LINE__, g_shm_lsn_limit->head.size);
		sem_mutex_unlock(g_shm_lsn_limit->head.lock_id);
		return SW_ERROR;
	}

	node = &(g_shm_lsn_limit->first) + g_shm_lsn_limit->head.use;
	node->limit = limit;
	strncpy(node->name, lsn, sizeof(node->name)-1);
	g_shm_lsn_limit->head.use ++;
	sem_mutex_unlock(g_shm_lsn_limit->head.lock_id);
	pub_log_info("[%s][%d] listen %s mtype_max=%d", __FILE__, __LINE__, lsn, limit);

	return SW_OK;
}

sw_int32_t mtype_limit_check(char *prdt, char *lsn)
{
	int i = 0;
	int	warn = 0;
	sw_mtype_limit_node_t *prdt_limit = NULL;
	sw_mtype_limit_node_t *lsn_limit = NULL;
	if (g_shm_prdt_limit == NULL || g_shm_lsn_limit == NULL)
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (prdt != NULL && prdt[0] != '\0')
	{
		for (i = 0, prdt_limit = &(g_shm_prdt_limit->first); i < g_shm_prdt_limit->head.use; ++i, ++prdt_limit)
		{
			if (strcmp(prdt, prdt_limit->name) == 0)
			{
				if (prdt_limit->limit > 0 && prdt_limit->use >= prdt_limit->limit)
				{
					pub_log_error("[%s][%d] ERROR: product %s is out of mtype limit! use[%d] success[%d] fail[%d]", 
							__FILE__, __LINE__, prdt, prdt_limit->use, prdt_limit->success, prdt_limit->fail);
					return SW_ERROR;
				}
				prdt_limit->use ++;
				
				warn = prdt_limit->limit ? prdt_limit->limit / 2 : g_shm_prdt_limit->head.warn; 
				if (prdt_limit->use >= warn)
				{
					pub_log_error("[%s][%d] WARNING: product %s now has %d mtype inuse TAKE CARE!!!", 
							__FILE__, __LINE__, prdt, prdt_limit->use, prdt_limit->success, prdt_limit->fail);
				}
				break;
			}
		}
		prdt_limit = NULL;
	}

	if (lsn != NULL && lsn[0] != '\0')
	{
		for (i = 0, lsn_limit = &(g_shm_lsn_limit->first); i < g_shm_lsn_limit->head.use; ++i, lsn_limit++)
		{
			if (strcmp(lsn, lsn_limit->name) == 0)
			{
				if (lsn_limit->limit > 0 && lsn_limit->use >= lsn_limit->limit)
				{
					pub_log_error("[%s][%d] listen %s is out of mtype limit use[%d] success[%d] fail[%d]", 
							__FILE__, __LINE__, lsn, lsn_limit->use, lsn_limit->success, lsn_limit->fail);
					prdt_limit->use --;
					return SW_ERROR;
				}
				lsn_limit->use ++;

				warn = lsn_limit->limit ? lsn_limit->limit / 2 : g_shm_lsn_limit->head.warn; 
				if (lsn_limit->use >= warn)
				{
					pub_log_error("[%s][%d] WARNING: listen %s now has %d mtype inuse TAKE CARE!!!", 
							__FILE__, __LINE__, lsn, lsn_limit->use, lsn_limit->success, lsn_limit->fail);
				}
				break;
			}
		}
		lsn_limit = NULL;
	}

	return SW_OK;
}

sw_int32_t mtype_limit_free(char *prdt, char *lsn, int success)
{
	int i = 0;
	sw_mtype_limit_node_t *prdt_limit = NULL;
	sw_mtype_limit_node_t *lsn_limit = NULL;
	if (g_shm_prdt_limit == NULL || g_shm_lsn_limit == NULL)
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d] mtype_limit_free product[%s] listen[%s]", __FILE__, __LINE__, prdt, lsn);
	if (prdt != NULL && prdt[0] != '\0')
	{
		for (i = 0, prdt_limit = &(g_shm_prdt_limit->first); i < g_shm_prdt_limit->head.use; ++i, ++prdt_limit)
		{
			if (strcmp(prdt, prdt_limit->name) == 0)
			{
				prdt_limit->use --;
				if (success)
				{
					prdt_limit->success++;
				}
				else
				{
					prdt_limit->fail++;
				}
				break;
			}
		}
	}

	if (lsn != NULL && lsn[0] != '\0')
	{
		for (i = 0, lsn_limit = &(g_shm_lsn_limit->first); i < g_shm_lsn_limit->head.use; ++i, ++lsn_limit)
		{
			if (strcmp(lsn, lsn_limit->name) == 0)
			{
				lsn_limit->use --;
				if (success)
				{
					lsn_limit->success++;
				}
				else
				{
					lsn_limit->fail++;
				}
				break;
			}
		}
	}

	return 0;
}
