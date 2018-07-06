/*************************************************
  文 件 名:  flw_sp2107.c                       **
  功能描述:  根据维度查询维度组合               **
  作    者:  linpanfei                          **
  完成日期:  20160802                           **
 *************************************************/

#include "agent_comm.h"
#include "pub_db.h"

static int 	count = 0;
typedef struct
{
	char	dime_no[16 + 1];
	char	dime_name[60 + 1];
	char	dr_dsp[60 + 1];
	char	dime_val[60 + 1];
}app_dime_parm_t;

static int get_app_dime_parm_req_info(sw_loc_vars_t *vars, app_dime_parm_t *app_dime_parm, char * dime_num)
{
	int 	i = 0;
	char	buf[1024];
	char	path[1024];

	for (i = 0; i < atoi(dime_num); i++)
	{
		memset(buf, 0x00, sizeof(buf));
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Request.Dime_All_Infos.Dime_All_Info(%d).DIME_NO", i);
		loc_get_zd_data(vars, path, buf);
		strncpy(app_dime_parm[i].dime_no, buf, sizeof(app_dime_parm[i].dime_no) - 1);
		pub_log_debug("[%s][%d] get:[%s]=[%s]=>[[%d]->dime_no]", __FILE__, __LINE__, path, buf, i);
	}

	return 0;
}

