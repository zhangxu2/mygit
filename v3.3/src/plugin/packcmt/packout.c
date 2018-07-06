#include "packcmt.h"

int iDealTailOut(char *pkg, int *iLen)
{	
	int	iCurLen = 0;
	char	sTail[64];		
	
	memset(sTail, 0x0, sizeof(sTail));	

	iCurLen = *iLen;
	pub_log_info("[%s][%d] �鱨��β֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	memcpy(sTail, "{C:", 3);	
	memset(sTail + 3, 'k', 10);
	memset(sTail + 13, ' ', 22);
	memcpy(sTail + 35, "}", 1);		
	
	pub_log_info("[%s][%d] sTail===[%s] strlen(sTail)=[%d]", __FILE__, __LINE__, 
		sTail, strlen(sTail));		
	
	memcpy(pkg + iCurLen, sTail, 36);
	iCurLen += 36;
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] �鱨��֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_bin(SW_LOG_DEBUG, pkg, iCurLen, "[%s][%d] �鱨�Ľ���,���ĳ���[%d]\n",
		__FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iInitPkgOut
* ��������:	�����ʼ��
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-25
* ��    ��:
*		   locvars �����ػ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iCheckTx(char *sVarvalue, char *value);
int iInitPkgOut(sw_loc_vars_t *locvars, ST_PKGINFO *pstPkgInfo)
{
	int	i = 0;
	int	iRc = 0;
	char	sTmp[512];
	char	sVarname[512];
	char	sVarvalue[512];
	char	sXmlName[512];
	char	sTxVarname[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*savenode = NULL;
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sVarname, 0x0, sizeof(sVarname));
	memset(sVarvalue, 0x0, sizeof(sVarvalue));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	memset(sTxVarname, 0x0, sizeof(sTxVarname));

	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/map.xml", getenv("HOME"));
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/map.xml", getenv("SWWORK"));
	}
	
	pub_log_info("[%s][%d] sXmlName=[%s]\n", __FILE__, __LINE__, sXmlName);
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��XML��ʧ��,����XML�ļ�[%s]�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	pnode = pub_xml_locnode(pXml, "MAP.FIRSTWORK.MSGTYPE");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ����[MAP.FIRSTWORK.MSGTYPE]��ǩ!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		return -1;
	}
	memset(sTmp, 0x0, sizeof(sTmp));
	strcpy(sTmp, pnode->value);
	pub_log_info("[%s][%d] MSGTYPE����Ϊ[%s]", __FILE__, __LINE__, sTmp);
	if (sTmp[0] == '.')
	{
		if (locvars == NULL)
		{
			
			get_zd_data(sTmp, pstPkgInfo->sMsgType);
		}
		else
		{
			loc_get_zd_data(locvars, sTmp, pstPkgInfo->sMsgType);
		}
	}
	else
	{
		if (locvars == NULL)
		{
			get_zd_data(sTmp, pstPkgInfo->sMsgType);
		}
		else
		{
			loc_get_zd_data(locvars, sTmp, pstPkgInfo->sMsgType);
		}
	}
	zip_space(pstPkgInfo->sMsgType);
	pub_log_info("[%s][%d] MSGTYPE=[%s]", __FILE__, __LINE__, pstPkgInfo->sMsgType);
	
	pnode = pub_xml_locnode(pXml, "MAP.SECOND");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ����[MAP.SECOND]��ǩ!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		return -1;
	}
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "SECOND") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "MSGTYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����MAP.SECOND.MSGTYPE! MSGTYPE=[%s]", 
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);	
			return -1;
		}
		if (strcmp(node1->value, pstPkgInfo->sMsgType) == 0)
		{
			break;
		}	
		pnode = pnode->next;
	}
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] û���ҵ�ƥ���MSGTYPE! MSGTYPE=[%s]",
			__FILE__, __LINE__, pstPkgInfo->sMsgType);
		pub_xml_deltree(pXml);
		return -1;
	}
	
	pXml->current = pnode;
	savenode = pnode;
	pnode = pub_xml_locnode(pXml, "TXKIND");
	if (pnode != NULL)
	{
		memset(sVarname, 0x0, sizeof(sVarname));
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		strcpy(sVarname, pnode->value);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else
		{
			strcpy(sVarvalue, sVarname);
		}
		strcpy(pstPkgInfo->sTxKind, sVarvalue);
	}	
	pnode = pub_xml_locnode(pXml, "COUNT");
	if (pnode == NULL)
	{
		pstPkgInfo->iCount = 1;
	}
	else 
	{
		memset(sVarname, 0x0, sizeof(sVarname));
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		strcpy(sVarname, pnode->value);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else
		{
			strcpy(sVarvalue, sVarname);
		}
		pstPkgInfo->iCount = atoi(sVarvalue);
		if (pstPkgInfo->iCount == 0)
		{
			pub_log_error("[%s][%d] ��ϸ��������=[%s] ֵ=[%s]����ϸ����Ϊ0!",
				__FILE__, __LINE__, sVarname, sVarvalue);
			return -1;
		}
	}
	pub_log_info("[%s][%d] ��ϸ�ܱ���=[%d]", __FILE__, __LINE__, pstPkgInfo->iCount);	
	
	node1 = pub_xml_locnode(pXml, "TXTYPE");
	if (node1 != NULL)
	{
		strcpy(sTxVarname, node1->value);
		pub_log_info("[%s][%d] sTxVarname=[%s]", __FILE__, __LINE__, sTxVarname);
	}
	for (i = 0; i < pstPkgInfo->iCount; i++)
	{
		pub_log_info("[%s][%d] i=[%d]", __FILE__, __LINE__, i);
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		memset(sVarname, 0x0, sizeof(sVarname));
		sprintf(sVarname, sTxVarname, i);
		pub_log_info("[%s][%d] sVarname=[%s]", __FILE__, __LINE__, sVarname);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else
		{
			strcpy(sVarvalue, sVarname);
		}
		strcpy(pstPkgInfo->sTxType, sVarvalue);
		pub_log_info("[%s][%d] sVarvalue=[%s]", __FILE__, __LINE__, sVarvalue);
		pXml->current = savenode;
		node1 = pub_xml_locnode(pXml, "FIRST");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����FIRST! MSGTYPE=[%s]", 
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "FIRST") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "TXTYPE");
			if (node2 != NULL)
			{
				pub_log_info("[%s][%d] pstPkgInfo->sTxType=[%s] node2->value=[%s]",
					__FILE__, __LINE__, pstPkgInfo->sTxType, node2->value);
				if (iCheckTx(pstPkgInfo->sTxType, node2->value) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			node2 = pub_xml_locnode(pXml, "TXKIND");
			if (node2 != NULL)
			{
				if (iCheckTx(pstPkgInfo->sTxKind, node2->value) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			break;
		}
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] û���ҵ�ƥ���FIRST! MSGTYPE=[%s]",
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		pXml->current = node1;
		node1 = pub_xml_locnode(pXml, "PKGNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����PKGNO! MSGTYPE=[%s]", 
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		memset(sVarname, 0x0, sizeof(sVarname));
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		strcpy(sVarname, node1->value);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else 
		{
			strcpy(sVarvalue, sVarname);
		}
		strcpy(pstPkgInfo->sPkgNo, sVarvalue);
		pub_log_info("[%s][%d] ���ı��=[%s]", 
			__FILE__, __LINE__, pstPkgInfo->sPkgNo);
		
		node1 = pub_xml_locnode(pXml, "SYSTYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����SYSTYPE! MSGTYPE=[%s]",
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		memset(sVarname, 0x0, sizeof(sVarname));
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		strcpy(sVarname, node1->value);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else 
		{
			strcpy(sVarvalue, sVarname);
		}
		pub_log_info("[%s][%d] ϵͳ���=[%s]", __FILE__, __LINE__, sVarvalue);
		if (strcmp(sVarvalue, "HVPS") == 0)
		{
			/*** ���(CMT) ***/
			pstPkgInfo->cPkgType = '1';
			pub_xml_deltree(pXml);
			return 0;
		}
		
		node1 = pub_xml_locnode(pXml, "PKGTYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����PKGTYPE! MSGTYPE=[%s]",
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		memset(sVarname, 0x0, sizeof(sVarname));
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		strcpy(sVarname, node1->value);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else 
		{
			strcpy(sVarvalue, sVarname);
		}
		pub_log_info("[%s][%d] �������=[%s]", __FILE__, __LINE__, sVarname);
		if (strcmp(sVarvalue, "CMT") == 0)
		{
			/*** С��CMT ***/
			pstPkgInfo->cPkgType = '2';
			pub_xml_deltree(pXml);
			return 0;
		}
		pstPkgInfo->cPkgType = '3';

		node1 = pub_xml_locnode(pXml, "FACTORNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δָ��FACTORNO! MSGTYPE=[%s]",
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		memset(sVarname, 0x0, sizeof(sVarname));
		memset(sVarvalue, 0x0, sizeof(sVarvalue));
		strcpy(sVarname, node1->value);
		if (sVarname[0] == '.')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else if (sVarname[0] == '#')
		{
			if (locvars == NULL)
			{
				get_zd_data(sVarname, sVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, sVarname, sVarvalue);
			}
		}
		else 
		{
			strcpy(sVarvalue, sVarname);
		}
		strcpy(pstPkgInfo->sFactorNo, sVarvalue);
		pub_log_info("[%s][%d] ҵ��Ҫ�ؼ���=[%s]", __FILE__, __LINE__, sVarvalue);
		
		node1 = pub_xml_locnode(pXml, "FACTOR.TXTYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δָ��[FACTOR.TXTYPE]��ǩ! MSGTYPE=[%s]", 
				__FILE__, __LINE__, pstPkgInfo->sMsgType);
			pub_xml_deltree(pXml);
			return -1;
		}
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "TXTYPE") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "TYPE");
			if (node2 != NULL)
			{
				iRc = iCheckTx(pstPkgInfo->sTxType, node2->value);
				if (iRc != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			strcpy(pstPkgInfo->sType[i], node1->value);
			pub_log_info("[%s][%d] sType[%d]=[%s]", 
				__FILE__, __LINE__, i, pstPkgInfo->sType[i]);
			break;
		}
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] û���ҵ�ƥ���ҵ�����ͺ�!"
				"MSGTYPE=[%s]  FACTONO=[%s] TXTYPE=[%s]",
				__FILE__, __LINE__, pstPkgInfo->sMsgType, 
				pstPkgInfo->sFactorNo, pstPkgInfo->sTxType);
			pub_xml_deltree(pXml);
			return -1;
		}
	}
		
	pub_xml_deltree(pXml);
	return 0;
}

/****************************************************
* �� �� ��:	iBasHeadMapOut
* ��������:	������ͷ����ӳ��
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-25
* ��    ��:
*		   locvars �����ػ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iBasHeadMapOut(sw_loc_vars_t *locvars, ST_PKGINFO stPkgInfo)
{
	char	sTmp[512];
	char	*ptr = NULL;
	char	sAppTradeCode[9];
	char	sSendDate[9];
	char	sSendTime[7];
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
	memset(sSendDate, 0x0, sizeof(sSendDate));
	memset(sSendTime, 0x0, sizeof(sSendTime));
	
	if (locvars == NULL)
	{
		get_zd_data("#OrigSendTime", sSendTime);
		get_zd_data("#OrigSendDate", sSendDate);
		pub_log_info("[%s][%d] #OrigSendDate=[%s] #OrigSendTime=[%s]",
			__FILE__, __LINE__, sSendTime, sSendDate);
		memcpy(sTmp, sSendDate, 8);
		memcpy(sTmp + 8, sSendTime, 6);
		pub_log_info("[%s][%d] ����ʱ��=[%s]", __FILE__, __LINE__, sTmp);
		set_zd_data("#OrigSendTime", sTmp);
		
		memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
		if (stPkgInfo.cPkgType == '1')
		{
			sAppTradeCode[0] = '1';
		}
		else if (stPkgInfo.cPkgType == '2' || stPkgInfo.cPkgType == '3')
		{
			/*** С�� ***/
			sAppTradeCode[0] = '2';
		}
		else
		{
			pub_log_error("[%s][%d] ϵͳ���ͺ�����! cPkgType=[%c]",
				__FILE__, __LINE__, stPkgInfo.cPkgType);
			return -1;
		}
		
		ptr = strchr(stPkgInfo.sPkgNo, '_');
		if (ptr != NULL)
		{
			memcpy(sAppTradeCode + 1, ptr + 1, 3);
		}
		else
		{
			memcpy(sAppTradeCode + 1, stPkgInfo.sPkgNo, 3);
		}
		pub_log_info("[%s][%d] sPkgNo=[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
		pub_log_info("[%s][%d] appTradeCode=[%s]", __FILE__, __LINE__, sAppTradeCode);
		set_zd_data("$appTradeCode", sAppTradeCode);
	}
	else
	{
		loc_get_zd_data(locvars, "#OrigSendTime", sSendTime);
		loc_get_zd_data(locvars, "#OrigSendDate", sSendDate);
		pub_log_info("[%s][%d] #OrigSendDate=[%s] #OrigSendTime=[%s]",
			__FILE__, __LINE__, sSendTime, sSendDate);
		memcpy(sTmp, sSendDate, 8);
		memcpy(sTmp + 8, sSendTime, 6);
		pub_log_info("[%s][%d] ����ʱ��=[%s]", __FILE__, __LINE__, sTmp);
		loc_set_zd_data(locvars, "#OrigSendTime", sTmp);
		
		memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
		ptr = strchr(stPkgInfo.sPkgNo, '_');
		if (ptr != NULL)
		{
			memcpy(sAppTradeCode + 1, ptr + 1, 3);
		}
		else
		{
			memcpy(sAppTradeCode + 1, stPkgInfo.sPkgNo, 3);
		}
		
		if (stPkgInfo.cPkgType == '1')
		{
			sAppTradeCode[0] = '1';
		}
		else if (stPkgInfo.cPkgType == '2' || stPkgInfo.cPkgType == '3')
		{
			/*** С�� ***/
			sAppTradeCode[0] = '2';
		}
		pub_log_info("[%s][%d] $appTradeCode=[%s]", __FILE__, __LINE__, sAppTradeCode);
		loc_set_zd_data(locvars, "$appTradeCode", sAppTradeCode);
	}
	
	return 0;
}

