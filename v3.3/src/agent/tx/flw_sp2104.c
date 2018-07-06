/*************************************************
  文 件 名:  flw_sp2104.c                       **
  功能描述:  收费规则维护                       **
  作    者:  赵强                               **
  完成日期:  20160801                           **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"
static	char	errmsg[128];

typedef struct
{
	char    chrg_no[6 + 1];
	char    chrg_seq[4 + 1];
	char    chrg_type[1 + 1];
	char    chrg_rate[128 + 1];
	char    chrg_min_amt[128 + 1];
	char    chrg_max_amt[128 + 1];
	char    agio_mode[1 + 1];
	char    chrg_sts[1 + 1];
	char    memo[64 + 1];
	char	tx_code[4 + 1];
	char	tx_name[60 + 1];
	char	rate_type[2 + 1];
	char	dime_sets[1024 + 1];
}app_chrg_rule_t;


static int get_app_chrg_rule_req_info(sw_loc_vars_t *vars, app_chrg_rule_t *app_chrg_rule)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Chrg_Rule.TX_CODE", buf);
	strncpy(app_chrg_rule->tx_code, buf, sizeof(app_chrg_rule->tx_code) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Chrg_Rule.TX_CODE]=[%s]=>[app_chrg_rule->tx_code]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Chrg_Rule.TX_NAME", buf);
	strncpy(app_chrg_rule->tx_name, buf, sizeof(app_chrg_rule->tx_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Chrg_Rule.TX_NAME]=[%s]=>[app_chrg_rule->tx_name]",
			__FILE__, __LINE__, buf);

	return 0;
}

static int set_app_chrg_rule_res_info(sw_loc_vars_t *vars, app_chrg_rule_t *app_chrg_rule,int index)
{
	char	path[256];
	char 	code_desc[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Chrg_Rules.Eci_Chrg_Rule(%d).TX_CODE", index);
	loc_set_zd_data(vars, path, app_chrg_rule->tx_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->tx_code);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Chrg_Rules.Eci_Chrg_Rule(%d).TX_NAME", index);
	loc_set_zd_data(vars, path, app_chrg_rule->tx_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->tx_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Chrg_Rules.Eci_Chrg_Rule(%d).RATE_TYPE", index);
	loc_set_zd_data(vars, path, app_chrg_rule->rate_type);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->rate_type);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Chrg_Rules.Eci_Chrg_Rule(%d).RATE_TYPE_DESC", index);
	memset(code_desc, 0x0, sizeof(code_desc));
	set_app_data_dic_trans("rate_type",app_chrg_rule->rate_type, code_desc);
	loc_set_zd_data(vars, path, code_desc);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, code_desc);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Chrg_Rules.Eci_Chrg_Rule(%d).DIME_SETS", index);
	loc_set_zd_data(vars, path, app_chrg_rule->dime_sets);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->dime_sets);

	return 0;
}
static int crt_sel_sql(app_chrg_rule_t *app_chrg_rule, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_chrg_rule->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and a.tx_code = '%s'", app_chrg_rule->tx_code);
		strcat(wherelist, buf);
	}
	sprintf(sql, "select a.tx_code,a.rate_type  \
			from app_dime_tx a,app_dime_info b where %s and b.dime_no=a.dime_no and a.rate_type='0' \
			group by a.tx_code, a.rate_type ", wherelist);
	return 0;
}

static int get_app_prdt_tx(app_chrg_rule_t *app_chrg_rule, char * tx_code)
{
	int     cols = 0;
	int     i = 0;
	int     j = 0;
	char    sql[1024];
	char    *ptr = NULL;
	char    name[1024];

	memset(sql, 0x0, sizeof(sql));

	sprintf(sql, "select tx_name from app_prdt_tx where 1=1 and tx_code='%s'",tx_code);
	pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_squery(sql);
	if (cols <= 0)
	{
		pub_log_error("[%s][%d] 未查询到交易中文名! sql=[%s]", __FILE__, __LINE__, sql);
		return -1;
	}

	memset(name, 0x0, sizeof(name));
	ptr = pub_db_get_data_and_name(NULL, i + 1, j + 1, name, sizeof(name));
	agt_str_tolower(name);
	pub_str_ziphlspace(ptr);
	pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
	strncpy(app_chrg_rule->tx_name, ptr, sizeof(app_chrg_rule->tx_name) - 1);


	return 0;
}

int sp2104(sw_loc_vars_t *vars)
{
	int		ret = 0;
	char	opt[16];
	char	sql[2048];
	app_chrg_rule_t	app_chrg_rule;

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);
	ret = agt_table_detect("app_chrg_rule");
	if (ret < 0)
	{
		pub_log_error("[%s][%d]表不存在", __FILE__, __LINE__);
		strcpy(errmsg, "表app_chrg_rule不存在");
		goto ErrExit;
	}

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_chrg_rule, 0x0, sizeof(app_chrg_rule));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);
	if (opt[0] == 'S')
	{
		int		i = 0;
		int		j = 0;
		int		cols = 0;
		int		rows = 0;
		int		count = 0;
		int		index = 0;
		int		ttlcnt = 0;
		int		pageidx = 0;
		int		pagecnt = 0;
		int		pagesum = 0;
		int		flage = 0;
		char	*ptr = NULL;
		char	buf[256];
		char	name[128];
		char	sql2[1024];	
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

		get_app_chrg_rule_req_info(vars, &app_chrg_rule);
		ret = crt_sel_sql(&app_chrg_rule, sql);
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
				strcpy(errmsg, "查询数据失败");
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
				strcpy(errmsg, "无数据");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_chrg_rule", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_chrg_rule");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_chrg_rule");
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

				pub_log_debug("[%s][%d] cols======[%d]", __FILE__, __LINE__, cols);
				memset(&app_chrg_rule, 0x0, sizeof(app_chrg_rule));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_chrg_rule", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "tx_code") == 0)
					{
						strncpy(app_chrg_rule.tx_code, ptr, sizeof(app_chrg_rule.tx_code) - 1);
						get_app_prdt_tx(&app_chrg_rule,ptr);
					}
					if (strcmp(name, "rate_type") == 0)
					{
						strncpy(app_chrg_rule.rate_type, ptr, sizeof(app_chrg_rule.rate_type) - 1);
					}
				}
				memset(sql2, 0x0, sizeof(sql2));
				snprintf(sql2, sizeof(sql2)-1, "select b.dime_name from app_dime_tx a,app_dime_info b where b.dime_no=a.dime_no and a.tx_code = '%s' and a.rate_type = '%s'", app_chrg_rule.tx_code, app_chrg_rule.rate_type);
				ret = agt_wm_concat(sql2, app_chrg_rule.dime_sets);
				if (ret < 0)
				{
					pub_db_cclose("app_chrg_rule");
					pub_log_error("[%s][%d] agt_wm_concat error", __FILE__, __LINE__);
					strcpy(errmsg, "查询失败");
					goto ErrExit;
				}
				set_app_chrg_rule_res_info(vars, &app_chrg_rule,index - 1);
			}

			rows = pub_db_mfetch("app_chrg_rule");
			if (rows == 0)
			{
				pub_db_cclose("app_chrg_rule");
				pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_chrg_rule");
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

	pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2104 处理成功!");
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
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "E999");
	if (strlen(errmsg) > 0)
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", errmsg);
	}
	else
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2104 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
