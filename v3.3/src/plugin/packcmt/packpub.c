#include "packcmt.h"

int iInitItem(ST_ITEM *pstItem)
{
	memset(pstItem, 0x0, sizeof(ST_ITEM));
	pstItem->iBegin = 0;
	pstItem->iLength = 0;
	pstItem->iVarlength = 0;
	pstItem->iStart = 0;
	pstItem->iNum = 0;
	pstItem->iMax = 0;
	pstItem->iIndex = 0;
	pstItem->cConst = '0';
	return 0;
}

void initMap(ST_MAP *pstMap)
{
	memset(pstMap, 0x0, sizeof(ST_MAP));
	pstMap->cFlag = '0';
	pstMap->cType = '0';
	pstMap->cConst = '0';
	pstMap->cDate = '0';
	pstMap->cAmt = '0';
	pstMap->cVarLen = '0';
	pstMap->cMapFlag = '0';
	pstMap->cSendFlag = '0';
	pstMap->cOptional = '0';
	pstMap->cAnalysis = '0';
	pstMap->cNodeType = '0';
	pstMap->iLength = 0;
	pstMap->iBegin = 0;
	pstMap->iMapBegin = 0;
}

void initTag(TAGNODE *pstTagNode)
{
	memset(pstTagNode, 0x0, sizeof(TAGNODE));
	pstTagNode->cVarLen = '0';
	pstTagNode->cFlag = '0';
	pstTagNode->cConst = '0';
	pstTagNode->cType = '0';
	pstTagNode->cNum = '0';
	pstTagNode->cList = '0';
	pstTagNode->iLength = 0;	
	pstTagNode->cOptional = '0';	
	pstTagNode->cAnalysis = '0';
	pstTagNode->cNodeType = '0';
	pstTagNode->cNextIsVarLen = '0';
}

void vPkgNoMapInit(ST_PKGNOMAP *pstPkgNoMap)
{	
	int	i = 0;
	
	memset(pstPkgNoMap, 0x0, sizeof(ST_PKGNOMAP));
	
	pstPkgNoMap->iFlag = 0;
	pstPkgNoMap->iCount = -1;
	for (i = 0; i < MAX_VARMAP_NUM; i++)
	{
		pstPkgNoMap->stPkgNoMap[i].iCheck = 0;
		pstPkgNoMap->stPkgNoMap[i].iLength = 0;
	}
}

int iInitPkgNoMap(ST_PKGNOMAP *pstPkgNoMap)
{
	int	i = 0;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/pkgnomap.xml", getenv("HOME"));
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/pkgnomap.xml", getenv("SWWORK"));
	}
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 建XML树失败,请检查XML文件[%s]是否存在或者格式是否正确!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	
	i = 0;
	pnode = pub_xml_locnode(pXml, ".PKGNOMAP.ITEM");
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "FIRSTNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置FIRSTNO!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstPkgNoMap->stPkgNoMap[i].sFirstNo, node1->value, 
			sizeof(pstPkgNoMap->stPkgNoMap[i].sFirstNo) - 1);
		
		node1 = pub_xml_locnode(pXml, "SECONDNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置SECONDNO!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstPkgNoMap->stPkgNoMap[i].sSecondNo, node1->value, 
			sizeof(pstPkgNoMap->stPkgNoMap[i].sSecondNo) - 1);
		
		node1 = pub_xml_locnode(pXml, "CHECK");
		if (node1 != NULL)
		{
			memcpy(pstPkgNoMap->stPkgNoMap[i].sCheckVar, node1->value, 
				sizeof(pstPkgNoMap->stPkgNoMap[i].sCheckVar) - 1);
			
			node1 = pub_xml_locnode(pXml, "VALUE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] 指定了CHECK选项，但未配置VALUE!",
					__FILE__, __LINE__);
				pub_xml_deltree(pXml);
				return -1;
			}
			memcpy(pstPkgNoMap->stPkgNoMap[i].sCheckValue, node1->value, 
				sizeof(pstPkgNoMap->stPkgNoMap[i].sCheckValue) - 1);
			
			node1 = pub_xml_locnode(pXml, "LENGTH");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] 指定了CHECK选项,但未配置LENGTH!",
					__FILE__, __LINE__);
				pub_xml_deltree(pXml);
				return -1;
			}
			pstPkgNoMap->stPkgNoMap[i].iLength = atoi(node1->value);
			pstPkgNoMap->stPkgNoMap[i].iCheck = 1;
		}
		
		i++;
		pnode = pnode->next;
	}
	pstPkgNoMap->iCount = i;
	pstPkgNoMap->iFlag = 1;
	
	pub_xml_deltree(pXml);
	
	return 0;
}

int iGet2ndPkgNo(char *pkg, char *psPkgNo, char *sSecondNo)
{
	int	i = 0;
	int	j = 0;
	int	iRc = 0;
	char	*ptr = NULL;
	char	sPkgNo[8];
	char	sTag[6];
	char	sName[64];
	char	sVarValue[512];
	ST_PKGNOMAP	stPkgMap;
	
	memset(sTag, 0x0, sizeof(sTag));
	memset(sName, 0x0, sizeof(sName));
	memset(sPkgNo, 0x0, sizeof(sPkgNo));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	
	pub_log_info("[%s][%d] 报文编号=[%s]", __FILE__, __LINE__, psPkgNo);

	ptr = strchr(psPkgNo, '_');
	if (ptr != NULL)
	{
		memcpy(sPkgNo, ptr + 1, sizeof(sPkgNo) - 1);
	}
	else
	{
		memcpy(sPkgNo, psPkgNo, sizeof(sPkgNo) - 1);
	}
	
	pub_log_info("[%s][%d] sPkgNo=[%s]", __FILE__, __LINE__, sPkgNo);
	
	vPkgNoMapInit(&stPkgMap);
	iRc = iInitPkgNoMap(&stPkgMap);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 初始化报文编号对应关系失败!", __FILE__, __LINE__);
		return -1;
	}
	
	for (i = 0; i < stPkgMap.iCount; i++)
	{
		if (strcmp(stPkgMap.stPkgNoMap[i].sFirstNo, sPkgNo) == 0)
		{
			if (stPkgMap.stPkgNoMap[i].iCheck != 1)
			{
				break;
			}
			else
			{
				memset(sTag, 0x0, sizeof(sTag));
				sprintf(sTag, ":%s:", stPkgMap.stPkgNoMap[i].sCheckVar);
				pub_log_info("[%s][%d] CHECK TAG=[%s]", __FILE__, __LINE__, sTag);
				ptr = strstr(pkg, sTag);
				if (ptr == NULL)
				{
					pub_log_error("[%s][%d] 报文未指定[%s]标签!", __FILE__, __LINE__, sTag);
					return -1;
				}
				memset(sVarValue, 0x0, sizeof(sVarValue));
				memcpy(sVarValue, ptr + 5, stPkgMap.stPkgNoMap[i].iLength);
				if (iCheckTx(stPkgMap.stPkgNoMap[i].sCheckValue, sVarValue) == 0)
				{
					break;
				}
				pub_log_info("[%s][%d] CHECK未通过! CHECKVAR=[%s]=[%s]  VALUE=[%s]",
					__FILE__, __LINE__, stPkgMap.stPkgNoMap[i].sCheckVar, sVarValue,
					stPkgMap.stPkgNoMap[i].sCheckValue); 
			}
		}
	}
	if (i == stPkgMap.iCount)
	{
		pub_log_error("[%s][%d] 未找到匹配的报文编号! 一代报文编号=[%s][%s]",
			__FILE__, __LINE__, psPkgNo, sPkgNo);
		return -1;
	}
	strcpy(sSecondNo, stPkgMap.stPkgNoMap[i].sSecondNo);

	return 0;
}

