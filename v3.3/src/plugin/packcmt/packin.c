#include "packcmt.h"
/******************************************************
* �� �� ��:	iInitPkgIn
* ��������:	�����ʼ��
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-26
* �������:
*           pkg    ���Ļ����� 
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
******************************************************/
int iInitPkgIn(char *pkg, ST_PKGINFO *pstPkgInfo)
{
	int	iRc = 0;
	char	*ptr = NULL;
	char	*psBuf = pkg;
	char	sPkgNo[21];
	char	sCmtNo[4];
	char	sTxKind[6];
	char	sVarValue[512];
	char	sAppTradeCode[9];
	
	memset(sPkgNo, 0x0, sizeof(sPkgNo));
	memset(sCmtNo, 0x0, sizeof(sCmtNo));
	memset(sTxKind, 0x0, sizeof(sTxKind));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
	
	memcpy(sAppTradeCode, psBuf + 10, 8);
	pub_log_info("[%s][%d] ҵ������=[%s]", __FILE__, __LINE__, sAppTradeCode);
	memcpy(sCmtNo, sAppTradeCode + 1, 3);
	pub_log_info("[%s][%d] ���ı��=[%s]", __FILE__, __LINE__, sCmtNo);
	
	iRc = iGet2ndPkgNo(pkg, sCmtNo, sPkgNo);
	if (iRc == -1)
	{
		if (strcmp(sCmtNo, "108") == 0)
		{
			strcpy(sPkgNo, "hvps.111.001.01");
			pub_log_info("[%s][%d] ����˻�ҵ��!", __FILE__, __LINE__);
		}
		else if (strcmp(sCmtNo, "007") == 0)
		{
			strcpy(sPkgNo, "beps.121.001.01");
			pub_log_info("[%s][%d] С���˻�ҵ��!", __FILE__, __LINE__);
		}
		else
		{
			pub_log_error("[%s][%d] ȡ[%s]��Ӧ�������ı�ʶ�ų���!",
				__FILE__, __LINE__, sCmtNo);
			return -1;
		}
	}
	pub_log_info("[%s][%d] �������ı�ʶ��=[%s]", __FILE__, __LINE__, sPkgNo);
	memcpy(pstPkgInfo->sCmtNo, sPkgNo + 5, 3);
	memset(pstPkgInfo->sPkgNo, 0x0, sizeof(pstPkgInfo->sPkgNo));
	memcpy(pstPkgInfo->sPkgNo, sPkgNo + 5, 3);
	strcat(pstPkgInfo->sPkgNo, "_");
	memcpy(pstPkgInfo->sPkgNo + 4, sAppTradeCode + 1, 3);
	pub_log_info("[%s][%d] ���ĺ���=[%s]", __FILE__, __LINE__, pstPkgInfo->sPkgNo); 
	
	if (sAppTradeCode[0] == '1' || sAppTradeCode[0] == 'H')
	{
		/*** ���(CMT)���� ***/
		pstPkgInfo->cPkgType = '1';
	}
	else if (sAppTradeCode[0] == '2')
	{
		/*** С���(������CMT����PKG�ڽ��걨��ͷ��ȷ��) ***/
		pstPkgInfo->cPkgType = '2';
	}
	else
	{
		pub_log_error("[%s][%d] ҵ�������ʽ����ȷ, AppTradeCode=[%s]",
			__FILE__, __LINE__, sAppTradeCode);
		return -1;
	}
	
	return 0;
}

int iSetCheckVar(sw_loc_vars_t *locvars, char *pkg)
{
	int	iRc = 0;
	char	*ptr = NULL;
	char	*pValue = NULL;
	char	sTag[8];
	char	sCmtNo[8];
	char	sName[64];
	char	sValue[512];
	char	sXmlName[128];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pNode = NULL;
	sw_xmlnode_t	*pNode1 = NULL;
	
	memset(sTag, 0x0, sizeof(sTag));
	memset(sCmtNo, 0x0, sizeof(sCmtNo));
	memset(sName, 0x0, sizeof(sName));
	memset(sValue, 0x0, sizeof(sValue));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	sprintf(sXmlName, "%s/cfg/common/1st/check.xml", getenv("SWWORK"));
	iRc = access(sXmlName, F_OK);
	if (iRc)
	{
		pub_log_info("[%s][%d] [%s]������,�����д���!", __FILE__, __LINE__, sXmlName);
		return 0;
	}
	
	memset(sCmtNo, 0x0, sizeof(sCmtNo));
	strncpy(sCmtNo, pkg + 11, 3);
	pub_log_info("[%s][%d] һ�����ı��=[%s]", __FILE__, __LINE__, sCmtNo);

	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ����xml�ļ�[%s]����ʧ��!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	pNode = pub_xml_locnode(pXml, ".CHECK.ENTRY");
	while (pNode != NULL)
	{
		if (strcmp(pNode->name, "ENTRY") != 0)
		{
			pNode = pNode->next;
			continue;
		}
	
		pXml->current = pNode;
		pNode1 = pub_xml_locnode(pXml, "NO");
		if (pNode1 != NULL && pNode1->value != NULL)
		{
			if (strcmp(pNode1->value, sCmtNo) == 0)
			{
				break;
			}
		}
		pNode = pNode->next;
	}
	if (pNode == NULL)
	{
		pub_xml_deltree(pXml);
		pub_log_info("[%s][%d] δ����[%s]����Ϣ!", __FILE__, __LINE__, sCmtNo);
		return 0;
	}
	
	pNode = pub_xml_locnode(pXml, "ITEM");
	while (pNode != NULL)
	{
		if (strcmp(pNode->name, "ITEM") != 0)
		{
			pNode = pNode->next;
			continue;
		}
		
		memset(sTag, 0x0, sizeof(sTag));
		memset(sName, 0x0, sizeof(sName));
		memset(sValue, 0x0, sizeof(sValue));
		pXml->current = pNode;
		pNode1 = pub_xml_locnode(pXml, "TAG");
		if (pNode1 == NULL || pNode1->value == NULL)
		{
			pub_log_error("[%s][%d] δ����TAG��ǩ!", __FILE__, __LINE__);
			pNode = pNode->next;
			continue;
		}
		sprintf(sTag, ":%s:", pNode1->value);

		pNode1 = pub_xml_locnode(pXml, "NAME");
		if (pNode1 == NULL || pNode1->value == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			pNode = pNode->next;
			continue;
		}
		strncpy(sName, pNode1->value, sizeof(sName) - 1);
		ptr = strstr(pkg, sTag);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] ����δ��[%s]��ǩ!", __FILE__, __LINE__, sTag);
			pNode = pNode->next;
			continue;
		}
		ptr += 5;
		memset(sValue, 0x0, sizeof(sValue));
		pValue = sValue;
		while (ptr != '\0')
		{
			if (*ptr == '}' || (*ptr == ':' && *(ptr + 4) == ':'))
			{
				break;
			}
			*pValue++ = *ptr++;
		}
		pub_log_info("[%s][%d] �������:[%s]===[%s]===[%s]", __FILE__, __LINE__, sTag, sName, sValue);
		if (locvars == NULL)
		{
			set_zd_data(sName, sValue);
		}
		else
		{
			loc_set_zd_data(locvars, sName, sValue);
		}	
		pNode = pNode->next;
	}
	
	pub_xml_deltree(pXml);
	return 0;
}

/******************************************************
* �� �� ��:	iBasHeadMapIn
* ��������:	����ͷӳ��
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-26
* �������:
*           locvars    �����ػ����� 
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
******************************************************/
int iBasHeadMapIn(sw_loc_vars_t *locvars)
{
	char	sTmp[512];
	char	sSendTime[7];
	char	sVarValue[512];
	char	sAppTradeCode[9];
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sSendTime, 0x0, sizeof(sSendTime));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
	if (locvars == NULL)
	{
		memset(sAppTradeCode, 0x0, sizeof(sAppTradeCode));
		get_zd_data("$appTradeCode", sAppTradeCode);
		pub_log_info("[%s][%d] ҵ������$appTradeCode=[%s]", __FILE__, __LINE__, sAppTradeCode);
		if (sAppTradeCode[0] == '1' || sAppTradeCode[0] == 'H')
		{
			/*** ��� ***/
			/*** ����ϵͳ�� ***/
			set_zd_data("#OrigSenderSID", "HVPS");
			/*** ����ϵͳ�� ***/
			set_zd_data("#OrigReceiverSID", "HVPS");
		}
		else if (sAppTradeCode[0] == '2')
		{
			/*** С�� ***/
			/*** ����ϵͳ�� ***/
			set_zd_data("#OrigSenderSID", "BEPS");
			/*** ����ϵͳ�� ***/
			set_zd_data("#OrigReceiverSID", "BEPS");
		}
		
		memset(sTmp, 0x0, sizeof(sTmp));
		memset(sSendTime, 0x0, sizeof(sSendTime));
		/*** ���ķ���ʱ�� ***/
		get_zd_data("#OrigSendTime", sTmp);
		pub_log_info("[%s][%d] ���ķ���ʱ��#OrigSendTime=[%s]", __FILE__, __LINE__, sTmp);
		memcpy(sSendTime, sTmp + 8, sizeof(sSendTime) - 1);
		set_zd_data("#OrigSendTime", sSendTime);
		
		/*** ����ʱ���ʽHH:MM:SS ***/
		memset(sVarValue, 0x0, sizeof(sVarValue));
		memcpy(sVarValue, sSendTime, 2);
		memcpy(sVarValue + 2, ":", 1);
		memcpy(sVarValue + 3, sSendTime + 2, 2);
		memcpy(sVarValue + 5, ":", 1);
		memcpy(sVarValue + 6, sSendTime + 4, 2);
		set_zd_data("#TIME", sVarValue);
	}
	else
	{		
		loc_get_zd_data(locvars, "$appTradeCode", sAppTradeCode);
		pub_log_info("[%s][%d] $appTradeCode=[%s]", __FILE__, __LINE__, sAppTradeCode);
		if (sAppTradeCode[0] == '1' || sAppTradeCode[0] == 'H')
		{
			/*** ��� ***/
			/*** ����ϵͳ�� ***/
			loc_set_zd_data(locvars, "#OrigSenderSID", "HVPS");
			/*** ����ϵͳ�� ***/
			loc_set_zd_data(locvars, "#OrigReceiverSID", "HVPS");
		}
		else if (sAppTradeCode[0] == '2')
		{
			/*** С�� ***/
			/*** ����ϵͳ�� ***/
			loc_set_zd_data(locvars, "#OrigSenderSID", "BEPS");
			/*** ����ϵͳ�� ***/
			loc_set_zd_data(locvars, "#OrigReceiverSID", "BEPS");
		}
		
		memset(sTmp, 0x0, sizeof(sTmp));
		memset(sSendTime, 0x0, sizeof(sSendTime));
		/*** ���ķ���ʱ�� ***/
		loc_get_zd_data(locvars, "#OrigSendTime", sTmp);
		pub_log_info("[%s][%d] ���ķ���ʱ��#OrigSendTime=[%s]", __FILE__, __LINE__, sTmp);
		memcpy(sSendTime, sTmp + 8, sizeof(sSendTime) - 1);
		loc_set_zd_data(locvars, "#OrigSendTime", sSendTime);
		
		/*** ����ʱ���ʽHH:MM:SS ***/
		memset(sVarValue, 0x0, sizeof(sVarValue));
		memcpy(sVarValue, sSendTime, 2);
		memcpy(sVarValue + 2, ":", 1);
		memcpy(sVarValue + 3, sSendTime + 2, 2);
		memcpy(sVarValue + 5, ":", 1);
		memcpy(sVarValue + 6, sSendTime + 4, 2);
		loc_set_zd_data(locvars, "#TIME", sVarValue);
	}
	
	return 0;
} 

