#ifndef  __MSG_TRANS_H_
#define __MSG_TRANS_H_

#include "pub_msq.h"
#include "pub_fifo.h"
#include "pub_log.h"
#include "pub_type.h"
#include "pub_cfg.h"

#define SW_FD_MAX 2048

struct sw_cmd_s
{
	sw_int64_t	trace_no;		/* BP trace NO	*/        
	long	mtype;			/* mtype value	*/
	sw_int32_t	timeout;		/* the message timeout	*/	
	sw_int32_t	start_line;		/* flow def start line	*/  
	sw_int32_t	dst_type;		/* destination svr mode type*/
	sw_int32_t	ori_type;		/* ori svr mode type*/
	sw_int32_t	msg_type;		/* msg type ,SW_MSG_REQ ,SW_MSG_REQ*/
	sw_int32_t	task_flg;		/* task flag ,SW_STORE SW_FORGET*/
	sw_int32_t	type;			/* the flag of def grammar */	
	sw_int32_t	level;			/* tx level */
	sw_int32_t  status;

	sw_char_t	ori_def[NAME_LEN];	/* ori def name */
	sw_char_t	lsn_name[NAME_LEN];	/* lsn name */
	sw_char_t	def_name[NAME_LEN];	/* flow def name */
	sw_char_t	dst_prdt[NAME_LEN];	/* destination product name */
	sw_char_t	dst_svr[NAME_LEN];	/* destination svr name	*/
	sw_char_t	dst_svc[NAME_LEN];	/* destination svc or process name */

	sw_char_t	ori_prdt[NAME_LEN];	/* origin product name */
	sw_char_t	ori_svr[NAME_LEN];	/* origin svr name	*/
	sw_char_t	ori_svc[NAME_LEN];	/* origin svc or process name */
	
	sw_char_t	sys_date[DATE_LEN];	/* msg date */        
	sw_char_t	udp_name[NAME_LEN];	/* udp Protocol origin name*/                     
#if defined(SOLARIS) || defined(HPUX)
}__attribute__ ((packed));
#else
};
#endif
typedef struct sw_cmd_s sw_cmd_t;

SW_PUBLIC int msg_load_fd_by_mqid(int mqid);
SW_PUBLIC sw_int_t msg_trans_close_all();
SW_PUBLIC sw_int_t cmd_print(sw_cmd_t *stcmd);
SW_PUBLIC int msg_trans_create(sw_global_path_t *path,key_t key, int nSize,int*mqid);
SW_PUBLIC int msg_trans_open(sw_global_path_t *path,int mqid);
SW_PUBLIC int msg_trans_rcv(int iFd,char *pBuf,long *mtype,int *piLen);
SW_PUBLIC int msg_trans_send(int iFd,char *psBuf,long mtype,int iLen);
SW_PUBLIC int msg_trans_rm(sw_global_path_t *path, int fd);

#endif