void vVarMapInit(ST_VARMAP *pstVarMap)
{
	memset(pstVarMap, 0x0, sizeof(ST_VARMAP));
	pstVarMap->iFlag = 0;
	pstVarMap->iCount = -1;
}

int iInitVarMap12(ST_VARMAP *pstVarMap)
{
	int	i = 0;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/varmap12.xml", getenv("HOME"));
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/lsncfg/common/1st/varmap12.xml", getenv("SWWORK"));
	}
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 建XML树失败,请检查XML文件[%s]是否存在或者格式是否正确!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	i = 0;
	pnode = pub_xml_locnode(pXml, "VARMAP.ITEM");
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "FIRSTVAR");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置FIRSTVAR!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sFirstVar, node1->value, sizeof(pstVarMap->stVarMap[i].sFirstVar) - 1);
		
		node1 = pub_xml_locnode(pXml, "SECONDVAR");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置SECONDVAR!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sSecondVar, node1->value, sizeof(pstVarMap->stVarMap[i].sSecondVar) - 1);
		
		node1 = pub_xml_locnode(pXml, "FIRSTNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置FIRSTNO!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sFirstNo, node1->value, sizeof(pstVarMap->stVarMap[i].sFirstNo) - 1);
		
		node1 = pub_xml_locnode(pXml, "SECONDNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置SECONDNO!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sSecondNo, node1->value, sizeof(pstVarMap->stVarMap[i].sSecondNo) - 1);
		
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 != NULL)
		{
			memcpy(pstVarMap->stVarMap[i].sVarName, node1->value, sizeof(pstVarMap->stVarMap[i].sVarName) - 1);
		}
		
		node1 = pub_xml_locnode(pXml, "VALUE");
		if (node1 != NULL)
		{
			memcpy(pstVarMap->stVarMap[i].sVarValue, node1->value, sizeof(pstVarMap->stVarMap[i].sVarValue) - 1);
		}
		
		node1 = pub_xml_locnode(pXml, "MAPFLAG");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置MAPFLAG!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		pstVarMap->stVarMap[i].cMapFlag = node1->value[0];
		i++;
		pnode = pnode->next;
	}
	pstVarMap->iCount = i;
	pstVarMap->iFlag = 1;
	pub_xml_deltree(pXml);
	pub_log_info("[%s][%d] COUNT=[%d]", __FILE__, __LINE__, pstVarMap->iCount);
	return 0;
}

int iInitVar21Map(ST_VARMAP *pstVarMap)
{
	int	i = 0;
	char	sXmlName[512];
	sw_xmltree_t	*pXml = NULL;
	sw_xmlnode_t	*pnode = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sXmlName, 0x0, sizeof(sXmlName));
	
	if (getenv("SWWORK") == NULL)
	{
		sprintf(sXmlName, "%s/cfg/common/1st/varmap21.xml", getenv("HOME"));
	}
	else
	{
		sprintf(sXmlName, "%s/cfg/common/1st/varmap21.xml", getenv("SWWORK"));
	}
	
	pXml = pub_xml_crtree(sXmlName);
	if (pXml == NULL)
	{
		pub_log_error("[%s][%d] 建XML树失败,请检查XML文件[%s]是否存在或者格式是否正确!",
			__FILE__, __LINE__, sXmlName);
		return -1;
	}
	i = 0;
	pnode = pub_xml_locnode(pXml, "VARMAP.ITEM");
	while (pnode != NULL)
	{
		if (strcmp(pnode->name, "ITEM") != 0)
		{
			pnode = pnode->next;
			continue;
		}
		pXml->current = pnode;
		node1 = pub_xml_locnode(pXml, "FIRSTVAR");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置FIRSTVAR!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sFirstVar, node1->value, sizeof(pstVarMap->stVarMap[i].sFirstVar) - 1);
		
		node1 = pub_xml_locnode(pXml, "SECONDVAR");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置SECONDVAR!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sSecondVar, node1->value, sizeof(pstVarMap->stVarMap[i].sSecondVar) - 1);
		
		node1 = pub_xml_locnode(pXml, "FIRSTNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置FIRSTNO!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sFirstNo, node1->value, sizeof(pstVarMap->stVarMap[i].sFirstNo) - 1);
		
		node1 = pub_xml_locnode(pXml, "SECONDNO");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置SECONDNO!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		memcpy(pstVarMap->stVarMap[i].sSecondNo, node1->value, sizeof(pstVarMap->stVarMap[i].sSecondNo) - 1);
		
		node1 = pub_xml_locnode(pXml, "NAME");
		if (node1 != NULL)
		{
			memcpy(pstVarMap->stVarMap[i].sVarName, node1->value, sizeof(pstVarMap->stVarMap[i].sVarName) - 1);
		}
		
		node1 = pub_xml_locnode(pXml, "VALUE");
		if (node1 != NULL)
		{
			memcpy(pstVarMap->stVarMap[i].sVarValue, node1->value, sizeof(pstVarMap->stVarMap[i].sVarValue) - 1);
		}
		
		node1 = pub_xml_locnode(pXml, "MAPFLAG");
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] 未配置MAPFLAG!", __FILE__, __LINE__);
			pub_xml_deltree(pXml);
			return -1;
		}
		pstVarMap->stVarMap[i].cMapFlag = node1->value[0];
		i++;
		pnode = pnode->next;
	}
	pstVarMap->iCount = i;
	pstVarMap->iFlag = 1;
	pub_xml_deltree(pXml);
	
	return 0;
}

