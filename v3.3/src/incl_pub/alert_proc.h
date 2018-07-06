#if !defined(__ALERT_PROC_H__)
#define __ALERT_PROC_H__

#if defined(__ALERT_SUPPORT__)

#include "alert.h"
#include "cycle.h"
#include "pub_type.h"
#include "thread_pool.h"

#define ALERT_CMD_EXIT          (2)         /* Exit alert */

/* 长度设置 */
#define ALERT_PACK_HEAD_LEN     (8)		    /* 报文头长度 */
#define ALERT_NBUF_MAX_LEN      (2*1024)	/* 预警信息网络报文的最大长度 */
#define ALERT_RCVMSG_MAX_LEN    (512)   	/* 接收网络数据 缓冲区最大长度 */
#define ALERT_SELECT_TIMEOUT_SEC    (30)	/* 侦听超时设置: 秒 */
#define ALERT_SELECT_TIMEOUT_USEC   (0) 	/* 侦听超时设置: 微妙 */

/* 配置文件名 */
#define ERRNO_PLAT_CFG_FILE     "plat_errno.xml"	/*平台错误配置*/
#define ERRNO_OPR_CFG_FILE      "opr_errno.xml"		/*交易错误配置*/
#define ALERT_CFG_FILE          "alert.xml"

/* 预警线程名 */
#define ALERT_THREAD_PUSH_NAME         "swalert_push"
#define ALERT_THREAD_PLATMNT_NAME      "swalert_platmnt"
#define ALERT_THREAD_SYSMNT_NAME       "swalert_sysmnt"
#define ALERT_THREAD_RECV_NAME         "swalert_rcv"

/* 日志文件定义 */
#define ALERT_ERR_LOG   "error.log"
#define ALERT_DBG_LOG   "swalert.log"

typedef enum
{
    ALERT_THREAD_PUSH,                  /* 预警推送线程 */
    ALERT_THREAD_RECV,                  /* 接收线程[主] */
    ALERT_THREAD_SYSMNT,                /* 系统监控线程 */
    ALERT_THREAD_TOTAL                  /* 工作线程总数 */
}ALERT_THREAD_ENUM;

/* 错误级别 */
typedef enum
{
    ERR_LEVEL_PROMPT,                   /* 0: 提示 */
    ERR_LEVEL_WARNNING,                 /* 1: 警告 */
    ERR_LEVEL_CRITIAL,                  /* 2: 严重 */
    ERR_LEVEL_URGENT,                   /* 3: 紧急 */
    ERR_LEVEL_UNKNOW,                   /* 未知级别 */
    ERR_LEVEL_TOTAL = ERR_LEVEL_UNKNOW
}ERR_LEVEL_ENUM;

/* 平台错误码配置项 结构体 */
typedef struct
{
    int _errno;                         /* 错误码 */
    int level;                          /* 错误/预警级别，其取值范围ERR_LEVEL_ENUM中 */
    char desc[ERR_DESC_MAX_LEN];        /* 错误描述 */
    char reason[ERR_REASON_MAX_LEN];    /* 错误原因 */
    char solve[ERR_SOLVE_MAX_LEN];      /* 解决方案 */
}perrno_item_t;

/* 平台错误码配置集 结构体 */
typedef struct
{
    int minno;                          /* 集合中实际错误码最小值 */
    int maxno;                          /* 集合中实际错误码最大值 */
    int num;                            /* 集合中错误码个数 */
    perrno_item_t *item;                /* 错误码集合 */
}perrno_cfg_t;

/* 交易错误码预警条件配置结构体 */
typedef struct
{
    char txcode[ERR_TXCODE_MAX_LEN];    /* 交易码 */
    int seconds;                        /* 间隔时间 */
    int times;                          /* 此错误的次数达到TIMES时，发出预警 */

    int currtimes;                      /* 当前错误次数 */
    time_t timestamp;                   /* 时间戳。（注：只记录重复计数时的时间，
                                                其他情况下出现错误的时间并不记录，也无法记录） */
}oprcond_cfg_t;

