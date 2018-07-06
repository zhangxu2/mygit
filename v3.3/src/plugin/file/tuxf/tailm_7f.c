/***************************************************************************
 *** 程序作者 : SSH                                                       **
 *** 开始日期 : 2008-02-31                                                **
 *** 结束日期 :
 *** 所属模块 :                                                          ***
 *** 程序名称 : tailm_7f.c                                                ***
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

#define MAX_FILE_CNT	10

extern int readn(int fd, void *vptr, size_t n);

int tailm_7f_snd(sw_loc_vars_t *vars, int sockid, char *fname)
{	
	int	fd = 0;
	int	ret = 0;
	int	filelen = 0;
	char	*ptr = NULL;
	char	tmp[256];
	char	head[40];
	char	filename[256];
	struct stat	st;

	memset(tmp, 0x0, sizeof(tmp));
	memset(head, 0x0, sizeof(head));
	memset(filename, 0x0, sizeof(filename));

	strncpy(filename, fname, 30);
	pub_log_info("[%s][%d] filename=[%s]", __FILE__, __LINE__, filename);
	sprintf(tmp, "%s/dat/%s", getenv("SWWORK"), filename);
	pub_log_info("[%s][%d] tmp=[%s]", __FILE__, __LINE__, tmp);
	memset(&st, 0x0, sizeof(st));
	if (stat(tmp, &st) < 0)
	{
		pub_log_error("[%s][%d] Stat [%s] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, tmp, errno, strerror(errno));
		return -1;
	}
	filelen = st.st_size;
	pub_log_info("[%s][%d] [%s] size=[%d]", __FILE__, __LINE__, tmp, filelen);

	ptr = (char *)calloc(1, filelen + 2 + 38);		
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, filelen + 40, errno, strerror(errno));
		return -1;
	}
	
	fd = open(tmp, O_RDONLY);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, tmp, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	
	if (readn(fd, ptr + 39, filelen) != filelen)
	{
		pub_log_error("[%s][%d] Read error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, filelen, errno, strerror(errno));
		free(ptr);
		close(fd);
		return -1;
	}
	close(fd);
	
	ptr[38 + filelen + 1] = 0x00;
	memset(head, 0x0, sizeof(head));
	memset(head, ' ', 30);
	strncpy(head, filename, strlen(filename));
	sprintf(head + 30, "%08d", filelen);
	ptr[0] = 0x7f;
	memcpy(ptr + 1, head, 38);	
	pub_log_bin(SW_LOG_INFO, ptr, 38 + filelen + 1, "[%s][%d]发送文件内容:长度=[%d]",
		__FILE__, __LINE__, filelen + 38 + 1);
	ret = lsn_pub_send(sockid, ptr, 38 + filelen + 1);
	if (ret)
	{
		pub_log_error("[%s][%d] 发送文件错误! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		free(ptr);
		return 1;
	}
	free(ptr);

	return 0;
}

int tailm_snd(sw_loc_vars_t *vars, int sockid)
{
	int	i = 0;
	int	ret = 0;
	int	fcnt = 0;
	char	*ptr = NULL;
	char	*ptr1 = NULL;
	char	fname[128];
	char	allfile[1024];
	char	mfile[MAX_FILE_CNT][128];
	unsigned char	tmp[8];
	const char	*sep = "+";
	
	memset(fname, 0x0, sizeof(fname));
	memset(allfile, 0x0, sizeof(allfile));
	memset(mfile, 0x0, sizeof(mfile));
	memset(tmp, 0x0, sizeof(tmp));

	loc_get_zd_data(vars, "$fileflag", (char *)tmp);
	if (tmp[0] != '1')
	{
		pub_log_info("[%s][%d] 报文[%x]未指定文件! tmp=[%s]",
			__FILE__, __LINE__, tmp[0], tmp);
		ret = lsn_pub_send(sockid, "\xff", 1);
		if (ret)
		{
			pub_log_error("[%s][%d] 发送文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		return 0;
	}
	
	loc_get_zd_data(vars, "$filename", allfile);
	if (strlen(allfile) == 0)
	{
		pub_log_info("[%s][%d] 文件名不能为空!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] allfile===[%s]", __FILE__, __LINE__, allfile);
	
	i = 0;
	ptr = allfile;
	while (*ptr != '\0')
	{
		ptr1 = strstr(ptr, sep);
		if (ptr1 == NULL)
		{
			strcpy(mfile[i], ptr);
			i++;
			break;
		}
		strncpy(mfile[i], ptr, ptr1 - ptr);
		ptr = ptr1 + strlen(sep);
		i++;
		if (i >= MAX_FILE_CNT)
		{
			pub_log_info("[%s][%d] [%d]超过最大文件个数[%d]", __FILE__, __LINE__, i, MAX_FILE_CNT);
			return -1;
		}
	}
	fcnt = i;
	pub_log_info("[%s][%d] 共[%d]个文件需要发送!", __FILE__, __LINE__, fcnt);
	for (i = 0; i < fcnt; i++)
	{
		pub_log_info("[%s][%d] 第[%d]个文件名为[%s]", __FILE__, __LINE__, i + 1, mfile[i]);
	}
	
	for (i = 0; i < fcnt; i++)
	{
		memset(fname, 0x0, sizeof(fname));
		strncpy(fname, mfile[i], sizeof(fname) - 1);
		
		ret = tailm_7f_snd(vars, sockid, fname);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 发送第[%d]个文件[%s]失败!", __FILE__, __LINE__, i, fname);
			return -1;
		}
		pub_log_info("[%s][%d] 第[%d]个文件[%s]发送完毕!", __FILE__, __LINE__, i + 1, fname);
	}
	ret = lsn_pub_send(sockid, "\xff", 1);
	if (ret)
	{
		pub_log_error("[%s][%d] 发送文件错误! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	pub_log_info("[%s][%d] 共[%d]个文件发送完毕!", __FILE__, __LINE__, fcnt);
	
	return 0;
}

int tailm_7f_rcv(sw_loc_vars_t *vars, int sockid, char *fname)
{
	int	ret = 0;
	int	filelen = 0;
	FILE	*fp = NULL;
	char	*ptr = NULL;
	char	tmp[256];
	char	head[40];
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(head, 0x0, sizeof(head));

	ret = lsn_pub_recv_len(sockid, head, 38);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	filelen = atoi(head + 30);
	head[30] = '\0';
	pub_str_trim(head);

	ptr = (char *)calloc(1, filelen + 1);
	ret = lsn_pub_recv_len(sockid, ptr, filelen);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	pub_log_bin(SW_LOG_INFO, ptr, filelen, "[%s][%d] Gethere,文件内容", __FILE__, __LINE__);
	ptr[filelen] = '\0';
	strcpy(fname, head);
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%s/dat/%s", getenv("SWWORK"), head);
	fp = fopen(tmp, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] 保存文件[%s]错误! errno=[%d]:[%s]",
			__FILE__, __LINE__, tmp, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	fwrite(ptr, filelen, 1, fp);
	fclose(fp);
	free(ptr);
	pub_log_info("[%s][%d] 接收文件[%s]完成!", __FILE__, __LINE__, head);
	
	return 0;
}

int tailm_rcv(sw_loc_vars_t *vars, int sockid)
{
	int	ret = 0;
	int	fcnt = 0;
	char	fname[128];
	char	allfile[1024];
	unsigned char	tmp[8];
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(fname, 0x0, sizeof(fname));
	memset(allfile, 0x0, sizeof(allfile));
	
	ret = lsn_pub_recv_len(sockid, (char *)tmp, 1);
	if (ret == SW_ERROR)
	{
		pub_log_info("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	loc_set_zd_data(vars, "#endflag", (char *)tmp);
	if (tmp[0] != 0x7f)
	{
		pub_log_info("[%s][%d] 结尾字符[%x]未指定文件!", __FILE__, __LINE__, tmp[0]);
		return 0;
	}
	
	fcnt = 0;
	memset(fname, 0x0, sizeof(fname));
	memset(allfile, 0x0, sizeof(allfile));
	while (1)
	{
		memset(fname, 0x0, sizeof(fname));
		ret = tailm_7f_rcv(vars, sockid, fname);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 接收文件失败!", __FILE__, __LINE__);
			return -1;
		}
		strcat(allfile, fname);
		fcnt++;
		pub_log_info("[%s][%d] 接收第[%d]个文件[%s]完成!", __FILE__, __LINE__, fcnt, fname);

		memset(tmp, 0x0, sizeof(tmp));
		ret = lsn_pub_recv_len(sockid, (char *)tmp, 1);
		if (ret == SW_ERROR)
		{
			pub_log_info("[%s][%d] 接收文件错误! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		pub_log_info("[%s][%d] tmp=[%s][%x]", __FILE__, __LINE__, tmp, tmp[0]);
		if (tmp[0] != 0x7f)
		{
			break;
		}
		strcat(allfile, "+");
	}
	loc_set_zd_data(vars, "$fileflag", "1");
	loc_set_zd_data(vars, "$filename", allfile);
	pub_log_info("[%s][%d] 接收文件[%s]完成,共接收[%d]个文件!", __FILE__, __LINE__, allfile, fcnt);
	
	return 0;
}

int tailm_7f(sw_loc_vars_t *vars, int flag, int sockid, void *param)
{	
	char	tmp[256];
	
	/***0-发送 1-接收***/
	pub_log_info("[%s][%d] 文件发送/接收标志=[%d] socket=[%d]",
		__FILE__, __LINE__,  flag, sockid);
	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(vars, "$fileflag", tmp);
	pub_log_info("[%s][%d] 文件标志=[%s]", __FILE__, __LINE__, tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(vars, "$filename", tmp);
	pub_log_info("[%s][%d] 文件名称=[%s]", __FILE__, __LINE__, tmp);
	
	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(vars, "#tx_code", tmp);
	pub_log_info("[%s][%d] 交易代码=[%s]", __FILE__, __LINE__, tmp);
	
	if (flag == 0)
	{
		return tailm_snd(vars, sockid);
	}
	else
	{
		return tailm_rcv(vars, sockid);
	}
}