/****************************************************
* �� �� ��:	iDealBasHeadOut
* ��������:	�鱨��ͷ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pkgnode   ITEM�ڵ�
*          pXml      XML��
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iDealBasHeadOut(locvars, node, pXml, pkg, iLen)
sw_loc_vars_t *locvars;
sw_xmlnode_t *node;
sw_xmltree_t *pXml;
char *pkg;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	int	iTmpLen = 0;
	char	sVarName[64];
	char 	sDate[9];
	long	lOrderno = 0L;
	char	*pkgBuf = NULL;
	ST_ITEM	stItem;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sVarName, 0x0, sizeof(sVarName));
	memset(sDate, 0x0, sizeof(sDate));
	iCurLen = *iLen;
	pub_log_info("[%s][%d] �鱨ͷ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	iInitItem(&stItem);
	pkgBuf = pkg;
	pnode = node;
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		iInitItem(&stItem);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "INCLUDE");
		if (node1 != NULL)
		{
			get_zd_data("#firstvarname", sVarName);
			if (iLocChkinclude(locvars, sVarName, node2->value) != 0)
			{
				pnode = pnode->next;
				continue;
			}
		}

		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		memcpy(stItem.cVarname, node1->value, sizeof(stItem.cVarname) - 1);
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stItem.cConst = '1';
			strcpy(stItem.cConstvalue, node1->value);
		}
		
		if (stItem.cConst == '1')
		{	
			/*** ���� ***/
			strcpy(stItem.cVarvalue, stItem.cConstvalue);
		}
		else
		{	
			if (locvars == NULL)
			{	
				get_zd_data(stItem.cVarname, stItem.cVarvalue);
			}	
			else
			{	
				loc_get_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
			}
		#if 0	
			if (memcmp(stItem.cVarname,"#MesgID", 7) == 0 || memcmp(stItem.cVarname,"#MesgRefID", 10) == 0)
			{	
				lOrderno = 0L;
				memset(sDate, 0x0, sizeof(sDate));
				
				memcpy(sDate, stItem.cVarvalue, 8);
				lOrderno = atol(stItem.cVarvalue+8);
				
				memset(stItem.cVarvalue, 0x0, sizeof(stItem.cVarvalue));
				sprintf(stItem.cVarvalue, "%s%012ld", sDate, lOrderno);
			}
		#endif
		}
		
		if (stItem.cType == 'a')
		{
			memset(pkgBuf + iCurLen, ' ', stItem.iLength);
		}
		else if (stItem.cType == 'n')
		{
			memset(pkgBuf + iCurLen, '0', stItem.iLength);
		}
		else
		{
			pub_log_error("[%s][%d] ��������=[%c]ӦΪa��n,�Ҳ����������!",
				__FILE__, __LINE__, stItem.cType);
			return -1;
		}
		zip_tail(stItem.cVarvalue); 
		iTmpLen = strlen(stItem.cVarvalue);
		if (stItem.cType == 'n')
		{
			memcpy(pkgBuf + iCurLen + stItem.iLength - iTmpLen, stItem.cVarvalue, iTmpLen);
		}
		else
		{
			memcpy(pkgBuf + iCurLen, stItem.cVarvalue, iTmpLen);
		}
		iCurLen += stItem.iLength;
		pnode = pnode->next;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ���걨��ͷ֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_debug("[%s][%d] ����ͷ�������! ����ͷ����=[%d]",
		__FILE__, __LINE__, iCurLen);
	pub_log_bin(SW_LOG_DEBUG, pkgBuf, iCurLen, "[%s][%d] �鱨��ͷ����,����ͷ����[%d]\n",
			__FILE__, __LINE__, iCurLen);

	return 0;
}

