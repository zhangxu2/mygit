#include <pwd.h>
#include "alert_errno.h"
#include "agent_work.h"
#include "agent_comm.h"
#include "lsn_pub.h"
#include "alert.h"

#define PROC_INFO_NAME  "procinfo.txt"
#define MEM_INFO_NAME   "meminfo.txt"
#define CPU_INFO_NAME   "cpuinfo.txt"
#define MTYPE_INFO_NAME	"mtypeinfo.txt"

int	g_mem_use_cnt;
int	g_cpu_use_cnt;
int	g_mtype_use_cnt;
static	proc_warn	proc_warns[1024];
static	cpu_usage_stat_t	g_cpu_usage_data;
static	net_percent_stat_t	g_net_percent_data;

static int check_file_exist(int use, char *name)
{
	char	path[128];

	if (use)
	{
		return 0;
	}
	else
	{
		memset(path, 0x00, sizeof(path));	
		snprintf(path, 128, "%s/tmp/agtmon/%s", getenv("SWWORK"), name);
		if (pub_file_exist(path))
		{
			remove(path);
		}

		return 1;
	}
}
static int check_file_status(sw_agt_cfg_t *cfg, char *name)
{
	int 	ret = 0;
	char	filename[128];
	static int	frist = 1;
	static time_t	file_time = 0;
	struct stat	file_stat;

	memset(filename, 0x00, sizeof(filename));
	sprintf(filename, "%s/cfg/agentcfg/agent.xml", getenv("SWWORK"));
	ret = stat(filename, &file_stat);
	if (ret < 0)
	{
		if (errno == 2)
		{
			return -2;
		}

		pub_log_error("[%s][%d]stat error! errno=[%d]:[%s]", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	if (frist == 0)
	{
		if (file_time != file_stat.st_mtime)
		{
			frist = 1;
			pub_log_info("[%s][%d] cfg agent.xml already modify, need reload.", __FILE__, __LINE__);
			ret = agt_cfg_init(cfg, name);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] agent cfg init error.", __FILE__, __LINE__);
				return -1;
			}

		}
	}

	if (frist == 1)
	{
		file_time = file_stat.st_mtime;	
		frist = 0;
	}

	return 0;

}

static int tx_code_map(char *xmlname, char *srccode, char *dstcode)
{
	int	ret = 0;
	char	filename[128];
	char	extcode[32];
	char	incode[32];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(filename, 0x00, sizeof(filename));
	snprintf(filename, sizeof(filename), "%s/cfg/agentcfg/common/%s", getenv("SWWORK"), xmlname);
	ret = access(filename, F_OK);
	if (ret != 0)
	{
		strcpy(dstcode, srccode);
		pub_log_info("[%s][%d] not have cfg, not need map.", __FILE__, __LINE__);
		return 0;
	}

	xml = pub_xml_crtree(filename);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xml tree error.", __FILE__, __LINE__);
		return -1;
	}

	node = pub_xml_locnode(xml, ".MAP.ITEM");
	while(node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		memset(incode, 0x00, sizeof(incode));
		memset(extcode, 0x00, sizeof(extcode));
		node1 = pub_xml_locnode(xml, "EXTCODE");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(extcode, node1->value, strlen(node1->value));
		}

		node1 = pub_xml_locnode(xml, "INCODE");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(incode, node1->value, strlen(node1->value));
		}

		if ((strcmp(extcode, srccode) == 0) && (incode[0] != '\0'))
		{
			strncpy(dstcode, incode, strlen(incode));
			return 0;
		}

		node = node->next;
	}

	strcpy(dstcode, srccode);
	return 0;

}
typedef struct dlhandle_cache_s
{
	void	*dlhandle;
	char	txcode[32];
}dlhandle_cache_t;
#define MAX_TXCODE	200

