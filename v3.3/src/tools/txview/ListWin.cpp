/***************************************************************************
 *** �������� : Shi Shenghui  ʯ����                                      **
 *** ��    �� : 2004-3-2                                                * *
 *** ����ģ�� :                                                          ***
 *** �������� : ListWin.c                                                ***
 *** �������� : �����������ҹ������б��                                                         ***
 *** ʹ��ע�� :                                                          ***
 ***************************************************************************/
#include"ListWin.h"
#include"stdio.h"
#include"string.h"
int otdGetCh();

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
ListItem::ListItem() {
  caActionStr=NULL;
  for(int i=0;i<OTD_LIST_MAX_COLUMNS;i++)
  	caMenuStr[i]=NULL;
  nIndex=-1;
}
ListItem::~ListItem() {
	if(caActionStr!=NULL){
		delete caActionStr;
		caActionStr=NULL;
	}
	for(int i=0;i<OTD_LIST_MAX_COLUMNS;i++){
  		if(caMenuStr[i]!=NULL){
  			delete caMenuStr[i];
  			caMenuStr[i]=NULL;
  		}
  	}	
}
void ListItem::setSubItem(int pIndex,char *pText){
	if(pIndex>=OTD_LIST_MAX_COLUMNS || pIndex<0)
		return;
	if(pText==NULL)
		return;
	if(caMenuStr[pIndex]!=NULL){
		delete caMenuStr[pIndex];
	}
	caMenuStr[pIndex]=new char[strlen(pText)+1];
	strcpy(caMenuStr[pIndex],pText);
}
char *ListItem::getSubItem(int pIndex){
	if(pIndex>=OTD_LIST_MAX_COLUMNS || pIndex<0)
		return(NULL);
	return(caMenuStr[pIndex]);
}

