/*********************************************************************
 *** File: sw_uni_swconfig.h
 *** Function: Eswitch 's starting , stoping and management.
 *** Author: wangkun
 *** Create Date: 2013-4-3
 *** Update record:
 ********************************************************************/


#ifndef _SW_UNI_SVR_H
#define _SW_UNI_SVR_H

#include "procs.h"
#include "pub_cfg.h"
#include "pub_pool.h"
#include "pub_type.h"
#include "cycle.h"
#include "pub_string.h"


#define SW_FORCE 1
#define	SW_NORMAL 2

#define SW_UNI_MOD_NAME    "swuni"

#define SW_VERS_NO	"DFIS-BP.V3.0-201306212140"

#define SVC_NAME_MAX_LEN    (128)

/*Eswitch management interface's context.*/
typedef struct sw_uni_svr_s
{
    sw_char_t mod_name[NAME_LEN];   /* ģ����: UDPͨ��ʱʹ�� */
    sw_char_t swhome[FILE_NAME_MAX_LEN];    /* Env: SWHOME */
    sw_char_t swwork[FILE_NAME_MAX_LEN];    /* Env: SWWORK */
    sw_fd_t cmdfd;  /* ����ͨ���׽��� */
    void	*shm_cfg;	/*shm_cfg*/
}sw_uni_svr_t;

/*Eswitch process's status info.*/
typedef struct
{
	/*Eswitch process name*/
	sw_char_t	proc_name[32];

	/*Eswitch process type*/
	sw_int32_t	node_type;

	/*process id*/
	sw_int32_t	pid;

	/*process running status: SW_PROC_OK ok;SW_PROC_EXP exception*/
	sw_int32_t	cur_status;

	sw_char_t	status_desc[32];
}uni_proc_item_t;

typedef struct
{
	int shmid;			/*shm id*/
	int shm_size;			/*shm ��С*/
	time_t shm_atime;		/*���һ�����̸��ӵ��öε�ʱ��*/
	time_t shm_dtime;		/*���һ�������뿪�öε�ʱ��*/
	time_t shm_ctime;		/*���һ�������޸ĸöε�ʱ��*/
	int shm_cpid;			/*�����öν��̵�pid*/
	int shm_lpid;			/*�ڸö��ϲ��������һ�����̵�pid*/
	short shm_nattch;		/*��ǰ���ӵ��öεĽ��̸���*/
	char pname[128];		/*����shm�Ľ�����������*/
}uni_shm_t;

typedef struct
{
	int mqid;		/*mq id*/
	time_t msg_stime;	/*���͸���Ϣ�������һ����Ϣ��ʱ��*/
	time_t msg_rtime;	/*����Ϣ�����н��յ����һ����Ϣ��ʱ��*/
	time_t msg_ctime;	/*����޸Ķ��е�ʱ��*/
	long msg_cbytes;		/*������������Ϣ���ֽ���*/
	ushort msg_qnum;	/*��ǰ�����ϵ���Ϣ����*/
	int msg_qbytes;		/*��������ֽ���*/
	ushort msg_lspid;	/*�������һ����Ϣ�Ľ���id*/
	ushort msg_lrpid;	/*�������һ����Ϣ�Ľ���id*/
	int msg_pid;		/*����msg�Ľ���ID*/
	char pname[128];	/*����msg�Ľ�����*/
}uni_msg_t;

typedef struct
{
	int semid;		/*sem id*/
	long sem_otime;		/*���һ�ζ��ź�������*/
	long sem_ctime;		/*������ṹ�����һ���޸ĵ�ʱ��*/
	ushort sem_nsems;	/*���ź��������ϵ��ź���*/
	char pname[128];
}uni_sem_t;

typedef struct
{
	int used;
	uni_shm_t uni_shm;
	uni_msg_t uni_msg;
	uni_sem_t uni_sem;
}uni_msipc_t;

#define UNI_MAX_IPC_CNT 10240
#define uni_svr_get_version() SW_VERS_NO