/******************************************************
* �� �� ��:	iMapHeaderIn
* ��������:	������ͷ��Ҫӳ��ı���ӳ�䵽�µ�XML����
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-16
* �������:
*          locvars   �����ػ�����
*          pXml        XML��
*          xml         �µ�XML��
*          node        ITEM�ڵ�
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
******************************************************/
int iMapHeaderIn(locvars, stPkgInfo, pXml, xml, node)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t *pXml;
sw_xmltree_t *xml;
sw_xmlnode_t *node;
{
	int	iRc = 0;
	int	iFlag = 0;
	char	sVarValue[512];
	char	sSecondVar[512];
	char	sCheckValue[512];
	ST_MAP	stMap;
	ST_ITEM	stItem;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sSecondVar, 0x0, sizeof(sSecondVar));
	memset(sCheckValue, 0x0, sizeof(sCheckValue));
	
	pub_log_info("[%s][%d] ��ʼ���б���ͷ����ӳ��!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] δ���ñ�ǩNAME!", 
				__FILE__, __LINE__);
			return -1;
		}
		memcpy(stItem.cVarname, node1->value, sizeof(stItem.cVarname) - 1);
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ���ñ�ǩTYPE!", 
				__FILE__, __LINE__);
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
			pub_log_debug("[%s][%d] δ���ñ�ǩMAP,˵���ñ�������Ҫ����ӳ��!", 
				__FILE__, __LINE__);
			pnode = pnode->next;
			continue;
		}
		
		/*** ���ñ�����Ҫӳ��ʱ�Ž��ñ���ȡ�� ***/
		if (locvars == NULL)
		{
			get_zd_data(stItem.cVarname, stItem.cVarvalue);
		}
		else
		{
			loc_get_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
		}
		pub_log_info("[%s][%d] ����[%s]=[%s]", 
			__FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
		while (node1 != NULL)
		{
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

			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
				if (stMap.cSendFlag == '2')
				{
					node1 = node1->next;
					continue;
				}
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
						
						/*** ĳЩ��SET��xml���� ***/
						if (strlen(sVarValue) == 0)
						{
							iGetXmlVariable(xml, node2->value, sVarValue);
						}
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
						
						/*** ĳЩ��SET��xml���� ***/
						if (strlen(sVarValue) == 0)
						{
							iGetXmlVariable(xml, node2->value, sVarValue);
						}
					}
				}
				
				pub_log_info("[%s][%d] CHECK����===[%s] ֵ===[%s]",
					__FILE__, __LINE__, node2->value, sVarValue);
				node2 = pub_xml_locnode(pXml, "VALUE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
						__FILE__, __LINE__);
					return -1;
				}
				strcpy(sCheckValue, node2->value);
				if (iCheckTx(sVarValue, sCheckValue) != 0)
				{
					pub_log_info("[%s][%d] sVarValue=[%s] sCheckValue=[%s] ���δͨ��!",
						__FILE__, __LINE__, sVarValue, sCheckValue);
					node1 = node1->next;
					continue;
				}
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
			if (node2 == NULL && stItem.cFlag == '1')
			{	
				pub_log_error("[%s][%d] ָ��Ϊ���һӳ�䵫��ûָ��MAPBEGIN!",
					__FILE__, __LINE__);
				return -1;
			}
			else if (node2 != NULL)
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
			
			node2 = pub_xml_locnode(pXml, "CONST");
			if (node2 != NULL)
			{
				stMap.cConst = '1';
				strcpy(stMap.sConstValue, node2->value);
			}
			
			if (stItem.cFlag == '1')
			{
				/*** ���һӳ�� ***/
				if (stMap.cNodeType == '1')
				{
					iGetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				}
				else if (stMap.cAnalysis == '1')
				{
					
					iGetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
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
				pub_log_info("[%s][%d] ȡ�����һ����=[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
				memcpy(stMap.sVarvalue + stMap.iMapBegin,
					stItem.cVarvalue + stMap.iBegin,
					stMap.iLength);
				pub_log_info("[%s][%d] ���һ����=[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{	
				/*** һ�Զ� ***/
				if (stMap.cConst == '1')
				{
					memcpy(stMap.sVarvalue, stMap.sConstValue + stMap.iBegin, stMap.iLength);
				}
				else
				{
					memcpy(stMap.sVarvalue, stItem.cVarvalue + stMap.iBegin, stMap.iLength);
				}
			}
			
			if (stMap.cDate == '1')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate8To10(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ��λ����תʮλ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** ���������Ҫӳ��,��ӳ�� ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue + 5, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue + 5, sSecondVar);
				}
				else
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue, sSecondVar);
				}
			}
			
			if (stMap.cNodeType == '1')
			{			
				/*** ӳ��ΪXML���� ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] Ϊ�ڵ�[%s]��������ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					pub_xml_deltree(pXml);
					pXml = NULL;
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** ӳ��ΪXML�ڵ� ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���ýڵ�ʧ��!",
						__FILE__, __LINE__);
					pub_xml_deltree(pXml);
					pXml = NULL;
					return -1;
				}
			}
			else
			{
				if (locvars == NULL)
				{	
					
					set_zd_data(stMap.sAlias, stMap.sVarvalue);
					
				}
				else
				{
					loc_set_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
					
				}
			}
			pub_log_info("[%s][%d] ӳ�����[%s]=[%s]", __FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			node1 = node1->next;
		}
		pnode = pnode->next;
	}
	
	return 0;
}

/****************************************************
* �� �� ��:	pGetNodeByTag
* ��������:	����TAGֵ�õ�TAG�ڵ�
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   node    TAG��ǩ��ʼ�ڵ�
*          pTag    TAG��ǩֵ
* �������:
*          TAGֵ��Ӧ��TAG�ڵ�
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
sw_xmlnode_t *pGetNodeByTag(sw_xmlnode_t *node, char *pTag)
{
       sw_xmlnode_t       *pnode = NULL;
       sw_xmlnode_t       *node1 = NULL;

       pnode = node;
       while (pnode != NULL)
       {
               node1 = pnode->firstchild;
               while (node1 != NULL)
               {
                       if (strcmp(node1->name, "NAME") == 0)
                       {
                               break;
                       }
                       node1 = node1->next;
               }
               if (node1 == NULL)
               {
               		return NULL;
               }
               if (strcmp(node1->value, pTag) == 0)
               {
                       return pnode;
               }
               pnode = pnode->next;
       }
       return NULL;
}

/****************************************************
* �� �� ��:	iDealTagIn
* ��������:	��TAG��ǩ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        TAG�ڵ�
*          pXml        XML��
*          xml         �µ�xml�� 
*          pkg         ���Ļ�����
*          len        ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iDealTagIn(locvars, stPkgInfo, node, pXml, xml, pkg, len, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
sw_xmlnode_t	*node;
sw_xmltree_t	*pXml;
sw_xmltree_t	*xml;
char	*pkg;
int	len;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	*ptr = NULL;
	char	*pTmp = NULL;
	char	*pTail = NULL;
	char	sTag[4];
	char	sTmp[512];
	char	sAmt[64];
	char	sVarName[512];
	char	sVarValue[512];
	char	sSecondVar[128];
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*pSaveNode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sTag, 0x0, sizeof(sTag));
	memset(sVarName, 0x0, sizeof(sVarName));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sSecondVar, 0x0, sizeof(sSecondVar));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��TAG��ǩ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	initMap(&stMap);
	initTag(&stTagNode);
	
	while (1)
	{
		if (memcmp(pkg + iCurLen, "}", 1) == 0)
		{
			iCurLen += 1;
			pub_log_info("[%s][%d] ��TAG��ǩ����!", __FILE__, __LINE__);
			break;
		}
		
		memset(sTag, 0x0, sizeof(sTag));
		if (memcmp(pkg + iCurLen, "{", 1) == 0)
		{
			memcpy(sTag, pkg + iCurLen, 3);
		}
		else if (memcmp(pkg + iCurLen, ":", 1) == 0)
		{
			memcpy(sTag, pkg + iCurLen + 1, 3);
		}
		else
		{
			pub_log_error("[%s][%d] ��ǩλ������!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] TAG=[%s]", __FILE__, __LINE__, sTag);
		
		pnode = pGetNodeByTag(node, sTag);
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] TAG��ǩ[%s]δ�������ļ��г���!", __FILE__, __LINE__, sTag);
			return -1;
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
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stTagNode.cConst = '1';
		}
		
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "ANALYSIS");
		if (node1 != NULL)
		{	
			stTagNode.cAnalysis = node1->value[0];
		}
		
		if (stTagNode.cConst != '1')
		{
			iCurLen += strlen(stTagNode.sTag) + 2;
		}

		pTmp = pkg + iCurLen;
		ptr = strstr(pTmp, ":");
		pTail = strstr(pTmp, "}");
		if (pTail == NULL)
		{
			pub_log_error("[%s][%d] ���ĸ�ʽ�쳣! pTmp=[%s]", __FILE__, __LINE__, pTmp);
			return -1;
		}
		
		if (stTagNode.cConst != '1')
		{
			if (ptr == NULL)
			{
				stTagNode.iLength = pTail - pTmp;
			}
			else if (pTail > ptr)
			{
				stTagNode.iLength = ptr - pTmp;
			}
			else
			{
				stTagNode.iLength = pTail - pTmp;
			}
		}
		
		memcpy(stTagNode.sTagValue, pkg + iCurLen, stTagNode.iLength);
		iCurLen += stTagNode.iLength;
		
		if (iCurLen > len)
		{
			if (strcmp(stTagNode.sTag, "72A") == 0)
			{
				pub_log_info("[%s][%d] 72A=== g_iLe===[%d]", __FILE__, __LINE__, iCurLen);
				iCurLen -= stTagNode.iLength;
				
				pTmp = pkg + iCurLen;
				ptr = strstr(pTmp, "{C:");
				if (ptr == NULL)
				{
					pub_log_error("[%s][%d] ��������!", __FILE__, __LINE__);
					return -1;
				}
				pub_log_info("[%s][%d] ptr===[%s]", __FILE__, __LINE__, ptr);
				memset(stTagNode.sTagValue, 0x0, sizeof(stTagNode.sTagValue));
				memcpy(stTagNode.sTagValue, pkg + iCurLen, ptr - pTmp - 1);
				iCurLen += (ptr - pTmp - 1);
				pub_log_info("[%s][%d] TAG=[72A] VALUE=[%s] iCurLen==[%d]",
					__FILE__, __LINE__, stTagNode.sTagValue, iCurLen);
			}
			else
			{	
				pub_log_error("[%s][%d] ���ĳ�������! iCurLen=[%d] len=[%d]",
					__FILE__, __LINE__, iCurLen, len);
				return -1;
			}
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
					
					/*** ĳЩ��SET��xml���� ***/
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, node1->value, sVarValue);
					}
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
					
					/*** ĳЩ��SET��xml���� ***/
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, node1->value, sVarValue);
					}
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
			
			pub_log_info("[%s][%d] CHECK����===[%s] ֵ===[%s]",
				__FILE__, __LINE__, node1->value, sVarValue);
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
				continue;
			}
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
		
		if (strlen(stTagNode.sAlias) == 0)
		{
			sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
		}
		
		pub_log_info("[%s][%d] �������[%s]=[%s]", 
			__FILE__, __LINE__, stTagNode.sAlias, stTagNode.sTagValue);
		
		if (strcmp(stTagNode.sAlias, "#counts") == 0)
		{
			set_zd_data("$count", stTagNode.sTagValue);
		}
		
		if (locvars == NULL)
		{	
			set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
		}
		else
		{
			loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
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
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!", __FILE__, __LINE__);
				return -1;
			}
			strcpy(stMap.sAlias, node2->value);
			
			node2 = pub_xml_locnode(pXml, "SENDFLAG");
			if (node2 != NULL)
			{
				stMap.cSendFlag = node2->value[0];
			}
			if (stMap.cSendFlag == '2')
			{
				node1 = node1->next;
				continue;
			}
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��,����[%s]ûָ��MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δָ��LENGTH��ǩ!", __FILE__, __LINE__);
				pub_xml_deltree(pXml);
				pXml = NULL;
				return -1;
			}
			stMap.iLength = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPFLAG");
			if (node2 != NULL)
			{
				stMap.cMapFlag = node2->value[0];
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
				strcpy(stTagNode.sCheckVar, sVarValue);
			
				node2 = pub_xml_locnode(pXml, "VALUE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] ָ����CHECKѡ�����ûָ��VALUE!",
						__FILE__, __LINE__);
					return -1;
				}
				strcpy(stTagNode.sCheckValue, node2->value);
				if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
				{
					pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
						__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
					node1 = node1->next;
					continue;
				}
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
			
			if (stTagNode.cFlag == '1')
			{
				/*** ���һӳ�� ***/
				if (stMap.cNodeType == '1')
				{
					iGetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				}
				else if (stMap.cAnalysis == '1')
				{
					iGetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
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
				
				pub_log_info("[%s][%d] ȡ�����һ����[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
				memcpy(stMap.sVarvalue + stMap.iMapBegin, 
					stTagNode.sTagValue + stMap.iBegin, 
					stMap.iLength);
				pub_log_info("[%s][%d] ���һ����[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				/*** һ�Զ�ӳ�� ***/
				if (stMap.cConst == '1')
				{
					memcpy(stMap.sVarvalue, stMap.sConstValue + stMap.iBegin, stMap.iLength);
				}
				else
				{
					memcpy(stMap.sVarvalue, stTagNode.sTagValue + stMap.iBegin, stMap.iLength);
				}
			}
			
			if (stMap.cDate == '1')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate8To10(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ��λ����תʮλ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** ���������Ҫӳ��,��ӳ�� ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue + 5, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue + 5, sSecondVar);
				}
				else
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue, sSecondVar);
				}
			}
			
			if (stMap.cAmt == '1')
			{
				int	i = 0;
				
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					i += 5;
				}
				if (strncmp(stMap.sVarvalue + i, "CNY", 3) == 0)
				{
					i += 3;
				}
				memset(sVarValue, 0x0, sizeof(sVarValue));
				strncpy(sVarValue, stMap.sVarvalue, i);
				pub_log_info("[%s][%d] sVarValue=[%s] stMap.sVarvalue=[%s] AMT=[%s]",
					__FILE__, __LINE__, sVarValue, stMap.sVarvalue, stMap.sVarvalue + i);

				memset(sAmt, 0x0, sizeof(sAmt));
				iNumToDouble(stMap.sVarvalue + i, sAmt);
				pub_log_info("[%s][%d]AMT: stMap.sVarvalue=[%s] sAmt=[%s]",
					__FILE__, __LINE__, stMap.sVarvalue, sAmt);
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				sprintf(stMap.sVarvalue, "%s%s", sVarValue, sAmt);
				pub_log_info("[%s][%d]AMT: VALUE===[%s]", __FILE__, __LINE__, stMap.sVarvalue);
			}
			
			if (stMap.cNodeType == '1')
			{
				/*** ӳ��ΪXML���� ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] Ϊ�ڵ�[%s]��������ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
				pub_log_info("[%s][%d] ���Խڵ�=[%s]=[%s]", 
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** ӳ��ΪXML�ڵ� ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���ýڵ�[%s]ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
				pub_log_info("[%s][%d] �ӽڵ����[%s]=[%s]", 
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				if (locvars == NULL)
				{
					set_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_set_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			pub_log_info("[%s][%d] ����[%s]=[%s]", 
				__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			node1 = node1->next;
		}
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ����TAG��ǩ֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealTagIn
* ��������:	��һ��PKGѭ��ҵ����TAG
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        TAG��ǩ�ڵ�
*          pXml        xml��
*          xml         �µ�xml��
*          pkg         ���Ļ�����
*          len         ���ĳ���
*          index       ѭ������
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iPKGDealTagIn(locvars, stPkgInfo, node, pXml, xml, pkg, len, index, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
sw_xmlnode_t	*node;
sw_xmltree_t	*pXml;
sw_xmltree_t	*xml;
char	*pkg;
int	len;
int	index;
int	*iLen;
{
	int	i = 0;
	int	iRc = 0;
	int	iCurLen = 0;
	int	iNum = 0;
	char	sTag[4];
	char	sAmt[64];
	char	sTxType[6];
	char	sVarValue[512];
	char	sSecondVar[128];
	char	*ptr = NULL;
	char	*pTmp = NULL;
	char	*pTail = NULL;
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*pSaveNode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sTag, 0x0, sizeof(sTag));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sTxType, 0x0, sizeof(sTxType));
	memset(sSecondVar, 0x0, sizeof(sSecondVar));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��PKG������֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	initMap(&stMap);
	initTag(&stTagNode);
	
	while (1)
	{
		if (memcmp(pkg + iCurLen, "}", 1) == 0)
		{
			iCurLen += 1;
			pub_log_info("[%s][%d] ��TAG��ǩ����!", __FILE__, __LINE__);
			break;
		}
		
		memset(sTag, 0x0, sizeof(sTag));
		if (memcmp(pkg + iCurLen, "{", 1) == 0)
		{
			memcpy(sTag, pkg + iCurLen, 3);
		}
		else if (memcmp(pkg + iCurLen, ":", 1) == 0)
		{
			memcpy(sTag, pkg + iCurLen + 1, 3);
		}
		else
		{
			pub_log_info("[%s][%d] @@@@@@@@@@ index=[%d] iCurLen=[%d]", 
				__FILE__, __LINE__, index, iCurLen);
			pub_log_error("[%s][%d] ��ǩλ������! pkg=[%s]", 
				__FILE__, __LINE__, pkg + iCurLen);
			pub_log_bin(SW_LOG_DEBUG, pkg + iCurLen, len - iCurLen, "[%s][%d] ���ĳ���[%d]\n",
				__FILE__, __LINE__, len - iCurLen);
			return -1;
		}
		pub_log_info("[%s][%d] TAG=[%s]", __FILE__, __LINE__, sTag);
		
		
		if (strcmp(sTag, "72C") == 0)
		{
			/*** ������ ***/
			/*** ȡҵ�����ͺ�,�õ��������ʽ ***/
			pSaveNode = pXml->current;
			memset(sTxType, 0x0, sizeof(sTxType));
			get_zd_data("$txtype", sTxType);
			pub_log_info("[%s][%d] ҵ�����ͺ�=[%s]", __FILE__, __LINE__, sTxType);
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
				if (strcmp(node2->value, sTxType) == 0)
				{
					break;
				}
				node1 = node1->next;
			}
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] û���ҵ�ƥ���ҵ����������!", 
					__FILE__, __LINE__);
				/*** ����:72C: ***/
				iCurLen += 5;
				while (pkg[iCurLen] != '\0')
				{
					if (pkg[iCurLen] == '}' || (pkg[iCurLen] == ':' && pkg[iCurLen + 4] == ':'))
					{
						break;
					}
					iCurLen++;
				}
				pub_log_info("[%s][%d] 72C���ݱ�����!", __FILE__, __LINE__);
				pXml->current = pSaveNode;
				continue;
			}
			/*** �𸽼��� ***/
			pXml->current = node1;
			node1 = pub_xml_locnode(pXml, "TAG");
			iRc = iPKGDealAppendTagIn(locvars, stPkgInfo, node1, pXml, xml, pkg, len, index, &iCurLen);
			if (iRc == -1)
			{
				pub_log_error("[%s][%d] ����TAG��ǩʧ��!", __FILE__, __LINE__);
				return -1;
			}
			pXml->current = pSaveNode;
			continue;
		}	
		pnode = pGetNodeByTag(node, sTag);
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] TAG��ǩ[%s]δ�������ļ��г���!", __FILE__, __LINE__, sTag);
			return -1;
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
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		
		node1 = pub_xml_locnode(pXml, "CONST");
		if (node1 != NULL)
		{
			stTagNode.cConst = '1';
		}
		
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
		
		node1 = pub_xml_locnode(pXml, "LIST");
		if (node1 != NULL)
		{
			stTagNode.cList = node1->value[0];
		}
		
		if (stTagNode.cList == '1')
		{
			/*** �տ���/�������嵥 ***/
			iCurLen += 5;
			pSaveNode = pXml->current;
			for (i = 0; i < iNum; i++)
			{
				node1 = pub_xml_locnode(pXml, ".LIST.TAG");
				if (node1 == NULL)
				{
					pub_log_error("[%s][%d] �����ļ���ָ�����嵥��ʶ������"
						"û�������嵥�б�[LIST.TAG]!", __FILE__, __LINE__);
					return -1;
				}
				
				pub_log_info("[%s][%d] PKGNO====[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
				if (strcmp(stPkgInfo.sPkgNo, "133_006") == 0 || strcmp(stPkgInfo.sPkgNo, "125_005") == 0)
				{
					index = i;
				}
				
				iRc = iPKGDealListIn(locvars, stPkgInfo, node1, pXml, xml, pkg, len, index, i, &iCurLen);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ����TAG��ǩʧ��!", __FILE__, __LINE__);
					return -1;
				}
			}
			pXml->current = pSaveNode;
			continue;
		}
		
		node1 = pub_xml_locnode(pXml, "ANALYSIS");
		if (node1 != NULL)
		{	
			stTagNode.cAnalysis = node1->value[0];
		}
		
		if (stTagNode.cConst != '1')
		{
			iCurLen += strlen(stTagNode.sTag) + 2;
		}
		
		/*modify at xinyang*/
		pTmp = pkg + iCurLen;
		ptr = strstr(pTmp, ":");
		pTail = strstr(pTmp, "}");
		if (pTail == NULL)
		{
			pub_log_error("[%s][%d] ���ĸ�ʽ�쳣! pTmp=[%s]", __FILE__, __LINE__, pTmp);
			return -1;
		}
		if (stTagNode.cConst != '1')
		{
			if (ptr == NULL)
			{
				stTagNode.iLength = pTail - pTmp;
			}
			else if (pTail > ptr)
			{
				stTagNode.iLength = ptr - pTmp;
			}
			else
			{
				stTagNode.iLength = pTail - pTmp;
			}
		}
		
		memcpy(stTagNode.sTagValue, pkg + iCurLen, stTagNode.iLength);
		iCurLen += stTagNode.iLength;
		if (iCurLen > len)
		{
			pub_log_error("[%s][%d] ���ĳ�������! iCurLen=[%d] len=[%d]",
				__FILE__, __LINE__, iCurLen, len);
			return -1;
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
				continue;
			}
		}
		
		if (stTagNode.cNum == '1')
		{
			iNum = atoi(stTagNode.sTagValue);
			pub_log_info("[%s][%d] �տ���/��������Ŀ=[%d]",
				__FILE__, __LINE__, iNum);
		}
		
		if (strlen(stTagNode.sAlias) == 0)
		{
			sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
		}
		
		pub_log_info("[%s][%d] �������[%s]=[%s]", 
			__FILE__, __LINE__, stTagNode.sAlias, stTagNode.sTagValue);
		
		if (strcmp(stTagNode.sAlias, "#txtype") == 0)
		{
			set_zd_data("$txtype", stTagNode.sTagValue);
			
			if (locvars == NULL)
			{
				set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
			}
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
		
		if (locvars == NULL)
		{	
			set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
		}
		else
		{
			loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
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
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!", __FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, index);
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��,����[%s]ûָ��MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δָ��LENGTH��ǩ!", __FILE__, __LINE__);
				pub_xml_deltree(pXml);
				pXml = NULL;
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
			if (stMap.cSendFlag == '2')
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
			
			if (stTagNode.cFlag == '1')
			{
				/*** ���һӳ�� ***/
				if (stMap.cNodeType == '1')
				{
					iGetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				}
				else if (stMap.cAnalysis == '1')
				{
					iGetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
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
				/*** ������һ�������ܻ����ǰ��������ں�������,������Ҫ��ǰ��ĳ��Ȳ����Ȳ��� ***/
				if (strlen(stMap.sVarvalue) == 0)
				{
					memset(stMap.sVarvalue, '0', stMap.iMapBegin);
				}
				memcpy(stMap.sVarvalue + stMap.iMapBegin, 
					stTagNode.sTagValue + stMap.iBegin, 
					stMap.iLength);
			}
			else
			{
				/*** һ�Զ�ӳ�� ***/
				if (stMap.cConst == '1')
				{
					memcpy(stMap.sVarvalue, stMap.sConstValue + stMap.iBegin, stMap.iLength);
				}
				else
				{
					memcpy(stMap.sVarvalue, stTagNode.sTagValue + stMap.iBegin, stMap.iLength);
				}
			}
			
			if (stMap.cDate == '1')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate8To10(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ��λ����תʮλ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** ���������Ҫӳ��,��ӳ�� ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue + 5, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue + 5, sSecondVar);
				}
				else
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue, sSecondVar);
				}
			}
			
			if (stMap.cAmt == '1')
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				iNumToDouble(stMap.sVarvalue, sAmt);
				pub_log_info("[%s][%d]AMT: stMap.sVarvalue=[%s] sAmt=[%s]",
					__FILE__, __LINE__, stMap.sVarvalue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
				pub_log_info("[%s][%d]AMT: VALUE===[%s]", __FILE__, __LINE__, stMap.sVarvalue);
			}
			
			if (stMap.cNodeType == '1')
			{
				/*** ӳ��ΪXML���� ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] Ϊ�ڵ�[%s]��������ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** ӳ��ΪXML�ڵ� ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���ýڵ�[%s]ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
			}
			else
			{
				if (locvars == NULL)
				{
					set_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_set_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			node1 = node1->next;
		}
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ���긽����֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealAppendTagIn
* ��������:	��һ��PKGҵ���帽����
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        TAG��ǩ�ڵ�
*          pXml        xml��
*          xml         �µ�xml��
*          pkg         ���Ļ�����
*          len         ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iPKGDealAppendTagIn(locvars, stPkgInfo, node, pXml, xml, pkg, len, index, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
sw_xmlnode_t	*node;
sw_xmltree_t	*pXml;
sw_xmltree_t	*xml;
char	*pkg;
int	len;
int	index;
int	*iLen;
{
	int	i = 0;
	int	iRc = 0;
	int	iFlag = 0;
	int	iCurLen = 0;
	int	iAppendNum = 0;
	char	*ptr = NULL;
	char	*pValue = NULL;
	char	sAmt[64];
	char	sPath[256];
	char	sTxType[6];
	char	sVarValue[512];
	char	sSecondVar[512];
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*pSaveNode = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sPath, 0x0, sizeof(sPath));
	memset(sTxType, 0x0, sizeof(sTxType));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sSecondVar, 0x0, sizeof(sSecondVar));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] �𸽼���֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);

	initMap(&stMap);
	initTag(&stTagNode);
	
	/*** :72C: ***/
	iCurLen += 5;
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
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
		pub_log_info("[%s][%d] sAlias=[%s] stTagNode.iLenth=[%d]",
			__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength);
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "ANALYSIS");
		if (node1 != NULL)
		{	
			stTagNode.cAnalysis = node1->value[0];
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
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
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
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] ������������! VAR=[%s]",
						__FILE__, __LINE__, node1->value);
					return -1;
				}
			}
			stTagNode.iLength = atoi(sVarValue);
			pub_log_info("[%s][%d] TAG=[%s] LENGTH=[%d]",
				__FILE__, __LINE__, stTagNode.sTag, stTagNode.iLength);
		}
		
		iFlag = 0;
		if (stTagNode.iLength >= MAX_TAGVALUE_LEN)
		{
			pub_log_info("[%s][%d] ����[%s]����,���ж�̬����!",
				__FILE__, __LINE__, stTagNode.sAlias);
			iFlag = 1;
			ptr = calloc(1, sizeof(char) * stTagNode.iLength + 1);
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			memcpy(ptr, pkg + iCurLen, stTagNode.iLength);
		}
		else
		{
			memcpy(stTagNode.sTagValue, pkg + iCurLen, stTagNode.iLength);
			ptr = stTagNode.sTagValue;
		}
		
		iCurLen += stTagNode.iLength;
		pub_log_info("[%s][%d] AAAAAAAAAAAAA iCurLen=[%d]",
			__FILE__, __LINE__, iCurLen);
		if (iCurLen > len)
		{
			pub_log_error("[%s][%d] ���ĳ�������! iCurLen=[%d] len=[%d]",
				__FILE__, __LINE__, iCurLen, len);
			if (iFlag == 1)
			{
				free(ptr);
			}
			return -1;
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
				if (iFlag == 1)
				{
					free(ptr);
				}
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] ���δͨ��!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				if (iFlag == 1)
				{
					free(ptr);
				}
				continue;
			}
		}
		
		/*** �������е���ϸ���� ***/
		node1 = pub_xml_locnode(pXml, "NUM");
		if (node1 != NULL)
		{
			stTagNode.cNum = '1';
		}
		
		if (stTagNode.cNum == '1')
		{
			iAppendNum = atoi(ptr);
			pub_log_info("[%s][%d] ��������ϸ����=[%d]",
				__FILE__, __LINE__, iAppendNum);
		}
		
		/*** �������е���ϸ�嵥 ***/
		node1 = pub_xml_locnode(pXml, "LIST");
		if (node1 != NULL)
		{
			pSaveNode = pXml->current;
			memset(sTxType, 0x0, sizeof(sTxType));
			get_zd_data("$txtype", sTxType);
			pub_log_info("[%s][%d] ҵ�����ͺ�=[%s]", __FILE__, __LINE__, sTxType);
			node1 = pub_xml_locnode(pXml, ".APPENDLIST");
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
					if (iFlag == 1)
					{
						free(ptr);
					}
					return -1;
				}
				if (strcmp(node2->value, sTxType) == 0)
				{
					break;
				}
				node1 = node1->next;
			}
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] û���ҵ�ƥ��ĸ�����ҵ����������!", 
					__FILE__, __LINE__);
				if (iFlag == 1)
				{
					free(ptr);
				}
				return -1;
			}
			/*** �𸽼����е���ϸ�嵥 ***/
			pXml->current = node1;
			node1 = pub_xml_locnode(pXml, "TAG");
			for (i = 0; i < iAppendNum; i++)
			{
				iRc = iPKGDealListIn(locvars, stPkgInfo, node1, pXml, xml, pkg, len, index, i, &iCurLen);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���������е���ϸ�嵥ʧ��!", __FILE__, __LINE__);
					if (iFlag == 1)
					{
						free(ptr);
					}
					return -1;
				}
			}
			pXml->current = pSaveNode;
			pnode = pnode->next;
			continue;
		}
		
		if (strlen(stTagNode.sAlias) == 0)
		{
			sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
		}
		
		pub_log_info("[%s][%d] �������[%s]=[%s]", 
			__FILE__, __LINE__, stTagNode.sAlias, ptr);
		
		if (iFlag != 1)
		{
			if (locvars == NULL)
			{	
				set_zd_data(stTagNode.sAlias, ptr);
			}
			else
			{
				loc_set_zd_data(locvars, stTagNode.sAlias, ptr);
			}
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
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
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!", __FILE__, __LINE__);
				if (iFlag == 1)
				{
					free(ptr);
				}
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
					if (iFlag == 1)
					{
						free(ptr);
					}
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
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!", __FILE__, __LINE__);
				if (iFlag == 1)
				{
					free(ptr);
				}
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
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��,����[%s]ûָ��MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				if (iFlag == 1)
				{
					free(ptr);
				}
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δָ��LENGTH��ǩ!", __FILE__, __LINE__);
				if (iFlag == 1)
				{
					free(ptr);
				}
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
				}
				else if (node2->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
					stMap.iLength = atoi(sVarValue);
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
				}
				else if (node2->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
					stMap.iLength = atoi(sVarValue);
				}
				else
				{
					stMap.iLength = atoi(node2->value);		
				}
				pub_log_info("[%s][%d] sAlias=[%s] LENGTH=[%d]",
					__FILE__, __LINE__, stMap.sAlias, stMap.iLength);
			}
			
			if (stMap.iLength >= MAX_TAGVALUE_LEN)	
			{
				stMap.cVarLen = '1';
				pValue = (char *)calloc(1, stMap.iLength + 1);
				if (pValue == NULL)
				{
					if (iFlag == 1)
					{
						free(ptr);
					}
					pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]", 
						__FILE__, __LINE__, errno, strerror(errno));
					return -1;
				}
			}
			else
			{
				pValue = stMap.sVarvalue;
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
			if (stMap.cSendFlag == '2')
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
			
			if (stTagNode.cFlag == '1')
			{
				/*** ���һӳ�� ***/
				if (stMap.cNodeType == '1')
				{
					iGetXmlAttrVar(xml, stMap.sAlias, pValue);
				}
				else if (stMap.cAnalysis == '1')
				{
					iGetXmlVariable(xml, stMap.sAlias, pValue);
				}
				else
				{	
					if (locvars == NULL)
					{
						get_zd_data(stMap.sAlias, pValue);
					}
					else
					{
						loc_get_zd_data(locvars, stMap.sAlias, pValue);	
					}
				}
				memcpy(pValue + stMap.iMapBegin, ptr + stMap.iBegin, stMap.iLength);
			}
			else
			{
				/*** һ�Զ�ӳ�� ***/
				memcpy(pValue, ptr + stMap.iBegin, stMap.iLength);
			}
			
			if (stMap.cDate == '1')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate8To10(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ��λ����תʮλ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					if (iFlag == 1)
					{
						free(ptr);
					}
					if (stMap.cVarLen == '1')
					{
						free(pValue);
					}
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** ���������Ҫӳ��,��ӳ�� ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue + 5, stMap.cMapFlag);
						if (iFlag == 1)
						{
							free(ptr);
						}
						if (stMap.cVarLen == '1')
						{
							free(pValue);
						}
						return -1;
					}
					strcpy(stMap.sVarvalue + 5, sSecondVar);
				}
				else
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue, stMap.cMapFlag);
						if (iFlag == 1)
						{
							free(ptr);
						}
						if (stMap.cVarLen == '1')
						{
							free(pValue);
						}
						return -1;
					}
					strcpy(stMap.sVarvalue, sSecondVar);
				}
			}
			
			if (stMap.cAmt == '1')
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				iNumToDouble(stMap.sVarvalue, sAmt);
				pub_log_info("[%s][%d]AMT: stMap.sVarvalue=[%s] sAmt=[%s]",
					__FILE__, __LINE__, stMap.sVarvalue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
				pub_log_info("[%s][%d]AMT: VALUE===[%s]", __FILE__, __LINE__, stMap.sVarvalue);
			}
			
			if (stMap.cNodeType == '1')
			{
				/*** ӳ��ΪXML���� ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] Ϊ�ڵ�[%s]��������ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					if (iFlag == 1)
					{
						free(ptr);
					}
					if (stMap.cVarLen == '1')
					{
						free(pValue);
					}
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** ӳ��ΪXML�ڵ� ***/
				if (stMap.cVarLen == '1')
				{
					iRc = iSetLargeVariable(xml, stMap.sAlias, pValue);
				}
				else
				{
					iRc = iSetXmlVariable(xml, stMap.sAlias, pValue);
				}
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���ýڵ�[%s]ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					if (iFlag == 1)
					{
						free(ptr);
					}
					if (stMap.cVarLen == '1')
					{
						free(pValue);
					}
					return -1;
				}
			}
			else
			{
				if (locvars == NULL)
				{
					if (stMap.cVarLen == '1')
					{
						iSetLargeVar(stMap.sAlias, pValue, strlen(pValue));
					}
					else
					{
						set_zd_data(stMap.sAlias, pValue);
					}
				}
				else
				{
					if (stMap.cVarLen == '1')
					{
						iLocSetLargeVar(locvars, stMap.sAlias, pValue, strlen(pValue));
					}
					else
					{
						loc_set_zd_data(locvars, stMap.sAlias, pValue);
					}
				}
			}

			if (stMap.cVarLen == '1')
			{
				free(pValue);
			}
			node1 = node1->next;
		}
		
		if (iFlag == 1)
		{
			free(ptr);
		}
		pnode = pnode->next;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ���긽����֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealListIn
