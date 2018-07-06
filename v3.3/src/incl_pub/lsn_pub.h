#ifndef __LSN_PUB_H__
#define __LSN_PUB_H__

#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pack.h"
#include "procs.h"
#include "cycle.h"
#include "pub_buf.h"
#include "pub_file.h"
#include "pub_usocket.h"
#include "select_event.h"
#include "pub_regex.h"
#include "pub_code.h"
#include "pub_time.h"
#include "msg_trans.h"
#include "pub_vars.h"
#include "pub_route.h"
#include "lsn_prdt.h"
#include "pub_shm_ex.h"
#include "mtype.h"

#define MAX_MSG_LEN	1024 * 1024 * 4
#define MSG_MAX_LEN		128 * 1024
#define LOG_MAX_OPEN_RETRY	32
#define SWNAMEMAXLEN		32
#define MQ_NUM			5
#define MAX_ROUTE_NUM	20
#define MAX_COMM_PROC_CNT	10
#define MAX_GROUP_BUF_LEN	MAX_MSG_LEN * MAX_COMM_PROC_CNT

#define MAX_NODE_CHANNEL 5 			/*�������������ͨѶ��·����	*/
#define PTHDNUM	10

#define SW_LSN_TIMEOUT	60 * 5
#define SLEEP_TIME              (8)
#define SW_EVENT_LOOP_EXIT	-2
#define SW_CONTINUE              1
#define SW_DELETE		 2
#define MAX_PACK_LEN	128*1024

#define O_SEND  0
#define O_RECV  1
#define MAX_QMGR_NUM    3

#define SENDMSGID "$send_msgid"
#define RECVMSGID "$recv_msgid"
#define SENDCORRID "$send_corrid"
#define RECVCORRID "$recv_corrid"

typedef enum
{
	DEL_REMOTE_LINK = 0,		/*ֻɾ��Զ��*/
	DEL_LOCAL_LINK,			/*ֻɾ������*/
	DEL_NULL_LINK,			/*linknull*/
	DEL_ALL_LINK			/*ɾ�����غ�Զ��*/
}del_link_t;

struct qm_s
{
	char	qname[64];
};

struct mqcfg_s
{
	int	use;
	int	qcnt;
	char	qmgr[64];
	int	proc_cnt;
	struct qm_s	qm[MQ_NUM];
};

struct mqcfg_s	g_mqcfg;
struct mqcfg_s	g_mqcfg_ext;

typedef struct
{
	sw_int32_t	count;
	struct
	{
		struct
		{
			sw_char_t	ip[256];	/*Ϊ����soapsc����http���ö�����Ϊ256λ*/
			sw_int32_t	port;
			sw_char_t	service[33];
			sw_fd_t		sockid;
			sw_int32_t	starttime;
			sw_int16_t	use;		/*0-δʹ��,1-ʹ��	*/
			sw_int32_t	reset;
		}remote,local;
		struct mqcfg_s	send, recv;
		sw_char_t	buftype[32];
	}group[MAX_NODE_CHANNEL];
}sw_comm_t;

