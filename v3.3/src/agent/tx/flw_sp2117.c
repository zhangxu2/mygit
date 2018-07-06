/*************************************************
  文 件 名: flw_sp2117.c                         **
  功能描述: 产品机构控制                         **
  作    者: baidan                               **
  完成日期: 20160801-10:05                       **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	prdt_id[32 + 1]; 
	char	brno[32 + 1];	
	char	brchtp[1 + 1];
	char	brch_name[60 + 1];
	char	date_ctrl[1 + 1];
	char	start_date[128 + 1];
	char	end_date[128 + 1];
	char	query_type[1 + 1];
	char	brchtp_desc[64 + 1];
}app_prdt_brno_t;

/*获取产品机构控制请求信息*/
static int get_app_prdt_brno_req_info(sw_loc_vars_t *vars, app_prdt_brno_t *app_prdt_brno, int index)
{
	char	buf[1024];
	char	path[1024];	

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	memset(app_prdt_brno, 0x0, sizeof(app_prdt_brno));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).PRDT_ID", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->prdt_id, buf, sizeof(app_prdt_brno->prdt_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).PRDT_ID]=[%s]=>[app_prdt_brno->prdt_id]",
			__FILE__, __LINE__, index, buf);
	
	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).BRNO", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->brno, buf, sizeof(app_prdt_brno->brno) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).BRNO]=[%s]=>[app_prdt_brno->brno]",
			__FILE__, __LINE__, index, buf);

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).BRCHTP", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->brchtp, buf, sizeof(app_prdt_brno->brchtp) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).BRCHTP]=[%s]=>[app_prdt_brno->brchtp]",
			__FILE__, __LINE__, index, buf);

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).START_DATE", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->start_date, buf, sizeof(app_prdt_brno->start_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).START_DATE]=[%s]=>[app_prdt_brno->start_date]",
			__FILE__, __LINE__, index, buf);

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).END_DATE", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->end_date, buf, sizeof(app_prdt_brno->end_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).END_DATE]=[%s]=>[app_prdt_brno->end_date]",
			__FILE__, __LINE__, index, buf);

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).DATE_CTRL", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->date_ctrl, buf, sizeof(app_prdt_brno->date_ctrl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).DATE_CTRL]=[%s]=>[app_prdt_brno->date_ctrl]",
			__FILE__, __LINE__, index, buf);

	memset(buf, 0x0, sizeof(buf));
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).QUERY_TYPE", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_prdt_brno->query_type, buf, sizeof(app_prdt_brno->query_type) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Brnos.App_Prdt_Brno(%d).QUERY_TYPE]=[%s]=>[app_prdt_brno->query_type]",
			__FILE__, __LINE__, index, buf);
	return 0;
}

static int set_app_prdt_brno_res_info(sw_loc_vars_t *vars, app_prdt_brno_t *app_prdt_brno, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).PRDT_ID", index);
	loc_set_zd_data(vars, path, app_prdt_brno->prdt_id);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->prdt_id);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).BRNO", index);
	loc_set_zd_data(vars, path, app_prdt_brno->brno);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->brno);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).BRCH_NAME", index);
	loc_set_zd_data(vars, path, app_prdt_brno->brch_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->brch_name);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).BRCHTP", index);
	loc_set_zd_data(vars, path, app_prdt_brno->brchtp);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->brchtp);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).DATE_CTRL", index);
	loc_set_zd_data(vars, path, app_prdt_brno->date_ctrl);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->date_ctrl);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).START_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_brno->start_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->start_date);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).END_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_brno->end_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->end_date);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), ".TradeRecord.Response.App_Prdt_Brnos.App_Prdt_Brno(%d).BRCHTP_DESC", index);
	loc_set_zd_data(vars, path, app_prdt_brno->brchtp_desc);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_brno->brchtp_desc);
	return 0;
}

