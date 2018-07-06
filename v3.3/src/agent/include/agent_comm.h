#ifndef __AGENT_COMM_H__
#define __AGENT_COMM_H__

#include <libgen.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/utsname.h>
#include <pthread.h>
#include "pub_type.h"
#include "pub_vars.h"
#include "job.h"
#include "trace.h"
#include "omsinfo.h"
#include "pub_xml.h"
#include "mtype.h"
#include "uni_svr.h"
#include "pub_log.h"
#include "pub_db.h"
#include "pub_buf.h"
# ifdef AIX
#include <fstab.h>
#include <sys/statfs.h>
#include <sys/mntctl.h>
#include <sys/vmount.h>
#include <libperfstat.h>
#endif

#define SCRIPT_ERR	"err"
#define SCRIPT_OK	"ok"

#define CMD_RUNNING     1
#define CMD_NOT_RUN     2

#define CMD_NAME        "install_cmd"
#define LSN_DEPLOY      "listener.lsn"
#define PRDT_DEPLOY     "products.xml"
#define CHNEL_DEPLOY    "channels.xml"
#define DB_DEPLOY       "dbconfig.xml"
#define PRDT_LSN        "prdt_lsn"
#define SQL_NAME_LEN 64
#define SPACE		"tmp/prdtiodust"
#define SPLITCHAR	"/"

/*sys stats*/
#define OMS_MAX_CPU_NUM 64
#define OMS_MAX_NET_NUM 16
#define MAX_THREAD_NUM	50
#define MAX_DATA_TRANS_NUM 64
#define XML_REPLY_MSG_BUF_SIZE 128

#define		START_ALL_PRDT   "START_ALL_PRDT"
#define		STOP_ALL_PRDT    "STOP_ALL_PRDT"
#define		START_PRDT	     "START_PRDT"
#define		STOP_PRDT	     "STOP_PRDT"
#define		START_ONE	     "START_ONE"
#define		STOP_ONE	     "STOP_ONE"
#define		COUNT_FILE	   ".count_monitor"

__attribute__((unused)) SW_PROTECTED const char *operations[] = {
	START_ALL_PRDT,
	STOP_ALL_PRDT,
	START_PRDT,
	STOP_PRDT,
	START_ONE,
	STOP_ONE,
};

/*���ݿ������ת�����ò���*/
struct search
{
	char sBuf[128];
	char TxCode[32];
	char Lsn[32];
	char Prdt[32];
	char Svr[32];
	char Svc[32];
	char PlatFlow[32];
	char TxAmount[32];
	char RetCode[32];
	char RetCodeNo[32];
	char TxDate[32];
	char StartTime[32];
	char EndTime[32];
	char flag[4];
};

/*monitor log elem*/
typedef struct trade_info
{
	long long time_s ;/*item[5]*/
	long long time_e ;/*item[6]*/
	char trace_no[32];/*item[0]*/
	char pt_no[32];/*item[1]*/
	char svr[32];/*item[2]*/
	char svc[32];/*item[3]*/
	char date[10];/*item[4]*/
	char respce[10];/*item[7]*/
	char sts[10]; /*item[7]*/
	char pdt[64];/*item[8]*/
	char txamount[32];/*item[9]*/
	char deaccount[32];/*item[10]*/
	char craccount[32];/*item[11]*/
	char deaccname[64];/*item[12]*/
	char craccname[64];/*item[13]*/
	char poundage[32];/*item[14]*/
	char chaccount[32];/*item[15]*/
	char nowturnsign[32];/*item[[16]*/
	char borrowmark[10];/*item[17]*/
	char chl[32];/*item[18]*/
	char errcode[32];/*item[20]*/
	char resmsg[32];/*item[21]*/
	char bus_no[32];/*item[22]*/
	char response[32];/*item[23]*/
}sw_trade_info_t;

struct point
{
	char name[64];
	int right;
	int num;
	int response;
	long long d_time;
	struct point *next;
	struct point *pdt;
};
/*������־*/
typedef struct log_buf
{
	char msg_buf[1024*1024];
	char time[40][32];
	int postion[40];
	int index;
}log_buf_t;

