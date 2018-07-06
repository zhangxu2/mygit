#include "pub_computexp.h"

extern int maperrcode(sw_loc_vars_t *locvar, char *from, char *to, char *err);

extern char *zip_space(char *str);
extern char *zip_head(char *str);
extern char *zip_tail(char *str);

int trim(args_t *arg)
{
	char	*ptr = NULL;
	
	if (arg->argc != 1)
	{
		pub_log_error("[%s][%d] trim args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}
	ptr = arg->argv[0];
	zip_space(ptr);
	strcpy(vars.value, ptr);
	vars.type = TYPE_STRING;
	pub_log_debug("[%s][%d] value=[%s]", __FILE__, __LINE__, vars.value);
	
	return 0;
}

int ltrim(args_t *arg)
{
	char	*ptr = NULL;
	
	if (arg->argc != 1)
	{
		pub_log_error("[%s][%d] trim args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}
	ptr = arg->argv[0];
	zip_head(ptr);
	strcpy(vars.value, ptr);
	vars.type = TYPE_STRING;
	pub_log_debug("[%s][%d] value=[%s]", __FILE__, __LINE__, vars.value);
	
	return 0;
}

int rtrim(args_t *arg)
{
	char	*ptr = NULL;
	
	if (arg->argc != 1)
	{
		pub_log_error("[%s][%d] trim args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}
	ptr = arg->argv[0];
	zip_tail(ptr);
	strcpy(vars.value, ptr);
	vars.type = TYPE_STRING;
	pub_log_debug("[%s][%d] value=[%s]\n", __FILE__, __LINE__, vars.value);
	
	return 0;
}

int substr(args_t *arg)
{
	int     len = 0;
	size_t	index = 0;
	char    *ptr = NULL;

	if (arg->argc != 3 && arg->argc != 2)
	{
		pub_log_error("[%s][%d] substr args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}

	memset(&vars, 0x0, sizeof(vars));
	ptr = arg->argv[0];
	index = atoi(arg->argv[1]);
	if (arg->argc == 3 && index < strlen(ptr))
	{
		len = atoi(arg->argv[2]);
		strncpy(vars.value, ptr + index, len);
	}
	else
	{                                                                                                                
		strcpy(vars.value, ptr + index);                                                                         
	}                                                                                                                
	vars.type = TYPE_STRING;
	pub_log_debug("[%s][%d] value=[%s]", __FILE__, __LINE__, vars.value);

	return 0;                                                                                                        
} 

int max(args_t *arg)
{
	int	a = 0;
	int	b = 0;
	
	memset(&vars, 0x0, sizeof(vars));
	if (arg->argc != 2)
	{
		pub_log_error("[%s][%d] max args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}	

	a = atoi(arg->argv[0]);
	b = atoi(arg->argv[1]);
	
	if (a > b)
	{
		sprintf(vars.value, "%d", a);
	}
	else
	{
		sprintf(vars.value, "%d", b);
	}
	vars.type = TYPE_INT;
	pub_log_debug("[%s][%d] value=[%s]", __FILE__, __LINE__, vars.value);
	
	return 0;
}

int min(args_t *arg)
{
	int	a = 0;
	int	b = 0;
	
	memset(&vars, 0x0, sizeof(vars));
	if (arg->argc != 2)
	{
		pub_log_error("[%s][%d] max args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}	

	a = atoi(arg->argv[0]);
	b = atoi(arg->argv[1]);
	
	if (a < b)
	{
		sprintf(vars.value, "%d", a);
	}
	else
	{
		sprintf(vars.value, "%d", b);
	}
	vars.type = TYPE_INT;
	pub_log_debug("[%s][%d] value=[%s]", __FILE__, __LINE__, vars.value);
	
	return 0;
}

int date8to10(args_t *arg)
{
	char	date8[16];
	char	date10[16];
	
	memset(date8, 0x0, sizeof(date8));
	memset(date10, 0x0, sizeof(date10));
	memset(&vars, 0x0, sizeof(vars));

	strncpy(date8, arg->argv[0], sizeof(date8) - 1);
	zip_space(date8);
	if (strlen(date8) == 10)
	{
		strcpy(date10, date8);
	}
	else
	{
		memcpy(date10, date8, 4);
		memcpy(date10 + 4, "-", 1);
		memcpy(date10 + 5, date8 + 4, 2);
		memcpy(date10 + 7, "-", 1);	
		memcpy(date10 + 8, date8 + 6, 4);
	}
	strcpy(vars.value, date10);
	vars.type = TYPE_STRING;

	return 0;
}

int date10to8(args_t *arg)
{
	char	date8[16];
	char	date10[16];
	
	memset(date8, 0x0, sizeof(date8));
	memset(date10, 0x0, sizeof(date10));
	memset(&vars, 0x0, sizeof(vars));
	
	strncpy(date10, arg->argv[0], sizeof(date10) - 1);
	zip_space(date10);
	if (strlen(date10) <= 8)
	{
		strcpy(date8, date10);
	}
	else
	{
		memcpy(date8, date10, 4);
		memcpy(date8 + 4, date10 + 5, 2);	
		memcpy(date8 + 6, date10 + 8, 2);
	}
	strcpy(vars.value, date8);
	vars.type = TYPE_STRING;
	
	return 0;
}

int double2num(args_t *arg)
{
	char	amt[128];
	char    buf[128];
	char    *ptr = NULL;
	
	memset(amt, 0x0, sizeof(amt));
	memset(buf, 0x0, sizeof(buf));

	strcpy(amt, arg->argv[0]);
	sprintf(buf, "%.2f", strtod(amt, NULL) * 100);
	ptr = strchr(buf, '.');
	if (ptr != NULL)
	{	
		memcpy(vars.value, buf, ptr - buf);
		
	}
	else
	{
		strcpy(vars.value, buf);
	}
	vars.type = TYPE_INT;

	return 0;
}

int num2double(args_t *arg)
{
	char	amt[128];
	char	buf[128];
	double	value = 0.00;
	
	memset(amt, 0x0, sizeof(amt));
	memset(buf, 0x0, sizeof(buf));

	strcpy(amt, arg->argv[0]);
	value = strtod(amt, NULL);
	sprintf(vars.value, "%.2f", value / 100.00);
	vars.type = TYPE_DOUBLE;
	
	return 0;
}

int atof_ext(args_t *arg)
{
	char	amt[128];
	char	buf[128];
	double	value = 0.00;
	
	memset(amt, 0x0, sizeof(amt));
	memset(buf, 0x0, sizeof(buf));

	strcpy(amt, arg->argv[0]);
	value = strtod(amt, NULL);
	sprintf(vars.value, "%.2f", value);
	vars.type = TYPE_DOUBLE;
	
	return 0;
}

int atoii(args_t *arg)
{
	char	buf[128];
	char	tmp[128];
	
	memset(buf, 0x0, sizeof(buf));
	memset(tmp, 0x0, sizeof(tmp));
	strcpy(buf, arg->argv[0]);
	sprintf(vars.value, "%lld", atoll(buf));
	vars.type = TYPE_INT;
	
	return 0;
}

int strcat_ext(args_t *arg)
{
	char	arg1[512];
	char	arg2[512];
	
	memset(arg1, 0x0, sizeof(arg1));
	memset(arg2, 0x0, sizeof(arg2));

	if (arg->argc != 2)
	{
		pub_log_error("[%s][%d] strcat args error! argc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}	
	
	strncpy(arg1, arg->argv[0], sizeof(arg1) - 1);
	strncpy(arg2, arg->argv[1], sizeof(arg2) - 1);
	snprintf(vars.value, sizeof(vars.value), "%s%s", arg1, arg2);
	vars.type = TYPE_STRING;

	return 0;

}

int strlen_ext(args_t *arg)
{
	char	arg1[512];
	
	memset(arg1, 0x0, sizeof(arg1));
	
	if (arg->argc != 1)
	{
		pub_log_error("[%s][%d] strlen_ext args error! argrc=[%d]", __FILE__, __LINE__, arg->argc);
		return -1;
	}
	strcpy(arg1, arg->argv[0]);
	sprintf(vars.value, "%zd", strlen(arg1));
	vars.type = TYPE_INT;
	
	return 0;
}

void set_fun_table()
{
	int	id = 0;
	
	set_one_fun_addr(id++, "substr", substr);
	set_one_fun_addr(id++, "max", max);
	set_one_fun_addr(id++, "min", min);
	set_one_fun_addr(id++, "date8to10", date8to10);
	set_one_fun_addr(id++, "date10to8", date10to8);
	set_one_fun_addr(id++, "double2num", double2num);
	set_one_fun_addr(id++, "num2double", num2double);
	set_one_fun_addr(id++, "trim", trim);
	set_one_fun_addr(id++, "ltrim", ltrim);
	set_one_fun_addr(id++, "rtrim", rtrim);
	set_one_fun_addr(id++, "atoi", atoii);
	set_one_fun_addr(id++, "strcat", strcat_ext);
	set_one_fun_addr(id++, "atof", atof_ext);
	set_one_fun_addr(id++, "amt2num", double2num);
	set_one_fun_addr(id++, "num2amt", num2double);
	set_one_fun_addr(id++, "strlen", strlen_ext);
	g_fun_cnt = id;
	pub_log_info("[%s][%d] set_fun_table!", __FILE__, __LINE__);
}
