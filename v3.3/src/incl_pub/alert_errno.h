#if !defined(__ALERT_ERRNO_H__)
#define __ALERT_ERRNO_H__

/*********************************************************************************************

                                ����: �����������������޸�!!!

 **********************************************************************************************/

#define ERRNO_MAX_LEN       (5)         /* ���������󳤶� */
#define ERRNO_BEGIN         (1)         /* �����뿪ʼֵ */
#define ERRNO_END           (99999)     /* ���������ֵ */
#define ALERT_LIB_PATH      "txlib"
#define ALERT_CFG_PATH      "cfg/alert"		/* Ԥ������·�� */
#define ALERT_PERRNO_CFG_PATH "cfg/alert"	/* ƽ̨����������·�� */
#define ALERT_OERRNO_CFG_PATH   "cfg/alert"	/* ���״���������·�� */


/* ���������� */
typedef enum
{
	ERRNO_TYPE_PLAT,
	ERRNO_TYPE_OPR,
	ERRNO_TYPE_TOTAL,
	ERRNO_TYPE_UNKNOW = ERRNO_TYPE_TOTAL,
}ERRNO_TYPE_ENUM;


/* ���ø�ģ������� ������ */
#define ERRNO_PLAT_MAX_NUM   (30000)     /* ƽ̨������ ������ */
#define ERRNO_OPR_MAX_NUM   (20000)     /* ���״����� ������ */
#define ERRNO_RSV_MAX_NUM   (59999)     /* Ԥ�������� ������ */

/* �����ģ������� ��Χ */
#define ERRNO_PLAT_BEGIN     (ERRNO_BEGIN)                         /* ƽ̨������ ��ʼֵ  20001 */


#define ERRNO_PLAT_END       (ERRNO_PLAT_BEGIN + ERRNO_PLAT_MAX_NUM - 1)   /* ƽ̨������ ��ʼֵ  30000 */

#define ERRNO_OPR_BEGIN     (ERRNO_PLAT_END + 1)                         /* ���״����� ��ʼֵ 30001 */
#define ERRNO_OPR_END       (ERRNO_OPR_BEGIN + ERRNO_OPR_MAX_NUM - 1)   /* ���״����� ��ʼֵ 40000 */

#define ERRNO_RSV_BEGIN     (ERRNO_OPR_END + 1)                         /* Ԥ�������� ��ʼֵ 40001 */
#define ERRNO_RSV_END       (ERRNO_RSV_BEGIN + ERRNO_RSV_MAX_NUM - 1)   /* Ԥ�������� ��ʼֵ 99999 */


/*********************************************************************************************

                                �����ģ�������
                                       
                  ע��: �ڴ����õ�ֵ����������������ļ�����һ�£����򽫻�����쳣

 **********************************************************************************************/
/* ϵͳ��ش����� ���� */
/* ��ʼֵ:ERRNO_SYSMNT_BEGIN(00001) */
#define ERR_MEM		(00001)
#define ERR_CPU		(00002)


/* ===���ڴ���֮�ϰ�˳�����ε���=== */
/* ע��: ���ֵ���ܳ���ERRNO_SYSMNT_END(10000) */
/*****************************************************************************/
/* ƽ̨��ش����� ���� */
/* ��ʼֵ:ERRNO_PLATMNT_BEGIN(10001) */
#define ERR_TX_CNT	(10001)
#define ERR_MTYPE_USED	(10002)
#define ERR_PROC_MEM	(10003)
#define ERR_CORE	(10004)



/* ===���ڴ���֮�ϰ�˳�����ε���=== */
/* ע��: ���ֵ���ܳ���ERRNO_PLATMNT_END(20000) */
/*****************************************************************************/
/* ƽ̨������ ���� */
/* ��ʼֵ:ERRNO_PLAT_BEGIN(20001) */
#define ERR_CALLOC_FAILED	(20001)     	/* callocʧ�� */
#define ERR_MALLOC_FAILED	(20002)    	/* mallocʧ�� */
#define ERR_REALLOC_FAILED	(20003)		/*reallocʧ�� */
#define ERR_MTYPE_FAILED	(20004)		/*mtypeʧ��*/
#define ERR_TRCNO_FAILED	(20005)		/*tracenoʧ��*/
#define ERR_DATE_FAILED		(20006)		/*dateʧ��*/
#define ERR_PROC_FAILED		(20007)		/*procʧ��*/
#define ERR_OFTEN_RESTART_FAILED (20008)	/*�������ʱ��̫��*/
#define ERR_CNT_RESTART_FAILED	(20009)		/*�����ﵽ������*/
#define ERR_DBCONN_FAILED	(20010)		/*�������ݿ�ʧ��*/
#define ERR_TCPSS_FAILED    (20011)     /* TCPSSʧ�� */
#define ERR_TCPSC_FAILED    (20011)     /* TCPSCʧ�� */
#define ERR_TCPLC_FAILED    (20013)     /* TCPLCʧ�� */
#define ERR_CONMAX_FAILED	
#define ERR_RECV_FAILED
#define ERR_ENC_FAILED
#define ERR_PKGIN_FAILED
#define ERR_RECVFILE_FAILED
#define ERR_ROUTE_FAILED
#define ERR_REGTRACE_FAILED
#define ERR_UNSERISE_FAILED
#define ERR_FINDFD_FAILED
#define	ERR_BYMTYPE_INDEX_FAILED
#define ERR_PKGOUT_FAILED
#define ERR_DEC_FAILED
#define ERR_SEND_FAILED
#define ERR_CONN_FAILED
#define ERR_RECOVLINK_FAILED
#define ERR_DELLINK_FAILED
#define ERR_SAVELINK_FAILED


/* ===���ڴ���֮�ϰ�˳�����ε���=== */
/* ע��: ���ֵ���ܳ���ERRNO_PLAT_END(30000) */
/*****************************************************************************/
/* ���״����� */
/* ��ʼֵ:ERRNO_OPR_BEGIN(30001) */
#define ERR_DB_INSERT       (30001)     /* ��������ʧ�� */
#define ERR_DB_UPDATE       (30002)     /* ��������ʧ�� */
#define ERR_DB_SELECT       (30003)     /* ��ѯ����ʧ�� */
#define ERR_DB_DELETE       (30004)     /* ɾ������ʧ�� */
#define ERR_DEFAULT_SVC_OPR	(30005)
#define ERR_COUNT_SVC_OPR 	(30006)
#define ERR_FLOW_FAILED		(30007)


/* ===���ڴ���֮�ϰ�˳�����ε���=== */
/* ע��: ���ֵ���ܳ���ERRNO_OPR_END(40000) */
/****************************************************************************/

#endif /*__ALERT_ERRNO_H__*/
