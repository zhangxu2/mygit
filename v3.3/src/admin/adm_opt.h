#ifndef __ADM_OPT_H__
#define __ADM_OPT_H__

#include "pub_type.h"
#include "pub_mem.h"

#define SW_MAX_ARGC 24
#define SW_MAX_ARG_SIZE	32

/* 模块划分 */
typedef enum
{
	ADM_SUB_MOD_DFIS = 0x00000001,      /* DFIS BP类型 */
	ADM_SUB_MOD_PRDT = 0x00000002,      /* 产品类型 */
	ADM_SUB_MOD_ALL = ADM_SUB_MOD_DFIS |  ADM_SUB_MOD_PRDT   /* 所有类型 */
}adm_mod_type_t;


typedef enum
{
    /* DFIS-BP命令 */
    ADM_CMD_START_ALL = 0,      /* 启动所有程序 */
    ADM_CMD_START_LSN_ALL,      /* 启动所有侦听 */
    ADM_CMD_START_LSN,          /* 启动指定侦听 */
    ADM_CMD_START_LSN_IN_PRDT, /*启动指定产品的指定侦听*/
    ADM_CMD_START_PROC,         /* 启动指定程序 */
    ADM_CMD_START_SVC_ALL,      /* 启动所有SVC */
    ADM_CMD_START_SVC,          /* 启动指定SVC */
    ADM_CMD_START_SVR_IN_PRDT,  /* 启动指定产品下的指定server*/
    ADM_CMD_START_PRDT,         /* 启动指定PRDT */
    
    ADM_CMD_QUIT,               /* 退出ADMIN */

    ADM_CMD_STOP_ALL,           /* 停止所有程序 */
    ADM_CMD_STOP_PROC,
    ADM_CMD_STOP_LSN_ALL,       /* 停止所有侦听 */
    ADM_CMD_STOP_LSN,           /* 停止指定侦听 */
    ADM_CMD_STOP_LSN_IN_PRDT,           /* 停止指定产品的指定侦听 */
    ADM_CMD_STOP_SVC_ALL,       /* 停止所有服务 */
    ADM_CMD_STOP_SVC,           /* 停止指定服务 */
    ADM_CMD_STOP_PRDT,          /* 停止指定PRDT ?*/
    ADM_CMD_STOP_SVR_IN_PRDT,   /* 停止指定PRDT中的指定SVR */

    ADM_CMD_LIST_ALL,           /* 显示所有系统进程 */
    ADM_CMD_LIST_LSN_ALL,       /* 显示所有系统侦听主进程 */
    ADM_CMD_LIST_SVC_ALL,       /* 显示所有系统SVC主进程 */
    ADM_CMD_LIST_PRDT_ALL,      /* 显示所有产品 */
    ADM_CMD_LIST_ALL_IN_PRDT,      /* 显示所有产品所有进程 */
    ADM_CMD_LIST_VERSION,       /* 显示版本信息 */
    ADM_CMD_LIST_HIS_PATCH,    /*显示历史补丁信息*/
    ADM_CMD_LIST_MTYPE_ALL,     /* 显示所有Mtype信息 */
    ADM_CMD_LIST_MTYPE_BUSY,    /* 显示被占用Mtype信息 */
    ADM_CMD_LIST_MTYPE_FREE,    /* 显示未占用Mtype信息 */
    ADM_CMD_LIST_MTYPE,         /* 显示指定Mtype信息 */
    ADM_CMD_LIST_SEG_MTYPE,     /* 显示部分Mtype信息 */
    ADM_CMD_LIST_RES_ALL,       /* 显示所有资源信息 */
    ADM_CMD_LIST_RES_SHM,       /* 显示共享内存信息 */
    ADM_CMD_LIST_RES_MSQ,       /* 显示消息队列信息 */

    ADM_CMD_CLEAR,              /* 清屏操作 */
    ADM_CMD_CLEAN,              /* 平台资源清理 */
    ADM_CMD_LIST_TASK_ALL,
    ADM_CMD_OPT_TASK_ALL,
    ADM_CMD_LIST_TASK_M,
    ADM_CMD_LIST_TASK_AU,
    ADM_CMD_LIST_TASK_S,
    ADM_CMD_OPT_TASK_S,
    /* TRACE相关命令 */
    ADM_CMD_TOP,                /* 显示最新流水 */
    ADM_CMD_STEP,               /* 查看指定流水 */

	ADM_CMD_GET_LOG,
    /* 产品相关命令 */
    ADM_CMD_GOTO_PRDT,          /* 进入指定产品 */
    ADM_CMD_PRDT_START_ALL,     /* 启动所有产品下所有进程 */
    ADM_CMD_PRDT_START_ALL_LSN, /* 启动产品下所有侦听 */
    ADM_CMD_PRDT_START_LSN,     /* 启动产品下指定侦听 */
    ADM_CMD_PRDT_START_ALL_SVC, /* 启动产品下所有SVC */
    ADM_CMD_PRDT_START_SVC,     /* 启动产品下指定SVC */
    
    ADM_CMD_PRDT_STOP_ALL,      /* 停止所有产品下所有进程 */
    ADM_CMD_PRDT_STOP_LSN_ALL,  /* 停止产品下所有侦听 */
    ADM_CMD_PRDT_STOP_LSN,      /* 停止产品下指定侦听 */
    ADM_CMD_PRDT_STOP_SVC_ALL,  /* 停止产品下所有SVC */
    ADM_CMD_PRDT_STOP_SVC,      /* 停止产品下指定SVC */

    ADM_CMD_PRDT_LIST_ALL,      /* 显示产品下所有主进程 */
    ADM_CMD_PRDT_LIST_LSN_ALL,  /* 显示产品下所有侦听进程 */
    ADM_CMD_PRDT_LIST_SVC_ALL,  /* 显示产品下所有SVC进程 */
    ADM_CMD_PRDT_LIST_CHILD_ALL,/* 显示产品下所有子进程 */
    ADM_CMD_PRDT_LIST_MTYPE_ALL,/* 显示产品下所有MTYPE信息 */

    /* SET相关命令 */
    ADM_CMD_SET_PLT_FLOW,       /* 设置平台流水 */
    ADM_CMD_SET_BSN_FLOW,       /* 设置业务流水 */
    ADM_CMD_SET_DATE,           /* 设置平台日期: 未加锁 */
    ADM_CMD_SET_DATE_LOCK,      /* 锁住平台日期 */
    ADM_CMD_SET_DATE_UNLOCK,    /* 解锁平台日期 */

	/* GET相关命令 */
    ADM_CMD_GET_PLT_FLOW,       /* 获取平台流水 */
    ADM_CMD_GET_BSN_FLOW,       /* 获取业务流水 */
    ADM_CMD_GET_BSN_FLOW_ALL,   /* 获取所有业务流水 */
    ADM_CMD_GET_DATE,           /* 获取平台日期 */

    ADM_CMD_SAF_LIST,		/* saf list item  */
    ADM_CMD_SAF_START,		/* saf start */
    ADM_CMD_SAF_STOP,		/* saf stop */

    ADM_CMD_DEPLOY,             
    ADM_CMD_RELOAD,
    ADM_CMD_STARTUP,
    ADM_CMD_SHUTDOWN,

    /* 其他相关命令 */    
    ADM_CMD_HELP,               /* 帮助命令 */
    ADM_CMD_PARAM_ERR,          /* 命令参数错误 */
    ADM_CMD_UNKNOWN,            /* 未知命令 */
    ADM_CMD_TOTAL
}adm_cmd_type_e;

#define ADM_MOD_NAME_LEN  (64)  /* 模块名长度 */

typedef struct
{
    adm_cmd_type_e cmd_type;                        /* 当前命令类型 */

    sw_int32_t local_argc;                          /* 命令行参数个数 */
    sw_char_t *local_argv[SW_MAX_ARGC];             /* 命令行参数 */
    sw_char_t mem[SW_MAX_ARGC * SW_MAX_ARG_SIZE];
    
    adm_mod_type_t mod_type;                        /* 当前所在模块 */
    sw_char_t mod_name[ADM_MOD_NAME_LEN];           /* 当前模块名 */
}sw_adm_opt_t;

typedef sw_int_t (*opt_get_cmd_type_func_t)(sw_adm_opt_t *opt);

SW_PUBLIC sw_int_t adm_opt_parse(sw_adm_opt_t* opt, sw_char_t* line);

#define adm_cmd_is_valid(cmd_type) ((cmd_type >= ADM_CMD_START_ALL) && (cmd_type < ADM_CMD_TOTAL))

#endif /* __ADM_OPT_H__ */


