/***************************************************************************
 *** 程序作者 : SSH                                                       **
 *** 开始日期 : 2008-02-31                                                **
 *** 结束日期 :
 *** 所属模块 :                                                          ***
 *** 程序名称 : tux_7f.c                                                 ***
 *** 程序作用 : 发送/接收文件(东华核心TUX格式)                           ***
 *** 使用注意 :                                                          ***
 ***************************************************************************/
#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include <syslog.h>
#include <fcntl.h>
#include"atmi.h"
#include "pub_vars.h"
#include "pub_log.h"
#include "pub_code.h"

#ifndef INT4
#define INT4 int
#endif

/****TUXEDO传输文件结构定义****/
typedef struct{
	char mesgtype;  /****请求种类:0-查询,1-下载,2-上传****/
	char name[129];
	INT4 filesize;
	INT4 modtime;
	INT4 offset;
	INT4 datasize;
	char flag;      /****写文件标志,0-普通,1-覆盖****/
}T_BFHD;

typedef struct{
	T_BFHD head;
	char data[1];
}TBALFTP;

void balftp_hton(T_BFHD* pBFHD)
{
	pBFHD->filesize = (long)htonl((unsigned long)pBFHD->filesize);
	pBFHD->modtime  = (long)htonl((unsigned long)pBFHD->modtime );
	pBFHD->offset   = (long)htonl((unsigned long)pBFHD->offset  );
	pBFHD->datasize = (long)htonl((unsigned long)pBFHD->datasize);
}
void balftp_ntoh(T_BFHD* pBFHD)
{
	pBFHD->filesize = (long)ntohl((unsigned long)pBFHD->filesize);
	pBFHD->modtime  = (long)ntohl((unsigned long)pBFHD->modtime );
	pBFHD->offset   = (long)ntohl((unsigned long)pBFHD->offset  );
	pBFHD->datasize = (long)ntohl((unsigned long)pBFHD->datasize);
}

extern int readn(int fd, void *vptr, size_t n);

