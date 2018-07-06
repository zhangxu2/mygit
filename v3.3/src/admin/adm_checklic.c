#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/utsname.h>
#include "pub_log.h"
#include "pub_xml.h"
#include "pub_buf.h"

extern sw_xmltree_t *cfg_read_xml(char *filename);
extern int iMd5ToCh32(char * input, unsigned char * output, int len);

static int packencrypt(char *str, int len)
{
	int	i = 0;
	int	klen = 0;
	char	key[33];

	memset(key, 0x0, sizeof(key));
	strcpy(key, "DFISBP");
	klen = strlen(key);

	for (i = 0; i < len; i++)
	{
		str[i] = str[i] + key[i % klen];
	}

	return 0;
}

int check_name(char *prdt)
{
	int	len = 0;
	int	flag = 0;
	FILE	*fp = NULL;
	char	enc[128];
	char	lic[128];
	char	line[1024];
	char	prdtinfo[256];
	struct utsname	uts;
	
	memset(enc, 0x0, sizeof(enc));
	memset(lic, 0x0, sizeof(lic));
	memset(line, 0x0, sizeof(line));
	memset(prdtinfo, 0x0, sizeof(prdtinfo));
	memset(&uts, 0x0, sizeof(uts));
	
	uname(&uts);
	sprintf(prdtinfo, "%s%s", uts.machine, prdt);
	len = strlen(prdtinfo);
	packencrypt(prdtinfo, len);
	iMd5ToCh32(prdtinfo, (u_char *)enc, len); 
	sprintf(lic, "%s/cfg/.license.dat", getenv("SWWORK"));
	fp = fopen(lic, "rb");
	if (fp == NULL)
	{
		printf("打开许可文件[%s]失败!\n", lic);
		pub_log_error("打开许可文件[%s]失败! errno=[%d]:[%s]", __FILE__, __LINE__, lic, errno, strerror(errno));
		return -1;
	}
	while (1)
	{
		memset(line, 0x0, sizeof(line));
		if (fgets(line, sizeof(line), fp) == NULL)
		{
			break;
		}
		len = strlen(line);
		if (line[len - 1] == '\n')
		{
			line[len - 1] = '\0';
		}

		if (strcmp(line, enc) == 0)
		{
			flag = 1;
			break;
		}
	}
	fclose(fp);
	
	if (flag == 0)
	{
		pub_log_error("[%s][%d] Product [%s] not regist!", __FILE__, __LINE__, prdt);
		return -1;
	}

	return 0;
}

int check_regist()
{
	int	flag = 0;
	char	prdt[64];
	char	filename[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(prdt, 0x0, sizeof(prdt));
	memset(filename, 0x0, sizeof(filename));
	
	
	sprintf(filename, "%s/cfg/products.xml", getenv("SWWORK"));
	xml = cfg_read_xml(filename);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
			__FILE__, __LINE__, filename);
		return -1;
	}
	
	flag = 0;
	node = pub_xml_locnode(xml, ".DFISBP.PRODUCT");
	while (node != NULL)
	{
		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}
	
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] producsts.xml not config NAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		pub_log_info("[%s][%d] prdt:[%s]", __FILE__, __LINE__, node1->value);
		
		if (strcmp(node1->value, "O_APP") == 0 || strcmp(node1->value, "OMP") == 0)
		{
			node = node->next;
			continue;
		}

		if (check_name(node1->value) < 0)
		{
			flag = 1;
			printf("产品[%s]未注册,请先注册!\n", node1->value);
			pub_log_error("产品[%s]未注册,请先注册!", node1->value);
		}
			
		node = node->next;
	}
	pub_xml_deltree(xml);
	
	if (flag == 1)
	{
		pub_log_error("[%s][%d] 存在未注册的产品!", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}
