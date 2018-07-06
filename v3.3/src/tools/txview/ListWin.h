/***************************************************************************
 *** �������� : Shi Shenghui  ʯ����                                      **
 *** ��    �� : 2004-3-2                                                * *
 *** ����ģ�� :                                                          ***
 *** �������� : ListWin.h                                                ***
 *** �������� :                                                          ***
 *** ʹ��ע�� :                                                          ***
 ***************************************************************************/

#ifndef _LISTWIN_H_
#define _LISTWIN_H_


#include "CWin.h"

//Modified by SSH,2004.3.1,������������˵������


#define OTD_LIST_MAX_COLUMNS	48//LIST�������
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ALIGN_CENTER 2
#define LIST_CANCEL -1 //����ȡ��ѡ��
#define LIST_PRINT -2 //���ش�ӡ

/*��ΪList�е�Item��������ȷ��,���Բ�������洢��Ϊ��
��߲����ٶȣ���ÿ��Ԫ�ض��洢BLOCKITEMCOUNT��Item
*/
#define BLOCKITEMCOUNT 20  
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
class ListItem {
  private:
    char  *caMenuStr[OTD_LIST_MAX_COLUMNS];
    char  *caActionStr;
    int  nIndex;//��List�е�λ��
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
    char *pcColumnText[OTD_LIST_MAX_COLUMNS];//��ͷ�ı�
    int  nColumnAlignment[OTD_LIST_MAX_COLUMNS];//�ж��뷽ʽ
    int  nColumnWidth[OTD_LIST_MAX_COLUMNS];//�п�
    int nColumnSumFmt[OTD_LIST_MAX_COLUMNS];//�ϼƸ�ʽ
    double dColumnSum[OTD_LIST_MAX_COLUMNS];//�ϼƸ�ʽ
    int  nColumnCount;//����
    int  nItemCount;//����
    int  nBorder;//�Ƿ���ʾ���
  
    int  bAllowMultiSelected;//����ѡ�񣬱���
    ListBlock mListBlock;//Item�ķ�������
    int nListIndex;//ѡ����
    
    //sBeginLine ,sBeginCol,sLines,sColsʹ��CWin��ֵ
    
    int  nFirstIndex;//��һ���ɼ���
    int  nFirstColumn;//��һ���ɼ���
    int  getVisableColumns();
    int  getVisableLines();
	WINDOW  *poMenuWin;
  public:
  	friend class ListItem; 
    ListWin(int pLines,int pCols,int pBeginLine,int pBeginCol,int pBorder=1,int bSubWin=0);
    ~ListWin();
    //�����б�ͷ
    void setColumnCount(int pCount);
    void setColumn(int pIndex,char *pText,int pWidth=10,int pAlignment=ALIGN_LEFT);
    int getColumnCount();
    int getColumnWidth(int pIndex);
    int resetColumnWidth(int pIndex,int nWidth);/****���������г���****/
    char *getColumnText(int pIndex);
    //�й�Item
    ListItem *addListItem(char *pAction=NULL,int pIndex=-1);
    ListItem *getListItem(int pIndex);
    ListItem *getListItem(char *pAction);//����Action���ݲ��ҵ���һ��Item
    ListItem *setListItem(int pIndex,int pColumn, char *pText);
    void     removeListItem(int pIndex=-1);
    void     clearListItem();
    int     getListCount();
    //�й�ListWin����ʾ
    void   addVScroll();//��ʾ���¹���
    void   addHScroll();//��ʾ���ҹ���
    void   ensureVisable(int pIndex);//ʹĳ��item�ɼ�
    void   selectItem(int pIndex);//ѡ��ĳ��Item
    ListItem *getSelectedItem();//�õ�ѡ�е�Item
    void  showList(int silent=0);
    void  showSum();/****�ڵײ���ʾ�ϼ���Ϣ****/
    void  setSumColumn(int id,int iIntLen,int iDecLen);
    int   showMenu(char *pcMenuStr);/****�ڵײ���ʾѡ����Ϣ****/
    
    void  doPageDown();
    void  doPageUp();
    void  doScrollRight();
    void  doScrollLeft();
    void  doScrollUp();
    void  doScrollDown(); 
    void  doGoHome();
    void  doGoEnd(); 
    //Add by SSH,2004.7.8,���ӱ༭����
    void  doEdit();
    int   work();//Ϊ���������ؼ����ݣ��ڴ˷���ֵ����һ��ָ��Item��ָ��
    		//��������Index,��Ϊ������Ҫ�������ߴ���

};



#endif//end if LISTWIN

