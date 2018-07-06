#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "anet.h"
#include "pub_log.h"

#if defined(SOLARIS)
#include <sys/sockio.h>
#endif

static void anet_set_error(char *err, const char *fmt, ...)
{
	va_list	ap;
	
	if (err == NULL)
	{
		return ;
	}
	
	va_start(ap, fmt);
	vsnprintf(err, ANET_ERR_LEN, fmt, ap);
	va_end(ap);
}

int anet_nonblock(char *err, int fd)
{
	int	flags = 0;
	
	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
	{
		anet_set_error(err, "fcntl(F_GETFL):[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		anet_set_error(err, "fcntl(F_SETFL, O_NONBLOCK):[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	return ANET_OK;
}

int anet_keepalive(char *err, int fd, int interval)
{
	int	val = 1;
	
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
	{
		anet_set_error(err, "setsockopt SO_KEEPALIVE:[%s]", strerror(errno));
		return ANET_ERR;
	}

#ifdef __linux__
	val = interval;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0)
	{
		anet_set_error(err, "setsockopt TCP_KEEPIDLE:[%s]\n", strerror(errno));
		return ANET_ERR;
	}
	
	val = interval / 3;
	if (val == 0)
	{
		val = 1;
	}
	
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0)
	{
		anet_set_error(err, "setsockopt TCP_KEEPINTVL:[%s]\n", strerror(errno));
		return ANET_ERR;
	}
	
	val = 3;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0)
	{
		anet_set_error(err, "setsockopt TCP_KEEPCNT:[%s]", strerror(errno));
		return ANET_ERR;
	}
#endif
	
	return ANET_OK;
}

