#include "pub_type.h"
#include "pub_log.h"
#include "pub_xml.h"
#include "pub_vars.h"

int get_lsn_info(char *name, int *type)
{
	char	*ptr = NULL;
	char	lib[64];
	char	xmlname[128];
	static	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(lib, 0x0, sizeof(lib));
	
	if (name == NULL || name[0] == '\0')
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Param lsn name is null!", __FILE__, __LINE__);
		return -1;
	}
	
	if (xml == NULL)
	{
		memset(xmlname, 0x0, sizeof(xmlname));
		sprintf(xmlname, "%s/cfg/listener.lsn", getenv("SWWORK"));
		xml = pub_xml_crtree(xmlname);
		if (xml == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
			return -1;
		}
	}
	
	node = pub_xml_locnode(xml, ".DFISBP.SWLISTEN");
	while (node != NULL)
	{
		if (strcmp(node->name, "SWLISTEN") != 0)
		{
			node = node->next;
			continue;
		}
	
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_xml_deltree(xml);
			xml = NULL;
			uLog(SW_LOG_ERROR, "[%s][%d] Not config NAME!", __FILE__, __LINE__);
			return -1;
		}
		if (strcmp(node1->value, name) == 0)
		{
			break;
		}
		
		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		xml = NULL;
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s lsn info!", __FILE__, __LINE__, name);
		return -1;
	}

	node = pub_xml_locnode(xml, "DATA");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_data", node->value);
	}
	
	node = pub_xml_locnode(xml, "PKGCFG");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_xmltree", node->value);
	}
	
	node = pub_xml_locnode(xml, "LOADLIB");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_loadlib", node->value);
	}
	
	node = pub_xml_locnode(xml, "FILEDEAL");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_file_fun", node->value);
	}
	
	node = pub_xml_locnode(xml, "MAPCFG");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_mapcfg", node->value);
	}
	
	node = pub_xml_locnode(xml, "TIMEOUT");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_timeout", node->value);
	}
	
	node = pub_xml_locnode(xml, "PROCESSER.TRANSCODE");
	if (node != NULL && node->value != NULL)
	{
		ptr = strchr(node->value, '/');
		if (ptr == NULL)
		{
			set_zd_data("#sw_enc_lib", "libswdes.so");
			set_zd_data("#sw_enc_fun", ptr + 1);
		}
		else
		{
			memset(lib, 0x0, sizeof(lib));
			strncpy(lib, node->value, ptr - node->value);
			set_zd_data("#sw_enc_lib", lib);
			set_zd_data("#sw_enc_fun", ptr + 1);
		}
	}
	
	node = pub_xml_locnode(xml, "COMM.GROUP.REMOTE.IP");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_ip", node->value);
	}

	node = pub_xml_locnode(xml, "COMM.GROUP.REMOTE.PORT");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_port", node->value);
	}
	
	node = pub_xml_locnode(xml, "COMM.GROUP.REMOTE.SERVICE");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_service", node->value);
	}
	
	node = pub_xml_locnode(xml, "COMM.GROUP.BUFTYPE");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_buftype", node->value);
	}
	
	node = pub_xml_locnode(xml, "COMM.GROUP.SNDQ.QMGR");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_qmgr", node->value);
	}
	
	node = pub_xml_locnode(xml, "COMM.GROUP.SNDQ.QNAME");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_send_mq", node->value);
	}
	
	node = pub_xml_locnode(xml, "COMM.GROUP.RCVQ.QNAME");
	if (node != NULL && node->value != NULL)
	{
		set_zd_data("#sw_recv_mq", node->value);
	}
	
	node = pub_xml_locnode(xml, "COMTYPE");
	if (node == NULL || node->value == NULL)
	{
		pub_xml_deltree(xml);
		xml = NULL;
		uLog(SW_LOG_ERROR, "[%s][%d] Not config COMTYPE!", __FILE__, __LINE__);
		return -1;
	}
	
	if (strncmp(node->value, "TCP", 3) == 0)
	{
		*type = TCPIP;
	}
	else if (strncmp(node->value, "TUX", 3) == 0)
	{
		*type = TUXEDO;
	}
	else if (strncmp(node->value, "CIC", 3) == 0)
	{
		*type = CICS;
	}
	else if (strncmp(node->value, "TEA", 3) == 0)
	{
		*type = TEASY;
	}
	else if (strncmp(node->value, "MQ", 2) == 0)
	{
		*type = MQ;
	}
	else if (strncmp(node->value, "TLQ", 3) == 0)
	{
		*type = TLQ;
	}
	else if (strncmp(node->value, "SOAP", 4) == 0)
	{
		*type = SOAP;
	}

	/*pub_xml_deltree(xml);*/
	
	return 0;
}

