/***************************************************************************
 *** �������� : ʯ����												   ***
 *** ��	�� : 2008-02-13											   ***
 *** ����ģ�� :														  ***
 *** �������� : CWin.h												   ***
 *** �������� :														  ***
 *** ʹ��ע�� :														  ***
 ***************************************************************************/


#ifndef _CWIN_H_
#define _CWIN_H_
#define WIN_OK	    1 				//�س����˳�
#define WIN_ESC	    2 				//ESC���˳�
#define WIN_TAB	    3				//TAB���˳� 
#define WIN_TMOUT	4 				//��ʱδ����
#define WIN_R		5 				//����R���˳�
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
	void   showWin();		   // ��ʾ��window��ǰ̨
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

