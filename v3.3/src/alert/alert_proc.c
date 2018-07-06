#include "alert.h"
#include "common.h"
#include "alert_proc.h"
#include "alert_errno.h"
#include "procs.h"
#include "pub_log.h"
#include "pub_xml.h"
#include "msg_trans.h"
#include "thread_pool.h"
#include "pub_vars.h"
#include "pub_buf.h"
#include "pub_cfg.h"
#include "pub_usocket.h"

#if defined(__ALERT_SUPPORT__)

/* 预警报文体格式定义 */
/* 1. 平台错误预警报文体格式 */
#define ALERT_PACK_PLATERR_BODY_FORMAT \
"<?xml version=\"1.0\" encoding=\"GBK\" standalone=\"no\" ?> \
<ALERT>\
	<BPID>%s</BPID>\
	<DATE>%s</DATE>\
	<TIME>%s</TIME>\
	<ALERTTYPE>%s</ALERTTYPE>\
	<PLATERR ERRNO=\"%d\" LEVEL=\"%d\">\
	<DESC>%s</DESC>\
	<REASON>%s</REASON>\
	<SOLVE>%s</SOLVE>\
	<REMARK>%s</REMARK>\
	<PRDT>%s</PRDT>\
	</PLATERR>\
</ALERT>"

/* 2. 交易错误预警报文体格式 */
#define ALERT_PACK_OPRERR_BODY_FORMAT \
"<?xml version=\"1.0\" encoding=\"GBK\" standalone=\"no\" ?> \
<ALERT>\
	<SESS_ID>%s</SESS_ID>\
	<BPID>%s</BPID>\
	<DATE>%s</DATE>\
	<TIME>%s</TIME>\
	<ALERTTYPE>%s</ALERTTYPE>\
	<OPRERR ERRNO=\"%d\" LEVEL=\"%d\">\
	<DESC>%s</DESC>\
	<REASON>%s</REASON>\
	<SOLVE>%s</SOLVE>\
	<REMARK>%s</REMARK>\
	<PRDT>%s</PRDT>\
	</OPRERR>\
</ALERT>"

/* 3. 平台监控预警报文体格式 */


/* 4. 系统监控预警报文体格式 */


/*===============================================================================*/
/* 预警错误码配置 适配表			  警告: 请勿随意修改!!!!
   警告: type顺序必须和ERRNO_TYPE_ENUM的中定义一致，否则将会出现严重异常!!!!!!!! */
static const alert_errcfg_adapt_t g_errcfg_adapter[ERRNO_TYPE_TOTAL] = 
{
	{ERRNO_TYPE_PLAT, ERRNO_PLAT_BEGIN, ERRNO_PLAT_END, ERRNO_PLAT_CFG_FILE},
	{ERRNO_TYPE_OPR, ERRNO_OPR_BEGIN, ERRNO_OPR_END, ERRNO_OPR_CFG_FILE}
};

#define alert_errno_get_begin(type) (g_errcfg_adapter[type].begin) 	/* 错误码 开始值 */
#define alert_errno_get_end(type)   (g_errcfg_adapter[type].end) 	/* 错误码 结束值 */
#define alert_errno_get_fname(type) (g_errcfg_adapter[type].fname)  /* 错误码 配置文件名 */
/*===============================================================================*/

/* 获取平台监控配置文件绝对路径 */

/* 获取系统监控配置文件绝对路径 */

/* 获取动态链接库文件绝对路径 */
#define AlertGetLibPath(fname, size, libname) \
	snprintf(fname, size, "%s/%s/%s", getenv("SWWORK"), ALERT_LIB_PATH, libname);

/* 打印平台错误码详细信息 */
#define alert_perrno_print(item) \
{ \
	pub_log_info("\n\tERRNO:%d LEVEL:%d DESC:%s\n\tREASON:%s\n\tSOLVE:%s\n", \
			item->_errno, item->level, item->desc, item->reason, item->solve); \
}

/* 打印交易错误码详细信息 */
#define alert_oerrno_print(item) \
{ \
	pub_log_info("\n\tERRNO:%d LEVEL:%d DESC:%s\n\tREASON:%s\n\tSOLVE:%s\n", \
			item->_errno, item->level, item->desc, item->reason, item->solve); \
}

/* 是否达到平台预警级别: 当前错误预警级别是否高于平台预警级别 */
#define AlertLevelIsOk(cycle, _level)   ((_level) >= (cycle->alertcfg.level))

/* 是否达到远程服务预警级别: 当前错误预警级别是否高于平台预警级别 */
#define AlertSvrLevelIsOk(remote, _level)   ((_level) >= (remote->level))

#define AlertGetLevel(alert) (alert->alertcfg.level)

/* 系统coredump预警 */
#define	LABEL		"LABEL:"
#define IDENTIFIER	"IDENTIFIER:"
#define DATATIME	"Date/Time:"
#define MACHINEID	"Machine Id:"
#define NODEID		"Node Id:"
#define SIGNO		"SIGNAL NUMBER"
#define PROCESSID	"USER'S PROCESS ID:"
#define COREFILE	"CORE FILE NAME"
#define PROGRAM		"PROGRAM NAME"
#define SEPLINE		"---------------------------------------------------------"

pthread_mutex_t g_alert_mutex = PTHREAD_MUTEX_INITIALIZER;

/*===============================================================================*/
/* 静态函数声明 */
static sw_int_t alert_cycle_init(alert_cycle_t *cycle);
static sw_int_t alert_cycle_work(alert_cycle_t *cycle);
static sw_int_t alert_cycle_free(alert_cycle_t *cycle);

static void *alert_push_routine(void *args);
static sw_int_t alert_recv_routine(alert_cycle_t *cycle);

static sw_int_t alert_errno_load(alert_cycle_t *cycle);
static sw_int_t alert_errno_check(int type, void *errcfg);

static sw_int_t alert_perrno_load(alert_cycle_t *cycle);
static sw_int_t alert_perrno_parse(sw_xmltree_t *xml, perrno_cfg_t *errcfg);
static sw_int_t alert_perrno_parse_item(sw_xmltree_t *xml, perrno_item_t *item);
static sw_int_t alert_perrno_get_type(int _errno);
static const perrno_item_t *alert_perrno_search(const alert_cycle_t *cycle, int _errno);
static const perrno_item_t *alert_perrno_bsearch(const perrno_cfg_t *errcfg, int _errno);

static sw_int_t alert_oerrno_load(alert_cycle_t *cycle);
static sw_int_t alert_oerrno_parse(sw_xmltree_t * xml,oerrno_cfg_t * errcfg);
static sw_int_t alert_oerrno_parse_item(sw_xmltree_t * xml,oerrno_item_t * item);
static sw_int_t alert_oerrno_get_type(int _errno);
static const oerrno_item_t *alert_oerrno_search(const alert_cycle_t *cycle, int _errno);
static const oerrno_item_t *alert_oerrno_bsearch(const oerrno_cfg_t *errcfg, int _errno);

static sw_int_t alert_connect(const char *ipaddr, int port);
static sw_int_t alert_remote_connect(alert_cycle_t *cycle);

static sw_int_t alert_set_packcb(alert_cycle_t *cycle);
static sw_int_t alert_set_packfunc(alert_remote_t *remote, const alert_pack_func_t func);

static sw_int_t alert_proc_register(const char *name);

static sw_int_t alert_msg_deal(alert_cycle_t *cycle, alert_msq_t *msg);
static sw_int_t alert_platerr_deal(const alert_cycle_t *cycle, const alert_error_t *mtext);
static sw_int_t alert_oprerr_deal(alert_cycle_t *cycle, const alert_error_t *mtext);


static sw_int_t alert_nsend(alert_remote_t *remote, const char *msg, int len);

static sw_int_t alert_platerr_send(const alert_cycle_t *cycle, alert_perrmsg_t *perrmsg);
static sw_int_t alert_oprerr_send(alert_cycle_t *cycle, alert_oerrmsg_t *oerrmsg);

static sw_int_t alert_remote_pack(int type, const void *msg, char *pack, int len);
static sw_int_t alert_remote_pack_platerr(const alert_perrmsg_t *perrmsg, char *pack, int len);
static sw_int_t alert_remote_pack_oprerr(const alert_oerrmsg_t *oerrmsg, char *pack, int len);

static sw_int_t alert_remote_recv(int sckid);
static sw_int_t alert_swcmd_recv(int sckid);
static sw_int_t alert_save_msg_to_file(const char *msg, int len);
static sw_int_t alert_get_msg_from_file(char *msg, int *len);
static sw_int_t alert_check_msg_time(char *msg, int len);
static int alert_log_change(const char *fname);	
static void *alert_sysmnt_routine(void *args);

SW_PUBLIC int loc_set_zd_data(sw_loc_vars_t *vars, char *name, char *value);

