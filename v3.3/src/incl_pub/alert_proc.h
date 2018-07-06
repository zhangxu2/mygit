#if !defined(__ALERT_PROC_H__)
#define __ALERT_PROC_H__

#if defined(__ALERT_SUPPORT__)

#include "alert.h"
#include "cycle.h"
#include "pub_type.h"
#include "thread_pool.h"

#define ALERT_CMD_EXIT          (2)         /* Exit alert */

/* �������� */
#define ALERT_PACK_HEAD_LEN     (8)		    /* ����ͷ���� */
#define ALERT_NBUF_MAX_LEN      (2*1024)	/* Ԥ����Ϣ���籨�ĵ���󳤶� */
#define ALERT_RCVMSG_MAX_LEN    (512)   	/* ������������ ��������󳤶� */
#define ALERT_SELECT_TIMEOUT_SEC    (30)	/* ������ʱ����: �� */
#define ALERT_SELECT_TIMEOUT_USEC   (0) 	/* ������ʱ����: ΢�� */

/* �����ļ��� */
#define ERRNO_PLAT_CFG_FILE     "plat_errno.xml"	/*ƽ̨��������*/
#define ERRNO_OPR_CFG_FILE      "opr_errno.xml"		/*���״�������*/
#define ALERT_CFG_FILE          "alert.xml"

/* Ԥ���߳��� */
#define ALERT_THREAD_PUSH_NAME         "swalert_push"
#define ALERT_THREAD_PLATMNT_NAME      "swalert_platmnt"
#define ALERT_THREAD_SYSMNT_NAME       "swalert_sysmnt"
#define ALERT_THREAD_RECV_NAME         "swalert_rcv"

/* ��־�ļ����� */
#define ALERT_ERR_LOG   "error.log"
#define ALERT_DBG_LOG   "swalert.log"

typedef enum
{
    ALERT_THREAD_PUSH,                  /* Ԥ�������߳� */
    ALERT_THREAD_RECV,                  /* �����߳�[��] */
    ALERT_THREAD_SYSMNT,                /* ϵͳ����߳� */
    ALERT_THREAD_TOTAL                  /* �����߳����� */
}ALERT_THREAD_ENUM;

/* ���󼶱� */
typedef enum
{
    ERR_LEVEL_PROMPT,                   /* 0: ��ʾ */
    ERR_LEVEL_WARNNING,                 /* 1: ���� */
    ERR_LEVEL_CRITIAL,                  /* 2: ���� */
    ERR_LEVEL_URGENT,                   /* 3: ���� */
    ERR_LEVEL_UNKNOW,                   /* δ֪���� */
    ERR_LEVEL_TOTAL = ERR_LEVEL_UNKNOW
}ERR_LEVEL_ENUM;

/* ƽ̨������������ �ṹ�� */
typedef struct
{
    int _errno;                         /* ������ */
    int level;                          /* ����/Ԥ��������ȡֵ��ΧERR_LEVEL_ENUM�� */
    char desc[ERR_DESC_MAX_LEN];        /* �������� */
    char reason[ERR_REASON_MAX_LEN];    /* ����ԭ�� */
    char solve[ERR_SOLVE_MAX_LEN];      /* ������� */
}perrno_item_t;

/* ƽ̨���������ü� �ṹ�� */
typedef struct
{
    int minno;                          /* ������ʵ�ʴ�������Сֵ */
    int maxno;                          /* ������ʵ�ʴ��������ֵ */
    int num;                            /* �����д�������� */
    perrno_item_t *item;                /* �����뼯�� */
}perrno_cfg_t;

/* ���״�����Ԥ���������ýṹ�� */
typedef struct
{
    char txcode[ERR_TXCODE_MAX_LEN];    /* ������ */
    int seconds;                        /* ���ʱ�� */
    int times;                          /* �˴���Ĵ����ﵽTIMESʱ������Ԥ�� */

    int currtimes;                      /* ��ǰ������� */
    time_t timestamp;                   /* ʱ�������ע��ֻ��¼�ظ�����ʱ��ʱ�䣬
                                                ��������³��ִ����ʱ�䲢����¼��Ҳ�޷���¼�� */
}oprcond_cfg_t;

