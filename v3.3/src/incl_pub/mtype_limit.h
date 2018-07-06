/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : BP run-time shm. 
 *** name    : mtype.h
 *** function: mtype sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __MTYPE_LIMIT_H__
#define __MTYPE_LIMIT_H__
#include "pub_type.h"
#include "pub_cfg.h"

struct sw_mtype_limit_node_s
{
	sw_int32_t	limit;
	sw_int32_t	use;
	sw_int32_t	success;
	sw_int32_t	fail;
	sw_char_t	name[64];
};

typedef struct sw_mtype_limit_node_s sw_mtype_limit_node_t;

/*shm mtype_limit head info*/
struct sw_mtype_limit_head_s
{
	sw_int32_t	lock_id;
	sw_int32_t	size;		/*shm mtype limit size*/
	sw_int32_t	use;		/*the prdt or lsn in use*/
	sw_int32_t	warn;		/*if limit is higher than warn will warning*/
};
typedef struct sw_mtype_limit_head_s sw_mtype_limit_head_t;

struct sw_mtype_limit_s
{
	sw_mtype_limit_head_t head;
	sw_mtype_limit_node_t first;
};

typedef struct sw_mtype_limit_s sw_mtype_limit_t;

sw_int_t prdt_limit_init(sw_mtype_limit_t *shm_prdt_limit, sw_syscfg_t* sys_cfg);
sw_int_t lsn_limit_init(sw_mtype_limit_t *shm_lsn_limit, sw_syscfg_t* sys_cfg);
sw_int32_t prdt_limit_get_size(sw_syscfg_t *sys_cfg);
sw_int32_t lsn_limit_get_size(sw_syscfg_t *sys_cfg);
sw_int_t prdt_limit_set_addr(sw_char_t* addr);
const sw_mtype_limit_t *prdt_limit_get_addr(void);
sw_int_t lsn_limit_set_addr(sw_char_t* addr);
const sw_mtype_limit_t *lsn_limit_get_addr(void);
sw_int32_t loc_prdt_limit_register(char *prdt, int limit);
sw_int32_t loc_lsn_limit_register(char *lsn, int limit);
sw_int32_t mtype_limit_free(char *prdt, char *lsn, int success);
sw_int32_t prdt_limit_register(void);
sw_int32_t lsn_limit_register(void);

#endif /* __MTYPE_LIMIT_H__ */
