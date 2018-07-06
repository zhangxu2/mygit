/*************************************************
  �� �� ��:  flw_sp2101.c                        **
  ��������:  �����ֵ����                        **
  ��    ��:  Ѧ��                                **
  �������:  20160801                            **
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

/*��ȡ�����ֵ����������Ϣ*/
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

/*���������ֵ��ѯ��Ӧ����Ϣ*/
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
		pub_log_error("[%s][%d] ҵ��Ҫ�ر����ʶΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ر����ʶΪ��");
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
		pub_log_error("[%s][%d] ҵ��Ҫ�ر�������Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ر�������Ϊ��");
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
		pub_log_error("[%s][%d] ҵ��Ҫ�ر���ֵΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ر���ֵΪ��");
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
		pub_log_error("[%s][%d] ҵ��Ԫ�ر�������Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ԫ�ر�������Ϊ��");
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
		pub_log_error("[%s][%d] ҵ��Ԫ�ر���״̬Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ԫ�ر���״̬Ϊ��");
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
		pub_log_error("[%s][%d] ҵ��Ԫ�ر����ʶΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ԫ�ر����ʶΪ��");
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
		pub_log_error("[%s][%d] ҵ��Ԫ�ر���ֵΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ԫ�ر���ֵΪ��");
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
		pub_log_error("[%s][%d] ҵ��Ԫ�ر����ʶΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ԫ�ر����ʶΪ��");
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
		pub_log_error("[%s][%d] ҵ��Ԫ�ر���ֵΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ԫ�ر���ֵΪ��");
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

	pub_log_info("[%s][%d] [%s]����ʼ...", __FILE__, __LINE__, __FUNCTION__);

	ret = agt_table_detect("app_data_dic");
	if (ret < 0)
	{
		pub_log_error("[%s][%d]������", __FILE__, __LINE__);
		strcpy(errmsg, "��app_data_dic������");
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
			pub_log_error("[%s][%d] ִ�в������ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ִ�в������ʧ��");
			goto ErrExit;
		}
		else if (ret > 0)
		{
			pub_log_error("[%s][%d] �����Ѵ���! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "�����Ѵ���");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] �����������ʼ...", __FILE__, __LINE__);
		ret = crt_ins_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql�������ʧ��");
			pub_log_error("[%s][%d] ���ɲ������ʧ��!", __FILE__, __LINE__);
			goto ErrExit;
		}

		pub_log_info("[%s][%d] insert sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ�в������ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ִ�в������ʧ��");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] �������ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}
	else if (opt[0] == 'D')
	{
		pub_log_info("[%s][%d] ɾ����������ʼ...", __FILE__, __LINE__);
		get_app_data_dic_req_info(vars, &app_data_dic);
		ret = crt_del_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql�������ʧ��");
			pub_log_error("[%s][%d] ����ɾ�����ʧ��!", __FILE__, __LINE__);
			goto ErrExit;
		}

		pub_log_info("[%s][%d] delete sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ��ɾ�����ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ִ��ɾ�����ʧ��");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] ɾ�����ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}
	else if (opt[0] == 'M')
	{
		pub_log_info("[%s][%d] �޸Ĳ�������ʼ...", __FILE__, __LINE__);
		get_app_data_dic_req_info(vars, &app_data_dic);
		ret = crt_upd_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql�������ʧ��");
			pub_log_error("[%s][%d] ���ɸ������ʧ��!", __FILE__, __LINE__);
			goto ErrExit;
		}

		pub_log_info("[%s][%d] update sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ�и������ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ִ�и������ʧ��");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] �������ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
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
			flag = 1;
		}
		pub_log_debug("[%s][%d] pagecount=[%d] pageindex=[%d]", __FILE__, __LINE__, pagecnt, pageidx);

		get_app_data_dic_req_info(vars, &app_data_dic);
		ret = crt_sel_sql(&app_data_dic, sql);
		if (ret < 0)
		{
			strcpy(errmsg, "sql�������ʧ��");
			pub_log_error("[%s][%d] ���ɲ�ѯ���ʧ��!", __FILE__, __LINE__);
			goto ErrExit;
		}

		pub_log_info("[%s][%d] select sql=[%s]", __FILE__, __LINE__, sql);
		if (pagecnt > 0)
		{
			ttlcnt = pub_db_get_fetrows(sql);
			if (ttlcnt < 0)
			{
				pub_log_error("[%s][%d] Get fetch rows error!", __FILE__, __LINE__);
				strcpy(errmsg, "�α��ѯ����ʧ��");
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
				strcpy(errmsg, "������");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_data_dic", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] ��ѯ���ݿ�ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "��ѯ���ݿ�ʧ��");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_data_dic");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] �α��ѯ����ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "�α��ѯ����ʧ��");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_log_error("[%s][%d] ������!", __FILE__, __LINE__);
			pub_db_cclose("app_data_dic");
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
				strcpy(errmsg, "�α��ѯ����ʧ��");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", ttlcnt);
		pub_log_info("[%s][%d] ��ѯ�ɹ�!", __FILE__, __LINE__);
		goto OkExit;
	}
	else
	{
		pub_log_error("[%s][%d] ������ʶ[%s]����!", __FILE__, __LINE__, opt);
		strcpy(errmsg, "������ʶ����");
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2101 ����ɹ�!");
	ret = pub_db_commit();
	if (ret)
	{
		pub_log_error("[%s][%d] �ύ���ݿ�ʧ��!", __FILE__, __LINE__);
		strcpy(errmsg, "�ύ���ݿ�ʧ��");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2101 ����ʧ��");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
