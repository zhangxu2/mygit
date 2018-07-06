/***************************************************************************
 *** 程序作者 : SSH                                                       **
 *** 开始日期 : 2008-02-31                                                **
 *** 结束日期 :
 *** 所属模块 :                                                          ***
 *** 程序名称 : tail_7f.c                                                ***
 *** 程序作用 : 发送/接收文件(东华核心TCP格式)                           ***
 *** 使用注意 :                                                          ***
 ***************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include "pub_log.h"
#include "pub_vars.h"
#include "lsn_pub.h"

int g_file_socket;

int readn(int fd, void *vptr, size_t n)
{
	size_t	nleft = n;
	ssize_t	nread = 0;
	char	*ptr = vptr;

	while (nleft > 0)
	{
		nread = read(fd, ptr, nleft);
		if (nread <= 0)
		{
			if (errno == EINTR)
			{
				nread = 0;
			}
			else
			{
				return -1;
			}
		}
		ptr += nread;
		nleft -= nread;
	}

	return n - nleft;
}

int tail_7f_snd(sw_loc_vars_t *vars)
{	
	int	fd = 0;
	int	iRc = 0;
	int	iFileLen = 0;
	char	*ptr = NULL;
	char	sTmp[256];
	char	pcHead[40];
	char	sFileName[256];
	struct stat	st;

	memset(sTmp, 0x0, sizeof(sTmp));
	memset(pcHead, 0x0, sizeof(pcHead));
	memset(sFileName, 0x0, sizeof(sFileName));

	loc_get_zd_data(vars, "$fileflag", sTmp);
	if (sTmp[0] != '1')
	{
		pub_log_debug("[%s][%d] 报文[%x]未指定文件! sTmp=[%s]",
			__FILE__, __LINE__, sTmp[0], sTmp);
		iRc = lsn_pub_send(g_file_socket, "\xff", 1);
		if (iRc)
		{
			pub_log_error("[%s][%d] 发送文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		return 0;
	}

	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sFileName, 0x0, sizeof(sFileName));
	loc_get_zd_data(vars, "$filename", sFileName);
	pub_log_info("[%s][%d] sFileName=[%s]", __FILE__, __LINE__, sFileName);
	if (sFileName[0] == '\0')
	{
		pub_log_error("[%s][%d] $filename is null!", __FILE__, __LINE__);
		return -1;
	}
	sprintf(sTmp, "%s/dat/%s", getenv("SWWORK"), sFileName);
	pub_log_info("[%s][%d] sTmp=[%s]", __FILE__, __LINE__, sTmp);
	memset(&st, 0x0, sizeof(st));
	if (stat(sTmp, &st) < 0)
	{
		pub_log_error("[%s][%d] Stat [%s] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, sTmp, errno, strerror(errno));
		return -1;
	}
	iFileLen = st.st_size;
	pub_log_info("[%s][%d] [%s] size=[%d]", __FILE__, __LINE__, sTmp, iFileLen);

	ptr = (char *)calloc(1, iFileLen + 2 + 39);		
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, iFileLen + 2 + 39, errno, strerror(errno));
		return -1;
	}
	
	fd = open(sTmp, O_RDONLY);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, sTmp, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	if (readn(fd, ptr + 39, iFileLen) != iFileLen)
	{
		pub_log_error("[%s][%d] Read error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, iFileLen, errno, strerror(errno));
		free(ptr);
		close(fd);
		return -1;
	}
	close(fd);

	ptr[39 + iFileLen] = 0xff;
	ptr[39 + iFileLen + 1] = 0x00;
	strncpy(pcHead, sFileName, 30);
	sprintf(pcHead + 30, "%08d", iFileLen);
	ptr[0] = 0x7f;
	memcpy(ptr + 1, pcHead, 38);	
/*	pub_log_bin(SW_LOG_DEBUG, ptr, 39 + iFileLen + 1, "[%s][%d] 即将发送的文件内容:", __FILE__, __LINE__);*/
	iRc = lsn_pub_send(g_file_socket, ptr, 39 + iFileLen + 1);
	if (iRc)
	{
		pub_log_error("[%s][%d] 发送文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		free(ptr);
		return 1;
	}
	free(ptr);
	return 0;
}

