#include "tlq_api.h"
#include "tlq_error.h"
#include "lsn_pub.h"
#include "tlq_pub.h"

int tlqcall()
{
	int	ret = 0;
	long	recvlen = 0;
	char	*msgtext = NULL;
	char	recv_name[128];
	char	send_name[128];
	char	xmlname[64];
	char	prdt[32];
	char	enclib[64];
	char	encfun[64];
	char	mapcfg[32];
	char	qmgr[128];
	char    buf[128];
	char	recv_flag[8];
	u_char	msgid[128];
	u_char	corrid[128];
	TLQ_ID	gid;
	TLQ_QCUHDL	qcuId;
	sw_buf_t	locbuf;
	sw_xmltree_t	*xml = NULL;
	sw_xmltree_t	*xmlcfg = NULL;
	sw_loc_vars_t	*vars = NULL;
	sw_pkg_cache_list_t	pkg_cache;

	memset(buf, 0x0, sizeof(buf));
	memset(prdt, 0x0, sizeof(prdt));
	memset(qmgr, 0x0, sizeof(qmgr));
	memset(recv_name, 0x0, sizeof(recv_name));
	memset(send_name, 0x0, sizeof(send_name));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(enclib, 0x0, sizeof(enclib));
	memset(encfun, 0x0, sizeof(encfun));
	memset(mapcfg, 0x0, sizeof(mapcfg));
	memset(msgid, 0x0, sizeof(msgid));
	memset(corrid, 0x0, sizeof(corrid));
	
	vars = pub_get_global_vars();
	
	loc_get_zd_data(vars, "#sw_qmgr", qmgr);
	loc_get_zd_data(vars, "#sw_send_mq", send_name);
	loc_get_zd_data(vars, "#sw_recv_mq", recv_name);
	loc_get_zd_data(vars, "#sw_xmltree", xmlname);
	loc_get_zd_data(vars, "#sw_enc_lib", enclib);
	loc_get_zd_data(vars, "#sw_enc_fun", encfun);
	loc_get_zd_data(vars, "#sw_mapcfg", mapcfg);
	loc_get_zd_data(vars, "$product", prdt);
	
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
		uLog(SW_LOG_ERROR, "[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, buf);
		return SW_ERROR;
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
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_xml_deltree(xmlcfg);
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] pack out error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	uLog(SW_LOG_DEBUG, "[%s][%d] after pack, len=[%d] pkg=[%s]", __FILE__, __LINE__, locbuf.len, locbuf.data);
	
	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, SW_ENC);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] after enc, len=[%d] pkg=[%s]", __FILE__, __LINE__, locbuf.len, locbuf.data);
	}
	
	g_usejms = 0;
	memset(&g_jmsinfo, 0x0, sizeof(g_jmsinfo));
	if (strncmp(qmgr, "JMS:", 4) == 0)
	{
		char	*ptr = NULL;
		strncpy(g_jmsinfo.url, qmgr + 4, sizeof(g_jmsinfo.url) - 1);
		
		if (send_name[0] == '\0')
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

		ptr = strchr(send_name, '|');
		if (ptr == NULL)
		{
			uLog(SW_LOG_ERROR,"[%s][%d] qname [%s] format error, should facroty|jndiqueue!",
				__FILE__, __LINE__, send_name);
					
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_conn error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		strncpy(g_jmsinfo.factory, send_name, ptr - send_name);
		strcpy(g_jmsinfo.jsendqueue, ptr + 1);
		
		uLog(SW_LOG_INFO,"[%s][%d] JMS: url=[%s] facroty=[%s] jsendqueue=[%s]",
			__FILE__, __LINE__, g_jmsinfo.url, g_jmsinfo.factory, g_jmsinfo.jsendqueue);
		g_usejms = 1;

		memset(&g_qmgr, 0x0, sizeof(g_qmgr));
		g_qmgr.qcnt = 1;
	}

	if (!g_usejms)
	{
		ret = tlq_conn(&gid);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_conn error!", __FILE__, __LINE__);
			return -1;
		}
	
		ret = tlq_open(&gid, &qcuId, qmgr);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			tlq_disconn(&gid);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_open error!", __FILE__, __LINE__);
			return -1;	
		}
	}
	else
	{
		ret = tlq_jmsinit(g_jmsinfo.url, g_jmsinfo.factory, g_jmsinfo.jsendqueue, NULL, -1);
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
	
#ifndef __SW_USE_JMS__
	memset(msgid, 0x0, sizeof(msgid));
	memset(corrid, 0x0, sizeof(corrid));
	loc_get_zd_data(vars, SENDMSGID, msgid);
	loc_get_zd_data(vars, SENDCORRID, corrid);
	uLog(SW_LOG_INFO,"[%s][%d] msgid=[%s] corrid=[%s]", __FILE__, __LINE__, msgid, corrid);
	ret =  tlq_putmsg(&gid, &qcuId, send_name, locbuf.data, locbuf.len, msgid, corrid);
	if (ret < 0)
	{
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_xml_deltree(xmlcfg);
		pub_buf_clear(&locbuf);
		tlq_close(&gid, &qcuId);
		tlq_disconn(&gid);
		uLog(SW_LOG_ERROR, "[%s][%d] tlq_putmsg error!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] PutMsg:[%d]", __FILE__, __LINE__, locbuf.len);
	pub_buf_clear(&locbuf);
	
	memset(recv_flag, 0x0, sizeof(recv_flag));
	loc_get_zd_data(vars, "$synresflag", recv_flag);
	if (recv_flag[0] != '1')
	{
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_xml_deltree(xmlcfg);
		tlq_close(&gid, &qcuId);
		tlq_disconn(&gid);
		
		uLog(SW_LOG_INFO, "[%s][%d] not need synchronous response!", __FILE__, __LINE__);
		return SW_OK;
	}
	
	memset(corrid, 0x0, sizeof(corrid));
	memcpy(corrid, msgid, sizeof(corrid) - 1);
	memset(msgid, 0x0, sizeof(msgid));
	ret = tlq_getmsg(&gid, &qcuId, recv_name, &msgtext, &recvlen, msgid, corrid, 10);
	if (ret < 0) 
	{
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_xml_deltree(xmlcfg);
		tlq_close(&gid, &qcuId);
		tlq_disconn(&gid);
		uLog(SW_LOG_ERROR, "[%s][%d] tlq_getmsg error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
#else	
	if (g_usejms)
	{
		ret = tlq_jmssend(locbuf.data);
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

		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_xml_deltree(xmlcfg);
		uLog(SW_LOG_INFO, "[%s][%d] Don't recv response!", __FILE__, __LINE__);	

		return SW_OK;
	}
	else
	{
		memset(msgid, 0x0, sizeof(msgid));
		memset(corrid, 0x0, sizeof(corrid));
		loc_get_zd_data(vars, SENDMSGID, (char *)msgid);
		loc_get_zd_data(vars, SENDCORRID, (char *)corrid);
		uLog(SW_LOG_INFO,"[%s][%d] msgid=[%s] corrid=[%s]", __FILE__, __LINE__, msgid, corrid);
		ret = tlq_putmsg(&gid, &qcuId, send_name, locbuf.data, locbuf.len, msgid, corrid);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			tlq_close(&gid, &qcuId);
			tlq_disconn(&gid);
			uLog(SW_LOG_ERROR, "[%s][%d] tlq_putmsg error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] PutMsg:[%d]", __FILE__, __LINE__, locbuf.len);
	pub_buf_clear(&locbuf);
	
	memset(recv_flag, 0x0, sizeof(recv_flag));
	loc_get_zd_data(vars, "$synresflag", recv_flag);
	if (recv_flag[0] != '1')
	{
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		
		pub_xml_deltree(xmlcfg);
		if (g_usejms != 1)
		{
			tlq_close(&gid, &qcuId);
			tlq_disconn(&gid);
		}
		
		uLog(SW_LOG_INFO, "[%s][%d] not need synchronous response!", __FILE__, __LINE__);	
		return SW_OK;
	}
	
	if (!g_usejms)
	{
		memset(corrid, 0x0, sizeof(corrid));
		memcpy(corrid, msgid, sizeof(corrid) - 1);
		memset(msgid, 0x0, sizeof(msgid));
		ret = tlq_getmsg(&gid, &qcuId, recv_name, &msgtext, &recvlen, msgid, corrid, 3);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			tlq_close(&gid, &qcuId);
			tlq_disconn(&gid);
			return SW_ERROR;
		}
	}
#endif
	
	pub_log_bin(SW_LOG_DEBUG, msgtext, recvlen, "[%s][%d] Recv msg:[%d]", __FILE__, __LINE__, recvlen);
	memset(&locbuf, 0x0, sizeof(locbuf));
	pub_buf_init(&locbuf);
	pub_buf_chksize(&locbuf, recvlen);
	memcpy(locbuf.data, msgtext, recvlen);
	locbuf.len = recvlen;
	
	if (!g_usejms)
	{
		tlq_close(&gid, &qcuId);
		tlq_disconn(&gid);
	}
	
	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, SW_DEC);
		if (ret < 0)
		{
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			pub_xml_deltree(xmlcfg);
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] After dec, msg:[%s] len=[%d]", __FILE__, __LINE__, msgtext, recvlen);
	}
	
	ret = pkg_in(vars, &locbuf, xmlcfg, &pkg_cache, prdt);
	if (ret < 0)
	{
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_xml_deltree(xmlcfg);
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] pack in error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	pub_buf_clear(&locbuf);
	pub_xml_deltree(xmlcfg);
	uLog(SW_LOG_INFO,"[%s][%d] Pack in success!", __FILE__, __LINE__);
	
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
	uLog(SW_LOG_DEBUG, "[%s][%d] tlqcall success!", __FILE__, __LINE__);
	
	return 0;
}
