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
	/* SELECT����� */
	ifx_sqlda_t result[DB_FETCH_MAX_ROWS];     /* FETCH����� */
	int req_rows;                              /* ÿ������ȡ������ */
	int rows;                                  /* ��ǰʵ�ʵ����� */
	int isalloced;                             /* ������ռ��Ƿ񱻷��� 0:δ���� 1���ѷ��� */
	int is_nodata;                             /* ������: ��������ѱ�ȡ�� */

	/* ���ת�� */
	char *convert;                             /* ���ת���ռ� */
	size_t convert_size;                       /* ���ת���ռ�Ĵ�С */

	int msglen;                                /* ��Ų�ѯ���ݵ������еĳ��Ⱥ� */
	size_t row_size;                           /* �������C�����е������еĳ��Ⱥ� */
	alias_pool_t	*ap;
} ifx_dbconn_t;

/* ���ݿ�������������Ϣ */
typedef struct
{
	unsigned long	pid;                         /* ����ID */
	unsigned long	tid;                         /* �߳�ID */
	char	svrname[DB_SVR_NAME_MAX_LEN];         /* ������ */
	char	cnname[DB_CONN_NAME_MAX_LEN];         /* ������ */
	ifx_dbconn_t	conn[IFX_MAX_CONN];
	alias_pool_t	ap;                           /* �ڴ�� */
}ifx_cntx_t;

#endif /*__DB_IFX_TYPE_H__*/
