#include "log_cycle.h"

int main(int argc, char *argv[])
{
	sw_int32_t	ret = 0;
	sw_char_t	name[32];
	sw_char_t	log_name[64];
	sw_log_cycle_t	cycle;

	signal(SIGCHLD, SIG_DFL);
	memset(name, 0x0, sizeof(name));
	memset(log_name, 0x0, sizeof(log_name));

	sprintf(name, "%s", argv[0]);
	sprintf(log_name, "%s.log", name);
	pub_mem_memzero(&cycle, sizeof(sw_log_cycle_t));
	ret = log_cycle_init(&cycle, name, ND_LSN, "error.log", log_name);
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
		log_father_register(&cycle, SW_S_ABNORMAL);
		pub_log_exit("[%s][%d]lsn cycle init error! ret=[%d]",__FILE__,__LINE__, ret);
		return SW_ERROR;
	}

	ret = log_cycle_run(&cycle);
	if (ret != SW_OK)
	{
		log_father_register(&cycle, SW_S_ABNORMAL);
		log_cycle_destroy(&cycle);
		return SW_ERROR;
	}

	wait(NULL);
	log_cycle_destroy(&cycle);

	return SW_OK;
}

