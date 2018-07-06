#include "pub_usocket.h"
#include <sys/un.h>

#define CLI_PERM        S_IRWXU

sw_int_t pub_usocket_tcpbind(char *filename, int lsncnt)
{
	int	ret = 0;
	int	lsn_fd = 0;
	socklen_t	len = 0;
	struct sockaddr_un	un;
	
	if (filename == NULL || filename[0] == '\0')
	{
		return -1;
	}

	if( lsncnt <= 0 )    /** set default lsncnt  200 **/
	{
		lsncnt = 200;
	}

	lsn_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if ( lsn_fd < 0 )
	{
		return -1;
	}

	unlink(filename);        /* in case it already exists */

	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strncpy(un.sun_path, filename,sizeof(un.sun_path)-1);

	len = sizeof(struct sockaddr_un);
	ret = bind(lsn_fd, (struct sockaddr *)&un, len);
	if (ret)
	{
		ret = -2;
		goto ERROR;
	}

	ret = listen(lsn_fd, lsncnt);
	if (ret)
	{
		ret = -3;
		goto ERROR;
	}

	return lsn_fd;

ERROR:
	if( lsn_fd >= 0 )
	{
		close(lsn_fd);
	}

	return ret;
}

sw_int_t pub_usocket_accept(int lsnfd, uid_t *puid)
{
	int	clifd = 0;
	socklen_t	len = 0;
	struct sockaddr_un	un;

	len = sizeof(struct sockaddr_un);
	while (1)
	{
		memset(&un, 0x0, sizeof(struct sockaddr_un));
		clifd = accept(lsnfd, (struct sockaddr *)&un, (socklen_t *)&len);
		if (clifd < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return SW_ERROR;
		}
		
		return clifd;
	}

	return SW_ERROR;
}

sw_int_t pub_usocket_tcpconn(char *cliname, char *svrname)
{
	int	ret = 0;
	int	clifd = 0;
	socklen_t	len = 0;
	struct sockaddr_un	unix_addr;

	if (cliname == NULL || cliname[0] == '\0' || svrname == NULL || svrname[0] == '\0')
	{
		return -1;
	}
	
	clifd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (clifd < 0)
	{
		return -1;
	}

	memset(&unix_addr, 0x0, sizeof(struct sockaddr_un));
	unix_addr.sun_family = AF_UNIX;
	strncpy(unix_addr.sun_path, cliname, sizeof(unix_addr.sun_path) - 1);
	unlink(unix_addr.sun_path);     /* in case it already exists */

	len = sizeof(struct sockaddr_un);
	ret = bind(clifd, (struct sockaddr *)&unix_addr, len);
	if (ret < 0)
	{
		ret = -3;
		goto ERROR;
	}

	ret = chmod(unix_addr.sun_path, CLI_PERM);
	if (ret < 0)
	{
		ret = -5;
		goto ERROR;
	}

	memset(&unix_addr, 0x0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	strncpy(unix_addr.sun_path, svrname, sizeof(unix_addr.sun_path) - 1);

	len = sizeof(struct sockaddr_un);
	ret = connect(clifd, (struct sockaddr *)&unix_addr, len);
	if (ret < 0)
	{
		ret = -4;
		goto ERROR;
	}

	return clifd;
ERROR:
	if (clifd >= 0) 
	{
		close(clifd);
	}

	return ret;
}

sw_int_t pub_usocket_udpbind(char *svrfile)
{
	int	ret = 0;
	int	lsnfd = 0;
	socklen_t	len =0;
	struct sockaddr_un	server_addr;

	if (svrfile == NULL || svrfile[0] == '\0')
	{
		return -1;
	}

	lsnfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (lsnfd < 0)
	{
		return -2;
	}

	unlink(svrfile);
	memset(&server_addr, 0x0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, svrfile, sizeof(server_addr.sun_path) - 1);
	len = strlen(server_addr.sun_path) + sizeof(server_addr.sun_family) + 1;

	ret = bind(lsnfd, (struct sockaddr *)&server_addr, len);
	if (ret < 0)
	{
		return -3;
	}

	return lsnfd;
}

sw_int_t pub_usocket_udprecv(int clifd, int tmout, char *buffer, int bufferlen)
{
	int	ret = 0;
	fd_set	fd_read;
	socklen_t	fromlen = 0;
	struct timeval	time_out;
	struct sockaddr_un	from;
	
	if (clifd < 0 || bufferlen < 0)
	{
		return -1;
	}

	while (1)
	{
		FD_ZERO(&fd_read);
		FD_SET(clifd, &fd_read);
		time_out.tv_sec = tmout;
		time_out.tv_usec = 0;
		ret = select(clifd + 1, &fd_read, NULL, NULL, &time_out);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return -2;
		}
		else if (ret == 0)
		{
			return -4;
		}

		if (FD_ISSET(clifd, &fd_read) )
		{
			memset(&from, 0x0, sizeof(struct sockaddr_un));
			from.sun_family = AF_UNIX;
			ret = recvfrom(clifd, buffer, bufferlen, 0, (struct sockaddr *)&from, &fromlen);
			if (ret <= 0)
			{
				return -3;
			}
			return ret;
		}
		return -1;
	}

	return -1;
}

sw_int_t pub_usocket_udpsend(int clifd, char *filename, char *buffer, int bufferlen)
{
	int	ret = 0;
	socklen_t	tlen = 0;
	struct sockaddr_un	un;
	
	if (clifd < 0 || bufferlen < 0 || filename == NULL || filename[0] == '\0')
	{
		return -1;
	}
		
	memset(&un, 0x0, sizeof(un));
	un.sun_family = AF_UNIX;
	strncpy(un.sun_path, filename, sizeof(un.sun_path) - 1);
	tlen = strlen(un.sun_path) + sizeof(un.sun_family) + 1;

	ret = sendto(clifd, buffer, bufferlen, 0, (struct sockaddr*)&un, tlen);
	if (ret <= 0)
	{
		return -2;
	}
	return ret;
}

sw_int_t udp_bind(const char *filename)
{
	char	file[256];
	
	if (filename == NULL || filename[0] == '\0')
	{
		return -1;
	}
	
	memset(file, 0x0, sizeof(file));
	sprintf(file, "%s/tmp/.%s_.file", getenv("SWWORK"), filename);

	return pub_usocket_udpbind(file);
}

sw_int_t udp_send(int fd, char *buffer, size_t bufferlen, const char *name)
{
	char	filename[512];
	
	if (fd < 0 || name == NULL || name[0] == '\0')
	{
		return -1;
	}
	
	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/tmp/.%s_.file", getenv("SWWORK"), name);

	return  pub_usocket_udpsend(fd, filename, buffer, bufferlen);
}

sw_int_t udp_recv(int fd, char *buffer, int bufferlen)
{
	return pub_usocket_udprecv(fd, 3, buffer, bufferlen);
}

