/*************************************************
  文 件 名:  flw_sp2001.c                        **
  功能描述:  主机信息                            **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/
 
#include "agent_comm.h"

static int get_file_sys_stats(sw_loc_vars_t * vars)
{
	int 	pageidx = 0;
	int 	pagecnt = 0;
	int 	pagesum = 0;
	int 	i = 0, j = 0;
	float	data_total = 0.0;
	float	data_used = 0.0;
	char	buff[128] = {0};
	char	path[256] = {0};
	char	total_size[32] = {0};
	char	used_size[32] = {0};
	FILE	*fp = NULL;

	static struct proc_stat_s{
		char	mount_point[32];
		char	total_size[32];
		char	used_size[32];
		char	unit[32];
	}proc_stat;

	memset(path, 0x0, sizeof(path));
	memset(buff, 0x0, sizeof(buff));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.PageIndex");
	loc_get_zd_data(vars, path, buff);
	pageidx = atoi(buff);

	memset(path, 0x0, sizeof(path));
	memset(buff, 0x0, sizeof(buff));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.PageCount");
	loc_get_zd_data(vars, path, buff);
	pagecnt = atoi(buff);

	if (pageidx == 0 || pagecnt == 0)
	{
		pagecnt = 1;
	}

	fp = popen("df -mP", "r");
	if(fp == NULL) 
	{
		pub_log_error("[%s][%d] popen error " __FILE__, __LINE__);
		return SW_ERROR;
	}
	/* head */
	fgets(buff, sizeof(buff), fp);

	i = 0;
	j = 0;
	while(!feof(fp)) 
	{
		memset(&proc_stat, 0x0, sizeof(proc_stat));
		memset(total_size, 0x0, sizeof(total_size));
		memset(used_size, 0x0, sizeof(used_size));
		memset(buff, 0x0, sizeof(buff));

		fgets(buff, sizeof(buff), fp);
		if(strlen(buff) <= 0)
			break;

		sscanf(buff, "%*s %s %s %*s %*s %s",
			total_size, used_size, proc_stat.mount_point);
		/* AIX  /proc                 -         -         -       -  /proc*/
		if(strncmp(total_size, "-", 1) == 0) 
			continue;


		data_total = atol(total_size);
		data_used = atol(used_size);
		if(data_total >= 1024) {
			data_total = data_total / 1024;
			sprintf(proc_stat.total_size, "%0.2f", data_total);
			data_used = data_used /1024;
			sprintf(proc_stat.used_size, "%0.2f", data_used);
			strcpy(proc_stat.unit, "GB");
		} else {

			sprintf(proc_stat.total_size, "%0.2f", data_total);
			sprintf(proc_stat.used_size, "%0.2f", data_used);
			strcpy(proc_stat.unit, "MB");
		}

		pub_log_debug("[%s][%d]total_size=%s", __FILE__, __LINE__, proc_stat.total_size);
		pub_log_debug("[%s][%d]used_size=%s",__FILE__, __LINE__, proc_stat.used_size);
		pub_log_debug("[%s][%d]mount_point=%s ", __FILE__, __LINE__,proc_stat.mount_point);

		i++;
		if (pagecnt != 1 && (i <= pagecnt*(pageidx-1) || i > pagecnt*pageidx ))
		{
			continue;
		}
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.FileSystem.Forders.Forder(%d).FileName", j);
		loc_set_zd_data(vars, path, proc_stat.mount_point);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.FileSystem.Forders.Forder(%d).Size", j);
		loc_set_zd_data(vars, path, proc_stat.total_size);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.FileSystem.Forders.Forder(%d).Use", j);
		loc_set_zd_data(vars, path, proc_stat.used_size);
		
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.FileSystem.Forders.Forder(%d).Unit", j);
		loc_set_zd_data(vars, path, proc_stat.unit);
		
		j++;
	}
	fclose(fp);
	pagesum = (i%pagecnt)?(i/pagecnt+1):(i/pagecnt);
	loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);

	return SW_OK;
}

