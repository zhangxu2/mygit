/*************************************************
  文 件 名:  flw_sp2103.c                        **
  功能描述:  产品交易表的操作                    **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

static	char	errmsg[128];
static	char	errcode[8];

typedef struct
{
	char	prdt_id[32 + 1];
	char	prdt_lvl[8 + 1];
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
	char	group_en[60 + 1];
	char	tx_message[1 + 1];
	char	tx_sign[1 + 1];
}app_prdt_tx_t;

/*获取产品交易请求信息*/
static int get_app_prdt_tx_req_info(sw_loc_vars_t *vars, app_prdt_tx_t *app_prdt_tx)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.PRDT_ID", buf);
	strncpy(app_prdt_tx->prdt_id, buf, sizeof(app_prdt_tx->prdt_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.PRDT_ID]=[%s]=>[app_prdt_tx->prdt_id]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.PRDT_LVL", buf);
	strncpy(app_prdt_tx->prdt_lvl, buf, sizeof(app_prdt_tx->prdt_lvl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.PRDT_LVL]=[%s]=>[app_prdt_tx->prdt_lvl]",
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
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.GROUP_EN", buf);
	strncpy(app_prdt_tx->group_en, buf, sizeof(app_prdt_tx->group_en) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.GROUP_EN]=[%s]=>[app_prdt_tx->group_en]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_MESSAGE", buf);
	strncpy(app_prdt_tx->tx_message, buf, sizeof(app_prdt_tx->tx_message) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_MESSAGE]=[%s]=>[app_prdt_tx->tx_message]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Tx.TX_SIGN", buf);
	strncpy(app_prdt_tx->tx_sign, buf, sizeof(app_prdt_tx->tx_sign) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Tx.TX_SIGN]=[%s]=>[app_prdt_tx->tx_sign]",
			__FILE__, __LINE__, buf);

	return 0;
}

/*设置产品交易查询应答信息*/
static int set_app_prdt_tx_res_info(sw_loc_vars_t *vars, app_prdt_tx_t *app_prdt_tx, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_CODE", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_code);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Txs.App_Prdt_Tx(%d).TX_NAME", index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_name);
	
	return 0;
}