void ListItem::setAction(char *pAction){
	if(pAction==NULL)
		return;
	if(caActionStr!=NULL)
		delete caActionStr;
	caActionStr=new char[strlen(pAction)+1];
	strcpy(caActionStr,pAction);
}
char *ListItem::getAction(){
	return(caActionStr);
}
int ListItem::getIndex(){
	return nIndex;
}
void ListItem::setIndex(int pIndex){
	nIndex=pIndex;
}
/****************************************
*����ΪListItem�Ĳ���			*
*����ΪListWin�Ĳ��� 			*
****************************************/    
ListWin::ListWin(int pLines,int pCols,int pBeginLine,int pBeginCol,int pBorder,int bSubWin):CWin(){
	int id;
	poMenuWin=NULL;
	if(pBeginCol % 2==0)
		pBeginCol++;//�������п�ʼ����ΪҪ����
	if(pCols %2!=0)
		pCols++;
	resetWin(pLines,pCols,pBeginLine,pBeginCol);
	
	//����
	nBorder=pBorder;
	/*if(nBorder){
		box(wID,0,0);	
		//��һ������
		//mvwaddch(wID,2,0,OTDACS_LTEE);
		//mvwaddch(wID,2,pCols-1,OTDACS_RTEE);
		mvwhline(wID,2,1,'-',pCols-2);
	}*/
	//��ʼ������
		
		
	for(id=0;id<OTD_LIST_MAX_COLUMNS;id++){
		pcColumnText[id]=NULL;
		nColumnAlignment[id]=0;
		nColumnWidth[id]=0;
	}
	for(id=0;id<BLOCKITEMCOUNT;id++){
		mListBlock.block[id]=NULL;
	}
	mListBlock.next=NULL;
	
	nColumnCount=0;
	nItemCount=0;
	bAllowMultiSelected=0;//���������ѡ��
	nListIndex=0;
	nFirstColumn=0;
	nFirstIndex=0;
	memset(nColumnSumFmt,'\0',sizeof(nColumnSumFmt));
	memset(dColumnSum,'\0',sizeof(dColumnSum));
}
ListWin::~ListWin(){
	int id;
	
	//��ʼ������
	for(id=0;id<OTD_LIST_MAX_COLUMNS;id++){
		if(pcColumnText[id]!=NULL)
			delete pcColumnText[id];
		
	}
	ListBlock *pBlock,*pBakBlock;
	
	for(id=0;id<BLOCKITEMCOUNT;id++){
		if(mListBlock.block[id]!=NULL)
			delete mListBlock.block[id];
			
	}
	pBlock=mListBlock.next;
	
	while(pBlock!=NULL){
		for(id=0;id<BLOCKITEMCOUNT;id++){
		if(pBlock->block[id]!=NULL)
			delete pBlock->block[id];
			
		}
		pBakBlock=pBlock;
		pBlock=pBlock->next;
		delete pBakBlock;
	}
	//writelog(LOG_INFO,"ListWin deleted!");
	if(poMenuWin!=NULL){
		delete poMenuWin;
		poMenuWin=NULL;
	}
	
}
void ListWin::setColumnCount(int pCount){
	int id;
	if(nColumnCount>0){//�Ѿ����б�ͷ
		clearListItem();//ɾ������item
		for(id=0;id<nColumnCount;id++){
			if(pcColumnText[id]!=NULL){
				delete pcColumnText[id];
				pcColumnText[id]=NULL;
			}
		}
	}
	nColumnCount=pCount;
}
void ListWin::setColumn(int pIndex,char *pText,int pWidth,int pAlignment){
	if(pIndex<0 || pIndex>=nColumnCount){
		return;
	}
	if(pcColumnText[pIndex]!=NULL){
		delete pcColumnText[pIndex];
	}
	pcColumnText[pIndex]=new char[pWidth+1];
	pcColumnText[pIndex][pWidth]='\0';
	strncpy(pcColumnText[pIndex],pText,pWidth);
	nColumnWidth[pIndex]=pWidth;
	nColumnAlignment[pIndex]=pAlignment;
}
int ListWin::resetColumnWidth(int pIndex,int pWidth){
	char *pcTmp;
	if(pIndex<0 || pIndex>=nColumnCount){
		return(-1);
	}
	pcTmp=pcColumnText[pIndex];
	pcColumnText[pIndex]=new char[pWidth+1];
	pcColumnText[pIndex][pWidth]='\0';
	if(pcTmp!=NULL){
		strncpy(pcColumnText[pIndex],pcTmp,pWidth);
	}
	nColumnWidth[pIndex]=pWidth;
	if(pcTmp!=NULL){
		delete pcTmp;
	}
	return(0);
}
int ListWin::getColumnCount(){
	return(nColumnCount);
}
int ListWin::getColumnWidth(int pIndex){
	if(pIndex<0 || pIndex>nColumnCount-1)
		return 0;
	return nColumnWidth[pIndex];
	
}
char * ListWin::getColumnText(int pIndex){
	if(pIndex<0 || pIndex>nColumnCount-1)
		return NULL;
	return pcColumnText[pIndex];
	
}
ListItem *ListWin::addListItem(char *pAction,int pIndex){
	int blockId;//���ڿ�
	int itemId;//����λ��
	int id;
	int sCount;
	ListItem *pListItem;
	ListBlock *pListBlock;
	ListBlock *pScanBlock;//���ڱ���
	if(pIndex<0 || pIndex>nItemCount)
		pIndex=nItemCount;//Ĭ�ϼӵ����
	blockId=pIndex /BLOCKITEMCOUNT;
	itemId=pIndex % BLOCKITEMCOUNT;
	//pListBlock=&mListBlock;
	pListBlock=&mListBlock;
	while(pListBlock->next!=NULL)
		pListBlock=pListBlock->next;
	/*mvprintw(10,10,"cnt=%d",nItemCount);
		getch();
		touchwin(stdscr);
		wrefresh(stdscr);*/
	if(nItemCount % BLOCKITEMCOUNT ==0 && nItemCount>0){//��Ҫ�·���һ����
		
		
		pListBlock->next=new ListBlock;
		pListBlock=pListBlock->next;
		//��ʼ��
		for(id=0;id<BLOCKITEMCOUNT;id++){
			pListBlock->block[id]=NULL;
		}
		pListBlock->next=NULL;
	}
	//����ʱ��pListBlockָ�����һ����,�ҿ϶��пռ�
	if(pIndex==nItemCount){
		//һ��������ӵ����
		pListBlock->block[itemId]=new ListItem();
		if(pAction!=NULL){
			pListBlock->block[itemId]->setAction(pAction);
		}
		pListBlock->block[itemId]->setIndex(pIndex);
		nItemCount++;
		//writelog(LOG_INFO,"%s:%d,!ItemCount=%d,",__FILE__,__LINE__,nItemCount);	
		pListItem=pListBlock->block[itemId];
		return(pListBlock->block[itemId]);
	}else{
		//���Ǽ��������������ή��Ч�ʣ�Ӧ��������ʹ��
		//�����ҵ�����λ��
		pScanBlock=&mListBlock;
		ListItem *pBakItem,*pBakItem2;
		for(id=0;id<blockId;id++)
			pScanBlock=pScanBlock->next;
		pListBlock=pScanBlock;
		//��ʱpListBlockָ����Ҫ����Ŀ�
		pBakItem=pScanBlock->block[itemId];
		id=pIndex+1;
		while(id<=nItemCount){
			if(id % BLOCKITEMCOUNT==0){
				//��Ҫ����һ����
				pScanBlock=pScanBlock->next;
			}
			pBakItem2=pScanBlock->block[id % BLOCKITEMCOUNT];
			
			pScanBlock->block[id % BLOCKITEMCOUNT]=pBakItem;
			pScanBlock->block[id % BLOCKITEMCOUNT]->setIndex(pScanBlock->block[id % BLOCKITEMCOUNT]->getIndex()+1);
			pBakItem=pBakItem2;
			id++;
			
		}
		//��ʱpListBlockΪ��Ҫ����Ŀ飬itemIdΪ��Ҫ�����λ��
		pListBlock->block[itemId]=new ListItem();
		if(pAction!=NULL){
			pListBlock->block[itemId]->setAction(pAction);
		}
		pListBlock->block[itemId]->setIndex(pIndex);
		nItemCount++;
		return(pListBlock->block[itemId]);
	}
}
ListItem *ListWin::getListItem(int pIndex){
	int blockId,itemId,id;
	ListBlock *pListBlock;
	if(pIndex<0 || pIndex>=nItemCount)
		pIndex=0;
	blockId=pIndex /BLOCKITEMCOUNT;
	itemId=pIndex % BLOCKITEMCOUNT;
	
	pListBlock=&mListBlock;
	id=0;
	while(id<blockId){
		id++;
		pListBlock=pListBlock->next;
	}
	return(pListBlock->block[itemId]);
}
ListItem *ListWin::getListItem(char *pAction){
	int id;
	ListBlock *pListBlock;
	pListBlock=&mListBlock;
	id=0;
	while(id<nItemCount){
		if(strcmp(pListBlock->block[id % BLOCKITEMCOUNT]->getAction(),pAction)==0)
			break;
		id++;
		if(id % BLOCKITEMCOUNT==0)
			pListBlock=pListBlock->next;
	}
	if(id<nItemCount){
		return(pListBlock->block[id % BLOCKITEMCOUNT]);
	}else{
		return NULL;
	}
}
ListItem *ListWin::setListItem(int pIndex,int pColumn, char *pText){
	ListItem *pListItem;
	pListItem=getListItem(pIndex);
	if(pListItem==NULL)
		return(NULL);
	pListItem->setSubItem(pColumn,pText);
	return(pListItem);
}
void    ListWin::removeListItem(int pIndex){
	int id,blockId,itemId;
	ListItem *pListItem;
	ListItem *pBakItem;
	ListBlock *pBakBlock;
	int sCount;
	if(nItemCount==0)
		return ;
	if(pIndex<0 || pIndex>=nItemCount)
		pIndex=nItemCount-1 ;
	/*mvprintw(10,10,"pIndex=%d",pIndex);
	touchwin(stdscr);
	wrefresh(stdscr);
	getch();*/
	pListItem=getListItem(pIndex);
	delete pListItem;
	//�Ѻ����Item�ƶ���ǰ����
	blockId=pIndex /BLOCKITEMCOUNT;
	itemId=pIndex % BLOCKITEMCOUNT;
	id=0;
	pBakBlock=&mListBlock;
	while(id<blockId){
		id++;
		pBakBlock=pBakBlock->next;
	}
	//pBakBlockָ��Ҫɾ��item���ڿ�
	id=pIndex;
	while(id<=nItemCount-2){
		if((id+1) % BLOCKITEMCOUNT==0){
			pBakBlock->block[id % BLOCKITEMCOUNT]=pBakBlock->next->block[(id+1) % BLOCKITEMCOUNT];
			pBakBlock->block[id % BLOCKITEMCOUNT]->setIndex(pBakBlock->block[id % BLOCKITEMCOUNT]->getIndex()-1);
			pBakBlock=pBakBlock->next;
		}else{
			pBakBlock->block[id % BLOCKITEMCOUNT]=pBakBlock->block[(id+1) % BLOCKITEMCOUNT];
			pBakBlock->block[id % BLOCKITEMCOUNT]->setIndex(pBakBlock->block[id % BLOCKITEMCOUNT]->getIndex()-1);
		}
		id++;
	}
	pBakBlock->block[(nItemCount-1)%BLOCKITEMCOUNT]=NULL;
	nItemCount--;
	if(nItemCount % BLOCKITEMCOUNT==0){
		//���һ������ˣ�ɾ����
		id=nItemCount-1;
		blockId=id / BLOCKITEMCOUNT;
		id=0;
		pBakBlock=&mListBlock;
		while(id<blockId){
			id++;
			pBakBlock=pBakBlock->next;
		}
		delete pBakBlock->next;
		pBakBlock->next=NULL;
	}
	if(nListIndex>nItemCount-1)
		nListIndex=nItemCount-1;
}		
void     ListWin::clearListItem()			{
	while(nItemCount>0)
		removeListItem();
	nItemCount=0;
	nListIndex=0;
	nFirstColumn=0;
	nFirstIndex=0;
}
int     ListWin::getListCount(){
	return(nItemCount);
}
///////////////    	
int  ListWin::getVisableColumns(){
  //ȡ�ÿɼ�������
  int maxX,maxY;
  int visableColumns;
  int nWidth;
  int n;
  nWidth=0;
  visableColumns=0;
  n=nFirstColumn;
  getmaxyx(wID,maxY,maxX);
  //printw("\nn=%d,w=%d,nWidth=%d,maxX=%d",n,nColumnWidth[n],nWidth,maxX);
  	
  while(nWidth+nColumnWidth[n]<=maxX-2){
  	//printw("\nn=%d,w=%d,nWidth=%d,maxX=%d",n,nColumnWidth[n],nWidth,maxX);
  	
  	nWidth+=nColumnWidth[n];
  	n++;
  	if(n>nColumnCount-1)
  		break;
  }	
  return(n-nFirstColumn);

}
int  ListWin::getVisableLines(){
  //ȡ�ÿɼ�������
  int maxX,maxY;
  getmaxyx(wID,maxY,maxX);
  return(maxY-4);
}
////////////////
void ListWin::addVScroll(){
  //����������Ҫ���¹���ȷ�����
  int maxX,maxY;
  int visableLines;
  if(!nBorder)
  	return;//����ʾ���
  getmaxyx(wID,maxY,maxX);
  visableLines=getVisableLines();
  wattrset(wID,A_NORMAL);
  int centPos;
  centPos=(maxX-1)/2;
  if(centPos %2!=0)
  	centPos++;
  int pageNo,pageCount,pageLines;
  pageLines=maxY-4;
  pageCount=(nItemCount)/pageLines+1;
  pageNo=(nListIndex)/pageLines+1;
  if(nFirstIndex>0){//�����ڿ�ʼ���������滭һ���ϼ�ͷ
  	mvwaddstr(wID,2,centPos,"��");
  }else{
  	mvwaddstr(wID,2,centPos,"--");
  }
  if(nFirstIndex+visableLines<nItemCount){//���������棬�����滭һ���¼�ͷ
  	mvwaddstr(wID,maxY-1,centPos,"��");
  }else{
  	mvwaddch(wID,maxY-1,centPos,OTDACS_HLINE);
  	mvwaddch(wID,maxY-1,centPos+1,OTDACS_HLINE);
  }  
  //��ӡҳ��
  char tmpBuf[10];
  int tmpLen;
  
  sprintf(tmpBuf,"%d/%d",pageNo,pageCount);
  tmpLen=strlen(tmpBuf);
  mvwaddstr(wID,maxY-1,maxX-1-tmpLen-2,tmpBuf);
  if(tmpLen<5){
	  for(int i=0;i<5-tmpLen;i++){
	  	mvwaddch(wID,maxY-1,maxX-6+i-2,OTDACS_HLINE);
	  }
  }
  	
  
}   		
//////////////////
void   ListWin::addHScroll(){
  //����������Ҫ���ҹ���ȷ�����
  int maxX,maxY;
  int visableColumns;
  if(!nBorder)
  	return;//����ʾ���
  visableColumns=getVisableColumns();
  getmaxyx(wID,maxY,maxX);
  
  wattrset(wID,A_NORMAL);
  if(nFirstColumn>0){//�����ڿ�ʼ���������滭һ����ͷ
  	mvwaddch(wID,maxY-1,1,'<');
  }else{
  	mvwaddch(wID,maxY-1,1,OTDACS_HLINE);
  }
  if(nFirstColumn+visableColumns<nColumnCount){//���������棬�����滭һ���¼�ͷ
  	mvwaddch(wID,maxY-1,maxX-2,'>');
  }else{
  	mvwaddch(wID,maxY-1,maxX-2,OTDACS_HLINE);
  }
  
}   				
//////////////////		
void   ListWin::ensureVisable(int pIndex){//ʹĳ��item�ɼ�
  int visableLines;
  visableLines=getVisableLines();
  if(pIndex<0 || pIndex>nItemCount-1)
  	return ;
  if(pIndex<nFirstIndex)
  	nFirstIndex=pIndex;
  else if( nFirstIndex+visableLines<=pIndex)
  	nFirstIndex=pIndex-visableLines+1;
  showList();
}
void   ListWin::selectItem(int pIndex){//ѡ��ĳ��Item
  if(pIndex<0 || pIndex>nItemCount-1)
  	return;
  nListIndex=pIndex;
  //showList();
}
ListItem *ListWin::getSelectedItem(){//�õ�ѡ�е�Item
  return(getListItem(nListIndex));
}
void  ListWin::showList(int silent){
  int pLine,pCol;
  int pLeft;
  int visableLines,visableCols;
  int len;
  int id;
  int maxX,maxY;
  
  getmaxyx(wID,maxY,maxX);
  //����
  
  if(nBorder){
		wattrset(wID,A_NORMAL);
		box(wID,0,0);	
		//��һ������
		//mvwaddch(wID,2,0,OTDACS_LTEE);
		//mvwaddch(wID,2,pCols-1,OTDACS_RTEE);
		mvwhline(wID,2,1,'-',maxX-2);
	}
  ListItem *pListItem;
  visableLines=getVisableLines();
  visableCols=getVisableColumns();
  //placeCursor();
  addHScroll();
  addVScroll();
  
  //printw("vL=%d,vC=%d,fl=%d,fc=%d,col=%d\n",visableLines,visableCols,nFirstIndex,nFirstColumn,nColumnCount);
  //touchwin(stdscr);
  //wrefresh(stdscr);
  //endwin();
  //exit(0);
  
  pLeft=1;
  wattrset(wID,A_NORMAL);
 
  for(pCol=nFirstColumn;pCol<nFirstColumn+visableCols;pCol++){
	//ѭ����ʾÿһ����ͷ
  	len=strlen(pcColumnText[pCol]);
  	if(nColumnAlignment[pCol]==ALIGN_LEFT){
  		//�����
  		if(len<nColumnWidth[pCol]){
  			mvwaddstr(wID,1,pLeft,pcColumnText[pCol]);
  			//�Ҳ��ո�
  			for(id=pLeft+len;id<pLeft+nColumnWidth[pCol];id++)
  				mvwaddch(wID,1,id,' ');
  			
  		}else{
  			mvwaddnstr(wID,1,pLeft,pcColumnText[pCol],nColumnWidth[pCol]);
  		}
  		
  	}else if(nColumnAlignment[pCol]==ALIGN_RIGHT){
  		//�Ҷ���,����һ���ֽڿ�λ
  		if(len<nColumnWidth[pCol]-1){
  			//�󲹿ո�
  			for(id=pLeft;id<pLeft+nColumnWidth[pCol]-1-len;id++)
  				mvwaddch(wID,1,id,' ');
  			mvwaddstr(wID,1,id,pcColumnText[pCol]);
  			
  		}else{
  			mvwaddnstr(wID,1,pLeft,pcColumnText[pCol],nColumnWidth[pCol]-1);
  		}
  		mvwaddch(wID,1,pLeft+nColumnWidth[pCol]-1,' ');
  	}else if(nColumnAlignment[pCol]==ALIGN_CENTER){
  		//���У���ʱ�������Ժ�ʵ��
  	}//end if
  	pLeft+=nColumnWidth[pCol];
  }//end for pCol
  //��������һ����Ŀ�Ĳ���
  while(pLeft<maxX-1){
  	mvwaddch(wID,1,pLeft,' ');
  	pLeft++;
  }
  
  int cursorLine;
  cursorLine=3;
  
  for(pLine=nFirstIndex;pLine<nFirstIndex+visableLines;pLine++){
  	//ѭ����ʾÿһ��
  	
  	if(pLine>=nItemCount){
  		//��ӡ�հ���
  		wattrset(wID,A_NORMAL);
  		mvwhline(wID,pLine-nFirstIndex+3,1,' ',maxX-2);
  		continue;
  	}
  	pLeft=1;
  	pListItem=getListItem(pLine);
  	if(pLine==nListIndex){
		if(silent){
  			wattrset(wID,A_NORMAL);
		}else{
	  		wattrset(wID,A_REVERSE);
		}
		cursorLine=pLine-nFirstIndex+3;
  	}else
  		wattrset(wID,A_NORMAL);
  	
  	for(pCol=nFirstColumn;pCol<nFirstColumn+visableCols;pCol++){
  		//ѭ����ʾÿһ��
  		len=strlen(pListItem->getSubItem(pCol));
  		if(nColumnAlignment[pCol]==ALIGN_LEFT){
  			//�����
  			if(len<nColumnWidth[pCol]){
  				mvwaddstr(wID,pLine-nFirstIndex+3,pLeft,pListItem->getSubItem(pCol));
  				//�Ҳ��ո�
  				for(id=pLeft+len;id<pLeft+nColumnWidth[pCol];id++)
  					mvwaddch(wID,pLine-nFirstIndex+3,id,' ');
  				
  			}else{
  				mvwaddnstr(wID,pLine-nFirstIndex+3,pLeft,pListItem->getSubItem(pCol),nColumnWidth[pCol]);
  			}
  			
  		}else if(nColumnAlignment[pCol]==ALIGN_RIGHT){
  			//�Ҷ���,����һ���ַ���λ
  			if(len<nColumnWidth[pCol]-1){
  				//�󲹿ո�
  				for(id=pLeft;id<pLeft+nColumnWidth[pCol]-1-len;id++)
  					mvwaddch(wID,pLine-nFirstIndex+3,id,' ');
  				mvwaddstr(wID,pLine-nFirstIndex+3,id,pListItem->getSubItem(pCol));
  				
  			}else{
  				mvwaddnstr(wID,pLine-nFirstIndex+3,pLeft,pListItem->getSubItem(pCol),nColumnWidth[pCol]-1);
  			}
  			mvwaddch(wID,pLine-nFirstIndex+3,pLeft+nColumnWidth[pCol]-1,' ');
  		}else if(nColumnAlignment[pCol]==ALIGN_CENTER){
  			//���У���ʱ�������Ժ�ʵ��
  		}//end if
  		pLeft+=nColumnWidth[pCol];
  	}//end for pCol
  	//��������һ����Ŀ�Ĳ���
  	while(pLeft<maxX-1){
  		mvwaddch(wID,pLine-nFirstIndex+3,pLeft,' ');
  		pLeft++;
  	}
  }//end for pLine
  touchwin(wID);
  //placeCursor();
  wmove(wID,cursorLine,0);
  wrefresh(wID);	
}
void  ListWin::doGoHome(){
  if(nItemCount<=0)
  	return;
  nFirstIndex=0;
  nListIndex=nFirstIndex;
  showList();
}	
void  ListWin::doGoEnd(){
  if(nItemCount<=0)
  	return;
  int visableLines;
  visableLines=getVisableLines();
  nFirstIndex=(nItemCount/visableLines)*visableLines;
  if(nFirstIndex>nItemCount-1)
  	nFirstIndex=nItemCount-1;
  if(nFirstIndex<0)
  	nFirstIndex=0;
  
  nListIndex=nFirstIndex;
  showList();
}	
void  ListWin::doPageDown(){
  int visableLines;
  visableLines=getVisableLines();
  if(nFirstIndex+visableLines>nItemCount)
  	return;
  nFirstIndex+=visableLines;
  nListIndex=nFirstIndex;
  showList();
}	
void  ListWin::doPageUp(){
  int visableLines;
  visableLines=getVisableLines();
  if(nFirstIndex-visableLines<=0)
  	nFirstIndex=0;
  else
  	nFirstIndex-=visableLines;
  nListIndex=nFirstIndex;
  showList();
}
void  ListWin::doScrollRight(){
  int visableColumns;
  visableColumns=getVisableColumns();
  if(nFirstColumn+visableColumns>=nColumnCount)
  	return;
  nFirstColumn++;
 
  showList();
}

