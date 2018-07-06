
/*************************************************
 文 件 名:  flw_sp2004.c                        **
 功能描述: 本机资源信息查询                     **
 作    者:  薛辉                                **
 完成日期:  20160801                            **
 *************************************************/
 
#include "agent_comm.h"

/*NETWORK*/
static net_percent_stat_t g_net_percent_data;

network_stats_t *get_netwrk_stats(int *num)
{
	network_stats_t	*pnetwork_stats;

#ifndef AIX
	int		i = 0;
	int		entries = 0;
	network_stats_t		*ptr = NULL;
	sg_network_io_stats	*network_info = NULL;

	network_info = sg_get_network_io_stats(&entries);
	if (network_info == NULL)
	{
		pub_log_error("[%s][%d] sg_get_network_io_stats error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	pnetwork_stats = (network_stats_t *)calloc(entries, sizeof(network_stats_t));
	if (pnetwork_stats == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ptr = pnetwork_stats;
	for (i = 0; i < entries; i++)
	{
		strncpy(ptr->interface_name, network_info->interface_name, sizeof(ptr->interface_name) - 1);
		ptr->tx = network_info->tx;
		ptr->rx = network_info->rx;
		ptr->opackets = network_info->ipackets;
		ptr->ipackets = network_info->opackets;
		ptr->oerrors = network_info->ierrors;
		ptr->ierrors = network_info->oerrors;
		ptr->collisions = network_info->collisions;
		ptr->systime = network_info->systime;
		ptr++;
		network_info++;
	}

	*num = entries;
#endif

#ifdef AIX

	int	i = 0;
	int	ret = 0;
	int	tot = 0;
	perfstat_id_t	first;
	network_stats_t	*ptr = NULL;
	perfstat_netinterface_t	*statp = NULL;


	/* check how many perfstat_netinterface_t structures are available */
	tot = perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);
	if (tot < 0)
	{
		pub_log_error("[%s][%d] perfstat_netinterface error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	statp = (perfstat_netinterface_t *)calloc(tot, sizeof(perfstat_netinterface_t));
	if (statp == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	strcpy(first.name, FIRST_NETINTERFACE);

	ret = perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), tot);
	if (ret <= 0) 
	{
		free(statp);
		pub_log_error("[%s][%d] perfstat_netinterface error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	pnetwork_stats = (network_stats_t *)calloc(ret, sizeof(network_stats_t));
	if (pnetwork_stats == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ptr = pnetwork_stats;
	for (i = 0; i < ret; i++)
	{
		strncpy(ptr->interface_name, statp[i].name, sizeof(ptr->interface_name) - 1);
		ptr->tx = statp[i].obytes;
		ptr->rx = statp[i].ibytes;
		ptr->opackets = statp[i].opackets;
		ptr->ipackets = statp[i].ipackets;
		ptr->oerrors = statp[i].oerrors;
		ptr->ierrors = statp[i].ierrors;
		ptr->collisions = statp[i].collisions;
		ptr->systime = time(NULL);

		ptr++;
	}

	free(statp);
	statp = NULL;
	*num = ret;
#endif

	return pnetwork_stats;
}

static int net_percent_stat_function(void *data)
{
	int	i = 0;
	int	num = 0;
	network_stats_t	*net1 = NULL;
	network_stats_t	*net2 = NULL;

	net1 = (network_stats_t *)get_netwrk_stats(&num);
	if (net1 == NULL)
	{
		pub_log_error("[%s][%d] get_network_stats error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	sleep(1);
	net2 = (network_stats_t *)get_netwrk_stats(&num);
	if (net2 == NULL)
	{
		free(net1);
		pub_log_error("[%s][%d] get_network_stats error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	for (i = 0; i < num; i++)
	{
		g_net_percent_data.net_percent[i].sendrate = (net2[i].tx - net1[i].tx) * 8.0 / 1024;
		g_net_percent_data.net_percent[i].acceptrate = (net2[i].rx - net1[i].rx) * 8.0 / 1024;
		g_net_percent_data.net_percent[i].used = (g_net_percent_data.net_percent[i].sendrate + g_net_percent_data.net_percent[i].acceptrate) * 1.0 / 1024; 
		strncpy(g_net_percent_data.net_percent[i].name, net2[i].interface_name, sizeof(g_net_percent_data.net_percent[i].name) - 1);
 		g_net_percent_data.net_percent[i].imbytes = net2[i].rx * 1.0 / (1024 * 1024) ;
		g_net_percent_data.net_percent[i].ombytes = net2[i].tx * 1.0 / (1024 * 1024);
	}
	free(net1);
	net1 = NULL;
	free(net2);
	net2 = NULL;

	g_net_percent_data.net_num = num;
	return 0;
}

net_percent_t *get_net_percent(int *num)
{

	int	nums = 0;
	net_percent_t	net_array[OMS_MAX_NET_NUM];
	net_percent_t	*net_percent = NULL;


	memset(&g_net_percent_data, 0x00, sizeof(g_net_percent_data));
	net_percent_stat_function((void *)&g_net_percent_data);
	memcpy(net_array, g_net_percent_data.net_percent, sizeof(net_percent_t)*g_net_percent_data.net_num);
	nums = g_net_percent_data.net_num;

	net_percent = (net_percent_t *)calloc(nums, sizeof(net_percent_t));
	if (net_percent == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	*num = nums;
	memcpy(net_percent, net_array, sizeof(net_percent_t) * nums);

	return net_percent;
}


static int get_network_stat(sw_loc_vars_t *vars)
{
	int 	pageidx = 0;
	int 	pagecnt = 0;
	int 	pagesum = 0;
	int 	i = 0, j = 0;
	int 	num = 0;
	char	buf[128];
	char	reply[8];
	char	res_msg[256];
	char	name[256];
	char	used[256];
	char	path[256];
	char	send_rate[256];
	char	accpt_rate[256];
	char	imbytes[256];
	char	ombytes[256];
	net_percent_t	*ptr = NULL;
	net_percent_t	*net_percent = NULL;

	pub_mem_memzero(buf, sizeof(buf));
	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_mem_memzero(name, sizeof(name));
	pub_mem_memzero(used, sizeof(used));
	pub_mem_memzero(path, sizeof(path));
	pub_mem_memzero(send_rate, sizeof(send_rate));
	pub_mem_memzero(accpt_rate, sizeof(accpt_rate));

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
	pagecnt = atoi(buf);

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
	pageidx = atoi(buf);

	if (pagecnt == 0 || pageidx == 0)
	{
		pagecnt = 1;
	}
	net_percent = (net_percent_t *)get_net_percent(&num);
	if (net_percent == NULL)
	{
		pub_log_error("[%s][%d] get_net_percent error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d]net num = [%d]",
		__FILE__,__LINE__, num);

	pagesum = (num%pagecnt) ? (num/pagecnt+1) : (num/pagecnt);
	loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", pagesum);

	ptr = net_percent;
	j = 0;
	for (i = 0; i < num; i++)
	{
		pub_mem_memzero(name, sizeof(name));
		strcpy(name, ptr->name);

		pub_mem_memzero(send_rate, sizeof(send_rate));
		sprintf(send_rate, "%.1lfkbps", ptr->sendrate);

		pub_mem_memzero(accpt_rate, sizeof(accpt_rate));
		sprintf(accpt_rate, "%.1lfkbps", ptr->acceptrate);

		pub_mem_memzero(used, sizeof(used));
		sprintf(used, "%.1lf%%", ptr->used);

		pub_mem_memzero(imbytes, sizeof(imbytes));
		sprintf(imbytes, "%.1lfMB", ptr->imbytes);

		pub_mem_memzero(ombytes, sizeof(ombytes));
		sprintf(ombytes, "%.1lfMB", ptr->ombytes);

		pub_log_debug("[%s][%d] sSendingRate=[%s]", __FILE__, __LINE__, send_rate);
		pub_log_debug("[%s][%d] accpt_rate=[%s]", __FILE__, __LINE__, accpt_rate);
		pub_log_debug("[%s][%d] used=[%s]", __FILE__, __LINE__, used);

		if (pagecnt != 1 && (i < pagecnt*(pageidx-1) || i >= pagecnt*pageidx))
		{
			continue;
		}
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.NetWorks.NetWork(%d).Name", j);
		loc_set_zd_data(vars, path, name);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.NetWorks.NetWork(%d).SendingRate", j);
		loc_set_zd_data(vars, path, send_rate);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.NetWorks.NetWork(%d).Acceptrate", j);
		loc_set_zd_data(vars, path, accpt_rate);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.NetWorks.NetWork(%d).Utilization", j);
		loc_set_zd_data(vars, path, used);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.NetWorks.NetWork(%d).Imbytes", j);
		loc_set_zd_data(vars, path, imbytes);

		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.NetWorks.NetWork(%d).Ombytes", j);
		loc_set_zd_data(vars, path, ombytes);
		j++;
		ptr++;
	}

	free(net_percent);

	return SW_OK;
}

/*2004*/
int sp2004(sw_loc_vars_t *vars)
{
	int	ret = SW_ERROR;
	char	res_msg[256];
	char	reply[16];
	
	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	ret = get_network_stat(vars);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Take the network information failure!"
			, __FILE__, __LINE__);
		strcpy(reply, "E006");
		goto ErrExit;
	}


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