static void * get_dlhandle_from_cache(char *txcode)
{
	int i = 0;
	static int count = 0;
	static dlhandle_cache_t dlhandle_cache[MAX_TXCODE];
	void *dlhandle = NULL;
	char lib_name[128];

	if (count == 0)
	{
		memset(dlhandle_cache, 0x0, sizeof(dlhandle_cache));
	}

	for (i = 0; i < count; ++i)
	{
		if (strcmp(dlhandle_cache[i].txcode, txcode) == 0)
		{
			return dlhandle_cache[i].dlhandle;
		}
	}

	if (count == MAX_TXCODE)
	{
		pub_log_error("[%s][%d] too many txcode MAX[%d]!", __FILE__, __LINE__, MAX_TXCODE);
		return NULL;
	}

	memset(lib_name, 0x0, sizeof(lib_name));
	sprintf(lib_name, "%s/agent_lib/flw_sp%s.so", getenv("SWHOME"), txcode);
	pub_log_debug("[%s][%d] lib_name=[%s]", __FILE__, __LINE__, lib_name);

	if (access(lib_name, F_OK))
	{
		pub_log_error("[%s][%d]the lib_name=[%s] is not exist!", __FILE__, __LINE__, lib_name);
		return NULL;
	}

	dlhandle = (void *)dlopen(lib_name, RTLD_LAZY | RTLD_GLOBAL);
	if (dlhandle == NULL)
	{
		pub_log_error("[%s][%d] dlopen [%s] error! error:[%s]",
				__FILE__, __LINE__, lib_name, dlerror());
		return NULL;
	}

	strncpy(dlhandle_cache[count].txcode, txcode, sizeof(dlhandle_cache[count].txcode));
	dlhandle_cache[count].dlhandle = dlhandle;
	++count;
	return dlhandle;
}
static int do_tx_func(char *name, sw_loc_vars_t *vars, char *tx_code)
{
	int	ret = 0;
	char	lib[64];
	char	txcode[64];
	char	func_name[128];
	void	*dl_handle = NULL;
	int	(* dl_func)(sw_loc_vars_t*);

	pub_mem_memzero(lib, sizeof(lib));

	/*获取交易码*/
	loc_get_zd_data(vars, ".TradeRecord.Header.System.TradeCode", tx_code);
	pub_log_debug("[%s][%d] tx_code=[%s]", __FILE__, __LINE__, tx_code);
	pub_log_debug("[%s][%d]name=[%s]", __FILE__, __LINE__, name);

	memset(txcode, 0x00, sizeof(txcode));
	ret = tx_code_map(name, tx_code, txcode);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] txcode map error.", __FILE__, __LINE__);
		return -1;
	}

	dl_handle = get_dlhandle_from_cache(txcode);
	if (dl_handle == NULL)
	{
		pub_log_error("[%s][%d] dlopen [%s] error! ",
				__FILE__, __LINE__, txcode);
		return -1;
	}

	pub_mem_memzero(func_name, sizeof(func_name));
	sprintf(func_name, "sp%s", txcode);
	pub_log_debug("[%s][%d] func_name=[%s]", __FILE__, __LINE__, func_name);

	dl_func = (int (*)(sw_loc_vars_t*))dlsym(dl_handle, func_name);
	if (dl_func == NULL)
	{
		pub_log_error("[%s][%d] dlsym [%s][%s] error! error:[%s]",
				__FILE__, __LINE__, txcode, func_name, dlerror());
		dlclose(dl_handle);
		return -1;
	}

	ret = dl_func(vars);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 交易[%s]处理失败!", __FILE__, __LINE__, tx_code);
	}
	else
	{
		pub_log_debug("[%s][%d] 交易[%s]处理成功!", __FILE__, __LINE__, tx_code);
	}

	return 0;
}

int agt_work(sw_fd_list_t *fd_lists)
{
	int	ret = 0;
	int	sockid = 0;
	char	tx_code[8];
	char	xmlname[128];
	sw_buf_t	pkgbuf;
	sw_loc_vars_t	vars;
	sw_xmltree_t	*xml = NULL;
	sw_agt_cfg_t	*cfg = NULL;
	agt_cycle_t	*cycle = NULL;

	ret = pub_buf_init(&pkgbuf);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] buf init error.",__FILE__, __LINE__);
		return -1;
	}

	cycle = (agt_cycle_t *)fd_lists->data;
	cfg = &cycle->cfg;
	sockid = fd_lists->fd;

	ret = lsn_pub_recv(cfg->data, sockid, &pkgbuf);
	if (ret < 0)
	{
		close(sockid);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("[%s][%d] Recv error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));;
		return -1;
	}

	pub_log_bin(SW_LOG_INFO, pkgbuf.data, pkgbuf.len, "[%s][%d]接收到的报文:[%d]", 
			__FILE__, __LINE__,pkgbuf.len); 

	ret = pub_loc_vars_alloc(&vars, HEAP_VARS);
	if (ret != 0)
	{
		close(sockid);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("%s, %d, pub_loc_vars_alloc error.",__FILE__,__LINE__);
		return -1;
	}

	ret = vars.create(&vars, 0);
	if (ret != 0)
	{
		close(sockid);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("%s, %d, vars.create error.",__FILE__,__LINE__);
		return -1;
	}

	memset(xmlname, 0x00, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/agentcfg/common/%s", getenv("SWWORK"), cfg->xmlname);
	ret = access(xmlname, F_OK);
	if (ret != 0)
	{
		close(sockid);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("[%s][%d] not find file[%s].", __FILE__, __LINE__,xmlname);
		return -1;		
	} 

	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		close(sockid);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("%s, %d, pub_xml_crtree [%s] error.",__FILE__,__LINE__,xmlname);
		return -1;
	}

	/*拆包*/
	ret = pkg_xml_in(&vars, pkgbuf.data, (char*)xml, pkgbuf.len, 1);
	if (ret < 0)
	{
		close(sockid);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("[%s][%d] 解包出错!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return -1;
	}

	/*接收文件*/
	if (strlen(cfg->tranfunc) != 0)
	{
		ret = run_file_fun(cfg->tranfunc, &vars, 1, sockid, NULL);
		if (ret < 0)
		{
			pub_log_debug("[%s][%d]文件传输失败!", __FILE__, __LINE__);
		}
	}

	/*执行交易处理*/
	pub_log_info("[%s][%d]ready deal tx....", __FILE__, __LINE__);
	pub_mem_memzero(tx_code, sizeof(tx_code));
	ret = do_tx_func(cfg->map, &vars, tx_code);
	if (ret < 0)
	{
		close(sockid);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		pub_buf_clear(&pkgbuf);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("[%s][%d] [%s]处理失败!", __FILE__, __LINE__, tx_code);
		pub_xml_deltree(xml);
		return -1;
	}

	/*组包*/
	pkgbuf.len = 0;
	memset(pkgbuf.data, 0x00, pkgbuf.size);
	ret = pkg_xml_out(&vars, &pkgbuf, (char*)xml, 1);
	if (ret < 0)
	{
		close(sockid);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("[%s][%d] 组包失败!", __FILE__, __LINE__);
		pub_buf_clear(&pkgbuf);
		pub_xml_deltree(xml);
		return -1;
	}

	pub_log_debug("[%s][%d]即将发送的报文:[%d] %s",__FILE__, __LINE__, pkgbuf.len, pkgbuf.data); 
	ret = lsn_pub_send(sockid, pkgbuf.data, pkgbuf.len);
	if (ret < 0)
	{
		close(sockid);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		select_del_event(cycle->lsn_fds, sockid);
		pub_log_error("[%s][%d] 发送数据出错! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		pub_buf_clear(&pkgbuf);
		pub_xml_deltree(xml);
		return -1;
	}

	/*发送文件*/
	if (strlen(cfg->tranfunc) != 0)
	{
		ret = run_file_fun(cfg->tranfunc, &vars, 0, sockid, NULL);
		if (ret < 0)
		{
			pub_log_error("[%s][%d]文件传输失败!", __FILE__, __LINE__);
		}
	}

	pub_log_debug("[%s][%d] [%s]处理完成!", __FILE__, __LINE__, tx_code);
	vars.destroy(&vars);
	vars.free_mem(&vars);
	pub_buf_clear(&pkgbuf);
	pub_xml_deltree(xml);
	close(sockid);
	select_del_event(cycle->lsn_fds, sockid);
	return -1;
}

int agt_accept(sw_fd_list_t *fd_lists)
{
	int	ret = 0;
	int	acceptfd = 0;
	sw_fd_list_t	fd_list;
	agt_cycle_t	*cycle = NULL;

	if (fd_lists == NULL)
	{
		pub_log_error("[%s][%d]func accept argument error ", __FILE__, __LINE__);
		return -1;
	}

	cycle = (agt_cycle_t *)fd_lists->data;
	acceptfd = lsn_pub_accept(fd_lists->fd);
	if (acceptfd < 0 && errno == EAGAIN)
	{
		return 0;
	}
	else if (acceptfd < 0)
	{
		pub_log_error("[%s][%d] accept error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	pub_log_debug("[%s][%d] accept ok.", __FILE__, __LINE__);

	ret = lsn_set_fd_noblock(acceptfd);
	if (ret)
	{
		pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, acceptfd);
		close(acceptfd);
		return -1;
	}

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = acceptfd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)agt_work;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] select_add_event error fd[%d].", __FILE__, __LINE__, fd_list.fd);
		return -1;
	}

	return 0;		
}

