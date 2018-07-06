#if !defined(__DB_ODBC_TYPE_H__)
#define __DB_ODBC_TYPE_H__

#include "db_data_type.h"
#include "alias_pool.h"

#if 0
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#endif

#include <sqlcli.h>
#include <unistd.h>
#include <sys/types.h>

#define ODBC_MAX_CONN				(10)     /* 最大连接数 */
#define ODBC_BLOB_SIZE              (32276)
#define ODBC_FETCH_BUFF_MAX_SIZE    (4096)
#define ODBC_CONN_TIMEOUT           (5)
#define ODBC_ERR_STAT_MAX_LEN       (16)
#define ODBC_COL_NAME_MAX_LEN       (256)
#define ODBC_MEM_POOL_SIZE          (10*1024*1024)

/* 列数据信息 */
typedef struct
{
	SQLPOINTER buffer;      /* 查询结果 */
	SQLINTEGER buflen;      /* 查询结果长度 */
	SQLLEN null_ind;        /* 列是否为空的指示标志 */
}odbc_col_data_t;

/* 行数据信息 */
typedef struct
{
	odbc_col_data_t *col_data;
}odbc_row_data_t;

typedef struct
{
	SQLCHAR name[ODBC_COL_NAME_MAX_LEN];   /* 列名 */
	SQLSMALLINT name_len;   /* 列名长 */
	SQLLEN buflen;          /* 列值缓存长度 */
	SQLULEN precision;      /* 精度 */
	SQLSMALLINT scale;      /* 小数点位数 */
	SQLSMALLINT nullable;   /* 是否允许为空 */
	SQLSMALLINT type;       /* 列的数据类型 */
}odbc_col_desc_t;

/* 查询结果信息 */
typedef struct
{
	int req_rows;           /* FETCH请求行数 */
	int rows;               /* 结果集中实际行数 */
	int cols;               /* 列数 */
	int is_nodata;          /* 无数据: 结果集中数据已被取完 */

	odbc_row_data_t row_data[DB_FETCH_MAX_ROWS]; /* 行数据（实际数据） */
	odbc_col_desc_t *col_desc;  /* 列描述信息 */
}odbc_query_t;


typedef struct
{
	SQLHSTMT hqstmt;                           /* 查询SQL语句句柄 */
	char alias[128];
	odbc_query_t query;                         /* 查询结果信息 */
}odbc_conn_t;

/* 数据库连接上下文信息 */
typedef struct
{
	char svrname[DB_SVR_NAME_MAX_LEN];          /* 服务名 */

	SQLHENV henv;                               /* 环境句柄 */
	SQLHDBC hdbc;                               /* 连接句柄 */
	unsigned int is_connected;                  /* 是否连接数据库 */ 
	SQLHSTMT hnqstmt;                           /* 非查询SQL语句句柄 */

	odbc_conn_t *conn[ODBC_MAX_CONN];
	alias_pool_t ap;                         /* 别名池 */
}odbc_cntx_t;

#endif /*__DB_ODBC_TYPE_H__*/