/* 交易错误码配置项 结构体 */
typedef struct
{
    int _errno;                         /* 错误码 */
    int level;                          /* 预警级别，其取值范围ERR_LEVEL_ENUM中 */
    oprcond_cfg_t conditions;           /* 预警条件设置 */
    char desc[ERR_DESC_MAX_LEN];        /* 错误描述 */
    char reason[ERR_REASON_MAX_LEN];    /* 错误原因 */
    char solve[ERR_SOLVE_MAX_LEN];      /* 解决方案 */
}oerrno_item_t;

/* 交易错误码配置集 结构体 */
typedef struct
{
    int minno;                          /* 集合中实际错误码最小值 */
    int maxno;                          /* 集合中实际错误码最大值 */
    int num;                            /* 集合中错误码个数 */
    oerrno_item_t *item;                /* 错误码集合 */
}oerrno_cfg_t;

#if defined(__ALERT_MNT_SUPPORT__)
/* 系统监控配置 结构体 */
typedef struct
{
    int level;                          /* 预警级别 */
    int interval;                       /* 平台监控的间隔时间 */
    int swt_cpu;                        /* 开关：是否监控系统CPU状态；其取值为SWITCH_OFF或SWITCH_ON */
    int swt_mem;                        /* 开关：是否监控系统内存状态；其取值为SWITCH_OFF或SWITCH_ON */
    int swt_swap;                       /* 开关：是否监控交换区状态；其取值为SWITCH_OFF或SWITCH_ON */
}sysmnt_cfg_t;

/* 平台监控配置 结构体 */
typedef struct
{
    int level;                          /* 预警级别 */
    int interval;                       /* 平台监控的间隔时间 */
    int swt_stat;                       /* 开关：是否监控平台当前状态；其取值为SWITCH_OFF或SWITCH_ON */
    int swt_procnum;                    /* 开关：是否监控平台进程数；其取值为SWITCH_OFF或SWITCH_ON */
    int swt_flwnum;                     /* 开关：是否监控流水条数；其取值为SWITCH_OFF或SWITCH_ON */
    int swt_bufuse;                     /* 开关：是否监控节点缓冲区已用节点数；其取值为SWITCH_OFF或SWITCH_ON */
    int swt_buffree;                    /* 开关：是否监控节点缓冲区剩余节点数；其取值为SWITCH_OFF或SWITCH_ON */
}platmnt_cfg_t;
#endif /*__ALERT_MNT_SUPPORT__*/

/* 预警进程 结构体 */
typedef struct
{
	sw_cycle_t base;                    /* 上下文 */
    alert_cfg_t alertcfg;               /* 预警配置信息 */
    perrno_cfg_t perrno; /* 平台错误码集合 */
    oerrno_cfg_t oerrno;                /* 交易错误码集合 */
    thread_pool_t *threadpool;          /* 线程池 */
}alert_cycle_t;

/* 错误码配置适配表 */
typedef struct
{
    int type;                           /* 错误码类型， 其取值范围是枚举ERRNO_TYPE_ENUM中定义 */
    int begin;                          /* 错误码的开始值 */
    int end;                            /* 错误码的结束值 */
    char fname[FILE_NAME_MAX_LEN];      /* 错误码配置文件 */
}alert_errcfg_adapt_t;

/* 平台错误码预警信息 */
typedef struct
{
    const alert_error_t *mtext;      /* 预警队列内容 */
    const perrno_item_t *item;          /* 错误码配置信息 */
}alert_perrmsg_t;

/* 交易错误码预警信息 */
typedef struct
{
    const alert_error_t *mtext;      /* 预警队列内容 */
    const oerrno_item_t *item;          /* 错误码配置信息 */
}alert_oerrmsg_t;

#endif /*__ALERT_SUPPORT__*/
#endif /*__ALERT_PROC_H__*/