* ��������:	��һ��PKGҵ�����е��տ���/�������嵥
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        TAG��ǩ�ڵ�
*          pXml        xml��
*          xml         �µ�xml��
*          pkg         ���Ļ�����
*          len         ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iPKGDealListIn(locvars, stPkgInfo, node, pXml, xml, pkg, len, indexi, indexj, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
sw_xmlnode_t	*node;
sw_xmltree_t	*pXml;
sw_xmltree_t	*xml;
char	*pkg;
int	len;
int indexi;
int	indexj;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sAmt[64];
	char	sPath[256];
	char	sVarValue[512];
	char	sSecondVar[512];
	ST_MAP	stMap;
	TAGNODE	stTagNode;
	sw_xmlnode_t	*pnode = node;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sAmt, 0x0, sizeof(sAmt));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	memset(sSecondVar, 0x0, sizeof(sSecondVar));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] �𸽼����嵥֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	initMap(&stMap);
	initTag(&stTagNode);
	
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
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		stTagNode.cType = node1->value[0];
		stTagNode.iLength = atoi(node1->value + 1);
	
		node1 = pub_xml_locnode(pXml, "VARLEN");
		if (node1 != NULL && node1->value != NULL)
		{
			memset(sPath, 0x0, sizeof(sPath));
			sprintf(sPath, node1->value, indexi);
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					get_zd_data(sPath, sVarValue);
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
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
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					
					pub_log_info("[%s][%d] ���ȱ���=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] ������������! VAR=[%s]",
						__FILE__, __LINE__, node1->value);
					return -1;
				}
			}
			stTagNode.iLength = atoi(sVarValue);
			pub_log_info("[%s][%d] TAG=[%s] LENGTH=[%d]",
				__FILE__, __LINE__, stTagNode.sTag, stTagNode.iLength);
		}
		pub_log_info("[%s][%d] sAlias=[%s] stTagNode.iLenth=[%d]",
			__FILE__, __LINE__, stTagNode.sAlias, stTagNode.iLength);
		node1 = pub_xml_locnode(pXml, "FLAG");
		if (node1 != NULL)
		{
			stTagNode.cFlag = node1->value[0];
		}
		
		node1 = pub_xml_locnode(pXml, "ANALYSIS");
		if (node1 != NULL)
		{	
			stTagNode.cAnalysis = node1->value[0];
		}
		
		memcpy(stTagNode.sTagValue, pkg + iCurLen, stTagNode.iLength);
		iCurLen += stTagNode.iLength;
		pub_log_info("[%s][%d] AAAAAAAAAAAAA iCurLen=[%d]",
			__FILE__, __LINE__, iCurLen);
		if (iCurLen > len)
		{
			pub_log_error("[%s][%d] ���ĳ�������! iCurLen=[%d] len=[%d]",
				__FILE__, __LINE__, iCurLen, len);
			return -1;
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
				continue;
			}
		}
		
		if (strlen(stTagNode.sAlias) == 0)
		{
			sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
		}
		
		pub_log_info("[%s][%d] �������[%s]=[%s]", 
			__FILE__, __LINE__, stTagNode.sAlias, stTagNode.sTagValue);
		
		if (locvars == NULL)
		{	
			set_zd_data(stTagNode.sAlias, stTagNode.sTagValue);
		}
		else
		{
			loc_set_zd_data(locvars, stTagNode.sAlias, stTagNode.sTagValue);
		}
		
		node1 = pub_xml_locnode(pXml, "MAP");
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
				pub_log_error("[%s][%d] δ����ALIAS��ǩ!", __FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, indexi, indexj);
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δ����BEGIN��ǩ!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]ָ��FLAG=1Ϊ���һӳ��,����[%s]ûָ��MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] δָ��LENGTH��ǩ!", __FILE__, __LINE__);
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
			if (stMap.cSendFlag == '2')
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
			
			if (stTagNode.cFlag == '1')
			{
				/*** ���һӳ�� ***/
				if (stMap.cNodeType == '1')
				{
					iGetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				}
				else if (stMap.cAnalysis == '1')
				{
					iGetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
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
				memcpy(stMap.sVarvalue + stMap.iMapBegin, 
					stTagNode.sTagValue + stMap.iBegin, 
					stMap.iLength);
			}
			else
			{
				/*** һ�Զ�ӳ�� ***/
				if (stMap.cConst == '1')
				{
					memcpy(stMap.sVarvalue, stMap.sConstValue + stMap.iBegin, stMap.iLength);
				}
				else
				{	
					memcpy(stMap.sVarvalue, stTagNode.sTagValue + stMap.iBegin, stMap.iLength);
				}
			}
			
			if (stMap.cDate == '1')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate8To10(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ��λ����תʮλ���ڳ���! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** ���������Ҫӳ��,��ӳ�� ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue + 5, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue + 5, sSecondVar);
				}
				else
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] ȡ[%s]��Ӧ������ֵ����! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] һ�����ı��=[%s] һ����ֵ=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue, sSecondVar);
				}
			}
			
			if (stMap.cAmt == '1')
			{
				memset(sAmt, 0x0, sizeof(sAmt));
				iNumToDouble(stMap.sVarvalue, sAmt);
				pub_log_info("[%s][%d]AMT: stMap.sVarvalue=[%s] sAmt=[%s]",
					__FILE__, __LINE__, stMap.sVarvalue, sAmt);
				strcpy(stMap.sVarvalue, sAmt);
				pub_log_info("[%s][%d]AMT: VALUE===[%s]", __FILE__, __LINE__, stMap.sVarvalue);
			}
			
			if (stMap.cNodeType == '1')
			{
				/*** ӳ��ΪXML���� ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] Ϊ�ڵ�[%s]��������ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** ӳ��ΪXML�ڵ� ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] ���ýڵ�[%s]ʧ��!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
			}
			else
			{
				if (locvars == NULL)
				{
					set_zd_data(stMap.sAlias, stMap.sVarvalue);
				}
				else
				{
					loc_set_zd_data(locvars, stMap.sAlias, stMap.sVarvalue);
				}
			}
			node1 = node1->next;
		}
		pnode = pnode->next;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ���긽�����嵥�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}


/****************************************************
* �� �� ��:	iPKGDealPkgHeadIn
* ��������:	��һ��PKG��ͷ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          xml         XML��
*          pkg         ���Ļ�����
*          len        ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iPKGDealPkgHeadIn(locvars, stPkgInfo, xml, pkg, len, iLen)
sw_loc_vars_t	*locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t	*xml;
const char	*pkg;
const int	len;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��PKG��ͷ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] PKG��Ϊ =[%s]", 
		__FILE__, __LINE__, stPkgInfo.sPkgNo);
	if (getenv("SWWORK") != NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/PKG%s.xml", 
			getenv("SWWORK"), stPkgInfo.sPkgNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/PKG%s.xml", 
			getenv("HOME"), stPkgInfo.sPkgNo);
	}
	pub_log_info("[%s][%d] XML�ļ�Ϊ=[%s]", __FILE__, __LINE__, sXmlName);
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��XML��ʧ��,�����ļ�[%s]�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	/*** ���Ƚ�����ͷ����Ҫӳ��ı�������ӳ�� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.BEFORMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, xml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ӳ�䱨��ͷʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}

	/*** �������� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.PKGTRADE.TAG");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ����[TAGLIST.PKGTRADE.TAG]��ǩ!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	iRc = iDealTagIn(locvars, stPkgInfo, pnode, pXml, xml, pkg, len, iLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ����TAG��ǩʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pub_log_info("[%s][%d] ����PKG��ͷ֮�� iCurLen=[%d]", __FILE__, __LINE__, *iLen);
	
	/*** ������ͷ����Ҫӳ��ı�������ӳ�� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.ENDMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, xml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ӳ�䱨��ͷʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pub_log_info("[%s][%d] ���ͷ���!", __FILE__, __LINE__);
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return 0;
}

/****************************************************
* �� �� ��:	iPKGDealBodyIn
* ��������:	��һ��PKG����
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          xml         XML��
*          pkg         ���Ļ�����
*          len        ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iPKGDealBodyIn(locvars, stPkgInfo, xml, pkg, len, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t	*xml;
char	*pkg;
int	len;
int	*iLen;
{
	int	i = 0;
	int	iRc = 0;
	int	iCnt = 0;
	int	iCurLen = 0;
	char	sTmp[9];
	char	sFactorNo[8];
	char	sTradeHead[17]; /*** ҵ��ͷ16Ϊ 3(���ʶ) + 3(ҵ��Ҫ�ؼ���) + 9 + 1 ***/
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sFactorNo, 0x0, sizeof(sFactorNo));
	memset(sTradeHead, 0x0, sizeof(sTradeHead));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��PKG����֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** ȡ��ϸ�ܱ��� ***/
	get_zd_data("$count", sTmp);
	zip_space(sTmp);
	pub_log_info("[%s][%d] ��ϸ�ܱ���=[%s]", __FILE__, __LINE__, sTmp);
	iCnt = atoi(sTmp);
	pub_log_info("[%s][%d] ��ϸ�ܱ���iCnt=[%d]", __FILE__, __LINE__, iCnt);
	
	/*** ѭ��ҵ���ҵ��Ҫ�ؼ��Ŷ���ͬ,ֻȡһ�� ***/
	memcpy(sTradeHead, pkg + iCurLen, 16);
	if (strlen(stPkgInfo.sCmtNo) > 0)
	{
		memcpy(sFactorNo, stPkgInfo.sCmtNo, 3);
		strcat(sFactorNo, "_");
		memcpy(sFactorNo + 4, sTradeHead + 3, 3);
	}
	else
	{
		memcpy(sFactorNo, sTradeHead + 3, 3);
	}
	pub_log_info("[%s][%d] ҵ��Ҫ�ؼ���=[%s]", __FILE__, __LINE__, sFactorNo);
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	if (getenv("SWWORK") != NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/tx%s.xml", 
			getenv("SWWORK"), sFactorNo);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/tx%s.xml", 
			getenv("HOME"), sFactorNo);
	}
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��XML��ʧ��,����XML�ļ�[%s]"
			"�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	pub_log_info("[%s][%d] stPkgInfo.sPkgNo====[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
	
	if (strcmp(stPkgInfo.sPkgNo, "133_006") == 0 || strcmp(stPkgInfo.sPkgNo, "125_005") == 0)
	{
		iCnt = 1;
	}
	pub_log_info( "[%s][%d] =======iCnt=====[%d]", __FILE__, __LINE__, iCnt);
	for (i = 0; i < iCnt; i++)
	{
		/*** ��ҵ��ͷ 3 + 3 + 9 + 1 ***/
		memcpy(sTradeHead, pkg + iCurLen, 16);
		memcpy(sFactorNo, sTradeHead + 3, 3);
		iCurLen += 16;
		pub_log_info("[%s][%d] ҵ��Ҫ�ؼ���=[%s] iCurLen=[%d]", 
			__FILE__, __LINE__, sFactorNo, iCurLen);
		
		pXml->current = pXml->root;
		/*** �������� ***/
		pnode = pub_xml_locnode(pXml, ".TAGLIST.TAG");
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] XML�ļ�[%s]δ����TAG��ǩ!", 
				__FILE__, __LINE__, sXmlName);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
		iRc = iPKGDealTagIn(locvars, stPkgInfo, pnode, pXml, xml, pkg, len, i, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] ����TAG��ǩʧ��!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
	}
	pub_xml_deltree(pXml);
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ����������֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] �����������!", __FILE__, __LINE__);
	
	return 0;
}

