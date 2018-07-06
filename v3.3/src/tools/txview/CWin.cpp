/***************************************************************************
 *** 程序作者 : 石升辉                                                   ***
 *** 日    期 : 2008-02-13                                               ***
 *** 所属模块 :                                                          ***
 *** 程序名称 : CWin.c                                                   ***
 *** 程序作用 :                                                          ***
 *** 使用注意 :                                                          ***
 ***************************************************************************/

#ifdef AIX_CURSES
extern "C"{
#include "/usr/include/curses.h"
}
#else
#include"curses.h"
#endif
#include "CWin.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/time.h"
#include <signal.h>

CWin::CWin(){
  sLines = 0;
  sCols  = 0;
  sBeginLine = 24;
  sBeginCol  = 80;
  wID =stdscr;
}

CWin::CWin(int sL,int sC,int sBL,int sBC){
  sLines = sL;
  sCols  = sC;
  sBeginLine = sBL;
  sBeginCol  = sBC;
  wID = newwin((int)sLines,(int)sCols,(int)sBeginLine,(int)sBeginCol);
  if (wID == NULL)
  {
	printf("[%s][%d] newwin error!\n", __FILE__, __LINE__);
  }
  //wID->_delay=500;
}

CWin::~CWin() {
  if (wID != stdscr) {
      delwin(wID);
  }
}

WINDOW * CWin::resetWin(int sL,int sC,int sBL,int sBC) {
  sLines = sL;
  sCols  = sC;
  sBeginLine = sBL;
  sBeginCol  = sBC;
  if (wID != stdscr) {
      delwin(wID);
  }
  wID = newwin((int)sLines,(int)sCols,(int)sBeginLine,(int)sBeginCol);
  //wID->_delay=500;
  return(wID);
}


WINDOW * CWin::getWin() {
  return(wID);
}

int CWin::getCols() {
  return(sCols);
}

int CWin::getBeginCol() {
  return(sBeginCol);
}

void CWin::showWin() {
  
  touchwin(wID);
  wrefresh(wID);
  //placeCursor();
}

void CWin::clean() {
  //Modified by SSH,2004.7.28
  //wclear会刷新整个屏幕造成闪烁，所以不用
  //wclear(wID);
  int maxX,maxY;
  int i;
  getmaxyx(wID,maxY,maxX);
  wattrset(wID,A_NORMAL);
  for(i=0;i<maxY;i++){
	mvwhline(wID,i,0,' ',maxX);
  } 
}

void CWin::placeCursor(int sL,int sC) {
  move((int)sL,(int)sC);
  return;
}
void CWin::placeCursor() {
  move(23,78);
  //addch(' ');
  return;
}