/****************************************************
* �� �� ��:	iDealTagOut
* ��������:	��TAG��ǩ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pXml      xml��
*          node      TAG�ڵ�
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iDealTagOut(locvars, stPkgInfo, pXml, node, pkg, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t	*pXml;
sw_xmlnode_t	*node;
char	*pkg;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	int	iTmpLen = 0;
	char	*ptr = NULL;
	char	sAmt[64];
	char	sTmp[512];
	char	sPkgNo[4];
	char	*pkgBuf = NULL;
	char	sFirstVar[128];
	char	sVarValue[512];
	double	dValue = 0.00;
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sPkgNo, 0x0, sizeof(sPkgNo));
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sFirstVar, 0x0, sizeof(sFirstVar));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	initMap(&stMap);
	initTag(&stTagNode);
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��TAG��ǩ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pkgBuf = pkg;
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "TAG") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		initTag(&stTagNode);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stTagNode.sTag, node1->value);
		
		node1 = pub_xml_locnode(pXml, "ALIAS");
		if (node1 != NULL)
		{
			strcpy(stTagNode.sAlias, node1->value);
		}
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stTagNode.cConst = '1';
		}		
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ��TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stTagNode.sType, node1->value);
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "OPTIONAL");
		if (node1 != NULL)
		{
			stTagNode.cOptional = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "ANALYSIS");
		if (node1 != NULL)
		{
			stTagNode.cAnalysis = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "CHECK");
		if (node1 != NULL)
		{
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(node1->value, sVarValue);
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
		
			node1 = pub_xml_locnode(pXml, "VALUE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			pub_log_info("[%s][%d] CHECK: CHECKVAR=[%s] CHECKVALUE=[%s]",
				__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				pnode = pnode->next;
				continue;
			}
		}
		
		if (strcmp(stPkgInfo.sPkgNo + 4, "013") == 0 && strcmp(stTagNode.sTag, "0BG") == 0)
		{
			pub_log_info("[%s][%d] AAAAAAA stPkgInfo.sType[0]=[%s]",
				__FILE__, __LINE__, stPkgInfo.sType[0]);
			if (locvars == NULL)
			{
				set_zd_data(stTagNode.sAlias, stPkgInfo.sType[0]);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, stPkgInfo.sType[0]);
			}
		}
		
		if (strcmp(stTagNode.sAlias, "#counts") == 0)
		{
			memset(sTmp, 0x0, sizeof(sTmp));
			sprintf(sTmp, "%d", stPkgInfo.iCount);
			if (locvars == NULL)
			{
				set_zd_data(stTagNode.sAlias, sTmp);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, sTmp);
			}
		}
		
		if (strcmp(stTagNode.sAlias, "#pkgtype") == 0)
		{
			ptr = strstr(stPkgInfo.sPkgNo, "_");
			if (ptr != NULL)
			{
				memset(sPkgNo, 0x0, sizeof(sPkgNo));
				memcpy(sPkgNo, ptr + 1, sizeof(sPkgNo) - 1);
			}
			else
			{
				memset(sPkgNo, 0x0, sizeof(sPkgNo));
				memcpy(sPkgNo, stPkgInfo.sPkgNo, sizeof(sPkgNo));
			}
		
			if (locvars == NULL)
			{
				set_zd_data(stTagNode.sAlias, sPkgNo);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, sPkgNo);
			}
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
		if (node1 == NULL)
		{
			pub_log_info("[%s][%d] ����û��ӳ��,ֱ��ȡֵ!",
				__FILE__, __LINE__);
			if (strlen(stTagNode.sAlias) == 0)
			{
				sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
			}
			
			if (stTagNode.cConst == '1')
			{
				/*** ����ǳ���,Ϊ���ʶ,û��ֵ,ֱ�Ӹ���ֵ ***/
				memset(stTagNode.sTagValue, 0x0, sizeof(stTagNode.sTagValue));
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
				}
				else
				{
					loc_get_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
				}
			}
			memset(sTmp, 0x0, sizeof(sTmp));
			if (stTagNode.cConst == '1')
			{
				sprintf(sTmp, "%s", stTagNode.sTag);
			}
			else
			{
				sprintf(sTmp, ":%s", stTagNode.sTag);
			}
			
			if (stTagNode.cConst != '1')
			{
				strcat(sTmp, ":");
			}

			if (stTagNode.cConst == '1')
			{
				iTmpLen = strlen(sTmp);
				memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
				iCurLen += iTmpLen;
				pnode = pnode->next;
				continue;
			}
			
			/*** �������ֵΪ��,�����TAG ***/
			if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
			{
				pub_log_error("[%s][%d] ����ָ��Ϊ��ѡ,��ֵΪ��! sTag=[%s] sAlias=[%s]",
					__FILE__, __LINE__, stTagNode.sTag, stTagNode.sAlias);
				return -1;
			}
			 
			if (strlen(stTagNode.sTagValue) == 0)
			{
				pnode = pnode->next;
				continue;
			}
			
			iTmpLen = strlen(sTmp);
			memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
			iCurLen += iTmpLen;
			
			if (stTagNode.cType == 'a')
			{
				memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
			}
			else if (stTagNode.cType == 'n')
			{		
				memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
			}
			else
			{
				pub_log_error("[%s][%d] ��������=[%c], ӦΪa��n,�Ҳ����鱨����!",
					__FILE__, __LINE__, stTagNode.cType);
				return -1;
			}
			iTmpLen = strlen(stTagNode.sTagValue);
			if (stTagNode.cType == 'n')
			{
				memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
			}
			else
			{
				memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
			}
			iCurLen += stTagNode.iLength;
			
			pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
			
			pnode = pnode->next;
			continue;
		}
		
		pub_log_info("[%s][%d] TAG=[%s]", __FILE__, __LINE__, stTagNode.sTag);
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "MAP") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			initMap(&stMap);
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "ALIAS");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stMap.sAlias, node2->value);
			pub_log_info("[%s][%d] sAlias=[%s]", __FILE__, __LINE__, stMap.sAlias);
			
			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
			}
			if (stMap.cSendFlag == '1')
			{
				node1 = node1->next;
				continue;
			}
			
			node2 = pub_xml_locnode(pXml, "CHECK");
			if (node2 != NULL)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				if (locvars == NULL)
				{
					if (node2->value[0] == '#' || node2->value[0] == '$')
					{
						get_zd_data(node2->value, sVarValue);
					}
					else if (node2->value[0] == '.')
					{
						get_zd_data(node2->value, sVarValue);
					}
				}
				else
				{
					if (node2->value[0] == '#' || node2->value[0] == '$')
					{
						loc_get_zd_data(locvars, node2->value, sVarValue);
					}
					else if (node2->value[0] == '.')
					{
						loc_get_zd_data(locvars, node2->value, sVarValue);
					}
				}
				
				pub_log_info("[%s][%d] CHECK: VARNAME=[%s] VALUE=[%s]",
					__FILE__, __LINE__, node2->value, sVarValue);
				strcpy(stMap.sCheckVar, sVarValue);
			
				node2 = pub_xml_locnode(pXml, "VALUE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
						__FILE__, __LINE__);
					return -1;
				}
				strcpy(stMap.sCheckValue, node2->value);
				pub_log_info("[%s][%d] CHECK: VARNAME=[%s] VALUE=[%s] CHECKVALUE=[%s]",
					__FILE__, __LINE__, node2->value, sVarValue, node2->value);
				if (iCheckTx(stMap.sCheckVar, stMap.sCheckValue) != 0)
				{
					pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
						__FILE__, __LINE__, stMap.sCheckVar, stMap.sCheckValue);
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iBegin = atoi(node2->value);
					
			node2 = pub_xml_locnode(pXml, "MAPBEGIN");
			if (node2 != NULL)
			{
				stMap.iMapBegin = atoi(node2->value);
			}
			else if (stTagNode.cFlag == '1')
			{
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��"
					"����MAP��ûָ��MAPBEGIN!", __FILE__, __LINE__, stTagNode.sTag);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����LENGTH��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iLength = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPFLAG");
			if (node2 != NULL)
			{
				stMap.cMapFlag = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "NODETYPE");
			if (node2 != NULL)
			{
				stMap.cNodeType = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "ANALYSIS");
			if (node2 != NULL)
			{
				stMap.cAnalysis = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "CONST");
			if (node2 != NULL)
			{
				stMap.cConst = '1';
				strcpy(stMap.sConstValue, node2->value);
			}
			
			node2 = pub_xml_locnode(pXml, "TYPE");
			if (node2 != NULL)
			{
				stMap.cType = node2->value[0];
				stMap.iLength = atoi(node2->value + 1);
			}
			
			node2 = pub_xml_locnode(pXml, "DATE");
			if (node2 != NULL)
			{
				stMap.cDate = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "AMT");
			if (node2 != NULL)
			{
				stMap.cAmt = node2->value[0];
			}
			
			if (stMap.cConst == '1')
			{
				strcpy(stMap.sVarvalue, stMap.sConstValue);
			}
			else if (stMap.cNodeType == '1')
			{
				iGetXmlAttr(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else if (stMap.cAnalysis == '1')
			{
				iGetXmlVar(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_get_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			
			if (stMap.cAmt == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				dValue = strtod(stMap.sVarvalue, NULL);
				pub_log_info("[%s][%d]AMT: dValue===[%.2f]", __FILE__, __LINE__, dValue);
				iDoubleToNum(dValue, sAmt);
				pub_log_info("[%s][%d]AMT: dValue==[%.2f] sAmt==[%s]",
					__FILE__, __LINE__, dValue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
			}
			
			/*** ȥ������ǰ���/FD1/ ***/
			if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				strcpy(sVarValue, stMap.sVarvalue + 5);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cDate == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate10To8(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ʮλ����ת8λ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cType == 'n')
			{
				memset(sTmp, 0x0, sizeof(sTmp));
				memset(sTmp, '0', stMap.iLength);
				memcpy(sTmp + stMap.iLength - strlen(stMap.sVarvalue), stMap.sVarvalue, stMap.iLength);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sTmp);
				pub_log_info("[%s][%d] sAlias=[%s] sVarvalue=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}

			if (stMap.cMapFlag != '0' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sFirstVar, 0x0, sizeof(sFirstVar));
				zip_space(stMap.sVarvalue);
				iRc = iGetFirstVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sFirstVar);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ȡ[%s]��Ӧһ����ֵ����! sVarvalue=[%s] MAPFLAG=[%c]",
						__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.cMapFlag);
					pub_log_error("[%s][%d] һ�����ı��=[%s] �������ı��=[%s] ������ֵ=[%s]",
						__FILE__, __LINE__, stPkgInfo.sPkgNo, stPkgInfo.sMsgType, stMap.sVarvalue);
					return -1;
				}
				memcpy(stMap.sVarvalue, sFirstVar, sizeof(stMap.sVarvalue) - 1);
			}
			
			if (stTagNode.cFlag != '1' && strlen(stMap.sVarvalue) > stMap.iLength)
			{
				pub_log_error("[%s][%d] ����[%s]����! stMap.sVarvalue=[%s] iLength=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.iLength);
				return -1;
			}	
			
			if (stTagNode.cFlag == '1')
			{
				memcpy(stTagNode.sTagValue + stMap.iBegin, 	
					stMap.sVarvalue + stMap.iMapBegin, 
					stMap.iLength); 
			}
			else
			{
				memcpy(stTagNode.sTagValue + stMap.iBegin,
					stMap.sVarvalue,
					strlen(stMap.sVarvalue));
			}
			node1 = node1->next;
		}
		
		/*** �������ֵΪ��,�����TAG ***/
		if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
		{
			pub_log_error("[%s][%d] ����ָ��Ϊ��ѡ,��ֵΪ��! sTag=[%s] sAlias=[%s]",
				__FILE__, __LINE__, stTagNode.sTag, stTagNode.sAlias);
			return -1;
		}
		
		if (strlen(stTagNode.sTagValue) == 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		sprintf(sTmp, ":%s:", stTagNode.sTag);
		iTmpLen = strlen(sTmp);
		memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
		iCurLen += iTmpLen;
		if (stTagNode.cType == 'a')
		{
			memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
		}
		else if (stTagNode.cType == 'n')
		{
			memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
		}
		else
		{
			pub_log_error("[%s][%d] ��������=[%s],�Ҳ����鱨����!",
				__FILE__, __LINE__, stTagNode.sType);
			return -1;
		}
		iTmpLen = strlen(stTagNode.sTagValue);
		if (stTagNode.cType == 'n')
		{
			memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
		}
		else
		{
			memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
		}
		iCurLen += stTagNode.iLength;
		pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
		
		if (locvars == NULL)
		{
			set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
		}
		else
		{
			loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
		}
		
		pnode = pnode->next;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ��TAG��ǩ֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealPkgHeadOut
* ��������:	�鱨ͷ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGDealPkgHeadOut(locvars, stPkgInfo, pkg, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
char	*pkg;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ���ͷ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/PKG%s.xml",
			getenv("HOME"), stPkgInfo.sPkgNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/PKG%s.xml",
			getenv("SWWORK"), stPkgInfo.sPkgNo);
	}
	
	pub_log_info("[%s][%d] sXmlName=[%s]", __FILE__, __LINE__, sXmlName);
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��XML��ʧ��,����XML�ļ�[%s]�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	pnode = pub_xml_locnode(pXml, ".TAGLIST.PKGTRADE.TAG");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] ����δ����TAG��ǩ!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	iRc = iDealTagOut(locvars, stPkgInfo, pXml, pnode, pkg, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ��TAG��ǩʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		return -1;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] �����ͷ֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] ���ͷ���! ���ĳ���=[%d]", 
		__FILE__, __LINE__, iCurLen);
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealTagOut
* ��������:	��ѭ��TAG��ǩ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pXml      xml��
*          node      TAG�ڵ�
*          pkg       ���Ļ�����
*          index     ѭ������
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGDealTagOut(locvars, stPkgInfo, pXml, node, pkg, index, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t	*pXml;
sw_xmlnode_t	*node;
char	*pkg;
int	index;
int	*iLen;
{
	int	i = 0;
	int	iRc = 0;
	int	iNum = 0;
	int	iCurLen = 0;
	int	iTmpLen = 0;
	int	iAppendLen = 0;
	int	iAppendFlag = 0;
	char	sAmt[64];
	char	sTag[4];
	char	sTmp[512];
	char	sVarValue[512];
	char	*pkgBuf = NULL;
	char	sFirstVar[128];
	double	dValue = 0.00;
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	ST_APPEND	stAppend;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*pSaveNode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sTag, 0x0, sizeof(sTag));
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sFirstVar, 0x0, sizeof(sFirstVar));
	/*** Ϊ�������򳤶����� stAppend ***/
	memset(&stAppend, 0x0, sizeof(stAppend));
	stAppend.iWidth = 0;
	stAppend.iOffSet = 0;
	stAppend.iLength = 0;
	initMap(&stMap);
	initTag(&stTagNode);
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] LOOP��TAG��ǩ֮ǰ iCurLen=[%d]",
		__FILE__, __LINE__, iCurLen);

	pkgBuf = pkg;
	
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "TAG") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		initTag(&stTagNode);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stTagNode.sTag, node1->value);
		
		node1 = pub_xml_locnode(pXml, "ALIAS");
		if (node1 != NULL)
		{
			strcpy(stTagNode.sAlias, node1->value);
		}
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stTagNode.cConst = '1';
			strcpy(stTagNode.sConstValue, node1->value);
		}		
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ��TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stTagNode.sType, node1->value);
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "NUM");
		if (node1 != NULL)
		{
			stTagNode.cNum = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "APPENDLEN");
		if (node1 != NULL)
		{
			iAppendFlag = 1;
			stAppend.iWidth = stTagNode.iLength;
			stAppend.iOffSet = iCurLen + 5; /*** ƫ����ֱ�Ӵӱ�ǩ��ʼ ***/
		}
		
		node1 = pub_xml_locnode(pXml, "LIST");
		if (node1 != NULL)
		{
			stTagNode.cList = node1->value[0];
		}
		
		if (stTagNode.cList == '1')
		{
			memset(sTmp, 0x0, sizeof(sTmp));
			sprintf(sTmp, ":%s:", stTagNode.sTag);
			iTmpLen = strlen(sTmp);
			memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
			iCurLen += iTmpLen;
			
			/*** �տ���/�������嵥 ***/
			pSaveNode = pXml->current;
			for (i = 0; i < iNum; i++)
			{
				node1 = pub_xml_locnode(pXml, ".LIST.TAG");
				if (node1 == NULL)
				{
					pub_log_error("[%s][%d] �����ļ���ָ�����嵥��ʶ������"
						"û�������嵥�б�[LIST.TAG]", __FILE__, __LINE__);
					return -1;
				}
				
				pub_log_info("[%s][%d] PKGNO====[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
				if (strcmp(stPkgInfo.sPkgNo, "133_006") == 0 ||
					strcmp(stPkgInfo.sPkgNo, "125_005") == 0)
				{
					index = i;
				}
				
				iRc = iPKGDealListOut(locvars, stPkgInfo, pXml, node1, pkgBuf, index, i, &iAppendLen, &iCurLen);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���տ���/�������嵥ʧ��!", __FILE__, __LINE__);
					return -1;
				}
			}
			pXml->current = pSaveNode;
			pnode = pnode->next;
			continue;
		}
		
		node1 = pub_xml_locnode(pXml, "OPTIONAL");
		if (node1 != NULL)
		{
			stTagNode.cOptional = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "ANALYSIS");
		if (node1 != NULL)
		{
			stTagNode.cAnalysis = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "CHECK");
		if (node1 != NULL)
		{
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(node1->value, sVarValue);
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
		
			node1 = pub_xml_locnode(pXml, "VALUE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				pnode = pnode->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(pXml, "CHECK1");
		if (node1 != NULL)
		{
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(node1->value, sVarValue);
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
		
			node1 = pub_xml_locnode(pXml, "VALUE1");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				pnode = pnode->next;
				continue;
			}
		}
		
		if (strcmp(stTagNode.sTag, "72C") == 0)
		{
			/*** ������ ***/
			/*** �鸽���� ***/
			memset(sTmp, 0x0, sizeof(sTmp));
			memcpy(sTmp, ":72C:", sizeof(sTmp) - 1);
			iTmpLen = strlen(sTmp);
			memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
			iCurLen += iTmpLen;
			
			pSaveNode = pXml->current;
			node1 = pub_xml_locnode(pXml, ".APPEND");
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "APPEND") != 0)
				{
					node1 = node1->next;
					continue;
				}
				
				pXml->current = node1;
				node2 = pub_xml_locnode(pXml, "TXTYPE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] ������δ����TXTYPE!", __FILE__, __LINE__);
					return -1;
				}
				if (strcmp(node2->value, stPkgInfo.sType[index]) == 0)
				{
					break;
				}
				node1 = node1->next;
			}
			if (node1 == NULL && stTagNode.cOptional == 'M')
			{
				pub_log_error("[%s][%d] û���ҵ�ƥ���ҵ����������! sTxType=[%s]", 
					__FILE__, __LINE__, stPkgInfo.sType[index]);
				return -1;
			}
			else if (node1 == NULL)
			{
				pub_log_info("[%s][%d] û�и�������Ϣ!",
					__FILE__, __LINE__);
				/*** ���û�и�����,����72C TAG��ǩ ***/
				iCurLen -= 5;
				memset(pkgBuf + iCurLen, 0x0, 5);
				pXml->current = pSaveNode;
				pnode = pnode->next;
				continue;
			}
			pXml->current = node1;
			node1 = pub_xml_locnode(pXml, "TAG");
			iRc = iPKGDealAppendTagOut(locvars, stPkgInfo, pXml, node1, pkgBuf, index, &iAppendLen, &iCurLen);
			if (iRc == -1)
			{
				pub_log_error("[%s][%d] �鸽����ʧ��!", __FILE__, __LINE__);
				return -1;
			}
			pXml->current = pSaveNode;
			pnode = pnode->next;
			continue;
		}
		
		if (strcmp(stTagNode.sAlias, "#txtype") == 0)
		{
			if (locvars == NULL)
			{
				set_zd_data("#txtype", stPkgInfo.sType[index]);
			}
			else
			{
				loc_set_zd_data(locvars, "#txtype", stPkgInfo.sType[index]);
			}
		}	
		
		node1 = pub_xml_locnode(pXml, "MAP");
		if (node1 == NULL)
		{
			pub_log_info("[%s][%d] ����û��ӳ��,ֱ��ȡֵ!",
				__FILE__, __LINE__);
			if (strlen(stTagNode.sAlias) == 0)
			{
				sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
			}
			
			if (stTagNode.cConst == '1')
			{
				/*** ����ǳ���,Ϊ���ʶ,û��ֵ,ֱ�Ӹ���ֵ ***/
				memset(stTagNode.sTagValue, 0x0, sizeof(stTagNode.sTagValue));
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
				}
				else
				{
					loc_get_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
				}
			}
				
			if (stTagNode.cNum == '1')
			{
				iNum = atoi(stTagNode.sTagValue);
				pub_log_info("[%s][%d] �տ���/��������Ŀ=[%d]", 
					__FILE__, __LINE__, iNum);
			}
			
			memset(sTmp, 0x0, sizeof(sTmp));
			if (stTagNode.cConst == '1')
			{
				sprintf(sTmp, "%s", stTagNode.sTag);
			}
			else
			{
				sprintf(sTmp, ":%s", stTagNode.sTag);
			}
			
			if (stTagNode.cConst != '1')
			{
				strcat(sTmp, ":");
			}

			if (stTagNode.cConst == '1')
			{
				iTmpLen = strlen(sTmp);
				memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
				iCurLen += iTmpLen;
				pnode = pnode->next;
				continue;
			}
			
			/*** �������ֵΪ��,�����TAG ***/
			if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
			{
				pub_log_error("[%s][%d] ����ָ��Ϊ��ѡ,��ֵΪ��! sTag=[%s] sAlias=[%s]",
					__FILE__, __LINE__, stTagNode.sTag, stTagNode.sAlias);
				return -1;
			}
			
			if (strlen(stTagNode.sTagValue) == 0)
			{
				pnode = pnode->next;
				continue;
			}
			
			iTmpLen = strlen(sTmp);
			memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
			iCurLen += iTmpLen;
			
			if (stTagNode.cType == 'a')
			{
				memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
			}
			else if (stTagNode.cType == 'n')
			{		
				memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
			}
			else
			{
				pub_log_error("[%s][%d] ��������=[%c], ӦΪa��n,�Ҳ����鱨����!",
					__FILE__, __LINE__, stTagNode.cType);
				return -1;
			}
			iTmpLen = strlen(stTagNode.sTagValue);
			if (stTagNode.cType == 'n')
			{
				memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
			}
			else
			{
				memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
			}
			iCurLen += stTagNode.iLength;
			
			pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
			
			pnode = pnode->next;
			continue;
		}
		
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "MAP") != 0)
			{
				pnode = pnode->next;
				continue;
			}
			
			initMap(&stMap);
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "ALIAS");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, index);
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iBegin = atoi(node2->value);

			node2 = pub_xml_locnode(pXml, "MAPBEGIN");
			if (node2 != NULL)
			{
				stMap.iMapBegin = atoi(node2->value);
			}
			else if (stTagNode.cFlag == '1')
			{
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��"
					"����MAP��ûָ��MAPBEGIN!", __FILE__, __LINE__, stTagNode.sTag);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����LENGTH��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iLength = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPFLAG");
			if (node2 != NULL)
			{
				stMap.cMapFlag = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
			}
			if (stMap.cSendFlag == '1')
			{
				node1 = node1->next;
				continue;
			}
				
			node2 = pub_xml_locnode(pXml, "NODETYPE");
			if (node2 != NULL)
			{
				stMap.cNodeType = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "ANALYSIS");
			if (node2 != NULL)
			{
				stMap.cAnalysis = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "CONST");
			if (node2 != NULL)
			{
				stMap.cConst = '1';
				strcpy(stMap.sConstValue, node2->value);
			}
			
			node2 = pub_xml_locnode(pXml, "TYPE");
			if (node2 != NULL)
			{
				stMap.cType = node2->value[0];
				stMap.iLength = atoi(node2->value + 1);
			}
			
			node2 = pub_xml_locnode(pXml, "DATE");
			if (node2 != NULL)
			{
				stMap.cDate = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "AMT");
			if (node2 != NULL)
			{
				stMap.cAmt = node2->value[0];
			}
			
			if (stMap.cConst == '1')
			{
				strcpy(stMap.sVarvalue, stMap.sConstValue);
			}
			else if (stMap.cNodeType == '1')
			{
				iGetXmlAttr(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else if (stMap.cAnalysis == '1')
			{
				iGetXmlVar(locvars, stMap.sAlias, stMap.sVarvalue);
				pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_get_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			
			if (stMap.cAmt == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				dValue = strtod(stMap.sVarvalue, NULL);
				pub_log_info("[%s][%d]AMT: dValue===[%.2f]", __FILE__, __LINE__, dValue);
				iDoubleToNum(dValue, sAmt);
				pub_log_info("[%s][%d]AMT: dValue==[%.2f] sAmt==[%s]",
					__FILE__, __LINE__, dValue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
			}
			
			/*** ȥ������ǰ���/FD1/ ***/
			if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				strcpy(sVarValue, stMap.sVarvalue + 5);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cDate == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate10To8(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ʮλ����ת8λ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cType == 'n')
			{
				memset(sTmp, 0x0, sizeof(sTmp));
				memset(sTmp, '0', stMap.iLength);
				memcpy(sTmp + stMap.iLength - strlen(stMap.sVarvalue), stMap.sVarvalue, stMap.iLength);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sTmp);
				pub_log_info("[%s][%d] sAlias=[%s] sVarvalue=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			
			if (stMap.cMapFlag != '0' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sFirstVar, 0x0, sizeof(sFirstVar));
				zip_space(stMap.sVarvalue);
				iRc = iGetFirstVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sFirstVar);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ȡ[%s]��Ӧһ����ֵ����! sVarvalue=[%s] MAPFLAG=[%c]",
						__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.cMapFlag);
					pub_log_error("[%s][%d] һ�����ı��=[%s] �������ı��=[%s] ������ֵ=[%s]",
						__FILE__, __LINE__, stPkgInfo.sPkgNo, stPkgInfo.sMsgType, stMap.sVarvalue);
					return -1;
				}
				memcpy(stMap.sVarvalue, sFirstVar, sizeof(stMap.sVarvalue) - 1);
			}
			
			if (stTagNode.cFlag != '1' && strlen(stMap.sVarvalue) > stMap.iLength)
			{
				pub_log_error("[%s][%d] ����[%s]����! stMap.sVarvalue=[%s] iLength=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.iLength);
				return -1;
			}	
			
			if (stTagNode.cFlag == '1')
			{
				memcpy(stTagNode.sTagValue + stMap.iBegin, 	
					stMap.sVarvalue + stMap.iMapBegin, 
					stMap.iLength); 
			}
			else
			{
				memcpy(stTagNode.sTagValue + stMap.iBegin,
					stMap.sVarvalue,
					strlen(stMap.sVarvalue));
			}
			node1 = node1->next;
		}
		
		if (strcmp(stTagNode.sAlias, "#txkind") == 0)
		{
			if (locvars == NULL)
			{
				set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
			}
		}
		
		if (stTagNode.cNum == '1')
		{
			iNum = atoi(stTagNode.sTagValue);
			pub_log_info("[%s][%d] �տ���/��������Ŀ=[%d]",
				__FILE__, __LINE__, iNum);
		}
		
		/*** �������ֵΪ��,�����TAG ***/
		if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
		{
			pub_log_error("[%s][%d] ����ָ��Ϊ��ѡ,��ֵΪ��! sTag=[%s] sAlias=[%s]",
				__FILE__, __LINE__, stTagNode.sTag, stTagNode.sAlias);
			return -1;
		}
		
		if (strlen(stTagNode.sTagValue) == 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		sprintf(sTmp, ":%s:", stTagNode.sTag);
		iTmpLen = strlen(sTmp);
		memcpy(pkgBuf + iCurLen, sTmp, iTmpLen);
		iCurLen += iTmpLen;
		if (stTagNode.cType == 'a')
		{
			memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
		}
		else if (stTagNode.cType == 'n')
		{
			memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
		}
		else
		{
			pub_log_error("[%s][%d] ��������=[%s],�Ҳ����鱨����!",
				__FILE__, __LINE__, stTagNode.sType);
			return -1;
		}
		iTmpLen = strlen(stTagNode.sTagValue);
		if (stTagNode.cType == 'n')
		{
			memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
		}
		else
		{
			memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
		}
		iCurLen += stTagNode.iLength;
		pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
				
		if (locvars == NULL)
		{
			set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
		}
		else
		{
			loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
		}
		
		pnode = pnode->next;
	}
	
	if (iAppendFlag == 1)
	{
		/*** Ϊ�����򳤶ȸ�ֵ ***/
		memset(sVarValue, 0x0, sizeof(sVarValue));
		sprintf(sVarValue, "%d", iAppendLen);
		pub_log_info("[%s][%d] �����򳤶�=[%s]", __FILE__, __LINE__, sVarValue);
		memcpy(pkgBuf + stAppend.iOffSet + stAppend.iWidth - strlen(sVarValue), 
			sVarValue, 
			strlen(sVarValue));
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ��TAG��ǩ֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealAppendTagOut
* ��������:	�鸽����TAG��ǩ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pXml      xml��
*          node      TAG�ڵ�
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGDealAppendTagOut(locvars, stPkgInfo, pXml, node, pkg, index, iAppLen, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t	*pXml;
sw_xmlnode_t	*node;
char	*pkg;
int index;
int	*iAppLen;
int	*iLen;
{
	int	i = 0;
	int	iRc = 0;
	int	iCurLen = 0;
	int	iTmpLen = 0;
	int	iAppendLen = 0;
	int	iAppendNum = 0;
	char	*ptr = NULL;
	char	*pTag = NULL;
	char	sAmt[64];
	char	sBuf[256];
	char	sTmp[512];
	char	sPath[256];
	char	sVarValue[512];
	char	*pkgBuf = NULL;
	char	sFirstVar[512];
	double	dValue = 0.00;
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*pSaveNode = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sBuf, 0x0, sizeof(sBuf));
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sPath, 0x0, sizeof(sPath));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sFirstVar, 0x0, sizeof(sFirstVar));
	initMap(&stMap);
	initTag(&stTagNode);
	
	pkgBuf = pkg;
	
	iAppendLen = *iAppLen;
	iCurLen = *iLen;
	pub_log_info("[%s][%d] �鸽����֮ǰ iAppendLen=[%d] iCurLen=[%d]",
		__FILE__, __LINE__, iAppendLen, iCurLen);
	
	pub_log_info("[%s][%d] index=====[%d]", __FILE__, __LINE__, index);
	
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "TAG") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		initTag(&stTagNode);
		pXml->current = pnode;		
		node1 = pub_xml_locnode(pXml, "ALIAS");
		if (node1 != NULL)
		{
			strcpy(stTagNode.sAlias, node1->value);
		}
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stTagNode.cConst = '1';
			strcpy(stTagNode.sConstValue, node1->value);
		}		
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ��TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stTagNode.sType, node1->value);
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "OPTIONAL");
		if (node1 != NULL)
		{
			stTagNode.cOptional = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "CHECK");
		if (node1 != NULL)
		{
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(node1->value, sVarValue);
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
		
			node1 = pub_xml_locnode(pXml, "VALUE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				pnode = pnode->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(pXml, "NUM");
		if (node1 != NULL)
		{
			stTagNode.cNum = '1';
		}
			
		node1 = pub_xml_locnode(pXml, "LIST");
		if (node1 != NULL)
		{
			pub_log_info("[%s][%d] ��ϸ����=[%d]", __FILE__, __LINE__, iAppendNum);
			pub_log_info("[%s][%d] ҵ�����ͺ�=[%s]", __FILE__, __LINE__, stPkgInfo.sType[index]);
			pSaveNode = pXml->current;
			pXml->current = pXml->root;
			node1 = pub_xml_locnode(pXml, ".APPENDLIST");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] δ����APPENDLIST��ǩ!", __FILE__, __LINE__);
				return -1;
			}
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "APPENDLIST") != 0)
				{
					node1 = node1->next;
					continue;
				}
				
				pXml->current = node1;
				node2 = pub_xml_locnode(pXml, "TXTYPE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] ������δ����TXTYPE!", __FILE__, __LINE__);
					return -1;
				}
				pub_log_info("[%s][%d] node2->value=[%s] stPkgInfo.sType[%d]=[%s]",
					__FILE__, __LINE__, node2->value, index, stPkgInfo.sType[index]);
				if (strcmp(node2->value, stPkgInfo.sType[index]) == 0)
				{
					break;
				}
				node1 = node1->next;
			}
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] û���ҵ�ƥ���ҵ����������! sTxType=[%s]", 
					__FILE__, __LINE__, stPkgInfo.sType[index]);
				return -1;
			}
			pXml->current = node1;
			node1 = pub_xml_locnode(pXml, "TAG");
			for (i = 0; i < iAppendNum; i++)
			{
				iRc = iPKGDealListOut(locvars, stPkgInfo, pXml, node1, pkgBuf, index, i, &iAppendLen, &iCurLen);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ����ϸ�嵥ʧ��!", __FILE__, __LINE__);
					return -1;
				}
			}
			pXml->current = pSaveNode;
			pnode = pnode->next;
			continue;
		}		
		
		node1 = pub_xml_locnode(pXml, "VARLEN");
		if (node1 != NULL)
		{
			memset(sPath, 0x0, sizeof(sPath));
			sprintf(sPath, node1->value, index);

			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] ������������! VAR=[%s]",
						__FILE__, __LINE__, node1->value);
					return -1;
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] ������������! VAR=[%s]",
						__FILE__, __LINE__, node1->value);
					return -1;
				}
			}
			stTagNode.iLength = atoi(sVarValue);
		}
			
		node1 = pub_xml_locnode(pXml, "MAP");
		if (node1 == NULL)
		{
			pub_log_info("[%s][%d] ����û��ӳ��,ֱ��ȡֵ!",
				__FILE__, __LINE__);
			if (strlen(stTagNode.sAlias) == 0)
			{
				sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
			}
			
			if (stTagNode.cConst == '1')
			{
				/*** ����ǳ���,Ϊ���ʶ,û��ֵ,ֱ�Ӹ���ֵ ***/
				memset(stTagNode.sTagValue, 0x0, sizeof(stTagNode.sTagValue));
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
				}
				else
				{
					loc_get_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
				}
			}
			
			if (stTagNode.cNum == '1')
			{
				iAppendNum = atoi(stTagNode.sTagValue);
				pub_log_info("[%s][%d] ��ϸ����=[%d]", __FILE__, __LINE__, iAppendNum);
			}
			
			if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
			{
				pub_log_error("[%s][%d] ����[%s]ָ��Ϊ��ѡ,����ֵΪ��!", 
					__FILE__, __LINE__, stTagNode.sAlias);
				return -1;
			}
			
			if (stTagNode.cType == 'a')
			{
				memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
			}
			else if (stTagNode.cType == 'n')
			{		
				memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
			}
			else
			{
				pub_log_error("[%s][%d] ��������=[%c], ӦΪa��n,�Ҳ����鱨����!",
					__FILE__, __LINE__, stTagNode.cType);
				return -1;
			}
			
			iTmpLen = strlen(stTagNode.sTagValue);
			if (stTagNode.cType == 'n')
			{
				memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
			}
			else
			{
				memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
			}
			iCurLen += stTagNode.iLength;
			iAppendLen += stTagNode.iLength;
			
			pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
			
			pnode = pnode->next;
			continue;
		}
		
		pTag = stTagNode.sTagValue;
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "MAP") != 0)
			{
				pnode = pnode->next;
				continue;
			}
			
			initMap(&stMap);
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "ALIAS");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, index);
			
			node2 = pub_xml_locnode(pXml, "CHECK");
			if (node2 != NULL)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				if (locvars == NULL)
				{
					if (node2->value[0] == '#' || node2->value[0] == '$')
					{
						get_zd_data(node2->value, sVarValue);
					}
					else if (node2->value[0] == '.')
					{
						get_zd_data(node2->value, sVarValue);
					}
				}
				else
				{
					if (node2->value[0] == '#' || node2->value[0] == '$')
					{
						loc_get_zd_data(locvars, node2->value, sVarValue);
					}
					else if (node2->value[0] == '.')
					{
						loc_get_zd_data(locvars, node2->value, sVarValue);
					}
				}
				strcpy(stMap.sCheckVar, sVarValue);
			
				node2 = pub_xml_locnode(pXml, "VALUE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
						__FILE__, __LINE__);
					return -1;
				}
				strcpy(stMap.sCheckValue, node2->value);
				if (iCheckTx(stMap.sCheckVar, stMap.sCheckValue) != 0)
				{
					pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
						__FILE__, __LINE__, stMap.sCheckVar, stMap.sCheckValue);
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iBegin = atoi(node2->value);

			node2 = pub_xml_locnode(pXml, "MAPBEGIN");
			if (node2 != NULL)
			{
				stMap.iMapBegin = atoi(node2->value);
			}
			else if (stTagNode.cFlag == '1')
			{
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��"
					"����MAP��ûָ��MAPBEGIN!", __FILE__, __LINE__, stTagNode.sTag);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����LENGTH��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			/*** ����䳤���� ***/
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				memset(sPath, 0x0, sizeof(sPath));
				sprintf(sPath, node2->value, index);
				if (node2->value[0] == '#' || node2->value[0] == '$')
				{
					get_zd_data(sPath, sVarValue);
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
					stMap.iLength = atoi(sVarValue);
					stMap.cVarLen = '1';
				}
				else if (node2->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
					stMap.iLength = atoi(sVarValue);
					stMap.cVarLen = '1';
				}
				else
				{
					stMap.iLength = atoi(node2->value);		
				}
				pub_log_info("[%s][%d] sAlias=[%s] LENGTH=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.iLength);
			}
			else
			{
				memset(sPath, 0x0, sizeof(sPath));
				sprintf(sPath, node2->value, index);
				if (node2->value[0] == '#' || node2->value[0] == '$')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
					stMap.iLength = atoi(sVarValue);
					stMap.cVarLen = '1';
				}
				else if (node2->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
					stMap.iLength = atoi(sVarValue);
					stMap.cVarLen = '1';
				}
				else
				{
					stMap.iLength = atoi(node2->value);		
				}
				pub_log_info("[%s][%d] sAlias=[%s] LENGTH=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.iLength);
			}
			
			node2 = pub_xml_locnode(pXml, "MAPFLAG");
			if (node2 != NULL)
			{
				stMap.cMapFlag = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
			}
			if (stMap.cSendFlag == '1')
			{
				node1 = node1->next;
				continue;
			}
				
			node2 = pub_xml_locnode(pXml, "NODETYPE");
			if (node2 != NULL)
			{
				stMap.cNodeType = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "ANALYSIS");
			if (node2 != NULL)
			{
				stMap.cAnalysis = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "CONST");
			if (node2 != NULL)
			{
				stMap.cConst = '1';
				strcpy(stMap.sConstValue, node2->value);
			}
			
			node2 = pub_xml_locnode(pXml, "TYPE");
			if (node2 != NULL)
			{
				stMap.cType = node2->value[0];
				stMap.iLength = atoi(node2->value + 1);
			}
			
			node2 = pub_xml_locnode(pXml, "DATE");
			if (node2 != NULL)
			{
				stMap.cDate = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "AMT");
			if (node2 != NULL)
			{
				stMap.cAmt = node2->value[0];
			}
			
			if (stMap.cVarLen == '1')
			{
				ptr = (char *)calloc(1, stMap.iLength + 1);
				if (ptr == NULL)
				{
					pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
						__FILE__, __LINE__, errno, strerror(errno));
					return -1;
				}
				
				if (stTagNode.cVarLen == '0')
				{
					pTag = (char *)calloc(1, stMap.iLength + 1);
					if (pTag == NULL)
					{
						pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
							__FILE__, __LINE__, errno, strerror(errno));
						return -1;
					}
					stTagNode.cVarLen = '1';
				}
			}
			else
			{
				ptr = stMap.sVarvalue;
			}

			if (stMap.cConst == '1')
			{
				strcpy(ptr, stMap.sConstValue);
			}
			else if (stMap.cNodeType == '1')
			{
				iGetXmlAttr(locvars, stMap.sAlias, ptr);
			}
			else if (stMap.cAnalysis == '1')
			{
				iGetXmlVar(locvars, stMap.sAlias, ptr);
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stMap.sAlias, ptr);
				}
				else
				{
					loc_get_zd_data(locvars, stMap.sAlias, ptr);
				}
			}
			pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, stMap.sAlias, ptr);
			
			if (stMap.cAmt == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				dValue = strtod(stMap.sVarvalue, NULL);
				pub_log_info("[%s][%d]AMT: dValue===[%.2f]", __FILE__, __LINE__, dValue);
				iDoubleToNum(dValue, sAmt);
				pub_log_info("[%s][%d]AMT: dValue==[%.2f] sAmt==[%s]",
					__FILE__, __LINE__, dValue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
			}
			
			/*** ȥ������ǰ���/FD1/ ***/
			if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				strcpy(sVarValue, stMap.sVarvalue + 5);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cDate == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate10To8(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ʮλ����ת8λ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			
			if (stMap.cMapFlag != '0' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sFirstVar, 0x0, sizeof(sFirstVar));
				zip_space(stMap.sVarvalue);
				iRc = iGetFirstVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sFirstVar);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ȡ[%s]��Ӧһ����ֵ����! sVarvalue=[%s] MAPFLAG=[%c]",
						__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.cMapFlag);
					pub_log_error("[%s][%d] һ�����ı��=[%s] �������ı��=[%s] ������ֵ=[%s]",
						__FILE__, __LINE__, stPkgInfo.sPkgNo, stPkgInfo.sMsgType, stMap.sVarvalue);
					return -1;
				}
				memcpy(stMap.sVarvalue, sFirstVar, sizeof(stMap.sVarvalue) - 1);
			}
			
			if (stMap.cType == 'n')
			{
				memset(sTmp, 0x0, sizeof(sTmp));
				memset(sTmp, '0', stMap.iLength);
				memcpy(sTmp + stMap.iLength - strlen(stMap.sVarvalue), stMap.sVarvalue, stMap.iLength);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sTmp);
				pub_log_info("[%s][%d] sAlias=[%s] sVarvalue=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			
			if (stTagNode.cFlag != '1' && strlen(stMap.sVarvalue) > stMap.iLength)
			{
				if (stMap.cVarLen == '1')
				{
					free(ptr);
					free(pTag);
				}
				pub_log_error("[%s][%d] ����[%s]����! stMap.sVarvalue=[%s] iLength=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.iLength);
				return -1;
			}	
			
			if (stTagNode.cFlag == '1')
			{
				memcpy(pTag + stMap.iBegin, 	
					ptr + stMap.iMapBegin, 
					stMap.iLength); 
			}
			else
			{
				memcpy(pTag + stMap.iBegin,
					ptr,
					strlen(ptr));
			}
			if (stMap.cVarLen == '1')
			{
				free(ptr);
			}
			node1 = node1->next;
		}
		
		if (stTagNode.cNum == '1')
		{
			iAppendNum = atoi(stTagNode.sTagValue);
			pub_log_info("[%s][%d] ��ϸ����=[%d]", __FILE__, __LINE__, iAppendNum);
		}
		
		if (strlen(pTag) == 0 && stTagNode.cOptional == 'M')
		{
			if (stTagNode.cVarLen == '1')
			{
				free(pTag);
			}
			pub_log_error("[%s][%d] ����[%s]ָ��Ϊ��ѡ,����ֵΪ��!", 
				__FILE__, __LINE__, stTagNode.sAlias);
			return -1;
		}
		
		if (stTagNode.cType == 'a')
		{
			memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
		}
		else if (stTagNode.cType == 'n')
		{
			memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
		}
		else
		{
			if (stTagNode.cVarLen == '1')
			{
				free(pTag);
			}
			pub_log_error("[%s][%d] ��������=[%s],�Ҳ����鱨����!",
				__FILE__, __LINE__, stTagNode.sType);
			return -1;
		}
		iTmpLen = strlen(pTag);
		if (stTagNode.cType == 'n')
		{
			memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, pTag, iTmpLen);
		}
		else
		{
			memcpy(pkgBuf + iCurLen, pTag, iTmpLen);
		}
		iCurLen += stTagNode.iLength;
		iAppendLen += stTagNode.iLength;
		pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
		
		if (stTagNode.cVarLen != '1')
		{
			if (locvars == NULL)
			{
				set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
			}
		}

		if (stTagNode.cVarLen == '1')
		{
			free(pTag);
		}
		pnode = pnode->next;
	}
	
	*iAppLen = iAppendLen;
	*iLen = iCurLen;
	pub_log_info("[%s][%d] �鸽����֮�� iAppendLen=[%d] iCurLen=[%d]",
		__FILE__, __LINE__, iAppendLen, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealListOut
* ��������:	���տ���/�������嵥
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pXml      xml��
*          node      TAG�ڵ�
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGDealListOut(locvars, stPkgInfo, pXml, node, pkg, indexi, indexj, iAppLen, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t	*pXml;
sw_xmlnode_t	*node;
char	*pkg;
int	indexi;
int indexj;
int	*iAppLen;
int	*iLen;
{
	int	iRc = 0;
	int	iTmpLen = 0;
	int	iCurLen = 0;
	int	iAppendLen = 0;
	char	sAmt[64];
	char	sTmp[512];
	char	sVarValue[512];
	char	*pkgBuf = NULL;
	char	sFirstVar[512];
	double	dValue = 0.00;
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sFirstVar, 0x0, sizeof(sFirstVar));
	initMap(&stMap);
	initTag(&stTagNode);
	
	iAppendLen = *iAppLen;
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ���б�֮ǰ iAppendLen=[%d] iCurLen=[%d]",
		__FILE__, __LINE__, iAppendLen, iCurLen);
	
	pub_log_info("[%s][%d] =========indexi===[%d] ===indexj==[%d]",
		__FILE__, __LINE__, indexi, indexj);
	
	pkgBuf = pkg;
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "TAG") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		initTag(&stTagNode);
		pXml->current = pnode;		
		node1 = pub_xml_locnode(pXml, "ALIAS");
		if (node1 != NULL)
		{
			strcpy(stTagNode.sAlias, node1->value);
		}
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stTagNode.cConst = '1';
			strcpy(stTagNode.sConstValue, node1->value);
		}		
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ��TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stTagNode.sType, node1->value);
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "OPTIONAL");
		if (node1 != NULL)
		{
			stTagNode.cOptional = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "CHECK");
		if (node1 != NULL)
		{
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(node1->value, sVarValue);
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, node1->value, sVarValue);
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
		
			node1 = pub_xml_locnode(pXml, "VALUE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				pnode = pnode->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
		if (node1 == NULL)
		{
			pub_log_info("[%s][%d] ����û��ӳ��,ֱ��ȡֵ!",
				__FILE__, __LINE__);
			if (strlen(stTagNode.sAlias) == 0)
			{
				sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
			}
			
			if (stTagNode.cConst == '1')
			{
				/*** ����ǳ���,Ϊ���ʶ,û��ֵ,ֱ�Ӹ���ֵ ***/
				memset(stTagNode.sTagValue, 0x0, sizeof(stTagNode.sTagValue));
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
				}
				else
				{
					loc_get_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
				}
			}
			
			if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
			{
				pub_log_error("[%s][%d] ����[%s]ָ��Ϊ��ѡ,����ֵΪ��!", 
					__FILE__, __LINE__, stTagNode.sAlias);
				return -1;
			}
			
			if (stTagNode.cType == 'a')
			{
				memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
			}
			else if (stTagNode.cType == 'n')
			{		
				memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
			}
			else
			{
				pub_log_error("[%s][%d] ��������=[%c], ӦΪa��n,�Ҳ����鱨����!",
					__FILE__, __LINE__, stTagNode.cType);
				return -1;
			}
			iTmpLen = strlen(stTagNode.sTagValue);
			if (stTagNode.cType == 'n')
			{
				memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
			}
			else
			{
				memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
			}
			iCurLen += stTagNode.iLength;
			iAppendLen += stTagNode.iLength;
			pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
			
			pnode = pnode->next;
			continue;
		}

		while (node1 != NULL)
		{
			if (strcmp(node1->name, "MAP") != 0)
			{
				pnode = pnode->next;
				continue;
			}
			
			initMap(&stMap);
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "ALIAS");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, indexi, indexj);
			pub_log_info("[%s][%d] AAAAAAAA ALIAS====[%s]", __FILE__, __LINE__, stMap.sAlias);
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iBegin = atoi(node2->value);

			node2 = pub_xml_locnode(pXml, "MAPBEGIN");
			if (node2 != NULL)
			{
				stMap.iMapBegin = atoi(node2->value);
			}
			else if (stTagNode.cFlag == '1')
			{
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��"
					"����MAP��ûָ��MAPBEGIN!", __FILE__, __LINE__, stTagNode.sTag);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ����LENGTH��ǩ!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iLength = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPFLAG");
			if (node2 != NULL)
			{
				stMap.cMapFlag = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
			}
			if (stMap.cSendFlag == '1')
			{
				node1 = node1->next;
				continue;
			}
				
			node2 = pub_xml_locnode(pXml, "NODETYPE");
			if (node2 != NULL)
			{
				stMap.cNodeType = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "ANALYSIS");
			if (node2 != NULL)
			{
				stMap.cAnalysis = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "CONST");
			if (node2 != NULL)
			{
				stMap.cConst = '1';
				strcpy(stMap.sConstValue, node2->value);
			}
			
			node2 = pub_xml_locnode(pXml, "TYPE");
			if (node2 != NULL)
			{
				stMap.cType = node2->value[0];
				stMap.iLength = atoi(node2->value + 1);
			}
			
			node2 = pub_xml_locnode(pXml, "DATE");
			if (node2 != NULL)
			{
				stMap.cDate = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "AMT");
			if (node2 != NULL)
			{
				stMap.cAmt = node2->value[0];
			}
			
			if (stMap.cConst == '1')
			{
				strcpy(stMap.sVarvalue, stMap.sConstValue);
			}
			else if (stMap.cNodeType == '1')
			{
				iGetXmlAttr(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else if (stMap.cAnalysis == '1')
			{
				iGetXmlVar(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_get_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			
			if (stMap.cAmt == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				dValue = strtod(stMap.sVarvalue, NULL);
				pub_log_info("[%s][%d]AMT: dValue===[%.2f]", __FILE__, __LINE__, dValue);
				iDoubleToNum(dValue, sAmt);
				pub_log_info("[%s][%d]AMT: dValue==[%.2f] sAmt==[%s]",
					__FILE__, __LINE__, dValue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
			}
			
			/*** ȥ������ǰ���/FD1/ ***/
			if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				strcpy(sVarValue, stMap.sVarvalue + 5);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cDate == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate10To8(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ʮλ����ת8λ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cType == 'n')
			{
				memset(sTmp, 0x0, sizeof(sTmp));
				memset(sTmp, '0', stMap.iLength);
				memcpy(sTmp + stMap.iLength - strlen(stMap.sVarvalue), stMap.sVarvalue, stMap.iLength);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sTmp);
				pub_log_info("[%s][%d] sAlias=[%s] sVarvalue=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			
			if (stMap.cMapFlag != '0' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sFirstVar, 0x0, sizeof(sFirstVar));
				zip_space(stMap.sVarvalue);
				iRc = iGetFirstVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sFirstVar);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ȡ[%s]��Ӧһ����ֵ����! sVarvalue=[%s] MAPFLAG=[%c]",
						__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.cMapFlag);
					pub_log_error("[%s][%d] һ�����ı��=[%s] �������ı��=[%s] ������ֵ=[%s]",
						__FILE__, __LINE__, stPkgInfo.sPkgNo, stPkgInfo.sMsgType, stMap.sVarvalue);
					return -1;
				}
				memcpy(stMap.sVarvalue, sFirstVar, sizeof(stMap.sVarvalue) - 1);
			}
			
			if (stTagNode.cFlag != '1' && strlen(stMap.sVarvalue) > stMap.iLength)
			{
				pub_log_error("[%s][%d] ����[%s]����! stMap.sVarvalue=[%s] iTmpLength=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.iLength);
				return -1;
			}	
			
			if (stTagNode.cFlag == '1')
			{
				memcpy(stTagNode.sTagValue + stMap.iBegin, 	
					stMap.sVarvalue + stMap.iMapBegin, 
					stMap.iLength); 
			}
			else
			{
				memcpy(stTagNode.sTagValue + stMap.iBegin,
					stMap.sVarvalue,
					strlen(stMap.sVarvalue));
			}
			node1 = node1->next;
		}

		if (strlen(stTagNode.sTagValue) == 0 && stTagNode.cOptional == 'M')
		{
			pub_log_error("[%s][%d] ����[%s]ָ��Ϊ��ѡ,����ֵΪ��!", 
				__FILE__, __LINE__, stTagNode.sAlias);
			return -1;
		}

		if (stTagNode.cType == 'a')
		{
			memset(pkgBuf + iCurLen, ' ', stTagNode.iLength);
		}
		else if (stTagNode.cType == 'n')
		{
			memset(pkgBuf + iCurLen, '0', stTagNode.iLength);
		}
		else
		{
			pub_log_error("[%s][%d] ��������=[%s],�Ҳ����鱨����!",
				__FILE__, __LINE__, stTagNode.sType);
			return -1;
		}
		iTmpLen = strlen(stTagNode.sTagValue);
		if (stTagNode.cType == 'n')
		{
			memcpy(pkgBuf + iCurLen + stTagNode.iLength - iTmpLen, stTagNode.sTagValue, iTmpLen);
		}
		else
		{
			memcpy(pkgBuf + iCurLen, stTagNode.sTagValue, iTmpLen);
		}
		iCurLen += stTagNode.iLength;
		iAppendLen += stTagNode.iLength;
		pub_log_info("[%s][%d] AAA TAG=[%s] LENGTH=[%d] iCurLen=[%d]",
				__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength, iCurLen);
		
		if (locvars == NULL)
		{
			set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
		}
		else
		{
			loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
		}
		
		pnode = pnode->next;
	}
	
	*iAppLen = iAppendLen;
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ���б�֮�� iAppendLen=[%d] iCurLen=[%d]",
		__FILE__, __LINE__, iAppendLen, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealBodyOut
* ��������:	�����
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-25
* ��    ��:
*		   locvars �����ػ�����
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGDealBodyOut(locvars, stPkgInfo, pkg, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
char	*pkg;
int	*iLen;
{
	int	i = 0;
	int	iRc = 0;
	int	iCurLen = 0;
	char	*ptr = NULL;
	char	*psBuf = NULL;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	psBuf = pkg;
	iCurLen = *iLen;
	pub_log_info("[%s][%d] �����֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/tx%s.xml",
			getenv("HOME"), stPkgInfo.sFactorNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/tx%s.xml",
			getenv("SWWORK"), stPkgInfo.sFactorNo);
	}
	
	pub_log_info("[%s][%d] sXmlName===[%s]", __FILE__, __LINE__, sXmlName);
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��XML��ʧ��,����XML�ļ�[%s]�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	pub_log_info("[%s][%d] ��XML���ɹ�!", __FILE__, __LINE__);
	
	pub_log_info("[%s][%d] COUNT===[%d]", __FILE__, __LINE__, stPkgInfo.iCount);
	
	pub_log_info("[%s][%d] sPkgNo====[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
	
	if (strcmp(stPkgInfo.sPkgNo, "133_006") == 0 || strcmp(stPkgInfo.sPkgNo, "125_005") == 0)
	{
		stPkgInfo.iCount = 1;
	}
	pub_log_info("[%s][%d] ��ϸҵ���ܱ���===[%d]", __FILE__, __LINE__, stPkgInfo.iCount);
	
	for (i = 0; i < stPkgInfo.iCount; i++)
	{
		pub_log_info("[%s][%d] i===[%d] iCurLen=[%d]", 
			__FILE__, __LINE__, i, iCurLen);
		
		/*** ��ҵ��ͷ 3 + 3 + 9 + 1***/
		memset(psBuf + iCurLen, ' ', 16);
		memcpy(psBuf + iCurLen, "{2:", 3);
		iCurLen += 3;
		ptr = strchr(stPkgInfo.sFactorNo, '_');
		if (ptr != NULL)
		{
			memcpy(psBuf + iCurLen, ptr + 1, 3);
		}
		else
		{	
			memcpy(psBuf + iCurLen, stPkgInfo.sFactorNo, 3);
		}
		iCurLen += 3;
		memcpy(psBuf + iCurLen, "         ", 9);
		iCurLen += 9;
		memcpy(psBuf + iCurLen, "}", 1);
		iCurLen += 1;
		
		/*** ��ҵ���� ***/
		pXml->current = pXml->root;
		pnode = pub_xml_locnode(pXml, ".TAGLIST.TAG");
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] δ����TAG��ǩ!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		iRc = iPKGDealTagOut(locvars, stPkgInfo, pXml, pnode, pkg, i, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] ��TAG��ǩʧ��!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
	}
	pub_xml_deltree(pXml);
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] �������֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iMapHeaderOut
* ��������:	ӳ�䱨��ͷ����Ҫ�ı���
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pXml      XML��
*          pnode     ITEM�ڵ�
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iMapHeaderOut(locvars, stPkgInfo, pXml, pnode)
sw_loc_vars_t *locvars;
ST_PKGINFO stPkgInfo;
sw_xmltree_t *pXml;
sw_xmlnode_t *pnode;
{
	int	iRc = 0;
	char	sAmt[64];
	char	sFirstVar[512];
	char	sVarValue[512];
	ST_MAP	stMap;
	ST_ITEM	stItem;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sFirstVar, 0x0, sizeof(sFirstVar));
	initMap(&stMap);
	iInitItem(&stItem);
	
	pub_log_info("[%s][%d] ��ʼӳ�䱨��ͷ����!", __FILE__, __LINE__);
	
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		iInitItem(&stItem);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ���ñ�ǩNAME!", __FILE__, __LINE__);
			return -1;
		}
		memcpy(stItem.cVarname, node1->value, sizeof(stItem.cVarname) - 1);

		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ���ñ�ǩTYPE!", __FILE__, __LINE__);
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stItem.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ���ñ�ǩMAP, ����Ҫ����ӳ��!", 
				__FILE__, __LINE__);
			pnode = pnode->next;
			continue;
		}
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "MAP") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			initMap(&stMap);
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "ALIAS");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ���ñ�ǩALIAS!", 
					__FILE__, __LINE__);
				return -1;
			}
			memcpy(stMap.sAlias, node2->value, sizeof(stMap.sAlias) - 1);
			
			pub_log_info("[%s][%d] ALIAS===[%s]", __FILE__, __LINE__, stMap.sAlias); 
			
			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
			}
			if (stMap.cSendFlag == '1')
			{
				node1 = node1->next;
				continue;
			}
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δ���ñ�ǩBEGIN!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iBegin = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPBEGIN");
			if (node2 != NULL)
			{
				stMap.iMapBegin = atoi(node2->value);
			}

			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] δ���ñ�ǩLENGTH!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iLength = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPFLAG");
			if (node2 != NULL)
			{
				stMap.cMapFlag = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "NODETYPE");
			if (node2 != NULL)
			{
				stMap.cNodeType = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "ANALYSIS");
			if (node2 != NULL)
			{	
				stMap.cAnalysis = node2->value[0];
			}
			
			node2 = pub_xml_locnode(pXml, "DATE");
			if (node2 != NULL)
			{
				stMap.cDate = node2->value[0];
			}
			
			pub_log_info("[%s][%d] ALIAS===[%s]", __FILE__, __LINE__, stMap.sAlias);
			if (stMap.cNodeType == '1')
			{
				iGetXmlAttr(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else if (stMap.cAnalysis == '1')
			{
				iGetXmlVar(locvars, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				if (locvars == NULL)
				{
					get_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_get_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			
			/*** ȥ������ǰ���/FD1/ ***/
			if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				strcpy(sVarValue, stMap.sVarvalue + 5);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			if (stMap.cMapFlag != '0')
			{
				memset(sFirstVar, 0x0, sizeof(sFirstVar));
				zip_space(stMap.sVarvalue);
				iRc = iGetFirstVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sFirstVar);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ȡ[%s]��Ӧһ����ֵ����! sVarvalue=[%s] MAPFLAG=[%c]",
						__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue, stMap.cMapFlag);
					pub_log_error("[%s][%d] һ�����ı��=[%s] �������ı��=[%s] ������ֵ=[%s]",
						__FILE__, __LINE__, stPkgInfo.sPkgNo, stPkgInfo.sMsgType, stMap.sVarvalue);
					return -1;
				}
				memcpy(stMap.sVarvalue, sFirstVar, sizeof(stMap.sVarvalue) - 1);
			}
			
			if (stMap.cDate == '1' && strlen(stMap.sVarvalue) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate10To8(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ʮλ����ת8λ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}

			if (stItem.cFlag == '1')
			{
				memcpy(stItem.cVarvalue + stMap.iBegin, 
					stMap.sVarvalue + stMap.iMapBegin, 
					stItem.iLength); 
			}
			else
			{
				memcpy(stItem.cVarvalue + stMap.iBegin, 
					stMap.sVarvalue, 
					stMap.iLength);
			} 
			node1 = node1->next;
		}
		pub_log_info("[%s][%d]ITEM: ALIAS===[%s] VALUE=[%s]", __FILE__, __LINE__, stMap.sAlias, stItem.cVarvalue);

		if (strlen(stItem.cVarvalue) == 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		if (locvars == NULL)
		{
			set_zd_data(stItem.cVarname, stItem.cVarvalue);
		}
		else
		{
			loc_set_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
		}
		pub_log_info("[%s][%d] HEADERMAP: NAME==[%s] VALUE=[%s]",
			__FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
			
		pnode = pnode->next;
	}

	return 0;
}

/****************************************************
* �� �� ��:	iPKGInit
* ��������:	����ͷӳ���ʼ��(������ͷ��Ҫ�ı���ȡ��)
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGInit(sw_loc_vars_t *locvars, ST_PKGINFO stPkgInfo)
{
	int	iRc = 0;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;

	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/PKG%s.xml",
			getenv("HOME"), stPkgInfo.sPkgNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/PKG%s.xml",
			getenv("SWWORK"), stPkgInfo.sPkgNo);
	}
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��XML��ʧ��,����XML�ļ�[%s]�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	/*** ����ȡ������ͷ�еı��� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.BEFORMAP.ITEM");
	iRc = iMapHeaderOut(locvars, stPkgInfo, pXml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ȡ����ͷ����ʧ��!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	/*** ����ȡ������ͷ�еı��� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.ENDMAP.ITEM");
	iRc = iMapHeaderOut(locvars, stPkgInfo, pXml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ȡ����ͷ����ʧ��!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	pub_xml_deltree(pXml);
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealOut
* ��������:	������XML���İ�һ��PKG���ĸ�ʽ�������
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pkgnode   ITEM�ڵ�
*          pXml      XML��
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iPKGDealOut(locvars, stPkgInfo, pkgnode, pXml, pkg, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO stPkgInfo;
sw_xmlnode_t *pkgnode;
sw_xmltree_t *pXml;
char *pkg;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	*ptr = NULL;
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ���֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	iRc = iPKGInit(locvars, stPkgInfo);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] �����ʼ��ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	
	/*** ���Ƚ�����ͷ��Ҫӳ��ı�������ӳ�� ***/
	iRc = iBasHeadMapOut(locvars, stPkgInfo);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] �����ʼ������!", __FILE__, __LINE__);
		return -1;
	}
	/*** �鱨��ͷ ***/
	iRc = iDealBasHeadOut(locvars, pkgnode, pXml, pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] �鱨��ͷʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] �鱨��ͷ���!", __FILE__, __LINE__);
	pub_log_bin(SW_LOG_DEBUG, pkg, iCurLen, "[%s][%d] �鱨��ͷ����,���ĳ���[%d]\n",
		__FILE__, __LINE__, iCurLen);
		
	/*** ���ͷ ***/
	iRc = iPKGDealPkgHeadOut(locvars, stPkgInfo, pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] ���ͷʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_bin(SW_LOG_DEBUG, pkg, iCurLen, "[%s][%d] ���ͷ����,���ĳ���[%d]\n",
		__FILE__, __LINE__, iCurLen);
	
	ptr = pkg + iCurLen;
	/*** ����� ***/
	iRc = iPKGDealBodyOut(locvars, stPkgInfo, pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] ��������ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	
	pub_log_info("[%s][%d] ����=====[%s]", __FILE__, __LINE__, ptr);
	if (locvars == NULL)
	{
		iSetLargeVar("#detail", ptr, strlen(ptr));
	}
	else
	{
		iLocSetLargeVar(locvars, "#detail", ptr, strlen(ptr));
	}

	/*** �鱨��β ***/
	iRc = iDealTailOut(pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] �鱨��βʧ��!", __FILE__, __LINE__);
		return-1;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ������� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] ������! Successfully!", __FILE__, __LINE__);	
	
	if (locvars == NULL)
	{
		set_zd_data("#cmttype", "1");
	}
	else
	{
		loc_set_zd_data(locvars, "#cmttype", "1");
	}
	return 0;
}

