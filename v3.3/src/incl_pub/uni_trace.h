#ifndef _UNI_TRACE_H
#define _UNI_TRACE_H

#include "pub_type.h"
#include "trace.h"
#include "pub_cfg.h"
#include "pub_pool.h"

/*Eswitch trace process context.*/
typedef struct
{
    sw_syscfg_t syscfg;
    sw_trace_info_t *info;
}uni_trace_t;

/*Trace info.*/
typedef struct
{
    char tx_code[8];            /* ������ */
    char trace_no[16];          /* ƽ̨��ˮ�� */
    char busi_trace_no[16];     /* ҵ����ˮ�� */
    char tx_date[16];           /* �������� */
    char begin_time[32];        /* ��ʼʱ�� */
    char end_time[32];          /* ����ʱ�� */
    char cost_time[32];         /* ��ʱ */
    char tx_status[64];         /* ����״̬ */
}uni_trace_info_t;

#define SW_MAX_STEPS  64

/*Trace step info.*/
typedef struct
{
    sw_char_t step[8];
    sw_char_t msg_type[40];
    sw_char_t module[64];
    sw_char_t begin[16];
    sw_char_t end[16];
    sw_char_t deal_time[32];
    sw_char_t deal_sts[16];
}sw_trace_step_t;

SW_PUBLIC sw_int_t uni_trace_init(uni_trace_t *trace);
SW_PUBLIC sw_int_t uni_trace_destory(uni_trace_t *trace);
SW_PUBLIC sw_int_t uni_trace_get_top(uni_trace_t *trace, const sw_trace_item_t **list, sw_int32_t num);
SW_PUBLIC const sw_trace_item_t *uni_trace_item_search(uni_trace_t *trace, sw_int64_t trace_no);

#endif /* _UNI_TRACE_H */
