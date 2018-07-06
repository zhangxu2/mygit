#if !defined(__ALERT_H__)
#define __ALERT_H__

#include "pub_type.h"
#include "alert_errno.h"
#include "pub_log.h"

#if defined(__ALERT_SUPPORT__)

/* 开关 */
#define SWITCH_OFF      (0)             /* 开关: 关 */
#define SWITCH_ON       (1)             /* 开关: 开 */

/* 长度设置 */
#define ERR_DESC_MAX_LEN        (128)   /* 错误描述 最大长度 */
#define ERR_REASON_MAX_LEN      (256)   /* 错误原因 最大长度 */
#define ERR_SOLVE_MAX_LEN       (256)   /* 解决方案 最大长度 */
#define ERR_REAMRK_MAX_LEN      (256)   /* 错误备注 最大长度 */
#define ERR_TXCODE_MAX_LEN      (16)    /* 交易码最大长度 */

#define ALERT_MSQ_KEY_FILE      ".key_file"

/* 预警队列消息类型 */
typedef enum
{
    ALERT_TYPE_PLATERR = 1,              /* 平台异常信息类型 */
    ALERT_TYPE_OPRERR,                 /* 交易异常信息类型 */
    ALERT_TYPE_UNKNOW                 /* 未知信息类型 */
}ALERT_TYPE_ENUM;

/* 错误码预警队列消息内容 */
typedef struct
{
    int _errno;                         /* 错误码 */
    char remark[ERR_REAMRK_MAX_LEN];    /* 附加信息 */
}alert_error_t;

/* 平台监控预警队列消息内容 */
typedef struct
{
    int stat;                           /* 平台当前状态。对应进程表共享内存中stSMAHEAD的sw_work_stat字段 */
    int procnum;                        /* 平台进程数。对应进程表共享内存中stSMAHEAD的iCount字段 */
    int flwnum;                         /* 平台流水条数。对应运行时共享内存中stRunParam中的iCurUse字段 */
    int bufused;                        /* 节点缓冲区已用节点数。对应运行时共享内存中stRunParam中的iBufUse字段。 */
    int buffree;                        /* 节点缓冲区剩余节点数。对应运行时共享内存中stRunParam中的iBufFree字段。 */
}alert_platmnt_t;

/* 系统监控预警队列消息内容 */
typedef struct
{
	int stat;
}alert_sysmnt_t;

typedef union
{
    char alert_error_size[sizeof(alert_error_t)];       /* 平台&交易异常队列长度 */
    char alert_platmnt_size[sizeof(alert_platmnt_t)];   /* 平台监控队列长度 */
    char alert_sysmnt_size[sizeof(alert_sysmnt_t)];     /* 系统监控队列长度 */
}alert_mtext_mem_t;

/* 消息队列最大长度 */
#define ALERT_MTEXT_MAX_LEN (sizeof(alert_mtext_mem_t)+1)

typedef struct
{
    long mtype;                         /* 消息类型: 取值范围ALERT_TYPE_ENUM中定义 */
    char mtext[ALERT_MTEXT_MAX_LEN];    /* 消息内容 */
}alert_msq_t;

/******************************************************************************
 **函数名称: alert_pack_func_t
 **功    能: 组发送至远程服务的报文
 **输入参数: 
 **      type: 预警信息，其取值范围在ALERT_TYPE_ENUM中定义
 **      msg: 要被组包的预警信息
 **           1. 为平台异常信息时，msg的类型为alert_perrmsg_t或alert_oerrmsg_t
 **      pack: 报文缓冲区
 **      len: 报文缓冲区最大长度
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 ******************************************************************************/
typedef int (*alert_pack_func_t)(int type, const void *msg, char *pack, int len);

/* 远程服务配置 结构体 */
typedef struct _alert_remote_t
{
    int sckid;                          /* 通信SCK套接字 */
    alert_pack_func_t packfunc;         /* 组包函数指针 */
    void *handle;                       /* 动态库句柄 */
    
    /* 以下变量对应alert.xml中REMOTE配置 */
    int isuse;                          /* 开关:是否启动配置 取值为SWITCH_OFF或SWITCH_ON */
    int level;                          /* 预警级别: 必须满足的预警级别 */
    int port;                           /* 远程端口号 */
    long starttime;
    char ipaddr[IP_ADDR_MAX_LEN];       /* 远程IP地址 */
    char libname[LIB_NAME_MAX_LEN];     /* 动态库名 */
    char funcname[FUNC_NAME_MAX_LEN];   /* 函数名 */

    struct _alert_remote_t *next;
}alert_remote_t;

/* 预警配置信息 结构体 */
/* 注: 其对应alert.xml中的配置内容 */
typedef struct
{
    bool _switch;                       /* 预警开关 */
    int level;                          /* 平台预警级别 */
    alert_remote_t *remote;             /* 远程服务配置链表 */
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
