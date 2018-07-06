/***************************************************************************
 *** 程序作者 : Shi Shenghui  石升辉                                      **
 *** 日    期 : 2007-08-17                                                **
 *** 所属模块 :                                                          ***
 *** 程序名称 : minifun.c                                                ***
 *** 程序作用 : 报文转换的小函数                                     ***
 *** 使用注意 :                                                          ***
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
函数名:iAnalyzeMiniParam
功能  :解析小函数参数
参数  :无
返回值:0/-1
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
函数名:pub_comvars_get
功能  :得到包含小函数的变量表达式的值
参数  :表达式(不能大于128个字符)
返回值:变量长度/-1
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
		case 'T':/**截取**/
			return(iCountExpressT(vars, &param, value));
		case 'L':/**左对齐**/
			return(iCountExpressL(vars, &param, value));
		case 'R':/**右对齐**/
			return(iCountExpressR(vars, &param, value));
		case 'Z':/**压缩**/
			return(iCountExpressZ(vars, &param, value));
		case 'A':/**BCD转成ASCII**/
			return(iCountExpressA(vars, &param, value));
		case 'B':/**ASCII转成BCD**/
			return(iCountExpressB(vars, &param, value));
		case 'F':/**name value\n 格式**/
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
		case 'T':/**截取**/
			return(iCountExpressLenT(vars, &param));
		case 'L':/**左对齐**/
			return(iCountExpressLenL(vars, &param));
		case 'R':/**右对齐**/
			return(iCountExpressLenR(vars, &param));
		case 'Z':/**压缩**/
			return(iCountExpressLenZ(vars, &param));
		case 'A':/**BCD转成ASCII**/
			return(iCountExpressLenA(vars, &param));
		case 'B':/**ASCII转成BCD**/
			return(iCountExpressLenB(vars, &param));
		case 'F':/**name value\n 格式**/
			return(iCountExpressLenF(vars, &param));
		default:
			return(-1);
	}
	return(0);
}

/**
函数名:nCountParamOne
功能  :计算第一个参数的结果
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
       此函数计算表达式的第一个参数的值,通常有一下集中形式
       变量
       常量
       变量/常量+变量/常量......
       在以下的小函数中把第一个参数的各种形式都简称为P1
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
函数名:iCountExpressT
功能  :计算T函数(截取)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      T函数用法:
      T(P1)      直接取P1
      T(P1,a)    取P1的第a位开始往后的所有字符(从0开始记)
      T(P1,a,b)  取P1的第a位开始的共b个字节(从0开始记)
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
函数名:iCountExpressL
功能  :计算L函数(左对齐,右补)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      L函数用法:
      L(P1,n)    取P1右补空格,补足n位
      L(P1,n,a)  取P1右补a,补足n位
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
			/**左对齐**/
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
函数名:iCountExpressR
功能  :计算R函数(右对齐,左补)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      R函数用法:
      R(P1,n)    取P1左补空格,补足n位
      R(P1,n,a)  取P1左补a,补足n位
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
			/**右对齐**/
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
函数名:iCountExpressZ
功能  :计算Z函数(压缩)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      Z函数用法:
      Z(P1)   压缩P1中的空格
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
函数名:iCountExpressA
功能  :计算A函数(把BCD码转换成ASCII码)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      A函数用法:
      A(P1,n)      取P1的扩展ASCII码(扩完后的长度为2*n)
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
函数名:iCountExpressB
功能  :计算B函数(把ASCII码转换成BCD码)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      B函数用法:
      B(P1,n)      取P1的压缩BCD码(压缩后的长度为n/2)
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
函数名:iCountExpressF
功能  :把变量转换成变量名+空格+变量值+回车的格式
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      F函数用法:
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
函数名:nCountParamOne
功能  :计算第一个参数的结果
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
       此函数计算表达式的第一个参数的值,通常有一下集中形式
       变量
       常量
       变量/常量+变量/常量......
       在以下的小函数中把第一个参数的各种形式都简称为P1
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
函数名:iCountExpressT
功能  :计算T函数(截取)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      T函数用法:
      T(P1)      直接取P1
      T(P1,a)    取P1的第a位开始往后的所有字符(从0开始记)
      T(P1,a,b)  取P1的第a位开始的共b个字节(从0开始记)
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
函数名:iCountExpressL
功能  :计算L函数(左对齐,右补)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      L函数用法:
      L(P1,n)    取P1右补空格,补足n位
      L(P1,n,a)  取P1右补a,补足n位
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
函数名:iCountExpressR
功能  :计算R函数(右对齐,左补)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      R函数用法:
      R(P1,n)    取P1左补空格,补足n位
      R(P1,n,a)  取P1左补a,补足n位
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
函数名:iCountExpressZ
功能  :计算Z函数(压缩)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      Z函数用法:
      Z(P1)   压缩P1中的空格
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
函数名:iCountExpressA
功能  :计算A函数(把BCD码转换成ASCII码)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      A函数用法:
      A(P1,n)      取P1的扩展ASCII码(扩完后的长度为2*n)
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
函数名:iCountExpressB
功能  :计算B函数(把ASCII码转换成BCD码)
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      B函数用法:
      B(P1,n)      取P1的压缩BCD码(压缩后的长度为n/2)
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
函数名:iCountExpressF
功能  :把变量转换成变量名+空格+变量值+回车的格式
参数  :参数结构
       返回结果字符串
返回值:变量长度/-1
说明:
      F函数用法:
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
