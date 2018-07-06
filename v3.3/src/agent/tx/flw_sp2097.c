/*************************************************
  文 件 名:  flw_sp2097.c                        
  功能描述:  生成productList.tar                 
  作    者:  guxiaoxin                                    
  完成日期:  20160803                                    
 *************************************************/
#include "agent_comm.h"

#define MAX_PRDT	32
#define MAX_CHL		64

struct sw_prdt_s
{
	char	name[64];
	char	eng[64];
	char	desc[128];
	char	type[2];
	char	status[2];
};

typedef struct sw_prdt_s sw_prdt_t;

struct sw_chl_s
{
	char	eng[64];
	char	name[64];
	int		prdt_cnt;
	sw_prdt_t	prdts[MAX_PRDT];
};

typedef struct sw_chl_s sw_chl_t;

struct sw_prdts_info_s
{
	int	chl_cnt;
	sw_chl_t	chls[MAX_CHL];
};

typedef struct sw_prdts_info_s sw_prdts_info_t;
int get_prdt_status(sw_xmltree_t *xml, char *prdt_name, char *status)
{
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	if (xml == NULL || prdt_name == NULL || status == NULL)
	{
		pub_log_error("[%s][%d] input parameter error!",__FILE__,__LINE__);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.PRODUCT");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] No .DFISBP.PRODUCT !",__FILE__,__LINE__);
		return SW_ERROR;
	}

	while(node != NULL)
	{

		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] not config NAME!",__FILE__,__LINE__);
			return SW_ERROR;
		}
		else
		{
			if (strcmp(node1->value, prdt_name) == 0)
			{
				node2 = pub_xml_locnode(xml, "STATUS");
				if (node2 == NULL || node2->value == NULL || strlen(node2->value) == 0)
				{
					pub_log_error("[%s][%d] not config STATUS!",__FILE__,__LINE__);
					return SW_ERROR;					
				}
				else
				{
					strcpy(status, node2->value);
					break;
				}
			}
		}
		node = node->next;
	}
	return SW_OK;
}

int set_prdts_detail(sw_prdts_info_t *prdt_info, char *path)
{
	int		i = 0;
	int		j = 0;
	int		result = SW_ERROR;
	FILE	*fp  = NULL;
	char	*tmp = NULL;
	char	cmd[PATH_LEN];
	char	dir_path[PATH_LEN];
	char	file_path[PATH_LEN];
	char	xml_name[FILE_NAME_MAX_LEN] = {"productList.xml"};
	char	tar_name[FILE_NAME_MAX_LEN] = {"productList.tar"};

	if (prdt_info == NULL || path == NULL)
	{
		pub_log_error("[%s][%d] input parameter error!",__FILE__,__LINE__);
		return SW_ERROR;
	}

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("[%s][%d] getenv(SWWORK) error!",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(dir_path, sizeof(dir_path));
	sprintf(dir_path, "%s/tmp/cfg_bak", tmp);

	result = access(dir_path, F_OK);
	if (result != 0)
	{
		result = mkdir(dir_path, 0777);
		if (result != 0)
		{
			pub_log_error("[%s][%d] mkdir [%s], errno=[%d]:[%s].", 
							__FILE__, __LINE__, dir_path, errno, strerror(errno));
			return SW_ERROR;
		}
	}
	
	memset(file_path, 0x0, sizeof(file_path));
	sprintf(file_path, "%s/%s", dir_path, xml_name);
	fp = fopen(file_path, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Open file [%s] error, errno=[%d]:[%s].",
						__FILE__, __LINE__, file_path, errno, strerror(errno));
		return SW_ERROR;
	}

	fprintf(fp, "<?xml version= \"1.0\"  encoding= \"GBK\" ?>\n");
	fprintf(fp, "<DFISBP>\n");
	
	for (i = 0; i < prdt_info->chl_cnt; i++)
	{
		fprintf(fp, "\t<CHANNEL NAME=\"%s\" NAMEENG=\"%s\">\n",
					 prdt_info->chls[i].name, prdt_info->chls[i].eng);
		fprintf(fp, "\t\t<PRODUCTS>\n");

		for (j = 0; j < prdt_info->chls[i].prdt_cnt; j++)
		{
			fprintf(fp, "\t\t\t<PRODUCT NAME=\"%s\" NAMEENG=\"%s\" TYPE=\"%s\" DESC=\"%s\" STATUS=\"%s\"/>\n",
				prdt_info->chls[i].prdts[j].name, prdt_info->chls[i].prdts[j].eng,
				prdt_info->chls[i].prdts[j].type, prdt_info->chls[i].prdts[j].desc,
				prdt_info->chls[i].prdts[j].status);
		}

		fprintf(fp, "\t\t</PRODUCTS>\n");
		fprintf(fp, "\t</CHANNEL>\n");
	}

	fprintf(fp, "</DFISBP>\n");
	fclose(fp);
	fp = NULL;

	result = chdir(dir_path);
	if (result != 0)
	{
		pub_log_error("[%s][%d] chdir path[%s] error, errno=[%d]:[%s].",
						__FILE__, __LINE__, dir_path, errno, strerror(errno));
		return SW_ERROR;	
	}

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "tar cf %s %s 1>/dev/null 2>/dev/null", tar_name, xml_name);
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_error("[%s][%d] execute cmd[%s] error, errno=[%d]:[%s].",
						__FILE__, __LINE__, cmd, errno, strerror(errno));
		return SW_ERROR;		
	}

	sprintf(path, "%s/%s", dir_path, tar_name);
	return SW_OK;
}

