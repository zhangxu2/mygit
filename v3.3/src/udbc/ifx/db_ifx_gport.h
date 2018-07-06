#if !defined(__DB_IFX_GPORT_H__)
#define __DB_IFX_GPORT_H__

#include "db_data_type.h"
#include "db_ifx_type.h"

extern int db_ifx_open(void *arg, void **cntx);
extern int db_ifx_close(void *cntx);

extern int db_ifx_commit(void *cntx);
extern int db_ifx_rollback(void *cntx);

extern int db_ifx_nquery(const char *sql, void *cntx);
extern int db_ifx_mquery(const char *alias, const char *sql, int req_rows, void *cntx);
extern int db_ifx_squery(const char *sql, void *cntx);

extern int db_ifx_fetch(void *cntx);
extern int db_ifx_mfetch(const char *alias, void *cntx);
extern int db_ifx_cclose(const char *alias, void *cntx);

extern char *db_ifx_get_data_by_idx(const char *alias, int row, int col, void *cntx);
extern char *db_ifx_get_data_by_name(const char *alias, int row, const char *name, void *cntx);
extern char *db_ifx_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size, void *cntx);
extern int db_ifx_get_data_type(const char *alias, int col, void *cntx);
extern int db_ifx_conn_detect(void *_cntx);
#if defined(__DB_CONNECT_POOL__)
extern int db_ifx_pcreat(void *source, void **cntx);
extern int db_ifx_pclose(void **cntx);
#endif /*__DB_CONNECT_POOL__*/

#endif /*__DB_IFX_GPROT_H__*/
