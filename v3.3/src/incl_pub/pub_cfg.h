#ifndef __PUB_CFG_H__
#define __PUB_CFG_H__

#include "pub_type.h"
#include "pub_file.h"
#include "pub_log.h"
#include "pub_key.h"
#include "pub_xml.h"
#include <assert.h>
#include "pub_autoconfig.h"
#include "pub_ares.h"

#define CFG_SHM_FLG 0
#define RUN_SHM_FLG 1
#define VAR_SHM_FLG 2
#define COM_SEM_FLG 3

#define SAFD_NOUSE   0
#define SAFD_USE     1
#define SW_CFG_PATH 	"/cfg/"
#define SW_SYSCFG_FILE "swconfig.xml"
#define SW_KEY_FILE  "/"
#define SW_SYSLOG_PATH "/log/syslog/"
#define SW_JOB_FILE  "job.xml"
#define SW_PRDT_FILE "products.xml"
#define SW_CHANNEL_FILE "channels.xml"
#define SW_DBCFG_FILE "dbconfig.xml"
#define SW_MSG_FILE "/tmp/"
#define SW_INVALID_IPC_ID -1

#define BLOCKAT(shmaddr,offset) ((char *)shmaddr + offset)
#define OFFSET(shmaddr,block) ((size_t)(((char*)block) - (char*)shmaddr))

/*need to add more info*/
typedef enum 
{
	DB_IDTF_LOCAL,
	DB_IDTF_USER
}DB_IDTF;

typedef enum 
{
	LOG_FILE,
	LOG_SHM,
	LOG_TOOLS,
	LOG_PKG
}LOG_MODE;


typedef enum 
{
	SEQ_DB,
	SEQ_SHM
}SEQ_MODE;

typedef enum 
{
	SEQ_UNIQ,
	SEQ_CYC
}SEQ_CYCLE;


typedef struct
{
	sw_int32_t lglvl;
	sw_int32_t lgmode;
	sw_int32_t  lgfile_size;
	sw_char_t lgpath[PATH_LEN];
	sw_char_t lgname_fmt[FORMAT_LEN];
	sw_char_t lginfo_fmt[FORMAT_LEN];
}sw_logcfg_t;

typedef struct
{
	sw_int32_t sqmode;
	sw_int32_t sqcycle;
	sw_char_t sqid[SID_LEN];
	sw_char_t sqname[NAME_LEN];
	sw_char_t sqstart[SQN_LEN];
	sw_char_t sqstep[SQN_LEN];
	sw_char_t sqmax[SQN_LEN];
	sw_char_t sqprefix[SQN_LEN];
}sw_seqcfg_t;

typedef struct
{
	sw_char_t dbid[SID_LEN];
	sw_char_t dbtype[NAME_LEN];

	sw_char_t dbname[NAME_LEN];
	sw_char_t dbsid[SID_LEN];

	sw_char_t dbuser[USER_LEN];
	sw_char_t dbpasswd[PASSWD_LEN];

	sw_char_t dbmoule[NAME_LEN];
	sw_int32_t dbidtf;		/*the identify mode of database*/
	sw_int32_t exptime;
	sw_int32_t mode;
}sw_dbcfg_t;

typedef struct
{
	sw_char_t workpath[PATH_LEN];
	sw_char_t cfgpath[PATH_LEN];
	sw_char_t datapath[PATH_LEN];
	sw_char_t key_file[PATH_LEN];
	sw_char_t job_file[PATH_LEN];
	sw_char_t syscfg_file[PATH_LEN];
	sw_char_t chl_file[PATH_LEN];
	sw_char_t prdt_file[PATH_LEN];
	sw_char_t dbcfg_file[PATH_LEN];
}sw_global_path_t;

typedef struct sw_syscfg_s sw_syscfg_t;
struct sw_syscfg_s
{
	sw_char_t name[NAME_LEN];
	sw_char_t sid[SID_LEN];
	sw_int32_t  lsn_max;	
	sw_int32_t  svr_max;
	sw_int32_t  prdt_max;
	sw_int32_t  processe_max;	
	sw_int32_t  session_max;
	sw_int32_t  seq_max;
	sw_int32_t  job_max;
	sw_int32_t  db_max;
	sw_int32_t  scantime;	
	sw_int32_t  semsize;
	sw_int32_t  runshmsize;
	sw_int32_t  share_pool_size;

	sw_int32_t run_shmid;
	sw_int32_t vars_shmid;
};

