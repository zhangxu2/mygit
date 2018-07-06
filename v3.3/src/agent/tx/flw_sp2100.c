/*************************************************
 文 件 名:  flw_sp2100.c                        **
 功能描述:  获取产品列表                        **
 作    者:  邹佩                                **
 完成日期:  20160801                            **
*************************************************/
#include "agent_comm.h"

int sp2100(sw_loc_vars_t *vars)
{
	int	i = 0, j = 0;
	int	page_cnt = 0;
	int	page_idx = 0;
	int	page_sum = 0;
	char	res_msg[256];
	char	reply[8];
	char	buf[256];
	char	path[512];
	unit_t  *pproduct = NULL;
	unit_t  *punit = NULL;

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));
	
	pub_log_info("[%s][%d] 2100交易处理开始!", __FILE__, __LINE__);
	pub_mem_memzero(buf, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
	page_idx = atoi(buf);

	pub_mem_memzero(buf, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
	page_cnt = atoi(buf);

	if (page_cnt == 0 || page_idx == 0)
	{
		page_cnt = 1;
	}

	pproduct = get_prdt_info();
	if (pproduct == NULL)
	{
		pub_log_error("%s, %d, 获取产品列表失败",__FILE__,__LINE__);
		strcpy(reply, "E047");
		goto ErrExit;
	}

	j = 0;
	for (punit = pproduct, i = 0; punit != NULL; punit = punit->next, i++)
	{
		if (page_cnt != 1 && (i < (page_idx-1) * page_cnt || i >= page_idx * page_cnt))
		{
			continue;
		}

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path) - 1, ".TradeRecord.Response.Products.Product(%d).CNNAME", j);
		pub_log_debug("[%s][%d]产品中文名[%s]", __FILE__, __LINE__, punit->cnname);
		loc_set_zd_data(vars, path, punit->cnname);

		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path) - 1, ".TradeRecord.Response.Products.Product(%d).NAME", j);
		pub_log_debug("[%s][%d]产品名[%s]", __FILE__, __LINE__, punit->name);
		loc_set_zd_data(vars, path, punit->name);
		j++;
	}
	
	free_unit(pproduct);
	pproduct = NULL;
	
	if (i % page_cnt)
	{
		page_sum = i / page_cnt + 1;
	}
	else
	{
		page_sum = i / page_cnt;
	}
	
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "%d", page_sum);
	pub_log_debug("[%s][%d]页码总数[%d]", __FILE__, __LINE__, page_sum);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path) - 1, ".TradeRecord.Response.PageSum");
	loc_set_zd_data(vars, path, buf);

	goto OkExit;
OkExit:
	pub_log_info("[%s][%d] [%s]OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return 0;
ErrExit:
	pub_log_info("[%s][%d] [%s]ERROR EXIT!", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
