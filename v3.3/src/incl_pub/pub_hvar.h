#ifndef _VAR_H
#define _VAR_H
#include "pub_hash.h"
#include "pub_xml.h"
#include "pub_buf.h"

#define MAX_DISTINCT_ITEM 512
#define MAX_VARNAME_LEN 64

typedef struct
{
	char	name[MAX_VARNAME_LEN];
	char	*value;
	char	type;
	size_t	length;
}var_node_t;

typedef struct
{
	hash_t	*comvar;
	sw_xmltree_t	*tree;
}sw_hvar_t;

extern int comvar_set_value(sw_hvar_t *vars, char *name, char *value, size_t size, char type);
extern size_t comvar_get_value(sw_hvar_t *vars, char *name, char *value);
extern char *comvar_get_var_addr(sw_hvar_t *vars, char *name, int *len);

#define var_set_string(vars, name, value) comvar_set_value(vars, name, value, strlen(value), 'a')
#define var_set_bin(vars, name, value, length) comvar_set_value(vars, name, value, length, 'b')

int hvar_init(sw_hvar_t *vars);
int hvar_clear(sw_hvar_t *vars);
int extvar_clear(sw_hvar_t *vars);
int hvar_set_value(sw_hvar_t *vars, char *name, char *value, size_t size, char type);
size_t hvar_get_value(sw_hvar_t *vars, char *name, char *value);
int hvar_remove(sw_hvar_t *vars, char *name);
int hvar_serialize(sw_hvar_t *vars, sw_buf_t *buf);
int hvar_unserialize(sw_hvar_t *vars, char *str);
int hvar_set_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value);
int hvar_get_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value);
int hvar_get_field_len(sw_hvar_t *vars, char *name);
char *hvar_get_var_addr(sw_hvar_t *vars, char *name, int *len);
char *hvar_get_null(sw_hvar_t *vars, char *name, int len);
int extvar_get_value(sw_hvar_t *vars, char* name, char* value);
int extvar_set_value(sw_hvar_t *vars, char *var_name, char *value, int creat);
int extvar_set_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value);
int extvar_get_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value);

#endif
