/*************************************************
 文 件 名:  flw_sp2116.c                        **
 功能描述:  产品信息表维护                      **
 作    者:  邹佩                                **
 完成日期:  20160801                            **
*************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	prdt_id[32 + 1];
	char	prdt_name[60 + 1];
	char	prdt_up_id[32 + 1];
	char	prdt_lvl[4 + 1];
	char	prdt_type[4 + 1];
	char	prdt_ctgy[20 + 1];
	char	prdt_bus[20 + 1];
	char	prdt_ver[10 + 1];
	char	prdt_mangr[32 + 1];
	char	prdt_datetp[1 + 1];
	char	prdt_ctrl_date[1 + 1];
	char	prdt_start_date[128 + 1];
	char	prdt_end_date[128 + 1];
	char	prdt_ctrl_chnl[1 + 1];
	char	prdt_ctrl_brno[1 + 1];
	char	prdt_ctrl_fee[1 + 1];
	char	prdt_ctrl_sign[1 + 1];
	char	prdt_memo[60 + 1];
}app_products_t;

extern char *agt_str_tolower(char *str);
static int set_app_products_res_info(sw_loc_vars_t *vars, app_products_t *app_products, int index);

static int get_app_products_req_info(sw_loc_vars_t *vars, app_products_t *app_products)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_ID", buf);
	strncpy(app_products->prdt_id, buf, sizeof(app_products->prdt_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_ID]=[%s]=>[app_products->prdt_id]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_NAME", buf);
	strncpy(app_products->prdt_name, buf, sizeof(app_products->prdt_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_NAME]=[%s]=>[app_products->prdt_name]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_LVL", buf);
	strncpy(app_products->prdt_lvl, buf, sizeof(app_products->prdt_lvl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_LVL]=[%s]=>[app_products->prdt_lvl]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_TYPE", buf);
	strncpy(app_products->prdt_type, buf, sizeof(app_products->prdt_type) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_TYPE]=[%s]=>[app_products->prdt_type]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_CTGY", buf);
	strncpy(app_products->prdt_ctgy, buf, sizeof(app_products->prdt_ctgy) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_CTGY]=[%s]=>[app_products->prdt_ctgy]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_BUS", buf);
	strncpy(app_products->prdt_bus, buf, sizeof(app_products->prdt_bus) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_BUS]=[%s]=>[app_products->prdt_bus]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_VER", buf);
	strncpy(app_products->prdt_ver, buf, sizeof(app_products->prdt_ver) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_VER]=[%s]=>[app_products->prdt_ver]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_MANGR", buf);
	strncpy(app_products->prdt_mangr, buf, sizeof(app_products->prdt_mangr) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_MANGR]=[%s]=>[app_products->prdt_mangr]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_DATETP", buf);
	strncpy(app_products->prdt_datetp, buf, sizeof(app_products->prdt_datetp) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_DATETP]=[%s]=>[app_products->prdt_datetp]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_CTRL_DATE", buf);
	strncpy(app_products->prdt_ctrl_date, buf, sizeof(app_products->prdt_ctrl_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_CTRL_DATE]=[%s]=>[app_products->prdt_ctrl_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_START_DATE", buf);
	strncpy(app_products->prdt_start_date, buf, sizeof(app_products->prdt_start_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_START_DATE]=[%s]=>[app_products->prdt_start_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_END_DATE", buf);
	strncpy(app_products->prdt_end_date, buf, sizeof(app_products->prdt_end_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_END_DATE]=[%s]=>[app_products->prdt_end_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_CTRL_CHNL", buf);
	strncpy(app_products->prdt_ctrl_chnl, buf, sizeof(app_products->prdt_ctrl_chnl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_CTRL_CHNL]=[%s]=>[app_products->prdt_ctrl_chnl]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_CTRL_BRNO", buf);
	strncpy(app_products->prdt_ctrl_brno, buf, sizeof(app_products->prdt_ctrl_brno) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_CTRL_BRNO]=[%s]=>[app_products->prdt_ctrl_brno]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_CTRL_FEE", buf);
	strncpy(app_products->prdt_ctrl_fee, buf, sizeof(app_products->prdt_ctrl_fee) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_CTRL_FEE]=[%s]=>[app_products->prdt_ctrl_fee]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_CTRL_SIGN", buf);
	strncpy(app_products->prdt_ctrl_sign, buf, sizeof(app_products->prdt_ctrl_sign) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_CTRL_SIGN]=[%s]=>[app_products->prdt_ctrl_sign]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Products.PRDT_MEMO", buf);
	strncpy(app_products->prdt_memo, buf, sizeof(app_products->prdt_memo) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Products.PRDT_MEMO]=[%s]=>[app_products->prdt_memo]",
		 __FILE__, __LINE__, buf);

	return 0;
}

static int set_app_products_res_info(sw_loc_vars_t *vars, app_products_t *app_products, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_ID", index);
	loc_set_zd_data(vars, path, app_products->prdt_id);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_id);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_NAME", index);
	loc_set_zd_data(vars, path, app_products->prdt_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_LVL", index);
	loc_set_zd_data(vars, path, app_products->prdt_lvl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_lvl);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_TYPE", index);
	loc_set_zd_data(vars, path, app_products->prdt_type);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_type);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_CTGY", index);
	loc_set_zd_data(vars, path, app_products->prdt_ctgy);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ctgy);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_BUS", index);
	loc_set_zd_data(vars, path, app_products->prdt_bus);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_bus);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_VER", index);
	loc_set_zd_data(vars, path, app_products->prdt_ver);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ver);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_MANGR", index);
	loc_set_zd_data(vars, path, app_products->prdt_mangr);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_mangr);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_DATETP", index);
	loc_set_zd_data(vars, path, app_products->prdt_datetp);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_datetp);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_CTRL_DATE", index);
	loc_set_zd_data(vars, path, app_products->prdt_ctrl_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ctrl_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_START_DATE", index);
	loc_set_zd_data(vars, path, app_products->prdt_start_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_start_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_END_DATE", index);
	loc_set_zd_data(vars, path, app_products->prdt_end_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_end_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_CTRL_CHNL", index);
	loc_set_zd_data(vars, path, app_products->prdt_ctrl_chnl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ctrl_chnl);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_CTRL_BRNO", index);
	loc_set_zd_data(vars, path, app_products->prdt_ctrl_brno);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ctrl_brno);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_CTRL_FEE", index);
	loc_set_zd_data(vars, path, app_products->prdt_ctrl_fee);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ctrl_fee);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_CTRL_SIGN", index);
	loc_set_zd_data(vars, path, app_products->prdt_ctrl_sign);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_ctrl_sign);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Productss.App_Products(%d).PRDT_MEMO", index);
	loc_set_zd_data(vars, path, app_products->prdt_memo);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_products->prdt_memo);

	return 0;
}

static int crt_ins_sql(app_products_t *app_products, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	if (app_products->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_id);
		strcat(values, buf);
		strcat(cols, "prdt_id");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_name);
		strcat(values, buf);
		strcat(cols, "prdt_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_lvl);
		strcat(values, buf);
		strcat(cols, "prdt_lvl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_type);
		strcat(values, buf);
		strcat(cols, "prdt_type");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ctgy[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ctgy);
		strcat(values, buf);
		strcat(cols, "prdt_ctgy");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_bus[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_bus);
		strcat(values, buf);
		strcat(cols, "prdt_bus");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ver[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ver);
		strcat(values, buf);
		strcat(cols, "prdt_ver");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_mangr[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_mangr);
		strcat(values, buf);
		strcat(cols, "prdt_mangr");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_datetp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_datetp);
		strcat(values, buf);
		strcat(cols, "prdt_datetp");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ctrl_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ctrl_date);
		strcat(values, buf);
		strcat(cols, "prdt_ctrl_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_products->prdt_start_date);
		strcat(values, buf);
		strcat(cols, "prdt_start_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_products->prdt_end_date);
		strcat(values, buf);
		strcat(cols, "prdt_end_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ctrl_chnl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ctrl_chnl);
		strcat(values, buf);
		strcat(cols, "prdt_ctrl_chnl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ctrl_brno[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ctrl_brno);
		strcat(values, buf);
		strcat(cols, "prdt_ctrl_brno");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ctrl_fee[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ctrl_fee);
		strcat(values, buf);
		strcat(cols, "prdt_ctrl_fee");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_ctrl_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_ctrl_sign);
		strcat(values, buf);
		strcat(cols, "prdt_ctrl_sign");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_products->prdt_memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_products->prdt_memo);
		strcat(values, buf);
		strcat(cols, "prdt_memo");
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

	sprintf(sql, "insert into app_products(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_products_t *app_products, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_products->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s' ", app_products->prdt_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "delete from app_products where %s", wherelist);

	return 0;
}

static int crt_upd_sql(app_products_t *app_products, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_products->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_id = '%s',", app_products->prdt_id);
		strcat(setlist, buf);
	}

	if (app_products->prdt_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_name = '%s',", app_products->prdt_name);
		strcat(setlist, buf);
	}

	if (app_products->prdt_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_lvl = '%s',", app_products->prdt_lvl);
		strcat(setlist, buf);
	}

	if (app_products->prdt_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_type = '%s',", app_products->prdt_type);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ctgy[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ctgy = '%s',", app_products->prdt_ctgy);
		strcat(setlist, buf);
	}

	if (app_products->prdt_bus[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_bus = '%s',", app_products->prdt_bus);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ver[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ver = '%s',", app_products->prdt_ver);
		strcat(setlist, buf);
	}

	if (app_products->prdt_mangr[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_mangr = '%s',", app_products->prdt_mangr);
		strcat(setlist, buf);
	}

	if (app_products->prdt_datetp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_datetp = '%s',", app_products->prdt_datetp);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ctrl_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ctrl_date = '%s',", app_products->prdt_ctrl_date);
		strcat(setlist, buf);
	}

	if (app_products->prdt_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_start_date = %s,", app_products->prdt_start_date);
		strcat(setlist, buf);
	}

	if (app_products->prdt_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_end_date = %s,", app_products->prdt_end_date);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ctrl_chnl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ctrl_chnl = '%s',", app_products->prdt_ctrl_chnl);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ctrl_brno[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ctrl_brno = '%s',", app_products->prdt_ctrl_brno);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ctrl_fee[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ctrl_fee = '%s',", app_products->prdt_ctrl_fee);
		strcat(setlist, buf);
	}

	if (app_products->prdt_ctrl_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_ctrl_sign = '%s',", app_products->prdt_ctrl_sign);
		strcat(setlist, buf);
	}

	if (app_products->prdt_memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " prdt_memo = '%s',", app_products->prdt_memo);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';

	if (app_products->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_products->prdt_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_products set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_products_t *app_products, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_products->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_products->prdt_id);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_name = '%s'", app_products->prdt_name);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_lvl = '%s'", app_products->prdt_lvl);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_type = '%s'", app_products->prdt_type);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ctgy[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ctgy = '%s'", app_products->prdt_ctgy);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_bus[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_bus = '%s'", app_products->prdt_bus);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ver[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ver = '%s'", app_products->prdt_ver);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_mangr[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_mangr = '%s'", app_products->prdt_mangr);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_datetp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_datetp = '%s'", app_products->prdt_datetp);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ctrl_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ctrl_date = '%s'", app_products->prdt_ctrl_date);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_start_date = %s", app_products->prdt_start_date);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_end_date = %s", app_products->prdt_end_date);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ctrl_chnl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ctrl_chnl = '%s'", app_products->prdt_ctrl_chnl);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ctrl_brno[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ctrl_brno = '%s'", app_products->prdt_ctrl_brno);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ctrl_fee[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ctrl_fee = '%s'", app_products->prdt_ctrl_fee);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_ctrl_sign[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_ctrl_sign = '%s'", app_products->prdt_ctrl_sign);
		strcat(wherelist, buf);
	}

	if (app_products->prdt_memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_memo = '%s'", app_products->prdt_memo);
		strcat(wherelist, buf);
	}
	
	sprintf(sql, "select * from app_products where %s", wherelist);
	return 0;
}

int sp2116(sw_loc_vars_t *vars)
{
	int	ret = 0;
	char	opt[16];
	char	sql[2048];
	char	tmp[128];
	char    errcode[256];
	char    errmsg[256];

	memset(errcode, 0x00, sizeof(errcode));
	memset(errmsg, 0x0, sizeof(errmsg));
	memset(tmp, 0x00, sizeof(tmp));
	app_products_t	app_products;

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_products");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_products不存在");
		goto ErrExit;
	}

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_products, 0x0, sizeof(app_products));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] 操作标识opt=[%s]", __FILE__, __LINE__, opt);
	if (opt[0] == 'A')
	{
		pub_log_info("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		get_app_products_req_info(vars, &app_products);
		ret = crt_ins_sql(&app_products, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成插入语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]插入语句sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_products_req_info(vars, &app_products);
		ret = crt_del_sql(&app_products, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成删除语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "删除失败");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]删除语句sql=[%s]", __FILE__, __LINE__, sql);
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

	if (opt[0] == 'M')
	{
		pub_log_info("[%s][%d] 修改操作处理开始...", __FILE__, __LINE__);
		get_app_products_req_info(vars, &app_products);
		ret = crt_upd_sql(&app_products, sql);
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
			pub_log_error("[%s][%d] 执行删除语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "更新失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] 更新语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'S')
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
		get_app_products_req_info(vars, &app_products);
		ret = crt_sel_sql(&app_products, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]查询语句sql=[%s]", __FILE__, __LINE__, sql);
		if (pagecnt > 0)
		{
			ttlcnt = pub_db_get_fetrows(sql);
			if (ttlcnt < 0)
			{
				pub_log_error("[%s][%d]获取行数失败!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d]当前页码超出范围! pageindex=[%d] pagesum=[%d]", __FILE__, __LINE__, pageidx, pagesum);
				strcpy(errmsg, "页面超出范围");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_products", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_products");
		if (rows < 0)
		{
			pub_db_cclose("app_products");
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_products");
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

				memset(&app_products, 0x0, sizeof(app_products));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_products", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_info("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "prdt_id") == 0)
					{
						strncpy(app_products.prdt_id, ptr, sizeof(app_products.prdt_id) - 1);
					}

					if (strcmp(name, "prdt_name") == 0)
					{
						strncpy(app_products.prdt_name, ptr, sizeof(app_products.prdt_name) - 1);
					}

					if (strcmp(name, "prdt_up_id") == 0)
					{
						strncpy(app_products.prdt_up_id, ptr, sizeof(app_products.prdt_up_id) - 1);
					}

					if (strcmp(name, "prdt_lvl") == 0)
					{
						strncpy(app_products.prdt_lvl, ptr, sizeof(app_products.prdt_lvl) - 1);
					}

					if (strcmp(name, "prdt_type") == 0)
					{
						strncpy(app_products.prdt_type, ptr, sizeof(app_products.prdt_type) - 1);
					}

					if (strcmp(name, "prdt_ctgy") == 0)
					{
						strncpy(app_products.prdt_ctgy, ptr, sizeof(app_products.prdt_ctgy) - 1);
					}

					if (strcmp(name, "prdt_bus") == 0)
					{
						strncpy(app_products.prdt_bus, ptr, sizeof(app_products.prdt_bus) - 1);
					}

					if (strcmp(name, "prdt_ver") == 0)
					{
						strncpy(app_products.prdt_ver, ptr, sizeof(app_products.prdt_ver) - 1);
					}

					if (strcmp(name, "prdt_mangr") == 0)
					{
						strncpy(app_products.prdt_mangr, ptr, sizeof(app_products.prdt_mangr) - 1);
					}

					if (strcmp(name, "prdt_datetp") == 0)
					{
						strncpy(app_products.prdt_datetp, ptr, sizeof(app_products.prdt_datetp) - 1);
					}

					if (strcmp(name, "prdt_ctrl_date") == 0)
					{
						strncpy(app_products.prdt_ctrl_date, ptr, sizeof(app_products.prdt_ctrl_date) - 1);
					}

					if (strcmp(name, "prdt_start_date") == 0)
					{
						strncpy(app_products.prdt_start_date, ptr, sizeof(app_products.prdt_start_date) - 1);
					}

					if (strcmp(name, "prdt_end_date") == 0)
					{
						strncpy(app_products.prdt_end_date, ptr, sizeof(app_products.prdt_end_date) - 1);
					}

					if (strcmp(name, "prdt_ctrl_chnl") == 0)
					{
						strncpy(app_products.prdt_ctrl_chnl, ptr, sizeof(app_products.prdt_ctrl_chnl) - 1);
					}

					if (strcmp(name, "prdt_ctrl_brno") == 0)
					{
						strncpy(app_products.prdt_ctrl_brno, ptr, sizeof(app_products.prdt_ctrl_brno) - 1);
					}

					if (strcmp(name, "prdt_ctrl_fee") == 0)
					{
						strncpy(app_products.prdt_ctrl_fee, ptr, sizeof(app_products.prdt_ctrl_fee) - 1);
					}

					if (strcmp(name, "prdt_ctrl_sign") == 0)
					{
						strncpy(app_products.prdt_ctrl_sign, ptr, sizeof(app_products.prdt_ctrl_sign) - 1);
					}

					if (strcmp(name, "prdt_memo") == 0)
					{
						strncpy(app_products.prdt_memo, ptr, sizeof(app_products.prdt_memo) - 1);
					}
				}
				set_app_products_res_info(vars, &app_products, index - 1);
			}

			rows = pub_db_mfetch("app_products");
			if (rows == 0)
			{
				pub_db_cclose("app_products");
				pub_log_info("[%s][%d] 查询结束!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_products");
				pub_log_error("[%s][%d] 查询失败!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");	
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}

	pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2116 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2116 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
