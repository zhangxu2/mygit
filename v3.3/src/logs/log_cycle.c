#include "log_pub.h"
#include "log_comm.h"
#include "pub_proc.h"

sw_int_t log_cycle_init(cycle, name, module_type, err_log, dbg_log)
	sw_log_cycle_t	*cycle;
	sw_char_t	*name;
	sw_int32_t	module_type;
	sw_char_t	*err_log;
	sw_char_t	*dbg_log;
{
	int	ret = SW_ERROR;
	sw_proc_info_t  proc_info;
	sw_svr_grp_t    grp;

	strcpy(cycle->log_conf.name, name);

	ret = cycle_init(&cycle->base, name, module_type, err_log, dbg_log, NULL);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d]name[%s]", __FILE__, __LINE__, cycle->base.name.data);
	ret = cycle_link_shm((sw_cycle_t *)cycle);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_public_link error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] cfg.use= %d", __FILE__, __LINE__, g_alog_cfg.use);
	g_alog_cfg.use = 0;

	memset(&proc_info, 0x0, sizeof(proc_info));
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
				cycle->shm.id = proc_info.shmid;
				pub_log_info("[%s][%d] 共享内存存在! 不需要创建!", __FILE__, __LINE__);
			}
		}
	}

	cycle->cmd_fd = udp_bind(cycle->base.name.data);
	if (cycle->cmd_fd < 0)
	{
		pub_log_error("%s, %d, udp_bind error! udp_fd=[%d]",
				__FILE__, __LINE__, cycle->cmd_fd);
		cycle->base.proc_info->status = SW_S_ABNORMAL;
		return SW_ERROR;
	}
	pub_log_info("[%s][%d]udp_fd=[%d]", __FILE__, __LINE__, cycle->cmd_fd);

	/*Allcate memory for event object.*/
	cycle->log_fds = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (cycle->log_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(&grp, sizeof(sw_svr_grp_t));
	grp.svc_max = g_alog_cfg.proc_num;
	strcpy(grp.svrname, cycle->base.name.data);
	ret = procs_svr_alloc(&grp);
	if (ret != SW_OK)
	{   
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}   
	pub_log_info("[%s][%d] procs_svr_alloc(log) sucess!",__FILE__,__LINE__);

	/*init select event context.*/
	ret = select_init(cycle->log_fds);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = log_father_register(cycle, SW_S_START);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] log father register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = log_shm_init(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] logs shm init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t log_cycle_run(sw_log_cycle_t *cycle)
{
	sw_int_t  i = 0;
	sw_int_t	ret = 0;
	sw_int_t	recv_cnt = 0;
	sw_int64_t	timer = 0;
	sw_fd_list_t *fd_work;
	sw_fd_set_t	*fd_set = NULL;
	sw_fd_list_t	fd_list;

	fd_set = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (fd_set == NULL)
	{
		pub_log_error("[%s][%d] pub_pool_palloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = select_init(fd_set);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(&fd_list, 0x0, sizeof(fd_list));
	fd_list.fd = cycle->cmd_fd;
	fd_list.data = (void*)cycle;
	fd_list.event_handler = (sw_event_handler_pt)log_handle_control_cmd;
	ret = select_add_event(fd_set, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, select_add_event error fd[%d].", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}

	ret = log_create_child(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_create_child error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	while (1)
	{
		timer = 6000;
		recv_cnt = select_process_events(fd_set, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error!", __FILE__, __LINE__);
			continue;
		}
		else if (recv_cnt == 0)
		{

			ret = log_handle_timeout(cycle);
			if( ret != SW_OK)
			{
				pub_log_error("[%s][%d] log_handle_timeout error !", __FILE__, __LINE__);

			}
		}
		else
		{
			pub_log_info("[%s][%d] Data arrived! recv_cnt=[%d]", __FILE__, __LINE__, recv_cnt);
			for (i = 0; i < recv_cnt; i++)
			{
				pub_log_info("[%s][%d] i=[%d]", __FILE__, __LINE__, i);
				ret = fd_work[i].event_handler((sw_log_cycle_t*)fd_work[i].data);
				if (ret == SW_ABORT)
				{
					pub_log_info("[%s][%d] Recv exit cmd! Exit!", __FILE__, __LINE__);
					return SW_OK;
				}
				else if(ret != SW_OK)
				{
					if(ret == READ_ERROR)
					{
						select_del_event(cycle->log_fds, fd_work[i].fd);
					}
					pub_log_error("[%s][%d] event_handler error!", __FILE__, __LINE__);
				}
			}
		}
	}

	return SW_OK;
}

sw_int_t log_cycle_destroy(sw_log_cycle_t *cycle)
{

	return SW_OK;
}

