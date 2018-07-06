#if !defined(__ALOG_H__)
#define __ALOG_H__

#if defined(__ASYNC_LOG__) || defined(__ASYNC_ULOG__)

#include "common.h"
#include "pub_type.h"
#include "thread_pool.h"

/* ���ܺ꿪�� */
/* #define __ALOG_MULT_SHARE__ */ /* ֧�ֶ������/�߳�ʹ����ͬ���ļ����� */
/* #define __ALOG_ERR2TRC__ */
#define __ALOG_ERR_SYNC__       /* ���ִ�����־ʱ��ǿ��ͬ����Ӧ�������� */
#define __ALOG_SVR_SYNC__       /* ����������ͬʱ������־ͬ�� */
/* #define __ALOG_TRCLOG_OPEN_ONCE__ */

/* ͬ������������Ϣ */
#define ALOG_SHM_SIZE           (100*1024*1024) /* ��־�����ڴ��С */
#define ALOG_FILE_CACHE_SIZE    (64*1024)       /* �ļ�����ߴ�(����) */
#define ALOG_SYNC_TIMEOUT       (3)             /* ͬ����ʱʱ�� */
#if defined(__ALOG_MULT_SHARE__)
    #define ALOG_PID_MAX_NUM    (20)            /* ���û���Ľ�������� */
#endif /*__ALOG_MULT_SHARE__*/

#define ALOG_SHM_KEY            (0x32313123)    /* �����ڴ�KEY */

#define ALOG_OPEN_MODE          (0666)          /* ������־Ȩ�� */
#define ALOG_OPEN_FLAGS         (O_CREAT|O_WRONLY|O_APPEND) /* ����־��ʶ */
#define ALOG_DIR_MODE           (0777)          /* ����Ŀ¼Ȩ�� */

#define ALOG_MSG_MAX_LEN        (2048)          /* ��־��󳤶�(����) */
#define ALOG_SVR_THREAD_NUM     (1)             /* ��־�߳���Ŀ */
#define ALOG_FILE_MAX_SIZE      (8*1024*1024)   /* ��־�ļ����ߴ� */

#define ALOG_DEFAULT_ERRLOG     "error.log"     /* Ĭ�ϴ�����־ */
#define ALOG_DEFAULT_TRCLOG     "trc"       /* Ĭ�ϸ�����־ */
#define AlogGetDefErrlogPath(path, size)        /* Ĭ�ϴ�����־·�� */ \
    snprintf(path, size, "%s/log/syslog/%s", GetWorkDir(), ALOG_DEFAULT_ERRLOG);
#if defined(__ALOG_MULT_SHARE__)
#define AlogGetDefTrclogPath(path, size)        /* Ĭ�ϸ�����־·�� */ \
    snprintf(path, size, "%s/log/syslog/%s.log", GetWorkDir(), ALOG_DEFAULT_TRCLOG);
#else /*__ALOG_MULT_SHARE__*/
#define AlogGetDefTrclogPath(path, size)        /* Ĭ�ϸ�����־·�� */ \
    snprintf(path, size, "%s/log/syslog/%s.%d.log", GetWorkDir(), ALOG_DEFAULT_TRCLOG, getpid());
#endif /*__ALOG_MULT_SHARE__*/
#define ALOG_LOCK_FILE          ".alog.lck"     /* ���ļ��� */
#define AlogGetLockPath(path, size)             /* ���ļ�����·�� */ \
    snprintf(path, size, "%s/tmp/alog/%s", GetWorkDir(), ALOG_LOCK_FILE)

#define ALOG_INVALID_PID        (-1)            /* �Ƿ�����ID */
#define ALOG_INVALID_FD         (-1)            /* �Ƿ��ļ������� */

/* ��ӡָ���ڴ������ */
#define ALOG_DUMP_COL_NUM       (16)            /* 16����: ÿ������ */
#define ALOG_DUMP_LINE_MAX_SIZE    (512)           /* 16����: ÿ�д�С */
#define ALOG_DUMP_PAGE_MAX_LINE    (20)            /* 16����: ÿҳ���� */
#define ALOG_DUMP_PAGE_MAX_SIZE    (2048)          /* 16����: ÿҳ��С */
#define ALOG_DUMP_HEAD_STR     \
    "\nDisplacement -1--2--3--4--5--6--7--8-Hex-0--1--2--3--4--5--6  --ASCII Value--\n"

#define AlogIsTimeout(diff_time) (diff_time >= ALOG_SYNC_TIMEOUT)

/* ��־���� */
typedef enum
{
    LOG_LEVEL_DEBUG,                /* ���Լ��� */
    LOG_LEVEL_INFO,                 /* ��Ϣ���� */
    LOG_LEVEL_WARNING,              /* ���漶�� */
    LOG_LEVEL_ERROR,                /* ���󼶱� */

    LOG_LEVEL_UNKNOWN,              /* δ֪���� */
    LOG_LEVEL_TOTAL
}alog_level_e;

