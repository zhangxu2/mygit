#if !defined(__PRDT_ARG_H__)
#define __PRDT_ARG_H__

#if defined(__PRDT_ARG__)
#include "param.h"
#include "pub_log.h"
#include "pub_type.h"


#define PRDT_GRP_NUM            (100)       /* 产品参数组最小个数 */
#define PRDT_GRP_SIZE           (100*1024)  /* 产品参数组最小空间 */

#define PRDT_ARG_FLOCK_DIR      "tmp/prdtarg/"      /* 产品参数锁路径 */
#define PRDT_GRP_FLOCK          ".prdt_grp_flock"   /* 参数组文件锁对应的文件名 */
#define PRDT_ARG_FLOCK          ".prdt_arg_flock"   /* 参数文件锁对应的文件名 */


#define PRDT_ARG_VALUE_MAX_SIZE (128)

/* 产品参数配置 */
typedef struct
{
    int grpnum; /* 组数 */
    int size;   /* 各组大小 */
}prdt_arg_cfg_t;

typedef struct
{
    char name[PARAM_NAME_LEN];              /* 参数名 */
    unsigned int gid;                       /* 组ID */
    char value[PRDT_ARG_VALUE_MAX_SIZE];    /* 参数值 */
    int env_type;                           /* 环境类型(0: 开发 1: 测试 2: 生产) */
    int load_type;                          /* 加载类型(0: 自动 1: 非自动) */
    char remark[PARAM_REMARK_LEN];          /* 备注信息 */
}prdt_arg_t;

typedef struct
{
    group_cntx_t gcntx;
    int gidx;
}prdt_arg_cntx_t;

extern prdt_arg_cntx_t g_prdt_arg_cntx;

#define prdt_arg_gcntx() (&g_prdt_arg_cntx.gcntx)   /* 获取产品参数组上下文 */
#define prdt_arg_gidx()  (g_prdt_arg_cntx.gidx)     /* 获取产品参数组IDX */
#define prdt_arg_set_gidx(idx)  (g_prdt_arg_cntx.gidx = idx) /* 设置产品参数组IDX */

/* 产品参数组初始化 */
extern int prdt_arg_init(void *addr);
extern int prdt_arg_link(void *addr);

/* 产品参数组申请 */
extern int prdt_arg_galloc(const char *name, unsigned int gid, unsigned int pgid, const char *remark);

/* 产品参数组释放 */
#define prdt_arg_gfree() group_free(prdt_arg_gcntx(), prdt_arg_gidx())

/* 设置指定产品参数 */
#define prdt_arg_set(name, arg, len) \
            param_set(prdt_arg_gcntx(), prdt_arg_gidx(), name, arg, len)

/* 获取指定产品参数 */
#define prdt_arg_get(name) param_get(prdt_arg_gcntx(), prdt_arg_gidx(), name)

/* 删除指定产品参数 */
#define prdt_arg_delete(name) param_delete(prdt_arg_gcntx(), prdt_arg_gidx(), name)

/* 删除所有产品参数 */
#define prdt_arg_delete_all() param_delete_all(prdt_arg_gcntx(), prdt_arg_gidx())

/* 获取指定产品参数节点 */
#define prdt_arg_getbyindex(idx, name) param_get_byindex(prdt_arg_gcntx(), prdt_arg_gidx(), idx, name)

#endif /*__PRDT_ARG__*/
#endif /*__PRDT_ARG_H__*/
