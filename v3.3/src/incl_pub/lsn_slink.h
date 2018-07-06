#ifndef __LSN_SLINK__
#define __LSN_SLINK__

#define MAX_CACHE_CNT 10
#define	MQ_SQL_NAME	"mq_linkinfo"
#define MQ_EXIST_SQL	"select 1 from mq_linkinfo"
#define MQ_INDEX_SQL	"create unique index linkindex on mq_linkinfo(keyinfo)"
#define MQ_SQL	"create table mq_linkinfo( \
		l_mtype integer not null, \
		sendtype integer, \
		l_type integer, \
		l_timeout integer, \
		notsend	integer, \
		asktype	char(2), \
		keyinfo	char(128) not null,\
		infoset	char(1024),\
		pkgtype	char(2), \
		synflag char(2), \
		infoflag char(2), \
		saveflag char(2), \
		clearflag char(2), \
		resflag	char(2), \
		qname char(64), \
		msgid char(32), \
		corrid	char(32), \
		l_traceno number(12) not null,\
		l_sys_date char(16) not null,\
		l_lsn_name char(64), \
		machine char(64) not null,\
		addr	char(32) not null,\
		starttime number(12), \
		c_traceno number(12) not null,\
		c_mtype integer not null,\
		c_timeout integer, \
		start_line integer, \
		dst_type integer, \
		ori_type integer, \
		msg_type integer,\
		task_flg integer, \
		c_type	integer, \
		c_level	integer, \
		ori_def char(32),\
		c_lsn_name char(32),\
		def_name char(32),\
		dst_prdt char(32),\
		dst_svr char(32),\
		dst_svc	char(32),\
		ori_prdt char(32),\
		ori_svr	char(32),\
		ori_svc char(32),\
		c_sys_date char(16) not null, \
		udp_name char(32))"
struct
{
	sw_int32_t	conn;
	sw_int64_t	start_time;
}g_mq_db_conn;

typedef struct
{
	char conn_name[NAME_LEN];
}mq_dbconn_cache_t;

sw_int_t	g_mq_dbconn_curr;
sw_int_t	g_mq_dbconn_cnt;
sw_int64_t	g_mq_exptime;
sw_int_t	g_mq_use_db;
mq_dbconn_cache_t	g_mq_dbconn_t[MAX_CACHE_CNT];

sw_int_t mq_db_init(sw_lsn_cycle_t *cycle);
sw_int_t slink_deal_send_timeout_la(sw_lsn_cycle_t *cycle);
sw_int_t slink_save_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link, sw_cmd_t *cmd);
sw_int_t slink_delete_linkinfo(sw_lsn_cycle_t *cycle, sw_link_t *link);
sw_int_t slink_load_linkinfo_by_key(sw_lsn_cycle_t *cycle, sw_link_t *link);
#endif
