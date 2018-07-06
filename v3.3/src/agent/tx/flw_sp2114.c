/*************************************************
 �� �� ��:  flw_sp2114.c                        **
 ��������:  ��Ʒ���������                      **
 ��    ��:  ����                                **
 �������:  20160801                                    **
*************************************************/

#include "agent_comm.h"
#include "pub_db.h"

typedef struct
{
	char	prdt_id[32 + 1];
	char	sys_code[32 + 1];
	char	tx_code[32 + 1];
	char	chnl_id[32 + 1];
	char	date_ctrl[1 + 1];
	char	start_date[128 + 1];
	char	end_date[128 + 1];
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
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.CHNL_ID", buf);
	strncpy(app_prdt_chnl->chnl_id, buf, sizeof(app_prdt_chnl->chnl_id) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.CHNL_ID]=[%s]=>[app_prdt_chnl->chnl_id]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.DATE_CTRL", buf);
	strncpy(app_prdt_chnl->date_ctrl, buf, sizeof(app_prdt_chnl->date_ctrl) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.DATE_CTRL]=[%s]=>[app_prdt_chnl->date_ctrl]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.START_DATE", buf);
	strncpy(app_prdt_chnl->start_date, buf, sizeof(app_prdt_chnl->start_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.START_DATE]=[%s]=>[app_prdt_chnl->start_date]",
		 __FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.App_Prdt_Chnl.END_DATE", buf);
	strncpy(app_prdt_chnl->end_date, buf, sizeof(app_prdt_chnl->end_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.App_Prdt_Chnl.END_DATE]=[%s]=>[app_prdt_chnl->end_date]",
		 __FILE__, __LINE__, buf);

	return 0;
}

static int set_app_prdt_chnl_res_info(sw_loc_vars_t *vars, app_prdt_chnl_t *app_prdt_chnl, int index)
{
	char	path[256];
	char	code_desc[61];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).PRDT_ID", index);
	loc_set_zd_data(vars, path, app_prdt_chnl->prdt_id);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_chnl->prdt_id);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).SYS_CODE", index);
	loc_set_zd_data(vars, path, app_prdt_chnl->sys_code);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_chnl->sys_code);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).CHNL_ID", index);
	loc_set_zd_data(vars, path, app_prdt_chnl->chnl_id);
	
	memset(code_desc, 0x0, sizeof(code_desc));
	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).CHNL_NAME", index); 
	set_app_data_dic_trans("chnltype",app_prdt_chnl->chnl_id, code_desc);
	loc_set_zd_data(vars, path, code_desc);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).DATE_CTRL", index);
	memset(code_desc, 0x0, sizeof(code_desc));
	set_app_data_dic_trans("date_ctrl",app_prdt_chnl->date_ctrl, code_desc);
	loc_set_zd_data(vars, path, code_desc);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).START_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_chnl->start_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_chnl->start_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.App_Prdt_Chnls.App_Prdt_Chnl(%d).END_DATE", index);
	loc_set_zd_data(vars, path, app_prdt_chnl->end_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_prdt_chnl->end_date);

	return 0;
}

static int crt_ins_sql(app_prdt_chnl_t *app_prdt_chnl, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));
	memset(setlist, 0x0, sizeof(setlist));	
	
	strcpy(wherelist, "1 = 1");
	if (app_prdt_chnl->chnl_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and chnl_id = '%s' ", app_prdt_chnl->chnl_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s' ", app_prdt_chnl->prdt_id);
		strcat(wherelist, buf);
	}
	
	if (app_prdt_chnl->start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " start_date = %s,", app_prdt_chnl->start_date);
		strcat(setlist, buf);
	}

	if (app_prdt_chnl->end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " end_date = %s,", app_prdt_chnl->end_date);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';
	
	sprintf(sql, "update app_prdt_chnl set date_ctrl='1', %s where %s", setlist, wherelist);
	return 0;
}

