/*********************************************************************
 *** version : v2.0
 *** author  : wangkun
 *** create  : 2013-4-16
 *** module  : swadmin 
 *** name    : adm_opt.c 
 *** function: option parse and process
 *** lists   :
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "adm_opt.h"
#include "pub_log.h"
#include "pub_mem.h"

extern int check_regist();

/* DFIS-BP命令解析 */
static adm_cmd_type_e adm_opt_def_parse_start(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_stop(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_task(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_list(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_clean(const sw_adm_opt_t *opt);

static adm_cmd_type_e adm_opt_def_parse_top(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_step(const sw_adm_opt_t *opt);

static adm_cmd_type_e adm_opt_def_parse_set(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_get(const sw_adm_opt_t *opt);

/* 产品命令解析 */
static adm_cmd_type_e adm_opt_prdt_parse_start(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_prdt_parse_stop(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_prdt_parse_list(const sw_adm_opt_t *opt);
static adm_cmd_type_e adm_opt_def_parse_saf(const sw_adm_opt_t *opt);

SW_PROTECTED sw_int_t adm_opt_def_get_cmd_type(sw_adm_opt_t* opt);
SW_PROTECTED sw_int_t adm_opt_prdt_get_cmd_type(sw_adm_opt_t* opt);

typedef struct
{
	adm_mod_type_t sub_mod;
	opt_get_cmd_type_func_t get_cmd_type;
}adm_opt_get_cmd_type_t;

/* 命令类型匹配 */
#define adm_opt_is_start(opt)   \
	(!strcasecmp(opt->local_argv[0], "start") || !strcasecmp(opt->local_argv[0], "s"))
#define adm_opt_is_stop(opt) (!strcasecmp(opt->local_argv[0], "stop"))
#define adm_opt_is_quit(opt)    \
	(!strcasecmp(opt->local_argv[0], "quit") \
	 || !strcasecmp(opt->local_argv[0], "exit") \
	 || !strcasecmp(opt->local_argv[0], "q"))
#define adm_opt_is_swcfg(opt)   (!strcasecmp(opt->local_argv[0], "swcfg"))
#define adm_opt_is_swtrace(opt) (!strcasecmp(opt->local_argv[0], "swtrace"))
#define adm_opt_is_top(opt)     (!strcasecmp(opt->local_argv[0], "top"))
#define adm_opt_is_step(opt)    (!strcasecmp(opt->local_argv[0], "step"))
#define adm_opt_is_set(opt)     (!strcasecmp(opt->local_argv[0], "set"))
#define adm_opt_is_get(opt)		(!strcasecmp(opt->local_argv[0], "get"))
#define adm_opt_is_clear(opt)   (!strcasecmp(opt->local_argv[0], "clear"))
#define adm_opt_is_clean(opt)   (!strcasecmp(opt->local_argv[0], "clean"))
#define adm_opt_is_task(opt)    (!strcasecmp(opt->local_argv[0], "task"))
#define adm_opt_is_saf(opt)    (!strcasecmp(opt->local_argv[0], "saf"))
#define adm_opt_is_goto(opt)    (!strcasecmp(opt->local_argv[0], "goto"))
#define adm_opt_is_deploy(opt)  (!strcasecmp(opt->local_argv[0], "deploy"))
#define adm_opt_is_reload(opt) (!strcasecmp(opt->local_argv[0], "reload"))
#define adm_opt_is_startup(opt) (!strcasecmp(opt->local_argv[0], "startup"))
#define adm_opt_is_shutdown(opt) (!strcasecmp(opt->local_argv[0], "shutdown"))
#define adm_opt_is_help(opt)    \
	(!strcasecmp(opt->local_argv[0], "help") || !strcasecmp(opt->local_argv[0], "h"))
#define adm_opt_is_list(opt)    \
	(!strcasecmp(opt->local_argv[0], "list") \
	 || !strcasecmp(opt->local_argv[0], "l") \
	 || !strcasecmp(opt->local_argv[0], "ls"))

/*============================================================================*/
/* 命令类型和命令解析的对应关系表 */
static const adm_opt_get_cmd_type_t g_adm_opt_get_cmd_type[] = 
{
	{ADM_SUB_MOD_DFIS, adm_opt_def_get_cmd_type}
	, {ADM_SUB_MOD_PRDT, adm_opt_prdt_get_cmd_type}
	, {ADM_SUB_MOD_ALL, adm_opt_def_get_cmd_type}
};

adm_cmd_type_e adm_opt_get_cmd_type(sw_adm_opt_t *opt)
{
	int	idx = 0;
	int	size = 0;

	size = sizeof(g_adm_opt_get_cmd_type);
	for(idx=0; idx<size; idx++)
	{
		if(opt->mod_type == g_adm_opt_get_cmd_type[idx].sub_mod)
		{
			return g_adm_opt_get_cmd_type[idx].get_cmd_type(opt);
		}
	}
	return ADM_CMD_UNKNOWN;
}

/*============================================================================*/


SW_PROTECTED bool adm_str_is_all_space(sw_char_t* line)
{
	sw_int32_t    i = 0;

	for (i = 0; line[i] != '\0'; i++)
	{
		if(line[i] == '\t')
		{
			line[i] = ' ';
		}
	}

	for (i = 0; line[i] != '\0'; i++)
	{
		if( line[i] != ' ')
		{
			return false;
		}
	}

	return true;
}

SW_PROTECTED void adm_str_format_space(sw_char_t* line)
{
	sw_int32_t    i = 0;
	sw_int32_t    j = 0;
#define START    1
#define BLACK    2
#define    SPACE    3
	sw_int32_t    status = START;

	for (i = 0; line[i] != '\0'; i++)
	{
		if(line[i] == '\t')
		{
			line[i] = ' ';
		}
	}

	j = 0;
	for (i = 0; line[i] != '\0'; i++)
	{
		if(status == START)
		{
			if(line[i] == ' ')
			{
				continue;
			}
			else
			{
				status = BLACK;
				line[j++] = line[i];
			}
		}

		else if(status == BLACK)
		{
			if(line[i] == ' ')
			{
				status = SPACE;
				line[j++] = line[i];
			}
			else
			{
				line[j++] = line[i];
			}
		}

		else if(status == SPACE)
		{
			if(line[i] == ' ')
			{
				continue;
			}
			else
			{
				status = BLACK;
				line[j++] = line[i];
			}
		}
	}

	if(line[j - 1] == ' ')
	{
		line[j - 1] = '\0';
	}
	else if(status == BLACK)
	{
		line[j] = '\0';
	}

	return;
}

/******************************************************************************
 ** Name : adm_opt_def_get_cmd_type
 ** Desc : Get command type
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.28 #
 ******************************************************************************/
SW_PROTECTED sw_int_t adm_opt_def_get_cmd_type(sw_adm_opt_t* opt)
{
	if(adm_opt_is_start(opt))
	{
		opt->cmd_type = adm_opt_def_parse_start(opt);
	}
	else if(adm_opt_is_quit(opt))
	{
		opt->cmd_type = ADM_CMD_QUIT;
	}
	else if(adm_opt_is_help(opt))
	{
		opt->cmd_type = ADM_CMD_HELP;
	}

	else if(adm_opt_is_top(opt))
	{
		opt->cmd_type = adm_opt_def_parse_top(opt);
	}
	else if(adm_opt_is_step(opt))
	{
		opt->cmd_type = adm_opt_def_parse_step(opt);
	}

	else if(adm_opt_is_set(opt))
	{
		opt->cmd_type = adm_opt_def_parse_set(opt);
	}
	else if(adm_opt_is_get(opt))
	{
		opt->cmd_type = adm_opt_def_parse_get(opt);
	}
	else if(adm_opt_is_stop(opt))
	{
		opt->cmd_type = adm_opt_def_parse_stop(opt);
	}
	else if(adm_opt_is_list(opt))
	{
		opt->cmd_type = adm_opt_def_parse_list(opt);
	}
	else if(adm_opt_is_clear(opt))
	{
		opt->cmd_type = ADM_CMD_CLEAR;
	}
	else if(adm_opt_is_clean(opt))
	{
		opt->cmd_type = adm_opt_def_parse_clean(opt);
	}
	else if(adm_opt_is_task(opt))
	{
		opt->cmd_type = adm_opt_def_parse_task(opt);
	}
	else if(adm_opt_is_saf(opt))
	{
		opt->cmd_type = adm_opt_def_parse_saf(opt);
	}
	else if(adm_opt_is_goto(opt))
	{
		opt->cmd_type = ADM_CMD_GOTO_PRDT;
	}
	else if (adm_opt_is_deploy(opt))
	{
		opt->cmd_type = ADM_CMD_DEPLOY;             
	}
	else if (adm_opt_is_reload(opt))
	{
		opt->cmd_type = ADM_CMD_RELOAD;
	}
	else if (adm_opt_is_startup(opt))
	{
		opt->cmd_type = ADM_CMD_STARTUP;
	}
	else if (adm_opt_is_shutdown(opt))
	{
		opt->cmd_type = ADM_CMD_SHUTDOWN;
	}
	else
	{
		pub_log_debug("[%s][%d] Unknown command. [%s]",
				__FILE__, __LINE__, opt->local_argv[0]);
		opt->cmd_type = ADM_CMD_UNKNOWN;
	}

	return SW_OK;

}

/******************************************************************************
 ** Name : adm_opt_prdt_get_cmd_type
 ** Desc : Get command type of product
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.24 #
 ******************************************************************************/
SW_PUBLIC  sw_int_t adm_opt_prdt_get_cmd_type(sw_adm_opt_t* opt)
{
	if(adm_opt_is_start(opt))
	{
		opt->cmd_type = adm_opt_prdt_parse_start(opt);
	}
	else if(adm_opt_is_quit(opt))
	{
		opt->cmd_type = ADM_CMD_QUIT;
	}
	else if(adm_opt_is_help(opt))
	{
		opt->cmd_type = ADM_CMD_HELP;
	}

	else if(adm_opt_is_top(opt))
	{
		opt->cmd_type = adm_opt_def_parse_top(opt);
	}
	else if(adm_opt_is_step(opt))
	{
		opt->cmd_type = adm_opt_def_parse_step(opt);
	}

	else if(adm_opt_is_set(opt))
	{
		opt->cmd_type = adm_opt_def_parse_set(opt);
	}
	else if(adm_opt_is_get(opt))
	{
		opt->cmd_type = adm_opt_def_parse_get(opt);
	}
	else if(adm_opt_is_stop(opt))
	{
		opt->cmd_type = adm_opt_prdt_parse_stop(opt);
	}
	else if(adm_opt_is_list(opt))
	{
		opt->cmd_type = adm_opt_prdt_parse_list(opt);
	}
	else if(adm_opt_is_goto(opt))
	{
		opt->cmd_type = ADM_CMD_GOTO_PRDT;
	}
	else
	{
		pub_log_debug("[%s][%d] Unknown command. [%s]",
				__FILE__, __LINE__, opt->local_argv[0]);
		opt->cmd_type = ADM_CMD_UNKNOWN;
	}

	return SW_OK;

}

/******************************************************************************
 ** Name : adm_opt_parse
 ** Desc : Parse command line
 ** Input: 
 **     opt: Operator command information
 **     line: Command line
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Wangkun # 2013.06.22 #
 ******************************************************************************/
sw_int_t adm_opt_parse(sw_adm_opt_t* opt, sw_char_t* line)
{
	sw_int_t    ret = SW_ERR;
	sw_int32_t    i = 0;
	sw_int32_t    j = 0;
	sw_int32_t    k = 0;

	if(line == NULL || strlen(line) == 0)
	{
		pub_log_debug("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	if(adm_str_is_all_space(line))
	{
		pub_log_error("[%s][%d] Param error.",__FILE__,__LINE__);
		return SW_ERR;
	}

	pub_mem_memzero(opt->mem, sizeof(opt->mem));

	adm_str_format_space(line);

	j = 0;
	k = 0;

	for (i = 0; i < SW_MAX_ARGC; i++)
	{
		opt->local_argv[i] = (sw_char_t*)opt->mem + i * SW_MAX_ARG_SIZE;
	}

	for (i = 0; line[i] != '\0'; i++)
	{
		if(line[i] != ' ')
		{
			opt->local_argv[k][j++] = line[i];
		}
		else
		{
			opt->local_argv[k][j] = '\0';
			k++;
			j = 0;
		}
	}

	opt->local_argc = k + 1;
	opt->local_argv[k][j] = '\0';

	if (strcmp("start", opt->local_argv[0]) == 0 || strcmp("s", opt->local_argv[0]) == 0)
	{
		if (check_regist() < 0)
		{
			pub_log_error("[%s][%d] Products not regist!", __FILE__, __LINE__);
			strcpy(opt->local_argv[0], "q");
		}
	}

	ret = adm_opt_get_cmd_type(opt);
	if(ret != SW_OK)
	{
		pub_log_debug("[%s][%d] Parse command failed.",__FILE__,__LINE__);

		for (i = 0; i < opt->local_argc; i++)
		{
			pub_log_debug("opt->local_argc[%d]=%s",i,opt->local_argv[i]);
		}

		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_start
 ** Desc : Parse start command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_start(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_START_ALL;
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_START_LSN_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_START_SVC_ALL;
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-pl") || 0 == strcasecmp(opt->local_argv[1], "-ps"))
			{
				return ADM_CMD_PARAM_ERR;	
			}

			return ADM_CMD_START_PROC;
		case 3:
			if (0 == strcasecmp(opt->local_argv[1], "-pl") || 0 == strcasecmp(opt->local_argv[1], "-ps"))
			{
				return ADM_CMD_PARAM_ERR;
			}
		default:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_START_LSN;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_START_SVC;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-p")
				|| 0 == strcasecmp(opt->local_argv[1], "-prdt"))
			{
				return ADM_CMD_START_PRDT;
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-pl"))
			{
				return ADM_CMD_START_LSN_IN_PRDT;	
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-ps"))
			{
				return ADM_CMD_START_SVR_IN_PRDT;	
			}

			return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_stop
 ** Desc : Parse stop command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_stop(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_STOP_ALL;
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_STOP_LSN_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_STOP_SVC_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-a")
				|| 0 == strcasecmp(opt->local_argv[1], "-all"))
			{
				return ADM_CMD_STOP_ALL;
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-pl") || 0 == strcasecmp(opt->local_argv[1], "-ps"))
			{
				return ADM_CMD_PARAM_ERR;	
			}

			return ADM_CMD_STOP_PROC;
		default:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_STOP_LSN;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_STOP_SVC;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-p")
				|| 0 == strcasecmp(opt->local_argv[1], "-prdt"))
			{
				return ADM_CMD_STOP_PRDT;
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-pl"))
			{
				return ADM_CMD_STOP_LSN_IN_PRDT;	
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-ps"))
			{
				return ADM_CMD_STOP_SVR_IN_PRDT;	
			}

			pub_log_debug("[%s][%d] Paramter is incorrect! [%s].",
				__FILE__, __LINE__, opt->local_argv[1]);

			return ADM_CMD_PARAM_ERR;
	}

	fprintf(stderr, "\n\tParamter is incorrect.\n");
	pub_log_debug("[%s][%d] Paramter is incorrect.",__FILE__,__LINE__);

	return ADM_CMD_PARAM_ERR;
}


/******************************************************************************
 ** Name : adm_opt_def_parse_top
 ** Desc : Parse top command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_top(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_TOP;
		case 2:
			if(!str_isdigit(opt->local_argv[1]))
			{
				pub_log_debug("[%s][%d] Parmater is not digit! [%s]",
					__FILE__, __LINE__, opt->local_argv[1]);
				return ADM_CMD_PARAM_ERR;
			}

			if(0 == atoi(opt->local_argv[1]))
			{
				return ADM_CMD_PARAM_ERR;
			}

			return ADM_CMD_TOP;    
		default:
			return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_step
 ** Desc : Parse step command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_step(const sw_adm_opt_t *opt)
{
	if(2 != opt->local_argc)
	{
		return ADM_CMD_PARAM_ERR;
	}

	if(!str_isdigit(opt->local_argv[1]))
	{
		pub_log_debug("[%s][%d] Parmater is incorrect. [%s]",
				__FILE__, __LINE__, opt->local_argv[1]);
		return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_STEP;
}


/******************************************************************************
 ** Name : adm_opt_def_parse_set
 ** Desc : Parse set command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 **     1. Judge the paramter whether correct?
 **     2. Judge the type of command
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.29 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_set(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 3:
			if(0 == strcasecmp(opt->local_argv[1], "-pf"))
			{
				if(!str_isdigit(opt->local_argv[2]))
				{
					pub_log_error("[%s][%d] Parmater is incorrect! [%s][%s]",
						__FILE__, __LINE__, opt->local_argv[1], opt->local_argv[2]);
					return ADM_CMD_PARAM_ERR;
				}

				return ADM_CMD_SET_PLT_FLOW;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-d")
				|| 0 == strcasecmp(opt->local_argv[1], "-date"))
			{
				if(0 == strcasecmp(opt->local_argv[2], "-unlock"))
				{
					return ADM_CMD_SET_DATE_UNLOCK;
				}
				else
				{
					if(!str_isdigit(opt->local_argv[2]))
					{
						pub_log_error("[%s][%d] Parmater is incorrect! [%s][%s]",
							__FILE__, __LINE__, opt->local_argv[1], opt->local_argv[2]);
						return ADM_CMD_PARAM_ERR;
					}

					if(strlen(opt->local_argv[2]) != strlen("YYYYMMDD"))
					{
						pub_log_error("[%s][%d] Parmater is incorrect! [%s]",
							__FILE__, __LINE__, opt->local_argv[2]);
						return ADM_CMD_PARAM_ERR;
					}

					return ADM_CMD_SET_DATE;
				}

				return ADM_CMD_PARAM_ERR;
			}

			return ADM_CMD_PARAM_ERR;
		case 4:
			if(0 == strcasecmp(opt->local_argv[1], "-bf"))      /* set -bf [FLWNO] [NAME] */
			{
				if(!str_isdigit(opt->local_argv[2]))
				{
					pub_log_error("[%s][%d] Parmater is incorrect! [%s][%s]",
						__FILE__, __LINE__, opt->local_argv[1], opt->local_argv[3]);
					return ADM_CMD_PARAM_ERR;
				}

				return ADM_CMD_SET_BSN_FLOW;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-d")   /* set -d [date] [-lock]/[-unlock] */
				|| 0 == strcasecmp(opt->local_argv[1], "-date"))
			{
				if(!str_isdigit(opt->local_argv[2]))
				{
					pub_log_error("[%s][%d] Parmater is incorrect! [%s][%s]",
						__FILE__, __LINE__, opt->local_argv[1], opt->local_argv[3]);
					return ADM_CMD_PARAM_ERR;
				}

				if(0 == strcasecmp(opt->local_argv[3], "-lock"))
				{
					return ADM_CMD_SET_DATE_LOCK;
				}

				return ADM_CMD_PARAM_ERR;
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-pf"))
			{
				if(!str_isdigit(opt->local_argv[2]))
				{
					pub_log_error("[%s][%d] Parmater is incorrect! [%s][%s]",
						__FILE__, __LINE__, opt->local_argv[1], opt->local_argv[3]);
					return ADM_CMD_PARAM_ERR;
				}

				return ADM_CMD_SET_PLT_FLOW;
			}

			return ADM_CMD_PARAM_ERR;
		default:
			return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_get
 ** Desc : Parse get command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 **     1. Judge the paramter whether correct?
 **     2. Judge the type of command
 ** Note :
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_get(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-pf"))      /* get -pf */
			{
				return ADM_CMD_GET_PLT_FLOW;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-bf")) /* get -bf */
			{
				return ADM_CMD_GET_BSN_FLOW_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-d")   /* get -d/date */
				|| 0 == strcasecmp(opt->local_argv[1], "-date"))
			{
				return ADM_CMD_GET_DATE;
			}

			return ADM_CMD_PARAM_ERR;
		case 3:
			if(0 == strcasecmp(opt->local_argv[1], "-bf"))      /* get -bf [NAME] */
			{
				return ADM_CMD_GET_BSN_FLOW;
			}
		case 5:
			if (0 == strcasecmp(opt->local_argv[1], "-log"))
			{
				return ADM_CMD_GET_LOG;
			}

			return ADM_CMD_PARAM_ERR;
		default:
			return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_saf
 ** Desc : Parse task command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_saf(const sw_adm_opt_t *opt)
{
	if( 4 == opt->local_argc)
	{
		if(!strcasecmp(opt->local_argv[1], "-l"))
		{
			return ADM_CMD_SAF_LIST;
		}
		else if(!strcasecmp(opt->local_argv[1], "-r"))
		{
			return ADM_CMD_SAF_START;
		}

		else if(!strcasecmp(opt->local_argv[1], "-e"))
		{
			return ADM_CMD_SAF_STOP;
		}
		else
		{
			pub_log_info( "[%s][%d] Unknown cmd %s.",
					__FILE__,__LINE__,opt->local_argv[1]);
			return ADM_CMD_PARAM_ERR;
		}
	}
	else
	{
		pub_log_info( "[%s][%d] cmd parse error%s.",
				__FILE__,__LINE__,opt->local_argv[1]);
		return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_task
 ** Desc : Parse task command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_task(const sw_adm_opt_t *opt)
{
	if(1 == opt->local_argc)
	{
		return ADM_CMD_LIST_TASK_ALL;
	}
	else if(2 == opt->local_argc)
	{
		if(!strcasecmp(opt->local_argv[1], "-la"))
		{
			return ADM_CMD_LIST_TASK_AU;
		}
		else if(!strcasecmp(opt->local_argv[1], "-lm"))
		{
			return ADM_CMD_LIST_TASK_M;
		}
		else if(!strcasecmp(opt->local_argv[1], "-l"))
		{
			return ADM_CMD_LIST_TASK_ALL;
		}
		else if(!strcasecmp(opt->local_argv[1], "-s"))
		{
			return ADM_CMD_OPT_TASK_ALL;
		}
		else
		{
			pub_log_debug("[%s][%d] Unknown cmd %s.",
					__FILE__,__LINE__,opt->local_argv[1]);
			return ADM_CMD_PARAM_ERR;
		}
	}
	else if (2 < opt->local_argc)
	{
		if(!strcasecmp(opt->local_argv[1], "-l"))
		{
			return ADM_CMD_LIST_TASK_S;
		}
		else if(!strcasecmp(opt->local_argv[1], "-s"))
		{
			return ADM_CMD_OPT_TASK_S;
		}
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_list
 ** Desc : Parse list command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_list(const sw_adm_opt_t *opt)
{
	sw_int_t idx = 0;

	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_LIST_ALL;
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_LIST_LSN_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_LIST_SVC_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-p")
				|| 0 == strcasecmp(opt->local_argv[1], "-product"))
			{
				return ADM_CMD_LIST_PRDT_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-v")
				|| 0 == strcasecmp(opt->local_argv[1], "-version"))
			{
				return ADM_CMD_LIST_VERSION;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-his")
				|| 0 == strcasecmp(opt->local_argv[1], "-history"))
			{
				return ADM_CMD_LIST_HIS_PATCH;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-am")
				|| 0 == strcasecmp(opt->local_argv[1], "-ma"))
			{
				return ADM_CMD_LIST_MTYPE_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-m")
				|| 0 == strcasecmp(opt->local_argv[1], "-bm")
				|| 0 == strcasecmp(opt->local_argv[1], "-mb")
				|| 0 == strcasecmp(opt->local_argv[1], "-mtype"))
			{
				return ADM_CMD_LIST_MTYPE_BUSY;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-fm")
				|| 0 == strcasecmp(opt->local_argv[1], "-mf"))
			{
				return ADM_CMD_LIST_MTYPE_FREE;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-r")
				|| 0 == strcasecmp(opt->local_argv[1], "-res"))
			{
				return ADM_CMD_LIST_RES_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-rm"))
			{
				return ADM_CMD_LIST_RES_SHM;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-rq"))
			{
				return ADM_CMD_LIST_RES_MSQ;
			}
			else if (0 == strcasecmp(opt->local_argv[1], "-pp"))
			{
				return ADM_CMD_LIST_ALL_IN_PRDT;
			}
			return ADM_CMD_PARAM_ERR;

		case 3:
			if(0 == strcasecmp(opt->local_argv[1], "-m")
				|| 0 == strcasecmp(opt->local_argv[1], "-mtype"))
			{
				if(!str_isdigit(opt->local_argv[2]))
				{
					return ADM_CMD_PARAM_ERR;
				}

				return ADM_CMD_LIST_MTYPE;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-r"))
			{
				if(0 == strcasecmp(opt->local_argv[2], "shm"))
				{
					return ADM_CMD_LIST_RES_SHM;
				}
				else if(0 == strcasecmp(opt->local_argv[2], "msq"))
				{
					return ADM_CMD_LIST_RES_MSQ;
				}

				return ADM_CMD_PARAM_ERR;
			}

			return ADM_CMD_PARAM_ERR;

		case 4:
			if(0 == strcasecmp(opt->local_argv[1], "-m")
				|| 0 == strcasecmp(opt->local_argv[1], "-mtype"))
			{
				for(idx=2; idx<opt->local_argc; idx++)
				{
					if(!str_isdigit(opt->local_argv[2]))
					{
						return ADM_CMD_PARAM_ERR;
					}
				}

				return ADM_CMD_LIST_SEG_MTYPE;
			}

			return ADM_CMD_PARAM_ERR;
		default:
			return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_clean
 ** Desc : Parse clean command.
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.08.28 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_def_parse_clean(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 2:
			if(0 == strcmp(opt->local_argv[1], "--yes")
				|| 0 == strcmp(opt->local_argv[1], "--y"))
			{
				return ADM_CMD_CLEAN;
			}

			return ADM_CMD_PARAM_ERR;
		default:
			return ADM_CMD_PARAM_ERR;
	}
	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_prdt_parse_start
 ** Desc : Parse start command of product.
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_prdt_parse_start(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_PRDT_START_ALL;
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_PRDT_START_ALL_LSN;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_PRDT_START_ALL_SVC;
			}

			return ADM_CMD_START_PROC;
		default:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_PRDT_START_LSN;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_PRDT_START_SVC;
			}

			return ADM_CMD_START_PROC;
	}

	return ADM_CMD_PARAM_ERR;
}

/******************************************************************************
 ** Name : adm_opt_def_parse_stop
 ** Desc : Parse stop command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_prdt_parse_stop(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_PRDT_STOP_ALL;
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-l"))
			{
				return ADM_CMD_PRDT_STOP_LSN_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s"))
			{
				return ADM_CMD_PRDT_STOP_SVC_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-a"))
			{
				return ADM_CMD_PRDT_STOP_ALL;
			}

			return ADM_CMD_STOP_PROC;
		default:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_PRDT_STOP_LSN;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_PRDT_STOP_SVC;
			}

			pub_log_debug("[%s][%d] Unknown cmd %s.", __FILE__, __LINE__, opt->local_argv[1]);

			return ADM_CMD_PARAM_ERR;
	}

	fprintf(stderr, "\n\tError param.\n");
	pub_log_debug("[%s][%d] Error param.",__FILE__,__LINE__);

	return ADM_CMD_PARAM_ERR;
}


/******************************************************************************
 ** Name : adm_opt_prdt_parse_list
 ** Desc : Parse list command
 ** Input: 
 **     opt: Operator command information
 ** Output: NONE
 ** Return: Command type
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.27 #
 ******************************************************************************/
static adm_cmd_type_e adm_opt_prdt_parse_list(const sw_adm_opt_t *opt)
{
	switch(opt->local_argc)
	{
		case 1:
			return ADM_CMD_PRDT_LIST_ALL;
		case 2:
			if(0 == strcasecmp(opt->local_argv[1], "-l")
				|| 0 == strcasecmp(opt->local_argv[1], "-lsn"))
			{
				return ADM_CMD_PRDT_LIST_LSN_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-s")
				|| 0 == strcasecmp(opt->local_argv[1], "-svc"))
			{
				return ADM_CMD_PRDT_LIST_SVC_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-p")
				|| 0 == strcasecmp(opt->local_argv[1], "-product"))
			{
				return ADM_CMD_LIST_PRDT_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-m")
				|| 0 == strcasecmp(opt->local_argv[1], "-mtype"))
			{
				return ADM_CMD_PRDT_LIST_MTYPE_ALL;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-v")
				|| 0 == strcasecmp(opt->local_argv[1], "-version"))
			{
				return ADM_CMD_LIST_VERSION;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-his")
				|| 0 == strcasecmp(opt->local_argv[1], "-history"))
			{
				return ADM_CMD_LIST_HIS_PATCH;
			}
			else if(0 == strcasecmp(opt->local_argv[1], "-c")
				|| (0 == strcasecmp(opt->local_argv[1], "-child")))
			{
				return ADM_CMD_PRDT_LIST_CHILD_ALL;
			}

			return ADM_CMD_PARAM_ERR;
		default:
			return ADM_CMD_PARAM_ERR;
	}

	return ADM_CMD_PARAM_ERR;
}

/*int main(int argc, char* argv[])
  {
  sw_adm_opt_t    opt;
  sw_int_t    ret = SW_ERR;
  sw_int32_t    i = 0;
  sw_char_t    test1[64] = {" start "};
  sw_char_t    test2[64] = {" start  -n "};
  sw_char_t    test3[64] = {" start -n name1  name2"};
  sw_char_t    test4[64] = {"exit"};
  sw_char_t    test5[64] = {"quit"};
  sw_char_t    test6[64] = {"stop"};
  sw_char_t    test7[64] = {"stop name "};
  sw_char_t    test8[64] = {" stop -n "};
  sw_char_t    test9[64] = {" stop            -a "};
  sw_char_t    test10[64] = {" stop            -af "};
  sw_char_t    test11[64] = {" stop            -nf name "};
  sw_char_t    test12[64] = {" stop    -nf name "};
  sw_char_t    test13[64] = {" stop            -n name "};
  sw_char_t    test14[64] = {" stop            -s name "};
  sw_char_t    test15[64] = {" stop            -sf name "};
  sw_char_t    test16[64] = {" stop            -f name "};
  sw_char_t    test17[64] = {" list"};
  sw_char_t    test18[64] = {" list -n"};
  sw_char_t    test19[64] = {" list -s"};
  sw_char_t    test20[64] = {" list -m"};
  sw_char_t    test21[64] = {" list node_type"};
  sw_char_t    test22[64] = {" trace"};
  sw_char_t    test23[64] = {" clean"};
  sw_char_t    test24[64] = {" task"};
  sw_char_t    test25[64] = {" task  -n "};
  sw_char_t    test26[64] = {" task  -n name1  name2  name3 "};


#define TEST_OPT_PARSE(A)    pub_mem_memzero(&opt, sizeof(opt));\
ret  = adm_opt_parse(&opt, A);\
if(ret != SW_OK)\
{\
pub_log_error("[%s][%d] adm_opt_parse fail.",__FILE__,__LINE__);\
return SW_ERR;\
}\
else \
{\
pub_log_stderr("cmd_type[%d] count: %d\n",opt.cmd_type, opt.local_argc);\
for (i = 0;i < opt.local_argc; i++)\
{\
pub_log_stderr("%s\n",opt.local_argv[i]);\
}\
}

TEST_OPT_PARSE(test1)
TEST_OPT_PARSE(test2)
TEST_OPT_PARSE(test3)
TEST_OPT_PARSE(test4)
TEST_OPT_PARSE(test5)
TEST_OPT_PARSE(test6)
TEST_OPT_PARSE(test7)
TEST_OPT_PARSE(test8)
TEST_OPT_PARSE(test9)
TEST_OPT_PARSE(test10)
TEST_OPT_PARSE(test11)
TEST_OPT_PARSE(test12)
TEST_OPT_PARSE(test13)
TEST_OPT_PARSE(test14)
TEST_OPT_PARSE(test15)
TEST_OPT_PARSE(test16)
TEST_OPT_PARSE(test17)
TEST_OPT_PARSE(test18)
TEST_OPT_PARSE(test19)
TEST_OPT_PARSE(test20)
TEST_OPT_PARSE(test21)
TEST_OPT_PARSE(test22)
	TEST_OPT_PARSE(test23)
	TEST_OPT_PARSE(test24)
	TEST_OPT_PARSE(test25)
TEST_OPT_PARSE(test26)

	return SW_OK;
	}
*/
