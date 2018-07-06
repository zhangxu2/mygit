#ifndef __SWDB_H__
#define __SWDB_H__

#define pub_db_query(sql) pub_db_mquery(sql, 1)
extern int pub_db_open(void);
extern int pub_db_close(void);
extern int pub_db_commit(void);
extern int pub_db_rollback(void);
extern int pub_db_nquery(const char *sql);
extern int pub_db_mquery(const char *alias, const char *sql, int rows);
extern int pub_db_squery(const char *sql);
extern int pub_db_update_by_rowid(const char *sql, const char *rowid);
extern int pub_db_fetch(void);
extern int pub_db_mfetch(const char *alias);
extern int pub_db_cclose(const char *alias);
extern int pub_db_del_all_conn();
extern char *pub_db_get_data(const char *alias, int rowno, int colno);
extern char *pub_db_get_data_by_name(const char *alias, int rowno, const char *name);
extern char *pub_db_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size);
extern int pub_db_get_type(const char *alias, int colno);
extern int pub_db_set_alias(const char *name, const void *value, int length);
extern const void *pub_db_get_alias(const char *name);
extern int pub_db_delete_alias(const char *name);
extern int pub_db_delete_all_alias(void);

#endif
