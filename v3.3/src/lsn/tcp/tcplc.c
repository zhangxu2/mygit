#include "lsn_pub.h"
#include "pthread.h"

#define LINKINFO_TIMEOUT (60 * 10)
static int	g_is_conn = 0;
static int	g_port = 0;
static int	g_sockid = 0;
static int  g_scantime = 0;
static char	g_ip[32];
static long	g_last_active_time = 0;
static sw_lsn_cycle_t *g_cycle = NULL;
static pthread_mutex_t g_linkinfo_mutex;

typedef enum
{
	RECV = 1,
	SEND,
	SCAN
} type_t;

typedef enum
{
	IDLE = 1,
	BUSY,
	DEAD
} status_t;

typedef struct thread_info_s
{
	int status;
	sw_cmd_t	cmd;
	pthread_t   tid;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	sw_buf_t  locbuf;
} thread_info_t;

thread_info_t *g_thread_infos = NULL;

int tcplc_extcevt(sw_fd_list_t *fd_lists);
static int create_work_thread(sw_lsn_cycle_t *cycle);
static int create_single_thread(long index, int type);
void *deal_recv_work(void * args);
void *deal_send_work(void * args);
void *deal_scan_work(void * args);

static int check_fd(int fd)
{
	int     ret = 0;
	fd_set  rmask;
	fd_set  wmask;
	fd_set  emask;
	struct timeval	time_out;

	if (fd < 0)
	{
		pub_log_error("[%s][%d] fd error!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] Select fd:[%d] error, errno=[%d]:[%s]",
					__FILE__, __LINE__, fd, errno, strerror(errno));
			return FD_ERROR;
		}
		else if (ret == 0)
		{
			pub_log_error("[%s][%d] check fd:[%d] select timeout!",
					__FILE__, __LINE__, fd);
			return FD_WRITEABLE;
		}

		if (FD_ISSET(fd, &emask))
		{
			pub_log_error("[%s][%d] Check fd:[%d] error! errno=[%d][%s]",
					__FILE__, __LINE__, fd, errno, strerror(errno));
			return FD_ERROR;
		}

		if (FD_ISSET(fd, &wmask))
		{
			return FD_WRITEABLE;
		}
	}

	return FD_ERROR;
}

static int tcplc_close_fd(sw_lsn_cycle_t *cycle)
{
	pub_log_info("[%s][%d] close fd[%d]",__FILE__,__LINE__,g_sockid);
	if (g_sockid > 0)
	{
		select_del_event(cycle->lsn_fds, g_sockid);
		close(g_sockid);
		g_sockid = -1;
	}
	g_is_conn = 0;

	return SW_OK;
}

static int tcplc_find_free(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	time_t 	now = time(NULL);
	static int curr_index = 0;

	pthread_mutex_lock(&g_linkinfo_mutex);
	if (curr_index >= cycle->lsn_conf.conmax)
	{
		curr_index = 0;
	}

	for (i = curr_index; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].use == 1 && now - cycle->link_info[i].start_time > LINKINFO_TIMEOUT)
		{
			pub_log_info("[%s][%d] linkinfo[%d] key[%s]  TIMEOUT, RESUE!", __FILE__, __LINE__, i, cycle->link_info[i].data_info);
			memset(cycle->link_info + i, 0x0, sizeof(cycle->link_info[i]));
		}
		if (cycle->link_info[i].use == 0 )
		{
			pub_log_info("[%s][%d] find free place for linkinfo[%d]!", __FILE__, __LINE__, i);
			curr_index = i + 1;
			cycle->link_info[i].use = 1;
			pthread_mutex_unlock(&g_linkinfo_mutex);
			return i;
		}
	}

	for (i = 0; i < curr_index; i++)
	{
		if (cycle->link_info[i].use == 1 && now - cycle->link_info[i].start_time > LINKINFO_TIMEOUT)
		{
			pub_log_info("[%s][%d] linkinfo[%d] key[%s]  TIMEOUT, RESUE!", __FILE__, __LINE__, i, cycle->link_info[i].data_info);
			memset(cycle->link_info + i, 0x0, sizeof(cycle->link_info[i]));
		}
		if (cycle->link_info[i].use == 0)
		{
			pub_log_info("[%s][%d] find free place for linkinfo[%d]!", __FILE__, __LINE__, i);
			curr_index = i + 1;
			cycle->link_info[i].use = 1;
			pthread_mutex_unlock(&g_linkinfo_mutex);
			return i;
		}
	}
	pthread_mutex_unlock(&g_linkinfo_mutex);
	pub_log_error("[%s][%d] No enough space to save link info!", __FILE__, __LINE__);

	return SW_ERROR;
}

