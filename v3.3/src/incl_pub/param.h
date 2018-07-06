#if !defined(__PARAM_H__)
#define __PARAM_H__

#include "pub_log.h"
#include "pub_type.h"
#include "shm_slab.h"


#define GRP_NAME_LEN            (64)        /* �������� */
#define GRP_REMARK_LEN          (128)       /* �鱸ע���� */
#define PARAM_REMARK_LEN        (128)       /* ������ע���� */
#define PARAM_NAME_LEN          (64)        /* ���������� */
#define PARAM_HASH_NUM          (111)       /* ������ϣ����Ŀ */
#define PARAM_GRP_MIN_SIZE      (40*1024)   /* ���������С�ռ� */

/* �������Ϣ�ṹ�� */
typedef struct
{
    int grpnum;                             /* ������ */
    size_t size;                            /* ��������ռ�ռ� */
    char gflock[FILE_NAME_MAX_LEN];         /* ���ļ���·�� */
    char pflock[FILE_NAME_MAX_LEN];         /* ���������������·�� */
}group_sum_t;

/* ����Ϣ�ṹ�� */
typedef struct
{
    int idx;                                /* ������IDX */
    unsigned int gid;                       /* ��ID */
    char name[GRP_NAME_LEN];                /* ���� */
    unsigned int pgid;                      /* ����ID */
    char remark[GRP_REMARK_LEN];            /* ��ע��Ϣ */
}group_t;

/* �����ڵ���Ϣ�ṹ�� */
typedef struct
{
    char name[PARAM_NAME_LEN];              /* ������ */
    unsigned int gid;                       /* ��ID */
    
    size_t value_offset;                    /* ֵƫ���� */
    int length;                             /* ֵ���� */
    
    size_t next_offset;                     /* ��һ�ڵ� */
#if defined(SOLARIS) || defined(HPUX)
}__attribute__ ((packed))param_node_t;
#else
}param_node_t;
#endif

/* ����ͷ����Ϣ�ṹ�� */
typedef struct
{
	int counts;								/* �������� */
    group_t group;                          /* �������Ϣ */
    param_node_t params[PARAM_HASH_NUM];    /* ������ϣ�� */
    shm_slab_pool_t pool;
}group_head_t;

/* ���������� */
typedef struct
{
    int gfd;    /* ���ļ������� */
    int pfd;    /* �����ļ������� */
    void *addr; /* GROUP�׵�ַ */
}group_cntx_t;

extern int group_init(group_cntx_t *cntx, void *addr,
            int grpnum, size_t size, const char *gflock, const char *pflock);
extern int group_alloc(group_cntx_t *cntx, const char *name,
        unsigned int gid, unsigned int pgid, const char *remark);
extern int group_free(group_cntx_t *cntx, int gidx);
extern int group_get_total_size(int grpnum, size_t size);
extern int param_set(group_cntx_t *cntx, int gidx, 
                const char *name, const void *value, int length);
extern const void *param_get(group_cntx_t *cntx, int gidx, const char *name);
extern int param_delete(group_cntx_t *cntx, int gidx, const char *name);
extern int param_delete_all(group_cntx_t *cntx, int gidx);
extern void *param_get_byindex(group_cntx_t *cntx, int gidx, int idx, char *name);

#endif /*__PARAM_H__*/