struct total
{
	char save_time[10];
	long end_size;
	int num;
	int right;
	int wrong;
	int reaction;
	long long d_time;
};

typedef struct
{
	cpu_usage_t	cpu_usage[OMS_MAX_CPU_NUM];
	int	cpu_num;
	pthread_mutex_t mutex;
	pthread_t	thread_tid;
}cpu_usage_stat_t;

typedef struct
{
	net_percent_t	net_percent[OMS_MAX_NET_NUM];
	int	net_num;
	pthread_mutex_t mutex;
	pthread_t	thread_tid;
}net_percent_stat_t;

typedef struct proc_top
{
	char    user[32];
	int     pid;
	char    sts[32];
	float   cpu;
	float   a_mem;
	long    v_mem;
	int     thcount;
	char    etime[64];
	char    name[32];
}proc_top_t;

typedef struct Node
{
	char	name[218];
	int	value;
	struct Node	*next;
}Node;

typedef struct ProcNode
{
	struct ProcNode     *next;
	proc_top_t      proc;
}Proc_Node_t;

typedef struct unit
{
	char 	name[64];
	char 	cnname[64];
	int  	cnt;
	float	rate;
	int 	right[24];
	int 	wrong[24];
	struct unit	*next;
}unit_t;

typedef int (*agt_set_trace_pt)(sw_loc_vars_t *, sw_trace_item_t *, int , unit_t **pp, char *ptid);
/*struct point ���򷽷�*/
typedef int (*agt_point_cmp_pt)(struct point*, struct point*);

/*�ַ�����дתСд*/
char *agt_str_tolower(char *str);

/*ִ����� �����ź�SIGCHLD*/
int agt_system(char *cmd);

/*ִ����� ���������buf��*/
int agt_popen(char *cmd, char *buf);

/*���շָ�������ַ���*/
int agt_str_split(char *sbuf, char *type, char item[][256], int cnt);

/*��ʱ�����Ϊ�ƶ���ʽ���ַ���*/
int agt_date_format(long time, char *date, char *type);

/*�жϴ�����ɹ���ʧ��*/
int agt_check_stat(char *result);

/*��ȡ��Ʒ������*/
int agt_get_pdt_name(char *pdt, char *pdtname, int len);

/*��ȡ����������*/
int agt_get_lsn_name(char *lsn, char *lsnname);

/*���ó�����Ϣ*/
int agt_error_info(char *code, char *desc);

/*��ĸ��ɨ���ļ���*/
struct dirent **agt_scan_dir(int *n,  char *path);

/*�ͷ��ļ��еĶ�̬�ڴ�*/
int agt_free_namelist(struct dirent **namelist, int n);

/*�����ʺ�*/
int agt_hide_account(char *account);

/*����monitor.log�ļ��е�TOTAL��Ϣ*/
int agt_get_trace_info(char *str, sw_trace_item_t *trace_item, char *ptid);

/*���ַ���ʱ��תΪ����*/
long agt_date_to_time(char *date);

/*�ͷ�point����*/
int agt_free_point(struct point *phead);

/*����point����*/
struct point *agt_insert_point(struct point *phead, char *name, int right, int response, long long d_time, char *pdt);

/*��������*/
struct point *agt_rank_point(struct point *phead, agt_point_cmp_pt point_cmp);

/*����Ŀ�Ƚ�*/
int agt_point_cmp_num(struct point *p1, struct point *p2);

/*����ȷ��*/
float agt_get_avgright(int right, int sum);

/*��ƽ��ʱ��*/
float agt_get_avgtime(long long d_time, int total);

/*����������ѯ������Ϣ*/
int agt_get_trc(sw_loc_vars_t *vars, int page_idx, int page_cnt, struct search *searchs, int flags, agt_set_trace_pt set_trace_info);

/*��ȡ�ڴ�״̬*/
int agt_get_mem_stats(mem_stats_t *mem_stats);

/*��ȡ����״̬*/
network_stats_t *agt_get_network_stats(int *num);

/*��ȡCPU״̬*/
cpu_stat_t *agt_get_cpu_stat(int *num);

