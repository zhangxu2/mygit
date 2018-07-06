/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : BP run-time shm. 
 *** name    : mtype.h
 *** function: mtype sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __MTYPE_H__
#define __MTYPE_H__
#include "pub_type.h"
#include "pub_cfg.h"

#define TASK_TIMEOUT 	(4)
#define MAX_MTYPE_CNT	(1000)

#define MTYPE_IDLE	(1)
#define MTYPE_TAIL	(-1)
#define MTYPE_USED	(-2)
#define MTYPE_ALL   (-3)

/*struct sw_mtype_cfg_s
{
	sw_int32_t	session_max;
};

typedef struct sw_mtype_cfg_s sw_mtype_cfg_t;*/

/*shm mtype head info*/
struct sw_mtype_head_s
{
	sw_int32_t	lock_id;
	sw_int32_t	size;		/*shm mtype size*/
	sw_int16_t	mtype_max;		/*max mtype number*/
	sw_int16_t	msg_cnt;		/*total record count*/
	sw_int16_t	cur_cnt;		/*current record count*/
	sw_int16_t	cur_use;		/*the number of used mtype*/
	sw_int32_t	succ_cnt;		/*success count*/
	sw_int32_t	fail_cnt;		/*fail count*/
	sw_int32_t	trc_cnt;		/*trace count*/
	sw_int32_t	trc_fail_cnt;	/*fail trace count*/
	sw_int64_t	start_time;
};

typedef struct sw_mtype_head_s sw_mtype_head_t;

/*mtype error info*/
struct sw_mtype_err_s
{
	sw_int32_t	err_code1:8;	        /*error code 1*/
	sw_int32_t	err_code2:8;	        /*error code 2*/
	sw_int16_t	mode_type;	        /*module type*/
	sw_int32_t	mode_num:16;	        /*module id*/
	sw_char_t	err_msg[64];            /*error description*/
};

typedef struct sw_mtype_err_s sw_mtype_err_t;

/*task info*/
struct sw_task_info_s
{		
	sw_int64_t	trace_no;		/*trace no*/
	sw_int32_t	status;		/*current task status*/
	sw_mtype_err_t	err_info;               /*mtype error info*/
};

typedef struct sw_task_info_s sw_task_info_t;

/*mtype node*/
struct sw_mtype_node_s
{
	sw_int32_t	flag;			/* range: MTYPE_IDLE, MTYPE_TAIL, MTYPE_USED*/
	sw_int_t	mtype;			/*mtype*/
	sw_int32_t	task_status;		/*lsn use this field as timeout*/
	sw_int32_t	timeout;		/*timeout*/
	sw_int32_t	calllsn_timeout;	/**calllsn timeout*/
	sw_int64_t	start_time;		/*outside request timestamp, for lsn*/
	sw_int_t	link_time;		/*inside request timestamp, for lsn*/	
	sw_int64_t	trace_no;		/*trace no*/
	sw_int64_t	bi_no;			/*business no*/	
	sw_int64_t	msg_id;			/*message id*/
	sw_int64_t	mq_time;		/*get in message queue timestamp*/
	sw_char_t	sys_date[32];		/*system date*/
	sw_char_t	key[128];		/*business key, for lsn*/
	sw_char_t	prdt_name[SW_NAME_LEN];	/*product name*/
	sw_char_t	server[SW_NAME_LEN];	/*origin server name*/
	sw_char_t	service[SW_NAME_LEN];	/*origin service name*/	
	sw_char_t	lsn_name[SW_NAME_LEN];	/*origin lsn name*/
	sw_char_t	saf_flow[SW_NAME_LEN];	/*saf flow file*/
	sw_char_t	pkg_response[8];	/*business response*/
	sw_char_t	sys_response[8];	/*BP response*/
	sw_int32_t	task_type;		/*task type*/
	sw_int32_t	log_flag;		/*log type, 1:shm, 2:file, 3:ignore this flag*/
	sw_task_info_t	task_info;       	/*task tmp info*/
};

typedef struct sw_mtype_node_s sw_mtype_node_t;

/*mtype shm layout*/
struct sw_mtype_s
{
	sw_mtype_head_t head;
	sw_mtype_node_t first;
};

typedef struct sw_mtype_s sw_mtype_t;

struct sw_trace_list_s
{
	sw_int64_t	trace_no;
	sw_char_t	start_time[16];
	sw_char_t	server[SW_NAME_LEN];
	sw_char_t	service[SW_NAME_LEN];
	sw_char_t	desc[32];
	sw_char_t	d_time[8];
	sw_char_t	resp[8];
	sw_char_t	resp_desc[32];
	sw_char_t	pkg_resp[8];
};

typedef struct sw_trace_list_s	sw_trace_list_t;

