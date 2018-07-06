#ifndef __DB_ORA_DATA_TYPE_H_
#define __DB_ORA_DATA_TYPE_H_

#include <oci.h>
#include "db_data_type.h"
#include "alias_pool.h"

#if defined(__DB_CONNECT_POOL__)
#define ORA_MAX_CONN    (10)    /* 最大连接数 */
#else /*__DB_CONNECT_POOL__*/
#define ORA_MAX_CONN    (10)     /* 最大连接数 */
#endif /*__DB_CONNECT_POOL__*/
#define ORA_MIN_CONN    (1)     /* 最小连接数 */
#define ORA_CONN_INCR   (1)     /* 增长连接 */
#define ORA_MEM_POOL_SIZE   (10*1024*1024)  /* 内存池空间大小 */

typedef struct 
{
    int rows;               /* 行数 */
    int cols;               /* 列数 */
    int *length;            /* 字段长度 */
    size_t rowlen;          /* 单行长度 */
    int *type;              /* 字段类型 */
    char **name;            /* 字段名 */
    unsigned char *result;  /* 字段值 */
}ora_result_t;

typedef struct 
{
    char alias[128];
    OCIEnv *henv;           /* 环境句柄 */
    OCISession *hssn;       /* 会话句柄 */
    OCIServer *hsvr;        /* 服务句柄 */
    OCISvcCtx *hsvc;        /* 服务上下文句柄 */ 
    OCIError *herror;       /* 错误句柄 */
    OCIStmt *hstmt;         /* 语句句柄 */
    pid_t pid;              /* 进程号 */
    OCIDefine **hdfn;       /* 绑定句柄 */
    int *ind;               /* 指示变量 */
    int is_nodata;          /* 无数据: 结果集中数据被取完 */
}ora_conn_t;

/* Context */
typedef struct 
{
#if defined(__DB_CONNECT_POOL__)
    OCIEnv *henv;           /* 环境句柄 */
    OCIError *herror;       /* 错误句柄 */
    OCICPool *hcpool;       /* 连接池句柄 */
    OraText *cpool_name;    /* 连接池名 */
    sb4 cpool_name_len;     /* 连接池名长度 */
#endif /*__DB_CONNECT_POOL__*/
    char conn_name[DB_CONN_NAME_MAX_LEN];
    char dbname[DB_SVR_NAME_MAX_LEN];
    ora_conn_t *conn[ORA_MAX_CONN];
    ora_result_t result[ORA_MAX_CONN];
    alias_pool_t ap;        /* 别名池 */
}ora_cntx_t;
#endif
