#ifndef __PUB_LOG_H__
#define __PUB_LOG_H__

#include "common.h"
#include "pub_file.h"
#include "pub_type.h"
#include "pub_alog.h"

#if defined(__ASYNC_LOG__)

#include "alog.h"

#endif

#define SW_LOG_MAX_MSG_STR	2048
#define SW_LOG_MAX_OPEN_RETRY	32
#define SW_LOG_MAX_FILE_LEN	128
#define SW_LOG_MSG_SEPERATE	"\n"
#define SW_LOG_SEPERATE_LEN	1

#if !defined(__ASYNC_LOG__)

#define SW_LOG_EMERG       0       /* system is unusable */
#define SW_LOG_ALERT       1       /* action must be taken immediately */
#define SW_LOG_CRIT        2       /* critical conditions */
#define SW_LOG_ERROR       3       /* error conditions */
#define SW_LOG_WARNING     4       /* warning conditions */
#define SW_LOG_NOTICE      5       /* normal but significant condition */
#define SW_LOG_INFO        6       /* informational */
#define SW_LOG_DEBUG       7       /* debug-level messages */
#define SW_LOG_ALL	   8       /*all information include DEBUG */
#define SW_LOG_PKG	   9       /*Detailed information */

#define SW_LOG_CHG_ERRFILE	'1'      /* change LOG_MODE      */
#define SW_LOG_CHG_DBGFILE	'2'
#define SW_LOG_CHG_MODE		'3'      /* change LOG_FILE_NAME */
#define SW_LOG_CHG_SIZE		'4'      /* change LOG_FILE_SIZE */
#define SW_LOG_CHG_MASK		'5'      /* change LOG_FILE_SIZE */
#endif /*__ASYNC_LOG__*/

#define LOG_DEFAULT_ERROR   "error.log" /* 默认错误日志文件 */
#define LOG_DEFAULT_DEBUG   "trc.log"   /* 默认调试日志文件 */
#define LOG_INVALID_FD      (-1)        /* 非法日志描述符 */
#define LOG_OPEN_FLAGS      (O_CREAT|O_WRONLY|O_APPEND) /* 日志文件权限 */
#define LOG_OPEN_MODE       (0666)      /* 日志文件权限 */
#define LOG_DIR_MODE        (0777)      /* 日志路径权限 */

extern sw_uint_t g_syslog_level;
extern sw_uint_t g_applog_level;

#if !defined(__ASYNC_LOG__)
/* 日志对象信息 */
typedef struct _log_cycle_t
{
    int fd;                             /* 文件描述符 */
    char path[FILE_NAME_MAX_LEN];       /* 日志文件路径 */
    pid_t pid;                          /* 进程ID */
    /* 日志对象的行为动作 */
    int (*action)(struct _log_cycle_t *log, int level, const char *msg, const char *addr, int len);
}log_cycle_t;


#if (SW_HAVE_C99_VARIADIC_MACROS)

#define SW_HAVE_VARIADIC_MACROS  1

void pub_log_error_core(sw_uint_t level, const char* addr, int len, const char *fmt, ...);
#define pub_log_error(...)   pub_log_error_core(SW_LOG_ERROR, NULL, 0, __VA_ARGS__); \
	pub_log_error_core(SW_LOG_INFO, NULL, 0, __VA_ARGS__);
#define pub_log_debug(...) pub_log_error_core(SW_LOG_DEBUG, NULL, 0, __VA_ARGS__)
#define pub_log_info(...) pub_log_error_core(SW_LOG_INFO, NULL, 0, __VA_ARGS__)
#define pub_log_begin(...)  pub_log_error_core(ALOG_BLOCK_BEGIN, NULL, 0, __VA_ARGS__)
#define pub_log_end(...)  pub_log_error_core(ALOG_END, NULL, 0, __VA_ARGS__)
#define pub_log_bend(...) pub_log_error_core(ALOG_BLOCK_END, NULL, 0, __VA_ARGS__)
#define pub_log_exit(...) pub_log_error_core(ALOG_BLOCK_EXIT, NULL, 0, __VA_ARGS__)
#define pub_log_bin(level, addr, len, ...)  pub_log_error_core(level, (addr), (len), __VA_ARGS__)
void pub_log_stderr(const char *fmt, ...);

