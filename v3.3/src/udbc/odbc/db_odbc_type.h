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

#define ODBC_MAX_CONN				(10)     /* ��������� */
#define ODBC_BLOB_SIZE              (32276)
#define ODBC_FETCH_BUFF_MAX_SIZE    (4096)
#define ODBC_CONN_TIMEOUT           (5)
#define ODBC_ERR_STAT_MAX_LEN       (16)
#define ODBC_COL_NAME_MAX_LEN       (256)
#define ODBC_MEM_POOL_SIZE          (10*1024*1024)

/* ��������Ϣ */
typedef struct
{
	SQLPOINTER buffer;      /* ��ѯ��� */
	SQLINTEGER buflen;      /* ��ѯ������� */
	SQLLEN null_ind;        /* ���Ƿ�Ϊ�յ�ָʾ��־ */
}odbc_col_data_t;

/* ��������Ϣ */
typedef struct
{
	odbc_col_data_t *col_data;
}odbc_row_data_t;

typedef struct
{
	SQLCHAR name[ODBC_COL_NAME_MAX_LEN];   /* ���� */
	SQLSMALLINT name_len;   /* ������ */
	SQLLEN buflen;          /* ��ֵ���泤�� */
	SQLULEN precision;      /* ���� */
	SQLSMALLINT scale;      /* С����λ�� */
	SQLSMALLINT nullable;   /* �Ƿ�����Ϊ�� */
	SQLSMALLINT type;       /* �е��������� */
}odbc_col_desc_t;

/* ��ѯ�����Ϣ */
typedef struct
{
	int req_rows;           /* FETCH�������� */
	int rows;               /* �������ʵ������ */
	int cols;               /* ���� */
	int is_nodata;          /* ������: ������������ѱ�ȡ�� */

	odbc_row_data_t row_data[DB_FETCH_MAX_ROWS]; /* �����ݣ�ʵ�����ݣ� */
	odbc_col_desc_t *col_desc;  /* ��������Ϣ */
}odbc_query_t;


typedef struct
{
	SQLHSTMT hqstmt;                           /* ��ѯSQL����� */
	char alias[128];
	odbc_query_t query;                         /* ��ѯ�����Ϣ */
}odbc_conn_t;

/* ���ݿ�������������Ϣ */
typedef struct
{
	char svrname[DB_SVR_NAME_MAX_LEN];          /* ������ */

	SQLHENV henv;                               /* ������� */
	SQLHDBC hdbc;                               /* ���Ӿ�� */
	unsigned int is_connected;                  /* �Ƿ��������ݿ� */ 
	SQLHSTMT hnqstmt;                           /* �ǲ�ѯSQL����� */

	odbc_conn_t *conn[ODBC_MAX_CONN];
	alias_pool_t ap;                         /* ������ */
}odbc_cntx_t;

#endif /*__DB_ODBC_TYPE_H__*/
