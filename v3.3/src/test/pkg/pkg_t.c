#include "pub_xml.h"
#include "pkg_processor.h"
#include "pkg_type_check.h"
#include "pkg.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	pkg[2048];
	sw_int_t	i = 0;
	sw_int_t	loop_cnt = 0;
	sw_char_t	tmp[128];
	sw_char_t	name[64];
	sw_xmltree_t	*xml_config = NULL;
	sw_vars_t	vars;
	sw_buf_t	xml_pkg;
	sw_buf_t	sep_pkg;
	sw_buf_t	out_pkg;

	vLocVarsInit(&vars);
	pub_buf_init(&out_pkg);
	
	result = pub_loc_vars_alloc(&vars, HEAP_VARS);
        if (result != SW_OK)
        {
                pub_log_error("%s, %d, pub_loc_vars_alloc error."
                                                        ,__FILE__, __LINE__);
                return SW_ERROR;
        }
	
	xml_config = pub_xml_crtree("t.xml");
	if (xml_config == NULL)
	{
		printf("Create xml error!\n");
		return -1;
	}
	
	strcpy(pkg, "aaaaaaaabbbbccccccddddd1234xml000211|||2222|||333333###44|||5555|||666666###aa|||bbbb|||cccccc###dd|||eeee|||ffffff###");
	printf("[%s][%d] pkg=[%s]\n", __FILE__, __LINE__, pkg);
	pkg_com_in(&vars, pkg, (sw_char_t *)xml_config, strlen(pkg), 1);
	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#tx_code", tmp);
	printf("[%s][%d] [#tx_code]=[%s]\n", __FILE__, __LINE__, tmp);

	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#aa", tmp);
	printf("[%s][%d] [#aa]=[%s]\n", __FILE__, __LINE__, tmp);

	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#bb", tmp);
	printf("[%s][%d] [#bb]=[%s]\n", __FILE__, __LINE__, tmp);

	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#cc", tmp);
	printf("[%s][%d] [#cc]=[%s]\n", __FILE__, __LINE__, tmp);

	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#dd", tmp);
	printf("[%s][%d] [#dd]=[%s]\n", __FILE__, __LINE__, tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#AA", tmp);
	printf("[%s][%d] [#AA]=[%s]\n", __FILE__, __LINE__, tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(&vars, "#loopcnt", tmp);
	printf("[%s][%d] loopcnt===[%s]\n", __FILE__, __LINE__, tmp);
	loop_cnt = atoi(tmp);
	for (i = 0; i < loop_cnt; i++)
	{
		memset(tmp, 0x0, sizeof(tmp));
		memset(name, 0x0, sizeof(name));
		sprintf(name, "#a(%d)", i);
		loc_get_zd_data(&vars, name, tmp);
		printf("[%s][%d] [%s]====[%s]\n", __FILE__, __LINE__, name, tmp);

		memset(tmp, 0x0, sizeof(tmp));
		memset(name, 0x0, sizeof(name));
		sprintf(name, "#b(%d)", i);
		loc_get_zd_data(&vars, name, tmp);
		printf("[%s][%d] [%s]====[%s]\n", __FILE__, __LINE__, name, tmp);

		memset(tmp, 0x0, sizeof(tmp));
		memset(name, 0x0, sizeof(name));
		sprintf(name, "#c(%d)", i);
		loc_get_zd_data(&vars, name, tmp);
		printf("[%s][%d] [%s]====[%s]\n", __FILE__, __LINE__, name, tmp);

	}
	
	memset(pkg, 0x0, sizeof(pkg));
	pkg_com_out(&vars, &out_pkg, (sw_char_t *)xml_config, 1);
	
	pub_xml_deltree(xml_config);
	result = pub_loc_vars_free(&vars);
        if (result != SW_OK)
        {
                pub_log_error("pub_loc_vars_alloc error.");
                return SW_ERROR;
        }

	return SW_OK;
}

