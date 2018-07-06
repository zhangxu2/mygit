#include "packcmt.h"
/******************************************************
* 函 数 名:	iInitPkgIn
* 函数功能:	解包初始化
* 作    者:	MaWeiwei
* 日    期:	2011-11-26
* 输入参数:
*           pkg    报文缓冲区 
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 业务交易码=[%s]", __FILE__, __LINE__, sAppTradeCode);
	memcpy(sCmtNo, sAppTradeCode + 1, 3);
	pub_log_info("[%s][%d] 报文编号=[%s]", __FILE__, __LINE__, sCmtNo);
	
	iRc = iGet2ndPkgNo(pkg, sCmtNo, sPkgNo);
	if (iRc == -1)
	{
		if (strcmp(sCmtNo, "108") == 0)
		{
			strcpy(sPkgNo, "hvps.111.001.01");
			pub_log_info("[%s][%d] 大额退汇业务!", __FILE__, __LINE__);
		}
		else if (strcmp(sCmtNo, "007") == 0)
		{
			strcpy(sPkgNo, "beps.121.001.01");
			pub_log_info("[%s][%d] 小额退汇业务!", __FILE__, __LINE__);
		}
		else
		{
			pub_log_error("[%s][%d] 取[%s]对应二代报文标识号出错!",
				__FILE__, __LINE__, sCmtNo);
			return -1;
		}
	}
	pub_log_info("[%s][%d] 二代报文标识号=[%s]", __FILE__, __LINE__, sPkgNo);
	memcpy(pstPkgInfo->sCmtNo, sPkgNo + 5, 3);
	memset(pstPkgInfo->sPkgNo, 0x0, sizeof(pstPkgInfo->sPkgNo));
	memcpy(pstPkgInfo->sPkgNo, sPkgNo + 5, 3);
	strcat(pstPkgInfo->sPkgNo, "_");
	memcpy(pstPkgInfo->sPkgNo + 4, sAppTradeCode + 1, 3);
	pub_log_info("[%s][%d] 报文号码=[%s]", __FILE__, __LINE__, pstPkgInfo->sPkgNo); 
	
	if (sAppTradeCode[0] == '1' || sAppTradeCode[0] == 'H')
	{
		/*** 大额(CMT)报文 ***/
		pstPkgInfo->cPkgType = '1';
	}
	else if (sAppTradeCode[0] == '2')
	{
		/*** 小额报文(具体是CMT还是PKG在解完报文头后确定) ***/
		pstPkgInfo->cPkgType = '2';
	}
	else
	{
		pub_log_error("[%s][%d] 业务交易码格式不正确, AppTradeCode=[%s]",
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
		pub_log_info("[%s][%d] [%s]不存在,不进行处理!", __FILE__, __LINE__, sXmlName);
		return 0;
	}
	
	memset(sCmtNo, 0x0, sizeof(sCmtNo));
	strncpy(sCmtNo, pkg + 11, 3);
	pub_log_info("[%s][%d] 一代报文编号=[%s]", __FILE__, __LINE__, sCmtNo);

	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 根据xml文件[%s]建树失败!", __FILE__, __LINE__, sXmlName);
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
		pub_log_info("[%s][%d] 未配置[%s]的信息!", __FILE__, __LINE__, sCmtNo);
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
			pub_log_error("[%s][%d] 未配置TAG标签!", __FILE__, __LINE__);
			pNode = pNode->next;
			continue;
		}
		sprintf(sTag, ":%s:", pNode1->value);

		pNode1 = pub_xml_locnode(pXml, "NAME");
		if (pNode1 == NULL || pNode1->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
			pNode = pNode->next;
			continue;
		}
		strncpy(sName, pNode1->value, sizeof(sName) - 1);
		ptr = strstr(pkg, sTag);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] 报文未组[%s]标签!", __FILE__, __LINE__, sTag);
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
		pub_log_info("[%s][%d] 拆出变量:[%s]===[%s]===[%s]", __FILE__, __LINE__, sTag, sName, sValue);
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
* 函 数 名:	iBasHeadMapIn
* 函数功能:	报文头映射
* 作    者:	MaWeiwei
* 日    期:	2011-11-26
* 输入参数:
*           locvars    变量池缓冲区 
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
		pub_log_info("[%s][%d] 业务交易码$appTradeCode=[%s]", __FILE__, __LINE__, sAppTradeCode);
		if (sAppTradeCode[0] == '1' || sAppTradeCode[0] == 'H')
		{
			/*** 大额 ***/
			/*** 发送系统号 ***/
			set_zd_data("#OrigSenderSID", "HVPS");
			/*** 接收系统号 ***/
			set_zd_data("#OrigReceiverSID", "HVPS");
		}
		else if (sAppTradeCode[0] == '2')
		{
			/*** 小额 ***/
			/*** 发送系统号 ***/
			set_zd_data("#OrigSenderSID", "BEPS");
			/*** 接收系统号 ***/
			set_zd_data("#OrigReceiverSID", "BEPS");
		}
		
		memset(sTmp, 0x0, sizeof(sTmp));
		memset(sSendTime, 0x0, sizeof(sSendTime));
		/*** 报文发送时间 ***/
		get_zd_data("#OrigSendTime", sTmp);
		pub_log_info("[%s][%d] 报文发送时间#OrigSendTime=[%s]", __FILE__, __LINE__, sTmp);
		memcpy(sSendTime, sTmp + 8, sizeof(sSendTime) - 1);
		set_zd_data("#OrigSendTime", sSendTime);
		
		/*** 设置时间格式HH:MM:SS ***/
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
			/*** 大额 ***/
			/*** 发送系统号 ***/
			loc_set_zd_data(locvars, "#OrigSenderSID", "HVPS");
			/*** 接收系统号 ***/
			loc_set_zd_data(locvars, "#OrigReceiverSID", "HVPS");
		}
		else if (sAppTradeCode[0] == '2')
		{
			/*** 小额 ***/
			/*** 发送系统号 ***/
			loc_set_zd_data(locvars, "#OrigSenderSID", "BEPS");
			/*** 接收系统号 ***/
			loc_set_zd_data(locvars, "#OrigReceiverSID", "BEPS");
		}
		
		memset(sTmp, 0x0, sizeof(sTmp));
		memset(sSendTime, 0x0, sizeof(sSendTime));
		/*** 报文发送时间 ***/
		loc_get_zd_data(locvars, "#OrigSendTime", sTmp);
		pub_log_info("[%s][%d] 报文发送时间#OrigSendTime=[%s]", __FILE__, __LINE__, sTmp);
		memcpy(sSendTime, sTmp + 8, sizeof(sSendTime) - 1);
		loc_set_zd_data(locvars, "#OrigSendTime", sSendTime);
		
		/*** 设置时间格式HH:MM:SS ***/
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
* 函 数 名:	iMapHeaderIn
* 函数功能:	将报文头需要映射的变量映射到新的XML树中
* 作    者:	MaWeiwei
* 日    期:	2011-11-16
* 输入参数:
*          locvars   变量池缓冲区
*          pXml        XML树
*          xml         新的XML树
*          node        ITEM节点
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	
	pub_log_info("[%s][%d] 开始进行报文头变量映射!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 未配置标签NAME!", 
				__FILE__, __LINE__);
			return -1;
		}
		memcpy(stItem.cVarname, node1->value, sizeof(stItem.cVarname) - 1);
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置标签TYPE!", 
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
			pub_log_debug("[%s][%d] 未配置标签MAP,说明该变量不需要进行映射!", 
				__FILE__, __LINE__);
			pnode = pnode->next;
			continue;
		}
		
		/*** 当该变量需要映射时才将该变量取出 ***/
		if (locvars == NULL)
		{
			get_zd_data(stItem.cVarname, stItem.cVarvalue);
		}
		else
		{
			loc_get_zd_data(locvars, stItem.cVarname, stItem.cVarvalue);
		}
		pub_log_info("[%s][%d] 变量[%s]=[%s]", 
			__FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
		while (node1 != NULL)
		{
			initMap(&stMap);
			pXml->current = node1;
			node2 = pub_xml_locnode(pXml, "ALIAS");
			if (node2 == NULL)
			{
				pub_log_error("[%s][%d] 未配置标签ALIAS!", 
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
						
						/*** 某些刚SET的xml变量 ***/
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
						
						/*** 某些刚SET的xml变量 ***/
						if (strlen(sVarValue) == 0)
						{
							iGetXmlVariable(xml, node2->value, sVarValue);
						}
					}
				}
				
				pub_log_info("[%s][%d] CHECK变量===[%s] 值===[%s]",
					__FILE__, __LINE__, node2->value, sVarValue);
				node2 = pub_xml_locnode(pXml, "VALUE");
				if (node2 == NULL)
				{
					pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
						__FILE__, __LINE__);
					return -1;
				}
				strcpy(sCheckValue, node2->value);
				if (iCheckTx(sVarValue, sCheckValue) != 0)
				{
					pub_log_info("[%s][%d] sVarValue=[%s] sCheckValue=[%s] 检查未通过!",
						__FILE__, __LINE__, sVarValue, sCheckValue);
					node1 = node1->next;
					continue;
				}
			}

			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未配置标签BEGIN!",
					__FILE__, __LINE__);
				return -1;
			}
			stMap.iBegin = atoi(node2->value);
			
			node2 = pub_xml_locnode(pXml, "MAPBEGIN");
			if (node2 == NULL && stItem.cFlag == '1')
			{	
				pub_log_error("[%s][%d] 指定为多对一映射但是没指定MAPBEGIN!",
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
				pub_log_error("[%s][%d] 未配置标签LENGTH!",
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
				/*** 多对一映射 ***/
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
				pub_log_info("[%s][%d] 取出多对一变量=[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
				memcpy(stMap.sVarvalue + stMap.iMapBegin,
					stItem.cVarvalue + stMap.iBegin,
					stMap.iLength);
				pub_log_info("[%s][%d] 多对一变量=[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{	
				/*** 一对多 ***/
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
					pub_log_error("[%s][%d] 八位日期转十位日期出错! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** 如果变量需要映射,先映射 ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
							__FILE__, __LINE__, stPkgInfo.sPkgNo, stMap.sVarvalue, stMap.cMapFlag);
						return -1;
					}
					strcpy(stMap.sVarvalue, sSecondVar);
				}
			}
			
			if (stMap.cNodeType == '1')
			{			
				/*** 映射为XML属性 ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 为节点[%s]设置属性失败!",
						__FILE__, __LINE__, stMap.sAlias);
					pub_xml_deltree(pXml);
					pXml = NULL;
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** 映射为XML节点 ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 设置节点失败!",
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
			pub_log_info("[%s][%d] 映射变量[%s]=[%s]", __FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			node1 = node1->next;
		}
		pnode = pnode->next;
	}
	
	return 0;
}

/****************************************************
* 函 数 名:	pGetNodeByTag
* 函数功能:	根据TAG值得到TAG节点
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   node    TAG标签初始节点
*          pTag    TAG标签值
* 输出参数:
*          TAG值对应的TAG节点
* 返 回 值:  	0 成功, －1 失败
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
* 函 数 名:	iDealTagIn
* 函数功能:	解TAG标签
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        TAG节点
*          pXml        XML树
*          xml         新的xml树 
*          pkg         报文缓冲区
*          len        报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆TAG标签之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	initMap(&stMap);
	initTag(&stTagNode);
	
	while (1)
	{
		if (memcmp(pkg + iCurLen, "}", 1) == 0)
		{
			iCurLen += 1;
			pub_log_info("[%s][%d] 拆TAG标签结束!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 标签位置有误!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] TAG=[%s]", __FILE__, __LINE__, sTag);
		
		pnode = pGetNodeByTag(node, sTag);
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] TAG标签[%s]未在配置文件中出现!", __FILE__, __LINE__, sTag);
			return -1;
		}

		initTag(&stTagNode);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 未配置TYPE标签!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 报文格式异常! pTmp=[%s]", __FILE__, __LINE__, pTmp);
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
					pub_log_error("[%s][%d] 报文有误!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] 报文长度有误! iCurLen=[%d] len=[%d]",
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
					
					/*** 某些刚SET的xml变量 ***/
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
					
					/*** 某些刚SET的xml变量 ***/
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, node1->value, sVarValue);
					}
				}
			}
			strcpy(stTagNode.sCheckVar, sVarValue);
			
			pub_log_info("[%s][%d] CHECK变量===[%s] 值===[%s]",
				__FILE__, __LINE__, node1->value, sVarValue);
			node1 = pub_xml_locnode(pXml, "VALUE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] 检查未通过!",
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
		
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
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
				pub_log_error("[%s][%d] 未配置ALIAS标签!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] 未配置BEGIN标签!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]指定FLAG=1为多对一映射,但是[%s]没指定MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未指定LENGTH标签!", __FILE__, __LINE__);
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
					pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
						__FILE__, __LINE__);
					return -1;
				}
				strcpy(stTagNode.sCheckValue, node2->value);
				if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
				{
					pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] 检查未通过!",
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
				/*** 多对一映射 ***/
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
				
				pub_log_info("[%s][%d] 取出多对一变量[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
				memcpy(stMap.sVarvalue + stMap.iMapBegin, 
					stTagNode.sTagValue + stMap.iBegin, 
					stMap.iLength);
				pub_log_info("[%s][%d] 多对一变量[%s]=[%s]",
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else
			{
				/*** 一对多映射 ***/
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
					pub_log_error("[%s][%d] 八位日期转十位日期出错! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** 如果变量需要映射,先映射 ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
				/*** 映射为XML属性 ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 为节点[%s]设置属性失败!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
				pub_log_info("[%s][%d] 属性节点=[%s]=[%s]", 
					__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** 映射为XML节点 ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 设置节点[%s]失败!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
				pub_log_info("[%s][%d] 子节点变量[%s]=[%s]", 
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
			pub_log_info("[%s][%d] 变量[%s]=[%s]", 
				__FILE__, __LINE__, stMap.sAlias, stMap.sVarvalue);
			node1 = node1->next;
		}
	}
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] 拆完TAG标签之后 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* 函 数 名:	iPKGDealTagIn
* 函数功能:	解一代PKG循环业务体TAG
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        TAG标签节点
*          pXml        xml树
*          xml         新的xml树
*          pkg         报文缓冲区
*          len         报文长度
*          index       循环次数
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆PKG正文体之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	initMap(&stMap);
	initTag(&stTagNode);
	
	while (1)
	{
		if (memcmp(pkg + iCurLen, "}", 1) == 0)
		{
			iCurLen += 1;
			pub_log_info("[%s][%d] 拆TAG标签结束!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 标签位置有误! pkg=[%s]", 
				__FILE__, __LINE__, pkg + iCurLen);
			pub_log_bin(SW_LOG_DEBUG, pkg + iCurLen, len - iCurLen, "[%s][%d] 报文长度[%d]\n",
				__FILE__, __LINE__, len - iCurLen);
			return -1;
		}
		pub_log_info("[%s][%d] TAG=[%s]", __FILE__, __LINE__, sTag);
		
		
		if (strcmp(sTag, "72C") == 0)
		{
			/*** 附加域 ***/
			/*** 取业务类型号,得到附加域格式 ***/
			pSaveNode = pXml->current;
			memset(sTxType, 0x0, sizeof(sTxType));
			get_zd_data("$txtype", sTxType);
			pub_log_info("[%s][%d] 业务类型号=[%s]", __FILE__, __LINE__, sTxType);
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
					pub_log_error("[%s][%d] 附加域未配置TXTYPE!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] 没有找到匹配的业务类型配置!", 
					__FILE__, __LINE__);
				/*** 跳过:72C: ***/
				iCurLen += 5;
				while (pkg[iCurLen] != '\0')
				{
					if (pkg[iCurLen] == '}' || (pkg[iCurLen] == ':' && pkg[iCurLen + 4] == ':'))
					{
						break;
					}
					iCurLen++;
				}
				pub_log_info("[%s][%d] 72C内容被丢弃!", __FILE__, __LINE__);
				pXml->current = pSaveNode;
				continue;
			}
			/*** 拆附加域 ***/
			pXml->current = node1;
			node1 = pub_xml_locnode(pXml, "TAG");
			iRc = iPKGDealAppendTagIn(locvars, stPkgInfo, node1, pXml, xml, pkg, len, index, &iCurLen);
			if (iRc == -1)
			{
				pub_log_error("[%s][%d] 处理TAG标签失败!", __FILE__, __LINE__);
				return -1;
			}
			pXml->current = pSaveNode;
			continue;
		}	
		pnode = pGetNodeByTag(node, sTag);
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] TAG标签[%s]未在配置文件中出现!", __FILE__, __LINE__, sTag);
			return -1;
		}

		initTag(&stTagNode);
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 未配置TYPE标签!", __FILE__, __LINE__);
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
			/*** 收款人/付款人清单 ***/
			iCurLen += 5;
			pSaveNode = pXml->current;
			for (i = 0; i < iNum; i++)
			{
				node1 = pub_xml_locnode(pXml, ".LIST.TAG");
				if (node1 == NULL)
				{
					pub_log_error("[%s][%d] 配置文件中指定了清单标识，但是"
						"没有配置清单列表[LIST.TAG]!", __FILE__, __LINE__);
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
					pub_log_error("[%s][%d] 处理TAG标签失败!", __FILE__, __LINE__);
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
			pub_log_error("[%s][%d] 报文格式异常! pTmp=[%s]", __FILE__, __LINE__, pTmp);
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
			pub_log_error("[%s][%d] 报文长度有误! iCurLen=[%d] len=[%d]",
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
				pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] 检查未通过!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				continue;
			}
		}
		
		if (stTagNode.cNum == '1')
		{
			iNum = atoi(stTagNode.sTagValue);
			pub_log_info("[%s][%d] 收款人/付款人数目=[%d]",
				__FILE__, __LINE__, iNum);
		}
		
		if (strlen(stTagNode.sAlias) == 0)
		{
			sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
		}
		
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
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
				pub_log_error("[%s][%d] 未配置ALIAS标签!", __FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, index);
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未配置BEGIN标签!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]指定FLAG=1为多对一映射,但是[%s]没指定MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未指定LENGTH标签!", __FILE__, __LINE__);
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
				/*** 多对一映射 ***/
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
				/*** 如果多对一变量可能会出现前面变量组在后面的情况,所以需要把前面的长度部分先补齐 ***/
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
				/*** 一对多映射 ***/
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
					pub_log_error("[%s][%d] 八位日期转十位日期出错! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** 如果变量需要映射,先映射 ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
				/*** 映射为XML属性 ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 为节点[%s]设置属性失败!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** 映射为XML节点 ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 设置节点[%s]失败!",
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
	pub_log_info("[%s][%d] 拆完附加域之后 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* 函 数 名:	iPKGDealAppendTagIn
* 函数功能:	解一代PKG业务体附加域
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        TAG标签节点
*          pXml        xml树
*          xml         新的xml树
*          pkg         报文缓冲区
*          len         报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆附加域之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);

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
			pub_log_error("[%s][%d] 未配置TYPE标签!", __FILE__, __LINE__);
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
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] 变量类型有误! VAR=[%s]",
						__FILE__, __LINE__, node1->value);
					return -1;
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] 变量类型有误! VAR=[%s]",
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
			pub_log_info("[%s][%d] 变量[%s]超长,进行动态分配!",
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
			pub_log_error("[%s][%d] 报文长度有误! iCurLen=[%d] len=[%d]",
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
				pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
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
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] 检查未通过!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				if (iFlag == 1)
				{
					free(ptr);
				}
				continue;
			}
		}
		
		/*** 附加域中的明细条数 ***/
		node1 = pub_xml_locnode(pXml, "NUM");
		if (node1 != NULL)
		{
			stTagNode.cNum = '1';
		}
		
		if (stTagNode.cNum == '1')
		{
			iAppendNum = atoi(ptr);
			pub_log_info("[%s][%d] 附加域明细条数=[%d]",
				__FILE__, __LINE__, iAppendNum);
		}
		
		/*** 附加域中的明细清单 ***/
		node1 = pub_xml_locnode(pXml, "LIST");
		if (node1 != NULL)
		{
			pSaveNode = pXml->current;
			memset(sTxType, 0x0, sizeof(sTxType));
			get_zd_data("$txtype", sTxType);
			pub_log_info("[%s][%d] 业务类型号=[%s]", __FILE__, __LINE__, sTxType);
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
					pub_log_error("[%s][%d] 附加域未配置TXTYPE!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] 没有找到匹配的附加域业务类型配置!", 
					__FILE__, __LINE__);
				if (iFlag == 1)
				{
					free(ptr);
				}
				return -1;
			}
			/*** 拆附加域中的明细清单 ***/
			pXml->current = node1;
			node1 = pub_xml_locnode(pXml, "TAG");
			for (i = 0; i < iAppendNum; i++)
			{
				iRc = iPKGDealListIn(locvars, stPkgInfo, node1, pXml, xml, pkg, len, index, i, &iCurLen);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 处理附加域中的明细清单失败!", __FILE__, __LINE__);
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
		
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
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
				pub_log_error("[%s][%d] 未配置ALIAS标签!", __FILE__, __LINE__);
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
					pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
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
					pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] 检查未通过!",
						__FILE__, __LINE__, stMap.sCheckVar, stMap.sCheckValue);
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未配置BEGIN标签!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]指定FLAG=1为多对一映射,但是[%s]没指定MAPBEGIN!",
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
				pub_log_error("[%s][%d] 未指定LENGTH标签!", __FILE__, __LINE__);
				if (iFlag == 1)
				{
					free(ptr);
				}
				return -1;
			}
			/*** 处理变长长度 ***/
			memset(sVarValue, 0x0, sizeof(sVarValue));
			if (locvars == NULL)
			{
				memset(sPath, 0x0, sizeof(sPath));
				sprintf(sPath, node2->value, index);
				if (node2->value[0] == '#' || node2->value[0] == '$')
				{
					get_zd_data(sPath, sVarValue);
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
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
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
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
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
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
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
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
				/*** 多对一映射 ***/
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
				/*** 一对多映射 ***/
				memcpy(pValue, ptr + stMap.iBegin, stMap.iLength);
			}
			
			if (stMap.cDate == '1')
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				iRc = iDate8To10(stMap.sVarvalue, sVarValue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 八位日期转十位日期出错! date=[%s]",
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
			
			/*** 如果变量需要映射,先映射 ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
				/*** 映射为XML属性 ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 为节点[%s]设置属性失败!",
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
				/*** 映射为XML节点 ***/
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
					pub_log_error("[%s][%d] 设置节点[%s]失败!",
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
	pub_log_info("[%s][%d] 拆完附加域之后 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* 函 数 名:	iPKGDealListIn
* 函数功能:	解一代PKG业务体中的收款人/付款人清单
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        TAG标签节点
*          pXml        xml树
*          xml         新的xml树
*          pkg         报文缓冲区
*          len         报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆附加域清单之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
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
			pub_log_error("[%s][%d] 未配置TYPE标签!", __FILE__, __LINE__);
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
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					get_zd_data(sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] 变量类型有误! VAR=[%s]",
						__FILE__, __LINE__, node1->value);
					return -1;
				}
			}
			else
			{
				if (node1->value[0] == '#' || node1->value[0] == '$')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else if (node1->value[0] == '.')
				{
					loc_get_zd_data(locvars, sPath, sVarValue);
					if (strlen(sVarValue) == 0)
					{
						iGetXmlVariable(xml, sPath, sVarValue);
					}
					
					pub_log_info("[%s][%d] 长度变量=[%s]=[%s]",
						__FILE__, __LINE__, sPath, sVarValue);
				}
				else
				{
					pub_log_error("[%s][%d] 变量类型有误! VAR=[%s]",
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
			pub_log_error("[%s][%d] 报文长度有误! iCurLen=[%d] len=[%d]",
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
				pub_log_error("[%s][%d] 指定了CHECK选项，但是没指定VALUE!",
					__FILE__, __LINE__);
				return -1;
			}
			strcpy(stTagNode.sCheckValue, node1->value);
			if (iCheckTx(stTagNode.sCheckVar, stTagNode.sCheckValue) != 0)
			{
				pub_log_info("[%s][%d] sCheckVar=[%s] sCheckValue=[%s] 检查未通过!",
					__FILE__, __LINE__, stTagNode.sCheckVar, stTagNode.sCheckValue);
				continue;
			}
		}
		
		if (strlen(stTagNode.sAlias) == 0)
		{
			sprintf(stTagNode.sAlias, "#PKG%s", stTagNode.sTag);
		}
		
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
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
				pub_log_error("[%s][%d] 未配置ALIAS标签!", __FILE__, __LINE__);
				return -1;
			}
			sprintf(stMap.sAlias, node2->value, indexi, indexj);
			
			node2 = pub_xml_locnode(pXml, "BEGIN");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未配置BEGIN标签!", __FILE__, __LINE__);
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
				pub_log_error("[%s][%d] [%s]指定FLAG=1为多对一映射,但是[%s]没指定MAPBEGIN!",
					__FILE__, __LINE__, stTagNode.sTag, stMap.sAlias);
				return -1;
			}
			
			node2 = pub_xml_locnode(pXml, "LENGTH");
			if (node2 == NULL)
			{	
				pub_log_error("[%s][%d] 未指定LENGTH标签!", __FILE__, __LINE__);
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
				/*** 多对一映射 ***/
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
				/*** 一对多映射 ***/
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
					pub_log_error("[%s][%d] 八位日期转十位日期出错! date=[%s]",
						__FILE__, __LINE__, stMap.sVarvalue);
					return -1;
				}
				memset(stMap.sVarvalue, 0x0, sizeof(stMap.sVarvalue));
				strcpy(stMap.sVarvalue, sVarValue);
			}
			
			/*** 如果变量需要映射,先映射 ***/
			if (stMap.cMapFlag != '0')
			{
				memset(sSecondVar, 0x0, sizeof(sSecondVar));
				zip_space(stMap.sVarvalue);
				if (stMap.sVarvalue[0] == '/' && stMap.sVarvalue[4] == '/')
				{
					iRc = iGetSecondVar(locvars, stPkgInfo, stMap.sVarvalue + 5, stMap.cMapFlag, sSecondVar);
					if (iRc == -1)
					{
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue + 5, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
						pub_log_error("[%s][%d] 取[%s]对应二代的值出错! MAPFLAG=[%c]",
							__FILE__, __LINE__, stMap.sVarvalue, stMap.cMapFlag);
						pub_log_error("[%s][%d] 一代报文编号=[%s] 一代的值=[%s] MAPFLAG=[%c]",
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
				/*** 映射为XML属性 ***/
				iRc = iSetXmlAttrVar(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 为节点[%s]设置属性失败!",
						__FILE__, __LINE__, stMap.sAlias);
					return -1;
				}
			}
			else if (stMap.cAnalysis == '1')
			{
				/*** 映射为XML节点 ***/
				iRc = iSetXmlVariable(xml, stMap.sAlias, stMap.sVarvalue);
				if (iRc == -1)
				{
					pub_log_error("[%s][%d] 设置节点[%s]失败!",
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
	pub_log_info("[%s][%d] 拆完附加域清单后 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}


/****************************************************
* 函 数 名:	iPKGDealPkgHeadIn
* 函数功能:	解一代PKG包头
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          xml         XML树
*          pkg         报文缓冲区
*          len        报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆PKG包头之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] PKG号为 =[%s]", 
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
	pub_log_info("[%s][%d] XML文件为=[%s]", __FILE__, __LINE__, sXmlName);
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 建XML树失败,请检查文件[%s]是否存在或者格式是否正确!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	/*** 首先将报文头中需要映射的变量进行映射 ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.BEFORMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, xml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 映射报文头失败!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}

	/*** 拆正文体 ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.PKGTRADE.TAG");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] 未配置[TAGLIST.PKGTRADE.TAG]标签!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	iRc = iDealTagIn(locvars, stPkgInfo, pnode, pXml, xml, pkg, len, iLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 处理TAG标签失败!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pub_log_info("[%s][%d] 拆完PKG包头之后 iCurLen=[%d]", __FILE__, __LINE__, *iLen);
	
	/*** 将报文头中需要映射的变量进行映射 ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.ENDMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, xml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 映射报文头失败!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pub_log_info("[%s][%d] 拆包头完成!", __FILE__, __LINE__);
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	return 0;
}

/****************************************************
* 函 数 名:	iPKGDealBodyIn
* 函数功能:	解一代PKG包体
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          xml         XML树
*          pkg         报文缓冲区
*          len        报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	char	sTradeHead[17]; /*** 业务头16为 3(块标识) + 3(业务要素集号) + 9 + 1 ***/
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sFactorNo, 0x0, sizeof(sFactorNo));
	memset(sTradeHead, 0x0, sizeof(sTradeHead));
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	iCurLen = *iLen;
	pub_log_info("[%s][%d] 拆PKG包体之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** 取明细总笔数 ***/
	get_zd_data("$count", sTmp);
	zip_space(sTmp);
	pub_log_info("[%s][%d] 明细总笔数=[%s]", __FILE__, __LINE__, sTmp);
	iCnt = atoi(sTmp);
	pub_log_info("[%s][%d] 明细总笔数iCnt=[%d]", __FILE__, __LINE__, iCnt);
	
	/*** 循环业务的业务要素集号都相同,只取一次 ***/
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
	pub_log_info("[%s][%d] 业务要素集号=[%s]", __FILE__, __LINE__, sFactorNo);
	
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
		pub_log_error("[%s][%d] 建XML树失败,请检查XML文件[%s]"
			"是否存在或者格式是否正确!", __FILE__, __LINE__, sXmlName);
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
		/*** 拆业务头 3 + 3 + 9 + 1 ***/
		memcpy(sTradeHead, pkg + iCurLen, 16);
		memcpy(sFactorNo, sTradeHead + 3, 3);
		iCurLen += 16;
		pub_log_info("[%s][%d] 业务要素集号=[%s] iCurLen=[%d]", 
			__FILE__, __LINE__, sFactorNo, iCurLen);
		
		pXml->current = pXml->root;
		/*** 拆正文体 ***/
		pnode = pub_xml_locnode(pXml, ".TAGLIST.TAG");
		if (pnode == NULL)
		{
			pub_log_error("[%s][%d] XML文件[%s]未配置TAG标签!", 
				__FILE__, __LINE__, sXmlName);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
		iRc = iPKGDealTagIn(locvars, stPkgInfo, pnode, pXml, xml, pkg, len, i, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] 处理TAG标签失败!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
	}
	pub_xml_deltree(pXml);
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] 拆完正文体之后 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	pub_log_info("[%s][%d] 拆正文体完成!", __FILE__, __LINE__);
	
	return 0;
}

/****************************************************
* 函 数 名:	iCMTDealTradeBodyIn
* 函数功能:	将正文体需要映射的变量映射到新的XML树中
* 作    者:	MaWeiwei
* 日    期:	2011-11-16
* 输入参数:
*		   locvars   变量池缓冲区
*          xml         新的XML树
*          psBuf       报文缓冲区(不含报文头及业务头)
*          iLen        报文长度(不含报文头及业务头)
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆正文体之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (memcmp(pkg + iCurLen, "{3:", 3) != 0)
	{
		pub_log_error("[%s][%d] 报文正文体开始标记有误!",
			__FILE__, __LINE__);
		pub_log_bin(SW_LOG_INFO, pkg, strlen(pkg), "报文正文体开始标记有误");
		return -1;
	}
	
	if (stPkgInfo.cPkgType == '1')
	{
		/*** 大额报文 ***/
		memcpy(sFileName, "HVPS_CMT", sizeof(sFileName) - 1);
	}
	else if (stPkgInfo.cPkgType == '2')
	{
		/*** 小额CMT报文 ***/
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
		pub_log_error("[%s][%d] 建xml树失败! 请检查xml文件[%s]"
			"是否存在或者格式是否正确!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	/*** 为二代日期增加临时变量,#TEMP="T" ***/		
	if (locvars == NULL)
	{
		set_zd_data("#TEMP", "T");
	}
	else
	{
		loc_set_zd_data(locvars, "#TEMP", "T");
	}
	/*** 首先将报文头中需要映射的变量进行映射 ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.BEFORMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, newxml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 映射报文头失败!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	/*** 拆正文体 ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.CMTTRADE.TAG");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] 报文未配置TAG标签!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	iRc = iDealTagIn(locvars, stPkgInfo, pnode, pXml, newxml, pkg, len, iLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 处理TAG标签失败!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	
	pub_log_info("[%s][%d] 拆TAG标签完成! iCurLen=[%d]", __FILE__, __LINE__, *iLen);
	/*** 将报文头中需要映射的变量进行映射 ***/
	pnode = pub_xml_locnode(pXml, ".TAGLIST.ENDMAP.ITEM");
	iRc = iMapHeaderIn(locvars, stPkgInfo, pXml, newxml, pnode);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 映射业务头失败!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pub_log_info("[%s][%d] 解正文体完成!", __FILE__, __LINE__);
	pub_xml_deltree(pXml);
	pXml = NULL;

	return 0;
}

/****************************************************
* 函 数 名:	iDealBasHeadIn
* 函数功能:	解一代CMT/PKG报文头
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        ITEM节点
*          xml         XML树
*          pkg         报文缓冲区
*          len         报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 解报文头之前iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
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
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(stItem.cVarname, node1->value);
		
		node1 = pub_xml_locnode(xml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置TYPE标签!", __FILE__, __LINE__);
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iVarlength = atoi(node1->value + 1);

		memcpy(stItem.cVarvalue, pkg + iCurLen, stItem.iVarlength);
		iCurLen += stItem.iVarlength;
		if (iCurLen > len)
		{
			pub_log_error("[%s][%d] 报文长度有误! 报文头长=[%d] 报文总长度=[%d]",
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
	pub_log_info("[%s][%d] 解报文头之前iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** 将报文头中的部分变量进行映射 ***/
	iBasHeadMapIn(locvars);
	pub_log_info("[%s][%d] 映射报文头完成!", __FILE__, __LINE__);
	iSetCheckVar(locvars, pkg);
	
	return 0;
}	

/****************************************************
* 函 数 名:	iBepsDealIn
* 函数功能:	解小额报文
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        ITEM节点
*          xml         XML树
*          newxml      新的XML树
*          pkg         报文缓冲区
*          len         报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
		
	/*** 解报文头 ***/
	iRc = iDealBasHeadIn(locvars, node, xml, pkg, len, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 解报文头失败!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] 解报文头完成! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	memcpy(sBlock, pkg + iCurLen, 3);
	if (strcmp(sBlock, "{P:") == 0)
	{
		/*** 小额PKG报文 ***/
		/*** 解包头 ***/
		iRc = iPKGDealPkgHeadIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] 解PKG包头失败!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] 拆PKG包头完成! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
		pub_log_info("[%s][%d] 包头处理完成!", __FILE__, __LINE__);
		
		pub_log_info("[%s][%d] 包体====[%s]", __FILE__, __LINE__, pkg + iCurLen);
		
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
			pub_log_info("[%s][%d] 明细长度====[%ld]\n", __FILE__, __LINE__, iLen);
			pDetail = calloc(1, iLen + 1);
			if (pDetail == NULL)
			{
				pub_log_error("[%s][%d] 申请内存失败! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			memcpy(pDetail, pTmp, iLen);
			
			/***
			pub_log_info( "[%s][%d] LEN=[%d] 明细====[%s]", __FILE__, __LINE__, iLen, pDetail);
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
			pub_log_info("[%s][%d] 明细====[%s]", __FILE__, __LINE__, pDetail);
			free(pDetail);
			pDetail = NULL;
		}
		
		/*** 解包体  业务头(业务要素集号)+正文体(业务类型号+基本业务要素+附加域) ***/
		iRc = iPKGDealBodyIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] 解包体失败!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] 解包完成!", __FILE__, __LINE__); 
		
		/*** 设置报文格式类型 ***/
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
		/*** 小额CMT报文 ***/
		/*** 直接解正文体,小额CMT报文无业务头 ***/
		iRc = iCMTDealTradeBodyIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] 解正文体失败!", __FILE__, __LINE__);
			return -1;
		}
		
		/*** 设置报文格式类型 ***/
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
		pub_log_error("[%s][%d] 报文块标识有误, sBlock=[%s]", 
			__FILE__, __LINE__, sBlock);
		return -1;
	}
	
	/*** 对二代报文编号进行特殊处理 ***/
	memset(sSecondNo, 0x0, sizeof(sSecondNo));
	iRc = iGet2ndPkgNo(pkg, stPkgInfo.sPkgNo, sSecondNo);
	if (iRc == -1)
	{
		if (strcmp(stPkgInfo.sPkgNo, "007") == 0 || strcmp(stPkgInfo.sPkgNo, "121_007") == 0)
		{
			strcpy(sSecondNo, "beps.121.001.01");
			pub_log_info("[%s][%d] 小额退汇业务!", 
				__FILE__, __LINE__);
		}
		else
		{
			pub_log_error("[%s][%d] 取二代报文编号出错! 一代报文编号=[%s]",
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
	pub_log_info("[%s][%d] 二代报文编号=[%s]", __FILE__, __LINE__, sSecondNo);
	
	return 0;
}

/****************************************************
* 函 数 名:	iCMTDealTradeHeaderIn
* 函数功能:	拆一代CMT报文业务头
* 作    者:	MaWeiwei
* 日    期:	2011-11-16
* 输    入:
*          locvars 变量池缓冲区
*          pkg       报文缓冲区(不含报文头)
*          iPkgLen   报文长度(不含报文头)
* 输    出:
*          iLen      业务头长度
* 返 回 值:  	0 成功, －1 失败
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
	pub_log_info("[%s][%d] 拆业务头之前 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
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
	
	/*** 首先根据cmt报文头xml文件解析报文头 ***/
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 建xml树失败, 请检查xml文件[%s]是否存在"
			"或者格式是否正确!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	pnode = pub_xml_locnode(pXml, ".CMTHEADER.ITEM");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] 没有找到第一个ITEM! 请检查xml文件[%s]!",
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
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		strcpy(stItem.cVarname, node1->value);
		
		node1 = pub_xml_locnode(pXml, "TYPE");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置TYPE标签!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		stItem.cType = node1->value[0];
		stItem.iVarlength = atoi(node1->value + 1);

		memcpy(stItem.cVarvalue, pkg + iCurLen, stItem.iVarlength);
		iCurLen += stItem.iVarlength;
		if (iCurLen > len)
		{	
			pub_log_error("[%s][%d] 报文长度有误! iCurLen=[%d] len=[%d]",
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
		pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", 
			__FILE__, __LINE__, stItem.cVarname, stItem.cVarvalue);
		pnode = pnode->next;
	}
	pub_xml_deltree(pXml);
	pXml = NULL;
	
	*iLen = iCurLen;
	pub_log_info("[%s][%d] 拆完业务头之后 iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	return 0;
}

/****************************************************
* 函 数 名:	iHvpsDealIn
* 函数功能:	解大额报文
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          node        ITEM节点
*          xml         XML树
*          newxml      新的XML树
*          pkg         报文缓冲区
*          len         报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	
	/*** 解报文头 ***/
	iRc = iDealBasHeadIn(locvars, node, xml, pkg, len, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 解报文头失败!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] 解报文头完成! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	if (strncmp(pkg + iCurLen, "{2:", 3) == 0)
	{
		pub_log_info("[%s][%d] 该大额报文存在业务头,开始解业务头!", __FILE__, __LINE__);
		/*** 解业务头***/
		iRc = iCMTDealTradeHeaderIn(locvars, pkg, len, &iCurLen);
		if (iRc == -1)
		{
			pub_log_error("[%s][%d] 解业务头失败!", __FILE__, __LINE__);
			return -1;
		}
		pub_log_info("[%s][%d] 解业务头完成! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	}
	else if (strncmp(pkg + iCurLen, "{3:", 3) != 0)
	{
		pub_log_error("[%s][%d] 正文体开始标记有误! 正文体=[%s]",
			__FILE__, __LINE__, pkg + iCurLen);
		return -1;
	}
	
	/*** 解正文体 ***/
	iRc = iCMTDealTradeBodyIn(locvars, stPkgInfo, newxml, pkg, len, &iCurLen);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 解正文体失败!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] 解正文体完成! iCurLen=[%d]", __FILE__, __LINE__, iCurLen);
	
	/*** 对二代报文编号进行特殊处理 ***/
	memset(sSecondNo, 0x0, sizeof(sSecondNo));
	iRc = iGet2ndPkgNo(pkg, stPkgInfo.sPkgNo, sSecondNo);
	if (iRc == -1)
	{
		if (strcmp(stPkgInfo.sPkgNo, "108") == 0 || strcmp(stPkgInfo.sPkgNo, "111_108") == 0)
		{
                        strcpy(sSecondNo, "hvps.111.001.01");
                        pub_log_info("[%s][%d] 大额退汇业务!", 
				__FILE__, __LINE__);
                }
                else
                {
			pub_log_error("[%s][%d] 取二代报文编号出错! 一代报文编号=[%s]",
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
	pub_log_info("[%s][%d] 二代报文编号=[%s]", __FILE__, __LINE__, sSecondNo);
	
	/*** 设置报文格式类型 ***/
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
* 函 数 名:	iPkgToSwitchPkgCmt
* 函数功能:	解一代报文到二代报文
* 作    者:	MaWeiwei
* 日    期:	2011-11-24
* 输入参数:
*		   locvars   变量池缓冲区
*          pkg         报文缓冲区
*          xmlname     xml文件名
*          len         报文长度
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
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
	
	/*** 为业务置一代标识 ***/
	if (locvars == NULL)
	{
		set_zd_data("$pkgtype", "1");
	}
	else
	{
		loc_set_zd_data(locvars, "$pkgtype", "1");
	}
	
	/*** 报文传输方向 ***/
	if (locvars == NULL)
	{
		set_zd_data("#MesgDirection", "D");
	}
	else
	{
		loc_set_zd_data(locvars, "#MesgDirection", "D");
	}
	
	/*** 为二代报文置币种 ***/
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
		pub_log_error("[%s][%d] 解包初始化错误!", __FILE__, __LINE__);
		return -1;
	}
	/*** 为新的XML树构造根节点 ***/
	proot = (sw_xmlnode_t *)calloc(1, sizeof(sw_xmlnode_t));
	if (proot == NULL)
	{
		pub_log_error("[%s][%d] 申请内存失败! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	s_initxmlnode(proot);
	proot->name = (char *)calloc(1, strlen("ROOT") + 1);
	if (proot->name == NULL)
	{
		pub_log_error("[%s][%d] 申请内存失败! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	strcpy(proot->name, "ROOT");
	
	proot->value = (char *)calloc(1, strlen("ROOT") + 1);
	if (proot->value == NULL)
	{
		pub_log_error("[%s][%d] 申请内存失败! errno=[%d] [%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	strcpy(proot->value, "ROOT");
	proot->node_type = SW_NODE_ROOT;
	
	pNewXml = (sw_xmltree_t *)calloc(1, sizeof(sw_xmltree_t));
	if (pNewXml == NULL)
	{
		pub_log_error("[%s][%d] 申请内存失败! errno=[%d] [%s]",
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
	pub_log_info("[%s][%d] 配置文件==[%s]", 
		__FILE__, __LINE__, sXmlName);
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败,请检查xml文件[%s]是否存在"
			"或者格式是否正确!", __FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	pnode = pub_xml_locnode(pXml, ".CBM.PKGTYPE");
	if (pnode == NULL || pnode->value == NULL)
	{
		pub_log_error("[%s][%d] 未找到.CBM.PKGTYPE标签!",
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
		pub_log_error("[%s][%d] 未找到[.CBM.ANALYZE]标签!",
			__FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}
	pXml->current = pnode;
	pnode = pstGetNodeByTarget(pXml, "ANALYZE", "PKGTYPE", "CMT");
	if (pnode == NULL)
	{
		pub_log_error("[%s][%d] 未找到[CMT]配置标签!", __FILE__, __LINE__);
		pub_xml_deltree(pXml);
		pXml = NULL;
		return -1;
	}

	/*** 开始处理firstwork ***/
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
	
	/*** 开始处理firstwork.item ***/
	pnode = pub_xml_locnode(pXml, "FIRSTWORK.ITEM");
	if (pnode != NULL)
	{
		pub_log_debug("[%s][%d] 正在处理FIRSTWORK.ITEM!",
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
	
	/*** 开始处理PACKAGE ***/
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
			pub_log_error("[%s][%d] 报文PACKAGE未配置RESPCD项! 释放xml", 
				__FILE__, __LINE__);
			pub_xml_deltree(pXml);
			pXml = NULL;
			return -1;
		}
		memcpy(sRespcdName, node1->value, sizeof(sRespcdName) - 1);

		/*** 检查INCLUDE条件, 此条件只能有一个 ***/
		node1 = pub_xml_locnode(pXml, "INCLUDE");
		if (node1 != NULL)
		{
			if (iLocChkinclude(locvars, stFirstwork.cVarname, node2->value) != 0)
			{
				pnode = pnode->next;
				continue;
			}
		}
		/*** 检查CHECK条件,此条件可能有多个 ***/
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
			/*** CHECK条件不满足,继续下一个PACKAGE ***/
			pnode = pnode->next;
			continue;
		}
		/****到此则或者INCLUDE和CHECK条件都满足或者是默认包****/
		pnode = pub_xml_locnode(pXml, "ITEM");
		pXml->current = pnode;
		if (stPkgInfo.cPkgType == '1')
		{
			/*** 大额(CMT)报文 ***/
			iRc = iHvpsDealIn(locvars, stPkgInfo, pnode, pXml, pNewXml, psBuf, iLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] 拆大额报文出错!", __FILE__, __LINE__);
				pub_xml_deltree(pXml);
				pXml = NULL;
				return -1;
			}
		}
		else 
		{
			/*** 小额报文 ***/
			iRc = iBepsDealIn(locvars, stPkgInfo, pnode, pXml, pNewXml, psBuf, iLen);
			if (iRc < 0)
			{
				pub_log_error("[%s][%d] 拆小额报文出错!", __FILE__, __LINE__);
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
		pub_log_info("[%s][%d] 拆包完成! 释放xml", __FILE__, __LINE__);
		return 0;
	}
	pub_xml_deltree(pXml);
	pXml = NULL;
	pub_log_error("[%s][%d] 严重错误:未找到拆包配置!", __FILE__, __LINE__);

	return -1;
}