int agt_comm_work(agt_cycle_t *cycle)
{	
	int	ret = 0;
	int	sockid = 0;
	sw_fd_list_t	fd_list;
	sw_agt_cfg_t	*cfg = &cycle->cfg;

	if (cfg->ip[0] == '\0' || cfg->port <= 0)
	{
		pub_log_error("[%s][%d] input argv error.", __FILE__, __LINE__);
		return -1;
	}

	sockid = lsn_pub_bind(cfg->ip, cfg->port);
	if (sockid < 0)
	{
		pub_log_error("[%s][%d] bind[%s][%d]error.", __FILE__, __LINE__, cfg->ip, cfg->port);
		return -1;
	}

	ret = lsn_set_fd_noblock(sockid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] noblock error! FD=[%d]", __FILE__, __LINE__, sockid);
		close(sockid);
		return -1;
	}

	cfg->lsn_fd = sockid;
	pub_log_info("[%s][%d] bind [%s][%d] sockid=[%d] ok.", __FILE__, __LINE__, cfg->ip, cfg->port, sockid);
	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = sockid;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)agt_accept;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

static int find_warn_proc(char *proc_name, int proc_id)
{
	int	i = 0;

	for (i = 0; i < 1024; i++)
	{
		if (proc_warns[i].use_time > 0 )
		{
			if (proc_warns[i].proc_id == proc_id && strcmp(proc_warns[i].proc_name, proc_name) == 0)
			{
				proc_warns[i].flage = 1;
				proc_warns[i].use_time++;
				return 0;
			}
			else
			{
				proc_warns[i].flage = 0;
			}
		}
	}

	return 1;
}

static int insert_warn_proc(char *proc_name, int proc_id)
{
	int	i = 0;
	for (i = 0; i < 1024; i++)
	{
		if (proc_warns[i].use_time == 0)
		{
			strcpy(proc_warns[i].proc_name, proc_name);
			proc_warns[i].proc_id = proc_id;
			proc_warns[i].use_time++;
			proc_warns[i].flage = 1;
			return 0;
		}
	}

	return -1;
}

static int check_warn_time(char *proc_name, int proc_id)
{
	if (proc_name == NULL || strlen(proc_name) == 0 || proc_id <= 0 )
	{
		pub_log_error("[%s][%d]proc_name[%s] or proc_id[%d] is error,please check!", __FILE__, __LINE__, proc_name, proc_id);
		return -1;
	}

	int	result = 0;

	result = find_warn_proc(proc_name, proc_id);
	if (result)
	{
		result = insert_warn_proc(proc_name, proc_id);
		if (result)
		{
			pub_log_error("[%s][%d]all insert_warn_proc is used!", __FILE__, __LINE__);
			return -1;
		}
	}
	return 0;
}