int iGetSecondVar(locvars, stPkgInfo, sFirstVar, cFlag, sSecondVar)
sw_loc_vars_t *locvars;
ST_PKGINFO stPkgInfo;
char *sFirstVar;
char cFlag;
char *sSecondVar;
{
	int	i = 0;
	int	iRc = 0;
	char	*ptr = NULL;
	char	sPkgNo[8];
	char	sVarValue[128];
	ST_VARMAP	stMapVar;
	
	memset(sPkgNo, 0x0, sizeof(sPkgNo));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	
	pub_log_info("[%s][%d] stPkgInfo.sPkgNo===[%s]", __FILE__, __LINE__, stPkgInfo.sPkgNo);
	ptr = strchr(stPkgInfo.sPkgNo, '_');
	if (ptr != NULL)
	{
		memcpy(sPkgNo, ptr + 1, sizeof(sPkgNo) - 1);
	}
	else
	{
		memcpy(sPkgNo, stPkgInfo.sPkgNo, sizeof(sPkgNo) - 1);
	}
	
	pub_log_info("[%s][%d] sPkgNo=[%s]", __FILE__, __LINE__, sPkgNo);

	vVarMapInit(&stMapVar);
	iRc = iInitVarMap12(&stMapVar);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 初始化VAPMAP12失败!", __FILE__, __LINE__);
		return -1;
	}

	pub_log_info("[%s][%d] sFirstVar=[%s]", __FILE__, __LINE__, sFirstVar);
	zip_space(sFirstVar);
	pub_log_info("[%s][%d] sFirstVar=[%s]", __FILE__, __LINE__, sFirstVar);
	
	pub_log_info("[%s][%d] stMapVar.iCount=[%d]", __FILE__, __LINE__, stMapVar.iCount);
	
	if (cFlag == '*')
	{
		for (i = 0; i < stMapVar.iCount; i++)
		{
			if (strcmp(stMapVar.stVarMap[i].sFirstVar, sFirstVar) == 0 && stMapVar.stVarMap[i].cMapFlag == cFlag)
			{
				strcpy(sSecondVar, stMapVar.stVarMap[i].sSecondVar);
				pub_log_info("[%s][%d] [%s]对应二代的值为=[%s]",
					__FILE__, __LINE__, sFirstVar, sSecondVar);
				return 0;
			}
		}
	}
		

	for (i = 0; i < stMapVar.iCount; i++)
	{
		if (strcmp(stMapVar.stVarMap[i].sFirstNo, sPkgNo) == 0
			&& strcmp(stMapVar.stVarMap[i].sFirstVar, sFirstVar) == 0
			&& stMapVar.stVarMap[i].cMapFlag == cFlag)
		{
			if (strlen(stMapVar.stVarMap[i].sVarName) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				if (stMapVar.stVarMap[i].sVarName[0] == '.')
				{
					if (locvars == NULL)
					{
						get_zd_data(stMapVar.stVarMap[i].sVarName, sVarValue);
					}
					else
					{
						loc_get_zd_data(locvars, stMapVar.stVarMap[i].sVarName, sVarValue);
					}
				}
				else if (stMapVar.stVarMap[i].sVarName[0] == '#')
				{
					if (locvars == NULL)
					{
						get_zd_data(stMapVar.stVarMap[i].sVarName, sVarValue);
					}
					else
					{
						loc_get_zd_data(locvars, stMapVar.stVarMap[i].sVarName, sVarValue);
					}
				}
				else
				{
					pub_log_info("[%s][%d] 变量类型有误! sVarName=[%s]",
						__FILE__, __LINE__, stMapVar.stVarMap[i].sVarName);
					return -1;
				}
				
				if (strcmp(stMapVar.stVarMap[i].sVarValue, sVarValue) == 0)
				{
					strcpy(sSecondVar, stMapVar.stVarMap[i].sSecondVar);
					pub_log_info("[%s][%d] [%s]对应二代的值为=[%s]",
						__FILE__, __LINE__, sFirstVar, sSecondVar);
					return 0;
				}
				continue;
			}
			strcpy(sSecondVar, stMapVar.stVarMap[i].sSecondVar);
			pub_log_info("[%s][%d] [%s]对应二代的值为=[%s]",
				__FILE__, __LINE__, sFirstVar, sSecondVar);
			return 0;
		}
	}
	pub_log_error("[%s][%d] 没找到[%s]对应的二代值!", __FILE__, __LINE__, sFirstVar);

	/*** 来账映射时暂时不报错,对于所有来账报文,都必须落地处理 ***/
	pub_log_info("[%s][%d] 来账映射时暂时不报错,对于所有来账报文,都必须落地处理!", __FILE__, __LINE__);
	pub_log_error("[%s][%d] 没找到[%s]对应的二代值! 赋原值!", __FILE__, __LINE__, sFirstVar);
	strcpy(sSecondVar, sFirstVar);

	return 0;
}

int iGetFirstVar(locvars, stPkgInfo, sSecondVar, cFlag, sFirstVar)
sw_loc_vars_t *locvars;
ST_PKGINFO stPkgInfo;
char *sSecondVar;
char cFlag;
char *sFirstVar;
{
	int	i = 0;
	int	iRc = 0;
	char	*ptr = NULL;
	char	sPkgNo[8];
	char	sVarValue[128];
	ST_VARMAP	stMapVar;
		
	memset(sPkgNo, 0x0, sizeof(sPkgNo));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	
	ptr = strchr(stPkgInfo.sPkgNo, '_');
	if (ptr != NULL)
	{
		memcpy(sPkgNo, ptr + 1, sizeof(sPkgNo) - 1);
	}
	else
	{
		memcpy(sPkgNo, stPkgInfo.sPkgNo, sizeof(sPkgNo) - 1);
	}
	
	vVarMapInit(&stMapVar);
	iRc = iInitVar21Map(&stMapVar);
	if (iRc == -1)
	{
		pub_log_error("[%s][%d] 初始化VAEMAP21失败!", __FILE__, __LINE__);
		return -1;
	}
	
	pub_log_info("[%s][%d] sFirstPkgNo=[%s] sSecondPkgNo=[%s] sSecondVar=[%s]",
		__FILE__, __LINE__, stPkgInfo.sPkgNo, stPkgInfo.sMsgType, sSecondVar);
	pub_log_info("[%s][%d] sSecondVar=[%s]", __FILE__, __LINE__, sSecondVar);
	zip_space(sSecondVar);
	pub_log_info("[%s][%d] sSecondVar=[%s]", __FILE__, __LINE__, sSecondVar);
	if (cFlag == '*')
	{
		for (i = 0; i < stMapVar.iCount; i++)
		{
			if (strcmp(stMapVar.stVarMap[i].sSecondVar, sSecondVar) == 0 && stMapVar.stVarMap[i].cMapFlag == cFlag)
			{
				strcpy(sFirstVar, stMapVar.stVarMap[i].sFirstVar);
				pub_log_info("[%s][%d] [%s]对应一代的值为=[%s]",
					__FILE__, __LINE__, sSecondVar, sFirstVar);
				return 0;
			}
		}
	}

	for (i = 0; i < stMapVar.iCount; i++)
	{
		if (strcmp(stMapVar.stVarMap[i].sSecondNo, stPkgInfo.sMsgType) == 0
			&& strcmp(stMapVar.stVarMap[i].sFirstNo, sPkgNo) == 0
			&& strcmp(stMapVar.stVarMap[i].sSecondVar, sSecondVar) == 0
			&& stMapVar.stVarMap[i].cMapFlag == cFlag)
		{
			if (strlen(stMapVar.stVarMap[i].sVarName) > 0)
			{
				memset(sVarValue, 0x0, sizeof(sVarValue));
				if (stMapVar.stVarMap[i].sVarName[0] == '.')
				{
					if (locvars == NULL)
					{
						get_zd_data(stMapVar.stVarMap[i].sVarName, sVarValue);
					}
					else
					{
						loc_get_zd_data(locvars, stMapVar.stVarMap[i].sVarName, sVarValue);
					}
				}
				else if (stMapVar.stVarMap[i].sVarName[0] == '#')
				{
					if (locvars == NULL)
					{
						get_zd_data(stMapVar.stVarMap[i].sVarName, sVarValue);
					}
					else
					{
						loc_get_zd_data(locvars, stMapVar.stVarMap[i].sVarName, sVarValue);
					}
				}
				else
				{
					pub_log_info("[%s][%d] 变量类型有误! sVarName=[%s]",
						__FILE__, __LINE__, stMapVar.stVarMap[i].sVarName);
					return -1;
				}
				
				pub_log_info("[%s][%d] VARNAME=[%s] VARVALUE=[%s]",
					__FILE__, __LINE__, stMapVar.stVarMap[i].sVarName, sVarValue);
				
				if (strcmp(stMapVar.stVarMap[i].sVarValue, sVarValue) == 0)
				{
					strcpy(sFirstVar, stMapVar.stVarMap[i].sFirstVar);
					pub_log_info("[%s][%d] [%s]对应一代的值为[%s]",
						__FILE__, __LINE__, sSecondVar, sFirstVar);  
					return 0;
				}
				continue;
			}
			strcpy(sFirstVar, stMapVar.stVarMap[i].sFirstVar);
			pub_log_info("[%s][%d] [%s]对应一代的值为=[%s]",
				__FILE__, __LINE__, sSecondVar, sFirstVar);
			return 0;
		}
	}
	pub_log_error("[%s][%d] 没找到[%s]对应的一代值!", __FILE__, __LINE__, sSecondVar);
	return -1;
}

