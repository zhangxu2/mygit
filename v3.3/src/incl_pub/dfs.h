#ifndef __DFS_H__
#define __DFS_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>

#define dfs_memzero(buf, n)	memset(buf, 0, n)
#define	DFS_ALIGNMENT	sizeof(unsigned long)
#define	DFS_OK	0
#define DFS_EXIST 2
#define DFS_ERROR	-1
#define DFS_AGAIN	-2
#define DFS_BUSY         -3
#define DFS_DONE         -4
#define DFS_DECLINED     -5
#define DFS_ABORT        -6
#define DFS_INVALID_PID  -1
#define SID_LEN 	32
#define DFS_MAX_NAME_LEN	32
#define SQN_LEN		32
#define NAME_LEN  	32
#define MAX_SEQS_CNT	20
#define SW_NAME_LEN		32
#define DFS_MAX_FILENAME_LEN	128

#define dfs_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define dfs_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define dfs_free	free

typedef struct dfs_array_s      dfs_array_t;

#define dfs_signal_helper(n)     SIG##n
#define dfs_signal_value(n)      dfs_signal_helper(n)

#define dfs_value_helper(n)	#n
#define dfs_value(n)		dfs_value_helper(n)

#define dfs_random               random

#define DFS_STOP_SIGNAL		ABRT
#define DFS_SHUTDOWN_SIGNAL      QUIT
#define DFS_TERMINATE_SIGNAL     TERM
#define DFS_NOACCEPT_SIGNAL      WINCH
#define DFS_RECONFIGURE_SIGNAL   HUP

#define DFS_REOPEN_SIGNAL        USR1
#define DFS_CHANGEBIN_SIGNAL     USR2

#define dfs_cdecl
#define dfs_libc_cdecl

typedef pid_t   dfs_pid_t;
typedef int	dfs_fd_t;
typedef int	dfs_err_t;
typedef int	dfs_socket_t;
typedef intptr_t    dfs_int_t;
typedef uintptr_t   dfs_uint_t;
typedef intptr_t    dfs_flag_t;
typedef char		dfs_char_t;
typedef int32_t		dfs_int32_t;
typedef int64_t		dfs_int64_t;
typedef u_char      dfs_uchar_t;
typedef int16_t		dfs_int16_t;

typedef struct dfs_cycle_s	dfs_cycle_t;
typedef struct dfs_adm_cycle_s	dfs_adm_cycle_t;
typedef struct dfs_conf_s	dfs_conf_t;
typedef struct dfs_event_s       dfs_event_t;
typedef struct dfs_connection_s  dfs_connection_t;
typedef int (*dfs_method_pt)(dfs_cycle_t *cycle);
typedef void (*dfs_event_handler_pt)(dfs_event_t *ev);
typedef void (*dfs_connection_handler_pt)(dfs_connection_t *c);
typedef ssize_t (*dfs_recv_pt)(dfs_connection_t *c, u_char *buf, size_t size);
typedef ssize_t (*dfs_send_pt)(dfs_connection_t *c, u_char *buf, size_t size);

extern dfs_uint_t dfs_process;
#define DFS_PROCESS_WORKER 3
extern volatile dfs_cycle_t *dfs_cycle;
extern volatile dfs_conf_t *dfs_conf;

#define DFS_SOCKADDRLEN	sizeof(struct sockaddr_un)

#define dfs_mutex_trylock(m)  DFS_OK
#define dfs_mutex_lock(m)
#define dfs_mutex_unlock(m)
#define dfs_abs(value)       (((value) >= 0) ? (value) : - (value))
#define dfs_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define dfs_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define dfs_socket socket
#define dfs_nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define dfs_blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define dfs_strcmp(s1, s2)  strcmp((const char *) s1, (const char *) s2)
#define dfs_strlen(s)       strlen((const char *) s)
#define DFS_LISTEN_BACKLOG  511
#define DFS_DEFAULT_CONNECTIONS  512
#define DFS_MAX_PROCESSES         1024
#define DFS_DEFAULT_SCANTIME	300
#define DFS_DEFAULT_THREADCNT	3
#define DFS_DEFAULT_RCVBUF_SIZE	65536
#define DFS_DEFAULT_SNDBUF_SIZE	65536

#endif