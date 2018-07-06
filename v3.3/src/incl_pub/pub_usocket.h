#ifndef  __PUB_USOCKET_H__
#define  __PUB_USOCKET_H__

#include "pub_type.h"
#include "pub_log.h"

sw_int_t ipc_bind(char *filename, int lsncnt);
sw_int_t pub_usocket_accept(int lsnfd, uid_t *puid);
sw_int_t ipc_conn(char *cliname, char *svrname);

sw_int_t  pub_usocket_udpbind(char *svrfile);
sw_int_t pub_usocket_udprecv(int clifd, int second, char *buff, int length);
sw_int_t  pub_usocket_udpsend(int clifd, char *filename,char *sBuf,int iLen );

sw_int_t udp_bind(const char *pName);
sw_int_t udp_send(int fd,char *pBuf, size_t iLen, const char *pName);
sw_int_t udp_recv(int fd,char *pBuf,int iLen);


#endif