static int anet_set_tcp_nodelay(char *err, int fd, int val)
{
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
	{
		anet_set_error(err, "setsockopt TCP_NODELAY:[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	return ANET_OK;
}

int anet_enable_tcp_nodelay(char *err, int fd)
{
	return anet_set_tcp_nodelay(err, fd, 1);
}

int anet_disable_tcp_nodelay(char *err, int fd)
{
	return anet_set_tcp_nodelay(err, fd, 0);
}

int anet_set_sendbuffer(char *err, int fd, int buffsize)
{
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1)
	{
		anet_set_error(err, "setsockopt SO_SNDBUF:[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	return ANET_OK;
}

int anet_set_recvbuffer(char *err, int fd, int buffsize)
{
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof(buffsize)) == -1)
	{
		anet_set_error(err, "setsockopt SO_SNDBUF:[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	return ANET_OK;
}
int anet_tcp_keepalive(char *err, int fd)
{
	int	yes = 1;
	
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1)
	{
		anet_set_error(err, "setsockopt SO_KEEPALIVE:[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	return ANET_OK;
}

int anet_generic_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len, int flags)
{
	int	rc = 0;
	struct addrinfo	hints, *info;
	
	memset(&hints, 0x0, sizeof(hints));
	
	if (flags & ANET_IP_ONLY)
	{
		hints.ai_flags = AI_NUMERICHOST;
	}
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	rc = getaddrinfo(host, NULL, &hints, &info);
	if (rc != 0)
	{
		anet_set_error(err, "%s", gai_strerror(rc));
		return ANET_ERR;
	}
	
	if (info->ai_family == AF_INET)
	{
		struct sockaddr_in	*sa = (struct sockaddr_in *)info->ai_addr;
		inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
	}
	else
	{
		struct sockaddr_in6	*sa = (struct sockaddr_in6 *)info->ai_addr;
		inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
	}
	freeaddrinfo(info);
	
	return ANET_OK;
}

int anet_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len)
{
	return anet_generic_resolve(err, host, ipbuf, ipbuf_len, ANET_NONE);
}

int anet_resolve_ip(char *err, char *host, char *ipbuf, size_t ipbuf_len)
{
	return anet_generic_resolve(err, host, ipbuf, ipbuf_len, ANET_IP_ONLY);
}

static int anet_set_reuseaddr(char *err, int fd)
{
	int	yes = 1;
	
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	{
		anet_set_error(err, "setsockopt SO_REUSEADDR:[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	return ANET_OK;
}

static int anet_create_socket(char *err, int domain)
{
	int	sockid = 0;
	
	sockid = socket(domain, SOCK_STREAM, 0);
	if (sockid == -1)
	{
		anet_set_error(err, "socket:[%s]", strerror(errno));
		return ANET_ERR;
	}
	
	if (anet_set_reuseaddr(err, sockid) == ANET_ERR)
	{
		close(sockid);
		return ANET_ERR;
	}
	
	return sockid;
}

#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1
static int anet_tcp_generic_connect(char *err, char *addr, int port, int flags)
{
	int	rc = 0;
	int	sockid = ANET_ERR;
	char	portstr[8];
	struct addrinfo	hints, *servinfo, *p;
	
	memset(portstr, 0x0, sizeof(portstr));
	memset(&hints, 0x0, sizeof(hints));
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	snprintf(portstr, sizeof(portstr), "%d", port);
	rc = getaddrinfo(addr, portstr, &hints, &servinfo);
	if (rc != 0)
	{
		anet_set_error(err, "%s", gai_strerror(rc));
		return ANET_ERR;
	}
	
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		sockid = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockid == -1)
		{
			continue;
		}
		
		if (anet_set_reuseaddr(err, sockid) == ANET_ERR)
		{
			goto error;
		}
	
		if ((flags & ANET_CONNECT_NONBLOCK) && anet_nonblock(err, sockid) != ANET_OK)
		{
			goto error;
		}
		
		if (connect(sockid, p->ai_addr, p->ai_addrlen) == -1)
		{
			if (errno == EINPROGRESS && flags & ANET_CONNECT_NONBLOCK)
			{
				goto end;
			}
			close(sockid);
			sockid = ANET_ERR;
			continue;
		}
		
		goto end;
	}
	
	if (p == NULL)
	{
		anet_set_error(err, "Creating socket:[%s]", strerror(errno));
	}

error:
	if (sockid != ANET_ERR)
	{
		close(sockid);
		sockid = ANET_ERR;
	}

end:
	freeaddrinfo(servinfo);
	
	return sockid;
}

int anet_tcp_connect(char *err, char *addr, int port)
{
	return anet_tcp_generic_connect(err, addr, port, ANET_CONNECT_NONE);
}

int anet_tcp_nonblock_connect(char *err, char *addr, int port)
{
	return anet_tcp_generic_connect(err, addr, port, ANET_CONNECT_NONBLOCK);
}

int anet_unix_generic_connect(char *err, char *path, int flags)
{
	int	sockid = 0;
	struct sockaddr_un	sa;
	
	memset(&sa, 0x0, sizeof(sa));
	
	sockid = anet_create_socket(err, AF_UNIX);
	if (sockid == ANET_ERR)
	{
		return ANET_ERR;
	}
	
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
	if ((flags & ANET_CONNECT_NONBLOCK) && anet_nonblock(err, sockid) != ANET_OK)
	{
		return ANET_ERR;
	}
	
	if (connect(sockid, (struct sockaddr *)&sa, sizeof(sa)) == -1)
	{
		if (errno == EINPROGRESS && flags & ANET_CONNECT_NONBLOCK)
		{
			return sockid;
		}
	
		anet_set_error(err, "connect:[%s]", strerror(errno));
		close(sockid);
		return ANET_ERR;
	}
	
	return sockid;
}

int anet_unix_connect(char *err, char *path)
{
	return anet_unix_generic_connect(err, path, ANET_CONNECT_NONE);
}

int anet_unix_nonblock_connect(char *err, char *path)
{
	return anet_unix_generic_connect(err, path, ANET_CONNECT_NONBLOCK);
}

int anet_read(int fd, void *vptr, size_t n)
{
	int 	times = 0;
	size_t	nleft = n;
	ssize_t	nread = 0;
	char	*ptr = vptr;
	
	while (nleft > 0)
	{
		nread = read(fd, ptr, nleft);
		if (nread < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			if (errno == EAGAIN && times < 5)
			{
				times++;
				usleep(1000);
				continue;
			}
			return n - nleft;
		}
		else if (nread == 0)
		{
			break;
		}
		
		nleft -= nread;
		ptr += nread;
	}
	
	return n - nleft;
}

int anet_readn(int fd, void *vptr, size_t n)
{
	int 	times = 0;
	size_t	nleft = n;
	ssize_t	nread = 0;
	char	*ptr = vptr;
	
	while (nleft > 0)
	{
		nread = read(fd, ptr, nleft);
		if (nread < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			if (errno == EAGAIN && times < 5)
			{
				times++;
				usleep(1000);
				continue;
			}
			return n - nleft;
		}
		else if (nread == 0)
		{
			break;
		}
		
		nleft -= nread;
		ptr += nread;
	}
	
	return n - nleft;
}

int anet_write(int fd, void *vptr, size_t n)
{
	int 	times = 0;
	size_t	nleft = n;
	ssize_t	nwritten = 0;
	char	*ptr = vptr;
	
	while (nleft > 0)
	{
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0)
		{
			if (nwritten < 0 && errno == EINTR)
			{	
				continue;
			}
			if (errno == EAGAIN && times < 5)
			{
				times++;
				usleep(1000);
				continue;
			}
			return -1;
		}
		
		nleft -= nwritten;
		ptr += nwritten;
	}
	
	return n;
}

int anet_writen(int fd, void *vptr, size_t n)
{
	int 	times = 0;
	size_t	nleft = n;
	ssize_t	nwritten = 0;
	char	*ptr = vptr;
	
	while (nleft > 0)
	{
		nwritten = write(fd, ptr, nleft);		
		if (nwritten <= 0)
		{
			if (nwritten < 0 && errno == EINTR)
			{	
				continue;
			}
			if (errno == EAGAIN && times < 5)
			{
				times++;
				usleep(1000);
				continue;
			}
			return -1;
		}
		
		nleft -= nwritten;
		ptr += nwritten;
	}
	
	return n;
}

static int anet_listen(char *err, int sockid, struct sockaddr *sa, socklen_t len, int backlog)
{
	if (bind(sockid, sa, len) == -1)
	{
		anet_set_error(err, "Bind:[%s]", strerror(errno));
		close(sockid);
		return ANET_ERR;
	}
	
	if (listen(sockid, backlog) == -1)
	{
		anet_set_error("Listen:[%s]", strerror(errno));
		close(sockid);
		return ANET_ERR;
	}
	
	return ANET_OK;
}


static int anet_v6only(char *err, int sockid)
{
	int	yes = 1;
	
	if (setsockopt(sockid, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) == -1)
	{
		anet_set_error(err, "setsockopt:[%s]", strerror(errno));
		close(sockid);
		return ANET_ERR;
	}
	
	return ANET_OK;
}

static int _anet_tcp_server(char *err, int port, char *bindaddr, int af, int backlog)
{
	int	rc = 0;
	int	sockid = ANET_ERR;
	char	_port[6];
	struct addrinfo	hints, *servinfo, *p;
	
	memset(_port, 0x0, sizeof(_port));
	memset(&hints, 0x0, sizeof(hints));
	
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	snprintf(_port, 6, "%d", port);
	rc = getaddrinfo(bindaddr, _port, &hints, &servinfo);
	if (rc != 0)
	{
		anet_set_error(err, "%s", gai_strerror(rc));
		return ANET_ERR;
	}
	
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		sockid = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockid == -1)
		{
			continue;
		}
		
		if (af == AF_INET6 && anet_v6only(err, sockid) == ANET_ERR)
		{
			goto error;
		}
	
		if (anet_set_reuseaddr(err, sockid) == ANET_ERR)
		{
			goto error;
		}
		
		if (anet_listen(err, sockid, p->ai_addr, p->ai_addrlen, backlog) == ANET_ERR)
		{
			goto  error;
		}	
		{
			int	port = 0;
			char	ip[32];
			
			memset(ip, 0x0, sizeof(ip));
			anet_sockname(sockid, ip, sizeof(ip), &port);
			printf("sockid=[%d] ip=[%s] port=[%d]\n", sockid, ip, port);
		}
		
		goto end;
	}
	if (p == NULL)
	{
		anet_set_error(err, "Unable to bind socket:[%s]", strerror(errno));
		goto error;
	}

