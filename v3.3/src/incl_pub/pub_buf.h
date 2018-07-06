/*********************************************************************
 *** 版 本 号: v2.*
 *** 程序作者: liteng
 *** 生成日期: 2011-11-21
 *** 所属模块: 公共模块
 *** 程序名称: pub_buf.h
 *** 程序作用: 定义了平台基础函数库
 *** 函数列表:
 ***		pub_buf_init	平台报文缓冲区初始化
 ***		pub_buf_update	平台报文缓冲区大小更新
 ***		pub_buf_clear	平台报文缓冲区清空
 *** 使用注意:
 *** 修改记录:
 *** 	修改作者:
 *** 	修改时间:
 *** 	修改内容:
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
* 结构体名:  sw_buf_t
* 功能描述:  平台全局报文缓冲区结构体
* 修改作者:  liteng
* 修改日期： 2011年03月16日
*************************************************/
typedef struct
{
	sw_int_t	len; 		/*平台变量缓冲区大小*/
	sw_int_t	size;		/*平台变量缓冲区实际大小*/
	sw_int_t	index;		/*用于存放缓存链路id*/
	sw_char_t	*data; 		/*平台变量缓冲区地址*/
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