typedef int (*sw_callfun_pt)();
typedef struct
{
	void    *handle;
	sw_callfun_pt     fun;
	char	name[64];
	char    libname[256];
	char    funname[256];
}call_dlnode_t;

#define MAX_CALL_NUM	20

static call_dlnode_t	g_call_dlcache[MAX_CALL_NUM];

static sw_callfun_pt call_dlcache_get_dlfun_by_name(char *name)
{
	int	i = 0;
	
	for (i = 0; i < MAX_CALL_NUM; i++)
	{
		if (strcmp(g_call_dlcache[i].name, name) == 0)
		{
			return g_call_dlcache[i].fun;
		}
	}
	
	return NULL;
}

static int call_find_free_index()
{
	int	i = 0;
	
	for (i = 0; i < MAX_CALL_NUM; i++)
	{
		if (g_call_dlcache[i].name[0] == '\0')
		{
			return i;
		}
	}
	
	return -1;
}

int call(char *name)
{
	int	ret = 0;
	int	type = 0;
	int	index = 0;
	static int	first_call = 1;
	char	cache[8];
	char	libso[128];
	char	funname[64];
	void	*handle = NULL;
	sw_callfun_pt	dlfun = NULL;
	
	memset(cache, 0x0, sizeof(cache));
	memset(libso, 0x0, sizeof(libso));
	memset(funname, 0x0, sizeof(funname));
	
	if (name == NULL || name[0] == '\0')
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Name is null!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = get_lsn_info(name, &type);
	if (ret < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get lsn [%s] info error!", __FILE__, __LINE__, name);
		return -1;
	}
	set_zd_data("$sw_call_chnl", name);

	if (first_call)
	{
		memset(g_call_dlcache, 0x0, sizeof(g_call_dlcache));
		first_call = 0;
	}

	memset(cache, 0x0, sizeof(cache));
	get_zd_data("$dlcache", cache);
	uLog(SW_LOG_INFO, "[%s][%d] dlcache:[%s]", __FILE__, __LINE__, cache);
	if (cache[0] == '\0')
	{
		cache[0] = '1';
	}
	
	if (cache[0] == '1')
	{
		dlfun = call_dlcache_get_dlfun_by_name(name);
		if (dlfun != NULL)
		{
			ret = dlfun();
			if (ret < 0)
			{
				uLog(SW_LOG_ERROR, "[%s][%d] dlfun error! ret=[%d]", __FILE__, __LINE__, ret);
				return -1;
			}
			uLog(SW_LOG_DEBUG, "[%s][%d] call [%s] success!", __FILE__, __LINE__, name);

			return 0;
		}
	}

	if (type == TCPIP)
	{
		strcpy(funname, "tcpcall");
	}
	else if (type == TUXEDO)
	{
		strcpy(funname, "tuxcall");
	}
	else if (type == CICS)
	{
		strcpy(funname, "ciccall");
	}
	else if (type == TEASY)
	{
		strcpy(funname, "teacall");
	}
	else if (type == MQ)
	{
		strcpy(funname, "mqcall");
	}
	else if (type == TLQ)
	{
		strcpy(funname, "tlqcall");
	}
	else if (type == SOAP)
	{
		strcpy(funname, "soapcall");
	}
	else
	{
		uLog(SW_LOG_ERROR, "[%s][%d] call type error! type=[%d]", __FILE__, __LINE__);
		return -1;
	}

	sprintf(libso, "%s/plugin/%s.so", getenv("SWHOME"), funname);
	handle = (void *)dlopen(libso, RTLD_LAZY | RTLD_GLOBAL);
	if (handle == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlopen [%s] error! [%s]", __FILE__, __LINE__, libso, dlerror());
		return -1;
	}
	
	dlfun = (int (*)())dlsym(handle, funname);
	if (dlfun == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlsym [%s][%d] error! [%s]", __FILE__, __LINE__, libso, dlerror());
		dlclose(handle);
		return -1;
	}
	if (cache[0] == '1')
	{
		index = call_find_free_index();
		if (index >= 0)
		{
			g_call_dlcache[index].handle = handle;
			g_call_dlcache[index].fun = dlfun;
			strcpy(g_call_dlcache[index].name, name);
			strcpy(g_call_dlcache[index].libname, libso);
			strcpy(g_call_dlcache[index].funname, funname);
		}
	}

	ret = dlfun();
	if (cache[0] != '1')
	{
		dlclose(handle);
	}
	if (ret < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlfun error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	uLog(SW_LOG_DEBUG, "[%s][%d] call [%s] success!", __FILE__, __LINE__, name);
	
	return 0;
}

