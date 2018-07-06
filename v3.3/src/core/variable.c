#include "pub_xml.h"
#include "pub_vars.h"
#include "pub_log.h"
#include "pub_mem.h"

int loc_vars_clear(sw_loc_vars_t *vars)
{
	vars->clear_vars(vars);
	
	return SW_OK;
}

int loc_extvars_clear(sw_loc_vars_t *vars)
{
	vars->clear_xml_vars(vars);

	return SW_OK;
}

int loc_var_remove(sw_loc_vars_t *vars, char *name)
{
	if (name == NULL || name[0] == '\0')
	{
		pub_log_error("%s, %d, Param error.", __FILE__,__LINE__);
		return SW_ERROR;
	}
	
	if (name[0] == '$' || name[0] == '#')
	{
		vars->remove_var(vars, name);
	}
	
	return SW_OK;
}

int loc_set_xml_attr(sw_loc_vars_t *vars, char *var_name, char *attr_name, char *attr_value)
{
	return vars->set_attr(vars, var_name, attr_name, attr_value);
}

int loc_get_xml_attr(sw_loc_vars_t *vars, char *var_name, char *attr_name, char *attr_value)
{
	return vars->get_attr(vars, var_name, attr_name, attr_value);
}

int loc_set_xml_data(sw_loc_vars_t *vars, char *name, char * value)
{
	if (name == NULL || strlen(name) == 0 || value == NULL)
	{
		return -1;
	}
	
	if (name[0] != '.')
	{
		pub_log_error("[%s][%d] name:[%s] error!", __FILE__, __LINE__, name);
		return -1;
	}
	vars->set_xml_var(vars, name, value, 1);
	
	return 0;
}

int loc_set_zd_data(sw_loc_vars_t *vars, char *name, char * value)
{
	if (name == NULL || strlen(name) == 0 || value == NULL)
	{
		return -1;
	}
	
	switch(name[0])
	{
		case '$':
		case '#':
			vars->set_string(vars, name, value);
			break;
		case '.':
			vars->set_xml_var(vars, name, value, 0);
			break;
		default:
			value = NULL;
			break;
	}
	
	return 0;
}

int loc_get_zd_data(sw_loc_vars_t *vars, char *name, char * value)
{	
	if (name == NULL || strlen(name) == 0)
	{
		return -1;
	}
	
	switch(name[0])
	{
		case '$':
		case '#':
			vars->get_var(vars, name, value);
			break;
		case '.':
			vars->get_xml_var(vars, name, value);
			break;
		default:
			value = NULL;
			break;
	}
	
	pub_str_ziphlspace(value);
	
	return 0;
}

int loc_get_zd_data_space(sw_loc_vars_t *vars, char *name, char * value)
{		
	if (name == NULL || strlen(name) == 0)
	{
		return -1;
	}
	
	switch (name[0])
	{
		case '$':
		case '#':
			vars->get_var(vars, name, value);
			
			break;
		case '.':
			vars->get_xml_var(vars, name, value);
			break;
		default:
			value = NULL;
			break;
	}
	
	return 0;
}

int loc_get_zd_data_len(sw_loc_vars_t *vars, char *name, char * value, int len)
{
	int	totallen = 0;
	char	*tmp = NULL;
	
	if(name==NULL || strlen(name)==0 )
	{
		return -1;
	}
	
	switch(name[0])
	{
		case '$':
		case '#':
			tmp = vars->get_var_addr(vars, name, &totallen);
			memcpy(value, tmp, len);
			break;
		case '.':
			vars->get_xml_var(vars,name,value);
			break;
		default:
			value=NULL;
			break;
	}
	
	return 0;
}

int loc_get_data_len(sw_loc_vars_t *vars, char *name, char * value)
{
	int	len = 0;
	if(name==NULL || strlen(name)==0 )
	{
		return -1;
	}
	
	switch(name[0])
	{
		case '$':
		case '#':
			len = vars->get_var(vars,name,value);
			if (len < 0)
			{
				len = 0;
			}
			break;
		case '.':
			vars->get_xml_var(vars,name,value);
			len = strlen(value);
			break;
		default:
			value=NULL;
			break;
	}
	
	return len;
}


int loc_set_zd_data_len(sw_loc_vars_t *vars, char *name, char * value, int len)
{
	if (name == NULL || strlen(name) == 0 
		|| (name[0] != '#' && name[0] != '$') )
	{
		return SW_ERROR;
	}
	
	return vars->set_var(vars, name, 'b', value, (short)len);
}

int loc_set_zd_int(sw_loc_vars_t *vars, char *sjbm, int int_data)
{
		char	data[20];
		
		pub_mem_memzero(data, sizeof(data));
		sprintf(data, "%d", int_data);
		
		return loc_set_zd_data(vars, sjbm, data);
}