static int tcplc_find_index(sw_lsn_cycle_t *cycle, char *cbmkey)
{
	int	i = 0;

	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].use == 0)
		{
			continue;
		}

		if (strcmp(cycle->link_info[i].data_info, cbmkey) == 0)
		{
			pub_log_info("[%s][%d]  find [%s]'s link info![%d]", __FILE__, __LINE__, cbmkey, i);
			return i;
		}
	}
	pub_log_error("[%s][%d] Can not find [%s]'s link info!", __FILE__, __LINE__, cbmkey);

	return SW_ERROR;
}

static int tcplc_reconn(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	sw_fd_list_t	fd_list;

	if (g_is_conn)
	{
		if (check_fd(g_sockid) == FD_WRITEABLE)
		{
			return SW_OK;
		}
		tcplc_close_fd(cycle);
	}

	g_sockid = lsn_pub_connect(g_ip, g_port);
	if (g_sockid < 0)
	{
		pub_log_error("[%s][%d] Connect to [%s]:[%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, g_ip, g_port, errno, strerror(errno));
		g_is_conn = 0;

		return SW_AGAIN;
	}
	pub_log_info("[%s][%d] Connect to [%s]:[%d] success!", __FILE__, __LINE__, g_ip, g_port);

	ret = lsn_set_fd_noblock(g_sockid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d[ Noblock error! FD=[%d]", __FILE__, __LINE__, g_sockid);
		close(g_sockid);
		return SW_ERROR;
	}

	if (cycle->lsn_conf.socksndbufsize > 0)
	{
		socklen_t       optlen = sizeof(sw_int32_t);
		sw_int32_t      optval = cycle->lsn_conf.socksndbufsize;
		errno = 0;
		setsockopt(g_sockid, SOL_SOCKET, SO_SNDBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] socksndbufsize=[%ld] errno=[%d]:[%s]",
				__FILE__, __LINE__, optval, errno, strerror(errno));
	}

	if (cycle->lsn_conf.sockrcvbufsize > 0)
	{
		socklen_t       optlen = sizeof(sw_int32_t);
		sw_int32_t      optval = cycle->lsn_conf.sockrcvbufsize;
		errno = 0;
		setsockopt(g_sockid, SOL_SOCKET, SO_RCVBUF, (char *)&optval, optlen);
		pub_log_debug("[%s][%d] sockrcvbufsize=[%ld] errno=[%d]:[%s]",
				__FILE__, __LINE__, optval, errno, strerror(errno));
	}

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = g_sockid;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)tcplc_extcevt;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d[ Add event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	g_is_conn = 1;
	pub_log_info("[%s][%d] %s success",__FILE__,__LINE__, __FUNCTION__);

	return SW_OK;
}

int tcplc_init(sw_lsn_cycle_t *cycle)
{

	int	ret = 0;
	char log_path[256];

	alog_cfg_t alog;
	memset(&alog, 0x0, sizeof(alog_cfg_t));
	log_set_alog_cfg(alog);
	memset(log_path, 0x0, sizeof(log_path));
	sprintf(log_path, "%s/log/syslog/%s_lsn.log", getenv("SWWORK"), cycle->lsn_conf.name);
	pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	pub_log_info("[%s][%d] %s begin", __FILE__, __LINE__, __FUNCTION__);

	pthread_mutex_init(&g_linkinfo_mutex, NULL);
	memset(g_ip, 0x0, sizeof(g_ip));
	strcpy(g_ip, cycle->lsn_conf.comm.group[0].remote.ip);
	g_port = cycle->lsn_conf.comm.group[0].remote.port;
	g_is_conn = 0;
	g_sockid = -1;
	g_cycle = cycle;
	g_scantime = cycle->lsn_conf.scantime;
	if (g_scantime <= 0)
	{
		g_scantime = 10;
	}
	g_thread_infos = malloc(cycle->lsn_conf.procmin * sizeof(thread_info_t) * 2);
	if (g_thread_infos == NULL)
	{
		pub_log_error("[%s][%d] malloc error, create g_thread_infos error", __FILE__, __LINE__);
		return -1;
	}

	create_work_thread(cycle);
	ret = tcplc_reconn(cycle);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] Connect error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	sleep(1);

	return SW_OK;
}

