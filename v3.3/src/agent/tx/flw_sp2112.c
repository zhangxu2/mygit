/*************************************************
 �� �� ��:  flw_sp2112.c                        **
 ��������:  ά������                            **
 ��    ��:  linpanfei                           **
 �������:  20160802                            **
*************************************************/

#include "agent_comm.h"
#include "pub_db.h"

static	char	errmsg[128];
static	char	errcode[8];
typedef struct
{
	char	dime_no[32 + 1];
	char	parm_value[512 + 1];
	char	parm_dsp[64 + 1];
	char	parm_start_date[60 + 1];
	char	parm_end_date[32 + 1];
	char	parm_sts[1 + 1];
	char 	dime_fmt[32];
}app_dime_parm_t;

static int get_dime_type(char *dime_no)
{
	int  	cols = 0;
	char	name[64];
	char	sql[1024];
	char	*ptr = NULL;

	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select dime_type from app_dime_info where dime_no='%s'", dime_no);
	cols = pub_db_squery(sql);
	if(cols < 0)
	{
		pub_log_error("[%s][%d]��ѯ���ݿ�ʧ��", __FILE__, __LINE__);
		return -1;
	} 
	memset(name, 0x0, sizeof(name));
	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	agt_str_tolower(name);
	pub_str_ziphlspace(ptr);	
	pub_log_debug("[%s][%d]name[%s] ptr=[%s]", __FILE__, __LINE__, name, ptr);
	if (ptr[0] == '$')
	{
		return 0;
	}
	return 1;
}

static int get_app_dime_parm_req_info(sw_loc_vars_t *vars, app_dime_parm_t *app_dime_parm)
{
	int 	ret = -1;
	char	buf[1024];
	char	*ptmp = NULL;
	char	tmp1[32];
	char	tmp2[32];

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.DIME_NO", buf);
	strncpy(app_dime_parm->dime_no, buf, sizeof(app_dime_parm->dime_no) - 1);
	pub_log_info("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.DIME_NO]=[%s]=>[app_dime_parm->dime_no]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.PARM_VALUE", buf);
	strncpy(app_dime_parm->parm_value, buf, sizeof(app_dime_parm->parm_value) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.PARM_VALUE]=[%s]=>[app_dime_parm->parm_value]",
			__FILE__, __LINE__, buf);

	ret = get_dime_type(app_dime_parm->dime_no);
	if (ret == 0)
	{
		ptmp = strchr(buf, '-');
		if (ptmp != NULL)
		{
			*ptmp = '\0';
			memset(tmp1, 0x0, sizeof(tmp1));
			memset(tmp2, 0x0, sizeof(tmp2));
			strcpy(tmp1, buf);
			strcpy(tmp2, ptmp + 1);
			memset(app_dime_parm->parm_value, 0x0, sizeof(app_dime_parm->parm_value));
			snprintf(app_dime_parm->parm_value, sizeof(app_dime_parm->parm_value) - 1, "%012.02lf-%012.02lf", atof(tmp1), atof(tmp2));
		}
	}
	else if (ret == -1)
	{
		pub_log_error("[%s][%d] do get_dime_type error!", __FILE__, __LINE__);
		return -1;
	}

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.PARM_DSP", buf);
	strncpy(app_dime_parm->parm_dsp, buf, sizeof(app_dime_parm->parm_dsp) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.PARM_DSP]=[%s]=>[app_dime_parm->parm_dsp]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.PARM_START_DATE", buf);
	strncpy(app_dime_parm->parm_start_date, buf, sizeof(app_dime_parm->parm_start_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.PARM_START_DATE]=[%s]=>[app_dime_parm->parm_start_date]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.PARM_END_DATE", buf);
	strncpy(app_dime_parm->parm_end_date, buf, sizeof(app_dime_parm->parm_end_date) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.PARM_END_DATE]=[%s]=>[app_dime_parm->parm_end_date]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.PARM_STS", buf);
	strncpy(app_dime_parm->parm_sts, buf, sizeof(app_dime_parm->parm_sts) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.PARM_STS]=[%s]=>[app_dime_parm->parm_sts]",
			__FILE__, __LINE__, buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.Eci_Dime_Parm.DIME_FMT", buf);
	strncpy(app_dime_parm->dime_fmt, buf, sizeof(app_dime_parm->dime_fmt) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.Eci_Dime_Parm.DIME_FMT]=[%s]=>[app_dime_parm->dime_fmt]",
			__FILE__, __LINE__, buf);

	return 0;
}

