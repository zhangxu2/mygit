#if !defined(__ALERT_H__)
#define __ALERT_H__

#include "pub_type.h"
#include "alert_errno.h"
#include "pub_log.h"

#if defined(__ALERT_SUPPORT__)

/* ���� */
#define SWITCH_OFF      (0)             /* ����: �� */
#define SWITCH_ON       (1)             /* ����: �� */

/* �������� */
#define ERR_DESC_MAX_LEN        (128)   /* �������� ��󳤶� */
#define ERR_REASON_MAX_LEN      (256)   /* ����ԭ�� ��󳤶� */
#define ERR_SOLVE_MAX_LEN       (256)   /* ������� ��󳤶� */
#define ERR_REAMRK_MAX_LEN      (256)   /* ����ע ��󳤶� */
#define ERR_TXCODE_MAX_LEN      (16)    /* ��������󳤶� */

#define ALERT_MSQ_KEY_FILE      ".key_file"

/* Ԥ��������Ϣ���� */
typedef enum
{
    ALERT_TYPE_PLATERR = 1,              /* ƽ̨�쳣��Ϣ���� */
    ALERT_TYPE_OPRERR,                 /* �����쳣��Ϣ���� */
    ALERT_TYPE_UNKNOW                 /* δ֪��Ϣ���� */
}ALERT_TYPE_ENUM;

/* ������Ԥ��������Ϣ���� */
typedef struct
{
    int _errno;                         /* ������ */
    char remark[ERR_REAMRK_MAX_LEN];    /* ������Ϣ */
}alert_error_t;

/* ƽ̨���Ԥ��������Ϣ���� */
typedef struct
{
    int stat;                           /* ƽ̨��ǰ״̬����Ӧ���̱����ڴ���stSMAHEAD��sw_work_stat�ֶ� */
    int procnum;                        /* ƽ̨����������Ӧ���̱����ڴ���stSMAHEAD��iCount�ֶ� */
    int flwnum;                         /* ƽ̨��ˮ��������Ӧ����ʱ�����ڴ���stRunParam�е�iCurUse�ֶ� */
    int bufused;                        /* �ڵ㻺�������ýڵ�������Ӧ����ʱ�����ڴ���stRunParam�е�iBufUse�ֶΡ� */
    int buffree;                        /* �ڵ㻺����ʣ��ڵ�������Ӧ����ʱ�����ڴ���stRunParam�е�iBufFree�ֶΡ� */
}alert_platmnt_t;

/* ϵͳ���Ԥ��������Ϣ���� */
typedef struct
{
	int stat;
}alert_sysmnt_t;

typedef union
{
    char alert_error_size[sizeof(alert_error_t)];       /* ƽ̨&�����쳣���г��� */
    char alert_platmnt_size[sizeof(alert_platmnt_t)];   /* ƽ̨��ض��г��� */
    char alert_sysmnt_size[sizeof(alert_sysmnt_t)];     /* ϵͳ��ض��г��� */
}alert_mtext_mem_t;

/* ��Ϣ������󳤶� */
#define ALERT_MTEXT_MAX_LEN (sizeof(alert_mtext_mem_t)+1)

typedef struct
{
    long mtype;                         /* ��Ϣ����: ȡֵ��ΧALERT_TYPE_ENUM�ж��� */
    char mtext[ALERT_MTEXT_MAX_LEN];    /* ��Ϣ���� */
}alert_msq_t;

/******************************************************************************
 **��������: alert_pack_func_t
 **��    ��: �鷢����Զ�̷���ı���
 **�������: 
 **      type: Ԥ����Ϣ����ȡֵ��Χ��ALERT_TYPE_ENUM�ж���
 **      msg: Ҫ�������Ԥ����Ϣ
 **           1. Ϊƽ̨�쳣��Ϣʱ��msg������Ϊalert_perrmsg_t��alert_oerrmsg_t
 **      pack: ���Ļ�����
 **      len: ���Ļ�������󳤶�
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 ******************************************************************************/
typedef int (*alert_pack_func_t)(int type, const void *msg, char *pack, int len);

/* Զ�̷������� �ṹ�� */
typedef struct _alert_remote_t
{
    int sckid;                          /* ͨ��SCK�׽��� */
    alert_pack_func_t packfunc;         /* �������ָ�� */
    void *handle;                       /* ��̬���� */
    
    /* ���±�����Ӧalert.xml��REMOTE���� */
    int isuse;                          /* ����:�Ƿ��������� ȡֵΪSWITCH_OFF��SWITCH_ON */
    int level;                          /* Ԥ������: ���������Ԥ������ */
    int port;                           /* Զ�̶˿ں� */
    long starttime;
    char ipaddr[IP_ADDR_MAX_LEN];       /* Զ��IP��ַ */
    char libname[LIB_NAME_MAX_LEN];     /* ��̬���� */
    char funcname[FUNC_NAME_MAX_LEN];   /* ������ */

    struct _alert_remote_t *next;
}alert_remote_t;

/* Ԥ��������Ϣ �ṹ�� */
/* ע: ���Ӧalert.xml�е��������� */
typedef struct
{
    bool _switch;                       /* Ԥ������ */
    int level;                          /* ƽ̨Ԥ������ */
    alert_remote_t *remote;             /* Զ�̷����������� */
}alert_cfg_t;

extern int alert_link(void);
extern int alert_get_msqid(void);
extern void alert_set_msqid(int msqid);
extern int alert_check_msqid(void);
extern int alert_msg(int _errno, const char *format, ...);

extern sw_int32_t alert_msq_get_key(void);
extern sw_int32_t alert_msq_creat(void);
extern sw_int_t alert_cfg_load(alert_cfg_t *cfg);

#endif /*__ALERT_SUPPORT__*/
#endif /*__ALERT_H__*/
