/*************************************************
  文 件 名:  flw_sp2115.c                        **
  功能描述:  产品参数表维护                      **
  作    者:  邹佩                                **
  完成日期:  20160801                            **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	arg_name[60 + 1];
	char	arg_value[120 + 1];
	char	arg_owner[32 + 1];
	char	arg_ctgy[32 + 1];
	char	arg_bus[32 + 1];
	char	arg_load[2 + 1];
	char	arg_memo[60 + 1];
}app_params_t;

static int get_app_params_req_info(sw_loc_vars_t *vars, app_params_t *app_params)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_NAME", buf);
	strncpy(app_params->arg_name, buf, sizeof(app_params->arg_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_NAME]=[%s]=>[app_params->arg_name]",
		__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_VALUE", buf);
	strncpy(app_params->arg_value, buf, sizeof(app_params->arg_value) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_VALUE]=[%s]=>[app_params->arg_value]",
		__FILE__, __LINE__, buf);


	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_OWNER", buf);
	strncpy(app_params->arg_owner, buf, sizeof(app_params->arg_owner) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_OWNER]=[%s]=>[app_params->arg_owner]",
		__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_CTGY", buf);
	pub_str_ziphlspace(buf);
	strncpy(app_params->arg_ctgy, buf, sizeof(app_params->arg_ctgy) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_CTGY]=[%s]=>[app_params->arg_ctgy]",
		__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_BUS", buf);
	pub_str_ziphlspace(buf);
	strncpy(app_params->arg_bus, buf, sizeof(app_params->arg_bus) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_BUS]=[%s]=>[app_params->arg_bus]",
		__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_LOAD", buf);
	strncpy(app_params->arg_load, buf, sizeof(app_params->arg_load) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_LOAD]=[%s]=>[app_params->arg_load]",
		__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Params.ARG_MEMO", buf);
	strncpy(app_params->arg_memo, buf, sizeof(app_params->arg_memo) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Params.ARG_MEMO]=[%s]=>[app_params->arg_memo]",
		__FILE__, __LINE__, buf);

	return 0;
}

static int set_app_params_res_info(sw_loc_vars_t *vars, app_params_t *app_params, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_NAME", index);
	loc_set_zd_data(vars, path, app_params->arg_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_VALUE", index);
	loc_set_zd_data(vars, path, app_params->arg_value);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_value);


	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_OWNER", index);
	loc_set_zd_data(vars, path, app_params->arg_owner);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_owner);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_CTGY", index);
	loc_set_zd_data(vars, path, app_params->arg_ctgy);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_ctgy);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_BUS", index);
	loc_set_zd_data(vars, path, app_params->arg_bus);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_bus);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_LOAD", index);
	loc_set_zd_data(vars, path, app_params->arg_load);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_load);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Paramss.App_Params(%d).ARG_MEMO", index);
	loc_set_zd_data(vars, path, app_params->arg_memo);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_params->arg_memo);

	return 0;
}

static int crt_ins_sql(app_params_t *app_params, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));

	if (app_params->arg_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_name);
		strcat(values, buf);
		strcat(cols, "arg_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_params->arg_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_value);
		strcat(values, buf);
		strcat(cols, "arg_value");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_params->arg_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_owner);
		strcat(values, buf);
		strcat(cols, "arg_owner");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_params->arg_ctgy[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_ctgy);
		strcat(values, buf);
		strcat(cols, "arg_ctgy");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_params->arg_bus[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_bus);
		strcat(values, buf);
		strcat(cols, "arg_bus");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_params->arg_load[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_load);
		strcat(values, buf);
		strcat(cols, "arg_load");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_params->arg_memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_params->arg_memo);
		strcat(values, buf);
		strcat(cols, "arg_memo");
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

	sprintf(sql, "insert into app_params(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_params_t *app_params, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_params->arg_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_owner = '%s' ", app_params->arg_owner);
		strcat(wherelist, buf);
	}

	if (app_params->arg_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_name = '%s' ", app_params->arg_name);
		strcat(wherelist, buf);
	}

	sprintf(sql, "delete from app_params where %s", wherelist);

	return 0;
}

static int crt_upd_sql(app_params_t *app_params, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");

	if (app_params->arg_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_name = '%s',", app_params->arg_name);
		strcat(setlist, buf);
	}

	if (app_params->arg_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_value = '%s',", app_params->arg_value);
		strcat(setlist, buf);
	}


	if (app_params->arg_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_owner = '%s',", app_params->arg_owner);
		strcat(setlist, buf);
	}

	if (app_params->arg_ctgy[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_ctgy = '%s',", app_params->arg_ctgy);
		strcat(setlist, buf);
	}

	if (app_params->arg_bus[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_bus = '%s',", app_params->arg_bus);
		strcat(setlist, buf);
	}

	if (app_params->arg_load[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_load = '%s',", app_params->arg_load);
		strcat(setlist, buf);
	}

	if (app_params->arg_memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " arg_memo = '%s',", app_params->arg_memo);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';
	if (app_params->arg_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_owner = '%s'", app_params->arg_owner);
		strcat(wherelist, buf);
	}

	if (app_params->arg_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_name = '%s'", app_params->arg_name);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_params set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_params_t *app_params, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_params->arg_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_name = '%s'", app_params->arg_name);
		strcat(wherelist, buf);
	}

	if (app_params->arg_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_value = '%s'", app_params->arg_value);
		strcat(wherelist, buf);
	}


	if (app_params->arg_owner[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_owner = '%s'", app_params->arg_owner);
		strcat(wherelist, buf);
	}

	if (app_params->arg_ctgy[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_ctgy = '%s'", app_params->arg_ctgy);
		strcat(wherelist, buf);
	}

	if (app_params->arg_bus[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_bus = '%s'", app_params->arg_bus);
		strcat(wherelist, buf);
	}

	if (app_params->arg_load[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_load = '%s'", app_params->arg_load);
		strcat(wherelist, buf);
	}

	if (app_params->arg_memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and arg_memo = '%s'", app_params->arg_memo);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select * from app_params where %s order by arg_owner,arg_name", wherelist);

	return 0;
}

int sp2115(sw_loc_vars_t *vars)
{
	int	ret = 0;
	char	opt[16];
	char	sql[2048];
	char	errmsg[128];
	memset(errmsg, 0x00, sizeof(errmsg));
	app_params_t	app_params;

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_params");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_params不存在");
		goto ErrExit;
	}

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_params, 0x0, sizeof(app_params));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d]操作标识opt=[%s]", __FILE__, __LINE__, opt);
	
	if (opt[0] == 'A')
	{
		pub_log_info("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		get_app_params_req_info(vars, &app_params);
		ret = crt_ins_sql(&app_params, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成插入语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成插入语句失败!");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]插入语句sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行插入语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行插入语句失败!");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);

	}

	if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_params_req_info(vars, &app_params);
		ret = crt_del_sql(&app_params, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成删除语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成删除语句失败!");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]删除语句sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行删除语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行删除语句失败!");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 删除语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);

	}

	if (opt[0] == 'M')
	{
		pub_log_info("[%s][%d] 修改操作处理开始...", __FILE__, __LINE__);
		get_app_params_req_info(vars, &app_params);
		ret = crt_upd_sql(&app_params, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成更新语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成更新语句失败!");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]更新语句sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 执行更新语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "执行更新语句失败!");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] 更新语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);

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

		get_app_params_req_info(vars, &app_params);
		ret = crt_sel_sql(&app_params, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "生成查询语句失败!");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]查询语句sql=[%s]", __FILE__, __LINE__, sql);
		if (pagecnt > 0)
		{
			ttlcnt = pub_db_get_fetrows(sql);
			if (ttlcnt < 0)
			{
				pub_log_error("[%s][%d]获取行数错误!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败!");
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

		cols = pub_db_mquery("app_params", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败!");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_params");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败!");
			goto ErrExit;
		}

		if (rows == 0)
		{
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

				memset(&app_params, 0x0, sizeof(app_params));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_params", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_info("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);


					if (strcmp(name, "arg_name") == 0)
					{
						strncpy(app_params.arg_name, ptr, sizeof(app_params.arg_name) - 1);
					}

					if (strcmp(name, "arg_value") == 0)
					{
						strncpy(app_params.arg_value, ptr, sizeof(app_params.arg_value) - 1);
					}


					if (strcmp(name, "arg_owner") == 0)
					{
						strncpy(app_params.arg_owner, ptr, sizeof(app_params.arg_owner) - 1);
					}

					if (strcmp(name, "arg_ctgy") == 0)
					{
						strncpy(app_params.arg_ctgy, ptr, sizeof(app_params.arg_ctgy) - 1);
					}

					if (strcmp(name, "arg_bus") == 0)
					{
						strncpy(app_params.arg_bus, ptr, sizeof(app_params.arg_bus) - 1);
					}

					if (strcmp(name, "arg_load") == 0)
					{
						strncpy(app_params.arg_load, ptr, sizeof(app_params.arg_load) - 1);
					}

					if (strcmp(name, "arg_memo") == 0)
					{
						strncpy(app_params.arg_memo, ptr, sizeof(app_params.arg_memo) - 1);
					}
				}
				set_app_params_res_info(vars, &app_params, index - 1);
			}

			rows = pub_db_mfetch("app_params");
			if (rows == 0)
			{
				pub_log_info("[%s][%d] 查询结束!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_log_error("[%s][%d] 查询错误!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		goto OkExit;
	}

	if (opt[0] == 'A' || opt[0] == 'D' || opt[0] == 'M')
	{
		char cmd[128];

		memset(cmd, 0x00, sizeof(cmd));
		sprintf(cmd, "swadmin reload -prdt %s arg %s", app_params.arg_owner, app_params.arg_owner);
		pub_log_info("[%s][%d] cmd=[%s]", __FILE__, __LINE__, cmd);
		agt_system(cmd);
		goto OkExit;
	}


	pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
	strcpy(errmsg, "操作标识有误!");
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2115 处理成功!");
	ret = pub_db_commit();
	if (ret)
	{
		pub_log_error("[%s][%d] 提交数据库失败!", __FILE__, __LINE__);
		strcpy(errmsg, "提交数据库失败!!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2115 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