/* ���״����������� �ṹ�� */
typedef struct
{
    int _errno;                         /* ������ */
    int level;                          /* Ԥ��������ȡֵ��ΧERR_LEVEL_ENUM�� */
    oprcond_cfg_t conditions;           /* Ԥ���������� */
    char desc[ERR_DESC_MAX_LEN];        /* �������� */
    char reason[ERR_REASON_MAX_LEN];    /* ����ԭ�� */
    char solve[ERR_SOLVE_MAX_LEN];      /* ������� */
}oerrno_item_t;

/* ���״��������ü� �ṹ�� */
typedef struct
{
    int minno;                          /* ������ʵ�ʴ�������Сֵ */
    int maxno;                          /* ������ʵ�ʴ��������ֵ */
    int num;                            /* �����д�������� */
    oerrno_item_t *item;                /* �����뼯�� */
}oerrno_cfg_t;

#if defined(__ALERT_MNT_SUPPORT__)
/* ϵͳ������� �ṹ�� */
typedef struct
{
    int level;                          /* Ԥ������ */
    int interval;                       /* ƽ̨��صļ��ʱ�� */
    int swt_cpu;                        /* ���أ��Ƿ���ϵͳCPU״̬����ȡֵΪSWITCH_OFF��SWITCH_ON */
    int swt_mem;                        /* ���أ��Ƿ���ϵͳ�ڴ�״̬����ȡֵΪSWITCH_OFF��SWITCH_ON */
    int swt_swap;                       /* ���أ��Ƿ��ؽ�����״̬����ȡֵΪSWITCH_OFF��SWITCH_ON */
}sysmnt_cfg_t;

/* ƽ̨������� �ṹ�� */
typedef struct
{
    int level;                          /* Ԥ������ */
    int interval;                       /* ƽ̨��صļ��ʱ�� */
    int swt_stat;                       /* ���أ��Ƿ���ƽ̨��ǰ״̬����ȡֵΪSWITCH_OFF��SWITCH_ON */
    int swt_procnum;                    /* ���أ��Ƿ���ƽ̨����������ȡֵΪSWITCH_OFF��SWITCH_ON */
    int swt_flwnum;                     /* ���أ��Ƿ�����ˮ��������ȡֵΪSWITCH_OFF��SWITCH_ON */
    int swt_bufuse;                     /* ���أ��Ƿ��ؽڵ㻺�������ýڵ�������ȡֵΪSWITCH_OFF��SWITCH_ON */
    int swt_buffree;                    /* ���أ��Ƿ��ؽڵ㻺����ʣ��ڵ�������ȡֵΪSWITCH_OFF��SWITCH_ON */
}platmnt_cfg_t;
#endif /*__ALERT_MNT_SUPPORT__*/

/* Ԥ������ �ṹ�� */
typedef struct
{
	sw_cycle_t base;                    /* ������ */
    alert_cfg_t alertcfg;               /* Ԥ��������Ϣ */
    perrno_cfg_t perrno; /* ƽ̨�����뼯�� */
    oerrno_cfg_t oerrno;                /* ���״����뼯�� */
    thread_pool_t *threadpool;          /* �̳߳� */
}alert_cycle_t;

/* ��������������� */
typedef struct
{
    int type;                           /* ���������ͣ� ��ȡֵ��Χ��ö��ERRNO_TYPE_ENUM�ж��� */
    int begin;                          /* ������Ŀ�ʼֵ */
    int end;                            /* ������Ľ���ֵ */
    char fname[FILE_NAME_MAX_LEN];      /* �����������ļ� */
}alert_errcfg_adapt_t;

/* ƽ̨������Ԥ����Ϣ */
typedef struct
{
    const alert_error_t *mtext;      /* Ԥ���������� */
    const perrno_item_t *item;          /* ������������Ϣ */
}alert_perrmsg_t;

/* ���״�����Ԥ����Ϣ */
typedef struct
{
    const alert_error_t *mtext;      /* Ԥ���������� */
    const oerrno_item_t *item;          /* ������������Ϣ */
}alert_oerrmsg_t;

#endif /*__ALERT_SUPPORT__*/
#endif /*__ALERT_PROC_H__*/