int tcplc_destroy()
{
	if (g_thread_infos == NULL || g_cycle == NULL)
	{
		return 0;
	}
	int i = 0;
	sw_lsn_cycle_t *cycle = g_cycle;

	for (i = 0; i < cycle->lsn_conf.procmin * 2; ++i)
	{
		if (g_thread_infos[i].tid > 0 && pthread_kill(g_thread_infos[i].tid, 0) == 0)
		{
			pthread_cancel(g_thread_infos[i].tid);
		}
		pthread_join(g_thread_infos[i].tid, NULL);
		pthread_mutex_destroy(&(g_thread_infos[i].mutex));
		pthread_cond_destroy(&(g_thread_infos[i].cond));
		pub_buf_clear(&(g_thread_infos[i].locbuf));
	}
	free(g_thread_infos);
	pub_log_info("[%s][%d] %s success", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

int tcplc_timeoutevt(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	len = 0;
	int	sockid = 0;
	long	now = 0;
	sw_time_t	now_time = -1;
	sw_int32_t	timeout = 0;
	sw_cmd_t	cmd;
	int	ret = 0;
	sw_char_t	buf[64];
	sw_char_t	activemsg[512];
	sw_loc_vars_t	vars;

	memset(buf, 0x0, sizeof(buf));
	if (cycle == NULL)
	{
		pub_log_info("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	now = (long)time(NULL);

	sockid = g_sockid;
	if (sockid > 0 && check_fd(sockid) == FD_WRITEABLE)
	{
		if ((cycle->lsn_conf.activemsg[0] != '\0') &&
				(now - g_last_active_time >= cycle->lsn_conf.activetime))
		{
			memset(activemsg, 0x0, sizeof(activemsg));
			deal_hex_data(cycle->lsn_conf.activemsg, activemsg);
			len = strlen(activemsg);
			ret = lsn_pub_send(sockid, activemsg, len);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] Send heart msg[%s] error!", __FILE__, __LINE__, activemsg);
			}
			g_last_active_time = now;
		}
	}

	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].sockid >= 0 && cycle->link_info[i].use == 1)
		{
			now_time = (int)time(NULL);
			timeout = cycle->link_info[i].timeout;
			if (timeout == 0)
			{
				timeout = cycle->lsn_conf.timeout;
			}
			if (cycle->link_info[i].start_time != 0 && now_time - cycle->link_info[i].start_time > timeout)
			{
				pub_log_info("[%s][%d]Socket[%d][%d][%d] TIMEOUT!",
						__FILE__, __LINE__, i, cycle->link_info[i].sockid, cycle->link_info[i].trace_no);
				if (cycle->link_info[i].cmd_type == SW_CALLLSNREQ)
				{
					pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%d] traceno=[%lld]",
							__FILE__, __LINE__, cycle->link_info[i].mtype, cycle->link_info[i].trace_no);
					ret = pub_loc_vars_alloc(&vars, SHM_VARS);
					if (ret == SW_ERROR)
					{
						pub_log_bend("[%s][%d] pub_loc_vars_alloc error!",
								__FILE__, __LINE__);
						continue;
					}
					pub_log_info("[%s][%d] vars alloc success! mtype=[%d]traceno=[%lld]",
							__FILE__, __LINE__, cycle->link_info[i].mtype, cycle->link_info[i].trace_no);

					memset(buf, 0x0, sizeof(buf));
					sprintf(buf, "shm%08d", cycle->link_info[i].mtype);
					ret = vars.unserialize(&vars, buf);
					if (ret == SW_ERROR)
					{
						vars.free_mem(&vars);
						pub_log_bend("[%s][%d] vars unserialize error! mtype=[%d]",
								__FILE__, __LINE__, cycle->link_info[i].mtype);
						continue;
					}
					char    chnl[64];
					memset(chnl, 0x0, sizeof(chnl));
					loc_get_zd_data(&vars, "$listen", chnl);
					alog_set_sysinfo(cycle->link_info[i].mtype, cycle->link_info[i].cmd.sys_date,
							cycle->link_info[i].cmd.trace_no, chnl);
					pub_log_info("[%s][%d] vars unserialize success! mtype=[%d]",
							__FILE__, __LINE__, cycle->link_info[i].mtype);
					lsn_set_err(&vars, ERR_LSNOUT);
					memcpy(&cmd, &cycle->link_info[i].cmd, sizeof(sw_cmd_t));
					lsn_deal_err(cycle, &vars, &cmd);
					vars.free_mem(&vars);
				}
				memset(cycle->link_info + i, 0x0, sizeof(cycle->link_info[i]));
				pub_log_bend("[%s][%d] free linkinfo[%d]", __FILE__, __LINE__, i);

				continue;
			}
		}
	}

	return SW_OK;
}

