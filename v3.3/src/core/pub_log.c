#include <pthread.h>
#include "common.h"
#include "pub_log.h"
#include "pub_mem.h"
#include "pub_time.h"
#include "pub_vars.h"
#include "pub_ares.h"

#if !defined(__ASYNC_LOG__)

/* ϵͳ��־���� */
sw_uint_t g_syslog_level = SW_LOG_INFO;
#define LogGetSyslogLevel() (g_syslog_level)
void log_set_syslog_level(int level) {g_syslog_level = level;}

/* ҵ����־���� */
sw_uint_t g_applog_level = SW_LOG_INFO;
#define LogGetApplogLevel() (g_applog_level)
void log_set_applog_level(int level) {g_applog_level = level;}

static size_t	g_log_max_size = 8192 * 1024;		/*max log file size */

int log_file_too_large(size_t size) {return size > g_log_max_size;}

void log_set_size(size_t size){g_log_max_size = size;}

pthread_key_t g_key_dbg;                            /* �߳��������� */
static sw_int_t	g_key_dbgflg = 0;                   /* ������־�ı�ʶ */
#define log_dbg_get_flag() (g_key_dbgflg)           /* ��ȡ���Ա�ʶ */
#define log_dbg_set_flag() (g_key_dbgflg = 1)       /* ���õ��Ա�ʶ */
#define log_dbg_reset_flag() (g_key_dbgflg = 0)     /* ���õ��Ա�ʶ */

#define log_wrlock(fd)      fbit_wrlock(fd, 0)      /* д�� */
#define log_unlock(fd)      fbit_unlock(fd, 0)      /* ���� */
#define ALOG_ID_MAX	    (2^31 - 1)
static void log_write_errmsg(log_cycle_t *log, sw_int_t level, const char *msg, const char *dump_addr, long dump_len);
static void log_write_mem_dump(sw_int_t fd, const unsigned char *addr, long len);

static int log_dbg_init(log_cycle_t *log, int level, const char *msg, const char *addr, int len);
static void log_dbg_release(log_cycle_t *log);
static int log_dbg_write(log_cycle_t *log, int level, const char *msg, const char *addr, int len);
static int log_err_init(log_cycle_t *log, int level, const char *msg, const char *addr, int len);
static int log_err_write(log_cycle_t *log, int level, const char * msg, const char * addr, int len);

/* Ĭ�ϵ��Ժʹ�����־��Ϣ */
static log_cycle_t g_default_dbglog = {LOG_INVALID_FD, LOG_DEFAULT_DEBUG, -1, log_dbg_init};
static log_cycle_t g_default_errlog = {LOG_INVALID_FD, LOG_DEFAULT_ERROR, -1, log_err_init};

int writelog(char *fmt, ...);

extern int alog_init(char *fpath);
extern int alog_write_cache(int fd);
extern int alog_core(sw_uint_t level, char *msg, const char *addr, int len);