/****************************************************
* �� �� ��:	iCMTDealTradeBodyIn
* ��������:	����������Ҫӳ��ı���ӳ�䵽�µ�XML����
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-16
* �������:
*		   locvars   �����ػ�����
*          xml         �µ�XML��
*          psBuf       ���Ļ�����(��������ͷ��ҵ��ͷ)
*          iLen        ���ĳ���(��������ͷ��ҵ��ͷ)
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/				
int iCMTDealTradeBodyIn(locvars, stPkgInfo, newxml, pkg, len, iLen)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmltree_t *newxml;
char *pkg;
int len;
int	*iLen;
{	
	int	iRc = 0;
	int	iCurLen = 0;
	char	sXmlName[512];
	char	sFileName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;

	memset(sXmlName, 0x0, sizeof(sXmlName));
	memset(sFileName, 0x0, sizeof(sFileName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��������֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (memcmp(pkg + iCurLen, "{3:", 3) != 0)
	{
		pub_log_error("[%s][%d] ���������忪ʼ�������!",
			__FILE__, __LINE__);
		pub_log_bin(SW_LOG_INFO, pkg, strlen(pkg), "���������忪ʼ�������");
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
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��xml��ʧ��! ����xml�ļ�[%s]"
			"�Ƿ���ڻ��߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	/*** Ϊ��������������ʱ����,#TEMP="T" ***/		
	if (locvars == NULL)
	{
		set_zd_data("#TEMP", "T");
	}
	else
	{
		loc_set_zd_data(locvars, "#TEMP", "T");
	}
	/*** ���Ƚ�����ͷ����Ҫӳ��ı�������ӳ�� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.BEFORMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, newxml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ӳ�䱨��ͷʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
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
	iRc = iDealTagIn(locvars, stPkgInfo, pnode, pXml, newxml, pkg, len, iLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ����TAG��ǩʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	pub_log_info("[%s][%d] ��TAG��ǩ���! iCurLen=[%d]", __FILE__, __LINE__, *iLen);
	/*** ������ͷ����Ҫӳ��ı�������ӳ�� ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.ENDMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, newxml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ӳ��ҵ��ͷʧ��!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pub_log_info("[%s][%d] �����������!", __FILE__, __LINE__);
	pub_xml_deltree(pXml);
	pXml = NULL;

	return 0;
}

/****************************************************
* �� �� ��:	iDealBasHeadIn
* ��������:	��һ��CMT/PKG����ͷ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        ITEM�ڵ�
*          xml         XML��
*          pkg         ���Ļ�����
*          len         ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/	
int iDealBasHeadIn(locvars, node, xml, pkg, len, iLen)
sw_loc_vars_t	*locvars;
sw_xmlnode_t	*node;
sw_xmltree_t	*xml;
char	*pkg;
int	len;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sDate[9];
	long	lOrderno = 0L;
	
	ST_ITEM	stItem;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(sDate, 0x0, sizeof(sDate));
	iCurLen = *iLen;
	pub_log_info("[%s][%d] �ⱨ��ͷ֮ǰiCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	iInitItem(&stItem);
	pnode = node;
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}

		iInitItem(&stItem);
		xml->current = pnode;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stItem.cVarname, node1->value);
		
		node1 = pub_xml_locnode(xml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iVarlength = atoi(node1->value + 1);

		memcpy(stItem.cVarvalue, pkg + iCurLen, stItem.iVarlength);
		iCurLen += stItem.iVarlength;
		if (iCurLen > len)
		{
			pub_log_error("[%s][%d] ���ĳ�������! ����ͷ��=[%d] �����ܳ���=[%d]",
				__FILE__, __LINE__, iCurLen, len);
			return -1;
		}
		
		/*modify at xinyang*/
		
		if (memcmp(stItem.cVarname, "#MesgRefID", 10) == 0)
		{	
			lOrderno = 0L;
			memset(sDate, 0x0, sizeof(sDate));
			
			memcpy(sDate, stItem.cVarvalue, 8);
			lOrderno = atol(stItem.cVarvalue+8);
			
			memset(stItem.cVarvalue, 0x0, sizeof(stItem.cVarvalue));
			sprintf(stItem.cVarvalue, "%s%08ld", sDate, lOrderno);
		}
		if (locvars == NULL)
		{	
			
			set_zd_data(stItem.cVarname, stItem.cVarvalue);
		}
		else
		{	
			loc_set_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
		}
		pub_log_info("[%s][%d] [%s]=[%s]", __FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
		pnode = pnode->next;
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] �ⱨ��ͷ֮ǰiCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** ������ͷ�еĲ��ֱ�������ӳ�� ***/
	iBasHeadMapIn(locvars);
	pub_log_info("[%s][%d] ӳ�䱨��ͷ���!", __FILE__, __LINE__);
	iSetCheckVar(locvars, pkg);
	
	return 0;
}	

