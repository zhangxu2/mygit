#if !defined(__ALOG_H__)
#define __ALOG_H__

#if defined(__ASYNC_LOG__) || defined(__ASYNC_ULOG__)

#include "common.h"
#include "pub_type.h"
#include "thread_pool.h"

/* 功能宏开关 */
/* #define __ALOG_MULT_SHARE__ */ /* 支持多个进程/线程使用相同的文件缓存 */
/* #define __ALOG_ERR2TRC__ */
#define __ALOG_ERR_SYNC__       /* 出现错误日志时，强制同步对应缓存数据 */
#define __ALOG_SVR_SYNC__       /* 允许服务进程同时进行日志同步 */
/* #define __ALOG_TRCLOG_OPEN_ONCE__ */

/* 同步机制配置信息 */
#define ALOG_SHM_SIZE           (100*1024*1024) /* 日志共享内存大小 */
#define ALOG_FILE_CACHE_SIZE    (64*1024)       /* 文件缓存尺寸(单个) */
#define ALOG_SYNC_TIMEOUT       (3)             /* 同步超时时间 */
#if defined(__ALOG_MULT_SHARE__)
    #define ALOG_PID_MAX_NUM    (20)            /* 共用缓存的进程最大数 */
#endif /*__ALOG_MULT_SHARE__*/

#define ALOG_SHM_KEY            (0x32313123)    /* 共享内存KEY */

#define ALOG_OPEN_MODE          (0666)          /* 设置日志权限 */
#define ALOG_OPEN_FLAGS         (O_CREAT|O_WRONLY|O_APPEND) /* 打开日志标识 */
#define ALOG_DIR_MODE           (0777)          /* 设置目录权限 */

#define ALOG_MSG_MAX_LEN        (2048)          /* 日志最大长度(单条) */
#define ALOG_SVR_THREAD_NUM     (1)             /* 日志线程数目 */
#define ALOG_FILE_MAX_SIZE      (8*1024*1024)   /* 日志文件最大尺寸 */

#define ALOG_DEFAULT_ERRLOG     "error.log"     /* 默认错误日志 */
#define ALOG_DEFAULT_TRCLOG     "trc"       /* 默认跟踪日志 */
#define AlogGetDefErrlogPath(path, size)        /* 默认错误日志路径 */ \
    snprintf(path, size, "%s/log/syslog/%s", GetWorkDir(), ALOG_DEFAULT_ERRLOG);
#if defined(__ALOG_MULT_SHARE__)
#define AlogGetDefTrclogPath(path, size)        /* 默认跟踪日志路径 */ \
    snprintf(path, size, "%s/log/syslog/%s.log", GetWorkDir(), ALOG_DEFAULT_TRCLOG);
#else /*__ALOG_MULT_SHARE__*/
#define AlogGetDefTrclogPath(path, size)        /* 默认跟踪日志路径 */ \
    snprintf(path, size, "%s/log/syslog/%s.%d.log", GetWorkDir(), ALOG_DEFAULT_TRCLOG, getpid());
#endif /*__ALOG_MULT_SHARE__*/
#define ALOG_LOCK_FILE          ".alog.lck"     /* 锁文件名 */
#define AlogGetLockPath(path, size)             /* 锁文件件的路径 */ \
    snprintf(path, size, "%s/tmp/alog/%s", GetWorkDir(), ALOG_LOCK_FILE)

#define ALOG_INVALID_PID        (-1)            /* 非法进程ID */
#define ALOG_INVALID_FD         (-1)            /* 非法文件描述符 */

/* 打印指定内存的数据 */
#define ALOG_DUMP_COL_NUM       (16)            /* 16进制: 每行列数 */
#define ALOG_DUMP_LINE_MAX_SIZE    (512)           /* 16进制: 每行大小 */
#define ALOG_DUMP_PAGE_MAX_LINE    (20)            /* 16进制: 每页行数 */
#define ALOG_DUMP_PAGE_MAX_SIZE    (2048)          /* 16进制: 每页大小 */
#define ALOG_DUMP_HEAD_STR     \
    "\nDisplacement -1--2--3--4--5--6--7--8-Hex-0--1--2--3--4--5--6  --ASCII Value--\n"

