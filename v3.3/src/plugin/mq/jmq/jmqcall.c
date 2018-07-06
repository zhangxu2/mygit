#include "jmq.h"

msg_format_enum_t g_msg_format = MSG_NONE;

int mqcall()
{
	int	ret = 0;
	int	sid = 0;
	int 	port = 0;
	int	timeout = 0;
	int	bufferlen = 0;
	char	buf[128];
	char	prdt[32];
	char	ip[32];
	char	xmlname[64];
	char	enclib[64];
	char	encfun[64];
	char	mapcfg[32];
	char	qmgr[64];
	char	sendmq[64];
	char	recvmq[64];
	char	buftype[32];
	u_char	msgid[32];
	u_char	corrid[32];
	sw_buf_t	locbuf;
	sw_xmltree_t	*xml = NULL;
	sw_xmltree_t	*xmlcfg = NULL;
	sw_loc_vars_t	*vars = NULL;
	sw_pkg_cache_list_t	pkg_cache;

	memset(ip, 0x0, sizeof(ip));
	memset(buf, 0x0, sizeof(buf));
	memset(prdt, 0x0, sizeof(prdt));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(enclib, 0x0, sizeof(enclib));
	memset(encfun, 0x0, sizeof(encfun));
	memset(mapcfg, 0x0, sizeof(mapcfg));
	memset(qmgr, 0x0, sizeof(qmgr));
	memset(sendmq, 0x0, sizeof(sendmq));
	memset(recvmq, 0x0, sizeof(recvmq));
	memset(buftype, 0x00, sizeof(buftype));
	memset(&pkg_cache, 0x0, sizeof(pkg_cache));

	uLog(SW_LOG_DEBUG, "[%s][%d] mqcall begin...", __FILE__, __LINE__);
	vars = pub_get_global_vars();
	loc_get_zd_data(vars, "$listen", buf);
	uLog(SW_LOG_DEBUG, "[%s][%d] $listen=[%s]", __FILE__, __LINE__, buf);

	memset(prdt, 0x0, sizeof(prdt));
	loc_get_zd_data(vars, "$product", prdt);
	uLog(SW_LOG_DEBUG, "[%s][%d] $product=[%s]", __FILE__, __LINE__, prdt);

	loc_get_zd_data(vars, "#sw_ip", ip);
	loc_get_zd_data(vars, "#sw_xmltree", xmlname);
	loc_get_zd_data(vars, "#sw_enc_lib", enclib);
	loc_get_zd_data(vars, "#sw_enc_fun", encfun);
	loc_get_zd_data(vars, "#sw_mapcfg", mapcfg);
	loc_get_zd_data(vars, "#sw_qmgr", qmgr);
	loc_get_zd_data(vars, "#sw_send_mq", sendmq);
	loc_get_zd_data(vars, "#sw_recv_mq", recvmq);
	loc_get_zd_data(vars, "#sw_buftype", buftype);
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, "#sw_timeout", buf);
	if (buf[0] != '\0')
	{
		timeout = atoi(buf);
	}
	else
	{
		timeout = 10;
	}

	if (strcmp(buftype, "STRING") == 0)
	{
		g_msg_format = MSG_STRING;
	}

	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, "#sw_port", buf);
	if (buf[0] == '\0')
	{
		pub_log_error("[%s][%d] port is null", __FILE__, __LINE__)
	}
	port = atoi(buf);

	if (xmlname[0] == '\0')
	{
		uLog(SW_LOG_ERROR, "[%s][%d] xmlname is null!", __FILE__, __LINE__);
		return -1;
	}	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
	xmlcfg = cfg_read_xml(buf);
	if (xmlcfg == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, buf);
		return -1;
	}

	if (mapcfg[0] != '\0')
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s/cfg/common/%s", getenv("SWWORK"), mapcfg);
		xml = cfg_read_xml(buf);
		if (xml == NULL)
		{
			pub_xml_deltree(xmlcfg);
			uLog(SW_LOG_ERROR, "[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, buf);
			return -1;
		}

		ret = map(vars, xml, O_SEND);
		if (ret < 0)
		{
			pub_xml_deltree(xmlcfg);
			pub_xml_deltree(xml);
			uLog(SW_LOG_ERROR, "[%s][%d] map error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] map success!", __FILE__, __LINE__);
	}
	pub_buf_init(&locbuf);
	ret = pkg_out(vars, &locbuf, xmlcfg, &pkg_cache, prdt);
	if (ret <= 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] pack out error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	locbuf.len = ret;	
	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, SW_ENC);
		if (ret < 0)
		{
			pub_xml_deltree(xmlcfg);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] after enc, len=[%d] pkg=[%s]", __FILE__, __LINE__, locbuf.len, locbuf.data);
	}

	g_usejms = 0;
	if (strncmp(qmgr, "JMS:", 4) == 0)
	{
		memset(&g_qmgr, 0x0, sizeof(g_qmgr));
		strncpy(g_qmgr.qmgr, qmgr + 4, sizeof(g_qmgr.qmgr) - 1);

		if (sendmq[0] == '\0')
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Send Queue is null!", __FILE__, __LINE__);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_conn error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		if (recvmq[0] == '\0')
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Recv Queue is null!", __FILE__, __LINE__);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_conn error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		if (ip[0] == '\0' || port <= 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Recv Queue is null!", __FILE__, __LINE__);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_conn error!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		uLog(SW_LOG_INFO,"[%s][%d] JMS: qmgr=[%s] sendmq=[%s] recvmq=[%s]",
				__FILE__, __LINE__, g_qmgr.qmgr, sendmq, recvmq);
		g_usejms = 1;
		g_qmgr.qcnt = 1;
	}

	if (!g_usejms)
	{
		char *ccsid = NULL;
		char *connchl = NULL;
#if defined(AIX)
		sid = 819;
#elif defined(LINUX)
		sid = 1386;
#elif defined(HPUX)
		sid = 1051;
#elif defined(SOLARIS)
		sid = 1386;
#else
		sid = 1208;
#endif
		ccsid = getenv("CCSID");
		if (ccsid != NULL)
		{
			sid = atoi(ccsid);
		}

		connchl = getenv("CONNCHNL");
		ret = mq_javainit(ip, port, qmgr,connchl, sid);	
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			pub_log_error("[%s][%d] init qmgr [%s] error!", __FILE__, __LINE__, qmgr);
			return SW_ERROR;
		}

		ret = mq_javaopen(sendmq, recvmq, 0, O_SEND);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			pub_log_error("[%s][%d] open send mq queue.", __FILE__, __LINE__);
			return SW_ERROR;
		}

		ret = mq_javaopen(recvmq, sendmq, 0, O_RECV);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			pub_log_error("[%s][%d] open recv mq queue.", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		ret = mq_jmsinit(ip, port, g_qmgr.qmgr, sendmq, recvmq, 0);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] jmsinit error!", __FILE__, __LINE__);
			return -1;
		}
	}

	if (!g_usejms)
	{	
		memset(msgid, 0x0, sizeof(msgid));
		memset(corrid, 0x0, sizeof(corrid));
		loc_get_zd_data(vars, SENDMSGID, (char *)msgid);
		loc_get_zd_data(vars, SENDCORRID, (char *)corrid);
		pub_log_info("[%s][%d] msgid=[%s] corrid=[%s]", __FILE__, __LINE__, msgid, corrid);
		ret = mq_javasend(locbuf.data, locbuf.len, msgid, corrid, 10000, 0);
		if (ret < 0)
		{
			pub_xml_deltree(xmlcfg);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_buf_clear(&locbuf);
			pub_log_info("[%s][%d] mq_putmsg error!", __FILE__, __LINE__);
			return -1;
		}
	}
	else
	{
		ret = mq_jmssend(locbuf.data);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR,"[%s][%d] JMS send message error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] PutMsg:[%d]", __FILE__, __LINE__, locbuf.len);

	memset(locbuf.data, 0x00, locbuf.size);
	if (!g_usejms)
	{
		bufferlen = 0;
		ret = mq_javarecv(0, locbuf.data, msgid, corrid, &bufferlen);
		if (ret < 0 && ret != SW_NO_MSG)
		{
			pub_log_error("[%s][%d] JMS get message error!", __FILE__, __LINE__);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		locbuf.len = bufferlen;
	}
	else
	{
		bufferlen = 0;
		ret = mq_jmsrecv(4000, locbuf.data, &bufferlen);
		if (ret < 0 && ret != SW_NO_MSG)
		{
			pub_log_error("[%s][%d] JMS get message error!", __FILE__, __LINE__);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		locbuf.len = bufferlen;
	}


	if (!g_usejms)
	{
		mq_javadestroy();
	}
	else
	{	
		mq_jmsdestroy();
	}

	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, SW_DEC);
		if (ret < 0)
		{
			pub_xml_deltree(xmlcfg);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			pub_buf_clear(&locbuf);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] After dec, msg:[%s] len=[%d]", __FILE__, __LINE__, locbuf.data, locbuf.len);
	}

	xmlcfg->current = xmlcfg->root;
	ret = pkg_in(vars, &locbuf, xmlcfg, &pkg_cache, prdt);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		uLog(SW_LOG_ERROR, "[%s][%d] pack in error! ret=[%d]", __FILE__, __LINE__, ret);
		pub_buf_clear(&locbuf);
		return -1;
	}
	pub_buf_clear(&locbuf);
	pub_xml_deltree(xmlcfg);

	if (mapcfg[0] != '\0')
	{
		xml->current = xml->root;
		ret = map(vars, xml, O_RECV);
		if (ret < 0)
		{
			pub_xml_deltree(xml);
			uLog(SW_LOG_ERROR, "[%s][%d] map error!", __FILE__, __LINE__);
			return -1;
		}
		pub_xml_deltree(xml);
		uLog(SW_LOG_DEBUG, "[%s][%d] map success!", __FILE__, __LINE__);
	}
	loc_var_remove(vars, "#sw_xmltree");
	loc_var_remove(vars, "#sw_enc_lib");
	loc_var_remove(vars, "#sw_enc_fun");
	loc_var_remove(vars, "#sw_data");
	loc_var_remove(vars, "#sw_mapcfg");
	loc_var_remove(vars, "#sw_timeout");
	loc_var_remove(vars, "#sw_qmgr");
	loc_var_remove(vars, "#sw_send_mq");
	loc_var_remove(vars, "#sw_recv_mq");
	uLog(SW_LOG_DEBUG, "[%s][%d] mqcall success!", __FILE__, __LINE__);

	return 0;
}
