#include "uni_trace.h"
#include "pub_type.h"
#include "run.h"

static sw_int_t uni_trace_top_sort(const sw_trace_item_t **list, sw_int32_t count);

/******************************************************************************
 ** Name : uni_trace_init
 ** Desc : Initialize trace uniform interface
 ** Input: 
 **        trace: Uniform trace
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
sw_int_t uni_trace_init(uni_trace_t* trace)
{
    sw_int_t ret = SW_ERR;
    const sw_cfgshm_hder_t *shmcfg = NULL;
    
    ret = run_link();
    if(SHM_MISS == ret)
    {
        ret = run_init();
        if(SW_OK != ret)
        {
            pub_log_error("[%s][%d] Init run-time failed!", __FILE__, __LINE__);
            return SW_ERR;
        }
    }

    trace->info = (sw_trace_info_t *)run_get_route();
    shmcfg = (const sw_cfgshm_hder_t *)run_get_syscfg();

    cfg_get_sys(shmcfg, &trace->syscfg);

    return SW_OK;
}

/******************************************************************************
 ** Name : uni_trace_init
 ** Desc : Initialize trace uniform interface
 ** Input: 
 **        svr: Uniform server object
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Wangkun # 2013.04.03 #
 ******************************************************************************/
sw_int_t uni_trace_destory(uni_trace_t *trace)
{
    return SW_OK;
}

/******************************************************************************
 ** Name : uni_trace_get_top
 ** Desc : Get top trace information
 ** Input: 
 **     trace: Trace object
 **     list: Store result
 **     num: The num of list member
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
sw_int_t uni_trace_get_top(uni_trace_t *trace, const sw_trace_item_t **list, sw_int32_t num)
{
    char	tmp1[128];
    char	tmp2[128];
    sw_int32_t idx=0, count=0;
    sw_trace_item_t *item = NULL;
    const sw_syscfg_t *syscfg = &trace->syscfg;


    item = trace->info->trace_item;

    list[0] = item;
    item++;
    count++;
    
    for(idx=1; idx<syscfg->session_max; idx++)
    {
        if(item->trace_no <= 0)
        {
            item++;
            continue;
        }
        
        if(count < num)
        {
            list[count] = item;
            count++;
        }
        else
        {
            memset(tmp1, 0x0, sizeof(tmp1));
            memset(tmp2, 0x0, sizeof(tmp2));
            sprintf(tmp1, "%s%012lld", list[count-1]->start_date, list[count-1]->trace_no);
            sprintf(tmp2, "%s%012lld", item->start_date, item->trace_no);
            if (strcmp(tmp1, tmp2) >= 0)
            {
                item++;
                continue;
            }

            list[count-1] = item;
        }

        uni_trace_top_sort(list, count);
        item++;
    }

    return SW_OK;
}

/******************************************************************************
 ** Name : uni_trace_top_sort
 ** Desc : Insert sort
 ** Input: 
 **     list: Result list
 **     count: Num of list
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
static sw_int_t uni_trace_top_sort(const sw_trace_item_t **list, sw_int32_t count)
{
    sw_int_t idx = 0;
    char	tmp1[128];
    char	tmp2[128];
    const sw_trace_item_t *swap = NULL;
    
    for(idx=count-1; idx>0; idx--)
    {
        memset(tmp1, 0x0, sizeof(tmp1));
        memset(tmp2, 0x0, sizeof(tmp2));
        sprintf(tmp1, "%s%012lld", list[idx]->start_date, list[idx]->trace_no);
        sprintf(tmp2, "%s%012lld", list[idx-1]->start_date, list[idx-1]->trace_no);
        if (strcmp(tmp1, tmp2) <= 0)
        {
            break;
        }

        swap = list[idx];
        list[idx] = list[idx-1];
        list[idx-1] = swap;
    }

    return SW_OK;
}

/******************************************************************************
 ** Name : uni_trace_item_search
 ** Desc : Search special item by trace no
 ** Input: 
 **     list: Result list
 **     trace_no: Trace no which is should be searched.
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.28 #
 ******************************************************************************/
const sw_trace_item_t *uni_trace_item_search(uni_trace_t *trace, sw_int64_t trace_no)
{
    sw_int_t idx = 0;
    const sw_trace_item_t *item = NULL;
    const sw_syscfg_t *syscfg = &trace->syscfg;

    item = trace->info->trace_item;
    for(idx=0; idx<syscfg->session_max; idx++)
    {
        if(item->trace_no == trace_no)
        {
            return item;
        }
        item++;
    }

    return NULL;
}