static int crt_ins_sql(app_prdt_brno_t *app_prdt_brno, char *sql)
{
	char	buf[1024];
	char	setlist[1024];
	char 	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x00, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strncpy(wherelist, "1 = 1", sizeof(wherelist) - 1);
	if(app_prdt_brno->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_brno->prdt_id);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 产品代码为空！", __FILE__, __LINE__);
		return -1;
	}

	if (app_prdt_brno->brno[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and brno = '%s'", app_prdt_brno->brno);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->brchtp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and brchtp = '%s'", app_prdt_brno->brchtp);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, ", start_date = '%s'", app_prdt_brno->start_date);
		strcat(setlist, buf);
	}

	if (app_prdt_brno->end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, ", end_date = '%s'", app_prdt_brno->end_date);
		strcat(setlist, buf);
	}

	if (setlist[0] == '\0')
	{
		pub_log_error("[%s][%d] 设置时间为空!", __FILE__, __LINE__);
		return -1;
	}

	sprintf(sql, "update app_prdt_brno set date_ctrl = '1' %s where %s", setlist, wherelist);
	return 0;
}

static int crt_del_sql(app_prdt_brno_t *app_prdt_brno, char *sql)
{
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x00, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if(app_prdt_brno->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_brno->prdt_id);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] prdt_id 为空.", __FILE__, __LINE__);
		return -1;
	}

	if (app_prdt_brno->brno[0] != '\0')
	{
		memset(buf, 0x00, sizeof(buf));
		sprintf(buf, " and brno = '%s'", app_prdt_brno->brno);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->brchtp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and brchtp = '%s'", app_prdt_brno->brchtp);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, ", start_date = '%s'", app_prdt_brno->start_date);
		strcat(setlist, buf);
	}

	if (app_prdt_brno->end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, ", end_date = '%s'", app_prdt_brno->end_date);
		strcat(setlist, buf);
	}

	sprintf(sql, "update app_prdt_brno set date_ctrl = '0' %s where %s", setlist, wherelist);
	return 0;
}

static int crt_upd_sql(app_prdt_brno_t *app_prdt_brno, char *sql)
{
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_prdt_brno->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_brno->prdt_id);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] 产品代码为空.", __FILE__, __LINE__);
		return -1;
	}

	if (app_prdt_brno->brno[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and brno = '%s'", app_prdt_brno->brno);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->brchtp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and brchtp = '%s'", app_prdt_brno->brchtp);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " start_date = '%s',", app_prdt_brno->start_date);
		strcat(setlist, buf);
	}

	if (app_prdt_brno->end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " end_date = '%s'", app_prdt_brno->end_date);
		strcat(setlist, buf);
	}

	if (setlist[0] == '\0')
	{
		pub_log_error("[%s][%d] setlist 为空.", __FILE__, __LINE__);
		return -1;
	}
	
	sprintf(sql, "update app_prdt_brno set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_prdt_brno_t *app_prdt_brno, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if(app_prdt_brno->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_brno->prdt_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_brno->date_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and date_ctrl = '%s'", app_prdt_brno->date_ctrl);
		strcat(wherelist, buf);
	}

	/*按照机构类型进行查询*/
	if(!strcmp(app_prdt_brno->query_type, "2"))
	{
		sprintf(sql, "select  distinct(brchtp), code_desc, date_ctrl from app_prdt_brno, app_data_dic where \
				%s and brchtp = code_value and code_id = 'brchtp'", wherelist);
	}
	else if(!strcmp(app_prdt_brno->query_type, "1"))
	{
		/*按照机构号进行查询*/
		sprintf(sql, "select brno, brch_name, date_ctrl, start_date, end_date from app_prdt_brno where %s", wherelist);
	}
	else
	{
		pub_log_info("[%s][%d] 输入机构查询类型[%s]错误", __FILE__, __LINE__, app_prdt_brno->query_type);
		return -1;
	}

	return 0;
}

