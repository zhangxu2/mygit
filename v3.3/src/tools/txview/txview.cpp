 /************************************
 * 程 序 名: txview.cpp              * 
 * 程序功能: 平台流水实时监控        *
 * 作    者: MaWeiwei                *
 * 完成日期: 2013-12-01              *
 ************************************/

#include "CWin.h"
#include "ListWin.h"
#include "InputWin.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
extern "C" {
#include "cycle.h"
#include "pub_type.h"
#include "seqs.h"
#include "pub_log.h"
}

#define SYS_TRACE_NAME  "SYS_SEQS_NAME"

char	sSysDate[32];
ListWin	*pListMain = NULL;
ListWin	*pListRequest = NULL;
ListWin *pListResponse = NULL;
InputWin	*pInput = NULL;
sw_int64_t	lLastTraceNo = 0;

unsigned int OTDACS_HLINE;
unsigned int OTDACS_VLINE;
unsigned int OTDACS_LTEE;
unsigned int OTDACS_RTEE;
unsigned int OTDACS_ULCORNER;
unsigned int OTDACS_LLCORNER;
unsigned int OTDACS_URCORNER;
unsigned int OTDACS_LRCORNER;
unsigned int OTDACS_PLUS;
unsigned int OTDACS_BTEE;
unsigned int OTDACS_TTEE;

typedef struct
{
	char	sTxCode[32];
	char	sChnl[32];
	char	sPrdtName[32];
	char	sTxDate[16];
	char	sTraceNo[20];
	char	sTxAmt[20];
	char	sTxSts[20];
	char	sRespcd[8];
	char	sFeeAmt[16];
	char	sTxTime[16];
	char	sSvrName[64];
	char	sSvcName[64];
}TXLOG_T;

int	g_TraceNo = 0;

char *zip_space(char *str)
{
	char	*ptr = NULL;
	char	*destr = NULL;	

	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			*ptr++ = *str;
		}
		str++;
	}
	*ptr = '\0';

	return destr;
}

char *zip_head(char *str)
{
	char	*ptr = NULL;
	char	*destr = NULL;

	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			break;
		}
		str++;
	}
	while (*str != '\0')
	{
		*ptr++ = *str++;
	}
	*ptr = '\0';
	return destr;
}

char *zip_tail(char *str)
{
	char	*ptr = NULL;
	char	*destr = NULL;
	
	destr = str;
	ptr = str + strlen(str) - 1;
	while (str <= ptr)
	{
		if ((*ptr != ' ') && (*ptr != '\t'))
		{
			ptr++;
			*ptr = '\0';
			break;
		}
		ptr--;
	}
	return destr;
}

