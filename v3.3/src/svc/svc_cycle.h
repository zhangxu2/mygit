#ifndef _SW_SVC_H
#define _SW_SVC_H

#include "pub_type.h"
#include "pub_pool.h"
#include "pub_shm_ex.h"
#include "cycle.h"
#include "pub_cfg.h"
#include "pub_buf.h"
#include <dlfcn.h>
#include "pub_express.h"
#include "select_event.h"
#include "pub_usocket.h"
#include "pub_log.h"
#include "procs.h"
#include "msg_trans.h"
#include "pub_vars.h"
#include "pub_route.h"
#include "pub_db.h"
#include <sys/wait.h>
#include "param.h"
#include "prdt_arg.h"
#include "pub_stack.h"
#include "pub_hash.h"
#include "pub_filter.h"

#define SW_SVC_DONE	0
#define SW_SVC_DOING	1
#define MAX_SVR_CNT	10
#define	MAX_SVC_CNT	200
#define	MAX_CACHE_CNT	20
#define MIN_PROC_CNT	3
#define	MAX_PROC_CNT	100
#define	SVC_RELOAD_DEFAULT	0
#define	SVC_STATUS_DEFAULT	1
#define SVRLVL_STATUS_DEFAULT	0
#define MAX_OPEN_RETRY	20
#define MAX_BUS_BAK_SIZE	8192 * 1024

#define SECTION_BEFORE  0     /***DECLARE֮ǰ��û���õ�����****/
#define SECTION_DECLARE 1
#define SECTION_BEGIN   2
#define	SECTION_CMPBEG	3

#define MAX_CMP_LEVEL 256
#define SYS_TRACE_NAME	"SYS_SEQS_NAME"
#define DEFAULT_TRACE_NAME	"SEQS_DEFAULT_NAME"

#define BLOCK_BEGIN	1
#define BLOCK_END	2

#define	DB_MODE_UDBC	0
#define DB_MODE_ESQL	1

typedef int (* sw_db_init_pt)();
typedef int (* sw_db_connect_pt)();
typedef int (* sw_db_commit_pt)();
typedef int (* sw_db_rollback_pt)();
typedef int (* sw_db_close_pt)();
typedef int (* sw_db_getstatus_pt)();

typedef struct
{
	int	status;         /*** service status ***/
	sw_char_t	trace_flag;
	sw_char_t	name[NAME_LEN]; /*** service name ***/
	sw_char_t	lib[NAME_LEN];
}sw_svc_t;

typedef struct
{
	sw_int32_t	min;			/*** min service ***/
	sw_int32_t	max;			/*** max service?**/
	sw_int32_t	index;
	sw_int32_t	status;			/*** server used ***/
	sw_int32_t	reload;			/*** ***/
	sw_int32_t	use_dlcache;		/*** use cached ***/
	sw_int32_t	cache_cnt;		/*** ***/
	sw_int32_t	use_svrlevel;
	sw_int64_t	exptime;
	sw_int32_t	scantime;               /* ɨ����ʱ�� */
	sw_char_t	svr_name[NAME_LEN];	/*��ǰserver���Ƚ��̵ķ�����	*/
	sw_int32_t 	cur_child_cnt;		/*��ǰ�������ӽ��̸���		*/
	sw_int32_t	exp_child_cnt;		/*��ǰ�������ӽ�����,��ʼֵΪ��С�ӽ�����*/
	sw_int32_t	use_db;			/*�Ƿ��������ݿ�*/
	sw_int32_t	service_cnt;
	sw_char_t	db_conn_name[NAME_LEN*8];	/*** dbconn name ***/
	sw_int32_t	db_mode;                /*** ���ݿ����ӷ�ʽ 0:ͳһ���ݷ���ģʽ 1:ESQL��ʽ ***/
	sw_char_t	seqs_name[128];
	sw_char_t	flow_path[128];
	sw_char_t	work_path[128];
	sw_svc_t	services[MAX_SVC_CNT];
	void	*db_handle;
	sw_db_init_pt	db_init;
	sw_db_connect_pt	db_connect;
	sw_db_commit_pt	db_commit;
	sw_db_close_pt	db_close;
	sw_db_rollback_pt	db_rollback;
	sw_db_getstatus_pt	db_getstatus;
}sw_svr_t;
sw_svr_t	*g_svr;
sw_int64_t	g_exptime;

struct
{
	sw_int32_t	conn;
	sw_int64_t	start_time;/* ���ݿ�����ʱ�� ***/
}g_db_conn;

typedef struct
{
	sw_int_t	svr_cnt;
}sw_svr_cache_head_t;

typedef struct
{
	sw_svr_cache_head_t	head;
	sw_svr_t	svr[1];
}sw_svr_cache_t;

typedef struct
{
	sw_cycle_t	base;/*Base class object,This object must be the first member of child class.*/
	sw_fd_t	fd_out[MAX_SVR_LEVEL_CNT];          /*socket used for send/receive message to/from other model.*/
	sw_fd_t	msg_out[MAX_SVR_LEVEL_CNT];          /*socket used for send/receive message to/from other model.*/
	sw_int32_t level[MAX_SVR_LEVEL_CNT];
	sw_fd_t	cmd_fd;          /*socket used for send/receive message to/from other model.*/
	sw_shm_t	shm;
	sw_int_t	proc_index;
	sw_int32_t	list_size;
	sw_char_t	proc_name[SW_NAME_LEN];
	sw_char_t	*flow_addr;
	sw_char_t	*trace_addr;
	sw_char_t	prdt[NAME_LEN];
	sw_fd_set_t	*svc_fds;
	sw_svr_cache_t	*svrs;       /*** ***/
}sw_svc_cycle_t;