error:
	if (sockid != ANET_ERR)
	{
		close(sockid);
		sockid = ANET_ERR;
	}
end:
	freeaddrinfo(servinfo);
	
	return sockid;
}

int anet_tcp_server(char *err, int port, char *bindaddr, int backlog)
{
	return _anet_tcp_server(err, port, bindaddr, AF_INET, backlog);
}

int anet_tcp6_server(char *err, int port, char *bindaddr, int backlog)
{
	return _anet_tcp_server(err, port, bindaddr, AF_INET6, backlog);
}

int anet_unix_server(char *err, char *path, mode_t perm, int backlog)
{
	int	sockid = ANET_ERR;
	struct sockaddr_un	sa;
	
	unlink(path);
	memset(&sa, 0x0, sizeof(sa));
	
	sockid = anet_create_socket(err, AF_UNIX);
	if (sockid == ANET_ERR)
	{
		return ANET_ERR;
	}
	
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
	if (anet_listen(err, sockid, (struct sockaddr *)&sa, sizeof(sa), backlog) == ANET_ERR)
	{
		return ANET_ERR;
	}
	
	if (perm)
	{
		chmod(sa.sun_path, perm);
	}
	
	return sockid;
}

static int anet_generic_accept(char *err, int sockid, struct sockaddr *sa, socklen_t *len)
{
	int	fd = ANET_ERR;
	
	while (1)
	{
		fd = accept(sockid, sa, len);
		if (fd == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
		
			anet_set_error(err, "accept:[%s]", strerror(errno));
			return ANET_ERR;
		}
		break;
	}
	
	return fd;
}

