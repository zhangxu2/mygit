/***************************************************************************
 *** 程序作者 : Shi Shenghui  石升辉                                      **
 *** 日    期 : 2008-05-27                                                **
 *** 所属模块 :                                                          ***
 *** 程序名称 : fbbm_comm.c                                              ***
 *** 程序作用 : 解析表达式用到的底层函数                                 ***
 *** 使用注意 :                                                          ***
 ***************************************************************************/
#include "svc_cycle.h"
#include "pub_express.h"

#define 	TRUE  	1
#define 	FALSE 	0

static CTNODE *stOperatorStack[MAX_OPERATOR_CNT];
static CTNODE *stDataStack[MAX_DATA_CNT];
static int    nOperatorIndex=-1;
static int    nDataIndex=-1;
static int    nQuotFlag=0;

#define STR2INT(ptr) ((long)(*( (double *)(ptr) )  ) )
#define STR2DBL(ptr) ((*( (double *)(ptr) )  ) )
#define DBLZERO   0.00001
#define STRFIX   '"'
void vDestroyStackNode(CTNODE *ct);

int nFun_zip();
int nFun_mod();
int nFun_param();
int nFun_substr();
int nFun_chr();
int nFun_money();
static int nComputeBaseExpress(char *operator);

/**
函数名:vInitExprEnv
功能  :初始化表达式计算环境(变量栈和运算符栈)
参数  :
       无
返回值:
       无
**/
void vInitExprEnv(){
	int i;
	for(i=0;i<=nOperatorIndex;i++){
		vDestroyStackNode(stOperatorStack[i]);  /**销毁运算符栈的节点**/
	}
	for(i=0;i<=nDataIndex;i++){
		vDestroyStackNode(stDataStack[i]);      /**销毁变量栈的节点**/
	}
	memset(stOperatorStack,'\0',sizeof(stOperatorStack));
	memset(stDataStack,'\0',sizeof(stDataStack));
	nOperatorIndex=nDataIndex=-1;
	nQuotFlag=0;
}
/**
函数名:nPushOperator
功能  :运算符入栈
参数  :
       operator         运算符
       length           运算符的长度
返回值:
       0/-1
**/
int nPushOperator(char *operator,int length){
	CTNODE *ct;
	if(nOperatorIndex>=MAX_OPERATOR_CNT-1){
		pub_log_error( "[%s][%d]运算符堆栈越界!",__FILE__,__LINE__);
		return(-1);
	}
	ct=(CTNODE *)malloc(sizeof(CTNODE));
	ct->type=CTYPE_OPERATOR;
	ct->data=(char *)malloc(length+1);
	memcpy(ct->data,operator,length);
	ct->data[length]='\0';
	nOperatorIndex++;
	stOperatorStack[nOperatorIndex]=ct;
	return(0);
}
/**
函数名:nPushData
功能  :运算数据入栈
参数  :
       data             数据(可能是变量或者常量)
       length           数据的长度
返回值:
       0/-1
**/
int nPushData(char *data,int length){
	CTNODE *ct;
	sw_int_t    iRc;
	char buf[1024];
	if(nDataIndex>=MAX_DATA_CNT-1){
		pub_log_error( "[%s][%d]数据堆栈越界!",__FILE__,__LINE__);
		return(-1);
	}
	ct=(CTNODE *)malloc(sizeof(CTNODE));
	if((data[0]>='0' && data[0]<='9')||
		(data[0]=='-' && data[1]>='0' && data[1]<='9')){
		/****数字型常量****/
		if(length>60){
			pub_log_error( "[%s][%d]数字型常量长度[%d]过长",__FILE__,__LINE__,length);
			return(-1);
		}
		memcpy(buf,data,length);
		buf[length]='\0';
		ct->type=CTYPE_NUMBER;
		ct->data=(char *)malloc(sizeof(double));
		*(double *)ct->data=atof(buf);
	}else if(data[0]=='#'||data[0]=='$'){
		/****字符串变量****/
		char cName[33];
		memset(cName,'\0',sizeof(cName));
		int	len = 0;
		len = sizeof(cName) - 1;
		if(length > len){
			pub_log_error( "[%s][%d]变量名太长[%d]",__FILE__,__LINE__,length);
			return(-1);
		}
		memcpy(cName,data,length);
		memset(buf, 0x0, sizeof(buf));
		iRc=get_data_len(cName,buf);
		if(iRc>=1024){
			pub_log_error( "[%s][%d]变量太长[%d]",__FILE__,__LINE__,iRc);
			return(-1);
		}
		if(iRc<0){
			iRc=0;	/****未定义的变量当做空串处理****/
		}
		ct->type=CTYPE_STRING;
		ct->data=(char *)malloc(iRc+1);
		if(iRc>0){
			memcpy(ct->data,buf,iRc);
		}
		ct->data[iRc]='\0';
	}else if(data[0]==STRFIX){
		/****字符串常量****/
		ct->type=CTYPE_STRING;
		ct->data=(char *)malloc(length-1);
		memcpy(ct->data,data+1,length-2);
		ct->data[length-2]='\0';
	}else{
		pub_log_error( "[%s][%d]非法的数据类型[%.*s]",__FILE__,__LINE__,length,data);
		return(-1);
	}
	nDataIndex++;
	stDataStack[nDataIndex]=ct;
	return(0);
}
/**
函数名:nPushString
功能  :字符串常量数据入栈(自动追加字符串引导符)
参数  :
       data             数据(可能是变量或者常量)
返回值:
       0/-1
**/
int nPushString(char *data){
	CTNODE *ct;
	int  length=strlen(data);
	if(nDataIndex>=MAX_DATA_CNT-1){
		pub_log_error( "[%s][%d]数据堆栈越界!",__FILE__,__LINE__);
		return(-1);
	}
	ct=(CTNODE *)malloc(sizeof(CTNODE));
	/****字符串常量****/
	ct->type=CTYPE_STRING;
	ct->data=(char *)malloc(length+1);
	memcpy(ct->data,data,length);
	ct->data[length]='\0';
	nDataIndex++;
	stDataStack[nDataIndex]=ct;
	return(0);
}

