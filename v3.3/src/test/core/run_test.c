#include "run.h"
#include "pub_mem.h"
#include "pub_log.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_int32_t	size = 0;
	sw_syscfg_t	syscfg;

	pub_mem_memzero(&syscfg, sizeof(syscfg));

	/*set shm_sem_lock*/
	syscfg.semsize = 128;
	
	/*set shm_run key*/

	/*set mtype configure param*/
	syscfg.session_max = 1000;

	/*set trace_no configure param*/

	/*set procs configure param*/
	syscfg.processe_max = 100;

	/*set route configure param*/

	/*set job configure param*/
	syscfg.job_max = 64;

	/*test shm_run init*/
	result = run_init();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	size = run_get_size();
	if (size == -1)
	{
		pub_log_error("%s, %d, run_get_size error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, shm_run size(%d)", __FILE__, __LINE__, size);

	return SW_OK;
}

