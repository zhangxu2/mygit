/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-06
 *** module  : Interface for BP change date, system trace no
 		, multi business trace no. 
 *** name    : seqs.h 
 *** function: trace_no sub module in BP run-time shm.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#ifndef __SEQS_H__
#define __SEQS_H__
#include "pub_type.h"
#include "pub_cfg.h"
#include "pub_time.h"

#define MAX_SEQS_CNT	20
#define TRACE_INC	1000
#define SW_MAX_DEFAULT_TRACE	99999999L

#define SEQS_DATE_UNLOCK    (0)
#define SEQS_DATE_LOCK      (1) 

#define SEQ_CYCLE_DISABLE        0
#define SEQ_CYCLE_ENABLE         1
#define SEQ_CYCLE_DB_GET         2

typedef enum
{
	SEQS_SYS_STAT_RUN = 1,
	SEQS_SYS_STAT_STOP
}SEQS_SYS_STATUS_ENUM;

#define seqs_sys_is_run(status) (status == SEQS_SYS_STAT_RUN)
#define seqs_sys_is_stop(status) (status == SEQS_SYS_STAT_STOP)

/* 流水保存方式 */
typedef enum
{
	SEQS_SAVE_STAT_FIXED = 1,
	SEQS_SAVE_STAT_CLEAR,	
	SEQS_SAVE_STAT_TOTAL		
}SEQS_SAVE_STATUS_ENUM;

typedef struct sw_seqs_unit_s
{
	sw_char_t	name[SW_NAME_LEN];
	sw_int_t        lock_id;
	sw_char_t       prefix[SQN_LEN];     /* for printf easy, the real prefix*/
	sw_char_t       trn_prefix[SQN_LEN]; /* for count easy, pow()-ed */
	sw_int64_t      base;       /* init base value */   
	sw_int64_t      trn_base;   /* ejfno = trn_prefix + trn_base */
	sw_int64_t      ejfno_max; 
	sw_int_t        base_increment;
	sw_uchar_t      cycle;
} sw_seqs_unit_t;

typedef struct sw_seqs_head_s
{
	sw_int32_t	date_lock_flag;	/*date change lock flag. Range: SEQS_DATE_LOCK or SEQS_DATE_UNLOCK*/
	sw_int32_t	lock_id;	/*head access lock id*/
	sw_int32_t	sys_status;	/*Range in SEQS_SYS_STATUS_ENUM */
	sw_int32_t	status;		/*Range in SEQS_SAVE_STATUS_ENUM */
        sw_int32_t	busi_trc_cnt;	/*business trace num*/
	sw_int32_t      bp_trc_cnt;      /* used  bp_trc_array */
        sw_int32_t	max_seq;	/*max number of business seqs*/
	sw_char_t	sys_date[DATE_MAX_LEN];	/*system date*/
	sw_char_t	vfs_date[DATE_MAX_LEN];	/*virtual system date*/
} sw_seqs_head_t;

typedef struct sw_seqs_s
{
	sw_seqs_head_t	head;
	sw_seqs_unit_t  bp_trc_array[MAX_SEQS_CNT];
	sw_seqs_unit_t  busi_trc_array[1];
} sw_seqs_t;


sw_int32_t seqs_get_size(sw_syscfg_t *syscfg);

sw_int_t   seqs_set_addr(const sw_char_t* addr);
sw_int_t   seqs_init(sw_seqs_t *shm_seqs, const sw_syscfg_t* syscfg);

/* business api */
sw_int_t   seqs_add_bsn_trace(const sw_seqcfg_t *seqcfg); 
sw_int64_t seqs_new_bsn_no(const char *name, sw_char_t *trace_no); 

sw_int_t   seqs_set_bsn_flow(const char *name, sw_int64_t flwno);
sw_int_t   seqs_reset_bsn_trace(const char *name, const sw_seqcfg_t *seqcfg);

sw_int_t   seqs_print_bsn_flow(const char *bsn_name);
sw_int_t   seqs_print_bsn_flow_all(void);

sw_int_t   seqs_add_business_trace(const char *name, sw_int64_t prefix); /* deprecated */
sw_int64_t seqs_new_business_no(const char *name, int step, sw_char_t *trace_no); /* deprecated */

sw_int64_t seqs_get_bsn_no(const char *name, sw_char_t *trace_no);

/* bp api */
sw_int_t   seqs_add_bp_trace(const sw_seqcfg_t *seqcfg);
sw_int64_t seqs_new_bp_no(const char *name); 

sw_int_t   seqs_set_bp_flow(const char *name, sw_int64_t flwno); 
sw_int_t   seqs_reset_bp_trace(const char *name, const sw_seqcfg_t *seqcfg);

sw_int_t   seqs_print_plat_flow(void);

sw_int64_t seqs_new_trace_no();        /* use the 1st bp seq unit, not recommended */ 
sw_int64_t seqs_get_trace_no();       /* do NOT use if u no what u r doing */
sw_int_t   seqs_set_plat_flow(sw_int64_t flwno);   /* deprecated */

/* date api */
/* the following 2 funcionts are same */
sw_int_t   seqs_get_sysdate(char *sys_date);
sw_int_t   seqs_get_curdate(char *sys_date);
sw_int_t   seqs_get_date(char *date, int size); 
sw_int_t   seqs_is_date_locked(void);

sw_int_t   seqs_change_date(const char *new_sysdate); /* unlock version of seqs_set_date_lock */
sw_int_t   seqs_set_date_lock(const char *new_date);
sw_int_t   seqs_set_date_unlock(void);

/* misc api */
sw_int_t   seqs_save(int flag);
sw_int_t   seqs_recover();

sw_int_t seqs_get(sw_seqs_head_t* seqhead);
sw_int64_t seqs_get_bsn_trace(sw_char_t *bsn_name, sw_char_t *trace_no);
const char *seqs_get_syssts_desc();
int seqs_get_traceno_and_sysdate(sw_int64_t *trace_no, sw_char_t *sysdate);
sw_int32_t seqs_sys_start();
sw_int32_t seqs_sys_stop();
sw_int32_t seqs_get_syssts();

#endif /* __SEQS_H__ */