int anet_tcp_accept(char *err, int sockid, char *ip, size_t len, int *port)
{
	int	fd = ANET_ERR;
	struct sockaddr_storage	sa;
	socklen_t	salen = sizeof(sa);
	
	fd = anet_generic_accept(err, sockid, (struct sockaddr *)&sa, &salen);
	if (fd == ANET_ERR)
	{
		return ANET_ERR;
	}
	
	if (sa.ss_family == AF_INET)
	{
		struct sockaddr_in	*s = (struct sockaddr_in *)&sa;
		if (ip)
		{
			inet_ntop(AF_INET, (void *)&(s->sin_addr), ip, len);
		}
		
		if (port)
		{
			*port = ntohs(s->sin_port);
		}
	}
	else
	{
		struct sockaddr_in6	*s = (struct sockaddr_in6 *)&sa;
		if (ip)
		{
			inet_ntop(AF_INET6, (void *)&(s->sin6_addr), ip, len);
		}
	
		if (port)
		{
			*port = ntohs(s->sin6_port);
		}
	}
	
	return fd;
}

int anet_unix_accept(char *err, int sockid)
{
	int	fd = ANET_ERR;
	struct sockaddr_un	sa;
	socklen_t	salen = sizeof(sa);
	
	fd = anet_generic_accept(err, sockid, (struct sockaddr *)&sa, &salen);
	if (fd == ANET_ERR)
	{
		return ANET_ERR;
	}
	
	return fd;
}

int anet_peer_tostring(int fd, char *ip, size_t len, int *port)
{
	struct sockaddr_storage	sa;
	socklen_t	salen = sizeof(sa);
	
	memset(&sa, 0x0, sizeof(sa));
	
	if (getpeername(fd, (struct sockaddr *)&sa, &salen) == -1)
	{
		if (port)
		{
			*port = 0;
		}
		ip[0] = '?';
		ip[1] = '\0';
		return ANET_ERR;
	}
	
	if (sa.ss_family == AF_INET)
	{
		struct sockaddr_in	*s = (struct sockaddr_in *)&sa;
		if (ip)
		{
			inet_ntop(AF_INET, (void *)&(s->sin_addr), ip, len);
		}
		
		if (port)
		{
			*port = ntohs(s->sin_port);
		}
	}
	else
	{
		struct sockaddr_in6	*s = (struct sockaddr_in6 *)&sa;
		if (ip)
		{
			inet_ntop(AF_INET6, (void *)&(s->sin6_addr), ip, len);
		}
		
		if (port)
		{
			*port = ntohs(s->sin6_port);
		}
	}
	
	return 0;
}

int anet_sockname(int fd, char *ip, size_t len, int *port)
{
	struct sockaddr_storage	sa;
	socklen_t	salen = sizeof(sa);
	
	memset(&sa, 0x0, sizeof(sa));
	
	if (getsockname(fd, (struct sockaddr *)&sa, &salen) == -1)
	{
		if (port)
		{
			*port = 0;
		}
		ip[0] = '?';
		ip[1] = '\0';
		return ANET_ERR;
	}
	
	if (sa.ss_family == AF_INET)
	{
		struct sockaddr_in	*s = (struct sockaddr_in *)&sa;
		if (ip)
		{
			inet_ntop(AF_INET, (void *)&(s->sin_addr), ip, len);
		}
		
		if (port)
		{
			*port = ntohs(s->sin_port);
		}
	}
	else
	{
		struct sockaddr_in6	*s = (struct sockaddr_in6 *)&sa;
		if (ip)
		{
			inet_ntop(AF_INET6, (void *)&(s->sin6_addr), ip, len);
		}
		
		if (port)
		{
			*port = ntohs(s->sin6_port);
		}
	}
	
	return 0;
}

int anet_get_hostip(char *outip)
{
        int     i = 0;
        int     sockid = 0;
        char    *ip = NULL;
        struct ifreq    buf[MAXINTERFACES];
        struct ifconf   ifc;

        if ((sockid = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
                perror("socket(AF_INET, SOCK_DGRAM, 0)");
                return -1;
        }

        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t) buf;
        if (ioctl(sockid, SIOCGIFCONF, (char *)&ifc) == -1)
        {
                perror("SIOCGIFCONF ioctl");
                close(sockid);
                return -1;
        }

        for (i = 0; i < MAXINTERFACES; i++)
        {
                if (ioctl(sockid, SIOCGIFADDR, (char *)&buf[i]) < 0)
                {
                        continue;
                }
                ip = (char*)inet_ntoa(((struct sockaddr_in*) (&buf[i].ifr_addr))->sin_addr);
                if (strncmp(ip, "127.0.0.", 8) != 0)
                {
                        strcpy(outip, ip);
                        break;
                }
        }
        close(sockid);

        return 0;
}
