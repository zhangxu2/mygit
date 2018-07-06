#include "dfs_plist.h"
#include "dfs_pool.h"
#include "dfs_sem.h"
#include "anet.h"
#include "log_pub.h"
#include "log_comm.h"

#define HEAD_LEN		8
#define SEP_FLAG		"|"
#define LOG_OPEN_MODE	        (0666)
#define DUMP_COL_NUM        	(16)
#define	DFS_MAX_TIMEOUT		(2 * 60 * 60)
#define LOG_MAX_BAK_SIZE	(8 * 1024 * 1024)
#define DUMP_LINE_MAX_SIZE  	(512)
#define DUMP_PAGE_MAX_LINE  	(20)
#define DUMP_PAGE_MAX_SIZE  	(2048)

extern sw_char_t g_logname[128];

static int log_thread_cnt = 0;
static log_thread_t *log_thread = NULL;
static dfs_plist_t	*g_slave_queue;
static int g_fds_lockid = 0;
static int g_fd_index_lockid = 0;
static pid_t g_pid = 0;
static int g_scantime = 30;
dfs_lock_t lock_addr;

pthread_cond_t	g_thread_cond	= PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_log_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()		pthread_mutex_lock(&g_thread_mutex)
#define	UNLOCK()	pthread_mutex_unlock(&g_thread_mutex)	
#define LOGLOCK()	pthread_mutex_lock(&g_log_thread_mutex)
#define	LOGUNLOCK()	pthread_mutex_unlock(&g_log_thread_mutex)	
#define FDLOCK()	dfs_lock_mutex_lock(g_fds_lockid);
#define	FDUNLOCK()	dfs_lock_mutex_unlock(g_fds_lockid)	
#define FDINDEXLOCK()	dfs_lock_mutex_lock(g_fd_index_lockid);
#define	FDINDEXUNLOCK()	dfs_lock_mutex_unlock(g_fd_index_lockid)	

#define DFS_ALLOC_FROM_POOL 'P'
#define DFS_ALLOC_FROM_HEAP 'H'

extern int log_file_too_large(size_t size);

static void print_request_info(log_context_t *context)
{
	char	msg[LOG_MAX_MSG_LEN];

	memset(msg, 0x0, sizeof(msg));

	sprintf(msg, "machine:[%s] pid:[%s] mtype:[%d] sys_date:[%s] sys_traceno:[%s]",
			context->machine, context->pid, context->mtype, context->sys_date, context->sys_traceno);
	pub_log_bin(SW_LOG_INFO, context->msg, context->msglen, "[%s][%d] %s", __FILE__, __LINE__, msg);
}

static int log_free_msg(dfs_pool_t *pool, log_context_t *context)
{
	if (context == NULL)
	{
		return SW_ERROR;
	}

	char	*ptr = context->msg - 1;

	if (*ptr == DFS_ALLOC_FROM_HEAP)
	{
		pub_log_info("[%s][%d] Free heap memory:[0x%x]", __FILE__, __LINE__, ptr);
		free(context->msg - 1);
	}
	else if (*ptr == DFS_ALLOC_FROM_POOL)
	{
		pub_log_info("[%s][%d] Free pool memory:[0x%x]", __FILE__, __LINE__, ptr);
		dfs_pfree(pool, context->msg - 1);
	}
	else
	{
		pub_log_error("[%s][%d] Memory alloc source [%x][%c] error!"
				__FILE__, __LINE__, ptr, *ptr);
		return SW_ERROR;
	}
	ptr = NULL;

	return SW_OK;
}