int tcplc_call(cycle, locvar, cmd, locbuf)
	sw_lsn_cycle_t *cycle;
	sw_loc_vars_t *locvar;
	sw_cmd_t *cmd;
	sw_buf_t *locbuf;
{
	int	index = 0;
	int	sockid = 0;
	char	cbmkey[128];
	int	ret = 0;

	if (cycle == NULL || locvar == NULL || locbuf == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (tcplc_reconn(cycle) != SW_OK)
	{
		pub_log_error("[%s][%d] Reconnect error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sockid = g_sockid;
	memcpy(&cycle->link_info[sockid].cmd, cmd, sizeof(sw_cmd_t));
	ret = lsn_pub_send(sockid , locbuf->data, locbuf->len);
	if (ret != SW_OK)
	{
		lsn_set_err(locvar, ERR_SEND);
		pub_log_error("[%s][%d] tcplc send msg error! SockId=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, sockid, errno, strerror(errno));
		tcplc_close_fd(cycle);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf->data, locbuf->len, "[%s][%d] send pkg success! len=[%d]", 
			__FILE__, __LINE__, locbuf->len);
	if (cycle->chnl.fun.file_func != NULL)
	{
		pub_log_info("[%s][%d] Send file begin...", __FILE__, __LINE__);
		ret = cycle->chnl.fun.file_func(locvar, 0, sockid, NULL);
		if(ret < 0)
		{
			lsn_set_err(locvar, ERR_SNDFILE);
			pub_log_error("[%s][%d] send file error!", __FILE__, __LINE__);
			tcplc_close_fd(cycle);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] Send file success!", __FILE__, __LINE__);
	}

	if (cmd->task_flg == SW_DEL)
	{
		pub_log_info("[%s][%d] TASK DELETE!", __FILE__, __LINE__);
		mtype_delete(cmd->mtype, 0);
		return SW_OK;
	}

	memset(cbmkey, 0x0, sizeof(cbmkey));
	loc_get_zd_data(locvar, "#cbmkey", cbmkey);
	pub_log_info("[%s][%d] cbmkey=[%s]", __FILE__, __LINE__, cbmkey);

	index = tcplc_find_free(cycle);
	if (index == SW_ERROR)
	{
		lsn_set_err(locvar, ERR_FINDE);
		pub_log_error("[%s][%d] Find free link space error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->link_info[index].mtype = cmd->mtype;
	cycle->link_info[index].sockid = sockid;
	cycle->link_info[index].trace_no = cmd->trace_no;
	cycle->link_info[index].cmd_type = cmd->type;
	cycle->link_info[index].start_time = (int)time(NULL);
	cycle->link_info[index].timeout = cmd->timeout;
	memcpy(&cycle->link_info[index].cmd, cmd, sizeof(sw_cmd_t));
	strncpy(cycle->link_info[index].prdt, cmd->ori_prdt, sizeof(cycle->link_info[index].prdt) - 1);
	strncpy(cycle->link_info[index].def_name, cmd->def_name, sizeof(cycle->link_info[index].def_name) - 1);
	strncpy(cycle->link_info[index].data_info, cbmkey, sizeof(cycle->link_info[index].data_info) - 1);

	pub_log_info("[%s][%d] tcplc_call success! cbmkey=[%s]", __FILE__, __LINE__, cbmkey);

	return SW_OK;
}

int tcplc_interevt(sw_fd_list_t *fd_lists)
{
	long	mytype = 0;
	sw_fd_t		out_fd = 0;
	int	len = 0;
	int	ret = 0;
	int find = 0;
	int index = 0;
	static int curr_index = 0;
	sw_char_t	buf[128];
	sw_char_t   cmdinfo[1024];
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error! fd_lists is null!", __FILE__, __LINE__);
		return SW_CONTINUE;
	}

	if (g_thread_infos == NULL || g_cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error! g_thread_infos g_cycle is null!", __FILE__, __LINE__);
		return SW_CONTINUE;
	}
	memset(buf, 0x0, sizeof(buf));

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	out_fd = fd_lists->fd;
	pub_log_info("[%s][%d] %s fd=%d", __FILE__, __LINE__, __FUNCTION__, out_fd);

	if (curr_index < cycle->lsn_conf.procmin || curr_index >= cycle->lsn_conf.procmin * 2)
	{
		curr_index = cycle->lsn_conf.procmin;
	}
	find = 0;
	while(!find)
	{
		for (index = curr_index; index < cycle->lsn_conf.procmin * 2; ++index)
		{
			if (g_thread_infos[index].status == IDLE)
			{
				find = 1;
				break;
			}
		}

		if (find)
		{
			break;
		}
		for (index = cycle->lsn_conf.procmin; index < curr_index; ++index)
		{
			if (g_thread_infos[index].status == IDLE)
			{
				find = 1;
				break;
			}
		}
		pub_log_info("[%s][%d] all send thread busy! wait for 10ms", __FILE__, __LINE__);
		usleep(1000 * 10);
	}

	pub_log_info("[%s][%d] find free thread[%d]", __FILE__, __LINE__, index);
	curr_index = index + 1;
	len = 0;
	memset(cmdinfo, 0x01, sizeof(cmdinfo));
	pthread_mutex_lock(&(g_thread_infos[index].mutex)); 
	ret = msg_trans_rcv(out_fd, cmdinfo, &mytype, &len);
	if (ret != SW_OK )
	{
		pub_log_error("[%s][%d] recv cmd error! ret=[%d]", __FILE__, __LINE__, ret);
		pthread_mutex_unlock(&(g_thread_infos[index].mutex)); 
		return SW_CONTINUE;
	}
	memcpy(&(g_thread_infos[index].cmd), cmdinfo, sizeof(sw_cmd_t));

	g_thread_infos[index].status = BUSY;  
	pthread_cond_signal(&(g_thread_infos[index].cond)); 
	pthread_mutex_unlock(&(g_thread_infos[index].mutex)); 
	pub_log_info("[%s][%d] deal interevt request success", __FILE__, __LINE__);

	return SW_OK;
}

int tcplc_extcevt(sw_fd_list_t *fd_lists)
{
	int	index = 0;
	int sockid = 0;
	int	ret = 0;
	int find = 0;
	static int curr_index = 0;
	sw_lsn_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d] Param error! NULL!", __FILE__, __LINE__);
		return SW_DELETE;
	}

	cycle = (sw_lsn_cycle_t *)fd_lists->data;
	cycle->accept_fd = fd_lists->fd;
	sockid = cycle->accept_fd;

	if (curr_index < 0 || curr_index >= cycle->lsn_conf.procmin)
	{
		curr_index = 0;
	}
	find = 0;
	while(!find)
	{
		for (index = curr_index; index < cycle->lsn_conf.procmin; ++index)
		{
			if (g_thread_infos[index].status == IDLE)
			{
				find = 1;
				break;
			}
		}

		if (find)
		{
			break;
		}
		for (index = 0; index < curr_index; ++index)
		{
			if (g_thread_infos[index].status == IDLE)
			{
				find = 1;
				break;
			}
		}
		pub_log_info("[%s][%d] all recv thread busy! wait for 10ms", __FILE__, __LINE__);
		usleep(1000 * 10);
	}
	pub_log_info("[%s][%d] find free thread[%d]", __FILE__, __LINE__, index);
	curr_index = index + 1;

	pthread_mutex_lock(&(g_thread_infos[index].mutex)); 

	ret = lsn_pub_recv(cycle->lsn_conf.data, sockid, &(g_thread_infos[index].locbuf));
	if (ret <= SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_recv error!", __FILE__, __LINE__);
		tcplc_close_fd(cycle);
		tcplc_reconn(cycle);
		buf_refresh(&(g_thread_infos[index].locbuf));
		pthread_mutex_unlock(&(g_thread_infos[index].mutex)); 
		return SW_ERROR;
	}

	g_thread_infos[index].status = BUSY;  
	pthread_cond_signal(&(g_thread_infos[index].cond)); 
	pthread_mutex_unlock(&(g_thread_infos[index].mutex)); 
	pub_log_info("[%s][%d] deal exterevt request success", __FILE__, __LINE__);

	return SW_OK;
}

static int create_work_thread(sw_lsn_cycle_t *cycle)
{
	int i = 0;
	int    ret = 0;

	pub_log_info("[%s][%d]begin to create stat thread...", __FILE__, __LINE__);

	for (i = 0; i < cycle->lsn_conf.procmin; ++i)
	{
		ret = create_single_thread(i, RECV);	
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d]create RECV thread[%d] error", __FILE__, __LINE__, i);
			return -1;
		}
	}
	for (i = cycle->lsn_conf.procmin; i < cycle->lsn_conf.procmin * 2; ++i)
	{
		ret = create_single_thread(i, SEND);	
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d]create RECV thread[%d] error", __FILE__, __LINE__, i);
			return -1;
		}
	}

	ret = create_single_thread(cycle->lsn_conf.procmin * 2, SCAN);
	return 0;

}

