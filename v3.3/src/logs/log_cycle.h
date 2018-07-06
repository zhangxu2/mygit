#ifndef __SW_LOG_CYCLE_H__
#define __SW_LOG_CYCLE_H__

#include "log_pub.h"
#include "log_comm.h"

sw_int_t log_cycle_init(sw_log_cycle_t *cycle, char *name, sw_int32_t module_type, char *err_log, char *dbg_log);
sw_int_t log_cycle_run(sw_log_cycle_t *cycle);
sw_int_t log_cycle_destroy(sw_log_cycle_t *cycle);

#endif
