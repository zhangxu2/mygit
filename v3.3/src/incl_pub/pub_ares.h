#ifndef	__PUB_ARES_H__
#define	__PUB_ARES_H__

#include "pub_type.h"

typedef struct
{
	int	use;
	int	port;
	int	seqs_use;
	int	mon_use;
	int	mtype_use;
	int	wait_time;
	char	ip[IP_ADDR_MAX_LEN];
} ares_cfg_t;
ares_cfg_t g_ares_cfg;

typedef struct
{
	int     fd;
	int     port;
	int     err;
	int	is_init;
	int	wait_time;
	char    ip[32];
	pid_t   pid;
	char    errstr[256];
} ares_cycle_t;
ares_cycle_t	g_ares_cycle;

#define g_use_ares	(g_ares_cfg.use == 1)
#define g_ares_fd	g_ares_cycle.fd
#define g_ares_ip	g_ares_cycle.ip
#define g_ares_port	g_ares_cycle.port
#define g_ares_pid	g_ares_cycle.pid
#define g_ares_is_init	g_ares_cycle.is_init
#define g_ares_wait_time g_ares_cycle.wait_time
#define g_in_ares	(g_use_ares && g_ares_is_init)
#define g_seqs_use_ares (g_ares_cfg.seqs_use == 1)
#define g_link_use_ares (1)
#define g_seqs_in_ares	(g_in_ares && g_seqs_use_ares)
#define g_mtype_use_ares (g_ares_cfg.mtype_use == 1)
#define g_mtype_in_ares	(g_in_ares && g_mtype_use_ares)
#define g_link_in_ares	(g_in_ares && g_link_use_ares)
#define g_mon_use_ares	(g_ares_cfg.mon_use == 1)
#define g_mon_in_ares	(g_in_ares && g_mon_use_ares)

typedef enum
{
	ARES_INIT = 0,
	MTYPE_NEW,
	MTYPE_DELETE,
	MTYPE_GET_MAX,
	MTYPE_GET_FLAG_DESC,
	MTYPE_SET_TIME,
	MTYPE_SET_INFO,
	SEQS_BP_NEW,
	SEQS_BUS_NEW,
	SEQS_GET_SYSSTS,
	SEQS_SET_SYSSTS,
	SEQS_CHNG_DATE,
	SEQS_GET_SYSDATE,
	SEQS_GET_DATE,
	SEQS_ADD_BUSINESS_TRACE,
	SEQS_POL_UPDATE_ESWITCH_DATE,
	SEQS_IS_DATE_LOCKED,
	SEQS_SET_DATE_LOCK,
	SEQS_SET_DATE_UNLOCK,
	SEQS_SET_BSN_FLOW,
	SEQS_SET_BP_FLOW,
	SEQS_SET_PLAT_FLOW,
	SEQS_PRINT_PLAT_FLOW_NEW,
	SEQS_PRINT_BSN_FLOW_NEW,
	SEQS_PRINT_BSN_FLOW_ALL_NEW,
	LINKLIST_SAVE,
	LINKLIST_UPD,
	LINKLIST_LOAD,
	LINKLIST_DEL_BY_KEY,
	LINKLIST_DEL_BY_MTYPE,
	ARES_LINK,
	MTYPE_PRINT,
	TRACE_CREATE_INFO,
	TRACE_INSERT_SVR,
	TRACE_INSERT,
	ADMIN_TOP,
	ADMIN_STEP,
	SEQS_BP_TRACE_DATE,
	TRACE_WRITE_MONITOR,
	LINKLIST_DEL_BY_MTYPE_AND_KEY,
	MTYPE_RENEW,
	CMD_TOTAL

} cmd_type_e;

#define ARG_OK	"00"
#define ARG_ERR	"99"
#define MAX_BUF_LEN (1024*12)
#define MAX_ARES_RECV_BUF_LEN (256*1024)
#define ARES_SEP "~"
#define ARES_SEP_LEN strlen(ARES_SEP) 

int ares_init();
int ares_comm(int fd, char *buf, size_t size, int type);
int ares_recv(int fd, char *vptr);
int ares_send(int fd, char *vptr, size_t size, int type);
int ares_link(int fd);
void ares_close_fd();
void ares_set_ares_cfg(ares_cfg_t ares);
int ares_send_cfg();
int ares_top(int num);
int ares_step(sw_int64_t trace_no);

#endif /*#ifdef	__PUB_ARES__*/

