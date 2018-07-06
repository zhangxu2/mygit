/*************************************************
  文 件 名:  flw_sp2101.c                        **
  功能描述:  数据字典管理                        **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

static	char	errmsg[128];
static	char	errcode[8];
typedef struct
{
	char	code_id[32 + 1];
	char	code_name[32 + 1];
	char	code_value[32 + 1];
	char	code_desc[32 + 1];
	char	code_sts[1 + 1];
}app_data_dic_t;

/*获取数据字典操作请求信息*/
static int get_app_data_dic_req_info(sw_loc_vars_t *vars, app_data_dic_t *app_data_dic)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Data_Dic.CODE_ID", buf);
	strncpy(app_data_dic->code_id, buf, sizeof(app_data_dic->code_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Data_Dic.CODE_ID]=[%s]=>[app_data_dic->code_id]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Data_Dic.CODE_NAME", buf);
	strncpy(app_data_dic->code_name, buf, sizeof(app_data_dic->code_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Data_Dic.CODE_NAME]=[%s]=>[app_data_dic->code_name]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Data_Dic.CODE_VALUE", buf);
	strncpy(app_data_dic->code_value, buf, sizeof(app_data_dic->code_value) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Data_Dic.CODE_VALUE]=[%s]=>[app_data_dic->code_value]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Data_Dic.CODE_DESC", buf);
	strncpy(app_data_dic->code_desc, buf, sizeof(app_data_dic->code_desc) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Data_Dic.CODE_DESC]=[%s]=>[app_data_dic->code_desc]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Data_Dic.CODE_STS", buf);
	strncpy(app_data_dic->code_sts, buf, sizeof(app_data_dic->code_sts) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Data_Dic.CODE_STS]=[%s]=>[app_data_dic->code_sts]",
			__FILE__, __LINE__, buf);

	return 0;
}

/*设置数据字典查询的应答信息*/
static int set_app_data_dic_res_info(sw_loc_vars_t *vars, app_data_dic_t *app_data_dic, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Data_Dics.Eci_Data_Dic(%d).CODE_ID", index);
	loc_set_zd_data(vars, path, app_data_dic->code_id);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_data_dic->code_id);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Data_Dics.Eci_Data_Dic(%d).CODE_NAME", index);
	loc_set_zd_data(vars, path, app_data_dic->code_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_data_dic->code_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Data_Dics.Eci_Data_Dic(%d).CODE_VALUE", index);
	loc_set_zd_data(vars, path, app_data_dic->code_value);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_data_dic->code_value);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Data_Dics.Eci_Data_Dic(%d).CODE_DESC", index);
	loc_set_zd_data(vars, path, app_data_dic->code_desc);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_data_dic->code_desc);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Data_Dics.Eci_Data_Dic(%d).CODE_STS", index);
	loc_set_zd_data(vars, path, app_data_dic->code_sts);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_data_dic->code_sts);

	return 0;
}

static int crt_ins_sql(app_data_dic_t *app_data_dic, char *sql)
{
	int 	i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	if (app_data_dic->code_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_data_dic->code_id);
		strcat(values, buf);
		strcat(cols, "code_id");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务要素编码标识为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务要素编码标识为空");
		return -1;
	}

	if (app_data_dic->code_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_data_dic->code_name);
		strcat(values, buf);
		strcat(cols, "code_name");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务要素编码名称为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务要素编码名称为空");
		return -1;
	}

	if (app_data_dic->code_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_data_dic->code_value);
		strcat(values, buf);
		strcat(cols, "code_value");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务要素编码值为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务要素编码值为空");
		return -1;
	}

	if (app_data_dic->code_desc[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_data_dic->code_desc);
		strcat(values, buf);
		strcat(cols, "code_desc");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素编码描述为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务元素编码描述为空");
		return -1;
	}

	if (app_data_dic->code_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_data_dic->code_sts);
		strcat(values, buf);
		strcat(cols, "code_sts");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素编码状态为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务元素编码状态为空");
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

	sprintf(sql, "insert into app_data_dic(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_data_dic_t *app_data_dic, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_data_dic->code_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_id = '%s' ", app_data_dic->code_id);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素编码标识为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务元素编码标识为空");
		return -1;
	}

	if (app_data_dic->code_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_value = '%s' ", app_data_dic->code_value);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素编码值为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务元素编码值为空");
		return -1;
	}
	sprintf(sql, "delete from app_data_dic where %s", wherelist);

	return 0;
}

