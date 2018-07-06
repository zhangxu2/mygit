/*************************************************
 *  文 件 名:  flw_sp2105.c                        **
 *  功能描述:  收费的增删改                        **
 *  作    者:  赵强                                **
 *  完成日期:  20160801                            **
 *  *************************************************/
#include "agent_comm.h"
#include "pub_db.h"
static	char 	errmsg[128];
static	char	errcode[8];

typedef struct
{
	char 	tx_code[32 + 1];
	char 	dime_no[32 + 1];

}app_dime_tx_t;
typedef struct
{
	char    chrg_no[16][6 + 1];
	char	chrg_name[16][60 + 1];
	char    chrg_seq[16][4 + 1];
	char    chrg_type[16][1 + 1];
	char    chrg_rate[16][128 + 1];
	char    chrg_min_amt[16][128 + 1];
	char    chrg_max_amt[16][128 + 1];
	char    agio_mode[16][1 + 1];
	char    chrg_sts[16][1 + 1];
	char    memo[16][64 + 1];
	char 	agio_flg[16][1 + 1];
}app_chrg_rule_t;
typedef struct
{
	char 	dt_start_date[10 + 1];
	char 	dt_end_date[10 + 1];
	char    dime_name[64][60 + 1];
	char    dime_val[64][32 + 1];
	char    dime_no[64][32 + 1];
	char    dr_dsp[64][64 + 1];
	char    dt_sts[64][1 + 1];
	char 	rate_no[32 + 1];
	char 	rate_type[32 + 1];
}app_dime_info_t;
typedef struct
{
	char    tx_code[4 + 1];
	char    tx_name[60 + 1];
}app_prdt_tx_t;
typedef struct 
{
	char	dime_num[2][8 + 1];
	char	chrg_num[2][8 + 1];
	char	account_num[8 + 1];
}app_account_dime_t;

