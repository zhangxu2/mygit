#include "pub_log.h"
#include "pub_xml.h"
#include "pub_vars.h"
#include "lsn_pub.h"
#include <pthread.h>

extern void EXIT();

int pack_init(sw_loc_vars_t *vars, char *txcode)
{
	char	name[64];
	char	value[1024];
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	memset(xmlname, 0x0, sizeof(xmlname));
	
	sprintf(xmlname, "%s/cfg/common/autosend.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return -1;
	}
	
	node = pub_xml_locnode(xml, ".AUTOSEND.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
	
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			pub_log_info("[%s][%d] txcode=[%s] include=[%s]", __FILE__, __LINE__, txcode, node1->value);
			if (pub_str_include(txcode, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		break;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] 未找到交易[%s]的配置!", __FILE__, __LINE__, txcode);
		pub_xml_deltree(xml);
		return -1;
	}
	
	xml->current = node;
	node = pub_xml_locnode(xml, "ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置NAME标签!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		strncpy(name, node1->value, sizeof(name) - 1);
		
		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] 未配置VALUE标签!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		strncpy(value, node1->value, sizeof(value) - 1);
		
		loc_set_zd_data(vars, name, value);
		pub_log_info("[%s][%d] 设置变量[%s]=[%s]", __FILE__, __LINE__, name, value);
	
		node = node->next;
	}
	loc_set_zd_data(vars, "#tx_code", txcode);
	pub_xml_deltree(xml);
	pub_log_info("[%s][%d] 交易[%s]初始化完成!", __FILE__, __LINE__, txcode);
	
	return 0;
}	

int main(int argc, char **argv)
{
	int	ret = 0;
	int	port = 0;
	int	sockid = 0;
	char	*ptr = NULL;
	char	ip[64];
	char	data[32];
	char	tmp[128];
	char	txcode[32];
	char	xmltree[64];
	unsigned char	endflag[8];
	sw_buf_t	locbuf;
	sw_loc_vars_t	vars;
	sw_xmltree_t	*xml = NULL;
	sw_pkg_cache_list_t	pkg_cache;
	
	if (argc != 3)
	{
		printf("Usage: %s <txcode> <xmltree>\n", argv[0]);
		return -1;
	}
	
	memset(ip, 0x0, sizeof(ip));
	memset(data, 0x0, sizeof(data));
	memset(tmp, 0x0, sizeof(tmp));
	memset(txcode, 0x0, sizeof(txcode));
	memset(xmltree, 0x0, sizeof(xmltree));
	memset(endflag, 0x0, sizeof(endflag));
	memset(&locbuf, 0x0, sizeof(locbuf));
	memset(&vars, 0x0, sizeof(vars));
	memset(&pkg_cache, 0x0, sizeof(pkg_cache));
	
	atexit(EXIT);
	sprintf(tmp, "%s/log/syslog/autosend.log", getenv("SWWORK"));
	pub_log_chglog(SW_LOG_CHG_DBGFILE, tmp);
	
	strncpy(txcode, argv[1], sizeof(txcode) - 1);
	strncpy(xmltree, argv[2], sizeof(xmltree) - 1);
	pub_log_info("[%s][%d] txcode=[%s] xmltree=[%s]", __FILE__, __LINE__, txcode, xmltree);

	ret = pub_loc_vars_alloc(&vars, HEAP_VARS);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Vars alloc error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = pub_loc_vars_create(&vars, 0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Vars create error!", __FILE__, __LINE__);
		return -1;
	}
	
	pub_log_info("[%s][%d] 交易初始化开始...", __FILE__, __LINE__);
	ret = pack_init(&vars, txcode);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 交易初始化失败!", __FILE__, __LINE__);
		vars.free_mem(&vars);
		return -1;
	}
	
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%s/cfg/common/%s", getenv("SWWORK"), xmltree);
	pub_log_info("[%s][%d] xmlname=[%s]", __FILE__, __LINE__, tmp);
	xml = pub_xml_crtree(tmp);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, tmp);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		return -1;
	}
	
	pub_buf_init(&locbuf);
	ret = pkg_out(&vars, &locbuf, xml, &pkg_cache, NULL);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] pack out error!", __FILE__, __LINE__);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		pub_xml_deltree(xml);
		return -1;
	}
	pub_xml_deltree(xml);
	vars.destroy(&vars);
	vars.free_mem(&vars);
	pub_log_bin(SW_LOG_INFO, locbuf.data, locbuf.len, "[%s][%d] Pack data:[%d]", __FILE__, __LINE__, locbuf.len);
	
	ptr = getenv("AUTOSENDIP");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] 未设置环境变量AUTOSENDIP!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return -1;
	}
	strncpy(ip, ptr, sizeof(ip) - 1);
	
	ptr = getenv("AUTOSENDPORT");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] 未设置环境变量AUTOSENDPORT!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return -1;
	}
	port = atoi(ptr);
	
	sockid = lsn_pub_connect(ip, port);
	if (sockid < 0)
	{
		pub_log_error("[%s][%d] Connect[%s][%d] error! errno=[%d]:[%s]", 
			__FILE__, __LINE__, ip, port, errno, strerror(errno));
		pub_buf_clear(&locbuf);
		return -1;
	}
	
	ret = lsn_pub_send(sockid, locbuf.data, locbuf.len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send data error!", __FILE__, __LINE__);
		close(sockid);
		pub_buf_clear(&locbuf);
		return -1;
	}
	pub_log_bin(SW_LOG_INFO, locbuf.data, locbuf.len, "[%s][%d] Send data:[%d]", __FILE__, __LINE__, locbuf.len);
	pub_buf_clear(&locbuf);
	
	ptr = getenv("AUTOSENDEND");
	if (ptr == NULL || ptr[0] != '0')
	{
		memset(endflag, 0x0, sizeof(endflag));
		endflag[0] = 0xff;
		ret = lsn_pub_send(sockid, (char *)endflag, 1);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Send data error!", __FILE__, __LINE__);
			close(sockid);
			return -1;
		}
	}
	pub_log_info("[%s][%d] Send data success!", __FILE__, __LINE__);
	
	memset(data, 0x0, sizeof(data));
	ptr = getenv("AUTOSENDDATA");
	if (ptr != NULL)
	{
		strcpy(data, ptr);
	}
	else
	{
		strcpy(data, "A(8)R(L)");
	}
	pub_log_info("[%s][%d] data=[%s]", __FILE__, __LINE__, data);

	pub_buf_init(&locbuf);
	ret = lsn_pub_recv(data, sockid, &locbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] recv data error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return -1;
	}
	pub_log_bin(SW_LOG_INFO, locbuf.data, locbuf.len, "[%s][%d] Recv data:[%d]", __FILE__, __LINE__, locbuf.len);
	close(sockid);
	pub_buf_clear(&locbuf);

	return 0;
}

void EXIT()
{
	pthread_exit(0);
}
