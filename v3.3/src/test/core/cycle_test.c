#include "cycle.h"
#include "pub_log.h"
#include "pub_vars.h"
#include "mtype.h"

sw_int_t handle_timeout(sw_cycle_t* cycle)
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	status[256];

	pub_mem_memzero(status, sizeof(status));
	get_zd_data("#status", status);

	pub_log_info("%s, %d, BP [%s]: %s"
			, __FILE__, __LINE__,cycle->syscfg->name, status);

	return SW_OK;
}

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_cycle_t	cycle;
	sw_fd_list_t	fd_list;
	sw_loc_vars_t	vars;
	sw_int_t	mtype;

	pub_mem_memzero(&cycle, sizeof(cycle));

	result = cycle_init(&cycle, "swadmin", ND_ADM, "error.log", "swadmin.log", NULL);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	cycle.timeout_handler = handle_timeout;

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = 0;
	result = select_add_event(cycle.lsn_fds, &fd_list);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, select_add_event error.",__FILE__,__LINE__);
		return SW_ERROR;
	}	

	result = cycle_link_shm_run(&cycle);
	if (result == SHM_MISS)
	{
		pub_log_info("%s, %d, cycle_link_shm_run error, result[%d]."
				,__FILE__,__LINE__,result);
		result = cycle_create_shm_run(&cycle);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, cycle_create_shm_run error.",__FILE__,__LINE__);
			return SW_ERROR;
		}

	}
	else if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, cycle_link_shm_run error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = cycle_init_shm_vars(&cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_init_shm_vars error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(&vars, sizeof(vars));
	result = pub_vars_alloc(SHM_VARS);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_alloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	mtype = mtype_new();
	if (mtype == -1)
	{
		pub_log_error("%s, %d, mtype_new error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	result = pub_vars_create(mtype);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_create mtype[%d] error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	set_zd_data("#status", "running");
	
	/*result = cycle_create_shm(&cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_create_shm error.",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	/*result = cycle_register(&cycle, NEVER_RESTART, 0);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_register error.",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	
	result = cycle_run(&cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_run error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = cycle_destory(&cycle);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, cycle_destory error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

