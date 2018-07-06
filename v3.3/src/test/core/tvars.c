#include "pub_vars.h"
#include "pub_log.h"
#include "pub_buf.h"
#include "cycle.h"

int main()
{
	int	ret = 0;
	int	mtype = 1000;
	sw_loc_vars_t	vars;
	sw_cycle_t	cycle;
	
	memset(&cycle, 0x0, sizeof(cycle));
	
	ret = cycle_link_shm(&cycle);
	if (ret)
	{
		printf("Link shm error!\n");
		return -1;
	}
	printf("Link shm success!\n");
	memset(&vars, 0x0, sizeof(vars));
	ret = pub_loc_vars_alloc(&vars, SHM_VARS);
	if (ret)
	{
		printf("vars alloc error!\n");
		return -1;
	}

	ret = vars.create(&vars, 1000);
	if (ret)
	{
		printf("Create vars error!\n");
		return -1;
	}
	loc_set_zd_data(&vars, ".head.name", "maweiwei");
	
	ret = vars.create(&vars, 200);
	if (ret)
	{
		printf("[%s][%d] Create vars error!\n", __FILE__, __LINE__);
		return -1;
	}
	mtype_delete(1000, 0);
	mtype_delete(200, 0);
	vars.free_mem(&vars);
	
	return 0;
}

