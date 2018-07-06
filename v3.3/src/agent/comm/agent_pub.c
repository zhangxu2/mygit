#include "agent_pub.h"

static int	g_scantime;

int strchr_cnt(char *str, char ch)
{
	int	count = 0;

	while (str && (str = strchr(str, ch)))
	{
		count++;
		str++;
	}

	return count;
}

int strstr_cnt(char *str, char *needle)
{
	int	count = 0;
	char	*p = str; 

	while(p && (p = strstr(p, needle)))
	{
		count++;
		p += strlen(needle);
	}

	return count;
}

int agt_cfg_init(sw_agt_cfg_t *cfg, char *name)
{	
	int	ret = 0;
	char	*ptr = NULL;
	char	agtname[64];
	char	filename[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL, *node1 = NULL;

	memset(filename, 0x00, sizeof(filename));
	ptr = getenv("SWWORK");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d], No env SWWORK.",__FILE__,__LINE__);
		return -1;
	}
	else
	{
		snprintf(filename, sizeof(filename), "%s/cfg/agentcfg/agent.xml", ptr);
	}

	xml = pub_xml_crtree(filename);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xmltree error.", __FILE__, __LINE__);
		return -1;
	}

	node = pub_xml_locnode(xml, ".CFG.GLOBAL.SCANTIME");
	if (node != NULL && node->value != NULL && strlen(node->value) != 0)
	{
		cfg->scantime = atoi(node->value);
	}
	else
	{
		cfg->scantime = 10;
	}
	g_scantime = cfg->scantime;

	node = pub_xml_locnode(xml, ".CFG.GLOBAL.LOGLEVEL");
	if (node != NULL && node->value != NULL && strlen(node->value) != 0)
	{
		cfg->loglevel = atoi(node->value);
	}


	node = pub_xml_locnode(xml, ".CFG.GLOBAL.LOGSIZE");
	if (node != NULL && node->value != NULL && strlen(node->value) != 0)
	{
		cfg->logsize = pub_cfg_parse_size(node->value);
	}

	node = pub_xml_locnode(xml, ".CFG.AGENT");
	while(node != NULL)
	{
		if (strcmp(node->name, "AGENT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		memset(agtname, 0x00, sizeof(agtname));
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(agtname, node1->value, strlen(node1->value));
		}

		pub_log_debug("[%s][%d]name=[%s]argname=[%s]", __FILE__, __LINE__,name, agtname);
		if (strcmp(agtname, name) == 0)
		{
			break;
		}

		node = node->next;
	}

	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] not find name[%s]", __FILE__, __LINE__, name);
		return -1;
	}

	strncpy(cfg->name, agtname, sizeof(agtname));
	node1 = pub_xml_locnode(xml, "USE");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		cfg->use = atoi(node1->value);
	}

	node1 = pub_xml_locnode(xml, "TYPE");
	if (node1 != NULL && node1->value != NULL)
	{
		cfg->type = atoi(node1->value);
	}

	node1 = pub_xml_locnode(xml, "XMLTREE");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->xmlname, node1->value, sizeof(cfg->xmlname));
	}

	node1 = pub_xml_locnode(xml, "DATA");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->data, node1->value, sizeof(cfg->data));
	}

	node1 = pub_xml_locnode(xml, "PROCCNT");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		cfg->proccnt = atoi(node1->value);
	}
	else
	{
		cfg->proccnt = 1; 
	}

	node1 = pub_xml_locnode(xml, "SCANTIME");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		cfg->scantime = atoi(node1->value);
	}
	else
	{
		cfg->scantime = 300;
	}

	node1 = pub_xml_locnode(xml, "IP");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->ip, node1->value, sizeof(cfg->ip));
	}
	else
	{
		strncpy(cfg->ip, "127.0.0.1", sizeof(cfg->ip));
	}

	node1 = pub_xml_locnode(xml, "WARNPER");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0 )
	{
		cfg->warnper = atoi(node1->value);
	}
	else
	{
		cfg->warnper = 50;
	}

	node1 = pub_xml_locnode(xml, "WARNNUM");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		cfg->warncnt = atoi(node1->value);
	}
	else
	{
		cfg->warncnt = 10;
	}

	node1 = pub_xml_locnode(xml, "DURTIME");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		cfg->durtime = atoi(node1->value);
	}
	else
	{
		cfg->durtime = 1800;
	}

	node1 = pub_xml_locnode(xml, "PORT");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		cfg->port = atoi(node1->value);
	}

	node1 = pub_xml_locnode(xml, "MAP");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->map, node1->value, sizeof(cfg->map));
	}

	node1 = pub_xml_locnode(xml, "TRANFUNC");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->tranfunc, node1->value, sizeof(cfg->tranfunc));
	}

	node1 = pub_xml_locnode(xml, "INIT");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->init, node1->value, sizeof(cfg->tranfunc));
	}

	node1 = pub_xml_locnode(xml, "DEAL");
	if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
	{
		strncpy(cfg->deal, node1->value, sizeof(cfg->tranfunc));
	}

	pub_xml_deltree(xml);
	return 0;

}


