#ifndef __PUB_MSQ_H__
#define __PUB_MSQ_H__

#include "pub_type.h"
#include "pub_log.h"
#include <sys/msg.h>

#define QMSG_MAX_LEN 	128*1024
#define IPC_QU_THRESHOLD   80
#define MSG_QU_FILE     "msgqufile_dmp"

typedef struct
{
	long mtype;
	char data[QMSG_MAX_LEN+1];
}sw_qmsg_t;

sw_int_t pub_msq_check_size(int msqid, int len);
sw_int_t pub_msq_open(key_t key);
sw_int_t pub_msq_creat(key_t key, int size);
sw_int_t pub_msq_clear(int msqid, long mtype, const char *file);
sw_int_t pub_msq_rm(int mqsid);
sw_int_t pub_msq_get(int mqsid, char *buf, long *mtype, int *len);
sw_int_t pub_msq_put(int mqsid, const char *buf, int len, int mtype);

#endif