int tux_7f_snd(sw_loc_vars_t *vars)
{	
	int	fd = 0;
	int	iRc = 0;
	int	iFileLen = 0;
	int	iTimes = 0;
	long	iTuxRecvLen = 0;
	char	pcHead[40];
	char	sTmp[256];
	char	sFileName[128];
	char	*pcTuxSendBuf = NULL;
	char	*pcTuxRecvBuf = NULL;
	T_BFHD	*poFileStru;
	struct stat	st;

	memset(sTmp, 0x0, sizeof(sTmp));
	memset(pcHead, 0x0, sizeof(pcHead));
	memset(sFileName, 0x0, sizeof(sFileName));
	
	loc_get_zd_data(vars, "$fileflag", sTmp);
	if (sTmp[0] != '1')
	{
		pub_log_debug("[%s][%d] 报文[%x]未指定文件!", __FILE__, __LINE__, sTmp[0]);
		return 0;
	}
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sFileName, 0x0, sizeof(sFileName));
	loc_get_zd_data(vars, "$filename", sFileName);
	if (sFileName[0] == '\0')
	{
		pub_log_error("[%s][%d] $filename is null!", __FILE__, __LINE__);
		return -1;
	}
	sprintf(sTmp, "%s/dat/%s", getenv("SWWORK"), sFileName);
	memset(&st, 0x0, sizeof(st));
	if (stat(sTmp, &st) < 0)
	{
		pub_log_error("[%s][%d] Stat [%s] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, sTmp, errno, strerror(errno));
		return -1;
	}
	iFileLen = st.st_size;
	pub_log_info("[%s][%d] [%s] FileLen=[%d]", __FILE__, __LINE__, sTmp, iFileLen);
	
	fd = open(sTmp, O_RDONLY);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, sTmp, errno, strerror(errno));
		return -1;
	}
	poFileStru=(T_BFHD *)malloc(iFileLen+2+sizeof(T_BFHD));		
	memset(poFileStru,'\0',sizeof(T_BFHD));
	if (readn(fd, (char *)poFileStru+sizeof(T_BFHD), iFileLen) != iFileLen)
	{
		pub_log_error("[%s][%d] Read error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, iFileLen, errno, strerror(errno));
		free(poFileStru);
		close(fd);
		return -1;
	}
	close(fd);
	((char *)poFileStru)[sizeof(T_BFHD)+iFileLen]='\0';
	pub_log_bin(SW_LOG_DEBUG, (char *)poFileStru + sizeof(T_BFHD), iFileLen,"TUXEDO方式即将发送的文件为:");
	strncpy(poFileStru->name, sFileName, 129);
	pub_code_numtonet(iFileLen,(char *)&poFileStru->filesize,4);
	pub_code_numtonet(iFileLen,(char *)&poFileStru->datasize,4);
	pub_code_numtonet(0,(char *)&poFileStru->offset,4);
	poFileStru->mesgtype='2';
	poFileStru->flag='1';
	pcTuxSendBuf=(char *)tpalloc("CARRAY",NULL,iFileLen+2+sizeof(T_BFHD));	
	memcpy(pcTuxSendBuf,poFileStru,iFileLen+1+sizeof(T_BFHD));
	free(poFileStru);
	pcTuxRecvBuf=(char *)tpalloc("CARRAY",NULL,128);	
	iTimes = 0;
	while (1)
	{
		iRc = tpcall("BALFTP",pcTuxSendBuf,iFileLen+sizeof(T_BFHD),&pcTuxRecvBuf,&iTuxRecvLen,0);
		if (iRc)
		{
			if (tperrno != TPESYSTEM)
			{
				pub_log_bin(SW_LOG_ERROR, pcTuxSendBuf, iFileLen+sizeof(T_BFHD)+1,
					"TUXEDO方式发送文件附件失败[%d][%s]", tperrno, tpstrerror(tperrno));
				tpfree(pcTuxSendBuf);
				tpfree(pcTuxRecvBuf);
				return -1;
			}
			
			iTimes++;
			if (iTimes > 1)
			{
				pub_log_error("[%s][%d] 重连后发送文件仍然失败! errno=[%d]:[%s]",
					__FILE__, __LINE__, tperrno, tpstrerror(tperrno));
				tpfree(pcTuxSendBuf);
				tpfree(pcTuxRecvBuf);
				return -1;
			}

			tpterm();
			if (tpinit((TPINIT *)NULL) == -1)
			{
				pub_log_error("[%s][%d] 发送文件时重连失败! WSNADDR=[%s] errno=[%d]:[%s]",
					__FILE__, __LINE__, getenv("WSNADDR"), tperrno, tpstrerror(tperrno));
				tpfree(pcTuxSendBuf);
				tpfree(pcTuxRecvBuf);
				return -1;
			}
			continue;
		}
		break;
	}
	pub_log_debug("TUXEDO方式发送文件附件成功");
	tpfree(pcTuxSendBuf);
	tpfree(pcTuxRecvBuf);
	return(0);
}

