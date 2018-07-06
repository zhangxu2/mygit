#ifndef __PUB_LOG_H__
#define __PUB_LOG_H__

#define SW_LOG_ERROR       3       /* error conditions */
#define SW_LOG_WARNING     4       /* warning conditions */
#define SW_LOG_NOTICE      5       /* normal but significant condition */
#define SW_LOG_INFO        6       /* informational */
#define SW_LOG_DEBUG       7       /* debug-level messages */
#define SW_LOG_ALL	   8       /*all information include DEBUG */
#define SW_LOG_PKG	   9       /*Detailed information */

#define LOG_MAX_MSG_LEN	2048

#if (SW_HAVE_C99_VARIADIC_MACROS)

#define pub_log_error(...)   pub_log_error_core(SW_LOG_ERROR, NULL, 0, __VA_ARGS__)
#define pub_log_debug(...) pub_log_error_core(SW_LOG_DEBUG, NULL, 0, __VA_ARGS__)
#define pub_log_info(...) pub_log_error_core(SW_LOG_INFO, NULL, 0, __VA_ARGS__)
#define pub_log_bin(level, addr, len, ...)  pub_log_error_core(level, (addr), (len), __VA_ARGS__)

#elif (SW_HAVE_GCC_VARIADIC_MACROS)

#define pub_log_error(args...)  pub_log_error_core(SW_LOG_ERROR, NULL, 0, args)
#define pub_log_debug(args...) pub_log_error_core(SW_LOG_DEBUG, NULL, 0, args)
#define pub_log_info(args...) pub_log_error_core(SW_LOG_INFO, NULL, 0, args)
#define pub_log_bin(level, addr, len, args...) pub_log_error_core(level, (addr), (len), args)

#else /*no variadic_macros*/

#define pub_log_error(...)  pub_log_error_core(SW_LOG_ERROR, NULL, 0, __VA_ARGS__)
#define pub_log_debug(...)  pub_log_error_core(SW_LOG_DEBUG, NULL, 0, __VA_ARGS__)
#define pub_log_info(...)  pub_log_error_core(SW_LOG_INFO, NULL, 0, __VA_ARGS__)
#define pub_log_bin(level, addr, len, ...) pub_log_error_core(level, addr, len, __VA_ARGS__);

#endif /*__ASYNC_LOG__*/

void pub_log_stderr(const char *fmt, ...);
int pub_log_chglog(char type, const char *value);
extern int ulog_init(int trcno, const char *prdt_name, const char *svr_name, const char *svc_name, int idx);
extern void uLog(int level, const char *fmt, ...);
extern void ulog_release(void);


#endif

