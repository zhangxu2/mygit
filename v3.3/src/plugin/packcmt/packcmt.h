#ifndef __PACKCMT_H__
#define __PACKCMT_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pub_log.h"
#include "pub_vars.h"
#include "pub_xml.h"

#define RESPCD_REJECT1 "9991"
#define MAX_COUNT	2000
#define	MAX_VARMAP_NUM	1024
#define MAX_MAPALIAS_LEN 256
#define MAX_TAGNAME_LEN  256  /*tag别名的最大长度*/
#define MAX_TAGLIST_ITEM 256  /*taglist中tag的最大个数*/
#define MAX_TAGVALUE_LEN 1024 /*tag报文域的最大长度*/

typedef struct
{
   int  iNum;
   int  iMax;
   int  iVarlength;
   int  iIndex;
   int  iBegin;
   int  iStart;
   int  iLength;
   char cVarname[21];
   char cBitname[10];
   char cVartype[21];
   char cConstvalue[1024];
   char cCode[10];/*编码方式*/
   char cLenCode[10];/*长度域的编码方式*/
   char cType;
   char cConst;
   char cBlank;
   char cFlag;
   char cVarvalue[2048];
   char cSep[10];
   char cMiddle[10];
}ST_ITEM;

typedef struct
{
	char	cVarname[21];
	char	cVarvalue[1024];
}ST_FIRST;


typedef struct
{
	int	iFlag;
	int	iCount;
	struct
	{
		char	cMapFlag;
		char	sVarName[128];
		char	sVarValue[128];
		char	sFirstNo[32];    /*** 一代报文编号 ***/
		char	sSecondNo[128];  /*** 二代报文编号 ***/
		char	sFirstVar[128];  /*** 一代变量的值 ***/
		char	sSecondVar[128]; /*** 二代变量的值 ***/
	}stVarMap[MAX_VARMAP_NUM];
}ST_VARMAP;

typedef struct
{
	int	iFlag; 
	int	iCount;
	struct
	{
		int	iCheck;
		int	iLength;
		char	sCheckVar[128];
		char	sCheckValue[128];
		char	sFirstNo[21];    /*** 一代报文编号 ***/
		char	sSecondNo[21];  /*** 二代报文编号 ***/
	}stPkgNoMap[MAX_VARMAP_NUM];
}ST_PKGNOMAP;

typedef struct
{
	int	iWidth;  /*** 附加域宽度 ***/
	int	iLength; /*** 附加域长度大小***/
	int	iOffSet; /*** 附加域起始位置(不包含标签本身) ***/
}ST_APPEND;

typedef struct stTagMap
{
	char	cFlag; /*** 变量映射成XML标记，1为映射，非1不映射默认不映射成xml ***/
	char	cType;   /*** 数据类型 ***/
	char	cConst;
	char	cDate;
	char	cAmt;
	char	cVarLen;
	char	cMapFlag;
	char	cSendFlag;
	char	cOptional;
	char	cAnalysis;/*解析标志,0--不解析，丢弃1--解析，2--循环，3--循环开始，4--循环结束，9--循环次数*/
	char	cNodeType; /*** XML节点类型 1表示XML属性节点, 非1为XML子节点 ***/
	int	iLength; /*** 数据长度 ***/
	int	iBegin;  /*** 数据起始位置 ***/
	int	iMapBegin;
	char	sType[6];/*** 数据类型 + 数据长度 ***/
	char	sCheckVar[MAX_TAGNAME_LEN];  /*** CHECK选项 ***/
	char	sCheckValue[MAX_TAGVALUE_LEN]; /***CHECK值 ***/
	char	sAlias[MAX_MAPALIAS_LEN]; /*** 映射后的变量名 ***/
	char	sExplain[MAX_TAGNAME_LEN]; /*** 参数名称 ***/
	char	sVarvalue[MAX_TAGVALUE_LEN];
	char	sConstValue[MAX_TAGVALUE_LEN];
}ST_MAP;