/****************************************************
* �� �� ��:	iBepsDealIn
* ��������:	��С���
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        ITEM�ڵ�
*          xml         XML��
*          newxml      �µ�XML��
*          pkg         ���Ļ�����
*          len         ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/	
int iBepsDealIn(locvars, stPkgInfo, node, xml, newxml, pkg, len)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmlnode_t	*node;
sw_xmltree_t	*xml;
sw_xmltree_t	*newxml;
char	*pkg;
int	len;
{
	int	iRc = 0;
	int	iCurLen = 0;
	size_t	iLen = 0;
	char	*ptr = NULL;
	char	*pTmp = NULL;
	char	*pDetail = NULL;
	char	sBlock[4];
	char	sSecondNo[21];
	
	memset(sBlock, 0x0, sizeof(sBlock));
	memset(sSecondNo, 0x0, sizeof(sSecondNo));
		
	/*** �ⱨ��ͷ ***/
	iRc = iDealBasHeadIn(locvars, node, xml, pkg, len, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] �ⱨ��ͷʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] �ⱨ��ͷ���! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	memcpy(sBlock, pkg + iCurLen, 3);
	if (strcmp(sBlock, "{P:") == 0)
	{
		/*** С��PKG���� ***/
		/*** ���ͷ ***/
		iRc = iPKGDealPkgHeadIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] ��PKG��ͷʧ��!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] ��PKG��ͷ���! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
		pub_log_info("[%s][%d] ��ͷ�������!", __FILE__, __LINE__);
		
		pub_log_info("[%s][%d] ����====[%s]", __FILE__, __LINE__, pkg + iCurLen);
		
		pTmp = pkg + iCurLen;
		ptr = strstr(pTmp, "{C:");
		if (ptr == NULL)
		{
			if (locvars == NULL)
			{
				iSetLargeVar("#detail", pkg + iCurLen, strlen(pkg + iCurLen));
			}
			else
			{
				iLocSetLargeVar(locvars, "#detail", pkg + iCurLen, strlen(pkg + iCurLen));
			}
		}
		else
		{
			iLen = ptr - pTmp;
			pub_log_info("[%s][%d] ��ϸ����====[%ld]\n", __FILE__, __LINE__, iLen);
			pDetail = calloc(1, iLen + 1);
			if (pDetail == NULL)
			{
				pub_log_error("[%s][%d] �����ڴ�ʧ��! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			memcpy(pDetail, pTmp, iLen);
			
			/***
			pub_log_info( "[%s][%d] LEN=[%d] ��ϸ====[%s]", __FILE__, __LINE__, iLen, pDetail);
			***/
			
			if (locvars == NULL)
			{
				iSetLargeVar("#detail", pDetail, iLen);
			}
			else
			{
				pub_log_info("[%s][%d] iLen====[%d]", __FILE__, __LINE__, iLen);
				iLocSetLargeVar(locvars, "#detail", pDetail, iLen);
			}
			pub_log_info("[%s][%d] ��ϸ====[%s]", __FILE__, __LINE__, pDetail);
			free(pDetail);
			pDetail = NULL;
		}
		
		/*** �����  ҵ��ͷ(ҵ��Ҫ�ؼ���)+������(ҵ�����ͺ�+����ҵ��Ҫ��+������) ***/
		iRc = iPKGDealBodyIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] �����ʧ��!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] ������!", __FILE__, __LINE__); 
		
		/*** ���ñ��ĸ�ʽ���� ***/
		if (locvars == NULL)
		{
			set_zd_data("#StructType", "PKG");
		}
		else
		{
			loc_set_zd_data(locvars, "#StructType", "PKG");
		}
	}
	else if (strcmp(sBlock, "{3:") == 0)
	{
		/*** С��CMT���� ***/
		/*** ֱ�ӽ�������,С��CMT������ҵ��ͷ ***/
		iRc = iCMTDealTradeBodyIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] ��������ʧ��!", __FILE__, __LINE__);
			return -1;
		}
		
		/*** ���ñ��ĸ�ʽ���� ***/
		if (locvars == NULL)
		{
			set_zd_data("#StructType", "CMT");
		}
		else
		{
			loc_set_zd_data(locvars, "#StructType", "CMT");
		}
	}
	else
	{
		pub_log_error("[%s][%d] ���Ŀ��ʶ����, sBlock=[%s]", 
			__FILE__, __LINE__, sBlock);
		return -1;
	}
	
	/*** �Զ������ı�Ž������⴦�� ***/
	memset(sSecondNo, 0x0, sizeof(sSecondNo));
	iRc = iGet2ndPkgNo(pkg, stPkgInfo.sPkgNo, sSecondNo);
	if (iRc == -1)
	{
		if (strcmp(stPkgInfo.sPkgNo, "007") == 0 || strcmp(stPkgInfo.sPkgNo, "121_007") == 0)
		{
			strcpy(sSecondNo, "beps.121.001.01");
			pub_log_info("[%s][%d] С���˻�ҵ��!", 
				__FILE__, __LINE__);
		}
		else
		{
			pub_log_error("[%s][%d] ȡ�������ı�ų���! һ�����ı��=[%s]",
				__FILE__, __LINE__, stPkgInfo.sPkgNo);
			return -1;
		}
	}
	if (locvars == NULL)
	{
		set_zd_data("#MesgType", sSecondNo);
	}
	else
	{
		loc_set_zd_data(locvars, "#MesgType", sSecondNo);
	}
	pub_log_info("[%s][%d] �������ı��=[%s]", __FILE__, __LINE__, sSecondNo);
	
	return 0;
}