void  ListWin::doScrollLeft(){
  int visableColumns;
  visableColumns=getVisableColumns();
  if(nFirstColumn<=0)
  	nFirstColumn=0;
  else
  	nFirstColumn--;
  showList();
}
void  ListWin::doEdit(){
  int visableColumns;
  char caTmpBuf[1024];
  int sBeginLine,sBeginCol;
  int sEditCol;
  int sTotalLen;
  int iInputChar=0;
  int sCurEditPos=0;
  int nRes;
  if(nItemCount<=0)
  	return;
  if(nListIndex<0)
  	nListIndex=0;
  nFirstColumn=0;
  //�༭����ӵ�һ�п�ʼ
  ListItem *pListItem=getListItem(nListIndex);
  sEditCol=0;
  
  while(1){
  	//���Ե�ǰ�༭������
  	nFirstColumn=sEditCol;
    showList();
    sTotalLen=getColumnWidth(sEditCol);
    sBeginLine=nListIndex-nFirstIndex+3;
    sBeginCol=1;
    if(pListItem->getSubItem(sEditCol)==NULL)
    	caTmpBuf[0]='\0';
    else
    	strcpy(caTmpBuf,pListItem->getSubItem(sEditCol));
    for(int i=strlen(caTmpBuf);i<sTotalLen;i++)
    	caTmpBuf[i]=' ';
    caTmpBuf[sTotalLen]='\0';
    wattrset(wID,A_UNDERLINE);
    wattron(wID,A_REVERSE);
    mvwaddstr(wID,sBeginLine,sBeginCol,caTmpBuf);
    wmove(wID,sBeginLine,sBeginCol);
    touchwin(wID);
    wrefresh(wID);
    //��������
    sCurEditPos=sBeginCol;
    nRes=0;//0-�˳����ѭ����1-�������ѭ��,2-�����ڲ�ѭ��
    while(1){
    	if(iInputChar==0)
    		iInputChar=otdGetCh();
    	switch(iInputChar){
    	case KEY_ENTER:
    	case '\r':
    		pListItem->setSubItem(sEditCol,caTmpBuf);
    		wattrset(wID,A_NORMAL);
    		showList();
    		nRes=0;//�˳����ѭ��
    		iInputChar=0;
    		break;
    	case '\t':
    		pListItem->setSubItem(sEditCol,caTmpBuf);
    		wattrset(wID,A_NORMAL);
    		sEditCol++;
    		if(sEditCol>nColumnCount-1)
    			sEditCol=0;
    		nRes=1;//�������ѭ��
    		iInputChar=0;
    		break;
    	case KEY_LEFT:
    		if(sCurEditPos>sBeginCol){
    			sCurEditPos--;
    			wmove(wID,sBeginLine,sCurEditPos);
    			wrefresh(wID);
    			nRes=2;//�����ڲ�ѭ��
    			iInputChar=0;
    			break;
    		}else{
    			//����������һ���ַ�����˵���һ��������
    			if(sEditCol==0){
    				nRes=2;//�����ڲ�ѭ��
    				iInputChar=0;
    				break;
    			}
    			sEditCol--;
    			nRes=1;//�������ѭ��
    			iInputChar=0;
    			break;
    		}
    		break;
    	case KEY_BACKSPACE:
    		if(sCurEditPos>sBeginCol){
    			sCurEditPos--;
    			for(int i=sCurEditPos-sBeginCol;i<sTotalLen-1;i++)
    				caTmpBuf[i]=caTmpBuf[i+1];
    			caTmpBuf[sTotalLen-1]=' ';
    			mvwaddstr(wID,sBeginLine,sBeginCol,caTmpBuf);
	    		wmove(wID,sBeginLine,sCurEditPos);
	    		//touchwin(wID);
			    wrefresh(wID);
			    iInputChar=0;
			    nRes=2;//�����ڲ�ѭ��
			    break;
    		}
    		iInputChar=0;
    		nRes=2;//�����ڲ�ѭ��
    		break;
    	case KEY_RIGHT:
    		if(sCurEditPos<sTotalLen+sBeginCol-1){
    			sCurEditPos++;
    			wmove(wID,sBeginLine,sCurEditPos);
    			wrefresh(wID);
    			nRes=2;//�����ڲ�ѭ��
    			iInputChar=0;
    			break;
    		}else{
    			//��������ұ�һ���ַ���ǰ������һ��������
    			if(sEditCol==nColumnCount-1){
    				nRes=2;//�����ڲ�ѭ��
    				iInputChar=0;
    				break;
    			}
    			sEditCol++;
    			nRes=1;//�������ѭ��
    			iInputChar=0;
    			break;
    		}
    		iInputChar=0;
    		break;
    	case 27:
    	case KEY_DC:
    		wattrset(wID,A_NORMAL);
    		showList();
    		nRes=0;//�˳����ѭ��
    		iInputChar=0;
    		break;
    	case KEY_HOME:
    		memset(caTmpBuf,' ',sTotalLen);
    		mvwaddstr(wID,sBeginLine,sBeginCol,caTmpBuf);
    		wmove(wID,sBeginLine,sBeginCol);
    		//touchwin(wID);
		    wrefresh(wID);
		    sCurEditPos=sBeginCol;
		    iInputChar=0;
		    nRes=0;//�����ڲ�ѭ��
		    break;
		default:
			if(sCurEditPos>=sTotalLen+sBeginCol-1){
				nRes=2;//�����ڲ�ѭ��
				iInputChar=0;
				break;
			}
			caTmpBuf[sCurEditPos-sBeginCol]=iInputChar;	
    		mvwaddstr(wID,sBeginLine,sBeginCol,caTmpBuf);
    		wmove(wID,sBeginLine,sCurEditPos+1);
    		sCurEditPos++;
    		//touchwin(wID);
		    wrefresh(wID);
		    iInputChar=0;
		    nRes=2;//�����ڲ�ѭ��
		    break;
		}
		if(nRes==2)//�����ڲ�ѭ��
			continue;
		else
			break;
	}//end �ڲ�while
	if(nRes==1)
		continue;//�������ѭ��
	else
		break;
  }//end ���while
 
}
void  ListWin::doScrollUp(){
  nListIndex--;
  if(nListIndex<0)
  	nListIndex=0;
  if(nListIndex<nFirstIndex)
  	nFirstIndex=nListIndex;
  showList();
}
void  ListWin::doScrollDown(){
  int visableLines;
  visableLines=getVisableLines();
  nListIndex++;
  if(nListIndex>nItemCount-1)
  	nListIndex=nItemCount-1;
  if(nListIndex>=nFirstIndex+visableLines)
  	nFirstIndex=nListIndex-visableLines+1;
  showList();
}