/****************************************************
* �� �� ��:	iCMTDealTradeHeadOut
* ��������:	��ҵ��ͷ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iCMTDealTradeHeadOut(sw_loc_vars_t *locvars, char *pkg, int *iLen)
{
	int	iRc = 0;
	int	iTmpLen = 0;
	int	iCurLen = 0;
	char	*pkgBuf = NULL;
	char	sXmlName[512];
	ST_ITEM	stItem;
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	memset(sXmlName, 0x0, sizeof(sXmlName));
	iInitItem(&stItem);
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��ҵ��ͷ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pkgBuf = pkg;
	
	if (getenv("SWWORK") != NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/cmtheader.xml", 
			getenv("SWWORK"));
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/cmtheader.xml", 
			getenv("HOME"));
	}
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ����ʧ��,����XML�ļ�[%s]�Ƿ����"
			"���߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	pnode = pub_xml_locnode(pXml, "CMTHEADER.ITEM");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] û���ҵ���һ��ITEM! ����xml�ļ�[%s]!",
			__FILE__, __LINE__, sXmlName);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		iInitItem(&stItem);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			pXml = NULL;	
			return -1;
		}
		strcpy(stItem.cVarname, node1->value);
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stItem.cConst = '1';
			strcpy(stItem.cConstvalue, node1->value);
		}
	
		if (stItem.cConst == '1')
		{	
			strcpy(stItem.cVarvalue, stItem.cConstvalue);
		}
		else
		{
			if (locvars == NULL)
			{
				get_zd_data(stItem.cVarname, stItem.cVarvalue);
			}
			else
			{
				loc_get_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
			}
		}
		pub_log_info("[%s][%d] ����[%s]=[%s]", 
			__FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
			
		if (stItem.cType == 'a')
		{
			memset(pkgBuf + iCurLen, ' ', stItem.iLength);
		}
		else if (stItem.cType == 'n')
		{
			memset(pkgBuf + iCurLen, '0', stItem.iLength);
		}
		else
		{
			pub_log_error("[%s][%d] ��������=[%c]ӦΪa��n!û�ҵ��������!",
				__FILE__, __LINE__, stItem.cType);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		} 

		iTmpLen = strlen(stItem.cVarvalue);
		if (stItem.cType == 'n')
		{
			memcpy(pkgBuf + iCurLen + stItem.iLength - iTmpLen, stItem.cVarvalue, iTmpLen);
		}
		else
		{	
			memcpy(pkgBuf + iCurLen, stItem.cVarvalue, iTmpLen);
		}
		iCurLen += stItem.iLength;
		pnode = pnode->next;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ��ҵ��ͷ���! ���ĳ���=[%d]",
		__FILE__, __LINE__, iCurLen);
	
	pub_log_bin(SW_LOG_DEBUG, pkgBuf, iCurLen, "[%s][%d] ��ҵ��ͷ����,���ĳ���[%d]\n",
			__FILE__, __LINE__, iCurLen);
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return 0;
}

/****************************************************
* �� �� ��:	iCMTDealTradeBodyOut
* ��������:	��������
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iCMTDealTradeBodyOut(locvars, stPkgInfo, pkg, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
char	*pkg;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sFileName[512];
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sFileName, 0x0, sizeof(sFileName));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��������֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (strlen(stPkgInfo.sPkgNo) == 0)
	{
		pub_log_error("[%s][%d] δָ��PKGNO!",
			__FILE__, __LINE__);
		return -1;
	}
	if (stPkgInfo.cPkgType == '1')
	{
		/*** ���� ***/
		memcpy(sFileName, "HVPS_CMT", sizeof(sFileName) - 1);
	}
	else if (stPkgInfo.cPkgType == '2')
	{
		/*** С��CMT���� ***/
		memcpy(sFileName, "BEPS_CMT", sizeof(sFileName) - 1);
	}

	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/%s%s.xml",
			getenv("HOME"), sFileName, stPkgInfo.sPkgNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/%s%s.xml",
			getenv("SWWORK"), sFileName, stPkgInfo.sPkgNo);
	}
	pub_log_info("[%s][%d] XMLNAME==[%s]", __FILE__, __LINE__, sXmlName);
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��xml��ʧ��! ����xml�ļ�[%s]"
			"�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}

	/*** �������� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.CMTTRADE.TAG");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] ����δ����TAG��ǩ!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	iRc = iDealTagOut(locvars, stPkgInfo, pXml, pnode, pkg, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ��TAG��ǩʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		return -1;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] �����������! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return 0;
}

/****************************************************
* �� �� ��:	iMapInit
* ��������:	����ͷӳ���ʼ��(������ͷ��Ҫ�ı���ȡ��)
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iMapInit(sw_loc_vars_t *locvars, ST_PKGINFO stPkgInfo)
{
	int	iRc = 0;
	char	sXmlName[512];
	char	sFileName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	memset(sFileName, 0x0, sizeof(sFileName));

	if (strlen(stPkgInfo.sPkgNo) == 0)
	{
		pub_log_error("[%s][%d] δָ��PKGNO!",
			__FILE__, __LINE__);
		return -1;
	}
	if (stPkgInfo.cPkgType == '1')
	{
		/*** ���� ***/
		memcpy(sFileName, "HVPS_CMT", sizeof(sFileName) - 1);
	}
	else if (stPkgInfo.cPkgType == '2')
	{
		/*** С��CMT���� ***/
		memcpy(sFileName, "BEPS_CMT", sizeof(sFileName) - 1);
	}

	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/%s%s.xml",
			getenv("HOME"), sFileName, stPkgInfo.sPkgNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/%s%s.xml",
			getenv("SWWORK"), sFileName, stPkgInfo.sPkgNo);
	}
	pub_log_info("[%s][%d] XMLNAME=[%s]", __FILE__, __LINE__, sXmlName);
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��xml��ʧ��! ����xml�ļ�[%s]"
			"�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	/*** ����ȡ������ͷ�еı��� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.BEFORMAP.ITEM");
	iRc = iMapHeaderOut(locvars, stPkgInfo, pXml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ȡ����ͷ����ʧ��!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	/*** ����ȡ������ͷ�еı��� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.ENDMAP.ITEM");
	iRc = iMapHeaderOut(locvars, stPkgInfo, pXml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ȡ����ͷ����ʧ��!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return 0;
}

/****************************************************
* �� �� ��:	iCMTDealOut
* ��������:	������XML���İ�һ��CMT���ĸ�ʽ�������
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-18
* ��    ��:
*		   locvars �����ػ�����
*          pkgnode   ITEM�ڵ�
*          pXml      XML��
*          pkg       ���Ļ�����
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iCMTDealOut(locvars, stPkgInfo, pkgnode, pXml, pkg, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO stPkgInfo;
sw_xmlnode_t *pkgnode;
sw_xmltree_t *pXml;
char *pkg;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sCmtNo[4];
	char	sMacNo[512];
	char	sAppTradeCode[9];
	
	memset(sCmtNo, 0x0, sizeof(sCmtNo));
	memset(sMacNo, 0x0, sizeof(sMacNo));
	memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode) - 1);
	
	pub_log_info("[%s][%d] ���֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	memcpy(sMacNo, "253 301 302 303 309 310 313 314 721 724 725 910", sizeof(sMacNo) - 1);
	pub_log_info("[%s][%d] sMacNo===[%s]", __FILE__, __LINE__, sMacNo);

	iRc = iMapInit(locvars, stPkgInfo);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] �����ʼ��ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	
	/*** ���Ƚ�����ͷ��Ҫӳ��ı�������ӳ�� ***/
	iRc = iBasHeadMapOut(locvars, stPkgInfo);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] �����ʼ������!", __FILE__, __LINE__);
		return -1;
	}
	
	/*** �鱨��ͷ ***/
	iRc = iDealBasHeadOut(locvars, pkgnode, pXml, pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] �鱨��ͷʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] �鱨��ͷ���! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (stPkgInfo.cPkgType == '1')
	{
		memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
		if (locvars == NULL)
		{
			get_zd_data("$appTradeCode", sAppTradeCode);
		}
		else
		{
			loc_get_zd_data(locvars, "$appTradeCode", sAppTradeCode);
		}
		pub_log_info("[%s][%d] sAppTradeCode===[%s]", __FILE__, __LINE__, sAppTradeCode);
	
		memset(sCmtNo, 0x0, sizeof(sCmtNo));
		memcpy(sCmtNo, sAppTradeCode + 1, 3);
		pub_log_info("[%s][%d] sCmtNo====[%s]", __FILE__, __LINE__, sCmtNo);
		
		if (iCheckTx(sCmtNo, sMacNo) != 0)
		{
			/*** ���(CMT)���� ��ҵ��ͷ ***/
			iRc = iCMTDealTradeHeadOut(locvars, pkg, &iCurLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] ��CMTҵ��ͷʧ��!", __FILE__, __LINE__);
				return -1;
			}
			pub_log_info("[%s][%d] ����[%s]��ҵ��ͷ���! iCurLen=[%d]",
				__FILE__, __LINE__, sCmtNo, iCurLen);

			/*** #cmttype="1" ��ʾ�����ҵ��ͷ��Ҫ��Ѻ��CMT���� ***/
			if (locvars == NULL)
			{
				set_zd_data("#cmttype", "1");
			}
			else
			{
				loc_set_zd_data(locvars, "#cmttype", "1");
			}
		}
	}
	else if (stPkgInfo.cPkgType == '2')
	{
		/*** #cmttype="2" ��ʾС��CMT����, ����Ѻ ***/
		if (locvars == NULL)
		{
			set_zd_data("#cmttype", "2");
		}
		else
		{
			loc_set_zd_data(locvars, "#cmttype", "2");
		}
	}

	/*** �������� ***/
	iRc = iCMTDealTradeBodyOut(locvars, stPkgInfo, pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] ��������ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] �����������! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** �鱨��β ***/
	iRc = iDealTailOut(pkg, &iCurLen);
	if (iRc < 0)
	{
		pub_log_error("[%s][%d] �鱨��βʧ��!", __FILE__, __LINE__);
		return -1;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ������! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] ������! Successfully!", __FILE__, __LINE__);
	
	return 0;
}