typedef struct
{
	char	err_name[64];
	char	err_code[8];
	char	up_name[64];
}sw_svc_cmp_t;

sw_svc_cmp_t	g_cmp_info[MAX_CMP_LEVEL];

int	g_idx;
int	g_complete;
int	g_exiting;
char	g_svc[NAME_LEN];
char	g_prdt[NAME_LEN];
sw_cmd_t	g_cmd;
sw_global_path_t	*g_path;

typedef struct
{
	int	flow_cnt;
}sw_flow_head_t;

typedef struct
{
	sw_flow_head_t	head;
	NTFLOW	flow[1];
}sw_flow_t;
TFLOW	flow;
sw_flow_t	*g_nflow;

typedef struct
{
	int	status; /*** ������״̬ ***/
	sw_int_t	mtype; 
	sw_int_t	errcode;    /*** ������ ***/
	sw_int64_t	start_time; /*** ����ʼʱ�� ***/
	sw_int64_t	end_time;   /*** �������ʱ�� ***/
	sw_int32_t	req_cnt;    /*** CALL CALLLSN��������� ***/
	sw_int64_t	trace_no;   /*** ������ˮ ***/
	sw_char_t	lsn_name[NAME_LEN]; /*** �������� ***/
	sw_char_t	tx_code[32]; /*** ������ ***/
	sw_char_t	prdt[NAME_LEN];
	sw_char_t	svr[NAME_LEN];
	sw_char_t	svc[NAME_LEN];
}sw_svc_trace_item_t;

typedef struct
{
	sw_int_t	curr_cnt;  /*** ��ǰ���������� ***/
	sw_int_t	total_cnt; /*** ������������ ***/
}sw_svc_trace_head_t;

typedef struct
{
	sw_svc_trace_head_t	head;
	sw_svc_trace_item_t	trace[1];
}sw_svc_trace_t;

sw_svc_trace_t	*g_trace;

typedef struct
{
	sw_int_t	errcode;    /*** ������ ***/
	sw_int_t	total;      /*** �ܱ��� ***/
	sw_int_t	failed;     /*** ʧ�ܱ��� ***/
	sw_int_t	success;    /*** �ɹ����� ***/
	sw_int_t	err[8];     /*** ������� ***/
	sw_char_t	tx_code[32];/*** ������ ***/
	sw_int64_t	last_time;  /*** �ϴ�Ԥ��ʱ�� ***/
	sw_int64_t	first_time; /*** ���μ�¼�ĵ�һ��ʧ�ܽ���ʱ�� ***/
}sw_svc_trace_info_t;

typedef enum
{
	E_SUCCESS = 0,
	E_LSNOUT,
	E_DOERR,
	E_FAILED
}err_type_t;

typedef struct node
{
	sw_svc_trace_item_t	item;
	struct node	*next;
}node_t, *list_t;

sqstack_t	*g_afmt_stack;
sw_xmltree_t	*g_arule_xml;

#define MAX_LIB_NAME_LEN	256
#define MAX_FUN_NAME_LEN	128
#define MAX_LNAME_LEN	(MAX_LIB_NAME_LEN + MAX_FUN_NAME_LEN)
#define MAX_DISTINCT_LIB	1024

typedef int (*sw_dlfun_pt)(void);

typedef struct
{
	void	*handle;
	sw_dlfun_pt	fun;
	char	libname[MAX_LIB_NAME_LEN];
	char	funname[MAX_FUN_NAME_LEN];
}dlnode_t;


typedef struct
{
	char conn_name[NAME_LEN];
}dbconn_cache_t;

int	g_dbconn_curr;
int	g_dbconn_cnt;
dbconn_cache_t	g_dbconn_t[MAX_CACHE_CNT];

int dlcache_init();
int dlcache_free(char *libname, char *funname);
int dlcache_clear();
sw_dlfun_pt dlcache_get_dlfun_by_name(char *libname, char *funname);

SW_PUBLIC sw_int_t svc_cycle_init(sw_svc_cycle_t* cycle, sw_char_t* name, 
	sw_int32_t module_type, sw_char_t* err_log, sw_char_t* dbg_log, sw_char_t *prdt);
SW_PUBLIC sw_int_t svc_cycle_run(sw_svc_cycle_t* cycle);
SW_PUBLIC sw_int_t svc_cycle_destroy(sw_svc_cycle_t* cycle);
SW_PUBLIC sw_int_t svc_init(sw_svc_cycle_t *cycle);
SW_PUBLIC sw_int_t svc_create_svr(sw_svc_cycle_t *cycle);
SW_PUBLIC sw_int_t svc_work(sw_svc_cycle_t *cycle, sw_int_t index, sw_int_t id);
SW_PUBLIC sw_int_t svc_handle_control_cmd(sw_svc_cycle_t *cycle);
SW_PUBLIC sw_int_t svc_child_register(sw_svc_cycle_t *cycle, sw_int32_t status);
SW_PUBLIC sw_int_t svc_cycle_child_destroy(sw_svc_cycle_t *cycle);
SW_PUBLIC sw_int_t svc_cycle_destroy(sw_svc_cycle_t *cycle);
SW_PUBLIC sw_int32_t svc_father_register(sw_svc_cycle_t *cycle, sw_int32_t status);

#endif
