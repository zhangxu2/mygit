/***************************************************************************
 *** �������� : Shi Shenghui  ʯ����                                      **
 *** ��    �� : 2007-08-17                                                **
 *** ����ģ�� :                                                          ***
 *** �������� : minifun.c                                                ***
 *** �������� : ����ת����С����                                     ***
 *** ʹ��ע�� :                                                          ***
 ***************************************************************************/
#include "string.h"
#include "pub_vars.h"
#include "pub_comvars.h"
#include "pub_code.h"

#define MAX_MPARAM 5
typedef struct{
	char cFunName;
	int  argc;
	char argv[MAX_MPARAM][128];
}TMPARAM;

SW_PROTECTED int iCountExpressT(sw_loc_vars_t *vars,TMPARAM *param,char *value);
SW_PROTECTED int iCountExpressL(sw_loc_vars_t *vars,TMPARAM *param,char *value);
SW_PROTECTED int iCountExpressR(sw_loc_vars_t *vars,TMPARAM *param,char *value);
SW_PROTECTED int iCountExpressZ(sw_loc_vars_t *vars,TMPARAM *param,char *value);
SW_PROTECTED int iCountExpressA(sw_loc_vars_t *vars,TMPARAM *param,char *value);
SW_PROTECTED int iCountExpressB(sw_loc_vars_t *vars,TMPARAM *param,char *value);
SW_PROTECTED int iCountExpressF(sw_loc_vars_t *vars,TMPARAM *param,char *value);

SW_PROTECTED int iCountExpressLenT(sw_loc_vars_t *vars,TMPARAM *param);
SW_PROTECTED int iCountExpressLenL(sw_loc_vars_t *vars,TMPARAM *param);
SW_PROTECTED int iCountExpressLenR(sw_loc_vars_t *vars,TMPARAM *param);
SW_PROTECTED int iCountExpressLenZ(sw_loc_vars_t *vars,TMPARAM *param);
SW_PROTECTED int iCountExpressLenA(sw_loc_vars_t *vars,TMPARAM *param);
SW_PROTECTED int iCountExpressLenB(sw_loc_vars_t *vars,TMPARAM *param);
SW_PROTECTED int iCountExpressLenF(sw_loc_vars_t *vars,TMPARAM *param);

int iGetVarParamValue(sw_loc_vars_t *vars, char *name, char *value)
{
	if (name == NULL || name[0] == '\0')
	{
		return -1;	
	}

	if (vars == NULL)
	{
		return pub_vars_get_variable(name,value);
	}
	else
	{
		return loc_get_data_len(vars, name, value);
	}

	return -1;
}

/**
������:iAnalyzeMiniParam
����  :����С��������
����  :��
����ֵ:0/-1
**/
int iAnalyzeMiniParam(char *source,TMPARAM *param){
	char *ptr;
	param->cFunName=source[0];
	param->argc=0;
	if(source[1]!='('){
		return(-1);
	}
	ptr=strtok(source+2,",");
	while(ptr!=NULL){
		strcpy(param->argv[param->argc],ptr);
		param->argc++;
		ptr=strtok(NULL,",");
	}
	if(param->argc>0){
		char *last=param->argv[param->argc-1];
		int  len=strlen(last);
		if(last[len-1]==')'){
			last[len-1]='\0';
		}
	}
	return(0);
}

int iCountConstValue(char *source, char *buf){
	char *ptr;
	int  offset=0;
	int  nLen=0;
	ptr=strtok(source,"+");
	while(ptr!=NULL){
		nLen=strlen(ptr);
		strcpy(buf+offset,ptr);
		offset+=nLen;
		ptr=strtok(NULL,"+");
	}
	return(offset);
}

int iCountConstValueLen(char *source){
	char *ptr;
	int  offset=0;
	int  nLen=0;
	ptr=strtok(source,"+");
	while(ptr!=NULL){
		nLen=strlen(ptr);
		offset+=nLen;
		ptr=strtok(NULL,"+");
	}
	return(offset);
}

