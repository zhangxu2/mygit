/*********************************************************************
 *** version : v3.0
 *** author  : 
 *** create  : 
 *** module  : swlsn 
 *** name    : lsn_main.c
 *** function: main function
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/
#include "lsn_pub.h"
#include <sys/wait.h>
#include <libgen.h>

extern sw_int_t lsn_cycle_init(sw_lsn_cycle_t *, sw_char_t *, sw_char_t *, sw_int32_t, sw_char_t *, sw_char_t *);
extern sw_int_t lsn_cycle_run(sw_lsn_cycle_t *cycle);
extern sw_int32_t lsn_father_register(sw_lsn_cycle_t *cycle, sw_int32_t status);

int main(int argc, char *argv[])
{
	sw_int32_t	ret = 0;
	sw_lsn_cycle_t	cycle;
	sw_char_t	name[32];
	sw_char_t	log_name[64];
	
	if (argc != 2)
	{
		pub_log_stderr("[%s][%d] Param error! Usage: %s lsnname", __FILE__, __LINE__, argv[0]);
		return SW_ERROR;
	}
	signal(SIGCHLD, SIG_DFL);
	memset(name, 0x0, sizeof(name));
	memset(log_name, 0x0, sizeof(log_name));
	sprintf(name, "swlsn_%s", argv[1]);
	sprintf(log_name, "%s.log", name);
	pub_mem_memzero(&cycle, sizeof(sw_lsn_cycle_t));
	ret = lsn_cycle_init(&cycle, name, argv[1], ND_LSN, "error.log", log_name);
	if (ret == SW_DONE)
	{
		pub_log_exit("[%s][%d] LSN [%s] not used!", __FILE__, __LINE__, argv[1]);
		return SW_OK;
	}
	else if (ret == SW_EXIST)
	{
		pub_log_exit("[%s][%d] LSN [%s] already exist!", __FILE__, __LINE__, argv[1]);
		return SW_OK;
	}
	else if (ret != SW_OK)
	{
		lsn_father_register(&cycle, SW_S_ABNORMAL);
		pub_log_exit("[%s][%d]lsn cycle init error! ret=[%d]",__FILE__,__LINE__, ret);
		return SW_ERROR;
	}
	
	ret = lsn_cycle_run(&cycle);
	if (ret != SW_OK)
	{
		if (ret == SW_DECLINED)
		{
			lsn_father_register(&cycle, SW_S_ABORTED);
		}
		else
		{
			lsn_father_register(&cycle, SW_S_ABNORMAL);
		}
		lsn_cycle_destroy(&cycle);
		return SW_ERROR;
	}
	wait(NULL);
	lsn_cycle_destroy(&cycle);
	
	return SW_OK;
}
