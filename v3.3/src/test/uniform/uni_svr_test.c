#include "uni_svr.h"

int main(int	argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_uni_svr_t	uni_svr;
	sw_char_t	name[32];
	sw_int32_t	flag = -1;

	pub_mem_memzero(&uni_svr, sizeof(uni_svr));

	/*服务管理统一接口初始化*/
	result = uni_svr_init(&uni_svr);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]sw_uni_svr_init error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*启动平台所有服务*/
	result = uni_svr_start_all(&uni_svr);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_start_all error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	
	/*Start all task
	result = uni_svr_start_all_task(&uni_svr);
	if (result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_start_all_task error",__FILE__,__LINE__);
		return SW_ERROR;
	}
	usleep(100000);*/

	/*Start a task*/
	pub_mem_memzero(name, sizeof(name));
	strcpy(name, "00");
	result = uni_svr_start_task(&uni_svr, name);
	if (result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_start_task %s error.", __FILE__, __LINE__, name);
		return SW_ERROR;
	}
	usleep(100000);
	
	/*启动所有平台侦听进程*/
	result = uni_svr_start_all_lsn(&uni_svr);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_start_all_lsn error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*启动任务调度进程*/
	pub_mem_memzero(name, sizeof(name));
	strcpy(name, "swtaskman");
	result = uni_svr_start(&uni_svr, name);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_start error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*启动所单个侦听进程*/
	pub_mem_memzero(name, sizeof(name));
	strcpy(name, "omscbs");
	result = uni_svr_start_lsn(&uni_svr, name);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_start_all_lsn error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	usleep(100000);
	/*停止单个侦听进程
	pub_mem_memzero(name, sizeof(name));
	strcpy(name,"omscbs");
	result = uni_svr_stop_lsn(&uni_svr,name);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_stop_lsn error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	/*停止所有平台侦听进程
	result = uni_svr_stop_all_lsn(&uni_svr);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_stop_all_lsn error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	/*停止单个容器进程
	pub_mem_memzero(name, sizeof(name));
	strcpy(name,"oms");
	result = uni_svr_stop_svc(&uni_svr,name);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_stop_svc error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	/*停止单个平台服务
	pub_mem_memzero(name, sizeof(name));
	strcpy(name,"omscbs");
	result = uni_svr_stop(&uni_svr,name,SW_FORCE);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_stop error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	/*停止所有平台进程
	result = uni_svr_stop_all(&uni_svr, SW_FORCE);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_stop_all error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	usleep(100000);
	
	/*获取进程信息列表*/
	sw_proc_info_t* proc_info_list = NULL;
	sw_int32_t	len = 0;
	sw_int32_t	i = 0;
	result = uni_svr_get_all_proc_info(&uni_svr, &proc_info_list, &len);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_get_all_proc_info error",__FILE__,__LINE__);
		return SW_ERROR;
	}
	for(i = 0; i < len; i++)
	{
		writelog(LOG_INFO,"proc_name[%s] pid[%d] node_type[%d] cur_status[%d]",proc_info_list[i].proc_name, proc_info_list[i].pid, proc_info_list[i].node_type, proc_info_list[i].cur_status);
	}
	pub_mem_free(proc_info_list);

	/*获取某一类平台服务的进程信息*/
	proc_info_list = NULL;
	len = 0;
	i = 0;
	result = uni_svr_get_group_proc_info(&uni_svr, ND_LSNMAN, &proc_info_list, &len);
	if(result != SW_OK)
	{
		writelog(LOG_INFO,"proc_name[%s] pid[%d] node_type[%d] cur_status[%d]",proc_info_list[i].proc_name, proc_info_list[i].pid, proc_info_list[i].node_type, proc_info_list[i].cur_status);
	}
	
	for(i = 0; i < len; i++)
	{
		writelog(LOG_INFO,"proc_name[%s] pid[%d] node_type[%d] cur_status[%d]",proc_info_list[i].proc_name, proc_info_list[i].pid, proc_info_list[i].node_type, proc_info_list[i].cur_status);
	}
	pub_mem_free(proc_info_list);
	

	/*停止所有平台进程
	result = uni_svr_stop_all(&uni_svr, 0);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_stop_all error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	usleep(100000);
	/*清除平台进程表信息
	result = uni_svr_clear_info(&uni_svr);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_clear_info error",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	
	/*释放服务管理统一接口资源*/
	result = uni_svr_destroy(&uni_svr);
	if(result != SW_OK)
	{
		sysLog(LOG_ERR,"[%s][%d]uni_svr_destroy error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	
	return SW_OK;
}


