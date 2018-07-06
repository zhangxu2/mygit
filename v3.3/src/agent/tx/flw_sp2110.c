/******************************************************************
  文 件 名:  flw_sp2110.c                       
  功能描述:  应用管理-应用信息-优惠规则维护页面，查询优惠规则记录
  作    者:  guxiaoxin                                    
  完成日期:  20160801                                   
 *******************************************************************/
#include "agent_comm.h"
#include "pub_db.h"

static 	char	errmsg[128];
static	char	errcode[8];
typedef struct
{
	char 	agio_no[6 + 1];
	char 	agio_name[60 + 1];
	char 	agio_type[4 + 1];
	char	tx_code[8 + 1];
	char	tx_name[60 + 1];
}app_agio_rule_t;

static int get_app_agio_rule_req_info(sw_loc_vars_t *vars, app_agio_rule_t * app_agio_rule)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio_Rule.TX_CODE", buf);
	strncpy(app_agio_rule->tx_code, buf, sizeof(app_agio_rule->tx_code) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Agio_Rule.TX_CODE]=[%s]=>[app_agio_rule->tx_code]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio_Rule.TX_NAME", buf);
	strncpy(app_agio_rule->tx_name, buf, sizeof(app_agio_rule->tx_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Agio_Rule.TX_NAME]=[%s]=>[app_agio_rule->tx_name]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x00, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio_Rule.AGIO_NAME", buf);
	strncpy(app_agio_rule->agio_name, buf, sizeof(app_agio_rule->agio_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Agio_Rule.AGIO_NAME]=[%s]=>[app_agio_rule->agio_name]", 
			__FILE__, __LINE__, buf);

	memset(buf, 0x00, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio_Rule.AGIO_TYPE", buf);
	strncpy(app_agio_rule->agio_type, buf, sizeof(app_agio_rule->agio_type) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Agio_Rule.AGIO_TYPE]=[%s]=>[app_agio_rule->agio_type]", 
			__FILE__, __LINE__, buf);
	return 0;
}