int tux_7f_rcv(sw_loc_vars_t *vars)
{
	int	iRc = 0;
	int	datasize = 0;
	int	iFileLen = 0;
	int	headlen = 0;
	long	iTuxRecvLen = 0;
	int	iTimes = 0;
	FILE	*fp = NULL;
	char	sTmp[256];
	char	sFileName[256];
	char	pcFileName[256];
	char	*pFileBuf = NULL;
	char	*pcTuxSendBuf = NULL;
	char	*pcTuxRecvBuf = NULL;
	TBALFTP	*poFileStru = NULL;

	memset(sTmp, 0x0, sizeof(sTmp));
	memset(pcFileName, 0x0, sizeof(pcFileName));

	loc_set_zd_data(vars, "$fileflag", "0");
	
	loc_get_zd_data(vars, "#reqtype", sTmp);
	pub_log_debug("[%s][%d] 文件标识=[%s]", __FILE__, __LINE__, sTmp);
	
	if (sTmp[0] != '9')
	{
		pub_log_debug("[%s][%d] 结尾字符[%x]未指定文件!", __FILE__, __LINE__, sTmp[0]);
		return 0;
	}

	pcTuxSendBuf=(char *)tpalloc("CARRAY",NULL, MSGBUF_MAXLEN);
	if (pcTuxSendBuf == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return -1;
	}
	
	pcTuxRecvBuf=(char *)tpalloc("CARRAY",NULL,MSGBUF_MAXLEN);
	if (pcTuxRecvBuf == NULL)
	{
		tpfree(pcTuxSendBuf);
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return -1;
	}
	memset(pcTuxSendBuf,'\0',MSGBUF_MAXLEN);
	poFileStru=(TBALFTP*)pcTuxSendBuf;
	poFileStru->head.mesgtype = '1';
	memset(poFileStru->head.name,'\0',sizeof(poFileStru->head.name));
	loc_get_zd_data(vars, "$filename", poFileStru->head.name);
	if (poFileStru->head.name[0] == '\0')
	{
		loc_get_zd_data(vars, "#sys3", poFileStru->head.name);
		loc_get_zd_data(vars, "#sys10", poFileStru->head.name + strlen(poFileStru->head.name));
	}

	datasize = MSGBUF_MAXLEN - sizeof(T_BFHD);
	memset(sFileName, 0x00, sizeof(sFileName));
	strncpy(sFileName, poFileStru->head.name, sizeof(sFileName) - 1);
	pub_str_zipspace(poFileStru->head.name);
	poFileStru->head.filesize = 0;
	poFileStru->head.modtime = 0;
	poFileStru->head.datasize = datasize;
	poFileStru->head.offset = 0;
	poFileStru->head.flag = '0';
	balftp_hton(&(poFileStru->head));
	pub_log_debug("[%s][%d] 请求文件名:[%s]", __FILE__, __LINE__, poFileStru->head.name);
	iTimes = 0;
	pFileBuf = NULL;
	while (1)
	{
		iTuxRecvLen = 0;
		memset(pcTuxRecvBuf, '\0', MSGBUF_MAXLEN);
		while (1)
		{
			iRc = tpcall("BALFTP",(char *)poFileStru, sizeof(T_BFHD), &pcTuxRecvBuf, &iTuxRecvLen, 0);
		if (iRc)
		{
			if (tperrno != TPESYSTEM)
			{
			    pub_log_error("[%s][%d] 接收文件错误[%d][%s]!",__FILE__, __LINE__, tperrno, tpstrerror(tperrno));
				tpfree(pcTuxSendBuf);
				tpfree(pcTuxRecvBuf);
					if (pFileBuf != NULL)
					{
						free((char*)pFileBuf);
						pFileBuf = NULL;
					}
				return -1;
			}
			
			iTimes++;
			if (iTimes > 1)
			{
					pub_log_error("[%s][%d] 重连后接收文件仍然失败! errno=[%d]:[%s]",__FILE__, __LINE__, tperrno, tpstrerror(tperrno));
					tpfree(pcTuxSendBuf);
					tpfree(pcTuxRecvBuf);
					if (pFileBuf != NULL)
					{
						free((char*)pFileBuf);
						pFileBuf = NULL;
					}
				return -1;
			}

			tpterm();
			if (tpinit((TPINIT *)NULL) == -1)
			{
				pub_log_error("[%s][%d] 接收文件时重连失败! WSNADDR=[%s] errno=[%d]:[%s]",
					__FILE__, __LINE__, getenv("WSNADDR"), tperrno, tpstrerror(tperrno));
				tpfree(pcTuxSendBuf);
				tpfree(pcTuxRecvBuf);
					if (pFileBuf != NULL)
					{
						free((char*)pFileBuf);
						pFileBuf = NULL;
					}
				return -1;
			}
			continue;
		}
		break;
	}
		if(iTuxRecvLen < sizeof(T_BFHD))
		{
			pub_log_error("%s:%d ,[%s]Recv file  failed!",__FILE__,__LINE__,"BALFTP");
			tpfree(pcTuxSendBuf);
			tpfree(pcTuxRecvBuf);	
			if (pFileBuf != NULL)
			{
				free((char*)pFileBuf);
				pFileBuf = NULL;
			}
			return -1;
			
		}
		memset(poFileStru, 0x00, sizeof(T_BFHD) + 1);
		memcpy(poFileStru, pcTuxRecvBuf, sizeof(T_BFHD));
		balftp_ntoh(&(poFileStru->head));

		if( pFileBuf == NULL)
		{
			pub_log_debug("[%s][%d] filesize=[%d]", __FILE__, __LINE__, poFileStru->head.filesize);
			pFileBuf = (char*)malloc(poFileStru->head.filesize + 1);
			if(pFileBuf == NULL )
			{
				pub_log_error("[%s][%d] malloc error.", __FILE__, __LINE__);
				tpfree(pcTuxSendBuf);
				tpfree(pcTuxRecvBuf);	
				return -1;
			}
		}

		pub_log_debug("[%s][%d] before offset=[%d], datasize=[%d]", __FILE__, __LINE__, poFileStru->head.offset, poFileStru->head.datasize);

		memcpy(pFileBuf + poFileStru->head.offset, pcTuxRecvBuf + sizeof(T_BFHD), poFileStru->head.datasize);
		if( poFileStru->head.offset + poFileStru->head.datasize >= poFileStru->head.filesize)
		{
			break;
		}

		poFileStru->head.offset = poFileStru->head.offset + poFileStru->head.datasize;
		if(poFileStru->head.filesize - poFileStru->head.offset  > MSGBUF_MAXLEN) 
		{
			poFileStru->head.datasize = datasize;
		}
		else
		{
			poFileStru->head.datasize = poFileStru->head.filesize - poFileStru->head.offset;
		}

		pub_log_debug("[%s][%d] after offset=[%d],datasize=[%d]", __FILE__, __LINE__, poFileStru->head.offset,poFileStru->head.datasize);
	}

	iFileLen = poFileStru->head.filesize;
	pFileBuf[iFileLen]='\0';
	tpfree(pcTuxSendBuf);
	tpfree(pcTuxRecvBuf);	
	pub_log_bin(SW_LOG_DEBUG, pFileBuf, iFileLen, "recv file success,[%s][%d] len=[%d]", __FILE__, __LINE__, iFileLen);

	memset(pcFileName,0x00,sizeof(pcFileName));
	snprintf(pcFileName, sizeof(pcFileName) - 1, "%s/dat/%s",getenv("SWWORK"), sFileName);
	pub_str_zipspace(pcFileName);
	pub_log_info("[%s][%d]recv filename=[%s]!",__FILE__,__LINE__,pcFileName);
	fp=fopen(pcFileName,"wb");
	if(fp==NULL)
	{
		pub_log_error("[%s][%d]保存文件[%s]错误[%d].",__FILE__,__LINE__,pcFileName, errno);
		if( pFileBuf != NULL)
		{
			free(pFileBuf);
			pFileBuf = NULL;
		}

		return -1;
	}

	fwrite(pFileBuf, iFileLen, 1, fp);
	fclose(fp);

	if (pFileBuf != NULL)
	{
		free(pFileBuf);
		pFileBuf = NULL;	
	}

	loc_set_zd_data(vars, "$fileflag", "1");
	loc_set_zd_data(vars, "$filename", sFileName);
	pub_log_debug("[%s][%d] TUXEDO接收文件[%s]完成!", __FILE__, __LINE__, sFileName);
	
	return 0;
}
int tux_7f(sw_loc_vars_t *vars, int iFlag, int iSockFd, void *param)
{	/***1-接收 0-发送***/
	char sTmp[256];
	memset(sTmp,'\0',sizeof(sTmp));
	pub_log_debug("%s,%d,文件发送/接收标志[%d]!",__FILE__,__LINE__, iFlag);
	loc_get_zd_data(vars, "$fileflag",sTmp);
	pub_log_debug("%s,%d,文件标志[%s]!",__FILE__,__LINE__,sTmp);
	memset(sTmp,'\0',sizeof(sTmp));
	loc_get_zd_data(vars, "$filename",sTmp);
	pub_log_debug("%s,%d,文件名称[%s]!",__FILE__,__LINE__,sTmp);
	memset(sTmp,'\0',sizeof(sTmp));
	loc_get_zd_data(vars, "#tx_code",sTmp);
	pub_log_debug("%s,%d,交易代码[%s]",__FILE__,__LINE__,sTmp);
	if(iFlag==0){
		return(tux_7f_snd(vars));
	}else{
		return(tux_7f_rcv(vars));
	}
}
