#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include "pub_xml.h"

extern int anet_get_hostip(char *outip);
extern int iMd5ToCh16(char * input, unsigned char * output, int len);

int gen_eswid(char *eswid)
{
	char	buf[128];
	char	outbuf[128];
	char	hostip[64];
	struct utsname	uts;
	
	memset(buf, 0x0, sizeof(buf));
	memset(outbuf, 0x0, sizeof(outbuf));
	memset(&uts, 0x0, sizeof(uts));

	if (uname(&uts) < 0)
	{
		pub_log_error("[%s][%d] uname error£¡ errno=[%d][%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	memset(hostip, 0x0, sizeof(hostip));
	if (anet_get_hostip(hostip) < 0)
	{
		pub_log_error("[%s][%d] Get host ip error!", __FILE__, __LINE__);
		return -1;
	}
	sprintf(buf, "%s%s%s", uts.machine, hostip, getenv("USER"));
	iMd5ToCh16(buf, (u_char *)outbuf, sizeof(outbuf));
	strcpy(eswid, outbuf);

	return 0;
}

int check_eswid()
{
	int	ret = 0;
	char	eswid[64];
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	memset(eswid, 0x0, sizeof(eswid));
	memset(xmlname, 0x0, sizeof(xmlname));

	ret = gen_eswid(eswid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] gen eswid error!", __FILE__, __LINE__);
		return -1;
	}

	sprintf(xmlname, "%s/cfg/swconfig.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xml tree error! xmlname=[%s]",
			__FILE__, __LINE__, xmlname);
		return -1;
	}

	node = pub_xml_locnode(xml, ".SWCFG.GLOBAL.ESWSID");
	if (node != NULL)
	{
		if (node->value != NULL)
		{
			if (strcmp(node->value, eswid) != 0)
			{
				pub_xml_set_value(node, eswid);
			}
		}
		else
		{
			pub_xml_set_value(node, eswid);
		}
		pub_xml_pack(xml, xmlname);
	}
	else
	{
		node = pub_xml_crnode(xml, ".SWCFG.GLOBAL.ESWSID", 0);
		if (node == NULL)
		{
			pub_log_error("[%s][%d] Create xml node error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		pub_xml_set_value(node, eswid);
		pub_xml_pack(xml, xmlname);
	}
	pub_xml_deltree(xml);

	return 0;
}

