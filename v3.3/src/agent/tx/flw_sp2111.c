/*************************************************
  文 件 名:  flw_sp2111.c                        **
  功能描述:  优惠规则维护                        **
  作    者:  baidan                              **
  完成日期:  2016-08-02                          **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

static 	char	errmsg[128];
static	char	errcode[8];
typedef struct
{
	char 	rate_type[32 + 1];
	char 	tx_code[32 + 1];
	char 	dime_no[32 + 1];
	char 	rate_no[32 + 1];
	char	dt_start_date[10 + 1];
	char	dt_end_date[10 + 1];	
}app_dime_tx_t;

typedef struct
{
	char    dime_name[64][60 + 1];
	char    dime_val[64][32 + 1];
	char    dr_dsp[64][60 + 1];
	char    dime_no[64][32 + 1];
	char	dt_sts[64][1 + 1];
}app_dime_info_t;

typedef struct 
{
	char	tx_code[4 + 1];
	char	tx_name[60 + 1];
	char	tx_type[2 + 1];
}app_prdt_tx_t;

typedef struct
{
	char agio_no[6 + 1];
	char agio_seq[64][6 + 1];
	char agio_name[64][60 + 1];
	char agio_type[64][4 + 1];
	char agio_amt[64][32 + 1];
	char agio_max_amt[64][32 + 1];
	char agio_min_amt[64][32 + 1];
	char agio_sts[64][4 + 1];
	char agio_memo[64][60 + 1];
}app_agio_rule_t;

typedef struct
{
	char    dime_num[8 + 1];
	char    agio_num[8 + 1];
}app_account_dime_t;

/*获取优惠规则请求信息*/
static int get_app_agio_rule_req_info(sw_loc_vars_t *vars, app_agio_rule_t * app_agio_rule,app_dime_tx_t *app_dime_tx,
		app_dime_info_t * app_dime_info,app_prdt_tx_t * app_prdt_tx,app_account_dime_t * app_account_dime) 
{
	int 	i = 0;
	int 	j = 0;
	char	buf[1024];
	char 	path[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Request.Eci_Agio.TX_CODE");
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_tx->tx_code, buf, sizeof(app_prdt_tx->tx_code) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Agios.TX_CODE]=[%s]=>[app_prdt_tx->tx_code]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x00, sizeof(buf));
	memset(path, 0x00, sizeof(path));
	sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.AGIO_NUM");
	loc_get_zd_data(vars,path,buf);
	strncpy(app_account_dime->agio_num, buf, sizeof(app_account_dime->agio_num) - 1);
	pub_log_debug("[%s][%d]get:[%s]=[%s]=>[app_account_dime->agio_num]", __FILE__, __LINE__,path,buf);

	memset(buf, 0x00, sizeof(buf));
	memset(path, 0x00, sizeof(path));
	sprintf(path, ".TradeRecord.Request.Eci_Agio.Dime_All_Infos.DIME_NUM");
	loc_get_zd_data(vars,path,buf);
	strncpy(app_account_dime->dime_num, buf, sizeof(app_account_dime->dime_num) - 1);
	pub_log_debug("[%s][%d]get:[%s]=[%s]=>[app_account_dime->dime_num]", __FILE__, __LINE__,path, buf);

	/*优惠规则请求信息设置*/
	for(i = 0; i < atoi(app_account_dime->agio_num); i++)
	{
		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.AGIO_NAME");
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_name[i], buf, sizeof(app_agio_rule->agio_name[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_name]", __FILE__, __LINE__, path, buf);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_TYPE", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_type[i], buf, sizeof(app_agio_rule->agio_type[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_type]", __FILE__, __LINE__, path, buf);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_AMT", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_amt[i], buf, sizeof(app_agio_rule->agio_amt[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_amt]", __FILE__, __LINE__, path, buf);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_MAX_AMT", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_max_amt[i], buf, sizeof(app_agio_rule->agio_max_amt[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_max_amt]", __FILE__, __LINE__, path, buf);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_MIN_AMT", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_min_amt[i], buf, sizeof(app_agio_rule->agio_min_amt[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_min_amt]", __FILE__, __LINE__, path, buf);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_STS", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_sts[i], buf, sizeof(app_agio_rule->agio_sts[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_sts]", __FILE__, __LINE__, path, buf);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_MEMO", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_agio_rule->agio_memo[i], buf, sizeof(app_agio_rule->agio_memo[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_agio_rule->agio_memo]", __FILE__, __LINE__, path, buf);
	}

	/*优惠维度规则请求设置*/
	for(j = 0; j < atoi(app_account_dime->dime_num); j++)
	{
		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DIME_NO", j);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_dime_info->dime_no[j], buf, sizeof(app_dime_info->dime_no[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[->dime_no[%d]]", __FILE__, __LINE__,path, buf, j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DIME_VAL", j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dime_val[j], buf, sizeof(app_dime_info->dime_val[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[->dime_val[%d]]", __FILE__, __LINE__,path, buf, j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DIME_NAME", j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dime_name[j], buf, sizeof(app_dime_info->dime_name[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[->dime_name[%d]]", __FILE__, __LINE__,path, buf, j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DR_DSP", j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dr_dsp[j], buf, sizeof(app_dime_info->dr_dsp[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[->dr_dsp[%d]]", __FILE__, __LINE__,path, buf,j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DT_STS", j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dt_sts[j], buf, sizeof(app_dime_info->dt_sts[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[->dt_sts[%d]]", __FILE__, __LINE__,path, buf,j);
	}
	return 0;
}

/*设置优惠规则应答信息*/
static int set_app_dime_tx_res_info(sw_loc_vars_t *vars, app_agio_rule_t *app_agio_rule,app_prdt_tx_t * app_prdt_tx,
		app_dime_tx_t * app_dime_tx, app_dime_info_t *app_dime_info, int agio_num, int dime_num)
{
	int 	i = 0;
	int 	j = 0;
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio.AGIO_NO");
	loc_set_zd_data(vars, path, app_agio_rule->agio_no);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_no);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio.TX_CODE");
	loc_set_zd_data(vars, path, app_prdt_tx->tx_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_tx->tx_code);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Agio.AGIO_NAME");
	loc_set_zd_data(vars, path, app_agio_rule->agio_name[0]);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_name);	

	pub_log_debug("[%s][%d] set=[%d]", __FILE__, __LINE__, agio_num);	
	for(i = 0; i < agio_num; i++)
	{
		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_SEQ", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_seq[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_agio_rule->agio_seq[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_TYPE", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_type[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_type[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_AMT", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_amt[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_amt[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_MAX_AMT", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_max_amt[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_max_amt[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_MIN_AMT", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_min_amt[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_min_amt[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_STS", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_sts[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_sts[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Eci_Agio_Rules.Eci_Agio_Rule(%d).AGIO_MEMO", i);
		loc_set_zd_data(vars, path, app_agio_rule->agio_memo[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_agio_rule->agio_memo[i]);

	}

	pub_log_debug("[%s][%d] set=[%d]", __FILE__, __LINE__, dime_num);	
	for(j = 0; j < dime_num; j++)
	{
		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DIME_NO", j);
		loc_set_zd_data(vars, path, app_dime_info->dime_no[j]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_no[j]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DIME_VAL", j);
		loc_set_zd_data(vars, path, app_dime_info->dime_val[j]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_val[j]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DR_DSP", j);
		loc_set_zd_data(vars, path, app_dime_info->dr_dsp[j]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dr_dsp[j]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Agio.Dime_All_Infos.Dime_All_Info(%d).DIME_NAME", j);
		loc_set_zd_data(vars, path, app_dime_info->dime_name[j]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_name[j]);
	}

	return 0;
}

/*获取优惠规则列表信息*/
static int get_app_agio_rule(app_agio_rule_t *app_agio_rule, app_prdt_tx_t * app_prdt_tx, char *agio_no, int * agio_num)
{
	int     i = 0;
	int     j = 0;
	int     cols = 0;
	int     rows = 0;
	char    sql[1024];
	char    *ptr = NULL;
	char    name[1024];

	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select distinct(a.agio_no),a.agio_seq,a.agio_name,a.agio_type,a.agio_amt,\
			a.agio_max_amt,a.agio_min_amt,a.agio_sts, a.agio_memo,  b.dime_val from app_agio_rule a, \
			app_dime_rate b,app_dime_info c where 1=1 and a.agio_no=b.rate_no and \
			a.agio_no='%s'and b.rate_type='1'and b.dime_no=c.dime_no and length(b.dime_val) > 3 ",
			agio_no);

	pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_mquery("app_agio_rule", sql, 5);
	if (cols < 0)
	{
		pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
		strcpy(errmsg, "查询数据库失败");
		strcpy(errcode, "S001");
		return -1;
	}

	rows = pub_db_mfetch("app_agio_rule");
	if (rows < 0)
	{
		pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
		strcpy(errmsg, "游标查询数据失败");
		strcpy(errcode, "S005");
		return -1;
	}
	if (rows == 0)
	{
		pub_db_cclose("app_agio_rule");
		pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
		strcpy(errmsg, "无数据");
		strcpy(errcode, "S004");
		return -2;
	}
	while(1)
	{
		for(i = 0; i < rows; i++)
		{
			for(j = 0; j < cols; j++)
			{
				memset(name, 0x0, sizeof(name));
				ptr = pub_db_get_data_and_name("app_agio_rule", i + 1, j + 1, name, sizeof(name));
				agt_str_tolower(name);
				pub_str_ziphlspace(ptr);
				pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

				if (strcmp(name, "agio_no") == 0)
				{
					strncpy(app_agio_rule->agio_no, ptr, sizeof(app_agio_rule->agio_no) - 1);
				}
				if (strcmp(name, "agio_seq") == 0)
				{
					strncpy(app_agio_rule->agio_seq[i], ptr, sizeof(app_agio_rule->agio_seq[i]) - 1);
				}
				if (strcmp(name, "agio_name") == 0)
				{
					strncpy(app_agio_rule->agio_name[0], ptr, sizeof(app_agio_rule->agio_name[0]) - 1);
				}
				if (strcmp(name, "agio_type") == 0)
				{
					strncpy(app_agio_rule->agio_type[i], ptr, sizeof(app_agio_rule->agio_type[i]) - 1);
				}
				if (strcmp(name, "agio_amt") == 0)
				{
					strncpy(app_agio_rule->agio_amt[i], ptr, sizeof(app_agio_rule->agio_amt[i]) - 1);
				}
				if (strcmp(name, "agio_max_amt") == 0)
				{
					strncpy(app_agio_rule->agio_max_amt[i], ptr, sizeof(app_agio_rule->agio_max_amt[i]) - 1);
				}
				if (strcmp(name, "agio_min_amt") == 0)
				{
					strncpy(app_agio_rule->agio_min_amt[i], ptr, sizeof(app_agio_rule->agio_min_amt[i]) - 1);
				}
				if (strcmp(name, "agio_sts") == 0)
				{
					strncpy(app_agio_rule->agio_sts[i], ptr, sizeof(app_agio_rule->agio_sts[i]) - 1);
				}
				if (strcmp(name, "agio_memo") == 0)
				{
					strncpy(app_agio_rule->agio_memo[i], ptr, sizeof(app_agio_rule->agio_memo[i]) - 1);
				}
				if (strcmp(name, "dime_val") == 0)
				{
					strncpy(app_prdt_tx->tx_code, ptr, sizeof(app_prdt_tx->tx_code) - 1);
				}
			}
		}
		rows = pub_db_mfetch("app_agio_rule");
		if (rows == 0)
		{
			pub_db_cclose("app_agio_rule");
			pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
			*agio_num = i;
			break;
		}
		else if (rows < 0)
		{
			pub_db_cclose("app_agio_rule");
			pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			strcpy(errcode, "S005");
			return -1;
		}
	}

	return 0;
}

/*获取优惠规则维度信息*/
static int get_app_dime_info(app_dime_info_t *app_dime_info, app_dime_tx_t * app_dime_tx, char *agio_no, int *dime_num)
{
	int     i = 0;
	int     j = 0;
	int     cols = 0;
	int     rows = 0;
	int 	ret = 0;
	char    *ptr = NULL;
	char    sql[1024];
	char    name[1024];
	char	sql2[1024];

	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select a.rate_no,b.dime_type, a.dime_no, b.dime_name from app_dime_rate a,app_dime_info b,app_dime_parm c \
			where a.rate_no='%s' and a.dime_no = c.dime_no and a.dime_val = c.parm_value and a.rate_type = '1'\
			and a.dime_no = b.dime_no and b.dime_fmt != '#txcode' \
			group by a.dime_no, a.rate_no, b.dime_name, b.dime_type", agio_no);


	pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_mquery("app_dime_rate", sql, 50);
	if (cols < 0)
	{
		pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
		strcpy(errmsg, "查询数据库失败");
		strcpy(errcode, "S001");
		return -1;
	}

	rows = pub_db_mfetch("app_dime_rate");
	if (rows < 0)
	{
		pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
		strcpy(errmsg, "游标查询数据失败");
		strcpy(errcode, "S005");
		return -1;
	}
	if (rows == 0)
	{
		pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
		strcpy(errmsg, "无数据");
		strcpy(errcode, "S004");
		return -2;
	}
	while(1)
	{
		for(i = 0; i < rows; i++)
		{
			for(j = 0; j < cols; j++)
			{
				memset(name, 0x0, sizeof(name));
				ptr = pub_db_get_data_and_name("app_dime_rate", i + 1, j + 1, name, sizeof(name));
				agt_str_tolower(name);
				pub_str_ziphlspace(ptr);
				pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

				if (strcmp(name, "dime_no") == 0)
				{
					strncpy(app_dime_info->dime_no[i], ptr, sizeof(app_dime_info->dime_no[i]) - 1);
				}

				if (strcmp(name, "dime_type") == 0)
				{
					if (ptr[0] == 'd' || ptr[0] == '$')
					{
						strncpy(app_dime_info->dr_dsp[i], app_dime_info->dime_val[i], sizeof(app_dime_info->dime_val[i]) - 1);
					}
				}
				if (strcmp(name, "dime_name") == 0)
				{
					strncpy(app_dime_info->dime_name[i], ptr, sizeof(app_dime_info->dime_name[i]) - 1);
				}

			}

			memset(sql2, 0x0, sizeof(sql2));
			snprintf(sql2, sizeof(sql2) - 1, "select distinct a.dime_val from app_dime_rate a,app_dime_info b,app_dime_parm c \
					where a.rate_no='%s' and a.dime_no='%s' and a.dime_no=c.dime_no and a.dime_val=c.parm_value \
					and a.rate_type='1' and a.dime_no=b.dime_no and b.dime_fmt!='#txcode'", 
					agio_no, app_dime_info->dime_no[i]);

			pub_log_debug("[%s][%d] sql2=[%s]", __FILE__, __LINE__, sql2);
			memset(app_dime_info->dime_val[i], 0x00, sizeof(app_dime_info->dime_val[i]));
			ret = agt_wm_concat(sql2, app_dime_info->dime_val[i]);
			if (ret)
			{
				pub_db_cclose("app_dime_rate");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				return -1;
			}

			memset(sql2, 0x0, sizeof(sql2));
			snprintf(sql2, sizeof(sql2)-1, "select distinct c.parm_dsp from app_dime_rate a,app_dime_info b,app_dime_parm c \
					where a.rate_no='%s' and a.dime_no='%s' and a.dime_no=c.dime_no and a.dime_val=c.parm_value \
					and a.rate_type='1' and a.dime_no=b.dime_no and b.dime_fmt!='#txcode'",
					agio_no, app_dime_info->dime_no[i]);

			pub_log_debug("[%s][%d] sql2=[%s]", __FILE__, __LINE__, sql2);
			memset(app_dime_info->dr_dsp[i], 0x00, sizeof(app_dime_info->dr_dsp[i]));
			ret = agt_wm_concat(sql2, app_dime_info->dr_dsp[i]);
			if (ret)
			{
				pub_db_cclose("app_dime_rate");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				return -1;
			}
		}

		rows = pub_db_mfetch("app_dime_rate");
		if (rows == 0)
		{
			pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
			*dime_num = i;
			break;
		}
		else if (rows < 0)
		{
			pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			strcpy(errcode, "S005");
			return -1;
		}
	}
	return 0;
}

/*构造优惠规则删除语句*/
static int crt_del_sql_1(char * agio_no, char *sql)
{
	char    buf[32];
	char    wherelist[1204];

	memset(wherelist, 0x0, sizeof(wherelist));
	if (agio_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and agio_no = '%s'", agio_no);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] agio_no 为空.", __FILE__, __LINE__);
		return -1;
	}

	sprintf(sql, "delete app_agio_rule where 1=1 %s",wherelist);
	pub_log_debug("[%s][%d]sql_1[%s]", __FILE__, __LINE__, sql);
	return 0;
}

/*构造优惠规则维度删除语句*/
static int crt_del_sql_3(char * agio_no, char *sql)
{
	char    buf[32];
	char    wherelist[1204];

	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");
	if (agio_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and rate_no = '%s'", agio_no);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] agio_no为空.", __FILE__, __LINE__);
		return -1;		
	}

	sprintf(sql, "delete app_dime_rate where %s and rate_type ='1'",wherelist);
	pub_log_debug("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);
	return 0;
}

/*登记优惠规则*/
static int insert_app_agio_rule(app_agio_rule_t *app_agio_rule,app_dime_tx_t * app_dime_tx,
		app_dime_info_t * app_dime_info, app_account_dime_t *app_account_dime)
{
	int 	j=0;
	int 	n=0;
	int 	ret = 0;
	char	sql[1024];
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(sql, 0x00, sizeof(sql));
	for(j = 0; j < atoi(app_account_dime->agio_num); j++)	
	{	
		memset(buf, 0x00, sizeof(buf));
		memset(cols, 0x00, sizeof(cols));
		memset(values, 0x00, sizeof(values));
		if(app_dime_tx->rate_no[0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_no");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_tx->rate_no);
			strcat(values, buf);
			strcat(values, ",");
		}
		else
		{
			pub_log_error("[%s][%d] 输入rate_no不能为空.", __FILE__, __LINE__);
			return -1;
		}

		sprintf(app_agio_rule->agio_seq[j], "%d", j + 1);
		if(app_agio_rule->agio_seq[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_seq");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_seq[j]);
			strcat(values, buf);
			strcat(values, ",");
		}

		if(app_agio_rule->agio_name[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_name");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_name[j]);
			strcat(values, buf);
			strcat(values, ",");
		}

		if(app_agio_rule->agio_type[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_type");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_type[j]);
			strcat(values, buf);
			strcat(values, ",");
		}

		if(app_agio_rule->agio_amt[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_amt");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_amt[j]);
			strcat(values, buf);
			strcat(values, ",");
		}

		if(app_agio_rule->agio_max_amt[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_max_amt");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_max_amt[j]);
			strcat(values, buf);
			strcat(values, ",");
		}

		if(app_agio_rule->agio_min_amt[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_min_amt");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_min_amt[j]);
			strcat(values, buf);
			strcat(values, ",");	
		}

		if(app_agio_rule->agio_sts[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_sts");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_agio_rule->agio_sts[j]);
			strcat(values, buf);
			strcat(values, ",");
		}

		if(cols[0] == '\0')
		{
			pub_log_error("[%s][%d]维度交易关系表字段列表为空！", __FILE__, __LINE__);
			return -1;
		}

		n = strlen(cols) - 1;
		if(cols[n] == ',')
		{
			cols[n] = '\0';
		}

		n = strlen(values) - 1;
		if(values[n] == ',')
		{
			values[n] = '\0';
		}

		sprintf(sql, "insert into app_agio_rule(%s) values(%s)", cols, values);
		pub_log_debug("[%s][%d]cols_1=[%s],values_1[%s]", __FILE__, __LINE__, cols,values);
		pub_log_debug("[%s][%d],sql[%s]", __FILE__, __LINE__,sql);

		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			return -1;
		}
	}

	return 0;
}

/*登记交易维度信息*/
static int insert_app_dime_tx(app_agio_rule_t *app_agio_rule,app_dime_tx_t * app_dime_tx,
		app_dime_info_t * app_dime_info,app_account_dime_t *app_account_dime,char * tx_code)
{
	int 	k=0;
	int 	n=0;
	int 	ret = 0;
	char	sql_3[1024];
	char	sql[1024];
	char    buf[1024];
	char	cols[1024];
	char	values[1024];

	for(k = 0; k < atoi(app_account_dime->dime_num); k++)
	{
		memset(sql, 0x0, sizeof(sql));
		sprintf(sql, "select * from app_dime_tx where tx_code = '%s' and dime_no = '%s' and rate_type = '1'", \
				tx_code, app_dime_info->dime_no[k]);

		pub_log_debug("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);	
		ret = pub_db_mquery("app_dime_tx", sql, 100);
		if(ret < 0)
		{
			pub_log_info("[%s][%d] 查询失败！", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			strcpy(errcode, "S001");
			return -1;
		}

		ret = pub_db_mfetch("app_dime_tx");
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败！\n", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			strcpy(errcode, "S005");
			return -1;
		}

		if(ret > 0)
		{
			continue;
		}	
		if(ret == 0)
		{
			memset(cols, 0x00, sizeof(cols));
			memset(buf, 0x00, sizeof(buf));
			memset(values, 0x00, sizeof(values));
			memset(sql_3, 0x00, sizeof(sql_3));
			if(app_dime_tx->rate_type[0] == '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "rate_type");
				strcat(cols, ",");
				strcpy(buf,"1");
				strcat(values, buf);
				strcat(values, ",");
			}

			if(app_dime_info->dime_no[k][0] != '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "dime_no");
				strcat(cols, ",");
				sprintf(buf, "'%s'", app_dime_info->dime_no[k]);
				strcat(values, buf);
				strcat(values, ",");
			}

			if(app_dime_info->dt_sts[k][0] != '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "dt_sts");
				strcat(cols, ",");
				sprintf(buf, "'%s'", app_dime_info->dt_sts[k]);
				strcat(values, buf);
				strcat(values, ",");
			}

			if(tx_code[0] != '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "tx_code");
				strcat(cols, ",");
				sprintf(buf, "'%s'",tx_code);
				strcat(values, buf);
				strcat(values, ",");
			}
			else
			{
				pub_log_error("[%s][%d] tx_code 为空！", __FILE__, __LINE__);
				return -1;
			}

			if(cols[0] == '\0')
			{
				pub_log_error("[%s][%d]维度交易关系表字段列表为空！", __FILE__, __LINE__);
				return -1;
			}

			n = strlen(cols) - 1;
			if(cols[n] == ',')
			{
				cols[n] = '\0';
			}

			n = strlen(values) - 1;
			if(values[n] == ',')
			{
				values[n] = '\0';
			}

			sprintf(sql_3, "insert into app_dime_tx(%s) values(%s)", cols, values);	
			pub_log_debug("[%s][%d]cols_3=[%s],values_3[%s]", __FILE__, __LINE__, cols,values);
			pub_log_debug("[%s][%d],sql_3[%s]", __FILE__, __LINE__,sql_3);
			ret = pub_db_nquery(sql_3);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] 执行插入语句失败! sql_3=[%s]", __FILE__, __LINE__, sql_3);
				strcpy(errmsg, "执行插入语句失败");
				strcpy(errcode, "S002");
				return -1;
			}
		}
	}

	return 0;
}

/*登记维度信息*/
static int insert_app_dime_rate(app_agio_rule_t *app_agio_rule,app_dime_tx_t * app_dime_tx,
		app_dime_info_t * app_dime_info,app_account_dime_t *app_account_dime,app_prdt_tx_t * app_prdt_tx)
{
	int 	k=0;
	int 	n=0;
	int 	ret = 0;
	char	*ptr = NULL;
	char	name[32];
	char	sql[1024];
	char    buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x00, sizeof(buf));
	memset(cols, 0x00, sizeof(cols));
	memset(values, 0x00, sizeof(values));
	for(k = 0; k < atoi(app_account_dime->dime_num); k++)
	{
		memset(cols, 0x00, sizeof(cols));
		memset(values, 0x00, sizeof(values));
		memset(buf, 0x00, sizeof(buf));
		memset(sql, 0x00, sizeof(sql));

		if(app_dime_tx->rate_type[0] == '\0')
		{
			strcat(cols, "rate_type");
			strcat(cols, ",");
			strcpy(buf,"1");
			strcat(values, buf);
			strcat(values, ",");
		}

		if(app_dime_tx->rate_no[0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "rate_no");	
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_tx->rate_no);
			strcat(values, buf);
			strcat(values, ",");
		}
		else
		{
			pub_log_error("[%s][%d] rate_no 为空!", __FILE__, __LINE__);
			return -1;
		}

		if(app_dime_info->dime_no[k][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "dime_no");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_info->dime_no[k]);
			strcat(values, buf);
			strcat(values, ",");
		}
		else
		{
			pub_log_error("[%s][%d] dime_no[%d] 为空!", __FILE__, __LINE__,k);
			return -1;
		}

		if(app_dime_info->dime_val[k][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "dime_val");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_info->dime_val[k]);
			strcat(values, buf);
			strcat(values, ",");
		}
		else
		{
			pub_log_error("[%s][%d] dime_val[%d] 为空!", __FILE__, __LINE__, k);
			return -1;
		}

		if(cols[0] == '\0')
		{
			pub_log_error("[%s][%d] 维度费率关系表字段列表为空！", __FILE__, __LINE__);
			return -1;
		}

		n = strlen(cols) - 1;
		if(cols[n] == ',')
		{
			cols[n] = '\0';
		}

		n = strlen(values) - 1;
		if(values[n] == ',')
		{
			values[n] = '\0';
		}

		sprintf(sql,"insert into app_dime_rate(%s) values(%s)", cols, values);
		pub_log_debug("[%s][%d]cols_2=[%s],values_2[%s]", __FILE__, __LINE__, cols,values);
		pub_log_debug("[%s][%d],sql[%s]", __FILE__, __LINE__,sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行插入语句失败");
			strcpy(errcode, "S002");
			return -1;
		}

	}

	/*登记维度信息之后?再次登记该笔交易的交易码 作为连接优惠维度表和维度信息表的桥梁(#txcode)*/
	memset(cols, 0x00, sizeof(cols));
	memset(values, 0x00, sizeof(values));
	memset(sql, 0x00, sizeof(sql));

	strncpy(sql, "select dime_no from app_dime_info where dime_fmt = '#txcode'", sizeof(sql) - 1);
	ret = pub_db_squery(sql);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 查询dime_no失败.", __FILE__, __LINE__);
		return -1;
	}

	memset(name, 0x00, sizeof(name) - 1);
	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	agt_str_tolower(name);
	pub_str_ziphlspace(ptr);	
	pub_log_debug("[%s][%d] ptr=[%s]", __FILE__, __LINE__, ptr);

	memset(sql, 0x00, sizeof(sql));
	strncpy(cols, "rate_type, rate_no, dime_no, dime_val", sizeof(cols) - 1);
	snprintf(values, sizeof(values), "1, '%s', '%s', '%s'", app_dime_tx->rate_no, ptr, app_prdt_tx->tx_code);
	sprintf(sql,"insert into app_dime_rate(%s) values(%s)", cols, values);

	pub_log_debug("[%s][%d],sql[%s]", __FILE__, __LINE__,sql);
	ret = pub_db_nquery(sql);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
		strcpy(errmsg, "执行插入语句失败");
		strcpy(errcode, "S002");
		return -1;
	}

	return 0;
}

/*检查数据唯一性*/
static int dime_check(app_account_dime_t *app_account_dime, app_dime_info_t *app_dime_info, app_prdt_tx_t *app_prdt_tx)
{
	int 	i =0;
	int 	num = 0;
	int 	cols = 0;
	int 	rows = 0;
	char 	sql[1024];
	char 	buf[1024];
	char 	name[64];
	char 	lft_val[32];
	char 	rght_val[32];
	char 	*ptr = NULL;

	num = atoi(app_account_dime->dime_num);
	if (num <= 0)
	{
		pub_log_error("[%s][%d] num  error.", __FILE__, __LINE__);
		return -1;
	}

	memset(sql, 0x0, sizeof(sql));
	memset(buf, 0x0, sizeof(buf));
	sprintf(sql, "select rate_no from app_dime_rate a, app_dime_info b where a.rate_type='1' and b.dime_fmt='#txcode' and a.dime_val='%s'",
			app_prdt_tx->tx_code);
	sprintf(buf, " %s intersect ", sql);
	while(i < num)
	{
		memset(sql, 0x0, sizeof(sql));
		sprintf(sql, "select dime_type from app_dime_info where dime_no='%s'", app_dime_info->dime_no[i]);
		cols = pub_db_squery(sql);
		if(cols < 0)
		{
			pub_log_error("[%s][%d]查询数据库失败", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			strcpy(errcode, "S001");
			return -1;
		}

		memset(name, 0x0, sizeof(name));
		ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
		agt_str_tolower(name);
		pub_str_ziphlspace(ptr);	
		pub_log_debug("[%s][%d]name[%s]", __FILE__, __LINE__, name);

		if (strcmp(name, "dime_type") == 0)
		{
			pub_log_debug("[%s][%d]ptr[%s]", __FILE__, __LINE__, ptr);
			if (ptr[0] == 'n' || ptr[0] == 'a')
			{
				/*整数和字符串可以直接比较**/
				memset(sql, 0x0, sizeof(sql));
				sprintf(sql, " select distinct rate_no from app_dime_rate where dime_no='%s' and dime_val='%s' and rate_type='1'", 
						app_dime_info->dime_no[i], app_dime_info->dime_val[i]);
			}
			else if (ptr[0] == 'd')
			{
				/*日期区间按xxx-xxx格式保存，需要做特殊处理**/
				memset(lft_val, 0x0, sizeof(lft_val));
				memset(rght_val, 0x0, sizeof(rght_val));
				sscanf(app_dime_info->dime_val[i], "%08s-%08s", lft_val, rght_val);
				memset(sql, 0x0, sizeof(sql));
				pub_log_info("[%s][%d] lft_val=%s, rght_val=%s", __FILE__, __LINE__, lft_val, rght_val);
				sprintf(sql, " select distinct rate_no from app_dime_rate where substr(dime_val, 1,8) <='%s' \
						and substr(dime_val, 10, 17) >='%s' and  dime_no='%s' and rate_type='1'", 
						lft_val,rght_val,app_dime_info->dime_no[i]);
			}
			else if (ptr[0] == '$')
			{
				/*金额区间按xxx-xxx格式保存，需要做特殊处理,暂未处理**/
				memset(lft_val, 0x0, sizeof(lft_val));
				memset(rght_val, 0x0, sizeof(rght_val));
				sscanf(app_dime_info->dime_val[i], "%[^-]-%s", lft_val, rght_val);
				pub_log_info("[%s][%d] lft_val=%s, rght_val=%s", __FILE__, __LINE__, lft_val, rght_val);
				sprintf(sql, " select distinct rate_no from app_dime_rate where substr(dime_val, 1, 12) <='%012.02lf' \
						and substr(dime_val,14,25) >='%012.02lf' and  dime_no='%s' and rate_type='1'",
						atof(lft_val),atof(rght_val),app_dime_info->dime_no[i]);
				memset(app_dime_info->dime_val[i], 0x0, sizeof(app_dime_info->dime_val[i]));
				snprintf(app_dime_info->dime_val[i], sizeof(app_dime_info->dime_val[i])-1, "%012.02lf-%012.02lf", atof(lft_val), atof(rght_val));
			}
			else
			{
				pub_log_error("[%s][%d] 暂时不支持[%s]类型", __FILE__, __LINE__, ptr);
				return -1;
			}
		}
		else
		{
			pub_log_error("[%s][%d] name[%s]错误.", __FILE__, __LINE__, name);
			return -1;
		}

		if (i != num -1)
		{
			sprintf(buf + strlen(buf), " %s intersect", sql);
		}
		else
		{
			sprintf(buf + strlen(buf), " %s", sql);
		}
		i ++;	
	}

	pub_log_debug("[%s][%d]sql[%s]", __FILE__, __LINE__, buf);
	rows = pub_db_get_fetrows(buf);
	if (rows < 0)
	{
		pub_log_error("[%s][%d] get cnt error.", __FILE__, __LINE__);
		return -1;
	}
	else if (rows >= 1)
	{
		pub_log_debug("[%s][%d]不符合唯一性条件rows=[%d]cols[%d]", __FILE__, __LINE__, rows,cols);
		return -2;
	}

	return 0;
}

int sp2111(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	int     dime_num = 0;
	int     agio_num = 0;
	char	option[8];
	char	agio_sum[256];
	char	sql_1[1024];
	char	sql_2[1024];
	char	sql_3[1024];
	char	sql_4[1024];
	char    tx_code[9];
	char	agio_no[10];
	char	agio_seq[10];
	app_prdt_tx_t	app_prdt_tx;
	app_dime_tx_t	app_dime_tx;
	app_agio_rule_t	app_agio_rule;
	app_dime_info_t	app_dime_info;
	app_account_dime_t	app_account_dime;

	ret = agt_table_detect("app_agio_rule, app_dime_info, app_prdt_tx, app_dime_tx");
	if (ret < 0)
	{
		strcpy(errmsg, "表不存在");
		pub_log_info("[%s][%d] app_agio_rule, app_dime_info, app_prdt_tx, app_dime_tx有不存在的表", __FILE__, __LINE__);
		goto ErrExit;
	}

	memset(&app_dime_info, 0x00, sizeof(app_dime_info));
	memset(&app_dime_tx, 0x00, sizeof(app_dime_tx));
	memset(&app_prdt_tx, 0x0, sizeof(app_prdt_tx));
	memset(&app_agio_rule, 0x0, sizeof(app_agio_rule));
	memset(&app_account_dime, 0x0, sizeof(app_account_dime));

	memset(agio_sum, 0x0, sizeof(agio_sum));
	memset(tx_code, 0x00, sizeof(tx_code));
	memset(agio_no, 0x00, sizeof(agio_no));
	memset(agio_seq, 0x00, sizeof(agio_seq));
	memset(sql_1, 0x00, sizeof(sql_1));
	memset(sql_2, 0x00, sizeof(sql_2));
	memset(sql_3, 0x00, sizeof(sql_3));
	memset(sql_4, 0x00, sizeof(sql_4));

	memset(option, 0x0, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);
	pub_log_debug("[%s][%d]option[%s]", __FILE__, __LINE__, option);

	memset(tx_code, 0x0, sizeof(tx_code));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio.TX_CODE", tx_code);
	pub_log_debug("[%s][%d]tx_code[%s]", __FILE__, __LINE__, tx_code);

	memset(agio_no, 0x0, sizeof(agio_no));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio.AGIO_NO", agio_no);
	pub_log_debug("[%s][%d]agio_no[%s]", __FILE__, __LINE__, agio_no);

	memset(agio_seq, 0x0, sizeof(agio_seq));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Agio.AGIO_SEQ", agio_seq);
	pub_log_debug("[%s][%d]agio_seq[%s]", __FILE__, __LINE__, agio_seq);

	if (option[0] == 'A')
	{
		get_app_agio_rule_req_info(vars, &app_agio_rule, &app_dime_tx, &app_dime_info, &app_prdt_tx, &app_account_dime);
		ret = dime_check(&app_account_dime, &app_dime_info, &app_prdt_tx);
		if (ret < 0)
		{
			if (ret == -2)
			{
				pub_log_error("[%s][%d]唯一性查询失败", __FILE__, __LINE__);	
				memset(errmsg, 0x0, sizeof(errmsg));
				sprintf(errmsg, "已存在满足条件的记录");
				goto ErrExit;
			}

			pub_log_error("[%s][%d] dime_check失败.", __FILE__, __LINE__);
			sprintf(errmsg, "唯一性检查失败");
			goto ErrExit;
		}

		ret = agt_creat_code("rate_no_sequence", app_dime_tx.rate_no);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 生成编码失败！", __FILE__, __LINE__);
			sprintf(errmsg, "生成编码失败");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d] app_dime_tx.rate_no[%s]", __FILE__, __LINE__,app_dime_tx.rate_no);
		ret = insert_app_agio_rule(&app_agio_rule, &app_dime_tx, &app_dime_info, &app_account_dime);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}

		ret = insert_app_dime_rate(&app_agio_rule, &app_dime_tx, &app_dime_info, &app_account_dime, &app_prdt_tx);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}

		ret = insert_app_dime_tx(&app_agio_rule, &app_dime_tx, &app_dime_info, &app_account_dime, tx_code);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}

		goto OkExit;
	}
	else if (option[0] == 'M')
	{
		/*修改时，由于只能修改优惠规则，优惠规则中无唯一索引,则只能先删除在登记最新信息*/
		pub_log_debug("[%s][%d]agio_no[%s]", __FILE__, __LINE__, agio_no);
		get_app_agio_rule_req_info(vars, &app_agio_rule, &app_dime_tx, &app_dime_info, &app_prdt_tx, &app_account_dime);
		ret = crt_del_sql_1(agio_no, sql_1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 创建删除app_agio_rule语句失败.", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = pub_db_nquery(sql_1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行删除语句失败! sql_1=[%s]", __FILE__, __LINE__, sql_1);
			strcpy(errmsg, "删除失败");
			goto ErrExit;
		}

		strncpy(app_dime_tx.rate_no, agio_no, sizeof(agio_no) - 1);	
		ret = insert_app_agio_rule(&app_agio_rule, &app_dime_tx, &app_dime_info, &app_account_dime);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}

		goto OkExit;
	}
	else if (option[0] == 'D')
	{
		ret = crt_del_sql_1(agio_no, sql_1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 创建删除app_agio_rule语句失败.", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = pub_db_nquery(sql_1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行删除语句失败! sql_1=[%s]", __FILE__, __LINE__, sql_1);
			strcpy(errmsg, "删除失败");	
			goto ErrExit;
		}

		ret = crt_del_sql_3(agio_no, sql_3);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 创建删除app_agio_rule语句失败.", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = pub_db_nquery(sql_3);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行删除语句失败! sql_3=[%s]", __FILE__, __LINE__, sql_3);
			strcpy(errmsg, "删除失败");	
			goto ErrExit;
		}
		goto OkExit;
	}
	else if (option[0] == 'S')
	{
		get_app_agio_rule(&app_agio_rule, &app_prdt_tx, agio_no, &agio_num);
		ret = get_app_dime_info(&app_dime_info, &app_dime_tx, agio_no, &dime_num);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 查询失败", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");	
			return -1;
		}

		set_app_dime_tx_res_info(vars,&app_agio_rule, &app_prdt_tx, &app_dime_tx, &app_dime_info, agio_num, dime_num);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}
	else
	{
		pub_log_error("[%s][%d] 输入操作类型[%s]错误.", __FILE__, __LINE__);
		goto ErrExit;
	}


OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2111 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2111 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
