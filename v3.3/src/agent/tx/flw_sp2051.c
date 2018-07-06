/*************************************************
  文 件 名:  flw_sp2051.c                        **
  功能描述:  平台配置管理                        **
  作    者:  linpanfei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

static int pub_prdtio_out_bkconf()
{
	char	now[32];
	char	dustdir[256];
	char	cmd[512];

	if (getenv("SWWORK") == NULL)
	{
		pub_log_error("[%s][%d]not env SWWORK error ", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(now, sizeof(now));
	agt_date_format(time(NULL), now, "F");

	pub_mem_memzero(dustdir, sizeof( dustdir));
	sprintf(dustdir, "%s/%s/%s", getenv("SWWORK"), SPACE, now);
	if (aix_mkdirp(dustdir, 0777) < 0)
	{
		pub_log_error("[%s][%d]pub_dir_creat error ", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(cmd, sizeof(cmd));
	sprintf(cmd, "cp %s/cfg/swconfig.xml %s", getenv("SWWORK"), dustdir);
	if (agt_system(cmd) < 0)
	{
		pub_log_error("[%s][%d] deal cmd=[%s] error ", __FILE__, __LINE__, cmd);
		return -1;
	}

	return 0;
}

/* 2051 */
int sp2051(sw_loc_vars_t *vars)
{
	int		ret = -1;
	char	sopt[8];
	char	cmd[256];
	char	name[256];
	char	value[512];
	char	base[128];
	char	reply[16];
	char	res_msg[256];
	char	xmlbuf[10240];	
	FILE	*fp = NULL;
	sw_xmltree_t	*xmltree = NULL;
	sw_xmltree_t	*xmltreebak = NULL;	
	sw_xmlnode_t	*node = NULL;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset(sopt, 0x00, sizeof(sopt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", sopt);

	if (sopt[0] == 'M')
	{
		ret = agt_back_file("swconfig.xml");
		if (ret < 0)
		{
			pub_log_error("[%s][%d]备份文件失败", __FILE__, __LINE__);
			strcpy(res_msg, "备份文件失败");
			goto ErrExit;
		}
		memset(name, 0x00, sizeof(name));
		sprintf(name, "%s/cfg/swconfig.xml", getenv("SWWORK"));

		memset(cmd, 0x00, sizeof(cmd));
		sprintf(cmd, "touch %s", name);
		ret = agt_system(cmd);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] deal cmd=[%s] error", __FILE__, __LINE__, cmd);
			goto ErrExit;
		}
		if (pub_prdtio_out_bkconf() < 0)
		{
			pub_log_error("[%s][%d]back file swconfig.xml error", __FILE__, __LINE__ );
			strcpy(reply, "E048");
			goto ErrExit;
		}

		xmltree = pub_xml_crtree(name);
		if (xmltree == NULL)
		{
			pub_log_error("[%s][%d].creat xml tree error  name[%s]", __FILE__, __LINE__, name );
			strncpy(reply, "E045", sizeof(reply) - 1);
			goto ErrExit;
		}

#define	locfunc(path) 	do{ memset(value, 0x00, sizeof(value)); \
	memset(base,0x00, sizeof(base)); \
	sprintf(base,".TradeRecord.Request%s",path); \
	loc_get_zd_data(vars,base,value);\
	if(strlen(value) > 0 && strcmp(value, "NULL") != 0 )	\
	{\
		xmltreebak = vars->tree; \
		vars->tree=xmltree; \
		loc_set_zd_data(vars, path, value); \
		vars->tree=xmltreebak; \
	};}while(0);

		locfunc(".SWCFG.GLOBAL.ESWSID");
		locfunc(".SWCFG.GLOBAL.ESWNAME");
		locfunc(".SWCFG.GLOBAL.LSNMAX");
		locfunc(".SWCFG.GLOBAL.SVRMAX");
		locfunc(".SWCFG.GLOBAL.PRDTMAX");
		locfunc(".SWCFG.GLOBAL.PROCESS");
		locfunc(".SWCFG.GLOBAL.SESSION");
		locfunc(".SWCFG.GLOBAL.SEQMAX");
		locfunc(".SWCFG.GLOBAL.JOBMAX");
		locfunc(".SWCFG.GLOBAL.DBMAX");
		locfunc(".SWCFG.GLOBAL.SCANTIME");
		locfunc(".SWCFG.GLOBAL.SEMSIZE");
		locfunc(".SWCFG.GLOBAL.RUNSHMSIZE");
		locfunc(".SWCFG.GLOBAL.SHAREPOOLSIZE");

		memset(value, 0x00, sizeof(value)); 
		memset(base, 0x00, sizeof(base)); 
		sprintf(base, ".TradeRecord.Request.SWCFG.GLOBAL.WORKMODE"); 
		loc_get_zd_data(vars,base,value);
		if (strcmp(value, "1") == 0)
		{
			xmltreebak = vars->tree; 
			vars->tree = xmltree; 
			loc_set_zd_data(vars, ".SWCFG.GLOBAL.WORKMODE", "MP"); 
			vars->tree = xmltreebak; 
		}else if (strcmp(value, "0") == 0)
		{
			xmltreebak = vars->tree; 
			vars->tree = xmltree; 
			loc_set_zd_data(vars, ".SWCFG.GLOBAL.WORKMODE", ""); 
			vars->tree = xmltreebak; 
		}

		locfunc(".SWCFG.SYSLOG.LOGLVL");
		locfunc(".SWCFG.SYSLOG.LOGFILESIZE");

		locfunc(".SWCFG.ALOG.USE");
		locfunc(".SWCFG.ALOG.IP");
		locfunc(".SWCFG.ALOG.TRANSPORT");
		locfunc(".SWCFG.ALOG.PORT");
		locfunc(".SWCFG.ALOG.PROCNUM");
		locfunc(".SWCFG.ALOG.THREADNUM");
		locfunc(".SWCFG.ALOG.WAITTIME");
		locfunc(".SWCFG.ALOG.TIMEOUT");
		locfunc(".SWCFG.ALOG.LOGCACHESIZE");
		locfunc(".SWCFG.ALOG.SHMALLOCSIZE");

		locfunc(".SWCFG.ARES.USE");
		locfunc(".SWCFG.ARES.IP");
		locfunc(".SWCFG.ARES.PORT");
		locfunc(".SWCFG.ARES.MONUSE");
		locfunc(".SWCFG.ARES.SEQSUSE");
		locfunc(".SWCFG.ARES.MTYPEUSE");
		locfunc(".SWCFG.ARES.WAITTIME");

		pub_mem_memzero(xmlbuf, sizeof(xmlbuf));
		ret = pub_xml_pack_ext(xmltree, xmlbuf);
		if (ret != 0)
		{
			pub_log_error("[%s][%d]get xmltree value to buf error ret=[%d]", __FILE__, __LINE__, ret);
			pub_xml_deltree(xmltree);
			strncpy(reply, "E046", sizeof(reply) - 1);
			goto ErrExit;
		}

		fp = fopen(name, "w+");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d]crete xml file  name=[%s] error", __FILE__, __LINE__, name);
			pub_xml_deltree(xmltree);
			strncpy(reply, "E016", sizeof(reply) - 1);
			goto ErrExit;
		}
		fprintf(fp, "<?xml version= \"1.0\"  encoding= \"gbk\" ?>\n%s", xmlbuf);	
		fclose(fp);
		pub_xml_deltree(xmltree);

