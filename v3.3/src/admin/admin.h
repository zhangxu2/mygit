#ifndef __ADMIN_H__
#define __ADMIN_H__

#include "cycle.h"
#include "pub_vars.h"
#include "pub_xml.h"
#include "pub_type.h"
#include "adm_opt.h"
#include "uni_svr.h"
#include "uni_trace.h"

#define ADM_CMD_LINE_MAX_LEN    (1024)
#define ADM_MOD_DFIS_STR	"DFIS-BP"
#define ADM_MOD_SWCFG_STR   "DFIS-CFG"

/*swadmin running context*/
typedef struct sw_adm_cycle_s
{
	sw_cycle_t	base;		/* Base class object,This object must be the first member of child class */
	sw_adm_opt_t *opt;		/* argc argv*/
	sw_uni_svr_t *uni_svr;		/* eswitch service management uniform interface context */
	uni_trace_t *trace;		/* eswitch trace management uniform interface context */
	sw_fd_t	cmdfd;			/* socket used for send/receive message to/from other model */
	sw_char_t sub_mode[32];	/* swadmin's current sub-mode, for example: swman, swtrace, swcfg */
}sw_adm_cycle_t;

#define LINK_ERR	-1
#define	SHM_MISS	-2

#endif