/*tag配置文件的内容：序号	参数代码	参数名称	TAG值 数据类型 数据长度 强制标识 解析标志 功能标识*/
typedef struct stTagNode
{	
	char	cVarLen;
	char	cFlag;        /***映射时使用, cFlag = '1'时,tag为多对一映射, 否则按一对多处理 ***/
	char	cConst;       /*** 暂时用'1'表示块标识 ***/
	char	cType;	      /*** 数据类型 a-字符, n-数值 ***/
	char	cNum;         /*** 收款人/付款人数目标识 ***/ 
	char	cList;        /*** 收款人/付款人清单标识 ***/
	char	cOptional;    /*** 强制标识,必选域(M)在报文中必须存在,可选域(O)在报文中，可以存在也可以不存在 ***/
	char	cAnalysis;    /*解析标志,0--不解析，丢弃1--解析，2--循环，3--循环开始，4--循环结束，9--循环次数*/
	char	cNodeType;    /*** '1'--xml属性  非'1'--子节点 ***/
	char	cNextIsVarLen;/*指示下一个业务要素是否是变长的。如果该项值为"1",表示该项内容是下一项变长数据的长度，否则为"0"*/
	int 	iLength;	   /*数据长度,部分域是变长（比如图像数据），其长度配置为0，具体长度由图像长度确定*/
	char	sTag[6];      /*TAG值*/
	char	sType[6];     /*数据类型+数据长度*/
	char	sCheckVar[MAX_TAGNAME_LEN];  /*** CHECK选项 ***/
	char	sCheckValue[MAX_TAGVALUE_LEN]; /***CHECK值 ***/
	char	sAlias[MAX_TAGNAME_LEN];/*参数代码，是在变量池中保存tag报文域内容的平台变量名*/
	char	sExplain[MAX_TAGNAME_LEN];/*参数名称,即TAG域名*/
	char	sTagValue[MAX_TAGVALUE_LEN];/*报文域中的值*/
	char	sConstValue[MAX_TAGVALUE_LEN];/*报文域中的值*/
}TAGNODE;

typedef struct PkgInfoNode
{
	
	char    cPkgType;     /*** 1:大额报文(CMT) 2:小额CMT 3:小额PKG ***/
	int     iCount;       /*** 明细笔数 ***/
	char    sCmtNo[8];    /*** 报文头中的CMT号码3位*/
	char    sFactorNo[8]; /*** 业务要素集号 ***/
	char    sPkgNo[8];    /*** 报文编号 ***/
	char	sTxType[6];   /*** 业务类型号 ***/
	char	sTxKind[13];  /*** 业务种类 ***/
	char    sMsgType[32]; /*** 二代报文的报文MsgType ***/
	char    sType[MAX_COUNT][6]; /*** 业务类型号数组,暂时按每种业务至多50种业务类型号处理 ***/
}ST_PKGINFO;

/*函数声明*/
int iCheckTx(char *sVarvalue, char *value);
sw_xmlnode_t *pstGetNodeByTarget(sw_xmltree_t *xmltree,char *psTargetName,char sTarg[64],char sName[64]);
sw_xmlnode_t *vars_extvar_get_node(sw_xmltree_t *pxml, sw_xmlnode_t *pnode, char *express);
char *msstrtok(char *instr, char *outstr, char *delimiter);
sw_xmlnode_t *pstLocGetExtVar(sw_loc_vars_t *vars,char *name,int creat);
char * psLocGetExtVarAttr(sw_loc_vars_t *locvars,sw_xmlnode_t *node,char *attr);
char *pcGetXmlVarAttr(sw_xmltree_t *xml, sw_xmlnode_t *pvar, char *attr);
int iGetXmlAttrVar(sw_xmltree_t *xml, char *varname, char *attrvalue);
char *psGetAttrName(char *nodename, char *attrname);
int iSetXmlVarAttr(sw_xmltree_t *xml, sw_xmlnode_t *pvar, char *attr, char *avalue);
int iSetXmlAttrVar(sw_xmltree_t *xml, char *varname, char *attrvalue);
sw_xmlnode_t *poGetXmlVar(sw_xmltree_t *xml, char *name);
sw_xmlnode_t *poLocGetXmlVar(sw_xmltree_t *xml, char *name, int creat);
int iGetXmlVariable(sw_xmltree_t *xml, char *varname, char *value);
int iSetXmlVariable(sw_xmltree_t *xml, char * varname, char *value);
int iGetXmlVar(sw_loc_vars_t *locvars, char *varname, char *value);
int iGetXmlAttr(sw_loc_vars_t *locvars, char *varname, char *attrvalue);
char *zip_space(char *str);
char *zip_head(char *str);
char *zip_tail(char *str);
char *zip_bothsides(char *str);
int iDate8To10(char *sDate8, char *sDate10);
int iDate10To8(char *sDate10, char *sDate8);
#endif