static int create_single_thread(long index, int type)
{
	int ret = -1;
	pthread_attr_t  pthread_attr;

	if (g_thread_infos == NULL)
	{
		return -1;
	}

	ret = pthread_attr_init(&pthread_attr);
	if (ret)
	{
		pub_log_error("[%s][%d] pthread_attr_init error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	ret = pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
	if (ret)
	{
		pub_log_error("[%s][%d] pthread_attr_setdetachstate error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	ret = pthread_attr_setstacksize(&pthread_attr, 0x800000);
	if (ret)
	{
		pub_log_error("[%s][%d] pthread_attr_setstacksize error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	if (type == RECV)
	{
		ret = pthread_create(&(g_thread_infos[index].tid), &pthread_attr, &deal_recv_work, (void *)index);
		if (ret)
		{
			pub_log_error("[%s][%d]create thread error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			pthread_attr_destroy(&pthread_attr);
			return -1;
		}
		pub_log_info("[%s][%d] create recv thread[%d] OK!", __FILE__, __LINE__, index);
		pthread_detach(g_thread_infos[index].tid);
	}
	else if (type == SEND)
	{
		ret = pthread_create(&(g_thread_infos[index].tid), &pthread_attr, &deal_send_work, (void *)index);
		if (ret)
		{
			pub_log_error("[%s][%d]create thread error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			pthread_attr_destroy(&pthread_attr);
			return -1;
		}
		pub_log_info("[%s][%d] create send thread[%d] OK!", __FILE__, __LINE__, index);
		pthread_detach(g_thread_infos[index].tid);
	}
	else if (type == SCAN)
	{
		ret = pthread_create(&(g_thread_infos[index].tid), &pthread_attr, &deal_scan_work, (void *)index);
		if (ret)
		{
			pub_log_error("[%s][%d]create thread error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			pthread_attr_destroy(&pthread_attr);
			return -1;
		}
		pub_log_info("[%s][%d] create send thread[%d] OK!", __FILE__, __LINE__, index);
		pthread_detach(g_thread_infos[index].tid);
	}
	else
	{
		pub_log_error("[%s][%d] unknown type[%d]", __FILE__, __LINE__, type);

	}
	pthread_attr_destroy(&pthread_attr);
	pub_log_debug("[%s][%d] index=[%d] tid=[%u]", __FILE__, __LINE__, index, g_thread_infos[index].tid);
	return 0;
}

void *deal_recv_work(void * args)
{
	if (g_thread_infos == NULL ||  g_cycle == NULL)
	{
		return NULL;
	}

	int pos = (long)args;
	int index = 0;
	long	mtype = 0;
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_buf_t	locbuf;
	sw_char_t	buf[128];
	sw_char_t	cbmkey[128];
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = g_cycle;
	thread_info_t *thread_info = NULL;
	char debug_log[256];
	char err_log[256];
	memset(debug_log, 0x0, sizeof(debug_log));
	snprintf(debug_log, sizeof(debug_log), "%s/log/syslog/%s_rcv_%d.log",getenv("SWWORK"), cycle->lsn_conf.name, pos);
	pub_log_chglog(SW_LOG_CHG_DBGFILE, debug_log);
	memset(err_log, 0x0, sizeof(err_log));
	snprintf(err_log, sizeof(err_log), "%s/log/syslog/error.log",getenv("SWWORK"));
	pub_log_chglog(SW_LOG_CHG_ERRFILE, err_log); 


	memset(buf, 0x0, sizeof(buf));
	memset(&cmd, 0x0, sizeof(cmd));

	thread_info = g_thread_infos + pos;
	pthread_mutex_init(&(thread_info->mutex),NULL);
	pthread_cond_init(&(thread_info->cond),NULL);
	thread_info->status = IDLE;
	pub_buf_init(&(thread_info->locbuf));

	pub_log_info("[%s][%d] recv thread[%d] start success tid[%u]", __FILE__, __LINE__, pos, thread_info->tid);
	while(1)
	{
		pub_log_info("[%s][%d] waiting for recv work tid=[%u]", __FILE__, __LINE__, thread_info->tid);
		buf_refresh(&(thread_info->locbuf));
		thread_info->status = IDLE;
		pthread_mutex_lock(&(thread_info->mutex));
		while(thread_info->status == IDLE)
		{
			pthread_cond_wait(&(thread_info->cond), &(thread_info->mutex)); 
		}
		pthread_mutex_unlock(&(thread_info->mutex));

		pub_log_bin(SW_LOG_DEBUG, thread_info->locbuf.data, thread_info->locbuf.len, "Recv data:[%s][%d] len[%d]",
				__FILE__, __LINE__, thread_info->locbuf.len);

		ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
		if (ret != SW_OK)
		{
			pub_log_error("%s, %d, pub_loc_vars_construct error.", __FILE__, __LINE__);
			tcplc_close_fd(cycle);
			return NULL;
		}

		mtype = mtype_new();
		if (mtype < 0)
		{
			pub_log_error("[%s][%d] Create mtype error! ret=[%d]",
					__FILE__, __LINE__, mtype);
			tcplc_close_fd(cycle);
			return NULL;
		}

		ret = locvar.create(&locvar, mtype);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Create var pool error, mtype=[%ld]", 
					__FILE__,__LINE__, mtype);
			locvar.free_mem(&locvar);
			tcplc_close_fd(cycle);
			return NULL;
		}
		loc_set_zd_data(&locvar, "#first", "1");


		if (cycle->handler.des_handler != NULL)
		{
			pub_log_info("[%s][%d] Dec begin...", __FILE__, __LINE__);
			ret = cycle->handler.des_handler(&locvar, &(thread_info->locbuf), SW_DEC);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Dec error!", __FILE__, __LINE__);
				locvar.free_mem(&locvar);
				continue;
			}
			pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] After dec: len=[%d]",
					__FILE__, __LINE__, locbuf.len);
		}

		memset(cbmkey, 0x0, sizeof(cbmkey));
		loc_get_zd_data(&locvar, "#cbmkey", cbmkey);

		pub_log_info("[%s][%d] cbmkey=[%s]", __FILE__, __LINE__, cbmkey);
		if (cbmkey[0] == '\0')
		{
			pub_log_error("[%s][%d] cbmkey is null!", __FILE__, __LINE__);
			locvar.free_mem(&locvar);
			continue;
		}
		locvar.free_mem(&locvar);
		mtype_delete(mtype, 2);

		index = tcplc_find_index(cycle, cbmkey);
		if (index < 0)
		{
			pub_log_error("[%s][%d] Find [%s]'s index error!", __FILE__, __LINE__, cbmkey);
			locvar.free_mem(&locvar);
			continue;
		}

		cmd.mtype = cycle->link_info[index].mtype;
		cmd.trace_no = cycle->link_info[index].trace_no;
		cmd.type = cycle->link_info[index].cmd_type;
		strcpy(cmd.sys_date, cycle->link_info[index].cmd.sys_date);
		strncpy(cmd.dst_prdt, cycle->link_info[index].prdt, sizeof(cmd.dst_prdt) - 1);
		strncpy(cmd.def_name, cycle->link_info[index].def_name, sizeof(cmd.def_name) - 1);
		pub_log_info("[%s][%d] mtype=[%ld] trace_no=[%lld] type=[%d]dst_prdt=[%s] def_name=[%s], date=[%s] ", 
				__FILE__, __LINE__, cmd.mtype, cmd.trace_no, cmd.type, cmd.dst_prdt, cmd.def_name, cmd.sys_date);

		ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
		if (ret != SW_OK)
		{
			pub_log_error("%s, %d, pub_loc_vars_construct error.", __FILE__, __LINE__);
			return NULL;
		}

		ret = locvar.create(&locvar, cmd.mtype);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Create var pool error, mtype=[%ld]", 
					__FILE__,__LINE__, cmd.mtype);
			locvar.free_mem(&locvar);
			return NULL;
		}

		ret = lsn_pub_deal_in_task(cycle, &locvar, &cmd, &(thread_info->locbuf));
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_pub_deal_in_task error!", __FILE__, __LINE__);
			locvar.free_mem(&locvar);
			continue;
		}
		trace_insert(&locvar, &cmd, TRACE_OUT);
		memset(cycle->link_info + index, 0x0, sizeof(cycle->link_info[index]));
		/*cycle->link_info[index].use = 0;*/
		locvar.free_mem(&locvar);
		pub_log_bend("\0");
	}
	return NULL;
}


void *deal_send_work(void *args)
{
	if (g_thread_infos == NULL || g_cycle == NULL)
	{
		return NULL;
	}
	int pos = (long)args;
	int	ret = 0;
	int	err_flag = 0;
	sw_char_t	buf[128];
	sw_char_t	errcode[32];
	sw_buf_t	locbuf;
	sw_loc_vars_t	locvar;
	sw_lsn_cycle_t	*cycle = g_cycle;
	thread_info_t *thread_info = NULL;
	char debug_log[256];
	char err_log[256];
	memset(debug_log, 0x0, sizeof(debug_log));
	snprintf(debug_log, sizeof(debug_log), "%s/log/syslog/%s_snd_%d.log",getenv("SWWORK"), cycle->lsn_conf.name, pos - cycle->lsn_conf.procmin);
	pub_log_chglog(SW_LOG_CHG_DBGFILE, debug_log);
	memset(err_log, 0x0, sizeof(err_log));
	snprintf(err_log, sizeof(err_log), "%s/log/syslog/error.log",getenv("SWWORK"));
	pub_log_chglog(SW_LOG_CHG_ERRFILE, err_log); 

	thread_info = g_thread_infos + pos;
	pthread_mutex_init(&(thread_info->mutex),NULL);
	pthread_cond_init(&(thread_info->cond),NULL);
	memset(&(thread_info->cmd), 0x0, sizeof(thread_info->cmd));
	thread_info->status = IDLE;

	pub_log_info("[%s][%d] send thread[%d] start success tid=[%u]", __FILE__, __LINE__, pos - cycle->lsn_conf.procmin, thread_info->tid);
	while(1)
	{
		pub_log_info("[%s][%d] waiting for send work tid=[%u]", __FILE__, __LINE__, thread_info->tid);
		memset(&(thread_info->cmd), 0x0, sizeof(thread_info->cmd));
		thread_info->status = IDLE;
		pthread_mutex_lock(&(thread_info->mutex));
		while(thread_info->status == IDLE)
		{
			pthread_cond_wait(&(thread_info->cond), &(thread_info->mutex)); 
		}
		pthread_mutex_unlock(&(thread_info->mutex));

		memset(buf, 0x0, sizeof(buf));
		memset(&locvar, 0x0, sizeof(locvar));

		ret = pub_loc_vars_alloc(&locvar, SHM_VARS);
		if (ret != SW_OK)
		{
			pub_log_error("%s, %d, pub_loc_vars_construct error.", __FILE__, __LINE__);
			return NULL;
		}

		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "SHM%08ld", thread_info->cmd.mtype);
		ret = locvar.unserialize(&locvar, buf);
		if (ret != SW_OK)
		{
			locvar.free_mem(&locvar);
			pub_log_error("[%s][%d] unserialize error.", __FILE__,__LINE__);
			return NULL;
		}

		err_flag = 0;
		cmd_print(&(thread_info->cmd));
		trace_insert(&locvar, &(thread_info->cmd), TRACE_IN);
		pub_buf_init(&locbuf);
		switch (thread_info->cmd.type)
		{
			case SW_POSTLSNREQ:
			case SW_CALLLSNREQ:
			case SW_LINKLSNREQ:
				ret = lsn_pub_deal_out_task(cycle, &locvar, &(thread_info->cmd), &locbuf);
				if (ret != SW_OK)
				{
					err_flag = 1;
					pub_log_error("[%s][%d]lsn_pub_platform_deal error",__FILE__,__LINE__);
					break;
				}
				pub_log_info("[%s][%d] deal out task success!", __FILE__, __LINE__);
				ret = tcplc_call(cycle, &locvar, &(thread_info->cmd), &locbuf);
				if (ret != SW_OK)
				{
					err_flag = 1;
					pub_log_error("[%s][%d]send data error",__FILE__,__LINE__);
					break;
				}
				break;
			default: 
				pub_log_error("[%s][%d]receive illegal cmd	itype=[%d]",__FILE__,__LINE__,thread_info->cmd.type);
				cmd_print(&(thread_info->cmd));
				thread_info->cmd.type = SW_ERRCMD;
				thread_info->cmd.ori_type = ND_LSN;
				thread_info->cmd.dst_type = ND_SVC;
				thread_info->cmd.msg_type = SW_MSG_RES;
				thread_info->cmd.task_flg = SW_FORGET;
				strcpy(thread_info->cmd.ori_svr, cycle->base.name.data);
				sprintf(buf, "%s_%d", cycle->lsn_conf.name, cycle->lsn_conf.process_index);
				strcpy(thread_info->cmd.ori_svc, buf);
				ret = route_snd_dst(&locvar, cycle->base.global_path, &(thread_info->cmd));
				if (ret < 0)
				{
					err_flag = 1;
					pub_log_error("[%s][%d] send msg to svc error!", __FILE__, __LINE__);
				}
				break;
		}

		if (err_flag == 1)
		{
			memset(errcode, 0x0, sizeof(errcode));
			lsn_get_err(&locvar, errcode);
			pub_log_error("[%s][%d]deal task error errcode=[%s]", __FILE__, __LINE__, errcode);
		}
		else
		{
			pub_log_info("[%s][%d] deal task success! mtype=[%ld]", __FILE__, __LINE__, thread_info->cmd.mtype);
		}
		pub_buf_clear(&locbuf);
		locvar.free_mem(&locvar);
		pub_log_bend("\0");
	}

	return NULL;
}