void CWin::clear_ok() {
  clearok(wID,TRUE);
}
void sigGetChTimer(int signo){
	return;
}
int otdGetCh_a()
{
	int  iInputChar;
	/*Modified by SSH,2004.2.13,当getch返回－1时可能是由于发生了时钟中断，所以不能退出*/
	int chKeys[][5]={/*。键值映射最后一位为映射值，长度不足补0*/
		/*Add by SSH,2004.5.25,增加实达终端CT100*/
		{27,32,0,0,KEY_F(1)},
		{27,33,0,0,KEY_F(2)},
		{27,34,0,0,KEY_F(3)},
		{27,35,0,0,KEY_F(4)},
		{27,36,0,0,KEY_F(5)},
		{27,37,0,0,KEY_F(6)},
		{27,38,0,0,KEY_F(7)},
		{27,39,0,0,KEY_F(8)},
		{27,40,0,0,KEY_F(9)},
#ifdef SCO
		{27,64,0,0,KEY_IC},/*'-'*/
#endif
		{27,64,0,0,KEY_IC},/*INSERT*/
		{27,79,80,0,KEY_F(1)},/*f1*/
		{27,79,81,0,KEY_F(2)},/*f2*/
		{27,79,82,0,KEY_F(3)},/*f3*/
		{27,79,83,0,KEY_F(4)},/*f4*/
		{27,79,116,0,KEY_F(5)},/*f5*/
		{27,79,117,0,KEY_F(6)},/*f6*/
		{27,79,118,0,KEY_F(7)},/*f7*/
		{27,79,108,0,KEY_F(8)},/*f8*/
		{27,79,119,0,KEY_F(9)},/*f9*/
		{27,79,120,0,KEY_F(10)},/*f10*/
		{27,79,121,0,KEY_F(11)},/*f11*/
		{27,79,122,0,KEY_F(12)},/*f12*/

		{27,79,110,0,KEY_DC},/*delete*/
		{27,79,112,0,KEY_IC},/*INSERT*/
		{27,79,120,0,KEY_UP},
		{27,79,114,0,KEY_DOWN},
		{27,79,115,0,KEY_NPAGE},
		{27,79,116,0,KEY_LEFT},
		{27,79,118,0,KEY_RIGHT},
		{27,79,77,0,KEY_ENTER},/*enter*/
		{27,79,109,0,'-'},/*'-'*/
		{27,91,76,0,KEY_IC},/*'-'*/
		{27,91,68,0,KEY_LEFT},/*left*/
		{27,91,67,0,KEY_RIGHT},/*right*/
		{27,91,65,0,KEY_UP},/*up*/
		{27,91,66,0,KEY_DOWN},/*down*/
		{27,91,72,0,KEY_HOME},
		{27,91,70,0,KEY_END},
		{27,91,73,0,KEY_PPAGE},
		{27,91,71,0,KEY_NPAGE},
		{27,91,77,0,KEY_F(1)},/*f1*/
		{27,91,78,0,KEY_F(2)},/*f2*/
		{27,91,79,0,KEY_F(3)},/*f3*/
		{27,91,80,0,KEY_F(4)},/*f4*/
		{27,91,81,0,KEY_F(5)},/*f5*/
		{27,91,82,0,KEY_F(6)},/*f6*/
		{27,91,83,0,KEY_F(7)},/*f7*/
		{27,91,84,0,KEY_F(8)},/*f8*/
		{27,91,85,0,KEY_F(9)},/*f9*/
		{27,91,86,0,KEY_F(10)},/*f10*/
		{27,91,87,0,KEY_F(11)},/*f11*/
		{27,91,88,0,KEY_F(12)},/*f12*/
		{27,118,0,0,KEY_PPAGE},/*previous page*/
		/*Add by SSH,2004.5.25,END 增加实达终端CT100*/
		{0,0,0,0,0}
	};


	/*只有在接受字符输入时才打开时钟中断察看消息对列*/
	while((iInputChar = getch())==-1);


	if(iInputChar==27){/*是字符专义*/
		int ch;
		int col,row;
		row=0;

		for(col=1;col<4;col++){
			if(chKeys[row][col]!=0){
				/*认为还有后续字符*/
				/*Add by SSH,2004.6.25,缩短等待时间为1毫秒*/
				struct itimerval value,ovalue;
				signal(SIGALRM,sigGetChTimer);
				value.it_value.tv_sec=0;
				value.it_value.tv_usec=100000;
				value.it_interval.tv_sec=0;
				value.it_interval.tv_usec=0;
				setitimer(ITIMER_REAL,&value,&ovalue);

				ch=getch();
				if(ch<0){
					signal(SIGALRM,SIG_IGN);
					return(27);
				}
				signal(SIGALRM,SIG_IGN);
				/*找到第一个相符合的行*/
				while(chKeys[row][0]!=0){
					if(chKeys[row][col]==ch)
						break;
					row++;
				}
				if(chKeys[row][0]!=0){
					/*存在相符的字符定义,再等待下一个字符*/
					continue;
				} else{
					/*不存在相符的字符定义*/
					break;
				}


			} else{/*没有后续字符了,即找到了映射*/
				iInputChar=chKeys[row][4];
				break;
			}/*end if*/

		}/*end for*/
	}
	if(iInputChar==8)
		iInputChar=KEY_BACKSPACE;/*backspace*/
	else if(iInputChar==127)
		iInputChar=KEY_DC;/*delete*/
	else if(iInputChar==13)
		iInputChar=KEY_ENTER;/*enter*/
	else if(iInputChar==43 )
		iInputChar='+';
	else if(iInputChar==1938 || iInputChar==22)
		iInputChar=KEY_NPAGE;
	else if(iInputChar==1)
		iInputChar=KEY_HOME;
	else if(iInputChar==5)
		iInputChar=KEY_END;


	if (iInputChar == -1) {
		exit(1);
	}

	switch(iInputChar) {
	case 275 : /* ESC*/
		iInputChar = 27;          
		break;
	case 270:  /* F1*/
		iInputChar = KEY_F(1);    
		break;
	case 271:  /* F2*/
		iInputChar = KEY_F(2);    
		break;
	case 272:  /* F3*/
		iInputChar = KEY_F(3);    
		break;
	case 273:  /* F4*/
		iInputChar = KEY_F(4);    
		break;
	case 274:  /* F5*/
		iInputChar = KEY_F(5);    
		break;
	case 276:  /* F6*/
		iInputChar = KEY_F(6);    
		break;
	case 277:  /* F7*/
		iInputChar = KEY_F(7);    
		break;
	case 278:  /* F8*/
		iInputChar = KEY_F(8);    
		break;
	case 281:  /* F9*/
		iInputChar = KEY_F(9);    
		break;
	case 282:  /* F10*/
		iInputChar = KEY_F(10);   
		break;
	case 283:  /* F11*/
		iInputChar = KEY_F(11);   
		break;
	case 284:  /* F12*/
		iInputChar = KEY_F(12);   
		break;
	case 362:  /* Insert*/
		iInputChar = KEY_IC;      
		break;
	case 331:  /* Home*/
		iInputChar = KEY_HOME;    
		break;
	case 385:  /* Del*/
		iInputChar = KEY_DC;/*127;         break;*/
	case 339:  /* End*/
		iInputChar = KEY_END;/*F(4);    break;*/
	case 338:  /* PageDown:*/
		iInputChar = KEY_NPAGE;/*F(4);    break;*/
	case 265:  /* NumLock:*/
		iInputChar = KEY_F(1);    
		break;
	case 266:  /* little '/'*/
		iInputChar = '/';         
		break;
	case 267:  /* little '*'*/
		iInputChar = '*';         
		break;
	case 268:  /* little '-'*/
		iInputChar = '-';         
		break;
	default:
		break;
	}
	if(iInputChar==12){/*Ctrl+L刷新*/
		iInputChar=otdGetCh();
	}
	return(iInputChar);
}
int otdGetCh(){
	int iInputChar;
	iInputChar=0;

	while(iInputChar==0){
		iInputChar=otdGetCh_a();
	}
	return(iInputChar);
}

