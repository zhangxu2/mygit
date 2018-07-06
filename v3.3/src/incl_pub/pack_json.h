#ifndef __PACK_JSON_H__
#define __PACK_JSON_H__

#include "pub_vars.h"
#include "pub_buf.h"

int json_in(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag);
sw_int_t json_out(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, char *xmlname, sw_int32_t cache_flag);

#endif
