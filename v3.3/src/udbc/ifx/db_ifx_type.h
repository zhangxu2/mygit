#if !defined(__DB_IFX_TYPE_H__)
#define __DB_IFX_TYPE_H__

#include "db_data_type.h"
#include "alias_pool.h"

#define IFX_BLOB_SIZE               (32276)
#define IFX_FETCH_BUFF_MAX_SIZE     (4096)
#define IFX_MEM_POOL_SIZE           (10*1024*1024)

#define IFX_MAX_CONN	10

typedef struct
{
	char	alias[128];
	int	used;
	/* SELECT结果集 */
	ifx_sqlda_t result[DB_FETCH_MAX_ROWS];     /* FETCH结果集 */
	int req_rows;                              /* 每次申请取的行数 */
	int rows;                                  /* 当前实际的行数 */
	int isalloced;                             /* 结果集空间是否被分配 0:未分配 1：已分配 */
	int is_nodata;                             /* 无数据: 结果集中已被取完 */

	/* 结果转换 */
	char *convert;                             /* 结果转换空间 */
	size_t convert_size;                       /* 结果转换空间的大小 */

	int msglen;                                /* 存放查询数据的所有列的长度和 */
	size_t row_size;                           /* 存放了在C程序中的所有列的长度和 */
	alias_pool_t	*ap;
} ifx_dbconn_t;

/* 数据库连接上下文信息 */
typedef struct
{
	unsigned long	pid;                         /* 进程ID */
	unsigned long	tid;                         /* 线程ID */
	char	svrname[DB_SVR_NAME_MAX_LEN];         /* 服务名 */
	char	cnname[DB_CONN_NAME_MAX_LEN];         /* 连接名 */
	ifx_dbconn_t	conn[IFX_MAX_CONN];
	alias_pool_t	ap;                           /* 内存池 */
}ifx_cntx_t;

#endif /*__DB_IFX_TYPE_H__*/
