#include "pub_alog.h"
#include "pub_shmex.h"
#include "pub_log.h"
#include "pub_time.h"
#include "sem_lock.h"
#include "pub_file.h"
#include "anet.h"

#define ALOG_MAX_HEAD_LEN	256
#define ALOG_TRACE_DEFAULT_STR "000000|000000000000|"
#define ALOG_TRACE_STR_LEN	20
static sw_shmex_t	g_alog_shm;
static alog_cache_t	*g_alog_cache;
static long	g_last_conn_time = 0;
static int	g_log_end = 0;
static int	g_alog_update = 0;
static sw_int64_t	g_msg_time = 0;
static alog_bits_t	*g_alog_bits;

static int alog_send_block_msg(int level);
extern int log_alog_dump(u_char *dumpaddr, const unsigned char *addr, long len);
extern int log_get_dump_size(int len);
extern int writelog(char *fmt, ...);

static alog_connect_t	g_alog_connect = NULL;

static int alog_connect();

int log_set_alog_cfg(alog_cfg_t alog)
{
	memset(&g_alog_cfg, 0x0, sizeof(g_alog_cfg));
	memcpy(&g_alog_cfg, &alog, sizeof(g_alog_cfg));
	
	return SW_OK;
}

int alog_set_bits(alog_cfg_t *alog)
{
	g_alog_bits = &alog->bits;
	pub_log_info("[%s][%d] alog max_size=[%d] lockid=[%d]", __FILE__, __LINE__,
		g_alog_bits->max_size, g_alog_bits->lockid);
	
	return 0;
}