sw_xmlnode_t *vars_extvar_get_node(sw_xmltree_t *pxml, sw_xmlnode_t *pnode, char *express)
{
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode2 = NULL;
	sw_xmlnode_t	*pnode_bak = NULL;

	if (express[0] >= '0' && express[0] <= '9')
	{
		/****常量索引****/
		int	i = 0;
		int	index = atoi(express);

		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (i == index)
			{
				break;
			}
			pnode1 = pnode1->next;
			i++;
		}
		if (pnode1 == NULL)
		{
			/****未找到指定结点****/
			return NULL;
		}
		return pnode1;
	}
	else
	{
		/****表达式****/
		size_t	com_pos = 0;
		int	equal_flag = 0;/**相等标志,0-相等,1-不等****/
		char	loc_buf[128];

		pnode_bak = pxml->current;
		pxml->current = pnode;
		while (express[com_pos] != '\0')
		{
			if (express[com_pos] == '!' || express[com_pos] == '=')
			{
				break;
			}
			com_pos++;
		}
		if (express[com_pos] == '!' && express[com_pos+1] != '=')
		{
			pxml->current = pnode_bak;
			return NULL;
		}
		if (com_pos > sizeof(loc_buf) - 1)
		{
			pxml->current = pnode_bak;
			return NULL;
		}
		memcpy(loc_buf, express, com_pos);
		loc_buf[com_pos] = '\0';
		if (express[com_pos] == '!')
		{
			com_pos += 2;
			equal_flag = 1;
		}
		else
		{
			com_pos++;
			equal_flag = 0;
		}
		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			pxml->current = pnode1;
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			pnode2 = pub_xml_locnode(pxml, loc_buf);
			if (pnode2 == NULL || pnode2->value == NULL)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (equal_flag == 0)
			{
				if (strcmp(pnode2->value, express + com_pos) == 0)
				{
					/****相等,找到****/
					break;
				}
				else
				{
					pnode1 = pnode1->next;
					continue;
				}
			}
			else if (equal_flag == 1)
			{
				if (strcmp(pnode2->value, express + com_pos) != 0)
				{
					/****不等,找到****/
					break;
				}
				else
				{
					pnode1 = pnode1->next;
					continue;
				}
			}
			pnode1 = pnode1->next;
		}
		pxml->current = pnode_bak;
		if (pnode1 != NULL)
		{
			return pnode1;
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}

sw_xmlnode_t *poGetXmlVar(sw_xmltree_t *xml, char *name)
{
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode2 = NULL;
	sw_xmlnode_t	*pnode_bak = NULL;
	int	pos = 0;
	int	pre = 0;

	if (name[0] == '.')
	{
		xml->current = xml->root;
	}

	pos = pre = 0;
	pnode_bak = xml->current;
	while (name[pos] != '\0')
	{
		if (name[pos] == '(')
		{
			while (name[pos] != '\0')
			{
				if (name[pos]!=')')
				{
					pos++;
					continue;
				}
				else
				{
					break;
				}
			}
			if (name[pos] != ')')
			{
				pub_log_error("[%s][%d] 路径表达式[%s]错误",
					__FILE__, __LINE__, name);
				xml->current = pnode_bak;
				return NULL;
			}
		}	
		pos++;
		if (name[pos] == '.' || name[pos] == '\0')
		{
			/****一个变量取出****/
			char cVarName[1024];
			char cExpress[1024];
			if (name[pos-1] == ')')
			{
				/****带表达式变量****/
				int i = pre;
				while (i < pos)
				{
					if (name[i] == '(')
					{
						break;
					}
					cVarName[i-pre] = name[i];
					i++;
				}
				if(i<pos)
				{
					cVarName[i-pre] = '\0';
					memcpy(cExpress, name + i + 1, pos - i - 2);
					cExpress[pos - i - 2] = '\0';
				}
				else
				{
					pub_log_error("[%s][%d] 变量表达式错误[%s]",
						__FILE__, __LINE__, name);
					xml->current = pnode_bak;
					return NULL;
				}
			}
			else
			{
				/****不带表达式变量****/
				memcpy(cVarName, name + pre, pos - pre);	
				cVarName[pos - pre] = '\0';
				cExpress[0] = '\0';
			}
			zip_space(cExpress);
			pnode1 = pub_xml_locnode(xml, cVarName);	
			if (pnode1 != NULL && cExpress[0] != '\0')
			{
				/****有表达式****/
				pnode1 = vars_extvar_get_node(xml, pnode1, cExpress);
			}
			if (pnode1 == NULL)
			{
				xml->current = pnode_bak;
				pub_log_error("[%s][%d] 未找到[%s][%s]",
					__FILE__, __LINE__, name, cVarName);
				return NULL;
			}
			/****找到指定结点****/
			xml->current = pnode1;
			/******/
			if (name[pos] == '\0')
			{
				break;
			}
			pos++;
			pre = pos;
		}
	}
	pnode1 = xml->current;
	return pnode1;
}

int iGetXmlVariable(sw_xmltree_t *xml, char *varname, char *value)
{
	sw_xmlnode_t     *node1, *node2;
	node1= poGetXmlVar(xml, varname);
	if(node1 != NULL)
	{
		strcpy(value, node1->value);
		return 0;
	}
	else
	{
		return -1;
	}
}


/****************************************************
* 函 数 名:	iSetXmlVariable
* 函数功能:	XML树增加节点(属性)
* 作    者:	MaWeiwei
* 日    期:	2011-11-22
* 输入参数:
*		   pxml     XML树
*          varname  变量名称
*          value    
*          type     节点类型 '1': 属性 非'1':子节点 
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
*****************************************************/
int iSetXmlVariable(sw_xmltree_t *pxml, char * varname, char *value)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	char	*ptIn = NULL;
	char	cFlag='1'; /*'0' 绝对路径 '1'相对路径*/
	char	sOut[100];
	
	ptIn = varname;
	if (varname[0] == '.')
	{
		ptIn = ptIn+1;
		cFlag = '1';	
	}
	
	while (ptIn != NULL)
	{
		memset(sOut, 0x0, sizeof(sOut));
		if ('1' == cFlag)
		{
			sOut[0]='.';
			ptIn = msstrtok(ptIn, sOut + 1, ".");
			cFlag = '0';
		}
		else
		{
			ptIn = msstrtok(ptIn, sOut, ".");
		}
		/*** 如果值为空则不创建路径 ***/
		if (value == NULL || value[0] == '\0')
		{
			node1 = poLocGetXmlVar(pxml, sOut, 0);
		}
		else
		{
			node1 = poLocGetXmlVar(pxml, sOut, 1);
		}
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] [%s]值为空,不创建路径",
				__FILE__,__LINE__, varname);
			return 0;
		}	
		/***
		sysLog(LOG_DEBUG,"[%s][%d] 创建[%s]成功",__FILE__, __LINE__, sOut);
		***/
	}
 	
 	if (node1->value != NULL)
 	{
 		free(node1->value);
 	}
 	node1->value = calloc(1, strlen(value) + 1);
 	strcpy(node1->value, value);
			
	return 0;
}