static int get_os_info(sw_loc_vars_t *vars, char *os)
{
	int 	ret = 0;
	char	cmd[512];
	char	buf[512];
	char	xml_name[128];
	char	name[64];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	pub_mem_memzero(xml_name, sizeof(xml_name));
	sprintf(xml_name, "%s/cfg/agentcfg/common/agent_ext.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xml_name);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d]xml[%s]create error!", __FILE__, __LINE__,xml_name);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".MON.ENTRY");
	while (node != NULL)
	{
		if (strcmp(node->name, "ENTRY") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "TXCODE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] not set[TXCODE]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (strcmp(node1->value, "2001") == 0)
		{
			break;
		}

		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		return 0;
	}

	xml->current = node;
	node = pub_xml_locnode(xml, "SYSTEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "SYSTEM") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "OS");
		if (node1 != NULL)
		{
			if (strcmp(node1->value, os) != 0)
			{
				node = node->next;
				continue;
			}
		}

		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node = node1->next;
				continue;
			}

			xml->current = node1;
			node2 = pub_xml_locnode(xml, "NAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_error("[%s][%d]not set NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
			pub_mem_memzero(name, sizeof(name));
			strncpy(name, node2->value, sizeof(name) - 1);

			node2 =  pub_xml_locnode(xml, "CMD");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d]not set VALUE!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
			pub_mem_memzero(cmd, sizeof(cmd));
			strncpy(cmd, node2->value, sizeof(cmd) - 1);
			pub_log_debug("[%s][%d] CMD=[%s]", __FILE__, __LINE__, cmd);

			pub_mem_memzero(buf, sizeof(buf));
			ret = agt_popen(cmd, buf);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] POPEN error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				pub_xml_deltree(xml);
				return SW_ERROR;
			}

			loc_set_zd_data(vars, name, buf);
			pub_log_debug("[%s][%d] POPEN: [%s]=[%s]", __FILE__, __LINE__, name, buf);
			node1 = node1->next;
		}
		node = node->next;
	}
	pub_xml_deltree(xml);

	pub_mem_memzero(xml_name, sizeof(xml_name));
	sprintf(xml_name,"%s/cfg/swconfig.xml",getenv("SWWORK"));

	xml = pub_xml_crtree(xml_name);
	if(xml == NULL)
	{
		pub_log_error("[%s][%d].creat xml tree error  name[%s]", __FILE__, __LINE__, xml_name );
		return SW_ERROR;
	}

	pub_mem_memzero(buf,sizeof(buf));
	sprintf(buf,"%s",".SWCFG.GLOBAL.PROCESS");
	node = pub_xml_locnode(xml,buf);
	if(node != NULL && node->value != NULL && strlen(node->value) > 0) 
	{
		pub_mem_memzero(buf,sizeof(name));
		sprintf(name,".TradeRecord.Response.HostInfo.Process"); 
		loc_set_zd_data(vars, name, node->value );
	}
	pub_xml_deltree(xml);

	pub_log_debug("[%s][%d]Variables are mapped to complete!", __FILE__, __LINE__);

	return SW_OK;
}
			
/*2001*/
int sp2001(sw_loc_vars_t *vars)
{
	int 	ret = 0;
	char	reply[16];
	char	res_msg[256];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0 ,sizeof(res_msg));

	ret = get_file_sys_stats(vars);
	if (ret != 0)
	{    
		pub_log_error("[%s][%d]Take the file system information failure!"
			, __FILE__, __LINE__);

		strcpy(reply, "E003");
		goto ErrExit;
	}

#ifdef LINUX
	ret = get_os_info(vars, "LINUX");
	if (ret != SW_OK)
	{    
		pub_log_error("[%s][%d]get_os_info failure for linux!"
			, __FILE__, __LINE__);
		strcpy(reply, "E004");
		goto ErrExit;
	}
#endif
#ifdef AIX
	int i = 0, j = 0;
	int  num = 0;
	char buf[256];
	char sOpt[16];
	char sZcnt[16];
	process_stats_t	*proc_stats = NULL;
	perfstat_cpu_total_t    cputotal;

	memset(&cputotal, 0x00, sizeof(cputotal));
	ret = perfstat_cpu_total(NULL, &cputotal, sizeof(perfstat_cpu_total_t), 1);
	if (ret == -1)
	{
		pub_log_error("[%s][%d] perfstat_cpu_total error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		strcpy(reply, "E008");
		goto ErrExit;

	}

	pub_mem_memzero(buf, sizeof(buf));
	sprintf(buf, "%lld", cputotal.processorHZ/1000000);
	loc_set_zd_data(vars, ".TradeRecord.Response.HostInfo.CPUFrequency", buf);
	loc_set_zd_data(vars, ".TradeRecord.Response.HostInfo.CPUType", cputotal.description);

	ret = get_os_info(vars, "AIX");
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d]get_os_info failure for aix!"
			, __FILE__, __LINE__);
		strcpy(reply, "E004");
		goto ErrExit;
	}

	proc_stats = (process_stats_t *)agt_get_process_stat(&num);
	if (proc_stats == NULL)
	{
		pub_log_error("[%s][%d] get_process_stats error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));

		strcpy(reply, "E005");
		goto ErrExit;
	}
	for (i = 0; i < num; i++)
	{
		if (proc_stats->state == PROCESS_STATE_ZOMBIE)
		{
			j++;
		}
	}
	memset(sZcnt, 0x00, sizeof(sZcnt));
	sprintf(sZcnt, "%d", j);
	loc_set_zd_data(vars, ".TradeRecord.Response.HostInfo.DieCotentCount", sZcnt);
#endif

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return SW_OK;
ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}

