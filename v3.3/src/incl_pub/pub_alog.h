#ifndef __PUB_ALOG_H__
#define __PUB_ALOG_H__

#include <sys/timeb.h>
#include <sys/utsname.h>
#include "pub_type.h"

#define MAX_NAME_LEN 64
#define ALOG_MAX_FILE_NAME_LEN 128
#define ALOG_MAX_MSG_LEN 1024 * 2
#define ALOG_MAX_MSGHEAD_LEN 64
#define LOGSVR_TRANS_MAX_MSGLEN 1024 * 3
#define BP_LOG_RECONN 60
#define ALOG_MAX_MSG_NUM	2000
#define ALOG_SYSINFO_LEN	(6 + 1 + 12 + 1)
#define	DEFAULT_SHM_ALLOC_SIZE	(200 * 1024 * 1024)

typedef enum
{
	ALOG_BEGIN = 100,
	ALOG_BLOCK_BEGIN,
	ALOG_BLOCK_END,
	ALOG_END,
	ALOG_BLOCK_EXIT
} alog_block_t;

typedef enum
{
	STATUS_INIT = 1,
	STATUS_WRITED
} alog_status_t;


typedef struct
{
	int	id;
	int	mtype;
	int	oldmtype;
	int	cmdtype;
	char	chnl[MAX_NAME_LEN];
	char	sys_date[16];
	char	sys_time[16];
	char	sys_traceno[32];
	char	oldchnl[MAX_NAME_LEN];
} alog_sys_info_t;

typedef struct
{
	int	type;
	char	name[MAX_NAME_LEN];
} alog_module_info_t;
alog_module_info_t	g_module_info;

#define ALOG_MAX_CNT 10000
typedef struct
{
	int	use;
	int	cnt;
	char	sys_date[16];
	char	sys_traceno[16];
} alog_info_t;

typedef struct
{
	int	lockid;
	int	current;
	int	max_size;
	alog_info_t bit[ALOG_MAX_CNT];
} alog_bits_t;

typedef struct
{
	int	use;
	int	transport;
	int	port;
	int	wait_time;
	int     timeout;
	int     proc_num;
	int     pthread_num;
	size_t	buffer_size;
	size_t shm_alloc_size;
	char	ip[32];
	char	upath[ALOG_MAX_FILE_NAME_LEN];
	char	trc_prefix[8];
	alog_bits_t bits;
} alog_cfg_t;

typedef struct
{
	int	fd;
	int	err;
	int	port;
	int	type;
	int	is_init;
	int	wait_time;
	int	transport;
	pid_t	pid;
	size_t	buffer_size;
	char	ip[32];
	char	upath[ALOG_MAX_FILE_NAME_LEN];
	char	errstr[256];
	char	filename[ALOG_MAX_FILE_NAME_LEN];
	char	name[MAX_NAME_LEN];
	char	machine[MAX_NAME_LEN];
} alog_cycle_t;

alog_cfg_t	g_alog_cfg;
alog_sys_info_t	g_alog_sys_info;
alog_cycle_t	g_alog_cycle;

typedef int (*alog_connect_t)();

#define g_use_alog      (g_alog_cfg.use == 1)
#define g_alog_alloc_size g_alog_cycle.buffer_size
#define g_alog_fd       g_alog_cycle.fd
#define g_alog_ip       g_alog_cycle.ip
#define g_alog_port     g_alog_cycle.port
#define g_alog_pid      g_alog_cycle.pid
#define g_alog_is_init  g_alog_cycle.is_init
#define g_alog_wait_time g_alog_cycle.wait_time
#define g_in_alog       (g_use_alog && g_alog_is_init)
#define g_alog_filename	g_alog_cycle.filename
#define g_alog_name	g_alog_cycle.name
#define g_alog_type	g_alog_cycle.type
#define g_alog_machine  g_alog_cycle.machine
#define g_alog_trc_prefix g_alog_cfg.trc_prefix
#define g_alog_tcp	(g_alog_cfg.transport != SW_UNIX_TRANSPORT)

#define ALOG_DEFAULT_ALLOC_SIZE	1024*1024*8

typedef struct
{
	size_t	size;   /*** 日志缓冲区总大小 ***/
	size_t	offset; /*** 日志缓冲区偏移量 ***/
	int	nelts; /*** 日志条数,主要为了后续要查找日期跟流水的偏移量 ***/
	int	trace_offsets[ALOG_MAX_MSG_NUM]; /*** 每条日志的日期流水偏移量数组 ***/
	u_char	*ptr;   /*** 日志缓冲区首地址 ***/
} alog_cache_t;

typedef enum
{
	FD_ERROR = -1,
	FD_READABLE,
	FD_WRITEABLE
} fd_status_t;

extern int alog_set_sysinfo(int mtype, char *sys_date, sw_int64_t trace_no, char *chnl);
extern int alog_check_fd(int fd);
extern int alog_update_count();
extern int log_set_alog_cfg(alog_cfg_t alog);
extern int alog_set_bits(alog_cfg_t *alog);
extern int alog_bits_init(alog_cfg_t *alog);
extern int alog_get_count();
extern int alog_add_count(int count);

#endif

