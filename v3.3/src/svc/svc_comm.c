/***************************************************************************
 *** �������� : Shi Shenghui  ʯ����                                      **
 *** ��    �� : 2008-05-27                                                **
 *** ����ģ�� :                                                          ***
 *** �������� : fbbm_comm.c                                              ***
 *** �������� : �������ʽ�õ��ĵײ㺯��                                 ***
 *** ʹ��ע�� :                                                          ***
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
������:vInitExprEnv
����  :��ʼ�����ʽ���㻷��(����ջ�������ջ)
����  :
       ��
����ֵ:
       ��
**/
void vInitExprEnv(){
	int i;
	for(i=0;i<=nOperatorIndex;i++){
		vDestroyStackNode(stOperatorStack[i]);  /**���������ջ�Ľڵ�**/
	}
	for(i=0;i<=nDataIndex;i++){
		vDestroyStackNode(stDataStack[i]);      /**���ٱ���ջ�Ľڵ�**/
	}
	memset(stOperatorStack,'\0',sizeof(stOperatorStack));
	memset(stDataStack,'\0',sizeof(stDataStack));
	nOperatorIndex=nDataIndex=-1;
	nQuotFlag=0;
}
/**
������:nPushOperator
����  :�������ջ
����  :
       operator         �����
       length           ������ĳ���
����ֵ:
       0/-1
**/
int nPushOperator(char *operator,int length){
	CTNODE *ct;
	if(nOperatorIndex>=MAX_OPERATOR_CNT-1){
		pub_log_error( "[%s][%d]�������ջԽ��!",__FILE__,__LINE__);
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
������:nPushData
����  :����������ջ
����  :
       data             ����(�����Ǳ������߳���)
       length           ���ݵĳ���
����ֵ:
       0/-1
**/
int nPushData(char *data,int length){
	CTNODE *ct;
	sw_int_t    iRc;
	char buf[1024];
	if(nDataIndex>=MAX_DATA_CNT-1){
		pub_log_error( "[%s][%d]���ݶ�ջԽ��!",__FILE__,__LINE__);
		return(-1);
	}
	ct=(CTNODE *)malloc(sizeof(CTNODE));
	if((data[0]>='0' && data[0]<='9')||
		(data[0]=='-' && data[1]>='0' && data[1]<='9')){
		/****�����ͳ���****/
		if(length>60){
			pub_log_error( "[%s][%d]�����ͳ�������[%d]����",__FILE__,__LINE__,length);
			return(-1);
		}
		memcpy(buf,data,length);
		buf[length]='\0';
		ct->type=CTYPE_NUMBER;
		ct->data=(char *)malloc(sizeof(double));
		*(double *)ct->data=atof(buf);
	}else if(data[0]=='#'||data[0]=='$'){
		/****�ַ�������****/
		char cName[33];
		memset(cName,'\0',sizeof(cName));
		int	len = 0;
		len = sizeof(cName) - 1;
		if(length > len){
			pub_log_error( "[%s][%d]������̫��[%d]",__FILE__,__LINE__,length);
			return(-1);
		}
		memcpy(cName,data,length);
		memset(buf, 0x0, sizeof(buf));
		iRc=get_data_len(cName,buf);
		if(iRc>=1024){
			pub_log_error( "[%s][%d]����̫��[%d]",__FILE__,__LINE__,iRc);
			return(-1);
		}
		if(iRc<0){
			iRc=0;	/****δ����ı��������մ�����****/
		}
		ct->type=CTYPE_STRING;
		ct->data=(char *)malloc(iRc+1);
		if(iRc>0){
			memcpy(ct->data,buf,iRc);
		}
		ct->data[iRc]='\0';
	}else if(data[0]==STRFIX){
		/****�ַ�������****/
		ct->type=CTYPE_STRING;
		ct->data=(char *)malloc(length-1);
		memcpy(ct->data,data+1,length-2);
		ct->data[length-2]='\0';
	}else{
		pub_log_error( "[%s][%d]�Ƿ�����������[%.*s]",__FILE__,__LINE__,length,data);
		return(-1);
	}
	nDataIndex++;
	stDataStack[nDataIndex]=ct;
	return(0);
}
/**
������:nPushString
����  :�ַ�������������ջ(�Զ�׷���ַ���������)
����  :
       data             ����(�����Ǳ������߳���)
����ֵ:
       0/-1
**/
int nPushString(char *data){
	CTNODE *ct;
	int  length=strlen(data);
	if(nDataIndex>=MAX_DATA_CNT-1){
		pub_log_error( "[%s][%d]���ݶ�ջԽ��!",__FILE__,__LINE__);
		return(-1);
	}
	ct=(CTNODE *)malloc(sizeof(CTNODE));
	/****�ַ�������****/
	ct->type=CTYPE_STRING;
	ct->data=(char *)malloc(length+1);
	memcpy(ct->data,data,length);
	ct->data[length]='\0';
	nDataIndex++;
	stDataStack[nDataIndex]=ct;
	return(0);
}

/**
������:poPopOperator
����  :�������ջ
����  :
       ��
����ֵ:
       CTNODE�ڵ�ָ��/NULL
**/
CTNODE *poPopOperator(){
	if(nOperatorIndex<0){
		return(NULL);
	}
	return(stOperatorStack[nOperatorIndex--]);
}
/**
������:poPopData
����  :�������ݳ�ջ
����  :
       ��
����ֵ:
       CTNODE�ڵ�ָ��/NULL
**/
CTNODE *poPopData(){
	if(nDataIndex<0){
		return(NULL);
	}
	return(stDataStack[nDataIndex--]);
}
/**
������:poGetTopOperator
����  :�õ�ջ�������,������ջ
����  :
       ��
����ֵ:
       CTNODE�ڵ�ָ��/NULL
**/
CTNODE *poGetTopOperator(){
	if(nOperatorIndex<0){
		return(NULL);
	}
	return(stOperatorStack[nOperatorIndex]);
}
/**
������:poGetTopData
����  :�õ�ջ����������,������ջ
����  :
       ��
����ֵ:
       CTNODE�ڵ�ָ��/NULL
**/
CTNODE *poGetTopData(){
	if(nDataIndex<0){
		return(NULL);
	}
	return(stDataStack[nDataIndex]);
}
/**
������:vDestroyStackNode
����  :�ѽڵ�����
����  :
       ct      �����ٵĽڵ�ָ��
����ֵ:
       ��
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
������:nPriorCmp
����  :�Ƚ���������������ȼ�
����  :
       op1     ��һ�������
       op2     �ڶ��������
����ֵ:
       1       op1�����ȼ�>op2
       0       op1�����ȼ�=op2
       -1      op1�����ȼ�<op2
**/
int nPriorCmp(char *op1, char *op2)
{
	/*������ĳ�����op1Ϊ׼��������'\0'�����ı�׼�ַ���
	���ȼ���ӵ͵��� or and not  ��> >= < <= != ==�� ��+ -�� ��* /�� ([
	)��]�����ȼ���computexp�����⴦��
	([Ϊop1���ȼ����,Ϊop2���ȼ����
	�����ں�����������С���ţ����ȼ���С������ͬ
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
������:IsValidChar
����  :�ж��Ƿ�Ϸ��ַ�
����  :
       ch      �ַ�
����ֵ:
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
			return TRUE;/*�������ڵ��ַ�����֤*/
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
������:pcGetWord
����  :��һ�����ʽ��ȡ��һ������
����  :
       express ���ʽ�ĵ�ַ,NULL��ʾ�����ϴε�λ�ÿ�ʼȡ
����ֵ:
       ����/NULL
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
	/*�ҵ���һ����Ч�ַ�*/
	while(!IsValidChar(*pos )){   /**�ж��Ƿ�Ϸ��ַ�  **/
		if(*pos=='\0')
			break;
		pos++;
	}
	if(*pos=='\0'){
		return NULL;/*����*/
	}
	pcLast=pos;
	ch=*pos;
	/****/
	if((ch>='a' && ch<='z') || (ch>='A' && ch <='Z') ||
		ch=='$' || ch=='#' || ch=='_'){
		/*���ַ��ǹؼ��֡������������ߺ�����*/
		pos=pcLast;
		while(1){
			oneword[pos-pcLast]=ch;
			pos++;
			ch=*pos;
			/*����ĸ�����ֻ��»��߽���*/
			if(!((ch>='a' && ch<='z')||(ch>='A' && ch<='Z')||
				ch=='_' ||(ch>='0' && ch<='9'))){
				break;
			}
		}
		oneword[pos-pcLast]='\0';
		pcLast=pos;
		/****���⴦��һ�º���****/
		if((oneword[0]>='a' && oneword[0]<='z')||(oneword[0]>='A' && oneword[0]<='Z')||oneword[0]=='_'){
			if(strcmp(oneword,"and")!=0 && strcmp(oneword,"or")!=0 && strcmp(oneword,"not")!=0 ){
				if(*pcLast!='('){
					pub_log_error( "[%s][%d]��������Ƿ�[%s]",__FILE__,__LINE__,oneword);
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
		/*���ַ��ǵ��ֽ������*/
		if(ch=='/' && pcLast[1]=='/'){
			/*��ע��,ɾ��*/
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
	if(ch=='=' ||ch=='>' || ch=='<'||ch=='!'){/*�Ƚϻ�ֵ*/
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
	if(ch==STRFIX){/*���ַ�����ʼ������*/
		/*��ʱnQuotFlag��Ϊ1��������ǳ�����**/
		pos=pcLast;
		while(1){
			oneword[pos-pcLast]=ch;
			pos++;
			ch=*pos;
			if(ch=='\0')
				break;
			/*��"����**/
			if(ch==STRFIX){
				oneword[pos-pcLast]=ch;
				pos++;
				break;
			}
		}
		if(ch=='\0')
			return NULL;/*������*/
		nQuotFlag=0;
		oneword[pos-pcLast]='\0';
		pcLast=pos;
		return oneword;
	}
	if((ch>='0' && ch<='9')){/*���ֽ�������*/
		int dotflg;
		dotflg=0;
		pos=pcLast;
		while(1){
			oneword[pos-pcLast]=ch;
			pos++;
			ch=*pos;
			/*����ĸ�����ֻ��»��߽���*/
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
������:nComputeExpress
����  :����һ�����ʽ��ֵ
����  :
       express          ���ʽ������
       length           ���ʽ�ĳ���
       outbuf           ���������(����ASCII���ʽ���,���ֱ���5λС��)
����ֵ:
       0/-1
**/
int nComputeExpress(char *express,int length,char *outbuf){
	char *pcExprBuf;
	char *pcWord;
	int  iRc;
	CTNODE *ct=NULL;
	vInitExprEnv();  /**��ʼ�����ʽ���㻷����������ջ�������ջ���**/
	if(strcmp(express,"true")==0){
		strcpy(outbuf,"1");
		return(0);
	}else if(strcmp(express,"false")==0){
		strcpy(outbuf,"0");
		return(0);
	}
	pcExprBuf=(char *)malloc(length+1);
	memcpy(pcExprBuf,express,length);
	pcExprBuf[length]='\0';      /*strcpy�����Զ��ڽ������׷�ӡ�\0��*/
	pcWord=pcGetWord(pcExprBuf);
	while(pcWord!=NULL){
		if(pcWord[0]=='#' ||pcWord[0]=='$'){
			/****����****/
			nPushData(pcWord,strlen(pcWord));  /*����������ջ*/
		}else if((pcWord[0]>='0' && pcWord[0]<='9')||(pcWord[0]==STRFIX)){
			/****����****/
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
				/****����ջΪ��,��ջ****/
				nPushOperator(pcWord,strlen(pcWord));
			}else if(nPriorCmp(pcWord,ct->data)>0){
				/****���߼��������,��ջ****/
				nPushOperator(pcWord,strlen(pcWord));
			}else{
				/*��ͬ�����ͼ��������������ջ����ֱ�����ͼ���...*/
				while(1){
					ct=poGetTopOperator();
					if(ct==NULL){
						break;
					}else if(nPriorCmp(pcWord,ct->data)>0){
						break;
					}
					iRc=nComputeBaseExpress(ct->data);
					if(iRc<0){
						pub_log_error( "[%s][%d]�������!\n",__FILE__,__LINE__);
						free(pcExprBuf);
						return(-1);
					}
					vDestroyStackNode(ct);
					poPopOperator();
				}
				nPushOperator(pcWord,strlen(pcWord));
			}
		}else if(pcWord[0]==')' || pcWord[0]==']'){
			/*��)��]�����ջ����ֱ��(����[Ϊֹ*/
			if(pcWord[0]==']'){
				pub_log_error( "[%s][%d]Ŀǰ��֧���±������!\n",__FILE__,__LINE__);
				free(pcExprBuf);
				return(-1);
			}
			while(1){
				ct=poGetTopOperator();
				if(ct==NULL){
					pub_log_error( "[%s][%d]δ�ҵ�ƥ���С����!\n",__FILE__,__LINE__);
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
					pub_log_error( "[%s][%d]�������!\n",__FILE__,__LINE__);
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
			/****���ű��ʽ:��ǰ����ֱ����һ�����Ż�������****/
			while(1){
				ct=poGetTopOperator();
				if(ct==NULL){
					pub_log_error( "[%s][%d]δ�ҵ�ƥ���С���Ż򶺺�!\n",__FILE__,__LINE__);
					free(pcExprBuf);
					return(-1);
				}
				if(ct->data[strlen(ct->data)-1]=='('||ct->data[0]==','){
					break;
				}
				iRc=nComputeBaseExpress(ct->data);
				if(iRc<0){
					pub_log_error( "[%s][%d]�������!\n",__FILE__,__LINE__);
					free(pcExprBuf);
					return(-1);
				}
				vDestroyStackNode(ct);
				poPopOperator();
			}
		}else{
			pub_log_error( "[%s][%d]�﷨����:[%s]\n",__FILE__,__LINE__,pcWord);
			free(pcExprBuf);
			return(-1);
		}
		pcWord=pcGetWord(NULL);
	}
	/****ѭ����δ��ջ����������м���****/
	while(1){
		ct=poGetTopOperator();
		if(ct==NULL){
			break;
		}
		iRc=nComputeBaseExpress(ct->data);
		if(iRc<0){
			pub_log_error( "[%s][%d]����[%s]����!\n",__FILE__,__LINE__,ct->data);
			free(pcExprBuf);
			return(-1);
		}
		vDestroyStackNode(ct);
		poPopOperator();
	}
	free(pcExprBuf);
	/****������ջ��Ӧ�û���һ��������Ϊ����ֵ****/
	ct=poPopData();
	if(ct==NULL){
		pub_log_error( "[%s][%d]�����������ж�ջ����\n",__FILE__,__LINE__);
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
������:nFun_zip
����  :�Զ��庯��(ѹ���ո�)
����  :
       �ַ���
����ֵ:
       0/-1
**/
int nFun_zip(){
	/****ȡ����****/
	CTNODE *ct1;
	ct1=poPopData();
	if(ct1==NULL){
		pub_log_error("[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
		pub_log_error("[%s][%d]�������ʹ���!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);

}

/**
������:nComputeBaseExpress
����  :����һ���������ʽ��ֵ,���ѽ����������ջ
����  :
       operator   �����
����ֵ:
       0/-1
**/
int nComputeBaseExpress(char *operator){
	CTNODE *ct1,*ct2;
	int    iRc=0;
	if(strcmp(operator,"+")==0){
		/****�ӷ�****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"-")==0){
		/****����****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"*")==0){
		/****�˷�****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			char buf[64];
			sprintf(buf,"%f",STR2DBL(ct1->data)*STR2DBL(ct2->data));
			nPushData(buf,strlen(buf));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"/")==0){
		/****����****/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
			return(-1);
		}
		if(ct1->type==CTYPE_NUMBER && ct2->type==CTYPE_NUMBER){
			char buf[64];
			sprintf(buf,"%f",STR2DBL(ct1->data)/STR2DBL(ct2->data));
			nPushData(buf,strlen(buf));
			vDestroyStackNode(ct1);
			vDestroyStackNode(ct2);
		}else{
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,">")==0){
		/****���ڱȽ�*/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"<")==0){
		/****С�ڱȽ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,">=")==0){
		/****���ڵ��ڱȽ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"<=")==0){
		/****С�ڵ��ڱȽ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���[%d][%d]!",__FILE__,__LINE__,ct1->type,ct2->type);
			return(-1);
		}
	}else if(strcmp(operator,"==")==0){
		/****���ڱȽ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���[%d][%d]!",__FILE__,__LINE__,ct1->type,ct2->type);
			return(-1);
		}
	}else if(strcmp(operator,"!=")==0){
		/****�����ڱȽ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"and")==0){
		/****��Ƚ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"or")==0){
		/****��Ƚ�**/
		ct2=poPopData();
		ct1=poPopData();
		if(ct1==NULL || ct2==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else if(strcmp(operator,"not")==0){
		/****��Ƚ�**/
		ct1=poPopData();
		if(ct1==NULL){
			pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
			pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
			return(-1);
		}
	}else{
		char cFunName[33];
		if(operator[strlen(operator)-1]!='('){
			pub_log_error( "[%s][%d]�Ƿ������[%s]\n",operator);
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
			pub_log_error( "[%s][%d]δ����ĺ���[%s]\n",cFunName);
			return(-1);
		}
	}
	return(0);
}
/**
������:nFun_mod
����  :�Զ��庯��mod(����)
����  :
       ��
����ֵ:
       0/-1
**/
int nFun_mod(){
	/****����****/
	CTNODE *ct1,*ct2;
	ct2=poPopData();
	ct1=poPopData();
	if(ct1==NULL || ct2==NULL){
		pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
		pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
������:nFun_param
����  :�Զ��庯��(ȡ����)
����  :
       ��������
����ֵ:
       0/-1
**/
int nFun_param(){
	/****ȡ����****/
	CTNODE *ct1;
	ct1=poPopData();
	if(ct1==NULL ){
		pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_STRING){
		char buf[1024];
		memset(buf,'\0',sizeof(buf));
		/*nGetComParam(ct1->data,buf);*/
		nPushString(buf);
		vDestroyStackNode(ct1);
	}else{
		pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
������:nFun_substr
����  :�Զ��庯��(ȡ�Ӵ�)
����  :
       ����
       ��ʼλ��
       ����
����ֵ:
       0/-1
**/
int nFun_substr(){
	/****ȡ����****/
	CTNODE *ct1,*ct2,*ct3;
	ct3=poPopData();
	ct2=poPopData();
	ct1=poPopData();
	if(ct1==NULL || ct2==NULL||ct3==NULL){
		pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
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
		pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
������:nFun_chr
����  :�Զ��庯��(�õ�ascii���Ӧ�ַ���)
����  :
       ascii��ֵ
����ֵ:
       0/-1
**/
int nFun_chr(){
	/****ȡ����****/
	CTNODE *ct1;
	ct1=poPopData();
	if(ct1==NULL){
		pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_NUMBER){
		char buf[2];
		memset(buf,'\0',2);
		buf[0]=(char )STR2INT(ct1->data);
		nPushString(buf);
		vDestroyStackNode(ct1);
	}else{
		pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}
/**
������:nFun_money
����  :�Զ��庯��(�õ�ָ����ʽ�Ľ��)
����  :
       ԭ�������
       ���Ե�Ȩ��
       �����ʽ
����ֵ:
       0/-1
**/
int nFun_money(){
	/****ȡ����****/
	CTNODE *ct1;
	CTNODE *ct2;
	CTNODE *ct3;
	ct3=poPopData();
	ct2=poPopData();
	ct1=poPopData();
	if(ct1==NULL || ct2==NULL||ct3==NULL){
		pub_log_error( "[%s][%d]��������г��ִ���!",__FILE__,__LINE__);
		return(-1);
	}
	if(ct1->type==CTYPE_STRING && ct2->type==CTYPE_NUMBER && ct3->type==CTYPE_STRING){
		char buf[64];
		memset(buf,'\0',sizeof(buf));
		pub_log_error( "[%s][%d]�����ʽ[%s],Ȩ��[%.2f],ԭֵ[%.2f]",__FILE__,__LINE__,ct3->data,STR2DBL(ct2->data),atof(ct1->data));
		sprintf(buf,ct3->data,atof(ct1->data)*STR2DBL(ct2->data));
		nPushString(buf);
		vDestroyStackNode(ct1);
		vDestroyStackNode(ct2);
		vDestroyStackNode(ct3);
	}else{
		pub_log_error( "[%s][%d]�������ʹ���!",__FILE__,__LINE__);
		return(-1);
	}
	return(0);
}

/**
������:nExtendEqual
����  :�Զ��ж����Ͳ��Ƚ������ַ����Ƿ����(��������ְ����ֱȽ�)
����  :
       �ַ���1
       �ַ���2
����ֵ:
       0����/1���
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
				/****���ַ������ַ�****/
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
				/****���ַ������ַ�****/
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
		/****��������,�������ͱȽ�****/
		double r;
		r=atof(s1)-atof(s2);
		if(r> -0.000001 && r<0.000001){
			pub_log_info("���ֱȽ�[%s][%s],���!",s1,s2);
			return(1);	/****���****/
		}else{
			pub_log_info("���ֱȽ�[%s][%s],�����!",s1,s2);
			return(0);	/****�����****/
		}
	}else{
		if(strcmp(s1,s2)==0){
			pub_log_info("�ַ��Ƚ�[%s][%s],���!",s1,s2);
			return(1);	/****���****/
		}else{
			pub_log_info("�ַ��Ƚ�[%s][%s],�����!",s1,s2);
			return(0);	/****�����****/
		}
	}
	return(0);
}
