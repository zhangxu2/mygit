/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-06
 *** module  : Interface for route path in BP. 
 *** name    : trace.h
 *** function: trace sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __PUB_TRACE_H__
#define __PUB_TRACE_H__
#include "pub_type.h"
#include "pub_cfg.h"
#include "pub_vars.h"
#include "msg_trans.h"
#include "pub_route.h"

#define MAXROUTE		30		/**普通任务经过节点数**/
/*0为进入模块登记，1为出模块登记 2为流程结束 3 引发新流水*/
#define TRACE_IN 0
#define TRACE_OUT 1
#define TRACE_OVER 2
#define TRACE_NEW 3
#define TRACE_TIMEOUT 4

/*struct sw_trace_cfg_s
{
	sw_int32_t	max_trace;
	sw_int32_t	max_cache;
};

typedef struct sw_trace_cfg_s sw_trace_cfg_t;*/

struct sw_trace_s
{
	sw_int32_t	module_type;	        /*当前任务所在模块类型 ND_LSN~ND_...*/
	sw_int32_t	module_num;	        /*当前任务所在模块编号*/
	sw_int32_t	status;		/*当前任务状态*/
	sw_int32_t	error_code;		/**错误码**/
	sw_int64_t	start_time;		/**模块接到任务时间ms**/
	sw_int64_t	end_time;		/**模块处理完成时间ms**/
	sw_int32_t	next;			/**下一个模块序号*/
	sw_int32_t	tx_type;		/*交易类型*/
	sw_int32_t	prior;			/**上一个节点序号*/
	sw_int32_t	use;			/*该节点是否占用*/
	sw_char_t	node[SW_NAME_LEN];
};

typedef struct sw_trace_s sw_trace_t;

struct sw_ejf_info_s
{
	sw_int_t	trace_no;		/**平台流水**/
	sw_int_t	busi_no;		/**业务流水**/
	sw_int_t	next_trno;		/**由此产生的流水**/
	sw_int_t	seqs;			/**业务序号**/
	sw_char_t	tx_code[12];		/**交易码**/
	sw_char_t	tx_respcd[12];		/*业务应答码 $pkgrespcd*/
	sw_char_t	start_date[16];		/*交易日期*/
	sw_trace_t	route;
};

typedef	struct sw_ejf_info_s sw_ejf_info_t;

struct sw_trace_item_s
{
	sw_int32_t	flag;			/**记录交易是否完成**/
	sw_int64_t	trace_no;		/**平台流水**/
	sw_int64_t	next_trno;		/**由此产生的流水**/
	sw_int64_t	busi_no;		/**业务流水**/
	sw_int64_t	sys_no;		        /**平台流水**/
	sw_int32_t	seq;			/**业务序号**/
	sw_int32_t	head;
	sw_int32_t	last;			/*指向节点缓冲区中该流水的最后一个节点的序号，小于零表示未占用节点缓冲区中的节点*/
	sw_int32_t	current;
	sw_int32_t	change_index;		/*如果为1，表示该节点在出模块的时候还未登记，但下一节点的入模块已经登记了，需要做特殊处理*/
	sw_int64_t	start_time;		/*交易开始时间*/
	sw_int64_t	end_time;		/*交易结束时间*/	
	sw_char_t	prdt_name[SW_NAME_LEN]; /*product*/
	sw_char_t	server[SW_NAME_LEN];	/*server*/
	sw_char_t	service[SW_NAME_LEN];	/*当前 service 名*/
	sw_char_t	tx_code[12];		/**交易码**/
	sw_char_t	sys_errcode[8];         /*平台处理码 $errcode */
	sw_char_t	tx_respcd[12];		/*业务应答码 #errcode*/
	sw_char_t	tx_errmsg[128];          /*业务处理信息 #errmsg */
	sw_char_t	start_date[16];		/*交易日期 YYYYMMD 8位*/
	sw_char_t	tx_amt[32];             /*交易金额*/
	sw_char_t	dr_ac_no[32];           /*借方账号*/
	sw_char_t	cr_ac_no[32];           /*贷方账号*/
	sw_char_t	dr_ac_name[60];         /*借方户名*/
	sw_char_t	cr_ac_name[60];         /*贷方户名*/
	sw_char_t	fee_amt[32];		/*手续费*/
	sw_char_t	fee_ac_no[32];		/*手续费账号*/
	sw_char_t	ct_ind[8];		/*现转标志*/
	sw_char_t	dc_ind[8];		/*借贷标志*/
	sw_char_t	chnl[32];		/*发起渠道*/
	sw_char_t	time[32];		/*交易时间*/
	sw_char_t	resflag[8];		/*响应标识 0:未响应 1:响应*/
	sw_trace_t	route[MAXROUTE];
};

typedef	struct sw_trace_item_s sw_trace_item_t;

struct sw_trace_info_head_s
{
	sw_int32_t	cur_use;
	sw_int32_t	count;
};

typedef struct sw_trace_info_head_s sw_trace_info_head_t;

struct sw_trace_info_s
{
	sw_trace_info_head_t	head;
	sw_trace_item_t	trace_item[1];
};

typedef struct sw_trace_info_s sw_trace_info_t;

struct sw_tcache_head_s
{
	sw_int32_t	count;
	sw_int32_t	lock_id;
	sw_int32_t	use;			/*节点缓冲区已用节点数*/
	sw_int32_t	free;			/*节点缓冲区中空余节点的个数*/
	sw_int32_t	last;
};

typedef struct sw_tcache_head_s sw_tcache_head_t;

struct sw_trace_cache_s
{
	sw_tcache_head_t	head;
	sw_trace_t	route[1];
};

typedef struct sw_trace_cache_s sw_trace_cache_t;

SW_PUBLIC sw_int_t trace_init(sw_char_t* addr, sw_syscfg_t* syscfg);

SW_PUBLIC sw_int_t trace_set_addr(sw_char_t *addr);

SW_PUBLIC sw_int32_t trace_get_size(sw_syscfg_t* syscfg);

SW_PUBLIC sw_trace_info_t* trace_get_addr();

SW_PUBLIC sw_trace_cache_t* trace_get_cache_addr();

SW_PUBLIC sw_int_t trace_create_info(sw_cmd_t *cmd);

SW_PUBLIC sw_int_t trace_insert_prdt(sw_int_t mtype, sw_char_t *prdt, sw_int64_t trace_no);

SW_PUBLIC sw_int_t trace_insert_svr(sw_int_t mtype
		, sw_char_t *server, sw_char_t *service, sw_int64_t trace_no);

SW_PUBLIC sw_int_t trace_insert_ejf(sw_int_t mtype, sw_ejf_info_t* ejf, int flag);

SW_PUBLIC sw_int_t trace_get_error(sw_char_t *tmp, sw_int32_t error_code);

SW_PUBLIC sw_int_t trace_get_msg_type(sw_char_t *tmp, sw_int32_t type);

SW_PUBLIC sw_int_t trace_show_info(sw_int64_t trace_no, sw_char_t *trace_buf);
const sw_trace_t *trace_get_exp_route(int idx);
SW_PUBLIC sw_int_t trace_insert(sw_loc_vars_t *vars, sw_cmd_t *cmd, int flag);

#endif /* __PUB_TRACE_H__ */
