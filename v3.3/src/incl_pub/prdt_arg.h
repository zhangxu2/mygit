#if !defined(__PRDT_ARG_H__)
#define __PRDT_ARG_H__

#if defined(__PRDT_ARG__)
#include "param.h"
#include "pub_log.h"
#include "pub_type.h"


#define PRDT_GRP_NUM            (100)       /* ��Ʒ��������С���� */
#define PRDT_GRP_SIZE           (100*1024)  /* ��Ʒ��������С�ռ� */

#define PRDT_ARG_FLOCK_DIR      "tmp/prdtarg/"      /* ��Ʒ������·�� */
#define PRDT_GRP_FLOCK          ".prdt_grp_flock"   /* �������ļ�����Ӧ���ļ��� */
#define PRDT_ARG_FLOCK          ".prdt_arg_flock"   /* �����ļ�����Ӧ���ļ��� */


#define PRDT_ARG_VALUE_MAX_SIZE (128)

/* ��Ʒ�������� */
typedef struct
{
    int grpnum; /* ���� */
    int size;   /* �����С */
}prdt_arg_cfg_t;

typedef struct
{
    char name[PARAM_NAME_LEN];              /* ������ */
    unsigned int gid;                       /* ��ID */
    char value[PRDT_ARG_VALUE_MAX_SIZE];    /* ����ֵ */
    int env_type;                           /* ��������(0: ���� 1: ���� 2: ����) */
    int load_type;                          /* ��������(0: �Զ� 1: ���Զ�) */
    char remark[PARAM_REMARK_LEN];          /* ��ע��Ϣ */
}prdt_arg_t;

typedef struct
{
    group_cntx_t gcntx;
    int gidx;
}prdt_arg_cntx_t;

extern prdt_arg_cntx_t g_prdt_arg_cntx;

#define prdt_arg_gcntx() (&g_prdt_arg_cntx.gcntx)   /* ��ȡ��Ʒ������������ */
#define prdt_arg_gidx()  (g_prdt_arg_cntx.gidx)     /* ��ȡ��Ʒ������IDX */
#define prdt_arg_set_gidx(idx)  (g_prdt_arg_cntx.gidx = idx) /* ���ò�Ʒ������IDX */

/* ��Ʒ�������ʼ�� */
extern int prdt_arg_init(void *addr);
extern int prdt_arg_link(void *addr);

/* ��Ʒ���������� */
extern int prdt_arg_galloc(const char *name, unsigned int gid, unsigned int pgid, const char *remark);

/* ��Ʒ�������ͷ� */
#define prdt_arg_gfree() group_free(prdt_arg_gcntx(), prdt_arg_gidx())

/* ����ָ����Ʒ���� */
#define prdt_arg_set(name, arg, len) \
            param_set(prdt_arg_gcntx(), prdt_arg_gidx(), name, arg, len)

/* ��ȡָ����Ʒ���� */
#define prdt_arg_get(name) param_get(prdt_arg_gcntx(), prdt_arg_gidx(), name)

/* ɾ��ָ����Ʒ���� */
#define prdt_arg_delete(name) param_delete(prdt_arg_gcntx(), prdt_arg_gidx(), name)

/* ɾ�����в�Ʒ���� */
#define prdt_arg_delete_all() param_delete_all(prdt_arg_gcntx(), prdt_arg_gidx())

/* ��ȡָ����Ʒ�����ڵ� */
#define prdt_arg_getbyindex(idx, name) param_get_byindex(prdt_arg_gcntx(), prdt_arg_gidx(), idx, name)

#endif /*__PRDT_ARG__*/
#endif /*__PRDT_ARG_H__*/
