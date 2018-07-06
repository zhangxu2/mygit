/*************************************************
  文 件 名:  flw_sp2106.c                        **
  功能描述:  应用维度管理                        **
  作    者:  赵强                                **
  完成日期:  20160801                            **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

char	errcode[128];
char	errmsg[256];
typedef struct
{
	char	dime_no[32 + 1];
	char	dime_name[60 + 1];
	char	dime_type[4 + 1];
	char	dime_lvl[128 + 1];
	char	dime_dsp[128 + 1];
	char	dime_fmt[60 + 1];
	char	dime_start_date[128 + 1];
	char	dime_end_date[128 + 1];
	char	dime_sts[1 + 1];
	char	dime_group[128 +1];
}app_dime_info_t;

static int get_app_dime_info_req_info(sw_loc_vars_t *vars, app_dime_info_t *app_dime_info)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_NO", buf);
	strncpy(app_dime_info->dime_no, buf, sizeof(app_dime_info->dime_no) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_NO]=[%s]=>[app_dime_info->dime_no]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_NAME", buf);
	strncpy(app_dime_info->dime_name, buf, sizeof(app_dime_info->dime_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_NAME]=[%s]=>[app_dime_info->dime_name]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_TYPE", buf);
	strncpy(app_dime_info->dime_type, buf, sizeof(app_dime_info->dime_type) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_TYPE]=[%s]=>[app_dime_info->dime_type]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_LVL", buf);
	strncpy(app_dime_info->dime_lvl, buf, sizeof(app_dime_info->dime_lvl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_LVL]=[%s]=>[app_dime_info->dime_lvl]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_DSP", buf);
	strncpy(app_dime_info->dime_dsp, buf, sizeof(app_dime_info->dime_dsp) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_DSP]=[%s]=>[app_dime_info->dime_dsp]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_FMT", buf);
	strncpy(app_dime_info->dime_fmt, buf, sizeof(app_dime_info->dime_fmt) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_FMT]=[%s]=>[app_dime_info->dime_fmt]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_START_DATE", buf);
	strncpy(app_dime_info->dime_start_date, buf, sizeof(app_dime_info->dime_start_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_START_DATE]=[%s]=>[app_dime_info->dime_start_date]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_END_DATE", buf);
	strncpy(app_dime_info->dime_end_date, buf, sizeof(app_dime_info->dime_end_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_END_DATE]=[%s]=>[app_dime_info->dime_end_date]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_STS", buf);
	strncpy(app_dime_info->dime_sts, buf, sizeof(app_dime_info->dime_sts) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_STS]=[%s]=>[app_dime_info->dime_sts]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Info.DIME_GROUP", buf);
	strncpy(app_dime_info->dime_group, buf, sizeof(app_dime_info->dime_group) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Info.DIME_GROUP]=[%s]=>[app_dime_info->dime_group]",
			__FILE__, __LINE__, buf);

	return 0;
}

static int set_app_dime_info_res_info(sw_loc_vars_t *vars, app_dime_info_t *app_dime_info, int index)
{
	char	path[256];
	char	code_desc[61];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_NO", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_no);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_no);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_NAME", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_TYPE", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_type);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_type);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_LVL", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_lvl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_lvl);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_DSP", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_dsp);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_dsp);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_FMT", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_fmt);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_fmt);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_START_DATE", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_start_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_start_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_END_DATE", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_end_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_end_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_STS", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_sts);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_sts);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_GROUP", index);
	loc_set_zd_data(vars, path, app_dime_info->dime_group);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_info->dime_group);	

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Infos.Eci_Dime_Info(%d).DIME_GROUP_DESC", index);
	memset(code_desc, 0x0, sizeof(code_desc));
	set_app_data_dic_trans("dime_group",app_dime_info->dime_group, code_desc);
	loc_set_zd_data(vars, path, code_desc);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, code_desc);	

	return 0;

}

static int crt_ins_sql(app_dime_info_t *app_dime_info, char *sql)
{
	int		i = 0;
	int 	ret = -1;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];
	char	sql_1[1024];
	char	dime_no[16];
	char 	*ptr = NULL;
	char 	name[256];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	memset(sql_1, 0x0, sizeof(sql_1));
	memset(dime_no, 0x0, sizeof(dime_no));
	memset(name, 0x0, sizeof(name));

	/*生成ID号*/
	sprintf(sql_1, "select max(dime_no) from app_dime_info");
	ret = pub_db_squery(sql_1);		
	if (ret < 0)
	{
		pub_log_error("[%s][%d]数据库查询失败, sql=[%s]", __FILE__, __LINE__, sql_1);
		return -1;
	}

	memset(dime_no, 0x0, sizeof(dime_no));
	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	pub_str_ziphlspace(ptr);
	if (strlen(ptr) == 0)
	{
		strcpy(dime_no, "0001");
	}
	else 
	{
		sprintf(dime_no, "%04d", atoi(ptr) + 1);
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "'%s'", dime_no);
	strcat(values, buf);
	strcat(cols, "dime_no");
	strcat(cols, ",");
	strcat(values, ",");
	if (app_dime_info->dime_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_info->dime_name);
		strcat(values, buf);
		strcat(cols, "dime_name");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维护名称为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_info->dime_type);
		strcat(values, buf);
		strcat(cols, "dime_type");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度类型为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_dime_info->dime_lvl);
		strcat(values, buf);
		strcat(cols, "dime_lvl");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_dime_info->dime_dsp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_info->dime_dsp);
		strcat(values, buf);
		strcat(cols, "dime_dsp");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_dime_info->dime_fmt[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_info->dime_fmt);
		strcat(values, buf);
		strcat(cols, "dime_fmt");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度变量名为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_dime_info->dime_start_date);
		strcat(values, buf);
		strcat(cols, "dime_start_date");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度生效日期为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_dime_info->dime_end_date);
		strcat(values, buf);
		strcat(cols, "dime_end_date");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度截止日期为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_info->dime_sts);
		strcat(values, buf);
		strcat(cols, "dime_sts");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度状态为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_group[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_info->dime_group);
		strcat(values, buf);
		strcat(cols, "dime_group");
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

	sprintf(sql, "insert into app_dime_info(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_dime_info_t *app_dime_info, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_dime_info->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_no = '%s' ", app_dime_info->dime_no);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度编号为空", __FILE__, __LINE__);
		return -1;
	}

	sprintf(sql, "delete from app_dime_info where %s", wherelist);

	return 0;
}