int tail_7f_rcv(sw_loc_vars_t *vars)
{
	int	i = 0;
	int	iRc = 0;
	int	iFileLen = 0;
	FILE	*fp = NULL;
	char	*ptr = NULL;
	char	sTmp[256];
	char	pcHead[40];
	unsigned char	sEndFlag[4];

	memset(sTmp, 0x0, sizeof(sTmp));
	memset(pcHead, 0x0, sizeof(pcHead));
	memset(sEndFlag, 0x0, sizeof(sEndFlag));

	loc_set_zd_data(vars, "$fileflag", "0");
	iRc = lsn_pub_recv_len(g_file_socket, sEndFlag, 1);
	if (iRc == SW_ERROR)
	{
		pub_log_error("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	loc_set_zd_data(vars, "#endflag", sEndFlag);
	pub_log_debug("[%s][%d] ENDFLAG=[%x]", __FILE__, __LINE__, sEndFlag[0]);
	if (sEndFlag[0] != 0x7f)
	{
		pub_log_debug("[%s][%d] 结尾字符[%x]未指定文件!", __FILE__, __LINE__, sEndFlag[0]);
		return 0;
	}

	iRc = lsn_pub_recv_len(g_file_socket, pcHead, 38);
	if (iRc == SW_ERROR)
	{
		pub_log_error("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	pub_log_info("[%s][%d] pcHead=[%s]", __FILE__, __LINE__, pcHead);
	iFileLen = atoi(pcHead + 30);
	pcHead[30] = '\0';
	pub_str_trim(pcHead);

	pub_log_info("[%s][%d] iFileLen=[%d]", __FILE__, __LINE__, iFileLen);
	ptr = (char *)calloc(1, iFileLen + 1);
	iRc = lsn_pub_recv_len(g_file_socket, ptr, iFileLen + 1);
	if (iRc == SW_ERROR)
	{
		pub_log_error("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	/*pub_log_bin(SW_LOG_DEBUG, ptr, iFileLen, "[%s][%d] 接收的文件内容:", __FILE__, __LINE__);*/
	ptr[iFileLen] = '\0';
	memset(sTmp, 0x0, sizeof(sTmp));
	sprintf(sTmp, "%s/dat/%s", getenv("SWWORK"), pcHead);
	fp = fopen(sTmp, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] 保存文件[%s]错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, sTmp, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	fwrite(ptr, iFileLen, 1, fp);
	fclose(fp);
	free(ptr);
	loc_set_zd_data(vars, "$fileflag", "1");
	loc_set_zd_data(vars, "$filename", pcHead);
	pub_log_info("[%s][%d] 接收文件[%s]完成!", __FILE__, __LINE__, pcHead);

	return 0;
}

int tail_7f(sw_loc_vars_t *vars, int iFlag, int iSockFd, void *param)
{	
	char	sTmp[256];

	/***0-发送 1-接收***/
	pub_log_info("[%s][%d] 文件发送/接收标志=[%d] socket=[%d]",
			__FILE__, __LINE__,  iFlag, iSockFd);
	memset(sTmp, 0x0, sizeof(sTmp));
	loc_get_zd_data(vars, "$fileflag", sTmp);
	pub_log_info("[%s][%d] 文件标志=[%s]", __FILE__, __LINE__, sTmp);

	memset(sTmp, 0x0, sizeof(sTmp));
	loc_get_zd_data(vars, "$filename", sTmp);
	pub_log_info("[%s][%d] 文件名称=[%s]", __FILE__, __LINE__, sTmp);

	memset(sTmp, 0x0, sizeof(sTmp));
	loc_get_zd_data(vars, "#tx_code", sTmp);
	pub_log_info("[%s][%d] 交易代码=[%s]", __FILE__, __LINE__, sTmp);

	g_file_socket = iSockFd;

	if (iFlag == 0)
	{
		return tail_7f_snd(vars);
	}
	else
	{
		return tail_7f_rcv(vars);
	}
}
