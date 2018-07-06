#include "pub_config.h"
#include "svc_cycle.h"
#include "pub_log.h"

int main(int argc, char **argv)
{
	int	ret = 0;
	sw_char_t	name[64];
	sw_char_t	log_name[64];
	sw_svc_cycle_t	cycle;

	if (argc != 2)
	{
		pub_log_stderr("swsvcman param error! Usege: %s prdt", argv[0]);
		return SW_ERROR;
	}

	signal(SIGCHLD, SIG_DFL);
	memset(name, 0x0, sizeof(name));
	memset(log_name, 0x0, sizeof(log_name));
	sprintf(name, "swsvcman_%s", argv[1]);
	sprintf(log_name, "%s.log", name);
	pub_mem_memzero(&cycle, sizeof(cycle));
	ret = svc_cycle_init(&cycle, name, ND_SVC, "error.log", log_name, argv[1]);
	if (ret == SW_EXIST)
	{
		svc_cycle_destroy(&cycle);
		pub_log_exit("[%s][%d] SVC [%s] already exist!", __FILE__, __LINE__, name);
		return SW_OK;
	}
	else if (ret != SW_OK)
	{
		svc_father_register(&cycle, SW_S_ABNORMAL);
		pub_log_exit("[%s][%d] svc_cycle_init error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	ret = svc_cycle_run(&cycle);
	if (ret != SW_OK)
	{
		if (ret == SW_DECLINED)
		{
			svc_father_register(&cycle, SW_S_ABORTED);
		}
		else
		{
			svc_father_register(&cycle, SW_S_ABNORMAL);
		}
		pub_log_exit("[%s][%d] svc_cycle_run error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	wait(NULL);
	svc_cycle_destroy(&cycle);
	pub_log_exit("[%s][%d] swsvcman stoped!", __FILE__, __LINE__);

	return SW_OK;
}