/****************************************************
* �� �� ��:	iCMTDealTradeHeaderIn
* ��������:	��һ��CMT����ҵ��ͷ
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-16
* ��    ��:
*          locvars �����ػ�����
*          pkg       ���Ļ�����(��������ͷ)
*          iPkgLen   ���ĳ���(��������ͷ)
* ��    ��:
*          iLen      ҵ��ͷ����
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
****************************************************/
int iCMTDealTradeHeaderIn(locvars, pkg, len, iLen)
sw_loc_vars_t *locvars;
char *pkg;
int len;
int	*iLen;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sXmlName[512];
	ST_ITEM	stItem;
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] ��ҵ��ͷ֮ǰ iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	iInitItem(&stItem);
	
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
	
	/*** ���ȸ���cmt����ͷxml�ļ���������ͷ ***/
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] ��xml��ʧ��, ����xml�ļ�[%s]�Ƿ����"
			"���߸�ʽ�Ƿ���ȷ!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	pnode = pub_xml_locnode(pXml, ".CMTHEADER.ITEM");
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
		iInitItem(&stItem);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����NAME��ǩ!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		strcpy(stItem.cVarname, node1->value);
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] δ����TYPE��ǩ!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iVarlength = atoi(node1->value + 1);

		memcpy(stItem.cVarvalue, pkg + iCurLen, stItem.iVarlength);
		iCurLen += stItem.iVarlength;
		if (iCurLen > len)
		{	
			pub_log_error("[%s][%d] ���ĳ�������! iCurLen=[%d] len=[%d]",
				__FILE__, __LINE__, iCurLen, len);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
		
		if (locvars == NULL)
		{
			set_zd_data(stItem.cVarname, stItem.cVarvalue);
		}
		else
		{
			loc_set_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
		}
		pub_log_info("[%s][%d] �������[%s]=[%s]", 
			__FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
		pnode = pnode->next;
	}
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] ����ҵ��ͷ֮�� iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* �� �� ��:	iHvpsDealIn
* ��������:	�����
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          node        ITEM�ڵ�
*          xml         XML��
*          newxml      �µ�XML��
*          pkg         ���Ļ�����
*          len         ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iHvpsDealIn(locvars, stPkgInfo, node, xml, newxml, pkg, len)
sw_loc_vars_t *locvars;
ST_PKGINFO	stPkgInfo;
sw_xmlnode_t	*node;
sw_xmltree_t	*xml;
sw_xmltree_t	*newxml;
char	*pkg;
int	len;
{
	int	iRc = 0;
	int	iCurLen = 0;
	char	sSecondNo[21];
	
	memset(sSecondNo, 0x0, sizeof(sSecondNo));
	
	/*** �ⱨ��ͷ ***/
	iRc = iDealBasHeadIn(locvars, node, xml, pkg, len, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] �ⱨ��ͷʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] �ⱨ��ͷ���! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (strncmp(pkg + iCurLen, "{2:", 3) == 0)
	{
		pub_log_info("[%s][%d] �ô��Ĵ���ҵ��ͷ,��ʼ��ҵ��ͷ!", __FILE__, __LINE__);
		/*** ��ҵ��ͷ***/
		iRc = iCMTDealTradeHeaderIn(locvars, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] ��ҵ��ͷʧ��!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] ��ҵ��ͷ���! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	}
	else if (strncmp(pkg + iCurLen, "{3:", 3) != 0)
	{
		pub_log_error("[%s][%d] �����忪ʼ�������! ������=[%s]",
			__FILE__, __LINE__, pkg + iCurLen);
		return -1;
	}
	
	/*** �������� ***/
	iRc = iCMTDealTradeBodyIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] ��������ʧ��!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] �����������! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** �Զ������ı�Ž������⴦�� ***/
	memset(sSecondNo, 0x0, sizeof(sSecondNo));
	iRc = iGet2ndPkgNo(pkg, stPkgInfo.sPkgNo, sSecondNo);
	if (iRc == -1)
	{
		if (strcmp(stPkgInfo.sPkgNo, "108") == 0 || strcmp(stPkgInfo.sPkgNo, "111_108") == 0)
		{
                        strcpy(sSecondNo, "hvps.111.001.01");
                        pub_log_info("[%s][%d] ����˻�ҵ��!", 
				__FILE__, __LINE__);
                }
                else
                {
			pub_log_error("[%s][%d] ȡ�������ı�ų���! һ�����ı��=[%s]",
				__FILE__, __LINE__, stPkgInfo.sPkgNo);
			return -1;
                }	
	}
	if (locvars == NULL)
	{
		set_zd_data("#MesgType", sSecondNo);
	}
	else
	{
		loc_set_zd_data(locvars, "#MesgType", sSecondNo);
	}
	pub_log_info("[%s][%d] �������ı��=[%s]", __FILE__, __LINE__, sSecondNo);
	
	/*** ���ñ��ĸ�ʽ���� ***/
	if (locvars == NULL)
	{
		set_zd_data("#StructType", "CMT");
	}
	else
	{
		loc_set_zd_data(locvars, "#StructType", "CMT");
	}
	
	return 0;
}