int nSetpkglen(char *cLen, int *begin, int *end)
{
	int	i = 1;
	int	n = 0;
	int	ipkglen = 0;
	char	cbegin[5];
	char	cend[5];
	char	cpkglen[8];
	
	memset(cbegin, 0x0, sizeof(cbegin));
	memset(cend, 0x0, sizeof(cend));
	memset(cpkglen, 0x0, sizeof(cpkglen));
	
	if (cLen[0] != '[')
	{
		ipkglen = atoi(cLen);
	}
	else
	{
		while (cLen[i] != '.')
		{
			cbegin[i-1] = cLen[i];
			i++;
		}
		i += 2;
		while (cLen[i] != ']')
		{
			cend[n]=cLen[i];
			i++;
			n++;
		}
		*begin = atoi(cbegin);
		*end = atoi(cend);
		return 0 ;
	}
	
	return -1;
}

int addpackvalue(char *pkg, int ibegin, short length, char *value, char type, int *iLen)
{
	int	iCurLen = 0;
	char	*psBuf = pkg;
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ADDPACK: iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (type != 'B' && type != 'b')
	{
		if (ibegin > 0)
		{
			iCurLen = ibegin;
			if (strlen(value) > 0)
			{
				memcpy(psBuf + iCurLen, value, length);
			}
			else
			{
				memset(psBuf + iCurLen, 0, length);
			}
			iCurLen += length;
		}
		else
		{
			if (strlen(value) > 0)
			{
				memcpy(psBuf + iCurLen, value, length);
				pub_log_debug("[%s][%d], addpack[%.*s], iLockpkg=[%d] [%d]",
					__FILE__, __LINE__, length, value, iCurLen, iCurLen + length);
			}
			else
			{
				memset(psBuf + iCurLen, 0, length);
			}
			iCurLen += length;
		}
	}
	else 
	{
		if (ibegin > 0)
		{
			iCurLen = ibegin;
		}
		pub_log_debug("value[%x%x%x%x%x%x%x%x%x%x%x]", 
			value[0], value[1], value[2], value[3], value[4], value[5],
			value[6], value[7], value[8], value[9], value[10]);
		memcpy(psBuf + iCurLen, value, length);
		iCurLen += length;
		pub_log_debug("[%s][%d], addpack[%.*s], iLockpkg=[%d] [%d]",
			__FILE__, __LINE__, length, value, iCurLen - length, iCurLen);
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

int iDealHead(sw_loc_vars_t *locvars, ST_PKGINFO stPkgInfo)
{
	char	*ptr = NULL;
	char	sCmtNo[4];
	char	sPayPri[2];
	
	memset(sCmtNo, 0x0, sizeof(sCmtNo));
	memset(sPayPri, 0x0, sizeof(sPayPri));
	
	/*** ��ҵ��ͷ���д��� ***/
	/*** ���CMT���� ***/
	pub_log_info("[%s][%d] stPkgInfo.sPkgNo=[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
	if (stPkgInfo.cPkgType == '1')
	{
		ptr = strstr(stPkgInfo.sPkgNo, "_");
		if (ptr == NULL)
		{
			memcpy(sCmtNo, stPkgInfo.sPkgNo, sizeof(sCmtNo) - 1);
		}
		else
		{
			memcpy(sCmtNo, ptr + 1, sizeof(sCmtNo) - 1);
		}
		pub_log_info("[%s][%d] sCmtNo=[%s]", __FILE__, __LINE__, sCmtNo);
		if (locvars == NULL)
		{
			set_zd_data("#1CMTCode", sCmtNo);
		}
		else
		{
			loc_set_zd_data(locvars, "#1CMTCode", sCmtNo);
		}
	}
	
	/*** ֧�����ȼ� ***/
	if (locvars == NULL)
	{
		get_zd_data("#MesgPriority", sPayPri);
	}
	else
	{
		loc_get_zd_data(locvars, "#MesgPriority", sPayPri);
	}
	pub_log_info("[%s][%d] ����֧�����ȼ�=[%s]", __FILE__, __LINE__, sPayPri);
	if (sPayPri[0] == '1')
	{
		sPayPri[0] = '2';
	}
	else if (sPayPri[0] == '2')
	{
		sPayPri[0] = '1';
	}
	else if (sPayPri[0] == '3')
	{
		sPayPri[0] = '0';
	}
	else
	{
		sPayPri[0] = '0';
	}
	pub_log_info("[%s][%d] һ��֧�����ȼ�=[%s]", __FILE__, __LINE__, sPayPri);
	if (stPkgInfo.cPkgType == '1')
	{
		if (locvars == NULL)
		{
			set_zd_data("#1MesgPriority", sPayPri);
		}
		else
		{
			loc_set_zd_data(locvars, "#1MesgPriority", sPayPri);
		}
	}
	
	if (locvars == NULL)
	{
		set_zd_data("#MesgPRI", sPayPri);
	}
	else
	{
		loc_set_zd_data(locvars, "#MesgPRI", sPayPri);
	}
	
	return 0;
}

/****************************************************
* �� �� ��:	iSwitchToPkgPkgCmt
* ��������:	������XML���İ�һ��PKG���ĸ�ʽ�������
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-25
* ��    ��:
*		   locvars �����ػ�����
*          pkg       ���Ļ�����
*          xmlname   ���XML�����ļ���
* ��    ��:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iSwitchToPkgPkgCmt(sw_loc_vars_t *locvars, char *pkg, char *xmlname)
{
	int	iRc = 0;
	int	iLen = 0;
	int	iBegin = 0;
	int	iEnd = 0;
	int	iCurLen = 0;
	int	iTmpLen = 0;	
	int	iMySelf = 0;
	char	sTmp[512];
	char	*psBuf = NULL;
	char	cVarvalue[1024];
	char	sPkgType[32];
	char	sXmlName[512];
	char	sFirstName[32];
	char    sPkgLibName[64];
	char    sPkgFunName[64];
	char    sRespcdName[32];
	char	sSysRespcd[5];
	char	sPkgRespcd[32];
	ST_ITEM	stItem;
	ST_FIRST	stFirstwork;
	ST_PKGINFO stPkgInfo;
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*pSaveNode = NULL;
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sPkgType, 0x0, sizeof(sPkgType));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	memset(sFirstName, 0x0, sizeof(sFirstName));
	memset(sPkgLibName, 0x0, sizeof(sPkgLibName));
	memset(sPkgFunName, 0x0, sizeof(sPkgFunName));
	memset(sRespcdName, 0x0, sizeof(sRespcdName));
	memset(sSysRespcd, 0x0, sizeof(sSysRespcd));
	memset(sPkgRespcd, 0x0, sizeof(sPkgRespcd));
	memset(&stPkgInfo, 0x0, sizeof(stPkgInfo));
	memset(&stFirstwork, 0x0, sizeof(stFirstwork));
	iInitItem(&stItem);
	
	if (locvars == NULL)
	{
		set_zd_data("$pkgtype", "1");
	}
	else
	{
		loc_set_zd_data(locvars, "$pkgtype", "1");
	}
	
	psBuf = pkg;
	iRc = iInitPkgOut(locvars, &stPkgInfo);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] �鱨��ʼ��ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	
	iRc = iDealHead(locvars, stPkgInfo);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ���������ȼ�ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	
	if (getenv("SWWORK") != NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/%s",
			getenv("SWWORK"), xmlname);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/%s",
			getenv("HOME"), xmlname);
	}

	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ����ʧ��,����xml�ļ�[%s]�Ƿ����"
			"���߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}

	pnode = pub_xml_locnode(pXml, ".CBM.PKGTYPE");
	if (pnode == NULL || pnode->value == NULL)
	{
		pub_log_error("[%s][%d] δ�ҵ�.CBM.PKGTYPE��ǩ!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	memcpy(sPkgType, pnode->value, sizeof(sPkgType) - 1);
	pub_log_info("[%s][%d] sPkgType=[%s]", __FILE__, __LINE__, sPkgType);
	
	pnode = pub_xml_locnode(pXml, ".CBM.INTEGRATE");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ�ҵ�[.CBM.INTEGRATE]��ǩ!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	pnode = pstGetNodeByTarget(pXml, "INTEGRATE", "PKGTYPE", "CMT");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ�ҵ�[CMT]��ǩ!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pSaveNode = pnode;
	pXml->current = pnode;
	pnode = pub_xml_locnode(pXml, "FIRSTWORK.ITEM.NAME");
	if (pnode != NULL)
	{
		strcpy(sFirstName, pnode->value);
		strcpy(stFirstwork.cVarname, pnode->value);
		set_zd_data("#firstvarname", stFirstwork.cVarname);
	}
	
	/*** ��ʼ����PACKAGE ***/
	pXml->current = pSaveNode;
	pnode = pub_xml_locnode(pXml, "PACKAGE");
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "PACKAGE") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "RESPCD");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] ����PAKAGEδ����RESPCDѡ��!",
				__FILE__, __LINE__);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
		memcpy(sRespcdName, node1->value, sizeof(sRespcdName) - 1);
			
		/*** ���INCLUDE����, ������ֻ����һ�� ***/
		node1 = pub_xml_locnode(pXml, "INCLUDE");
		if (node1 != NULL)
		{
			if (iLocChkinclude(locvars, sFirstName, node1->value) != 0)
			{
				pnode = pnode->next;
				continue;
			}
		}
		/*** ���CHECK����,�����������ж�� ***/
		node1 = pub_xml_locnode(pXml, "CHECK");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "CHECK") != 0)
			{
				node1 = node1->next;
				continue;
			}
			if (iLocCheck(locvars, node1->value) != 0)
			{
				break;
			}
			node1 = node1->next;
		}
		if (node1 != NULL)
		{
			/*** CHECK����������,������һ��PACKAGE ***/
			pnode = pnode->next;
			continue;
		}
		
		/*** ���:���$respcd������Ӧ����,���ñ���Ӧ����Ϊ����Ӧ���� ***/
		if(locvars == NULL)
		{
			get_zd_data("$respcd", sSysRespcd);
			get_zd_data(sRespcdName, sPkgRespcd);
		}
		else
		{
			loc_get_zd_data(locvars, "$respcd", sSysRespcd);
			loc_get_zd_data(locvars, sRespcdName, sPkgRespcd);
		}
		if (memcmp(sSysRespcd, RESPCD_REJECT1, 4) == 0)
		{
			iLen = strlen(sPkgRespcd);
			if (iLen <= 0)
			{
				pub_log_error("���ʱӦ���벻����,������!",
					__FILE__, __LINE__);
			}
			else
			{
				memset(sTmp, 0x0, sizeof(sTmp));
				memcpy(sTmp, sSysRespcd, iLen);
				if (locvars == NULL)
				{
					set_zd_data(sRespcdName, sTmp);
				}
				else
				{
					loc_set_zd_data(locvars, sRespcdName, sTmp);
				}
			}
		}
		
		/*** ���������INCLUDE��CHECK���������������Ĭ�ϰ� ***/
		pnode = pub_xml_locnode(pXml, "ITEM");
		pXml->current = pnode;
		if (stPkgInfo.cPkgType == '1' || stPkgInfo.cPkgType == '2')
		{
			/*** ���(CMT)���Ļ���С��CMT���� ***/
			iRc = iCMTDealOut(locvars, stPkgInfo, pnode, pXml, psBuf, &iCurLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] �鱨��ʧ��!", __FILE__, __LINE__);
				pub_log_bin(SW_LOG_DEBUG, psBuf, iCurLen, "[%s][%d] �������,���ĳ���[%d]\n",
					__FILE__, __LINE__, iCurLen);
				pub_xml_deltree(pXml);
				pXml = NULL;
				return -1;
			}
		}
		else
		{
			/*** С��PKG���� ***/
			iRc = iPKGDealOut(locvars, stPkgInfo, pnode, pXml, psBuf, &iCurLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] �鱨��ʧ��!", __FILE__, __LINE__);
				pub_log_bin(SW_LOG_DEBUG, psBuf, iCurLen, "[%s][%d] �������,���ĳ���[%d]\n",
					__FILE__, __LINE__, iCurLen);
				pub_xml_deltree(pXml);
				pXml = NULL;
				return -1;
			}
		}
		iRc = pub_str_zip0a09(psBuf + iCurLen, 0);
		iCurLen += iRc;
		
		/*** ����LASTWORK ***/
		pXml->current = pSaveNode;
		pnode = pub_xml_locnode(pXml, "LASTWORK.ITEM");
		while (pnode != NULL)
		{
			if (strcmp(pnode->name, "ITEM") != 0)
			{
				pnode = pnode->next;
				continue;
			}
			
			iInitItem(&stItem);
			pXml->current = pnode;
			node1 = pub_xml_locnode(pXml, "NAME");
			if (node1 != NULL)
			{
				strcpy(stItem.cVarname, node1->value);
			}

			node1 = pub_xml_locnode(pXml, "LENGTH");
			if (node1 != NULL) 
			{
				pub_log_info("[%s][%d] LENGTH=[%s]", __FILE__, __LINE__, node1->value);
				nSetpkglen(node1->value, &iBegin, &iEnd);
				pub_log_debug("[%s][%d] begin=[%d] end[=%d]", 
					__FILE__, __LINE__, iBegin, iEnd);
			}

			node1 = pub_xml_locnode(pXml, "MYSELF");
			if (node1 != NULL) 
			{
				if (strcmp(node1->value, "TRUE") == 0) 
				{
					iTmpLen = iCurLen;
					memset(cVarvalue, 0, sizeof(cVarvalue));
					sprintf(cVarvalue, "%0*d", iEnd - iBegin, iCurLen);
					if (iBegin == 0)
					{
						iCurLen = 0;
					}
					addpackvalue(psBuf, iBegin, iEnd - iBegin, cVarvalue, 'A', &iCurLen);
					pub_log_debug("[%s][%d] after addpackvalue()", 
						__FILE__, __LINE__);
					iCurLen = iTmpLen;
				} 
				else 
				{
					if (strcmp(node1->value, "FALSE") == 0) 
					{
						iTmpLen = iCurLen;
						memset(cVarvalue, 0, sizeof(cVarvalue));
						sprintf(cVarvalue, "%0*d", iEnd - iBegin, iCurLen - iEnd - 1);
						if (iBegin == 0)
						{
							iCurLen = 0;
						}
						addpackvalue(psBuf, iBegin, iEnd - iBegin, cVarvalue, 'A', &iCurLen);
						pub_log_debug("[%s][%d] after addpackvalue()", 
							__FILE__, __LINE__);
						iCurLen = iTmpLen;
					} 
					else 
					{
						int	tmp1 = 0;
						iTmpLen = iCurLen;
						tmp1 = atoi(node1->value);
						memset(cVarvalue, 0, sizeof(cVarvalue));
						sprintf(cVarvalue, "%0*d", iEnd - iBegin, iCurLen - tmp1);
						if (iBegin == 0)
						{
							iCurLen = 0;
						}
						addpackvalue(psBuf, iBegin, iEnd - iBegin, cVarvalue, 'A', &iCurLen);
						pub_log_debug("[%s][%d] after addpackvalue()",
							__FILE__, __LINE__);
						iCurLen = iTmpLen;
					}
				}
			}
			pnode = pnode->next;
		}
		
		pub_log_bin(SW_LOG_DEBUG, psBuf, iCurLen, "[%s][%d] �������,���ĳ���[%d]\n",
			__FILE__, __LINE__, iCurLen);
		psBuf[iCurLen] = '\0';
		pub_xml_deltree(pXml);
		
		return iCurLen;
	}
	
	pub_log_error("[%s][%d] ���ش���:δ�ҵ��������!", __FILE__, __LINE__);
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return -1;
}