static int set_app_dime_tx_res_info(sw_loc_vars_t *vars, app_agio_rule_t *app_agio_rule,int index)
{
	char	path[256];
	char 	code_desc[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_NO", index);
	loc_set_zd_data(vars, path,app_agio_rule->agio_no);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_no);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_NAME", index);
	loc_set_zd_data(vars, path,app_agio_rule->agio_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_agio_rule->agio_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_TYPE", index);
	loc_set_zd_data(vars, path,app_agio_rule->agio_type);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_agio_rule->agio_type);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_TYPE_DESC", index);
	set_app_data_dic_trans("agio_type",app_agio_rule->agio_type, code_desc);
	loc_set_zd_data(vars, path, code_desc);
	pub_log_debug("[%s][%d] set:[%s]=[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_type, code_desc);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio_Rules.Eci_Agio_Rule(%d).TX_NAME", index);
	loc_set_zd_data(vars, path,app_agio_rule->tx_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_agio_rule->tx_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio_Rules.Eci_Agio_Rule(%d).TX_CODE", index);
	loc_set_zd_data(vars, path,app_agio_rule->tx_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_agio_rule->tx_code);

	return 0;
}

static int crt_sel_sql(app_agio_rule_t * app_agio_rule, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");

	if (app_agio_rule->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and b.dime_val = '%s'", app_agio_rule->tx_code);
		strcat(wherelist, buf);
	}
	if (app_agio_rule->agio_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and a.agio_type = '%s'", app_agio_rule->agio_type);
		strcat(wherelist, buf);
	}
	if (app_agio_rule->agio_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and a.agio_name like '%%%s%%'", app_agio_rule->agio_name);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select distinct(a.agio_no),a.agio_name,b.dime_val,d.tx_name,c.tx_code\
			from app_agio_rule a,app_dime_rate b,app_dime_tx c ,app_prdt_tx d \
			where %s and a.agio_no = b.rate_no and b.dime_val = c.tx_code  and c.tx_code = d.tx_code \
			and c.rate_type='1'", wherelist);
	return 0;
}


int sp2110(sw_loc_vars_t *vars)
{
	int		i       = 0;
	int		j       = 0;
	int		ret     = 0;
	int		cols    = 0;
	int		rows    = 0;
	int		count   = 0;
	int 	index   = 0;
	int		flage   = 0;
	int		ttlcnt  = 0;
	int		pageidx = 0;
	int		pagecnt = 0;
	int		pagesum = 0;
	char	*ptr    = NULL;
	char	buf[256];
	char	opt[16];
	char	sql[2048];
	char	name[128];
	char 	option[8];
	char 	sql_1[1024];
	char 	sql_2[1024];
	char 	sql_3[1024];
	char 	sql_4[1024];
	app_agio_rule_t 	app_agio_rule;

	ret = agt_table_detect("app_agio_rule");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_agio_rule不存在");
		pub_log_error("[%s][%d] %s", __FILE__, __LINE__, errmsg);
		goto ErrExit;
	}

	memset(&app_agio_rule, 0x0, sizeof(app_agio_rule));
	memset(sql_1, 0x00, sizeof(sql_1));
	memset(sql_2, 0x00, sizeof(sql_2));
	memset(sql_3, 0x00, sizeof(sql_3));
	memset(sql_4, 0x00, sizeof(sql_4));

	memset(option, 0x0, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);
	pub_log_info("[%s][%d]option[%s]", __FILE__, __LINE__, option);
	if (option[0] == 'S')
	{
		memset(&app_agio_rule, 0x0, sizeof(app_agio_rule));
		memset(buf, 0x0, sizeof(buf));
		pub_log_info("[%s][%d] 查询操作处理开始...", __FILE__, __LINE__);

		memset(opt, 0x0, sizeof(opt));
		memset(sql, 0x0, sizeof(sql));

		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
		pagecnt = atoi(buf);

		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
		pageidx = atoi(buf);
		if (pagecnt == 0)
		{
			pagecnt = 1;
			flage   = 1;
		}
		pub_log_info("[%s][%d] pagecount=[%d] pageindex=[%d]", __FILE__, __LINE__, pagecnt, pageidx);
		get_app_agio_rule_req_info(vars, &app_agio_rule);
		ret = crt_sel_sql(&app_agio_rule, sql);
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
				strcpy(errmsg, "游标查询数据失败");
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
				strcpy(errmsg, "页码超过范围");
				goto ErrExit;
			}
		}
		pub_log_info("[%s][%d] pageindex=[%d] pagesum=[%d]", __FILE__, __LINE__, pageidx, pagesum);

		cols = pub_db_mquery("app_agio_rule", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_agio_rule");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_agio_rule");
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			strcpy(errmsg, "无数据");
			goto ErrExit;
		}

		count = 0;
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

				memset(&app_agio_rule, 0x0, sizeof(app_agio_rule));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_agio_rule", i + 1, j + 1, name, sizeof(name));
					if (ptr == NULL)
					{
						pub_log_error("[%s][%d] get data and name which row=[%d] column=[%d] error", 
								__FILE__, __LINE__, i, j);
						strcpy(errmsg, "获取数据库数据失败");
						return -1;
					}
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_info("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
					if (strcmp(name, "dime_val") == 0)
					{
						strncpy(app_agio_rule.tx_code, ptr, sizeof(app_agio_rule.tx_code) - 1);
					}
					if (strcmp(name, "agio_name") == 0)
					{
						strncpy(app_agio_rule.agio_name, ptr, sizeof(app_agio_rule.agio_name) - 1);
					}
					if (strcmp(name, "agio_no") == 0)
					{
						strncpy(app_agio_rule.agio_no, ptr, sizeof(app_agio_rule.agio_no) - 1);
					}
					if (strcmp(name, "agio_type") == 0)
					{
						strncpy(app_agio_rule.agio_type, ptr, sizeof(app_agio_rule.agio_type) - 1);
					}
					if (strcmp(name, "tx_name") == 0)
					{
						strncpy(app_agio_rule.tx_name, ptr, sizeof(app_agio_rule.tx_name) - 1);
					}
				}
				set_app_dime_tx_res_info(vars, &app_agio_rule, index - 1);
			}

			rows = pub_db_mfetch("app_agio_rule");
			if (rows == 0)
			{
				pub_db_cclose("app_agio_rule");
				pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_agio_rule");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "游标查询数据失败");
				goto ErrExit;
			}
		}	
		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}
	else
	{
		strcpy(errcode, "E012");
		strcpy(errmsg, "参数错误");
		pub_log_error("[%s][%d] input parameter error", __FILE__, __LINE__);
		goto ErrExit;
	}


OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2110 处理成功!");

	ret = record_oper_log(vars, errcode, errmsg);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] 登记流水错误", __FILE__, __LINE__);
		strcpy(errmsg, "登记操作记录失败");
		goto ErrExit;
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
	pub_log_error("[%s][%d] [%s] ERROR EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", errcode);
	if (strlen(errmsg) > 0)
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", errmsg);
	}
	else
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2110 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}