static int crt_upd_sql(app_dime_info_t *app_dime_info, char *sql)
{
	int		i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_dime_info->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_no = '%s',", app_dime_info->dime_no);
		strcat(setlist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素维度编号为空", __FILE__, __LINE__);
		return -1;
	}

	if (app_dime_info->dime_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_name = '%s',", app_dime_info->dime_name);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_type = '%s',", app_dime_info->dime_type);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_lvl = %s,", app_dime_info->dime_lvl);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_dsp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_dsp = '%s',", app_dime_info->dime_dsp);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_fmt[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_fmt = '%s',", app_dime_info->dime_fmt);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_start_date = %s,", app_dime_info->dime_start_date);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_end_date = %s,", app_dime_info->dime_end_date);
		strcat(setlist, buf);
	}

	if (app_dime_info->dime_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_sts = '%s',", app_dime_info->dime_sts);
		strcat(setlist, buf);
	}
	if (app_dime_info->dime_group[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_group = '%s',", app_dime_info->dime_group);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';
	if (app_dime_info->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_no = '%s'", app_dime_info->dime_no);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_dime_info set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_dime_info_t *app_dime_info, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_dime_info->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_no = '%s'", app_dime_info->dime_no);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_name = '%s'", app_dime_info->dime_name);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_type[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_type = '%s'", app_dime_info->dime_type);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_lvl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_lvl = %s", app_dime_info->dime_lvl);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_dsp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_dsp = '%s'", app_dime_info->dime_dsp);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_fmt[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_fmt = '%s'", app_dime_info->dime_fmt);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_start_date = %s", app_dime_info->dime_start_date);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_end_date = %s", app_dime_info->dime_end_date);
		strcat(wherelist, buf);
	}

	if (app_dime_info->dime_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_sts = '%s'", app_dime_info->dime_sts);
		strcat(wherelist, buf);
	}
	if (app_dime_info->dime_group[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_group = '%s'", app_dime_info->dime_group);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select * from app_dime_info where %s order by dime_no", wherelist);

	return 0;
}