sw_xmlnode_t *poLocGetXmlVar(sw_xmltree_t *pxml, char *name, int creat)
{
	int	pos = 0;
	int	pre = 0;
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode2 = NULL;
	sw_xmlnode_t	*pnode_bak = NULL;

	if (pxml == NULL)
	{
		pub_log_error("[%s][%d] 创建XML树失败!", __FILE__, __LINE__);
		return NULL;
	}
	if (name[0] == '.')
	{
		pxml->current = pxml->root;
	}
	pos = pre = 0;
	pnode_bak = pxml->current;
	while (name[pos] != '\0')
	{
		if(name[pos] == '(')
		{
			while (name[pos] != '\0')
			{
				if (name[pos] != ')')
				{
					pos++;
					continue;
				}
				else
				{
					break;
				}
			}
			if (name[pos] != ')')
			{
				pub_log_error("[%s][%d] 路径表达式[%s]错误",
					__FILE__, __LINE__, name);
				pxml->current = pnode_bak;
				return NULL;
			}
		}	
		pos++;
		if (name[pos] == '.' || name[pos] == '\0')
		{
			/****一个变量取出****/
			char cVarName[1024];
			char cExpress[1024];
			if (name[pos-1] == ')')
			{
				/****带表达式变量****/
				int i = pre;
				while (i < pos)
				{
					if (name[i] == '(')
					{
						break;
					}
					cVarName[i-pre] = name[i];
					i++;
				}
				if (i < pos)
				{
					cVarName[i-pre] = '\0';
					memcpy(cExpress, name + i + 1, pos - i- 2);
					cExpress[pos - i - 2] = '\0';
				}
				else
				{
					pub_log_error("[%s][%d]变量表达式错误[%s]",
						__FILE__,__LINE__,name);
					pxml->current = pnode_bak;
					return NULL;
				}
			}
			else
			{
				/****不带表达式变量****/
				memcpy(cVarName, name + pre, pos - pre);	
				cVarName[pos-pre] = '\0';
				cExpress[0] = '\0';
			}
			zip_space(cExpress);
			pnode1 = pub_xml_locnode(pxml, cVarName);	
			if (pnode1 != NULL && cExpress[0] != '\0')
			{
				/****有表达式****/
				pnode1 = vars_extvar_get_node(pxml, pnode1, cExpress);
			}
			if (pnode1 == NULL)
			{
				if (name[pos] == '\0' && creat)
				{
					/****最底层结点且指定了创建标志****/
					/***
					pub_log_error("[%s][%d] 未找到[%s]但程序指定了创建标志",
						__FILE__, __LINE__, cVarName);
					pub_log_error("[%s][%d] 当前节点[%s]", 
						__FILE__, __LINE__, pxml->current->name);
					***/
					if(cVarName[0] != '.')
					{
						pnode2 = pub_xml_addnode(pxml->current, cVarName, "", SW_NODE_ELEMENT);
						pxml->current = pnode_bak;
					}
					else
					{
						pnode2 = pub_xml_addnode(pxml->current, cVarName + 1, "", SW_NODE_ELEMENT);
						pxml->current = pnode2;
					}
					/***
					pub_log_error("[%s][%d] 创建[%s]", __FILE__, __LINE__, pnode2->name);
					sysLog(LOG_DEBUG,"[%s][%d] XML树[%s]", __FILE__, __LINE__, pxml->current->name);
					***/
					pxml->current = pnode2;
					return pnode2;	
				}
				/****不是最底层变量或者没有指定创建标志****/
				pxml->current = pnode_bak;
				/***
				sysLog(LOG_DEBUG, "[%s][%d] name[pos]==[%s],[%d],creat[%d]",
					__FILE__, __LINE__, name[pos], pos, creat);
				pub_log_error("[%s][%d] 未找到[%s][%s]",
					__FILE__, __LINE__, name, cVarName);
				***/
				return NULL;
			}
			/****找到指定结点****/
			pxml->current = pnode1;

			if (name[pos] == '\0')
			{
				break;
			}
			pos++;
			pre = pos;
		}
	}
	pnode1 = pxml->current;

	return pnode1;
}

char *psGetAttrName(char *nodename, char *attrname)
{
	char    *ptr = NULL;
	char    *deststr = nodename;
	
	ptr = strrchr(nodename, '.');
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] 变量格式有误! nodename=[%s]", __FILE__, __LINE__, nodename);
		return NULL;
	}
	strcpy(attrname, ptr + 1);
	ptr = nodename + strlen(nodename) - 1;
	while (nodename <= ptr)
	{
		if (*ptr == '.')
		{
			*ptr = '\0';
			break;
		}
		ptr--;	
	}
	pub_log_info("[%s][%d]nodename=[%s] attrname=[%s]", 
		__FILE__, __LINE__, nodename, attrname);
	return deststr;
}

int  iSetXmlVarAttr(sw_xmltree_t *pxml, sw_xmlnode_t *pvar, char *attr, char *avalue)
{
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode_bak = NULL;

	if (pvar == NULL)
	{
		return -1;
	}
	pnode_bak = pxml->current;
	pxml->current = (sw_xmlnode_t *)pvar;
	pnode1 = pub_xml_locnode(pxml, attr);
	if (pnode1 == NULL)
	{
		pnode1 = pub_xml_addnode(pxml->current, attr, avalue, SW_NODE_ATTRIB);
		pxml->current = pnode_bak;
		return 0;
	}
	if (pnode1->value != NULL)
	{
		free(pnode1->value);
	}
	pnode1->value = (char *)malloc(strlen(avalue) + 1);
	strcpy(pnode1->value, avalue);
	pxml->current = pnode_bak;
	return 0;
}

int iSetXmlAttrVar(sw_xmltree_t *pxml, char *varname, char *attrvalue)
{
	char	*ptIn = NULL;
	char	cFlag = '1'; 
	char	sOut[256];
	char	sNode[256];
	char	sNodeName[256];
	char	sAttrName[256];
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(sOut, 0x0, sizeof(sOut));
	memset(sNode, 0x0, sizeof(sNode));
	memset(sNodeName, 0x0, sizeof(sNodeName));
	memset(sAttrName, 0x0, sizeof(sAttrName));
	memcpy(sNodeName, varname, sizeof(sNodeName) - 1);
	psGetAttrName(sNodeName, sAttrName);
	pub_log_info("[%s][%d] 节点名称为=[%s] 属性名称为=[%s]",
		__FILE__, __LINE__, sNodeName, sAttrName);
	ptIn = sNodeName;
	if (sNodeName[0] == '.')
	{
		ptIn = msstrtok(ptIn, sOut, ".");
		cFlag = '1';
	}	
	while (ptIn != NULL)
	{
		memset(sOut, 0x0, sizeof(sOut));
		sOut[0] = '.';
		ptIn = msstrtok(ptIn, sOut + 1, ".");
		if ('1' == cFlag)
		{
			cFlag = '0';
		}
		strcat(sNode, sOut);
		node1 = poLocGetXmlVar(pxml, sNode, 1);
	}
	iSetXmlVarAttr(pxml, node1, sAttrName, attrvalue);
	return 0;
}

char *pcGetXmlVarAttr(sw_xmltree_t *xml, sw_xmlnode_t *pvar, char *attr)
{
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode_bak;

	if (pvar == NULL)
	{
		return NULL;
	}
	pnode_bak = xml->current;
	xml->current = pvar;
	pnode1 = pub_xml_locnode(xml, attr);
	if (pnode1 == NULL)
	{
		pub_log_info("[%s][%d] AAAAAAAAAAA", __FILE__, __LINE__);
		xml->current = pnode_bak;
		return NULL;
	}
	xml->current = pnode_bak;
	pub_log_info("[%s][%d] attr=[%s] value=[%s]", __FILE__, __LINE__, attr, pnode1->value);
	return pnode1->value;
}