/**
函数名:poPopOperator
功能  :运算符出栈
参数  :
       无
返回值:
       CTNODE节点指针/NULL
**/
CTNODE *poPopOperator(){
	if(nOperatorIndex<0){
		return(NULL);
	}
	return(stOperatorStack[nOperatorIndex--]);
}
/**
函数名:poPopData
功能  :运算数据出栈
参数  :
       无
返回值:
       CTNODE节点指针/NULL
**/
CTNODE *poPopData(){
	if(nDataIndex<0){
		return(NULL);
	}
	return(stDataStack[nDataIndex--]);
}
/**
函数名:poGetTopOperator
功能  :得到栈顶运算符,但不出栈
参数  :
       无
返回值:
       CTNODE节点指针/NULL
**/
CTNODE *poGetTopOperator(){
	if(nOperatorIndex<0){
		return(NULL);
	}
	return(stOperatorStack[nOperatorIndex]);
}
/**
函数名:poGetTopData
功能  :得到栈顶运算数据,但不出栈
参数  :
       无
返回值:
       CTNODE节点指针/NULL
**/
CTNODE *poGetTopData(){
	if(nDataIndex<0){
		return(NULL);
	}
	return(stDataStack[nDataIndex]);
}
/**
函数名:vDestroyStackNode
功能  :把节点销毁
参数  :
       ct      待销毁的节点指针
返回值:
       无
**/
void vDestroyStackNode(CTNODE *ct){
	if(ct==NULL){
		return;
	}
	if(ct->data!=NULL){
		free(ct->data);
	}
	free((void *)ct);
}
/**
函数名:nPriorCmp
功能  :比较两个运算符的优先级
参数  :
       op1     第一个运算符
       op2     第二个运算符
返回值:
       1       op1的优先级>op2
       0       op1的优先级=op2
       -1      op1的优先级<op2
**/
int nPriorCmp(char *op1, char *op2)
{
	/*运算符的长度以op1为准，它是以'\0'结束的标准字符串
	优先级别从低到高 or and not  【> >= < <= != ==】 【+ -】 【* /】 ([
	)与]的优先级在computexp中特殊处理
	([为op1优先级最高,为op2优先级最低
	函数在函数的最后带上小括号，优先级与小括号相同
	**/
	char opc1,opc2;
	if((op1[0]>='a' && op1[0]<='z')||(op1[0]>='A' && op1[0]<='Z')||op1[0]=='_'){
		if(strcmp(op1,"and")==0){
			opc1='a';
		}else if(strcmp(op1,"or")==0){
			opc1='o';
		}else if(strcmp(op1,"not")==0){
			opc1='n';
		}else{
			opc1='(';
		}
	}else{
		opc1=op1[0];
	}
	if((op2[0]>='a' && op2[0]<='z')||(op2[0]>='A' && op2[0]<='Z')||op2[0]=='_'){
		if(strcmp(op2,"and")==0){
			opc2='a';
		}else if(strcmp(op2,"or")==0){
			opc2='o';
		}else if(strcmp(op2,"not")==0){
			opc2='n';
		}else{
			opc2='(';
		}
	}else{
		opc2=op2[0];
	}
	if(opc1=='(')
		return 1;
	else if(opc2=='(')
		return 1;
	if(opc1=='[')
		return 1;
	else if(opc2=='[')
		return 1;

	switch(opc1){
	case 'o':
		if(opc2=='o')
			return 0;
		else
			return -1;
		break;
	case 'a':
		if(opc2=='o')
			return 1;
		else if(opc2=='a')
			return 0;
		else
			return -1;
		break;
	case 'n':
		if(opc2=='o'||opc2=='a')
			return 1;
		else if(opc2=='n')
			return 0;
		else
			return -1;
		break;
	case '>':
	case '<':
	case '=':
	case '!':
		if(opc2=='o'||opc2=='a'||opc2=='n')
			return 1;
		else if(opc2=='>'||opc2=='<'||opc2=='!'||opc2=='=')
			return 0;
		else
			return -1;
		break;
	case '+':
	case '-':
		if(opc2=='o'||opc2=='a'||opc2=='n'||opc2=='>'||opc2=='<'||opc2=='!'||opc2=='=')
			return 1;
		else if(opc2=='+'||opc2=='-')
			return 0;
		else
			return -1;
		break;
	case '*':
	case '/':
		if(opc2=='o'||opc2=='a'||opc2=='n'||opc2=='>'||opc2=='<'||opc2=='!'||opc2=='='||opc2=='+'||opc2=='-')
			return 1;
		else if(opc2=='*'||opc2=='/')
			return 0;
		else
			return -1;
		break;
	case '(':
	case '[':
		return 1;
	}
	
	return -1;
}
/**
函数名:IsValidChar
功能  :判断是否合法字符
参数  :
       ch      字符
返回值:
       TRUE/FALSE
**/
int IsValidChar(char ch)
{
	if( ch==STRFIX){ 			/**** 	#define STRFIX  '"' 	 ***/
		if(nQuotFlag){
			nQuotFlag=0;
			return TRUE;
		}else{
			nQuotFlag=1;
			return TRUE;
		}
	}else{
		if(nQuotFlag)
			return TRUE;/*在引号内的字符不验证*/
		switch(ch){
		case '_':
		case '#':
		case '$':
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '|':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case ':':
		case ',':
		case ';':
		case '.':
		case '!':
		case '>':
		case '<':
		case '=':
			return TRUE;
		default:
			if(ch>='a' && ch<='z')
				return TRUE;
			else if(ch>='A' && ch<='Z')
				return TRUE;
			else if(ch >='0' && ch<='9')
				return TRUE;
			else
				return FALSE;
		}
	}


}
/**
函数名:pcGetWord
功能  :从一个表达式中取出一个单词
参数  :
       express 表达式的地址,NULL表示接着上次的位置开始取
返回值:
       单词/NULL
**/
char * pcGetWord(char *express)
{
	static char *pcLast=NULL;
	static char oneword[1024];
	char ch;
	char* pos;
	if(express!=NULL){
		pos=express;
		pcLast=express;
	}else{
		pos=pcLast;
	}
	nQuotFlag=0;
	/*找到第一个有效字符*/
	while(!IsValidChar(*pos )){   /**判断是否合法字符  **/
		if(*pos=='\0')
			break;
		pos++;
	}
	if(*pos=='\0'){
		return NULL;/*结束*/
	}
	pcLast=pos;
	ch=*pos;
	/****/
	if((ch>='a' && ch<='z') || (ch>='A' && ch <='Z') ||
		ch=='$' || ch=='#' || ch=='_'){
		/*首字符是关键字、变量名、或者函数名*/
		pos=pcLast;
		while(1){
			oneword[pos-pcLast]=ch;
			pos++;
			ch=*pos;
			/*以字母、数字或下划线结束*/
			if(!((ch>='a' && ch<='z')||(ch>='A' && ch<='Z')||
				ch=='_' ||(ch>='0' && ch<='9'))){
				break;
			}
		}
		oneword[pos-pcLast]='\0';
		pcLast=pos;
		/****特殊处理一下函数****/
		if((oneword[0]>='a' && oneword[0]<='z')||(oneword[0]>='A' && oneword[0]<='Z')||oneword[0]=='_'){
			if(strcmp(oneword,"and")!=0 && strcmp(oneword,"or")!=0 && strcmp(oneword,"not")!=0 ){
				if(*pcLast!='('){
					pub_log_error( "[%s][%d]函数定义非法[%s]",__FILE__,__LINE__,oneword);
					return(NULL);
				}else{
					strcat(oneword,"(");
					pcLast++;
				}
			}
		}
		return oneword;
	}
	if(ch=='(' || ch==')' || ch=='[' || ch==']' ||ch=='{' ||ch=='}'||
		ch=='+'||ch=='-' || ch=='*'|| ch=='/'||ch==';'||ch==','||ch=='%'||ch=='|'||ch==':'){
		/*首字符是单字节运算符*/
		if(ch=='/' && pcLast[1]=='/'){
			/*是注释,删除*/
			int id;
			id=2;
			while(pcLast[id]!=0x0d && pcLast[id]!=0x0a && pcLast[id] != 0x00) /**0x0d \n,0x0a \r **/
				id++;
			pcLast=pcLast+id;
			return pcGetWord(NULL);
		}
		oneword[0]=ch;
		oneword[1]='\0';
		pcLast++;
		return oneword;
	}
	if(ch=='=' ||ch=='>' || ch=='<'||ch=='!'){/*比较或赋值*/
		pos=pcLast;
		pos++;
		if(*pos!='='){
			oneword[0]=ch;
			oneword[1]='\0';
			pcLast++;
			return oneword;
		}else{
			oneword[0]=ch;
			oneword[1]=*(pcLast+1);
			oneword[2]='\0';
			pcLast+=2;
			return oneword;
		}
	}
	if(ch==STRFIX){/*是字符串开始的引号*/
		/*此时nQuotFlag必为1，否则就是出错了**/
		pos=pcLast;
		while(1){
			oneword[pos-pcLast]=ch;
			pos++;
			ch=*pos;
			if(ch=='\0')
				break;
			/*以"结束**/
			if(ch==STRFIX){
				oneword[pos-pcLast]=ch;
				pos++;
				break;
			}
		}
		if(ch=='\0')
			return NULL;/*出错了*/
		nQuotFlag=0;
		oneword[pos-pcLast]='\0';
		pcLast=pos;
		return oneword;
	}
	if((ch>='0' && ch<='9')){/*首字节是数字*/
		int dotflg;
		dotflg=0;
		pos=pcLast;
		while(1){
			oneword[pos-pcLast]=ch;
			pos++;
			ch=*pos;
			/*以字母、数字或下划线结束*/
			if(!(ch>='0' && ch<='9')){
				if(ch=='.')
					if(dotflg==0){
						dotflg=1;
					}else
						break;
				else
					break;
			}
		}
		oneword[pos-pcLast]='\0';
		pcLast=pos;
		return oneword;
	}
	return(NULL);
}
/**
函数名:nComputeExpress
功能  :计算一个表达式的值
参数  :
       express          表达式的内容
       length           表达式的长度
       outbuf           输出缓冲区(都以ASCII码格式存放,数字保留5位小数)
返回值:
       0/-1
**/
int nComputeExpress(char *express,int length,char *outbuf){
	char *pcExprBuf;
	char *pcWord;
	int  iRc;
	CTNODE *ct=NULL;
	vInitExprEnv();  /**初始化表达式计算环境，将变量栈和运算符栈清空**/
	if(strcmp(express,"true")==0){
		strcpy(outbuf,"1");
		return(0);
	}else if(strcmp(express,"false")==0){
		strcpy(outbuf,"0");
		return(0);
	}
	pcExprBuf=(char *)malloc(length+1);
	memcpy(pcExprBuf,express,length);
	pcExprBuf[length]='\0';      /*strcpy不会自动在结果后面追加’\0’*/
	pcWord=pcGetWord(pcExprBuf);
	while(pcWord!=NULL){
		if(pcWord[0]=='#' ||pcWord[0]=='$'){
			/****变量****/
			nPushData(pcWord,strlen(pcWord));  /*运算数据入栈*/
		}else if((pcWord[0]>='0' && pcWord[0]<='9')||(pcWord[0]==STRFIX)){
			/****常量****/
			nPushData(pcWord,strlen(pcWord));
		}else if(pcWord[0]=='(' || pcWord[0]=='['||
				 pcWord[0]=='+' || pcWord[0]=='-'||
				 pcWord[0]=='*' || pcWord[0]=='/'||
				 strcmp(pcWord,">=")==0 || strcmp(pcWord,"<=")==0 ||
				 strcmp(pcWord,">")==0 || strcmp(pcWord,"<")==0 ||
				 strcmp(pcWord,"==")==0 || strcmp(pcWord,"!=")==0 ||
				(pcWord[0]>='A' && pcWord[0]<='Z') ||
				(pcWord[0]>='a' && pcWord[0]<='z') ||
				 pcWord[0]=='_' )
		{
			ct=poGetTopOperator();
			if(ct==NULL){
				/****符号栈为空,入栈****/
				nPushOperator(pcWord,strlen(pcWord));
			}else if(nPriorCmp(pcWord,ct->data)>0){
				/****更高级别运算符,入栈****/
				nPushOperator(pcWord,strlen(pcWord));
			}else{
				/*是同级别或低级别的运算符，则出栈计算直到更低级别...*/
				while(1){
					ct=poGetTopOperator();
					if(ct==NULL){
						break;
					}else if(nPriorCmp(pcWord,ct->data)>0){
						break;
					}
					iRc=nComputeBaseExpress(ct->data);
					if(iRc<0){
						pub_log_error( "[%s][%d]计算出错!\n",__FILE__,__LINE__);
						free(pcExprBuf);
						return(-1);
					}
					vDestroyStackNode(ct);
					poPopOperator();
				}
				nPushOperator(pcWord,strlen(pcWord));
			}
		}else if(pcWord[0]==')' || pcWord[0]==']'){
			/*是)或]，则出栈计算直到(或者[为止*/
			if(pcWord[0]==']'){
				pub_log_error( "[%s][%d]目前不支持下标运算符!\n",__FILE__,__LINE__);
				free(pcExprBuf);
				return(-1);
			}
			while(1){
				ct=poGetTopOperator();
				if(ct==NULL){
					pub_log_error( "[%s][%d]未找到匹配的小括号!\n",__FILE__,__LINE__);
					free(pcExprBuf);
					return(-1);
				}
				if(ct->data[strlen(ct->data)-1]=='('){
					if(ct->data[0]=='('){
						vDestroyStackNode(ct);
						poPopOperator();
						break;
					}
				}
				iRc=nComputeBaseExpress(ct->data);
				if(iRc<0){
					pub_log_error( "[%s][%d]计算出错!\n",__FILE__,__LINE__);
					free(pcExprBuf);
					return(-1);
				}
				if(ct->data[strlen(ct->data)-1]=='('){
					vDestroyStackNode(ct);
					poPopOperator();
					break;
				}else{
					vDestroyStackNode(ct);
					poPopOperator();
				}
			}
		}else if(pcWord[0]==','){
			/****逗号表达式:向前计算直到上一个逗号或者括号****/
			while(1){
				ct=poGetTopOperator();
				if(ct==NULL){
					pub_log_error( "[%s][%d]未找到匹配的小括号或逗号!\n",__FILE__,__LINE__);
					free(pcExprBuf);
					return(-1);
				}
				if(ct->data[strlen(ct->data)-1]=='('||ct->data[0]==','){
					break;
				}
				iRc=nComputeBaseExpress(ct->data);
				if(iRc<0){
					pub_log_error( "[%s][%d]计算出错!\n",__FILE__,__LINE__);
					free(pcExprBuf);
					return(-1);
				}
				vDestroyStackNode(ct);
				poPopOperator();
			}
		}else{
			pub_log_error( "[%s][%d]语法错误:[%s]\n",__FILE__,__LINE__,pcWord);
			free(pcExprBuf);
			return(-1);
		}
		pcWord=pcGetWord(NULL);
	}
	/****循环把未出栈的运算符进行计算****/
	while(1){
		ct=poGetTopOperator();
		if(ct==NULL){
			break;
		}
		iRc=nComputeBaseExpress(ct->data);
		if(iRc<0){
			pub_log_error( "[%s][%d]计算[%s]出错!\n",__FILE__,__LINE__,ct->data);
			free(pcExprBuf);
			return(-1);
		}
		vDestroyStackNode(ct);
		poPopOperator();
	}
	free(pcExprBuf);
	/****最后变量栈中应该还有一个变量即为返回值****/
	ct=poPopData();
	if(ct==NULL){
		pub_log_error( "[%s][%d]计算错误过程中堆栈错误\n",__FILE__,__LINE__);
		return(-1);
	}else if(ct->type==CTYPE_NUMBER){
		sprintf(outbuf,"%.5f",STR2DBL(ct->data));
		vDestroyStackNode(ct);
	}else{
		sprintf(outbuf,"%s",ct->data);
		vDestroyStackNode(ct);
	}
	return(0);
}