static int set_app_dime_parm_res_info(sw_loc_vars_t *vars, app_dime_parm_t *app_dime_parm, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Parms.Eci_Dime_Parm(%d).DIME_NO", index);
	loc_set_zd_data(vars, path, app_dime_parm->dime_no);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_parm->dime_no);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Parms.Eci_Dime_Parm(%d).PARM_VALUE", index);
	loc_set_zd_data(vars, path, app_dime_parm->parm_value);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_parm->parm_value);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Parms.Eci_Dime_Parm(%d).PARM_DSP", index);
	loc_set_zd_data(vars, path, app_dime_parm->parm_dsp);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_parm->parm_dsp);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Parms.Eci_Dime_Parm(%d).PARM_START_DATE", index);
	loc_set_zd_data(vars, path, app_dime_parm->parm_start_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_parm->parm_start_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Parms.Eci_Dime_Parm(%d).PARM_END_DATE", index);
	loc_set_zd_data(vars, path, app_dime_parm->parm_end_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_parm->parm_end_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Dime_Parms.Eci_Dime_Parm(%d).PARM_STS", index);
	loc_set_zd_data(vars, path, app_dime_parm->parm_sts);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_dime_parm->parm_sts);

	return 0;
}

static int crt_ins_sql(app_dime_parm_t *app_dime_parm, char *sql)
{
	int		i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	if (app_dime_parm->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_parm->dime_no);
		strcat(values, buf);
		strcat(cols, "dime_no");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ��ά�ȱ��Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ��ά�ȱ��Ϊ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_parm->parm_value);
		strcat(values, buf);
		strcat(cols, "parm_value");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز���ֵΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز���ֵΪ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_dsp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_parm->parm_dsp);
		strcat(values, buf);
		strcat(cols, "parm_dsp");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز���ֵ����Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز���ֵ����Ϊ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_parm->parm_start_date);
		strcat(values, buf);
		strcat(cols, "parm_start_date");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز�����ʼ����Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز�����ʼ����Ϊ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_parm->parm_end_date);
		strcat(values, buf);
		strcat(cols, "parm_end_date");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز�����ֹ����Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز�����ֹ����Ϊ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_dime_parm->parm_sts);
		strcat(values, buf);
		strcat(cols, "parm_sts");
		strcat(cols, ",");
		strcat(values, ",");
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز���״̬Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز���״̬Ϊ��");
		strcpy(errcode, "L004");
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

	sprintf(sql, "insert into app_dime_parm(%s) values(%s)", cols, values);

	return 0;
}

static int crt_del_sql(app_dime_parm_t *app_dime_parm, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_dime_parm->parm_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_value = '%s' ", app_dime_parm->parm_value);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز���ֵΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز���ֵΪ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_no = '%s' ", app_dime_parm->dime_no);
		strcat(wherelist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ��ά�ȱ���Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ��ά�ȱ���Ϊ��");
		strcpy(errcode, "L004");
		return -1;
	}

	sprintf(sql, "delete from app_dime_parm where %s", wherelist);

	return 0;
}

static int crt_upd_sql(app_dime_parm_t *app_dime_parm, char *sql)
{
	int		i = 0;
	char	buf[1024];
	char	setlist[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(setlist, 0x0, sizeof(setlist));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_dime_parm->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " dime_no = '%s',", app_dime_parm->dime_no);
		strcat(setlist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ��ά�ȱ���Ϊ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ��ά�ȱ���Ϊ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " parm_value = '%s',", app_dime_parm->parm_value);
		strcat(setlist, buf);
	}
	else
	{
		pub_log_error("[%s][%d] ҵ��Ҫ�ز���ֵΪ��", __FILE__, __LINE__);
		strcpy(errmsg, "ҵ��Ҫ�ز���ֵΪ��");
		strcpy(errcode, "L004");
		return -1;
	}

	if (app_dime_parm->parm_dsp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " parm_dsp = '%s',", app_dime_parm->parm_dsp);
		strcat(setlist, buf);
	}

	if (app_dime_parm->parm_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " parm_start_date = '%s',", app_dime_parm->parm_start_date);
		strcat(setlist, buf);
	}

	if (app_dime_parm->parm_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " parm_end_date = '%s',", app_dime_parm->parm_end_date);
		strcat(setlist, buf);
	}

	if (app_dime_parm->parm_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " parm_sts = '%s',", app_dime_parm->parm_sts);
		strcat(setlist, buf);
	}

	i = strlen(setlist) - 1;
	while (setlist[i] == ',')
	{
		i--;
	}
	i++;
	setlist[i] = '\0';
	if (app_dime_parm->parm_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_value = '%s'", app_dime_parm->parm_value);
		strcat(wherelist, buf);
	}

	if (app_dime_parm->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_no = '%s'", app_dime_parm->dime_no);
		strcat(wherelist, buf);
	}

	sprintf(sql, "update app_dime_parm set %s where %s", setlist, wherelist);

	return 0;
}