int sp2106(sw_loc_vars_t *vars)
{
	int		ret = 0;
	char	opt[16];
	char	sql[2048];
	app_dime_info_t	app_dime_info;


	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_dime_info");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_dime_info不存在");
		pub_log_debug("[%s][%d] %s", __FILE__, __LINE__, errmsg);
		goto ErrExit;
	}
	memset(errmsg, 0x00, sizeof(errmsg));
	memset(errcode, 0x0, sizeof(errcode));
	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_dime_info, 0x0, sizeof(app_dime_info));
	
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);
	if (opt[0] == 'A')
	{
		pub_log_debug("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		get_app_dime_info_req_info(vars, &app_dime_info);
		ret = crt_ins_sql(&app_dime_info, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成插入语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成数据库语句失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] insert sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行插入语句失败");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'D')
	{
		pub_log_debug("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_dime_info_req_info(vars, &app_dime_info);
		ret = crt_del_sql(&app_dime_info, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成删除语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成插入语句失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] delete sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行删除语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行删除语句失败");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 删除语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'M')
	{
		pub_log_debug("[%s][%d] 修改操作处理开始...", __FILE__, __LINE__);
		get_app_dime_info_req_info(vars, &app_dime_info);
		ret = crt_upd_sql(&app_dime_info, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成更新语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成插入语句失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] update sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行更新语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行更新语句失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] 更新语句执行成功! ", __FILE__, __LINE__);
		goto OkExit;
	}

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

		get_app_dime_info_req_info(vars, &app_dime_info);
		ret = crt_sel_sql(&app_dime_info, sql);
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
				strcpy(errmsg, "页面超出范围");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_dime_info", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_dime_info");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 游标查询数据失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			strcpy(errmsg, "无数据");
			pub_db_cclose("app_dime_info");
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

				memset(&app_dime_info, 0x0, sizeof(app_dime_info));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_dime_info", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "dime_no") == 0)
					{
						strncpy(app_dime_info.dime_no, ptr, sizeof(app_dime_info.dime_no) - 1);
					}

					if (strcmp(name, "dime_name") == 0)
					{
						strncpy(app_dime_info.dime_name, ptr, sizeof(app_dime_info.dime_name) - 1);
					}

					if (strcmp(name, "dime_type") == 0)
					{
						strncpy(app_dime_info.dime_type, ptr, sizeof(app_dime_info.dime_type) - 1);
					}

					if (strcmp(name, "dime_lvl") == 0)
					{
						strncpy(app_dime_info.dime_lvl, ptr, sizeof(app_dime_info.dime_lvl) - 1);
					}

					if (strcmp(name, "dime_dsp") == 0)
					{
						strncpy(app_dime_info.dime_dsp, ptr, sizeof(app_dime_info.dime_dsp) - 1);
					}

					if (strcmp(name, "dime_fmt") == 0)
					{
						strncpy(app_dime_info.dime_fmt, ptr, sizeof(app_dime_info.dime_fmt) - 1);
					}

					if (strcmp(name, "dime_start_date") == 0)
					{
						strncpy(app_dime_info.dime_start_date, ptr, sizeof(app_dime_info.dime_start_date) - 1);
					}

					if (strcmp(name, "dime_end_date") == 0)
					{
						strncpy(app_dime_info.dime_end_date, ptr, sizeof(app_dime_info.dime_end_date) - 1);
					}

					if (strcmp(name, "dime_sts") == 0)
					{
						strncpy(app_dime_info.dime_sts, ptr, sizeof(app_dime_info.dime_sts) - 1);
					}
					if (strcmp(name, "dime_group") == 0)
					{
						strncpy(app_dime_info.dime_group, ptr, sizeof(app_dime_info.dime_group) - 1);
					}
				}
				set_app_dime_info_res_info(vars, &app_dime_info, index - 1);
			}

			rows = pub_db_mfetch("app_dime_info");
			if (rows == 0)
			{
				pub_db_cclose("app_dime_info");
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "游标查询数据失败");
				strcpy(errcode, "S005");
				pub_db_cclose("app_dime_info");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}

	pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
	strcpy(errmsg, "操作标识有误");
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2106 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2106 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