int iLinkShm()
{
	int	iRc = 0;
	sw_cycle_t	cycle;
	
	memset(&cycle, 0x0, sizeof(cycle));
	iRc = cycle_init(&cycle, "txview", ND_ADM, "txview_error.log", "txview.log", NULL);
	if (iRc != SW_OK)
	{
		printf("[%s][%d] Cycle init error!\n", __FILE__, __LINE__);
		return -1;
	}

	iRc = cycle_link_shm_run(&cycle);
	if (iRc != SW_OK)
	{
		printf("[%s][%d] Link shm error!\n", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

sw_int64_t lGetLastTraceNo()
{
	int	ret = 0;
	sw_char_t	trace_no[32];
	
	memset(trace_no, 0x0, sizeof(trace_no));
	ret = seqs_get_bsn_trace(SYS_TRACE_NAME, trace_no);
	if (ret != SW_OK || atoll(trace_no) <= 1)
	{
		return 0;
	}
	
	return atoll(trace_no) - 1;
}

sw_int64_t lGetTraceNo()
{
	int	ret = 0;
	sw_char_t	trace_no[32];
	
	memset(trace_no, 0x0, sizeof(trace_no));
	ret = seqs_get_bsn_trace(SYS_TRACE_NAME, trace_no);
	if (ret != SW_OK)
	{
		return -1;
	}
	
	return atoll(trace_no);
}

int iGetSysDate(char *sDate)
{
	pub_time_getdate(sDate, 1);
	
	return 0;
}

int iGetTotParam(char *buf, TXLOG_T *stTxLog)
{
	int	i = 0;
	char	sOut[60][64];
	char	*ptr = NULL;
	char	*pTmp = NULL;
	const char	*sep = "|";
	
	memset(sOut, 0x0, sizeof(sOut));
	i = 0;
	ptr = buf;
	while (*ptr != '\0')
	{
		pTmp = strstr(ptr, sep);
		if (pTmp == NULL)
		{
			strcpy(sOut[i], ptr);
			i++;
			break;
		}
		memcpy(sOut[i], ptr, pTmp - ptr);
		ptr = pTmp + strlen(sep);
		i++;	
	}
	strcpy(stTxLog->sTxCode, sOut[0]);
	strcpy(stTxLog->sTraceNo, zip_space(sOut[1]));
	strcpy(stTxLog->sSvrName, zip_space(sOut[2]));
	strcpy(stTxLog->sSvcName, zip_space(sOut[3]));
	strcpy(stTxLog->sTxDate, zip_space(sOut[4]));
	strcpy(stTxLog->sPrdtName, zip_space(sOut[8]));
	if (strncmp(sOut[18], "CHNL_", 5) == 0)
	{
		strcpy(stTxLog->sChnl, zip_space(sOut[18] + 5));
	}
	else
	{
		strcpy(stTxLog->sChnl, zip_space(sOut[18]));
	}
	strcpy(stTxLog->sRespcd, zip_space(sOut[7]));
	if (strcmp(sOut[7], "0000") == 0 || strcmp(sOut[7] + 3, "0000") == 0)
	{
		strcpy(stTxLog->sTxSts, "交易成功");
	} 
	else
	{
		strcpy(stTxLog->sTxSts, "交易失败");
	}
	
	return 0;
}

int iGetTraceInfo(sw_int64_t lTraceNo, TXLOG_T *pstTxLog)
{
	int	iFlag = 0;
	FILE	*fp = NULL;
	char	sDate[32];
	char	sLine[1024];
	char	sFileName[128];
	TXLOG_T	stTxLog;
	
	memset(sDate, 0x0, sizeof(sDate));
	memset(sLine, 0x0, sizeof(sLine));
	memset(sFileName, 0x0, sizeof(sFileName));

	pub_time_getdate(sDate, 1);
	sprintf(sFileName, "%s/tmp/monitor/%s/monitor.log", getenv("SWWORK"), sDate);
	fp = fopen(sFileName, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, sFileName, errno, strerror(errno));
		return -1;
	}
	
	iFlag = 0;
	while (1)
	{
		memset(sLine, 0x0, sizeof(sLine));
		if (fgets(sLine, sizeof(sLine), fp) == NULL)
		{
			break;
		}
		
		if (strncmp(sLine, "TOTAL:", 6) != 0)
		{
			continue;
		}
		memset(&stTxLog, 0x0, sizeof(TXLOG_T));
		iGetTotParam(sLine + 6, &stTxLog);
		if (atoll(stTxLog.sTraceNo) == lTraceNo)
		{
			iFlag = 1;
			memcpy(pstTxLog, &stTxLog, sizeof(TXLOG_T));
			break;
		}
	}
	fclose(fp);
	
	if (iFlag == 0)
	{
		pub_log_error("[%s][%d] 未找到[%lld]的交易信息!", __FILE__, __LINE__, lTraceNo);
		return -1;
	}
	
	return 0;
}

int iGetNextTrace(sw_int64_t lTraceNo, TXLOG_T *stTxLog)
{
	int	iRc = 0;
	sw_int64_t	lCurrTraceNo = 0;
	
	lCurrTraceNo = lGetTraceNo();
	if (lTraceNo <= 0 || lTraceNo >= lCurrTraceNo)
	{
		return -1;
	}
	
	iRc = iGetTraceInfo(lTraceNo, stTxLog);
	if (iRc)
	{
		return -1;
	}
	
	return 0;
}

int initCurses()
{
	initscr();          /* 开启curses模式 */
	cbreak();           /* 设置按键被立即读取 */
	nonl();
	noecho();           /* 键盘输入字元时不会将字元显示在终端机上 */
	intrflush(stdscr,FALSE);
	keypad(stdscr,TRUE);
	wrefresh(stdscr);

	return 0;
}

int iClearResource()
{
	if (pListMain != NULL)
	{
		delete pListMain;
		pListMain = NULL;
	}
	
	if (pListRequest != NULL)
	{
		delete pListRequest;
		pListRequest = NULL;
	}
	
	if (pListResponse != NULL)
	{
		delete pListResponse;
		pListResponse = NULL;
	}
	
	if (pInput != NULL)
	{
		delete pInput;
		pInput = NULL;
	}
	
	endwin();
	printf("\n");
	return 0;
}

typedef struct node
{
	int	step;
	char	*data;
	struct node	*next;
}node_t, *list_t;

int init_list(list_t *list)
{
	*list = (list_t)calloc(1, sizeof(node_t));
	if (*list == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	(*list)->step = -1;
	(*list)->next = NULL;
	
	return 0;
}

int add_node(list_t *list, char *data, int len, int step)
{
	list_t	p = NULL;
	list_t	r = NULL;
	list_t	s = NULL;

	if (*list == NULL)
	{
		pub_log_error("[%s][%d] list is null!", __FILE__, __LINE__);
		return -1;
	}
	
	p = *list;
	while (p->next != NULL)
	{
		p = p->next;
	}
	
	s = (list_t)calloc(1, sizeof(node_t));
	if (s == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	s->data = (char *)calloc(1, len + 1);
	if (s->data == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	memcpy(s->data, data, len);
	s->step = step;
	s->next = p->next;
	p->next = s;
	
	return 0;
}

int print_list(list_t list, int step)
{
	list_t	p = NULL;
	ListItem	*pListItem = NULL;
	
	if (list == NULL)
	{
		return 0;
	}
	
	p = list->next;
	while (p != NULL)
	{
		if (p->step == step)
		{
			pListItem = pListRequest->addListItem("");
			if (pListItem == NULL)
			{
				iClearResource();
				pub_log_error("[%s][%d] 插入列表框失败!", __FILE__, __LINE__);
				exit(1);
			}
			pListItem->setSubItem(0, p->data);
		}
		p = p->next;
	}
	
	return 0;
}

int destroy_list(list_t *list)                                                                                          
{
	list_t	p = NULL;
	list_t	q = NULL;
	
	q = *list;
	if (q == NULL)
	{
		return 0;
	}
	
	while (q->next != NULL)
	{
		p = q->next;
		q->next = p->next;
		if (p->data != NULL)
		{
			free(p->data);
			p->data = NULL;
		}
		free(p);
		p = NULL;
	}
	free(*list);
	*list = NULL;
	
	return 0;
}                      

int iPrintVars(char *sTraceNo, char *sPrdtName, char *sSvrName, char *sSvcName)
{
	int	i = 0;
	int	iRc = 0;
	int	iLen = 0;
	int	iStep = 0;
	FILE	*fp = NULL;
	char	*ptr = NULL;
	char	sBuf[128];
	char	sLine[1024];
	char	sFileName[128];
	list_t	list = NULL;
	ListItem	*pListItem = NULL;
	
	memset(sBuf, 0x0, sizeof(sBuf));
	memset(sLine, 0x0, sizeof(sLine));
	memset(sFileName, 0x0, sizeof(sFileName));
	
	sprintf(sFileName, "%s/log/%s/%s/%s_%s.pkg", getenv("SWWORK"), sPrdtName, sSvrName, sSvcName, sSysDate);
	fp = fopen(sFileName, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, sFileName, errno, strerror(errno));
		return -1;
	}
	
	iStep = 0;
	init_list(&list);
	while (1)
	{
		memset(sLine, 0x0, sizeof(sLine));
		if (fgets(sLine, sizeof(sLine), fp) == NULL)
		{
			break;
		}
		iLen = strlen(sLine);
		if (sLine[iLen - 1] == '\n')
		{
			sLine[iLen - 1] = '\0';
		}
	
		iLen = strlen(sLine);
		if (sLine[iLen - 1] == '|')
		{
			sLine[iLen - 1] = '\0';
		}	
	
		memset(sBuf, 0x0, sizeof(sBuf));
		sprintf(sBuf, "%s|---------------BEGIN---------------|", sTraceNo);
		if (strncmp(sLine, sBuf, strlen(sBuf)) == 0)
		{
			continue;
		}

		memset(sBuf, 0x0, sizeof(sBuf));
		sprintf(sBuf, "%s|----------------END----------------|", sTraceNo);
		if (strncmp(sLine, sBuf, strlen(sBuf)) == 0)
		{
			break;
		}

		ptr = strchr(sLine, '|');
		if (ptr == NULL)
		{
			continue;
		}
		memset(sBuf, 0x0, sizeof(sBuf));
		strncpy(sBuf, sLine, ptr - sLine);
		if (strcmp(sBuf, sTraceNo) != 0)
		{
			continue;
		}
		
		memset(sBuf, 0x0, sizeof(sBuf));
		sprintf(sBuf, "%s|@@@:", sTraceNo);
		if (strncmp(sLine, sBuf, strlen(sBuf)) == 0)
		{
			iStep++;
		}
		add_node(&list, ptr + 1, strlen(ptr + 1), iStep);
	}
	fclose(fp);
	
	i = 1;
	while (1)
	{
		pListRequest->clearListItem();
		if (i > iStep)
		{
			showmsg("无数据到达!");
			pInput->nClearTraceNo();
			break;
		}
		memset(sBuf, 0x0, sizeof(sBuf));
		sprintf(sBuf, "                        交易要素(第%d步)", i);
		pListRequest->setColumn(0, sBuf, 76);
		print_list(list, i);
		iRc = pListRequest->work();
		if (iRc == WIN_OK || iRc == WIN_TAB)
		{
			i++;
			continue;
		}
		else if (iRc == WIN_R)
		{
			i--;
			if (i <= 0)
			{
				i = 1;
			}
			continue;
		}
		else
		{
			break;
		}
	}
	destroy_list(&list);
	pListMain->showList(1);
	pInput->nClearTraceNo();
	
	return 0;
}

int main()
{
	int	iRc = 0;
	int	iTimes = 0;
	TXLOG_T	stTxLog;
	ListItem	*pListItem = NULL;
	
	memset(sSysDate, 0x0, sizeof(sSysDate));
	
	iRc = iLinkShm();
	if (iRc)
	{
		printf("[%s][%d] Link shm error!\n", __FILE__, __LINE__);
		return -1;
	}
	iGetSysDate(sSysDate);
	lLastTraceNo = lGetLastTraceNo();
	
	initCurses();
	pListMain = new ListWin(22, 121, 1, 0);
	if (pListMain == NULL)
	{
		iClearResource();
		printf("初始化主窗口失败!\n");
		return -1;
	}
	pListMain->setColumnCount(10);
        pListMain->setColumn(0, "    产品标识    ", 16);
        pListMain->setColumn(1, "发起渠道", 10);
        pListMain->setColumn(2, "交易码", 10);
        pListMain->setColumn(3, "交易日期", 10);
        pListMain->setColumn(4, "平台流水", 14);
        pListMain->setColumn(5, "交易金额", 20);
        pListMain->setColumn(6, "交易状态", 10);
        pListMain->setColumn(7, "处理码", 8);
        pListMain->setColumn(8, "手续费", 10);
        pListMain->setColumn(9, "交易时间", 10);
	/*** 请求列表框 ***/
	pListRequest = new ListWin(22, 121, 1, 0);
	if (pListRequest == NULL)
	{
		iClearResource();
		printf("初始化请求窗口失败!\n");
		return -1;
	}
	pListRequest->setColumnCount(1);
	pListRequest->setColumn(0, "  交易要素 ", 76);
	
	pListResponse = NULL;
	pInput = new InputWin();
	if (pInput == NULL)
	{
		iClearResource();
		printf("初始化输入窗口失败!\n");
		return -1;
	}
	pInput->setDate(sSysDate);
	
	mvaddstr(0, 50, "平台流水监控系统");
	refresh();
	while (1)
	{
		while (1)
		{
			memset(&stTxLog, 0x0, sizeof(stTxLog));
			iRc = iGetNextTrace(lLastTraceNo, &stTxLog);
			if (iRc)
			{
				break;
			}
			lLastTraceNo++;
			pListItem = pListMain->addListItem("");
			if (pListItem == NULL)
			{
				printf("在主界面插入列表框失败!\n");
				endwin();
				return -1;
			}
			pListItem->setSubItem(0, stTxLog.sPrdtName);
			pListItem->setSubItem(1, stTxLog.sChnl);
			pListItem->setSubItem(2, stTxLog.sTxCode);
			pListItem->setSubItem(3, stTxLog.sTxDate);
			pListItem->setSubItem(4, stTxLog.sTraceNo);
			pListItem->setSubItem(5, stTxLog.sTxAmt);
			pListItem->setSubItem(6, stTxLog.sTxSts);
			pListItem->setSubItem(7, stTxLog.sRespcd);
			pListItem->setSubItem(8, stTxLog.sFeeAmt);
			pListItem->setSubItem(9, stTxLog.sTxTime);
			pListItem->setSubItem(10, stTxLog.sSvrName);
			pListItem->setSubItem(11, stTxLog.sSvcName);
		}
		
		if (pListMain->getListCount() > 0)
		{
			pListMain->ensureVisable(pListMain->getListCount() - 1);
			pListMain->selectItem(pListMain->getListCount() - 1);
		}
		pListMain->showList(1);
		
		while (1)
		{
			iRc = pInput->input();
			if (iRc == WIN_OK)
			{
				char	*sTraceNo = pInput->pcGetTraceNo();
				if (atoll(sTraceNo) == 0)
				{
					showmsg("流水不能为空!");
					pInput->nClearTraceNo();
					continue;
				}
				pub_log_debug("[%s][%d] sTraceNo=[%s]", __FILE__, __LINE__, sTraceNo);
				memset(&stTxLog, 0x0, sizeof(stTxLog));
				iRc = iGetTraceInfo(atoll(sTraceNo), &stTxLog);
				if (iRc)
				{
					showmsg("没有找到流水[%s]的记录!", sTraceNo);
					pInput->nClearTraceNo();
					continue;
				}
				iPrintVars(stTxLog.sTraceNo, stTxLog.sPrdtName, stTxLog.sSvrName, stTxLog.sSvcName);
				continue;
			}
			else if (iRc == WIN_ESC)
			{
				iClearResource();
				return 0;
			}
			else if (iRc == WIN_TAB)
			{
				pInput->show();
				iRc = pListMain->work();
				if (iRc == WIN_TAB)
				{
					pListMain->showList(1);
					continue;
				}
				else if (iRc ==  WIN_ESC)
				{
					iClearResource();
					return 0;
				}
				else if (iRc == WIN_OK)
				{
					char	*sTraceNo = pListMain->getSelectedItem()->getSubItem(4);
					char	*sSvrName = pListMain->getSelectedItem()->getSubItem(10);
					char	*sSvcName = pListMain->getSelectedItem()->getSubItem(11);
					char	*sPrdtName = pListMain->getSelectedItem()->getSubItem(0);
					iPrintVars(sTraceNo, sPrdtName, sSvrName, sSvcName);
					continue;
				}
				else
				{
					pListMain->showList(1);
					continue;
				}
			}
			else if (iRc == WIN_TMOUT)
			{
				pInput->show();
				break;
			}
			else
			{
				pListMain->showList(1);
				continue;
			}
		}
	}
	iClearResource();
	
	return 0;
}
