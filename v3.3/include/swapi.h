#ifndef __PUB_LOC_API_H__
#define __PUB_LOC_API_H__
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "swxml.h"
#include "swlog.h"
#include "swver2.h"
#include "swdb.h"

extern int vars_clear();
extern int extvars_clear();
extern int var_remove(char * name);
extern int set_xml_attr(char *var_name, char *attr_name, char *attr_value);
extern int get_xml_attr(char *var_name, char *attr_name, char *attr_value);
extern int set_zd_data_len(char * name, char * value, int len);
extern int set_zd_int(char *sjbm,int int_data);
extern int get_zd_int(char *sjbm, int * int_data);
extern int set_zd_long(char *sjbm, long long_data);
extern int get_zd_long(char *sjbm,long * long_data);
extern int set_zd_float(char *sjbm,float float_data);
extern int get_zd_float(char *sjbm,float * float_data);
extern int set_zd_double(char *sjbm, double double_data);
extern int get_zd_double(char *sjbm, double * double_data);
extern int set_zd_data(char *name, char * value);
extern int get_zd_data(char * name, char * value);
extern int get_zd_data_space(char * name, char * value);
extern int get_zd_data_len(char * name, char * value, int len);
extern int get_field_len(char *name);
extern int cp_field_zip(char *from, char *to, char *desc);
extern int cp_field(char *from, char *to, char *desc);

#endif /* __PUB_LOC_API_H__ */