#define AlogIsTimeout(diff_time) (diff_time >= ALOG_SYNC_TIMEOUT)

/* 日志级别 */
typedef enum
{
    LOG_LEVEL_DEBUG,                /* 调试级别 */
    LOG_LEVEL_INFO,                 /* 信息级别 */
    LOG_LEVEL_WARNING,              /* 警告级别 */
    LOG_LEVEL_ERROR,                /* 错误级别 */

    LOG_LEVEL_UNKNOWN,              /* 未知级别 */
    LOG_LEVEL_TOTAL
}alog_level_e;

/* 日志命令类型 */
typedef enum
{
    ALOG_CMD_INVALID,               /* 非法命令 */
    ALOG_CMD_SYNC,                  /* 同步命令 */

    ALOG_CMD_UNKNOWN,               /* 未知命令 */
    ALOG_CMD_TOTAL = ALOG_CMD_UNKNOWN /* 命令数目 */
}alog_cmd_e;

/* 日志参数信息 */
typedef enum
{
    ALOG_PARAM_ERR,                 /* 错误日志路径参数 */
    ALOG_PARAM_TRC,                 /* 跟踪日志路径参数 */
    ALOG_PARAM_SIZE,                /* 日志大小参数 */

    ALOG_PARAM_UNKNOWN,             /* 未知参数 */
    ALOG_PARAM_TOTAL = ALOG_PARAM_UNKNOWN
}alog_param_e;

/* 日志命令结构 */
typedef struct
{
    int idx;                        /* 缓存索引 */
    alog_cmd_e type;                /* 日志命令类型 */
}alog_cmd_t;

/* 文件缓存信息 */
typedef struct
{
    int idx;                        /* 索引号 */
    char path[FILE_NAME_MAX_LEN];   /* 日志文件绝对路径 */
    size_t in_offset;               /* 写入偏移 */
    size_t out_offset;              /* 同步偏移 */
#if defined(__ALOG_MULT_SHARE__)
    pid_t pid[ALOG_PID_MAX_NUM];    /* 使用日志缓存的进程ID 列表 */
#else /*__ALOG_MULT_SHARE__*/
    pid_t pid;                      /* 使用日志缓存的进程ID */
#endif /*__ALOG_MULT_SHARE__*/
    struct timeb sync_time;         /* 上次同步的时间 */
}alog_file_t;

typedef struct
{
    int mtype;                      /* 消息类型 */
    alog_cmd_t command;             /* 命令内容 */
}alog_msqbuf_t;

/* 日志生命周期 */
typedef struct _alog_cycle_t
{
    int fd;                         /* 日志描述符 */
    alog_file_t *file;              /* 文件缓存 */
    pid_t pid;                      /* 当前进程ID */
    int (*action)(struct _alog_cycle_t *cycle,  /* 日志行为 */
            int level, const char *fname, int lineno,
            const void *dump, int dumplen, const char *msg, const struct timeb *curr_time);
}alog_cycle_t;

/* 日志服务进程 */
typedef struct
{
    int fd;                         /* 文件锁FD */
    void *addr;                     /* 配置信息 */
    thread_pool_t *pool;            /* 线程池 */
}alog_svr_t;

/* 对外接口 */
extern void alog_set_level(alog_level_e level);
extern void alog_core(int level, const char *fname, int lineno, const void *dump, int dumplen, const char *fmt, ...);