sw_int_t log_shm_init(sw_log_cycle_t *cycle)
{
	char	file[128];
	size_t	shm_alloc_size = 0;
	dfs_pool_t	*pool = NULL;

	memset(file, 0x0, sizeof(file));

	dfs_lock_init((char *)&lock_addr, 200);

	shm_alloc_size = g_alog_cfg.shm_alloc_size;

	g_scantime = g_alog_cfg.timeout;
	pub_log_info("[%s][%d] ....%d", __FILE__, __LINE__,g_scantime);
	pool = dfs_pool_create(shm_alloc_size, DFS_POOL_TYPE_SLAB);
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] create pool error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_queue_data = dfs_plist_create(pool, sizeof(data_queue_item_t));
	if (g_queue_data == NULL)
	{
		pub_log_error("[%s][%d] create mp data queue error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	pool = dfs_pool_create(shm_alloc_size, DFS_POOL_TYPE_SLAB);
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] create pool error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_slave_queue = dfs_plist_create(pool, sizeof(data_item_t));
	if (g_queue_data == NULL)
	{
		pub_log_error("[%s][%d] create mp data queue error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	memset(file, 0x00, sizeof(file));
	sprintf(file, "%s/tmp/%s", getenv("SWWORK"), DFS_QUEUE_LOCK_FILE);
	memset(&g_logsvr_mutex, 0x00, sizeof(g_logsvr_mutex));
	if (dfs_shmtx_create(&g_logsvr_mutex,(u_char *)file) != SW_OK) 
	{
		pub_log_error("[%s][%d] create logsvr shmtx error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	g_fds_lockid = dfs_lock_newid();
	if (g_fds_lockid < 0)
	{
		pub_log_error("[%s][%d] Get new lock error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_fd_index_lockid = dfs_lock_newid();
	if (g_fd_index_lockid < 0)
	{
		pub_log_error("[%s][%d] Get new lock error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] queue data create success.", __FILE__, __LINE__);
	return SW_OK;	
}

sw_int_t log_child_init()
{
	size_t	shm_cache_size = 0;
	dfs_pool_t *pool = NULL;

	shm_cache_size = g_alog_cfg.shm_alloc_size * 3 / 5;

	pool = dfs_pool_create(shm_cache_size, DFS_POOL_TYPE_SLAB);
	if (pool == NULL)
	{
		pub_log_error("[%s][%d] create pool error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	g_cache_queue = dfs_plist_create(pool, sizeof(log_context_t));
	if (g_cache_queue == NULL)
	{
		pub_log_error("[%s][%d] create mp data queue error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	g_pid = getpid();
	pub_log_info("[%s][%d] queue cache create success.", __FILE__, __LINE__);

	return SW_OK;
}

static int find_free_index(data_queue_item_t *queue)
{
	int	i = 0;

	FDINDEXLOCK();	
	for (i = 0; i < MAX_FD_CNT; i++)
	{
		if (queue->fds[i].use == FD_INIT)
		{
			FDINDEXUNLOCK();	
			return i;
		}
	}
	pub_log_error("[%s][%d] Not find free index!", __FILE__, __LINE__);

	FDINDEXUNLOCK();	
	return SW_ERROR;
}

static int find_queue_fd(data_queue_item_t *queue)
{
	int	i = 0;
	int	index = -1;

	if (access(queue->filename, F_OK) < 0)
	{
		for (i = 0; i < MAX_FD_CNT; i++)
		{
			if (queue->fds[i].fd > 0)
			{
				if (queue->fds[i].pid == g_pid)
				{
					close(queue->fds[i].fd);
					queue->fds[i].fd = -1;
				}
				queue->fds[i].use = FD_FREE;
			}
		}
	}

	for (i = 0; i < MAX_FD_CNT; i++)
	{
		if (queue->fds[i].pid == g_pid && queue->fds[i].use == FD_FREE)
		{
			if (queue->fds[i].fd > 0)
			{
				close(queue->fds[i].fd);
				queue->fds[i].fd = -1;
			}
			queue->fds[i].fd = open(queue->filename, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
			if (queue->fds[i].fd < 0)
			{
				pub_log_error("[%s][%d] Can not open [%s], errno=[%d]:[%s]",
						__FILE__, __LINE__, queue->filename, errno, strerror(errno));
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] open [%s] success! fd=[%d]",
					__FILE__, __LINE__, queue->filename, queue->fds[i].fd);
			queue->fds[i].use = FD_USED;
			index = i;

			break;
		}
		else if (queue->fds[i].pid == g_pid)
		{
			pub_log_info("[%s][%d] fds[%d].pid=[%u] g_pid=[%u] getpid=[%u]",
					__FILE__, __LINE__, i, queue->fds[i].pid, g_pid, getpid());
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		index = find_free_index(queue);
		if (index < 0)
		{
			pub_log_error("[%s][%d] Not find free index!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		queue->fds[index].fd = open(queue->filename, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
		if (queue->fds[index].fd <= 0)
		{
			pub_log_error("[%s][%d] errmsg:[%d]%s.",__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		queue->fds[index].pid = getpid();
		queue->fds[index].use = FD_USED;
		pub_log_info("[%s][%d] open [%s] success, index=[%d] fd=[%d]",
				__FILE__, __LINE__, queue->filename, index, queue->fds[index].fd);
	}

	return queue->fds[index].fd;
}

static int write_data_item(data_queue_item_t *queue, data_item_t *item, int fd)
{	
	ssize_t	size = 0;

	if (queue == NULL || item == NULL || fd < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	size = write(fd, item->msg, item->msglen);
	if (size != item->msglen)
	{
		pub_log_error("[%s][%d] write msg to file error, fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, fd, errno, strerror(errno));
		return SW_ERROR;
	}
	item->status = LOG_STATUS_WRITED;

	return SW_OK;
}


static int get_log_by_traceno_and_date(sw_cmd_t *cmd)
{
	int	fd   = 0;
	int flag = 0;
	ssize_t size = 0;
	char	traceno[32];
	char	fname[FILE_NAME_MAX_LEN];
	data_item_t	*pitem  = NULL;
	dfs_plist_iter_t	iter;
	dfs_plist_node_t	*node = NULL;
	data_queue_item_t	*queue = NULL;

	dfs_plist_rewind(g_queue_data, &iter);
	while ((node = dfs_plist_next(&iter)) != NULL)
	{
		queue = (data_queue_item_t *)(node->data);
		if (strcmp(queue->name, cmd->lsn_name) == 0)
		{
			break;
		}
	}
	if (NULL == node)
	{
		pub_log_error("[%s][%d] not find log of lsn_name[%s] traceno[%ld] date[%s]", 
				__FILE__, __LINE__, cmd->lsn_name, cmd->trace_no, cmd->sys_date);
		return SW_ERROR;
	}

	memset(fname, 0x0, sizeof(fname));
	sprintf(fname, "%s/dat/%s_%012lld.log", getenv("SWWORK"), cmd->sys_date, cmd->trace_no);
	fd = open(fname, LOG_OPEN_FLAGS, LOG_OPEN_MODE);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] open file[%s] error, errno=[%d]:[%s]", 
				__FILE__, __LINE__, fname, errno, strerror(errno));
		return SW_ERROR;
	}

	node = NULL;
	memset(&iter, 0x0, sizeof(iter));

	dfs_plist_rewind(queue->list, &iter);
	while ((node = dfs_plist_next(&iter)) != NULL)
	{
		pitem = (data_item_t *)node->data;
		memset(traceno, 0x0, sizeof(traceno));
		sprintf(traceno, "%012lld", cmd->trace_no);

		pub_log_debug("[%s][%d] pitem: sysdate[%s] traceno[%s]", __FILE__, __LINE__, pitem->sys_date, pitem->sys_traceno);
		pub_log_debug("[%s][%d]   cmd: sysdate[%s] traceno[%s]", __FILE__, __LINE__, cmd->sys_date, traceno);

		if (pitem->sys_date[0] != '\0' && pitem->sys_traceno[0] != '\0' &&
				(strcmp(pitem->sys_date, cmd->sys_date) != 0 || strcmp(pitem->sys_traceno, traceno) != 0))
		{
			continue;
		}

		flag = 1;
		pub_log_debug("[%s][%d] msglen[%d] flag=[%d]", __FILE__, __LINE__, pitem->msglen, flag);
		size = write(fd, pitem->msg, pitem->msglen);
		if (size != pitem->msglen)
		{
			pub_log_error("[%s][%d] write msg to file error, fd=[%d] errno=[%d]:[%s]",
					__FILE__, __LINE__, fd, errno, strerror(errno));
			close(fd);
			return SW_ERROR;
		}
	}
	close(fd);
	if (flag != 1)
	{
		pub_log_info("[%s][%d] not find log which date=[%s] traceno=[%s] lsn_name=[%s]", 
						__FILE__, __LINE__, cmd->sys_date, traceno, cmd->lsn_name);
		return -2;
	}
	pub_log_info("[%s][%d] write data into file ok.", __FILE__, __LINE__);

	return SW_OK;
}

int dfs_get_log(sw_cmd_t *cmd)
{
	int	ret = 0;

	dfs_shmtx_lock(&g_logsvr_mutex);
	ret = get_log_by_traceno_and_date(cmd);
	dfs_shmtx_unlock(&g_logsvr_mutex);

	return ret;
}

static int print_by_traceno_date(data_queue_item_t *queue, data_item_t *item)
{
	int	fd = 0;
	int	ret = 0;
	char	date_str[MAX_NAME_LEN];
	char	time_str[MAX_NAME_LEN];
	char	fname[FILE_NAME_MAX_LEN];
	struct stat	st;
	data_item_t	*pditem = NULL;
	dfs_plist_iter_t	diter;
	dfs_plist_node_t *dnode = NULL;

	memset(fname, 0x0, sizeof(fname));
	memset(date_str, 0x0, sizeof(date_str));
	memset(time_str, 0x0, sizeof(time_str));

	FDLOCK();
	fd = find_queue_fd(queue);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] find fd error.", __FILE__, __LINE__);
		FDUNLOCK();
		return SW_ERROR;
	}

	if (dfs_lock_fd(fd) < 0)
	{
		pub_log_error("[%s][%d] lock [%d] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
	}

	memset(&st, 0x0, sizeof(st));
	if (fstat(fd, &st) < 0)
	{
		pub_log_error("[%s][%d] Stat [%s][%d] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, queue->filename, fd, errno, strerror(errno));
		FDUNLOCK();
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] Print [%s][%s] begin...", __FILE__, __LINE__, item->sys_date, item->sys_traceno);
	memset(&diter, 0x00, sizeof(diter));
	dfs_plist_rewind(queue->list, &diter);
	while ((dnode = dfs_plist_next(&diter)) != NULL)
	{
		pditem = (data_item_t *)dnode->data;
		if (pditem->status == LOG_STATUS_WRITED)
		{
			pub_log_info("[%s][%d] sys_date=[%s] sys_traceno=[%s] WRITED!",
					__FILE__, __LINE__, pditem->sys_date, pditem->sys_traceno);
			continue;
		}

		if (pditem->sys_date[0] != '\0' && pditem->sys_traceno[0] != '\0' &&
				(strcmp(pditem->sys_date, item->sys_date) != 0 || strcmp(pditem->sys_traceno, item->sys_traceno) != 0))
		{
			continue;
		}

		if ((pditem->sys_date[0] == '\0' || pditem->sys_traceno[0] == '\0') && strcmp(pditem->pid, item->pid) != 0)
		{
			continue;
		}

		if (write_data_item(queue, pditem, fd) < 0)
		{
			log_free_msg(queue->list->pool, pditem);	
			pub_log_error("[%s][%d] write item to file error!",__FILE__, __LINE__);
			FDUNLOCK();
			return SW_ERROR;
		}
		log_free_msg(queue->list->pool, pditem);	
		dfs_plist_del_node(queue->list, dnode);
	}

	pub_log_info("[%s][%d] write date into file ok.", __FILE__, __LINE__);
	if (dfs_unlock_fd(fd) < 0)
	{
		pub_log_error("[%s][%d] Unlock [%d] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
	}

	if (log_file_too_large(st.st_size)) 
	{
		memset(fname, 0x0, sizeof(fname));
		memset(date_str, 0x0, sizeof(date_str));
		memset(time_str, 0x0, sizeof(time_str));
		pub_time_format("%02d%02d%02d", "%02d%02d%02d", NULL, date_str, time_str, 0);
		sprintf(fname, "%s%s%s.bak", queue->filename, date_str, time_str);
		ret = rename(queue->filename, fname);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Rename [%s] error! errno=[%d]:[%s]",
					__FILE__, __LINE__, queue->filename, errno, strerror(errno));
			FDUNLOCK();
			return SW_ERROR;
		}
	}

	FDUNLOCK();

	return SW_OK;
}


static int log_timeout_locked()
{
	long	now = 0;
	data_item_t	*pditem = NULL;
	dfs_plist_node_t	*node = NULL;
	dfs_plist_node_t	*dnode = NULL;
	dfs_plist_iter_t	iter, diter;
	data_queue_item_t	*pitem = NULL;
	log_context_t	*context = NULL;

	if (g_queue_data == NULL)
	{
		pub_log_error("[%s][%d] g_queue_data is null", __FILE__, __LINE__);
		return SW_ERROR;
	}

	now = (long)time(NULL);

	memset(&iter, 0x00, sizeof(iter));
	dfs_plist_rewind(g_queue_data, &iter);
	while ((node = dfs_plist_next(&iter)) != NULL)
	{
		pitem = (data_queue_item_t *)(node->data);
		pub_log_info("[%s][%d] ready to scan queue [%s]!", __FILE__, __LINE__, pitem->name);
		memset(&diter, 0x00, sizeof(diter));
		dfs_plist_rewind(pitem->list, &diter);
		while ((dnode = dfs_plist_next(&diter)) != NULL)
		{
			pditem = (data_item_t *)dnode->data;
			if (pditem == NULL)
			{
				break;
			}
			context = (log_context_t *)pditem;
			if (now - context->push_time > g_scantime)
			{
				pub_log_error("[%s][%d] mtype:[%d] sys_date:[%s] sys_traceno:[%s] timeout!",
						__FILE__, __LINE__, context->mtype, context->sys_date, context->sys_traceno); 
				print_by_traceno_date(pitem, context);	
			}
		}
	}

	return SW_OK;	
}

static int log_print_queue(dfs_plist_t *list)
{
	data_item_t	*pditem = NULL;
	dfs_plist_node_t	*dnode = NULL;
	dfs_plist_iter_t	diter;
	log_context_t	*context = NULL;

	pub_log_info("[%s][%d] list->size=[%d]", __FILE__, __LINE__, list->size);
	memset(&diter, 0x00, sizeof(diter));
	dfs_plist_rewind(list, &diter);
	while ((dnode = dfs_plist_next(&diter)) != NULL)
	{
		pditem = (data_item_t *)dnode->data;
		if (pditem == NULL)
		{
			break;
		}
		context = (log_context_t *)pditem;
		pub_log_error("[%s][%d] mtype:[%d] sys_date:[%s] sys_traceno:[%s]",
				__FILE__, __LINE__, context->mtype, context->sys_date, context->sys_traceno); 
	}

	return 0;
}



static int log_timeout_handling()
{
	dfs_shmtx_lock(&g_logsvr_mutex);
	log_timeout_locked();
	dfs_shmtx_unlock(&g_logsvr_mutex);

	return 0;
}

static char *log_alloc_msg(dfs_pool_t *pool, size_t size)
{
	char	*ptr = NULL;

	size = dfs_align(size, DFS_ALIGNMENT);
	if (pool == NULL || (ptr = dfs_pcalloc(pool, size)) == NULL)
	{
		ptr = (char *)calloc(1, size);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] Calloc error, size=[%d] errno=[%d]:[%s]",
					__FILE__, __LINE__, size, errno, strerror(errno));
			return NULL;
		}
		*ptr = DFS_ALLOC_FROM_HEAP;
		pub_log_info("[%s][%d] Heap alloc [0x%x][%d]",  __FILE__, __LINE__, ptr, size);
	}
	else
	{
		*ptr = DFS_ALLOC_FROM_POOL;
		pub_log_info("[%s][%d] Pool alloc [0x%x][%d]", __FILE__, __LINE__, ptr, size);
	}

	return ptr + 1;
}

static int datetime_cmp(data_item_t *data1, data_item_t *data2)
{
	if (strcmp(data1->datetime, data2->datetime) > 0)
	{
		return GREATER;
	}
	else if (strcmp(data1->datetime, data2->datetime) < 0)
	{
		return LESS;
	}
	else
	{
		return EQUAL;
	}
}

static int flow_match(data_item_t *data1, data_item_t *data2)
{
	if ((data1->level == data2->level) 
			&& (strcmp(data1->sys_date, data2->sys_date) == 0)
			&& (strcmp(data1->sys_traceno, data2->sys_traceno) == 0))
	{
		return 1;
	}

	return 0;
}

static dfs_plist_node_t *judge_log_in_slave(data_item_t *ditem)
{
	dfs_plist_iter_t	iter;
	data_item_t	*pditem = NULL;
	dfs_plist_node_t	*dnode = NULL;

	dfs_plist_rewind(g_slave_queue, &iter);
	while ((dnode = dfs_plist_next(&iter)) != NULL)
	{
		pditem = (data_item_t *)dnode->data;
		if (pditem == NULL)
		{
			break;
		}
		if (strcmp(pditem->sys_date, ditem->sys_date) == 0 &&
				strcmp(pditem->sys_traceno, ditem->sys_traceno) == 0)
		{
			return dnode;
		}
	}

	return NULL;
}

static int judge_log_is_over(data_queue_item_t *queue, data_item_t *ditem)
{
	int	cnt = 0;
	data_item_t	*pditem = NULL;
	dfs_plist_iter_t	iter;
	dfs_plist_node_t	*dnode = NULL;

	if (queue == NULL || ditem == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return 1;
	}

	if (ditem->sys_date[0] == '\0' || ditem->sys_traceno[0] == '\0')
	{
		return 1;
	}

	memset(&iter, 0x0, sizeof(iter));

	cnt = 0;
	dfs_plist_rewind(queue->list, &iter);
	while ((dnode = dfs_plist_next(&iter)) != NULL)
	{
		pditem = (data_item_t *)dnode->data;
		if (pditem == NULL)
		{
			break;
		}
		if (strcmp(pditem->sys_date, ditem->sys_date) == 0 &&
				strcmp(pditem->sys_traceno, ditem->sys_traceno) == 0)
		{
			cnt++;
		}
	}
	pub_log_info("[%s][%d] mtype:[%d] sys_date:[%s] sys_traceno:[%s] count:[%d][%d]",
			__FILE__, __LINE__, ditem->mtype, ditem->sys_date, ditem->sys_traceno, ditem->count, cnt); 

	if (cnt >= ditem->count)
	{
		return 1;
	}

	return 0;
}

static int log_push(log_context_t *c)
{ 
	char	tmp[128];
	char	head[128];
	char	name[128];
	char	filename[128];
	data_item_t	ditem;
	dfs_plist_iter_t iter;
	dfs_plist_node_t *node = NULL;
	data_queue_item_t	item;
	data_queue_item_t	*pitem = NULL;

	memset(name, 0x0, sizeof(name));
	memset(filename, 0x0, sizeof(filename));
	memset(tmp, 0x0, sizeof(tmp));
	memset(head, 0x0, sizeof(head));
	memset(&ditem, 0x00, sizeof(ditem));
	memset(&iter, 0x00, sizeof(iter));

	if (c->chnl[0] == '\0')
	{
		strncpy(name, c->name, sizeof(name) - 1);
		sprintf(filename, "%s/log/syslog/%s_lsn.log", getenv("SWWORK"), name);
	}
	else
	{
		strncpy(name, c->chnl, sizeof(name) - 1);
		sprintf(filename, "%s/log/syslog/%s_lsn.log", getenv("SWWORK"), c->chnl);
	}

	dfs_plist_rewind(g_queue_data, &iter);
	while ((node = dfs_plist_next(&iter)) != NULL)
	{
		pitem = (data_queue_item_t *)(node->data);
		if (strcmp(pitem->name, name) == 0)
		{
			break;
		}
	}
	if (node == NULL)
	{
		memset(&item, 0x0, sizeof(item));
		strncpy(item.name, name, sizeof(item.name) - 1);
		strncpy(item.filename, filename, sizeof(item.filename) - 1);
		item.list = dfs_plist_create(g_queue_data->pool, sizeof(data_item_t));
		if (item.list == NULL)
		{
			log_timeout_handling();
			pub_log_error("[%s][%d] deal timeout info, ready next alloc", __FILE__, __LINE__);
			item.list = dfs_plist_create(g_queue_data->pool, sizeof(data_item_t));
			if (item.list == NULL)
			{
				pub_log_error("[%s][%d] list_create error!", __FILE__, __LINE__);
				print_request_info(c);
				goto err;
			}
		}

		dfs_plist_set_match(item.list, (dfs_plist_match_pt)flow_match);
		dfs_plist_set_sort(item.list, (dfs_plist_sort_pt)datetime_cmp);
		if (dfs_plist_addnode_tail(g_queue_data, &item) < 0)
		{
			log_timeout_handling();
			pub_log_error("[%s][%d] deal timeout info, ready next alloc", __FILE__, __LINE__);
			if (dfs_plist_addnode_tail(g_queue_data, &item) < 0)
			{
				pub_log_error("[%s][%d] list_addnode error!", __FILE__, __LINE__);
				print_request_info(c);
				dfs_pfree(item.list->pool, item.list);
				goto err;
			}
		}
		pitem = &item;
	}

	memset(&ditem, 0x0, sizeof(ditem));
	memcpy(&ditem, c, sizeof(ditem));
	ditem.block = ditem.level;

	ditem.msg = (char *)log_alloc_msg(pitem->list->pool, c->msglen + 1);
	if (ditem.msg == NULL)
	{
		pub_log_error("[%s][%d] Alloc from pool error!", __FILE__, __LINE__);
		print_request_info(c);
		goto err;
	}
	memcpy(ditem.msg, c->msg, c->msglen);
	ditem.msglen = c->msglen;

	/*	pub_log_info("[%s][%d] msglen=[%d] msg=[%s]", __FILE__, __LINE__, ditem.msglen, ditem.msg);*/
	pub_log_info("[%s][%d] msglen=[%d]", __FILE__, __LINE__, ditem.msglen);

	ditem.push_time = (long)time(NULL);
	ditem.status = LOG_STATUS_INIT;
	if (dfs_plist_insert_sort(pitem->list, &ditem) < 0)
	{
		int	size = 0;
		size = dfs_plist_length(pitem->list);
		pub_log_info("[%s][%d]BEFORE...............size=[%d]", __FILE__, __LINE__, size);
		log_timeout_handling();
		size = dfs_plist_length(pitem->list);
		pub_log_info("[%s][%d]AFTER...............size=[%d]", __FILE__, __LINE__, size);
		pub_log_error("[%s][%d] deal timeout info, ready next alloc", __FILE__, __LINE__);
		if (dfs_plist_insert_sort(pitem->list, &ditem) < 0)
		{
			log_free_msg(pitem->list->pool, &ditem);
			pub_log_error("[%s][%d] list_addnode error!", __FILE__, __LINE__);
			print_request_info(c);
			goto err;
		}
	}

	if (ditem.block != LOG_END && ditem.sys_date[0] != '\0' && ditem.sys_traceno[0] != '\0')
	{
		pub_log_info("[%s][%d] PUSH ALOG_BLOCK_END sys_date=[%s] sys_traceno=[%s]",
				__FILE__, __LINE__, ditem.sys_date, ditem.sys_traceno);
		data_item_t	*pditem = NULL;
		dfs_plist_node_t	*pdnode = NULL;
		pdnode = judge_log_in_slave(&ditem);
		if (pdnode)
		{
			pub_log_info("[%s][%d] SLAVE ALOG_BLOCK_END sys_date=[%s] sys_traceno=[%s]",
					__FILE__, __LINE__, ditem.sys_date, ditem.sys_traceno);
			pditem = (data_item_t *)pdnode->data;
			if (judge_log_is_over(pitem, pditem))
			{
				print_by_traceno_date(pitem, &ditem);
				dfs_plist_del_node(g_slave_queue, pdnode);
				pub_log_info("[%s][%d] Good! sys_date=[%s] sys_traceno=[%s] over!",
						__FILE__, __LINE__, ditem.sys_date, ditem.sys_traceno);
			}
		}
	}

	if (ditem.block == LOG_END)
	{
		if (ditem.sys_date[0] != '\0' && ditem.sys_traceno[0] != '\0')
		{
			pub_log_info("[%s][%d] PUSH ALOG_END sys_date=[%s] sys_traceno=[%s]",
					__FILE__, __LINE__, ditem.sys_date, ditem.sys_traceno);
			if (!judge_log_is_over(pitem, &ditem))
			{
				pub_log_info("[%s][%d] sys_date=[%s] sys_traceno=[%s] not over!",
						__FILE__, __LINE__, ditem.sys_date, ditem.sys_traceno);
				if (dfs_plist_addnode_tail(g_slave_queue, c) == 0)
				{
					pub_log_info("[%s][%d] ADD TO SLAVE QUEUE! msglen=[%d] msg=[%s]",
							__FILE__, __LINE__, ditem.msglen, ditem.msg);
					goto ok;
				}
				pub_log_error("[%s][%d] Add to queue error!", __FILE__, __LINE__);
			}
		}
		print_by_traceno_date(pitem, &ditem);
	}

ok:
	pub_log_info("[%s][%d] push deal ok...", __FILE__, __LINE__);
	return SW_OK;

err:
	return SW_ERROR;
}

void *log_scan_thread(void *args)
{
	int	i = 0;
	int	result = 0;
	char	logpath[MAX_FILE_NAME_LEN];

	memset(logpath, 0x00, sizeof(logpath));
	sprintf(logpath, "%s/log/syslog/%s_%d.log", getenv("SWWORK"), g_logname, (int)args);
	pub_log_chglog(SW_LOG_CHG_DBGFILE, logpath);
	pub_log_info("[%s][%d] [%s] start......", __FILE__, __LINE__, __FUNCTION__);

	while (1)
	{
		for (i = 1; i < log_thread_cnt; i++)
		{
			result = pthread_kill(log_thread[i].thread_tid, 0);
			if (result == 0)
			{
				continue;
			}

			if (result == ESRCH)
			{
				pub_log_error("[%s][%d] thread[%d][%d]ËÀÍö, restart!",
						__FILE__, __LINE__, i, log_thread[i].thread_tid);
				result = create_single_thread(i);
				if (result < 0)
				{
					pub_log_info("[%s][%d] restart thread error.", __FILE__, __LINE__);
					return NULL;
				}
				pub_log_info("[%s][%d] thread[%d]restart success!",
						__FILE__, __LINE__, log_thread[i].thread_tid);
			}
		}

		log_timeout_handling();
		sleep(g_scantime);
	}
}

void *log_process(void *args)
{
	int	ret = 0;
	char	logpath[MAX_FILE_NAME_LEN];
	void	*node = NULL;
	log_context_t	*context = NULL;
	log_context_t	logcontext;

	memset(logpath, 0x0, sizeof(logpath));
	sprintf(logpath, "%s/log/syslog/%s_%d.log", getenv("SWWORK"), g_logname, (int)args);
	pub_log_chglog(SW_LOG_CHG_DBGFILE, logpath);

	pub_log_info("[%s][%d] [%s] start........", __FILE__, __LINE__, __FUNCTION__);
	while (1)
	{
		LOCK();
		while (g_cache_queue->size == 0)
		{
			pthread_cond_wait(&g_thread_cond, &g_thread_mutex);
		}

		node = dfs_plist_pop_head(g_cache_queue);
		if (node == NULL)
		{
			UNLOCK();
			continue;
		}

		context =(log_context_t *)node;
		if (context->level == LOG_BLOCK_END && context->sys_date[0] != '\0')
		{
			pub_log_info("[%s][%d] LOG_PROCESSALOG_BLOCK_END sys_date=[%s] sys_traceno=[%s]",
					__FILE__, __LINE__, context->sys_date, context->sys_traceno);
		}
		if (context->level == LOG_END && context->sys_date[0] != '\0')
		{
			pub_log_info("[%s][%d] LOG_PROCESSALOG_END sys_date=[%s] sys_traceno=[%s]",
					__FILE__, __LINE__, context->sys_date, context->sys_traceno);
		}
		memset(&logcontext, 0x00, sizeof(logcontext));
		memcpy(&logcontext, context, sizeof(log_context_t));
		errno = 0;
		logcontext.msg = calloc(1, context->msglen);
		if (logcontext.msg == NULL)
		{
			pub_log_error("[%s][%d] Alloc error! size=[%d] errno=[%d]:[%s]",
					__FILE__, __LINE__, context->msglen, errno, strerror(errno));
			log_free_msg(g_cache_queue->pool, context);
			dfs_pfree(g_cache_queue->pool, node);
			node = NULL;
			UNLOCK();
			continue;
		}
		memcpy(logcontext.msg, context->msg, logcontext.msglen);
		log_free_msg(g_cache_queue->pool, context);
		dfs_pfree(g_cache_queue->pool, node);
		node = NULL;
		UNLOCK();

		LOGLOCK();
		dfs_shmtx_lock(&g_logsvr_mutex);
		ret = log_push(&logcontext);
		dfs_shmtx_unlock(&g_logsvr_mutex);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] push data error.", __FILE__, __LINE__);	
			print_request_info(&logcontext);
		}
		free(logcontext.msg);
		logcontext.msg = NULL;
		LOGUNLOCK();
	}
}

sw_int_t create_single_thread(int i)
{
	int	ret = 0;
	pthread_attr_t	threadattr;

	ret = pthread_attr_init(&threadattr);
	if (ret)
	{
		errno = ret;
		pub_log_error("%s,%d,pthread_attr_init error,errno=%d",__FILE__,__LINE__,errno);
		exit(1);
	}

	ret = pthread_attr_setstacksize(&threadattr, 0x800000);
	if (ret)
	{
		errno = ret;
		pub_log_error("%s,%d,pthread_attr_setstacksize,error,errno=%d",__FILE__,__LINE__,errno);
		exit(1);
	}

	ret = pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);
	if (ret)
	{
		errno = ret;
		pub_log_error("%s,%d,pthread_attr_setdetachstate error,errno=%d",__FILE__,__LINE__,errno);
		exit(1);
	}

	if (i == 0)
	{
		ret = pthread_create(&log_thread[i].thread_tid, &threadattr, &log_scan_thread, (int *)i);
		if (ret) 
		{
			errno = ret;
			pub_log_error("%s,%d,pthread_create error,errno=%d",__FILE__,__LINE__,errno);
			pthread_attr_destroy(&threadattr);
			exit(1);
		}
	}
	else
	{
		ret = pthread_create(&log_thread[i].thread_tid, &threadattr, &log_process, (int *)i);
		if (ret) 
		{
			errno = ret;
			pub_log_error("%s,%d,pthread_create error,errno=%d",__FILE__,__LINE__,errno);
			pthread_attr_destroy(&threadattr);
			exit(1);
		}
	}
	pthread_detach(log_thread[i].thread_tid);
	pthread_attr_destroy(&threadattr);
	pub_log_info("[%s][%d] Make [%d]th thread[%u] Ok.",
			__FILE__, __LINE__, i + 1, (long)log_thread[i].thread_tid);

	return SW_OK;

}

static int get_value_by_sep(char *pkg_buf, int num, char *sep, char *value)
{
	int	cnt = 0;
	int	sep_len = 0;
	size_t	var_len = 0;
	char	*p = value;
	char	*ptr = pkg_buf;
	char	*dst = value;
	unsigned char	ch1;
	unsigned char	ch2;

	if (num <= 0)
	{
		num = 1;
	}
	sep_len = strlen(sep);
	var_len = 0;
	while (cnt < num && strlen(pkg_buf) > var_len)
	{

		ch1 = (unsigned char)*ptr;
		ch2 = (unsigned char)*(ptr + 1);
		if ((ch1 >= 0xB0 && ch1 <= 0xF7 && ch2 >= 0xA0 && ch2 <= 0xFE) ||
				(ch1 >= 0x81 && ch1 <= 0xFE && ch2 >= 0x40 && ch2 <= 0xFE))
		{
			/*** ºº×Ö ***/
			*p++ = *ptr++;
			*p++ = *ptr++;
			var_len += 2;
			continue;
		}

		if (memcmp(ptr, sep, sep_len) == 0)
		{
			if (cnt == num - 1)
			{
				*p = '\0';
				return var_len;
			}
			ptr += sep_len;
			cnt++;
			memset(dst, 0x0, var_len);
			p = dst;
			var_len = 0;
			continue;
		}
		*p++ = *ptr++;
		var_len++;
	}
	if (cnt == num - 1)
	{
		*p = '\0';
		return var_len;
	}
	value[0] = '\0';

	return 0;
}


static int pkg_to_struct(dfs_char_t *buf, log_context_t *context)
{
	int	varlen = 0;
	int	pos = 0;
	char	tmp[128];

	if (buf == NULL || context == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	/***
	  datatime|pid|name|filename|level|mtype|chnl|sys_date|sys_traceno|machine|msglen|
	 ***/
	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->datetime, tmp, sizeof(context->datetime) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->pid, tmp, sizeof(context->pid) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->name, tmp, sizeof(context->name) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->filename, tmp, sizeof(context->filename) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	context->level = atoi(tmp);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	context->mtype = atoi(tmp);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->chnl, tmp, sizeof(context->chnl) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->sys_date, tmp, sizeof(context->sys_date) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->sys_traceno, tmp, sizeof(context->sys_traceno) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	context->count = atoi(tmp);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	strncpy(context->machine, tmp, sizeof(context->machine) - 1);
	pos += varlen + 1;

	memset(tmp, 0x0, sizeof(tmp));
	varlen = get_value_by_sep(buf + pos, 1, SEP_FLAG, tmp);
	context->msglen = atoi(tmp);
	pos += varlen + 1;
	pub_log_info("[%s][%d] msglen:[%s][%d]", __FILE__, __LINE__, tmp, context->msglen);

	return 0;
}

static int log_add_to_cache(log_context_t *context)
{
	int	ret = 0;

	LOCK();
	ret = dfs_plist_addnode_tail(g_cache_queue, context);
	UNLOCK();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Add to cache queue error!",
				__FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

static int log_recv_ext(int fd, int len, char *req)
{
	int	err = 0;
	int	ret = 0;

	if (fd <= 0 || len <= 0)
	{
		pub_log_error("[%s][%d] input argvment error.", __FILE__,__LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] fd[%d] len[%d]", __FILE__, __LINE__, fd, len);
	ret = anet_read(fd, req, len);
	pub_log_info("[%s][%d]  ret[%d]", __FILE__, __LINE__, ret);
	if (ret == 0)
	{
		pub_log_error("[%s][%d] read peer closed!", __FILE__, __LINE__);
		return READ_ERROR;
	}
	else if (ret != len)
	{
		pub_log_error("[%s][%d] Read error, ret=[%d][%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, ret, len, err, strerror(err));
		return READ_ERROR;
	}

	return ret;
}

static sw_int_t log_rcv_msg(sw_fd_list_t *fd_lists)
{
	int	i = 0;
	int	fd = 0;
	int	cnt = 0;
	int	ret = 0;
	int	size = 0;
	char	buf[1024*10];
	log_context_t	context;

	if (g_pid <= 0)
	{
		g_pid = getpid();
	}

	fd = fd_lists->fd;
	pub_log_info("[%s][%d]the log accept_fd[%d]", __FILE__, __LINE__, fd);

	memset(buf, 0x0, sizeof(buf));
	size = log_recv_ext(fd, HEAD_LEN, buf);
	if (size != HEAD_LEN)
	{
		pub_log_info("[%s][%d] Recv head error, size=[%d][%s]",
				__FILE__, __LINE__, size, buf);
		return size;
	}
	pub_log_info("[%s][%d]AAAA buf[%s]", __FILE__, __LINE__, buf);

	while (buf[i] != '\0')
	{
		if (!(buf[i] >= '0' && buf[i] <= '9'))
		{
			pub_log_error("[%s][%d] Read head buffer error, buf=[%s]",
					__FILE__, __LINE__, buf);
			return SW_ERROR;
		}
		i++;
	}
	size = atoi(buf);

	pub_log_info("[%s][%d]the log len[%d]", __FILE__, __LINE__, size);

	memset(buf, 0x00, sizeof(buf));
	ret = log_recv_ext(fd, size, buf);
	if (ret != size)
	{
		pub_log_error("[%s][%d] Recv error! ret=[%d] size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, ret, size, errno, strerror(errno));
		return ret;
	}

	pub_log_info("[%s][%d]the log FD=[%d] len[%d]buf[%s]", __FILE__, __LINE__, fd, size, buf);

	memset(&context, 0x0, sizeof(context));
	ret = pkg_to_struct(buf, &context);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pkg to struct error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (context.level == LOG_BLOCK_END && context.sys_date[0] != '\0')
	{
		pub_log_info("[%s][%d] ALOG_BLOCK_END sys_date=[%s] sys_traceno=[%s]",
				__FILE__, __LINE__, context.sys_date, context.sys_traceno);
	}

	if (context.level == LOG_END && context.sys_date[0] != '\0')
	{
		pub_log_info("[%s][%d] ALOG_END sys_date=[%s] sys_traceno=[%s]",
				__FILE__, __LINE__, context.sys_date, context.sys_traceno);
	}

	if (context.msglen <= 0)
	{
		pub_log_error("[%s][%d] Recv msg [%s] len <= 0!", __FILE__, __LINE__, buf);
		return SW_ERROR;
	}

	context.msg = log_alloc_msg(g_cache_queue->pool, context.msglen + 1);
	if (context.msg == NULL)
	{
		pub_log_error("[%s][%d] Alloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = log_recv_ext(fd, context.msglen, context.msg);
	if (ret != context.msglen)
	{
		pub_log_error("[%s][%d] Recv error! ret=[%d] size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, ret, context.msglen, errno, strerror(errno));	
		log_free_msg(g_cache_queue->pool, &context);

		return SW_ERROR;
	}

	pub_log_info("[%s][%d]the log msg", __FILE__, __LINE__);


	ret = log_add_to_cache(&context);
	if (ret < 0)
	{
		cnt = 0;
		while (1)
		{
			if (cnt >= 3)
			{
				print_request_info(&context);
				log_free_msg(g_cache_queue->pool, &context);
				log_print_queue(g_cache_queue);
				pub_log_error("[%s][%d] Retry 3 times, already add failed!",
						__FILE__, __LINE__);

				return SW_ERROR;
			}
			sleep(1);
			if (log_add_to_cache(&context) < 0)
			{
				cnt++;
				continue;
			}
			pub_log_info("[%s][%d] readd success.", __FILE__, __LINE__);
			break;
		}
	}
	pthread_cond_signal(&g_thread_cond);

	pub_log_info("[%s][%d] mp recv ok.........", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t log_connection(sw_fd_list_t *fd_lists)
{
	int	fd = 0;
	int	ret = 0;
	int	acceptfd = 0;
	char	errs[256];
	sw_fd_list_t	fd_list;
	sw_log_cycle_t	*cycle = NULL;

	fd = fd_lists->fd;

	memset(errs, 0x0, sizeof(errs));
	acceptfd = anet_unix_accept(errs, fd);
	if (acceptfd < 0 && errno == EAGAIN)
	{
		return SW_OK;
	}
	else if (acceptfd < 0)
	{
		pub_log_info("[%s][%d] accept error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_CONTINUE;
	}

	anet_nonblock(NULL, acceptfd);
	cycle = (sw_log_cycle_t *)fd_lists->data;

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = acceptfd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)log_rcv_msg;
	ret = select_add_event(cycle->log_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error fd[%d].", 
				__FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t create_work_thread()
{
	int	i = 0;
	int	ret = 0;
	int	log_thread_cnt;

	pub_log_info("[%s][%d][%s]ready create work thread.", __FILE__, __LINE__, __FUNCTION__);
	log_thread_cnt = g_alog_cfg.pthread_num;
	log_thread = calloc(log_thread_cnt+1, sizeof(log_thread_t));
	if (log_thread == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__,__LINE__);
		return SW_ERROR;	
	}

	pub_log_info("[%s][%d]log_thread_cnt[%d]", __FILE__, __LINE__, log_thread_cnt);	
	for (i = 0; i <= log_thread_cnt; i++)
	{
		ret = create_single_thread(i);
		if (ret)
		{
			pub_log_error("[%s][%d] create worker thread error.", __FILE__,__LINE__);
			free(log_thread);
			return SW_ERROR;
		}
	}
	return SW_OK;
}

