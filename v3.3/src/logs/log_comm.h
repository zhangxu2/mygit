#ifndef __LOG_COMM_H__
#define __LOG_COMM_H__

#include "dfs_plist.h"
#include "dfs_shmtx.h"
/*log level*/
/*#define SW_LOG_ERROR    1
#define SW_LOG_DEBUG    2
#define SW_LOG_WARNING  3
#define SW_LOG_INFO     4
#define SW_LOG_ALL      8*/

/*modules*/
#define ND_LSN  101
#define ND_SVC  500
#define ND_LSN_SND      102
#define ND_LSN_RCV      103

/*fd status*/
#define FD_INIT 0
#define FD_USED 1
#define FD_FREE 2

#define MAX_FD_CNT	100
#define MAX_NAME_LEN		64
#define MAX_FILE_NAME_LEN	128
#define LOG_MAX_MSG_LEN		(1024 * 8)
#define DATA_BUFFER_SIZE	(1024 * 2)
#define READ_BUFFER_INIT_SIZE   (1024 * 2)

/*lock file*/
#define	DFS_QUEUE_LOCK_FILE	".logsvr.queue.lock"
#define	DFS_SLAB_LOCK_FILE	".logsvr.slab.lock"
#define	DFS_CACHE_SLAB_LOCK_FILE	".cache.slab.lock"

/*read data result*/
enum try_read_result
{
	READ_DATA_RECEIVED,
	READ_NO_DATA_RECEIVED,
	READ_ERROR,            
	READ_MEMORY_ERROR      
};

/*block type*/
typedef enum
{
	LOG_BEGIN = 100,
	LOG_BLOCK_BEGIN,
	LOG_BLOCK_END,
	LOG_END
} log_block_t;

/*log status*/
typedef enum
{
	LOG_STATUS_INIT = 1,
	LOG_STATUS_WRITED
} log_status_t;

typedef struct
{	
	pthread_t	thread_tid;	
	long		thread_cnt;
} log_thread_t;

typedef struct
{
	int	use;
	int	fd;
	pid_t	pid;
} queue_fd_info_t;

typedef struct
{
	int 	type;
	char    name[MAX_NAME_LEN];
	char    filename[MAX_FILE_NAME_LEN];	
	queue_fd_info_t	fds[MAX_FD_CNT];
	dfs_plist_t *list;
} data_queue_item_t;

typedef struct
{
	int	type;
	char	name[MAX_NAME_LEN];
	char	filename[MAX_FILE_NAME_LEN];
} log_head_t;

typedef struct
{
	int			id;
	int     dumplen;
	int     type;
	int     level;
	int     msglen;
	int     mtype;
	int	oldmtype;
	log_block_t     block;
	log_status_t    status;
	dfs_int64_t tmb;
	char    pid[MAX_NAME_LEN];
	char    prdt[MAX_NAME_LEN];
	char    chnl[MAX_NAME_LEN];
	char	oldchnl[MAX_NAME_LEN];
	char    sys_date[16];
	char    sys_time[16];
	char    sys_traceno[32];
	char    svr[MAX_NAME_LEN];
	char    svc[MAX_NAME_LEN];
	char    bus_date[16];
	char    bus_time[16];
	char    bus_traceno[32];
	char	convtmb[32];
	char	machine[16];
	char    msg[LOG_MAX_MSG_LEN];
	char    *dumpaddr;
} log_body_t;

typedef struct
{
	int	fd;
	long	push_time;
	char	datetime[64];
	char	name[MAX_NAME_LEN]; /*** PROCESS NAME ***/
	char	filename[MAX_FILE_NAME_LEN]; /*** default log filename ***/
	int     level; /*** log level ***/
	int     mtype;
	int     msglen;
	char    pid[MAX_NAME_LEN];
	char    chnl[MAX_NAME_LEN];
	char    sys_date[16];
	char    sys_traceno[32];
	int	count;
	char	machine[16];
	char	*msg;
	log_block_t	block;
	log_status_t	status;
} log_context_t;

typedef struct
{
	int 	 sfd;
	char    *rbuf;   
	char    *rcurr;  
	int     rsize;   
	int     rbytes;  
	char    *wbuf;
	char    *wcurr;
	int     wsize;
	int     wbytes;
	dfs_connection_t *conn;
} log_request_t;

dfs_shmtx_t	g_logsvr_mutex;
typedef log_context_t data_item_t;
dfs_plist_t	*g_queue_data;
dfs_plist_t	*g_cache_queue;

sw_int_t create_work_thread();
sw_int_t create_single_thread(int i);
sw_int_t log_connection(sw_fd_list_t *fd_lists);
sw_int_t log_shm_init(sw_log_cycle_t *cycle);
sw_int_t log_child_init();

#endif

