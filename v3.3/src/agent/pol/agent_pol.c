#include "agent_pub.h"

int agtpol_handle_timeout()
{
	int	i = 0;
	int	ret = 0;
	sw_agt_procs_t	procs;

	pub_mem_memzero(&procs, sizeof(procs));
	ret = agt_get_procs(&procs);
	if (ret != 0)
	{
		pub_log_error("%s, %d, agt_get_procs error.",__FILE__,__LINE__);
		return -1;
	}

	for (i = 0; i < procs.count; i++)
	{
		if (procs.proc[i].pid > 0 && procs.proc[i].stat != SW_S_STOPED && procs.proc[i].type == SW_PARENT)
		{
			ret = agt_check_pid(procs.proc[i].pid);
			if (ret == -1)
			{
				if (procs.proc[i].restartcnt >= 0)
				{
					pub_log_info("[%s][%d] process [%s][%d] is abnormal,restart it...",
							__FILE__, __LINE__, procs.proc[i].name, procs.proc[i].pid);

					if (procs.proc[i].restartcnt == 0)
					{
						procs.proc[i].stat = SW_S_ABNORMAL;
						procs.proc[i].restartcnt--;
						ret = agt_set_proc_by_name(procs.proc[i].name, &procs.proc[i]);
						if (ret < 0)
						{
							pub_log_error("[%s][%d] register proc[%s] error.", __FILE__, __LINE__, procs.proc[i].name);
							return -1;
						}
						pub_log_info("[%s][%d] more restart cnt,not restart.", __FILE__, __LINE__);
						continue;
					}

					procs.proc[i].stat = SW_S_ABNORMAL;
					procs.proc[i].restartcnt--;
					ret = agt_set_proc_by_name(procs.proc[i].name, &procs.proc[i]);
					if (ret < 0)
					{
						pub_log_error("[%s][%d] register proc[%s] error.", __FILE__, __LINE__, procs.proc[i].name);
						return -1;
					}

					ret = agt_start_node(procs.proc[i].binary, procs.proc[i].name);
					if (ret < 0)
					{
						pub_log_error("[%s][%d] restart proc[%s] error.", __FILE__, __LINE__);
						return -1;
					}

					pub_log_info("[%s][%d] Start binary[%s][%s] success."
							, __FILE__, __LINE__, procs.proc[i].binary, procs.proc[i].name);
				}
				else
				{
					pub_log_error("[%s][%d] proc[%s] restart reach max[%d]",
							__FILE__, __LINE__,  procs.proc[i].name,  MAX_RESTART_CNT);
				}
			}
		}
	}

	return 0;
}

int agtpol_cycle_init(char* name, char* err_log, char* dbg_log)
{
	int	ret = 0;
	sw_agt_proc_t   agt_proc;

	ret = agt_set_logpath(err_log, dbg_log);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] set log path error.", __FILE__, __LINE__);
		return -1;
	}

	ret = agt_set_log_cfg();
        if (ret < 0)
        {
                pub_log_error("[%s][%d] set golbal log cfg error.", __FILE__, __LINE__);
                return -1;
        }

	pub_mem_memzero(&agt_proc, sizeof(agt_proc));
	strcpy(agt_proc.binary, name);
	strcpy(agt_proc.name, name);
	agt_proc.proc_idx = -1;
	agt_proc.pid = getpid();
	agt_proc.stat = SW_S_START;
	agt_proc.type = SW_PARENT;
	ret = agt_proc_register(&agt_proc);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] agt_proc_register fail.", __FILE__, __LINE__);
		return -1;
	}

	return 0;	
}

int agtpol_cycle_run()
{
	int	ret = 0;
	int	sec = 0;

	sec = agt_get_scantime() > 0 ? agt_get_scantime() : 10;
	while(1)
	{
		ret = pub_time_timer(sec, 0);
		if (ret < 0)
		{
			continue;
		}
		else if (ret == 0)
		{
			ret = agtpol_handle_timeout();
			if (ret != 0)
			{
				pub_log_error("%s, %d, pol_handle_timeout error.", __FILE__, __LINE__);
			}
		}
	}

	return 0;
}

int agtpol_cycle_destroy()
{
	return 0;
}

int main(int argc,char *argv[])
{
	int	ret = 0;

	ret = agtpol_cycle_init(PROC_NAME_AGTPOL, "agt_error.log", "agentpol.log");
	if (ret != 0)
	{
		pub_log_error("%s, %d, agtpol_cycle_init error.", __FILE__, __LINE__);
		return -1;
	}

	pub_log_info("[%s][%d]agtpol starting...", __FILE__, __LINE__);
	ret = agtpol_cycle_run();
	if(ret != 0)
	{
		pub_log_error("%s, %d, Run agtpol event process loop fail.", __FILE__, __LINE__);
		return -1;
	}

	agtpol_cycle_destroy();
	pub_log_info("[%s][%d] agtpol exit.", __FILE__, __LINE__);
	return 0;
}