int loc_get_zd_int(sw_loc_vars_t *vars, char *sjbm, int * int_data)
{
	char	data[20];
	
	pub_mem_memzero(data, sizeof(data));
	
	if (sjbm == NULL || int_data == NULL || strlen(sjbm) == 0)
	{
		return SW_ERROR;
	}
	
	switch(sjbm[0])
	{
		case '$':
		case '#':
			vars->get_var(vars, sjbm, data);
			*int_data = atoi(data);
			break;
		case '.':
			vars->get_xml_var(vars, sjbm, data);
			*int_data = atoi(data);
			break;
		default:
			return SW_ERROR;
			break;
	}
	
	return SW_OK;
}

int loc_set_zd_long(sw_loc_vars_t *vars, char *sjbm, long long_data)
{
	char data[200];
		
	pub_mem_memzero(data, sizeof(data));
	sprintf(data, "%ld", long_data);
	
	return loc_set_zd_data(vars, sjbm, data);
}

int loc_get_zd_long(sw_loc_vars_t *vars,char *sjbm,long * long_data)
{
	char data[200];
	pub_mem_memzero(data, sizeof(data));
		
	if(sjbm==NULL || strlen(sjbm)==0 )
	{
		return SW_ERROR;
	}
	
	switch(sjbm[0])
	{
		case '$':
		case '#':
			vars->get_var(vars,sjbm,data);
			*long_data = atol(data);
			break;
		case '.':
			vars->get_xml_var(vars,sjbm,data);
			sscanf( data, "%ld", long_data);
			break;
		default:
			return SW_ERROR;
			break;
	}
	
	return SW_OK;
}

int loc_set_zd_float(sw_loc_vars_t *vars,char *sjbm, float float_data)
{
	char data[200];
		
	pub_mem_memzero(data, sizeof(data));
	sprintf(data, "%.2f", float_data);
		
	return loc_set_zd_data(vars,sjbm,data);
}

int loc_get_zd_float(sw_loc_vars_t *vars,char *sjbm, float * float_data)
{
	char data[200];
	pub_mem_memzero(data, sizeof(data));
		
	if (sjbm == NULL || strlen(sjbm) == 0 || float_data == NULL)
	{
		return SW_ERROR;
	}
	
	switch(sjbm[0])
	{
		case '$':
		case '#':
			vars->get_var(vars,sjbm,data);
			sscanf( data, "%f", float_data);
			break;
		case '.':
			vars->get_xml_var(vars,sjbm,data);
			sscanf( data, "%f", float_data);
			break;
		default:
			return SW_ERROR;
			break;
	}
	
	return SW_OK;
}

int loc_set_zd_double(sw_loc_vars_t *vars, char *sjbm, double double_data)
{
	char data[200]; 
		
	pub_mem_memzero(data, sizeof(data));
	sprintf(data, "%.2lf", double_data);
		
	return loc_set_zd_data(vars, sjbm, data);
}

int loc_get_zd_double(sw_loc_vars_t *vars, char *sjbm, double * double_data)
{
	char data[200];
	
	pub_mem_memzero(data, sizeof(data));
	
	if (sjbm == NULL || strlen(sjbm) == 0 || double_data == NULL)
	{
		return SW_ERROR;
	}
	
	switch(sjbm[0])
	{
		case '$':
		case '#':
			vars->get_var(vars, sjbm, data);
			*double_data = strtod(data, NULL);
			break;
		case '.':
			vars->get_xml_var(vars, sjbm, data);
			*double_data = strtod(data, NULL);
			break;
		default:
			return SW_ERROR;
			break;
	}
	
	return SW_OK;
}

int vars_clear()
{
	return loc_vars_clear(pub_get_global_vars());
}

int extvars_clear()
{
	return loc_extvars_clear(pub_get_global_vars());
}

int var_remove(char * name)
{
	return loc_var_remove(pub_get_global_vars(), name);
}

int set_xml_attr(char *var_name, char *attr_name, char *attr_value)
{
	return loc_set_xml_attr(pub_get_global_vars(), var_name
					, attr_name, attr_value);
}

int get_xml_attr(char *var_name, char *attr_name, char *attr_value)
{
	return loc_get_xml_attr(pub_get_global_vars(), var_name
					, attr_name, attr_value);
}

int set_xml_data(char *name, char * value)
{
	return loc_set_xml_data(pub_get_global_vars(), name, value);
}

int set_zd_data(char *name, char * value)
{
	return loc_set_zd_data(pub_get_global_vars(), name, value);
}

int get_zd_data(char * name, char * value)
{
	return loc_get_zd_data(pub_get_global_vars(), name, value);
}