/************************************
函 数 名： iGetXmlAttrVar   
参    数： 
         varname 标签名 
         attrvalue 属性的值
功    能： 得到指定xml标签的属性值
************************************/
int iGetXmlAttrVar(sw_xmltree_t *xml, char *varname, char *attrvalue)
{
	char	sNodeName[256];
	char	sAttrName[256];
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	memset(sNodeName, 0x0, sizeof(sNodeName));
	memset(sAttrName, 0x0, sizeof(sAttrName));
	strcpy(sNodeName, varname);
	psGetAttrName(sNodeName, sAttrName);
	node_bak = xml->current;
	node1 = poLocGetXmlVar(xml, sNodeName, 0);
	if (node1 != NULL)
	{
		strcpy(attrvalue, pcGetXmlVarAttr(xml, node1, sAttrName));
		xml->current = node_bak;
		return 0;
	}
	else
	{
		xml->current = node_bak;
		return -1;
	}
}
sw_xmlnode_t *pstLocGetExtVar(sw_loc_vars_t *vars, char *name, int creat)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	sw_int32_t	pos = 0;
	sw_int32_t	pre = 0;

	if (vars == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("%s, %d, Param error."
						,__FILE__, __LINE__);
		return NULL;
	}
	
	if(vars->tree == NULL)
	{
		vars->tree = pub_xml_unpack(NULL);
	}
	
	if(vars->tree == NULL)
	{
		return NULL;
	}
	
	if(name[0]=='.')
	{
	 	vars->tree->current = vars->tree->root;
  	}
  	
	pos = pre = 0;
	
	node_bak = vars->tree->current;
	
	while (name[pos] != '\0')
	{
		if(name[pos] == '(')
		{
			while (name[pos] != '\0')
			{
				if(name[pos] != ')')
				{
					pos++;
					continue;
				}
				else
				{
					break;
				}
			}
			
			if (name[pos]!=')')
			{
				vars->tree->current = node_bak;
				return NULL;
			}
		}
		
		pos++;
		if (name[pos] == '.' || name[pos] == '\0')
		{
			char var_name[1024];
			char experss[1024];
			if (name[pos-1]==')')
			{

				sw_int_t	i = pre;
				
				while (i < pos)
				{
					if(name[i]=='(')
					{
						break;
					}
					
					var_name[i-pre] = name[i];
					i++;
				}
				
				if(i < pos)
				{
					var_name[i - pre] = '\0';
					pub_mem_memcpy(experss, name + i + 1, pos - i - 2);
					experss[pos - i - 2] = '\0';
				}
				else
				{
					vars->tree->current = node_bak;
					return NULL;
				}
			}
			else
			{
				pub_mem_memcpy(var_name, name + pre, pos - pre);
				var_name[pos - pre] = '\0';
				experss[0] = '\0';
			}
			
			pub_str_zipspace((u_char*)experss);
			node1 = pub_xml_locnode(vars->tree, var_name);
			if(node1 != NULL && experss[0] != '\0')
			{
				node1 = vars_extvar_get_node(vars->tree, node1, experss);
			}
			
			if (node1 == NULL)
			{
				if (name[pos] == '\0' && creat)
				{
					if (var_name[0] != '.')
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name, "",SW_NODE_ELEMENT);
						vars->tree->current = node_bak;
					}
					else
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name + 1, "",SW_NODE_ELEMENT);
						vars->tree->current = node2;
					}
					
					return node2;
				}
		
				vars->tree->current = node_bak;
				
				return NULL;
			}
			
			vars->tree->current = node1;
			
			if(name[pos] == '\0')
			{
				break;
			}
			
			pos++;
			pre = pos;
		}
	}
	
	node1 = vars->tree->current;
	vars->tree->current = node_bak;
	
	return node1;
}

char * psLocGetExtVarAttr(sw_loc_vars_t *vars, sw_xmlnode_t *node, char *attr)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	if (vars == NULL || node == NULL || attr == NULL)
	{
		return NULL;
	}
	
	node_bak = vars->tree->current;
	vars->tree->current = (sw_xmlnode_t *)node;
	
	node1 = pub_xml_locnode(vars->tree, attr);
	if(node1 == NULL)
	{
		vars->tree->current = node_bak;
		return NULL;
	}
	
	vars->tree->current = node_bak;
	
	return node1->value;
}
sw_char_t *pcGetExtVarAttr(sw_xmlnode_t *pvar,char *attr)
{
	sw_loc_vars_t *vars = NULL;
	
	if (pvar == NULL)
	{
		return NULL;
	}
	
	vars = pub_get_global_vars();
	if (vars == NULL)
	{
		pub_log_error("[%s][%d] vars is null!", __FILE__, __LINE__);
		return NULL;
	}
	
	return psLocGetExtVarAttr(vars, pvar, attr);
}

sw_xmlnode_t *poGetExtVar(char *name,int creat)
{
	sw_loc_vars_t *vars = NULL;
	
	vars = pub_get_global_vars();
	if (vars == NULL)
	{
		pub_log_error("[%s][%d] vars is null!", __FILE__, __LINE__);
		return NULL;
	}
	
	return pstLocGetExtVar(vars, name, creat);
}


int iGetXmlVar(sw_loc_vars_t *locvars, char *varname, char *varvalue)
{
	sw_xmlnode_t	*pnode = NULL;
	
	if (varname == NULL || varvalue == NULL)
	{
		pub_log_error("[%s][%d] 输入参数有误!", __FILE__, __LINE__);
		return -1;
	}
	if (locvars == NULL)
	{
		pnode = poGetExtVar(varname, 0);
	}
	else
	{
		pnode = pstLocGetExtVar(locvars, varname, 0);
	}
	if (pnode != NULL)
	{
		strcpy(varvalue, pnode->value);
		return 0;
	}
	return -1;
}

/************************************
函 数 名： iGetXmlAttr   
参    数： 
         varname 标签名 
         attrvalue 属性的值
功    能： 得到指定xml标签的属性值
************************************/
int iGetXmlAttr(sw_loc_vars_t *locvars, char *varname, char *attrvalue)
{
	char	sNodeName[256];
	char	sAttrName[256];
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	memset(sNodeName, 0x0, sizeof(sNodeName));
	memset(sAttrName, 0x0, sizeof(sAttrName));
	strcpy(sNodeName, varname);
	psGetAttrName(sNodeName, sAttrName);
	pub_log_info("[%s][%d] sNodeName=[%s] sAttrName=[%s]", 
		__FILE__, __LINE__, sNodeName, sAttrName);
	if (locvars == NULL)
	{
		node1 = poGetExtVar(sNodeName, 0);
		if (node1 != NULL)
		{
			strcpy(attrvalue, pcGetExtVarAttr(node1, sAttrName));
			return 0;
		}
	}
	else
	{
		node1 = pstLocGetExtVar(locvars, sNodeName, 0);
		if (node1 != NULL)
		{
			strcpy(attrvalue, (char *)psLocGetExtVarAttr(locvars, node1, sAttrName));
			return 0;
		}
	}
	
	return -1;
}

