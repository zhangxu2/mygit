#ifndef __ADM_OPT_H__
#define __ADM_OPT_H__

#include "pub_type.h"
#include "pub_mem.h"

#define SW_MAX_ARGC 24
#define SW_MAX_ARG_SIZE	32

/* ģ�黮�� */
typedef enum
{
	ADM_SUB_MOD_DFIS = 0x00000001,      /* DFIS BP���� */
	ADM_SUB_MOD_PRDT = 0x00000002,      /* ��Ʒ���� */
	ADM_SUB_MOD_ALL = ADM_SUB_MOD_DFIS |  ADM_SUB_MOD_PRDT   /* �������� */
}adm_mod_type_t;


typedef enum
{
    /* DFIS-BP���� */
    ADM_CMD_START_ALL = 0,      /* �������г��� */
    ADM_CMD_START_LSN_ALL,      /* ������������ */
    ADM_CMD_START_LSN,          /* ����ָ������ */
    ADM_CMD_START_LSN_IN_PRDT, /*����ָ����Ʒ��ָ������*/
    ADM_CMD_START_PROC,         /* ����ָ������ */
    ADM_CMD_START_SVC_ALL,      /* ��������SVC */
    ADM_CMD_START_SVC,          /* ����ָ��SVC */
    ADM_CMD_START_SVR_IN_PRDT,  /* ����ָ����Ʒ�µ�ָ��server*/
    ADM_CMD_START_PRDT,         /* ����ָ��PRDT */
    
    ADM_CMD_QUIT,               /* �˳�ADMIN */

    ADM_CMD_STOP_ALL,           /* ֹͣ���г��� */
    ADM_CMD_STOP_PROC,
    ADM_CMD_STOP_LSN_ALL,       /* ֹͣ�������� */
    ADM_CMD_STOP_LSN,           /* ָֹͣ������ */
    ADM_CMD_STOP_LSN_IN_PRDT,           /* ָֹͣ����Ʒ��ָ������ */
    ADM_CMD_STOP_SVC_ALL,       /* ֹͣ���з��� */
    ADM_CMD_STOP_SVC,           /* ָֹͣ������ */
    ADM_CMD_STOP_PRDT,          /* ָֹͣ��PRDT ?*/
    ADM_CMD_STOP_SVR_IN_PRDT,   /* ָֹͣ��PRDT�е�ָ��SVR */

    ADM_CMD_LIST_ALL,           /* ��ʾ����ϵͳ���� */
    ADM_CMD_LIST_LSN_ALL,       /* ��ʾ����ϵͳ���������� */
    ADM_CMD_LIST_SVC_ALL,       /* ��ʾ����ϵͳSVC������ */
    ADM_CMD_LIST_PRDT_ALL,      /* ��ʾ���в�Ʒ */
    ADM_CMD_LIST_ALL_IN_PRDT,      /* ��ʾ���в�Ʒ���н��� */
    ADM_CMD_LIST_VERSION,       /* ��ʾ�汾��Ϣ */
    ADM_CMD_LIST_HIS_PATCH,    /*��ʾ��ʷ������Ϣ*/
    ADM_CMD_LIST_MTYPE_ALL,     /* ��ʾ����Mtype��Ϣ */
    ADM_CMD_LIST_MTYPE_BUSY,    /* ��ʾ��ռ��Mtype��Ϣ */
    ADM_CMD_LIST_MTYPE_FREE,    /* ��ʾδռ��Mtype��Ϣ */
    ADM_CMD_LIST_MTYPE,         /* ��ʾָ��Mtype��Ϣ */
    ADM_CMD_LIST_SEG_MTYPE,     /* ��ʾ����Mtype��Ϣ */
    ADM_CMD_LIST_RES_ALL,       /* ��ʾ������Դ��Ϣ */
    ADM_CMD_LIST_RES_SHM,       /* ��ʾ�����ڴ���Ϣ */
    ADM_CMD_LIST_RES_MSQ,       /* ��ʾ��Ϣ������Ϣ */

    ADM_CMD_CLEAR,              /* �������� */
    ADM_CMD_CLEAN,              /* ƽ̨��Դ���� */
    ADM_CMD_LIST_TASK_ALL,
    ADM_CMD_OPT_TASK_ALL,
    ADM_CMD_LIST_TASK_M,
    ADM_CMD_LIST_TASK_AU,
    ADM_CMD_LIST_TASK_S,
    ADM_CMD_OPT_TASK_S,
    /* TRACE������� */
    ADM_CMD_TOP,                /* ��ʾ������ˮ */
    ADM_CMD_STEP,               /* �鿴ָ����ˮ */

	ADM_CMD_GET_LOG,
    /* ��Ʒ������� */
    ADM_CMD_GOTO_PRDT,          /* ����ָ����Ʒ */
    ADM_CMD_PRDT_START_ALL,     /* �������в�Ʒ�����н��� */
    ADM_CMD_PRDT_START_ALL_LSN, /* ������Ʒ���������� */
    ADM_CMD_PRDT_START_LSN,     /* ������Ʒ��ָ������ */
    ADM_CMD_PRDT_START_ALL_SVC, /* ������Ʒ������SVC */
    ADM_CMD_PRDT_START_SVC,     /* ������Ʒ��ָ��SVC */
    
    ADM_CMD_PRDT_STOP_ALL,      /* ֹͣ���в�Ʒ�����н��� */
    ADM_CMD_PRDT_STOP_LSN_ALL,  /* ֹͣ��Ʒ���������� */
    ADM_CMD_PRDT_STOP_LSN,      /* ֹͣ��Ʒ��ָ������ */
    ADM_CMD_PRDT_STOP_SVC_ALL,  /* ֹͣ��Ʒ������SVC */
    ADM_CMD_PRDT_STOP_SVC,      /* ֹͣ��Ʒ��ָ��SVC */

    ADM_CMD_PRDT_LIST_ALL,      /* ��ʾ��Ʒ������������ */
    ADM_CMD_PRDT_LIST_LSN_ALL,  /* ��ʾ��Ʒ�������������� */
    ADM_CMD_PRDT_LIST_SVC_ALL,  /* ��ʾ��Ʒ������SVC���� */
    ADM_CMD_PRDT_LIST_CHILD_ALL,/* ��ʾ��Ʒ�������ӽ��� */
    ADM_CMD_PRDT_LIST_MTYPE_ALL,/* ��ʾ��Ʒ������MTYPE��Ϣ */

    /* SET������� */
    ADM_CMD_SET_PLT_FLOW,       /* ����ƽ̨��ˮ */
    ADM_CMD_SET_BSN_FLOW,       /* ����ҵ����ˮ */
    ADM_CMD_SET_DATE,           /* ����ƽ̨����: δ���� */
    ADM_CMD_SET_DATE_LOCK,      /* ��סƽ̨���� */
    ADM_CMD_SET_DATE_UNLOCK,    /* ����ƽ̨���� */

	/* GET������� */
    ADM_CMD_GET_PLT_FLOW,       /* ��ȡƽ̨��ˮ */
    ADM_CMD_GET_BSN_FLOW,       /* ��ȡҵ����ˮ */
    ADM_CMD_GET_BSN_FLOW_ALL,   /* ��ȡ����ҵ����ˮ */
    ADM_CMD_GET_DATE,           /* ��ȡƽ̨���� */

    ADM_CMD_SAF_LIST,		/* saf list item  */
    ADM_CMD_SAF_START,		/* saf start */
    ADM_CMD_SAF_STOP,		/* saf stop */

    ADM_CMD_DEPLOY,             
    ADM_CMD_RELOAD,
    ADM_CMD_STARTUP,
    ADM_CMD_SHUTDOWN,

    /* ����������� */    
    ADM_CMD_HELP,               /* �������� */
    ADM_CMD_PARAM_ERR,          /* ����������� */
    ADM_CMD_UNKNOWN,            /* δ֪���� */
    ADM_CMD_TOTAL
}adm_cmd_type_e;

#define ADM_MOD_NAME_LEN  (64)  /* ģ�������� */

typedef struct
{
    adm_cmd_type_e cmd_type;                        /* ��ǰ�������� */

    sw_int32_t local_argc;                          /* �����в������� */
    sw_char_t *local_argv[SW_MAX_ARGC];             /* �����в��� */
    sw_char_t mem[SW_MAX_ARGC * SW_MAX_ARG_SIZE];
    
    adm_mod_type_t mod_type;                        /* ��ǰ����ģ�� */
    sw_char_t mod_name[ADM_MOD_NAME_LEN];           /* ��ǰģ���� */
}sw_adm_opt_t;

typedef sw_int_t (*opt_get_cmd_type_func_t)(sw_adm_opt_t *opt);

SW_PUBLIC sw_int_t adm_opt_parse(sw_adm_opt_t* opt, sw_char_t* line);

#define adm_cmd_is_valid(cmd_type) ((cmd_type >= ADM_CMD_START_ALL) && (cmd_type < ADM_CMD_TOTAL))

#endif /* __ADM_OPT_H__ */