static int set_app_dime_parm_res_info(sw_loc_vars_t *vars, app_dime_parm_t *app_dime_parm, int m, int n)
{
	int 	i = 0;	
	char	path[1024];

	for (i = 0; i <= m; i++)
	{
		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Alls.Dime_All_Infos(%d).Dime_All_Info(%d).DIME_NO", n, i);
		loc_set_zd_data(vars, path, app_dime_parm[i].dime_no);
		pub_log_debug("[%s][%d] set:[%s]=[app_dime_parm->dime_no]=[%s]", __FILE__, __LINE__, path, app_dime_parm[i].dime_no);	

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Alls.Dime_All_Infos(%d).Dime_All_Info(%d).DIME_VAL", n, i);
		loc_set_zd_data(vars, path, app_dime_parm[i].dime_val);
		pub_log_debug("[%s][%d] set:[%s]=[app_dime_parm->dime_val]=[%s]", __FILE__, __LINE__, path, app_dime_parm[i].dime_val);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Alls.Dime_All_Infos(%d).Dime_All_Info(%d).DR_DSP", n, i);
		loc_set_zd_data(vars, path, app_dime_parm[i].dr_dsp);
		pub_log_debug("[%s][%d] set:[%s]=[app_dime_parm->dr_dsp]=[%s]", __FILE__, __LINE__, path, app_dime_parm[i].dr_dsp);

		memset(path, 0x0, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Eci_Alls.Dime_All_Infos(%d).Dime_All_Info(%d).DIME_NAME", n, i);
		loc_set_zd_data(vars, path, app_dime_parm[i].dime_name);
		pub_log_debug("[%s][%d] set:[%s]=[app_dime_parm->dime_name]=[%s]", __FILE__, __LINE__, path, app_dime_parm[i].dime_name);
	}

	return 0;
}

static int dime_group(sw_loc_vars_t *vars, app_dime_parm_t * app_dime_parm, int index)
{
	int 	i = 0;
	int 	j = 0;
	int 	ret = 0;
	int 	cols = 0;
	int 	rows = 0;
	char	sql[1024];
	char	name[256];
	char	alias[64];
	char	*ptr = NULL;

	if (app_dime_parm[index].dime_no[0] != '\0')
	{
		memset(alias, 0x0, sizeof(alias));
		memset(name, 0x0, sizeof(name));
		memset(sql, 0x0, sizeof(sql));

		sprintf(sql, "select a.dime_no,a.dime_name,b.parm_value,b.parm_dsp from app_dime_info a,app_dime_parm b where 1=1 and a.dime_no=b.dime_no and a.dime_no='%s'", app_dime_parm[index].dime_no);
		pub_log_info("[%s][%d]sql=[%s]", __FILE__, __LINE__, sql);
		sprintf(alias, "app_dime_parm%d", index);

		cols = pub_db_mquery(alias, sql, 10);
		if (cols < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			return -1;
		}

		rows = pub_db_mfetch(alias);
		if (rows < 0)
		{
			pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
			return -1;
		}

		if (rows == 0)
		{
			pub_db_cclose(alias);
			pub_log_error("[%s][%d] 无数据!", __FILE__, __LINE__);
			return -2;
		}
		for (i = 0; i < rows; i++)
		{
			for (j = 0; j < cols; j++)
			{
				memset(name, 0x0, sizeof(name));
				ptr = pub_db_get_data_and_name(alias, i + 1, j + 1, name, sizeof(name));
				agt_str_tolower(name);
				pub_str_ziphlspace(ptr);
				
				if (strcmp(name, "parm_value") == 0)
				{
					strncpy(app_dime_parm[index].dime_val, ptr, sizeof(app_dime_parm[index].dime_val));
				}
				if (strcmp(name, "parm_dsp") == 0)
				{
					strncpy(app_dime_parm[index].dr_dsp, ptr, sizeof(app_dime_parm[index].dr_dsp));
				}
				if (strcmp(name, "dime_name") == 0)
				{
					strncpy(app_dime_parm[index].dime_name, ptr, sizeof(app_dime_parm[index].dime_name));
				}
				if (strcmp(name, "dime_no") == 0)
				{
					strncpy(app_dime_parm[index].dime_no, ptr, sizeof(app_dime_parm[index].dime_no));
				}
			}
			pub_log_info("[%s][%d]index[%d]parm_value[%s]parm_dsp[%s]dime_name[%s]dime_no[%s]", __FILE__, __LINE__, app_dime_parm[index].dime_val, app_dime_parm[index].dr_dsp, app_dime_parm[index].dime_name, app_dime_parm[index].dime_no);

			if (app_dime_parm[index + 1].dime_no[0] == '\0')
			{
				set_app_dime_parm_res_info(vars, app_dime_parm, index, count);
				count++;
			}
			else
			{
				ret = dime_group(vars, app_dime_parm, index + 1);
				if (ret == -2)
				{
					pub_log_error("[%s][%d] dime_group() 查询无数据", __FILE__, __LINE__);
					return -2;
				}
				if (ret < 0)
				{
					pub_log_error("[%s][%d] dime_group error, index=[%d]", __FILE__, __LINE__, index + 1);
					return -1;
				}
			}
		}
		pub_db_cclose(alias);

	}
	return 0;
}

int sp2107(sw_loc_vars_t *vars)
{
	int 	index=0;
	int 	ret = 0;
	char	opt[16];
	char	sql[2048];
	char	buf[256];
	char	dime_num[256];
	char	errcode[256];
	char	errmsg[256];
	app_dime_parm_t app_dime_parm[256];

	memset(errcode, 0x0, sizeof(errcode));
	memset(errmsg, 0x0, sizeof(errmsg));
	memset(opt, 0x0, sizeof(opt));
	memset(dime_num, 0x0, sizeof(dime_num));
	memset(app_dime_parm, 0x0, sizeof(app_dime_parm));

	ret = agt_table_detect("app_dime_parm");
	if (ret < 0)
	{
		strcpy(errmsg, "表app_dime_parm不存在");
		pub_log_info("[%s][%d] %s", __FILE__, __LINE__, errmsg);
		goto ErrExit;
	}
	pub_log_info("[%s][%d] [%s]处理开始...", __FILE__, __LINE__, __FUNCTION__);

	memset(opt, 0x0, sizeof(opt));
	memset(sql, 0x0, sizeof(sql));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d] opt=[%s]", __FILE__, __LINE__, opt);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.DIME_NUM", buf);
	strncpy(dime_num, buf, sizeof(dime_num) - 1);
	pub_log_debug("[%s][%d] get:[.TradeRecord.Request.DIME_NUM=[%s]=>[dime_num]",
			__FILE__, __LINE__, buf);

	if (opt[0] == 'S')
	{
		get_app_dime_parm_req_info(vars, app_dime_parm, dime_num);	
		ret = dime_group(vars, app_dime_parm, index);
		if (ret == -2)
		{
			strcpy(errmsg, "无数据");
			pub_log_error("[%s][%d] 数据库查询无数据!", __FILE__, __LINE__);
			goto ErrExit;
		}
		if (ret < 0)
		{
			strcpy(errmsg, "查询失败");
			pub_log_error("[%s][%d] 查询错误！", __FILE__, __LINE__);
			goto ErrExit;
		}
		pub_log_info("[%s][%d] success!", __FILE__, __LINE__);
		goto OkExit;
	}

	pub_log_error("[%s][%d] 操作标识[%s]有误!", __FILE__, __LINE__, opt);
	strcpy(errcode, "L002");
	strcpy(errmsg, "操作类型错误");
	goto ErrExit;

OkExit:
	pub_log_info("[%s][%d] [%s] OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2107 处理成功!");
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
	if (strlen(errcode) > 0)
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
		loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "2107 处理失败");
	}
	pub_db_rollback();
	pub_db_del_all_conn();
	return -1;
}