int iCheckTx(char *sVarvalue, char *value)
{
	int	i = 0;
	int	n = 0;
	char	cInc[512];
	char	cTmp[512];
	
	memset(cTmp, 0x0, sizeof(cTmp));
	memset(cInc, 0x0, sizeof(cInc));
	
	if (value[0] == '~')
	{
		return pub_regex_match(sVarvalue, value);
	}
	
	strcpy(cTmp, value);
	while (cTmp[i] != 0)
	{

		if (cTmp[i] != ' ')
		{
			cInc[n] = cTmp[i];
			n++;
			i++;
		}
		else
		{
			cInc[n] = '\0';
			n = 0;
			i++;
			if (strcmp(sVarvalue, cInc) == 0)
			{
				return 0;
			}
			else
			{
				memset(cInc, 0x0, sizeof(cInc));
			}
		}
	}
	if (strcmp(sVarvalue, cInc) == 0)
	{
		return 0;
	}
	else
	{
		pub_log_debug("[%s][%d] CHECK检查[%s]未通过!\n",
			__FILE__, __LINE__, value);
		return -1;
	}
}

/*****************************************
 函  数  名: zip_space
 功      能: 压缩字符串中的所有空格及制表符
 
 输      入: str 源字符串
 输      出: str 目标字符串
******************************************/
char *zip_space(char *str)
{
	char	*ptr;
	char	*destr;
	
	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			*ptr++ = *str;
		}
		str++;
	}
	*ptr = '\0';

	return destr;
}

/******************************************
 函  数  名: zip_head
 功      能: 压缩字符串头部的所有空格及制表符
 
 输      入: str 源字符串
 输      出: str 目标字符串
******************************************/
char *zip_head(char *str)
{
	char	*ptr;
	char	*destr;
	
	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			break;
		}
		str++;
	}
	while (*str != '\0')
	{
		*ptr++ = *str++;
	}
	*ptr = '\0';
	
	return destr;
}

/******************************************
 函  数  名: zip_tail
 功      能: 压缩字符串尾部的所有空格及制表符
 
 输      入: str 源字符串
 输      出: str 目标字符串
******************************************/
char *zip_tail(char *str)
{
	char	*ptr;
	char	*destr;

	destr = str;
	ptr = str + strlen(str) - 1;
	while (str <= ptr)
	{
		if ((*ptr != ' ') && (*ptr != '\t'))
		{
			ptr++;
			*ptr = '\0';
			break;
		}
		ptr--;
	}
	
	return destr;
}

/******************************************
 函  数  名: zip_bothsides
 功      能: 压缩字符串两端的所有空格及制表符
 
 输      入: str 源字符串
 输      出: str 目标字符串
******************************************/
char *zip_bothsides(char *str)
{
	char	*ptr;
	char	*tptr;
	char	*destr;

	tptr = str + strlen(str) - 1;
	ptr = str;
	destr = str;
	while (str <= tptr)
	{
		if ((*tptr != ' ') && (*tptr != '\t'))
		{
			tptr++;
			*tptr = '\0';
			break;
		}
		tptr--;
	}
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			break;
		}
		str++;
	}
	while (*str != '\0')
	{
		*ptr++ = *str++;
	}
	*ptr = '\0';
	
	return destr;
}


int iDate8To10(char *sDate8, char *sDate10)
{
	if (strlen(sDate8) != 8)
	{
		pub_log_error("[%s][%d] 日期格式有误! date=[%s]\n",
			__FILE__, __LINE__, sDate8);
		return -1;
	}
	pub_log_info("[%s][%d] 日期=[%s]", __FILE__, __LINE__, sDate8);
	memcpy(sDate10, sDate8, 4);
	memcpy(sDate10 + 4, "-", 1);
	memcpy(sDate10 + 5, sDate8 + 4, 2);
	memcpy(sDate10 + 7, "-", 1);
	memcpy(sDate10 + 8, sDate8 + 6, 4);
	
	return 0;
}

int iDate10To8(char *sDate10, char *sDate8)
{
	if (strlen(sDate10) != 10)
	{
		pub_log_error("[%s][%d] 日期格式有误! date=[%s]\n",
			__FILE__, __LINE__, sDate10);
		return -1;
	}
	pub_log_info("[%s][%d] 日期=[%s]", __FILE__, __LINE__, sDate10);
	memcpy(sDate8, sDate10, 4);
	memcpy(sDate8 + 4, sDate10 + 5, 2);
	memcpy(sDate8 + 6, sDate10 + 8, 2);
	
	return 0;
}

int iDoubleToNum(double amt, char *value)
{
	char    sBuf[256];
	long 	lBuf = 0L;
	char    *ptr = NULL;
	char    *pTmp = NULL;
	
	memset(sBuf, 0x0, sizeof(sBuf));
	sprintf(sBuf, "%.2f", amt*100);
	ptr = sBuf;
	pTmp = sBuf;
	
	pTmp = strchr(sBuf, '.');
	if (pTmp != NULL)
	{	
		memcpy(value, sBuf, pTmp - ptr);
		
	}
	else
	{
		strcpy(value, sBuf);
	}
	return 0;
}

int iNumToDouble(char *pIn, char *pOut)
{
	char    sBuf[64];
	double  dValue = 0.00;
	
	memset(sBuf, 0x0, sizeof(sBuf));
	dValue = strtod(pIn, NULL);
	
	sprintf(sBuf, "%.2f", dValue / 100.00);
	strcpy(pOut, sBuf);
	
	return 0;
}

int iLocChkinclude(sw_loc_vars_t *locvars, char *varname, char *value)
{
	
	int	i = 0;
	int	n = 0;
	int	ilen = 0;
	char	sInc[11];
	char	sTmp[512];
	char	sVarValue[1024];
	
	memset(sInc, 0x0, sizeof(sInc));
	memset(sTmp, 0x0, sizeof(sTmp));
	memset(sVarValue, 0x0, sizeof(sVarValue));
	
	strcpy(sTmp, value);
	ilen = strlen(value);
	
	if (varname == NULL || varname[0] == '\0')
	{
		if (locvars == NULL)
		{
			get_zd_data("#tx_code", sVarValue);
		}
		else
		{
			loc_get_zd_data(locvars, "#tx_code", sVarValue);
		}
		pub_log_info("[%s][%d] FIRST: NAME=[#tx_code] VALUE=[%s]",
			__FILE__, __LINE__, sVarValue);
	}
	else
	{
		if (locvars == NULL)
		{
			get_zd_data(varname, sVarValue);
		}
		else
		{
			loc_get_zd_data(locvars, varname, sVarValue);
		}
		pub_log_info("[%s][%d] FIRST: NAME=[%s] VALUE=[%s]",
			__FILE__, __LINE__, varname, sVarValue);
	}
	
	pub_log_info("[%s][%d] CHECKVALUE===[%s]", __FILE__, __LINE__, sTmp);
	
	if (sTmp[0] == '~')
	{	
		return pub_regex_match(sVarValue, sTmp);	
	}

	while (sTmp[i] != 0)
	{

		if (sTmp[i] != ' ')
		{
			sInc[n] = sTmp[i];
			n++;
			i++;
		}
		else
		{
			sInc[n] = '\0';
			n = 0;
			i++;

			if (strcmp(sVarValue, sInc) == 0)
			{
				return 0;
			}
			else
			{
				memset(sInc, 0, sizeof(sInc));
			}
		}
	}
	if (strcmp(sVarValue, sInc) == 0)
	{
		return 0;
	}
	else
	{
		pub_log_info("[%s][%d] INCLUDE检查未通过! CHECKVALUE=[%s] VALUE=[%s]",
			__FILE__, __LINE__, value, sVarValue);
		return -1;
	}
}

