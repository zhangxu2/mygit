#include "pub_log.h"
#include "pub_type.h"
#include "pub_vars.h"
#include "pub_buf.h"

int savepkg(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	/*** ע�⣺sw_buf_t�Ǹ��ṹ��,�ýṹ���е���Ҫ��Ա��data��len ***
	typedef struct
	{
             int len;
             char *data;
	     .......
	}sw_buf_t;
        
	data��Ա��ŵ��Ǳ��ģ�len�Ǳ��ĳ��� ***/
	
	if (flag == SW_DEC)
	{
		pub_log_info("[%s][%d] ��ʼ�Խ��յı��Ľ��б���...", __FILE__, __LINE__);
		/*** �ڴ˽�buf�ṹ���dataд���ļ�����,data�ĳ���Ϊlen ***/
	}
	else
	{
		pub_log_info("[%s][%d] ��ʼ�Է��͵ı��Ľ��б���...", __FILE__, __LINE__);
		/*** �ڴ˽�buf�ṹ���dataд���ļ�����,data�ĳ���Ϊlen ***/
	}
	
	return 0;
}