typedef struct
{
	sw_char_t 	name[64];         	/*������ǩ	*/
	sw_char_t	comtype[32];		/*ͨѶ���� TCP MQ ...*/
	sw_char_t 	init[256];            	/*������ʼ�����*/
	sw_char_t 	destroy[256];		/*������Դ������*/
	sw_char_t 	interevt[256];          /* �ڲ��¼������� */
	sw_char_t	extcevt[256];		/*�ⲿ�¼�������*/
	sw_char_t	transcode[256];		/*ת��������*/
	sw_char_t	pkgdeal[256];		/* ���Ĵ��������*/
	sw_char_t	timeoutevt[256];	/* ��ʱ�¼�������*/
	sw_char_t	keepalive[256];		/* ��·ά�ֲ��*/	
	sw_char_t	httpscfg[256];          /* https֤�顢˽Կ�����ļ�·�� */
	sw_int32_t	socksndbufsize;		/* socket ��������С */
	sw_int32_t	sockrcvbufsize;		/* socket ��������С */
	sw_int32_t	procmax;		/* ��������������*/
	sw_int32_t	procmin;		/* ������С������*/
	sw_int32_t	conmax;			/* ���������*/
	sw_int32_t	timeout;		/*��ʱʱ��*/
	sw_char_t	data[64];		/* �ֽ������ȼ��㺯��*/
	sw_char_t	db_conn_name[128];	/*���������������ݿ�ʱ�ı�־*/
	sw_char_t	activemsg[128];		/*��·������Ϣ*/
	sw_int32_t	activetime;		/*����ʱ����*/
	sw_int32_t	scantime;		/* ɨ����ʱ�� */
	sw_comm_t 	comm;             	/*ͨѶ�����ṹ			*/
	sw_int32_t 	in_fd;			/*����ܵ�fd	*/
	sw_int32_t 	out_fd;			/*����ܵ�fd	*/
	sw_int32_t	issuper;		/* �Ƿ񳬼����� */
	sw_char_t	*shmaddr;		/*�򿪹����ڴ��ĵ�ַ*/
	sw_int32_t	in_msgid;
	sw_int32_t	out_msgid;
	sw_int32_t	fastclose;      /* recvʧ��ʱ����ǶԷ������ر�����,�Ƿ������ر�,1�����ر�,Ĭ��0 */
	sw_int32_t 	process_index;		/*��������*/
	sw_int32_t	sendtype;		/*���͡���������, MQ/TLQ ��*/
	sw_int32_t	group_index;
	sw_char_t	loadlib[MAX_NAME_LEN];  /*** SOAP��� ***/
	sw_char_t       pkgcfg[MAX_NAME_LEN];           /*���Ĵ���������*/
	sw_char_t       filedir[MAX_NAME_LEN * 2];      /*�ļ�·��*/
	sw_char_t       filedeal[MAX_NAME_LEN];         /* �ļ�������*/
	sw_char_t       mapdeal[MAX_NAME_LEN];          /* ����Ҫ��ӳ����*/
	sw_char_t       mapcfg[MAX_NAME_LEN];           /* ����Ҫ��ӳ������*/
	sw_char_t       factoranayly[MAX_NAME_LEN];     /* ����Ҫ�ط���*/
	sw_char_t       svrmap[MAX_NAME_LEN];           /* ����ӳ����*/
	sw_char_t       svrmapcfg[MAX_NAME_LEN];        /* ����ӳ������*/
	sw_char_t       denyservice[MAX_NAME_LEN];      /*�ܾ������Ĳ��*/
	sw_char_t       denycfg[MAX_NAME_LEN];          /* �ܾ����������ļ�*/
	sw_char_t	startfunc[MAX_NAME_LEN];	/* Ԥ������ */
}sw_lsn_config_t;

typedef struct
{
	sw_fd_t  	sockid;		        /*��·id			*/
	sw_int_t  	mtype;			/*mtypeΨһ��ʶ			*/
	sw_int64_t 	trace_no;			/*ƽ̨��ˮ			*/
	sw_int32_t   	cmd_type;			/*ͨѶָ������			*/
	sw_int32_t	saf_type;		/*�洢ת������			*/
	sw_int32_t  	timeout;
	sw_int32_t  	start_time;		/*����ʼʱ���		*/
	sw_int32_t   	use;			/*��ǰ��·�Ƿ����		*/
	sw_char_t	remote_ip[64];
	sw_char_t 	data_info[1024];		/*������Ϣ			*/
	sw_char_t	prdt[32];
	sw_char_t	def_name[32];
	sw_route_t	route;
	sw_cmd_t	cmd;
}sw_link_info_t;

typedef int (*sw_lsn_deal_handle_pt)(void *);
typedef int (*sw_des_handle_pt)(sw_loc_vars_t *, sw_buf_t *locbuf, int);
typedef  int (*sw_soap_fun_pt)(sw_loc_vars_t *, sw_buf_t *locbuf);

typedef struct
{
	sw_des_handle_pt		des_handler;
	sw_lsn_deal_handle_pt	init_handler;
	sw_lsn_deal_handle_pt	destroy_handler;
	sw_event_handler_pt	timeout_handler;
	sw_event_handler_pt     deal_pkg_handler; 
}sw_lsn_interface_t;


typedef struct
{
	sw_int_t	route_cnt;
}sw_route_cache_head_t;

typedef struct
{
	sw_route_cache_head_t	head;
	sw_route_t	route[1];
}sw_route_cache_t;

typedef struct 
{
	sw_cycle_t		base;
	sw_fd_t			udp_fd;
	sw_fd_t			lsn_fd;
	sw_fd_t			accept_fd;
	sw_char_t		proc_name[32];
	void	*param;
	sw_pkg_cache_list_t	cache_buf;
	sw_lsn_config_t		lsn_conf;
	sw_link_info_t		*link_info;
	sw_link_list_t		*link_list;
	sw_fd_set_t		*lsn_fds;
	sw_int_t		list_size;
	void	*handle;
	sw_lsn_interface_t	handler;
	sw_int32_t	semid;
	sw_shm_t	shm;
	sw_shm_t	buf_shm;
	sw_shm_t	link_shm;
	sw_shm_t	sock_shm;
	sw_shm_t        route_shm;
	sw_chnl_t	chnl;
	sw_route_cache_t	*routes;
	sw_route_t	route;
}sw_lsn_cycle_t;