/******************************************************************************
 **��������: log_init
 **��    ��: ��ʼ����־��Ϣ
 **�������: 
 **     syslog: ϵͳ��־����
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     1. ��ȡ���̺�
 **     2. ����־�ļ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
static int log_init(log_cycle_t *log)
{
	/* 1. ��ȡ���̺� */
	log->pid = getpid();

	/* 2. ����־�ļ� */
	log->fd = Open(log->path, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
	if(log->fd < 0)
	{
		pub_log_stderr("[%s][%d] errmsg:[%d]%s\n", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **��������: log_dbg_init
 **��    ��: ��ʼ����׫д��־
 **�������: 
 **     log: ��־����
 **     level: ��־����
 **     msg: ������־��Ϣ
 **     addr: ���ӡ���ڴ��ַ
 **     len: ��ӡ����
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
static int log_dbg_init(log_cycle_t *log, int level, const char *msg, const char *addr, int len)
{
	int ret = -1;

	/* 1. ��ʼ����־ģ�� */
	ret = log_init(log);
	if(SW_OK != ret)
	{
		log_dbg_release(log);
		pub_log_stderr("[%s][%d] Init log failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. �޸ĺ�����Ϊ */
	log->action = log_dbg_write;

	return log->action(log, level, msg, addr, len);
}

/******************************************************************************
 **��������: log_release
 **��    ��: �ͷ���־��Ϣ
 **�������: 
 **     syslog: ϵͳ��־����
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     1. ��ȡ���̺�
 **     2. ����־�ļ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
static void log_dbg_release(log_cycle_t *log)
{
	if(log->fd >= 0)
	{
		close(log->fd);
		log->fd = LOG_INVALID_FD;
	}
	log->action = log_dbg_init;
}

/******************************************************************************
 **��������: log_dbg_write
 **��    ��: д��־��Ϣ
 **�������: 
 **     log: ��־����
 **     level: ��־����
 **     msg: ������־��Ϣ
 **     addr: ���ӡ���ڴ��ַ
 **     len: ��ӡ����
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     1. �ж���־�ļ��Ƿ����
 **     2. ����Ϣд����־�ļ�
 **     3. �������������־�ļ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
static int log_dbg_write(log_cycle_t *log, int level, const char *msg, const char *addr, int len)
{
	int	ret = 0;
	char	fbackup[FILE_NAME_MAX_LEN];
	char	date_str[DATE_LEN];
	char	time_str[TIME_MAX_LEN];
	struct stat	buff;

	memset(&buff, 0, sizeof(buff));
	memset(fbackup, 0x0, sizeof(fbackup));
	memset(date_str, 0x0, sizeof(date_str));
	memset(time_str, 0x0, sizeof(time_str));

	do
	{
		/* 1. �ж��ļ��Ƿ���� */
		ret = lstat(log->path, &buff);
		if(0 != ret)
		{
			if(ENOENT != errno)
			{
				break;
			}

			log_dbg_release(log);

			return log_dbg_init(log, level, msg, addr, len);
		}

		if (!g_in_alog)
		{
			log_write_errmsg(log, level, msg, addr, len);
		}
		else
		{
			alog_write_cache(log->fd);
		}

		/* 3. ��������־�ļ� */
		if (log_file_too_large(buff.st_size))
		{
			log_dbg_release(log);

			pub_time_format("%02d%02d%02d", "%02d%02d%02d", NULL, date_str, time_str, 0);
			sprintf(fbackup,"%s%s%s.bak", log->path, date_str, time_str);

			ret = rename(log->path, fbackup);
			if (ret < 0) 
			{
				break;
			}

			return SW_OK;
		}

		return SW_OK;
	}while(0);

	log_dbg_release(log);
	return SW_ERR;
}

/******************************************************************************
 **��������: log_err_init
 **��    ��: ��ʼ��������־
 **�������: 
 **     log: ��־����
 **     msg: ������־��Ϣ
 **     addr: ���ӡ���ڴ��ַ
 **     len: ��ӡ����
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
static int log_err_init(log_cycle_t *log, int level, const char *msg, const char *addr, int len)
{
	log->pid = getpid();
	if('\0' == log->path[0])
	{
		snprintf(log->path, sizeof(log->path), "%s", LOG_DEFAULT_ERROR);
	}

	log->action = log_err_write;

	return log->action(log, level, msg, addr,len);
}

/******************************************************************************
 **��������: log_err_write
 **��    ��: д������־��Ϣ
 **�������: 
 **     log: ��־����
 **     msg: ������־��Ϣ
 **     addr: ���ӡ���ڴ��ַ
 **     len: ��ӡ����
 **�������: NONE
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
static int log_err_write(log_cycle_t *log, int level, const char *msg, const char *addr, int len)
{
	int	size = 0;
	int ret = 0;
	char fbackup[FILE_NAME_MAX_LEN] = {0},
	     date_str[DATE_LEN] = {0},
	     time_str[TIME_MAX_LEN] = {0};

	log->fd = Open(log->path, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
	if(log->fd < 0)
	{
		pub_log_stderr("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	lockf(log->fd, F_LOCK, 0);

	size = lseek(log->fd, 0, SEEK_END);
	if(-1 == size) 
	{
		close(log->fd);
		return SW_ERROR;
	}

	log_write_errmsg(log, level, msg, addr, len);

	close(log->fd);

	if (log_file_too_large(size))
	{		
		pub_time_format("%02d%02d%02d", "%02d%02d%02d", NULL, date_str, time_str, 0);
		sprintf(fbackup,"%s%s%s.bak", log->path, date_str, time_str);

		ret = rename(log->path, fbackup);
		if (ret < 0) 
		{
			return SW_ERR;
		}	
	}
	return SW_OK;
}

/******************************************************************************
 **��������: pub_log_error_core
 **��    ��: ��ӡ��־��Ϣ
 **�������: 
 **     level: ��־����
 **     addr: �ڴ��׵�ַ
 **     len: ��ӡ����
 **     fmt: ��ʽ�����
 **�������: NONE
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.16 #
 ******************************************************************************/
#if (SW_HAVE_VARIADIC_MACROS)
void pub_log_error_core(sw_uint_t level, const char* addr, int len, const char *fmt, ...)
#else
void pub_log_error_core(sw_uint_t level, const char* addr, int len, const char *fmt, va_list args)
#endif
{
#if (SW_HAVE_VARIADIC_MACROS)
	va_list args;
#endif
	log_cycle_t *log = NULL;
	sw_char_t msg[SW_LOG_MAX_MSG_STR] = {0};

	/* ��־����Ϸ�����֤ */
	if (!(level >= ALOG_BEGIN && level <= ALOG_END))
	{
		if (LogGetSyslogLevel() > 0)
		{
			if((int)LogGetSyslogLevel() < (int)level) 
			{
				return;
			}
		}
		else
		{
			log_set_syslog_level(SW_LOG_ERROR);
		}
	}

	/* ��ȡ��־��Ϣ */
#if (SW_HAVE_VARIADIC_MACROS)
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
#else /*SW_HAVE_VARIADIC_MACROS*/
	vsnprintf(msg, sizeof(msg), fmt, args);	
#endif /*SW_HAVE_VARIADIC_MACROS*/
	if (g_in_alog && level != SW_LOG_ERROR)
	{
		if (alog_core(level, msg, addr, len) == 0)
		{
			return ;
		}
	}

	switch (level)
	{
		case SW_LOG_ERROR:
			log = &g_default_errlog;
			break;
		default:
			if(log_dbg_get_flag())
			{
				log = (log_cycle_t *)pthread_getspecific(g_key_dbg);
				if(NULL == log)
				{
					log = &g_default_dbglog;
				}
			}
			else
			{
				log = &g_default_dbglog;
			}
			break;
	}

	log->action(log, level, msg, addr, len);
}

#define LOG_DUMP_COL_NUM		(16)		/* 16����: ÿ������ */
#define LOG_DUMP_LINE_MAX_SIZE	(512)		/* 16����: ÿ�д�С */
#define LOG_DUMP_PAGE_MAX_LINE	(20)		/* 16����: ÿҳ���� */
#define LOG_DUMP_PAGE_MAX_SIZE	(2048)		/* 16����: ÿҳ��С */
#define LOG_DUMP_HEAD_STR 	\
	"\nDisplacement -1--2--3--4--5--6--7--8-Hex-0--1--2--3--4--5--6  --ASCII Value--\n"

/* д16������־ͷ����Ϣ */
#define log_mem_dump_head(fd)	\
{	\
	write(fd, LOG_DUMP_HEAD_STR, strlen(LOG_DUMP_HEAD_STR));	\
}

int log_get_dump_size(int len)
{
	int	size = 0;

	size = len * 4 + 16 + (((len / LOG_DUMP_COL_NUM) / LOG_DUMP_PAGE_MAX_LINE) + 1) * strlen(LOG_DUMP_HEAD_STR);

	return size;
}

/******************************************************************************
 **��������: log_write_mem_dump
 **��    ��: ��16���ƴ�ӡ��־��Ϣ
 **�������: 
 **     fd: ��־�ļ�������
 **     addr: �ڴ��׵�ַ
 **     len: ��ӡ����
 **�������: NONE
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.06.19 #
 ******************************************************************************/
static void log_write_mem_dump(sw_int_t fd, const unsigned char *addr, long len)
{
	sw_int_t i = 0, lineno = 0, n = 0, count = 0, total_lines=0, page_length = 0;
	char *ptr = NULL;
	char line[LOG_DUMP_LINE_MAX_SIZE] = {0},
	     page[LOG_DUMP_PAGE_MAX_SIZE] = {0};
	unsigned char var[2] = {0, 31};	
	const unsigned char *dump = NULL;	


	dump = addr;
	total_lines = (len - 1)/LOG_DUMP_COL_NUM;

	while (dump < (addr + len)) 
	{		
		for (lineno=0; lineno<=total_lines; lineno++) 
		{
			ptr = line;
			memset(line, 0, sizeof(line));

			count = lineno * LOG_DUMP_COL_NUM;			

			sprintf(ptr, "%05d", count);
			ptr += 5;
			sprintf(ptr, "(%05x) ", count);		
			ptr += 8;

			for (i=0; (i<LOG_DUMP_COL_NUM) && (dump<(addr + len)); i++)
			{
				sprintf(ptr, "%02x ", *dump);
				ptr += 3;
				dump += 1;
			}		

			for (n=0; n<LOG_DUMP_COL_NUM-i; n++) 
			{
				sprintf(ptr, "   ");
				ptr += 3;
			}			

			sprintf(ptr, " ");			
			ptr += 1;
			dump = dump - i;			

			for (n=0; n<i; n++) 
			{
				if (*dump <= var[1] && *dump >= var[0]) 
				{				    
					sprintf(ptr, "*");
					ptr += 1;
				} 	
				else 
				{				    
					sprintf(ptr, "%c", *dump);
					ptr += 1;
				}				
				dump += 1;			
			}		

			strcat(page, line);	
			strcat(page, "\n");			
			page_length += (ptr-line+1);

			if (0 == (lineno + 1)%LOG_DUMP_PAGE_MAX_LINE)
			{
				log_mem_dump_head(fd);

				write(fd, page, page_length);
				memset(page, 0, sizeof(page));
				page_length = 0;
			}		
		} /* end of for    */	
	} /* end of while    */	

	log_mem_dump_head(fd);

	write(fd, page, page_length);
	return;
}

int log_alog_dump(u_char *dumpaddr, const unsigned char *addr, long len)
{
	sw_int_t i = 0, lineno = 0, n = 0, count = 0, total_lines=0, page_length = 0;
	int	dump_offset = 0;
	char *ptr = NULL;
	char line[LOG_DUMP_LINE_MAX_SIZE] = {0},
	     page[LOG_DUMP_PAGE_MAX_SIZE] = {0};
	unsigned char var[2] = {0, 31};	
	const unsigned char *dump = NULL;	

	dump_offset = 0;
	dump = addr;
	total_lines = (len - 1)/LOG_DUMP_COL_NUM;

	while (dump < (addr + len)) 
	{		
		for (lineno=0; lineno<=total_lines; lineno++) 
		{
			ptr = line;
			memset(line, 0, sizeof(line));

			count = lineno * LOG_DUMP_COL_NUM;			

			sprintf(ptr, "%05d", count);
			ptr += 5;
			sprintf(ptr, "(%05x) ", count);		
			ptr += 8;

			for (i=0; (i<LOG_DUMP_COL_NUM) && (dump<(addr + len)); i++)
			{
				sprintf(ptr, "%02x ", *dump);
				ptr += 3;
				dump += 1;
			}		

			for (n=0; n<LOG_DUMP_COL_NUM-i; n++) 
			{
				sprintf(ptr, "   ");
				ptr += 3;
			}			

			sprintf(ptr, " ");			
			ptr += 1;
			dump = dump - i;			

			for (n=0; n<i; n++) 
			{
				if (*dump <= var[1] && *dump >= var[0]) 
				{				    
					sprintf(ptr, "*");
					ptr += 1;
				} 	
				else 
				{				    
					sprintf(ptr, "%c", *dump);
					ptr += 1;
				}				
				dump += 1;			
			}		

			strcat(page, line);	
			strcat(page, "\n");			
			page_length += (ptr-line+1);

			if (0 == (lineno + 1)%LOG_DUMP_PAGE_MAX_LINE)
			{
				memcpy(dumpaddr + dump_offset, LOG_DUMP_HEAD_STR, strlen(LOG_DUMP_HEAD_STR));
				dump_offset += strlen(LOG_DUMP_HEAD_STR);

				memcpy(dumpaddr + dump_offset, page, page_length);
				dump_offset += page_length;
				memset(page, 0, sizeof(page));
				page_length = 0;
			}		
		} /* end of for    */	
	} /* end of while    */	

	memcpy(dumpaddr + dump_offset, LOG_DUMP_HEAD_STR, strlen(LOG_DUMP_HEAD_STR));
	dump_offset += strlen(LOG_DUMP_HEAD_STR);

	memcpy(dumpaddr + dump_offset, page, page_length);
	dump_offset += page_length;

	return dump_offset;
}

static void log_write_errmsg(log_cycle_t *log,
		sw_int_t level, const char *msg, const char *dump, long dump_len)
{
	char flag[2],
	     str_date[40],
	     errmsg[SW_LOG_MAX_MSG_STR];

	memset(flag, 0, sizeof(flag));
	memset(str_date, 0, sizeof(str_date));
	memset(errmsg, 0, sizeof(errmsg));

	switch(level)
	{
		case SW_LOG_INFO:
			memcpy(flag, "I", 1);
			break;
		case SW_LOG_WARNING:
			memcpy(flag, "W", 1);
			break;
		case SW_LOG_ERROR:
			memcpy(flag, "E", 1);
			break;
		case SW_LOG_DEBUG:
			memcpy(flag, "D", 1);
			break;
		default:
			memcpy(flag, "O", 1);
			break;
	}

	pub_time_gettime(NULL, "%d %d %d|%d:%d:%d", "%Y%m%d|%H:%M:%S", ".%.3ld",0, 17, str_date);

	snprintf(errmsg, sizeof(errmsg), "@%d|%.31s|%s %s\n", log->pid, str_date, flag, msg);	

	write(log->fd, errmsg, strlen(errmsg));

	if((NULL != dump) && (dump_len > 0)) 
	{		
		log_write_mem_dump(log->fd, (unsigned char *)dump, (long)dump_len);	
	}
	return;
}

/******************************************************************************
 **��������: pub_log_chglog
 **��    ��: ������־�ļ�
 **�������: 
 **     type: ����
 **     fpath: ��־·��
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     1. ��ʼ��������־��Ϣ
 **     2. ��ʼ��������־��Ϣ
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.15 #
 ******************************************************************************/
sw_int_t pub_log_chglog(char type, const char *fpath)
{
	sw_int_t ret = 0;
	log_cycle_t *dbglog = NULL;

	switch (type)	
	{	 		
		case SW_LOG_CHG_ERRFILE:
			snprintf(g_default_errlog.path, sizeof(g_default_errlog.path), "%s", fpath);
			return SW_OK;

		case SW_LOG_CHG_DBGFILE:
			if(!log_dbg_get_flag())
			{
				ret = pthread_key_create(&g_key_dbg, free);
				if(SW_OK != ret)
				{
					pub_log_stderr("[%s][%d] errmsg:[%d]%s\n", __FILE__, __LINE__, errno, strerror(errno));
					return SW_ERROR;
				}
				log_dbg_set_flag();
			}

			dbglog = calloc(1, sizeof(log_cycle_t));
			if(NULL == dbglog)
			{
				pub_log_stderr("[%s][%d] errmsg:[%d]%s\n", __FILE__, __LINE__, errno, strerror(errno));
				return SW_ERR;
			}

			snprintf(dbglog->path, sizeof(dbglog->path), "%s", fpath);
			dbglog->action = log_dbg_init;

			ret = pthread_setspecific(g_key_dbg, (void *)dbglog);
			if(ret != SW_OK)
			{
				pub_log_stderr("[%s][%d] errmsg:[%d]%s\n", __FILE__, __LINE__, errno, strerror(errno));
				free(dbglog);
				return SW_ERROR;
			}

			if (g_use_alog)
			{	
				if (!g_alog_is_init || g_alog_pid != getpid())
				{
					if (alog_init((char *)fpath) < 0)
					{
						pub_log_error("[%s][%d] Alog init error!",
								__FILE__, __LINE__);
						exit(1);
					}
				}
			}

			if (g_use_ares)
			{
				if (!g_ares_is_init || g_ares_pid != getpid())
				{
					if (ares_init() < 0)
					{
						pub_log_error("[%s][%d] ares init error!", __FILE__, __LINE__);
						exit(1);
					}
				}
			}

			return SW_OK;
	}	

	pub_log_stderr("change type invalid.");

	return SW_ERR;
}

int writelog(char *fmt, ...)
{
	FILE	*fp = NULL;
	char	tmp[32];
	char	date[32];
	char	fulltime[256];
	char	filename[512];
	char	buff[SW_LOG_MAX_MSG_STR];
	va_list	ap;
	struct tm	*pstm;
	struct timeb	tmb;
	int ret = 0;
	char fbackup[FILE_NAME_MAX_LEN] = {0},
	     date_str[DATE_LEN] = {0},
	     time_str[TIME_MAX_LEN] = {0};
	struct stat st;

	memset(tmp, 0x0, sizeof(tmp));
	memset(buff, 0x0, sizeof(buff));
	memset(date, 0x0, sizeof(date));
	memset(fulltime, 0x0, sizeof(fulltime));
	memset(filename, 0x0, sizeof(filename));

	va_start(ap, fmt);
	vsnprintf(buff, sizeof(buff) - 1, fmt, ap);
	va_end(ap);
	strcat(buff, "\n");

	ftime(&tmb);
	pstm = localtime(&tmb.time);
	strftime(tmp, 32, "%Y-%m-%d %H:%M:%S", pstm);
	sprintf(fulltime, "@%d|%s.%03d ", getpid(), tmp, tmb.millitm);

	strftime(date, 32, "%Y%m%d", pstm);
	
	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/log/syslog/trc_%s.log", getenv("SWWORK"), date);
	fp = fopen(filename, "a+");
	if (fp == NULL)
	{
		fprintf(stderr, "[%s][%d] ���ļ�[%s]ʧ��! errno=[%d]:[%s]\n",
				__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	fputs(fulltime, fp);
	fputs(buff, fp);
	fclose(fp);

	stat(filename, &st);

	if(log_file_too_large(st.st_size)) 
	{
		pub_time_format("%02d%02d%02d", "%02d%02d%02d", NULL, date_str, time_str, 0);
		sprintf(fbackup,"%s%s.bak", filename, time_str);

		ret = rename(filename, fbackup);
		if (ret < 0) 
		{
			return -1;
		}
	}
	return 0;
}

#endif /*__ASYNC_LOG__*/

/******************************************************************************
 ** Name : pub_log_stderr
 ** Desc : ��������Ϣ����
 ** Input: 
 **	    fmt: ��ʽ�����
 ** Output: NONE
 ** Return: void
 ** Process:
 ** Note :
 ** Author: # zhanghailu # 2013.xx.xx #
 ******************************************************************************/
#if (SW_HAVE_VARIADIC_MACROS)
void pub_log_stderr(const char *fmt, ...)
#else
void pub_log_stderr(const char *fmt, va_list args)
#endif
{
#if (SW_HAVE_VARIADIC_MACROS)
	va_list		args;
#endif
	sw_char_t	msg[SW_LOG_MAX_MSG_STR];

#if (SW_HAVE_VARIADIC_MACROS)
	va_start(args, fmt);
	vsnprintf(msg, SW_LOG_MAX_MSG_STR, fmt, args);
	va_end(args);
#else
	vsnprintf(msg, SW_LOG_MAX_MSG_STR, fmt, args);	
#endif

	write(pub_stderr, msg, strlen(msg));

	return ;
}

#if !defined(__ASYNC_ULOG__)
/*******************************************************************************
 * ģ��: ͬ��ҵ����־ģ��
 * ����: ����ҵ����־��ӡ��ָ�����ļ���
 * ˵��: 
 *      ��־Ŀ¼: ��Ʒ��+������
 *      ��־����: ������_YYYYMMDD_��������.log
 * ע��: 
 *      1. ���ǵ������ܵ�Ҫ��, ׫д��־�Ĺ����в�������, ��˲�ͬ�Ľ��̲�����
 *         ҵ����־��Ϣд��ͬһ���ļ���.
 *      2. ���л���־֮ǰ, ֻ����־�ļ�һ��
 * ����: # Qifeng.zou # 2013.07.04 #
 ******************************************************************************/
static ulog_cycle_t g_usr_log;  /* ҵ����־���� */
#define uLogGetCycle() (&g_usr_log)

static sw_int_t g_ulog_max_size = ULOG_MAX_SIZE;

/******************************************************************************
 ** Name : usr_log_init
 ** Desc : ��ʼ��ҵ����־ģ��
 ** Input: 
 **	    trcno: ��ˮ��
 **     prdt_name: ��Ʒ��
 **     svr_name: ������
 **     svc_name: SVC��
 **     idx: ��������IDX
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.04 #
 ******************************************************************************/
sw_int_t ulog_init(
		sw_int64_t trcno,
		const char *prdt_name, const char *svr_name, const char *svc_name, int idx)
{
	time_t ctime = 0;
	ulog_cycle_t *ulog = uLogGetCycle();

	if(ulog->fd > 0)
	{
		Close(ulog->fd);
	}

	memset(ulog, 0, sizeof(ulog_cycle_t));

	ctime = time(NULL);
	localtime_r(&ctime, &ulog->dtime);
	ulog->dtime.tm_year += 1900;
	ulog->dtime.tm_mon += 1;

	ulog->trcno = trcno;
	ulog->idx = idx;
	ulog->pid = getpid();
	snprintf(ulog->prdt_name, sizeof(ulog->prdt_name), "%s", prdt_name);
	snprintf(ulog->svr_name, sizeof(ulog->svr_name), "%s", svr_name);
	snprintf(ulog->svc_name, sizeof(ulog->svc_name), "%s", svc_name);

	snprintf(ulog->fpath, sizeof(ulog->fpath),
			"%s/log/%s/%s/%s_%04d%02d%02d_%d.log",
			getenv("SWWORK"), prdt_name, svr_name, svc_name,
			ulog->dtime.tm_year, ulog->dtime.tm_mon, ulog->dtime.tm_mday, ulog->idx);

	Mkdir2(ulog->fpath, LOG_DIR_MODE);

	ulog->fd = Open(ulog->fpath, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
	if(ulog->fd < 0)
	{
		pub_log_error("[%s][%d] Open file failed![%s]", __FILE__, __LINE__, ulog->fpath);
		return -1;
	}

	uLog(SW_LOG_INFO, "======================================= [%lld] BEGIN ====================================", ulog->trcno);

	return 0;
}

/******************************************************************************
 ** Name : ulog_check_and_switch
 ** Desc : �����л���־
 ** Input: 
 **	    level: ��־����
 **	    ulog: ҵ����־��Ϣ
 ** Output: NONE
 ** Return: VOID
 ** Process:
 **     1. �����־�ļ��Ƿ����
 **     2. �����־�ļ���С
 **     3. �����־�ļ�����
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.04 #
 ******************************************************************************/
sw_int_t ulog_check_and_switch(ulog_cycle_t *ulog)
{
	int ret = 0;
	struct tm dtime;
	struct stat buff;
	time_t ctime = 0;
	const char *swwork = NULL;
	char new_fname[FILE_NAME_MAX_LEN] = {0};

	memset(&dtime, 0, sizeof(dtime));
	memset(&buff, 0, sizeof(buff));

	/* 1. �����־�ļ��Ƿ���ڣ������ڣ��򴴽�֮ */
	ret = lstat(ulog->fpath, &buff);
	if(ret < 0)
	{
		/* �ļ���ɾ�������´��� */
		if(ENOENT != errno)
		{
			pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;            
		}

		Close(ulog->fd);
		Mkdir2(ulog->fpath, LOG_DIR_MODE);

		ulog->fd = Open(ulog->fpath, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
		if(ulog->fd < 0)
		{
			pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;            
		}
		return SW_OK;
	}

	/* 2. �����־�ļ���С */
	if(buff.st_size >= g_ulog_max_size)
	{
		Close(ulog->fd);

		ctime = time(NULL);
		localtime_r(&ctime, &dtime);
		dtime.tm_year += 1900;
		dtime.tm_mon += 1;

		snprintf(new_fname, sizeof(new_fname),
				"%s/log/%s/%s/%s_%04d%02d%02d%02d%02d%02d_%d.log",
				getenv("SWWORK"), ulog->prdt_name, ulog->svr_name, ulog->svc_name,
				dtime.tm_year, dtime.tm_mon, dtime.tm_mday,
				dtime.tm_hour, dtime.tm_min, dtime.tm_sec, ulog->idx);
		Rename(ulog->fpath, new_fname);

		/* �ı��ļ��Ĵ���ʱ�� �����ı��ļ��� */
		memcpy(&ulog->dtime, &dtime, sizeof(dtime));

		ulog->fd = Open(ulog->fpath, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
		if(ulog->fd < 0)
		{
			pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;            
		}
		return SW_OK;
	}

	/* 3. �����־�ļ����� */
	ctime = time(NULL);

	localtime_r(&ctime, &dtime);
	dtime.tm_year += 1900;
	dtime.tm_mon += 1;

	if((dtime.tm_year != ulog->dtime.tm_year)
			|| (dtime.tm_mon != ulog->dtime.tm_mon)
			|| (dtime.tm_mday != ulog->dtime.tm_mday))
	{
		swwork = getenv("SWWORK");
		Close(ulog->fd);

		snprintf(new_fname, sizeof(new_fname),
				"%s/log/%s/%s/%s_%04d%02d%02d%02d%02d%02d_%d.log",
				swwork, ulog->prdt_name, ulog->svr_name, ulog->svc_name,
				dtime.tm_year, dtime.tm_mon, dtime.tm_mday, 
				dtime.tm_hour, dtime.tm_min, dtime.tm_sec, ulog->idx);
		Rename(ulog->fpath, new_fname);

		/* �ı��ļ��Ĵ���ʱ����ļ��� */
		memcpy(&ulog->dtime, &dtime, sizeof(dtime));

		snprintf(ulog->fpath, sizeof(ulog->fpath),
				"%s/log/%s/%s/%s_%04d%02d%02d_%d.log",
				swwork, ulog->prdt_name, ulog->svr_name, ulog->svc_name,
				ulog->dtime.tm_year, ulog->dtime.tm_mon, ulog->dtime.tm_mday, ulog->idx);

		ulog->fd = Open(ulog->fpath, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
		if(ulog->fd < 0)
		{
			pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;            
		}

		return SW_OK;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : ulog_get_head
 ** Desc : ��ҵ����־ͷ����Ϣ
 ** Input: 
 **     ulog: ҵ����־����
 **	    level: ��־����
 **	    errmsg: ��־����
 **     size: �����С
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.04 #
 ******************************************************************************/
static int ulog_get_head(ulog_cycle_t *ulog, sw_fd_t level, char *errmsg, size_t size)
{
	sw_int_t len = 0;
	char str_date[DTIME_MAX_LEN] = {0};

	pub_time_gettime(NULL, "%d:%d:%d", "%H:%M:%S", ".%.3ld", 0, 8, str_date);

	switch(level)
	{
		case SW_LOG_INFO:
			{
				len = snprintf(errmsg, size, 
						"%lld|%.31s|I ", ulog->trcno, str_date);
				break;
			}
		case SW_LOG_WARNING:
			{
				len = snprintf(errmsg, size, 
						"%lld|%.31s|W ", ulog->trcno, str_date);		
				break;
			}
		case SW_LOG_ERROR:
			{
				len = snprintf(errmsg, size, 
						"%lld|%.31s|E ", ulog->trcno, str_date);	
				break;
			}
		case SW_LOG_DEBUG:
			{
				len = snprintf(errmsg, size, 
						"%lld|%.31s|D ", ulog->trcno, str_date);	
				break;
			}
		default:
			{
				len = snprintf(errmsg, size, 
						"%lld|%.31s|O ", ulog->trcno, str_date);	
				break;
			}
	}

	return len;
}

/******************************************************************************
 ** Name : uLog
 ** Desc : дҵ����־
 ** Input: 
 **	    level: ��־����
 **	    fmt: �����ʽ
 **	    ...: �ɱ����
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. �����־�ļ�ʱ�䡢��С��״̬
 **     2. д��־ͷ����Ϣ
 **     3. д��־ʵ������
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.04 #
 ******************************************************************************/
void uLog(sw_int_t level, const char *fmt, ...)
{
	int head_len = 0;
	va_list args;
	ulog_cycle_t *ulog = uLogGetCycle();
	sw_char_t errmsg[SW_LOG_MAX_MSG_STR] = {0};

	memset(&args, 0, sizeof(args));
	if(ulog->fpath[0] == '\0')
	{
		va_start(args, fmt);
		vsnprintf(errmsg, sizeof(errmsg), fmt, args);
		va_end(args);
		pub_log_error_core(level, NULL, 0, "%s", errmsg);
		return;
	}

	/* ��־����Ϸ�����֤ */
	if (!(level >= ALOG_BEGIN && level <= ALOG_END))
	{
		if (LogGetApplogLevel() > 0)
		{
			if((int)LogGetApplogLevel() < (int)level)
			{
				return;
			}
		}
		else
		{
			log_set_applog_level(SW_LOG_ERROR);
		}
	}

	head_len = 0;
	if (!g_in_alog)
	{
		/* 1. ������־�л� */
		ulog_check_and_switch(ulog);
	
		/* 2. ����־ͷ����Ϣ */
		head_len = ulog_get_head(ulog, level, errmsg, sizeof(errmsg));
	}

	/* 3. ����־ʵ����Ϣ */
	va_start(args, fmt);
	vsnprintf(errmsg + head_len, sizeof(errmsg) - head_len - 1, fmt, args);
	va_end(args);
	
	if (g_in_alog)
	{
		if (alog_core(level, errmsg, NULL, 0) == 0)
		{
			return ;
		}
	}
	strcat(errmsg, "\n");
	Writen(ulog->fd, errmsg, strlen(errmsg));

	return;
}

/******************************************************************************
 ** Name : ulog_release
 ** Desc : �ͷ���־��Ϣ
 ** Input: NONE
 ** Output: NONE
 ** Return: VOID
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.04 #
 ******************************************************************************/
void ulog_release(void)
{

	ulog_cycle_t *log = uLogGetCycle();

	uLog(SW_LOG_INFO, "======================================= [%lld] END ====================================", log->trcno);

	Close(log->fd);

	memset(log, 0, sizeof(ulog_cycle_t));
	return;
}
#endif /*__ASYNC_ULOG__*/
