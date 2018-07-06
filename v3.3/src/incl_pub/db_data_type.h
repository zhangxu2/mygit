#if !defined(__DB_DATA_TYPE_H__)
#define __DB_DATA_TYPE_H__

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define FILE_NAME_MAX_LEN     (256)   /* �ļ�������󳤶� */
#define FUNC_NAME_MAX_LEN     (256)   /* ����������󳤶� */
#define DB_ERR_MSG_MAX_LEN    (1024)  /* ������Ϣ��󳤶� */
#define DB_SQL_MAX_LEN        (1024)  /* SQL�����󳤶� */
#define DB_FETCH_MAX_ROWS     (100)   /* һ�����ȡ������ */

#define DB_USR_NAME_MAX_LEN   (64)    /* �û�������󳤶� */
#define DB_PWD_MAX_LEN        (64)    /* �������󳤶� */
#define DB_CONN_NAME_MAX_LEN  (64)    /* ���ݿ������� ��󳤶� */
#define DB_SVR_NAME_MAX_LEN   (64)    /* ����������󳤶� */

#define DB_CONNECT_MAX_NUM    (10)    /* ���ݿ����ӵ������ */

#define DB_CFG_FILE_NAME      "$(SWWORK)/etc/dbcfg/db_cfg.xml"  /* ���ݿ������ļ��� */

/* �����ݿ��Ӧ�Ķ�̬�� */
#define DB_LIB_ORACLE       "libswora.so"
#define DB_LIB_ORACLE_PC    "libswora_pc.so"
#define DB_LIB_INFORMIX     "libswifx.so"
#define DB_LIB_ODBC         "libswodbc.so"
#define DB_LIB_DB2          DB_LIB_ODBC

/* ���ݿ��������� */
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


/* ���ݿ����� */
typedef enum
{
	DB_TYPE_ORACLE,
	DB_TYPE_INFORMIX,
	DB_TYPE_DB2,

	DB_TYPE_UNKOWN,
	DB_TYPE_TOTAL = DB_TYPE_UNKOWN
}db_type_t;

/* SQL������� */
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

/* ���ݿ��������� */
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



/* ���ݿ����: �ص����� */
typedef struct
{
#if defined(__DB_CONNECT_POOL__)
	db_open_func_t db_pool;                /* �������ӳ� */
	db_comm_func_t db_close_pool;          /* �ر����ݿ����ӳ� */
	db_open_func_t db_get_conn;            /* ��ȡ���� */
	db_comm_func_t db_put_conn;            /* �Ż����� */
#endif /*__DB_CONNECT_POOL__*/

	db_open_func_t db_open;                /* �������ݿ� */
	db_comm_func_t db_close;			   /* �ر����ݿ����� */

	db_mquery_func_t db_mquery;            /* ִ�в�ѯ���(������ѯ���) */
	db_squery_func_t db_squery;            /* ִ�в�ѯ���(���һ�����) */
	db_nquery_func_t db_nquery;            /* ִ�зǲ�ѯSQL��� */
	db_update_by_rowid_func_t db_update_by_rowid;   /* ͨ���кŸ������� */

	db_mfetch_func_t db_mfetch;               /* ȡ�α�ֵ */
	db_comm_func_t db_fetch;               /* ȡ�α�ֵ */
	db_clo_fet_func_t db_cclose;              /* �ر��α� */
	db_del_all_conn_func_t db_del_all_conn;  /* ɾ���������� */

	db_comm_func_t db_commit;              /* �ύ���� */
	db_comm_func_t db_rollback;            /* �ع����� */

	db_get_data_func_t db_get_data;        /* ��ȡ��ֵ */
	db_get_data_by_name_func_t db_get_data_by_name;     /* ͨ��������ȡ��ֵ */
	db_get_data_and_name_func_t db_get_data_and_name;   /* ͨ�����л�ȡ��ֵ������ */
	db_get_type_func_t db_get_type;        /* ��ȡ������ */

	db_set_alias_func_t db_set_alias;
	db_get_alias_func_t db_get_alias;
	db_delete_alias_func_t db_delete_alias;
	db_delete_all_alias_func_t db_delete_all_alias;
	db_conn_detect_func_t db_conn_detect;
}db_cb_ptr_t;

