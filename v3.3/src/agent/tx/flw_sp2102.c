/*************************************************
  文 件 名:  flw_sp2102.c                        **
  功能描述:  节假日信息管理                      **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

static	char 	errmsg[128];
static	char	errcode[8];

typedef struct
{
	char 	dates[2048];
	char	hld_year[4 + 1];
	char	hld_date[128 + 1];
	char	hld_week[10 + 1];
	char	hld_ydays[128 + 1];
	char	hld_mdays[128 + 1];
	char	hld_sts[1 + 1];
	char	hld_name[32 + 1];
}app_holiday_info_t;

typedef struct 
{
	char 	date_sat[8];
	char 	date_sun[8];
}date_sat_sun_t;

/****判断是否为闰年?***/
static int is_leap_year(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/*获取某月天数*/
static int getmonthdays(int year, int month)
{
	switch(month)
	{
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
		case 2:
			return 28 + is_leap_year(year);
		default:
			return 31;
	}
}

/**********计算该天为一年中的第几天****************/
static int dm(int year, int month, int day)
{
	int a = 0;

	switch(month - 1)
	{
		case 11 :a += 30;
		case 10 :a += 31;
		case 9 :a += 30;
		case 8 :a += 31;
		case 7 :a += 31;
		case 6 :a += 30;
		case 5 :a += 31;
		case 4 :a += 30;
		case 3 :a += 31;
		case 2 :a += 28 + is_leap_year(year);
		case 1 :a += 31;
				break;
	}
	a += day;
	
	return a;
}

/**********计算该天为周几***/
static int calcluate_weekday(int y, int m, int d)
{
	if (m == 1 || m == 2) 
	{
		m += 12;
		y--;
	}

	return ( d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
}

/*获取节假日请求信息*/
static int get_app_holiday_info_req_info(sw_loc_vars_t *vars, app_holiday_info_t *app_holiday_info, int index)
{
	char	buf[1024];
	char	path[256];

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).DATES", index);
	loc_get_zd_data(vars,path, app_holiday_info->dates);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path,app_holiday_info->dates);

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_DATE", index);
	loc_get_zd_data(vars, path, buf);
	strncpy(app_holiday_info->hld_date, buf, sizeof(app_holiday_info->hld_date) - 1);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_date);

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_WEEK", index);
	loc_get_zd_data(vars,path, buf);
	strncpy(app_holiday_info->hld_week, buf, sizeof(app_holiday_info->hld_week) - 1);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_week);

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_YDAYS", index);
	loc_get_zd_data(vars,path, buf);
	strncpy(app_holiday_info->hld_ydays, buf, sizeof(app_holiday_info->hld_ydays) - 1);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_ydays);

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_MDAYS", index);
	loc_get_zd_data(vars,path, buf);
	strncpy(app_holiday_info->hld_mdays, buf, sizeof(app_holiday_info->hld_mdays) - 1);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_mdays);

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_STS", index);
	loc_get_zd_data(vars,path, buf);
	strncpy(app_holiday_info->hld_sts, buf, sizeof(app_holiday_info->hld_sts) - 1);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_sts);

	memset(buf, 0x0, sizeof(buf));
	sprintf(path, ".TradeRecord.Request.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_NAME", index);
	loc_get_zd_data(vars,path, buf);
	strncpy(app_holiday_info->hld_name, buf, sizeof(app_holiday_info->hld_name) - 1);
	pub_log_debug("[%s][%d] get:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_name);

	return 0;
}

/*设置节假日查询的应答信息*/
static int set_app_holiday_info_res_info(sw_loc_vars_t *vars, app_holiday_info_t *app_holiday_info, int index)
{
	char	path[256];

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_DATE", index);
	loc_set_zd_data(vars, path, app_holiday_info->hld_date);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_date);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_WEEK", index);
	loc_set_zd_data(vars, path, app_holiday_info->hld_week);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_week);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_YDAYS", index);
	loc_set_zd_data(vars, path, app_holiday_info->hld_ydays);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_ydays);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_MDAYS", index);
	loc_set_zd_data(vars, path, app_holiday_info->hld_mdays);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_mdays);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_STS", index);
	loc_set_zd_data(vars, path, app_holiday_info->hld_sts);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_sts);

	memset(path, 0x0, sizeof(path));
	sprintf(path, ".TradeRecord.Response.Eci_Holiday_Infos.Eci_Holiday_Info(%d).HLD_NAME", index);
	loc_set_zd_data(vars, path, app_holiday_info->hld_name);
	pub_log_debug("[%s][%d] set:[%s]=[%s]", __FILE__, __LINE__, path, app_holiday_info->hld_name);

	return 0;
}

