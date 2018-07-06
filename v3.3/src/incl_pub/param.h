#if !defined(__PARAM_H__)
#define __PARAM_H__

#include "pub_log.h"
#include "pub_type.h"
#include "shm_slab.h"


#define GRP_NAME_LEN            (64)        /* 组名长度 */
#define GRP_REMARK_LEN          (128)       /* 组备注长度 */
#define PARAM_REMARK_LEN        (128)       /* 参数备注长度 */
#define PARAM_NAME_LEN          (64)        /* 参数名长度 */
#define PARAM_HASH_NUM          (111)       /* 参数哈希表数目 */
#define PARAM_GRP_MIN_SIZE      (40*1024)   /* 单个组的最小空间 */

/* 组汇总信息结构体 */
typedef struct
{
    int grpnum;                             /* 组总数 */
    size_t size;                            /* 单个组所占空间 */
    char gflock[FILE_NAME_MAX_LEN];         /* 组文件锁路径 */
    char pflock[FILE_NAME_MAX_LEN];         /* 各分组参数管理锁路径 */
}group_sum_t;

/* 组信息结构体 */
typedef struct
{
    int idx;                                /* 组索引IDX */
    unsigned int gid;                       /* 组ID */
    char name[GRP_NAME_LEN];                /* 组名 */
    unsigned int pgid;                      /* 父组ID */
    char remark[GRP_REMARK_LEN];            /* 备注信息 */
}group_t;

/* 参数节点信息结构体 */
typedef struct
{
    char name[PARAM_NAME_LEN];              /* 参数名 */
    unsigned int gid;                       /* 组ID */
    
    size_t value_offset;                    /* 值偏移量 */
    int length;                             /* 值长度 */
    
    size_t next_offset;                     /* 下一节点 */
#if defined(SOLARIS) || defined(HPUX)
}__attribute__ ((packed))param_node_t;
#else
}param_node_t;
#endif

/* 各组头部信息结构体 */
typedef struct
{
	int counts;								/* 参数个数 */
    group_t group;                          /* 组基本信息 */
    param_node_t params[PARAM_HASH_NUM];    /* 参数哈希表 */
    shm_slab_pool_t pool;
}group_head_t;

/* 参数上下文 */
typedef struct
{
    int gfd;    /* 组文件描述符 */
    int pfd;    /* 参数文件描述符 */
    void *addr; /* GROUP首地址 */
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
