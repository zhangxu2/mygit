#include "pub_xml.h"
#include "pub_ares.h"
#include "pub_buf.h"
#include "anet.h"

static long g_last_conn_time = 0;
extern int readfile(const char *filename, sw_buf_t *readbuf);

int ares_init()
{
	memset(&g_ares_cycle, 0x0, sizeof(g_ares_cycle));
	g_ares_cycle.port = g_ares_cfg.port;
	g_ares_cycle.pid = getpid();
	strncpy(g_ares_cycle.ip, g_ares_cfg.ip, sizeof(g_ares_cycle.ip) - 1);
	g_ares_cycle.wait_time = g_ares_cfg.wait_time;

	memset(g_ares_cycle.errstr, 0x0, sizeof(g_ares_cycle.errstr));
	g_ares_fd = anet_tcp_connect(g_ares_cycle.errstr, g_ares_cycle.ip, g_ares_cycle.port);
	if (g_ares_fd < 0)
	{
		pub_log_error("[%s][%d] Connect to res server [%s]:[%d] error, errmsg:[%s]",
				__FILE__, __LINE__, g_ares_cycle.ip, g_ares_cycle.port, g_ares_cycle.errstr);
		g_ares_is_init = 0;
		return SW_ERROR;
	}
	else
	{
		pub_log_info("[%s][%d] Connect to res server success, FD=[%d]",
				__FILE__, __LINE__, g_ares_fd);
		g_ares_is_init = 1;
	}

	return SW_OK;
}

void ares_close_fd()
{
	if (g_ares_fd > 0)
	{
		close(g_ares_fd);
		g_ares_fd = -1;
	}
}

int ares_reconn()
{
	fd_status_t     fs;

	fs = alog_check_fd(g_ares_fd);
	if (fs == FD_WRITEABLE)
	{
		return SW_OK;
	}

	ares_close_fd();

	long	now = (long)time(NULL);
	if (g_last_conn_time > 0 && now - g_last_conn_time < g_ares_wait_time)
	{
		return SW_ERROR;
	}

	g_ares_fd = anet_tcp_connect(g_ares_cycle.errstr, g_ares_cycle.ip, g_ares_cycle.port);
	if (g_ares_fd < 0)
	{
		g_last_conn_time = (long)time(NULL);
		pub_log_error("[%s][%d] Reconnect to res server [%s]:[%d] error, errmsg:[%s]",
				__FILE__, __LINE__, g_ares_cycle.ip, g_ares_cycle.port, g_ares_cycle.errstr);
		return SW_ERROR;
	}
	g_last_conn_time = 0;
	pub_log_info("[%s][%d] Reconnect to res server [%s]:[%d] success!",
			__FILE__, __LINE__, g_ares_cycle.ip, g_ares_cycle.port);

	return SW_OK;
}

void ares_set_ares_cfg(ares_cfg_t ares)
{
	memset(&g_ares_cfg, 0x0, sizeof(g_ares_cfg));
	memcpy(&g_ares_cfg, &ares, sizeof(g_ares_cfg));
}

