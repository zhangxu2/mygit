#include "lsn_pub.h"
#include "mq_pub.h"
msg_format_enum_t g_msg_format = MSG_NONE;

int mqcall()
{
	int	ret = 0;
	int	timeout = 0;
	char	buf[128];
	char	prdt[32];
	char	xmlname[64];
	char	enclib[64];
	char	encfun[64];
	char	mapcfg[32];
	char	qmgr[64];
	char	sendmq[64];
	char	recvmq[64];
	u_char	msgid[32];
	u_char	corrid[32];
	MQHCONN	Hcon;
	MQHOBJ	Hobjs;
	MQHOBJ	Hobjr;
	sw_buf_t	locbuf;
	sw_xmltree_t	*xml = NULL;
	sw_xmltree_t	*xmlcfg = NULL;
	sw_loc_vars_t	*vars = NULL;
	sw_pkg_cache_list_t	pkg_cache;

	memset(buf, 0x0, sizeof(buf));
	memset(prdt, 0x0, sizeof(prdt));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(enclib, 0x0, sizeof(enclib));
	memset(encfun, 0x0, sizeof(encfun));
	memset(mapcfg, 0x0, sizeof(mapcfg));
	memset(qmgr, 0x0, sizeof(qmgr));
	memset(sendmq, 0x0, sizeof(sendmq));
	memset(recvmq, 0x0, sizeof(recvmq));
	memset(&pkg_cache, 0x0, sizeof(pkg_cache));

	uLog(SW_LOG_DEBUG, "[%s][%d] mqcall begin...", __FILE__, __LINE__);
	vars = pub_get_global_vars();
	loc_get_zd_data(vars, "$listen", buf);
	uLog(SW_LOG_DEBUG, "[%s][%d] $listen=[%s]", __FILE__, __LINE__, buf);

	memset(prdt, 0x0, sizeof(prdt));
	loc_get_zd_data(vars, "$product", prdt);
	uLog(SW_LOG_DEBUG, "[%s][%d] $product=[%s]", __FILE__, __LINE__, prdt);

	loc_get_zd_data(vars, "#sw_xmltree", xmlname);
	loc_get_zd_data(vars, "#sw_enc_lib", enclib);
	loc_get_zd_data(vars, "#sw_enc_fun", encfun);
	loc_get_zd_data(vars, "#sw_mapcfg", mapcfg);
	loc_get_zd_data(vars, "#sw_qmgr", qmgr);
	loc_get_zd_data(vars, "#sw_send_mq", sendmq);
	loc_get_zd_data(vars, "#sw_recv_mq", recvmq);
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

	memset(msgid, 0x0, sizeof(msgid));
	ret = mq_conn(&Hcon, qmgr);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] conn qmgr [%s] error!", 
				__FILE__, __LINE__, qmgr);
		return -1;
	}

	ret = mqm_open(Hcon, &Hobjs, sendmq, O_SEND);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] open qmgr:[%s] qname:[%s] error!",
				__FILE__, __LINE__, qmgr, sendmq);
		return -1;
	}

	ret = mqm_open(Hcon, &Hobjr, recvmq, O_RECV);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] open qmgr:[%s] qname:[%s] error!",
				__FILE__, __LINE__, qmgr, recvmq);
		return -1;
	}

	memset(msgid, 0x0, sizeof(msgid));
	memset(corrid, 0x0, sizeof(corrid));
	loc_get_zd_data(vars, SENDMSGID, (char *)msgid);
	loc_get_zd_data(vars, SENDCORRID, (char *)corrid);
	pub_log_info("[%s][%d] msgid=[%s] corrid=[%s]", __FILE__, __LINE__, msgid, corrid);
	ret = mq_putmsg(Hcon, Hobjs, locbuf.data, locbuf.len, msgid, corrid, 0);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] mq_putmsg error!", __FILE__, __LINE__);
		return -1;
	}
	pub_buf_clear(&locbuf);

	pub_buf_init(&locbuf);
	memset(corrid, 0x0, sizeof(corrid));
	memcpy(corrid, msgid, sizeof(corrid) - 1);
	memset(msgid, 0x0, sizeof(msgid));
	MQLONG	rlen = locbuf.size;
	ret = mq_getmsg(Hcon, Hobjr, locbuf.data, &rlen, msgid, corrid, timeout);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		pub_log_error("[%s][%d] mq_getmsg error!", __FILE__, __LINE__);
		return -1;
	}
	locbuf.len = rlen;
	uLog(SW_LOG_DEBUG, "[%s][%d] recv msg:[%s] len=[%d]", __FILE__, __LINE__, locbuf.data, locbuf.len);

	mq_freeobj(Hcon, &Hobjs);
	mq_freeobj(Hcon, &Hobjr);
	mq_freeconn(&Hcon);

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
