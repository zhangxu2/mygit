#ifndef __DB_ORA_DATA_TYPE_H_
#define __DB_ORA_DATA_TYPE_H_

#include <oci.h>
#include "db_data_type.h"
#include "alias_pool.h"

#if defined(__DB_CONNECT_POOL__)
#define ORA_MAX_CONN    (10)    /* ��������� */
#else /*__DB_CONNECT_POOL__*/
#define ORA_MAX_CONN    (10)     /* ��������� */
#endif /*__DB_CONNECT_POOL__*/
#define ORA_MIN_CONN    (1)     /* ��С������ */
#define ORA_CONN_INCR   (1)     /* �������� */
#define ORA_MEM_POOL_SIZE   (10*1024*1024)  /* �ڴ�ؿռ��С */

typedef struct 
{
    int rows;               /* ���� */
    int cols;               /* ���� */
    int *length;            /* �ֶγ��� */
    size_t rowlen;          /* ���г��� */
    int *type;              /* �ֶ����� */
    char **name;            /* �ֶ��� */
    unsigned char *result;  /* �ֶ�ֵ */
}ora_result_t;

typedef struct 
{
    char alias[128];
    OCIEnv *henv;           /* ������� */
    OCISession *hssn;       /* �Ự��� */
    OCIServer *hsvr;        /* ������ */
    OCISvcCtx *hsvc;        /* ���������ľ�� */ 
    OCIError *herror;       /* ������ */
    OCIStmt *hstmt;         /* ����� */
    pid_t pid;              /* ���̺� */
    OCIDefine **hdfn;       /* �󶨾�� */
    int *ind;               /* ָʾ���� */
    int is_nodata;          /* ������: ����������ݱ�ȡ�� */
}ora_conn_t;

/* Context */
typedef struct 
{
#if defined(__DB_CONNECT_POOL__)
    OCIEnv *henv;           /* ������� */
    OCIError *herror;       /* ������ */
    OCICPool *hcpool;       /* ���ӳؾ�� */
    OraText *cpool_name;    /* ���ӳ��� */
    sb4 cpool_name_len;     /* ���ӳ������� */
#endif /*__DB_CONNECT_POOL__*/
    char conn_name[DB_CONN_NAME_MAX_LEN];
    char dbname[DB_SVR_NAME_MAX_LEN];
    ora_conn_t *conn[ORA_MAX_CONN];
    ora_result_t result[ORA_MAX_CONN];
    alias_pool_t ap;        /* ������ */
}ora_cntx_t;
#endif
