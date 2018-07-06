#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include <syslog.h>
#include <fcntl.h>
#include "teapi.h"
#include "pub_vars.h"
#include "pub_log.h"

#define ONCE_FILE_BLOCK  2048
int g_tptime = 60;

#ifndef INT4
#define INT4 int
#endif

/****TONGSC传输文件结构定义****/
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

int MD5_Init()
{
	return 0;
}

int MD5_Update ()
{
	return 0;
}

int MD5_Final()
{
	return 0;
}

int balftp_fileinfo(char *filename, TBALFTP* pBFR)
{
	struct stat  tFileStat;
	if (stat(filename, &tFileStat) < 0)
	{
		return -1;
	}
	
	if ((tFileStat.st_mode & S_IFMT) != S_IFREG)
	{
		return -1;
	}
	
	pBFR->head.filesize  = tFileStat.st_size;
	pBFR->head.modtime   = tFileStat.st_mtime;
	pBFR->head.offset   = 0; 
	pBFR->head.datasize = tFileStat.st_size; 
	return 0;
}

void balftp_ntoh(TBALFTP* pBFHD)
{
	pBFHD->head.filesize = (long)ntohl((unsigned long)pBFHD->head.filesize);
	pBFHD->head.modtime  = (long)ntohl((unsigned long)pBFHD->head.modtime );
	pBFHD->head.offset   = (long)ntohl((unsigned long)pBFHD->head.offset  );
	pBFHD->head.datasize = (long)ntohl((unsigned long)pBFHD->head.datasize);
}

void balftp_hton(TBALFTP* pBFHD)
{
	pBFHD->head.filesize = (long)htonl((unsigned long)pBFHD->head.filesize);
	pBFHD->head.modtime  = (long)htonl((unsigned long)pBFHD->head.modtime );
	pBFHD->head.offset   = (long)htonl((unsigned long)pBFHD->head.offset  );
	pBFHD->head.datasize = (long)htonl((unsigned long)pBFHD->head.datasize);
}

int tea_7f(sw_loc_vars_t *vars, int flag, int socket, TE_ID id)
{	
	pub_log_info("[%s][%d]文件发送/接收标志[%d]", __FILE__, __LINE__, flag);
	pub_log_info("[%s][%d]socket=[%d]id==[%d]",__FILE__,__LINE__, socket, id);
	
	int ret = 0;
	
	if(flag == 0)
	{
		ret = tea_7f_snd(id, vars);
	}
	else
	{
		ret = tea_7f_rcv(id, vars);
	}
	
	return ret ;
}

