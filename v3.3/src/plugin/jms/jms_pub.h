#ifndef __SW_JMS_PUB_H__

#include "pub_log.h"

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
};

typedef struct
{
	char	url[64];
	char	jndifactory[128];
	char	connfactory[64];
	char	jsendqueue[64];
	char	jrecvqueue[64];
} jms_info_t;

jms_info_t	g_jmsinfo;
struct qmgr_s	g_qmgr;

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

} jmscontext_t;
jmscontext_t	g_jmscontext;
#define SW_JMS_ALL	3
#define SW_JMS_NO_MSG	-2
#define SW_JMS_RECV_TIMEOUT	4
int jms_jmsinit(char *jndifactory, char *curl, char *cfactory, char *csendqueue, char *crecvqueue, int flag);
int jms_jmssend(char *cmessage);
int jms_jmsrecv(int timeout, char *cmessage, long *len);

#endif

#endif