/******************************************************************************
 **函数名称: main
 **功	能: 预警进程 入口函数
 **输入参数: NONE
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 初始化设置
 **	 2. 调用工作接口
 **	 3. 预警进程释放
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
int main(void)
{
	sw_int_t ret = SW_ERR;
	alert_cycle_t cycle;

	memset(&cycle, 0, sizeof(cycle));

	/* 1. 预警进程初始化 */
	ret = alert_cycle_init(&cycle);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Initalize process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	pthread_mutex_init(&g_alert_mutex, NULL);
	/* 2. 预警进程工作 */
	ret = alert_cycle_work(&cycle);
	if(SW_OK != ret)
	{
		alert_cycle_free(&cycle);
		pub_log_error("[%s][%d] Process work exception!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. 预警进程释放 */
	alert_cycle_free(&cycle);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_cycle_init
 **功	能: 预警进程初始化
 **输入参数: 
 **	 cycle: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 初始化预警日志
 **	 2. 加载平台配置
 **	 3. 加载预警配置
 **	 4. 加载错误码配置
 **	 5. 连接远程服务
 **	 6. 设置组包回调函数
 **	 7. 注册预警进程
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.10 #
 ******************************************************************************/
static sw_int_t alert_cycle_init(alert_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;

	/* 1. 初始化BASE CYCLE */
	ret = cycle_init((sw_cycle_t *)cycle, PROC_NAME_ALERT, ND_ALERT, ALERT_ERR_LOG, ALERT_DBG_LOG, NULL);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Initialize base cycle failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. 加载预警配置 */
	ret = alert_cfg_load(&cycle->alertcfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Load cycle configure failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	if(SWITCH_OFF == cycle->alertcfg._switch)
	{
		pub_log_info("[%s][%d] Alert didn't switch on!", __FILE__, __LINE__);
		exit(0);
	}

	/* 3. 连接运行时共享内存 */
	ret = run_link_and_init();
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Link runtime share memory failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 4. 创建/打开预警消息队列 */
	ret = alert_link();
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Load cycle configure failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 5. 加载错误码配置文件 */
	ret = alert_errno_load(cycle);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Load errno configure failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 6. 连接远程服务 */
	/*** 预警模块跟运维后台采用TCP短链接方式进行通讯 
	  ret = alert_remote_connect(cycle);
	  if(SW_OK != ret)
	  {
	  pub_log_info("[%s][%d] Connect remote server(s) failed!", __FILE__, __LINE__);
	  return SW_ERR;
	  }
	 ***/

	/* 7. 设置组包回调函数 */
	ret = alert_set_packcb(cycle);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Set callback function failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 8. 注册预警进程 */
	ret = alert_proc_register(PROC_NAME_ALERT);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Register process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	pub_log_info("[%s][%d] Initialize process success!", __FILE__, __LINE__);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_cycle_work
 **功	能: 预警工作接口
 **输入参数: 
 **	  cycle: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 启动多个线程
 **	 2. 为线程分配工作
 **注意事项: 
 **	 线程包括: 侦听线程(主线程)、推送线程、平台监控线程、系统监控线程
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_cycle_work(alert_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;


	alert_log_change(ALERT_THREAD_RECV_NAME);
	pub_log_info("[%s][%d] alert_cycle_init begin...", __FILE__, __LINE__);

	/* 1. 创建多线程 */
	ret = thread_pool_init(&cycle->threadpool, ALERT_THREAD_TOTAL-1);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Create thread pool failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	pub_log_info("[%s][%d] thread_pool_init success!", __FILE__, __LINE__);

	/* 2. 为线程分配工作 */
	thread_pool_add_worker(cycle->threadpool, alert_push_routine, cycle);
	pub_log_info("[%s][%d] thread_pool_add success!", __FILE__, __LINE__);

#ifdef AIX
	thread_pool_add_worker(cycle->threadpool, alert_sysmnt_routine, cycle);
#endif

	/* 3. 主线程: 侦听端口 */
	ret = alert_recv_routine(cycle);
	if(SW_OK != ret)
	{
		pub_log_info("[%s][%d] Listen port failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_cycle_free
 **功	能: 预警进程空间释放
 **输入参数: 
 **	  cycle: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 释放远程配置资源
 **	 2. 释放错误码动态空间
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_cycle_free(alert_cycle_t *cycle)
{
	int	mqid = 0;
	alert_remote_t *remote=NULL, *remote2=NULL;

	/* 1. 释放远程配置资源 */
	remote = cycle->alertcfg.remote;
	while(NULL != remote)
	{
		if(remote->sckid >= 0)
		{
			close(remote->sckid);
			remote->sckid = -1;
		}

		if(NULL != remote->handle)
		{
			dlclose(remote->handle);
			remote->handle = NULL;
		}

		remote2 = remote;
		remote = remote->next;
		free(remote2), remote2=NULL;
	}
	cycle->alertcfg.remote = NULL;

	/* 2. 释放错误码动态空间 */
	/* 2.1 释放平台错误码空间 */
	if (NULL != cycle->perrno.item)
	{
		free(cycle->perrno.item);
		cycle->perrno.item = NULL;
	}

	/* 2.2 释放交易错误码空间 */
	if(NULL != cycle->oerrno.item)
	{
		free(cycle->oerrno.item);
		cycle->oerrno.item = NULL;
	}

	/* 3. 释放线程池 */
	if(NULL != cycle->threadpool)
	{
		thread_pool_destroy(cycle->threadpool);
		cycle->threadpool = NULL;
	}

	mqid = alert_get_msqid();
	if (mqid > 0)
	{
		pub_msq_rm(mqid);
	}

	/* 4. free mp conn*/
	if (g_in_ares)
	{
		ares_close_fd();
	}
	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_log_change
 **功	能: 改变线程日志文件
 **输入参数: 
 **	 fname: 文件名
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
int alert_log_change(const char *fname)
{
	char fpath[FILE_NAME_MAX_LEN] = {0};

	snprintf(fpath, sizeof(fpath), "%s/log/syslog/%s.log", getenv("SWWORK"), fname);

	pub_log_chglog(SW_LOG_CHG_DBGFILE, fpath);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_errno_load
 **功	能: 加载错误码配置文件
 **输入参数: 
 **	  cycle: 预警进程结构体
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  1. 加载平台错误码配置文件
 **	  2. 加载交易错误码配置文件
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.10 #
 ******************************************************************************/
static sw_int_t alert_errno_load(alert_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;

	/* 1. 加载系统错误码配置文件 */
	ret = alert_perrno_load(cycle);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Load platform errno failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. 加载交易错误码配置文件 */
	ret = alert_oerrno_load(cycle);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Load operation errno failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_errno_get_cfg_path
 **功	能: 获取错误码配置文件的路径
 **输入参数: 
 **	 type: 错误码类型. Range: ERRNO_TYPE_ENUM
 **	 size: 配置文件路径缓存长度
 **输出参数: 
 **		path: 配置文件路径
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
static sw_int_t alert_errno_get_cfg_path(int type, char *path, int size)
{
	switch(type)
	{
		case ERRNO_TYPE_PLAT:
			{
				snprintf(path, size, "%s/%s/%s",
						getenv("SWWORK"), ALERT_PERRNO_CFG_PATH, alert_errno_get_fname(type));
				break;
			}
		case ERRNO_TYPE_OPR:
			{
				snprintf(path, size, "%s/%s/%s",
						getenv("SWWORK"), ALERT_OERRNO_CFG_PATH, alert_errno_get_fname(type));
				break;
			}
		default:
			{
				pub_log_error("[%s][%d] Errno type [%d] is incorrect!", __FILE__, __LINE__);
				return SW_ERR;
			}
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_perrno_load
 **功	能: 加载平台错误码配置文件
 **输入参数: 
 **	  cycle: 预警进程结构体
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  1. 加载错误码配置文件
 **	  2. 从错误码配置中提取有效信息
 **	  3. 验证错误码的正确性
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.10 #
 ******************************************************************************/
static sw_int_t alert_perrno_load(alert_cycle_t *cycle)
{
	int	ret = 0;
	char fname[FILE_NAME_MAX_LEN] = {0};
	sw_xmltree_t	*xml = NULL;

	ret = alert_errno_get_cfg_path(ERRNO_TYPE_PLAT, fname, sizeof(fname));
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get cfg path failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 1. 加载错误码配置文件 */
	xml = pub_xml_crtree(fname);
	if(NULL == xml)
	{
		pub_log_error("[%s][%d] Load errno configure file failed![%s]",
				__FILE__, __LINE__, fname);
		return SW_ERR;
	}

	/* 2. 从错误码配置中提取有效信息 */
	ret = alert_perrno_parse(xml, &(cycle->perrno));
	if(SW_OK != ret)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Get errno information failed![%s]",
				__FILE__, __LINE__, fname);
		return SW_ERR;
	}

	/* 3. 验证错误码的正确性 */
	ret = alert_errno_check(ERRNO_TYPE_PLAT, (void *)&(cycle->perrno));
	if(SW_OK != ret)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Errno configuration is not right![%s]",
				__FILE__, __LINE__, fname);
		return SW_ERR;
	}

	pub_log_info("[%s][%d] Load platform errno configure file success!", __FILE__, __LINE__);
	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_oerrno_load
 **功	能: 加载交易错误码配置文件
 **输入参数: 
 **	  cycle: 预警进程结构体
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  1. 加载错误码配置文件
 **	  2. 从错误码配置中提取有效信息
 **	  3. 验证错误码的正确性
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
static sw_int_t alert_oerrno_load(alert_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	sw_xmltree_t *xml = NULL;
	char fname[FILE_NAME_MAX_LEN] = {0};

	ret = alert_errno_get_cfg_path(ERRNO_TYPE_OPR, fname, sizeof(fname));
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get cfg path failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 1. 加载错误码配置文件 */
	xml = pub_xml_crtree(fname);
	if(NULL == xml)
	{
		pub_log_error("[%s][%d] Load operation errno failed![%s]", __FILE__, __LINE__, fname);
		return SW_ERR;
	}

	/* 2. 从错误码配置中提取有效信息 */
	ret = alert_oerrno_parse(xml, &(cycle->oerrno));
	if(SW_OK != ret)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Get operation errno failed![%s]", __FILE__, __LINE__, fname);
		return SW_ERR;
	}

	/* 3. 验证错误码的正确性 */
	ret = alert_errno_check(ERRNO_TYPE_OPR, (void *)&(cycle->oerrno));
	if(SW_OK != ret)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Operation errno is not right![%s]", __FILE__, __LINE__, fname);
		return SW_ERR;
	}

	pub_log_info("[%s][%d] Load operation errno success!", __FILE__, __LINE__);
	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_perrno_parse
 **功	能: 加载平台错误码配置文件
 **输入参数: 
 **	  cycle: 预警进程结构体
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  1. 计算配置文件中错误码个数
 **	  2. 分配错误码集合空间
 **	  3. 依次提取有效信息
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.10 #
 ******************************************************************************/
static sw_int_t alert_perrno_parse(sw_xmltree_t *xml, perrno_cfg_t *errcfg)
{
	int num = 0, ret = 0;
	perrno_item_t *erritem = NULL;
	sw_xmlnode_t *node = NULL;

	/* 1. 计算配置文件中错误码个数 */
	node = pub_xml_locnode(xml, ".ERROR.ITEM");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Errno configuration is empty!", __FILE__, __LINE__);
		return SW_ERR;
	}

	num++;
	while(NULL != node->next)
	{
		num++;
		node = node->next;
	}

	/* 2. 分配错误码集合空间 */
	errcfg->num = num;
	errcfg->item = (perrno_item_t*)calloc(1, num*sizeof(perrno_item_t));
	if(NULL == errcfg->item)
	{
		pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. 依次提取有效信息 */
	erritem = errcfg->item;
	node = xml->current;
	while(NULL != node)
	{
		xml->current = node;

		ret = alert_perrno_parse_item(xml, erritem);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Errno configuration is not right!", __FILE__, __LINE__);
			return SW_ERR;
		}

		erritem++;
		node = node->next;
	}

	errcfg->minno = errcfg->item[0]._errno;
	errcfg->maxno = errcfg->item[errcfg->num - 1]._errno;

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_perrno_parse_item
 **功	能: 从平台错误码配置项中提取有效信息
 **输入参数: 
 **	  xml: 错误码配置文件
 **	  item: 错误码配置项
 **输出参数: 
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.10 #
 ******************************************************************************/
static sw_int_t alert_perrno_parse_item(sw_xmltree_t *xml, perrno_item_t *item)
{
	sw_xmlnode_t *attr = NULL;

	/* 1. 错误码: 错误唯一标识，不允许重复 */
	attr = pub_xml_locnode(xml, "ERRNO");
	if((NULL == attr)
			|| (NULL == attr->value) || ('\0' == attr->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure ERRNO!", __FILE__, __LINE__);
		return SW_ERR;
	}
	item->_errno = atoi(attr->value);

	/* 2. 错误级别: 非空 */
	attr = pub_xml_locnode(xml, "LEVEL");
	if((NULL == attr)
			|| (NULL == attr->value) || ('\0' == attr->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure LEVEL!", __FILE__, __LINE__);
		return SW_ERR;
	}
	item->level = atoi(attr->value);
	if((item->level < ERR_LEVEL_PROMPT) || (item->level >= ERR_LEVEL_TOTAL))
	{
		pub_log_error("[%s][%d] Level is not right! level:[%d]",
				__FILE__, __LINE__, item->level);
		return SW_ERR;
	}

	/* 3. 错误描述: 非空 */
	attr = pub_xml_locnode(xml, "DESC");
	if (attr != NULL && attr->value != NULL && attr->value[0] != '\0')
	{
		snprintf(item->desc, sizeof(item->desc), "%s", attr->value);
	}

	/* 4. 错误原因: 非空 */
	attr = pub_xml_locnode(xml, "REASON");
	if (attr != NULL && attr->value != NULL && attr->value[0] != '\0')
	{
		snprintf(item->reason, sizeof(item->reason), "%s", attr->value);
	}

	/* 5. 解决方案: 非空 */
	attr = pub_xml_locnode(xml, "SOLVE");
	if (attr != NULL && attr->value != NULL && attr->value[0] != '\0')
	{
		snprintf(item->solve, sizeof(item->solve), "%s", attr->value);
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_errno_check
 **功	能: 检查平台/交易错误码的正确性
 **输入参数: 
 **	  type: 平台错误码类型，其取值范围是枚举类型ERRNO_TYPE_ENUM中定义
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  1. 验证错误码范围是否正确
 **	  2. 验证错误码中间值是否有序
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.10 #
 ******************************************************************************/
static sw_int_t alert_errno_check(int type, void *errcfg)
{
	int idx = 0;
	perrno_cfg_t *perrcfg = NULL;
	oerrno_cfg_t *oerrcfg = NULL;

	switch(type)
	{
		case ERRNO_TYPE_PLAT:
			{
				perrcfg = (perrno_cfg_t *)errcfg;

				/* 1. 验证错误码范围是否正确 */
				if((perrcfg->minno < alert_errno_get_begin(type))
						|| (perrcfg->maxno > alert_errno_get_end(type)))
				{
					pub_log_error("[%s][%d] Errno is out of range! minno:[%d][%d] maxno:[%d][%d]",
							__FILE__, __LINE__, perrcfg->minno, 
							alert_errno_get_begin(type), perrcfg->maxno, alert_errno_get_end(type));
					return SW_ERR;
				}

				/* 2. 验证错误码中间值是否有序 */
				for(idx=1; idx<perrcfg->num; idx++)
				{
					if(perrcfg->item[idx]._errno <= perrcfg->item[idx-1]._errno)
					{
						pub_log_error("[%s][%d] The sequence of errno is not right! errno:[%d]",
								__FILE__, __LINE__, perrcfg->item[idx]._errno);
						return SW_ERR;
					}
				}

				return SW_OK;
			}
		case ERRNO_TYPE_OPR:
			{
				oerrcfg = (oerrno_cfg_t *)errcfg;

				/* 1. 验证错误码范围是否正确 */
				if((oerrcfg->minno < alert_errno_get_begin(type))
						|| (oerrcfg->maxno > alert_errno_get_end(type)))
				{
					pub_log_error("[%s][%d] Errno is out of range! minno:[%d][%d] maxno:[%d][%d]",
							__FILE__, __LINE__, oerrcfg->minno, 
							alert_errno_get_begin(type), oerrcfg->maxno, alert_errno_get_end(type));
					return SW_ERR;
				}

				/* 2. 验证错误码中间值是否有序 */
				for(idx=1; idx<oerrcfg->num; idx++)
				{
					if(oerrcfg->item[idx]._errno <= oerrcfg->item[idx-1]._errno)
					{
						pub_log_error("[%s][%d] The squence of errno is not right! errno[%d]",
								__FILE__, __LINE__, oerrcfg->item[idx]._errno);
						return SW_ERR;
					}
				}

				return SW_OK;
			}
	}

	pub_log_info("[%s][%d] The type of errno is not right!", __FILE__, __LINE__);
	return SW_ERR;
}

/******************************************************************************
 **函数名称: alert_oerrno_parse
 **功	能: 加载交易错误码配置文件
 **输入参数: 
 **	  cycle: 预警进程结构体
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  1. 计算配置文件中错误码个数
 **	  2. 分配错误码集合空间
 **	  3. 依次提取有效信息
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
static sw_int_t alert_oerrno_parse(sw_xmltree_t *xml, oerrno_cfg_t *errcfg)
{
	int num = 0, ret = 0;
	oerrno_item_t *erritem = NULL;
	sw_xmlnode_t *node = NULL;

	/* 1. 计算配置文件中错误码个数 */
	node = pub_xml_locnode(xml, ".ERROR.ITEM");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Errno configuration is empty!", __FILE__, __LINE__);
		return SW_ERR;
	}

	num++;
	while(NULL != node->next)
	{
		num++;
		node = node->next;
	}

	/* 2. 分配错误码集合空间 */
	errcfg->num = num;
	errcfg->item = (oerrno_item_t*)calloc(1, num*sizeof(oerrno_item_t));
	if(NULL == errcfg->item)
	{
		pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. 依次提取有效信息 */
	erritem = errcfg->item;
	node = xml->current;
	while(NULL != node)
	{
		xml->current = node;

		ret = alert_oerrno_parse_item(xml, erritem);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Errno information is not right!", __FILE__, __LINE__);
			return SW_ERR;
		}

		erritem++;
		node = node->next;
	}

	errcfg->minno = errcfg->item[0]._errno;
	errcfg->maxno = errcfg->item[errcfg->num - 1]._errno;

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_oerrno_parse_item
 **功	能: 从平台错误码配置项中提取有效信息
 **输入参数: 
 **	  xml: 错误码配置文件
 **	  item: 错误码配置项
 **输出参数: 
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	  依次加载各标签配置的值
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
static sw_int_t alert_oerrno_parse_item(sw_xmltree_t *xml, oerrno_item_t *item)
{
	sw_xmlnode_t *attr = NULL;

	/* 1. 错误码: 错误唯一标识，不允许重复 */
	attr = pub_xml_locnode(xml, "ERRNO");
	if((NULL == attr)
			|| (NULL == attr->value) || ('\0' == attr->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure ERRNO!", __FILE__, __LINE__);
		return SW_ERR;
	}
	item->_errno = atoi(attr->value);

	/* 2. 错误级别: 非空 */
	attr = pub_xml_locnode(xml, "LEVEL");
	if((NULL == attr)
			|| (NULL == attr->value) || ('\0' == attr->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure LEVEL!", __FILE__, __LINE__);
		return SW_ERR;
	}
	item->level = atoi(attr->value);
	if((item->level < ERR_LEVEL_PROMPT) || (item->level >= ERR_LEVEL_TOTAL))
	{
		pub_log_error("[%s][%d] Level is not right! level:[%d]",
				__FILE__, __LINE__, item->level);
		return SW_ERR;
	}

	/* 3. 错误描述: 非空 */
	attr = pub_xml_locnode(xml, "DESC");
	if (attr != NULL && attr->value != NULL && attr->value[0] != '\0')
	{
		snprintf(item->desc, sizeof(item->desc), "%s", attr->value);
	}

	/* 4. 错误原因: 非空 */
	attr = pub_xml_locnode(xml, "REASON");
	if (attr != NULL && attr->value != NULL && attr->value[0] != '\0')
	{
		snprintf(item->reason, sizeof(item->reason), "%s", attr->value);
	}

	/* 5. 解决方案: 非空 */
	attr = pub_xml_locnode(xml, "SOLVE");
	if (attr != NULL && attr->value != NULL && attr->value[0] != '\0')
	{
		snprintf(item->solve, sizeof(item->solve), "%s", attr->value);
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_remote_connect
 **功	能: 连接远程服务
 **输入参数: 
 **	  remote: 远程服务配置信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 扫描远程服务配置链表，依次连接远程服务
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_remote_connect(alert_cycle_t *cycle)
{
	alert_remote_t *remote = NULL;


	remote = cycle->alertcfg.remote;
	while(NULL != remote)
	{
		if(SWITCH_ON != remote->isuse)
		{
			remote->sckid = -1;
			remote = remote->next;
			continue;
		}

		remote->sckid = alert_connect(remote->ipaddr, remote->port);
		if(remote->sckid < 0)
		{
			pub_log_info("[%s][%d] Connect remote server failed! ipaddr:%s[%d]",
					__FILE__, __LINE__, remote->ipaddr, remote->port);
		}
		else
		{
			pub_log_info("[%s][%d] Connect remote server success! ipaddr:[%s][%d] sockid=[%d]",
					__FILE__, __LINE__, remote->ipaddr, remote->port, remote->sckid);
		}
		remote->starttime = (long)time(NULL);
		remote = remote->next;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_remote_keepalive
 **功	能: 远程服务连接保活
 **输入参数: 
 **	  remote: 远程服务配置信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 创建套接字
 **	 2. 连接远程服务
 **	 3. 设置为非阻塞状态
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_remote_keepalive(alert_remote_t *remote)
{

	if((SWITCH_ON != remote->isuse) || (remote->sckid >= 0))
	{
		return SW_OK;
	}

	remote->sckid = alert_connect(remote->ipaddr, remote->port);
	if(remote->sckid < 0)
	{
		pub_log_info("[%s][%d] Keepalive failed! ipaddr:%s[%d]",
				__FILE__, __LINE__, remote->ipaddr, remote->port);
		return SW_ERR;
	}

	pub_log_info("[%s][%d] Keepalive success! ipaddr:%s[%d]", __FILE__, __LINE__, remote->ipaddr, remote->port);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_set_packcb
 **功	能: 设置预警回调函数
 **输入参数: 
 **	  cycle: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_set_packcb(alert_cycle_t *cycle)
{
	sw_int_t ret = SW_ERR;
	alert_remote_t *remote = NULL;

	remote = cycle->alertcfg.remote;
	while(NULL != remote)
	{
		ret = alert_set_packfunc(remote, alert_remote_pack);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Set the pack function of remote server failed!",
					__FILE__, __LINE__);
			return SW_ERR;
		}

		remote = remote->next;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_set_packfunc
 **功	能: 设置预警组包回调函数
 **输入参数: 
 **	  remote: 远程服务配置
 **	  func: 默认组包函数
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **	  当未配置远程服务组包函数时，则使用默认组包函数
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_set_packfunc(alert_remote_t *remote, const alert_pack_func_t func)
{
	char *penv = getenv("SWWORK"),
	     libso[FILE_NAME_MAX_LEN] = {0};

	if(NULL == penv)
	{
		pub_log_error("[%s][%d] Didn't configure SWWORK!", __FILE__, __LINE__);
		return SW_ERR;
	}

	if(SWITCH_ON != remote->isuse)
	{
		remote->packfunc = NULL;
		return SW_OK;
	}

	if((strlen(remote->libname) > 0)
			&& (strlen(remote->funcname) > 0))
	{
		AlertGetLibPath(libso, sizeof(libso), remote->libname);

		remote->handle = (void*)dlopen(libso, RTLD_NOW|RTLD_GLOBAL);
		if(NULL == remote->handle)
		{
			pub_log_error("[%s][%d] Open dynamic library failed![%s][%s]",
					__FILE__, __LINE__, libso, dlerror());
			return SW_ERR;
		}

		remote->packfunc = (alert_pack_func_t)dlsym(remote->handle, remote->funcname);
		if(NULL == remote->packfunc)
		{
			dlclose(remote->handle);
			remote->handle = NULL;
			pub_log_error("[%s][%d] Get the function of dynamic library failed![%s][%s][%s]",
					__FILE__, __LINE__, libso, remote->funcname, dlerror());
			return SW_ERR;
		}
	}
	else
	{
		remote->packfunc = func;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_push_routine
 **功	能: 预警推送线程入口功能
 **输入参数: 
 **	  args: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 接收预警信息
 **	 3. 发送预警信息
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static void *alert_push_routine(void *args)
{
	int ret=0, msqid=0;
	alert_msq_t msg;
	alert_cycle_t *cycle = (alert_cycle_t *)args;

	alert_log_change(ALERT_THREAD_PUSH_NAME);
	pub_log_info("[%s][%d] Begin to run the thread of alert push...", __FILE__, __LINE__);

	msqid = alert_get_msqid();
	pub_log_debug("[%s][%d] msqid=[%d]", __FILE__, __LINE__, msqid);
	while(1)
	{
		memset(&msg, 0, sizeof(msg));

		/* 1. 接收预警信息 */
		ret = msgrcv(msqid, &msg, ALERT_MTEXT_MAX_LEN-1, 0, 0);
		if(ret < 0)
		{
			if(EINTR == ret)
			{
				pub_log_info("[%s][%d] Interrupt accept alert message!", __FILE__, __LINE__);
				continue;
			}
			pub_log_error("[%s][%d] Accept alert message exception! msqid:[%d] errmsg:[%d][%s]",
					__FILE__, __LINE__, msqid, errno, strerror(errno));
			return NULL;
		}

		pub_log_info("[%s][%d] Accepted alert message success...", __FILE__, __LINE__);

		/* 2. 发送预警信息 */
		ret = alert_msg_deal(cycle, &msg);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Send alert message failed!", __FILE__, __LINE__);
			continue;
		}
	}

	return NULL;
}

/******************************************************************************
 **函数名称: alert_platmnt_routine
 **功	能: 平台监控线程 入口函数
 **输入参数: 
 **	  args: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/



/* Do nothing */


/******************************************************************************
 **函数名称: alert_sysmnt_routine
 **功	能: 系统监控线程 入口函数
 **输入参数: 
 **	  args: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
typedef struct
{
	char	label[64];
	char	identifier[64];
	char	datetime[64];
	char	machineid[64];
	char	nodeid[64];
	char	signo[32];
	char	pid[32];
	char	corefile[256];
	char	pname[128];
}errpt_t;

typedef struct node
{
	errpt_t	*data;
	struct node	*next;
}node_t, *list_t;

int init_list(list_t *list)
{
	*list = (list_t)calloc(1, sizeof(node_t));
	if (*list == NULL)
	{
		printf("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	(*list)->next = NULL;

	return 0;
}

int add_node(list_t *list, errpt_t *data)
{
	list_t	p = NULL;
	list_t	s = NULL;

	if (*list == NULL)
	{
		printf("[%s][%d] list is null!", __FILE__, __LINE__);
		return -1;
	}

	p = *list;
	while (p->next != NULL)
	{
		p = p->next;
	}

	s = (list_t)calloc(1, sizeof(node_t));
	if (s == NULL)
	{
		printf("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	s->data = calloc(1, sizeof(errpt_t));
	if (s->data == NULL)
	{
		printf("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	memcpy(s->data, data, sizeof(errpt_t));
	s->next = p->next;
	p->next = s;

	return 0;
}

int destroy_list(list_t *list)
{
	list_t	p = NULL;
	list_t	q = NULL;

	q = *list;
	if (q == NULL)
	{
		return 0;
	}

	while (q->next != NULL)
	{
		p = q->next;
		q->next = p->next;
		if (p->data != NULL)
		{
			free(p->data);
			p->data = NULL;
		}
		free(p);
		p = NULL;
	}
	free(*list);
	*list = NULL;

	return 0;
}

int print_list(list_t list)
{
	list_t	p = NULL;

	if (list == NULL)
	{
		return 0;
	}

	p = list->next;
	while (p != NULL)
	{
		pub_log_info("label=[%s] identifier=[%s] datetime=[%s] machineid=[%s] nodeid=[%s] signo=[%s] pid=[%s] corefile=[%s] pname=[%s]\n",
				p->data->label, p->data->identifier, p->data->datetime, p->data->machineid,
				p->data->nodeid, p->data->signo, p->data->pid, p->data->corefile, p->data->pname);
		p = p->next;
	}

	return 0;
}

char *zip_space(char *str)
{
	char*ptr = NULL;
	char*destr = NULL;

	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			*ptr++ = *str;
		}
		str++;
	}
	*ptr = '\0';

	return destr;
}

char *zip_head(char *str)
{
	char*ptr = NULL;
	char*destr = NULL;

	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			break;
		}
		str++;
	}
	while (*str != '\0')
	{
		*ptr++ = *str++;
	}
	*ptr = '\0';

	return destr;
}

char *zip_tail(char *str)
{
	char*ptr = NULL;
	char*destr = NULL;

	destr = str;
	ptr = str + strlen(str) - 1;
	while (str <= ptr)
	{
		if ((*ptr != ' ') && (*ptr != '\t'))
		{
			ptr++;
			*ptr = '\0';
			break;
		}
		ptr--;
	}

	return destr;
}

char *zip_ht(char *str)
{
	char	*dest = str;

	zip_head(str);
	zip_tail(str);

	return dest;
}

static void *alert_sysmnt_routine(void *args)
{
	int	ret = 0;
	int	len = 0;
	int	first = 0;
	FILE	*fp = NULL;
	char	buf[256];
	char	cmd[128];
	char	time[32];
	char	filename[128];
	char	line[1024];
	errpt_t	errpt;
	list_t	list = NULL;
	struct tm	*pstm;
	struct timeb	tmb;
	alert_error_t	mtext;
	alert_cycle_t	*cycle = (alert_cycle_t *)args;

	alert_log_change(ALERT_THREAD_SYSMNT_NAME);
	pub_log_info("[%s][%d] Begin to run the thread of system monitor...", __FILE__, __LINE__);

	while (1)
	{
		memset(buf, 0x0, sizeof(buf));
		memset(cmd, 0x0, sizeof(cmd));
		memset(time, 0x0, sizeof(time));
		memset(filename, 0x0, sizeof(filename));
		memset(line, 0x0, sizeof(line));

		memset(&tmb, 0x0, sizeof(tmb));
		ftime(&tmb);
		tmb.time -= 90;
		pstm = localtime(&tmb.time);
		strftime(time, 32, "%m%d%H%M%y", pstm);

		memset(filename, 0x0, sizeof(filename));
		sprintf(filename, "%s/tmp/.errpt.%ld", getenv("SWWORK"), pthread_self());
		sprintf(cmd, "errpt -a -s %s -J CORE_DUMP > %s", time, filename);
		ret = system(cmd);
		if (ret)
		{
			printf("[%s][%d] system error! errno=[%d]:[%s] cmd=[%s]\n",
					__FILE__, __LINE__, errno, strerror(errno), cmd);
			return NULL;
		}

		fp = fopen(filename, "r");
		if (fp == NULL)
		{
			printf("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
					__FILE__, __LINE__, filename, errno, strerror(errno));
			return NULL;
		}

		first = 1;
		memset(&errpt, 0x0, sizeof(errpt));
		init_list(&list);
		while (1)
		{
			memset(line, 0x0, sizeof(line));
			if (fgets(line, sizeof(line), fp) == NULL)
			{
				break;
			}

			len = strlen(line);
			if (line[len - 1] == '\n')
			{
				line[len - 1] = '\0';
			}		

			if (strncmp(line, SEPLINE, strlen(SEPLINE)) == 0)
			{
				if (first == 0)
				{
					add_node(&list, &errpt);
				}
				else
				{
					first = 0;
					memset(&errpt, 0x0, sizeof(errpt_t));
				}
			}

			if (strncmp(line, LABEL, strlen(LABEL)) == 0)
			{
				strcpy(errpt.label, zip_ht(line + strlen(LABEL)));
			}

			if (strncmp(line, IDENTIFIER, strlen(IDENTIFIER)) == 0)
			{
				strcpy(errpt.identifier, zip_ht(line + strlen(IDENTIFIER)));
			}

			if (strncmp(line, DATATIME, strlen(DATATIME)) == 0)
			{
				strcpy(errpt.datetime, zip_ht(line + strlen(DATATIME)));
			}

			if (strncmp(line, MACHINEID, strlen(MACHINEID)) == 0)
			{
				strcpy(errpt.machineid, zip_space(line + strlen(MACHINEID)));
			}

			if (strncmp(line, NODEID, strlen(NODEID)) == 0)
			{
				strcpy(errpt.nodeid, zip_space(line + strlen(NODEID)));
			}

			if (strncmp(line, SIGNO, strlen(SIGNO)) == 0)
			{
				memset(line, 0x0, sizeof(line));
				fgets(line, sizeof(line), fp);
				len = strlen(line);
				if (line[len - 1] == '\n')
				{
					line[len - 1] = '\0';
				}
				strcpy(errpt.signo, zip_space(line));
			}

			if (strncmp(line, PROCESSID, strlen(PROCESSID)) == 0)
			{
				memset(line, 0x0, sizeof(line));
				fgets(line, sizeof(line), fp);
				len = strlen(line);
				if (line[len - 1] == '\n')
				{
					line[len - 1] = '\0';
				}
				strcpy(errpt.pid, zip_space(line));
			}

			if (strncmp(line, COREFILE, strlen(COREFILE)) == 0)
			{
				memset(line, 0x0, sizeof(line));
				fgets(line, sizeof(line), fp);
				len = strlen(line);
				if (line[len - 1] == '\n')
				{
					line[len - 1] = '\0';
				}
				strcpy(errpt.corefile, zip_space(line));
			}

			if (strncmp(line, PROGRAM, strlen(PROGRAM)) == 0)
			{
				memset(line, 0x0, sizeof(line));
				fgets(line, sizeof(line), fp);
				len = strlen(line);
				if (line[len - 1] == '\n')
				{
					line[len - 1] = '\0';
				}
				strcpy(errpt.pname, zip_space(line));
			}
		}
		fclose(fp);
		remove(filename);

		if (first == 0)
		{
			add_node(&list, &errpt);
		}

		list_t	p = list->next;
		p = list->next;
		while (p != NULL)
		{
			memset(&mtext, 0x0, sizeof(mtext));
			mtext._errno = ERR_CORE;
			snprintf(mtext.remark, sizeof(mtext.remark), "机器[%s]的进程[%s][%s]在[%s]时coredump,core文件为[%s],请尽快解决!",
					p->data->nodeid, p->data->pname, p->data->pid, p->data->datetime, p->data->corefile);
			alert_platerr_deal(cycle, (const alert_error_t*)&mtext);
			p = p->next;
		}
		destroy_list(&list);

		sleep(60);
	}

	return NULL;
}

/******************************************************************************
 **函数名称: alert_recv_routine
 **功	能: 侦听线程 入口函数(主线程)
 **输入参数: 
 **	  args: 预警上下文
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_recv_routine(alert_cycle_t *cycle)
{
	int ret=0, maxfd=-1, cmdfd=-1;
	int	len = 0;
	long	now = 0;
	long	lasttime = 0;
	fd_set readset;
	char	pack[ALERT_NBUF_MAX_LEN];
	struct timeval timeout;
	alert_remote_t *remote = NULL;


	pub_log_info("[%s][%d] Begin to listen socket...", __FILE__, __LINE__);

	/* 1. 创建接收平台命令套接字 */
	cmdfd = udp_bind(PROC_NAME_ALERT);
	if(cmdfd < 0)
	{
		pub_log_error("[%s][%d] Create udp socket failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	while(1)
	{
		maxfd = -1;
		FD_ZERO(&readset);

		/* 2. 加入读集和套接字保活 */
		remote = cycle->alertcfg.remote;
		while (NULL != remote)
		{
			now = (long)time(NULL);
			/*** 如果连接远端服务失败,等待60秒之后再选择重发 ***/
			if (lasttime == 0 || (lasttime > 0 && now - lasttime > 60))
			{
				/*** 把文件中未发送的消息全部发送 ***/
				while (1)
				{
					len = 0;
					memset(pack, 0x0, sizeof(pack));
					ret = alert_get_msg_from_file(pack, &len);
					if (ret == -1)
					{
						pub_log_info("[%s][%d] 从文件中读取消息时发生错误!", __FILE__, __LINE__);
						break;
					}
					else if (ret == 0)
					{
						break;
					}

					ret = alert_check_msg_time(pack + 8, len - 8);
					if (ret == 2)
					{
						pub_log_info("[%s][%d] 该预警信息已过期,不再进行发送! Msg:[%s]",
								__FILE__, __LINE__, pack);
						pub_time_timer(0, 100000);
						continue;
					}

					ret = alert_nsend(remote, pack, len);
					if (ret < 0)
					{
						lasttime = (long)time(NULL);
						remote->sckid = -1;
						pub_log_info("[%s][%d] 发送消息失败!", __FILE__, __LINE__);
						pub_log_info("[%s][%d] Pack:[%s]", __FILE__, __LINE__, pack);
						break;
					}
					lasttime = 0;
					pub_log_bin(SW_LOG_DEBUG, pack, len,"[%s][%d] push send msg success,len=[%d]", __FILE__, __LINE__, len);
					pub_time_timer(0, 100000);
				}
			}	
			remote = remote->next;
		}

		FD_SET(cmdfd, &readset);
		maxfd = ((cmdfd > maxfd)? cmdfd: maxfd);

		/* 3. 开始侦听套接字 */
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = ALERT_SELECT_TIMEOUT_SEC;
		timeout.tv_usec = ALERT_SELECT_TIMEOUT_USEC;

		ret = select(maxfd+1, &readset, NULL, NULL, &timeout);
		if(ret < 0)
		{
			if(EINTR == errno)
			{
				continue;
			}
			pub_log_error("[%s][%d] Select failed! errmsg:[%d][%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;
		}
		else if(0 == ret)
		{
			continue;
		}


		/* 4.1 是否为SWMAN命令 */
		if(FD_ISSET(cmdfd, &readset))
		{
			ret = alert_swcmd_recv(cmdfd);
			if(ALERT_CMD_EXIT == ret)
			{
				pub_log_info("[%s][%d] Exit alert! ", __FILE__, __LINE__);
				return SW_OK;
			}
			else if(SW_OK != ret)
			{
				pub_log_error("[%s][%d] Receive platform command failed! ", __FILE__, __LINE__);
				return SW_ERR;
			}
		}
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_msg_deal
 **功	能: 处理预警信息
 **输入参数: 
 **	  cycle: 预警上下文
 **	  msg: 预警队列信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 判断信息类型
 **	 2. 调用发送接口
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_msg_deal(alert_cycle_t *cycle, alert_msq_t *msg)
{
	switch(msg->mtype)
	{
		case ALERT_TYPE_PLATERR:
			{
				return alert_platerr_deal(cycle, (const alert_error_t*)msg->mtext);
			}
		case ALERT_TYPE_OPRERR:
			{
				return alert_oprerr_deal(cycle, (const alert_error_t*)msg->mtext);
			}
	}

	pub_log_info("[%s][%d] The type of alert message is not right!", __FILE__, __LINE__);
	return SW_ERR;
}

/******************************************************************************
 **函数名称: alert_platerr_deal
 **功	能: 平台错误信息处理
 **输入参数: 
 **	  cycle: 预警上下文
 **	  alertmsg: 预警队列信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 查找错误码配置信息
 **	 2. 错误预警级别是否达到要求
 **	 3. 执行预警触发的行动
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_platerr_deal(const alert_cycle_t *cycle, const alert_error_t *mtext)
{
	sw_int_t ret = SW_ERR;
	alert_perrmsg_t perrmsg; 
	const perrno_item_t *item = NULL;

	/* 1. 查找错误码配置信息 */
	item = alert_perrno_search(cycle, mtext->_errno);
	if(NULL == item)
	{
		pub_log_info("[%s][%d] Didn't find errno configuration! errno:[%d][%s]",
				__FILE__, __LINE__, mtext->_errno, mtext->remark);
		return SW_OK;
	}

	/***
	  alert_perrno_print(item);
	 ***/

	/* 2. 错误预警级别是否达到要求 */
	if(!AlertLevelIsOk(cycle, item->level))
	{
		pub_log_info("[%s][%d] Level is lower than alert level! level:[%d][%d]",
				__FILE__, __LINE__, item->level, cycle->alertcfg.level);
		return SW_OK;
	}

	perrmsg.mtext = mtext;
	perrmsg.item = item;

	/* 3. 执行预警触发的行动 */
	ret = alert_platerr_send(cycle, &perrmsg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Send alert message failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_platerr_send
 **功	能: 发送平台错误信息
 **输入参数: 
 **	  cycle: 预警上下文
 **	  perrmsg: 平台错误吗预警信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 错误预警级别是否达到要求
 **	 2. 组报文
 **	 3. 发送报文
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.15 #
 ******************************************************************************/
static sw_int_t alert_platerr_send(const alert_cycle_t *cycle, alert_perrmsg_t *perrmsg)
{
	sw_int_t ret = SW_ERR;
	const perrno_item_t *item = NULL;
	alert_remote_t	*remote = NULL;
	char packmsg[ALERT_NBUF_MAX_LEN] = {0};

	item = perrmsg->item;
	remote = cycle->alertcfg.remote;
	while(NULL != remote)
	{
		if((SWITCH_ON != remote->isuse)
				|| (!AlertSvrLevelIsOk(remote, item->level)))
		{
			remote = remote->next;
			continue;
		}

		/* 1. 组报文 */
		pub_log_info("[%s][%d] Pack package...", __FILE__, __LINE__);
		ret = remote->packfunc(ALERT_TYPE_PLATERR, (const void*)perrmsg, packmsg, sizeof(packmsg));
		if(SW_OK != ret)
		{
			pub_log_info("[%s][%d] Pack package failed!", __FILE__, __LINE__);
			remote = remote->next;
			continue;
		}

		pub_log_bin(SW_LOG_INFO, packmsg, strlen(packmsg), "[%s][%d] len=[%d]", __FILE__, __LINE__, strlen(packmsg));	
		/* 2. 发送报文 */
		ret = alert_nsend(remote, packmsg, strlen(packmsg));
		if(ret < 0)
		{
			pub_log_info("[%s][%d] Send package failed! sckid:%d ipaddr:%s[%d] errmsg:[%d][%s]",
					__FILE__, __LINE__, remote->sckid, remote->ipaddr, remote->port, errno, strerror(errno));
			remote = remote->next;
			continue;
		}

		pub_log_info("[%s][%d] Send package success! [%s][%d]", __FILE__, __LINE__, remote->ipaddr, remote->port);
		remote = remote->next;
	}	

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_perrno_search
 **功	能: 查找平台错误码信息
 **输入参数: 
 **	  cycle: 预警上下文
 **	  _errno: 平台错误码
 **输出参数: NONE
 **返	回: item: 错误码配置项信息
 **实现描述: 
 **	 1. 获取平台错误码类型
 **	 2. 查找平台错误码配置信息
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static const perrno_item_t *alert_perrno_search(const alert_cycle_t *cycle, int _errno)
{
	int type = 0;
	const perrno_item_t *item = NULL;

	type = alert_perrno_get_type(_errno);
	if(ERRNO_TYPE_UNKNOW == type)
	{
		pub_log_error("[%s][%d] Get the type of errno failed![%d]", __FILE__, __LINE__, _errno);
		return NULL;
	}

	item = alert_perrno_bsearch(&cycle->perrno, _errno);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Didn't find errno![%d]", __FILE__, __LINE__, _errno);
		return NULL;
	}

	return item;
}

/******************************************************************************
 **函数名称: alert_perrno_bsearch
 **功	能: 二分查找平台错误码配置信息
 **输入参数: 
 **	  errcfg: 平台错误码配置信息
 **	  _errno: 平台错误码
 **输出参数: NONE
 **返	回: item: 错误码配置项信息
 **实现描述: 
 **	 由二分查找算法实现
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static const perrno_item_t *alert_perrno_bsearch(const perrno_cfg_t *errcfg, int _errno)
{
	int low=0, high=0, mid=0;
	perrno_item_t *item = errcfg->item;

	low = 0;
	high = errcfg->num-1;

	while(low <= high)
	{
		mid = (low+high) >> 1;
		if(_errno == item[mid]._errno)
		{
			return item+mid;
		}
		else if(item[mid]._errno > _errno)
		{
			high = mid - 1;
		}
		else if(item[mid]._errno < _errno)
		{
			low = mid + 1;
		}
	}

	return NULL;
}

/******************************************************************************
 **函数名称: alert_perrno_get_type
 **功	能: 获取平台错误码类型
 **输入参数: 
 **	  no: 错误码
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **	   错误码类型在枚举ERRNO_TYPE_ENUM中定义
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_perrno_get_type(int _errno)
{

	if((_errno >= alert_errno_get_begin(ERRNO_TYPE_PLAT)) 
			&& (_errno <= alert_errno_get_end(ERRNO_TYPE_PLAT)))
	{
		return ERRNO_TYPE_PLAT;
	}

	return ERRNO_TYPE_UNKNOW;
}

/******************************************************************************
 **函数名称: alert_oerrno_get_type
 **功	能: 获取交易错误码类型
 **输入参数: 
 **	  no: 错误码
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **	   错误码类型在枚举ERRNO_TYPE_ENUM中定义
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_oerrno_get_type(int _errno)
{
	if((_errno >= alert_errno_get_begin(ERRNO_TYPE_OPR)) 
			&& (_errno <= alert_errno_get_end(ERRNO_TYPE_OPR)))
	{
		return ERRNO_TYPE_OPR;
	}

	return ERRNO_TYPE_UNKNOW;
}

/******************************************************************************
 **函数名称: alert_oerrno_search
 **功	能: 查找交易错误码信息
 **输入参数: 
 **	  cycle: 预警上下文
 **	  _errno: 平台错误码
 **输出参数: NONE
 **返	回: item: 错误码配置项信息
 **实现描述: 
 **	 1. 获取平台错误码类型
 **	 2. 查找平台错误码配置信息
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static const oerrno_item_t *alert_oerrno_search(const alert_cycle_t *cycle, int _errno)
{
	int type = 0;
	const oerrno_item_t *item = NULL;

	type = alert_oerrno_get_type(_errno);
	if(ERRNO_TYPE_UNKNOW == type)
	{
		pub_log_error("[%s][%d] Get the type of errno failed![%d]", __FILE__, __LINE__, _errno);
		return NULL;
	}

	item = alert_oerrno_bsearch(&cycle->oerrno, _errno);
	if(NULL == item)
	{
		pub_log_error("[%s][%d] Didn't find errno![%d]", __FILE__, __LINE__, _errno);
		return NULL;
	}

	return item;
}

/******************************************************************************
 **函数名称: alert_oerrno_bsearch
 **功	能: 二分查找交易错误码配置信息
 **输入参数: 
 **	  errcfg: 平台错误码配置信息
 **	  _errno: 平台错误码
 **输出参数: NONE
 **返	回: item: 错误码配置项信息
 **实现描述: 
 **	 由二分查找算法实现
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static const oerrno_item_t *alert_oerrno_bsearch(const oerrno_cfg_t *errcfg, int _errno)
{
	int low=0, high=0, mid=0;
	oerrno_item_t *item = errcfg->item;

	low = 0;
	high = errcfg->num-1;

	while(low <= high)
	{
		mid = (low+high) >> 1;
		if(_errno == item[mid]._errno)
		{
			return item+mid;
		}
		else if(item[mid]._errno > _errno)
		{
			high = mid - 1;
		}
		else if(item[mid]._errno < _errno)
		{
			low = mid + 1;
		}
	}

	return NULL;
}

/******************************************************************************
 **函数名称: alert_oprerr_deal
 **功	能: 交易错误信息处理
 **输入参数: 
 **	  cycle: 预警上下文
 **	  alertmsg: 预警队列信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 判断信息类型
 **	 2. 调用发送接口
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_oprerr_deal(alert_cycle_t *cycle, const alert_error_t *mtext)
{
	sw_int_t ret = SW_ERR;
	alert_oerrmsg_t oerrmsg; 
	const oerrno_item_t *item = NULL;

	/* 1. 查找错误码配置信息 */
	item = alert_oerrno_search(cycle, mtext->_errno);
	if(NULL == item)
	{
		pub_log_info("[%s][%d] Didn't find the errno! errno:[%d][%s]",
				__FILE__, __LINE__, mtext->_errno, mtext->remark);
		return SW_OK;
	}

	alert_oerrno_print(item);

	/* 2. 预警级别判断 */
	if(!AlertLevelIsOk(cycle, item->level))
	{
		pub_log_info("[%s][%d] Leve is lower than cycle level! level:[%d][%d]",
				__FILE__, __LINE__, item->level, cycle->alertcfg.level);
		return SW_OK;
	}

	oerrmsg.mtext = mtext;
	oerrmsg.item = item;

	/* 3. 执行交易错误触发的行动 */
	ret = alert_oprerr_send(cycle, &oerrmsg);
	if(SW_OK != ret)
	{
		pub_log_info("[%s][%d] Send cycle message failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_oprerr_send
 **功	能: 发送交易错误信息
 **输入参数: 
 **	  cycle: 预警上下文
 **	  oerrmsg: 交易错误码预警信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 错误预警级别是否达到要求
 **	 2. 组报文
 **	 3. 发送报文
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.15 #
 ******************************************************************************/
static sw_int_t alert_oprerr_send(alert_cycle_t *cycle, alert_oerrmsg_t *oerrmsg)
{
	sw_int_t ret = SW_ERR;
	const oerrno_item_t *item = NULL;
	alert_remote_t *remote = NULL;
	char packmsg[ALERT_NBUF_MAX_LEN] = {0};

	item = oerrmsg->item;
	remote = cycle->alertcfg.remote;
	while(NULL != remote)
	{
		if((SWITCH_ON != remote->isuse)
				|| (!AlertSvrLevelIsOk(remote, item->level)))
		{
			remote = remote->next;
			continue;
		}

		if(remote->sckid < 0)
		{
			pub_log_info("[%s][%d] Connect remote server exception... ipaddr:%s[%d]",
					__FILE__, __LINE__, remote->ipaddr, remote->port);
			remote = remote->next;
			continue;
		}

		/* 1. 组报文 */
		pub_log_info("[%s][%d] Pack package...", __FILE__, __LINE__);
		ret = remote->packfunc(ALERT_TYPE_OPRERR, (const void*)oerrmsg, packmsg, sizeof(packmsg));
		if(SW_OK != ret)
		{
			pub_log_info("[%s][%d] Pack package failed!", __FILE__, __LINE__);
			remote = remote->next;
			continue;
		}

		pub_log_bin(SW_LOG_INFO, packmsg, strlen(packmsg), "[%s][%d] len=[%d]", __FILE__, __LINE__, strlen(packmsg));	

		/* 2. 发送报文 */
		ret = alert_nsend(remote, packmsg, strlen(packmsg));
		if(ret < 0)
		{
			pub_log_info("[%s][%d] Send package failed! sckid:%d ipaddr:%s[%d] errmsg:[%d][%s]",
					__FILE__, __LINE__, remote->sckid, remote->ipaddr, remote->port, errno, strerror(errno));
			remote = remote->next;
			continue;
		}
		remote->starttime = (long)time(NULL);

		pub_log_info("[%s][%d] Send package success!", __FILE__, __LINE__);
		remote = remote->next;
	}	

	return SW_OK;
}


/******************************************************************************
 **函数名称: alert_platmnt_deal
 **功	能: 平台监控信息处理
 **输入参数: 
 **	  cycle: 预警上下文
 **	  alertmsg: 预警队列信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 判断信息类型
 **	 2. 调用发送接口
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/

/* 1. 判断预警级别 */

/* 2. 调用发送接口 */



/******************************************************************************
 **函数名称: alert_platmnt_send
 **功	能: 发送平台监控信息
 **输入参数: 
 **	  cycle: 预警上下文
 **	  platmnt: 平台监控预警信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 判断信息类型
 **	 2. 调用发送接口
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/



/* 1.1 组报文 */

/* 1.2 发送报文 */



/******************************************************************************
 **函数名称: alert_sysmnt_deal
 **功	能: 发送平台监控信息至远程服务
 **输入参数: 
 **	  cycle: 预警上下文
 **	  alertmsg: 预警队列信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 判断信息类型
 **	 2. 调用发送接口
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/

/* 1. 判断预警级别 */

/* 2. 调用发送接口 */


/******************************************************************************
 **函数名称: alert_sysmnt_send
 **功	能: 发送平台监控信息至远程服务
 **输入参数: 
 **	  cycle: 预警上下文
 **	  alertmsg: 预警队列信息
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 组报文
 **	 2. 发送报文
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.17 #
 ******************************************************************************/


/* 1.1 组报文 */


/* 1.2 发送报文 */



sw_int_t alert_set_fd_noblock(sw_fd_t fd)
{
	sw_int32_t flags = 0;

	flags = fcntl(fd, F_GETFL, 0);

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int alert_nrecv(int sockid, char *msg, int len)
{
	int	n = 0;
	int	times = 0;
	int	readlen = 0;

	while (1)
	{
		n = recv(sockid, msg + readlen, len - readlen, 0);
		if (n < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			if (errno == EAGAIN)
			{
				times++;
				if (times > 3)
				{
					pub_log_info("[%s][%d] read times=[%d] readlen=[%d] length=[%d]",
							__FILE__, __LINE__, times, readlen, len);
					return readlen;
				}
				sleep(1);
				continue;
			}
			pub_log_info("[%s][%d] Recv error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			return readlen;
		}
		else if (n == 0)
		{
			pub_log_info("[%s][%d] read ack!", __FILE__, __LINE__);
			return readlen;
		}

		readlen += n;
		if (readlen >= len)
		{
			return len;
		}
	}

	return SW_ERROR;
}

/******************************************************************************
 **函数名称: alert_nsend
 **功	能: 发送信息
 **输入参数: 
 **	  sckid: 套接字描述符
 **	  msg: 发送内容
 **	  len: 内容长度
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_nsend(alert_remote_t *remote, const char *msg, int len)
{
	int left = len, n = 0;
	char	recvmsg[1024];

	remote->sckid = alert_connect(remote->ipaddr, remote->port);
	if (remote->sckid < 0)
	{
		pub_log_info("[%s][%d] 连接远端服务失败! 报文预警信息至文件!", __FILE__, __LINE__);
		alert_save_msg_to_file(msg, len);
		return SW_ERROR;
	}

	while (left > 0)
	{
		n = send(remote->sckid, msg, left, 0);
		if(n < 0)
		{
			if(EINTR == errno)
			{
				continue;
			}
			pub_log_error("[%s][%d] 发送失败,保存信息!", __FILE__, __LINE__);
			alert_save_msg_to_file(msg, len);
			close(remote->sckid);
			return SW_ERR;
		}
		left -= n;
	}

	memset(recvmsg, 0x0, sizeof(recvmsg));
	n = alert_nrecv(remote->sckid, recvmsg, sizeof(recvmsg));
	if (n < 0)
	{
		pub_log_error("[%s][%d] Recv date error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
	}
	else
	{
		pub_log_info("[%s][%d] Recv data:[%s][%d]", __FILE__, __LINE__, recvmsg, n);
	}
	close(remote->sckid);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_connect
 **功	能: 连接远程服务器
 **输入参数: 
 **	  ipaddr: ip地址
 **	  port: 端口号
 **输出参数: NONE
 **返	回: 套接字ID
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.16 #
 ******************************************************************************/
static sw_int_t alert_connect(const char *ipaddr, int port)
{
	int ret=0, sckid=0;
	struct sockaddr_in svraddr;

	memset(&svraddr, 0, sizeof(svraddr));

	/* 1. 创建套接字 */
	sckid = socket(AF_INET, SOCK_STREAM, 0);
	if(sckid < 0)
	{
		pub_log_error("[%s][%d] Create socket failed!errmsg:[%d][%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	/* 2. 连接远程服务 */
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(port);
	svraddr.sin_addr.s_addr = inet_addr(ipaddr);

	ret = connect(sckid, (struct sockaddr *)&svraddr, sizeof(svraddr));
	if(ret < 0)
	{
		close(sckid);
		pub_log_error("[%s][%d] Connect remote host failed! ipaddr:[%s][%d] errmsg:[%d][%s]",
				__FILE__, __LINE__, ipaddr, port, errno, strerror(errno));
		return SW_ERR;
	}

	/* 3. 设置为非阻塞状态 */
	ret = alert_set_fd_noblock(sckid);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] Set socket to noblock status failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return sckid;
}


/******************************************************************************
 **函数名称: alert_remote_pack
 **功	能: 组包远程服务的报文
 **输入参数: 
 **	  type: 预警信息，其取值范围在ALERT_TYPE_ENUM中定义
 **	  msg: 要被组包的预警信息
 **		   1. 为平台异常信息时，msg的类型为alert_perrmsg_t或alert_oerrmsg_t
 **	  pack: 报文缓冲区
 **	  len: 报文缓冲区最大长度
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
sw_int_t alert_remote_pack(int type, const void *msg, char *pack, int len)
{	
	switch(type)
	{
		case ALERT_TYPE_PLATERR:
			{
				return alert_remote_pack_platerr((alert_perrmsg_t *)msg, pack, len);
			}
		case ALERT_TYPE_OPRERR:
			{
				return alert_remote_pack_oprerr((alert_oerrmsg_t *)msg, pack, len);
			}
	}

	return SW_ERR;
}

/******************************************************************************
 **函数名称: alert_remote_pack_platerr
 **功	能: 组平台错误报文
 **输入参数: 
 **	  alerterr: 预警错误信息
 **	  pack: 报文缓冲区
 **	  len: 报文缓冲区最大长度
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:  
 **	 报文头格式: 长度（8个字符)
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_remote_pack_platerr(const alert_perrmsg_t *perrmsg, char *pack, int len)
{
	char *ptr = NULL;
	char bpid[64];
	char dates[32];
	char times[32];
	char tmp[128];
	char prdt[64];
	char remark[512];
	alert_error_t *mtext = NULL;
	perrno_item_t *item = NULL;
	char length[ALERT_PACK_HEAD_LEN + 1] = {0};

	mtext = (alert_error_t *)perrmsg->mtext;
	item = (perrno_item_t *)perrmsg->item;

	memset(bpid, 0x0, sizeof(bpid));
	pub_cfg_get_eswid(bpid);

	memset(dates, 0x0, sizeof(dates));
	pub_time_getdate(dates, 1);

	memset(times, 0x0, sizeof(times));
	pub_time_getdate(times, 2);

	memset(prdt, 0x00, sizeof(prdt));
	memset(remark, 0x00, sizeof(remark));
	if (mtext->remark[0] != '\0')
	{
		ptr = strstr(mtext->remark, "[PRDT:");
		if (ptr != NULL)
		{
			int	i = 0;
			memset(tmp, 0x0, sizeof(tmp));
			ptr += 6;
			while (*ptr != ']' && *ptr != '\0')
			{
				tmp[i++] = *ptr++;
			}
			if (*ptr == ']')
			{
				ptr++;
			}

			strncpy(remark, ptr, sizeof(remark) - 1);
			strncpy(prdt, tmp, strlen(tmp));
		}
		else
		{
			strncpy(remark, mtext->remark, sizeof(remark) - 1);
		}
	}

	snprintf(pack + ALERT_PACK_HEAD_LEN, len - ALERT_PACK_HEAD_LEN,
			ALERT_PACK_PLATERR_BODY_FORMAT,
			bpid, dates, times, "PLATERR", 
			item->_errno, item->level, item->desc, item->reason, 
			item->solve, remark, prdt);

	snprintf(length, sizeof(length), "%08zd", strlen(pack + ALERT_PACK_HEAD_LEN));
	memcpy(pack, length, ALERT_PACK_HEAD_LEN);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_remote_pack_oprerr
 **功	能: 组交易错误报文
 **输入参数: 
 **	  alerterr: 预警错误信息
 **	  pack: 报文缓冲区
 **	  len: 报文缓冲区最大长度
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **	 报文头格式: 长度（8个字符)
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_remote_pack_oprerr(const alert_oerrmsg_t *oerrmsg, char *pack, int len)
{
	char	*ptr = NULL;
	char	ssid[128];
	char	bpid[64];
	char	dates[32];
	char	times[32];
	char	tmp[128];
	char	prdt[64];
	char	remark[512];
	char length[ALERT_PACK_HEAD_LEN + 1] = {0};
	alert_error_t	*mtext = NULL;
	const oerrno_item_t	*item = NULL;

	item = (const oerrno_item_t *)oerrmsg->item;
	mtext = (alert_error_t *)oerrmsg->mtext;
	memset(bpid, 0x0, sizeof(bpid));
	pub_cfg_get_eswid(bpid);
	memset(dates, 0x0, sizeof(dates));
	pub_time_getdate(dates, 1);
	memset(times, 0x0, sizeof(times));
	pub_time_getdate(times, 2);

	memset(prdt, 0x00, sizeof(prdt));
	memset(remark, 0x00, sizeof(remark));
	if (mtext->remark[0] != '\0')
	{
		ptr = strstr(mtext->remark, "[MSGID:");
		if (ptr != NULL)
		{
			int	i = 0;
			memset(tmp, 0x0, sizeof(tmp));
			ptr += 7;
			while (*ptr != ']' && *ptr != '\0')
			{
				tmp[i++] = *ptr++;
			}
			if (*ptr == ']')
			{
				ptr++;
			}
			memset(ssid, 0x0, sizeof(ssid));
			memcpy(ssid, tmp, strlen(tmp));
			i = 0;
			memset(tmp, 0x0, sizeof(tmp));
			if (memcmp(ptr, "[PRDT:", 6) == 0)
			{
				ptr += 6;
			}
			while (*ptr != ']' && *ptr != '\0')
			{
				tmp[i++] = *ptr++;
			}
			if (*ptr == ']')
			{
				ptr++;
			}
			strncpy(remark, ptr, sizeof(remark) - 1);
			strncpy(prdt, tmp, strlen(tmp));
		}
		else
		{
			strncpy(remark, mtext->remark, sizeof(remark) - 1);
		}
	}
	snprintf(pack + ALERT_PACK_HEAD_LEN, len - ALERT_PACK_HEAD_LEN,
			ALERT_PACK_OPRERR_BODY_FORMAT,
			ssid, bpid, dates, times, "OPRERR", 
			item->_errno, item->level, item->desc, item->reason, 
			item->solve, remark, prdt);

	snprintf(length, sizeof(length), "%08zd", strlen(pack+ALERT_PACK_HEAD_LEN));
	memcpy(pack, length, ALERT_PACK_HEAD_LEN);

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_remote_recv
 **功	能: 接收来自远程服务的报文
 **输入参数: 
 **	  sckid: 套接字描述符
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.12 #
 ******************************************************************************/
static sw_int_t alert_remote_recv(int sckid)
{
	sw_int_t ret = SW_ERR;
	char rcvmsg[ALERT_RCVMSG_MAX_LEN] = {0};

	pub_log_info("[%s][%d] Begin to receive remote message...!", __FILE__, __LINE__);
	do
	{
		memset(rcvmsg, 0, sizeof(rcvmsg));

		ret = read(sckid, rcvmsg, sizeof(rcvmsg)-1);
		if(ret < 0)
		{
			if(EINTR == errno)
			{
				continue;
			}
			pub_log_error("[%s][%d] Receive remote message failed! errmsg:[%d]%s",
					__FILE__, __LINE__, errno, strerror(errno));
			break;
		}
		else if(ret > 0)
		{
			/* 注: 在此可进一步扩展功能... */
			pub_log_info("%s", rcvmsg);
		}
		else if(0 == ret)
		{
			pub_log_info("[%s][%d] Socket is closed...!", __FILE__, __LINE__);
		}
		break;
	}while(1);

	pub_log_info("[%s][%d] End of receiving message...!", __FILE__, __LINE__);
	return ret;
}

/******************************************************************************
 **函数名称: alert_proc_register
 **功	能: 注册预警进程
 **输入参数: 
 **	  name: 进程名
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **	 1. 确认是否已经进行注册
 **	 2. 将进程注册到共享内存
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.15 #
 ******************************************************************************/
static sw_int_t alert_proc_register(const char *name)
{
	sw_int_t ret = SW_ERR;
	sw_proc_info_t info;

	memset(&info, 0, sizeof(info));

	/* 1. 确认是否已经进行注册 */
	if(procs_is_sys_exist(name))
	{
		pub_log_info("[%s][%d] System process [%s] exist!", __FILE__, __LINE__, name);
		exit(0);
		return SW_OK;
	}

	/* 2. 将进程注册到共享内存 */
	snprintf(info.name, sizeof(info.name), "%s", name);
	info.pid = getpid();
	info.status = SW_S_START;
	info.type = ND_ALERT;
	info.restart_type = LIMITED_RESTART;

	ret = procs_sys_register(&info);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Register process failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}


/******************************************************************************
 **函数名称: alert_swcmd_recv
 **功	能: 接收平台命令
 **输入参数: 
 **	  sckid: 套接字
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Qifeng.zou # 2013.04.15 #
 ******************************************************************************/
static sw_int_t alert_swcmd_recv(int sckid)
{
	int ret=0;
	char upd_name[32] = {0};
	sw_cmd_t command;
	sw_proc_info_t info;

	memset(&info, 0, sizeof(info));
	memset(&command, 0, sizeof(command));

	ret = udp_recv(sckid, (char*)&command, sizeof(command));
	if(ret < 0)
	{
		pub_log_error("[%s][%d] Receive command failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	cmd_print(&command);

	snprintf(upd_name, sizeof(upd_name), "%s", command.udp_name);

	switch(command.type)
	{
		case SW_MSTOPSELF: /* 停止自身 */
			{
				command.type = SW_RESCMD;

				memset(command.udp_name, 0, sizeof(command.udp_name));
				snprintf(command.udp_name, sizeof(command.udp_name), "%s", PROC_NAME_ALERT);

				udp_send(sckid, (char*)&command, sizeof(sw_cmd_t), upd_name);

				ret = procs_get_sys_by_name(PROC_NAME_ALERT, &info);
				if(SW_OK != ret)
				{
					pub_log_error("[%s][%d] Get process information failed!", __FILE__, __LINE__);
					return SW_ERR;
				}

				info.status = SW_S_STOPED;

				ret = procs_sys_register(&info);
				if(SW_OK != ret)
				{
					pub_log_error("[%s][%d] Get process information failed!", __FILE__, __LINE__);
					return SW_ERR;
				}
				return ALERT_CMD_EXIT;
			}
		default:	/*收到非法命令*/
			{
				command.type = SW_ERRCMD;

				memset(command.udp_name, 0, sizeof(command.udp_name));
				memcpy(command.udp_name, "swserv", sizeof(command.udp_name)-1);

				udp_send(sckid, (char*)&command, sizeof(sw_cmd_t), upd_name);
				break;
			}
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_save_msg_to_file
 **功	能: 将预警信息保存到文件中
 **输入参数: 
 **	  msg: 预警信息
 **   len: 信息长度
 **输出参数: NONE
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Maweiwei # 2013.10.18 #
 ******************************************************************************/
static sw_int_t alert_save_msg_to_file(const char *msg, int len)
{
	int	fd = -1;
	int	ret = 0;
	int	first = 0;
	int	unsents = 0;
	char	buf[128];
	char	dir[128];
	char	filename[128];

	memset(buf, 0x0, sizeof(buf));
	memset(dir, 0x0, sizeof(dir));
	memset(filename, 0x0, sizeof(filename));
	sprintf(dir, "%s/tmp/alert", getenv("SWWORK"));

	pthread_mutex_lock(&g_alert_mutex);
	ret = aix_mkdirp(dir, 0777);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mkdir [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, dir, errno, strerror(errno));
		return SW_ERROR;
	}
	sprintf(filename, "%s/alert.txt", dir);
	first = 1;
	ret = access(filename, F_OK);
	if (ret == 0)
	{
		first = 0;
	}

	fd = open(filename, O_RDWR | O_CREAT, 0777);
	if (fd < 0)
	{
		pthread_mutex_unlock(&g_alert_mutex);
		printf("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]\n",
				__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}

	if (first == 1)
	{
		/*** 12位偏移量 + 4位已发消息数 + 4位未发消息数 + 换行符***/
		write(fd, "00000000000000000000\n", 21);
	}
	lseek(fd, 0, SEEK_END);
	write(fd, msg, len);
	write(fd, "\n", 1);
	lseek(fd, 16, SEEK_SET);
	memset(buf, 0x0, sizeof(buf));
	read(fd, buf, 4);
	unsents = atoi(buf) + 1;
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%04d", unsents);
	lseek(fd, 16, SEEK_SET);
	write(fd, buf, 4);
	close(fd);
	pthread_mutex_unlock(&g_alert_mutex);

	return 0;
}

/******************************************************************************
 **函数名称: alert_get_msg_from_file
 **功	能: 从文件中获取预警信息
 **输入参数: 
 **   len: 缓存长度
 **输出参数: 
 **	  msg: 预警信息缓存
 **返	回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作	者: # Maweiwei # 2013.10.18 #
 ******************************************************************************/
static sw_int_t alert_get_msg_from_file(char *msg, int *len)
{
	int	fd = 0;
	int	ret = 0;
	int	sents = 0;
	int	unsents = 0;
	size_t	length = 0;
	size_t	offset = 0;
	char	buf[128];
	char	head[128];
	char	filename[128];

	memset(buf, 0x0, sizeof(buf));
	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/tmp/alert/alert.txt", getenv("SWWORK"));

	pthread_mutex_lock(&g_alert_mutex);
	ret = access(filename, F_OK);
	if (ret < 0)
	{
		pthread_mutex_unlock(&g_alert_mutex);
		return 0;
	}

	fd = open(filename, O_RDWR | O_CREAT, 0777);
	if (fd < 0)
	{
		pthread_mutex_unlock(&g_alert_mutex);
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]\n",
				__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}

	/*** 12位偏移量 + 4位已发消息数 + 4位未发消息数 ***/
	memset(head, 0x0, sizeof(head));
	lseek(fd, 0, SEEK_SET);
	read(fd, head, 20);
	/*** 4 位未发送消息数 ***/
	memset(buf, 0x0, sizeof(buf));
	memcpy(buf, head + 16, 4);
	unsents = atoi(buf);
	pub_log_debug("[%s][%d] unsents=[%d][%s]", __FILE__, __LINE__, unsents, buf);
	if (unsents == 0)
	{
		lseek(fd, 0, SEEK_SET);
		remove(filename);
		close(fd);
		pthread_mutex_unlock(&g_alert_mutex);
		pub_log_info("[%s][%d] 没有需要发送的消息!", __FILE__, __LINE__);
		return 0;
	}

	/*** 4 位已发送消息数 ***/
	memset(buf, 0x0, sizeof(buf));
	memcpy(buf, head + 12, 4);
	sents = atoi(buf);
	/*** 12 位偏移量 ***/
	memset(buf, 0x0, sizeof(buf));
	memcpy(buf, head, 12);
	offset = atol(buf);
	pub_log_debug("head=[%s] offset=[%d] sents=[%d] unsents=[%d]\n", head, offset, sents, unsents);

	/*** 8位消息长度 ***/
	lseek(fd, offset + 20 + 1, SEEK_SET);
	memset(buf, 0x0, sizeof(buf));
	read(fd, buf, 8);
	length = atol(buf);
	memcpy(msg, buf, 8);
	memset(buf, 0x0, sizeof(buf));
	read(fd, msg + 8, length);
	offset += 8 + 1 + length;
	lseek(fd, 0, SEEK_SET);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%012zd%04d%04d", offset, sents + 1, unsents - 1);
	write(fd, buf, 20);
	pub_log_info("[%s][%d] Head:[%s]", __FILE__, __LINE__, buf);
	close(fd);
	*len = length + 8;
	pthread_mutex_unlock(&g_alert_mutex);

	return 1;
}

sw_int_t alert_check_msg_time(char *msg, int len)
{
	int	flag = 0;
	int	space = 0;
	char	date[32];
	char	time[32];
	char	time1[32];
	char	time2[32];
	char	ori_date[32];
	char	ori_time[32];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(date, 0x0, sizeof(date));
	memset(time, 0x0, sizeof(time));
	memset(time1, 0x0, sizeof(time1));
	memset(time2, 0x0, sizeof(time2));
	memset(ori_date, 0x0, sizeof(ori_date));
	memset(ori_time, 0x0, sizeof(ori_time));

	pub_time_getdate(date, 1);
	pub_time_getdate(time, 2);
	sprintf(time1, "%s%s", date + 2, time);

	xml = pub_xml_crtree_ext(msg, len);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! msg=[%s]", __FILE__, __LINE__, msg);
		return -1;
	}

	node = pub_xml_locnode(xml, ".ALERT.DATE");
	if (node != NULL && node->value != NULL)
	{
		if (strcmp(date, node->value) != 0)
		{
			pub_log_info("[%s][%d] 原预警日期:[%s] 当前日期:[%s]",
					__FILE__, __LINE__, node->value, date);
			flag = 1;
		}
		strcpy(time2, node->value + 2);
	}

	node = pub_xml_locnode(xml, ".ALERT.TIME");
	if (node != NULL && node->value != NULL)
	{
		strcat(time2, node->value);
		space = pub_time_minus(time2, time1);
		pub_log_info("[%s][%d] time1=[%s] time2=[%s] space=[%d]",
				__FILE__, __LINE__, time1, time2, space);
		if (space > 60 * 30) 
		{
			pub_log_info("[%s][%d] 原预警时间:[%s] 当前时间:[%s]",
					__FILE__, __LINE__, node->value, time);
			flag = 1;
		}
	}
	pub_xml_deltree(xml);

	if (flag == 1)
	{
		return 2;
	}

	return 0;
}

#else /*__ALERT_SUPPORT__*/
int main(void)
{
	return 0;
}
#endif /*__ALERT_SUPPORT__*/
