#if !defined(__ALIAS_POOL__)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>

#include "slab.h"


#define ALIAS_NAME_MAX_LEN  (32)        /* �������� */
#define ALIAS_NODE_NUM      (301)       /* �����ڵ��� */

/* ���ݿ������Ϣ */
typedef struct _alias_node_t
{
    char name[ALIAS_NAME_MAX_LEN];      /* ���� */
    void *value;                        /* ����ֵ */
    int type;                           /* ����ֵ���� */
    size_t length;                      /* ����ֵʵ�ʳ��� */
    size_t size;                        /* ����ֵ��ռ�ռ� */

    struct _alias_node_t *next;         /* ��һ���ڵ� */
}alias_node_t;

typedef struct
{
    alias_node_t node[ALIAS_NODE_NUM];  /* �����ڵ� */
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
