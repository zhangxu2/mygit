#ifndef __JMQ_H
#define __JMQ_H

#include <jni.h>
#include "lsn_pub.h"

typedef struct
{
	jmethodID mid;
}jMethod;

typedef struct
{
	jobject	obj;
	jclass	cls;
	JavaVM	*jvm;
	JNIEnv	*env;
	jmethodID mid;
	jMethod	send[MQ_NUM];
	jMethod recv[MQ_NUM];
} jmq_t;

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

typedef enum {
	MSG_NONE=1,
	MSG_STRING
}msg_format_enum_t;

struct qmgr_s	g_qmgr;
int	g_usejms;
jmq_t	g_jmq;
#define SW_ALL	3
#define SW_NO_MSG	-2
#define SW_RECV_TIMEOUT	4

int mq_javainit(char *cip, int cport, char *cqmgr, char *cconnchl, int cccsid);
int mq_javaopen(char *queue, char *queue_ext, int index, int flag);
int mq_javasend(char *cmessage, sw_int_t size, u_char *msgid, u_char *corrid, int exptime, int index);
int mq_javarecv(int index, char *cmessage, u_char *msgid, u_char *corrid, int *len);
int mq_javafmt(int flag);
int mq_javadestroy();
int mq_jmsinit(char *cip, int cport, char *cqmgr, char *csendqueue, char *crecvqueue, int flag);
int mq_jmssend(char *cmessage);
int mq_jmsrecv(int timeout, char *cmessage, int *len);
int mq_jmsdestroy();

#endif