static int crt_del_sql(app_prdt_chnl_t *app_prdt_chnl, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_prdt_chnl->chnl_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and chnl_id = '%s' ", app_prdt_chnl->chnl_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s' ", app_prdt_chnl->prdt_id);
		strcat(wherelist, buf);
	}
	
	sprintf(sql, "update app_prdt_chnl set date_ctrl='0' where %s", wherelist);
	return 0;
}

static int crt_upd_sql(app_prdt_chnl_t *app_prdt_chnl, char *sql)
{
	int	i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_prdt_chnl->date_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " date_ctrl = '%s',", app_prdt_chnl->date_ctrl);
		strcat(setlist, buf);
	}

	if (app_prdt_chnl->start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " start_date = %s,", app_prdt_chnl->start_date);
		strcat(setlist, buf);
	}

	if (app_prdt_chnl->end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " end_date = %s,", app_prdt_chnl->end_date);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';

	if (app_prdt_chnl->sys_code[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and sys_code = '%s'", app_prdt_chnl->sys_code);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->chnl_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and chnl_id = '%s'", app_prdt_chnl->chnl_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->prdt_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and prdt_id = '%s'", app_prdt_chnl->prdt_id);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_prdt_chnl set %s where %s", setlist, wherelist);
	return 0;
}

static int crt_sel_sql(app_prdt_chnl_t *app_prdt_chnl, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

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

	if (app_prdt_chnl->chnl_id[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and chnl_id = '%s'", app_prdt_chnl->chnl_id);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->date_ctrl[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and date_ctrl = '%s'", app_prdt_chnl->date_ctrl);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and start_date = %s", app_prdt_chnl->start_date);
		strcat(wherelist, buf);
	}

	if (app_prdt_chnl->end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and end_date = %s", app_prdt_chnl->end_date);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select prdt_id, chnl_id, start_date, end_date, date_ctrl from app_prdt_chnl where %s", wherelist);

	return 0;
}

