/*************************************************
 文 件 名:  flw_sp2113.c                        **
 功能描述:  产品交易控制维护                    **
 作    者:  邹佩                                **
 完成日期:  20160801                            **
*************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	group_en[60 + 1];
	char	tx_owner[32 + 1];
	char	tx_code[4 + 1];
	char	tx_name[60 + 1];
	char	tx_type[1 + 1];
	char	tx_status[1 + 1];
	char	tx_ctrl_date[1 + 1];
	char	tx_start_date[128 + 1];
	char	tx_end_date[128 + 1];
	char	tx_ctrl_limit[128 + 1];
	char	tx_lmt_min[128 + 1];
	char	tx_lmt_max[128 + 1];
	char	tx_lmt_day[128 + 1];
	char	tx_lmt_mon[128 + 1];
	char	tx_lmt_ttl[128 + 1];
	char	tx_ctrl_fee[1 + 1];
	char	tx_ctrl_auth[1 + 1];
	char	tx_ctrl_sign[1 + 1];
	char	tx_sign_chnl[256 + 1];
	char	tx_ctrl_mdm[1 + 1];
	char	tx_channels[256 + 1];
	char	tx_allows[60 + 1];
	char	tx_log[1 + 1];
	char	tx_ctrl[1 + 1];
	char	tx_sign[1 + 1];
	char	tx_itg[1 + 1];
	char	prdt_id[32 + 1];
	char	prdt_name[120 + 1];
	char	tx_ctrl_flow[1 + 1];
	char	tx_date[8 + 1];
	long long	tx_max_flow;
	long long 	tx_cur_flow;
}app_prdt_tx_t;

static int get_app_prdt_tx_req_info(sw_loc_vars_t *vars, app_prdt_tx_t *app_prdt_tx)
{
	char	buf[1024];
	
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.PRDT_ID", buf);
	strncpy(app_prdt_tx->prdt_id, buf, sizeof(app_prdt_tx->prdt_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.PRDT_ID]=[%s]=>[app_prdt_tx->prdt_id]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_FLOW", buf);
	strncpy(app_prdt_tx->tx_ctrl_flow, buf, sizeof(app_prdt_tx->tx_ctrl_flow) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_FLOW]=[%s]=>[app_prdt_tx->tx_ctrl_flow]",
		 __FILE__, __LINE__, buf);
	
	if (app_prdt_tx->tx_ctrl_flow[0] != '0' && app_prdt_tx->tx_ctrl_flow[0] != '\0')
	{
		char temp[256];

		memset(temp, 0x0, sizeof(buf));
		memset(buf, 0x0, sizeof(buf));

		loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_MAX_FLOW", buf);
		strncpy(temp, buf, sizeof(app_prdt_tx->tx_max_flow) - 1);
		app_prdt_tx->tx_max_flow = atoll(temp);
		pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_MAX_FLOW]=[%lld]=>[app_prdt_tx->tx_max_flow]",
			__FILE__, __LINE__, buf);

	}

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.GROUP_EN", buf);
	strncpy(app_prdt_tx->group_en, buf, sizeof(app_prdt_tx->group_en) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.GROUP_EN]=[%s]=>[app_prdt_tx->group_en]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_OWNER", buf);
	strncpy(app_prdt_tx->tx_owner, buf, sizeof(app_prdt_tx->tx_owner) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_OWNER]=[%s]=>[app_prdt_tx->tx_owner]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CODE", buf);
	strncpy(app_prdt_tx->tx_code, buf, sizeof(app_prdt_tx->tx_code) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CODE]=[%s]=>[app_prdt_tx->tx_code]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_NAME", buf);
	strncpy(app_prdt_tx->tx_name, buf, sizeof(app_prdt_tx->tx_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_NAME]=[%s]=>[app_prdt_tx->tx_name]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_TYPE", buf);
	strncpy(app_prdt_tx->tx_type, buf, sizeof(app_prdt_tx->tx_type) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_TYPE]=[%s]=>[app_prdt_tx->tx_type]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_STATUS", buf);
	strncpy(app_prdt_tx->tx_status, buf, sizeof(app_prdt_tx->tx_status) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_STATUS]=[%s]=>[app_prdt_tx->tx_status]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_DATE", buf);
	strncpy(app_prdt_tx->tx_ctrl_date, buf, sizeof(app_prdt_tx->tx_ctrl_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_DATE]=[%s]=>[app_prdt_tx->tx_ctrl_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_START_DATE", buf);
	strncpy(app_prdt_tx->tx_start_date, buf, sizeof(app_prdt_tx->tx_start_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_START_DATE]=[%s]=>[app_prdt_tx->tx_start_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_END_DATE", buf);
	strncpy(app_prdt_tx->tx_end_date, buf, sizeof(app_prdt_tx->tx_end_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_END_DATE]=[%s]=>[app_prdt_tx->tx_end_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_LIMIT", buf);
	strncpy(app_prdt_tx->tx_ctrl_limit, buf, sizeof(app_prdt_tx->tx_ctrl_limit) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_LIMIT]=[%s]=>[app_prdt_tx->tx_ctrl_limit]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_LMT_MIN", buf);
	strncpy(app_prdt_tx->tx_lmt_min, buf, sizeof(app_prdt_tx->tx_lmt_min) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_LMT_MIN]=[%s]=>[app_prdt_tx->tx_lmt_min]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_LMT_MAX", buf);
	strncpy(app_prdt_tx->tx_lmt_max, buf, sizeof(app_prdt_tx->tx_lmt_max) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_LMT_MAX]=[%s]=>[app_prdt_tx->tx_lmt_max]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_LMT_DAY", buf);
	strncpy(app_prdt_tx->tx_lmt_day, buf, sizeof(app_prdt_tx->tx_lmt_day) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_LMT_DAY]=[%s]=>[app_prdt_tx->tx_lmt_day]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_LMT_MON", buf);
	strncpy(app_prdt_tx->tx_lmt_mon, buf, sizeof(app_prdt_tx->tx_lmt_mon) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_LMT_MON]=[%s]=>[app_prdt_tx->tx_lmt_mon]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_LMT_TTL", buf);
	strncpy(app_prdt_tx->tx_lmt_ttl, buf, sizeof(app_prdt_tx->tx_lmt_ttl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_LMT_TTL]=[%s]=>[app_prdt_tx->tx_lmt_ttl]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_FEE", buf);
	strncpy(app_prdt_tx->tx_ctrl_fee, buf, sizeof(app_prdt_tx->tx_ctrl_fee) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_FEE]=[%s]=>[app_prdt_tx->tx_ctrl_fee]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_AUTH", buf);
	strncpy(app_prdt_tx->tx_ctrl_auth, buf, sizeof(app_prdt_tx->tx_ctrl_auth) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_AUTH]=[%s]=>[app_prdt_tx->tx_ctrl_auth]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_SIGN", buf);
	strncpy(app_prdt_tx->tx_ctrl_sign, buf, sizeof(app_prdt_tx->tx_ctrl_sign) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_SIGN]=[%s]=>[app_prdt_tx->tx_ctrl_sign]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_SIGN_CHNL", buf);
	strncpy(app_prdt_tx->tx_sign_chnl, buf, sizeof(app_prdt_tx->tx_sign_chnl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_SIGN_CHNL]=[%s]=>[app_prdt_tx->tx_sign_chnl]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL_MDM", buf);
	strncpy(app_prdt_tx->tx_ctrl_mdm, buf, sizeof(app_prdt_tx->tx_ctrl_mdm) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL_MDM]=[%s]=>[app_prdt_tx->tx_ctrl_mdm]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CHANNELS", buf);
	strncpy(app_prdt_tx->tx_channels, buf, sizeof(app_prdt_tx->tx_channels) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CHANNELS]=[%s]=>[app_prdt_tx->tx_channels]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_ALLOWS", buf);
	strncpy(app_prdt_tx->tx_allows, buf, sizeof(app_prdt_tx->tx_allows) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_ALLOWS]=[%s]=>[app_prdt_tx->tx_allows]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_LOG", buf);
	strncpy(app_prdt_tx->tx_log, buf, sizeof(app_prdt_tx->tx_log) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_LOG]=[%s]=>[app_prdt_tx->tx_log]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_CTRL", buf);
	strncpy(app_prdt_tx->tx_ctrl, buf, sizeof(app_prdt_tx->tx_ctrl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_CTRL]=[%s]=>[app_prdt_tx->tx_ctrl]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_ITG", buf);
	strncpy(app_prdt_tx->tx_itg, buf, sizeof(app_prdt_tx->tx_itg) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_ITG]=[%s]=>[app_prdt_tx->tx_itg]",
		 __FILE__, __LINE__, buf);
	return 0;
}

static int  get_channel_desc(char *tmpchannel, char *channeldesc )
{
	int	res = -1;
	int	i = 0;
	char	*pr = NULL;
	char	sql[256];
	char	*ptr = NULL;
	char	name[256];
	char	*p = NULL;
	
	if (tmpchannel == NULL)
	{
	    return -1;
	}
	
	p = tmpchannel;
	while ((pr = strsep(&p, "|")) != NULL && strlen(pr) != 0)
	{
		pub_log_debug("[%s][%d] 渠道标识[%s]", __FILE__, __LINE__, pr);
		
		memset(sql, 0x0, sizeof(sql));
		sprintf(sql,"select code_desc from app_data_dic where code_id='chnltype' and code_value='%s'", pr);
		pub_log_debug("[%s][%d]sql =[%s]",__FILE__, __LINE__, sql);
		
		memset(name, 0x0, sizeof(name));
		res = pub_db_squery(sql);
		if (res <= 0)
	  {
	     pub_log_error("[%s][%d]数据库查询失败", __FILE__, __LINE__);
	     return -1;
	  }
	  
	  ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));//按行列取值
	  if (ptr == NULL)
	  {
	  	pub_log_error("[%s][%d] 获取渠道中文名错误!", __FILE__, __LINE__);
	  }
	  
	  pub_str_rtrim(ptr);
		strcat(channeldesc, ptr);
		strcat(channeldesc, "|");
		pub_log_debug("[%s][%d] 渠道中文名[%s]",__FILE__, __LINE__, channeldesc);
	}
	
	for (i = strlen(channeldesc); i >= 0; i--)
	{
		if(channeldesc[i] == '|')
		{
			channeldesc[i] = '\0';
			break;	
		}	
	}
	
	return 0;
}

static int set_app_prdt_tx_res_info(sw_loc_vars_t *vars, app_prdt_tx_t *app_prdt_tx, int index)
{
	int	res = 0;
	char	path[256];
	char	buf[256];
	char	channeldesc[1024];
	char	tmpchannel[256];
	
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_FLOW", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_flow);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_flow);
	
	if ( app_prdt_tx->tx_ctrl_flow[0] != '0' && app_prdt_tx->tx_ctrl_flow[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x0, sizeof(path));
		sprintf(buf, "%lld", app_prdt_tx->tx_max_flow);
		sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_MAX_FLOW", index);
		loc_set_zd_data(vars, path, buf);
		
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, buf);
	}

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).PRDT_ID", index);
	loc_set_zd_data(vars, path, app_prdt_tx->prdt_id);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->prdt_id);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).PRDT_NAME", index);
	loc_set_zd_data(vars, path, app_prdt_tx->prdt_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->prdt_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).GROUP_EN", index);
	loc_set_zd_data(vars, path, app_prdt_tx->group_en);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->group_en);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_OWNER", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_owner);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_owner);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CODE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_code);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_NAME", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_TYPE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_type);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_type);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_STATUS", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_status);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_status);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_START_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_start_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_start_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_END_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_end_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_end_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_LIMIT", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_limit);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_limit);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_LMT_MIN", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_lmt_min);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_lmt_min);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_LMT_MAX", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_lmt_max);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_lmt_max);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_LMT_DAY", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_lmt_day);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_lmt_day);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_LMT_MON", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_lmt_mon);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_lmt_mon);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_LMT_TTL", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_lmt_ttl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_lmt_ttl);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_FEE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_fee);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_fee);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_AUTH", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_auth);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_auth);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_SIGN", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_sign);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_sign);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_SIGN_CHNL", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_sign_chnl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_sign_chnl);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL_MDM", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl_mdm);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl_mdm);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CHANNELS", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_channels);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_channels);
	
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_ALLOWS", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_allows);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_allows);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_LOG", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_log);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_log);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_ITG", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_itg);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_itg);
	
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CTRL", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_ctrl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_ctrl);
	
	memset(path, 0x0, sizeof(path));
	memset(channeldesc, 0x0, sizeof(channeldesc));
	memset(tmpchannel, 0x0, sizeof(tmpchannel));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CHANNELS_DESC", index);
	strncpy(tmpchannel,app_prdt_tx->tx_channels, sizeof(app_prdt_tx->tx_channels) -1  );
	pub_log_debug("[%s][%d],tmpchannel[%s]", __FILE__, __LINE__, tmpchannel);
	res = get_channel_desc(tmpchannel, channeldesc );
	if (res < 0)
	{
		pub_log_error("[%s][%d] get channels desc error",__FILE__,__LINE__);
		return -1;	
	}
	loc_set_zd_data(vars, path, channeldesc);
	
	pub_log_debug("[%s][%d] set:channeldesc[%s]=[%s]", __FILE__, __LINE__, path, channeldesc);
	return 0;
}

static int crt_upd_sql(app_prdt_tx_t *app_prdt_tx, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_prdt_tx->group_en[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " group_en = '%s',", app_prdt_tx->group_en);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_owner = '%s',", app_prdt_tx->tx_owner);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_code = '%s',", app_prdt_tx->tx_code);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_name = '%s',", app_prdt_tx->tx_name);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_type = '%s',", app_prdt_tx->tx_type);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_status[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_status = '%s',", app_prdt_tx->tx_status);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_date = '%s',", app_prdt_tx->tx_ctrl_date);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_start_date = %s,", app_prdt_tx->tx_start_date);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_end_date = %s,", app_prdt_tx->tx_end_date);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_limit[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_limit = %s,", app_prdt_tx->tx_ctrl_limit);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_lmt_min[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_lmt_min = %s,", app_prdt_tx->tx_lmt_min);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_lmt_max[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_lmt_max = %s,", app_prdt_tx->tx_lmt_max);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_lmt_day[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_lmt_day = %s,", app_prdt_tx->tx_lmt_day);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_lmt_mon[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_lmt_mon = %s,", app_prdt_tx->tx_lmt_mon);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_lmt_ttl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_lmt_ttl = %s,", app_prdt_tx->tx_lmt_ttl);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_fee[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_fee = '%s',", app_prdt_tx->tx_ctrl_fee);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_auth[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_auth = '%s',", app_prdt_tx->tx_ctrl_auth);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_sign = '%s',", app_prdt_tx->tx_ctrl_sign);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_sign_chnl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_sign_chnl = '%s',", app_prdt_tx->tx_sign_chnl);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_mdm[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_mdm = '%s',", app_prdt_tx->tx_ctrl_mdm);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_channels[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_channels = '%s',", app_prdt_tx->tx_channels);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_allows[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_allows = '%s',", app_prdt_tx->tx_allows);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_log[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_log = '%s',", app_prdt_tx->tx_log);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_itg[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_itg = '%s',", app_prdt_tx->tx_itg);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl_flow[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl_flow = '%s',", app_prdt_tx->tx_ctrl_flow);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_max_flow - 0 != 0)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_max_flow = %lld,", app_prdt_tx->tx_max_flow);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl = '%s',", app_prdt_tx->tx_ctrl);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->tx_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_sign = '%s',", app_prdt_tx->tx_sign);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_id = '%s',", app_prdt_tx->prdt_id);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';
	
	if (app_prdt_tx->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_code = '%s'", app_prdt_tx->tx_code);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_tx->prdt_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_prdt_tx set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_prdt_tx_t *app_prdt_tx, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");

	if (app_prdt_tx->group_en[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and group_en = '%s'", app_prdt_tx->group_en);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_owner = '%s'", app_prdt_tx->tx_owner);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_code = '%s'", app_prdt_tx->tx_code);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_name = '%s'", app_prdt_tx->tx_name);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_type = '%s'", app_prdt_tx->tx_type);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_status[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_status = '%s'", app_prdt_tx->tx_status);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl_date = '%s'", app_prdt_tx->tx_ctrl_date);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_start_date = %s", app_prdt_tx->tx_start_date);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_end_date = %s", app_prdt_tx->tx_end_date);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_limit[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl_limit = %s", app_prdt_tx->tx_ctrl_limit);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_lmt_min[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_lmt_min = %s", app_prdt_tx->tx_lmt_min);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_lmt_max[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_lmt_max = %s", app_prdt_tx->tx_lmt_max);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_lmt_day[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_lmt_day = %s", app_prdt_tx->tx_lmt_day);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_lmt_mon[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_lmt_mon = %s", app_prdt_tx->tx_lmt_mon);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_lmt_ttl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_lmt_ttl = %s", app_prdt_tx->tx_lmt_ttl);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_fee[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl_fee = '%s'", app_prdt_tx->tx_ctrl_fee);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_auth[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl_auth = '%s'", app_prdt_tx->tx_ctrl_auth);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl_sign = '%s'", app_prdt_tx->tx_ctrl_sign);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_sign_chnl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_sign_chnl = '%s'", app_prdt_tx->tx_sign_chnl);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_mdm[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl_mdm = '%s'", app_prdt_tx->tx_ctrl_mdm);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_channels[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_channels = '%s'", app_prdt_tx->tx_channels);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_allows[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_allows = '%s'", app_prdt_tx->tx_allows);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_log[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_log = '%s'", app_prdt_tx->tx_log);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_itg[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_itg = '%s'", app_prdt_tx->tx_itg);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl = '%s'", app_prdt_tx->tx_ctrl);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_sign = '%s'", app_prdt_tx->tx_sign);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and a.prdt_id = '%s'", app_prdt_tx->prdt_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->tx_ctrl_flow[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and a.tx_ctrl_flow = '%s'", app_prdt_tx->tx_ctrl_flow);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select a.*,b.prdt_name from app_prdt_tx a, app_products b\
		where a.prdt_id=b.prdt_id and %s", wherelist);

	return 0;
}

static int crt_sel_list_sql(app_prdt_tx_t *app_prdt_tx, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");

	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_tx->prdt_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->group_en[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and group_en = '%s'", app_prdt_tx->group_en);
		strcat(wherelist, buf);
	}
	if (app_prdt_tx->tx_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and (tx_code like '%%%s%%' or tx_name like '%%%s%%')", app_prdt_tx->tx_name, app_prdt_tx->tx_name);
		strcat(wherelist, buf);
	}
	
	sprintf(sql, "select * from app_prdt_tx where %s order by tx_code,prdt_id", wherelist);
	return 0;
}

int sp2113(sw_loc_vars_t *vars)
{
	int	ret = 0;
	char	opt[16];
	char	sql[2048];
	char	tmp[128];
	char	errcode[256];
	char	errmsg[256];

	memset(errcode, 0x00, sizeof(errcode));
	memset(errmsg, 0x0, sizeof(errmsg));
	memset(tmp, 0x00, sizeof(tmp));
	app_prdt_tx_t	app_prdt_tx;

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_prdt_tx");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_prdt_tx不存在");
		goto ErrExit;
	}
	
	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_prdt_tx, 0x0, sizeof(app_prdt_tx));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] 操作标识opt=[%s]", __FILE__, __LINE__, opt);
	if (opt[0] == 'M' && opt[0] == 'S' && opt[0] == 'L')
	{
		pub_log_error("操作标识[%s]有误", __FILE__, __LINE__, opt);
		strcpy(errmsg, "操作标识有误");
		goto ErrExit;
	}
	
	if (opt[0] == 'M')
	{
		pub_log_info("[%s][%d] 修改操作处理开始...", __FILE__, __LINE__);
		get_app_prdt_tx_req_info(vars, &app_prdt_tx);
		ret = crt_upd_sql(&app_prdt_tx, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成更新语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "更新失败");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]更新语句sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行更新语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "更新失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] 更新语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'S' || opt[0] == 'L')
	{
		int	i = 0;
		int	j = 0;
		int	cols = 0;
		int	rows = 0;
		int	count = 0;
		int	index = 0;
		int	ttlcnt = 0;
		int	pageidx = 0;
		int	pagecnt = 0;
		int	pagesum = 0;
		int	flage = 0;
		char	*ptr = NULL;
		char	buf[256];
		char	name[128];
		pub_log_info("[%s][%d] 查询操作处理开始...", __FILE__, __LINE__);

		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
		pagecnt = atoi(buf);

		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
		pageidx = atoi(buf);
		if (pagecnt == 0)
		{
			pagecnt = 1;
			flage = 1;
		}
		pub_log_info("[%s][%d] pagecount=[%d] pageindex=[%d]", __FILE__, __LINE__, pagecnt, pageidx);

		get_app_prdt_tx_req_info(vars, &app_prdt_tx);
		if(opt[0] == 'S')
		{
			ret = crt_sel_sql(&app_prdt_tx, sql);
		}
		else
		{
			ret = crt_sel_list_sql(&app_prdt_tx, sql);
		}

		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d]查询语句sql=[%s]", __FILE__, __LINE__, sql);
		if (pagecnt > 0)
		{
			ttlcnt = pub_db_get_fetrows(sql);
			if (ttlcnt < 0)
			{
				pub_log_error("[%s][%d] Get fetch rows error!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
			if (ttlcnt < pagecnt)
			{
				pagesum = 1;
			}
			else if (ttlcnt % pagecnt == 0)
			{
				pagesum = ttlcnt / pagecnt;
			}
			else
			{
				pagesum = ttlcnt / pagecnt + 1;
			}

			if (pageidx > pagesum)
			{
				pub_log_error("[%s][%d] 当前页码超出范围! pageindex=[%d] pagesum=[%d]", __FILE__, __LINE__, pageidx, pagesum);
				strcpy(errmsg, "页面超出范围");	
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_prdt_tx", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_prdt_tx");
		if (rows < 0)
		{
			pub_db_cclose("app_prdt_tx");
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_prdt_tx");
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			strcpy(errmsg, "无数据");
			goto ErrExit;
		}

		count = 0;
		index = 0;
		while (1)
		{
			for (i = 0; i < rows; i++)
			{
				count++;
				if (pagecnt > 0 && pageidx > 0)
				{
					if (count < (pageidx - 1) * pagecnt + 1)
					{
						continue;
					}
				}

				index++;
				if (pagecnt > 0 && index > pagecnt && flage == 0)
				{
					break;
				}

				memset(&app_prdt_tx, 0x0, sizeof(app_prdt_tx));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_prdt_tx", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] 字段名=[%s] 字段值=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "group_en") == 0)
					{
						strncpy(app_prdt_tx.group_en, ptr, sizeof(app_prdt_tx.group_en) - 1);
					}

					if (strcmp(name, "tx_owner") == 0)
					{
						strncpy(app_prdt_tx.tx_owner, ptr, sizeof(app_prdt_tx.tx_owner) - 1);
					}

					if (strcmp(name, "tx_code") == 0)
					{
						strncpy(app_prdt_tx.tx_code, ptr, sizeof(app_prdt_tx.tx_code) - 1);
					}

					if (strcmp(name, "tx_name") == 0)
					{
						strncpy(app_prdt_tx.tx_name, ptr, sizeof(app_prdt_tx.tx_name) - 1);
					}

					if (strcmp(name, "tx_type") == 0)
					{
						strncpy(app_prdt_tx.tx_type, ptr, sizeof(app_prdt_tx.tx_type) - 1);
					}

					if (strcmp(name, "tx_status") == 0)
					{
						strncpy(app_prdt_tx.tx_status, ptr, sizeof(app_prdt_tx.tx_status) - 1);
					}

					if (strcmp(name, "tx_ctrl_date") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_date, ptr, sizeof(app_prdt_tx.tx_ctrl_date) - 1);
					}

					if (strcmp(name, "tx_ctrl_flow") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_flow, ptr, sizeof(app_prdt_tx.tx_ctrl_flow) - 1);
					}

					if (strcmp(name, "tx_max_flow") == 0)
					{
						memset(buf, 0x0, sizeof(buf));
						strncpy(buf, ptr, sizeof(buf) - 1);
						app_prdt_tx.tx_max_flow = atoll(buf);
					}

					if (strcmp(name, "tx_start_date") == 0)
					{
						strncpy(app_prdt_tx.tx_start_date, ptr, sizeof(app_prdt_tx.tx_start_date) - 1);
					}

					if (strcmp(name, "tx_end_date") == 0)
					{
						strncpy(app_prdt_tx.tx_end_date, ptr, sizeof(app_prdt_tx.tx_end_date) - 1);
					}

					if (strcmp(name, "tx_ctrl_limit") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_limit, ptr, sizeof(app_prdt_tx.tx_ctrl_limit) - 1);
					}

					if (strcmp(name, "tx_lmt_min") == 0)
					{
						strncpy(app_prdt_tx.tx_lmt_min, ptr, sizeof(app_prdt_tx.tx_lmt_min) - 1);
					}

					if (strcmp(name, "tx_lmt_max") == 0)
					{
						strncpy(app_prdt_tx.tx_lmt_max, ptr, sizeof(app_prdt_tx.tx_lmt_max) - 1);
					}

					if (strcmp(name, "tx_lmt_day") == 0)
					{
						strncpy(app_prdt_tx.tx_lmt_day, ptr, sizeof(app_prdt_tx.tx_lmt_day) - 1);
					}

					if (strcmp(name, "tx_lmt_mon") == 0)
					{
						strncpy(app_prdt_tx.tx_lmt_mon, ptr, sizeof(app_prdt_tx.tx_lmt_mon) - 1);
					}

					if (strcmp(name, "tx_lmt_ttl") == 0)
					{
						strncpy(app_prdt_tx.tx_lmt_ttl, ptr, sizeof(app_prdt_tx.tx_lmt_ttl) - 1);
					}

					if (strcmp(name, "tx_ctrl_fee") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_fee, ptr, sizeof(app_prdt_tx.tx_ctrl_fee) - 1);
					}

					if (strcmp(name, "tx_ctrl_auth") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_auth, ptr, sizeof(app_prdt_tx.tx_ctrl_auth) - 1);
					}

					if (strcmp(name, "tx_ctrl_sign") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_sign, ptr, sizeof(app_prdt_tx.tx_ctrl_sign) - 1);
					}

					if (strcmp(name, "tx_sign_chnl") == 0)
					{
						strncpy(app_prdt_tx.tx_sign_chnl, ptr, sizeof(app_prdt_tx.tx_sign_chnl) - 1);
					}

					if (strcmp(name, "tx_ctrl_mdm") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl_mdm, ptr, sizeof(app_prdt_tx.tx_ctrl_mdm) - 1);
					}

					if (strcmp(name, "tx_channels") == 0)
					{
						strncpy(app_prdt_tx.tx_channels, ptr, sizeof(app_prdt_tx.tx_channels) - 1);
					}

					if (strcmp(name, "tx_allows") == 0)
					{
						strncpy(app_prdt_tx.tx_allows, ptr, sizeof(app_prdt_tx.tx_allows) - 1);
					}

					if (strcmp(name, "tx_log") == 0)
					{
						strncpy(app_prdt_tx.tx_log, ptr, sizeof(app_prdt_tx.tx_log) - 1);
					}

					if (strcmp(name, "tx_itg") == 0)
					{
						strncpy(app_prdt_tx.tx_itg, ptr, sizeof(app_prdt_tx.tx_itg) - 1);
					}

					if (strcmp(name, "tx_ctrl") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl, ptr, sizeof(app_prdt_tx.tx_ctrl) - 1);
					}

					if (strcmp(name, "tx_sign") == 0)
					{
						strncpy(app_prdt_tx.tx_sign, ptr, sizeof(app_prdt_tx.tx_sign) - 1);
					}

					if (strcmp(name, "prdt_id") == 0)
					{
						strncpy(app_prdt_tx.prdt_id, ptr, sizeof(app_prdt_tx.prdt_id) - 1);
					}

					if (strcmp(name, "prdt_name") == 0)
					{
						strncpy(app_prdt_tx.prdt_name, ptr, sizeof(app_prdt_tx.prdt_name) - 1);
					}
				}
				set_app_prdt_tx_res_info(vars, &app_prdt_tx, index - 1);
			}

			rows = pub_db_mfetch("app_prdt_tx");
			if (rows == 0)
			{
				pub_db_cclose("app_prdt_tx");
				pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_prdt_tx");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2113 处理成功!");
	if (opt[0] != 'S')
	{
		ret = record_oper_log(vars, errcode, errmsg);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 登记流水错误", __FILE__, __LINE__);
			strcpy(errmsg, "登记操作记录失败");
			goto ErrExit;
		}
	}
	ret = pub_db_commit();
	if (ret)
	{
		pub_log_error("[%s][%d] 提交数据库失败!", __FILE__, __LINE__);
		goto ErrExit;
	}
	pub_db_del_all_conn();
	return 0;

ErrExit:
	pub_log_info("[%s][%d] [%s] ERROR EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "E999");
	if (strlen(errmsg) > 0)
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", errmsg);
	}
	else
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2113 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
