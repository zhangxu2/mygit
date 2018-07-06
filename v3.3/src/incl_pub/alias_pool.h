#if !defined(__ALIAS_POOL__)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>

#include "slab.h"


#define ALIAS_NAME_MAX_LEN  (32)        /* 别名长度 */
#define ALIAS_NODE_NUM      (301)       /* 别名节点数 */

/* 数据库变量信息 */
typedef struct _alias_node_t
{
    char name[ALIAS_NAME_MAX_LEN];      /* 别名 */
    void *value;                        /* 别名值 */
    int type;                           /* 别名值类型 */
    size_t length;                      /* 别名值实际长度 */
    size_t size;                        /* 别名值所占空间 */

    struct _alias_node_t *next;         /* 下一个节点 */
}alias_node_t;

typedef struct
{
    alias_node_t node[ALIAS_NODE_NUM];  /* 别名节点 */
    slab_link_t spl;                    /* Slab pool link */
}alias_pool_t;


extern int alias_init(alias_pool_t *ap, size_t size);
extern void alias_destory(alias_pool_t *ap);
extern void *alias_alloc(alias_pool_t *ap, size_t size);
extern void alias_free(alias_pool_t *ap, void *p);
extern int alias_set(alias_pool_t *ap, const char *name, const void *value, int length);
extern const void *alias_get(alias_pool_t *ap, const char *name);
extern int alias_delete(alias_pool_t *ap, const char *name);
extern int alias_delete_all(alias_pool_t *ap);

#endif /*__ALIAS_POOL__*/