#define log_debug(...) alog_core(LOG_LEVEL_DEBUG, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_info(...) alog_core(LOG_LEVEL_INFO, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_warn(...) alog_core(LOG_LEVEL_WARNING, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_error(...) alog_core(LOG_LEVEL_ERROR, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_binary(dump, dumplen, ...) alog_core(LOG_LEVEL_INFO, __FILE__, __LINE__, dump, dumplen, __VA_ARGS__)

/* 内部接口 */
extern unsigned int alog_cache_total(void);
extern int alog_trclog_sync(alog_file_t *file);
extern void alog_set_max_size(size_t size);

#if defined(__ASYNC_ULOG__)
/*******************************************************************************
 * 模块: 异步步业务日志模块
 * 作用: 负责将业务日志打印到指定的文件中
 * 说明: 
 *      日志目录: 产品名+服务名
 *      日志命名: 服务名_YYYYMMDD_进程索引.log
 * 注意: 
 *      1. 考虑到对性能的要求, 撰写日志的过程中并不加锁, 因此不同的进程不允许将
 *         业务日志信息写入同一个文件中.
 *      2. 在切换日志之前, 只打开日志文件一次
 * 作者: # Qifeng.zou # 2013.11.08 #
 ******************************************************************************/
 #define ULOG_MAX_SIZE   (8192 * 1024) /* 日志文件最大尺寸 */
 
/* 业务日志信息 */
typedef struct
{
    sw_fd_t fd;						/* 日志文件描述符 */
    int idx;                        /* 进程索引IDX */
    pid_t pid;                      /* 进程ID */
    sw_int64_t trcno;				/* 业务流水号 */
    char prdt_name[SW_NAME_LEN];	/* 产品名 */
    char svr_name[SW_NAME_LEN];		/* SVR名 */
    char svc_name[SW_NAME_LEN];		/* SVC名 */
    struct tm dtime;				/* 日志文件创建的日期 */
    alog_file_t *file;              /* 异步日志缓存 */
}ulog_cycle_t;

extern void ulog_set_level(alog_level_e level);
extern sw_int_t ulog_init(sw_int64_t trcno,
        const char *prdt_name, const char *svr_name, const char *svc_name, int idx);
extern void uLog(int level, const char *fmt, ...);
extern void ulog_release(void);
#endif /*__ASYNC_ULOG__*/

#if defined(__ALOG_MULT_SHARE__)
extern void alog_file_reset_pid(alog_file_t *file);
extern bool alog_file_is_pid_live(const alog_file_t *file);
#else /*__ALOG_MULT_SHARE__*/
#define alog_file_reset_pid(file) ((file)->pid = ALOG_INVALID_PID)
#endif /*__ALOG_MULT_SHARE__*/

#if 1 /* 接口适配 */

#define SW_LOG_ERROR LOG_LEVEL_ERROR
#define SW_LOG_DEBUG LOG_LEVEL_DEBUG
#define SW_LOG_WARNING LOG_LEVEL_WARNING
#define SW_LOG_INFO LOG_LEVEL_INFO

#define SW_LOG_CHG_ERRFILE ALOG_PARAM_ERR
#define SW_LOG_CHG_DBGFILE ALOG_PARAM_TRC

#define pub_log_chglog(type, path) alog_set_path(type, path)

#define pub_log_error(...) alog_core(LOG_LEVEL_ERROR, NULL, 0, NULL, 0, __VA_ARGS__)
#define pub_log_debug(...) alog_core(LOG_LEVEL_DEBUG, NULL, 0, NULL, 0, __VA_ARGS__)
#define pub_log_info(...) alog_core(LOG_LEVEL_INFO, NULL, 0, NULL, 0, __VA_ARGS__)
#define pub_log_bin(level, addr, len, ...) alog_core(level, NULL, 0, addr, len, __VA_ARGS__)

#define log_set_syslog_level(level) alog_set_level(level)
#define log_set_applog_level(level) ulog_set_level(level)
#endif

#endif /*__ASYNC_LOG__ || __ASYNC_ULOG__*/
#endif /*__ALOG_H__*/