/* ��־�������� */
typedef enum
{
    ALOG_CMD_INVALID,               /* �Ƿ����� */
    ALOG_CMD_SYNC,                  /* ͬ������ */

    ALOG_CMD_UNKNOWN,               /* δ֪���� */
    ALOG_CMD_TOTAL = ALOG_CMD_UNKNOWN /* ������Ŀ */
}alog_cmd_e;

/* ��־������Ϣ */
typedef enum
{
    ALOG_PARAM_ERR,                 /* ������־·������ */
    ALOG_PARAM_TRC,                 /* ������־·������ */
    ALOG_PARAM_SIZE,                /* ��־��С���� */

    ALOG_PARAM_UNKNOWN,             /* δ֪���� */
    ALOG_PARAM_TOTAL = ALOG_PARAM_UNKNOWN
}alog_param_e;

/* ��־����ṹ */
typedef struct
{
    int idx;                        /* �������� */
    alog_cmd_e type;                /* ��־�������� */
}alog_cmd_t;

/* �ļ�������Ϣ */
typedef struct
{
    int idx;                        /* ������ */
    char path[FILE_NAME_MAX_LEN];   /* ��־�ļ�����·�� */
    size_t in_offset;               /* д��ƫ�� */
    size_t out_offset;              /* ͬ��ƫ�� */
#if defined(__ALOG_MULT_SHARE__)
    pid_t pid[ALOG_PID_MAX_NUM];    /* ʹ����־����Ľ���ID �б� */
#else /*__ALOG_MULT_SHARE__*/
    pid_t pid;                      /* ʹ����־����Ľ���ID */
#endif /*__ALOG_MULT_SHARE__*/
    struct timeb sync_time;         /* �ϴ�ͬ����ʱ�� */
}alog_file_t;

typedef struct
{
    int mtype;                      /* ��Ϣ���� */
    alog_cmd_t command;             /* �������� */
}alog_msqbuf_t;

/* ��־�������� */
typedef struct _alog_cycle_t
{
    int fd;                         /* ��־������ */
    alog_file_t *file;              /* �ļ����� */
    pid_t pid;                      /* ��ǰ����ID */
    int (*action)(struct _alog_cycle_t *cycle,  /* ��־��Ϊ */
            int level, const char *fname, int lineno,
            const void *dump, int dumplen, const char *msg, const struct timeb *curr_time);
}alog_cycle_t;

/* ��־������� */
typedef struct
{
    int fd;                         /* �ļ���FD */
    void *addr;                     /* ������Ϣ */
    thread_pool_t *pool;            /* �̳߳� */
}alog_svr_t;

/* ����ӿ� */
extern void alog_set_level(alog_level_e level);
extern void alog_core(int level, const char *fname, int lineno, const void *dump, int dumplen, const char *fmt, ...);

#define log_debug(...) alog_core(LOG_LEVEL_DEBUG, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_info(...) alog_core(LOG_LEVEL_INFO, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_warn(...) alog_core(LOG_LEVEL_WARNING, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_error(...) alog_core(LOG_LEVEL_ERROR, __FILE__, __LINE__, NULL, 0, __VA_ARGS__)
#define log_binary(dump, dumplen, ...) alog_core(LOG_LEVEL_INFO, __FILE__, __LINE__, dump, dumplen, __VA_ARGS__)

/* �ڲ��ӿ� */
extern unsigned int alog_cache_total(void);
extern int alog_trclog_sync(alog_file_t *file);
extern void alog_set_max_size(size_t size);

#if defined(__ASYNC_ULOG__)
/*******************************************************************************
 * ģ��: �첽��ҵ����־ģ��
 * ����: ����ҵ����־��ӡ��ָ�����ļ���
 * ˵��: 
 *      ��־Ŀ¼: ��Ʒ��+������
 *      ��־����: ������_YYYYMMDD_��������.log
 * ע��: 
 *      1. ���ǵ������ܵ�Ҫ��, ׫д��־�Ĺ����в�������, ��˲�ͬ�Ľ��̲�����
 *         ҵ����־��Ϣд��ͬһ���ļ���.
 *      2. ���л���־֮ǰ, ֻ����־�ļ�һ��
 * ����: # Qifeng.zou # 2013.11.08 #
 ******************************************************************************/
 #define ULOG_MAX_SIZE   (8192 * 1024) /* ��־�ļ����ߴ� */
 
/* ҵ����־��Ϣ */
typedef struct
{
    sw_fd_t fd;						/* ��־�ļ������� */
    int idx;                        /* ��������IDX */
    pid_t pid;                      /* ����ID */
    sw_int64_t trcno;				/* ҵ����ˮ�� */
    char prdt_name[SW_NAME_LEN];	/* ��Ʒ�� */
    char svr_name[SW_NAME_LEN];		/* SVR�� */
    char svc_name[SW_NAME_LEN];		/* SVC�� */
    struct tm dtime;				/* ��־�ļ����������� */
    alog_file_t *file;              /* �첽��־���� */
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

#if 1 /* �ӿ����� */

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
