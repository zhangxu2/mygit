#include "pub_computexp.h"
#include "pub_stack.h"

#define PREC	0.005
sw_loc_vars_t	*g_locvar;
static sqstack_t	*data_stack;
static sqstack_t	*oper_stack;

extern void set_fun_table();

struct
{
	char	funname[64];
	fun_pt	funaddr;
}g_fun_table[MAX_FUN_CNT];

typedef struct
{
	int	type;
	char	*value;
}node_t;

void destroy_node(node_t *data)
{
	if (data == NULL || data->value == NULL)
	{
		return ;
	}
	free(data->value);
	free(data);

	return ;
}

int push_data(char *exp, size_t len)
{
	int	length = 0;
	char	*ptr = NULL;
	char	buf[2048];
	char	name[32];
	node_t	*data = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(name, 0x0, sizeof(name));
	
	data = (node_t *)calloc(1, sizeof(node_t));
	if (data == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	if ((*exp >= '0' && *exp <= '9') || (*exp >= '0' && *(exp + 1) >= '0' && *(exp + 1) <= '9'))
	{
		if (len > 32)
		{
			pub_log_error("[%s][%d] 数字型常量超长! length=[%d]\n", __FILE__, __LINE__, len);
			return -1;
		}
		
		ptr = strchr(exp, '.');
		if (ptr == NULL)
		{
			strncpy(buf, exp, len);
			data->type = TYPE_INT;
			data->value = (char *)calloc(1, sizeof(long));
			if (data->value == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			*(long *)data->value = atol(buf);
		}
		else
		{
			strncpy(buf, exp, len);
			data->type = TYPE_DOUBLE;
			data->value = (char *)calloc(1, sizeof(double));
			if (data->value == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			*(double *)data->value = atof(buf);
		}
	}
	else if (*exp == '#' || *exp == '$')
	{
		memset(buf, 0x0, sizeof(buf));
		memset(name, 0x0, sizeof(name));
		if (len > sizeof(name) - 1)
		{
			pub_log_error("[%s][%d] 变量名太长! length=[%d]\n", __FILE__, __LINE__, length);
			return -1;
		}
		strncpy(name, exp, len);
		length = loc_get_data_len(g_locvar, name, buf);
		data->type = TYPE_STRING;
		data->value = (char *)calloc(1, length + 1);
		if (data->value == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		strncpy(data->value, buf, length);
	}
	else if (*exp == '\'')
	{
		data->type = TYPE_STRING;
		data->value = (char *)calloc(1, len + 1);
		if (data->value == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		strncpy(data->value, exp + 1, len - 2);
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! exp=[%s] len=[%d]", __FILE__, __LINE__, exp, len);
		return -1;
	}
	pub_stack_push(data_stack, data);
	
	return 0;
}

int push_oper(char *exp, int len)
{
	node_t	*data = NULL;

	data = (node_t *)calloc(1, sizeof(node_t));
	if (data == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	data->type = TYPE_OPERATOR;
	data->value = (char *)calloc(1, len + 1);
	strncpy(data->value, exp, len);
	
	pub_stack_push(oper_stack, data);
	
	return 0;
}

node_t *pop_data()
{
	return (node_t *)pub_stack_pop(data_stack);
}

node_t *pop_oper()
{
	return (node_t *)pub_stack_pop(oper_stack);
}

node_t *top_data()
{
	return (node_t *)pub_sqstack_top(data_stack);
}

node_t *top_oper()
{
	return (node_t *)pub_sqstack_top(oper_stack);
}

int init_exp_env()
{
	data_stack = pub_stack_new(NULL, (stack_pop_free_pt)destroy_node);
	if (data_stack == NULL)
	{
		pub_log_error("[%s][%d] Alloc stack error!", __FILE__, __LINE__);
		return -1;
	}
	
	oper_stack = pub_stack_new(NULL, (stack_pop_free_pt)destroy_node);
	if (oper_stack == NULL)
	{
		pub_log_error("[%s][%d] Alloc stack error!", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

int destroy_exp_env()
{
	pub_stack_pop_free(data_stack);
	pub_stack_pop_free(oper_stack);

	return 0;
}

int g_fun_cnt = 0;

void set_one_fun_addr(int id, char *funname, fun_pt ptr)
{
	strcpy(g_fun_table[id].funname, funname);
	g_fun_table[id].funaddr = ptr;
	return ;
}

int get_fun_from_lib(char *funname)
{
	int	index = -1;
	char	libso[128];
	static int	first = 1;
	static void	*handle = NULL;
	fun_pt	dlfun = NULL;
	
	memset(libso, 0x0, sizeof(libso));
	sprintf(libso, "%s/plugin/libswudf.so", getenv("SWHOME"));
	if (access(libso, F_OK) != 0)
	{
		return -1;
	}
	
	if (first == 1)
	{
		handle = (void *)dlopen(libso, RTLD_NOW | RTLD_GLOBAL);
		if (handle == NULL)
		{
			pub_log_error("[%s][%d] dlopen [%s] error! error:[%s]",
				__FILE__, __LINE__, libso, dlerror());
			return -1;
		}
		first = 0;
		pub_log_info("[%s][%d] dlopen [%s] success!", __FILE__, __LINE__, libso);
	}
	
	dlfun = (fun_pt)dlsym(handle, funname);
	if (dlfun == NULL)
	{
		pub_log_error("[%s][%d] dlsym [%s][%s] error! error:[%s]",
			__FILE__, __LINE__, libso, dlerror());
		return -1;
	}
	index = g_fun_cnt;
	strcpy(g_fun_table[g_fun_cnt].funname, funname);
	g_fun_table[g_fun_cnt].funaddr = dlfun;
	g_fun_cnt++;
	
	return index;
}

int run(args_t arg, char *funname)
{
	int	i = 0;
	static int	first = 0;
	fun_pt	fun;
	
	if (first == 0)
	{
		set_fun_table();
		first = 1;
	}
	
	for (i = 0; i < g_fun_cnt; i++)
	{
		if (strcmp(g_fun_table[i].funname, funname) == 0)
		{
			break;
		}
	}
	
	if (i == g_fun_cnt)
	{
		if ((i = get_fun_from_lib(funname)) < 0)
		{
			pub_log_error("function [%s] not defined!\n", funname);
			return -1;
		}
	}
	
	fun = g_fun_table[i].funaddr;
	fun(&arg);
	
	return 0;
}

char *get_one_word(char *exp)
{
	int	block = 0;
	char	first = '\0';
	char	*p = NULL;
	char	*ptr = NULL;
	static char	*last = NULL;
	static char	word[1024];
	
	memset(word, 0x0, sizeof(word));
	p = word;
	
	if (exp != NULL)
	{
		ptr = exp;
		last = exp;
	}
	else
	{
		ptr = last;
	}
	
	while (*ptr != '\0')
	{
		if (*ptr != ' ')
		{
			break;
		}
		ptr++;
	}
	
	first = *ptr;
	if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || *ptr == '#' || *ptr == '$')
	{
		while (*ptr != '\0')
		{
			*p++ = *ptr++;
			if ((first == '#' || first == '$') && (*ptr == '(' || *ptr == ')'))
			{
				if (*ptr == '(')
				{
					if (block == 1)
					{
						pub_log_error("[%s][%d] Express [%s] format error!",
							__FILE__, __LINE__, last);
						return NULL;
					}
					block = 1;
				}
				if (*ptr == ')')
				{
					if (block == 0)
					{
						break;
					}
					block = 0;
				}
				continue;
			}
			else if (!((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') 
				|| (*ptr >= '0' && *ptr <= '9') || (*ptr == '_') || (*ptr == '.')))
			{
				break;
			}
		}
		last = ptr;
		*p = '\0';
		return word;
	}

	if (*ptr == '(' || *ptr == ')' || *ptr == '+' || *ptr == '-' || *ptr == '/' || *ptr == '*' || *ptr == ',')
	{
		*p++ = *ptr++;
		last = ptr;
		*p = '\0';
		return word;
	}
	
	if (*ptr == '=' || *ptr == '>' || *ptr == '<' || *ptr == '!')
	{
		*p++ = *ptr++;
		if (*ptr == '=')
		{
			*p++ = *ptr++;
		}
		last = ptr;
		*p = '\0';
		return word;
	}
	
	if (*ptr == '&')
	{
		*p++ = *ptr++;
		if (*ptr == '&')
		{
			*p++ = *ptr++;
		}
		last = ptr;
		*p = '\0';
		return word;
	}
	
	if (*ptr == '|')
	{
		*p++ = *ptr++;
		if (*ptr == '|')
		{
			*p++ = *ptr++;
		}
		last = ptr;
		*p = '\0';
		return word;
	}

	if (*ptr >= '0' && *ptr <= '9')
	{
		while (*ptr != '\0')
		{
			*p++ = *ptr++;
			if (!((*ptr >= '0' && *ptr <= '9') || (*ptr == '.')))
			{
				break;
			}
		}
		last = ptr;
		*p = '\0';
		return word;
	}
	
	if (*ptr == '\'')
	{
		while (*ptr != '\0')
		{
			*p++ = *ptr++;
			if (*ptr == '\'')
			{
				break;
			}
		}
		if (*ptr == '\0')
		{
			return NULL;
		}
		*p++ = *ptr++;
		last = ptr;
		*p = '\0';
		return word;
	}
	
	return NULL;
}

int check_exp(char *sentbuf)
{
	int	cnt = 0;
	char	*ptr = NULL;
	
	ptr = get_one_word(NULL);
	if (ptr == NULL || *ptr != '(')
	{
		pub_log_error("[%s][%d] Format error!\n", __FILE__, __LINE__);
		return -1;
	}
	strcat(sentbuf, ptr);
	
	cnt = 1;
	while (cnt != 0)
	{
		ptr = get_one_word(NULL);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] Format error!\n", __FILE__, __LINE__);
			return -1;
		}
		
		if (*ptr == '#' ||  *ptr == '$')
		{
			strcat(sentbuf,  ptr);
		}
		else if (*ptr == '\'' || (*ptr >= '0' && *ptr <= '9') || *ptr == '+' ||
			*ptr == '-' || *ptr == '*' || *ptr == '/' || *ptr == '.' || *ptr == ',')
		{
			strcat(sentbuf,  ptr);
		}
		else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z'))
		{
			strcat(sentbuf,  ptr);
		} 
		else if (*ptr == '(')
		{
			strcat(sentbuf,  ptr);
			cnt++;
		}
		else if (*ptr == ')')
		{
			strcat(sentbuf,  ptr);
			cnt--;
		}
		else
		{
			pub_log_error("[%s][%d] Format error!\n", __FILE__, __LINE__);
			return -1;
		}
	}
	
	if (cnt != 0)
	{
		pub_log_error("[%s][%d] Express format error!\n", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

int get_args(char *exp, args_t *arg)
{
	int	len = 0;
	int	quto = 0;
	int	block = 0;
	char	*p = NULL;
	char	*ptr = exp;
	char	buf[512];
	char	argv[128];
	
	memset(buf, 0x0, sizeof(buf));
	memset(argv, 0x0, sizeof(argv));

	while (*ptr != '\0' && *ptr != '(')
	{
		ptr++;
	}
	
	if (*ptr == '\0')
	{
		pub_log_error("[%s][%d] Format error! exp=[%s]\n", __FILE__, __LINE__, exp);
		return -1;
	}
	
	if (*(ptr + 1) == ')')
	{
		arg->argc = 0;
		return 0;
	}
	
	ptr++;
	quto = 0;
	p = argv;
	while (*ptr != '\0')
	{
		if (*ptr == '\'')
		{
			if (quto == 0)
			{
				quto = 1;
			}
			else
			{
				quto = 0;
			}
			ptr++;
			continue;
		}
		
		if (*ptr == ',' && quto == 0)
		{
			if (argv[0] == '#' || argv[0] == '$')
			{
				memset(buf, 0x0, sizeof(buf));
				len = loc_get_data_len(g_locvar, argv, buf); 
				if (len < 0)
				{
					len = 0;
				}
				arg->argv[arg->argc] = (char *)calloc(1, len + 1);
				strncpy(arg->argv[arg->argc], buf, len);
			}
			else
			{
				len = strlen(argv);
				arg->argv[arg->argc] = (char *)calloc(1, len + 1);
				strncpy(arg->argv[arg->argc], argv, len);
			}
			arg->argc++;
			ptr++;
			while (*ptr == ' ')
			{
				ptr++;
			}
			memset(argv, 0x0, sizeof(argv));
			p = argv;
			continue;
		}
		
		if (*ptr == ')' && quto == 0 && block == 0)
		{
			if (argv[0] == '#' || argv[0] == '$')
			{
				memset(buf, 0x0, sizeof(buf));
				len = loc_get_data_len(g_locvar, argv, buf); 
				if (len < 0)
				{
					len = 0;
				}
				arg->argv[arg->argc] = (char *)calloc(1, len + 1);
				strncpy(arg->argv[arg->argc], buf, len);
			}
			else
			{
				len = strlen(argv);
				arg->argv[arg->argc] = (char *)calloc(1, len + 1);
				strncpy(arg->argv[arg->argc], argv, len);
			}
			arg->argc++;
			return 0;
		}

		if ((*ptr == '(' || *ptr == ')') && (argv[0] == '#' || argv[0] == '$'))
		{
			if (*ptr == '(')
			{
				if (block == 1)
				{
					pub_log_error("[%s][%d] Function express [%s] format error!",
						__FILE__, __LINE__, exp);
					return -1;
				}
				block = 1;
			}
			else if (*ptr == ')' && block == 1)
			{
				block = 0;
			}

			*p++ = *ptr++;
			pub_log_debug("[%s][%d] argv=[%s]", __FILE__, __LINE__, argv);
			continue;
		}
		*p++ = *ptr++;
	}
	
	return 0;
}

int clean_args(args_t *arg)
{
	int	i = 0;
	
	for (i = 0; i < arg->argc; i++)
	{
		if (arg->argv[i] != NULL)
		{
			free(arg->argv[i]);
			arg->argv[i] = NULL;
		}
	}
	
	return 0;
}

int run_fun(char *fun)
{
	int	ret = 0;
	char	exp[512];
	args_t	arg;
	
	memset(exp, 0x0, sizeof(exp));
	memset(&arg, 0x0, sizeof(arg));
	
	if (fun == NULL || fun[0] == '\0')
	{
		pub_log_error("[%s][%d] Run_fun error! fun is null!\n", __FILE__, __LINE__);
		return -1;
	}
	
	strcpy(exp, fun);
	ret = check_exp(exp);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Check exp error!\n", __FILE__, __LINE__);
		return -1;
	}
	get_args(exp, &arg);
	ret = run(arg, fun);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] run fun [%s] error!\n", __FILE__, __LINE__, fun);
		clean_args(&arg);
		return -1;
	}
	clean_args(&arg);
	
	return 0;
}

static int comp_dbl(double d1, double d2)
{
	const double	prec = 0.005;
	
	if (d1 - d2 > -prec && d1 - d2 < prec)
	{
		return 0;
	}

	if (d1 - d2 > prec)
	{
		return 1;
	}

	return -1;
}

static int compute_add(node_t *data1, node_t *data2)
{
	char	buf[128];
	char	*ptr = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		ptr = (char *)calloc(1, strlen(data1->value) + strlen(data2->value) + 3);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]\n", __FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		strcat(ptr, "'");
		strcat(ptr, data1->value);
		strcat(ptr, data2->value);
		strcat(ptr, "'");
		push_data(ptr, strlen(ptr));
		free(ptr);
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) + STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%ld", STR2INT(data1->value) + STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2INT(data1->value) + STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) + STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]\n", 
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	
	return 0;
}


static int compute_sub(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) - STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%ld", STR2INT(data1->value) - STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2INT(data1->value) - STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) - STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误!\n", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

static int compute_mul(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) * STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%ld", STR2INT(data1->value) * STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2INT(data1->value) * STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) * STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误!\n", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

static int compute_div(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) == 0)
		{
			pub_log_error("[%s][%d] The divisor is zero!", __FILE__, __LINE__);
			return -1;
		}
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) / STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%ld", STR2INT(data1->value) / STR2INT(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", (STR2INT(data1->value) * 1.0) / STR2DBL(data2->value));
		push_data(buf, strlen(buf));
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%f", STR2DBL(data1->value) / (STR2INT(data2->value) * 1.0));
		push_data(buf, strlen(buf));
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]\n", 
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	
	return 0;
}

