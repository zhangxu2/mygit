#ifndef __LOG_PUB_H__
#define __LOG_PUB_H__

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cycle.h"
#include "pub_usocket.h"
#include "select_event.h"
#include "pub_shm_ex.h"
#include "pub_type.h"

typedef struct
{
	sw_char_t 	name[64];
	sw_int32_t 	process_index;
} sw_log_config_t;

typedef struct
{
	sw_cycle_t base;
	sw_fd_t  cmd_fd;
	sw_fd_t  udp_fd;
	sw_fd_t  lsn_fd;
	sw_fd_t	 accept_fd;
	sw_fd_set_t *log_fds;
	sw_log_config_t log_conf;
	sw_shm_t	shm;
} sw_log_cycle_t;

typedef struct
{
	sw_int32_t pid;
	sw_char_t  proc_name[64];
} sw_log_proc_t;

SW_PUBLIC sw_int_t log_handle_control_cmd(sw_log_cycle_t *cycle);
SW_PUBLIC sw_int_t log_father_register(sw_log_cycle_t *cycle, sw_int32_t status);
SW_PUBLIC sw_int_t log_create_child(sw_log_cycle_t *cycle);
SW_PUBLIC sw_int_t log_handle_timeout(sw_log_cycle_t *cycle);

#endif