static int get_machine(char *name)
{
	struct utsname uts;
	memset(&uts, 0x00, sizeof(uts));

	if (uname(&uts) < 0)
	{
		writelog("[%s][%d] uname error! errno=[%d][%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	strncpy(name, uts.machine, strlen(uts.machine));

	return SW_OK;
}

static int alog_get_lastname(char *fpath, char *lastname)
{
	char	*ptr = NULL;
	char	name[128];

	memset(name, 0x0, sizeof(name));

	if (fpath == NULL || fpath[0] == '\0' || lastname == NULL)
	{
		return SW_ERROR;
	}
	get_last_name(fpath, name);
	ptr = strstr(name, ".log");
	if (ptr != NULL)
	{
		*ptr = '\0';
	}
	strcpy(lastname, name);

	return SW_OK;
}

static int alog_cache_refresh()
{
	int	i = 0;

	g_alog_cache->offset = ALOG_MAX_HEAD_LEN;
	g_alog_cache->nelts = 0;
	for (i = 0; i < ALOG_MAX_MSG_NUM; i++)
	{
		g_alog_cache->trace_offsets[i] = 0;
	}

	return SW_OK;
}

static int alog_cache_init(char *fpath)
{
	char	fname[128];

	memset(fname, 0x0, sizeof(fname));

	alog_get_lastname(fpath, fname);
	memset(&g_alog_shm, 0x0, sizeof(g_alog_shm));
	g_alog_shm.size = g_alog_alloc_size;
	strncpy(g_alog_shm.filename, fname, sizeof(g_alog_shm.filename) - 1);
	if (pub_shmex_alloc(&g_alog_shm) < 0)
	{
		writelog("[%s][%d] Alloc shm error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	g_alog_cache = (alog_cache_t *)g_alog_shm.addr;
	g_alog_cache->ptr = g_alog_shm.addr + sizeof(alog_cache_t);
	g_alog_cache->size = g_alog_alloc_size - sizeof(alog_cache_t) - ALOG_MAX_HEAD_LEN;
	if (g_alog_cache->nelts > 0)
	{
		if (alog_send_block_msg(ALOG_END) < 0)
		{
			writelog("[%s][%d] Send msg to log server error!", __FILE__, __LINE__);
		}
	}
	alog_cache_refresh();

	return SW_OK;
}

static int alog_cache_append(char *msg, int len)
{
	if (g_alog_cache->size - g_alog_cache->offset < (size_t)len)
	{
		writelog("[%s][%d] ERROR! size=[%d] offset=[%d] len=[%d]",
				__FILE__, __LINE__, g_alog_cache->size, g_alog_cache->offset, len);
		return SW_ERROR;
	}
	memcpy(g_alog_cache->ptr + g_alog_cache->offset, msg, len);
	g_alog_cache->offset += len;

	return SW_OK;
}

int alog_close_fd()
{
	if (g_alog_fd > 0)
	{
		close(g_alog_fd);
		g_alog_fd = -1;
	}

	return SW_OK;
}

int alog_destroy()
{
	alog_close_fd();

	pub_shmex_free(&g_alog_shm);

	return SW_OK;
}

int alog_init(char *fpath)
{
	memset(&g_alog_cycle, 0x0, sizeof(g_alog_cycle));
	get_last_name(fpath, g_alog_filename);
	g_alog_type = g_module_info.type;
	g_alog_pid = getpid();
	g_alog_cycle.transport = g_alog_cfg.transport;
	g_alog_port = g_alog_cfg.port;
	g_alog_wait_time = g_alog_cfg.wait_time;
	g_alog_alloc_size = g_alog_cfg.buffer_size > ALOG_DEFAULT_ALLOC_SIZE ? g_alog_cfg.buffer_size : ALOG_DEFAULT_ALLOC_SIZE;
	strncpy(g_alog_name, g_module_info.name, sizeof(g_alog_name) - 1);
	strncpy(g_alog_ip, g_alog_cfg.ip, sizeof(g_alog_cycle.ip) - 1);
	strncpy(g_alog_cycle.upath, g_alog_cfg.upath, sizeof(g_alog_cycle.upath) - 1);
	writelog("[%s][%d] g_alog_filename[%s]upath[%s]transport[%d]!", 
		__FILE__, __LINE__, g_alog_filename, g_alog_cfg.upath, g_alog_cfg.transport);

	memset(g_alog_cycle.errstr, 0x0, sizeof(g_alog_cycle.errstr));
	g_alog_fd = alog_connect();
	if (g_alog_fd < 0)
	{
		writelog("[%s][%d] Connect to logsvr [%s]:[%d] error",
				__FILE__, __LINE__, g_alog_ip, g_alog_port);
		g_alog_is_init = 0;
		return SW_ERROR;
	}
	writelog("[%s][%d] Connect to logsvr success!", __FILE__, __LINE__);

	if (alog_cache_init(fpath) < 0)
	{
		writelog("[%s][%d] Alog cache init error!", __FILE__, __LINE__);
		close(g_alog_fd);
		g_alog_is_init = 0;
		return SW_ERROR;
	}
	memset(&g_alog_sys_info, 0x0, sizeof(g_alog_sys_info));
	g_alog_is_init = 1;

	return SW_OK;
}
	

int alog_check_fd(int fd)
{
	int	ret = 0;
	fd_set  rmask;
	fd_set  wmask;
	fd_set  emask;
	struct timeval	time_out;
	
	if (fd < 0)
	{
		writelog("[%s][%d] Fd error, fd:[%d]", __FILE__, __LINE__, fd);
		return FD_ERROR;
	}
	
	while (1)
	{
		FD_ZERO(&rmask);
		FD_ZERO(&wmask);
		FD_ZERO(&emask);
		FD_SET(fd, &rmask);
		FD_SET(fd, &wmask);
		FD_SET(fd, &emask);
		time_out.tv_sec  = 0 ;
		time_out.tv_usec = 50000;

		ret = select(fd + 1, &rmask, &wmask, &emask, &time_out);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			writelog("[%s][%d] Select fd:[%d] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, fd, errno, strerror(errno));
			return FD_ERROR;
		}
		else if (ret == 0)
		{
			writelog("[%s][%d] check fd:[%d] select timeout!",
				__FILE__, __LINE__, fd);
			return FD_WRITEABLE;
		}

		if (FD_ISSET(fd, &emask))
		{
			writelog("[%s][%d] Check fd:[%d] error!", __FILE__, __LINE__, fd);
			return FD_ERROR;
		}
	
		if (FD_ISSET(fd, &rmask))
		{
			writelog("[%s][%d] Check fd:[%d] available for reading!",
				__FILE__, __LINE__, fd);
			return FD_READABLE;
		}

		if (FD_ISSET(fd, &wmask))
		{
			return FD_WRITEABLE;
		}
	}
	
	return FD_ERROR;
}

int alog_reconn()
{
	fd_status_t	fs = 0;
	
	fs = alog_check_fd(g_alog_fd);
	if (fs == FD_WRITEABLE)
	{
		return SW_OK;
	}	

	alog_close_fd();

	long    now = (long)time(NULL);
	if (g_last_conn_time > 0 && now - g_last_conn_time < g_alog_wait_time)
	{
		return SW_ERROR;
	}

	g_alog_fd = alog_connect();
	if (g_alog_fd < 0)
	{
		g_last_conn_time = (long)time(NULL);
		writelog("[%s][%d] Reconnect to log server [%s]:[%d] error, errmsg:[%s]",
			__FILE__, __LINE__, g_alog_cycle.ip, g_alog_cycle.port, g_alog_cycle.errstr);
		return SW_ERROR;
	}
	g_last_conn_time = 0;
	writelog("[%s][%d] Reconnect to log server [%s]:[%d] success!",
		__FILE__, __LINE__, g_alog_cycle.ip, g_alog_cycle.port);

	return SW_OK;
}

int alog_comm(int fd, u_char *buf, size_t len)
{
	int     ret = 0;
	int     times = 0;

	if (alog_reconn() < 0)
	{
		return SW_ERROR;
	}

	times = 0;
	while (1)
	{
		ret = anet_writen(g_alog_fd, buf, len);
		if (ret < 0)
		{
			writelog("[%s][%d] Send data error!", __FILE__, __LINE__);
			if (errno == EPIPE)
			{
				if (alog_reconn() < 0)
				{
					return SW_ERROR;
				}
				times++;
				if (times > 1)
				{
					writelog("[%s][%d] Reconnect log server error!",
						__FILE__, __LINE__);
					return SW_ERROR;
				}
				continue;
			}
			return SW_ERROR;
		}
		break;
	}

	return SW_OK;
}

static char alog_get_level_text(sw_uint_t level)
{
	char	flag;

	switch (level)
	{
		case SW_LOG_INFO:
			flag = 'I';
			break;
		case SW_LOG_WARNING:
			flag = 'W';
			break;
		case SW_LOG_ERROR:
			flag = 'E';
			break;
		case SW_LOG_DEBUG:
			flag = 'D';
			break;
		default:
			flag = 'O';
			break;
	}

	return flag;
}

static int alog_get_msghead(char *msghead)
{
	char	ch = '\0';
	char	str_date[64];

	memset(str_date, 0x0, sizeof(str_date));

	pub_time_gettime(NULL, "%d %d %d|%d:%d:%d", "%Y%m%d|%H:%M:%S", ".%.3ld",0, 17, str_date);

	if (g_alog_type == ND_LSN)
	{
		ch = 'C';
	}
	else if (g_alog_type == ND_SVC)
	{
		ch = 'B';
	}
	else
	{
		ch = 'O';
	}
	sprintf(msghead, "@%d|%c|%.31s|", g_alog_pid, ch, str_date);

	return SW_OK;
}

int alog_core(sw_uint_t level, char *msg, const char *addr, int len)
{
	int	dumplen = 0;
	int	left = 0;
	int	size = 0;
	int	msg_size = 0;
	char	level_text = '\0';
	char	msghead[128];
	
	if (msg != NULL && msg[0] != '\0')
	{
		memset(msghead, 0x0, sizeof(msghead));
		alog_get_msghead(msghead);
		level_text = alog_get_level_text(level);

		if (len > 0 && addr != NULL)
		{
			size = log_get_dump_size(len);
		}
		left = g_alog_cache->size - g_alog_cache->offset;
		msg_size = strlen(msghead) + ALOG_TRACE_STR_LEN + 3 + strlen(msg) + size;
		if (left < msg_size || (g_alog_cache->nelts + 1 >= ALOG_MAX_MSG_NUM))
		{
			writelog("[%s][%d] No enough space to cache msg, send begin...", __FILE__, __LINE__);
			alog_update_count();
			if (alog_reconn() < 0)
			{
				writelog("[%s][%d] alog_reconn error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			if (alog_send_block_msg(ALOG_BLOCK_END) < 0)
			{
				writelog("[%s][%d] Send msg error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			alog_cache_refresh();
			left = g_alog_cache->size - g_alog_cache->offset;
		}

		if (left < msg_size)
		{
			writelog("[%s][%d] Too large msg, don't send to log server!",
					__FILE__, __LINE__);
			return SW_ERROR;
		}

		alog_cache_append(msghead, strlen(msghead));
		g_alog_cache->trace_offsets[g_alog_cache->nelts] = g_alog_cache->offset;
		alog_cache_append(ALOG_TRACE_DEFAULT_STR, ALOG_TRACE_STR_LEN);
		alog_cache_append(&level_text, 1);
		alog_cache_append(" ", 1);
		alog_cache_append(msg, strlen(msg));
		alog_cache_append("\n", 1);
		g_alog_cache->nelts++;

		if (len > 0 && addr != NULL)
		{
			dumplen = log_alog_dump(g_alog_cache->ptr + g_alog_cache->offset, (const unsigned char *)addr, len);
			g_alog_cache->offset += dumplen;
		}
	}

	if (level == ALOG_END)
	{
		g_log_end = 1;
	}

	if (level == ALOG_BLOCK_END && g_log_end)
	{
		return SW_OK;
	}

	if (level != ALOG_END && g_log_end == 1)
	{
		g_log_end = 0;
	}
	
	if (level == ALOG_BLOCK_END)
	{
		alog_update_count();
	}
	
	if (level == ALOG_END || level == ALOG_BLOCK_EXIT)
	{
		g_msg_time = 0;
	}
	
	if (level == ALOG_BLOCK_END || level == ALOG_END || level == ALOG_BLOCK_EXIT)
	{
		if (alog_send_block_msg(level) < 0)
		{
			writelog("[%s][%d] Send msg error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		alog_cache_refresh();
		memset(&g_alog_sys_info, 0x0, sizeof(g_alog_sys_info));
	}

	if (level == ALOG_BLOCK_EXIT)
	{
		alog_destroy();
	}

	return SW_OK;
}

int alog_send_block_msg(int level)
{
	int	i = 0;
	int	count = 0;
	int	offset = 0;
	size_t	headlen = 0;
	char	tmp[256];
	char	head[256];
	
	if (g_alog_cache->nelts == 0)
	{
		if (g_alog_update)
		{
			g_alog_update = 0;
		}
		return SW_OK;
	}
	
	for (i = 0; i < g_alog_cache->nelts; i++)
	{	
		memset(tmp, 0x0, sizeof(tmp));
		sprintf(tmp, "%06d|%-12s|", g_alog_sys_info.mtype, g_alog_sys_info.sys_traceno);
		memcpy(g_alog_cache->ptr + g_alog_cache->trace_offsets[i], tmp, ALOG_TRACE_STR_LEN);
	}
	
	if (level == ALOG_END && g_alog_sys_info.sys_date[0] != '\0' && g_alog_sys_info.sys_traceno[0] != '\0')
	{
		count = alog_get_count() + 1;
	}
	else
	{
		count = 0;
	}
	/*** datetime|pid|name|filename|level|mtype|chnl|sys_date|sys_traceno|count|machine|msglen| ***/
	memset(head, 0x0, sizeof(head));
	sprintf(head, "00000000%lld|%d|%s|%s|%d|%d|%s|%s|%s|%d|%s|%08zd|",
			g_msg_time > 0 ? g_msg_time : pub_time_get_current(), g_alog_pid, g_alog_name, g_alog_filename,
			level, g_alog_sys_info.mtype, g_alog_sys_info.chnl, g_alog_sys_info.sys_date,
			g_alog_sys_info.sys_traceno, count, g_alog_machine, g_alog_cache->offset - ALOG_MAX_HEAD_LEN);
	g_msg_time = 0;
	headlen = strlen(head);
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08zd", headlen - 8);
	memcpy(head, tmp, 8);

	offset = ALOG_MAX_HEAD_LEN - headlen;
	memcpy(g_alog_cache->ptr + offset, head, headlen);
	if (alog_comm(g_alog_fd, g_alog_cache->ptr + offset, g_alog_cache->offset - offset) < 0)
	{
		writelog("[%s][%d] Send msg to log server error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	if (g_alog_update)
	{
		g_alog_update = 0;
	}

	return SW_OK;
}

int alog_set_sysinfo(int mtype, char *sys_date, sw_int64_t trace_no, char *chnl)
{
	if (mtype > 0)
	{
		g_alog_sys_info.mtype = mtype;
	}

	if (trace_no > 0)
	{
		memset(g_alog_sys_info.sys_traceno, 0x0, sizeof(g_alog_sys_info.sys_traceno));
		sprintf(g_alog_sys_info.sys_traceno, "%s%0*lld", g_alog_trc_prefix, (int)(SW_TRACE_LEN - strlen(g_alog_trc_prefix)), trace_no);
	}

	if (sys_date != NULL && sys_date[0] != '\0')
	{
		memset(g_alog_sys_info.sys_date, 0x0, sizeof(g_alog_sys_info.sys_date));
		memcpy(g_alog_sys_info.sys_date, sys_date, sizeof(g_alog_sys_info.sys_date) - 1);
	}

	if (chnl != NULL && chnl[0] != '\0')
	{
		memset(g_alog_sys_info.chnl, 0x0, sizeof(g_alog_sys_info.chnl));
		memcpy(g_alog_sys_info.chnl, chnl, sizeof(g_alog_sys_info.chnl) - 1);
	}

	return 0;
}

int alog_bits_init(alog_cfg_t *alog)
{
	int	i = 0;
	
	g_alog_bits = &alog->bits;
	
	g_alog_bits->lockid = sem_new_lock_id();
	if (g_alog_bits->lockid < 0)
	{
		pub_log_error("[%s][%d] Alog alloc new lock error!", __FILE__, __LINE__);
		return -1;
	}
	g_alog_bits->max_size = ALOG_MAX_CNT;
	g_alog_bits->current = 0;

	for (i = 0; i < g_alog_bits->max_size; i++)
	{
		g_alog_bits->bit[i].use = 0;
		g_alog_bits->bit[i].cnt = 0;
		memset(g_alog_bits->bit[i].sys_date, 0x0, sizeof(g_alog_bits->bit[i].sys_date));
		memset(g_alog_bits->bit[i].sys_traceno, 0x0, sizeof(g_alog_bits->bit[i].sys_traceno));
		
	}
	pub_log_info("[%s][%d] alog bits init success!", __FILE__, __LINE__);
	
	return 0;
}

int alog_get_count()
{
	int	i = 0;
	int	cnt = 0;
	
	if (!g_use_alog)
	{
		return 0;
	}
	
	if (g_alog_sys_info.sys_date[0] == '\0' || g_alog_sys_info.sys_traceno[0] == '\0')
	{
		return 0;
	}
	
	if (g_alog_bits == NULL)
	{
		return 0;
	}
	sem_mutex_lock(g_alog_bits->lockid);
	for (i = 0; i < g_alog_bits->max_size; i++)
	{
		if (strcmp(g_alog_bits->bit[i].sys_date, g_alog_sys_info.sys_date) == 0 &&
			strcmp(g_alog_bits->bit[i].sys_traceno, g_alog_sys_info.sys_traceno) == 0)
		{
			cnt = g_alog_bits->bit[i].cnt;
			g_alog_bits->bit[i].use = 0;	
			g_alog_bits->bit[i].cnt = 0;	
			memset(g_alog_bits->bit[i].sys_date, 0x0, sizeof(g_alog_bits->bit[i].sys_date));
			memset(g_alog_bits->bit[i].sys_traceno, 0x0, sizeof(g_alog_bits->bit[i].sys_traceno));
			goto DONE;
		}
	}

DONE:	
	sem_mutex_unlock(g_alog_bits->lockid);
	return cnt;
}

int alog_add_count(int count)
{
	int	i = 0;
	
	if (!g_use_alog)
	{
		return SW_OK;
	}
	
	if (g_alog_sys_info.sys_date[0] == '\0' || g_alog_sys_info.sys_traceno[0] == '\0')
	{
		g_msg_time = 0;
		return SW_OK;
	}

	if (g_alog_bits == NULL)
	{
		writelog("[%s][%d] g_alog_bits is null!", __FILE__, __LINE__);
		return -1;
	}

	sem_mutex_lock(g_alog_bits->lockid);
	for (i = 0; i < g_alog_bits->max_size; i++)
	{
		if (strcmp(g_alog_bits->bit[i].sys_date, g_alog_sys_info.sys_date) == 0 &&
				strcmp(g_alog_bits->bit[i].sys_traceno, g_alog_sys_info.sys_traceno) == 0)
		{
			g_alog_bits->bit[i].cnt += count;
			g_msg_time = pub_time_get_current();
			g_alog_update = 1;
			sem_mutex_unlock(g_alog_bits->lockid);
			return SW_OK;
		}
	}
	sem_mutex_unlock(g_alog_bits->lockid);

	return SW_ERR;
}

int alog_update_count()
{
	int	i = 0;
	int	index = -1;
	
	if (!g_use_alog)
	{
		return SW_OK;
	}
	
	if (g_alog_sys_info.sys_date[0] == '\0' || g_alog_sys_info.sys_traceno[0] == '\0')
	{
		g_msg_time = 0;
		return SW_OK;
	}
	
	if (g_alog_update)
	{
		g_alog_update = 0;
		return SW_OK;
	}
	
	if (g_alog_bits == NULL)
	{
		writelog("[%s][%d] g_alog_bits is null!", __FILE__, __LINE__);
		return -1;
	}

	sem_mutex_lock(g_alog_bits->lockid);
	
	if (g_alog_bits->current >= g_alog_bits->max_size)
	{
		g_alog_bits->current = 0;
	}

	for (i = 0; i < g_alog_bits->max_size; i++)
	{
		if (strcmp(g_alog_bits->bit[i].sys_date, g_alog_sys_info.sys_date) == 0 &&
			strcmp(g_alog_bits->bit[i].sys_traceno, g_alog_sys_info.sys_traceno) == 0)
		{
			index = i;
			g_alog_bits->bit[i].cnt++;	
			goto DONE;
		}
	}
	
	for (i = g_alog_bits->current; i < g_alog_bits->max_size; i++)
	{
		if (g_alog_bits->bit[i].use == 0)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		for (i = 0; i < g_alog_bits->current; i++)
		{
			if (g_alog_bits->bit[i].use == 0)
			{
				index = i;
				break;
			}
		}
	}
	g_alog_bits->current++;

	if (index == -1)
	{
		writelog("[%s][%d] No enough space to save log bits!", __FILE__, __LINE__);
		goto ERR;
	}
	
	g_alog_bits->bit[index].use = 1;
	g_alog_bits->bit[index].cnt = 1;
	memcpy(g_alog_bits->bit[index].sys_date, g_alog_sys_info.sys_date, sizeof(g_alog_bits->bit[index].sys_date) - 1);
	memcpy(g_alog_bits->bit[index].sys_traceno, g_alog_sys_info.sys_traceno, sizeof(g_alog_bits->bit[index].sys_traceno) - 1);
	
DONE:	
	g_msg_time = pub_time_get_current();
	g_alog_update = 1;
	sem_mutex_unlock(g_alog_bits->lockid);
	return SW_OK;
ERR:
	sem_mutex_unlock(g_alog_bits->lockid);
	return SW_ERROR;
}

int alog_write_cache(int fd)
{
	int ret = 0;
	int buflen = 0;

	buflen = g_alog_cache->offset - ALOG_MAX_HEAD_LEN;
	ret = write(fd, g_alog_cache->ptr + ALOG_MAX_HEAD_LEN, buflen);
	if (ret != buflen)
	{
		writelog("[%s][%d] write cache log error. errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	alog_cache_refresh();

	return SW_OK;
}

static int alog_unix_connect()
{
	int	sockid = 0;
	char	errmsg[256];
	char	path[256];

	memset(errmsg, 0x0, sizeof(errmsg));
	memset(path, 0x0, sizeof(path));

	snprintf(path, sizeof(path), "%s/tmp/.%s_.file", getenv("SWWORK"), g_alog_cfg.upath);
	sockid = anet_unix_nonblock_connect(errmsg, path);
	if (sockid <= 0)
	{
		pub_log_error("[%s][%d] Unix connect error! path=[%s] errmsg=[%s]",
			__FILE__, __LINE__, path, errmsg);
		return SW_ERROR;
	}

	return sockid;
}

static int alog_tcp_connect()
{
	int	sockid = 0;
	char	errmsg[256];
	
	memset(errmsg, 0x0, sizeof(errmsg));

	sockid = anet_tcp_connect(errmsg, g_alog_ip, g_alog_port);
	if (sockid < 0)
	{
		pub_log_error("[%s][%d] Connect to [%s]:[%d] error, errmsg=[%s]",
			__FILE__, __LINE__, g_alog_ip, g_alog_port, errmsg);
		return SW_ERROR;
	}

	return sockid;
}

static int alog_comm_init(int flag)
{
	if (flag == SW_UNIX_TRANSPORT)
	{
		g_alog_connect = alog_unix_connect;
	}
	else
	{
		g_alog_connect = alog_tcp_connect;
	}
	
	return SW_OK;
}

static int alog_connect()
{
	static int	first = 1;
	
	if (first)
	{
		alog_comm_init(g_alog_cfg.transport);
		first = 0;
	}

	return g_alog_connect();
}

