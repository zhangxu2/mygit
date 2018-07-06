/***************************************************************************
 *** 程序作者 : Shi Shenghui  石升辉                                      **
 *** 日    期 : 2008-02-13                                               ***
 *** 所属模块 :                                                          ***
 *** 程序名称 : InputWin.c                                               ***
 *** 程序作用 : 输入流水号的窗口                                         ***
 *** 使用注意 :                                                          ***
 ***************************************************************************/
#include"InputWin.h"
#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include <errno.h>
#include <stdarg.h>
#if defined SCO ||defined AIX
#include <stdarg.h>
#endif
int otdGetCh();
int nWaitInput(int tmout);
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
InputWin::InputWin():CWin(1,80,23,0) {
	memset(caTraceNo,'\0',sizeof(caTraceNo));
	memset(caDate,'\0',sizeof(caDate));
	sCurCol=0;
	sInputTmout=INPUT_TMOUT;
}
int  InputWin::setDate(char *pcDate){
	memcpy(caDate,pcDate,sizeof(caDate)-1);
	return(0);
}
int  InputWin::show(){
	char sTmp[16];
	memset(sTmp,' ',sizeof(sTmp));
	wattrset(wID,A_NORMAL);
	mvwaddstr(wID,0,2,"交易日期:");
	memcpy(sTmp,caDate,strlen(caDate));
	sTmp[sizeof(caDate)-1]='\0';
	mvwaddstr(wID,0,12,sTmp);
	mvwaddstr(wID,0,29,"流水号:");
	wattron(wID,A_UNDERLINE);
	memset(sTmp,' ',sizeof(sTmp));
	memcpy(sTmp,caTraceNo,strlen(caTraceNo));
	sTmp[sizeof(caTraceNo)-1]='\0';
	mvwaddstr(wID,0,37,sTmp);
	wattrset(wID,A_NORMAL);
	showWin();
	return(0);
}
char *InputWin::pcGetTraceNo(){
	return(caTraceNo);
}
int  InputWin::nClearTraceNo(){
	memset(caTraceNo,'\0',sizeof(caTraceNo));
	sCurCol=0;
	return(0);
}
int  InputWin::input(){
	char sTmp[16];
	int  iInputChar;
	int  len;
	int  i;
	memset(sTmp,' ',sizeof(sTmp));
	wattrset(wID,A_NORMAL);
	mvwaddstr(wID,0,2,"交易日期:");
	memcpy(sTmp,caDate,strlen(caDate));
	sTmp[sizeof(caDate)-1]='\0';
	mvwaddstr(wID,0,12,sTmp);
	mvwaddstr(wID,0,29,"流水号:");
	wattron(wID,A_UNDERLINE);
	wattron(wID,A_REVERSE);
	/****TODO****/
	memset(sTmp,' ',sizeof(sTmp));
	memcpy(sTmp,caTraceNo,strlen(caTraceNo));
	sTmp[sizeof(caTraceNo)-1]='\0';
	mvwaddstr(wID,0,37,sTmp);
	showWin();
	sCurCol=strlen(caTraceNo);
	if(sCurCol==6){
		iInputChar=KEY_ENTER;
	}else{
		iInputChar=0;
	}
	while(1){
		wmove(wID,0,37+sCurCol);
		showWin();
		if(iInputChar==0){
			if(!nWaitInput(sInputTmout)){
				return(WIN_TMOUT);
			}
			iInputChar=otdGetCh();
		}
		switch(iInputChar){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(sCurCol>=6){
				return(-1);
			}
			caTraceNo[sCurCol++]=iInputChar;		
			waddch(wID,iInputChar);	
			if(sCurCol>=6){
				iInputChar=KEY_ENTER;
			}else{
				iInputChar=0;
			}
			break;
		case KEY_DC:
			for(i=sCurCol;i<6;i++){
				caTraceNo[i]=caTraceNo[i+1];
			}
			memset(sTmp,' ',sizeof(sTmp));
			memcpy(sTmp,caTraceNo,strlen(caTraceNo));
			sTmp[sizeof(caTraceNo)-1]='\0';
			mvwaddstr(wID,0,37,sTmp);
			iInputChar=0;
			break;
		case KEY_BACKSPACE:
			if(sCurCol>0){
				sCurCol--;
				iInputChar=KEY_DC;
				break;
			}else{
				iInputChar=0;
				break;
			}	
		case KEY_LEFT:
			if(sCurCol>0){
				sCurCol--;
			}
			iInputChar=0;
			break;
		case KEY_RIGHT:
			if(sCurCol<6-1 && caTraceNo[sCurCol]!='\0'){
				sCurCol++;
			}
			iInputChar=0;
			break;
		case 27:
			return(WIN_ESC);
		case KEY_ENTER:
		case 13:
			return(WIN_OK);			
		case '\t':
			return(WIN_TAB);			
		default:
			iInputChar=0;
		}
	}
	return(0);
}
/**
  函数名:nWaitInput
  参数:
         tmout             等待超时时间 
  功能:  等待一段时间并检查有没有输入 
  返回值:0(超时)/1(有输入)/-1(出错)
**/
int nWaitInput(int tmout){
	fd_set    rmask;
	struct    timeval time_out,*ptr;
	int iRc;
	memset(&time_out,0,sizeof(time_out));
	FD_ZERO(&rmask);
	FD_SET((unsigned int)0,&rmask);
	time_out.tv_sec  = tmout ;
	time_out.tv_usec = 0;
	ptr = &time_out;
	iRc=0;
	/*循环等待，直到socket空闲可读*/
	while (1)
	{
		iRc = select(1,&rmask,0,0,ptr);
		if (iRc>0)
		{
			break;
		}else if(iRc==0)
		{
			/*超时*/
			break;
		}else
		{
			if (errno==EINTR)
				continue;
			else
				break;
		}
	}
	return iRc;
}
int showmsg(char *pcFormat,...)
{
	char sTmp[31];
	WINDOW *wMsg;
	va_list  argptr;
	memset(sTmp,'\0',sizeof(sTmp));
	va_start(argptr,pcFormat);
	vsnprintf(sTmp,sizeof(sTmp)-1,pcFormat,argptr);
	va_end(argptr);
	wMsg=newwin(1,32,23,45);
	mvwaddstr(wMsg,0,0,"[");	
	mvwaddstr(wMsg,0,1,sTmp);	
	mvwaddstr(wMsg,0,31,"]");	
	touchwin(wMsg);
	wrefresh(wMsg);
	otdGetCh();
	memset(sTmp,' ',sizeof(sTmp));
	mvwaddstr(wMsg,0,0,sTmp);	
	touchwin(wMsg);
	wrefresh(wMsg);
	delwin(wMsg);	
	return(0);
}

