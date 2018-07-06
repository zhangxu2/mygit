#include "lsn_pub.h"
#include "pub_filter.h"
#include "pub_proc.h"

extern sw_int_t lsn_get_chl_stat(char *name, int *stat);
extern sw_int_t lsn_route_init(sw_lsn_cycle_t *cycle);
extern sw_int_t lsn_get_all_cache(sw_lsn_cycle_t *cycle);
extern sw_int32_t lsn_father_register(sw_lsn_cycle_t *cycle, sw_int32_t status);
extern int lsn_main(sw_lsn_cycle_t *cycle);
extern sw_int_t lsn_free_all_cache(sw_lsn_cycle_t *cycle);

sw_int_t lsn_cycle_init(cycle, name, lsn_name, module_type, err_log, dbg_log)
sw_lsn_cycle_t	*cycle;
sw_char_t	*name;
sw_char_t	*lsn_name;
sw_int32_t	module_type;
sw_char_t	*err_log;
sw_char_t	*dbg_log;
{
	int	reuse = 0;
	int	stat = 0;
	int	pcnt = 0;
	sw_int_t	ret = 0;
	sw_svr_grp_t	grp;
	sw_proc_info_t  proc_info;
	
	/*init base class object. */
	ret = cycle_init(&cycle->base, name, module_type, err_log, dbg_log, NULL);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = lsn_get_chl_stat(lsn_name, &stat);
	if (ret != SW_OK)
	{
		cycle_destory((sw_cycle_t *)cycle);
		pub_log_error("[%s][%d] get [%s] status error!", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] [%s] stat [%d]", __FILE__, __LINE__, lsn_name, stat);
	if (stat != 1)
	{
		cycle_destory((sw_cycle_t *)cycle);
		pub_log_info("[%s][%d] [%s] not used!", __FILE__, __LINE__, lsn_name);
		return SW_DONE;
	}
	
	ret = lsn_pub_cfg_init(cycle, lsn_name);
	if (ret != SW_OK)
	{
		cycle_destory((sw_cycle_t *)cycle);
		pub_log_error("%s, %d, lsn_pub_cfg_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d]name[%s]", __FILE__, __LINE__, cycle->base.name.data);
	ret = cycle_link_shm((sw_cycle_t *)cycle);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_public_link error.", __FILE__,
			__LINE__);
		return SW_ERROR;
	}
	
	reuse = 0;
	pub_mem_memzero(&proc_info, sizeof(proc_info));
	ret = procs_get_sys_by_name(cycle->base.name.data, &proc_info);
	if ( ret != SW_ERROR)
	{
		if ( pub_proc_checkpid(proc_info.pid) ==  SW_OK)
		{
			pub_log_info("[%s][%d] name=[%s] register info exist, iStatus=[%d],PID=[%d]",
				__FILE__, __LINE__, cycle->base.name.data, proc_info.status, proc_info.pid);
			return SW_EXIST;
		}
		
		if (proc_info.shmid > 0)
		{
			cycle->shm.addr = shmat(proc_info.shmid, NULL, 0);
			if (cycle->shm.addr != (void *)-1)
			{
				reuse = 1;
				cycle->shm.id = proc_info.shmid;
				pub_log_info("[%s][%d] 共享内存存在! 不需要创建!", __FILE__, __LINE__);
			}
		}
		cycle->semid = proc_info.lockid;
		pub_log_info("[%s][%d] lockid=[%d]", __FILE__, __LINE__, cycle->semid);
	}
	else
	{
		cycle->semid = sem_new_lock_id();
		if (cycle->semid == SW_ERROR)
		{
			pub_log_error("[%s][%d] sem_new_lock_id error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] new lockid=[%d]", __FILE__, __LINE__, cycle->semid);
	}
	
	cycle->list_size = mtype_get_max();
	if (cycle->list_size <= 0)
	{
		pub_log_error("[%s][%d] get mtype max error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	cycle->link_shm.size = pub_mem_align(((sizeof(sw_link_list_t) + sizeof(sw_link_node_t) * cycle->list_size) * cycle->lsn_conf.comm.count), SW_ALIGNMENT);

	cycle->route_shm.size = pub_mem_align((sizeof(sw_route_cache_t) + sizeof(sw_route_t) * MAX_ROUTE_NUM), SW_ALIGNMENT);
	cycle->sock_shm.size = pub_mem_align((cycle->lsn_conf.procmax * sizeof(sw_link_info_t) * cycle->lsn_conf.conmax), SW_ALIGNMENT);
	
	pcnt = cycle->lsn_conf.procmax > MAX_COMM_PROC_CNT ? cycle->lsn_conf.procmax : MAX_COMM_PROC_CNT;
	cycle->buf_shm.size = pub_mem_align((cycle->lsn_conf.comm.count * pcnt * MAX_MSG_LEN), SW_ALIGNMENT);
	
	cycle->shm.size = cycle->link_shm.size + cycle->route_shm.size + cycle->sock_shm.size + cycle->buf_shm.size;
	pub_log_info("[%s][%d] shm.size=[%d]", __FILE__, __LINE__, cycle->shm.size);
	if (reuse == 0)
	{
		pub_log_info("[%s][%d] 共享内存不存在,需要创建!", __FILE__, __LINE__);
		ret = pub_shm_alloc(&(cycle->shm));
		if (ret != SW_OK)
		{
			pub_log_stderr("[%s][%d] pub_shm_alloc error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
		
	cycle->link_shm.addr = cycle->shm.addr;
	cycle->route_shm.addr = cycle->link_shm.addr + cycle->link_shm.size;
	cycle->sock_shm.addr = cycle->route_shm.addr + cycle->route_shm.size;
	cycle->buf_shm.addr = cycle->sock_shm.addr + cycle->sock_shm.size;

	cycle->routes = (sw_route_cache_t *)cycle->route_shm.addr;
	cycle->routes->head.route_cnt = 0;
	ret = lsn_route_init(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] route init error!", __FILE__,__LINE__);
		return SW_ERROR;
	}
	
	ret = lsn_get_all_cache(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_get_all_cache error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_mem_memzero(&grp, sizeof(sw_svr_grp_t));
	grp.svc_max = cycle->lsn_conf.procmax;
	strcpy(grp.svrname, cycle->base.name.data);
	ret = procs_svr_alloc(&grp);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_svr_alloc(lsn) sucess!",__FILE__,__LINE__);
	
	pub_mem_memzero(&cycle->cache_buf, sizeof(sw_pkg_cache_list_t));

	/*bind socket for control cmd. */
	cycle->udp_fd = udp_bind(cycle->base.name.data);
	if (cycle->udp_fd < 0)
	{
		pub_log_error("%s, %d, udp_bind error! udp_fd=[%d]",
			__FILE__, __LINE__, cycle->udp_fd);
		cycle->base.proc_info->status = SW_S_ABNORMAL;
		return SW_ERROR;
	}
	pub_log_info("[%s][%d]udp_fd=[%d]", __FILE__, __LINE__, cycle->udp_fd);

	/*Allcate memory for event object.*/
	cycle->lsn_fds = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (cycle->lsn_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*init select event context.*/
	ret = select_init(cycle->lsn_fds);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ret = lsn_father_register(cycle, SW_S_START);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Lsn register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ret = pub_filter_init();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Filter init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] lsn_cycle_init success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_cycle_run(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = 0;
	
	ret = lsn_main(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_cycle_run error!", __FILE__, __LINE__);
		return ret;
	}
	pub_log_info("[%s][%d] lsn_cycle_run success!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t lsn_cycle_child_destroy(sw_lsn_cycle_t *cycle)
{
	int i = 0;
	
	pub_log_info("[%s][%d] Lsn child [%s] exit! destroy begin...", __FILE__, __LINE__, cycle->base.name.data);
	if (cycle->lsn_conf.out_fd > 0)
	{
		pub_log_info("[%s][%d] OUT_FD====[%d]", __FILE__, __LINE__, cycle->lsn_conf.out_fd);
		msg_trans_rm(cycle->base.global_path, cycle->lsn_conf.out_fd);
	}

	for (i = 0; i < MAX_MACHINE_NUM; i++)
	{
		if (g_mp_addr[i].sockid > 0)
		{
			close(g_mp_addr[i].sockid);
		}
	}
	lsn_free_all_cache(cycle);
	cycle_destory((sw_cycle_t *)cycle);
	pub_log_exit("[%s][%d] Lsn child [%s] exit! destroy success...", __FILE__, __LINE__, cycle->base.name.data);

	return SW_OK;
}

sw_int_t lsn_cycle_destroy(sw_lsn_cycle_t *cycle)
{
	pub_log_info("[%s][%d] Lsn [%s] exit! destroy begin...", __FILE__, __LINE__, cycle->base.name.data);
	close(cycle->udp_fd);
	if (cycle->lsn_fd > 0)
	{
		close(cycle->lsn_fd);
	}
	lsn_free_all_cache(cycle);
	pub_shm_free(&cycle->shm);
	cycle_destory((sw_cycle_t *)cycle);
	pub_filter_free();
	pub_log_exit("[%s][%d] Lsn [%s] exit! destroy success...", __FILE__, __LINE__, cycle->base.name.data);

	return SW_OK;
}