static int agt_cfg_get_log(sw_agt_cfg_t *cfg)
{	
	char	*ptr = NULL;
	char	filename[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(filename, 0x00, sizeof(filename));
	ptr = getenv("SWWORK");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d], No env SWWORK.",__FILE__,__LINE__);
		return -1;
	}
	else
	{
		snprintf(filename, sizeof(filename), "%s/cfg/agentcfg/agent.xml", ptr);
	}

	xml = pub_xml_crtree(filename);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xmltree error.", __FILE__, __LINE__);
		return -1;
	}

	node = pub_xml_locnode(xml, ".CFG.GLOBAL.LOGLEVEL");
	if (node != NULL && node->value != NULL && strlen(node->value) != 0)
	{
		cfg->loglevel = atoi(node->value);
	}


	node = pub_xml_locnode(xml, ".CFG.GLOBAL.LOGSIZE");
	if (node != NULL && node->value != NULL && strlen(node->value) != 0)
	{
		cfg->logsize = pub_cfg_parse_size(node->value);
	}

	pub_xml_deltree(xml);
	return 0;

}

int agt_set_log_cfg()
{
	int	ret = 0;
	sw_agt_cfg_t	cfg;

	memset(&cfg, 0x00, sizeof(cfg));	
	ret = agt_cfg_get_log(&cfg);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get global cfg error.", __FILE__, __LINE__);
		return -1;
	}


	if (cfg.loglevel)
	{
		log_set_syslog_level(cfg.loglevel);
	}

	if (cfg.logsize > 0)
	{
		log_set_size(cfg.logsize);
	}

	pub_log_debug("[%s][%d] loglevel=[%d] logsize=[%d]", __FILE__, __LINE__, cfg.loglevel, cfg.logsize);
	return 0;
}

int agt_get_scantime()
{
	return g_scantime;
}

int agt_set_logpath(const char *errlog, const char *dbglog)
{
	int	ret = 0;
	char	*tmp = NULL;
	char	path[128];
	char	errpath[256];
	char	dbgpath[256];

	if (errlog == NULL || dbglog == NULL)
	{
		pub_log_info("[%s][%d], input param error",__FILE__, __LINE__);
		return -1;
	}

	pub_signal_nozombie();
	pub_signal_ignore();

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_stderr("cycle_init get env SWWORK error.\n");
		pub_log_info("[%s][%d] error", __FILE__, __LINE__);
		return -1;
	}

	if (errlog != NULL && dbglog != NULL)
	{
		pub_mem_memzero(path, sizeof(path));
		snprintf(path, sizeof(path), "%s/log/syslog", tmp);
		aix_mkdirp(path, 0777);
		
		pub_mem_memzero(errpath, sizeof(errpath));
		snprintf(errpath, sizeof(errpath), "%s/%s", path, errlog);
		
		ret = pub_log_chglog(SW_LOG_CHG_ERRFILE, errpath);
		if (ret != SW_OK)
		{
			pub_log_stderr("pub_log_chglog error path error.\n");
			pub_log_info("[%s][%d] error",__FILE__, __LINE__);
			return -1;
		}

		pub_mem_memzero(dbgpath, sizeof(dbgpath));
		snprintf(dbgpath, sizeof(dbgpath), "%s/%s", path, dbglog);

		ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, dbgpath);
		if (ret != SW_OK)
		{
			pub_log_stderr("pub_log_chglog debug path error.\n");
			pub_log_info("[%s][%d] error", __FILE__, __LINE__);
			return -1;
		}
	}

	return 0;	
}

int agt_get_db_cfg(sw_dbcfg_t *dbcfg)
{
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/dbconfig.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
			__FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".DBCONFIG.DATABASE");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Î´ÅäÖÃDATABASE!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	xml->current = node;
	node = pub_xml_locnode(xml, "DBNAME");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg->dbname, node->value, sizeof(dbcfg->dbname) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBSID");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg->dbsid, node->value, sizeof(dbcfg->dbsid) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBUSER");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg->dbuser, node->value, sizeof(dbcfg->dbuser) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBPASSWD");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg->dbpasswd, node->value, sizeof(dbcfg->dbpasswd) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBTYPE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg->dbtype, node->value, sizeof(dbcfg->dbtype) - 1);
	}

	node = pub_xml_locnode(xml, "DBMODE");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		dbcfg->mode = 1;
	}

	pub_xml_deltree(xml);

	return 0;
}