/**
������:pub_comvars_get
����  :�õ�����С�����ı������ʽ��ֵ
����  :���ʽ(���ܴ���128���ַ�)
����ֵ:��������/-1
**/
int pub_comvars_get(sw_loc_vars_t *vars, char *express,char *value)
{
	int iRc = -1;
	TMPARAM param;
	char *ptr = NULL;
	char source[128];

	memset(&param, 0, sizeof(param));
	memset(source, 0, sizeof(source));
	
	strcpy(source,express);
	pub_str_zipspace(source);
	if(source[0]=='#' || source[0]=='$'){
		return iGetVarParamValue(vars, source, value);
	}
	
	ptr = strchr(source, '(');
	if (ptr == NULL) {
		return iCountConstValue(source, value); 
	}

	iRc=iAnalyzeMiniParam(source,&param);
	if(iRc){
		return(iRc);
	}
	switch(param.cFunName)
	{
		case 'T':/**��ȡ**/
			return(iCountExpressT(vars, &param, value));
		case 'L':/**�����**/
			return(iCountExpressL(vars, &param, value));
		case 'R':/**�Ҷ���**/
			return(iCountExpressR(vars, &param, value));
		case 'Z':/**ѹ��**/
			return(iCountExpressZ(vars, &param, value));
		case 'A':/**BCDת��ASCII**/
			return(iCountExpressA(vars, &param, value));
		case 'B':/**ASCIIת��BCD**/
			return(iCountExpressB(vars, &param, value));
		case 'F':/**name value\n ��ʽ**/
			return(iCountExpressF(vars, &param, value));
		default:
			return(-1);
	}
	return(0);
}

int pub_comvars_get_len(sw_loc_vars_t *vars, char *express)
{
	int iRc = -1;
	TMPARAM param;
	char *ptr = NULL;
	char source[128];

	memset(&param, 0, sizeof(param));
	memset(source, 0, sizeof(source));
	
	strcpy(source,express);
	pub_str_zipspace(source);
	if(source[0]=='#' || source[0]=='$'){
		return vars->get_field_len(vars,source);
	}

	ptr = strchr(source, '(');
	if (ptr == NULL) {
		return iCountConstValueLen(source); 
	}

	iRc=iAnalyzeMiniParam(source,&param);
	if(iRc){
		return(iRc);
	}
	switch(param.cFunName)
	{
		case 'T':/**��ȡ**/
			return(iCountExpressLenT(vars, &param));
		case 'L':/**�����**/
			return(iCountExpressLenL(vars, &param));
		case 'R':/**�Ҷ���**/
			return(iCountExpressLenR(vars, &param));
		case 'Z':/**ѹ��**/
			return(iCountExpressLenZ(vars, &param));
		case 'A':/**BCDת��ASCII**/
			return(iCountExpressLenA(vars, &param));
		case 'B':/**ASCIIת��BCD**/
			return(iCountExpressLenB(vars, &param));
		case 'F':/**name value\n ��ʽ**/
			return(iCountExpressLenF(vars, &param));
		default:
			return(-1);
	}
	return(0);
}

