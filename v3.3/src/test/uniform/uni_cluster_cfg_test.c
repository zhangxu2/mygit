#include "uni_cluster_cfg.h"

int main(int argc, char* argv[])
{
	sw_int_t		result = SW_ERROR;
	sw_cluster_cfg_t	cluster_cfg;

	pub_mem_memzero(&cluster_cfg, sizeof(cluster_cfg));

	result = uni_cluster_cfg_ctl(&cluster_cfg, SW_GET);

	if(result != SW_OK)
	{
		writelog(LOG_ERR, "%s,%d uni_cluster_cfg_ctl error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(cluster_cfg.host_name, sizeof(cluster_cfg.host_name));
	strcpy(cluster_cfg.host_name,"host2");

	
	result = uni_cluster_cfg_ctl(&cluster_cfg, SW_SET);

	if(result != SW_OK)
	{
		writelog(LOG_ERR, "%s,%d uni_cluster_cfg_ctl error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	
	return SW_OK;
}