/****************************************************
* �� �� ��:	iPkgToSwitchPkgCmt
* ��������:	��һ�����ĵ���������
* ��    ��:	MaWeiwei
* ��    ��:	2011-11-24
* �������:
*		   locvars   �����ػ�����
*          pkg         ���Ļ�����
*          xmlname     xml�ļ���
*          len         ���ĳ���
* �������:
*          ��
* �� �� ֵ:  	0 �ɹ�, ��1 ʧ��
*****************************************************/
int iPkgToSwitchPkgCmt(sw_loc_vars_t *locvars, char *pkg, char *xmlname, int iLen)
{
	int	iRc = 0;
	int	iPkgLen = 0;
	int	iMySelf = 0;
	char	*ptr = NULL;
	char	*psBuf = NULL;
	char	sPkgType[32];
	char	sXmlName[512];
	char	sPkgLibName[64];
	char	sPkgFunName[64];
	char	sRespcdName[32];
	sw_xmltree_t	*pXml = NULL;
	sw_xmltree_t	*pNewXml = NULL;
	sw_xmlnode_t	*proot = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	ST_PKGINFO	stPkgInfo;
	ST_FIRST	stFirstwork;
	ST_ITEM	stItem;
	
	memset(sPkgType, 0x0, sizeof(sPkgType));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	memset(sPkgLibName, 0x0, sizeof(sPkgLibName));
	memset(sPkgFunName, 0x0, sizeof(sPkgFunName));
	memset(sRespcdName, 0x0, sizeof(sRespcdName));
	memset(&stPkgInfo, 0x0, sizeof(stPkgInfo));
	memset(&stFirstwork, 0x0, sizeof(stFirstwork));
	iInitItem(&stItem);
	
	/*** Ϊҵ����һ����ʶ ***/
	if (locvars == NULL)
	{
		set_zd_data("$pkgtype", "1");
	}
	else
	{
		loc_set_zd_data(locvars, "$pkgtype", "1");
	}
	
	/*** ���Ĵ��䷽�� ***/
	if (locvars == NULL)
	{
		set_zd_data("#MesgDirection", "D");
	}
	else
	{
		loc_set_zd_data(locvars, "#MesgDirection", "D");
	}
	
	/*** Ϊ���������ñ��� ***/
	if (locvars == NULL)
	{
		set_zd_data("#CURNO", "CNY");
	}
	else
	{
		loc_set_zd_data(locvars, "#CURNO", "CNY");
	}
	
	psBuf = pkg;
	iRc = iInitPkgIn(psBuf, &stPkgInfo);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] �����ʼ������!", __FILE__, __LINE__);
		return -1;
	}
	/*** Ϊ�µ�XML��������ڵ� ***/
	proot = (sw_xmlnode_t *)calloc(1, sizeof(sw_xmlnode_t));
	if (proot == NULL)
	{
		pub_log_error("[%s][%d] �����ڴ�ʧ��! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	s_initxmlnode(proot);
	proot->name = (char *)calloc(1, strlen("ROOT") + 1);
	if (proot->name == NULL)
	{
		pub_log_error("[%s][%d] �����ڴ�ʧ��! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	strcpy(proot->name, "ROOT");
	
	proot->value = (char *)calloc(1, strlen("ROOT") + 1);
	if (proot->value == NULL)
	{
		pub_log_error("[%s][%d] �����ڴ�ʧ��! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	strcpy(proot->value, "ROOT");
	proot->node_type = SW_NODE_ROOT;
	
	pNewXml = (sw_xmltree_t *)calloc(1, sizeof(sw_xmltree_t));
	if (pNewXml == NULL)
	{
		pub_log_error("[%s][%d] �����ڴ�ʧ��! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	pNewXml->current = proot;
	pNewXml->root = proot;
	
	if (getenv("SWWORK") != NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/%s", getenv("HOME"), xmlname);
	}
	pub_log_info("[%s][%d] �����ļ�==[%s]", 
		__FILE__, __LINE__, sXmlName);
	
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
	pub_log_info("[%s][%d] pkgtype=[%s]", __FILE__, __LINE__, sPkgType);
	
	pnode = pub_xml_locnode(pXml, ".CBM.ANALYZE");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ�ҵ�[.CBM.ANALYZE]��ǩ!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pXml->current = pnode;
	pnode = pstGetNodeByTarget(pXml, "ANALYZE", "PKGTYPE", "CMT");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] δ�ҵ�[CMT]���ñ�ǩ!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}

	/*** ��ʼ����firstwork ***/
	pXml->current = pnode;
	pnode = pub_xml_locnode(pXml, "FIRSTWORK");
	if (pnode != NULL)
	{
		pXml->current = pnode;
		pnode = pub_xml_locnode(pXml, "LENGTH");
		if (pnode != NULL)
		{	
			iPkgLen = nGetpkgloc(pnode->value);
		}
		pnode = pub_xml_locnode(pXml, "MYSELF");
		if (pnode != NULL)
		{
			if (strcmp(pnode->value, "TRUE") != 0)
			{
				iMySelf = 0;
			}
		}
	}
	
	/*** ��ʼ����firstwork.item ***/
	pnode = pub_xml_locnode(pXml, "FIRSTWORK.ITEM");
	if (pnode != NULL)
	{
		pub_log_debug("[%s][%d] ���ڴ���FIRSTWORK.ITEM!",
			__FILE__, __LINE__);
		pXml->current = pnode;
		pnode = pub_xml_locnode(pXml, "NAME");
		if (pnode != NULL)
		{
			memcpy(stItem.cVarname, pnode->value, sizeof(stItem.cVarname) - 1);
		}
		
		pnode = pub_xml_locnode(pXml, "TYPE");
		if (pnode != NULL)
		{
			stItem.cType = pnode->value[0];
		}
		
		pnode = pub_xml_locnode(pXml, "BEGIN");
		if (pnode != NULL)
		{
			stItem.iBegin = atoi(pnode->value);
		}
		
		pnode = pub_xml_locnode(pXml, "LENGTH");
		if (pnode != NULL)
		{
			stItem.iVarlength = atoi(pnode->value);
		}
		
		pnode = pub_xml_locnode(pXml, "FILEFLAG");
		if (pnode != NULL)
		{
			if (locvars == NULL)
			{
				set_zd_data("$FILEFLAG", pnode->value);
			}
			else 
			{
				loc_set_zd_data(locvars, "$FILEFLAG", pnode->value);
			}
		}
		
		memcpy(stItem.cVarvalue, pkg + stItem.iBegin, stItem.iVarlength);
		if (locvars == NULL)
		{
			set_variable(stItem.cVarname, stItem.cType, 
				stItem.cVarvalue, stItem.iVarlength);
		}
		else
		{
			locvars->set_var(locvars, stItem.cVarname, stItem.cType, 
				stItem.cVarvalue, stItem.iVarlength);
		}
		memcpy(stFirstwork.cVarvalue, stItem.cVarvalue, 
			sizeof(stFirstwork.cVarvalue) - 1);
		memcpy(stFirstwork.cVarname, stItem.cVarname, 
			sizeof(stFirstwork.cVarname) - 1);
	}
	
	/*** ��ʼ����PACKAGE ***/
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
			pub_log_error("[%s][%d] ����PACKAGEδ����RESPCD��! �ͷ�xml", 
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
			if (iLocChkinclude(locvars, stFirstwork.cVarname, node2->value) != 0)
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
			if (iLocCheck(locvars, node2->value) != 0)
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
		/****���������INCLUDE��CHECK���������������Ĭ�ϰ�****/
		pnode = pub_xml_locnode(pXml, "ITEM");
		pXml->current = pnode;
		if (stPkgInfo.cPkgType == '1')
		{
			/*** ���(CMT)���� ***/
			iRc = iHvpsDealIn(locvars, stPkgInfo, pnode, pXml, pNewXml, psBuf, iLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] ����ĳ���!", __FILE__, __LINE__);
				pub_xml_deltree(pXml);
				pXml = NULL;
				return -1;
			}
		}
		else 
		{
			/*** С��� ***/
			iRc = iBepsDealIn(locvars, stPkgInfo, pnode, pXml, pNewXml, psBuf, iLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] ��С��ĳ���!", __FILE__, __LINE__);
				pub_xml_deltree(pXml);
				pXml = NULL;
				return -1;
			}
		}
		pNewXml->current = pNewXml->root;
		if (locvars == NULL)
		{
			pub_vars_reset_tree(pNewXml);
		}
		else
		{
			locvars->tree = pNewXml;
		}
		pub_xml_deltree(pXml);
		pub_log_info("[%s][%d] ������! �ͷ�xml", __FILE__, __LINE__);
		return 0;
	}
	pub_xml_deltree(pXml);
	pXml = NULL;
	pub_log_error("[%s][%d] ���ش���:δ�ҵ��������!", __FILE__, __LINE__);

	return -1;
}
