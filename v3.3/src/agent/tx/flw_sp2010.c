/*************************************************
  文 件 名:  flw_sp2010.c                        **
  功能描述:  服务监控                            **
  作    者:  薛辉                                **
  完成日期:  20160801                            **
 *************************************************/
#include "agent_comm.h"

typedef struct cfg_s
{
	char	name[32];
	char	use[32];
	char	scantime[32];
	char	warnper[32];
	char	warnnum[32];
	char	durtime[32];
}cfg_t;

static int get_cfg_info(sw_loc_vars_t *vars, cfg_t *cfg, int i)
{
	char path[256];

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.Cfgs.Cfg(%d).NAME", i);
	loc_get_zd_data(vars, path, cfg->name);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.Cfgs.Cfg(%d).USE", i);
	loc_get_zd_data(vars, path, cfg->use);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.Cfgs.Cfg(%d).SCANTIME", i);
	loc_get_zd_data(vars, path, cfg->scantime);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.Cfgs.Cfg(%d).WARNPER", i);
	loc_get_zd_data(vars, path, cfg->warnper);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.Cfgs.Cfg(%d).WARNNUM", i);
	loc_get_zd_data(vars, path, cfg->warnnum);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Request.Cfgs.Cfg(%d).DURTIME", i);
	loc_get_zd_data(vars, path, cfg->durtime);
	
	return 0;
}

static int set_cfg_info(sw_loc_vars_t *vars, cfg_t *cfg, int i)
{
	char path[256];

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Cfgs.Cfg(%d).NAME", i);
	loc_set_zd_data(vars, path, cfg->name);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Cfgs.Cfg(%d).USE", i);
	loc_set_zd_data(vars, path, cfg->use);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Cfgs.Cfg(%d).SCANTIME", i);
	loc_set_zd_data(vars, path, cfg->scantime);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Cfgs.Cfg(%d).WARNPER", i);
	loc_set_zd_data(vars, path, cfg->warnper);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Cfgs.Cfg(%d).WARNNUM", i);
	loc_set_zd_data(vars, path, cfg->warnnum);
	
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, ".TradeRecord.Response.Cfgs.Cfg(%d).DURTIME", i);
	loc_set_zd_data(vars, path, cfg->durtime);
	
	return 0;
}

int sp2010(sw_loc_vars_t *vars)
{
	int	i = 0;
	char	xmlname[128];
	char	opt[16];
	char	reply[8];
	char	deal[128];
	char	res_msg[256];
	char	names[4][32] = {"cpu", "mem", "mtype", "proc"};
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	cfg_t	cfg;


	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/agentcfg/agent.xml", getenv("SWWORK"));
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		strcpy(reply, "E009");
		return SW_ERROR;
	}

	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		strcpy(reply, "E045");
		return SW_ERROR;
	}

	memset(opt, 0x00, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);

	if (opt[0] == 'S')
	{
		for (i = 0; i < 4; ++i)
		{
			memset(&cfg, 0x0, sizeof(cfg));
			memset(deal, 0x0, sizeof(deal));
			snprintf(deal, sizeof(deal)-1, "libagent_work.so/agt_%s_work", names[i]);
			node = agt_xml_search(xml, ".CFG.AGENT", "DEAL", deal);
			if (node == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME! %s", __FILE__, __LINE__, names[i]);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				strcpy(res_msg, "未找到配置");
				goto ErrExit;
			}
			xml->current = node;
			strncpy(cfg.name, names[i], sizeof(cfg.name)-1);

			node1 = pub_xml_locnode(xml, "USE");
			if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
			{
				strncpy(cfg.use, node1->value, sizeof(cfg.use)-1);
			}

			node1 = pub_xml_locnode(xml, "SCANTIME");
			if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
			{
				strncpy(cfg.scantime, node1->value, sizeof(cfg.scantime)-1);
			}

			node1 = pub_xml_locnode(xml, "WARNPER");
			if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
			{
				strncpy(cfg.warnper, node1->value, sizeof(cfg.warnper)-1);
			}

			node1 = pub_xml_locnode(xml, "WARNNUM");
			if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
			{
				strncpy(cfg.warnnum, node1->value, sizeof(cfg.warnnum)-1);
			}

			node1 = pub_xml_locnode(xml, "DURTIME");
			if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
			{
				strncpy(cfg.durtime, node1->value, sizeof(cfg.durtime)-1);
			}
			set_cfg_info(vars, &cfg, i);
		}	
		loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", i);
		pub_xml_deltree(xml);
		goto OkExit;
	}
	else if(opt[0] == 'M')
	{
		for (i = 0; i < 4; ++i)
		{
			memset(&cfg, 0x0, sizeof(cfg));
			get_cfg_info(vars, &cfg, i);
			memset(deal, 0x0, sizeof(deal));
			snprintf(deal, sizeof(deal)-1, "libagent_work.so/agt_%s_work", names[i]);
			node = agt_xml_search(xml, ".CFG.AGENT", "DEAL", deal);
			if (node == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME! %s", __FILE__, __LINE__, cfg.name);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				goto ErrExit;
			}
			xml->current = node;

			node1 = pub_xml_locnode(xml, "USE");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				goto ErrExit;
			}
			pub_xml_set_value(node1, cfg.use);
			if (cfg.use[0] == '0')
			{
				continue;
			}

			node1 = pub_xml_locnode(xml, "SCANTIME");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				goto ErrExit;
			}
			pub_xml_set_value(node1, cfg.scantime);

			node1 = pub_xml_locnode(xml, "WARNPER");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				goto ErrExit;
			}
			pub_xml_set_value(node1, cfg.warnper);

			node1 = pub_xml_locnode(xml, "WARNNUM");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				goto ErrExit;
			}
			pub_xml_set_value(node1, cfg.warnnum);

			node1 = pub_xml_locnode(xml, "DURTIME");
			if (node1 == NULL)
			{
				pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				strcpy(reply, "E026");
				goto ErrExit;
			}
			pub_xml_set_value(node1, cfg.durtime);
		}
		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);
		goto OkExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_OK;

ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}