SW_PUBLIC sw_int_t uni_svr_init(sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_destroy(sw_uni_svr_t *svr);

SW_PUBLIC sw_int_t uni_svr_start_all(const sw_cycle_t *cycle, sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_start_lsn(const sw_uni_svr_t *svr, const sw_char_t* lsn_name);
SW_PUBLIC sw_int_t uni_svr_start_all_lsn( sw_uni_svr_t *svr);

SW_PUBLIC sw_int_t uni_svr_start_svc(const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_start_all_svc( sw_uni_svr_t *svr);
    
SW_PUBLIC sw_int_t uni_svr_start(const sw_cycle_t *cycle, sw_uni_svr_t *svr, const sw_char_t* sw_name);
SW_PUBLIC sw_int_t uni_svr_start_process(const sw_char_t  *name);

SW_PUBLIC sw_int_t uni_svr_stop_all(sw_cycle_t *cycle, sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_stop_lsn(sw_cycle_t *cycle, sw_uni_svr_t *svr, const sw_char_t* lsn_name);
SW_PUBLIC sw_int_t uni_svr_stop_all_lsn( sw_uni_svr_t *svr);

SW_PUBLIC sw_int_t uni_svr_stop_all_svc( sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_stop(sw_uni_svr_t *svr, const sw_char_t* sw_name, sw_int32_t flag);

SW_PUBLIC sw_int_t uni_svr_start_job(void);

SW_PUBLIC sw_int_t uni_svr_start_all_task(sw_uni_svr_t *svr);

SW_PUBLIC sw_int_t uni_svr_prdt_start_all( sw_uni_svr_t *svr, const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_prdt_stop_all( sw_uni_svr_t *svr, const char *prdt_name);

sw_int_t uni_svr_prdt_print_all( sw_uni_svr_t *svr, const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_prdt_print_all_svc(sw_uni_svr_t *svr, const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_prdt_print_all_lsn( sw_uni_svr_t *svr, const char *prdt_name);

SW_PUBLIC sw_int_t uni_svr_print_sys( int type);
SW_PUBLIC void uni_svr_print_all_product(sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_prdt_stop_all_lsn( sw_uni_svr_t *svr, const char *prdt_name);
SW_PUBLIC bool uni_svr_prdt_is_exist(const sw_uni_svr_t *svr, const char *name);
SW_PUBLIC sw_int_t uni_svr_prdt_start_all_lsn( sw_uni_svr_t *svr, const char *prdt_name);
SW_PUBLIC int uni_svr_prdt_start_all_svc( sw_uni_svr_t *svr, const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_prdt_stop_all_svc(sw_uni_svr_t *svr, const char *name);
SW_PUBLIC sw_int_t uni_svr_prdt_stop_lsn( sw_uni_svr_t *svr, const char *prdt_name, const char *lsn_name);
SW_PUBLIC sw_int_t uni_svr_prdt_stop_svc( sw_uni_svr_t *svr, const char *prdt_name);

SW_PUBLIC void uni_svr_mtype_print_head(void);
SW_PUBLIC void uni_svr_mtype_print_tail(int total);
SW_PUBLIC bool uni_svr_mtype_print_item(int idx, const sw_mtype_node_t *item, sw_int32_t flag);
SW_PUBLIC sw_int_t uni_svr_mtype_list(int mtype, int num);
SW_PUBLIC sw_int_t uni_svr_mtype_list_all();
SW_PUBLIC sw_int_t uni_svr_mtype_list_busy();
SW_PUBLIC sw_int_t uni_svr_mtype_list_free();
SW_PUBLIC sw_int_t uni_svr_prdt_print_child(const char *svr_name, const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_prdt_print_lsn_child(const sw_cycle_t *cycle, const char *prdt_name);
SW_PUBLIC sw_int_t uni_svr_prdt_print_svc_child(const sw_uni_svr_t *svr,const char *prdt_name);

SW_PUBLIC sw_int_t uni_svr_res_list_shm( sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_res_list_msq();
SW_PUBLIC sw_int_t uni_svr_res_clean(sw_int32_t shmid);

SW_PUBLIC sw_int_t uni_svr_stop_process(sw_uni_svr_t *svr, int cmd_type, const char *exec_name);
SW_PUBLIC void uni_svr_print_all();
SW_PUBLIC void uni_svr_print_proc_head(void);
SW_PUBLIC void uni_svr_print_tail(int total);
SW_PUBLIC sw_int_t uni_svr_prdt_start_from_lsn(sw_uni_svr_t *svr, const char *prdt_name, const char *lsn_name);

SW_PUBLIC sw_int_t uni_svr_prdt_start_lsn(
             sw_uni_svr_t *svr,
            const char *prdt_name, const char *lsn_name);
SW_PUBLIC sw_int_t uni_svr_task_get_head(sw_job_head_t *head);
SW_PUBLIC sw_int_t uni_svr_task_get_item_by_no(sw_char_t *no,sw_job_item_t *item);
SW_PUBLIC sw_int_t uni_svr_task_get_item_by_idx(sw_int32_t idx,sw_job_item_t *item);
SW_PUBLIC sw_int_t uni_svr_task_start_item( sw_uni_svr_t *svr, const sw_char_t *no);

SW_PUBLIC sw_int_t uni_svr_start_prdt(sw_uni_svr_t *svr, const sw_char_t *prdt_name);
SW_PUBLIC sw_int_t uni_svr_stop_prdt(sw_uni_svr_t *svr, sw_char_t *prdt_name);
SW_PUBLIC sw_int_t uni_svr_start_lsn_in_prdt(sw_uni_svr_t *svr, const sw_char_t *prdt_name, const sw_char_t *lsn_name);
SW_PUBLIC sw_int_t uni_svr_stop_lsn_in_prdt(sw_uni_svr_t *svr, sw_char_t *prdt_name, sw_char_t *lsn_name);
SW_PUBLIC sw_int_t uni_svr_print_in_all_prdt(sw_uni_svr_t *svr);
SW_PUBLIC sw_int_t uni_svr_get_log(sw_uni_svr_t *svr, sw_char_t *traceno, sw_char_t *date, sw_char_t *lsn_name);

int uni_svr_deploy_bp_by_cfgfile(sw_uni_svr_t *svr, const char *path);

int uni_svr_reload_lsn( sw_uni_svr_t *svr,
		const char *lsn_name,
		const char *lsn_action,
		const char *prdt_name,
		const char *prdt_action);
int uni_svr_reload_prdt( sw_uni_svr_t *svr,
	const char *prdt_name, 
	const char *prdt_action, 
	const char *svr_name, 
	const char *svr_action, 
	const char *svc_name, 
	const char *svc_action);

int uni_get_ipc_info(uni_msipc_t *msipc);
int uni_clear_ipc_source(uni_msipc_t *msipc);

#endif /* _SW_UNI_SVR_H */

