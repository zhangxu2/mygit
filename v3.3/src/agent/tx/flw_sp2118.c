/*************************************************
 文 件 名:  flw_sp2118.c                        **
 功能描述:  交易级别渠道控制                    **
 作    者:  邹佩                                **
 完成日期:  20160801                            **
*************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	prdt_id[32 + 1];
	char	prdt_up_id[32 + 1];
	char	sys_code[32 + 1];
	char	tx_code[32 + 1];
	char	chnl_id[32 + 1];
	char	date_ctrl[1 + 1];
	char	start_date[128 + 1];
	char	end_date[128 + 1];
	char	limit_ctrl[1 + 1];
	char	limit_min[128 + 1];
	char	limit_max[128 + 1];
	char	revesal[1 + 1];
	char	chnl_ctrl_sign[1 + 1];
	char	chnl_sign_flag[10 + 1];
	char 	tx_channels[256 + 1];
}app_prdt_chnl_t;

static int get_app_prdt_chnl_req_info(sw_loc_vars_t *vars, app_prdt_chnl_t *app_prdt_chnl)
{
	char	buf[1024];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.PRDT_ID", buf);
	strncpy(app_prdt_chnl->prdt_id, buf, sizeof(app_prdt_chnl->prdt_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.PRDT_ID]=[%s]=>[app_prdt_chnl->prdt_id]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.TX_CODE", buf);
	strncpy(app_prdt_chnl->tx_code, buf, sizeof(app_prdt_chnl->tx_code) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.TX_CODE]=[%s]=>[app_prdt_chnl->tx_code]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.TX_CHANNEL", buf);
	strncpy(app_prdt_chnl->tx_channels, buf, sizeof(app_prdt_chnl->tx_channels) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.TX_CHANNEL]=[%s]=>[app_prdt_chnl->tx_channels]",
		 __FILE__, __LINE__, buf);

	return 0;
}

static int set_app_prdt_chnl_res_info(sw_loc_vars_t *vars, app_prdt_chnl_t *app_prdt_chnl, int index)
{
	int 	i = 0;
	char	path[256];
	char	code_desc[61];
	char 	*ptr = NULL;
	char 	*channels = NULL;

	if (app_prdt_chnl->tx_channels[0] != '\0' && strcmp(app_prdt_chnl->tx_channels, "all") != 0)
	{
		channels = app_prdt_chnl->tx_channels;
		while((ptr = strsep(&channels, "|")) != NULL)
		{
			pub_log_debug("[%s][%d]spritptr==[%s]", __FILE__, __LINE__, ptr);
			
			memset(path, 0x0, sizeof(path));
			sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).TX_CHANNELS(%d).CHNL_ID",index, i);
			loc_set_zd_data(vars, path, ptr);

			memset(code_desc, 0x0, sizeof(code_desc));
			memset(path, 0x0, sizeof(path));
			sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).TX_CHANNELS(%d).CHNL_NAME", index, i); 
			set_app_data_dic_trans("chnltype",ptr, code_desc);
			loc_set_zd_data(vars, path, code_desc);
			i++;
		}
	}

	return 0;
}

static int crt_upd_sql(app_prdt_chnl_t *app_prdt_chnl, char *sql)
{
	char	buf[1024];
	char	values[1024];
	char 	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(values, 0x0, sizeof(values));
	
	if (app_prdt_chnl->tx_code[0] == '\0')
	{
		pub_log_error("[%s][%d] 交易码不能为空!", __FILE__, __LINE__);
		return -1;
	}
	
	memset(wherelist, 0x0, sizeof(wherelist));
	sprintf(wherelist, "1=1"); 	
	if (app_prdt_chnl->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_chnl->prdt_id);
		strcat(wherelist, buf);
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, " and tx_code = '%s'", app_prdt_chnl->tx_code);
	strcat(wherelist, buf);
	
	sprintf(sql, "update app_prdt_tx set tx_channels='%s' where %s", app_prdt_chnl->tx_channels, wherelist); 	
	return 0;
}

static int crt_sel_sql(app_prdt_chnl_t *app_prdt_chnl, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	if (app_prdt_chnl->tx_code[0] == '\0')
	{
		pub_log_error("[%s][%d] 交易码不能为空!", __FILE__, __LINE__);
		return -1;
	}

	strcpy(wherelist, "1 = 1");
	if (app_prdt_chnl->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_chnl->prdt_id);
		strcat(wherelist, buf);
	}
	
	if (app_prdt_chnl->sys_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and sys_code = '%s'", app_prdt_chnl->sys_code);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->tx_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and tx_code = '%s'", app_prdt_chnl->tx_code);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select tx_channels from app_prdt_tx where %s", wherelist);
	return 0;
}

int sp2118(sw_loc_vars_t *vars)
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
	app_prdt_chnl_t	app_prdt_chnl;

	ret = agt_table_detect("app_prdt_tx");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_prdt_tx不存在");
		goto ErrExit;
	}

	pub_log_info("[%s][%d] [%s]app_prdt_tx处理开始...", __FILE__, __LINE__, __FUNCTION__);

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_prdt_chnl, 0x0, sizeof(app_prdt_chnl));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	
	pub_log_info("[%s][%d] 操作标识:opt=[%s]", __FILE__, __LINE__, opt);
	if (opt[0] != 'A' && opt[0] != 'D' && opt[0] != 'S')
	{
		pub_log_error("[%s][%d] 操作标识无效!", __FILE__, __LINE__);
		goto ErrExit;
	}
	
	if (opt[0] == 'D' || opt[0] == 'A')
	{
		pub_log_info("[%s][%d] 更新操作处理开始...", __FILE__, __LINE__);
		get_app_prdt_chnl_req_info(vars, &app_prdt_chnl);
		ret = crt_upd_sql(&app_prdt_chnl, sql);
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
		get_app_prdt_chnl_req_info(vars, &app_prdt_chnl);
		ret = crt_sel_sql(&app_prdt_chnl, sql);
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
				pub_log_error("[%s][%d] 获取行数失败!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] 页面索引[%d]错误! 总页数[%d]", __FILE__, __LINE__, pageidx, pagesum);
				strcpy(errmsg, "页面超出范围");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_prdt_tx", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_prdt_tx");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_prdt_tx");
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

				memset(&app_prdt_chnl, 0x0, sizeof(app_prdt_chnl));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_prdt_tx", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] 字段名=[%s] 字段值=[%s]", __FILE__, __LINE__, name, ptr); 

					if (strcmp(name, "tx_channels") == 0)
					{
						strncpy(app_prdt_chnl.tx_channels, ptr, sizeof(app_prdt_chnl.tx_channels) - 1);
					}
				}
				set_app_prdt_chnl_res_info(vars, &app_prdt_chnl, index - 1);
			}

			rows = pub_db_mfetch("app_prdt_tx");
			if (rows == 0)
			{
				pub_db_cclose("app_prdt_tx");
				pub_log_info("[%s][%d] 查询结束!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_prdt_tx");
				pub_log_error("[%s][%d] 获取失败!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
		}

		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] 查询成功!", __FILE__, __LINE__);
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2118 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2118 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