static int crt_sel_sql(app_dime_parm_t *app_dime_parm, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (app_dime_parm->dime_fmt[0] != '\0')
	{
		sprintf(sql, "select a.* from app_dime_parm a,app_dime_info b where a.dime_no=b.dime_no and b.dime_fmt='%s' order by parm_value,a.dime_no", app_dime_parm->dime_fmt);
		return 0;
	}

	if (app_dime_parm->dime_no[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and dime_no = '%s'", app_dime_parm->dime_no);
		strcat(wherelist, buf);
	}

	if (app_dime_parm->parm_value[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_value = '%s'", app_dime_parm->parm_value);
		strcat(wherelist, buf);
	}

	if (app_dime_parm->parm_dsp[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_dsp = '%s'", app_dime_parm->parm_dsp);
		strcat(wherelist, buf);
	}

	if (app_dime_parm->parm_start_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_start_date = '%s'", app_dime_parm->parm_start_date);
		strcat(wherelist, buf);
	}

	if (app_dime_parm->parm_end_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_end_date = '%s'", app_dime_parm->parm_end_date);
		strcat(wherelist, buf);
	}

	if (app_dime_parm->parm_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and parm_sts = '%s'", app_dime_parm->parm_sts);
		strcat(wherelist, buf);
	}

	sprintf(sql, "select * from app_dime_parm where %s order by parm_value,dime_no", wherelist);

	return 0;
}

int sp2112(sw_loc_vars_t *vars)
{
	int		ret = 0;
	char	opt[16];
	char	sql[2048];
	char	tmp[128];
	app_dime_parm_t	app_dime_parm;

	memset(tmp, 0x00, sizeof(tmp));
	memset(errmsg, 0x0, sizeof(errmsg));
	memset(errcode, 0x0, sizeof(errcode));

	ret = agt_table_detect("app_dime_parm");
	if (ret < 0)
	{
		strcpy(errmsg, "��app_dime_parm������");
		goto ErrExit;
	}
	pub_log_info("[%s][%d] [%s]����ʼ...", __FILE__, __LINE__, __FUNCTION__);

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	memset(&app_dime_parm, 0x0, sizeof(app_dime_parm));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);
	if (opt[0] == 'A')
	{
		pub_log_debug("[%s][%d] �����������ʼ...", __FILE__, __LINE__);
		ret = get_app_dime_parm_req_info(vars, &app_dime_parm);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] do get_app_dime_parm_req_info error!", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = crt_ins_sql(&app_dime_parm, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ���ɲ������ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "���ɲ������ʧ��");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] insert sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ�в������ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ִ�в������ʧ��");
			strcpy(errcode, "S002");
			goto ErrExit;
		}
		pub_log_info("[%s][%d] �������ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'D')
	{
		pub_log_debug("[%s][%d] ɾ����������ʼ...", __FILE__, __LINE__);
		ret = get_app_dime_parm_req_info(vars, &app_dime_parm);
		if (ret != 0)
		{
			pub_log_info("[%s][%d] do get_app_dime_parm_req_info error!", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = crt_del_sql(&app_dime_parm, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ����ɾ�����ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "ɾ��ʧ��");
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

	if (opt[0] == 'M')
	{
		pub_log_debug("[%s][%d] �޸Ĳ�������ʼ...", __FILE__, __LINE__);
		ret = get_app_dime_parm_req_info(vars, &app_dime_parm);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] do get_app_dime_parm_req_info error!", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = crt_upd_sql(&app_dime_parm, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ���ɸ������ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "����ʧ��");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] update sql=[%s]", __FILE__, __LINE__, sql);
		ret = pub_db_nquery(sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ִ�и������ʧ��! sql=[%s]", __FILE__, __LINE__, sql);
			strcpy(errmsg, "ִ�и������ʧ��");
			strcpy(errcode, "S003");
			goto ErrExit;
		}

		pub_log_info("[%s][%d] �������ִ�гɹ�! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}

	if (opt[0] == 'S')
	{
		int		i = 0;
		int		j = 0;
		int		ret = -1;
		int 	flag = 0;
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
		pub_log_debug("[%s][%d] ��ѯ��������ʼ...", __FILE__, __LINE__);

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

		ret = get_app_dime_parm_req_info(vars, &app_dime_parm);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] do get_app_dime_parm_req_info error!", __FILE__, __LINE__);
			goto ErrExit;
		}

		flag = get_dime_type(app_dime_parm.dime_no);
		if (flag == -1)
		{
			pub_log_error("[%s][%d] do get_dime_type error!", __FILE__, __LINE__);
			goto ErrExit;
		}

		ret = crt_sel_sql(&app_dime_parm, sql);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ���ɲ�ѯ���ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "��ѯʧ��");
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
				strcpy(errcode, "S005");
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
				strcpy(errmsg, "ҳ�泬����Χ");
				goto ErrExit;
			}
		}

		cols = pub_db_mquery("app_dime_parm", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] ��ѯ���ݿ�ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "��ѯ���ݿ�ʧ��");
			strcpy(errcode, "S001");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_dime_parm");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] �α��ѯ����ʧ��!", __FILE__, __LINE__);
			strcpy(errmsg, "�α��ѯ����ʧ��");
			strcpy(errcode, "S005");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_dime_parm");
			pub_log_error("[%s][%d] ������!", __FILE__, __LINE__);
			strcpy(errmsg, "������");
			strcpy(errcode, "S004");
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

				memset(&app_dime_parm, 0x0, sizeof(app_dime_parm));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_dime_parm", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "dime_no") == 0)
					{
						strncpy(app_dime_parm.dime_no, ptr, sizeof(app_dime_parm.dime_no) - 1);
					}

					if (strcmp(name, "parm_value") == 0)
					{
						strncpy(app_dime_parm.parm_value, ptr, sizeof(app_dime_parm.parm_value) - 1);
					}

					if (strcmp(name, "parm_dsp") == 0)
					{
						strncpy(app_dime_parm.parm_dsp, ptr, sizeof(app_dime_parm.parm_dsp) - 1);
					}

					if (strcmp(name, "parm_start_date") == 0)
					{
						strncpy(app_dime_parm.parm_start_date, ptr, sizeof(app_dime_parm.parm_start_date) - 1);
					}

					if (strcmp(name, "parm_end_date") == 0)
					{
						strncpy(app_dime_parm.parm_end_date, ptr, sizeof(app_dime_parm.parm_end_date) - 1);
					}

					if (strcmp(name, "parm_sts") == 0)
					{
						strncpy(app_dime_parm.parm_sts, ptr, sizeof(app_dime_parm.parm_sts) - 1);
					}
				}
				if (flag == 0)
				{
					char	*ptmp = NULL;
					char	tmp1[32];
					char	tmp2[32];
					ptmp = strchr(app_dime_parm.parm_value, '-');
					if (ptmp != NULL)
					{
						*ptmp = '\0';
						memset(tmp1, 0x0, sizeof(tmp1));
						memset(tmp2, 0x0, sizeof(tmp2));
						strcpy(tmp1, app_dime_parm.parm_value);
						strcpy(tmp2, ptmp+1);
						memset(app_dime_parm.parm_value, 0x0, sizeof(app_dime_parm.parm_value));
						snprintf(app_dime_parm.parm_value, sizeof(app_dime_parm.parm_value) - 1, "%.2lf-%.2lf", atof(tmp1), atof(tmp2));
					}
				}
				set_app_dime_parm_res_info(vars, &app_dime_parm, index - 1);
			}

			rows = pub_db_mfetch("app_dime_parm");
			if (rows == 0)
			{
				pub_db_cclose("app_dime_parm");
				pub_log_info("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_dime_parm");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "�α��ѯ����ʧ��");
				strcpy(errcode, "S005");
				goto ErrExit;
			}
		}	

		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
		pub_log_info("[%s][%d] ��ѯ�ɹ�!", __FILE__, __LINE__);
		goto OkExit;
	}

	pub_log_error("[%s][%d] ������ʶ[%s]����!", __FILE__, __LINE__, opt);
	strcpy(errmsg, "������ʶ����");
	strcpy(errcode, "L002");
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2112 ����ɹ�!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2112 ����ʧ��");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
