#ifndef ANET_H
#define ANET_H

#define ANET_OK 0
#define ANET_ERR -1
#define ANET_ERR_LEN 256

/* Flags used with certain functions. */
#define ANET_NONE 0
#define ANET_IP_ONLY (1<<0)

#if defined(__sun)
#define AF_LOCAL AF_UNIX
#endif

#define MAXINTERFACES 16

int anet_tcp_connect(char *err, char *addr, int port);
int anet_tcp_nonblock_conenct(char *err, char *addr, int port);
int anet_unix_connect(char *err, char *path);
int anet_unix_nonblock_connect(char *err, char *path);
int anet_read(int fd, void *vptr, size_t n);
int anet_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len);
int anet_resolve_ip(char *err, char *host, char *ipbuf, size_t ipbuf_len);
int anet_tcp_server(char *err, int port, char *bindaddr, int backlog);
int anet_tcp6_server(char *err, int port, char *bindaddr, int backlog);
int anet_unix_server(char *err, char *path, mode_t per, int backlog);
int anet_tcp_accept(char *err, int serversock, char *ip, size_t len, int *port);
int anet_unix_accept(char *err, int serversock);
int anet_write(int fd, void *vptr, size_t n);
int anet_nonblock(char *err, int fd);
int anet_enable_tcp_nodelay(char *err, int fd);
int anet_disable_tcp_nodelay(char *err, int fd);
int anet_peer_tostring(int fd, char *ip, size_t len, int *port);
int anet_keeplive(char *err, int fd, int interval);
int anet_sockname(int fd, char *ip, size_t len, int *port);
int anet_get_hostip(char *outip);
int anet_writen(int fd, void *vptr, size_t n);
int anet_readn(int fd, void *vptr, size_t n);

#endif