int crt_prdts_detail(char *file_path)
{
	int		i = 0;
	int		j = 0;
	int		result = SW_ERROR;
	char	status[2];
	char	target[PATH_LEN];
	char	chl_path[PATH_LEN];
	char	prdt_path[PATH_LEN];
	char	*tmp = NULL;
	sw_xmltree_t	*chl_xml  = NULL;
	sw_xmltree_t	*prdt_xml = NULL;
	sw_xmlnode_t	*node  = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_prdts_info_t	prdt_info;

	if (file_path == NULL)
	{
		pub_log_error("[%s][%d] input parameter error.",__FILE__,__LINE__);
		return -1;
	}

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("[%s][%d] getenv SWWORK error",__FILE__,__LINE__);
		return -1;
	}

	pub_mem_memzero(chl_path, sizeof(chl_path));
	sprintf(chl_path, "%s/cfg/channels.xml", tmp);

	result = access(chl_path, F_OK);
	if (result != 0)
	{
		pub_log_error("[%s][%d] File[%s] not exist, errno=[%d]:[%s].",
						__FILE__, __LINE__, chl_path, errno, strerror(errno));
		return SW_ERROR;
	}

	chl_xml = pub_xml_crtree(chl_path);
	if (chl_xml == NULL)
	{
		pub_log_error("[%s][%d] Create xmltree error! xmlfile=[%s]", __FILE__, __LINE__, chl_path);
		return SW_ERROR;
	}
	
	pub_mem_memzero(prdt_path, sizeof(prdt_path));
	sprintf(prdt_path, "%s/cfg/products.xml", tmp);
	prdt_xml = pub_xml_crtree(prdt_path);
	if (chl_xml == NULL)
	{
		pub_log_error("[%s][%d] Create xmltree error, xmlfile=[%s]", __FILE__, __LINE__, prdt_path);
		pub_xml_deltree(chl_xml);
		return SW_ERROR;
	}

	node = pub_xml_locnode(chl_xml, ".DFISBP.CHANNEL");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] not config .DFISBP.CHANNEL!",__FILE__,__LINE__);
		pub_xml_deltree(chl_xml);
		pub_xml_deltree(prdt_xml);
		return SW_ERROR;
	}

	pub_mem_memzero(&prdt_info, sizeof(prdt_info));
	i = 0;
	while(node != NULL)
	{
		if (strcmp(node->name, "CHANNEL") != 0)
		{
			node = node->next;
			continue;
		}

		chl_xml->current = node;

		node1 = pub_xml_locnode(chl_xml, "LISTEN");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("[%s][%d] not config LISTEN!",__FILE__,__LINE__);
			pub_xml_deltree(chl_xml);
			pub_xml_deltree(prdt_xml);
			return SW_ERROR;
		}
		else
		{
			strcpy(prdt_info.chls[i].eng, node1->value);
		}

		node2 = pub_xml_locnode(chl_xml, "PRODUCT.NAME");
		j = 0;

		while (node2 != NULL)
		{
			if (strcmp(node2->name, "NAME") != 0)
			{
				node2 = node2->next;
				continue;
			}
			
			if (node2->value == NULL || strlen(node2->value) == 0)
			{
				pub_log_error("[%s][%d] not config PRODUCT.NAME's value!",__FILE__,__LINE__);
				pub_xml_deltree(chl_xml);
				pub_xml_deltree(prdt_xml);
				return SW_ERROR;
			}
			else
			{
				/*set the product's detail infomation.*/
				strcpy(prdt_info.chls[i].prdts[j].eng, node2->value);
				pub_mem_memzero(status, sizeof(status));
				result = get_prdt_status(prdt_xml, prdt_info.chls[i].prdts[j].eng, status);
				if (result != SW_OK)
				{
					pub_log_error("[%s][%d] get_prdt_status [%s] error!"
									 __FILE__, __LINE__, prdt_info.chls[i].prdts[j].name);
					pub_xml_deltree(chl_xml);
					pub_xml_deltree(prdt_xml);
					return SW_ERROR;
				}
				
				/*get product's status*/
				strcpy(prdt_info.chls[i].prdts[j].status, status);
			}
			j++;
			node2 = node2->next;
		}
		prdt_info.chls[i].prdt_cnt = j;
		i++;
		node = node->next;
	}

	prdt_info.chl_cnt = i;
	pub_mem_memzero(target, sizeof(target));
	result = set_prdts_detail(&prdt_info, target);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] set_prdts_detail error!", __FILE__,__LINE__);
		pub_xml_deltree(chl_xml);
		pub_xml_deltree(prdt_xml);
		return SW_ERROR;
	}

	pub_log_debug("[%s][%d] target[%s]",__FILE__,__LINE__,target);
	pub_xml_deltree(chl_xml);
	pub_xml_deltree(prdt_xml);

	/*strcpy(file_path, (char *)target + strlen(getenv("HOME")) + 1);20140325 by zoupei*/
	strcpy(file_path,target);
	
	return SW_OK;
}

int sp2097(sw_loc_vars_t *vars)
{
	int		result = SW_ERROR;
	char   	reply[8];
	char   	res_msg[256];
	char	file_path[PATH_LEN];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	memset(file_path, 0x0, sizeof(file_path));

	result = crt_prdts_detail(file_path);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] crt_prdts_detail fail!",__FILE__,__LINE__);
		strcpy(reply, "E999");
		strcpy(res_msg, "创建产品细节失败");
		goto ErrExit;
	}

	loc_set_zd_data(vars, ".TradeRecord.Response.Path", file_path);
	
OkExit:
	pub_log_debug("[%s][%d] deal end![END][OK]", __FILE__, __LINE__ );
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");
	return 0;

ErrExit:
	agt_error_info(reply, res_msg);
	pub_log_debug("[%s][%d] deal end![END][ERR]", __FILE__, __LINE__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode",  "E999");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage",  "Transaction processes failure");
	return -1;	
}

