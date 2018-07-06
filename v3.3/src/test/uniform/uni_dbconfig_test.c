#include "uni_dbconfig.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_dbconfig_t	dbconfig;

	pub_mem_memzero(&dbconfig, sizeof(dbconfig));
	result = uni_dbconfig_ctl(&dbconfig, SW_GET);

	if(result != SW_OK)
	{
		writelog(LOG_ERR,"%s,%d,uni_get_dbconfig fail!",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(dbconfig.db_con_array[1].name, sizeof(dbconfig.db_con_array[1].name));
	strcpy(dbconfig.db_con_array[1].name,"myora10");
	result = uni_dbconfig_ctl(&dbconfig, SW_SET);

	if(result != SW_OK)
	{
		writelog(LOG_ERR,"%s,%d,uni_set_dbconfig fail!",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

