#include "lsn_pub.h"

char *zip_space(char *str)
{
	char    *ptr;
	char    *destr;

	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			*ptr++ = *str;
		}
		str++;
	}
	*ptr = '\0';

	return destr;
}

char *zip_head_0(char *str)
{
	char    *ptr;
	char    *destr;

	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if (*str != '0')
		{
			break;
		}
		str++;
	}
	while (*str != '\0')
	{
		*ptr++ = *str++;
	}
	*ptr = '\0';

	return destr;
}


int start_cz_send(sw_loc_vars_t *vars, int sockid)
{
	int	ret = 0;
	int	len = 0;
	char	tmp[128];
	char	usr[16];
	char	passwd[16];
	char	sendbuf[1024];
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(usr, 0x0, sizeof(usr));
	memset(passwd, 0x0, sizeof(passwd));
	memset(sendbuf, 0x0, sizeof(sendbuf));
	
	loc_get_zd_data(vars, "#cbmkey", tmp);
	if (tmp[0] == '\0')
	{
		pub_log_error("[%s][%d] #cbmkey为空!", __FILE__, __LINE__);
		return -1;
	}
	
	strncpy(usr, tmp, 6);
	strncpy(passwd, tmp + 6, 6);
	zip_space(usr);
	zip_head_0(usr);
	zip_space(passwd);

	len = sprintf(sendbuf, "<?xml version=\"1.0\" encoding=\"GB2312\"?><LOGIN USERCODE=\"%s\" PASSWORD=\"%s\"></LOGIN>", usr, passwd);
	ret = lsn_pub_send(sockid, sendbuf, len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 发送数据失败!", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

/**
返回值:0正常/-2连接数已满/-1平台内部错误
使用说明: 如果收到连接数已满应答，10秒后重发，重发5次后，
仍然收到连接数已满应答，返回-2，表示连接数已满
**/
int start_cz_recv(sw_loc_vars_t *vars, int sockid)
{
	int	ret = 0;
	char	recvbuf[1024];
	fd_set	rmask;
	struct timeval	timeout;
	
	memset(recvbuf, 0x0, sizeof(recvbuf));
	memset(&timeout, 0x0, sizeof(timeout));

	while (1)
	{
		FD_ZERO(&rmask);
		FD_SET(sockid, &rmask);
		timeout.tv_sec  = 20;
		timeout.tv_usec = 0;

		ret = select(sockid + 1, &rmask, 0, 0, &timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				pub_log_error("[%s][%d] select error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
		}
		else if (ret == 0)
		{
			pub_log_error("[%s][%d] Timeout!", __FILE__, __LINE__);
			return -3;
		}
		
		memset(recvbuf, 0x0, sizeof(recvbuf));
		ret = recv(sockid, recvbuf, sizeof(recvbuf), 0);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] recv error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		pub_log_debug("[%s][%d] Recv data:[%s][%d]", __FILE__, __LINE__, recvbuf, ret);

		if (memcmp(recvbuf + 61, "FAILURE", 7) == 0 && memcmp(recvbuf + 101, "当前连接数已满", 14) == 0)
		{
			return -2;
		}

		if (memcmp(recvbuf + 61, "FAILURE", 7) == 0 && memcmp(recvbuf + 101, "当前连接数已满", 14) != 0)
		{
			return 0;
		}

		if (memcmp(recvbuf + 61, "SUCCESS", 7) == 0)
		{
			pub_log_debug("[%s][%d] recv confirm info!", __FILE__, __LINE__);
			break;
		}
		else
		{
			pub_log_error("[%s][%d] recv confirm info error!", __FILE__, __LINE__);
			return -1;
		}	
	}

	return 0;
}

int start_cz(sw_loc_vars_t *vars, int sockid)
{
	int	ret = 0;
	
	ret = start_cz_send(vars, sockid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 发送登录报文到财政失败!", __FILE__, __LINE__);
		return -1;
	}

	ret = start_cz_recv(vars, sockid);		
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 接收财政登录应答报文失败!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_debug("[%s][%d] 发送登录报文成功!", __FILE__, __LINE__);
	
	return 0;
}