void *deal_scan_work(void * args)
{
	
	if (g_thread_infos == NULL || g_cycle == NULL)
	{
		return NULL;
	}
	int ret = 0;
	int pos = (long)args;
	long i = 0;
	
	sw_lsn_cycle_t	*cycle = g_cycle;
	thread_info_t *thread_info = NULL;

	thread_info = g_thread_infos + pos;

	char debug_log[256];
	char err_log[256];
	memset(debug_log, 0x0, sizeof(debug_log));
	snprintf(debug_log, sizeof(debug_log), "%s/log/syslog/%s_scan.log",getenv("SWWORK"), cycle->lsn_conf.name);
	pub_log_chglog(SW_LOG_CHG_DBGFILE, debug_log);
	memset(err_log, 0x0, sizeof(err_log));
	snprintf(err_log, sizeof(err_log), "%s/log/syslog/error.log",getenv("SWWORK"));
	pub_log_chglog(SW_LOG_CHG_ERRFILE, err_log); 
	pub_log_info("[%s][%d] scan thread start success", __FILE__, __LINE__);
	while(1)
	{
		sleep(g_scantime);
		tcplc_reconn(cycle);

		for (i = 0; i < cycle->lsn_conf.procmin * 2; ++i)
		{
			if (g_thread_infos[i].tid <= 0 || pthread_kill(g_thread_infos[i].tid, 0) != 0)
			{
				pub_log_error("[%s][%d] 线程[%d][%u]死亡, 重新启动!",
						__FILE__, __LINE__, i, g_thread_infos[i].tid);
				g_thread_infos[i].status = DEAD;
			}
		}

		for (i = 0; i < cycle->lsn_conf.procmin * 2; ++i)
		{
			if (g_thread_infos[i].status != DEAD)
			{   
				continue;
			}   

			pthread_mutex_destroy(&(g_thread_infos[i].mutex));
			pthread_cond_destroy(&(g_thread_infos[i].cond));
			pub_buf_clear(&(g_thread_infos[i].locbuf));
			memset(g_thread_infos + i, 0x0, sizeof(thread_info_t));

			if (i < cycle->lsn_conf.procmin)
			{
				ret = create_single_thread(i, RECV);
			}
			else
			{
				ret = create_single_thread(i, SEND);
			}
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d]create RECV thread[%d] error", __FILE__, __LINE__, i);
				continue;
			}
			pub_log_info("[%s][%d] 线程[%d][%u]启动成功!",
					__FILE__, __LINE__, i, g_thread_infos[i].tid);
		}
		pub_log_info("[%s][%d] scan thread success", __FILE__, __LINE__);
	}
}