static int get_app_chrg_rule_req_info(sw_loc_vars_t *vars, app_chrg_rule_t *app_chrg_rule,app_account_dime_t *app_account_dime,app_dime_info_t *app_dime_info,int k)
{
	int		i = 0;
	int		j = 0;
	char	buf[1024];
	char	path[1024];

	memset(buf, 0x00, sizeof(buf));
	memset(path, 0x00, sizeof(path));
	sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.CHRG_NUM", k);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_account_dime->chrg_num[k], buf, sizeof(app_account_dime->chrg_num[k]) - 1);
	pub_log_debug("[%s][%d]get:[Eci_Account(%d).Eci_Chrg_Rules.CHRG_NUM]=[%s]=>[app_account_dime->chrg_num]", __FILE__, __LINE__, k, buf);

	memset(buf, 0x00, sizeof(buf));
	memset(path, 0x00, sizeof(path));
	sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.DIME_NUM", k);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_account_dime->dime_num[k], buf, sizeof(app_account_dime->dime_num[k]) - 1);
	pub_log_debug("[%s][%d]get:[Eci_Account(%d).Dime_All_Infos.DIME_NUM]=[%s]=>[app_account_dime->dime_num]", __FILE__, __LINE__, k, buf);

	for(i = 0;i < atoi(app_account_dime->chrg_num[k]); i++)
	{

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_NO", k, i);
		loc_get_zd_data(vars,path,buf);
		strncpy(app_chrg_rule->chrg_no[i], buf, sizeof(app_chrg_rule->chrg_no[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_no[%d]]",__FILE__, __LINE__, path,buf,i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_NAME", k, i);
		loc_get_zd_data(vars,path,buf);
		strncpy(app_chrg_rule->chrg_name[i], buf, sizeof(app_chrg_rule->chrg_name[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_name[%d]]",__FILE__, __LINE__, path,buf,i);	

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_SEQ", k, i);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_chrg_rule->chrg_seq[i], buf, sizeof(app_chrg_rule->chrg_seq[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_seq[%d]]",__FILE__, __LINE__,path, buf, i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_TYPE", k, i);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_chrg_rule->chrg_type[i], buf, sizeof(app_chrg_rule->chrg_type[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_type[%d]]",__FILE__, __LINE__,path, buf, i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_RATE",k,i);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_chrg_rule->chrg_rate[i], buf, sizeof(app_chrg_rule->chrg_rate[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_rate[%d]]",__FILE__, __LINE__,path, buf,i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_MIN_AMT",k,i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_chrg_rule->chrg_min_amt[i], buf, sizeof(app_chrg_rule->chrg_min_amt[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_min_amt[%d]]",__FILE__, __LINE__,path, buf,i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_MAX_AMT",k,i);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_chrg_rule->chrg_max_amt[i], buf, sizeof(app_chrg_rule->chrg_max_amt[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_max_amt[%d]]",__FILE__, __LINE__,path, buf,i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).AGIO_MODE",k,i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_chrg_rule->agio_mode[i], buf, sizeof(app_chrg_rule->agio_mode[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->agio_mode[%d]]",__FILE__, __LINE__, path,buf,i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_STS",k,i);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_chrg_rule->chrg_sts[i], buf, sizeof(app_chrg_rule->chrg_sts[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->chrg_sts[%d]]",__FILE__, __LINE__,path, buf,i);

		memset(buf, 0x0, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).MEMO",k,i);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_chrg_rule->memo[i], buf, sizeof(app_chrg_rule->memo[i]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->memo[%d]]",__FILE__, __LINE__,path, buf,i);
	}

	for(j = 0;j < atoi(app_account_dime->dime_num[k]); j++)
	{
		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DIME_NO",k,j);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_dime_info->dime_no[j], buf, sizeof(app_dime_info->dime_no[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_dime_info->dime_no[%d]]", __FILE__, __LINE__,path, buf, j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DIME_VAL",k,j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dime_val[j], buf, sizeof(app_dime_info->dime_val[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_dime_info->dime_val[%d]]", __FILE__, __LINE__,path, buf, j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DT_STS",k,j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dt_sts[j], buf, sizeof(app_dime_info->dt_sts[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[app_chrg_rule->dt_sts[%d]]", __FILE__, __LINE__,path, buf,j);

		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DIME_NAME",k,j);
		loc_get_zd_data(vars,path, buf);
		strncpy(app_dime_info->dime_name[j], buf, sizeof(app_dime_info->dime_name[j]) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[->dime_name[%d]]", __FILE__, __LINE__,path, buf, j);
	}
	return 0;
}
static int insert_app_chrg_rule(app_chrg_rule_t *app_chrg_rule, app_dime_info_t * app_dime_info, app_account_dime_t *app_account_dime, int i)
{
	int		j=0;
	int		n=0;
	int		ret = 0;
	char    sql_1[1024];
	char	buf[1024];
	char	cols[1024];
	char 	values[1024];

	for(j = 0;j < atoi(app_account_dime->chrg_num[i]); j++ )	
	{	
		memset(buf, 0x00, sizeof(buf));
		memset(cols, 0x00, sizeof(cols));
		memset(values, 0x00, sizeof(values));
		memset(sql_1, 0x00, sizeof(sql_1));
		if(app_dime_info->rate_no[0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_no");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_info->rate_no);
			strcat(values, buf);
			strcat(values, ",");
		}
		if(app_chrg_rule->chrg_name[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_name");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_name[j]);
			strcat(values, buf);
			strcat(values, ",");
		}
		sprintf(app_chrg_rule->chrg_seq[j], "%d", j + 1);
		if(app_chrg_rule->chrg_seq[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_seq");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_seq[j]);
			strcat(values, buf);
			strcat(values, ",");
		}
		if(app_chrg_rule->chrg_type[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_type");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_type[j]);
			strcat(values, buf);
			strcat(values, ",");
		}
		if(app_chrg_rule->chrg_rate[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_rate");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_rate[j]);
			strcat(values, buf);
			strcat(values, ",");
		}
		if(app_chrg_rule->chrg_max_amt[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_max_amt");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_max_amt[j]);
			strcat(values, buf);
			strcat(values, ",");
		}
		if(app_chrg_rule->chrg_min_amt[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_min_amt");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_min_amt[j]);
			strcat(values, buf);
			strcat(values, ",");	
		}
		if(app_chrg_rule->agio_mode[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "agio_mode");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->agio_mode[j]);
			strcat(values, buf);
			strcat(values, ",");	
		}

		if(app_chrg_rule->chrg_sts[j][0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "chrg_sts");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_chrg_rule->chrg_sts[j]);
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
		sprintf(sql_1, "insert into app_chrg_rule(%s) values(%s)", cols, values);
		pub_log_debug("[%s][%d]cols_1=[%s],values_1[%s]", __FILE__, __LINE__, cols,values);
		pub_log_info("[%s][%d],sql_1[%s]", __FILE__, __LINE__,sql_1);

		ret = pub_db_nquery(sql_1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql_1=[%s]", __FILE__, __LINE__, sql_1);
			return -1;
		}
	}
	return 0;
}
static int insert_app_dime_tx(app_chrg_rule_t *app_chrg_rule, app_dime_info_t * app_dime_info, app_account_dime_t *app_account_dime,char * tx_code, int i)
{
	int 	k=0;
	int 	n=0;
	int 	ret = 0;
	char    buf[1024];
	char 	cols[1024];
	char 	values[1024];
	char	sql[1024];
	char    sql_3[1024];

	for(k = 0;k < atoi(app_account_dime->dime_num[i]); k++)
	{

		memset(sql, 0x0, sizeof(sql));
		sprintf(sql, "select * from app_dime_tx where tx_code = '%s' and dime_no = '%s' and rate_type = '0'",
				tx_code, app_dime_info->dime_no[k]);
		pub_log_info("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_mquery("app_dime_tx", sql, 100);
		if(ret < 0)
		{
			pub_log_debug("[%s][%d] 查询失败！", __FILE__, __LINE__);
			return -1;
		}
		ret = pub_db_mfetch("app_dime_tx");
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败！\n", __FILE__, __LINE__);
			return -1;
		}
		if(ret > 0)
		{
			continue;
		}
		if(ret == 0)
		{		

			pub_db_cclose("app_dime_tx");
			memset(cols, 0x00, sizeof(cols));
			memset(buf, 0x00, sizeof(buf));
			memset(values, 0x00, sizeof(values));
			memset(sql_3, 0x00, sizeof(sql_3));
			if(app_dime_info->rate_type[0] != '\0')
			{
				strcat(cols, "rate_type");
				strcat(cols, ",");
				strcpy(buf,"0");
				strcat(values, buf);
				strcat(values, ",");
			}
			else if(app_dime_info->rate_type[0] == '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "rate_type");
				strcat(cols, ",");
				strcpy(buf,"0");
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
			if(app_dime_info->dt_start_date[0] != '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "dt_start_date");
				strcat(cols, ",");
				sprintf(buf, "'%s'", app_dime_info->dt_start_date);
				strcat(values, buf);
				strcat(values, ",");
			}
			if(app_dime_info->dt_end_date[0] != '\0')
			{
				memset(buf, 0x00, sizeof(buf));
				strcat(cols, "dt_end_date");
				strcat(cols, ",");
				sprintf(buf, "'%s'", app_dime_info->dt_end_date);
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
				return -1;
			}
		}
	}

	return 0;
}
static int insert_app_dime_rate(app_chrg_rule_t *app_chrg_rule, app_dime_info_t * app_dime_info, app_account_dime_t *app_account_dime, int i)
{
	int 	k=0;
	int 	n=0;
	int 	ret = 0;
	char    buf[1024];
	char 	cols[1024];
	char 	values[1024];
	char    sql_2[1024];

	memset(buf, 0x00, sizeof(buf));
	memset(cols, 0x00, sizeof(cols));
	memset(values, 0x00, sizeof(values));
	memset(sql_2, 0x00, sizeof(sql_2));
	for(k = 0;k < atoi(app_account_dime->dime_num[i]); k++)
	{
		memset(cols, 0x00, sizeof(cols));
		memset(values, 0x00, sizeof(values));
		memset(buf, 0x00, sizeof(buf));

		if(app_dime_info->rate_type[0] != '\0')
		{
			strcat(cols, "rate_type");
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_info->rate_type);
			strcat(values, buf);
			strcat(values, ",");
		}
		else if(app_dime_info->rate_type[0] == '\0')
		{
			strcat(cols, "rate_type");
			strcat(cols, ",");
			strcpy(buf,"0");
			strcat(values, buf);
			strcat(values, ",");
		}
		if(app_dime_info->rate_no[0] != '\0')
		{
			memset(buf, 0x00, sizeof(buf));
			strcat(cols, "rate_no");	
			strcat(cols, ",");
			sprintf(buf, "'%s'", app_dime_info->rate_no);
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
			pub_log_error("[%s][%d] dime_no[%d] 为空!", __FILE__, __LINE__, i);
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
			pub_log_error("[%s][%d] dime_val[%d] 为空!", __FILE__, __LINE__, i);
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
		pub_log_debug("[%s][%d]cols=[%s],values[%s]", __FILE__, __LINE__, cols,values);

		sprintf(sql_2,"insert into app_dime_rate(%s) values(%s)", cols, values);
		pub_log_debug("[%s][%d]cols_2=[%s],values_2[%s]", __FILE__, __LINE__, cols,values);
		pub_log_debug("[%s][%d],sql_2[%s]", __FILE__, __LINE__,sql_2);
		ret = pub_db_nquery(sql_2);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql_1=[%s]", __FILE__, __LINE__, sql_2);
			return -1;
		}
		ret = pub_db_commit();
		if (ret)
		{
			pub_log_error("[%s][%d] 提交数据库失败!", __FILE__, __LINE__);
			return -1;
		}
	}
	return 0;
}
static int crt_del_sql_1(app_dime_info_t *app_dime_info, char *sql)
{
	char    buf[32];
	char    wherelist[1204];

	memset(wherelist, 0x0, sizeof(wherelist));
	if (app_dime_info->rate_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " chrg_no = '%s'", app_dime_info->rate_no);
		strcat(wherelist, buf);
	}
	sprintf(sql, "delete app_chrg_rule where %s",wherelist);

	pub_log_info("[%s][%d]sql_1[%s]", __FILE__, __LINE__, sql);
	return 0;
}
static int crt_del_sql_2(char * tx_code, char *sql)
{
	char    buf[32];
	char    wherelist[1204];

	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");

	if (tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_code = '%s'",tx_code);
		strcat(wherelist, buf);
	}
	sprintf(sql, "delete app_dime_tx where %s and rate_type = '0'",wherelist);

	pub_log_info("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);
	return 0;
}
static int crt_del_sql_3(app_dime_info_t *app_dime_info, char *sql)
{
	char    buf[32];
	char    wherelist[1204];

	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");
	if (app_dime_info->rate_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and rate_no = '%s'", app_dime_info->rate_no);
		strcat(wherelist, buf);
	}

	sprintf(sql, "delete app_dime_rate where %s and rate_type ='0'",wherelist);

	pub_log_info("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);
	return 0;
}


static int set_app_chrg_rule_res_info(sw_loc_vars_t *vars, app_chrg_rule_t * app_chrg_rule, app_dime_info_t *app_dime_info,app_prdt_tx_t * app_prdt_tx,int chrg_num,int dime_num,int index)
{
	int 	ret = -1;
	int  	i = 0;
	char	path[256];
	char 	name[64];
	char 	code_desc[256];
	char 	sql[1024];
	char 	*ptr = NULL;
	char	buf[1024];
	char	wherelist[1024];
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).TX_CODE",index);
	loc_set_zd_data(vars, path, app_prdt_tx->tx_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_prdt_tx->tx_code);

	memset(sql, 0x0, sizeof(sql));
	memset(code_desc, 0x0, sizeof(code_desc));
	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	sprintf(sql, "select tx_name from app_prdt_tx where 1=1 and tx_code = '%s'",app_prdt_tx->tx_code);
	pub_log_info("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
	ret = pub_db_squery(sql);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]查询数据库失败", __FILE__, __LINE__);
		return -1;
	} 		
	else if(ret == 0)
	{
		pub_log_error("[%s][%d] 无数据", __FILE__, __LINE__);
		return -1;
	}
	memset(name, 0x0, sizeof(name));
	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	pub_str_rtrim(ptr);
	strcpy(code_desc, ptr);
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).TX_NAME",index);
	loc_set_zd_data(vars, path, code_desc);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, code_desc);

	pub_log_debug("[%s][%d]chrg_num[%d]", __FILE__, __LINE__, chrg_num);
	while(i < chrg_num)
	{
		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.CHRG_NUM", index);
		loc_set_zd_int(vars, path, chrg_num);
		pub_log_debug("[%s][%d] set:[%s]=[%d]", __FILE__, __LINE__, path, chrg_num);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_NO", index, i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_no[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_no[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_NAME", index, i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_name[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_name[i]);


		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_SEQ", index, i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_seq[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_seq[i]);	

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_TYPE", index, i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_type[i]);          
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_type[i]);

		memset(path, 0x0, sizeof(path));
		memset(code_desc,0x0,sizeof(code_desc));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_TYPE_DESC", index,i);
		set_app_data_dic_trans("chrg_type",app_chrg_rule->chrg_type[i], code_desc);
		loc_set_zd_data(vars, path, code_desc);
		pub_log_debug("[%s][%d] set:[%s]=[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_type[i], code_desc);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_RATE", index, i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_rate[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path,app_chrg_rule->chrg_rate[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_MIN_AMT", index, i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_min_amt[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_min_amt[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_MAX_AMT", index,i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_max_amt[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_max_amt[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).AGIO_MODE", index,i);
		loc_set_zd_data(vars, path, app_chrg_rule->agio_mode[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->agio_mode[i]);

		memset(path, 0x0, sizeof(path));
		memset(code_desc,0x0,sizeof(code_desc));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).AGIO_MODE_DESC", index,i);
		set_app_data_dic_trans("agio_mode",app_chrg_rule->agio_mode[i], code_desc);
		loc_set_zd_data(vars, path, code_desc);
		pub_log_debug("[%s][%d] set:[%s]=[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->agio_mode[i], code_desc);


		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).CHRG_STS", index,i);
		loc_set_zd_data(vars, path, app_chrg_rule->chrg_sts[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->chrg_sts[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).MEMO", index,i);
		loc_set_zd_data(vars, path, app_chrg_rule->memo[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->memo[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Eci_Chrg_Rules.Eci_Chrg_Rule(%d).AGIO_FLG", index,i);
		loc_set_zd_data(vars, path, app_chrg_rule->agio_flg[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_chrg_rule->agio_flg[i]);

		i++;
	}
	i=0;
	pub_log_debug("[%s][%d]dime_num[%d]", __FILE__, __LINE__, dime_num);
	while(i < dime_num)
	{
		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.DIME_NUM", index);
		loc_set_zd_int(vars, path, dime_num);
		pub_log_debug("[%s][%d] set:[%s]=[%d]", __FILE__, __LINE__, path, dime_num);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DIME_NO", index, i);
		loc_set_zd_data(vars, path, app_dime_info->dime_no[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_no[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DIME_VAL", index, i);
		loc_set_zd_data(vars, path, app_dime_info->dime_val[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_val[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DR_DSP", index, i);
		loc_set_zd_data(vars, path, app_dime_info->dr_dsp[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dr_dsp[i]);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Accounts.Eci_Account(%d).Dime_All_Infos.Dime_All_Info(%d).DIME_NAME", index, i);
		loc_set_zd_data(vars, path, app_dime_info->dime_name[i]);
		pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_name[i]);

		i++;
	}
	return 0;
}
static int crt_sel_sql(char *tx_code, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, "1 = 1");

	if (tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_val = '%s'",tx_code);
		strcat(wherelist, buf);
	}
	sprintf(sql, "select distinct(dime_val),rate_no from app_dime_rate a,app_dime_info b \
			where %s and a.rate_type='0' and a.dime_no = b.dime_no and b.dime_fmt ='#txcode' order by dime_val, rate_no",wherelist);
	pub_log_info("[%s][%d] sql = [%s] ", __FILE__, __LINE__, sql);
	return 0;
}
static int get_app_chrg_rule(app_chrg_rule_t *app_chrg_rule, char *chrg_no,int *chrg_num)
{

	int     cols = 0;
	int     rows = 0;
	int     i = 0;
	int     j = 0;
	char    sql[1024];
	char    *ptr = NULL;
	char    name[1024];

	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select * from app_chrg_rule where 1=1 and chrg_no='%s'",chrg_no);
	pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_mquery("app_chrg_rule", sql, 5);
	if (cols < 0)
	{
		pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
		return -1;
	}
	rows = pub_db_mfetch("app_chrg_rule");
	if (rows < 0)
	{
		pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
		return -1;
	}
	if (rows == 0)
	{
		pub_db_cclose("app_chrg_rule");
		pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
		return -2;
	}
	while(1)
	{
		for(i = 0; i < rows; i++)
		{
			for(j = 0; j < cols; j++)
			{
				memset(name, 0x0, sizeof(name));
				ptr = pub_db_get_data_and_name("app_chrg_rule", i + 1, j + 1, name, sizeof(name));
				agt_str_tolower(name);
				pub_str_ziphlspace(ptr);
				pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
				if (strcmp(name, "chrg_name") == 0)
				{
					strncpy(app_chrg_rule->chrg_name[i], ptr, sizeof(app_chrg_rule->chrg_name[i]) - 1);
				}
				if (strcmp(name, "chrg_no") == 0)
				{
					strncpy(app_chrg_rule->chrg_no[i], ptr, sizeof(app_chrg_rule->chrg_no[i]) - 1);
				}

				if (strcmp(name, "chrg_seq") == 0)
				{
					strncpy(app_chrg_rule->chrg_seq[i], ptr, sizeof(app_chrg_rule->chrg_seq[i]) - 1);
				}

				if (strcmp(name, "chrg_type") == 0)
				{
					strncpy(app_chrg_rule->chrg_type[i], ptr, sizeof(app_chrg_rule->chrg_type[i]) - 1);
				}

				if (strcmp(name, "chrg_rate") == 0)
				{
					strncpy(app_chrg_rule->chrg_rate[i], ptr, sizeof(app_chrg_rule->chrg_rate[i]) - 1);
				}

				if (strcmp(name, "chrg_min_amt") == 0)
				{
					strncpy(app_chrg_rule->chrg_min_amt[i], ptr, sizeof(app_chrg_rule->chrg_min_amt[i]) - 1);
				}

				if (strcmp(name, "chrg_max_amt") == 0)
				{
					strncpy(app_chrg_rule->chrg_max_amt[i], ptr, sizeof(app_chrg_rule->chrg_max_amt[i]) - 1);
				}

				if (strcmp(name, "agio_mode") == 0)
				{
					strncpy(app_chrg_rule->agio_mode[i], ptr, sizeof(app_chrg_rule->agio_mode[i]) - 1);
				}
				if (strcmp(name, "chrg_sts") == 0)
				{
					strncpy(app_chrg_rule->chrg_sts[i], ptr, sizeof(app_chrg_rule->chrg_sts[i]) - 1);
				}
				if (strcmp(name, "memo") == 0)
				{
					strncpy(app_chrg_rule->memo[i], ptr, sizeof(app_chrg_rule->memo[i]) - 1);
				}
				if (strcmp(name, "agio_flg") == 0)
				{
					strncpy(app_chrg_rule->agio_flg[i], ptr, sizeof(app_chrg_rule->agio_flg[i]) - 1);
				}
			}
		}
		rows = pub_db_mfetch("app_chrg_rule");
		if (rows == 0)
		{
			pub_db_cclose("app_chrg_rule");
			pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
			*chrg_num = i;
			break;
		}
		else if (rows < 0)
		{
			pub_db_cclose("app_chrg_rule");
			pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
			return -1;
		}
	}
	return 0;
}
static int get_app_dime_info(app_dime_info_t *app_dime_info,app_dime_tx_t * app_dime_tx, char *chrg_no, int *dime_num)
{
	int     cols = 0;
	int     rows = 0;
	int     i = 0;
	int     j = 0;
	int 	ret = 0;
	char    sql[1024];
	char    *ptr = NULL;
	char    name[1024];
	char	sql2[1024];

	memset(sql, 0x0, sizeof(sql));

	sprintf(sql, "select  a.rate_no,b.dime_type,a.dime_no,b.dime_name from app_dime_rate a,app_dime_info b,app_dime_parm c \
			where a.rate_no='%s' and a.dime_no=c.dime_no and a.dime_val=c.parm_value and a.rate_type='0' and a.dime_no=b.dime_no and b.dime_fmt!='#txcode' \
			group by a.dime_no,a.rate_no,b.dime_name,b.dime_type",chrg_no);
	pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_mquery("app_dime_rate", sql, 5);
	if (cols < 0)
	{
		pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
		return -1;
	}
	rows = pub_db_mfetch("app_dime_rate");
	if (rows < 0)
	{
		pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
		return -1;
	}
	if (rows == 0)
	{
		pub_db_cclose("app_dime_rate");
		pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
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
					strncpy(app_dime_info->dime_no[i], ptr, sizeof(app_dime_info->dime_no[i]));
				}

				if (strcmp(name, "dime_type") == 0)
				{
					if (ptr[0] == 'd' || ptr[0] == '$')
					{
						strncpy(app_dime_info->dr_dsp[i], app_dime_info->dime_val[i], sizeof(app_dime_info->dime_val[i]));
					}
				}
				if (strcmp(name, "dime_name") == 0)
				{
					strncpy(app_dime_info->dime_name[i], ptr, sizeof(app_dime_info->dime_name[i]));
				}
			}
			memset(sql2, 0x0, sizeof(sql2));
			snprintf(sql2, sizeof(sql2)-1, "select distinct a.dime_val from app_dime_rate a,app_dime_info b,app_dime_parm c \
				where a.rate_no='%s' and a.dime_no='%s' and a.dime_no=c.dime_no and a.dime_val=c.parm_value and a.rate_type='0' and a.dime_no=b.dime_no and b.dime_fmt!='#txcode'", 
				chrg_no, app_dime_info->dime_no[i]);
			ret = agt_wm_concat(sql2, app_dime_info->dime_val[i]);
			if (ret)
			{
				pub_db_cclose("app_dime_rate");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				return -1;
			}

			memset(sql2, 0x0, sizeof(sql2));
			snprintf(sql2, sizeof(sql2)-1, "select distinct c.parm_dsp from app_dime_rate a,app_dime_info b,app_dime_parm c \
				where a.rate_no='%s' and a.dime_no='%s' and a.dime_no=c.dime_no and a.dime_val=c.parm_value and a.rate_type='0' and a.dime_no=b.dime_no and b.dime_fmt!='#txcode'",
				chrg_no, app_dime_info->dime_no[i]);
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
			pub_db_cclose("app_dime_rate");
			pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
			*dime_num = i;
			break;
		}
		else if (rows < 0)
		{
			pub_db_cclose("app_dime_rate");
			pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
			return -1;
		}

	}
	return 0;
}
static 	int insert_txcode_as_dime_no(app_dime_info_t *app_dime_info,app_account_dime_t * app_account_dime,char * tx_code, int k)
{
	int 	i = 0;
	int     ret = 0;
	char    sql[1024];

	for( i = 0; i < atoi(app_account_dime->chrg_num[k]); i++)
	{
		memset(sql, 0x0, sizeof(sql));
		sprintf(sql, "insert into app_dime_rate (rate_type,rate_no,dime_no,dime_val) select '0','%s',dime_no,'%s' from  \
				app_dime_info where dime_fmt='#txcode'",app_dime_info->rate_no,tx_code);
		pub_log_info("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			return -1;      
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
	}
	return 0;
}
int sp2105(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	ret = 0;
	char 	buf[1024];
	char 	name[128];
	char 	sql_1[1024];
	char 	sql_2[1024];
	char 	sql_3[1024];
	char 	sql_4[1024];
	char 	option[8];
	char	tx_code[5];
	char	chrg_sum[256];
	app_chrg_rule_t 	app_chrg_rule;
	app_prdt_tx_t  		app_prdt_tx;
	app_dime_tx_t  		app_dime_tx;
	app_dime_info_t 	app_dime_info;
	app_account_dime_t 	app_account_dime;


	memset(sql_1, 0x00, sizeof(sql_1));
	memset(sql_2, 0x00, sizeof(sql_2));
	memset(sql_3, 0x00, sizeof(sql_3));
	memset(sql_4, 0x00, sizeof(sql_4));
	memset(buf, 0x00,sizeof(buf));
	memset(name, 0x00,sizeof(name));
	memset(tx_code, 0x00,sizeof(tx_code));
	memset(chrg_sum, 0x00,sizeof(chrg_sum));
	memset(&app_chrg_rule, 0x00, sizeof(app_chrg_rule));
	memset(&app_prdt_tx, 0x00, sizeof(app_prdt_tx));
	memset(&app_dime_tx, 0x00, sizeof(app_dime_tx));
	memset(&app_dime_info, 0x00, sizeof(app_dime_info));
	memset(&app_account_dime, 0x00,sizeof(app_account_dime));


	ret = agt_table_detect("app_chrg_rule a, app_prdt_tx b, app_dime_tx c, app_dime_info d");
	if (ret < 0)
	{
		strcpy(errmsg, "表或视图不存在");
		pub_log_debug("[%s][%d] %s", __FILE__, __LINE__, errmsg);
		goto ErrExit;
	}
		pub_log_debug("[%s][%d] %s", __FILE__, __LINE__, errmsg);
	memset(option, 0x0, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);

	memset(chrg_sum, 0x0, sizeof(chrg_sum));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Accounts.ACCOUNT_NUM", chrg_sum);
	pub_log_debug("[%s][%d]chrg_sum[%s]", __FILE__, __LINE__, chrg_sum);

	memset(tx_code, 0x0, sizeof(tx_code));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Accounts.TX_CODE", tx_code);
	pub_log_debug("[%s][%d]tx_code[%s]", __FILE__, __LINE__, tx_code);

	pub_log_info("[%s][%d][%s] 处理开始...", __FILE__, __LINE__, __FUNCTION__);
	pub_log_info("[%s][%d]option[%s]", __FILE__, __LINE__, option);

	if(option[0] == 'A')
	{
		int     ret = -1;
		int     cols = 0;
		int     rows = 0;
		char    sql[1024];

		memset(sql, 0x0, sizeof(sql)); 	
		ret = crt_sel_sql(tx_code, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成查询语句失败");
			goto ErrExit;
		}
		pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
		cols = pub_db_mquery("app_dime", sql, 5);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}
		rows = pub_db_mfetch("app_dime");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}
		if (rows == 0)
		{
			pub_db_cclose("app_dime");
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
		}
		if (rows > 0)
		{
			pub_db_cclose("app_dime");
			pub_log_error("[%s][%d] 交易已存在!", __FILE__, __LINE__);
			strcpy(errmsg, "交易已存在");
			goto ErrExit;
		}


		for(i = 0; i < atoi(chrg_sum); i++)
		{
			get_app_chrg_rule_req_info(vars, &app_chrg_rule, &app_account_dime, &app_dime_info, i);

			ret = agt_creat_code("rate_no_sequence", app_dime_info.rate_no);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 生成编码失败！", __FILE__, __LINE__);
				strcpy(errmsg, "生成编码失败");
				goto ErrExit;
			}

			ret = insert_app_chrg_rule(&app_chrg_rule, &app_dime_info, &app_account_dime, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}

			ret = insert_app_dime_rate(&app_chrg_rule,&app_dime_info,&app_account_dime, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}

			ret = insert_app_dime_tx(&app_chrg_rule,&app_dime_info,&app_account_dime,tx_code, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}

			ret = insert_txcode_as_dime_no(&app_dime_info,&app_account_dime,tx_code, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;	
			}
			memset(&app_chrg_rule, 0x00, sizeof(app_chrg_rule));
			memset(&app_dime_info, 0x00, sizeof(app_dime_info));
			memset(&app_account_dime, 0x00, sizeof(app_account_dime));
		}
		goto OkExit;

	}
	else if (option[0] == 'M')
	{
		int     ret = -1;
		int     cols = 0;
		int     rows = 0;
		int     i = 0;
		int     j = 0;
		char    sql[1024];
		char    *ptr = NULL;
		char    name[1024];

		memset(sql, 0x0, sizeof(sql));
		memset(sql_1, 0x0, sizeof(sql_1));
		memset(sql_2, 0x0, sizeof(sql_2));
		memset(sql_3, 0x0, sizeof(sql_3));

		ret = crt_sel_sql(tx_code, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}
		pub_log_debug("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
		cols = pub_db_mquery("app_dime", sql, 5);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}
		rows = pub_db_mfetch("app_dime");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}
		if (rows == 0)
		{
			pub_db_cclose("app_dime");
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			strcpy(errmsg, "无数据");
			goto ErrExit;
		}
		while(1)
		{
			for(i = 0; i < rows; i++)
			{
				for(j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_dime", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
					if (strcmp(name, "rate_no") == 0)
					{
						strncpy(app_dime_info.rate_no, ptr, sizeof(app_dime_info.rate_no) - 1);

						crt_del_sql_1(&app_dime_info, sql_1);
						ret = pub_db_nquery(sql_1);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] 执行删除语句失败! sql_1=[%s]", __FILE__, __LINE__, sql_1);
							strcpy(errmsg, "删除数据失败");
							goto ErrExit;
						}
						crt_del_sql_2(tx_code, sql_2);

						ret = pub_db_nquery(sql_2);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] 执行删除语句失败! sql_2=[%s]", __FILE__, __LINE__, sql_2);
							strcpy(errmsg, "删除数据失败");
							goto ErrExit;
						}
						crt_del_sql_3(&app_dime_info, sql_3);
						ret = pub_db_nquery(sql_3);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] 执行删除语句失败! sql_3=[%s]", __FILE__, __LINE__, sql_3);
							strcpy(errmsg, "删除数据失败");
							goto ErrExit;
						}
					}

				}
			}
			rows = pub_db_mfetch("app_dime");
			if (rows == 0)
			{
				pub_db_cclose("app_dime");
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_dime");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "查询数据失败");
				goto ErrExit;
			}
		}

		for(i=0;i < atoi(chrg_sum);i++)
		{
			get_app_chrg_rule_req_info(vars,&app_chrg_rule,&app_account_dime,&app_dime_info,i);

			if (strlen(app_dime_info.rate_no) == 0)
			{
				ret = agt_creat_code("rate_no_sequence", app_dime_info.rate_no);
				if(ret < 0)
				{
					pub_log_error("[%s][%d] 生成编码失败！", __FILE__, __LINE__);
					strcpy(errmsg, "生成编码失败！");
					goto ErrExit;
				}
			}
			insert_app_chrg_rule(&app_chrg_rule, &app_dime_info, &app_account_dime, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}
			ret = insert_app_dime_rate(&app_chrg_rule, &app_dime_info, &app_account_dime, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}
			insert_app_dime_tx(&app_chrg_rule, &app_dime_info, &app_account_dime, tx_code, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}
			ret = insert_txcode_as_dime_no(&app_dime_info,&app_account_dime,tx_code, i);
			if(ret < 0)
			{
				pub_log_error("[%s][%d] 插入数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "插入数据失败");
				goto ErrExit;
			}

			memset(&app_chrg_rule, 0x00, sizeof(app_chrg_rule));
			memset(&app_dime_info, 0x00, sizeof(app_dime_info));
			memset(&app_account_dime, 0x00, sizeof(app_account_dime));
		}
		goto OkExit;

	}	
	else if (option[0] == 'D')
	{

		int     ret = -1;
		int     cols = 0;
		int     rows = 0;
		int     i = 0;
		int     j = 0;
		char    sql[1024];
		char    *ptr = NULL;
		char    name[1024];

		memset(sql_1, 0x0, sizeof(sql_1));
		memset(sql_2, 0x0, sizeof(sql_2));
		memset(sql_3, 0x0, sizeof(sql_3));


		memset(sql, 0x0, sizeof(sql));


		sprintf(sql, "select distinct(rate_no) from app_dime_rate a,app_dime_info b \
				where 1 = 1 and dime_val = '%s' and a.rate_type='0' and a.dime_no = b.dime_no and b.dime_fmt ='#txcode'",tx_code);

		pub_log_info("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
		cols = pub_db_mquery("app_dime", sql, 5);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}
		rows = pub_db_mfetch("app_dime");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}
		if (rows == 0)
		{
			pub_db_cclose("app_dime");
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			strcpy(errmsg, "无数据");
			goto ErrExit;
		}
		while(1)
		{
			for(i = 0; i < rows; i++)
			{
				for(j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_dime", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
					if (strcmp(name, "rate_no") == 0)
					{
						strncpy(app_dime_info.rate_no, ptr, sizeof(app_dime_info.rate_no) - 1);

						crt_del_sql_1(&app_dime_info, sql_1);
						ret = pub_db_nquery(sql_1);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] 执行删除语句失败! sql_1=[%s]", __FILE__, __LINE__, sql_1);
							strcpy(errmsg, "执行删除语句失败");
							goto ErrExit;
						}
						crt_del_sql_2(tx_code, sql_2);

						ret = pub_db_nquery(sql_2);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] 执行删除语句失败! sql_2=[%s]", __FILE__, __LINE__, sql_2);
							goto ErrExit;
						}
						crt_del_sql_3(&app_dime_info, sql_3);
						ret = pub_db_nquery(sql_3);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] 执行删除语句失败! sql_3=[%s]", __FILE__, __LINE__, sql_3);
							goto ErrExit;
						}
					}

				}
			}
			rows = pub_db_mfetch("app_dime");
			if (rows == 0)
			{
				pub_db_cclose("app_dime");
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_dime");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "删除失败");
				goto ErrExit;
			}
		}
		goto OkExit;
	}
	if (option[0] == 'S')
	{
		int		i = 0;
		int		j = 0;
		int		cols = 0;
		int		rows = 0;
		int 	index=0;
		int		count = 0;
		int		ttlcnt = 0;
		int		pageidx = 0;
		int		pagecnt = 0;
		int		pagesum = 0;
		int 	dime_num = 0;
		int 	chrg_num = 0;
		int		flage = 0;
		char	*ptr = NULL;
		char	buf[256];
		char	name[128];
		char	opt[16];
		char	sql[2048];

		memset(buf, 0x0, sizeof(buf));

		pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

		memset(opt, 0x0, sizeof(opt));
		memset(sql, 0x0, sizeof(sql));
		pub_log_debug("[%s][%d] 查询操作处理开始...", __FILE__, __LINE__);

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
		ret = crt_sel_sql(tx_code, sql);
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
		pub_log_debug("[%s][%d] pageindex=[%d] pagesum=[%d]", __FILE__, __LINE__, pageidx, pagesum);

		cols = pub_db_mquery("app_chrg", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_chrg");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_chrg");
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			strcpy(errmsg, "无数据");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d] index=[%d]", __FILE__, __LINE__, index);
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

				memset(&app_chrg_rule, 0x0, sizeof(app_chrg_rule));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_chrg", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
					if (strcmp(name, "rate_no") == 0)
					{
						strncpy(app_dime_info.rate_no, ptr, sizeof(app_dime_info.rate_no) - 1);
						ret = get_app_dime_info(&app_dime_info,&app_dime_tx,ptr, &dime_num);
						if(ret == -1)
						{
							strcpy(errmsg, "查询数据库失败");
							goto ErrExit;
						}
						else if (ret == -2)
						{
							strcpy(errmsg, "app_dime_rate表中无相关数据");
							goto ErrExit;
						}
						ret = get_app_chrg_rule(&app_chrg_rule,ptr,&chrg_num);
						if(ret == -1)
						{
							strcpy(errmsg, "查询数据库失败");
							goto ErrExit;
						}
						else if (ret == -2)
						{
							strcpy(errmsg, "app_chrg_rule表中无相关数据");
							goto ErrExit;
						}
					}
					if (strcmp(name, "dime_val") == 0)
					{
						strncpy(app_prdt_tx.tx_code, ptr, sizeof(app_prdt_tx.tx_code) - 1);
					}
					pub_log_debug("[%s][%d]tx_code[%s]", __FILE__, __LINE__, app_prdt_tx.tx_code);

				}
				pub_log_debug("[%s][%d]tx_code[%s]  index=[%d]", __FILE__, __LINE__, app_prdt_tx.tx_code, index -1);
				set_app_chrg_rule_res_info(vars, &app_chrg_rule, &app_dime_info,&app_prdt_tx,chrg_num,dime_num,index - 1);

			}

			rows = pub_db_mfetch("app_chrg");
			if (rows == 0)
			{
				pub_db_cclose("app_chrg");
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_chrg");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "查询数据失败");
				goto ErrExit;
			}
		}	
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);

		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}
	else 
	{
		pub_log_error("[%s][%d]操作类型错误", __FILE__, __LINE__);
		strcpy(errmsg, "操作标识有误");
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2105 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2105 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