int tea_7f_snd(TE_ID id, sw_loc_vars_t *vars)
{	
	int i;
	int iRc;
	int iOffSet;
	int iFileSize;
	int FileFlag;
	int flag;
	int iTuxRecvLen;
	int fd; 
	int iAddiLen;
	int port;

	TE_ID id_send;
	char ip[128];
	char nodename[128];
	char sTmp[256];	
	char pcFileName[256];
	char pcServName[256];
	char pcMulu[30];
	char *pcTuxRecv=NULL;	
	TBALFTP *poFtp;
	UPNODE_INFO upinfo;
		
	i = -1;
	iRc  = -1;
	iOffSet = 0;
	iFileSize = -1;
	flag = 0;
	iTuxRecvLen = 0;
	fd = -1;
	iAddiLen = -1;
	pcTuxRecv = NULL;
	
	memset(sTmp, 0x00, sizeof(sTmp));
	memset(pcFileName, 0x00, sizeof(pcFileName));
	memset(pcServName, 0x00, sizeof(pcServName));
	memset(pcMulu, 0x00, sizeof(pcMulu));
	
	memset(sTmp, '\0', sizeof(sTmp));
	loc_get_zd_data(vars, "$fileflag", sTmp);
	if (sTmp[0] != '1')
	{
		pub_log_info("[%s][%d]报文[%x]未指定文件!", __FILE__,__LINE__, sTmp[0]);
		return 0;
	}
	
	memset(nodename, 0x0, sizeof(nodename));
	memset(ip, 0x0, sizeof(ip));
	port = 0;
	
	loc_get_zd_data(vars, "$nodename", nodename);
	loc_get_zd_data(vars, "$ip", ip);
	loc_get_zd_int(vars, "$port", &port);
	
	memset(&upinfo,0,sizeof(UPNODE_INFO));
	strcpy(upinfo.UName[0], nodename);
	upinfo.UPort[0] = port; 
	strcpy(upinfo.UIPAddr[0], ip);
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(pcFileName, 0x0, sizeof(pcFileName));
	loc_get_zd_data(vars, "$filename", pcFileName);
	if (pcFileName[0] == '\0')
	{
		pub_log_error("[%s][%d] $filename is null!", __FILE__, __LINE__);
		return -1;
	}
	
	pub_log_info( "[%s][%d] pcFileName=[%s]", __FILE__, __LINE__, pcFileName);
	loc_get_zd_data(vars, "#MULU", pcMulu);
	pub_log_info( "[%s][%d] pcMulu=[%s]", __FILE__, __LINE__,pcMulu);
	if(strlen(pcMulu))
	{
		sprintf(sTmp, "%s/dat/%s/%s", getenv("SWWORK"), pcMulu, pcFileName);
	}
	else
	{
		sprintf(sTmp, "%s/dat/%s", getenv("SWWORK"), pcFileName);
	}
	
	pub_log_info( "[%s][%d] 文件sTmp=[%s]", __FILE__, __LINE__, sTmp);
	fd = open(sTmp, O_RDONLY);
	if (fd < 0)
	{
		pub_log_error("%s,%d,打开文件[%s]错误[%d]fd=[%d]!\n",__FILE__,__LINE__, sTmp, errno, fd);
		return -1;
	}
	
	iFileSize = lseek(fd, 0, SEEK_END);
	if (iFileSize < 0)
	{
		pub_log_error("%s,%d,lseek错误[%d]!\n",__FILE__,__LINE__,errno);
		return 1;
	}
	
	iOffSet = 0;
	iAddiLen = 0;
	poFtp = NULL;
	
	if (iFileSize < 0)
	{
		pub_log_info("%s,%d,传输的文件为空文件,iFileSize=[%d]",__FILE__,__LINE__,iFileSize);
		close(fd);
		return 0;
	}

	id_send = NULL;
	id_send = TE_tpinit(0, 0, (char *)&upinfo);
	if (id_send == NULL)
	{
		pub_log_error("[%s][%d] TE_tpinit error!", __FILE__, __LINE__);
		return -1;
	}
	
	while(1)
	{
		if (iFileSize <= iOffSet)
		{
			break;
		}
		/**判断文件是否能一次传送过去**/
		if (iFileSize - iOffSet < ONCE_FILE_BLOCK)
		{
			iAddiLen = iFileSize - iOffSet;
			if (poFtp != NULL)
			{
				free((char *)poFtp);
				poFtp = NULL;
			}
		}
		else
		{
			iAddiLen = ONCE_FILE_BLOCK;
		}
		
		if (poFtp == NULL)/**资源复用 **/
		{
			poFtp = (TBALFTP *)malloc(iAddiLen+sizeof(T_BFHD) + 1);
			if (poFtp == NULL)
			{
				pub_log_error("[%s][%d],malloc failed,iAddiLen=[%d],sizeof(T_BFHD)=[%d]",__FILE__,__LINE__,iAddiLen,sizeof(T_BFHD));
				TE_tpcommit(id);
				close(fd);
				return -1;
			}
		}
		
		memset(poFtp, 0x00, iAddiLen + sizeof(T_BFHD) + 1);
		/****TODO****/
		poFtp->head.mesgtype = '2';
		memset(poFtp->head.name, 0x00, sizeof(poFtp->head.name));
		
		char tmpname[256];
		memset(tmpname, 0x0, sizeof(tmpname));
		get_last_name(sTmp, tmpname);
		strncpy(poFtp->head.name, tmpname, sizeof(poFtp->head.name) - 1);
		
		balftp_fileinfo(sTmp, poFtp);
		poFtp->head.offset = iOffSet;
		poFtp->head.datasize = iAddiLen;
		poFtp->head.flag = '1';
		balftp_hton(poFtp);	
	
		lseek(fd, 0, iOffSet);
		read(fd, poFtp->data, iAddiLen);
		poFtp->data[iAddiLen] = 0x00;
		/****上传文件****/
		pub_log_info("%s:%d Send file  start[%s]",__FILE__,__LINE__, poFtp->data);
		
		iRc = -1;
		iRc = TE_tpbegin(PKTNEEDANS, g_tptime, id_send);
		if (iRc < 0)
		{
			pub_log_error("%s,%d,TE_tpbegin failed!iRc=[%d]", __FILE__,__LINE__,iRc);
			if( poFtp != NULL )
			{
				free((char *)poFtp);
			}
		  	close(fd);
			return -1;
		}
		
	  	iRc = -1;
		iRc = TE_tpcall("BALFTP", (char *)poFtp, iAddiLen + sizeof(T_BFHD), &pcTuxRecv, &iTuxRecvLen, &FileFlag, id_send);	
		if (iRc)
		{
			pub_log_error("%s:%d ,[%s]Send file  failed!iRc=[%d]",__FILE__,__LINE__,"BALFTP", iRc);
			pub_log_error("%s:%d ,addlen[%d] rcvlen[%d][%s]",__FILE__,__LINE__,iAddiLen, iTuxRecvLen, pcTuxRecv);	
			if (poFtp != NULL)
			{
				free((char *)poFtp);
			}
			close(fd);
			return(-1);
		}
		iOffSet = iOffSet + iAddiLen;
		TE_tpcommit(id_send);	
	}
	close(fd);
	if (poFtp != NULL)/**这个变量可能没有值，先判断一下 **/
	{
		free((char *)poFtp);
	}
	
	return 0;
}