/****************************************************
* 函 数 名:	iSetXmlVariable
* 函数功能:	XML树增加节点(属性)
* 作    者:	MaWeiwei
* 日    期:	2011-11-22
* 输入参数:
*		   pxml     XML树
*          varname  变量名称
*          value    
*          type     节点类型 '1': 属性 非'1':子节点 
* 输出参数:
*          无
* 返 回 值:  	0 成功, －1 失败
*****************************************************/
int iSetLargeVariable(sw_xmltree_t *pxml, char * varname, char *value)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	char	*ptIn = NULL;
	char	cFlag='1'; /*'0' 绝对路径 '1'相对路径*/
	char	sOut[100];
	int	iLen = 0;
	FILE    *fp = NULL;
	struct  timeval tv;
	char    sBuf[128];
	char    sFileName[128];
	
	memset(sBuf, 0x0, sizeof(sBuf));
	memset(sFileName, 0x0, sizeof(sFileName));
	memset(&tv, 0x0, sizeof(tv));
	
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
	iLen = strlen(value);
	fwrite(value, iLen, 1, fp);
	fclose(fp);

	memset(sBuf, 0x0, sizeof(sBuf));
	sprintf(sBuf, "<LARGE>%08d%s", iLen, sFileName);
	
	ptIn = varname;
	if (varname[0] == '.')
	{
		ptIn = ptIn+1;
		cFlag = '1';	
	}
	
	while (ptIn != NULL)
	{
		memset(sOut, 0x0, sizeof(sOut));
		if ('1' == cFlag)
		{
			sOut[0]='.';
			ptIn = msstrtok(ptIn, sOut + 1, ".");
			cFlag = '0';
		}
		else
		{
			ptIn = msstrtok(ptIn, sOut, ".");
		}
		/*** 如果值为空则不创建路径 ***/
		if (value == NULL || value[0] == '\0')
		{
			node1 = poLocGetXmlVar(pxml, sOut, 0);
		}
		else
		{
			node1 = poLocGetXmlVar(pxml, sOut, 1);
		}
		if (node1 == NULL)
		{
			pub_log_error("[%s][%d] [%s]值为空,不创建路径",
				__FILE__,__LINE__, varname);
			return 0;
		}	
		/***
		sysLog(LOG_DEBUG,"[%s][%d] 创建[%s]成功",__FILE__, __LINE__, sOut);
		***/
	}
 	
 	if (node1->value != NULL)
 	{
 		free(node1->value);
 	}
 	node1->value = calloc(1, strlen(sBuf) + 1);
 	strcpy(node1->value, sBuf);
			
	return 0;
}

void s_initxmlnode(sw_xmlnode_t *pstXmlNode)
{
	pstXmlNode->next = NULL;
	pstXmlNode->firstchild = NULL;
	pstXmlNode->name = NULL;
	pstXmlNode->value = NULL;
	pstXmlNode->parent = NULL;
	pstXmlNode->node_type = 0;
}

int iLocCheck(sw_loc_vars_t *locvars, char *value)
{
	int i = 0;
	char cChkname[21];
	char cChkvalue[21];
	char cTmpchk[21];
	char cSign[3];
	
	memset(cTmpchk,0,sizeof(cTmpchk));
	memset(cChkname,0,sizeof(cChkname));
	memset(cChkvalue,0,sizeof(cChkvalue));
	memset(cSign,0,sizeof(cSign));
	strcpy(cChkvalue,value);
	
	while (cChkvalue[i] != '=' && cChkvalue[i] != '?')
	{
		cChkname[i] = value[i];
		i++;
		 
		if (cChkvalue[i] == 0)
		{
			pub_log_error("%s,%d, CHECK格式出错",__FILE__,__LINE__);
			return -1;
		}
	}
	
	loc_get_zd_data(locvars, cChkname, cTmpchk);
	i++;
	
	if (value[i - 1] == '?')
	{
		strcpy(cChkvalue,value + i);
		strcpy(cSign, "?");
	}
	else
	{
		strcpy(cChkvalue, value + i);
		strcpy(cSign, "=");
	}

	if (!strcmp(cSign, "="))
	{
		if(!strcmp(cChkvalue,cTmpchk))
		{
			return 0;
		}
		else
		{
			pub_log_error("%s,%d,  CHECK检查[%s][%s=%s]未通过!\n",__FILE__,__LINE__,value,cChkvalue,cTmpchk);
			return -1;
		}
	}
	else if(!strcmp(cSign,"?"))
	{
		if(!strcmp(cChkvalue,cTmpchk))
		{
			pub_log_error("%s,%d, CHECK检查[%s]未通过!\n",__FILE__,__LINE__,value);
			return -1;
		}
		else
		{
			return 0;
		}
	}
 }

/***********************************************************
*函数名: pstGetNodeByTarget
*作者 ：liteng
*功能  : 得到指定xmltree中指定标签名入口节点
*参数  : S_XMLTREE *xmltree	指定xml文件tree取当前节点
	psTargetName,		标签名
	char sTarg[64]		标签名 NAME
        char sName[64]      	需要查找的NAME 如：listen
*日期： 2011年3月24日
*返回值:入口节点的xml树节点位置   如 返回标签位置
**********************************************************/
sw_xmlnode_t *pstGetNodeByTarget(sw_xmltree_t *xmltree,char *psTargetName,char sTarg[64],char sName[64])
{
	sw_xmlnode_t *poBackCurrent;
	sw_xmlnode_t *poBackPnode1;
	sw_xmlnode_t *pnode1;
	sw_xmlnode_t *pnode2;

	pnode1 = NULL;
	pnode2 = NULL;
	poBackPnode1 = NULL;
	poBackCurrent = NULL;

	if(xmltree == NULL || sName == NULL || strlen(sName) == 0
		|| sTarg == NULL||strlen(sTarg) == 0 ||
		psTargetName == NULL || strlen(psTargetName) == 0)
	{
		return(NULL);
	}
	
	poBackCurrent = xmltree->current;
	pnode1 = xmltree->current;
	while (pnode1 != NULL)
	{
		xmltree->current = pnode1;
		poBackPnode1 = xmltree->current;
		pnode2 = pub_xml_locnode(xmltree, sTarg);
		if (pnode2 == NULL)
		{
			pnode1=pnode1->next;
		}
		else if ((strcmp(pnode2->value, sName)==0)&&
			(strcmp(pnode1->name, psTargetName)==0))
		{
			break;
		}
		else
		{
			pnode1=pnode1->next;
		}
	}
	xmltree->current = pnode1;

	if (pnode1 != NULL)
	{
		xmltree->current = poBackCurrent;
		return (pnode1);
	}
	else
	{
		xmltree->current=poBackCurrent;
		return(NULL);
	}
}

int nGetpkgloc(char *value)
{
	int i = 1,n = 0;
	int ipkglen = 0;
	char  cbegin[3],cend[3];
	
	memset(cbegin, 0, sizeof(cbegin));
	memset(cend, 0, sizeof(cend));
	
	if(value[0] != '[')
	{
		ipkglen = atoi(value);
	}
	else
	{
		while(value[i]!='.')
		{
 			cbegin[i - 1] = value[i];
			i++;
		}
		
		i += 2;
		while(value[i] != ']')
		{
			cend[n] = value[i];
			i++;
			n++;
		}
		
		ipkglen = atoi(cend) - atoi(cbegin);
	}
	
	return ipkglen;
 }