static int crt_ins_sql(app_holiday_info_t *app_holiday_info, char *year, char *sql)
{
	int 	i = 0;
	char	buf[1024];
	char	cols[1024];
	char	values[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(cols, 0x0, sizeof(cols));
	memset(values, 0x0, sizeof(values));
	
	if (year[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", year);
		strcat(values, buf);
		strcat(cols, "hld_year");
		strcat(cols, ",");
		strcat(values, ",");
	}
	if (app_holiday_info->hld_date[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_holiday_info->hld_date);
		strcat(values, buf);
		strcat(cols, "hld_date");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_holiday_info->hld_week[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_holiday_info->hld_week);
		strcat(values, buf);
		strcat(cols, "hld_week");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_holiday_info->hld_ydays[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_holiday_info->hld_ydays);
		strcat(values, buf);
		strcat(cols, "hld_ydays");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_holiday_info->hld_mdays[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s", app_holiday_info->hld_mdays);
		strcat(values, buf);
		strcat(cols, "hld_mdays");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_holiday_info->hld_sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_holiday_info->hld_sts);
		strcat(values, buf);
		strcat(cols, "hld_sts");
		strcat(cols, ",");
		strcat(values, ",");
	}

	if (app_holiday_info->hld_name[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "'%s'", app_holiday_info->hld_name);
		strcat(values, buf);
		strcat(cols, "hld_name");
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

	sprintf(sql, "insert into app_holiday_info(%s) values(%s)", cols, values);

	return 0;
}

/*删除某年周末*/
static int crt_del_sql_1(app_holiday_info_t *app_holiday_info,char * year,char *sts, char *sql)
{
	char	buf[1024];
	char	wherelist[1024];

	memset (buf, 0x0, sizeof(buf));
	memset (wherelist, 0x0, sizeof(wherelist));
	strcpy(wherelist, " 1 = 1 ");

	if (year[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and hld_year = '%s'",year);
		strcat(wherelist, buf);
	}

	if (sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and hld_sts = '%s' ", sts);
		strcat(wherelist, buf);
	}

	sprintf(sql, "delete from app_holiday_info where %s",wherelist);
	return 0;
}

/*删除某年节日*/
static int crt_del_sql_2(char *hld_date,app_holiday_info_t *app_holiday_info,char * year, char *sql)
{
	sprintf(sql, "delete from app_holiday_info where 1=1 and hld_date = '%s' and hld_name = '%s' and hld_year = '%s'",
			hld_date, app_holiday_info->hld_name, year);

	return 0;
}

/*查询周末*/
static int crt_sel_sql_1(app_holiday_info_t *app_holiday_info,char * year,char *sts, char *sql)
{
	char    buf[1024];
	char    wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");

	if (year[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and hld_year = '%s'",year);
		strcat(wherelist, buf);
	}

	if (sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and hld_sts = '%s'",sts);
		strcat(wherelist, buf);
	}
	sprintf(sql, "select * from app_holiday_info where %s order by hld_date", wherelist);

	return 0;
}

/*查询节假日*/
static int crt_sel_sql_2(app_holiday_info_t *app_holiday_info,char * year,char *sts, char *sql)
{
	char    buf[1024];
	char    wherelist[1024];

	memset(buf, 0x0, sizeof(buf));
	memset(wherelist, 0x0, sizeof(wherelist));

	strcpy(wherelist, "1 = 1");
	if (year[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and hld_year = '%s'",year);
		strcat(wherelist, buf);
	}
	if (sts[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, " and hld_sts = '%s'",sts);
		strcat(wherelist, buf);
	}
	sprintf(sql, "select * from app_holiday_info where %s order by hld_date", wherelist);

	return 0;
}


int sp2102(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	int 	inum = 0;
	char	opt[16];
	char	sql[2048];
	char 	year[5];
	char	sts[2];
	char 	holiday_num[64];
	app_holiday_info_t	app_holiday_info;
	date_sat_sun_t 		date_sat_sun;

	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);
	memset(errcode, 0x0, sizeof(errcode));
	memset(errmsg, 0x0, sizeof(errmsg));

	ret = agt_table_detect("app_holiday_info");
	if (ret < 0)
	{
		pub_log_error("[%s][%d]表不存在", __FILE__, __LINE__);
		strcpy(errmsg, "表app_holiday_info不存在");
		goto ErrExit;
	}
	
	memset(sql, 0x0, sizeof(sql));
	memset(&app_holiday_info, 0x0, sizeof(app_holiday_info));
	memset(&date_sat_sun, 0x0, sizeof(date_sat_sun));

	memset(holiday_num, 0x00, sizeof(holiday_num));
	loc_get_zd_data(vars, ".TradeRecord.Request.Holiday_Num",holiday_num);
	pub_log_debug("[%s][%d] holiday_num=[%s]", __FILE__, __LINE__, holiday_num);
	inum=atoi(holiday_num);

	memset(opt, 0x0, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_debug("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);

	memset(year, 0x0, sizeof(year));
	loc_get_zd_data(vars, ".TradeRecord.Request.Year", year);
	pub_log_debug("[%s][%d] year=[%s]", __FILE__, __LINE__,year);

	memset(sts, 0x0, sizeof(sts));
	loc_get_zd_data(vars, ".TradeRecord.Request.Hld_Sts", sts);
	pub_log_debug("[%s][%d] sts=[%s]", __FILE__, __LINE__, sts);

	if(strcmp(opt,"A") == 0 && sts[0] == '0')
	{
		int 	iyear = 0;
		int 	ndays = 0;
		int 	num = 0;
		int 	i = 0;
		int 	j = 0;
		int     cols = 0;
		int     rows = 0;
		int 	week = 0;

		sprintf(sql,"select * from app_holiday_info where 1=1 and hld_year='%s'and hld_sts='%s'",year,sts);

		cols = pub_db_mquery("app_holiday_week", sql, 1);
		if(cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败！\n", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}
		
		rows = pub_db_mfetch("app_holiday_week");
		if(rows < 0)
		{
			pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
			strcpy(errmsg, "查询失败");
			goto ErrExit;
		}
		if(rows == 0)
		{
			pub_db_cclose("app_holiday_week");
			pub_log_error("[%s][%d] 该年未设置周末，设置周末开始\n", __FILE__, __LINE__);

			sscanf(year,"%4d",&iyear);
			for (i = 1; i <= 12; i++)
			{
				ndays = getmonthdays(iyear, i);
				for(j = 1; j <= ndays; j++)
				{
					week = calcluate_weekday(iyear, i, j);
					if(week == 5)
					{
						num = dm(iyear, i, j);
						sprintf(app_holiday_info.hld_date, "%4d%02d%02d", iyear, i, j);
						strcpy(app_holiday_info.hld_week, "星期六");
						sprintf(app_holiday_info.hld_ydays, "%d", num);
						sprintf(app_holiday_info.hld_mdays, "%d", j);
						strcpy(app_holiday_info.hld_sts, "0");
						strcpy(app_holiday_info.hld_name, "0");	
						memset(sql, 0x0, sizeof(sql));
						ret = crt_ins_sql(&app_holiday_info, year, sql);
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
							strcpy(errmsg, "执行插入语句失败");
							goto ErrExit;
						}
					}
					if(week == 6)
					{
						num = dm(iyear, i, j);
						sprintf(app_holiday_info.hld_date, "%4d%02d%02d", iyear, i, j);
						strcpy(app_holiday_info.hld_week, "星期日");
						sprintf(app_holiday_info.hld_ydays, "%d", num);
						sprintf(app_holiday_info.hld_mdays, "%d", j);
						strcpy(app_holiday_info.hld_sts, "0");
						strcpy(app_holiday_info.hld_name, "1");	
						memset(sql, 0x0, sizeof(sql));
						ret = crt_ins_sql(&app_holiday_info, year, sql);
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
							strcpy(errmsg, "执行插入语句失败");
							goto ErrExit;
						}
					}
				}
			}
		}
		else if(rows > 0)
		{
			sprintf(errmsg, "%s年周末已设置", year);
			goto ErrExit; 	
		}
		pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
		goto OkExit;
	}	
	else if(strcmp(opt,"A") == 0 && sts[0] == '1')
	{
		int 	num = 0;
		int 	iyear = 0;
		int 	iday = 0;
		int 	imonth = 0;
		int 	i = 0 ;
		int 	cols = 0;
		int 	rows = 0;
		int 	week = 0;
		char 	*p = NULL;
		char 	*pr = NULL;

		while(i < inum)
		{
			memset(&app_holiday_info, 0x0, sizeof(app_holiday_info));
			get_app_holiday_info_req_info(vars, &app_holiday_info, i);

			memset(sql, 0x0, sizeof(sql));
			sprintf(sql, "select * from app_holiday_info where 1=1 and hld_name='%s'and hld_sts='%s' and hld_year='%s'",\
					app_holiday_info.hld_name, sts, year);

			cols = pub_db_mquery("app_holiday_hld", sql, 100);
			if(cols < 0)
			{
				pub_log_error("[%s][%d] 查询数据库失败！\n", __FILE__, __LINE__);
				strcpy(errmsg, "查询数据库失败");
				goto ErrExit;
			}
			rows = pub_db_mfetch("app_holiday_hld");
			if(rows < 0)
			{
				pub_log_error("[%s][%d] 游标查询数据失败", __FILE__, __LINE__);
				strcpy(errmsg, "游标查询数据失败");
				goto ErrExit;
			}
			if(rows == 0)
			{	
				pub_db_cclose("app_holiday_hld");
				pub_log_error("[%s][%d] 未查到重复节假日,节假日设置开始。\n", __FILE__, __LINE__);

				memset(sql,0x0,sizeof(sql));
				p = app_holiday_info.dates;
				while((pr = strsep(&p,"|")) != NULL && strlen(pr) != 0)
				{	
					sscanf(pr,"%4d%02d%02d", &iyear, &imonth, &iday);

					num = dm(iyear, imonth, iday);
					week = calcluate_weekday(iyear, imonth, iday);
					sprintf(app_holiday_info.hld_date, "%4d%02d%02d", iyear, imonth, iday);
					
					switch(week)
					{
						case 0: strcpy(app_holiday_info.hld_week, "星期一"); break;
						case 1: strcpy(app_holiday_info.hld_week, "星期二"); break;
						case 2: strcpy(app_holiday_info.hld_week, "星期三"); break;
						case 3: strcpy(app_holiday_info.hld_week, "星期四"); break;
						case 4: strcpy(app_holiday_info.hld_week, "星期五"); break;
						case 5: strcpy(app_holiday_info.hld_week, "星期六"); break;
						case 6: strcpy(app_holiday_info.hld_week, "星期日"); break;
						defalut: break;
					}

					sprintf(app_holiday_info.hld_ydays, "%d", num);
					sprintf(app_holiday_info.hld_mdays, "%d", iday);
					strcpy(app_holiday_info.hld_sts, "1");

					ret = crt_ins_sql(&app_holiday_info, year, sql);
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
						strcpy(errmsg, "执行插入语句失败");
						goto ErrExit;
					}
					ret = pub_db_commit();
					if (ret)
					{
						pub_log_error("[%s][%d] 提交数据库失败!", __FILE__, __LINE__);
						strcpy(errmsg, "提交数据库失败!!");
						goto ErrExit;
					}
				}		
				i++;
			}
			else
			{
				sprintf(errmsg, "%s年%s已设置", year, app_holiday_info.hld_name);
				goto ErrExit;
			}
			pub_log_info("[%s][%d] 插入语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);

		}
		goto OkExit;
	}
	else if (strcmp(opt,"D") == 0)
	{
		int 	i = 0;
		char 	*p = NULL;
		char 	*pr = NULL;
		
		if(sts[0] == '0')
		{
			pub_log_info("[%s][%d] 删除操作处理开始...", __FILE__, __LINE__);
			ret = crt_del_sql_1(&app_holiday_info, year, sts, sql);
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
				strcpy(errmsg, "执行删除语句失败");
				goto ErrExit;
			}
			pub_log_info("[%s][%d] 删除语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
			goto OkExit;
		}
		else if(sts[0] == '1')
		{
			while (i < inum)
			{
				memset(&app_holiday_info, 0x0, sizeof(app_holiday_info));
				get_app_holiday_info_req_info(vars, &app_holiday_info, i);

				p = app_holiday_info.dates;			

				while((pr = strsep(&p,"|")) != NULL && strlen(pr) != 0)
				{	

					memset(sql, 0x0, sizeof(sql));
					ret = crt_del_sql_2(pr, &app_holiday_info, year, sql);
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
						strcpy(errmsg, "执行删除语句失败");
						goto ErrExit;
					}
				}
				i++;
			}

			pub_log_info("[%s][%d] 删除语句执行成功! sql=[%s]", __FILE__, __LINE__, sql);
			goto OkExit;
		}
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
		if(sts[0] == '0')
		{
			ret = crt_sel_sql_1(&app_holiday_info, year, sts, sql);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
		}
		else if (sts[0] == '1')
		{
			ret = crt_sel_sql_2(&app_holiday_info, year, sts, sql);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] 生成查询语句失败!", __FILE__, __LINE__);
				strcpy(errmsg, "查询失败");
				goto ErrExit;
			}
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

		cols = pub_db_mquery("app_holiday_info", sql, 100);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "查询数据库失败");
			goto ErrExit;
		}

		rows = pub_db_mfetch("app_holiday_info");
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			strcpy(errmsg, "游标查询数据失败");
			goto ErrExit;
		}

		if (rows == 0)
		{
			pub_db_cclose("app_holiday_info");
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
				if (pagecnt > 0 && index > pagecnt && flag == 0)
				{
					break;
				}

				memset(&app_holiday_info, 0x0, sizeof(app_holiday_info));
				for (j = 0; j < cols; j++)
				{
					memset(name, 0x0, sizeof(name));
					ptr = pub_db_get_data_and_name("app_holiday_info", i + 1, j + 1, name, sizeof(name));
					agt_str_tolower(name);
					pub_str_ziphlspace(ptr);
					pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

					if (strcmp(name, "hld_date") == 0)
					{
						strncpy(app_holiday_info.hld_date, ptr, sizeof(app_holiday_info.hld_date) - 1);
					}

					if (strcmp(name, "hld_week") == 0)
					{
						strncpy(app_holiday_info.hld_week, ptr, sizeof(app_holiday_info.hld_week) - 1);
					}

					if (strcmp(name, "hld_ydays") == 0)
					{
						strncpy(app_holiday_info.hld_ydays, ptr, sizeof(app_holiday_info.hld_ydays) - 1);
					}

					if (strcmp(name, "hld_mdays") == 0)
					{
						strncpy(app_holiday_info.hld_mdays, ptr, sizeof(app_holiday_info.hld_mdays) - 1);
					}

					if (strcmp(name, "hld_sts") == 0)
					{
						strncpy(app_holiday_info.hld_sts, ptr, sizeof(app_holiday_info.hld_sts) - 1);
					}

					if (strcmp(name, "hld_name") == 0)
					{
						strncpy(app_holiday_info.hld_name, ptr, sizeof(app_holiday_info.hld_name) - 1);
					}
				}
				set_app_holiday_info_res_info(vars, &app_holiday_info, index - 1);
			}

			rows = pub_db_mfetch("app_holiday_info");
			if (rows == 0)
			{
				pub_db_cclose("app_holiday_info");
				pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
				break;
			}
			else if (rows < 0)
			{
				pub_db_cclose("app_holiday_info");
				pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
				strcpy(errmsg, "游标查询数据失败");
				goto ErrExit;
			}
		}	
		loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);
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
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2102 处理成功!");
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2102 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
