#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "alias_pool.h"
#include "pub_log.h"

#define alias_node_is_used(node) ('\0' != node->name[0])    /* 别名节点是否被使用 */
#define alias_get_index(name) (alias_hash(name)%ALIAS_NODE_NUM) /* 获取HASH索引 */
#define alias_name_is_vaild(name) ('\0' != name[0])         /* 别名是否合法*/

/******************************************************************************
 ** Name : alias_init
 ** Desc : Initialize database variable pool.
 ** Input: 
 **     alias: Alias pool
 **     size: Size of varialbe pool
 ** Output: 
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
int alias_init(alias_pool_t *ap, size_t size)
{
    int ret = -1;

    memset(ap, 0, sizeof(alias_pool_t));
    
    ret = slab_link_init(&ap->spl, size);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Initialize slab pool link failed!", __FILE__, __LINE__);
        return -1;
    }
    
    return 0;
}

/******************************************************************************
 ** Name : alias_destory
 ** Desc : Destory alias pool.
 ** Input: 
 **     alias: Alias pool
 ** Output: 
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
void alias_destory(alias_pool_t *ap)
{
    slab_link_destory(&ap->spl);
    memset(ap, 0, sizeof(alias_pool_t));
}

/******************************************************************************
 ** Name : alias_alloc
 ** Desc : Alloc memory from pool.
 ** Input: 
 **     alias: Alias pool
 **     size: Size of memory
 ** Output: 
 ** Return: Address of memory.
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
void *alias_alloc(alias_pool_t *ap, size_t size)
{
    return slab_link_alloc(&ap->spl, size);
}

/******************************************************************************
 ** Name : alias_free
 ** Desc : Free special memory
 ** Input: 
 **     dbv: Database variable pool
 **     p: Address of special memory
 ** Output: 
 ** Return: 0:success !0:failed
 ** Process:
 **     1. Find pointer belong to which SLAB POOL.
 **     2. Release space
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
void alias_free(alias_pool_t *ap, void *p)
{
    slab_link_free(&ap->spl, p);
}

/******************************************************************************
 ** Name : alias_hash
 ** Desc : Compute hash value of string
 ** Input: 
 **     str: String
 ** Output: 
 ** Return: Hash value
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
unsigned int alias_hash(const char *str)
{
    const char *p = str;
    unsigned int hash = 5381;

    while (*p)
    {
        hash += (hash << 5) + (*p++);
    }

    return (hash & 0x7FFFFFFF);
}

/******************************************************************************
 ** Name : alias_set
 ** Desc : Set variable information
 ** Input: 
 **     alias: Alias pool
 **     name: Name of variable
 **     value: Value of variable
 **     length: Length of value
 ** Output: 
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Compute index of variable
 **     2. Search varialbe by name
 **     3. Set value of variable
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
int alias_set(alias_pool_t *ap, const char *name, const void *value, int length)
{
    unsigned idx = 0;
    size_t size = length + 1;
    alias_node_t *node = NULL, *prev = NULL;

    if(!alias_name_is_vaild(name))
    {
        pub_log_error("[%s][%d] Name is invalid! [%s]", __FILE__, __LINE__, name);
        return -1;
    }

    idx = alias_get_index(name);

    node = &ap->node[idx];
    prev = node;
    while(NULL != node)
    {
        if(alias_node_is_used(node))
        {
            if(0 == strcasecmp(node->name, name))
            {
                break; /* found */
            }
            
            prev = node;
            node = node->next;
            continue;
        }
        break;  /* Not used node */
    }

    if(NULL == node)
    {
        node = alias_alloc(ap, sizeof(alias_node_t));
        if(NULL == node)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            return -1;
        }
        prev->next = node;
    }

    if(node->size < size)
    {
        if(node->value)
        {
            alias_free(ap, node->value);
            node->value = NULL;
        }
        
        node->value = alias_alloc(ap, size);
        if(NULL == node->value)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            return -1;
        }
        node->size = size;
    }

    snprintf(node->name, sizeof(node->name), "%s", name);
    memset(node->value, 0, node->size);
    memcpy(node->value, value, size);

    node->length = length;
    
    return 0;
}

/******************************************************************************
 ** Name : alias_get
 ** Desc : Get the value of special variable
 ** Input: 
 **     alias: Alias pool
 **     name: Name of variable
 ** Output: 
 ** Return: Value of varialbe
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
const void *alias_get(alias_pool_t *ap, const char *name)
{
    unsigned int idx = 0;
    alias_node_t *node = NULL, *head = NULL;

    idx = alias_get_index(name);
    node = &ap->node[idx];
    head = node;
    while(NULL != node)
    {
        if(alias_node_is_used(node) || (head == node))
        {
            if(0 == strcasecmp(node->name, name))
            {
                return node->value;
            }
            node = node->next;
            continue;
        }
        pub_log_error("[%s][%d] Didn't find! [%s]\n", __FILE__, __LINE__, name);
        return NULL;
    }
    pub_log_error("[%s][%d] Didn't find! [%s]\n", __FILE__, __LINE__, name);
    return NULL;
}

/******************************************************************************
 ** Name : alias_delete
 ** Desc : Delete special variable
 ** Input: 
 **     alias: Alias pool
 **     name: Name of variable
 ** Output: 
 ** Return: 0: Success !0: Failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
int alias_delete(alias_pool_t *ap, const char *name)
{
    unsigned int idx = 0;
    alias_node_t *node = NULL, *prev = NULL;

    idx = alias_get_index(name);
    node = &ap->node[idx];
    prev = node;
    while(NULL != node)
    {
        if(alias_node_is_used(node))
        {
            if(0 == strcasecmp(node->name, name))
            {
                if(node->value)
                {
                    alias_free(ap, node->value);
                    node->value = NULL;
                }
                
                if(prev == node)
                {
                    memset(node->name, 0, sizeof(node->name));
                    node->size = 0;
                    node->length = 0;
                }
                else
                {
                    prev->next = node->next;
                    alias_free(ap, node);
                }
                return 0;
            }
            prev = node;
            node = node->next;
            continue;
        }
        return 0; /* Didn't find */
    }
    return 0; /* Didn't find */
}

/******************************************************************************
 ** Name : alias_delete_all
 ** Desc : Delete all variables
 ** Input: 
 **     alias: Alias pool
 ** Output: 
 ** Return: 0: Success !0: Failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.12 #
 ******************************************************************************/
int alias_delete_all(alias_pool_t *ap)
{
    unsigned idx = 0;
    alias_node_t *node = NULL, *next = NULL, *first = NULL;

    for(idx=0; idx<ALIAS_NODE_NUM; idx++)
    {
        node = &ap->node[idx];
        next = node->next;
        first = node;
        while((NULL != node) && alias_node_is_used(node))
        {
            next = node->next;
            if(node->value)
            {
                alias_free(ap, node->value);
                node->value = NULL;
            }
            
            if(first != node)
            {
                alias_free(ap, node);
                node = NULL;
            }
            else
            {
                memset(node, 0, sizeof(alias_node_t));
            }
            node = next;
        }
    }
    return 0;
}
