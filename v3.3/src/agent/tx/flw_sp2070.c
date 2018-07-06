/*************************************************
  文 件 名:  flw_sp2070.c                        **
  功能描述:  检测是否有产品在发布                **
  作    者:  邹佩                                **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

static int  stok(sw_loc_vars_t *vars)
{
	int	index = 0;
	int	i = 0;
	int	file_num = 0;
	char	path[256];
	char	file_dir[256];
	struct dirent	**namelist = NULL;

	memset(file_dir,0x00,sizeof(file_dir));
	sprintf(file_dir,"%s/tmp",getenv("HOME"));
	if (0 != access(file_dir, F_OK))
	{
		pub_log_info ("[%s][%d] not exsist the file [%s]\n", __FILE__, __LINE__, file_dir);
		return 0;
	}

	i  = file_num = 0;
	namelist = agt_scan_dir(&file_num, file_dir);
	i = file_num - 1;
	while (i >= 0)
	{
		if (strncmp(namelist[i]->d_name, "bp_backup", strlen("bp_backup")) != 0)
		{
			i--;
			continue;
		}
		memset(path,0x00,sizeof(path));
		sprintf(path,".TradeRecord.Response.BackIDs.BackID(%d).ID", index);
		loc_set_zd_data(vars, path, namelist[i]->d_name + 9);
		pub_log_debug("[%s][%d] dir_ent->d_name=[%s]",__FILE__,__LINE__, namelist[i]->d_name + 9);
		index++;
		i--;
	}
	agt_free_namelist(namelist, file_num);
	return 0;
}

int sp2070(sw_loc_vars_t *vars)
{
	int	result = 0;
	char	opt[32];
	char	id[64];
	char	type[8];
	char	*home = NULL;
	char	*swhome = NULL;
	char	back_path[256];
	char	status[16];
	char	reply[16];
	char	filename[256];
	char	res_msg[256];
	char	flock_dir[256];
	FILE    *fp = NULL;

	memset(filename, 0x0, sizeof(filename));
	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	memset(opt, 0x0, sizeof(opt));
	memset(type, 0x0, sizeof(type));
	memset(status, 0x0, sizeof(status));
	
	memset(type, 0x0, sizeof(type));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", type);

	memset(flock_dir, 0x0, sizeof(flock_dir));
	sprintf(flock_dir, "%s/tmp/flock", getenv("HOME"));
	pub_file_check_dir(flock_dir);
	sprintf(filename, "%s/tmp/flock/CHECK_DEPLOY_CONFLICT.loc", getenv("HOME"));
	if (!access(filename, F_OK))
	{
		pub_log_info("[%s][%d]产品正在发布中", __FILE__, __LINE__);
		strcpy(reply, "E999");
		strcpy(res_msg, "产品正在发布中");
		goto ErrExit;
	}

	if (type[0] != 'S' && type[0] != 'R')
	{
		pub_log_info("[%s][%d]操作标识[%s]有误", __FILE__, __LINE__, type);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	if (type[0] == 'S')
	{
		pub_log_info("[%s][%d] 获取备份文件开始.....",__FILE__,__LINE__);
		result = stok(vars);
		if (result < 0)
		{
			pub_log_error("[%s][%d] 获取备份文件失败",__FILE__,__LINE__);
			sprintf(res_msg, "获取备份文件失败");
			strcpy(reply, "E999");
			goto ErrExit;
		}
	}
	else 
	{
		pub_mem_memzero(id, sizeof(id));
		loc_get_zd_data(vars, ".TradeRecord.Request.BackID.ID", id);

		home = getenv("HOME");
		swhome = getenv("SWHOME");

		/*检查恢复目录是否存在*/
		pub_mem_memzero(back_path, sizeof(back_path));
		sprintf(back_path, "%s/tmp/bp_backup%s", home, id);
		result = access(back_path, F_OK);
		if (result != 0)
		{
			pub_log_error("%s, %d, 备份目录[%s]不存在, error[%d][%s]."
					, __FILE__, __LINE__, back_path, errno, strerror(errno));
			sprintf(res_msg, "备份目录不存在，备份ID[%s]!", id);
			strcpy(reply, "E999");
			goto ErrExit;
		}

		/*触发备份恢复*/
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] 打开文件失败", __FILE__, __LINE__);
			strcpy(reply, "E999");
			strcpy(res_msg, "创建文件失败");
			goto ErrExit;
		}
		fclose(fp);
		result = restore(id);
		if (result != 0)
		{
			pub_log_error("err: %s, %d, restore id[%s] error!\n", __FILE__, __LINE__, id);
			strcpy(reply, "E010");
			remove(filename);
			goto ErrExit;
		}
		
		remove(filename);

		pub_log_info("%s, %d, 备份恢复完成!", __FILE__, __LINE__);
	}
OkExit:

	pub_log_debug("[%s][%d] [%s]OK EXIT", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return 0;

ErrExit:
	agt_error_info(reply, res_msg);
	pub_log_debug("[%s][%d] [%s]ERR EXIT", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return -1;
}
