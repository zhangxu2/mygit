#if !defined(__DB_DATA_TYPE_H__)
#define __DB_DATA_TYPE_H__

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define FILE_NAME_MAX_LEN     (256)   /* 文件名的最大长度 */
#define FUNC_NAME_MAX_LEN     (256)   /* 函数名的最大长度 */
#define DB_ERR_MSG_MAX_LEN    (1024)  /* 错误信息最大长度 */
#define DB_SQL_MAX_LEN        (1024)  /* SQL语句最大长度 */
#define DB_FETCH_MAX_ROWS     (100)   /* 一次最多取的行数 */

#define DB_USR_NAME_MAX_LEN   (64)    /* 用户名的最大长度 */
#define DB_PWD_MAX_LEN        (64)    /* 密码的最大长度 */
#define DB_CONN_NAME_MAX_LEN  (64)    /* 数据库连接名 最大长度 */
#define DB_SVR_NAME_MAX_LEN   (64)    /* 服务名的最大长度 */

#define DB_CONNECT_MAX_NUM    (10)    /* 数据库连接的最大数 */

#define DB_CFG_FILE_NAME      "$(SWWORK)/etc/dbcfg/db_cfg.xml"  /* 数据库配置文件名 */

/* 各数据库对应的动态库 */
#define DB_LIB_ORACLE       "libswora.so"
#define DB_LIB_ORACLE_PC    "libswora_pc.so"
#define DB_LIB_INFORMIX     "libswifx.so"
#define DB_LIB_ODBC         "libswodbc.so"
#define DB_LIB_DB2          DB_LIB_ODBC

/* 数据库类型名称 */
#define DB_NAME_ORACLE        "ORACLE"
#define DB_NAME_INFORMIX      "INFORMIX"
#define DB_NAME_DB2           "DB2"

#define ORA_DB_CONN_DETECT	"select 1 from dual"
#define IFX_DB_CONN_DETECT	"select first 1 current from systables"
#define ODBC_DB_CONN_DETECT	"select 1 from SYSIBM.SYSDUMMY1"

typedef int (*db_open_func_t)(void *source, void **cntx);

typedef int (*db_mquery_func_t)(const char *alias, const char *sql, int rows, void *cntx);
typedef int (*db_squery_func_t)(const char *sql,void *cntx);
typedef int (*db_nquery_func_t)(const char *sql, void *cntx);
typedef int (*db_update_by_rowid_func_t)(const char *_sql, const char *rowid, void *cntx);

typedef char* (*db_get_data_func_t)(const char *alias, int row, int col, void *cntx);
typedef char* (*db_get_data_by_name_func_t)(const char *alias, int row, const char *name, void *cntx);
typedef char* (*db_get_data_and_name_func_t)(const char *alias, int row, int col, char *name, int size, void *cntx);
typedef int (*db_get_type_func_t)(const char *alias, int col, void *cntx);

typedef int (*db_comm_func_t)(void *cntx);
typedef int (*db_mfetch_func_t)(const char *alias, void *cntx);
typedef int (*db_clo_fet_func_t)(const char *alias, void *cntx);
typedef int (*db_del_all_conn_func_t)(void *cntx);

typedef int (*db_set_alias_func_t)(void *cntx, const char *name, const void *value, int length);
typedef void* (*db_get_alias_func_t)(void *cntx, const char *name);
typedef int (*db_delete_alias_func_t)(void *cntx, const char *name);
typedef int (*db_delete_all_alias_func_t)(void *cntx);
typedef int (*db_conn_detect_func_t)(void *cntx);


/* 数据库类型 */
typedef enum
{
	DB_TYPE_ORACLE,
	DB_TYPE_INFORMIX,
	DB_TYPE_DB2,

	DB_TYPE_UNKOWN,
	DB_TYPE_TOTAL = DB_TYPE_UNKOWN
}db_type_t;

/* SQL语句类型 */
typedef enum
{
	DB_SQL_INSERT,
	DB_SQL_SELECT,
	DB_SQL_UPDATE,
	DB_SQL_DELETE,
	DB_SQL_FETCH,
	DB_SQL_CLOSE,
	DB_SQL_DROP,
	DB_SQL_UPDATE_DEC, /* Declare */
	DB_SQL_UPDATE_FET, /* Fetch */
	DB_SQL_UPDATE_UPD, /* Update */
	DB_SQL_UPDATE_CLO, /* Close */

	DB_SQL_UNKOWN,
	DB_SQL_TOTAL = DB_SQL_UNKOWN
}db_sql_cmd_t;