#define MTYPE_FLAG_DESC_MAX_LEN (32)
typedef struct
{
    int flag;
    char dest[MTYPE_FLAG_DESC_MAX_LEN];
}mtype_flag_desc_t;

SW_PUBLIC sw_int_t mtype_init(sw_mtype_t* shm_mtype, sw_syscfg_t* sys_cfg);

SW_PUBLIC sw_int32_t mtype_get_size(sw_syscfg_t* sys_cfg);

SW_PUBLIC sw_int_t mtype_set_addr(sw_char_t* addr);
SW_PUBLIC const sw_mtype_t *mtype_get_addr(void);

SW_PUBLIC sw_int32_t mtype_new();

SW_PUBLIC sw_int32_t mtype_cur_cnt();

SW_PUBLIC sw_int32_t mtype_get_max();

SW_PUBLIC sw_int_t mtype_delete(sw_int_t mtype, sw_int32_t flag);

SW_PUBLIC sw_int_t mtype_delete_info(sw_int_t mtype, sw_int32_t flag
		, sw_char_t *server, sw_char_t *service);

SW_PUBLIC sw_int_t  mtype_save_link_info(sw_int_t mtype, sw_int64_t bi_no, sw_int64_t trace_no
		, char *key, char *sys_date, char *lsn_name, sw_int32_t timeout);

SW_PUBLIC sw_int_t mtype_load_by_key(char *key, sw_int64_t *bi_no, sw_int64_t *trace_no
		, sw_int_t *mtype, char *sys_date);

SW_PUBLIC sw_int_t mtype_set_task_info(sw_int_t mtype, sw_task_info_t *task_info);

SW_PUBLIC sw_int_t mtype_get_tx_info(sw_int_t mtype,sw_trace_list_t *tx_list);

SW_PUBLIC sw_int_t mtype_get_task_info(sw_int_t mtype, sw_task_info_t *task_info);

SW_PUBLIC sw_int_t mtype_set_prdt(sw_int_t mtype, sw_char_t* prdt);

SW_PUBLIC sw_int_t mtype_get_prdt(sw_int_t mtype, sw_char_t* prdt);

SW_PUBLIC sw_int_t mtype_get_svr_svc(sw_int_t mtype, char *server, char *service);

SW_PUBLIC sw_int_t mtype_set_svr_svc(sw_int_t mtype, sw_int32_t type, char *server, char *service);

SW_PUBLIC sw_task_info_t * mtype_get_task_addr(sw_int_t mtype);

SW_PUBLIC sw_int_t mtype_get_status(sw_int_t mtype, sw_int32_t *status
		, sw_int32_t *task_status);

SW_PUBLIC sw_int_t mtype_set_status(sw_int_t mtype, sw_int32_t status
		, sw_int32_t task_status);

SW_PUBLIC sw_int32_t mtype_check_timeout(sw_int_t mtype, sw_int32_t timeout);

SW_PUBLIC sw_int32_t mtype_get_timeout(sw_int_t mtype);

SW_PUBLIC sw_int_t mtype_set_err(sw_int32_t force_flag, sw_int_t mtype
	, sw_int32_t mode_type, sw_int32_t mode_num, sw_int32_t err_code1
	, sw_int32_t err_code2, char *err_msg);

SW_PUBLIC sw_int_t mtype_set_saf_flow(sw_int_t mtype, char *saf_flow, sw_int32_t timeout);

SW_PUBLIC sw_int_t mtype_get_saf_flow(sw_int_t mtype, char *saf_flow, sw_int32_t *time_value);

SW_PUBLIC sw_int_t mtype_set_resp(int mtype, char *resp, char *sys_resp);

SW_PUBLIC sw_int_t mtype_get_resp(int mtype, char *resp, char *sys_resp);

SW_PUBLIC sw_int_t mtype_get_lsn(int mtype, char *lsn_name);

SW_PUBLIC sw_int_t mtype_get_head(sw_mtype_head_t *head);

SW_PUBLIC sw_int_t mtype_set_date(sw_int_t mtype, sw_int64_t trace_no, char *sys_date);

SW_PUBLIC sw_int_t mtype_get_date(int mtype, sw_int64_t *trace_no, char *sys_date);

SW_PUBLIC const char *mtype_get_flag_desc(int flag);
SW_PUBLIC sw_int_t mtype_loc_set_time(sw_mtype_t *shm_mtype);
SW_PUBLIC sw_int_t mtype_set_time();
SW_PUBLIC sw_int64_t mtype_loc_get_time(sw_mtype_t *shm_mtype);
SW_PUBLIC sw_int64_t mtype_get_time();
SW_PUBLIC sw_int_t pub_vars_destroy_by_mtype(int vid);

#endif /* __MTYPE_H__ */
