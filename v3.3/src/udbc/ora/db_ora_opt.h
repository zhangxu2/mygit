#ifndef __DB_ORA_OPT_H__
#define __DB_ORA_OPT_H__

#include <stdio.h>
#include <stdlib.h>

#include "db_data_type.h"
#include "db_ora_type.h"

int db_ora_open_pool(void *arg, void **param);
int db_ora_get_conn(void *arg, void *param);
int db_ora_put_conn(void *param);
int db_ora_close_pool(void *param);
int db_ora_open(void *arg, void **param);
int db_ora_close(void *param);
int db_ora_commit(void *param);
int db_ora_rollback(void *param);
int db_ora_non_query(const char *stmt, void *param);
int db_ora_single_query(const char *stmt, void *param);
int db_ora_mul_query(const char *alias, const char *stmt, int rows, void *param);
int db_ora_fetch(void *param);
int db_ora_mfetch(const char *alias, void *_cntx);
int db_ora_clo_fetch(const char *alias, void *param);
char *db_ora_get_col_value(const char *alias, int rows, int col, void *param);
int db_ora_get_col_type(const char *alias, int col, void *param);
char* db_ora_get_value_byname(const char *alias, int rows, const char *name, void *param);
char *db_ora_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size, void *param);

#endif /*__DB_ORA_OPT_H__*/

