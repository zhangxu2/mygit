#include <stdio.h>
#include "pub_vars.h"
#include "dfis_var.h"

int check_in(char* buf, int len)
{
	int result = 0;

	result = memcmp(buf + 4, "xm2", 3);
	if (result == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int test_analyze(sw_buf_t* pkg_buf, sw_loc_vars_t* vars, sw_xmltree_t* config)
{
	if (vars == NULL)
	{
		set_zd_data(".root.passwd", "1111111111111111111");
		
		set_zd_data("#pkgtype", "xml");
	}
	else
	{
		loc_set_zd_data(vars, ".root.passwd", "1111111111111111111");
		loc_set_zd_data(vars, "#pkgtype", "xml");
	}

	return 0;
}

int check_out(sw_loc_vars_t *vars)
{
	sw_char_t	value[256];

	pub_mem_memzero(value, sizeof(value));
	
	if (vars == NULL)
	{
		get_zd_data("#pkgtype", value);
	}
	else
	{
		loc_get_zd_data(vars, "#pkgtype", value);
	}

	pub_log_info("%s, %d, #pkgtype[%s]",__FILE__,__LINE__,value);

	if (strcmp(value, "xm2") == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int test_integrate(sw_buf_t* pkg_buf, sw_loc_vars_t* vars, sw_xmltree_t* config)
{
	sw_char_t*	base = NULL;
	sw_int32_t	index = 0;
	sw_char_t	tmp[256];
	
	base = pkg_buf->psBuf;
	index += 4;
	
	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "xm2<root><processor>test_processor</processor></root>");
	memcpy(base + index, tmp, strlen(tmp));
	index += strlen(tmp);

	pub_mem_memzero(tmp, sizeof(tmp));
	sprintf(tmp, "%0*d", 4, index);
	memcpy(base, tmp, 4);

	return index;
}

