#ifndef	__OMSINFO_H
#define	__OMSINFO_H

#include "pub_type.h"
#include "pub_log.h"
#include "pub_vars.h"
#include "trace.h"
#include "seqs.h"

#ifndef AIX
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "statgrab.h"
#endif

#ifdef AIX
#include <utmp.h>
#include <libperfstat.h>
#include <net/if_types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <strings.h>
#include <sys/proc.h>
#include <procinfo.h>
#include <sys/time.h>
#include <sys/utsname.h>

sw_int32_t getprocs64(void* procentry64, sw_int32_t
	, void* fdsinfo64, sw_int32_t, pid_t *, sw_int32_t);


#endif

typedef struct
{
	sw_int64_t	total;
	sw_int64_t	free;
	sw_int64_t	used;
}swap_stats_t;

sw_int_t get_swap_stats(swap_stats_t *swap_sts);

typedef struct 
{
	sw_int64_t	total;
	sw_int64_t	free;
	sw_int64_t	used;
	sw_int64_t	cache;
}mem_stats_t;

sw_int_t get_mem_stats(mem_stats_t *mem_stats);

typedef struct 
{
	sw_int64_t	size;
	sw_int64_t	used;
	sw_int64_t	avail;
	sw_int64_t	total_inodes;
	sw_int64_t	used_inodes;
	sw_int64_t	free_inodes;
	sw_int64_t	avail_inodes;
	sw_int64_t	io_size;
	sw_int64_t	block_size;
	sw_int64_t	total_blocks;
	sw_int64_t	free_blocks;
	sw_int64_t	used_blocks;
	sw_int64_t	avail_blocks;
	sw_char_t	fs_type[128];
	sw_char_t	mnt_point[128];
	sw_char_t	device_name[128];
}fs_stats_t;

fs_stats_t *get_fs_stats(sw_int32_t *num);

typedef struct
{
	sw_int64_t	freq;
	sw_char_t	desc[128];
	sw_char_t	uptime[128];
	sw_int64_t	user;
	sw_int64_t	nice;
	sw_int64_t	kernel;
	sw_int64_t	idle;
	sw_int64_t	iowait;
	sw_int64_t	swap;
	sw_int64_t	total;
	time_t		systime;
}cpu_total_t;

cpu_total_t  *get_cpu_total();

typedef struct
{
	sw_int64_t	user;
	sw_int64_t	nice;
	sw_int64_t	kernel;
	sw_int64_t	idle;
	sw_int64_t	iowait;
	sw_int64_t	swap;
	sw_int64_t	total;
	sw_char_t	desc[128];
}cpu_stat_t;	

cpu_stat_t *get_cpu_stat(sw_int32_t *num);


typedef struct
{
	double		user;
	double		kernel;
	double		idle;
	double		iowait;
	sw_char_t	desc[128];
}cpu_usage_t;

cpu_usage_t *get_cpu_percents(sw_int32_t *num);

typedef struct 
{
	sw_char_t	interface_name[128];
	sw_int64_t	tx;
	sw_int64_t	rx;
	sw_int64_t	ipackets;
	sw_int64_t	opackets;
	sw_int64_t	ierrors;
	sw_int64_t	oerrors;
	sw_int64_t	collisions;
	time_t		systime;
}network_stats_t;

network_stats_t *devel_get_netwrk_stats(sw_int32_t *num);



typedef struct 
{
	sw_char_t	name[128];
	double		sendrate;
	double		acceptrate;
	double		used;
	double		imbytes;
	double		ombytes;
}net_percent_t;

net_percent_t *get_net_percent(sw_int32_t *num);

typedef enum 
{
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_SLEEPING,
	PROCESS_STATE_STOPPED,
	PROCESS_STATE_ZOMBIE,
	PROCESS_STATE_UNKNOWN
}process_state;

typedef struct 
{
	sw_int32_t	nice;
	pid_t		pid;
	pid_t		parent;		/* Parent pid */
	pid_t		pgid;		/* process id of process group leader */
	uid_t		uid;
	uid_t		euid;
	gid_t		gid;
	gid_t		egid;
	sw_uint64_t	proc_size;	/* in bytes */
	sw_uint64_t	proc_resident;	/* in bytes */
	time_t		time_spent;	/* time running in seconds */
	double		cpu_percent;
	sw_char_t	proctitle[128];
	sw_char_t	process_name[128];
	process_state	state;
}process_stats_t;

process_stats_t *get_process_stat(sw_int32_t *num);

#endif