/* ���ݿ����: ������ */
typedef struct
{
#if defined(__DB_CONNECT_POOL__)
	char pool[FUNC_NAME_MAX_LEN];           /* �������ӳ� ������ */
	char close_pool[FUNC_NAME_MAX_LEN];     /* �ر����ݿ����ӳ� ������ */
#endif /*__DB_CONNECT_POOL__*/

	char open[FUNC_NAME_MAX_LEN];           /* �������ݿ� ������ */
	char close[FUNC_NAME_MAX_LEN];          /* �ر����ݿ����� ������ */

	char mquery[FUNC_NAME_MAX_LEN];         /* ִ�в�ѯ��� ������ */
	char squery[FUNC_NAME_MAX_LEN];         /* ִ��ֻ����������һ������Ĳ�ѯ��� ������ */
	char nquery[FUNC_NAME_MAX_LEN];         /* ִ�зǲ�ѯSQL��� ������ */
	char update_by_rowid[FUNC_NAME_MAX_LEN];/* ͨ��ROWID�������� */

	char fetch[FUNC_NAME_MAX_LEN];          /* ȡ�α�ֵ ������ */
	char mfetch[FUNC_NAME_MAX_LEN];          /* ȡ�α�ֵ ������ */
	char cclose[FUNC_NAME_MAX_LEN];         /* �ر��α� ������ */
	char del_all_conn[FUNC_NAME_MAX_LEN];   /* ɾ������ ������ */

	char commit[FUNC_NAME_MAX_LEN];         /* �ύ���� ������ */
	char rollback[FUNC_NAME_MAX_LEN];       /* �ع����� ������ */

	char get_data[FUNC_NAME_MAX_LEN];       /* ��ȡ��ֵ ������ */
	char get_data_by_name[FUNC_NAME_MAX_LEN];   /* ��ȡ��ֵ ������ */
	char get_data_and_name[FUNC_NAME_MAX_LEN];  /* ��ȡ��ֵ������ ������ */
	char get_type[FUNC_NAME_MAX_LEN];       /* ��ȡ������ ������ */

	char set_alias[FUNC_NAME_MAX_LEN];      /* ���ñ���ֵ */
	char get_alias[FUNC_NAME_MAX_LEN];      /* ��ȡ����ֵ */
	char delete_alias[FUNC_NAME_MAX_LEN];   /* ɾ��ָ������ */
	char delete_all_alias[FUNC_NAME_MAX_LEN];   /* ɾ�����б��� */
	char conn_detect[FUNC_NAME_MAX_LEN];        /* DB detect */
}db_cb_name_t;

/* ����Դ������Ϣ */
typedef struct
{
	db_type_t dbtype;                       /* ���ݿ����� */
	int ispool;                             /* �Ƿ�֧�����ӳ� */
	char svrname[DB_SVR_NAME_MAX_LEN];      /* ������ */
	char usrname[DB_USR_NAME_MAX_LEN];      /* �û��� */
	char passwd[DB_PWD_MAX_LEN];            /* ���� */
}db_data_src_t;

/* ���ݿ����������Ϣ */
typedef struct
{
	db_cb_ptr_t func;
	db_cb_name_t *name;	
}db_cb_func_t;

/* ���ݿ������Ϣ */
typedef struct
{
	void *handle;          /* ��̬���� */
	void *cntx;            /* ���ݿ����ӵ���������Ϣ */
	db_cb_func_t cbfunc;
}db_access_info_t;

/* ���ݿ���Ϣ */
typedef struct
{
	db_access_info_t access;
	db_data_src_t source;
}db_cycle_t;


#endif /*__DB_DATA_TYPE_H__*/
