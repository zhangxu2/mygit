/*********************************************************************
 *** version : v2.0
 *** author  : wangkun
 *** create  : 2013-4-16
 *** module  : swadmin 
 *** name    : swadmin.c 
 *** function: swadmin's main flow
 *** lists   :
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "pub_type.h"
#include "pub_signal.h"
#include "admin.h"
#include "common.h"
#include "adm_cmd_handler.h"

#define ADMIN_ERR_LOG       "error.log"
#define ADMIN_DEBUG_LOG     "swadmin.log"

extern int check_eswid();

SW_PROTECTED sw_int_t adm_init(sw_adm_cycle_t* cycle,const sw_char_t* name,
			sw_int32_t module_type, const sw_char_t* err_log, const sw_char_t* dbg_log);
SW_PROTECTED sw_int_t adm_run(sw_adm_cycle_t* cycle, sw_int_t argc, const sw_char_t *argv[]);
SW_PROTECTED sw_int_t adm_destroy(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_create_dirs(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int_t adm_link_ipc(sw_adm_cycle_t* cycle);
SW_PROTECTED sw_int32_t adm_read_line(sw_fd_t fd,sw_char_t *line,int len);

void adm_exit(int sig)
{
	pub_log_error("[%s][%d] swadmin catch signal [%d] and exit.", __FILE__, __LINE__,sig);

	exit(1);
}

/******************************************************************************
 ** Name : adm_init
 ** Desc : Initalize the cycle object of amdin
 ** Input: 
 **     cycle: Cycle object
 **     name: The name of module
 **     module_type: The type of module
 **     err_log: The file name of error log
 **     dbg_log: The file name of debug log
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Initalize base cycle
 **     2. Alloc memory for opt
 **     3. Init server amdin uniform interface
 **     4. Init trace admin uniform interface
 **     5. Init cmd handlers
 ** Note :
 ** Author: # Wangkun # 2013.04.17 #
 ******************************************************************************/