static int crt_ins_sql(app_prdt_tx_t *app_prdt_tx, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->prdt_id);
		strcat(values, buf);
		strcat(cols, "prdt_id");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->prdt_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->prdt_lvl);
		strcat(values, buf);
		strcat(cols, "prdt_lvl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_owner);
		strcat(values, buf);
		strcat(cols, "tx_owner");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_code);
		strcat(values, buf);
		strcat(cols, "tx_code");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_name);
		strcat(values, buf);
		strcat(cols, "tx_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_type);
		strcat(values, buf);
		strcat(cols, "tx_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_status[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_status);
		strcat(values, buf);
		strcat(cols, "tx_status");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_ctrl_date);
		strcat(values, buf);
		strcat(cols, "tx_ctrl_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_start_date);
		strcat(values, buf);
		strcat(cols, "tx_start_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_end_date);
		strcat(values, buf);
		strcat(cols, "tx_end_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl_limit[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_ctrl_limit);
		strcat(values, buf);
		strcat(cols, "tx_ctrl_limit");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_lmt_min[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_lmt_min);
		strcat(values, buf);
		strcat(cols, "tx_lmt_min");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_lmt_max[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_lmt_max);
		strcat(values, buf);
		strcat(cols, "tx_lmt_max");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_lmt_day[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_lmt_day);
		strcat(values, buf);
		strcat(cols, "tx_lmt_day");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_lmt_mon[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_lmt_mon);
		strcat(values, buf);
		strcat(cols, "tx_lmt_mon");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_lmt_ttl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_prdt_tx->tx_lmt_ttl);
		strcat(values, buf);
		strcat(cols, "tx_lmt_ttl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl_fee[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_ctrl_fee);
		strcat(values, buf);
		strcat(cols, "tx_ctrl_fee");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl_auth[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_ctrl_auth);
		strcat(values, buf);
		strcat(cols, "tx_ctrl_auth");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_ctrl_sign);
		strcat(values, buf);
		strcat(cols, "tx_ctrl_sign");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_sign_chnl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_sign_chnl);
		strcat(values, buf);
		strcat(cols, "tx_sign_chnl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl_mdm[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_ctrl_mdm);
		strcat(values, buf);
		strcat(cols, "tx_ctrl_mdm");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_channels[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_channels);
		strcat(values, buf);
		strcat(cols, "tx_channels");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_allows[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_allows);
		strcat(values, buf);
		strcat(cols, "tx_allows");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_log[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_log);
		strcat(values, buf);
		strcat(cols, "tx_log");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->tx_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->tx_ctrl);
		strcat(values, buf);
		strcat(cols, "tx_ctrl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_prdt_tx->group_en[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_prdt_tx->group_en);
		strcat(values, buf);
		strcat(cols, "group_en");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (cols[0] == '\0')
	{
		pub_log_error("[%s][%d] 字段列表为空!", __FILE__, __LINE__);
		return -1;
	}

	i = strlen(cols) - 1;
	if (cols[i] == ',')
	{
		cols[i] = '\0';
	}

	i = strlen(values) - 1;
	if (values[i] == ',')
	{
		values[i] = '\0';
	}

	sprintf(sql, "insert into app_prdt_tx(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_prdt_tx_t *app_prdt_tx, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	sprintf(sql, "delete from app_prdt_tx where %s", wherelist);

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
	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_id = '%s',", app_prdt_tx->prdt_id);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->prdt_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_lvl = '%s',", app_prdt_tx->prdt_lvl);
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

	if (app_prdt_tx->tx_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " tx_ctrl = '%s',", app_prdt_tx->tx_ctrl);
		strcat(setlist, buf);
	}

	if (app_prdt_tx->group_en[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " group_en = '%s',", app_prdt_tx->group_en);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	if (setlist[i] == ',')
	{
		setlist[i] = '\0';
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
	if (app_prdt_tx->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_tx->prdt_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->prdt_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_lvl = '%s'", app_prdt_tx->prdt_lvl);
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

	if (app_prdt_tx->tx_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_ctrl = '%s'", app_prdt_tx->tx_ctrl);
		strcat(wherelist, buf);
	}

	if (app_prdt_tx->group_en[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and group_en = '%s'", app_prdt_tx->group_en);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select tx_code, tx_name from app_prdt_tx where %s ", wherelist);

	return 0;
}

int sp2103(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	char	opt[16];
	char	sql[2048];
	app_prdt_tx_t	app_prdt_tx;

	memset(errmsg, 0x0, sizeof(errmsg));
	memset(errcode, 0x0, sizeof(errcode));

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_prdt_tx");
	if (ret < 0)
	{
		pub_log_error("[%s][%d]表不存在", __FILE__, __LINE__);
		strcpy(errmsg, "表app_prdt_tx不存在");
		goto ErrExit;
	}
	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_prdt_tx, 0x0, sizeof(app_prdt_tx));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);

	if (opt[0] == 'A')
	{
		pub_log_info("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		get_app_prdt_tx_req_info(vars, &app_prdt_tx);
		ret = crt_ins_sql(&app_prdt_tx, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成插入语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] insert sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行插入语句失败");
			strcpy(errcode, "S002");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}
	else if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_prdt_tx_req_info(vars, &app_prdt_tx);
		ret = crt_del_sql(&app_prdt_tx, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成删除语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "删除失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] delete sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行删除语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "删除失败");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 删除语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}
	else if (opt[0] == 'M')
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

		pub_log_info("[%s][%d] update sql=[%s]", __FILE__, __LINE__, sql);
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
	else if (opt[0] == 'S')
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
		pub_log_debug("[%s][%d] pagecount=[%d] pageindex=[%d]", __FILE__, __LINE__, pagecnt, pageidx);

		get_app_prdt_tx_req_info(vars, &app_prdt_tx);
		ret = crt_sel_sql(&app_prdt_tx, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] select sql=[%s]", __FILE__, __LINE__, sql);
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
				pub_log_error("[%s][%d] PageIndex error! pageindex=[%d] pagesum=[%d]", __FILE__, __LINE__, pageidx, pagesum);
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
			pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
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
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "prdt_id") == 0)
					{
						strncpy(app_prdt_tx.prdt_id, ptr, sizeof(app_prdt_tx.prdt_id) - 1);
					}

					if (strcmp(name, "prdt_lvl") == 0)
					{
						strncpy(app_prdt_tx.prdt_lvl, ptr, sizeof(app_prdt_tx.prdt_lvl) - 1);
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

					if (strcmp(name, "tx_ctrl") == 0)
					{
						strncpy(app_prdt_tx.tx_ctrl, ptr, sizeof(app_prdt_tx.tx_ctrl) - 1);
					}

					if (strcmp(name, "group_en") == 0)
					{
						strncpy(app_prdt_tx.group_en, ptr, sizeof(app_prdt_tx.group_en) - 1);
					}
				}
				set_app_prdt_tx_res_info(vars, &app_prdt_tx, index - 1);
			}

			rows = pub_db_mfetch("app_prdt_tx");
			if (rows == 0)
			{
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				pub_db_cclose("app_prdt_tx");
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

		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}
	else
	{
		pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
		strcpy(errmsg, "操作标识有误");
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2103 处理成功!");
	ret = pub_db_commit();
	if (ret)
	{
		pub_log_error("[%s][%d] 提交数据库失败!", __FILE__, __LINE__);
		strcpy(errmsg, "提交数据库失败");
		goto ErrExit;
	}
	pub_db_del_all_conn();
	return 0;

ErrExit:
	pub_log_info("[%s][%d] [%s] ERROR EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", errcode);
	if (strlen(errmsg) > 0)
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", errmsg);
	}
	else
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2103 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