static int scanf_warn_proc(int warn_time, int continue_time, float per)
{
	int	i = 0;

	for (i = 0; i < 1024; i++)
	{
		if (proc_warns[i].use_time > 0 && proc_warns[i].flage == 0 )
		{
			memset(&proc_warns[i], 0x00, sizeof(proc_warns[i]));
		}
		else if (proc_warns[i].use_time >=  warn_time)
		{

			alert_msg(ERR_PROC_MEM, "MEM告警:当前进程[%s]内存使用量以连续[%d]分钟超过预警临界值[%0.2f%], 请尽快检查!",
					proc_warns[i].proc_name, (continue_time*warn_time)/60, per);
			memset(&proc_warns[i], 0x00, sizeof(proc_warns[i]));

		}
	}

	return 0;
}

int agt_mem_work(agt_cycle_t *cycle)
{
	int	ret = 0;
	int	max = 0;
	int	cnt = 0;
	int	tolen = 0;
	float	per = 0;
	FILE	*fp = NULL;
	char	*p = NULL;
	char	*ptr = NULL;
	char	tmp[32];
	char	buf[256];
	char 	path[128];
	char	filename[128];
	time_t	curt = 0;
	mem_stats_t	mem_stats;
	sw_agt_cfg_t	*cfg = &cycle->cfg;

	ret = check_file_status(cfg, cfg->name);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check file status error.", __FILE__, __LINE__);
		return -1;
	}

	if (check_file_exist(cfg->use, MEM_INFO_NAME))
        {
                return 0;
        }
        
	ret = agt_get_mem_stats(&mem_stats);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get mem stats error!", __FILE__, __LINE__);
		return -1;
	}

	max = cfg->durtime / cfg->scantime + 1;

	memset(filename, 0x00, sizeof(filename));
	memset(path, 0x00, sizeof(path));
	sprintf(path, "%s/tmp/agtmon", getenv("SWWORK"));
	ret = access(path, W_OK);
	if (ret < 0)
	{
		if (errno == ENOENT)
		{
			ret = pub_file_check_dir(path);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
				return -1;
			}

		}
		else
		{
			pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
			return -1;
		}
	}

	sprintf(filename, "%s/%s", path, MEM_INFO_NAME);
	fp = fopen(filename, "ab+");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open file[%s] error,errno[%d] errmsg:[%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	tolen = ftell(fp);
	p = (char *)calloc(1, tolen + 1);
	if (p == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fread(p, tolen, 1, fp);
	cnt = strchr_cnt(p, '\n');
	pub_log_debug("[%s][%d] cnt=[%d]max=[%d]", __FILE__, __LINE__, cnt,max);
	if (cnt >= max)
	{
		fclose(fp);
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file[%s] error,errno[%d] errmsg:[%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
			free(p);
			return -1;
		}
		ptr = strchr(p, '\n');
		if (ptr != NULL)
		{
			ptr++;
			fwrite(ptr, strlen(ptr), 1, fp);
		}
	}

	free(p);
	p = NULL;	

	fseek(fp, 0, SEEK_END);
	memset(tmp, 0x00, sizeof(tmp));
	memset(buf, 0x00, sizeof(buf));
	curt = time(NULL);
	pub_time_change_time(&curt, tmp, 0);
	sprintf(buf, "TIME:%s|TOTAL:%lld|FREE:%lld|USED:%lld|CACHE:%lld|\n", tmp, mem_stats.total, mem_stats.free, mem_stats.used, mem_stats.cache);
	fwrite(buf, strlen(buf), 1, fp);
	fclose(fp);

	per = 100 * (double)mem_stats.used / (double)mem_stats.total;
	if (per >= cfg->warnper)
	{
		if (g_mem_use_cnt <= cfg->warncnt)
		{
			g_mem_use_cnt++;
		}
		else
		{
			alert_msg(ERR_MEM, "MEM告警:当前系统MEM使用量连续[%d]分钟内占用超过[%lld], 请尽快检查!",
					(cfg->scantime * cfg->warncnt) / 60);
			g_mem_use_cnt = 0;
		}
	}

	pub_log_info("[%s][%d] mem:total=[%lld] used=[%lld]", __FILE__, __LINE__, mem_stats.total / 1024, mem_stats.used / 1024);
	pub_log_info("[%s][%d] mem usage:[%.2f]", __FILE__, __LINE__, 100 * (double)mem_stats.used / (double)mem_stats.total);
	return 0;
}

int agt_cpu_work(agt_cycle_t *cycle)
{
	int	i = 0;
	int	tolen = 0;
	int	ret = 0, cnt = 0;
	int	num = 0,max = 0;
	double	cpu_usage = 0.00;
	double	warnper = 0.00;
	char	tmp[32];
	char	line[512];
	char	path[128];
	char	filename[128];
	char	*p = NULL;
	char 	*ptr = NULL;
	time_t	curt = 0;
	FILE	*fp = NULL;
	cpu_stat_t	cpu;
	cpu_stat_t	*cpu1 = NULL;
	cpu_stat_t	*cpu1_old = NULL;
	cpu_stat_t	*cpu2 = NULL;
	cpu_stat_t	*cpu2_old = NULL;
	sw_agt_cfg_t	*cfg = &cycle->cfg;

	ret = check_file_status(cfg, cfg->name);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check file status error.", __FILE__, __LINE__);
		return -1;
	}
	
	if (check_file_exist(cfg->use, CPU_INFO_NAME))
        {
                return 0;
        }

	cpu1 = agt_get_cpu_stat(&num);
	if (cpu1 == NULL)
	{
		pub_log_error("[%s][%d] getcpustat error!\n", __FILE__, __LINE__);
		return -1;
	}

	sleep(1);
	cpu2 = agt_get_cpu_stat(&num);
	if (cpu2 == NULL)
	{
		free(cpu1);
		pub_log_error("[%s][%d] getcpustat error!\n", __FILE__, __LINE__);
		return -1;
	}

	cpu1_old = cpu1;
	cpu2_old = cpu2;

	max = cfg->durtime / cfg->scantime + 1;

	memset(filename, 0x00, sizeof(filename));
	memset(path, 0x00, sizeof(path));
	sprintf(path, "%s/tmp/agtmon", getenv("SWWORK"));
	ret = access(path, W_OK);
	if (ret < 0)
	{
		if (errno == ENOENT)
		{
			ret = pub_file_check_dir(path);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
				return -1;
			}

		}
		else
		{
			pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
			return -1;
		}
	}
	sprintf(filename, "%s/%s", path, CPU_INFO_NAME);
	fp = fopen(filename, "ab+");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open file [%s] error.", __FILE__, __LINE__, filename);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	tolen = ftell(fp);
	p = (char *)calloc(1, tolen + 1);
	if (p == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fread(p, tolen, 1, fp);
	cnt = strstr_cnt(p, "TOTAL:");
	pub_log_debug("[%s][%d] cnt=[%d] max=[%d]", __FILE__, __LINE__, cnt,max);
	if (cnt >= max)
	{
		fclose(fp);
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file[%s] error,errno[%d] errmsg:[%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
			free(p);
			return -1;
		}

		ptr = strstr(p + 6, "TOTAL:");
		if (ptr != NULL)
		{
			fwrite(ptr, strlen(ptr), 1, fp);
		}
	}
	free(p);
	p = NULL;

	fseek(fp, 0, SEEK_END);
	cpu_usage = 0.00;	
	memset(line, 0x00, sizeof(line));
	sprintf(line, "TOTAL:%d|\n", num);
	fwrite(line, strlen(line), 1, fp);
	for (i = 0; i < num; i++)
	{
		memset(&cpu, 0x0, sizeof(cpu));
		cpu.user = cpu2->user - cpu1->user;
		cpu.kernel = cpu2->kernel - cpu1->kernel;
		cpu.idle = cpu2->idle - cpu1->idle;
		cpu.iowait = cpu2->iowait - cpu1->iowait;
		cpu.total = cpu2->total - cpu1->total;
		if (cpu.total == 0)
		{
			g_cpu_usage_data.cpu_usage[i].user = 0.00;
			g_cpu_usage_data.cpu_usage[i].kernel = 0.00;
			g_cpu_usage_data.cpu_usage[i].idle = 0.00;
			g_cpu_usage_data.cpu_usage[i].iowait = 0.00;
		}
		else
		{
			g_cpu_usage_data.cpu_usage[i].user = (double)cpu.user / (double)cpu.total * 100;
			g_cpu_usage_data.cpu_usage[i].kernel = (double)cpu.kernel / (double)cpu.total * 100;
			g_cpu_usage_data.cpu_usage[i].idle = (double)cpu.idle / (double)cpu.total * 100;
			g_cpu_usage_data.cpu_usage[i].iowait = (double)cpu.iowait / (double)cpu.total * 100;
		}
		strncpy(g_cpu_usage_data.cpu_usage[i].desc, cpu1->desc, sizeof(g_cpu_usage_data.cpu_usage[i].desc) - 1);
		cpu1++;
		cpu2++;

		cpu_usage += 100 - g_cpu_usage_data.cpu_usage[i].idle;
		memset(tmp, 0x00, sizeof(tmp));
		memset(line, 0x00, sizeof(line));
		curt = time(NULL);
		pub_time_change_time(&curt, tmp, 0);
		sprintf(line, "TIME:%s|USER:%lf|KERNEL:%lf|IDLE:%lf|IOWAIT:%lf|\n",
				tmp,g_cpu_usage_data.cpu_usage[i].user, g_cpu_usage_data.cpu_usage[i].kernel,
				g_cpu_usage_data.cpu_usage[i].idle, g_cpu_usage_data.cpu_usage[i].iowait);
		fwrite(line, strlen(line), 1, fp);
	}
	free(cpu1_old);
	cpu1 = NULL;
	free(cpu2_old);
	cpu2 = NULL;
	g_cpu_usage_data.cpu_num = num;
	fclose(fp);

	cpu_usage /= num;
	warnper = cfg->warnper;
	if (cpu_usage - warnper > 0.05)
	{
		if (g_cpu_use_cnt <= cfg->warncnt)
		{
			g_cpu_use_cnt++;
		}
		else
		{
			pub_log_error("CPU告警:当前[%d]个CPU的平均使用率为[%.2f],超过[%.2f],请尽快检查处理!", 
					num, cpu_usage, warnper);

			alert_msg(ERR_CPU, "CPU告警:当前[%d]个CPU在连续[%d]分钟内平均使用率为[%.2f],超过[%.2f],请尽快检查处理!", 
					num,(cfg->scantime * cfg->warncnt) / 60, cpu_usage, warnper);
		}
	}

	return 0;
}

