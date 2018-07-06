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

/*数据库表数据转移配置部分*/
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
/*整理日志*/
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
/*struct point 排序方法*/
typedef int (*agt_point_cmp_pt)(struct point*, struct point*);

/*字符串大写转小写*/
char *agt_str_tolower(char *str);

/*执行命令， 忽视信号SIGCHLD*/
int agt_system(char *cmd);

/*执行命令， 结果保存在buf中*/
int agt_popen(char *cmd, char *buf);

/*按照分隔符拆分字符串*/
int agt_str_split(char *sbuf, char *type, char item[][256], int cnt);

/*将时间输出为制定格式的字符串*/
int agt_date_format(long time, char *date, char *type);

/*判断处理码成功或失败*/
int agt_check_stat(char *result);

/*获取产品中文名*/
int agt_get_pdt_name(char *pdt, char *pdtname, int len);

/*获取渠道中文名*/
int agt_get_lsn_name(char *lsn, char *lsnname);

/*设置出错信息*/
int agt_error_info(char *code, char *desc);

/*字母序扫描文件夹*/
struct dirent **agt_scan_dir(int *n,  char *path);

/*释放文件夹的动态内存*/
int agt_free_namelist(struct dirent **namelist, int n);

/*隐藏帐号*/
int agt_hide_account(char *account);

/*解析monitor.log文件中的TOTAL信息*/
int agt_get_trace_info(char *str, sw_trace_item_t *trace_item, char *ptid);

/*将字符串时间转为秒数*/
long agt_date_to_time(char *date);

/*释放point链表*/
int agt_free_point(struct point *phead);

/*插入point链表*/
struct point *agt_insert_point(struct point *phead, char *name, int right, int response, long long d_time, char *pdt);

/*链表排序*/
struct point *agt_rank_point(struct point *phead, agt_point_cmp_pt point_cmp);

/*按数目比较*/
int agt_point_cmp_num(struct point *p1, struct point *p2);

/*求正确率*/
float agt_get_avgright(int right, int sum);

/*求平均时间*/
float agt_get_avgtime(long long d_time, int total);

/*按照条件查询交易信息*/
int agt_get_trc(sw_loc_vars_t *vars, int page_idx, int page_cnt, struct search *searchs, int flags, agt_set_trace_pt set_trace_info);

/*获取内存状态*/
int agt_get_mem_stats(mem_stats_t *mem_stats);

/*获取网络状态*/
network_stats_t *agt_get_network_stats(int *num);

/*获取CPU状态*/
cpu_stat_t *agt_get_cpu_stat(int *num);

/*获取进程信息*/
process_stats_t *agt_get_process_stat(int *num);

/*获取流水日志*/
int agt_get_trace_log(sw_loc_vars_t *vars, struct search  *requ_info, int flag, log_buf_t *log);

/*按CPU或MEM排序查找进程信息*/
int get_proc_sort(sw_loc_vars_t *vars, char *type);

/*链表插入*/
unit_t *insert_unit(unit_t *head, unit_t *unit);

/*链表更新*/
unit_t *update_unit(unit_t *head, char *name);

/*链表更新状态*/
unit_t *update_unit_status(unit_t *head, char *name, int hour, int status);

/*链表释放*/
int free_unit(unit_t *head);

/*获取产品信息*/
unit_t *get_prdt_info(void);

/*获取服务信息*/
unit_t *get_svr_info(unit_t *pproduct);

/*获取渠道信息*/
unit_t *get_chnl_info(void);

/*获取交易码信息*/
unit_t *get_txcode_info(unit_t *pproduct);

/*获取返回码信息*/
unit_t *get_retcode_info(unit_t *pproduct);

/*获取响应信息*/
unit_t *get_resflag_info(void);

/*获取返回信息*/
unit_t *get_return_info(void);

/*时间格式转换*/
int time_adjus(char *time, char *stime);

/*搜索XML节点*/
sw_xmlnode_t *agt_xml_search(sw_xmltree_t *xml,char *path, char *name, char *value);

/*删除XML节点*/
int agt_remove_node(sw_xmlnode_t *node);

/*特殊字符转换*/
int change_ral(char *ral, int flage);

/*插入*/
Node* insert(Node *head,sw_char_t *name,sw_int32_t value);

/*查看链表信息*/
void view(Node* head);

/*释放链表*/
void destory(Node* head);

/*预警规则转换*/
int pack_exp(sw_xmltree_t *xml, char *sRule, char *sTmp);

/*检查安装状态*/
int agt_check_inst_stat(sw_loc_vars_t *vars, char *opt, char *chk_run);

/*执行命令前检查*/
int pre_cmd(char *cmd_type);

/*备份文件*/
int agt_back_file(char *filename);

/*备份配置*/
int restore(char *id);

/*命令执行后*/
int post_cmd(char *cmd_type);

/*字符串解析*/
int agt_str_parse(char *sbuf, char item[][256], int cnt);

/*保存产品数据库列表*/
Node *xml_pack(sw_xmltree_t *xml, Node *head, char *deploy);
Node *same_deploy(char *install_dir, char *deploy, Node *head);

/*释放链表*/
void destory(Node* head);

/*脚本类型*/
void agt_get_exec_type(int exec_type, char *buf);

/*中文星期*/
int agt_trans_week(char *cweek, char *eweek);

/*判断平台工作模式 是否开启日志服务器,资源服务器*/
int judge_workmode(void);

/*统计当日交易*/
int count_monitor(struct  total *total, unit_t *pproduct, unit_t *pchannel);

/*获取保存的统计信息*/
int agt_get_monitor_data(struct total *total, unit_t *pproduct, unit_t *pchannel);

/*保存统计信息*/
int agt_save_monitor_data(struct total *total, unit_t *pproduct, unit_t *pchannel);

/*预警级别中文*/
char *agt_level_name(char *level);

/*查询数据字典*/
int set_app_data_dic_trans(char *code_id, char *code_value, char *code_desc);

/*自增序列查询值*/
int agt_creat_code(char *sequence, char* rate_no);


int record_oper_log(sw_loc_vars_t *vars, char *err_code, char *err_msg);


/*探测表是否存在*/
int agt_table_detect(char *table_name);

/*将查询结果用分隔符连接起来，替代oracle中的wm_concat函数*/
int agt_wm_concat(char *sql, char *columns);
#endif