/**
函数名:nFun_zip
功能  :自定义函数(压缩空格)
参数  :
       字符串
返回值:
       0/-1
**/
int nFun_zip(){
	/****取参数****/
	CTNODE *ct1;
	ct1=poPopData();
	if(ct1==NULL){
		pub_log_error("[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_STRING){
		char *ptr;
		ptr=(char *)malloc(strlen(ct1->data)+1);
		strcpy(ptr,ct1->data);
		pub_str_zipspace(ptr);
		nPushString(ptr);
		vDestroyStackNode(ct1);
		free(ptr);
	}else{
		pub_log_error("[%s][%d]数据类型错误!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);

}

/**
函数名:nComputeBaseExpress
功能  :计算一个基本表达式的值,并把结果放入数据栈
参数  :
       operator   运算符
返回值:
       0/-1
**/
int nComputeBaseExpress(char *operator){
	CTNODE *ct1,*ct2;
	int    iRc=0;
	if(strcmp(operator,"+")==0){
		/****加法****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			char *ptr;
			ptr=(char *)malloc(strlen(ct1->data)+strlen(ct2->data)+3);
			ptr[0]=STRFIX;
			strcpy(ptr+1,ct1->data);
			strcat(ptr,ct2->data);
			sprintf(ptr+strlen(ptr),"%c",STRFIX);
			nPushData(ptr,strlen(ptr));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
			free(ptr);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			char buf[64];
			sprintf(buf,"%f",STR2DBL(ct1->data)+STR2DBL(ct2->data));
			nPushData(buf,strlen(buf));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"-")==0){
		/****减法****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			char buf[64];
			sprintf(buf,"%f",STR2DBL(ct1->data)-STR2DBL(ct2->data));
			pub_log_error( "[%s][%d]Gethere[%s][%f][%f]\n",__FILE__,__LINE__,buf,STR2DBL(ct1->data),STR2DBL(ct2->data));
			nPushData(buf,strlen(buf));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"*")==0){
		/****乘法****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			char buf[64];
			sprintf(buf,"%f",STR2DBL(ct1->data)*STR2DBL(ct2->data));
			nPushData(buf,strlen(buf));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"/")==0){
		/****除法****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			char buf[64];
			sprintf(buf,"%f",STR2DBL(ct1->data)/STR2DBL(ct2->data));
			nPushData(buf,strlen(buf));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,">")==0){
		/****大于比较*/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			iRc=strcmp(ct1->data,ct2->data);
			if(iRc>0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2DBL(ct1->data)-STR2DBL(ct2->data)>DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"<")==0){
		/****小于比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			iRc=strcmp(ct1->data,ct2->data);
			if(iRc<0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2DBL(ct1->data)-STR2DBL(ct2->data)<-DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,">=")==0){
		/****大于等于比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			iRc=strcmp(ct1->data,ct2->data);
			if(iRc>=0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2DBL(ct1->data)-STR2DBL(ct2->data)>=-DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"<=")==0){
		/****小于等于比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			iRc=strcmp(ct1->data,ct2->data);
			if(iRc<=0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2DBL(ct1->data)-STR2DBL(ct2->data)<=DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误[%d][%d]!",__FILE__,__LINE__,ct1->type,ct2->type);
			return(-1);
		}
	}else if(strcmp(operator,"==")==0){
		/****等于比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			pub_str_zipspace(ct1->data);
			pub_str_zipspace(ct2->data);
			iRc=strcmp(ct1->data,ct2->data);
			if(iRc==0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_STRING){
			pub_str_zipspace(ct2->data);
			if(STR2DBL(ct1->data)-atof(ct2->data)>=-DBLZERO &&
				STR2DBL(ct1->data)-atof(ct2->data)<=DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_NUMBER){
			pub_str_zipspace(ct1->data);
			if(atof(ct1->data)-STR2DBL(ct2->data)>=-DBLZERO &&
				atof(ct1->data)-STR2DBL(ct2->data)<=DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2DBL(ct1->data)-STR2DBL(ct2->data)>=-DBLZERO &&
				STR2DBL(ct1->data)-STR2DBL(ct2->data)<=DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误[%d][%d]!",__FILE__,__LINE__,ct1->type,ct2->type);
			return(-1);
		}
	}else if(strcmp(operator,"!=")==0){
		/****不等于比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_STRING){
			iRc=strcmp(ct1->data,ct2->data);
			if(iRc!=0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2DBL(ct1->data)-STR2DBL(ct2->data)>DBLZERO ||
				STR2DBL(ct1->data)-STR2DBL(ct2->data)<-DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_STRING){
			pub_str_zipspace(ct2->data);
			if(STR2DBL(ct1->data)-atof(ct2->data)>DBLZERO ||
				STR2DBL(ct1->data)-atof(ct2->data) <-DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_NUMBER){
			pub_str_zipspace(ct1->data);
			if(atof(ct1->data)-STR2DBL(ct2->data)>DBLZERO ||
				atof(ct1->data)-STR2DBL(ct2->data)<-DBLZERO){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"and")==0){
		/****与比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2INT(ct1->data)!=0 && STR2INT(ct2->data)!=0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"or")==0){
		/****或比较**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			if(STR2INT(ct1->data)!=0 || STR2INT(ct2->data)!=0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"not")==0){
		/****与比较**/
		ct1=poPopData();
		if(ct1==NULL){
			pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER ){
			if(STR2INT(ct1->data)==0){
				nPushData("1",1);
			}else{
				nPushData("0",1);
			}
			vDestroyStackNode(ct1);
		}else{
			pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
			return(-1);
		}
	}else{
		char cFunName[33];
		if(operator[strlen(operator)-1]!='('){
			pub_log_error( "[%s][%d]非法运算符[%s]\n",operator);
			return(-1);
		}
		memset(cFunName,'\0',sizeof(cFunName));
		strncpy(cFunName,operator,sizeof(cFunName)-1);
		cFunName[strlen(cFunName)-1]='\0';
		if(strcmp(cFunName,"mod")==0){
			return(nFun_mod());
		}else if(strcmp(cFunName,"param")==0){
			return(nFun_param());
		}else if(strcmp(cFunName,"substr")==0){
			return(nFun_substr());
		}else if(strcmp(cFunName,"chr")==0){
			return(nFun_chr());
		}else if(strcmp(cFunName,"money")==0){
			return(nFun_money());
		}else if(strcmp(cFunName,"zip")==0){
			return(nFun_zip());
		}
		else{
			pub_log_error( "[%s][%d]未定义的函数[%s]\n",cFunName);
			return(-1);
		}
	}
	return(0);
}
/**
函数名:nFun_mod
功能  :自定义函数mod(求余)
参数  :
       无
返回值:
       0/-1
**/
int nFun_mod(){
	/****求余****/
	CTNODE *ct1,*ct2;
	ct2=poPopData();
	ct1=poPopData();
	if(ct1==NULL || ct2==NULL){
		pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
		char buf[64];
		sprintf(buf,"%ld",STR2INT(ct1->data)%STR2INT(ct2->data));
		pub_log_error( "[%s][%d]Gethere[%s][%f][%f]\n",__FILE__,__LINE__,buf,STR2DBL(ct1->data),STR2DBL(ct2->data));
		nPushData(buf,strlen(buf));
		vDestroyStackNode(ct1);
		vDestroyStackNode(ct2);
	}else{
		pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
函数名:nFun_param
功能  :自定义函数(取参数)
参数  :
       参数名称
返回值:
       0/-1
**/
int nFun_param(){
	/****取参数****/
	CTNODE *ct1;
	ct1=poPopData();
	if(ct1==NULL ){
		pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_STRING){
		char buf[1024];
		memset(buf,'\0',sizeof(buf));
		/*nGetComParam(ct1->data,buf);*/
		nPushString(buf);
		vDestroyStackNode(ct1);
	}else{
		pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
函数名:nFun_substr
功能  :自定义函数(取子串)
参数  :
       主串
       起始位置
       长度
返回值:
       0/-1
**/
int nFun_substr(){
	/****取参数****/
	CTNODE *ct1,*ct2,*ct3;
	ct3=poPopData();
	ct2=poPopData();
	ct1=poPopData();
	if(ct1==NULL || ct2==NULL||ct3==NULL){
		pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_NUMBER && ct3->type==CTYPE_NUMBER){
		char buf[1024];
		memcpy(buf,ct1->data+STR2INT(ct2->data),STR2INT(ct3->data));
		buf[STR2INT(ct3->data)]='\0';
		nPushString(buf);
		vDestroyStackNode(ct1);
		vDestroyStackNode(ct2);
		vDestroyStackNode(ct3);
	}else{
		pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
函数名:nFun_chr
功能  :自定义函数(得到ascii码对应字符串)
参数  :
       ascii码值
返回值:
       0/-1
**/
int nFun_chr(){
	/****取参数****/
	CTNODE *ct1;
	ct1=poPopData();
	if(ct1==NULL){
		pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_NUMBER){
		char buf[2];
		memset(buf,'\0',2);
		buf[0]=(char )STR2INT(ct1->data);
		nPushString(buf);
		vDestroyStackNode(ct1);
	}else{
		pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
函数名:nFun_money
功能  :自定义函数(得到指定格式的金额)
参数  :
       原金额内容
       乘以的权重
       输出格式
返回值:
       0/-1
**/
int nFun_money(){
	/****取参数****/
	CTNODE *ct1;
	CTNODE *ct2;
	CTNODE *ct3;
	ct3=poPopData();
	ct2=poPopData();
	ct1=poPopData();
	if(ct1==NULL || ct2==NULL||ct3==NULL){
		pub_log_error( "[%s][%d]计算过程中出现错误!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_NUMBER && ct3->type==CTYPE_STRING){
		char buf[64];
		memset(buf,'\0',sizeof(buf));
		pub_log_error( "[%s][%d]计算格式[%s],权重[%.2f],原值[%.2f]",__FILE__,__LINE__,ct3->data,STR2DBL(ct2->data),atof(ct1->data));
		sprintf(buf,ct3->data,atof(ct1->data)*STR2DBL(ct2->data));
		nPushString(buf);
		vDestroyStackNode(ct1);
		vDestroyStackNode(ct2);
		vDestroyStackNode(ct3);
	}else{
		pub_log_error( "[%s][%d]数据类型错误!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}

/**
函数名:nExtendEqual
功能  :自动判断类型并比较两个字符串是否相等(如果是数字按数字比较)
参数  :
       字符串1
       字符串2
返回值:
       0不等/1相等
**/
int nExtendEqual(char *s1,char *s2){
	int nS1IsNum,nS2IsNum;
	int i;
	int nBeginFlag=0;
	int nDotFlag=0;
	nS1IsNum=nS2IsNum=1;

	i=strlen(s1)-1;
	nBeginFlag=0;
	nDotFlag=0;
	while(i>=0){
		if(s1[i]==' '){
			if(nBeginFlag){
				/****发现非数字字符****/
				nS1IsNum=0;
				break;
			}
		}else{
			nBeginFlag=1;
			if((s1[i]>='0' && s1[i]<='9') || s1[i]=='.'||s1[i]=='-'){
				if(s1[i]=='.'){
					if(nDotFlag){
						nS1IsNum=0;
						break;
					}else{
						nDotFlag=1;
					}
				}else if(s1[i]=='-' && i!=0){
					nS1IsNum=0;
					break;
				}else{
				}
			}else{
				nS1IsNum=0;
				break;
			}
		}
		i--;
	}
	i=strlen(s2)-1;
	nBeginFlag=0;
	nDotFlag=0;
	while(i>=0){
		if(s2[i]==' '){
			if(nBeginFlag){
				/****发现非数字字符****/
				nS2IsNum=0;
				break;
			}
		}else{
			nBeginFlag=1;
			if((s2[i]>='0' && s2[i]<='9') || s2[i]=='.'||s2[i]=='-'){
				if(s2[i]=='.'){
					if(nDotFlag){
						nS2IsNum=0;
						break;
					}else{
						nDotFlag=1;
					}
				}else if(s2[i]=='-' && i!=0){
					nS2IsNum=0;
					break;
				}else{
				}
			}else{
				nS2IsNum=0;
				break;
			}
		}
		i--;
	}
	if(nS1IsNum && nS2IsNum){
		/****都是数字,按数字型比较****/
		double r;
		r=atof(s1)-atof(s2);
		if(r> -0.000001 && r<0.000001){
			pub_log_info("数字比较[%s][%s],相等!",s1,s2);
			return(1);	/****相等****/
		}else{
			pub_log_info("数字比较[%s][%s],不相等!",s1,s2);
			return(0);	/****不相等****/
		}
	}else{
		if(strcmp(s1,s2)==0){
			pub_log_info("字符比较[%s][%s],相等!",s1,s2);
			return(1);	/****相等****/
		}else{
			pub_log_info("字符比较[%s][%s],不相等!",s1,s2);
			return(0);	/****不相等****/
		}
	}
	return(0);
}
