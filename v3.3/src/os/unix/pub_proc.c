#include "pub_proc.h"
#include <sys/types.h>
#include <signal.h>

sw_int32_t pub_proc_checkpid(sw_int_t pid)
{
	if (pid <= 0)
	{
		return SW_ERROR;
	}
	
	if (kill(pid, 0) == 0)
	{
		return SW_OK;
	}
	
	return SW_ERROR;
}

