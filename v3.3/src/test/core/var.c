#include "pub_vars.h"
#include "pub_log.h"
#include "pub_buf.h"
#include "swapi.h"

int main()
{
	int	i = 0;
	int	ret = 0;
	char	name[128];
	char	value[128];
	sw_buf_t	vbuf;
	sw_loc_vars_t	vars;
	
	ret = pub_loc_vars_alloc(&vars, HEAP_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] vars alloc error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = pub_loc_vars_create(&vars, 1);
	if (ret != SW_OK)
	{	
		pub_log_error("[%s][%d] Vars create error!", __FILE__, __LINE__);
		return -1;
	}
	
	for (i = 0; i < 100; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, "#sys%d", i + 1);
		sprintf(value, "v%d", i + 1);
		loc_set_zd_data(&vars, name, value);
	}
	
	for (i = 0; i < 5; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.node%d.name%d", i + 1, i + 1);
		sprintf(value, "V%d", i + 1000);
		loc_set_zd_data(&vars, name, value);
	}
	
	for (i = 0; i < 8; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.person(%d).name", i + 5);
		sprintf(value, "N%d", i + 200);
		set_zdxml_data(&vars, name, value, '0', 1);
	}

	for (i = 0; i < 10; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.addr(%d)", i + 5);
		sprintf(value, "A%d", i + 500);
		set_zdxml_data(&vars, name, value, '0', 1);
	}

	for (i = 0; i < 8 + 5; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.person(%d).name", i);
		loc_get_zd_data(&vars, name, value);
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, name, value);
	}
	
	for (i = 0; i < 10 + 5; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.addr(%d)", i);
		loc_get_zd_data(&vars, name, value);
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, name, value);
	}

	pub_buf_init(&vbuf);
	vars.serialize(&vars, &vbuf);
	pub_log_bin(SW_LOG_INFO, vbuf.data, vbuf.len, "[%s][%d] serilize:[%d]", __FILE__, __LINE__, vbuf.len);
	
	vars.destroy(&vars);
	vars.free_mem(&vars);
/***
	ret = pub_loc_vars_free(&vars);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Vars free error!", __FILE__, __LINE__);
		return -1;
	}
***/
	
	ret = pub_loc_vars_alloc(&vars, HEAP_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] vars alloc error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = pub_loc_vars_create(&vars, 1);
	if (ret != SW_OK)
	{	
		pub_log_error("[%s][%d] Vars create error!", __FILE__, __LINE__);
		return -1;
	}
	
	vars.unserialize(&vars, vbuf.data);
	pub_buf_clear(&vbuf);
	
	for (i = 0; i < 100; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, "#sys%d", i + 1);
		loc_get_zd_data(&vars, name, value);
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, name, value);
	}
	
	for (i = 0; i < 5; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.node%d.name%d", i + 1, i + 1);
		loc_get_zd_data(&vars, name, value);
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, name, value);
	}

	for (i = 0; i < 8 + 5; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.person(%d).name", i);
		loc_get_zd_data(&vars, name, value);
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, name, value);
	}

	for (i = 0; i < 10 + 5; i++)
	{
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		sprintf(name, ".head.addr(%d)", i);
		loc_get_zd_data(&vars, name, value);
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, name, value);
	}
	
	vars.destroy(&vars);
	vars.free_mem(&vars);
	/***
	ret = pub_loc_vars_free(&vars);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Vars free error!", __FILE__, __LINE__);
		return -1;
	}
	***/

	return 0;
}
