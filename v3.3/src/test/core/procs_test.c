#include "procs.h"
#include "pub_log.h"
#include "pub_mem.h"
#include "run.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_proc_info_t	proc_info;
 	sw_int32_t	i,j;
	sw_procs_head_t procs_head;
	/*set shm_run key*/ 	
	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] run_link sucess!",__FILE__,__LINE__);

	pub_log_info("[%s][%d] procs_get_head sucess!",__FILE__,__LINE__);
	pub_mem_memzero(&proc_info, sizeof(proc_info));
	strcpy(proc_info.name, "swadmin");
	proc_info.pid = getpid();
	proc_info.type = ND_LSN;
	proc_info.status = SW_S_START; 

	/*test process register*/
	result = procs_sys_register(&proc_info);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d] procs_sys_register(swadmin) sucess!",__FILE__,__LINE__);

	pub_mem_memzero(&procs_head,sizeof(sw_procs_head_t));

	result = procs_get_head(&procs_head);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i=0 ;i< procs_head.sys_proc_use; i++ )
	{
		result = procs_get_sys_by_index(i,&proc_info);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, procs_get_sys_by_index error.", __FILE__, __LINE__);
			break;
		}
		pub_log_info("%s, %d,index=[%d] proc_info: name[%s] svr[%s] type[%d] pid[%d] status[%d]"
					,__FILE__,__LINE__,i,proc_info.name,proc_info.svr_name,proc_info.type, proc_info.pid, proc_info.status);
	}	
	pub_mem_memzero(&proc_info, sizeof(proc_info));
	strcpy(proc_info.name, "swpol");
	proc_info.pid = 1189;
	proc_info.type = ND_POL;
	proc_info.status = SW_S_START; 
	/*test process register*/
	result = procs_sys_register(&proc_info);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_sys_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_sys_register(swpol) sucess!",__FILE__,__LINE__);

	/*test process register*/
	sw_svr_grp_t grp;
	pub_mem_memzero(&grp,sizeof(sw_svr_grp_t));

	grp.svc_max = 10;
	strcpy(grp.svrname,"hello");
	strcpy(grp.prdtname,"world");

	result = procs_svr_alloc(&grp);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d] procs_svr_alloc(hello,world) sucess!",__FILE__,__LINE__);
	for (i = 0;i<5 ; i++)
	{
		pub_mem_memzero(&proc_info, sizeof(proc_info));
		sprintf(proc_info.name, "hello_%d",i);
		strcpy(proc_info.svr_name,"hello");
		strcpy(proc_info.prdt_name,"world");
		proc_info.pid = i+100;
		proc_info.type = ND_LSN;
		proc_info.status = SW_S_START; 
		
		result = procs_svr_register(&proc_info);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	pub_log_info("[%s][%d] procs_lsn_register(hello) sucess!",__FILE__,__LINE__);
	
	pub_mem_memzero(&grp,sizeof(sw_svr_grp_t));
	grp.svc_max = 5;
	strcpy(grp.svrname,"lsn");

	result = procs_svr_alloc(&grp);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_svr_alloc(lsn) sucess!",__FILE__,__LINE__);
	
	for (i = 0;i<5 ; i++)
	{
		pub_mem_memzero(&proc_info, sizeof(proc_info));
		sprintf(proc_info.name, "lsn_%d",i);
		strcpy(proc_info.svr_name,"lsn");
		proc_info.pid = i+100;
		proc_info.type = ND_LSN;
		proc_info.status = SW_S_START; 
		
		result = procs_lsn_register(&proc_info);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] procs_lsn_register(lsn) sucess!",__FILE__,__LINE__);
	sw_svr_grp_t *grp_svr;

	pub_mem_memzero(&procs_head,sizeof(sw_procs_head_t));

	result = procs_get_head(&procs_head);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("%s, %d, svr_grp_use[%d]",__FILE__,__LINE__,procs_head.svr_grp_use);

	for (i=0 ;i< procs_head.svr_grp_use; i++)
	{
		grp_svr = NULL;
		grp_svr = procs_get_svr_by_index(i);
		if (grp_svr == NULL)
		{
			pub_log_error("%s, %d, procs_get_info_by_name error.", __FILE__, __LINE__);
			break;
		}
		pub_log_info("%s, %d, svr_info: name[%s] svr[%s] "
		,__FILE__,__LINE__,grp_svr->svrname,grp_svr->prdtname);

		for ( j=0; j<grp_svr->svc_curr;j++)
		{
			pub_mem_memzero(&proc_info, sizeof(proc_info));
			result = procs_get_proces_by_index(grp_svr,j,&proc_info);
			if (result != SW_OK)
			{
				pub_log_error("%s, %d, procs_get_proces_by_index error.", __FILE__, __LINE__);
				break;
			}
			pub_log_info("%s, %d, proc_info: name[%s] svr[%s] type[%d] pid[%d] status[%d]"
				,__FILE__,__LINE__,proc_info.name,proc_info.svr_name,proc_info.type, proc_info.pid, proc_info.status);
		}
	}
	pub_log_info("[%s][%d] procs_get_proces_by_index  sucess!",__FILE__,__LINE__);
	/*test get proc_info by name*/
	
	grp_svr = procs_get_svr_by_name("hello","world");
	if (grp_svr == NULL)
	{
		pub_log_error("%s, %d, procs_get_info_by_name error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_get_svr_by_name (hello) sucess!",__FILE__,__LINE__);

	for ( i=0;i<grp_svr->svc_curr;i++)
	{		
		pub_mem_memzero(&proc_info, sizeof(proc_info));
		result = procs_get_proces_by_index(grp_svr,i,&proc_info);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, procs_get_proces_by_index error.", __FILE__, __LINE__);
			break;
		}
		pub_log_info("%s, %d, proc_info: name[%s] svr[%s] type[%d] pid[%d] status[%d]"
			,__FILE__,__LINE__,proc_info.name,proc_info.svr_name,proc_info.type, proc_info.pid, proc_info.status);
	}
	pub_log_info("[%s][%d] procs_get_proces_by_index (hello) sucess!",__FILE__,__LINE__);

	grp_svr = procs_get_svr_by_name("lsn",NULL);
	if (grp_svr == NULL)
	{
		pub_log_error("%s, %d, procs_get_info_by_name error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_get_svr_by_name (lsn) sucess!",__FILE__,__LINE__);
	
	for ( i=0;i<grp_svr->svc_curr;i++)
	{
		result = procs_get_proces_by_index(grp_svr,i,&proc_info);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, procs_get_proces_by_index error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("%s, %d, proc_info: name[%s] svr[%s] type[%d] pid[%d] status[%d]"
			,__FILE__,__LINE__,proc_info.name,proc_info.svr_name,proc_info.type, proc_info.pid, proc_info.status);
	}
	
	pub_log_info("[%s][%d] procs_get_proces_by_index (lsn) sucess!",__FILE__,__LINE__);

	result = procs_get_proces_by_name (grp_svr,"lsn_2",&proc_info);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, procs_get_proces_by_index error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("%s, %d, proc_info: name[%s] svr=[%s] type[%d] pid[%d] status[%d]"
		,__FILE__,__LINE__,proc_info.name,proc_info.svr_name,proc_info.type, proc_info.pid, proc_info.status);
			
	return SW_OK;
}
