#if !defined(__DB_INTERN_PROT_H__)
#define __DB_INTERN_PROT_H__

#include "db_data_type.h"
#include "pub_cfg.h"


extern int db_get_cfg_info(sw_dbcfg_t *db, db_data_src_t *source);
extern int db_set_cb_func(db_type_t type, int mode, db_cycle_t *obj);
extern int db_reset_cb_ptr(db_cycle_t *obj);
extern int db_cfg_release(db_cycle_t *obj);

#endif /*__DB_INTERN_PROT_H__*/
