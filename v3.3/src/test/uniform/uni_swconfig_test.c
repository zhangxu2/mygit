#include "uni_swconfig.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_config_t	swconfig;

	pub_mem_memzero(&swconfig, sizeof(swconfig));

	/*获取平台全局配置信息*/
	result = uni_swconfig_ctl(&swconfig,SW_GET);
	if(result != SW_OK)
	{
		writelog(LOG_ERR,"%s,%d,uni_get_swconfig err.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero((swconfig.log_level1), sizeof(swconfig.log_level1));
	strncpy((swconfig.log_level1),"8",sizeof(swconfig.log_level1));

	/*设置平台全局配置*/
	result = uni_swconfig_ctl(&swconfig,SW_SET);
	if(result != SW_OK)
	{
		writelog(LOG_ERR,"%s,%d,uni_get_swconfig err.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	/*设置平台全局配置*/
	result = uni_swconfig_ctl(&swconfig,10);
	if(result != SW_ERROR)
	{
		writelog(LOG_ERR,"%s,%d,uni_get_swconfig err.",__FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