int tea_7f_rcv(TE_ID id, sw_loc_vars_t *vars)
{
	int i;
	int iRc;
	int iAddiLen;
	int iFileLen;
	int FileFlag = 0;
	int iTuxRecvLen;	
	char sFileName[129];
	char pcFileName[256];
	char sTmp[256];
	char *pcTuxRecv = NULL;
	char *pFileBuf = NULL;
	TBALFTP *poFtp = NULL;
	FILE *fp = NULL;
	TE_ID id_recv;
	char *p=NULL;
	char sNode_name[15+1];
	char sIp_addr[15+1];
	int  iPort=0;
	int	iFor;
	int iSeq =0;
	int iDate=0;
	char sSeq[10];
	char sDate[10];

	UPNODE_INFO upinfo;
   	memset(&upinfo,0,sizeof(UPNODE_INFO));

	loc_get_zd_data(vars, "$nodename", sNode_name);
	loc_get_zd_data(vars, "$ip", sIp_addr);
	loc_get_zd_int(vars, "$port", &iPort);
	
	pub_log_info("[%s][%d] nodename=[%s], ip=[%s], port=[%d]", __FILE__, __LINE__, sNode_name, sIp_addr, iPort);	
	
	i = -1;
	iRc = -1;
	iAddiLen = -1;
	iFileLen = -1;
	iTuxRecvLen = 0;
	memset(sFileName, 0x00, sizeof(sFileName));
	memset(pcFileName, 0x00, sizeof(pcFileName));
	memset(sTmp, 0x00, sizeof(sTmp));
	pcTuxRecv = NULL;
	pFileBuf = NULL;
	poFtp = NULL;
	fp = NULL;
	
	memset(sTmp,0x00,sizeof(sTmp));
	loc_get_zd_data(vars, "$fileflag", sTmp);
	pub_log_info("[%s][%d] 文件标志=[%s]", __FILE__, __LINE__, sTmp);
	if (sTmp[0] != '1' && sTmp[0] != '2')
	{
		pub_log_info("[%s][%d]结尾字符[%x]未指定文件!",__FILE__,__LINE__, sTmp[0]);
		return 0;
	}

	strcpy(upinfo.UName[0],sNode_name);/**服务节点名称**/
	upinfo.UPort[0] = iPort; /**服务端口**/
	strcpy(upinfo.UIPAddr[0],sIp_addr);/**ip地址**/

	pub_log_error("[%s][%d],cNodeName=[%s],cAddr=[%s],Port=[%d]",__FILE__,__LINE__,upinfo.UName[0],upinfo.UIPAddr[0],upinfo.UPort[0]);

	/**尝试连接三次，三次都不成功才失败**/
	iFor = 0;
	
	while(1)
	{
		iFor++;		
		id_recv = TE_tpinit(0,0,&upinfo);
		if (id_recv == NULL)
		{
			pub_log_info("[%s][%d],TE_tpinit failed!!upnodename[%s],cAddr=[%s],port[%d]",__FILE__,__LINE__,upinfo.UName[0],upinfo.UIPAddr[0],upinfo.UPort[0]);
			if (iFor <= 3)
			{
				sleep(2);
				continue;
			}
			else
			{
				pub_log_error("[%s][%d],TE_tpinit  failed!!upnodename[%s],cAddr=[%s],port[%d]",__FILE__,__LINE__,upinfo.UName[0],upinfo.UIPAddr[0],upinfo.UPort[0]);					
				return -1;
			}
		}
		break;
	}

	if(id_recv == NULL)
	{
		pub_log_error("[%s][%d],id_recv is null", __FILE__,__LINE__);
		return -1;
	}

	memset(sFileName, 0x0, sizeof(sFileName));
	loc_get_zd_data(vars, "$filename", sFileName);
	if (sFileName[0] == '\0')
	{
		memset(sDate,0x00,sizeof(sDate));
		loc_get_zd_data(vars, "#sys3", sDate);
		memset(sSeq,0x00,sizeof(sSeq));
		loc_get_zd_data(vars, "#sys10", sSeq);
		sprintf(sFileName, "%s%s", sDate, sSeq);
	}
	
	pub_log_info("[%s][%d],要接收的文件名 is [%s]",__FILE__,__LINE__,sFileName);

	iAddiLen = sizeof(T_BFHD);
	pub_log_info("[%s][%d] pcTuxRecv, iAddiLen=[%d]", __FILE__,__LINE__,iAddiLen);
	
	poFtp = (TBALFTP *)malloc(iAddiLen + 1);
	if (poFtp == NULL )
	{
		pub_log_error("%s,%d,malloc failed!!",__FILE__,__LINE__);
		return -1;
	}
	
	memset(poFtp, 0x0, iAddiLen);
	memset(poFtp->head.name, 0x00, sizeof(poFtp->head.name));
	poFtp->head.mesgtype = '1';
	strncpy(poFtp->head.name, sFileName, sizeof(poFtp->head.name));
	poFtp->head.filesize = 0;
	poFtp->head.modtime = 0;
	poFtp->head.datasize = ONCE_FILE_BLOCK;
	poFtp->head.offset = 0;
	poFtp->head.flag = '0';
	
	while(1)
	{
		balftp_hton(poFtp);
		pub_log_info("%s,%d,开始执行TE_tpbegin ", __FILE__,__LINE__);
		iRc = TE_tpbegin(PKTNEEDANS, g_tptime, id_recv);
		if (iRc < 0)
		{
	  		pub_log_error("%s,%d,TE_tpbegin failed!iRc=[%d]", __FILE__,__LINE__,iRc);
		 	if (poFtp != NULL)
		 	{
				free((char *)poFtp);
			}
			return -1;
		}
		
		pub_log_error("%s,%d,开始执行TE_SetBranchMsg ", __FILE__,__LINE__);
		iRc = TE_SetBranchMsg(PKTNEEDANS|TECOMPPKT, g_tptime, id_recv);
		if (iRc)
		{
			pub_log_error("[%s][%d] ,TE_Setbranchmsg  failed!iRc=[%d]",__FILE__,__LINE__,iRc);	
			return -1;
		}
	
		/****接收文件****/
		pub_log_info("[%s][%d]开始执行TE_tpcall ", __FILE__,__LINE__);
		iRc = TE_tpcall("BALFTP", (char *)poFtp, iAddiLen, &pcTuxRecv, &iTuxRecvLen, &FileFlag, id_recv);
		if(iRc)
		{
			pub_log_error("%s:%d ,[%s]Recv file  failed!iRc=[%d]",__FILE__,__LINE__,"BALMONFTP",iRc);	
			if (poFtp != NULL)
			{
				free((char *)poFtp);
			}
			TE_tpabort(id_recv);
			TE_tpterm(id_recv);
			return -1;
		}
		
		pub_log_bin(LOG_DEBUG, pcTuxRecv,iTuxRecvLen,"recv file success![%s][%d],len=[%d]\n",__FILE__,__LINE__, iTuxRecvLen);
		if(iTuxRecvLen < sizeof(T_BFHD))
		{
			pub_log_error("%s:%d ,[%s]Recv file  failed!",__FILE__,__LINE__,"BALFTP");	
			if (poFtp != NULL)
			{
				free((char *)poFtp);
			}
			TE_tpcommit(id_recv);
			TE_tpterm(id_recv);
			return -1;
		}
		
		memcpy(poFtp, pcTuxRecv, sizeof(T_BFHD));
		balftp_ntoh(poFtp);
		if (pFileBuf == NULL)
		{
			pFileBuf = (char*)malloc(poFtp->head.filesize + 1);
			if (pFileBuf == NULL)
			{
				pub_log_error("%s:%d ,[%s]new pFileBuf failed!!",__FILE__,__LINE__,"BALFTP");
				if (poFtp != NULL)
				{
					free((char *)poFtp);
				}
				TE_tpcommit(id_recv);
				TE_tpterm(id_recv);
				return -1;
			}
		}
		
		pub_log_debug("[%s][%d] poFtp->head.offset=[%d], poFtp->head.datasize=[%d]", __FILE__, __LINE__, poFtp->head.offset,poFtp->head.datasize);
		memcpy(pFileBuf + poFtp->head.offset, pcTuxRecv+sizeof(T_BFHD), poFtp->head.datasize);
		if (poFtp->head.offset + poFtp->head.datasize >= poFtp->head.filesize)
		{
			TE_tpcommit(id_recv);
			TE_tpterm(id_recv);
			break;
		}
			
		poFtp->head.offset = poFtp->head.offset + poFtp->head.datasize ;
		if( poFtp->head.filesize - poFtp->head.offset  > ONCE_FILE_BLOCK )
		{
			poFtp->head.datasize = ONCE_FILE_BLOCK;
		}
		else
		{
			poFtp->head.datasize = poFtp->head.filesize - poFtp->head.offset;
		}
		pub_log_debug("[%s][%d] poFtp->head.offset=[%d], poFtp->head.datasize=[%d]", __FILE__, __LINE__, poFtp->head.offset, poFtp->head.datasize);
		TE_tpcommit(id_recv);
	}
	
	pFileBuf[poFtp->head.filesize] = '\0';
	iFileLen = poFtp->head.filesize;

	if (poFtp != NULL)
	{
		free(poFtp);
		poFtp = NULL;
	}
	
	memset(pcFileName, 0x00, sizeof(pcFileName));
	sprintf(pcFileName, "%s/tmp/", getenv("SWWORK"));
	memcpy(pcFileName + strlen(pcFileName), sFileName, sizeof(sFileName));
	zip_space(pcFileName);
	pub_log_info("[%s][%d]file=[%s]!", __FILE__, __LINE__, pcFileName);
	fp = fopen(pcFileName, "wb");
	if (fp == NULL)
	{
		pub_log_error("%s,%d,保存文件[%s]错误[%d]!\n",__FILE__,__LINE__,pcFileName, errno);
		if (pFileBuf != NULL)
		{
			free((char*)pFileBuf);
			pFileBuf = NULL;
		}
		return -1;
	}

	fwrite(pFileBuf, iFileLen, 1, fp);
	fclose(fp);
	
	if (pFileBuf != NULL)
	{
		free((char*)pFileBuf);
		pFileBuf = NULL;
	}
	
	pub_log_info("%s,%d,Gethere", __FILE__, __LINE__);
	return 0;
}