/////////////////////////////////////////////////
int   ListWin::work(){
  int  iInputChar;

  showList();
  iInputChar=0;
  while(1) {
      if(iInputChar==0)
          iInputChar = otdGetCh();
      
      switch(iInputChar) {
      //case KEY_F(2):
      //case KEY_F(4):
      //case KEY_F(9):
      //case '+'://SSH ,2004.2.28 Ӧ��������Ҫ����Ӧ+
      case 27:  // ESC ��
      case KEY_DC:
          return(WIN_ESC);
      case 'r':
      case 'R':
          return(WIN_R);
      case '\t':
          return(WIN_TAB);
      case 'o':
      case 'O':
      		iInputChar=0;
			/**�ݲ�֧�ִ�ӡ****
      		printList();
			*******************/
      		break;
      	   //return(LIST_PRINT);
      case KEY_UP:
          doScrollUp();
          iInputChar=0;
          break;

      case KEY_DOWN:
          doScrollDown();
          iInputChar=0;
          break;

      case KEY_BACKSPACE:
      case KEY_LEFT:
      	  doScrollLeft();
      	  iInputChar=0;
      	  break;
      case KEY_RIGHT:
      	  doScrollRight();
      	  iInputChar=0;
      	  break;
      case KEY_ENTER:
      case '\r':
          // �˴�����ѡ���ֵ
          return(WIN_OK);
      case KEY_PPAGE:
      case 'P':
      case 'p':
      	  doPageUp();
      	  iInputChar=0;
      	  break;
      case KEY_NPAGE:
      case 'N':
      case 'n':
      	  doPageDown();
      	  iInputChar=0;
      	  break;
      case KEY_HOME:
      case 'H':
      case 'h':
      	  doGoHome();
      	  iInputChar=0;
      	  break;
      case KEY_END:
      case 'E':
      case 'e':
      	  doGoEnd();
      	  iInputChar=0;
      	  break;
      default:
      	  iInputChar=0;
          break;
      }
  }
  return(0);
}
/****�к� ,����λ��,С��λ��***/
void ListWin::setSumColumn(int id,int iIntLen,int iDecLen){
	nColumnSumFmt[id]=iIntLen*100+iDecLen;
}
void ListWin::showSum(){
}
int ListWin::showMenu(char *pcMenuStr){
	return(0);
}
