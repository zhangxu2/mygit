#if !defined(__DB_ODBC_GPROT_H__)
#define __DB_ODBC_GPROT_H__

#include "db_data_type.h"

extern int db_odbc_open(void *arg, void **_cntx);
extern int db_odbc_close(void *_cntx);

extern int db_odbc_commit(void *_cntx);
extern int db_odbc_rollback(void *_cntx);

extern int db_odbc_nquery(const char *sql, void *_cntx);
extern int db_odbc_mquery(const char *alias, const char *sql, int req_rows, void *_cntx);
extern int db_odbc_squery(const char *sql, void *_cntx);

extern int db_odbc_fetch(void *_cntx);
extern int db_odbc_mfetch(const char *alias, void *_cntx);
extern int db_odbc_cclose(const char *alias, void *_cntx);

extern char *db_odbc_get_data_by_idx(const char *alias, int rowno, int colno, void *_cntx);
extern char *db_odbc_get_data_by_name(const char *alias, int rowno, const char *name, void *_cntx);
extern char *db_odbc_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size, void *_cntx);
extern int db_odbc_get_data_type(const char *alias, int col, void *_cntx);

#if defined(__DB_CONNECT_POOL__)
extern int db_odbc_pcreat(void *source, void **_cntx);
extern int db_odbc_pclose(void **_cntx);
#endif /*__DB_CONNECT_POOL__*/

#endif /*__DB_ODBC_GPROT_H__*/
