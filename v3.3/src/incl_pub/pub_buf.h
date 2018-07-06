/*********************************************************************
 *** �� �� ��: v2.*
 *** ��������: liteng
 *** ��������: 2011-11-21
 *** ����ģ��: ����ģ��
 *** ��������: pub_buf.h
 *** ��������: ������ƽ̨����������
 *** �����б�:
 ***		pub_buf_init	ƽ̨���Ļ�������ʼ��
 ***		pub_buf_update	ƽ̨���Ļ�������С����
 ***		pub_buf_clear	ƽ̨���Ļ��������
 *** ʹ��ע��:
 *** �޸ļ�¼:
 *** 	�޸�����:
 *** 	�޸�ʱ��:
 *** 	�޸�����:
 ********************************************************************/
#ifndef  __PUB_BUF_H__
#define  __PUB_BUF_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

#include "pub_type.h"
#include "pub_log.h"

#define BUF_MAXLEN 	128*1024
#define SW_BUFF_INCR_SIZE  2048L


/*************************************************
* �ṹ����:  sw_buf_t
* ��������:  ƽ̨ȫ�ֱ��Ļ������ṹ��
* �޸�����:  liteng
* �޸����ڣ� 2011��03��16��
*************************************************/
typedef struct
{
	sw_int_t	len; 		/*ƽ̨������������С*/
	sw_int_t	size;		/*ƽ̨����������ʵ�ʴ�С*/
	sw_int_t	index;		/*���ڴ�Ż�����·id*/
	sw_char_t	*data; 		/*ƽ̨������������ַ*/
}sw_buf_t;

extern int pub_buf_init(sw_buf_t *buf);
extern int pub_buf_update(sw_buf_t *buf, sw_int_t size);
extern sw_int_t pub_buf_chksize(sw_buf_t *buf, size_t size);
extern int pub_buf_clear(sw_buf_t *buf);

extern sw_buf_t *buf_new();
extern sw_buf_t *buf_new_string(char *data, int dlen);
extern int buf_refresh(sw_buf_t *buf);
extern int buf_append(sw_buf_t *buf, char *data, int dlen);
extern int buf_format_append(sw_buf_t *buf, char *fmt, ...);
extern int buf_update_string(sw_buf_t *buf, char *data, int dlen);
extern void buf_release(sw_buf_t *buf);
extern char *buf_string(sw_buf_t *buf);
extern int buf_length(sw_buf_t *buf);
extern int buf_checksize(sw_buf_t *buf, int len);

#endif

