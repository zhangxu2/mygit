/***************************************************************************
 *** 程序作者 : 石升辉												   ***
 *** 日	期 : 2008-02-13											   ***
 *** 所属模块 :														  ***
 *** 程序名称 : CWin.h												   ***
 *** 程序作用 :														  ***
 *** 使用注意 :														  ***
 ***************************************************************************/


#ifndef _CWIN_H_
#define _CWIN_H_
#define WIN_OK	    1 				//回车键退出
#define WIN_ESC	    2 				//ESC键退出
#define WIN_TAB	    3				//TAB键退出 
#define WIN_TMOUT	4 				//超时未输入
#define WIN_R		5 				//输入R键退出
#ifdef AIX_CURSES
extern "C"{
#include "/usr/include/curses.h"
}
#else
#include "curses.h"
#endif
#include"sys/types.h"

class CWin {
public:
	int sLines;
	int sCols;
	int sBeginLine;
	int sBeginCol;
	WINDOW *wID;
public:
	CWin();
	CWin(int,int,int,int);
	~CWin();
	WINDOW *getWin();
	WINDOW *resetWin(int,int,int,int);
	int  getCols();
	int  getBeginCol();
	void   showWin();		   // 显示该window在前台
	void   clean();
	void   placeCursor(int,int);
	void   placeCursor();
	void   clear_ok();
};
extern int otdGetCh();
extern unsigned int OTDACS_HLINE;
extern unsigned int OTDACS_VLINE;
extern unsigned int OTDACS_LTEE;
extern unsigned int OTDACS_RTEE;
extern unsigned int OTDACS_ULCORNER;
extern unsigned int OTDACS_LLCORNER;
extern unsigned int OTDACS_URCORNER;
extern unsigned int OTDACS_LRCORNER;
extern unsigned int OTDACS_PLUS;
extern unsigned int OTDACS_BTEE;
extern unsigned int OTDACS_TTEE;
int    showmsg(char *,...);
#endif

