/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swpol 
 *** name    : pol_cycle.c
 *** function: swpol life cycle class implement.
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "pol_cycle.h"
#include "pub_proc.h"
#include "alert.h"
#include "pub_usocket.h"

#define SW_EVENT_LOOP_EXIT -2

SW_PROTECTED sw_int_t pol_handle_error(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t pol_handle_timeout(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t pol_handle_control_cmd(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t pol_scan_proc_table(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t pol_start_proc(sw_proc_info_t *proc);
SW_PROTECTED sw_int_t pol_update_work_status(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t pol_update_eswitch_date(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t pol_calculate_date(char *old, char *current, int *day_num);
SW_PROTECTED sw_int_t pol_reset_job_info(sw_pol_cycle_t* cycle);
SW_PROTECTED sw_int_t change_vfs_date(char *vfs_date, char *new_date, int day_num);

/******************************************************************************
 *** name      : pol_cycle_init
 *** function  : pol life cycle object init.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
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
sw_int_t pol_cycle_init(sw_pol_cycle_t* cycle, sw_char_t* name
				, sw_int32_t module_type, sw_char_t* err_log, sw_char_t* dbg_log)
{
	sw_int_t	result = SW_ERROR;
	sw_fd_list_t	fd_list;

	/*init base class object.*/
	result = cycle_init((sw_cycle_t*)&cycle->base, name, module_type, err_log, dbg_log,NULL);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*Link shm_run shm_sma shm_ssa.*/
	result = cycle_link_shm((sw_cycle_t*)&cycle->base);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_link_shm.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(&cycle->procs,sizeof(cycle->procs));

	result = procs_get_sys_by_name(name,&cycle->procs);
	if (result != SW_ERR)
	{
		if ( pub_proc_checkpid(cycle->procs.pid)== SW_OK)
		{
			pub_log_error("%s, %d, [%s] have started,can't start again.", __FILE__, __LINE__,name);
			return SW_ERROR;
		}
	}
	strncpy(cycle->procs.name,name,sizeof(cycle->procs.name)-1);
	cycle->procs.type = module_type;
	cycle->procs.pid = getpid();
	cycle->procs.status = SW_S_START;
	/*Register this process to sma and set some restart paramters..*/
	result = procs_sys_register(&cycle->procs);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_cycle_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/*bind socket for control cmd.*/
	cycle->cmd_fd = udp_bind(cycle->base.name.data);
	if (cycle->cmd_fd < 0)
    	{
           pub_log_error("%s, %d, udp_bind error! udp_fd=[%d]", __FILE__, __LINE__, cycle->cmd_fd);
           return SW_ERROR;
    	}

	cycle->pol_fds = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (cycle->pol_fds == NULL)
	{
		pub_log_error("%s, %d, allocate error.", __FILE__,__LINE__);
		return SW_ERROR;
	}

	/*init select event context.*/
    	result = select_init(cycle->pol_fds);
    	if (result != SW_OK)
    	{
		pub_log_error("%s, %d, select_init error.", __FILE__, __LINE__);
		return SW_ERROR;
    	}

	/*Add event handler for control cmd process.*/
    	pub_mem_memzero(&fd_list, sizeof(fd_list));
    	fd_list.fd = cycle->cmd_fd;
    	fd_list.data = (void*)cycle;
    	fd_list.event_handler = (sw_event_handler_pt)pol_handle_control_cmd;

    	result = select_add_event(cycle->pol_fds, &fd_list);
    	if (result != SW_OK)
    	{
		pub_log_error("%s, %d, select_add_event error fd[%d].", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
    	}

	return SW_OK;
}

/******************************************************************************
 *** name      : pol_cycle_run
 *** function  : run event loop
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_cycle_run(sw_pol_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_fd_list_t	*fd_work;
	sw_int64_t	timer = 0;
	sw_int32_t	i = 0;
	sw_int_t	event_mum = 0;

	while(1)
	{
		timer = cycle->base.syscfg->scantime * 1000;
		
		event_mum = select_process_events(cycle->pol_fds, &fd_work, timer);
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
				pol_handle_error(cycle);
			}
		}
		else if (event_mum == 0)
		{
			/*process time out event.*/
			result = pol_handle_timeout(cycle);
			if (result != SW_OK)
			{
				pub_log_error("%s, %d, pol_handle_timeout error."
							, __FILE__, __LINE__);
			}
			
			continue;
		}
		else
		{
			/*Exec event handler.*/
			pub_log_info("receive control cmd.");
			for (i = 0; i < event_mum; i++)
			{
				result = fd_work[i].event_handler((sw_pol_cycle_t*)fd_work->data);
				if (result == SW_EVENT_LOOP_EXIT)
				{
					return SW_OK;
				}
				else if(result == SW_ERROR)
				{
					pub_log_error("%s, %d, pol_handle_control_cmd error."
							, __FILE__, __LINE__);
				}
			}

		}
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : pol_handle_error
 *** function  : error event handler
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_handle_error(sw_pol_cycle_t* cycle)
{
	pub_log_error("%s, %d, Event process loop exit abnormally.", __FILE__, __LINE__);
	exit(SW_ERROR);
}

/******************************************************************************
 *** name      : pol_handle_timeout
 *** function  : timeout event handler
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_handle_timeout(sw_pol_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	
	/*Save esw's trace info to file, for trace info recover.*/
	result = seqs_save(1);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, iSaveTraceInfo2File error",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*Scan sma table, if some process exit abnormally, start it.*/
	pol_scan_proc_table(cycle);

	/*Update esw's work status according to sma table info.*/
	pol_update_work_status(cycle);

	/*Update eswitch's date. If eswitch date change, do some callback.*/
	result = pol_update_eswitch_date(cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pol_update_eswitch_date error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : pol_scan_proc_table
 *** function  : Scan sma, if some process exit abnormally, start it according its
                 restart type and restart count.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_scan_proc_table(sw_pol_cycle_t* cycle)
{
	long lnow = 0;
	sw_int_t	result = SW_ERROR;
	sw_int32_t	index = 0;
	
	sw_procs_head_t head;
	sw_proc_info_t proc;

	result = procs_get_head(&head);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] procs_get_head",__FILE__,__LINE__);
		return SW_OK;
	}
	
	for(index = 0; index < head.sys_proc_use;index++)
	{
		result = procs_get_sys_by_index(index,&proc);
		if (result != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_sys_by_index error! index=[%d]",
			__FILE__,__LINE__,index);
			return SW_OK;
		}
		
		if(proc.name[0] != '\0' && proc.status != SW_S_STOPED && proc.status != SW_S_ABORTED &&
			proc.pid > 0 && pub_proc_checkpid(proc.pid) != SW_OK)
		{
			pub_log_info("[%s][%d] PROC [%s] ABNORMAL!", __FILE__, __LINE__, proc.name);
			/*Process exit abnormally, restart it.*/
			if (proc.restart_type == LIMITED_RESTART && proc.restart_cnt >= 0)
			{
				pub_log_info("restart %s restart_type=%d restart_cnt=%d."
					, proc.name,proc.restart_type,proc.restart_cnt);
				lnow = (long)time(NULL);
				if (lnow - proc.starttime < MIN_RESTART_TIME)
				{
					alert_msg(ERR_OFTEN_RESTART_FAILED, "进程[%s]在5分钟之内重启多次,请手动查看日志进行处理.", proc.name);
					pub_log_error("[%s][%d] [%s][%d] Time is less than 5 minutes, could not restart!",__FILE__, __LINE__, proc.name, proc.pid);
					proc.status = SW_S_ABNORMAL;
					proc.restart_cnt = -1;
					result = procs_sys_register(&proc);
					if (result != SW_OK)
					{
						pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
					}
					continue;
				}
				if (proc.restart_cnt == 0)
				{
					alert_msg(ERR_CNT_RESTART_FAILED, "紧急预警:管理进程[%s]重启次数过多,不能自动重启,请尽快查看渠道日志进行解决!", proc.name);
					proc.status = SW_S_ABNORMAL;
					proc.restart_cnt--;
					result = procs_sys_register(&proc);
					if (result != SW_OK)
					{
						pub_log_error("[%s][%d] Register [%s] error!", __FILE__, __LINE__, proc.name);
					}
					continue;
				}
				proc.status = SW_S_ABNORMAL;
				proc.restart_cnt--;
				result = procs_sys_register(&proc);
				if (result != SW_OK)
				{
					pub_log_error("[%s][%d] Register [%s] error!", __FILE__, __LINE__, proc.name);
				}
				result = pol_start_proc(&proc);
				if (result != SW_OK)
				{
					pub_log_error("[%s][%d] Restart proc [%s] error!", __FILE__, __LINE__, proc.name);
				}
			}
			else
			{
				pub_log_info("[%s][%d] Can not restart [%s], restart_type=[%d] restart_cnt=[%d]",
					__FILE__, __LINE__, proc.name, proc.restart_type, proc.restart_cnt);
			}
		}
	}
	
	return SW_OK;
}

/******************************************************************************
 *** name      : pol_start_proc
 *** function  : Start number of cnt eswitch process.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
            name  : eswitch process name
            cnt   : eswitch process count
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_start_proc(sw_proc_info_t *proc)
{
	sw_int32_t result;
	sw_char_t  exe_name[NAME_LEN];
	sw_char_t  argv[NAME_LEN];

	if(proc == NULL ||proc->name[0] == '\0')
	{
		pub_log_error("%s, %d, Param error, name[%s], cnt=[%d]\n", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_mem_memzero(exe_name,sizeof(exe_name));
	pub_mem_memzero(argv,sizeof(argv));	
	strcpy(argv,proc->name);
	char *p=NULL;
	char name[64]={0};
	p=strstr(proc->name, "_");
	if (p != NULL)
	{
		strcpy(name, p+1);
		pub_log_info("[%s][%d] proc->name=[%s], name=[%s], type=[%d]", __FILE__, __LINE__, proc->name, name, proc->type);
	}
	switch (proc->type)
	{
		case ND_LSN:
			strcpy(exe_name,PROC_NAME_LSN);
			pub_log_info("[%s][%d] pol restart lsn [%s]  ",__FILE__,__LINE__,proc->name);
			break;
		case ND_SVC:
			pub_mem_memzero(argv,sizeof(argv));	
			strcpy(argv,proc->prdt_name);
			strcpy(exe_name,PROC_NAME_SVC_MAN);			
			pub_log_info("[%s][%d] pol restart svr [%s]  ",__FILE__,__LINE__,proc->name);
			break;
		case ND_ALERT:
			strcpy(exe_name,PROC_NAME_ALERT);			
			pub_log_info("[%s][%d] pol restart alter [%s]  ",__FILE__,__LINE__,proc->name);
			break;
		case ND_JOB :
			strcpy(exe_name,PROC_NAME_JOB);
			pub_log_info("[%s][%d] pol restart job [%s]  ",__FILE__,__LINE__,proc->name);
			break;
		case ND_LOG :
			strcpy(exe_name,PROC_NAME_LOG);			
			pub_log_info("[%s][%d] pol restart log [%s]  ",__FILE__,__LINE__,proc->name);
			break;
		default:
			pub_log_error("[%s][%d] sys proc name=[%s] tpye=[%d] unkonwn",
			__FILE__,__LINE__,proc->name,proc->type);
			
			return SW_ERR;
	}
	result = fork();
	if(0 == result)
	{
		execlp(exe_name, exe_name, name, NULL); 
		alert_msg(ERR_PROC_FAILED, "紧急预警:管理进程[%s]异常退出后重启失败,请尽快查看渠道日志进行解决!", proc->name);
		pub_log_info("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		exit(SW_OK);
	}
	
	return SW_OK;
}

/******************************************************************************
 *** name      : pol_update_work_status
 *** function  : Update work status in sma according to all eswitch process status.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_update_work_status(sw_pol_cycle_t* cycle)
{
	/* TODO test*/
#if 0
	sw_int32_t	stat = 0;
	sw_int32_t	stop = 0;
	sw_int32_t	index = 0;
	
	for(index = 0; index < cycle->base.global_cfg->params.iSmaCnt; index++)
	{
		if (cycle->base.shm_sma->pSma[index].sName[0] != '\0' 
			&& memcmp(cycle->base.shm_sma->pSma[index].sName, "sw", 2) == 0 
			&& cycle->base.shm_sma->pSma[index].iPid > 0 
			&& nCheckPid(cycle->base.shm_sma->pSma[index].iPid) 
			&& cycle->base.shm_sma->pSma[index].iStatus != SW_S_START
			&& cycle->base.shm_sma->pSma[index].iStatus != SW_S_STOPED)
		{
			stat = 1;
		}
		
		if (cycle->base.shm_sma->pSma[index].iStatus == SW_S_STOPED
			|| cycle->base.shm_sma->pSma[index].iStatus == SW_S_KILL)
		{
			stop++;
		}
	}

	if (stop == cycle->base.shm_sma->stSmaHead.iCount)
	{
		cycle->base.shm_sma->stSmaHead.sw_work_stat = SW_ALL_STOP;
		return SW_OK;
	}
	if (stat == 1)
	{
		if ( cycle->base.shm_sma->stSmaHead.sw_work_stat == SW_ALL_START)
		{
			cycle->base.shm_sma->stSmaHead.sw_work_stat = SW_ABNORMAL;
		}
	}
	else if (stat == 0)
	{
		if ( cycle->base.shm_sma->stSmaHead.sw_work_stat != SW_ALL_START)
		{
			cycle->base.shm_sma->stSmaHead.sw_work_stat = SW_ALL_START;
		}
	}

#endif
	return SW_OK;
}

/******************************************************************************
 *** name      : pol_update_eswitch_date
 *** function  : Update work status in sma according to all eswitch process status.
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : pol cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_update_eswitch_date(sw_pol_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_int32_t	day_num = 0;
	sw_char_t	old_sys_date[10];
	sw_char_t	vfs_sys_date[10];
	sw_char_t	new_sys_date[32];
	sw_seqs_head_t 	esw_status;

	pub_mem_memzero(old_sys_date, sizeof(old_sys_date));
	pub_mem_memzero(vfs_sys_date, sizeof(vfs_sys_date));

	result = seqs_get(&esw_status);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d]seqs_get error!",__FILE__, __LINE__);
		return SW_ERROR;
	}
	
	result = seqs_get_curdate(old_sys_date);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d]iGetSysDate error,errno=[%d][%s]"
				, __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	
	if (esw_status.date_lock_flag == 1)
	{
		strncpy(vfs_sys_date, esw_status.vfs_date, 10);
		pub_time_app_get_time(NULL, 0, new_sys_date);
		new_sys_date[8]='\0';
		
		pub_log_info("[%s][%d]old_sys_date=[%s],new_sys_date=[%s],[%s][%d]"
				,__FILE__,__LINE__,old_sys_date,new_sys_date,vfs_sys_date,day_num);

		pol_calculate_date(vfs_sys_date, new_sys_date, &day_num);

		pub_log_info("[%s][%d]old_sys_date=[%s],new_sys_date=[%s],[%s][%d]"
				, __FILE__, __LINE__, old_sys_date, new_sys_date, vfs_sys_date, day_num);
		
		if (strncmp(vfs_sys_date, new_sys_date, 8) != 0)
		{
			strncpy(esw_status.vfs_date, new_sys_date, 8);
			change_vfs_date(old_sys_date, new_sys_date, day_num);
			
			result = seqs_change_date(new_sys_date);
			if (result != SW_OK)
			{
				pub_log_error("[%s][%d]iChangeSwSysDate error",__FILE__,__LINE__);
				return SW_ERROR;
			}
		}
	}
	else if (esw_status.date_lock_flag== 0)
	{
		pub_time_app_get_time(NULL, 0, new_sys_date);
		new_sys_date[8]='\0';
		if (strncmp(old_sys_date, new_sys_date, 8) != 0)
		{
			result = seqs_change_date(new_sys_date);
			if (result != SW_OK)
			{
		    		pub_log_error("[%s][%d]iChangeSwSysDate error",__FILE__,__LINE__);
				return SW_ERROR;
			}
		}
	}
	
	return SW_OK;
}

/******************************************************************************
 *** name      : pol_calculate_date
 *** function  : current - old = day_num
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  old : old time
        current : current time
        day_num : diff of old and current
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_calculate_date(char *old, char *current, int *day_num)
{
    struct tm	old_timestamp;
    struct tm	current_timestamp;
    time_t	old_time;
    time_t	current_time;
    sw_int32_t	old_day;
    sw_int32_t	current_day;

    memset(&old_timestamp, 0x00, sizeof(old_timestamp));
    memset(&current_timestamp, 0x00, sizeof(current_timestamp));

    old_day = atoi(old);
    current_day = atoi(current);
    old_timestamp.tm_year = old_day/10000 - 1900;
    old_timestamp.tm_mon = (old_day%10000)/100 - 1;
    old_timestamp.tm_mday = old_day%100;

    current_timestamp.tm_year = current_day/10000 - 1900;
    current_timestamp.tm_mon = (current_day%10000)/100 - 1;
    current_timestamp.tm_mday = current_day%100;

    old_time = mktime(&old_timestamp);
    current_time = mktime(&current_timestamp);

    *day_num = (current_time - old_time) / (3600 * 24);

    return SW_OK;
}

/******************************************************************************
 *** name      : pol_reset_job_info
 *** function  : current - old = day_num
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	cycle : pol life cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_reset_job_info(sw_pol_cycle_t* cycle)
{
#if 0
	sw_int32_t	i = 0;

	while(i < MAXJOB && cycle->jobs[i].sJobName[0] != '\0')
	{
		cycle->jobs[i].cJobStatus = JOB_IDLE;
		i++;
	}
#endif
	return SW_OK;
}

/******************************************************************************
 *** name      : change_vfs_date
 *** function  : current - old = day_num
 *** author    : wangkun
 *** create    : 2013-05-08
 *** call lists: 
 *** inputs    : 
 *** 	  old : old time
        current : current time
        day_num : diff of old and current
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t change_vfs_date(char *vfs_date, char *new_date, int day_num)
{
    struct tm	timestamp;
    sw_int32_t	time_num;
    time_t time;
    time_t time_new;

    pub_mem_memzero(&timestamp, sizeof(timestamp));
    time_num = atoi(vfs_date);
    timestamp.tm_year = time_num / 10000 - 1900;
    timestamp.tm_mon = (time_num % 10000) / 100 - 1;
    timestamp.tm_mday = time_num % 100;

    time = mktime(&timestamp);
    time_new = time + ((3600 * 24) * day_num);
    pub_time_change_time(&time_new, new_date, 3);
    
    return SW_OK;
}

/******************************************************************************
 *** name      : pol_handle_control_cmd
 *** function  : Event handler for control command process.
 *** author    : wangkun
 *** create    : 2013-05-09
 *** call lists: 
 *** inputs    : 
 *** 	cycle : pol life cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_handle_control_cmd(sw_pol_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	cmd_peer[64];
	sw_cmd_t	cmd;
	
	pub_mem_memzero(&cmd,sizeof(sw_cmd_t));
	
	result = udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t));
	if (result < 0)
	{
		pub_log_error("%s, %d, udp rece failed,result=[%d],sizeof(struct stCommCmd)=[%d]",__FILE__,__LINE__,result,sizeof(sw_cmd_t));
		return SW_ERROR;
	}
	
	pub_mem_memzero(cmd_peer, sizeof(cmd_peer));
	strncpy(cmd_peer, cmd.udp_name, sizeof(cmd_peer)-1);
	
	switch (cmd.type)
	{
		case SW_MSTOPSELF:
		case SW_MSTOPISELF:
		
			cmd.type  = SW_RESCMD;
			pub_mem_memzero(cmd.udp_name, sizeof(cmd.udp_name));
			sprintf( cmd.udp_name ,"%s" ,cycle->base.name.data);
			
			udp_send(cycle->cmd_fd,(char*)&cmd,sizeof(sw_cmd_t),cmd_peer);
			cycle->procs.status=SW_S_STOPED;
			procs_sys_register(&cycle->procs);
			pub_log_info("swpol exit normally.");
			
			return SW_EVENT_LOOP_EXIT;
			
		default:
			/*unknown control cmd*/
			cmd.type = SW_ERRCMD;
			pub_mem_memzero(cmd.udp_name, sizeof(cmd.udp_name));
			sprintf( cmd.udp_name, "%s" , cycle->base.name.data);
			udp_send(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), cmd_peer);
	
			break;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : pol_cycle_destroy
 *** function  : Destroy pol life cycle object and release its resource.
 *** author    : wangkun
 *** create    : 2013-05-09
 *** call lists: 
 *** inputs    : 
 *** 	cycle : pol life cycle object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pol_cycle_destroy(sw_pol_cycle_t* cycle)
{
	close(cycle->cmd_fd);
	select_clear(cycle->pol_fds);
	cycle_destory((sw_cycle_t *) &cycle->base);
	
	return SW_OK;
}

