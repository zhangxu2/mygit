#include "pack.h"
#include "pub_xml.h"
#include "variable.h"
#include "pub_vars.h"
#include "dfis_var.h"

int check_sep_in(sw_char_t *pkg, sw_int_t len)
{
	int	begin = 8;
	int	length = 2;
	char	value[128];
	
	memset(value, 0x0, sizeof(value));
	memcpy(value, pkg + begin, length);
	pub_log_info("[%s][%d] value=[%s]", __FILE__, __LINE__, value);
	if (strcmp(value, "US") == 0)
	{
		pub_log_info("[%s][%d] SEP!", __FILE__, __LINE__);
		return 0;
	}
	pub_log_info("[%s][%d] Not SEP!", __FILE__, __LINE__);
	return -1;
}

int check_8583_in(sw_char_t *pkg, sw_int_t len)
{
	int	begin = 8;
	int	length = 2;
	char	value[128];
	
	memset(value, 0x0, sizeof(value));
	memcpy(value, pkg + begin, length);
	pub_log_info("[%s][%d] value=[%s]", __FILE__, __LINE__, value);
	if (strcmp(value, "AS") == 0)
	{
		pub_log_info("[%s][%d] 8583!", __FILE__, __LINE__);
		return 0;
	}
	pub_log_info("[%s][%d] Not SEP!", __FILE__, __LINE__);
	return -1;
}

int check_sep_out(sw_loc_vars_t *vars)
{
	char	value[128];
	
	memset(value, 0x0, sizeof(value));
	loc_get_zd_data(vars, "#chnl_no", value);
	pub_log_info("[%s][%d] value=[%s]", __FILE__, __LINE__, value);
	if (strcmp(value, "US") == 0)
	{
		pub_log_info("[%s][%d] SEP!", __FILE__, __LINE__);
		return 0;
	}
	pub_log_info("[%s][%d] Not SEP!", __FILE__, __LINE__);
	
	return -1;
}

int check_8583_out(sw_loc_vars_t *vars)
{
	char	value[128];
	
	memset(value, 0x0, sizeof(value));
	loc_get_zd_data(vars, "#chnl_no", value);
	pub_log_info("[%s][%d] value=[%s]", __FILE__, __LINE__, value);
	if (strcmp(value, "AS") == 0)
	{
		pub_log_info("[%s][%d] 8583!", __FILE__, __LINE__);
		return 0;
	}
	pub_log_info("[%s][%d] Not SEP!", __FILE__, __LINE__);
	
	return -1;
}
