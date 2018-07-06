/*************************************************
 文 件 名:  flw_sp2108.c                        **
 功能描述:  业务类型处理交易                    **
 作    者:  linpanfei                           **
 完成日期:  20160802                            **
*************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	ctgy_id[32 + 1];
	char	ctgy_name[60 + 1];
	char	memo[60 + 1];
}app_ctgy_t;

static int get_app_ctgy_req_info(sw_loc_vars_t *vars, app_ctgy_t *app_ctgy)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Ctgy.CTGY_NAME", buf);
	strncpy(app_ctgy->ctgy_name, buf, sizeof(app_ctgy->ctgy_name) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Ctgy.CTGY_NAME]=[%s]=>[app_ctgy->ctgy_name]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Ctgy.CTGY_ID", buf);
	strncpy(app_ctgy->ctgy_id, buf, sizeof(app_ctgy->ctgy_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Ctgy.CTGY_ID]=[%s]=>[app_ctgy->ctgy_id]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Ctgy.MEMO", buf);
	strncpy(app_ctgy->memo, buf, sizeof(app_ctgy->memo) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Ctgy.MEMO]=[%s]=>[app_ctgy->memo]",
		 __FILE__, __LINE__, buf);

	return 0;
}

static int set_app_ctgy_res_info(sw_loc_vars_t *vars, app_ctgy_t *app_ctgy, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Ctgys.App_Ctgy(%d).CTGY_NAME", index);
	loc_set_zd_data(vars, path, app_ctgy->ctgy_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_ctgy->ctgy_name);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Ctgys.App_Ctgy(%d).CTGY_ID", index);
	loc_set_zd_data(vars, path, app_ctgy->ctgy_id);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_ctgy->ctgy_id);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Ctgys.App_Ctgy(%d).MEMO", index);
	loc_set_zd_data(vars, path, app_ctgy->memo);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_ctgy->memo);

	return 0;
}

static int crt_ins_sql(app_ctgy_t *app_ctgy, char *sql)
{
	int 	i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	if (app_ctgy->ctgy_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_ctgy->ctgy_id);
		strcat(values, buf);
		strcat(cols, "ctgy_id");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_ctgy->ctgy_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_ctgy->ctgy_name);
		strcat(values, buf);
		strcat(cols, "ctgy_name");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_ctgy->memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_ctgy->memo);
		strcat(values, buf);
		strcat(cols, "memo");
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

	sprintf(sql, "insert into app_ctgy(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_ctgy_t *app_ctgy, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_ctgy->ctgy_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and ctgy_id = '%s'", app_ctgy->ctgy_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "delete from app_ctgy where %s", wherelist);

	return 0;
}

static int crt_upd_sql(app_ctgy_t *app_ctgy, char *sql)
{
	int 	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_ctgy->ctgy_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " ctgy_id = '%s',", app_ctgy->ctgy_id);
		strcat(setlist, buf);
	}

	if (app_ctgy->ctgy_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " ctgy_name = '%s',", app_ctgy->ctgy_name);
		strcat(setlist, buf);
	}

	if (app_ctgy->memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " memo = '%s',", app_ctgy->memo);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';

	if (app_ctgy->ctgy_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and ctgy_id = '%s'", app_ctgy->ctgy_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_ctgy set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_ctgy_t *app_ctgy, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_ctgy->ctgy_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and ctgy_id = '%s'", app_ctgy->ctgy_id);
		strcat(wherelist, buf);
	}

	if (app_ctgy->ctgy_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and ctgy_name = '%s'", app_ctgy->ctgy_name);
		strcat(wherelist, buf);
	}

	if (app_ctgy->memo[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and memo = '%s'", app_ctgy->memo);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select * from app_ctgy where %s ", wherelist);

	return 0;
}

int sp2108(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	char	opt[16];
	char	sql[2048];
	char	errmsg[256];
	app_ctgy_t	app_ctgy;

	memset(errmsg, 0x0, sizeof(errmsg));

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_ctgy");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_ctgy不存在");
		pub_log_info("[%s][%d] %s", __FILE__, __LINE__, errmsg);
		goto ErrExit;
	}
	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_ctgy, 0x0, sizeof(app_ctgy));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);

	if (opt[0] == 'A')
	{
		pub_log_debug("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		get_app_ctgy_req_info(vars, &app_ctgy);
		ret = agt_creat_code("ctgy_id_sequence", app_ctgy.ctgy_id);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成编码失败！", __FILE__, __LINE__);
			strcpy(errmsg, "生成编码失败");
			goto ErrExit;
		}
		ret = crt_ins_sql(&app_ctgy, sql);
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
			strcpy(errmsg, "插入失败");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'D')
	{
		pub_log_debug("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_ctgy_req_info(vars, &app_ctgy);
		ret = crt_del_sql(&app_ctgy, sql);
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

	if (opt[0] == 'M')
	{
		pub_log_debug("[%s][%d] 修改操作处理开始...", __FILE__, __LINE__);
		get_app_ctgy_req_info(vars, &app_ctgy);
		ret = crt_upd_sql(&app_ctgy, sql);
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
			pub_log_error("[%s][%d] 执行删除语句失败! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "更新失败");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] 更新语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
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
		pub_log_info("[%s][%d] pagecount=[%d] pageindex=[%d]", __FILE__, __LINE__, pagecnt, pageidx);

		get_app_ctgy_req_info(vars, &app_ctgy);
		ret = crt_sel_sql(&app_ctgy, sql);
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

		cols = pub_db_mquery("app_ctgy", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_ctgy");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}
		if (rows == 0)
		{
			pub_db_cclose("app_ctgy");
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

				memset(&app_ctgy, 0x0, sizeof(app_ctgy));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_ctgy", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "ctgy_id") == 0)
					{
						strncpy(app_ctgy.ctgy_id, ptr, sizeof(app_ctgy.ctgy_id) - 1);
					}

					if (strcmp(name, "ctgy_name") == 0)
					{
						strncpy(app_ctgy.ctgy_name, ptr, sizeof(app_ctgy.ctgy_name) - 1);
					}

					if (strcmp(name, "memo") == 0)
					{
						strncpy(app_ctgy.memo, ptr, sizeof(app_ctgy.memo) - 1);
					}
				}
				set_app_ctgy_res_info(vars, &app_ctgy, index - 1);
			}

			rows = pub_db_mfetch("app_ctgy");
			if (rows == 0)
			{
				pub_db_cclose("app_ctgy");
				pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_ctgy");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		goto OkExit;
	}

	pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
	strcpy(errmsg, "操作标识有误");
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2108 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2108 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