#elif (SW_HAVE_GCC_VARIADIC_MACROS)

#define SW_HAVE_VARIADIC_MACROS  1

void pub_log_error_core(sw_uint_t level, const char* addr, int len, const char *fmt, ...);
#define pub_log_error(args...)  pub_log_error_core(SW_LOG_ERROR, NULL, 0, args);\
	pub_log_error_core(SW_LOG_INFO, NULL, 0, args);
#define pub_log_debug(args...) pub_log_error_core(SW_LOG_DEBUG, NULL, 0, args)
#define pub_log_info(args...) pub_log_error_core(SW_LOG_INFO, NULL, 0, args)
#define pub_log_begin(...)  pub_log_error_core(ALOG_BLOCK_BEGIN, NULL, 0, __VA_ARGS__)
#define pub_log_end(...)  pub_log_error_core(ALOG_END, NULL, 0, __VA_ARGS__)
#define pub_log_bend(...) pub_log_error_core(ALOG_BLOCK_END, NULL, 0, __VA_ARGS__)
#define pub_log_exit(...) pub_log_error_core(ALOG_BLOCK_EXIT, NULL, 0, __VA_ARGS__)
#define pub_log_bin(level, addr, len, args...) pub_log_error_core(level, (addr), (len), args)
void pub_log_stderr(const char *fmt, ...);

#else /*no variadic_macros*/

#define SW_HAVE_VARIADIC_MACROS  1

void pub_log_error_core(sw_uint_t level, const char* addr, int len, const char *fmt, ...);
#define pub_log_error(...)  pub_log_error_core(SW_LOG_ERROR, NULL, 0, __VA_ARGS__); \
	pub_log_error_core(SW_LOG_INFO, NULL, 0, __VA_ARGS__);
#define pub_log_debug(...)  pub_log_error_core(SW_LOG_DEBUG, NULL, 0, __VA_ARGS__)
#define pub_log_info(...)  pub_log_error_core(SW_LOG_INFO, NULL, 0, __VA_ARGS__)
#define pub_log_begin(...)  pub_log_error_core(ALOG_BLOCK_BEGIN, NULL, 0, __VA_ARGS__)
#define pub_log_end(...)  pub_log_error_core(ALOG_END, NULL, 0, __VA_ARGS__)
#define pub_log_bend(...) pub_log_error_core(ALOG_BLOCK_END, NULL, 0, __VA_ARGS__)
#define pub_log_exit(...) pub_log_error_core(ALOG_BLOCK_EXIT, NULL, 0, __VA_ARGS__)
#define pub_log_bin(level, addr, len, ...) pub_log_error_core(level, addr, len, __VA_ARGS__);
void pub_log_stderr(const char *fmt, ...);

#endif /*__ASYNC_LOG__*/
void pub_log_stderr(const char *fmt, ...);


#endif

sw_int_t pub_log_chglog(char type, const char *value);

#if !defined(__ASYNC_ULOG__)
/*============================================================================*
 * 模块: 业务日志模块
 * 作用: 负责打印业务日志 
 *
 * 作者: # Qifeng.zou # 2013.07.04 #
 *============================================================================*/
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
	char fpath[FILE_NAME_MAX_LEN];	/* 日志路径 */
	struct tm dtime;				/* 日志文件创建的日期 */
}ulog_cycle_t;

extern sw_int_t ulog_init(sw_int64_t trcno, const char *prdt_name, const char *svr_name, const char *svc_name, int idx);
extern void uLog(sw_int_t level, const char *fmt, ...);
extern void ulog_release(void);

#endif

sw_int_t pub_log_chglog(char type, const char *value);
void log_set_syslog_level(int level);
void log_set_applog_level(int level);
void log_set_size(size_t size);

#endif