int ares_send(int fd, char *buf, size_t size, int type)
{
	int	ret = 0;
	int	sendlen = 0;
	char	tmp[32];
	char	sendbuf[MAX_BUF_LEN];

	memset(tmp, 0x0, sizeof(tmp));
	memset(sendbuf, 0x0, sizeof(sendbuf));
	if (size > 0)
	{
		sendlen = snprintf(sendbuf + 8, MAX_BUF_LEN - 8, "%d%s%s%s", type, ARES_SEP, buf, ARES_SEP);
	}
	else if (size == 0)
	{
		sendlen = snprintf(sendbuf + 8, MAX_BUF_LEN - 8, "%d%s", type, ARES_SEP);
	}
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08d", sendlen);
	memcpy(sendbuf, tmp, 8);
	sendlen += 8;

	ret = anet_writen(fd, sendbuf, sendlen);
	if (ret != sendlen)
	{
		pub_log_error("[%s][%d] Send data error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	return SW_OK;
}

int ares_recv(int fd, char *buf)
{
	int	i = 0;
	int	ret = 0;
	int	len = 0;
	char	tmp[32];

	if (fd < 0 || buf == NULL)
	{
		return SW_ERROR;
	}

	memset(tmp, 0x0, sizeof(tmp));
	ret = anet_readn(fd, tmp, 8);
	if (ret == 0)
	{
		pub_log_error("[%s][%d] the peer has performed an orderly shutdown! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_DECLINED;
	}
	else if (ret < 0)
	{
		pub_log_error("[%s][%d] Read data error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	for (i = 0; i < 8; i++)
	{
		if (tmp[i] < '0' || tmp[0] > '9')
		{
			pub_log_error("[%s][%d] Data error, data:[%s][%d]",
					__FILE__, __LINE__, tmp, ret);
			return SW_ERROR;
		}
	}

	len = atoi(tmp);
	ret = anet_readn(fd, buf, len);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] Read data error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	return ret;
}

int ares_count_str(char *buf, char *substr)
{
	int	cnt = 0;	
	char	*p = NULL;
	char	*ptr = buf;

	while ((p = strstr(ptr, substr)) != NULL)
	{
		cnt++;
		ptr = p + strlen(substr);
	}

	return cnt;
}

int ares_comm(int fd, char *buf, size_t len, int type)
{
	int	ret = 0;
	int	times = 0;
	char	tmpbuf[MAX_ARES_RECV_BUF_LEN];

	if (ares_reconn() < 0)
	{
		return SW_ERROR;
	}

	times = 0;
	while (1)
	{
		ret = ares_send(g_ares_fd, buf, len, type);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Send data error!", __FILE__, __LINE__);
			if (errno == EPIPE)
			{
				if (ares_reconn() < 0)
				{
					return SW_ERROR;
				}
				times++;
				if (times > 1)
				{
					return SW_ERROR;
				}
				continue;
			}
			return SW_ERROR;
		}
		break;
	}

	memset(tmpbuf, 0x0, sizeof(tmpbuf));
	ret = ares_recv(g_ares_fd, tmpbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Recv data error!", __FILE__, __LINE__);
		ares_close_fd();
		return SW_ERROR;
	}

	if (strncmp(tmpbuf, ARG_OK, 2) != 0)
	{
		pub_log_error("[%s][%d] Res server deal config error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ret -= (2 + ARES_SEP_LEN);
	memcpy(buf, tmpbuf + 2 + ARES_SEP_LEN, ret);
	buf[ret] = '\0';

	if (pub_str_rstrncmp(buf, ARES_SEP, ARES_SEP_LEN) == 0 && ares_count_str(buf, ARES_SEP) == 1)
	{
		len = strlen(buf);
		buf[len - ARES_SEP_LEN] = '\0';
		ret -= ARES_SEP_LEN;
	}

	return ret;
}

int ares_send_cfg()
{
	int	ret = 0;
	char	filename[128];
	sw_buf_t	readbuf;

	memset(filename, 0x0, sizeof(filename));
	memset(&readbuf, 0x0, sizeof(readbuf));

	if (pub_buf_init(&readbuf) < 0)
	{
		pub_log_error("[%s][%d] Alloc buffer error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(filename, "%s/cfg/swconfig.xml", getenv("SWWORK"));
	ret = readfile(filename, &readbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] read file [%s] error!",
				__FILE__, __LINE__, filename);
		pub_buf_clear(&readbuf);
		return SW_ERROR;
	}

	if (ares_init() < 0)
	{
		pub_log_error("[%s][%d] ares init error!", __FILE__, __LINE__);
		pub_buf_clear(&readbuf);
		return SW_ERROR;
	}

	ret = ares_comm(g_ares_fd, readbuf.data, readbuf.len, ARES_INIT);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send config to res server error!", __FILE__, __LINE__);
		pub_buf_clear(&readbuf);
		return SW_ERROR;
	}
	pub_buf_clear(&readbuf);

	return SW_OK;
}


int ares_link(int fd)
{
	int	ret = 0;
	char	buf[32];

	memset(buf, 0x0, sizeof(buf));
	ret = ares_comm(g_ares_fd, buf, 0, ARES_LINK);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares link error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

int ares_top(int num)
{
	int	len = 0;
	int	ret = 0;
	char	buf[MAX_ARES_RECV_BUF_LEN];

	memset(buf, 0x0, sizeof(buf));
	len = snprintf(buf, sizeof(buf), "%d", num);
	ret = ares_comm(g_ares_fd, buf, len, ADMIN_TOP);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares get top [%d] trace error!",
				__FILE__, __LINE__, num);
		return SW_ERROR;
	}
	fprintf(stderr, "%s", buf);

	return SW_OK;
}

int ares_step(sw_int64_t trace_no)
{
	int     len = 0;
	int     ret = 0;
	sw_char_t       buf[MAX_BUF_LEN];

	memset(buf, 0x0, sizeof(buf));
	len = snprintf(buf, sizeof(buf), "%lld", trace_no);
	ret = ares_comm(g_ares_fd, buf, len, ADMIN_STEP);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares get trace step error! trace_no=[%lld]",
				__FILE__, __LINE__, trace_no);
		return SW_ERROR;
	}
	fprintf(stderr, "%s", buf);

	return SW_OK;
}

