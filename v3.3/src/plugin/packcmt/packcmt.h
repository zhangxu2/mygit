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
#define MAX_TAGNAME_LEN  256  /*tag��������󳤶�*/
#define MAX_TAGLIST_ITEM 256  /*taglist��tag��������*/
#define MAX_TAGVALUE_LEN 1024 /*tag���������󳤶�*/

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
   char cCode[10];/*���뷽ʽ*/
   char cLenCode[10];/*������ı��뷽ʽ*/
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
		char	sFirstNo[32];    /*** һ�����ı�� ***/
		char	sSecondNo[128];  /*** �������ı�� ***/
		char	sFirstVar[128];  /*** һ��������ֵ ***/
		char	sSecondVar[128]; /*** ����������ֵ ***/
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
		char	sFirstNo[21];    /*** һ�����ı�� ***/
		char	sSecondNo[21];  /*** �������ı�� ***/
	}stPkgNoMap[MAX_VARMAP_NUM];
}ST_PKGNOMAP;

typedef struct
{
	int	iWidth;  /*** �������� ***/
	int	iLength; /*** �����򳤶ȴ�С***/
	int	iOffSet; /*** ��������ʼλ��(��������ǩ����) ***/
}ST_APPEND;

typedef struct stTagMap
{
	char	cFlag; /*** ����ӳ���XML��ǣ�1Ϊӳ�䣬��1��ӳ��Ĭ�ϲ�ӳ���xml ***/
	char	cType;   /*** �������� ***/
	char	cConst;
	char	cDate;
	char	cAmt;
	char	cVarLen;
	char	cMapFlag;
	char	cSendFlag;
	char	cOptional;
	char	cAnalysis;/*������־,0--������������1--������2--ѭ����3--ѭ����ʼ��4--ѭ��������9--ѭ������*/
	char	cNodeType; /*** XML�ڵ����� 1��ʾXML���Խڵ�, ��1ΪXML�ӽڵ� ***/
	int	iLength; /*** ���ݳ��� ***/
	int	iBegin;  /*** ������ʼλ�� ***/
	int	iMapBegin;
	char	sType[6];/*** �������� + ���ݳ��� ***/
	char	sCheckVar[MAX_TAGNAME_LEN];  /*** CHECKѡ�� ***/
	char	sCheckValue[MAX_TAGVALUE_LEN]; /***CHECKֵ ***/
	char	sAlias[MAX_MAPALIAS_LEN]; /*** ӳ���ı����� ***/
	char	sExplain[MAX_TAGNAME_LEN]; /*** �������� ***/
	char	sVarvalue[MAX_TAGVALUE_LEN];
	char	sConstValue[MAX_TAGVALUE_LEN];
}ST_MAP;

/*tag�����ļ������ݣ����	��������	��������	TAGֵ �������� ���ݳ��� ǿ�Ʊ�ʶ ������־ ���ܱ�ʶ*/
typedef struct stTagNode
{	
	char	cVarLen;
	char	cFlag;        /***ӳ��ʱʹ��, cFlag = '1'ʱ,tagΪ���һӳ��, ����һ�Զദ�� ***/
	char	cConst;       /*** ��ʱ��'1'��ʾ���ʶ ***/
	char	cType;	      /*** �������� a-�ַ�, n-��ֵ ***/
	char	cNum;         /*** �տ���/��������Ŀ��ʶ ***/ 
	char	cList;        /*** �տ���/�������嵥��ʶ ***/
	char	cOptional;    /*** ǿ�Ʊ�ʶ,��ѡ��(M)�ڱ����б������,��ѡ��(O)�ڱ����У����Դ���Ҳ���Բ����� ***/
	char	cAnalysis;    /*������־,0--������������1--������2--ѭ����3--ѭ����ʼ��4--ѭ��������9--ѭ������*/
	char	cNodeType;    /*** '1'--xml����  ��'1'--�ӽڵ� ***/
	char	cNextIsVarLen;/*ָʾ��һ��ҵ��Ҫ���Ƿ��Ǳ䳤�ġ��������ֵΪ"1",��ʾ������������һ��䳤���ݵĳ��ȣ�����Ϊ"0"*/
	int 	iLength;	   /*���ݳ���,�������Ǳ䳤������ͼ�����ݣ����䳤������Ϊ0�����峤����ͼ�񳤶�ȷ��*/
	char	sTag[6];      /*TAGֵ*/
	char	sType[6];     /*��������+���ݳ���*/
	char	sCheckVar[MAX_TAGNAME_LEN];  /*** CHECKѡ�� ***/
	char	sCheckValue[MAX_TAGVALUE_LEN]; /***CHECKֵ ***/
	char	sAlias[MAX_TAGNAME_LEN];/*�������룬���ڱ������б���tag���������ݵ�ƽ̨������*/
	char	sExplain[MAX_TAGNAME_LEN];/*��������,��TAG����*/
	char	sTagValue[MAX_TAGVALUE_LEN];/*�������е�ֵ*/
	char	sConstValue[MAX_TAGVALUE_LEN];/*�������е�ֵ*/
}TAGNODE;

typedef struct PkgInfoNode
{
	
	char    cPkgType;     /*** 1:����(CMT) 2:С��CMT 3:С��PKG ***/
	int     iCount;       /*** ��ϸ���� ***/
	char    sCmtNo[8];    /*** ����ͷ�е�CMT����3λ*/
	char    sFactorNo[8]; /*** ҵ��Ҫ�ؼ��� ***/
	char    sPkgNo[8];    /*** ���ı�� ***/
	char	sTxType[6];   /*** ҵ�����ͺ� ***/
	char	sTxKind[13];  /*** ҵ������ ***/
	char    sMsgType[32]; /*** �������ĵı���MsgType ***/
	char    sType[MAX_COUNT][6]; /*** ҵ�����ͺ�����,��ʱ��ÿ��ҵ������50��ҵ�����ͺŴ��� ***/
}ST_PKGINFO;

/*��������*/
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