/*��ȡ������Ϣ*/
process_stats_t *agt_get_process_stat(int *num);

/*��ȡ��ˮ��־*/
int agt_get_trace_log(sw_loc_vars_t *vars, struct search  *requ_info, int flag, log_buf_t *log);

/*��CPU��MEM������ҽ�����Ϣ*/
int get_proc_sort(sw_loc_vars_t *vars, char *type);

/*�������*/
unit_t *insert_unit(unit_t *head, unit_t *unit);

/*�������*/
unit_t *update_unit(unit_t *head, char *name);

/*�������״̬*/
unit_t *update_unit_status(unit_t *head, char *name, int hour, int status);

/*�����ͷ�*/
int free_unit(unit_t *head);

/*��ȡ��Ʒ��Ϣ*/
unit_t *get_prdt_info(void);

/*��ȡ������Ϣ*/
unit_t *get_svr_info(unit_t *pproduct);

/*��ȡ������Ϣ*/
unit_t *get_chnl_info(void);

/*��ȡ��������Ϣ*/
unit_t *get_txcode_info(unit_t *pproduct);

/*��ȡ��������Ϣ*/
unit_t *get_retcode_info(unit_t *pproduct);

/*��ȡ��Ӧ��Ϣ*/
unit_t *get_resflag_info(void);

/*��ȡ������Ϣ*/
unit_t *get_return_info(void);

/*ʱ���ʽת��*/
int time_adjus(char *time, char *stime);

/*����XML�ڵ�*/
sw_xmlnode_t *agt_xml_search(sw_xmltree_t *xml,char *path, char *name, char *value);

/*ɾ��XML�ڵ�*/
int agt_remove_node(sw_xmlnode_t *node);

/*�����ַ�ת��*/
int change_ral(char *ral, int flage);

/*����*/
Node* insert(Node *head,sw_char_t *name,sw_int32_t value);

/*�鿴������Ϣ*/
void view(Node* head);

/*�ͷ�����*/
void destory(Node* head);

/*Ԥ������ת��*/
int pack_exp(sw_xmltree_t *xml, char *sRule, char *sTmp);

/*��鰲װ״̬*/
int agt_check_inst_stat(sw_loc_vars_t *vars, char *opt, char *chk_run);

/*ִ������ǰ���*/
int pre_cmd(char *cmd_type);

/*�����ļ�*/
int agt_back_file(char *filename);

/*��������*/
int restore(char *id);

/*����ִ�к�*/
int post_cmd(char *cmd_type);

/*�ַ�������*/
int agt_str_parse(char *sbuf, char item[][256], int cnt);

/*�����Ʒ���ݿ��б�*/
Node *xml_pack(sw_xmltree_t *xml, Node *head, char *deploy);
Node *same_deploy(char *install_dir, char *deploy, Node *head);

/*�ͷ�����*/
void destory(Node* head);

/*�ű�����*/
void agt_get_exec_type(int exec_type, char *buf);

/*��������*/
int agt_trans_week(char *cweek, char *eweek);

/*�ж�ƽ̨����ģʽ �Ƿ�����־������,��Դ������*/
int judge_workmode(void);

/*ͳ�Ƶ��ս���*/
int count_monitor(struct  total *total, unit_t *pproduct, unit_t *pchannel);

/*��ȡ�����ͳ����Ϣ*/
int agt_get_monitor_data(struct total *total, unit_t *pproduct, unit_t *pchannel);

/*����ͳ����Ϣ*/
int agt_save_monitor_data(struct total *total, unit_t *pproduct, unit_t *pchannel);

/*Ԥ����������*/
char *agt_level_name(char *level);

/*��ѯ�����ֵ�*/
int set_app_data_dic_trans(char *code_id, char *code_value, char *code_desc);

/*�������в�ѯֵ*/
int agt_creat_code(char *sequence, char* rate_no);


int record_oper_log(sw_loc_vars_t *vars, char *err_code, char *err_msg);


/*̽����Ƿ����*/
int agt_table_detect(char *table_name);

/*����ѯ����÷ָ����������������oracle�е�wm_concat����*/
int agt_wm_concat(char *sql, char *columns);
#endif