/**
������:nCountParamOne
����  :�����һ�������Ľ��
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
       �˺���������ʽ�ĵ�һ��������ֵ,ͨ����һ�¼�����ʽ
       ����
       ����
       ����/����+����/����......
       �����µ�С�����аѵ�һ�������ĸ�����ʽ�����ΪP1
**/
int iCountParamOne(sw_loc_vars_t *vars, TMPARAM *param,char *buf){
	char *ptr;
	int  offset=0;
	int  nLen=0;
	ptr=strtok(param->argv[0],"+");
	while(ptr!=NULL){
		if(ptr[0]=='#' || ptr[0]=='$'){
			nLen=iGetVarParamValue(vars,ptr,buf+offset);
			if(nLen<0){
				return(-1);
			}
			offset+=nLen;
		}else{
			nLen=strlen(ptr);
			strcpy(buf+offset,ptr);
			offset+=nLen;
		}
		ptr=strtok(NULL,"+");
	}
	return(offset);
}
/**
������:iCountExpressT
����  :����T����(��ȡ)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      T�����÷�:
      T(P1)      ֱ��ȡP1
      T(P1,a)    ȡP1�ĵ�aλ��ʼ����������ַ�(��0��ʼ��)
      T(P1,a,b)  ȡP1�ĵ�aλ��ʼ�Ĺ�b���ֽ�(��0��ʼ��)
**/
int iCountExpressT(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==1){
		return(iCountParamOne(vars, param,value));
	}else if(param->argc==3 || param->argc==2){
		int nFetStart,nFetLength;
		nLen=iCountParamOne(vars, param,buf);
		if(nLen<0){
			return(-1);
		}
		nFetStart=atoi(param->argv[1]);
		if(nFetStart<0)
			nFetStart=0;
		if(param->argc==3){
			nFetLength=atoi(param->argv[2]);
		}else{
			nFetLength=nLen-nFetStart;
		}
		if(nFetStart+nFetLength>nLen){
			nFetLength=nLen-nFetStart;
		}
		memcpy(value,buf+nFetStart,nFetLength);
		return(nFetLength);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressL
����  :����L����(�����,�Ҳ�)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      L�����÷�:
      L(P1,n)    ȡP1�Ҳ��ո�,����nλ
      L(P1,n,a)  ȡP1�Ҳ�a,����nλ
**/
int iCountExpressL(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==2 || param->argc==3){
		int n;
		char a;
		nLen=iCountParamOne(vars, param,buf);
		if(nLen<0){
			return(-1);
		}
		buf[nLen]='\0';
		pub_str_zipspace(buf);
		nLen = strlen(buf);
		n = atoi(param->argv[1]);
		if(n <= nLen)
		{
			/**�����**/
			memcpy(value,buf,n);
			return(n);
		}
		
		if(param->argc == 3)
		{
			a = param->argv[2][0];
		}
		else
		{
			a = ' ';
		}
		
		memcpy(value, buf, nLen);
		memset(value + nLen, a, n - nLen);
		
		return(n);
	}
	else
	{
		return(-1);
	}
	
	return(0);
}
/**
������:iCountExpressR
����  :����R����(�Ҷ���,��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      R�����÷�:
      R(P1,n)    ȡP1�󲹿ո�,����nλ
      R(P1,n,a)  ȡP1��a,����nλ
**/
int iCountExpressR(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==2 || param->argc==3){
		int n;
		char a;
		nLen=iCountParamOne(vars, param,buf);
		if(nLen<0){
			return(-1);
		}
		buf[nLen]='\0';
		pub_str_zipspace(buf);
		nLen=strlen(buf);
		n=atoi(param->argv[1]);
		if(n<=nLen){
			/**�Ҷ���**/
			memcpy(value,buf+nLen-n,n);
			return(n);
		}
		if(param->argc==3){
			a=param->argv[2][0];
		}else{
			a=' ';
		}
		memset(value,a,n-nLen);
		memcpy(value+n-nLen,buf,nLen);
		return(n);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressZ
����  :����Z����(ѹ��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      Z�����÷�:
      Z(P1)   ѹ��P1�еĿո�
**/
int iCountExpressZ(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==1){
		nLen=iCountParamOne(vars, param,buf);
		if(nLen<0){
			return(-1);
		}
		buf[nLen]='\0';
		pub_str_zipspace(buf);
		nLen=strlen(buf);
		memcpy(value,buf,nLen);
		return(nLen);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressA
����  :����A����(��BCD��ת����ASCII��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      A�����÷�:
      A(P1,n)      ȡP1����չASCII��(�����ĳ���Ϊ2*n)
**/
int iCountExpressA(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==2){
		int n;
		n=iCountParamOne(vars, param,buf);
		if(nLen<0){
			return(-1);
		}
		nLen=atoi(param->argv[1]);
		if(nLen>n){
			nLen=n;
		}
		pub_code_bcdtoasc(buf,value,nLen);
		return(nLen*2);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressB
����  :����B����(��ASCII��ת����BCD��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      B�����÷�:
      B(P1,n)      ȡP1��ѹ��BCD��(ѹ����ĳ���Ϊn/2)
**/
int iCountExpressB(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==2){
		int n;
		n=iCountParamOne(vars, param,buf);
		if(nLen<0){
			return(-1);
		}
		nLen=atoi(param->argv[1]);
		if(nLen>n){
			nLen=n;
		}
		pub_code_asctobcd(buf,value,nLen);
		return(nLen/2);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressF
����  :�ѱ���ת���ɱ�����+�ո�+����ֵ+�س��ĸ�ʽ
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      F�����÷�:
      F(P1)
**/
int iCountExpressF(sw_loc_vars_t *vars, TMPARAM *param,char *value){
	char buf[1024];
	int  nLen=0;
	buf[0]='\0';
	if(param->argc==1){
		nLen=iCountParamOne(vars, param,buf);
		if(nLen<=0){
			value[0]='\0';
			return(0);
		}
		buf[nLen]='\0';
		nLen=sprintf(value,"%s %s\n",param->argv[0]+1,buf);
		return(nLen);
	}else{
		return(-1);
	}
	return(0);
}

/**
������:nCountParamOne
����  :�����һ�������Ľ��
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
       �˺���������ʽ�ĵ�һ��������ֵ,ͨ����һ�¼�����ʽ
       ����
       ����
       ����/����+����/����......
       �����µ�С�����аѵ�һ�������ĸ�����ʽ�����ΪP1
**/
int iCountParamOneLen(sw_loc_vars_t *vars, TMPARAM *param){
	char *ptr;
	int  offset=0;
	int  nLen=0;
	ptr=strtok(param->argv[0],"+");
	while(ptr!=NULL){
		if(ptr[0]=='#' || ptr[0]=='$'){
			nLen = vars->get_field_len(vars, ptr);
			if(nLen<0){
				return(-1);
			}
			offset+=nLen;
		}else{
			nLen=strlen(ptr);
			offset+=nLen;
		}
		ptr=strtok(NULL,"+");
	}
	return(offset);
}
/**
������:iCountExpressT
����  :����T����(��ȡ)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      T�����÷�:
      T(P1)      ֱ��ȡP1
      T(P1,a)    ȡP1�ĵ�aλ��ʼ����������ַ�(��0��ʼ��)
      T(P1,a,b)  ȡP1�ĵ�aλ��ʼ�Ĺ�b���ֽ�(��0��ʼ��)
**/
int iCountExpressLenT(sw_loc_vars_t *vars, TMPARAM *param){
	int  nLen=0;

	if(param->argc==1){
		return(iCountParamOneLen(vars, param));
	}else if(param->argc==3 || param->argc==2){
		int nFetStart,nFetLength;
		nLen=iCountParamOneLen(vars, param);
		if(nLen<0){
			return(-1);
		}
		nFetStart=atoi(param->argv[1]);
		if(nFetStart<0)
			nFetStart=0;
		if(param->argc==3){
			nFetLength=atoi(param->argv[2]);
		}else{
			nFetLength=nLen-nFetStart;
		}
		if(nFetStart+nFetLength>nLen){
			nFetLength=nLen-nFetStart;
		}
		return(nFetLength);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressL
����  :����L����(�����,�Ҳ�)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      L�����÷�:
      L(P1,n)    ȡP1�Ҳ��ո�,����nλ
      L(P1,n,a)  ȡP1�Ҳ�a,����nλ
**/
int iCountExpressLenL(sw_loc_vars_t *vars, TMPARAM *param){

	if(param->argc==2 || param->argc==3){
		return atoi(param->argv[1]);
	}
	else
	{
		return(-1);
	}
	
	return(0);
}
/**
������:iCountExpressR
����  :����R����(�Ҷ���,��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      R�����÷�:
      R(P1,n)    ȡP1�󲹿ո�,����nλ
      R(P1,n,a)  ȡP1��a,����nλ
**/
int iCountExpressLenR(sw_loc_vars_t *vars, TMPARAM *param){

	if(param->argc==2 || param->argc==3){
		return atoi(param->argv[1]);
	}else{
		return(-1);
	}

	return(0);
}
/**
������:iCountExpressZ
����  :����Z����(ѹ��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      Z�����÷�:
      Z(P1)   ѹ��P1�еĿո�
**/
int iCountExpressLenZ(sw_loc_vars_t *vars, TMPARAM *param){
	int	nLen = 0;
	if(param->argc==1){
		nLen=iCountParamOneLen(vars, param);
		if(nLen<0){
			return(-1);
		}
		return(nLen);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressA
����  :����A����(��BCD��ת����ASCII��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      A�����÷�:
      A(P1,n)      ȡP1����չASCII��(�����ĳ���Ϊ2*n)
**/
int iCountExpressLenA(sw_loc_vars_t *vars, TMPARAM *param){
	int  nLen=0;
	if(param->argc==2){
		int n;
		n=iCountParamOneLen(vars, param);
		if(nLen<0){
			return(-1);
		}
		nLen=atoi(param->argv[1]);
		if(nLen>n){
			nLen=n;
		}
		return(nLen*2);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressB
����  :����B����(��ASCII��ת����BCD��)
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      B�����÷�:
      B(P1,n)      ȡP1��ѹ��BCD��(ѹ����ĳ���Ϊn/2)
**/
int iCountExpressLenB(sw_loc_vars_t *vars, TMPARAM *param){
	int  nLen=0;
	if(param->argc==2){
		int n;
		n=iCountParamOneLen(vars, param);
		if(nLen<0){
			return(-1);
		}
		nLen=atoi(param->argv[1]);
		if(nLen>n){
			nLen=n;
		}
		return(nLen/2);
	}else{
		return(-1);
	}
	return(0);
}
/**
������:iCountExpressF
����  :�ѱ���ת���ɱ�����+�ո�+����ֵ+�س��ĸ�ʽ
����  :�����ṹ
       ���ؽ���ַ���
����ֵ:��������/-1
˵��:
      F�����÷�:
      F(P1)
**/
int iCountExpressLenF(sw_loc_vars_t *vars, TMPARAM *param){
	int	nLen = 0;
	if(param->argc==1){
		nLen=iCountParamOneLen(vars, param);
		if(nLen<=0){
			return(0);
		}
		nLen += strlen(param->argv[0] + 1) + 2;
		return(nLen);
	}else{
		return(-1);
	}
	return(0);
}
