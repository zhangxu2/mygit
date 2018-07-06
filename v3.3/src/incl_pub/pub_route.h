#ifndef __PUB_ROUTE_H__
#define __PUB_ROUTE_H__

#include "pub_type.h"
#include "msg_trans.h"
#include "pub_vars.h"
#include "procs.h"
#include "pub_cfg.h"

#define MAX_TASK_CNT (20)

#define SW_ROUTE_VALUDE 	"$$REOUTE"
#define SW_ROUTE_LEN		(1024)
#define SW_MAX_CMD		(20)

struct sw_task_hd_s
{
	sw_int32_t task_max;			/* max task cnt*/
	sw_int32_t task_curr;			/* current task id*/
	sw_cmd_t  *cmd;			/* cmd arry head*/
#if defined(SOLARIS) || defined(HPUX)
}__attribute__ ((packed));
#else
};
#endif
typedef struct sw_task_hd_s  sw_task_hd_t; 

SW_PUBLIC sw_int_t route_add_cmd(sw_task_hd_t *head, sw_cmd_t *cmd);
SW_PUBLIC sw_int_t route_del_cur_cmd(sw_task_hd_t *head);
SW_PUBLIC sw_int_t route_recov_cmd(sw_task_hd_t *head,sw_cmd_t *cmd);
SW_PUBLIC sw_int_t route_snd_dst(sw_loc_vars_t *var,sw_global_path_t *path,sw_cmd_t *cmd);
SW_PUBLIC sw_int_t route_cmd_tpye_change(sw_cmd_t *cmd);
SW_PUBLIC sw_int_t route_free(sw_loc_vars_t *var);


#endif
