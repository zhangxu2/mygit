/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swjob 
 *** name    : job_cycle.c
 *** function: swjob life cycle class implement.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "job_cycle.h"
#include "job_schedule.h"
#include "pub_usocket.h"

#define SW_EVENT_LOOP_EXIT -2

SW_PROTECTED sw_int_t job_handle_error(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_handle_timeout(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_handle_control_cmd(sw_job_cycle_t* cycle);
SW_PROTECTED sw_int_t job_handle_job_status(sw_job_cycle_t* cycle);

/******************************************************************************
 *** name      : job_cycle_init
 *** function  : job life cycle object init.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
             name : module name
      module_type : module type
          err_log : error log name
          dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_cycle_init(sw_job_cycle_t* cycle, sw_char_t* name
				, sw_int32_t module_type, sw_char_t* err_log, sw_char_t* dbg_log)
{
	sw_int_t	result = SW_ERROR;
	sw_int32_t	i = 0;
	sw_fd_list_t	fd_list;
	
	/*init base class object.*/
	result = cycle_init((sw_cycle_t*)&cycle->base, name, module_type, err_log, dbg_log,NULL);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*Link shm_run shm_sma shm_ssa.*/
	result = cycle_link_shm((sw_cycle_t *)&cycle->base);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_link_shm.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	cycle->jobs = job_get_addr();

	pub_mem_memzero(&cycle->proc,sizeof(sw_proc_info_t));

	strncpy(cycle->proc.name,name,sizeof(cycle->proc.name)-1);
	cycle->proc.pid = getpid();
	cycle->proc.type = module_type;
	cycle->proc.status = SW_S_START;
	cycle->proc.restart_type = LIMITED_RESTART;
	
	/*Register this module and set restart paramter.*/
	result = procs_sys_register(&cycle->proc);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*init Unix local socket path for job status process.*/
	cycle->job_sock_path.data = pub_pool_palloc(cycle->base.pool, strlen("job_status") + 1);
	if (cycle->job_sock_path.data == NULL)
	{
		pub_log_error("%s, %d, allocate error.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	strcpy(cycle->job_sock_path.data, "job_status");
	cycle->job_sock_path.len = strlen("job_status") + 1;

	/*init job pid.*/
	for(i = 0; i < cycle->jobs->head.count; i++)
	{
		cycle->jobs->job_item[i].job_pid= -1;
		cycle->jobs->job_item[i].job_status = JOB_IDLE;
	}

	/*for job.xml configure file reload*/
	cycle->job_cfg_modify_time = 0;

	cycle->cmd_fd = SW_INVALID_FD;

	cycle->job_fds = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (cycle->job_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*init select event context.*/
	result = select_init(cycle->job_fds);
    	if (result != SW_OK)
    	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		return SW_ERROR;
    	}

	/*bind socket for control cmd.*/
	cycle->cmd_fd = udp_bind(cycle->base.name.data);
	if (cycle->cmd_fd < 0)
    	{
        	pub_log_error("%s, %d, udp_bind error! udp_fd=[%d]"
           		, __FILE__, __LINE__, cycle->cmd_fd);
        	return SW_ERROR;
    	}

	/*bind socket for job exit status process.*/
	cycle->job_status_fd = udp_bind(cycle->job_sock_path.data);
	if(cycle->job_status_fd < 0)
        {
		pub_log_error("%s, %d, udp_bind error! udp_fd=[%d]"
			, __FILE__, __LINE__, cycle->job_status_fd);
		return SW_ERROR;
        }

	/*Add event handler for control cmd.*/
    	pub_mem_memzero(&fd_list, sizeof(fd_list));
    	fd_list.fd = cycle->cmd_fd;
    	fd_list.data = (void*)cycle;
    	fd_list.event_handler = (sw_event_handler_pt)job_handle_control_cmd;

    	result = select_add_event(cycle->job_fds, &fd_list);
    	if (result != SW_OK)
    	{
		pub_log_error("%s, %d, select_add_event error fd[%d]."
			, __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
    	}

	/*Add event handler for job exit status process.*/
    	pub_mem_memzero(&fd_list, sizeof(fd_list));
    	fd_list.fd = cycle->job_status_fd;
    	fd_list.data = (void*)cycle;
    	fd_list.event_handler = (sw_event_handler_pt)job_handle_job_status;

    	result = select_add_event(cycle->job_fds, &fd_list);
    	if (result != SW_OK)
    	{
		pub_log_error("%s, %d, select_add_event error fd[%d]."
			, __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
    	}
    	

	return SW_OK;
}

/******************************************************************************
 *** name      : job_cycle_run
 *** function  : run event handle loop
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_cycle_run(sw_job_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_fd_list_t	*fd_work;
	sw_int64_t	timer = 0;
	sw_int32_t	i = 0;
	sw_int_t	event_mum = 0;

	while(1)
	{
		timer = cycle->base.syscfg->scantime * 1000;
			
		event_mum = select_process_events(cycle->job_fds, &fd_work, timer);
		if (event_mum < 0)
		{
			if (errno == EINTR)
			{
				pub_log_error("%s, %d, time interpute", __FILE__, __LINE__);
				continue;
			}
			else
			{
				/*process error event.*/
				job_handle_error(cycle);
			}
		}
		else if (event_mum == 0)
		{
			/*process time out event.*/
			result = job_handle_timeout(cycle);
			if (result != SW_OK)
			{
				pub_log_error("%s, %d, job_handle_timeout error."
							, __FILE__, __LINE__);
			}
			
			continue;
		}
		else
		{	/*run event handler*/
			for (i = 0; i < event_mum; i++)
			{
				result = fd_work[i].event_handler((sw_job_cycle_t*)fd_work[i].data);
				if (result == SW_EVENT_LOOP_EXIT)
				{
					return SW_OK;
				}
				else if(result == SW_ERROR)
				{
					pub_log_error("%s, %d, event_handler error."
							, __FILE__, __LINE__);
				}
			}
		}
	}
	
	return SW_OK;
}

/******************************************************************************
 *** name      : job_handle_error
 *** function  : error event handler
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_handle_error(sw_job_cycle_t* cycle)
{
	pub_log_error("%s, %d, Event process loop exit abnormally.", __FILE__, __LINE__);
	exit(SW_ERROR);
}

/******************************************************************************
 *** name      : job_handle_timeout
 *** function  : timeout event handler
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_handle_timeout(sw_job_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;

	/*Job schedule.*/
	result = job_schedule(cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job schedule error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : job_handle_control_cmd
 *** function  : event handler for control command processing.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_handle_control_cmd(sw_job_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	cmd_peer[64];
	sw_cmd_t	cmd;
	sw_int32_t	i = 0;
	
	pub_mem_memzero(&cmd, sizeof(sw_cmd_t));
	
	result = udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t));
	if (result < 0)
	{
		pub_log_error("%s, %d, udp rece failed,result=[%d],sizeof(sw_cmd_t)=[%d]",
		__FILE__,__LINE__,result,sizeof(sw_cmd_t));
		return SW_ERROR;
	}
	
	pub_mem_memzero(cmd_peer, sizeof(cmd_peer));
	strncpy(cmd_peer, cmd.udp_name, sizeof(cmd_peer)-1);
	
	switch (cmd.type)
	{
		case SW_MSTOPSELF:
		case SW_MSTOPISELF:
		
			cmd.type = SW_RESCMD;
			pub_mem_memzero(cmd.udp_name, sizeof(cmd.udp_name));
			sprintf( cmd.udp_name, "%s", cycle->base.name.data);
			
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(sw_cmd_t), cmd_peer);
			cycle->proc.status = SW_S_STOPED;
			procs_sys_register(&cycle->proc);
			pub_log_info("[%s][%d]swjob exit normally.", __FILE__, __LINE__);

			
			return SW_EVENT_LOOP_EXIT;
			
		case SW_MSTARTTASK:
		
			cmd.type = SW_RESCMD;
			pub_mem_memzero(cmd.udp_name, sizeof(cmd.udp_name));
			sprintf( cmd.udp_name, "%s", cycle->base.name.data);
			
			char	job_no[64];
			sw_job_t	*job = cycle->jobs;
			
			memset(job_no, 0x0, sizeof(job_no));
			strncpy(job_no, cmd.lsn_name, sizeof(job_no) - 1);

			for (i = 0; i < job->head.cur_cnt ; i++ )
			{
				if (job->job_item[i].job_name[0] != '\0' && strcmp(job->job_item[i].no, job_no) == 0)
				{
					break;
				}
			}

			if (i == job->head.cur_cnt || job->job_item[i].job_name[0] == '\0')
			{
				pub_log_error("%s, %d, Job[%s] not found!", __FILE__, __LINE__, job_no);
				cmd.type = SW_ERRCMD;
				udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(sw_cmd_t), cmd_peer);
				break;
			}
			
			if (job->job_item[i].manual != JOB_MANUAL)
			{
				pub_log_error("[%s][%d] This job is not a manual job! [%s]", __FILE__, __LINE__, job_no);
				cmd.type = SW_ERRCMD;
				udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(sw_cmd_t), cmd_peer);
				break;
			}
			
			pub_log_debug("[%s][%d] job_no=[%s]", __FILE__, __LINE__, job_no);
			result = job_run_by_no(cycle, cmd.lsn_name);
			if (result != SW_OK)
			{
				pub_log_error("%s, %d, job_run_by_no error.", __FILE__, __LINE__);
				cmd.type = SW_ERRCMD;
			}
			
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(sw_cmd_t), cmd_peer);
			
			pub_log_info("%s, %d, Receive cmd, job_no=[%s]", __FILE__, __LINE__, cmd.lsn_name);
						
			break;
			
		default:
			/*unknown control cmd*/
			cmd.type= SW_ERRCMD;
			pub_mem_memzero(cmd.udp_name, sizeof(cmd.udp_name));
			sprintf( cmd.udp_name , "%s" , cycle->base.name.data);
			udp_send(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), cmd_peer);
	
			break;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : job_handle_job_status
 *** function  : event handler for job child process exit status processing.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_handle_job_status(sw_job_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_job_status_t job_status;
	sw_int32_t	i = 0;
	
	/*process job's exit status.*/
	pub_mem_memzero(&job_status, sizeof(job_status));

	result = udp_recv(cycle->job_status_fd, (char *)&job_status, sizeof(job_status));
	if(result < 0  && result != -4)
	{
		pub_log_error("[%s][%d], udp rece failed,iRc=[%d],sizeof(job_status)=[%d],error[%s]"
					, __FILE__, __LINE__, result, sizeof(job_status), strerror(errno));
		return SW_ERROR;
	}

	pub_log_info("[%s][%d], job_status pid[%d], status[%c], lasttime[%ld]", 
		__FILE__, __LINE__, job_status.job_pid, job_status.return_code, job_status.last_run_time);

	switch(job_status.return_code)
	{
		case JOB_ERROR:
		case JOB_FINISHED:

			for(i = 0; i < cycle->jobs->head.cur_cnt; i++)
			{
				if(cycle->jobs->job_item[i].job_pid == job_status.job_pid 
					&& cycle->jobs->job_item[i].job_name[0] != '\0')
				{
					cycle->jobs->job_item[i].job_status = job_status.return_code;
					cycle->jobs->job_item[i].last_run_time = job_status.last_run_time;
					cycle->jobs->job_item[i].job_pid = -1;
				}
			}
		
		break;

		default:
			pub_log_error("[%s][%d],Unknown job status %c."
					,__FILE__,__LINE__,job_status.return_code);
		
			return SW_ERROR;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : job_cycle_destroy
 *** function  : destory job life cycle object, release resource.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : job cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t job_cycle_destroy(sw_job_cycle_t* cycle)
{
	close(cycle->cmd_fd);
	close(cycle->job_status_fd);
	cycle_destory((sw_cycle_t *)cycle);
	
	return SW_OK;
}