/* 数据库数据类型 */
typedef enum
{
	DB_DTYPE_CHAR,          /* Ifx:CHAR[n]*/
	DB_DTYPE_VCHAR,         /* Ifx:VARCHAR[n, 1] Ora:VARCHAR[n, 1] */
	DB_DTYPE_VCHAR2,        /* Ora:VARCHAR2[n, 1] */
	DB_DTYPE_NCHAR,         /* Ifx:NCHAR[n]*/
	DB_DTYPE_NVCHAR,        /* Ifx:NVARCHAR[n, 1] */
	DB_DTYPE_LVCHAR,        /* Ifx:LVARCHAR[n, 1] */
	DB_DTYPE_SMINT,         /* Ifx:SMALLINT Ora:SMINT */
	DB_DTYPE_INT,           /* Ifx:INT */
	DB_DTYPE_LONG,          /* Ora:LONG */
	DB_DTYPE_LNGLNG,        /* Ifx:INT8/SERIAL*/
	DB_DTYPE_SERIAL,        /* Ora:SERIAL */
	DB_DTYPE_BIGINT,        /* Ifx:BIGINT/SERIAL8*/
	DB_DTYPE_SMFLOAT,       /* Ifx:SMALLFLOAT Ora:SMALLFLOAT */
	DB_DTYPE_FLOAT,         /* Ifx:FLOAT      Ora:FLOAT */
	DB_DTYPE_DECIMAL,       /* Ifx:DECIMAL    Ora:DECIMAL */
	DB_DTYPE_NUMBER,        /* Ifx:NUMBER     Ora:NUMBER */
	DB_DTYPE_REAL,          /* Odbc:REAL */
	DB_DTYPE_DATE,          /* Ifx:DATE   Ora:DATE/DATETIME*/
	DB_DTYPE_TIME,          /* Odbc:TIME */
	DB_DTYPE_DTIME,         /* Ifx:DATETIME */
	DB_DTYPE_TIMESTAMP,     /* Odbc:SQL_TYPE_TIMESTAMP */
	DB_DTYPE_INTERVAL,      /* Ifx:INTERVAL  Ora:INTERVAL*/
	DB_DTYPE_MONEY,         /* Ifx:MONEY */
	DB_DTYPE_LOCATOR,       /* Ifx:BYTE/TEXT */
	DB_DTYPE_BOOL,          /* Ifx:BOOLEAN */
	DB_DTYPE_RAW,           /* Ora:RAW */
	DB_DTYPE_LONGRAW,       /* Ora:LONGRAW */
	DB_DTYPE_COLL,          /* Ifx:SET/LIST/MULTISET/COLLECTION */
	DB_DTYPE_ROW,           /* Ifx:ROW */
	DB_DTYPE_CLOB,          /* Odbc:SQL_LONGVARCHAR */
	DB_DTYPE_BLOB,          /* Odbc:SQL_LONGVARBINARY */

	DB_DTYPE_UNKNOW,
	DB_DTYPE_TOTAL = DB_DTYPE_UNKNOW
}db_data_type_t;



/* 数据库操作: 回调函数 */
typedef struct
{
#if defined(__DB_CONNECT_POOL__)
	db_open_func_t db_pool;                /* 创建连接池 */
	db_comm_func_t db_close_pool;          /* 关闭数据库连接池 */
	db_open_func_t db_get_conn;            /* 获取连接 */
	db_comm_func_t db_put_conn;            /* 放回连接 */
#endif /*__DB_CONNECT_POOL__*/

	db_open_func_t db_open;                /* 连接数据库 */
	db_comm_func_t db_close;			   /* 关闭数据库连接 */

	db_mquery_func_t db_mquery;            /* 执行查询语句(多条查询结果) */
	db_squery_func_t db_squery;            /* 执行查询语句(最多一条结果) */
	db_nquery_func_t db_nquery;            /* 执行非查询SQL语句 */
	db_update_by_rowid_func_t db_update_by_rowid;   /* 通过行号更新数据 */

	db_mfetch_func_t db_mfetch;               /* 取游标值 */
	db_comm_func_t db_fetch;               /* 取游标值 */
	db_clo_fet_func_t db_cclose;              /* 关闭游标 */
	db_del_all_conn_func_t db_del_all_conn;  /* 删除所有链接 */

	db_comm_func_t db_commit;              /* 提交事务 */
	db_comm_func_t db_rollback;            /* 回滚事务 */

	db_get_data_func_t db_get_data;        /* 获取列值 */
	db_get_data_by_name_func_t db_get_data_by_name;     /* 通过列名获取列值 */
	db_get_data_and_name_func_t db_get_data_and_name;   /* 通过行列获取列值和列名 */
	db_get_type_func_t db_get_type;        /* 获取列类型 */

	db_set_alias_func_t db_set_alias;
	db_get_alias_func_t db_get_alias;
	db_delete_alias_func_t db_delete_alias;
	db_delete_all_alias_func_t db_delete_all_alias;
	db_conn_detect_func_t db_conn_detect;
}db_cb_ptr_t;

