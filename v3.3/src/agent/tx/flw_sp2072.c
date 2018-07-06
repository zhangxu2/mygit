/*************************************************
  文 件 名:  flw_sp2072.c                        **
  功能描述:  安装备份文件                        **
  作    者:  邹佩                                **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

static int back_up(char *id)
{
	int	result = 0;
	char	*home = NULL;
	char	*swwork = NULL;
	char	back_path[256];
	char	cmd[256];
	char	cfg[256];
	char	pdt[256];
	char	back_cfg[256];
	char	back_pdt[256];
	char	path[256];
	FILE	*fp = NULL;

	memset(cfg, 0x00, sizeof(cfg));
	memset(pdt, 0x00, sizeof(pdt));

	home = getenv("HOME");
	swwork = getenv("SWWORK");

	memset(back_path, 0x00, sizeof(back_path));
	sprintf(back_path, "%s/tmp/bp_backup%s", home, id);

	result = pub_file_check_dir(back_path);
	if (result != 0)
	{
		pub_log_error("err: %s, %d, mkpath[%s] error!\n", __FILE__, __LINE__, back_path);
		return -1;
	}

	sprintf(cfg, "%s/cfg", swwork);
	sprintf(back_cfg, "%s/cfg", back_path);

	sprintf(pdt, "%s/products", swwork);
	sprintf(back_pdt, "%s/products", back_path);

	memset(path,0x00,sizeof(path));
	sprintf(path,	"%s/tmp/BACK_UP.out",	home);
	fp = fopen(path,"w+");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] fopen [%s]error!", __FILE__, __LINE__);
		return -1;
	}
	
	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "cp -R %s %s", cfg, back_cfg);
	pub_log_debug("[%s], [%d], cmd[%s]", __FILE__,__LINE__, cmd);
	system(cmd);

	fprintf(fp, "已将 %s 备份至 %s \n", cfg, back_cfg);

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "cp -R %s %s", pdt, back_pdt);
	pub_log_debug("[%s], [%d], cmd[%s]", __FILE__,__LINE__, cmd);
	system(cmd);

	fprintf(fp,"已将 %s 备份至 %s \n", pdt, back_pdt);
	fclose(fp);

	pub_log_info("[%s][%d] 备份结束!!",__FILE__,__LINE__);
	return 0;
}

int sp2072(sw_loc_vars_t *vars)
{
	int	result = 0;
	char	id[32];
	char	type[2];
	char	opt[32];
	char	chk_run[32];
	char	status[16];
	char	reply[8];
	char	res_msg[256];
	char	line[526];
	char	outfile[512];
	FILE	*pf = NULL;
	time_t	t;
	struct tm *nowtime = NULL;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	memset(status, 0x0, sizeof(status));
	memset(opt, 0x0, sizeof(opt));
	memset(type, 0x0, sizeof(type));
	memset(line, 0x0, sizeof(line));
	
	memset(type, 0x0, sizeof(type));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", type);
	pub_log_info("%s, %d, opt[%s] ", __FILE__, __LINE__, type);
	
	if (type[0] != 'B' && type[0] != 'S')
	{
		pub_log_error("[%s][%d] opt is error!", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
	
	memset(chk_run, 0x0, sizeof(chk_run));
	if (type[0] == 'S')
	{
		chk_run[0] = '1';
	}
	
	strcpy(opt, "BACK_UP");
	/*检查安装前备份是否在运行*/
	result = agt_check_inst_stat(vars, opt, chk_run);
	if (result < 0)
	{
		sprintf(res_msg, "获取备份文件状态失败");
		strcpy(reply, "E999");
		goto ErrExit;
	}
	
	if (chk_run[0] == '1')
	{
		memset(outfile, 0x00, sizeof(outfile));
		sprintf(outfile, "%s/tmp/%s.out", getenv("HOME"), opt);
		pf = fopen(outfile, "r");
		if (pf == NULL)
		{
			pub_log_debug("%s, %d,fopen  outfile[%s] failed", __FILE__, __LINE__, outfile);
			strcpy(reply, "E016");
			goto ErrExit;
		}
		
		memset(line,0x00,sizeof(line));
		memset(res_msg,0x00,sizeof(res_msg));
		while (fgets(line, 256, pf) != NULL)
		{
			strcat(res_msg, line);
			strcat(res_msg, "|");
		}
		
		res_msg[strlen(res_msg) - 1] = '\0';
		fclose(pf);
		goto OkExit;
	}
	
	if (result > 0)
	{
		goto OkExit;
	}

	pub_log_info("%s, %d, 触发备份命令!", __FILE__, __LINE__);
	pub_mem_memzero(id, sizeof(id));
	time(&t);
	nowtime = localtime(&t);
	strftime(id, sizeof(id), "%Y%m%d%H%M", nowtime);

	pre_cmd(opt);
	result = back_up(id);
	if (result != 0)
	{
		post_cmd(opt);
		pub_log_error("[%s][%d] 备份失败[%d][%s]", __FILE__, __LINE__, errno, strerror(errno));
		strcpy(reply, "E999");
		strcpy(res_msg, "备份失败");
		goto ErrExit;
	}
	
	post_cmd(opt);
	loc_set_zd_data(vars, ".TradeRecord.Response.BackID", id);
	pub_log_debug("%s, %d, 安装前备份完成!", __FILE__, __LINE__);
OkExit:

	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	if (strlen(res_msg) == 0)
	{
		strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	}
	
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return 0;

ErrExit:
	agt_error_info(reply, res_msg);
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return -1;
}