sw_int_t adm_init(sw_adm_cycle_t* cycle, const sw_char_t* name, 
            sw_int32_t module_type, const sw_char_t* err_log, const sw_char_t* dbg_log)
{
	sw_int_t ret = SW_ERROR;

	/* Init base class object */
	ret = cycle_init((sw_cycle_t*)&cycle->base, name, module_type, err_log, dbg_log, NULL);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Init cycle failed!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	cycle->cmdfd = SW_INVALID_FD;

	/*  catch SIGTSTP signal */
	signal(SIGTSTP,adm_exit);
	
	/* Allocate memory for opt */
	cycle->opt = pub_pool_palloc(cycle->base.pool, sizeof(sw_adm_opt_t));
	if(NULL == cycle->opt)
	{
		pub_log_error("[%s][%d] Allocate memory fail.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	/* Allocate memory for eswitch service management uniform interface context and init it */
	cycle->uni_svr = pub_pool_palloc(cycle->base.pool, sizeof(sw_uni_svr_t));
	if(NULL == cycle->uni_svr)
	{
		pub_log_error("[%s][%d] Allocate memory fail.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	ret = uni_svr_init(cycle->uni_svr);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Initialize server uniform interface failed.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	/* Allocate memory for eswitch trace management uniform interface context and init it. */
	cycle->trace = pub_pool_palloc(cycle->base.pool, sizeof(uni_trace_t));
	if(NULL == cycle->trace)
	{
		pub_log_error("[%s][%d] Allocate memory fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ret = uni_trace_init(cycle->trace);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Initialize uniform trace failed.", __FILE__, __LINE__);
		return SW_ERROR;
	}


	cycle->opt->mod_type = ADM_SUB_MOD_DFIS;

	adm_opt_set_submod_str(cycle->opt, ADM_MOD_DFIS_STR);

	/* Create needed dirs. */
	ret = adm_create_dirs(cycle);
	if (SW_OK != ret)
	{
		pub_log_error("[%s][%d] adm_create_dirs fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (run_is_init() && g_use_ares)
	{
		if (ares_send_cfg() < 0)
		{
			pub_log_error("[%s][%d] Send config to res server error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	return SW_OK; 
}


/******************************************************************************
 ** Name : adm_link_ipc
 ** Desc : Link ipc
 ** Input: 
 **		cycle: Cycle of Admin
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **		1. Link SHM
 **		2. Create SHM
 **		3. Recover information
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.19 #
 ******************************************************************************/
sw_int_t adm_link_ipc(sw_adm_cycle_t* cycle)
{
	sw_int_t ret = SW_ERR;

	ret = cycle_link_shm_run((sw_cycle_t *)&cycle->base);
	if((ret < 0) && (SHM_MISS != ret))
	{
		pub_log_error("[%s][%d] Link shm failed!", __FILE__, __LINE__);
		return SW_ERR;
	}
	else if(SHM_MISS == ret)
	{
		ret = cycle_create_shm_run((sw_cycle_t *)&cycle->base);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] Create shm failed!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		pub_log_debug("[%s][%d] Create shm success!", __FILE__, __LINE__);
	}

	return SW_OK;
}

/* 打印模块名 */
#define adm_print_submod(cycle) \
    fprintf(stderr, "%s> ", cycle->opt->mod_name);

sw_int_t adm_run(sw_adm_cycle_t* cycle, sw_int_t argc, const sw_char_t **argv)
{
	sw_int_t	i=0, ret = SW_ERROR;
	sw_int64_t	timeout;
	sw_fd_set_t	*fd_set = cycle->base.lsn_fds;
	sw_fd_list_t    fd_list;
	sw_fd_list_t 	*work_list = NULL;
	sw_char_t	line[ADM_CMD_LINE_MAX_LEN] = {0};

	if (SW_OK != adm_link_ipc(cycle))
	{
		pub_log_error("[%s][%d] ipc link error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(&fd_list ,sizeof(sw_fd_list_t));
 	
	if (argc >= 2)
	{
		line[0] = '\0';

		for (i = 1; i < argc; i++)
		{
			strcat(line, argv[i]);
			strcat(line, " ");
		}

		/*Handle cmd.*/
		ret = adm_cmd_handle(cycle, line);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] adm_cmd_handle error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
       		return SW_OK;
	}

	ret = select_init(fd_set);
	if (SW_OK != ret)
	{
		pub_log_error("[%s][%d] select_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	fd_list.fd = STDIN_FILENO;
	
	ret = select_add_event(fd_set, &fd_list);
	if (SW_OK != ret)
	{
		pub_log_error("[%s][%d] select_add_event error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	/* 显示版本号、所有产品信息 */
	adm_list_version(cycle);
	fprintf(stderr, "\n");
	adm_list_prdt_all(cycle);
	fprintf(stderr, "\n");
		
	while (1)
	{
		pub_mem_memzero(line, sizeof(line));

        	adm_print_submod(cycle);
		
		while(1)
		{
			timeout = 100000;

			ret = select_process_events(fd_set, &work_list, timeout);
			if (ret<0)
			{
				if (EINTR == errno)
				{	
					continue;
				}
				else
				{
					pub_log_error("[%s][%d] errmsg:[%d]%s",
							__FILE__, __LINE__, errno, strerror(errno));
					continue;
				}
			}
			else if (0 == ret)
			{			
				pub_log_info("[%s][%d] No input data, exit!", __FILE__, __LINE__);
				pub_log_stderr("\nTime out, no input data, exit.\n");
				return 0;
			}
            
            		break;
		}
		
		ret = adm_read_line(STDIN_FILENO, line, sizeof(line));
		if (ret < 0)
		{
			pub_log_error("[%s][%d] adm_read_line error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
		if (ret == 0 || line[0] == 0)
			continue;
        
		/*Handle cmd.*/
		ret = adm_cmd_handle(cycle, line);
		if(SW_OK != ret)
		{
			pub_log_error("[%s][%d] adm_cmd_handle error.", __FILE__, __LINE__);
			continue;
		}

		pub_log_stderr("\n");
	}

	select_clear(fd_set);
	
	return SW_OK;
}

sw_int_t adm_destroy(sw_adm_cycle_t* cycle)
{	
	Close(cycle->cmdfd);
	
	cycle_destory((sw_cycle_t *)cycle);
	
	return SW_OK;
}

sw_int32_t adm_read_line(sw_fd_t fd,sw_char_t *line,int len)
{
	sw_int32_t ret = 0, i = 0;

	if ( fd < 0 || line == NULL )
	{
		pub_log_error("[%s][%d] adm_read_line input error.", 
		__FILE__, __LINE__);
		return SW_ERROR;	
	}

	while (i < len)
	{
		ret = read(fd ,line+i ,1);
		if ( ret <= 0)
		{
			pub_log_error("[%s][%d] read fd=[%d] error. [%d]", 
					__FILE__, __LINE__,fd,sw_errno);
			return SW_ERROR;
		}

		if (line[i] == '\n')
		{
			line[i] = '\0';
			break;
		}
		if (line[i] == '\t')
		{
			line[i] = ' ';
		}
		if (line[i] == ' ' && (i==0 || line[i-1] == ' '))
		{
			continue;
		}

		i++;
	}
	return i;
}

sw_int_t adm_create_dirs(sw_adm_cycle_t* cycle)
{
	sw_char_t fpath[FILE_NAME_MAX_LEN] = {0};

	/* This function does NOT have a declaration in its header file,
	 * just declare it here for minimum changing  */
	sw_err_t pub_create_full_path(u_char *dir, sw_uint_t access);

	#define CREATE_A_DIR(DIR_NAME) \
    { \
        pub_mem_memzero(fpath, sizeof(fpath)); \
		sprintf(fpath, "%s/%s", cycle->base.work_dir.data,DIR_NAME); \
		if(pub_create_full_path((u_char *)fpath, 0744) != 0) \
		{ \
			pub_log_error(\
			    "creating file [%s]error,errno=[%d],strerror=[%s]", \
			    fpath, errno, strerror(errno)); \
			return SW_ERROR;\
		} \
    }

	CREATE_A_DIR("log");
	CREATE_A_DIR("log/syslog");
	CREATE_A_DIR("tmp");
	CREATE_A_DIR("tmp/callcache");
	CREATE_A_DIR("tmp/calllsncache");
	CREATE_A_DIR("tmp/calllsnsafcache");
	CREATE_A_DIR("tmp/monitor");
	CREATE_A_DIR("tmp/safctl");
	CREATE_A_DIR("tmp/flowdat");
	CREATE_A_DIR("log/default");
	CREATE_A_DIR("tmp/alertfiles");
	CREATE_A_DIR("tmp/logcache");
	
	return SW_OK;
}


int main(int argc, const char *argv[])
{
    sw_int_t ret = SW_ERROR;
    sw_adm_cycle_t cycle;
    
    pub_mem_memzero(&cycle, sizeof(cycle));
	
    ret = check_eswid();
    if (ret < 0)
    {
        fprintf(stderr, "[%s][%d] check eswid error!\n", __FILE__, __LINE__);
        return -1;
    }
    /* 启动日志服务主进程 */
#if defined(__ASYNC_LOG__) || defined(__ASYNC_ULOG__)
    System(PROC_NAME_ALOGSVR);
    usleep(500);
#endif /*__ASYNC_LOG__ || __ASYNC_ULOG__*/

    /* 1. Init swadmin cycle */
    ret = adm_init(&cycle, PROC_NAME_ADM, ND_ADM, ADMIN_ERR_LOG, ADMIN_DEBUG_LOG);
    if(SW_OK != ret)
    {
        pub_log_stderr("[%s][%d] Init admin's cycle failed!\n\n", __FILE__, __LINE__);
        return SW_ERROR;
    }

    pub_log_info("[%s][%d] swadmin running...", __FILE__, __LINE__);

    /* 2. Run swadmin's process loop. */
    ret = adm_run(&cycle, (sw_int_t)argc, argv);
    if(SW_OK != ret)
    {
        fprintf(stderr, "[%s][%d] Run swadmin failed!\n", __FILE__, __LINE__);
        pub_log_error("[%s][%d] Run swadmin failed!", __FILE__, __LINE__);
        return SW_ERROR;
    }

    /* 3. Release swadmin's resource. */
    adm_destroy(&cycle);
    
    return SW_OK;
}