int iSetLargeVar(char *sName, char *sValue, int iLen)
{
	FILE    *fp = NULL;
	struct  timeval tv;
	char	sBuf[128];
	char    sFileName[128];

	memset(&tv, 0x0, sizeof(tv));
	memset(sBuf, 0x0, sizeof(sBuf));
	memset(sFileName, 0x0, sizeof(sFileName));

	gettimeofday(&tv, NULL);
	sprintf(sFileName, "%s/ext/large_%d_%ld%ld.dat", getenv("SWWORK"), getpid(), tv.tv_sec, tv.tv_usec);
	pub_log_info("[%s][%d] sFileName====[%s]", __FILE__, __LINE__, sFileName);
	fp = fopen(sFileName, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, sFileName, errno, strerror(errno));
		return -1;
	}
	fwrite(sValue, iLen, 1, fp);
	fclose(fp);
	
	memset(sBuf, 0x0, sizeof(sBuf));
	sprintf(sBuf, "<LARGE>%08d%s", iLen, sFileName);
	set_zd_data(sName, sBuf);

	return 0;
}

int iLocSetLargeVar(sw_loc_vars_t *locvars, char *sName, char *sValue, int iLen)
{
	FILE    *fp = NULL;
	struct  timeval tv;
	char    sBuf[128];
	char    sFileName[128];

	memset(&tv, 0x0, sizeof(tv));
	memset(sBuf, 0x0, sizeof(sBuf));
	memset(sFileName, 0x0, sizeof(sFileName));

	gettimeofday(&tv, NULL);
	sprintf(sFileName, "%s/ext/large_%d_%ld%ld.dat", getenv("SWWORK"), getpid(), tv.tv_sec, tv.tv_usec);
	pub_log_info("[%s][%d] sFileName====[%s]", __FILE__, __LINE__, sFileName);
	fp = fopen(sFileName, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, sFileName, errno, strerror(errno));
		return -1;
	}
	fwrite(sValue, iLen, 1, fp);
	fclose(fp);

	memset(sBuf, 0x0, sizeof(sBuf));
	sprintf(sBuf, "<LARGE>%08d%s", iLen, sFileName);
	loc_set_zd_data(locvars, sName, sBuf);

	return 0;
}
