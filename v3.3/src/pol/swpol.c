/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-05-08
 *** module  : swpol 
 *** name    : swpol.c 
 *** function: swpol main function
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/
#include "pol_cycle.h"

int main(int argc,char *argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_pol_cycle_t	cycle;

	pub_mem_memzero(&cycle, sizeof(sw_pol_cycle_t));

	/*init swpol cycle object*/
	result = pol_cycle_init(&cycle, "swpol", ND_POL, "error.log", "swpol.log");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pol_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d]swpol starting...", __FILE__, __LINE__);

	/*run swpol event process loop*/
	result = pol_cycle_run(&cycle);
	if(result != SW_OK)
	{
		pub_log_error("%s, %d, Run swpol event process loop fail.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*destory swpol cycle object*/
	pol_cycle_destroy(&cycle);
	
	pub_log_info("[%s][%d]swpol exit.", __FILE__, __LINE__);
	
	return SW_OK;
}

