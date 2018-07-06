#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pub_log.h"
#include "lsn_pub.h"
#include "pub_stack.h"

#define MAXSIZE 128
#define MAX_ARG_CNT     10
#define MAX_FUN_CNT     50
#define TYPE_STRING     1
#define TYPE_INT        2
#define TYPE_DOUBLE      3
#define TYPE_OPERATOR   4

#define STR2INT(ptr) ((*((long *)(ptr))))
#define STR2DBL(ptr) ((*((double *)(ptr))))
#define DBLZERO   0.00001

typedef struct
{
        int     argc;
        char    *argv[MAX_ARG_CNT];
}args_t;

struct
{
        char    type;
        char    name[64];
        char    value[1024];
}vars;

extern int	g_fun_cnt;
typedef int (*fun_pt)(args_t *arg);
extern void set_one_fun_addr(int id, char *funname, fun_pt ptr);
extern int compute_exp(sw_loc_vars_t *locvars, char *str, char *value);