int get_zd_data_space(char * name, char * value)
{
	return loc_get_zd_data_space(pub_get_global_vars(), name, value);
}

int get_zd_data_len(char * name, char * value, int len)
{
	return loc_get_zd_data_len(pub_get_global_vars(), name, value, len);
}

int set_zd_data_len(char * name, char * value, int len)
{
	return loc_set_zd_data_len(pub_get_global_vars(), name, value, len);
}

int set_zd_int(char *sjbm,int int_data)
{
	return loc_set_zd_int(pub_get_global_vars(), sjbm, int_data);
}

int get_zd_int(char *sjbm, int * int_data)
{
	return loc_get_zd_int(pub_get_global_vars(), sjbm, int_data);
}

int set_zd_long(char *sjbm, long long_data)
{
	return loc_set_zd_long(pub_get_global_vars(), sjbm, long_data);
}

int get_zd_long(char *sjbm,long * long_data)
{
	return loc_get_zd_long(pub_get_global_vars(), sjbm, long_data);
}

int set_zd_float(char *sjbm,float float_data)
{
	return loc_set_zd_float(pub_get_global_vars(), sjbm, float_data);
}

int get_zd_float(char *sjbm,float * float_data)
{
	return loc_get_zd_float(pub_get_global_vars(), sjbm, float_data);
}

int set_zd_double(char *sjbm, double double_data)
{
	return loc_set_zd_double(pub_get_global_vars(), sjbm, double_data);
}

int get_zd_double(char *sjbm, double * double_data)
{
	return loc_get_zd_double(pub_get_global_vars(), sjbm, double_data);
}

int get_data_len(char * name, char * value)
{
	return loc_get_data_len(pub_get_global_vars(), name, value);
}

sw_int32_t get_field_len(char* name)
{
	sw_loc_vars_t *vars = NULL;
	
	if (NULL == name || strlen(name) == 0)
	{
		pub_log_error("%s, %d, Param error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	vars = pub_get_global_vars();

	return (vars->get_field_len(vars, name));
}

int cp_field_zip(char* from, char* to, char* desc)
{
	sw_int32_t from_len = get_field_len(from);

	if (0 > from_len)
	{
		from_len = 0;
	}

	char *from_buf = (char *) malloc(from_len + 1);
	if (NULL == from_buf)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(from_buf, from_len + 1);

	get_zd_data(from, from_buf);
	pub_str_zipspace(from_buf);
	set_zd_data(to, from_buf);

/*	pub_log_info("[%s()][%s][%s][%s][%s]"
			, __func__, from, to, desc, from_buf);*/

	free (from_buf);

	return SW_OK;
}

int cp_field(char* from, char* to, char* desc)
{
	sw_int32_t from_len = get_field_len(from);

	if (0 > from_len)
	{
		from_len = 0;
	}

	char *from_buf = (char *) malloc(from_len + 1);
	if (NULL == from_buf)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(from_buf, from_len + 1);

	get_zd_data_space(from, from_buf);
	set_zd_data(to, from_buf);

	pub_log_info("[%s()][%s][%s][%s][%s]", __func__, from, to, desc, from_buf);
	free(from_buf);

	return SW_OK;
}

int set_variable(char *name,char type,char *value,short length)
{
	if (name == NULL || strlen(name) == 0 || (name[0] != '#' && name[0] != '$') )
	{
		return SW_ERROR;
	}
	
	sw_loc_vars_t *vars = NULL;
	vars = pub_get_global_vars();
	
	return vars->set_var(vars, name, type, value, (short)length);
}

int set_xml_var(char *varname, char *value)
{
	if (varname == NULL || strlen(varname) == 0)
	{
		return SW_ERROR;
	}

	sw_loc_vars_t *vars = NULL;
	vars = pub_get_global_vars();
	
	return vars->set_xml_var(vars, varname, value, 0);
}

int get_xml_var(char *varname, char *value)
{
	if (varname == NULL || strlen(varname) == 0)
	{
		return SW_ERROR;
	}
	
	sw_loc_vars_t *vars = NULL;
	vars = pub_get_global_vars();
	
	return vars->get_xml_var(vars, varname, value);
}

int get_variable(char *name,char *value)
{
	if (name == NULL || strlen(name) == 0 || (name[0] != '#' && name[0] != '$') )
	{
		return SW_ERROR;
	}
	
	sw_loc_vars_t *vars = NULL;
	vars = pub_get_global_vars();
	
	return vars->get_var(vars, name, value);
}

int vars_init()
{
	int	ret = 0;

	ret = pub_vars_alloc(HEAP_VARS);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]Alloc vars error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = pub_vars_create(0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]Create vars error!", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

int vars_destroy()
{
	pub_vars_destory();
	pub_vars_free();
	
	return 0;
}
