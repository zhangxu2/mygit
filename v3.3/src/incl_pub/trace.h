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

#define MAXROUTE		30		/**��ͨ���񾭹��ڵ���**/
/*0Ϊ����ģ��Ǽǣ�1Ϊ��ģ��Ǽ� 2Ϊ���̽��� 3 ��������ˮ*/
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
	sw_int32_t	module_type;	        /*��ǰ��������ģ������ ND_LSN~ND_...*/
	sw_int32_t	module_num;	        /*��ǰ��������ģ����*/
	sw_int32_t	status;		/*��ǰ����״̬*/
	sw_int32_t	error_code;		/**������**/
	sw_int64_t	start_time;		/**ģ��ӵ�����ʱ��ms**/
	sw_int64_t	end_time;		/**ģ�鴦�����ʱ��ms**/
	sw_int32_t	next;			/**��һ��ģ�����*/
	sw_int32_t	tx_type;		/*��������*/
	sw_int32_t	prior;			/**��һ���ڵ����*/
	sw_int32_t	use;			/*�ýڵ��Ƿ�ռ��*/
	sw_char_t	node[SW_NAME_LEN];
};

typedef struct sw_trace_s sw_trace_t;

struct sw_ejf_info_s
{
	sw_int_t	trace_no;		/**ƽ̨��ˮ**/
	sw_int_t	busi_no;		/**ҵ����ˮ**/
	sw_int_t	next_trno;		/**�ɴ˲�������ˮ**/
	sw_int_t	seqs;			/**ҵ�����**/
	sw_char_t	tx_code[12];		/**������**/
	sw_char_t	tx_respcd[12];		/*ҵ��Ӧ���� $pkgrespcd*/
	sw_char_t	start_date[16];		/*��������*/
	sw_trace_t	route;
};

typedef	struct sw_ejf_info_s sw_ejf_info_t;

struct sw_trace_item_s
{
	sw_int32_t	flag;			/**��¼�����Ƿ����**/
	sw_int64_t	trace_no;		/**ƽ̨��ˮ**/
	sw_int64_t	next_trno;		/**�ɴ˲�������ˮ**/
	sw_int64_t	busi_no;		/**ҵ����ˮ**/
	sw_int64_t	sys_no;		        /**ƽ̨��ˮ**/
	sw_int32_t	seq;			/**ҵ�����**/
	sw_int32_t	head;
	sw_int32_t	last;			/*ָ��ڵ㻺�����и���ˮ�����һ���ڵ����ţ�С�����ʾδռ�ýڵ㻺�����еĽڵ�*/
	sw_int32_t	current;
	sw_int32_t	change_index;		/*���Ϊ1����ʾ�ýڵ��ڳ�ģ���ʱ��δ�Ǽǣ�����һ�ڵ����ģ���Ѿ��Ǽ��ˣ���Ҫ�����⴦��*/
	sw_int64_t	start_time;		/*���׿�ʼʱ��*/
	sw_int64_t	end_time;		/*���׽���ʱ��*/	
	sw_char_t	prdt_name[SW_NAME_LEN]; /*product*/
	sw_char_t	server[SW_NAME_LEN];	/*server*/
	sw_char_t	service[SW_NAME_LEN];	/*��ǰ service ��*/
	sw_char_t	tx_code[12];		/**������**/
	sw_char_t	sys_errcode[8];         /*ƽ̨������ $errcode */
	sw_char_t	tx_respcd[12];		/*ҵ��Ӧ���� #errcode*/
	sw_char_t	tx_errmsg[128];          /*ҵ������Ϣ #errmsg */
	sw_char_t	start_date[16];		/*�������� YYYYMMD 8λ*/
	sw_char_t	tx_amt[32];             /*���׽��*/
	sw_char_t	dr_ac_no[32];           /*�跽�˺�*/
	sw_char_t	cr_ac_no[32];           /*�����˺�*/
	sw_char_t	dr_ac_name[60];         /*�跽����*/
	sw_char_t	cr_ac_name[60];         /*��������*/
	sw_char_t	fee_amt[32];		/*������*/
	sw_char_t	fee_ac_no[32];		/*�������˺�*/
	sw_char_t	ct_ind[8];		/*��ת��־*/
	sw_char_t	dc_ind[8];		/*�����־*/
	sw_char_t	chnl[32];		/*��������*/
	sw_char_t	time[32];		/*����ʱ��*/
	sw_char_t	resflag[8];		/*��Ӧ��ʶ 0:δ��Ӧ 1:��Ӧ*/
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
	sw_int32_t	use;			/*�ڵ㻺�������ýڵ���*/
	sw_int32_t	free;			/*�ڵ㻺�����п���ڵ�ĸ���*/
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