#undef locfunc
	}
	else if (sopt[0] == 'S')
	{
		pub_mem_memzero(name, sizeof(name));
		sprintf(name, "%s/cfg/swconfig.xml", getenv("SWWORK"));

		xmltree = pub_xml_crtree(name);
		if (xmltree == NULL)
		{
			pub_log_error("[%s][%d].creat xml tree error  name[%s]", __FILE__, __LINE__, name);
			strncpy(reply, "E045", sizeof(reply) - 1);
			goto ErrExit;
		}

#define	locfunc(path)  do{ \
	node = pub_xml_locnode(xmltree,path);\
	if(node != NULL && node->value != NULL && strlen(node->value) > 0) \
	{\
		memset(base, 0x00, sizeof(base));\
		sprintf(base,".TradeRecord.Response%s",path); \
		loc_set_zd_data(vars, base, node->value );	\
	}; }while(0);

		locfunc(".SWCFG.GLOBAL.ESWSID");
		locfunc(".SWCFG.GLOBAL.ESWNAME");
		locfunc(".SWCFG.GLOBAL.LSNMAX");
		locfunc(".SWCFG.GLOBAL.SVRMAX");
		locfunc(".SWCFG.GLOBAL.PRDTMAX");
		locfunc(".SWCFG.GLOBAL.PROCESS");
		locfunc(".SWCFG.GLOBAL.SESSION");
		locfunc(".SWCFG.GLOBAL.SEQMAX");
		locfunc(".SWCFG.GLOBAL.JOBMAX");
		locfunc(".SWCFG.GLOBAL.DBMAX");
		locfunc(".SWCFG.GLOBAL.SCANTIME");
		locfunc(".SWCFG.GLOBAL.SEMSIZE");
		locfunc(".SWCFG.GLOBAL.RUNSHMSIZE");
		locfunc(".SWCFG.GLOBAL.SHAREPOOLSIZE");

		locfunc(".SWCFG.SYSLOG.LOGLVL");
		locfunc(".SWCFG.SYSLOG.LOGFILESIZE");

		locfunc(".SWCFG.ALOG.USE");
		locfunc(".SWCFG.ALOG.IP");
		locfunc(".SWCFG.ALOG.TRANSPORT");
		locfunc(".SWCFG.ALOG.PORT");
		locfunc(".SWCFG.ALOG.PROCNUM");
		locfunc(".SWCFG.ALOG.THREADNUM");
		locfunc(".SWCFG.ALOG.WAITTIME");
		locfunc(".SWCFG.ALOG.TIMEOUT");
		locfunc(".SWCFG.ALOG.LOGCACHESIZE");
		locfunc(".SWCFG.ALOG.SHMALLOCSIZE");

		locfunc(".SWCFG.ARES.USE");
		locfunc(".SWCFG.ARES.IP");
		locfunc(".SWCFG.ARES.PORT");
		locfunc(".SWCFG.ARES.MONUSE");
		locfunc(".SWCFG.ARES.SEQSUSE");
		locfunc(".SWCFG.ARES.MTYPEUSE");
		locfunc(".SWCFG.ARES.WAITTIME");

		node = pub_xml_locnode(xmltree, ".SWCFG.GLOBAL.WORKMODE");
		if(node != NULL && node->value != NULL && strcmp(node->value, "MP") == 0) 
		{
			memset(base, 0x00, sizeof(base));
			sprintf(base, ".TradeRecord.Response.SWCFG.GLOBAL.WORKMODE"); 
			loc_set_zd_data(vars, base, "1" );
		}
		else
		{
			memset(base, 0x00, sizeof(base));
			sprintf(base, ".TradeRecord.Response.SWCFG.GLOBAL.WORKMODE"); 
			loc_set_zd_data(vars, base, "0");
		}

		pub_xml_deltree(xmltree);
#undef locfunc
	}
	else
	{
		pub_log_error("[%s][%d].TradeRecord.Request.Option value error[%s]", __FILE__, __LINE__, cmd);
		strncpy(reply, "E012", sizeof(reply) - 1);
		goto ErrExit;
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	if (sopt[0] != 'S')
	{
		ret = record_oper_log(vars, reply, res_msg);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 操作登记流水错误", __FILE__, __LINE__);
			goto ErrExit;
		}
	}

	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
