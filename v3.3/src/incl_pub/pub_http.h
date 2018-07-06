#ifndef __PUB_HTTP_H__
#define __PUB_HTTP_H__

#include "pub_vars.h"
#include "pub_buf.h"


int http_append_value_encode(sw_buf_t *buffer, char *key, char *value, int len);
int http_append_and_encode(sw_buf_t *buffer, sw_loc_vars_t *vars, char *vname, char *key);
int http_append_and_obj(sw_buf_t *buffer, sw_loc_vars_t *vars, char *vname, char *key);
int http_append_value(sw_buf_t *buffer, char *key, char *value, int len);

#endif