typedef struct
{
	sw_char_t *shmaddr;
	sw_int32_t src_offset;
	sw_int32_t sys_offset;
	sw_int32_t slg_offset;
	sw_int32_t alg_offset;
	sw_int32_t safd_offset;
	sw_int32_t db_use;
	sw_int32_t seq_use;
	sw_int32_t lsn_use;
	sw_int32_t prdt_use;
	sw_int32_t db_offset;
	sw_int32_t seq_offset;
	sw_int32_t chl_offset;
	sw_int32_t prdt_offset;	
	sw_int32_t alog_offset;	
	sw_int32_t ares_offset;	
}sw_cfgshm_hder_t;

typedef struct sw_chl_cfg_s sw_chl_cfg_t;
struct sw_chl_cfg_s
{
	sw_char_t lsnname[NAME_LEN];
	sw_int16_t status;
	sw_int16_t prdt_cnt;
	sw_int32_t shmid;
	sw_char_t prdtname[SW_LSN_PRDT_MAX][NAME_LEN];
};

typedef struct sw_prdt_cfg_s sw_prdt_cfg_t;
struct sw_prdt_cfg_s
{
	sw_char_t prdtname[NAME_LEN];
	sw_int16_t status;              /* 产品状态: 0-关闭 1-开启 */
	sw_int16_t lsn_cnt;
	sw_int32_t shmid;
	sw_char_t lsnname[SW_PRDT_LSN_MAX][NAME_LEN];
};


struct sw_safd_cfg_s
{
	sw_int16_t status;
	sw_char_t dbcfg[NAME_LEN];
};

typedef struct sw_safd_cfg_s sw_safd_cfg_t;


sw_int_t cfg_get_sys(const sw_cfgshm_hder_t *addr,sw_syscfg_t *cfg);
sw_int_t cfg_get_slog(sw_cfgshm_hder_t *addr,sw_logcfg_t*log);
sw_int_t cfg_get_alog(sw_cfgshm_hder_t *addr,sw_logcfg_t*log);
sw_int_t cfg_get_saf(sw_cfgshm_hder_t*addr,sw_safd_cfg_t *safcfg);

sw_int_t cfg_set_path(sw_global_path_t *path);
sw_int_t cfg_size_calc(const sw_syscfg_t *cfg);
sw_int_t cfg_set_offset(sw_cfgshm_hder_t *addr,sw_syscfg_t *cfg);
sw_int_t cfg_init(sw_cfgshm_hder_t *cfgaddr,sw_global_path_t  *path);
sw_int_t cfg_get_prdt_shmid(sw_char_t * prdt_name,sw_cfgshm_hder_t *addr);
sw_int_t cfg_set_prdt_shmid(sw_int32_t shmid,sw_char_t * prdt_name,sw_cfgshm_hder_t * addr);
sw_int_t cfg_get_chl_shmid(sw_char_t * chl_name,sw_cfgshm_hder_t *addr);
sw_int_t cfg_set_chl_shmid(sw_int32_t shmid,sw_char_t * chl_name,sw_cfgshm_hder_t * addr);
sw_int_t cfg_read_syscfg(const sw_global_path_t *path, sw_syscfg_t *syscfg);
sw_int_t cfg_load_key(const sw_global_path_t *path, key_t *key);
int cfg_get_db_conn(sw_cfgshm_hder_t *shmaddr, sw_char_t *name, sw_dbcfg_t *db);

const sw_prdt_cfg_t *cfg_get_prdt_cfg(const sw_cfgshm_hder_t *addr);
const sw_chl_cfg_t *cfg_get_chnl_cfg(const sw_cfgshm_hder_t *shmcfg);
sw_int_t cfg_get_alog_cfg(sw_cfgshm_hder_t*addr, alog_cfg_t *alog);
sw_int_t cfg_get_ares_cfg(sw_cfgshm_hder_t*addr, ares_cfg_t *ares);

const sw_prdt_cfg_t *cfg_search_prdt_cfg(const sw_cfgshm_hder_t *shmcfg, const char *prdt_name);
int cfg_add_prdt(sw_cfgshm_hder_t *cfghder, sw_char_t *prdt);
int pub_cfg_get_eswid(char *eswid);
int pub_cfg_get_workmode(char *workmode);
int pub_cfg_get_logmode(char *logmode);
sw_xmltree_t *cfg_read_xml(sw_char_t *xmlname);
ssize_t pub_cfg_parse_size(char *line);

#endif

