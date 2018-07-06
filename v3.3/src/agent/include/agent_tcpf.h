#ifndef __AGENT_TCPF_H__
#define __AGENT_TCPF_H__

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "stdlib.h"
#include "string.h"
#include "sys/select.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "sys/stat.h"
#include "netinet/in.h"
#include "netinet/tcp.h"
#include "arpa/inet.h"
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/errno.h>
#include "pub_log.h"
#include "pub_vars.h"

#ifdef SCO
#define socklen_t size_t
#endif


#define WAITSECONDS 20 

#define	RANGE		(128)
#define FNAME_LEN	(256)
#define FTYPE_LEN	(2)
#define	FCONTENT_LEN	(8)
#define	FLAG_LEN	(1)
#define END_FLAG	(0xff)
#define	NOT_END		(0x7f)

struct sw_istack_s
{
	int	cnt;		/*统计所有的文件、空目录数量*/
	int	index;		/*栈顶索引*/
	int	size;		/*栈空间大小*/
	int	*data;		/*栈数据*/
	char	filename[256];	
	FILE	*fp;		/*存放遍历文件、空目录名字列表*/
};

typedef struct sw_istack_s sw_istack_t;

#endif