static int compute_not(node_t *data)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data->type == TYPE_INT)
	{
		memset(buf, 0x0, sizeof(buf));
		if (STR2INT(data->value) == 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data->value);
		if (comp_dbl(d, 0.00) == 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data->type == TYPE_STRING)
	{
		if (data->value[0] == '\0')
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! type=[%d]", __FILE__, __LINE__, data->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_ne(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
		
	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) != STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data1->value) - STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) == 0)
		{
			sprintf(buf, "0");
		}
		else
		{
			sprintf(buf, "1");
		}
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		double	d1 = STR2INT(data1->value);
		double	d2 = STR2DBL(data2->value);
		if (comp_dbl(d1, d2) == 0)
		{
			sprintf(buf, "0");
		}
		else
		{
			sprintf(buf, "1");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		double	d1 = STR2DBL(data1->value);
		double	d2 = STR2INT(data2->value);
		if (comp_dbl(d1, d2) == 0)
		{
			sprintf(buf, "0");
		}
		else
		{
			sprintf(buf, "1");
		}
	}
	else if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (strcmp(data1->value, data2->value) != 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]",
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;			
}

static int compute_eq(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) != STR2INT(data2->value))
		{
			sprintf(buf, "0");
		}
		else
		{
			sprintf(buf, "1");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data1->value) - STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) == 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		double	d1 = STR2INT(data1->value);
		double	d2 = STR2DBL(data2->value);
		if (comp_dbl(d1, d2) == 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		double	d1 = STR2DBL(data1->value);
		double	d2 = STR2INT(data2->value);
		if (comp_dbl(d1, d2) == 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (strcmp(data1->value, data2->value) != 0)
		{
			sprintf(buf, "0");
		}
		else
		{
			sprintf(buf, "1");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]",
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_gt(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{	
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) > STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data1->value) - STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) > 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		double	d1 = STR2INT(data1->value);
		double	d2 = STR2DBL(data2->value);
		if (comp_dbl(d1, d2) > 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		double	d1 = STR2DBL(data1->value);
		double	d2 = STR2INT(data2->value);
		if (comp_dbl(d1, d2) > 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (strcmp(data1->value, data2->value) > 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]",
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_lt(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) < STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data1->value) - STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) < 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		double	d1 = STR2INT(data1->value);
		double	d2 = STR2DBL(data2->value);
		if (comp_dbl(d1, d2) < 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		double	d1 = STR2DBL(data1->value);
		double	d2 = STR2INT(data2->value);
		if (comp_dbl(d1, d2) < 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (strcmp(data1->value, data2->value) < 0)
		{
			sprintf(buf, "0");
		}
		else
		{
			sprintf(buf, "1");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]",
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_ge(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	
	if (data1 == NULL || data2 == NULL)
	{	
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}	

	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) >= STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data1->value) - STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) >= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		double	d1 = STR2INT(data1->value);
		double	d2 = STR2DBL(data2->value);
		if (comp_dbl(d1, d2) >= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		double	d1 = STR2DBL(data1->value);
		double	d2 = STR2INT(data2->value);
		if (comp_dbl(d1, d2) >= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (strcmp(data1->value, data2->value) >= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]",
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_le(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	if (data1 == NULL || data2 == NULL)
	{	
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) <= STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		double	d = STR2DBL(data1->value) - STR2DBL(data2->value);
		if (comp_dbl(d, 0.00) <= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		double	d1 = STR2INT(data1->value);
		double	d2 = STR2DBL(data2->value);
		if (comp_dbl(d1, d2) <= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		double	d1 = STR2DBL(data1->value);
		double	d2 = STR2INT(data2->value);
		if (comp_dbl(d1, d2) <= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (strcmp(data1->value, data2->value) <= 0)
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	else
	{
		pub_log_error("[%s][%d] 数据类型有误! data1->type=[%d] data2->type=[%d]",
			__FILE__, __LINE__, data1->type, data2->type);
		return -1;
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_and(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) && STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}

	if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		if (STR2INT(data1->value) && comp_dbl(STR2DBL(data2->value), 0.00))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}

	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		if (comp_dbl(STR2DBL(data1->value), 0.00) && STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}

	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		if (comp_dbl(STR2DBL(data1->value), 0.00) && comp_dbl(STR2DBL(data2->value), 0.00))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	
	if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (data1->value[0] != '\0' && data2->value[0] != '\0')
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

static int compute_or(node_t *data1, node_t *data2)
{
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	if (data1 == NULL || data2 == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (data1->type == TYPE_INT && data2->type == TYPE_INT)
	{
		if (STR2INT(data1->value) || STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}

	if (data1->type == TYPE_INT && data2->type == TYPE_DOUBLE)
	{
		if (STR2INT(data1->value) || comp_dbl(STR2DBL(data2->value), 0.00))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}

	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_INT)
	{
		if (comp_dbl(STR2DBL(data1->value), 0.00) || STR2INT(data2->value))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}

	if (data1->type == TYPE_DOUBLE && data2->type == TYPE_DOUBLE)
	{
		if (comp_dbl(STR2DBL(data1->value), 0.00) || comp_dbl(STR2DBL(data2->value), 0.00))
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	
	if (data1->type == TYPE_STRING && data2->type == TYPE_STRING)
	{
		if (data1->value[0] != '\0' || data2->value[0] != '\0')
		{
			sprintf(buf, "1");
		}
		else
		{
			sprintf(buf, "0");
		}
	}
	push_data(buf, strlen(buf));
	
	return 0;
}

int compute_base(char *oper)
{
	int	ret = 0;
	node_t	*data1 = NULL;
	node_t	*data2 = NULL;
	
	if (oper == NULL || (*oper != '+' && *oper != '-' && *oper != '*' && *oper != '/' && 
		*oper != '!' && *oper != '=' && *oper != '>' && *oper != '<' && *oper != '&' && *oper != '|'))
	{
		pub_log_error("[%s][%d] Compute_base param error! oper=[%s]\n", __FILE__, __LINE__, oper);
		return -1;
	}
	
	data2 = pop_data();
	if (data2 == NULL)
	{
		pub_log_error("[%s][%d] Compute error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (strcmp(oper, "!") != 0)
	{
		data1 = pop_data();
		if (data1 == NULL)
		{
			pub_log_error("[%s][%d] data2->type=[%d]", __FILE__, __LINE__, data2->type);
			pub_log_error("[%s][%d] Compute error!", __FILE__, __LINE__);
			destroy_node(data2);
			return -1;
		}
	}

	if (*oper == '+')
	{
		ret = compute_add(data1, data2);
	}
	else if (*oper == '-')
	{
		ret = compute_sub(data1, data2);
	}
	else if (*oper == '*')
	{
		ret = compute_mul(data1, data2);
	}
	else if (*oper == '/')
	{
		ret = compute_div(data1, data2);
	}
	else if (strcmp(oper, "!") == 0)
	{
		ret = compute_not(data2);
	}
	else if (strcmp(oper, "==") == 0)
	{
		ret = compute_eq(data1, data2);
	}
	else if (strcmp(oper, "!=") == 0)
	{
		ret = compute_ne(data1, data2);
	}
	else if (strcmp(oper, ">") == 0)
	{
		ret = compute_gt(data1, data2);
	}
	else if (strcmp(oper, "<") == 0)
	{
		ret = compute_lt(data1, data2);
	}
	else if (strcmp(oper, ">=") == 0)
	{
		ret = compute_ge(data1, data2);
	}
	else if (strcmp(oper, "<=") == 0)
	{
		ret = compute_le(data1, data2);
	}
	else if (strcmp(oper, "&&") == 0)
	{
		ret = compute_and(data1, data2);
	}
	else if (strcmp(oper, "||") == 0)
	{
		ret = compute_or(data1, data2);
	}
	else
	{
		pub_log_error("[%s][%d] oper [%s] error!", __FILE__, __LINE__, oper);
		destroy_node(data1);
		destroy_node(data2);
		return -1;
	}
	if (ret < 0)
	{
		pub_log_error("[%s][%d] compute [%s] error!", __FILE__, __LINE__, oper);
		destroy_node(data1);
		destroy_node(data2);
		return -1;
	}
	destroy_node(data1);
	destroy_node(data2);
	
	return 0;
}

static int priorcmp(char *op1, char *op2)
{
	/***
	比较两个运算符的优先级，若op1>op2则返回1,＝返回0，小于返回－1
	运算符的长度以op1为准，它是以'\0'结束的标准字符串
	优先级别从低到高 or and not  【> >= < <= != ==】 【+ -】 【* /】 ([
	)与]的优先级在computexp中特殊处理
	([为op1优先级最高,为op2优先级最低
	***/
	char	opc1 = op1[0];
	char	opc2 = op2[0];
	
	if (strcmp(op1, "(") == 0)
	{
		return 1;
	}
	else if (strcmp(op2, "(") == 0)
	{
		return 1;
	}
	
	if (strcmp(op1, "[") == 0)
	{
		return 1;
	}
	else if (strcmp(op2, "[") == 0)
	{
		return 1;
	}
	
	if (strcmp(op1, "||") == 0)
	{
		if (strcmp(op2, "||") == 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	
	if (strcmp(op1, "&&") == 0)
	{
		if (strcmp(op2, "||") == 0)
		{
			return 1;
		}
		else if (strcmp(op2, "&&") == 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	
	if (strcmp(op1, "!") == 0)
	{
		if (strcmp(op2, "||") == 0 || strcmp(op2, "&&") == 0)
		{
			return 1;
		}
		else if (strcmp(op2, "!") == 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	
	switch (opc1)
	{
	case '>':
	case '<':
	case '=':
	case '!':
		if (strcmp(op2, "||") == 0 || strcmp(op2, "&&") == 0 || strcmp(op2, "!") == 0)
		{
			return 1;
		}
		else if (opc2 == '>' || opc2 == '<' || opc2 == '!' || opc2 == '=')
		{
			return 0;
		}
		else 
		{
			return -1;
		}
		break;
	case '+':
	case '-':
		if (strcmp(op2, "||") == 0 || strcmp(op2, "&&") == 0 || strcmp(op2, "!") == 0 ||
			opc2 == '>' || opc2 == '<' || opc2 == '!' || opc2 == '=')
		{
			return 1;
		}
		else if (opc2 == '+' || opc2 == '-')
		{
			return 0;
		}
		else
		{
			return -1;
		}
		break;
	case '*':
	case '/':
		if (strcmp(op2, "||") == 0 || strcmp(op2, "&&") == 0 || strcmp(op2, "!") == 0 || 
			opc2 == '>' || opc2 == '<' || opc2 == '!' || opc2 == '=' || opc2 == '+' || opc2 == '-')
		{
			return 1;
		}
		else if (opc2 == '*' || opc2 == '/')
		{
			return 0;
		}
		else
		{
			return -1;
		}
		break;
	case '(':
	case '[':
		return 1;
	}
	
	return -1;
}


int compute_exp(sw_loc_vars_t *locvars, char *str, char *value)
{
	int	ret = 0;
	char	*p = NULL;
	char	*ptr = NULL;
	char	funname[32];
	node_t	*data = NULL;
	
	g_locvar = locvars;

	init_exp_env();
	memset(funname, 0x0, sizeof(funname));
	
	ptr = get_one_word(str);
	while (ptr != NULL)
	{
		if (*ptr == '#' || *ptr == '$')
		{
			/*** 变量直接入栈 ***/
			push_data(ptr, strlen(ptr));
		}
		else if ((*ptr >= '0' && *ptr <= '9') || *ptr == '\'')
		{
			/*** 常量入栈 ***/
			push_data(ptr, strlen(ptr));
		}
		else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z'))
		{
			/*** 函数 计算后直接入栈 ***/
			memset(&vars, 0x0, sizeof(vars));
			memset(funname, 0x0, sizeof(funname));
			strcpy(funname, ptr);
			ret = run_fun(funname);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] run_fun [%s] error!\n", __FILE__, __LINE__, funname);
				goto err;
			}
			pub_log_debug("[%s][%d] FUNCTION:[%s] value=[%s] type=[%d]", 
				__FILE__, __LINE__, funname, vars.value, vars.type);
			if (vars.type == TYPE_INT)
			{
				p = (char *)calloc(1, strlen(vars.value) + 1);
				strcpy(p, vars.value);
				push_data(p, strlen(p));
			}
			else
			{
				p = (char *)calloc(1, strlen(vars.value) + 3);
				strcat(p, "'");
				strcpy(p + 1, vars.value);
				strcat(p, "'");
				push_data(p, strlen(p));
			}
			free(p);
		}
		else if (*ptr == '(')
		{
			/*** (直接入栈 ***/
			push_oper(ptr, strlen(ptr));
		}
		else if (*ptr == ')')
		{
			/*** 所有运算符出栈,直至遇到( ***/
			while (1)
			{
				data = top_oper();
				if (data == NULL)
				{
					pub_log_error("[%s][%d] 未找到匹配的(括号!\n", __FILE__, __LINE__);
					goto err;
				}
				if (data->value[0] == '(')
				{
					destroy_node(data);
					pop_oper();
					break;
				}
				ret = compute_base(data->value);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] compute error!\n", __FILE__, __LINE__);
					goto err;
				}
				destroy_node(data);
				pop_oper();
			}
		}
		else if (*ptr == '+' || *ptr == '-')
		{
			/*** 所有运算符出栈,直至遇到(或栈空 ***/
			while (1)
			{
				data = top_oper();
				if (data == NULL || data->value[0] == '(')
				{
					break;
				}
				ret = compute_base(data->value);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] compute error! oper=[%s]\n", __FILE__, __LINE__, data->value);
					goto err;
				}
				destroy_node(data);
				pop_oper();
			}
			push_oper(ptr, strlen(ptr));
		}
		else if (*ptr == '*' || *ptr == '/')
		{
			/*** *和/出栈 ***/
			while (1)
			{
				data = top_oper();
				if (data == NULL || (data->value[0] != '*' && data->value[0] != '/'))
				{
					break;
				}
				ret = compute_base(data->value);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] compute error!\n", __FILE__, __LINE__);
					goto err;
				}
				destroy_node(data);
				pop_oper();
			}
			push_oper(ptr, strlen(ptr));
		}
		else if (strcmp(ptr, "!=") == 0 || strcmp(ptr, "==") == 0 || 
			strcmp(ptr, ">") == 0 || strcmp(ptr, ">=") == 0 ||
			strcmp(ptr, "<") == 0 || strcmp(ptr, "<=") == 0 || 
			strcmp(ptr, "!") == 0 || strcmp(ptr, "&&") == 0 || strcmp(ptr, "||") == 0)
		{
			while (1)
			{
				data = top_oper();
				if (data == NULL)
				{
					break;
				}
				
				if (priorcmp(ptr, data->value) > 0)
				{
					break;
				}

				ret = compute_base(data->value);
				if (ret < 0)
				{
					pub_log_error("[%s][%d] compute error!\n", __FILE__, __LINE__);
					goto err;
				}
				destroy_node(data);
				pop_oper();
			}
			push_oper(ptr, strlen(ptr));
		}
		else if (strcmp(ptr, "=") == 0)
		{
			pub_log_error("[%s][%d] 暂时不支持赋值操作!", __FILE__, __LINE__);
			goto err;
		}

		ptr = get_one_word(NULL);
	}
	
	/*** 计算未出栈的算符 ***/
	while (1)
	{
		data = pop_oper();
		if (data == NULL)
		{
			break;
		}
		ret = compute_base(data->value);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] compute error!\n", __FILE__, __LINE__);
			goto err;
		}
		destroy_node(data);
	}
	
	data = pop_data();
	if (data == NULL)
	{
		pub_log_error("[%s][%d] Compute express error!\n", __FILE__, __LINE__);
		goto err;
	}
	
	if (data->type == TYPE_DOUBLE)
	{
		sprintf(value, "%.5f", STR2DBL(data->value));
	}
	else if (data->type == TYPE_INT)
	{
		sprintf(value, "%ld", STR2INT(data->value));
	}
	else
	{
		strcpy(value, data->value);
	}
	destroy_node(data);
	destroy_exp_env();
	return 0;

err:
	destroy_exp_env();
	return -1;
}