int agt_net_work(agt_cycle_t *cycle)
{
	int	i = 0;
	int	num = 0;
	network_stats_t	*net1 = NULL;
	network_stats_t	*net2 = NULL;

	net1 = (network_stats_t *)agt_get_network_stats(&num);
	if (net1 == NULL)
	{
		pub_log_error("[%s][%d] get_network_stats error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	sleep(1);
	net2 = (network_stats_t *)agt_get_network_stats(&num);
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
	}

	free(net1);
	net1 = NULL;
	free(net2);
	net2 = NULL;

	g_net_percent_data.net_num = num;
	return 0;	
}

int agt_proc_work(agt_cycle_t *cycle)
{
	int	idx = 0, nm = 0;
	int	pid = 0;
	int	index = 0, result = 0;
	int	len = 0, tolen = 0;
	int	max = 0, cnt = 0;
	double	per = 0.00;
	double  warnper = 0.00;
	long 	curt = 0, lastcur = 0;
	FILE 	*pf = NULL;
	FILE	*fp = NULL;
	char 	cur_time[64];
	char 	*p = NULL, *ptmp = NULL;
	char 	*s = NULL, *str = NULL;
	char 	cmd[256];
	char 	line[1024];
	char 	buf[1024];
	char	path[128];
	char	filename[128];
	char	cpu[32];
	char	mem[32];
	time_t	cur = 0;
	char	vsz[32];
	char	rss[32];
	char	stat[32];
	sw_svr_grp_t	*svr_grp = NULL;
	sw_svr_grp_t	*svr_grps = NULL;
	sw_proc_info_t	*proc_shm = NULL;
	sw_procs_head_t	*procs_head = NULL;
	sw_procs_head_t	head;
	sw_proc_info_t	info;
	struct passwd	*pwd;
	proc_mem	proc_mens[1024];
	sw_agt_cfg_t	*cfg = &cycle->cfg;

	result = check_file_status(cfg, cfg->name);
	if (result < 0)
	{
		pub_log_error("[%s][%d] check file status error.", __FILE__, __LINE__);
		return -1;
	}

	if (check_file_exist(cfg->use, PROC_INFO_NAME))
	{
		return 0;	
	}
	
	warnper = cfg->warnper;
	memset(&head, 0x00, sizeof(head));
	memset(&info, 0x0, sizeof(info));
	result = run_link_ext();
	if (result != 0)
	{
		pub_log_error("[%s][%d] Link shm error!", __FILE__, __LINE__);
		return -1;
	}

	result = procs_get_head(&head);
	if(0 != result)
	{
		pub_log_error("[%s][%d] Get procs head error!", __FILE__, __LINE__);
		return -1;
	}

	memset(proc_mens, 0x00, sizeof(proc_mens));
	for (idx = 0; idx < head.sys_proc_use; idx++)
	{
		memset(&info, 0x00, sizeof(info));
		result = procs_get_sys_by_index(idx, &info);
		if (0 != result)
		{
			pub_log_error("[%s][%d] Get sys procs by index error!", __FILE__, __LINE__);
			return -1;
		}

		strcpy(proc_mens[nm].proc_name, info.name);
		proc_mens[nm].pid = info.pid;
		proc_mens[nm].status = info.status;
		nm++;
	}

	procs_head = run_get_procs();
	if (procs_head == NULL)
	{
		pub_log_error("[%s][%d] run_get_procs failed", __FILE__, __LINE__);
		return -1;
	}

	svr_grp = (sw_svr_grp_t*)((char *)procs_head + procs_head->svr_grp_offset);
	for (idx = 0; idx < procs_head->svr_grp_use; idx++)
	{
		if (strlen(svr_grp[idx].svrname) > 0)
		{
			svr_grps = svr_grp+(int)idx;
			proc_shm =(sw_proc_info_t *)((char *)procs_head + svr_grps->offset);
			for (index = 0; index < svr_grps->svc_curr; index++)
			{
				memset(&info, 0x00, sizeof(info));
				pub_mem_memcpy(&info, proc_shm + index, sizeof(info));	

				strcpy(proc_mens[nm].proc_name, proc_shm[index].name);
				proc_mens[nm].pid = info.pid;
				proc_mens[nm].status = info.status;
				nm++;
			}
		}
	}

	run_destroy();
	max = cfg->durtime / cfg->scantime  + 1;
	memset(filename, 0x00, sizeof(filename));
	memset(path, 0x00, sizeof(path));
	sprintf(path, "%s/tmp/agtmon", getenv("SWWORK"));
	result = access(path, W_OK);
	if (result < 0)
	{
		if (errno == ENOENT)
		{
			result = pub_file_check_dir(path);
			if (result < 0)
			{
				pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
				return -1;
			}

		}
		else
		{
			pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
			return -1;
		}
	}
	sprintf(filename, "%s/%s", path, PROC_INFO_NAME);
	fp = fopen(filename, "ab+");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open file error.", __FILE__, __LINE__);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	tolen = ftell(fp);
	s = (char *)calloc(1, tolen + 1);
	if (s == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fread(s, tolen, 1, fp);
	cnt = strstr_cnt(s, "TOTAL:");
	pub_log_debug("[%s][%d]cnt=[%d]max=[%d]", __FILE__, __LINE__, cnt, max);
	if (cnt >= max)
	{	
		fclose(fp);
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file[%s] error,errno[%d] errmsg:[%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
			free(s);
			return -1;
		}

		str = strstr(s + 6, "TOTAL:");
		if (str != NULL)
		{
			fwrite(str, strlen(str), 1, fp);
		}
	}

	free(s);
	s = NULL;

	fseek(fp, 0, SEEK_END);
	memset(cur_time, 0x00, sizeof(cur_time));
	curt = time(NULL);
	agt_date_format(curt, cur_time, "G");

	memset(buf, 0x00, sizeof(buf));
	lastcur = lastcur - (cfg->durtime) * 3600;
	agt_date_format(lastcur, buf, "G");

	memset(cmd, 0x00, sizeof(cmd));
	memset(line, 0x00, sizeof(line));
	cur = time(NULL);
	pub_time_change_time(&cur, cmd, 0);
	/*进程个数 进程时间 进程秒数 进程最后时间*/
	sprintf(line, "TOTAL:%d|TIME:%s|PROCTIME:%s|PROCSEC:%ld|LASTTIME:%s|\n", nm, cmd, cur_time, curt, buf);
	fwrite(line, strlen(line),1, fp);

	memset(cmd, 0x00, sizeof(cmd));
	pwd = getpwuid(getuid());
	sprintf(cmd, "ps aux | grep %s| awk '{print $2 ,$3 ,$4 ,$5 ,$6}'", pwd->pw_name);
	pub_log_debug("[%s][%d] the cmd is [%s]",__FILE__, __LINE__, cmd);
	pf = popen(cmd, "r");
	if (pf == NULL)
	{
		pub_log_error("%s, %d, popen[%s] error[%d][%s].", __FILE__, __LINE__, cmd, errno, strerror(errno));
		return -1;
	}
	while (!feof(pf))
	{
		memset(line, 0x00, sizeof(line));
		ptmp = fgets(line, sizeof(line), pf);
		len = strlen(line);
		if (ptmp == NULL || len == 0)
		{
			pub_log_debug("[%s][%d] line ::[%s]", __FILE__, __LINE__, line);
			pclose(pf);
			break;
		}

		if (line[len -1] == '\n')
		{
			line[len -1] = '\0';
		}

		pid = 0;
		memset(cpu, 0x00, sizeof(cpu));
		memset(mem, 0x00, sizeof(mem));
		memset(vsz, 0x00, sizeof(vsz));
		memset(rss, 0x00, sizeof(rss));
		p = strtok(line, " ");
		if (p != NULL)
		{
			pid = atoi(p);
		}

		p = strtok(NULL, " ");
		if (p != NULL)
		{
			strcpy(cpu, p);
		}

		p = strtok(NULL, " ");
		if (p != NULL)
		{
			strcpy(mem, p);
		}

		p = strtok(NULL, " ");
		if (p != NULL)
		{
			strcpy(vsz, p);
		}

		p = strtok(NULL, " ");
		if (p != NULL)
		{
			strcpy(rss, p);
		}

		for(idx = 0; idx < nm; idx++)
		{
			if (pid == proc_mens[idx].pid)
			{
				break;
			}
		}

		if (per  - warnper >= 0.05 && strlen(proc_mens[idx].proc_name) > 0)
		{
			check_warn_time(proc_mens[idx].proc_name, pid);
		}
		if (idx != nm)
		{
			memset(stat, 0x00, sizeof(stat));
			memset(buf, 0x00, sizeof(buf));
			if (proc_mens[idx].status == SW_S_START)
			{
				strncpy(stat, "正常运行", sizeof(stat) - 1);
			}
			else if (proc_mens[idx].status == SW_S_STOPED)
			{
				strncpy(stat, "正常停止", sizeof(stat) - 1);
			}
			else if(proc_mens[idx].status == SW_S_KILL)
			{
				strncpy(stat, "强制停止", sizeof(stat) - 1);
			}
			else if (proc_mens[idx].status == SW_S_ABNORMAL)
			{
				strncpy(stat, "异常", sizeof(stat) - 1);
			}
			else
			{
				strncpy(stat, "未知", sizeof(stat) - 1);
			}
			
			pub_log_info("[%s][%d] name=[%s] status=[%d] pid=[%d]", __FILE__, __LINE__, proc_mens[idx].proc_name, proc_mens[idx].status, proc_mens[idx].pid);
			sprintf(buf, "%s|%d|%s|%s|%s|%s|%s|\n", proc_mens[idx].proc_name, proc_mens[idx].pid, cpu, mem, vsz, rss, stat);
			fwrite(buf, strlen(buf),1,fp);
		}
	}

	fclose(fp);

	scanf_warn_proc(cfg->warncnt, cfg->durtime, warnper);
	return 0;
}

int agt_mtype_work(agt_cycle_t *cycle)
{
	int	i = 0;
	int	max = 0, cnt = 0;
	int	used = 0, idle = 0;
	int	result = 0, tolen = 0;
	int	fail = 0, succ = 0;
	char	tmp[128];
	char	path[128];
	char	filename[256];
	char	line[2048];
	FILE	*fp = NULL;
	char	*s = NULL, *str = NULL;
	time_t	curt = 0;
	sw_int64_t	now = 0;
	sw_mtype_t      *addr = NULL;
	sw_mtype_head_t	*head = NULL;
	sw_mtype_node_t *item = NULL;
	sw_trace_info_t	*trace_info = NULL;
	sw_trace_item_t	*trace_item = NULL;
	sw_agt_cfg_t	*cfg = &cycle->cfg;
	result = check_file_status(cfg, cfg->name);
	if (result < 0)
	{
		pub_log_error("[%s][%d] check file status error.", __FILE__, __LINE__);
		return -1;
	}

	if (check_file_exist(cfg->use, MTYPE_INFO_NAME))
	{
		return 0;	
	}
	
	max = cfg->durtime / cfg->scantime + 1;

	memset(filename, 0x00, sizeof(filename));
	memset(path, 0x00, sizeof(path));
	sprintf(path, "%s/tmp/agtmon", getenv("SWWORK"));
	result = access(path, W_OK);
	if (result < 0)
	{
		if (errno == ENOENT)
		{
			result = pub_file_check_dir(path);
			if (result < 0)
			{
				pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
				return -1;
			}

		}
		else
		{
			pub_log_error("[%s][%d] not create path[%s],errno=[%d]", __FILE__, __LINE__,path, errno);
			return -1;
		}
	}
	sprintf(filename, "%s/%s", path, MTYPE_INFO_NAME);
	fp = fopen(filename, "ab+");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] open file[%s] error,errno[%d] errmsg:[%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	tolen = ftell(fp);
	s = (char *)calloc(1, tolen + 1);
	if (s == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fread(s, tolen, 1, fp);
	cnt = strstr_cnt(s, "TOTAL:");
	pub_log_info("[%s][%d] cnt=[%d]max=[%d]", __FILE__, __LINE__, cnt, max);
	if (cnt >= max)
	{
		fclose(fp);
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file[%s] error,errno[%d] errmsg:[%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
			free(s);
			return -1;
		}

		str = strstr(s + 6, "TOTAL:");
		if (str != NULL)
		{
			fwrite(str, strlen(str), 1, fp);
		}
	}
	free(s);
	s = NULL;

	result = run_link_ext();
	if (result != 0)
	{
		pub_log_error("[%s][%d] Link shm error!", __FILE__, __LINE__);
		return -1;
	}

	addr = (sw_mtype_t *)mtype_get_addr();
	trace_info = trace_get_addr();
	trace_item = trace_info->trace_item;	
	head = &addr->head;
	item = &addr->first;
	now = pub_time_get_current();
	for (i = 0; i < head->mtype_max; i++)
	{
		switch (item->flag)
		{
			case MTYPE_TAIL:
			case MTYPE_IDLE:
				{
					idle++;
					break;
				}
			case MTYPE_USED:
				{
					used++;
					break;
				}
			default:
				{
					break;
				}
		}
		item++;
	}

	memset(tmp, 0x00, sizeof(tmp));
	memset(line, 0x00, sizeof(line));
	curt = time(NULL);
	pub_time_change_time(&curt, tmp, 0);
	fseek(fp, 0, SEEK_END);
	sprintf(line, "TOTAL:%d|USED:%d|IDLE:%d|SUCC:%d|FAIL:%d|TIME:%s|\n",  head->mtype_max, used, idle, head->succ_cnt, head->fail_cnt, tmp);
	fwrite(line, strlen(line), 1, fp);
	fail = head->fail_cnt;
	succ = head->succ_cnt;
	max = head->mtype_max;
	if (used > 0)
	{
		memset(tmp, 0x0, sizeof(tmp));
		pub_change_time(now, tmp, 5);
		item = &addr->first;
		for (i = 0; i < head->mtype_max; i++)
		{
			if (item->flag == MTYPE_USED)
			{
				memset(line, 0x0, sizeof(line));
				memset(tmp, 0x0, sizeof(tmp));
				pub_change_time(trace_item[i].start_time, tmp, 5);
				sprintf(line, "MTYPE:[%d] TRACE_NO:[%lld] BUSI_NO:[%lld] PRDT:[%s] CHNL:[%s] TXCODE:[%s] SERVER:[%s] SERVICE:[%s] STARTTIME:[%s]\n", 
						i + 1, trace_item[i].trace_no, trace_item[i].busi_no, trace_item[i].prdt_name, 
						trace_item[i].chnl, trace_item[i].tx_code, trace_item[i].server, trace_item[i].service, tmp);
				fwrite(line, strlen(line),1, fp);
			}
			item++;
		}
	}

	fclose(fp);
	run_destroy();

	int	total = fail + succ;
	double	per = 0;
	double	baseper = 20.00;
	per = 100 * (double)fail  / (double)total;
	if (per - baseper > 0.05)
	{
		alert_msg(ERR_TX_CNT, "交易告警:当前产品业务失败率超过20%,请尽快检查!");
	}

	per = 100 * (double)used  / (double)max;
	if (per -  cfg->warnper > 0.001)
	{
		if (g_mtype_use_cnt <= cfg->warncnt)
		{
			g_mtype_use_cnt++;
		}
		else
		{
			alert_msg(ERR_MTYPE_USED, "MTYPE告警:当前系统MTYPE暂用量连续[%d]分钟内占用超过警戒线[%s%%], 请尽快检查!",
					(cfg->warncnt * cfg->scantime) / 60, cfg->warnper);
			g_mtype_use_cnt = 0;
		}
	}

	return 0;
}



