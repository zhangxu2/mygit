#include "pub_computexp.h"

extern sw_loc_vars_t *g_locvar;

extern int maperrcode(sw_loc_vars_t *locvar, char *from, char *to, char *err);

int maperr(args_t *arg)
{
	char	buf[64];
	char	value[128];
	char	errmsg[128];
	
	memset(buf, 0x0, sizeof(buf));
	memset(value, 0x0, sizeof(value));

	if (arg->argc < 1)
	{
		pub_log_error("[%s][%d] maperrin param error!", __FILE__, __LINE__);
		return -1;
	}
	strncpy(buf, arg->argv[0], sizeof(buf) - 1);
	if (arg->argc == 1)
	{
		maperrcode(g_locvar, buf, value, NULL);
	}
	else
	{
		memset(errmsg, 0x0, sizeof(errmsg));
		strcpy(errmsg, arg->argv[1]);
		maperrcode(g_locvar, buf, value, errmsg);
	}
	strncpy(vars.value, value, sizeof(vars.value) - 1);
	vars.type = TYPE_STRING;
	
	return 0;
}