int	g_trace_flag;
char	g_env_buf[128];

typedef struct
{
	sw_fd_t		sockid;
	sw_int32_t	use;
	sw_int32_t	port;
	sw_char_t	ip[64];
}sw_addr_info_t;

#define MAX_MACHINE_NUM	30
sw_addr_info_t g_mp_addr[MAX_MACHINE_NUM];

SW_PUBLIC sw_int_t lsn_set_fd_noblock(sw_fd_t fd);
SW_PUBLIC sw_int_t lsn_pub_cfg_init(sw_lsn_cycle_t *cycle, sw_char_t *lsn_name);
SW_PUBLIC sw_link_info_t *lsn_pub_socklistinit(sw_lsn_cycle_t *cycle);
SW_PUBLIC void *long_connect_thread(sw_lsn_cycle_t *cycle, void *arg);
SW_PUBLIC sw_int_t lsn_pub_err_deal(sw_lsn_cycle_t * cycle, sw_loc_vars_t *loc_vars, sw_cmd_t * cmd);
SW_PUBLIC sw_int_t lsn_pub_connect(sw_char_t *addr, sw_int32_t port);
SW_PUBLIC sw_int_t lsn_pub_bind(sw_char_t *addr, sw_int32_t port);
SW_PUBLIC sw_fd_t lsn_pub_accept(sw_fd_t sockid);
SW_PUBLIC sw_int_t lsn_pub_find_free_fd(sw_lsn_cycle_t *cycle, sw_fd_t accept_id);
SW_PUBLIC sw_int_t lsn_pub_find_old_fd(sw_lsn_cycle_t *cycle, sw_int_t mtype, sw_fd_t *fd);
SW_PUBLIC sw_int_t lsn_pub_close_fd(sw_lsn_cycle_t *cycle, sw_int32_t index);
SW_PUBLIC sw_fd_t lsn_get_fd_by_index(sw_lsn_cycle_t *cycle, int index);
SW_PUBLIC sw_int_t lsn_pub_rundlso(sw_char_t *process_name, sw_char_t *cbm_mode);
SW_PUBLIC sw_int_t lsn_pub_recv_len(sw_fd_t sock_id, sw_char_t *buf, sw_int32_t length);
SW_PUBLIC sw_int_t lsn_pub_getformat_func(sw_char_t *format,sw_int32_t index,sw_char_t *fun_name,sw_char_t *args);
SW_PUBLIC sw_int_t lsn_pub_create_loc_fd(sw_fd_t *fd, sw_char_t *lsn_name);
SW_PUBLIC sw_int_t  deal_hex_data(sw_char_t *str, sw_char_t *buf);
SW_PUBLIC sw_int_t check_fd_is_write(sw_fd_t sock_id);
SW_PUBLIC sw_int_t write_trace_list(sw_char_t *lsn_name, sw_int_t mtype);
SW_PUBLIC sw_int_t lsn_pub_send(sw_fd_t sock_id ,sw_char_t *buf,sw_int32_t length);
SW_PUBLIC sw_int_t lsn_pub_recv(sw_char_t *data, sw_fd_t sock_id, sw_buf_t *pkg_vars);
SW_PUBLIC sw_int_t lsn_pub_recv_ext(sw_fd_t sock_id, sw_buf_t *pkg_vars, sw_char_t *data);
SW_PUBLIC sw_int_t lsn_pub_analyze_udpcmd(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd);
SW_PUBLIC sw_int_t lsn_pub_svrsvc_map(sw_xmltree_t *map_tree, sw_loc_vars_t *loc_vars, sw_char_t *svr, sw_char_t *svc, sw_int32_t *plevel);
SW_PUBLIC sw_int_t lsn_handle_udp_deal(sw_fd_list_t * fd_lists);
SW_PUBLIC sw_int_t lsn_cycle_destroy(sw_lsn_cycle_t* cycle);
SW_PUBLIC sw_int_t lsn_cycle_ext_init(sw_lsn_cycle_t * cycle, sw_char_t * name);
SW_PUBLIC sw_int_t lsn_handle_timeout(sw_lsn_cycle_t* cycle);
SW_PUBLIC sw_int_t lsn_deal_err(sw_lsn_cycle_t *cycle, sw_loc_vars_t *loc_vars, sw_cmd_t *cmd);
SW_PUBLIC sw_int_t lsn_create_single_child(sw_lsn_cycle_t *cycle, sw_int_t index);
SW_PUBLIC sw_int_t lsn_handle_control_cmd(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_deal_recv_work_la(sw_lsn_cycle_t *cycle, sw_buf_t *locbuf, u_char *msgid, u_char *corrid);
SW_PUBLIC sw_int_t lsn_deal_send_work_la(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_cmd_t *cmd, sw_buf_t *locbuf);
SW_PUBLIC sw_int_t lsn_deal_linknull(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd);
SW_PUBLIC sw_int_t lsn_deal_send_timeout_la(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_deal_recv_timeout_la(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_get_route(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars, sw_buf_t *buf, sw_route_t *route_info, sw_int_t flag);
SW_PUBLIC sw_int_t lsn_get_route_by_name(sw_lsn_cycle_t *cycle, sw_char_t *prdt, sw_route_t *route);
SW_PUBLIC int lsn_set_err(sw_loc_vars_t *loc_vars, int errcode);
SW_PUBLIC int lsn_get_err(sw_loc_vars_t *loc_vars, char *errcode);
SW_PUBLIC sw_int_t lsn_pub_deal_out_task(sw_lsn_cycle_t *cycle, sw_loc_vars_t *loc_vars, sw_cmd_t *cmd, sw_buf_t *locbuf);
SW_PUBLIC sw_int_t lsn_pub_deal_in_task(sw_lsn_cycle_t *cycle, sw_loc_vars_t *loc_vars, sw_cmd_t *cmd, sw_buf_t *locbuf);
SW_PUBLIC sw_int_t lsn_update_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link);
SW_PUBLIC sw_int_t lsn_save_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, sw_cmd_t *cmd);
SW_PUBLIC sw_int_t lsn_load_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link);
SW_PUBLIC sw_int_t lsn_delete_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, int flag);
SW_PUBLIC sw_int_t lsn_print_linkinfo(sw_link_t *link);
SW_PUBLIC sw_int_t lsn_create_child(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_create_child_common(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_create_child_la(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_create_single_child_la(sw_lsn_cycle_t *cycle, int group_index, int sendtype);
SW_PUBLIC sw_int_t lsn_create_child_mq(sw_lsn_cycle_t *cycle);
SW_PUBLIC sw_int_t lsn_create_single_child_mq(sw_lsn_cycle_t *cycle, int group_index, int proc_index, int sendtype);
SW_PUBLIC sw_int_t lsn_create_single_child_common(sw_lsn_cycle_t *cycle, sw_int_t index);
SW_PUBLIC sw_int_t lsn_child_work(sw_lsn_cycle_t *cycle, sw_int32_t index);
SW_PUBLIC sw_int_t lsn_child_work_ext(sw_lsn_cycle_t *cycle, sw_int32_t index);
SW_PUBLIC sw_int_t lsn_recv_deny_la(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars);
SW_PUBLIC sw_int_t lsn_send_deny_la(sw_lsn_cycle_t *cycle, sw_loc_vars_t *vars, sw_buf_t *locbuf);
SW_PUBLIC int map(sw_loc_vars_t *locvar, sw_xmltree_t *mapcfg, int flag);
SW_PUBLIC sw_xmltree_t *lsn_read_xml(sw_char_t *xmlname);
SW_PUBLIC int run_enc_fun(sw_loc_vars_t *vars, sw_buf_t *locbuf, char *libname, char *funname, int flag);
SW_PUBLIC sw_int_t lsn_find_index_by_mtype(sw_lsn_cycle_t *cycle, sw_int_t mtype);
SW_PUBLIC sw_int_t lsn_find_index_by_fd(sw_lsn_cycle_t *cycle, sw_fd_t fd);
SW_PUBLIC sw_int_t lsn_pub_deal_timeout(sw_lsn_cycle_t * cycle);
SW_PUBLIC sw_int_t lsn_child_update_stat(sw_lsn_cycle_t *cycle, sw_int32_t status);
SW_PUBLIC int run_file_fun(char *funname, sw_loc_vars_t *vars, int flag, int sockid, void *param);
int 	get_xmltree_from_cache(sw_pkg_cache_list_t *cache, char *name);
void 	*get_handle_from_cache(sw_pkg_cache_list_t *cache, char *name);

#endif
