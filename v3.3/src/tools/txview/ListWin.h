/***************************************************************************
 *** 程序作者 : Shi Shenghui  石升辉                                      **
 *** 日    期 : 2004-3-2                                                * *
 *** 所属模块 :                                                          ***
 *** 程序名称 : ListWin.h                                                ***
 *** 程序作用 :                                                          ***
 *** 使用注意 :                                                          ***
 ***************************************************************************/

#ifndef _LISTWIN_H_
#define _LISTWIN_H_


#include "CWin.h"

//Modified by SSH,2004.3.1,增加允许的最大菜单项个数


#define OTD_LIST_MAX_COLUMNS	48//LIST最大列数
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ALIGN_CENTER 2
#define LIST_CANCEL -1 //返回取消选择
#define LIST_PRINT -2 //返回打印

/*因为List中的Item个数难以确定,所以采用链表存储，为了
提高查找速度，对每个元素都存储BLOCKITEMCOUNT个Item
*/
#define BLOCKITEMCOUNT 20  
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
class ListItem {
  private:
    char  *caMenuStr[OTD_LIST_MAX_COLUMNS];
    char  *caActionStr;
    int  nIndex;//在List中的位置
  public:
    ListItem();
    ~ListItem();
    void setSubItem(int pIndex,char *pText);
    char *getSubItem(int pIndex);
    void setAction(char *pAction);
    char *getAction();
    int getIndex();
    void setIndex(int pIndex);
    
};

struct _t_list_block{
	ListItem *block[BLOCKITEMCOUNT];
	struct _t_list_block *next;
}	;
typedef struct _t_list_block ListBlock;

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
class ListWin : public CWin {
  private:
    char *pcColumnText[OTD_LIST_MAX_COLUMNS];//题头文本
    int  nColumnAlignment[OTD_LIST_MAX_COLUMNS];//列对齐方式
    int  nColumnWidth[OTD_LIST_MAX_COLUMNS];//列宽
    int nColumnSumFmt[OTD_LIST_MAX_COLUMNS];//合计格式
    double dColumnSum[OTD_LIST_MAX_COLUMNS];//合计格式
    int  nColumnCount;//列数
    int  nItemCount;//行数
    int  nBorder;//是否显示遍框
  
    int  bAllowMultiSelected;//多行选择，保留
    ListBlock mListBlock;//Item的分组链表
    int nListIndex;//选择项
    
    //sBeginLine ,sBeginCol,sLines,sCols使用CWin的值
    
    int  nFirstIndex;//第一个可见项
    int  nFirstColumn;//第一个可见列
    int  getVisableColumns();
    int  getVisableLines();
	WINDOW  *poMenuWin;
  public:
  	friend class ListItem; 
    ListWin(int pLines,int pCols,int pBeginLine,int pBeginCol,int pBorder=1,int bSubWin=0);
    ~ListWin();
    //设置列表头
    void setColumnCount(int pCount);
    void setColumn(int pIndex,char *pText,int pWidth=10,int pAlignment=ALIGN_LEFT);
    int getColumnCount();
    int getColumnWidth(int pIndex);
    int resetColumnWidth(int pIndex,int nWidth);/****重新设置列长度****/
    char *getColumnText(int pIndex);
    //有关Item
    ListItem *addListItem(char *pAction=NULL,int pIndex=-1);
    ListItem *getListItem(int pIndex);
    ListItem *getListItem(char *pAction);//根据Action内容查找到第一个Item
    ListItem *setListItem(int pIndex,int pColumn, char *pText);
    void     removeListItem(int pIndex=-1);
    void     clearListItem();
    int     getListCount();
    //有关ListWin的显示
    void   addVScroll();//显示上下滚动
    void   addHScroll();//显示左右滚动
    void   ensureVisable(int pIndex);//使某个item可见
    void   selectItem(int pIndex);//选中某个Item
    ListItem *getSelectedItem();//得到选中的Item
    void  showList(int silent=0);
    void  showSum();/****在底部显示合计信息****/
    void  setSumColumn(int id,int iIntLen,int iDecLen);
    int   showMenu(char *pcMenuStr);/****在底部显示选择信息****/
    
    void  doPageDown();
    void  doPageUp();
    void  doScrollRight();
    void  doScrollLeft();
    void  doScrollUp();
    void  doScrollDown(); 
    void  doGoHome();
    void  doGoEnd(); 
    //Add by SSH,2004.7.8,增加编辑功能
    void  doEdit();
    int   work();//为了与其他控件兼容，在此返回值不是一个指向Item的指针
    		//而是他的Index,若为负数需要给调用者处理

};



#endif//end if LISTWIN

