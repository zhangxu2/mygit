#ifndef __MQ_PUB_H
#define __MQ_PUB_H

#include "pub_log.h"
#include "lsn_pub.h"
#include "cmqc.h"

#define LARGE_HEAD "$$<LARGE>$$"
#define LARGE_HEAD_LEN strlen(LARGE_HEAD)

struct qmq_s
{
        char    qname[64];
	MQHOBJ	h_obj;
};

struct qmgr_s
{
	int	index;
	int	use;
	int	qcnt;
	char	qmgr[64];
	struct qmq_s	qm[MQ_NUM];
	MQHCONN	h_conn;
};

typedef enum {
	MSG_NONE=1,
	MSG_STRING
}msg_format_enum_t;

struct qmgr_s	g_qmgr;

int mq_conn(MQHCONN *h_conn, char *qmgr_name);
int mqm_open(MQHCONN h_conn, MQHOBJ *h_obj, char *qname, int flag);
int mq_putmsg(MQHCONN h_conn, MQHOBJ h_obj, char *send_buf, int length, MQBYTE24 msgid, MQBYTE24 corrid, long exp_time);
int mq_getmsg(MQHCONN h_conn, MQHOBJ h_obj, char *recv_buf, MQLONG *length, MQBYTE24 msgid, MQBYTE24 corrid, long wait_time);
int mq_freeobj(MQHCONN h_conn, MQHOBJ *h_obj);
int mq_freeconn(MQHCONN *h_conn);
int mq_commit(MQHCONN h_conn);

#endif
