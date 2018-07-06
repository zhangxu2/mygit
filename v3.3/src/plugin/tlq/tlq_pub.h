#ifndef __SW_TLQ_PUB_H__

#include "pub_log.h"
#include "tlq_api.h"
#include "tlq_error.h"

struct qmq_s
{
        char    qname[64];
};
struct qmgr_s
{
	int	index;
	int	use;
	int	qcnt;
	char	qmgr[64];
	struct qmq_s	qm[MQ_NUM];
	TLQ_ID  gid;
	TLQ_QCUHDL      qcuid;
};

typedef struct
{
	char	url[64];
	char	factory[64];
	char	jsendqueue[64];
	char	jrecvqueue[64];
} tlq_jms_info_t;

int	g_usejms;
tlq_jms_info_t	g_jmsinfo;
struct qmgr_s	g_qmgr;

int tlq_conn(TLQ_ID *gid);
int tlq_open(TLQ_ID *gid, TLQ_QCUHDL *qcuid, char *qcu_name);
int tlq_putmsg(TLQ_ID *gid, TLQ_QCUHDL *qcuid, char *send_name, char *ptr, int len, u_char *msgid, u_char *corrid);
int tlq_getmsg(TLQ_ID *gid, TLQ_QCUHDL *qcuid, char *recv_name, char **msgtext, long *len, u_char *msgid, u_char *corrid, int wait);
int tlq_close(TLQ_ID *gid, TLQ_QCUHDL *qcuid);
int tlq_disconn(TLQ_ID *gid);

#ifdef __SW_USE_JMS__

#include <jni.h>
typedef struct
{
	jobject	obj;
	jclass	cls;
	JavaVM	*jvm;
	JNIEnv	*env;
	jmethodID	send_mid;
	jmethodID	recv_mid;

} tlqjms_t;
tlqjms_t	g_tlqjms;
#define SW_JMS_ALL	3
#define SW_JMS_NO_MSG	-2
#define SW_JMS_RECV_TIMEOUT	4
int tlq_jmsinit(char *curl, char *cfactory, char *csendqueue, char *crecvqueue, int flag);
int tlq_jmssend(char *cmessage);
int tlq_jmsrecv(int timeout, char *cmessage, long *len);

#endif

#endif