int sp2114(sw_loc_vars_t *vars)
{
	int	ret = 0;
	char	opt[16];
	char	sql[2048];
	char	tmp[128];
	char	errcode[256];
	char	errmsg[256];

	memset(errcode, 0x00, sizeof(errcode));
	memset(errmsg, 0x0, sizeof(errmsg));

	memset(tmp, 0x00, sizeof(tmp));
	app_prdt_chnl_t	app_prdt_chnl;

	ret = agt_table_detect("app_prdt_chnl");
	if (ret < 0)
	{
		strcpy(errmsg, "��app_prdt_chnl������");
		goto ErrExit;
	}

	pub_log_info("[%s][%d] [%s]app_prdt_chnl����ʼ...", __FILE__, __LINE__, __FUNCTION__);

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_prdt_chnl, 0x0, sizeof(app_prdt_chnl));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	
	pub_log_info("[%s][%d] ������ʶ:opt=[%s]", __FILE__, __LINE__, opt);
	
	if (opt[0] != 'M' && opt[0] != 'S' && opt[0] != 'A' && opt[0] != 'D')
	{
		pub_log_error("������ʶ[%s]����", __FILE__, __LINE__, opt);
		strcpy(errmsg, "������ʶ����");
		goto ErrExit;
	}
	
	if (opt[0] == 'A')
	{
		pub_log_info("[%s][%d] �����������ʼ...", __FILE__, __LINE__);
		get_app_prdt_chnl_req_info(vars, &app_prdt_chnl);
		ret = crt_ins_sql(&app_prdt_chnl, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ���ɲ������ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "����ʧ��");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]�������sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ�в������ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "����ʧ��");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] �������ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] ɾ����������ʼ...", __FILE__, __LINE__);
		get_app_prdt_chnl_req_info(vars, &app_prdt_chnl);
		ret = crt_del_sql(&app_prdt_chnl, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ����ɾ�����ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "ɾ��ʧ��");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]ɾ�����sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ��ɾ�����ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ɾ��ʧ��");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] ɾ�����ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}
	
	if (opt[0] == 'M')
	{
		pub_log_info("[%s][%d] �޸Ĳ�������ʼ...", __FILE__, __LINE__);
		get_app_prdt_chnl_req_info(vars, &app_prdt_chnl);
		ret = crt_upd_sql(&app_prdt_chnl, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ���ɸ������ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "����ʧ��");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]�������sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ��ɾ�����ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "����ʧ��");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] �������ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
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
		
		pub_log_info("[%s][%d] ��ѯ��������ʼ...", __FILE__, __LINE__);

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
			pub_log_error("[%s][%d] ���ɲ�ѯ���ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "��ѯʧ��");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d]��ѯ���sql=[%s]", __FILE__, __LINE__, sql);
		if (pagecnt > 0)
		{
			ttlcnt = pub_db_get_fetrows(sql);
			if (ttlcnt < 0)
			{
				pub_log_error("[%s][%d] ��ȡ����ʧ��!", __FILE__, __LINE__);
				strcpy(errmsg, "��ѯʧ��");
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
				pub_log_error("[%s][%d] ҳ������[%d]����! ��ҳ��[%d]", __FILE__, __LINE__, pageidx, pagesum);
				strcpy(errmsg, "ҳ�泬����Χ");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_prdt_chnl", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] ��ѯ���ݿ�ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "��ѯʧ��");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_prdt_chnl");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] ��ѯ���ݿ�ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "��ѯʧ��");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_prdt_chnl");
			pub_log_error("[%s][%d] ������!", __FILE__, __LINE__);
			strcpy(errmsg, "������");
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
					ptr = pub_db_get_data_and_name("app_prdt_chnl", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] �ֶ���=[%s] �ֶ�ֵ=[%s]", __FILE__, __LINE__, name, ptr); 
					if (strcmp(name, "prdt_id") == 0)
					{
						strncpy(app_prdt_chnl.prdt_id, ptr, sizeof(app_prdt_chnl.prdt_id) - 1);
					}

					if (strcmp(name, "sys_code") == 0)
					{
						strncpy(app_prdt_chnl.sys_code, ptr, sizeof(app_prdt_chnl.sys_code) - 1);
					}

					if (strcmp(name, "chnl_id") == 0)
					{
						strncpy(app_prdt_chnl.chnl_id, ptr, sizeof(app_prdt_chnl.chnl_id) - 1);
					}

					if (strcmp(name, "date_ctrl") == 0)
					{
						strncpy(app_prdt_chnl.date_ctrl, ptr, sizeof(app_prdt_chnl.date_ctrl) - 1);
					}

					if (strcmp(name, "start_date") == 0)
					{
						strncpy(app_prdt_chnl.start_date, ptr, sizeof(app_prdt_chnl.start_date) - 1);
					}

					if (strcmp(name, "end_date") == 0)
					{
						strncpy(app_prdt_chnl.end_date, ptr, sizeof(app_prdt_chnl.end_date) - 1);
					}
				}
				set_app_prdt_chnl_res_info(vars, &app_prdt_chnl, index - 1);
			}

			rows = pub_db_mfetch("app_prdt_chnl");
			if (rows == 0)
			{
				pub_db_cclose("app_prdt_chnl");
				pub_log_info("[%s][%d] ��ѯ����!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_prdt_chnl");
				pub_log_error("[%s][%d] ��ȡʧ��!", __FILE__, __LINE__);
				strcpy(errmsg, "��ѯʧ��");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] ��ѯ�ɹ�!", __FILE__, __LINE__);
		goto OkExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2114 ����ɹ�!");
	if (opt[0] != 'S')
	{
		ret = record_oper_log(vars, errcode, errmsg);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] �Ǽ���ˮ����", __FILE__, __LINE__);
			strcpy(errmsg, "�Ǽǲ�����¼ʧ��");
			goto ErrExit;
		}
	}
	ret = pub_db_commit();
	if (ret)
	{
		pub_log_error("[%s][%d] �ύ���ݿ�ʧ��!", __FILE__, __LINE__);
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2114 ����ʧ��");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