/* 数据库操作: 函数名 */
typedef struct
{
#if defined(__DB_CONNECT_POOL__)
	char pool[FUNC_NAME_MAX_LEN];           /* 创建连接池 函数名 */
	char close_pool[FUNC_NAME_MAX_LEN];     /* 关闭数据库连接池 函数名 */
#endif /*__DB_CONNECT_POOL__*/

	char open[FUNC_NAME_MAX_LEN];           /* 连接数据库 函数名 */
	char close[FUNC_NAME_MAX_LEN];          /* 关闭数据库连接 函数名 */

	char mquery[FUNC_NAME_MAX_LEN];         /* 执行查询语句 函数名 */
	char squery[FUNC_NAME_MAX_LEN];         /* 执行只返回零条或一条结果的查询语句 函数名 */
	char nquery[FUNC_NAME_MAX_LEN];         /* 执行非查询SQL语句 函数名 */
	char update_by_rowid[FUNC_NAME_MAX_LEN];/* 通过ROWID更新数据 */

	char fetch[FUNC_NAME_MAX_LEN];          /* 取游标值 函数名 */
	char mfetch[FUNC_NAME_MAX_LEN];          /* 取游标值 函数名 */
	char cclose[FUNC_NAME_MAX_LEN];         /* 关闭游标 函数名 */
	char del_all_conn[FUNC_NAME_MAX_LEN];   /* 删除链接 函数名 */

	char commit[FUNC_NAME_MAX_LEN];         /* 提交事务 函数名 */
	char rollback[FUNC_NAME_MAX_LEN];       /* 回滚事务 函数名 */

	char get_data[FUNC_NAME_MAX_LEN];       /* 获取列值 函数名 */
	char get_data_by_name[FUNC_NAME_MAX_LEN];   /* 获取列值 函数名 */
	char get_data_and_name[FUNC_NAME_MAX_LEN];  /* 获取列值和列名 函数名 */
	char get_type[FUNC_NAME_MAX_LEN];       /* 获取列类型 函数名 */

	char set_alias[FUNC_NAME_MAX_LEN];      /* 设置别名值 */
	char get_alias[FUNC_NAME_MAX_LEN];      /* 获取别名值 */
	char delete_alias[FUNC_NAME_MAX_LEN];   /* 删除指定别名 */
	char delete_all_alias[FUNC_NAME_MAX_LEN];   /* 删除所有别名 */
	char conn_detect[FUNC_NAME_MAX_LEN];        /* DB detect */
}db_cb_name_t;

/* 数据源配置信息 */
typedef struct
{
	db_type_t dbtype;                       /* 数据库类型 */
	int ispool;                             /* 是否支持连接池 */
	char svrname[DB_SVR_NAME_MAX_LEN];      /* 服务名 */
	char usrname[DB_USR_NAME_MAX_LEN];      /* 用户名 */
	char passwd[DB_PWD_MAX_LEN];            /* 密码 */
}db_data_src_t;

/* 数据库操作函数信息 */
typedef struct
{
	db_cb_ptr_t func;
	db_cb_name_t *name;	
}db_cb_func_t;

/* 数据库访问信息 */
typedef struct
{
	void *handle;          /* 动态库句柄 */
	void *cntx;            /* 数据库连接的上下文信息 */
	db_cb_func_t cbfunc;
}db_access_info_t;

/* 数据库信息 */
typedef struct
{
	db_access_info_t access;
	db_data_src_t source;
}db_cycle_t;


#endif /*__DB_DATA_TYPE_H__*/
