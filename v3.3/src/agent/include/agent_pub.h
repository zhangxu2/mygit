#ifndef __AGENT_PUB_H__
#define __AGENT_PUB_H__

#include "procs.h"
#include "pub_type.h"
#include "pub_xml.h"
#include "pub_log.h"
#include "pub_cfg.h"
#include "pub_file.h"

#define SW_CHILD	(1)
#define SW_PARENT	(2)
#define NO_DEVEL_PROC	(-2)
#define DEVEL_MAX_PROC_NUM	(128)
#define LOCK_FILE_NAME	".lock_file"
#define PROCS_FILE_NAME	".DEVEL_PROCS"

typedef struct
{	
	int	pid;
	int	stat;
	int	type;
	int	proc_idx;
	int	restartcnt;
	char	binary[64];
	char	name[64];
}sw_agt_proc_t;

typedef struct
{
	int	count;
	int    lock_id;
	sw_agt_proc_t	proc[DEVEL_MAX_PROC_NUM];
}sw_agt_procs_t;

typedef struct sw_agt_cfg_s
{
	int	use;
	int	type;
	int	port;
	int	proccnt;
	int	scantime;
	int	lsn_fd;
	int	warncnt;
	int	warnper;
	int	durtime;
	int	loglevel;
	int	proc_index;
	size_t	logsize;
	char	ip[32];
	char	map[64];
	char	init[256];
	char	deal[256];
	char	name[64];
	char	data[32];
	char	tranfunc[64];
	char	xmlname[64];
}sw_agt_cfg_t;


int agt_check_pid(int pid);
int agt_get_procs(sw_agt_procs_t* agt_procs);
int agt_set_procs(sw_agt_procs_t* agt_procs);
int agt_clean_procs();
int agt_get_by_name(char *name, sw_agt_proc_t *proc);
int agt_proc_register(sw_agt_proc_t *proc);
int agt_scan_node(char *name);
int agt_start_node(char* binary, char *name);
int agt_proc_print();
int agt_set_proc_by_name(char *name, sw_agt_proc_t *proc);
int agt_procs_exist_clean();

int strchr_cnt(char *str, char ch);
int strstr_cnt(char *str, char *needle);
int agt_cfg_init(sw_agt_cfg_t *cfg, char *name);
int agt_set_log_cfg();
int agt_get_scantime();
int agt_set_logpath(const char *errlog, const char *dbglog);
int agt_get_db_cfg(sw_dbcfg_t *dbcfg);

#endif