int sp2117(sw_loc_vars_t *vars)
{
	int	ret = 0;
	int 	num = 0;
	char	opt[16];
	char	sql[2048];
	char	buf[128];
	char    errcode[256];
	char    errmsg[256];
	app_prdt_brno_t	app_prdt_brno;

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(buf, 0x0, sizeof(buf));
	memset(errcode, 0x0, sizeof(errcode));
	memset(errmsg, 0x0, sizeof(errmsg));
	memset(&app_prdt_brno, 0x0, sizeof(app_prdt_brno));

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	loc_get_zd_data(vars, ".TradeRecord.Request.BRNO_NUM", buf);
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	num = atoi(buf);

	pub_log_debug("[%s][%d] opt=[%s] num=[%d]", __FILE__, __LINE__, opt, num);
	ret = agt_table_detect("app_prdt_brno");
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 表app_prdt_brno不存在", __FILE__, __LINE__);
		strcpy(errmsg, "表app_prdt_brno不存在");
		goto ErrExit;
	}

	if (opt[0] == 'A')
	{
		pub_log_info("[%s][%d] 插入操作处理开始...", __FILE__, __LINE__);
		get_app_prdt_brno_req_info(vars, &app_prdt_brno, 0);
		ret = crt_ins_sql(&app_prdt_brno, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成插入语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "构造插入语句失败");
			goto ErrExit;
		}

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
	else if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
		get_app_prdt_brno_req_info(vars, &app_prdt_brno, 0);
		ret = crt_del_sql(&app_prdt_brno, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成删除语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "构造删除语句失败");
			goto ErrExit;
		}

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
		get_app_prdt_brno_req_info(vars, &app_prdt_brno, 0);
		ret = crt_upd_sql(&app_prdt_brno, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成更新语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "构造更新语句失败");
			goto ErrExit;
		}

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

		get_app_prdt_brno_req_info(vars, &app_prdt_brno, 0);
		ret = crt_sel_sql(&app_prdt_brno, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d] select sql=[%s]", __FILE__, __LINE__, sql);
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
				pub_log_error("[%s][%d] PageIndex error,pageindex=[%d] pagesum=[%d]", __FILE__, __LINE__, pageidx, pagesum);
				strcpy(errmsg, "页面超出范围");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_prdt_brno", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_prdt_brno");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_prdt_brno");
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

				memset(&app_prdt_brno, 0x0, sizeof(app_prdt_brno));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_prdt_brno", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "prdt_id") == 0)
					{
						strncpy(app_prdt_brno.prdt_id, ptr, sizeof(app_prdt_brno.prdt_id) - 1);
					}

					if (strcmp(name, "brno") == 0)
					{
						strncpy(app_prdt_brno.brno, ptr, sizeof(app_prdt_brno.brno) - 1);
					}

					if (strcmp(name, "brch_name") == 0)
					{
						strncpy(app_prdt_brno.brch_name, ptr, sizeof(app_prdt_brno.brch_name) - 1);
					}

					if (strcmp(name, "brchtp") == 0)
					{
						strncpy(app_prdt_brno.brchtp, ptr, sizeof(app_prdt_brno.brchtp) - 1);
					}

					if (strcmp(name, "date_ctrl") == 0)
					{
						strncpy(app_prdt_brno.date_ctrl, ptr, sizeof(app_prdt_brno.date_ctrl) - 1);
					}

					if (strcmp(name, "start_date") == 0)
					{
						strncpy(app_prdt_brno.start_date, ptr, sizeof(app_prdt_brno.start_date) - 1);
					}

					if (strcmp(name, "end_date") == 0)
					{
						strncpy(app_prdt_brno.end_date, ptr, sizeof(app_prdt_brno.end_date) - 1);
					}
					if (strcmp(name, "code_desc") == 0)
					{
						strncpy(app_prdt_brno.brchtp_desc, ptr, sizeof(app_prdt_brno.brchtp_desc) - 1);
					}
				}
				set_app_prdt_brno_res_info(vars, &app_prdt_brno, index - 1);
			}

			rows = pub_db_mfetch("app_prdt_brno");
			if (rows == 0)
			{
				pub_db_cclose("app_prdt_brno");
				pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_prdt_brno");
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
	else
	{	
		pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2117 处理成功!");
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
	if (errcode[0] !=' \0')
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", errcode);
	}
	else
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "E999");
	}

	if (strlen(errmsg) > 0)
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", errmsg);
	}
	else
	{
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2117 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
