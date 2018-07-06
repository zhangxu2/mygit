#include "pub_log.h"
#include "pub_type.h"
#include "pub_vars.h"
#include "pub_buf.h"

int savepkg(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	/*** 注意：sw_buf_t是个结构体,该结构体中的主要成员是data跟len ***
	typedef struct
	{
             int len;
             char *data;
	     .......
	}sw_buf_t;
        
	data成员存放的是报文，len是报文长度 ***/
	
	if (flag == SW_DEC)
	{
		pub_log_info("[%s][%d] 开始对接收的报文进行保存...", __FILE__, __LINE__);
		/*** 在此将buf结构体的data写入文件即可,data的长度为len ***/
	}
	else
	{
		pub_log_info("[%s][%d] 开始对发送的报文进行保存...", __FILE__, __LINE__);
		/*** 在此将buf结构体的data写入文件即可,data的长度为len ***/
	}
	
	return 0;
}

