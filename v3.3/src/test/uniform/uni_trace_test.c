#include "uni_trace.h"

int main(int argc, char* argv)
{
	sw_int_t	result = SW_ERROR;
	sw_int64_t	trace_no = 0;
	sw_int32_t	page_index = 1;
	sw_int32_t	page_count = 10;
	uni_trace_t	trace;
	uni_trace_info_t	*trace_info_list = NULL;
	sw_int32_t	len = 0;
	sw_int32_t	i = 0;
	sw_trace_step_t	*steps = NULL;
	sw_trace_step_t	step_array;
	
	pub_mem_memzero(&trace, sizeof(trace));
	result = uni_trace_init(&trace);

	if(result != SW_OK)
	{
		pub_log_error("%s, %d, uni_trace_init error."
			, __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = uni_trace_get_list(&trace, &trace_info_list, &len, page_index, page_count);

	if(result != SW_OK)
	{
		pub_log_error("%s, %d, uni_trace_get_list error."
			, __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < len; i++)
	{
		pub_log_info("%d, trace_no[%s] tx_code[%s] tx_date[%s] begin_time[%s] end_time[%s] cost_time[%s] tx_status[%s]"
			, i, trace_info_list[i].trace_no, trace_info_list[i].tx_code,trace_info_list[i].tx_date
			, trace_info_list[i].begin_time, trace_info_list[i].end_time, trace_info_list[i].cost_time, trace_info_list[i].tx_status);
	}

	pub_mem_free(trace_info_list);

	result = uni_trace_get_steps(&trace, 5, &steps, &len);
	
	if(result != SW_OK)
	{
		pub_log_error("%s, %d, uni_trace_get_steps error."
			, __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < len; i++)
	{
		pub_log_info("step[%s] module[%s] msg_type[%s] begin[%s] end[%s] deal_time[%s] deal_sts[%s] "
			,steps[i].step, steps[i].module, steps[i].msg_type, steps[i].begin, steps[i].end, steps[i].deal_time, steps[i].deal_sts);
	}
	pub_mem_free(steps);

	uni_trace_destory(&trace);
	
	return SW_OK;
}

