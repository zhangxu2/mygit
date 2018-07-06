#ifndef __INPUT_WIN_H__
#define __INPUT_WIN_H__
#define INPUT_TMOUT 5
#include"CWin.h"
class InputWin: public CWin{
private:
	char caTraceNo[7];
	char caDate[9];
	int  sCurCol;
	int  sInputTmout;		/****等待输入超时时间****/
public:
	InputWin();
	int  setDate(char *pcDate);
	int  show();
	int  input();
	char *pcGetTraceNo();
	int  nClearTraceNo();
};
#endif