static int crt_upd_sql(app_data_dic_t *app_data_dic, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_data_dic->code_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " code_id = '%s',", app_data_dic->code_id);
		strcat(setlist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素编码标识为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务元素编码标识为空");
		return -1;
	}

	if (app_data_dic->code_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " code_name = '%s',", app_data_dic->code_name);
		strcat(setlist, buf);
	}

	if (app_data_dic->code_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " code_value = '%s',", app_data_dic->code_value);
		strcat(setlist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 业务元素编码值为空", __FILE__, __LINE__);
		strcpy(errmsg, "业务元素编码值为空");
		return -1;
	}

	if (app_data_dic->code_desc[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " code_desc = '%s',", app_data_dic->code_desc);
		strcat(setlist, buf);
	}

	if (app_data_dic->code_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " code_sts = '%s',", app_data_dic->code_sts);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	if (setlist[i] == ',')
	{
		setlist[i] = '\0';
	}
	
	if (app_data_dic->code_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_value = '%s'", app_data_dic->code_value);
		strcat(wherelist, buf);
	}

	if (app_data_dic->code_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_id = '%s'", app_data_dic->code_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_data_dic set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_data_dic_t *app_data_dic, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_data_dic->code_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_id = '%s'", app_data_dic->code_id);
		strcat(wherelist, buf);
	}
	else
	{
		sprintf(sql, "select distinct code_id, code_name from app_data_dic ");
		return 0;
	}

	if (app_data_dic->code_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_name = '%s'", app_data_dic->code_name);
		strcat(wherelist, buf);
	}

	if (app_data_dic->code_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_value = '%s'", app_data_dic->code_value);
		strcat(wherelist, buf);
	}

	if (app_data_dic->code_desc[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_desc = '%s'", app_data_dic->code_desc);
		strcat(wherelist, buf);
	}

	if (app_data_dic->code_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and code_sts = '%s'", app_data_dic->code_sts);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select * from app_data_dic where %s order by code_value,code_id", wherelist);

	return 0;
}

int sp2101(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	char	opt[16];
	char	sql[2048];
	app_data_dic_t	app_data_dic;

	memset(errmsg, 0x0, sizeof(errmsg));
	memset(errcode, 0x0, sizeof(errcode));

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_data_dic");
	if (ret < 0)
	{
		pub_log_error("[%s][%d]表不存在", __FILE__, __LINE__);
		strcpy(errmsg, "表app_data_dic不存在");
		goto ErrExit;
	}
	
	memset(sql, 0x0, sizeof(sql));
	memset(&app_data_dic, 0x0, sizeof(app_data_dic));
	
	memset(opt, 0x0, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);
	
	if (opt[0] == 'A')
	{
		get_app_data_dic_req_info(vars, &app_data_dic);
		sprintf(sql, "select * from app_data_dic where code_id = '%s' and code_value = '%s' ",app_data_dic.code_id,app_data_dic.code_value);
		ret = pub_db_squery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行插入语句失败");
			goto ErrExit;
		}
		else if (ret > 0)
		{
			pub_log_error("[%s][%d] 编码已存在! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "编码已存在");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		ret = crt_ins_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql语句生成失败");
			pub_log_error("[%s][%d] 生成插入语句失败!", __FILE__, __LINE__);
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
	else if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_data_dic_req_info(vars, &app_data_dic);
		ret = crt_del_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql语句生成失败");
			pub_log_error("[%s][%d] 生成删除语句失败!", __FILE__, __LINE__);
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
	else if (opt[0] == 'M')
	{
		pub_log_info("[%s][%d] 修改操作处理开始...", __FILE__, __LINE__);
		get_app_data_dic_req_info(vars, &app_data_dic);
		ret = crt_upd_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql语句生成失败");
			pub_log_error("[%s][%d] 生成更新语句失败!", __FILE__, __LINE__);
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
		int	flag = 0;
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
			flag = 1;
		}
		pub_log_debug("[%s][%d] pagecount=[%d] pageindex=[%d]", __FILE__, __LINE__, pagecnt, pageidx);

		get_app_data_dic_req_info(vars, &app_data_dic);
		ret = crt_sel_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql语句生成失败");
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
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
				strcpy(errmsg, "无数据");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_data_dic", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_data_dic");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 游标查询数据失败!", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			pub_db_cclose("app_data_dic");
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
				if (pagecnt > 0 && index > pagecnt && flag == 0)
				{
					break;
				}

				memset(&app_data_dic, 0x0, sizeof(app_data_dic));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_data_dic", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "code_id") == 0)
					{
						strncpy(app_data_dic.code_id, ptr, sizeof(app_data_dic.code_id) - 1);
					}

					if (strcmp(name, "code_name") == 0)
					{
						strncpy(app_data_dic.code_name, ptr, sizeof(app_data_dic.code_name) - 1);
					}

					if (strcmp(name, "code_value") == 0)
					{
						strncpy(app_data_dic.code_value, ptr, sizeof(app_data_dic.code_value) - 1);
					}

					if (strcmp(name, "code_desc") == 0)
					{
						strncpy(app_data_dic.code_desc, ptr, sizeof(app_data_dic.code_desc) - 1);
					}

					if (strcmp(name, "code_sts") == 0)
					{
						strncpy(app_data_dic.code_sts, ptr, sizeof(app_data_dic.code_sts) - 1);
					}
				}
				set_app_data_dic_res_info(vars, &app_data_dic, index - 1);
			}

			rows = pub_db_mfetch("app_data_dic");
			if (rows == 0)
			{
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				pub_db_cclose("app_data_dic");
				break;
			}
			else if (rows < 0)
			{
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				pub_db_cclose("app_data_dic");
				strcpy(errmsg, "游标查询数据失败");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
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
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2101 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2101 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
