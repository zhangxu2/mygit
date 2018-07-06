#include "lsn_pub.h"
#include "soap_pub.h"

int clear_vars(sw_loc_vars_t *vars)
{
	loc_var_remove(vars, "#sw_port");
	loc_var_remove(vars, "#sw_ip");
	loc_var_remove(vars, "#sw_xmltree");
	loc_var_remove(vars, "#sw_buftype");
	loc_var_remove(vars, "#sw_enc_lib");
	loc_var_remove(vars, "#sw_enc_fun");
	loc_var_remove(vars, "#sw_file_fun");
	loc_var_remove(vars, "#sw_mapcfg");
	loc_var_remove(vars, "#sw_service");
	loc_var_remove(vars, "#sw_loadlib");

	return 0;
}

int soapcall_set_xmlns(sw_xmltree_t *xmlcfg, struct soap *soap)
{
	int	cnt = 0;
	size_t	n = 0;
	size_t	len = 0;
	char	id[32];
	char	ns[128];
	char	*ptr = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	cnt = 0;
	xml = xmlcfg;
	node = pub_xml_locnode(xml, ".CBM.XMLNS");
	while (node != NULL)
	{
		if (strcmp(node->name, "XMLNS") != 0)
		{
			node = node->next;
			continue;
		}
		pub_log_info("[%s][%d] xmlns:[%s]", __FILE__, __LINE__, node->value);	
		cnt++;
		node = node->next;
	}

	if (cnt > 0 && soap->namespaces)
	{
		g_namespaces = (struct Namespace*)SOAP_MALLOC(soap, (cnt + 1) * sizeof(struct Namespace));
		if (g_namespaces == NULL)
		{
			pub_log_error("[%s][%d] SOAP MALLOC error!", __FILE__, __LINE__);
			return soap->error;
		}
		memset(g_namespaces, 0x00, (cnt + 1) * sizeof(struct Namespace));

		char	*pid = NULL;
		char	*pns = NULL;
		n = 0;
		node = pub_xml_locnode(xml, ".CBM.XMLNS");
		while (node != NULL)
		{
			if (strcmp(node->name, "XMLNS") != 0)
			{
				node = node->next;
				continue;
			}

			memset(id, 0x0, sizeof(id));
			memset(ns, 0x0, sizeof(ns));
			ptr = strchr(node->value, '=');
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] XMLNS [%s] format error!", __FILE__, __LINE__, node->value);
				return soap->error = SOAP_ERR;
			}
			memcpy(id, node->value, ptr - node->value);
			strcpy(ns, ptr + 1);

			len = strlen(id);
			pid = (char *)SOAP_MALLOC(soap, len + 1);
			if (pid == NULL)
			{
				pub_log_error("[%s][%d] SOAP MALLOC error!", __FILE__, __LINE__);
				return soap->error;
			}
			memset(pid, 0x0, len + 1);
			strncpy(pid, id, len);

			len = strlen(ns);
			pns = (char *)SOAP_MALLOC(soap, len + 1);
			if (pns == NULL)
			{
				pub_log_error("[%s][%d] SOAP MALLOC error!", __FILE__, __LINE__);
				return soap->error;
			}
			memset(pns, 0x0, len + 1);
			strncpy(pns, ns, len);

			g_namespaces[n].id = pid;
			g_namespaces[n].ns = pns;
			g_namespaces[n].in = NULL;
			g_namespaces[n].out = NULL;
			n++;

			node = node->next;
		}
		g_namespaces[n].id = NULL;
		g_namespaces[n].ns = NULL;
		g_namespaces[n].in = NULL;
		g_namespaces[n].out = NULL;

		soap->namespaces = g_namespaces;
	}

	return SW_OK;
}

sw_int_t soapcall_init(char *chnlname)
{
	char	logdir[128];

	memset(&ssoap, 0x0, sizeof(ssoap));
	g_soap = &ssoap;
	soap_init(g_soap);
	soap_set_mode(g_soap, SOAP_C_UTFSTRING);

#ifdef SOAP_DEBUG
	memset(logdir, 0x0, sizeof(logdir));
	sprintf(logdir, "%s/log/syslog/soapcall_%s_soaprecv.log", getenv("SWWORK"), chnlname);
	soap_set_recv_logfile(g_soap, logdir);
	memset(logdir, 0x0, sizeof(logdir));
	sprintf(logdir, "%s/log/syslog/soapcall_%s_soapsend.log", getenv("SWWORK"), chnlname);
	soap_set_sent_logfile(g_soap, logdir);
	memset(logdir, 0x0, sizeof(logdir));
	sprintf(logdir, "%s/log/syslog/soapcall_%s_soapdebug.log", getenv("SWWORK"), chnlname);
	soap_set_test_logfile(g_soap, logdir);
#endif
	pub_log_info("[%s][%d] soapcall_init success!", __FILE__, __LINE__);

	return 0;
}

sw_int_t soapcall_destroy()
{
	soap_destroy(g_soap);
	soap_end(g_soap);
	soap_done(g_soap);

	return 0;
}

int soapcall()
{
	int	ret    = 0;
	int sockid = 0;
	char	ip[256];
	char 	tmp[128];
	char	buf[128];
	char	port[32];
	char	prdt[32];
	char	enclib[128];
	char	encfun[128];
	char	xmlname[64];
	char	mapcfg[32];
	char	service[128];
	char	vservice[128];
	char	chnlname[128];
	sw_buf_t	locbuf;
	fd_set	readfds;
	struct  timeval	timeout;
	sw_xmltree_t	*xml = NULL;
	sw_xmltree_t	*xmlcfg = NULL;
	sw_loc_vars_t	*vars = NULL;
	sw_pkg_cache_list_t	pkg_cache;

	memset(ip, 0x0, sizeof(ip));
	memset(buf, 0x0, sizeof(buf));
	memset(port, 0x0, sizeof(port));
	memset(prdt, 0x0, sizeof(prdt));
	memset(enclib, 0x0, sizeof(enclib));
	memset(encfun, 0x0, sizeof(encfun));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(mapcfg, 0x0, sizeof(mapcfg));
	memset(service, 0x0, sizeof(service));
	memset(vservice, 0x0, sizeof(vservice));
	memset(chnlname, 0x0, sizeof(chnlname));
	memset(&pkg_cache, 0x0, sizeof(pkg_cache));

	uLog(SW_LOG_DEBUG, "[%s][%d] soapcall begin...", __FILE__, __LINE__);
	vars = pub_get_global_vars();
	loc_get_zd_data(vars, "$listen", buf);
	uLog(SW_LOG_DEBUG, "[%s][%d] $listen=[%s]", __FILE__, __LINE__, buf);

	memset(prdt, 0x0, sizeof(prdt));
	loc_get_zd_data(vars, "$product", prdt);
	uLog(SW_LOG_DEBUG, "[%s][%d] $product=[%s]", __FILE__, __LINE__, prdt);

	loc_get_zd_data(vars, "#sw_ip", ip);
	loc_get_zd_data(vars, "#sw_port", port);
	loc_get_zd_data(vars, "#sw_xmltree", xmlname);
	loc_get_zd_data(vars, "#sw_mapcfg", mapcfg);
	loc_get_zd_data(vars, "#sw_enc_lib", enclib);
	loc_get_zd_data(vars, "#sw_enc_fun", encfun);
	loc_get_zd_data(vars, "#sw_service", vservice);
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, "#sw_timeout", buf);
	if (buf[0] != '\0')
	{
		timeout.tv_sec = atoi(buf);
	}
	else
	{
		timeout.tv_sec = 5;
	}
	clear_vars(vars);

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
			pub_xml_deltree(xml);
			pub_xml_deltree(xmlcfg);
			uLog(SW_LOG_ERROR, "[%s][%d] map error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] map success!", __FILE__, __LINE__);
	}

	pub_buf_init(&locbuf);
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
	}
	pub_buf_clear(&locbuf);


	memset(buf, 0x0, sizeof(buf));
	if (ip[0] == '$' || ip[0] == '#')
	{
		memset(tmp, 0x0, sizeof(tmp));
		loc_get_zd_data(vars, ip, tmp);
		if (strncmp(tmp, "http:", 5) == 0 || strncmp(tmp, "https:", 6) == 0)
		{
			sprintf(buf, "%s", tmp);
		}
		else
		{
			sprintf(buf, "http://%s", tmp);
		}
	
	}
	else if (strncmp(ip, "http:", 5) == 0 || strncmp(ip, "https:", 6) == 0)
	{
		sprintf(buf, "%s", ip);
	}
	else
	{
		sprintf(buf, "http://%s", ip);
	}

	if (vservice[0] != '\0')
	{
		memset(service, 0x0, sizeof(service));
		if (vservice[0] == '#' || vservice[0] == '$')
		{
			loc_get_zd_data(vars, vservice, service);
		}
		else
		{
			strcpy(service, vservice);
		}
		strcat(buf, "/");
		strcat(buf, service);
		pub_log_info("[%s][%d] service=[%s][%s]", __FILE__, __LINE__, vservice, service);
	}
	loc_set_zd_data(vars, "$soapendpoint", buf);

	memset(chnlname, 0x0, sizeof(chnlname));
	get_zd_data("$sw_call_chnl", chnlname);
	pub_log_info("[%s][%d] Call chnl [%s]...", __FILE__, __LINE__, chnlname);
	ret = soapcall_init(chnlname);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_log_info("[%s][%d] Soapcall init error!", __FILE__, __LINE__);
		return -1;
	}

	ret = soapcall_set_xmlns(xmlcfg, g_soap);
	if (ret != SOAP_OK)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_log_error("[%s][%d] Soapcall set xmlns error!", __FILE__, __LINE__);
		return -1;
	}

	if (g_soap->namespaces && !g_soap->local_namespaces)
	{
		register struct Namespace *ns1;
		for (ns1 = g_soap->namespaces; ns1->id; ns1++)
		{
			pub_log_info("[%s][%d] namespaces:id=[%s] ns=[%s]",
					__FILE__, __LINE__, ns1->id, ns1->ns);
		}
	}

	ret = soap_client_request(g_soap, vars, xmlcfg);
	if (ret != SOAP_OK)
	{
		pub_log_error("[%s][%d] soap_client_request error!", __FILE__, __LINE__);
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		soapcall_destroy();
		return -1;
	}

	sockid = g_soap->socket;
	uLog(SW_LOG_DEBUG, "[%s][%d] soap socket[%d]", __FILE__, __LINE__, g_soap->socket);
	FD_ZERO(&readfds);
	FD_SET(sockid, &readfds);
	while (1)
	{
		ret = select(sockid + 3, &readfds, NULL, NULL, &timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				uLog(SW_LOG_DEBUG, "[%s][%d] interrupt!", __FILE__, __LINE__);
				continue;
			}
			else
			{
				pub_xml_deltree(xmlcfg);
				if (mapcfg[0] != '\0')
				{
					pub_xml_deltree(xml);
				}
				soapcall_destroy();
				uLog(SW_LOG_DEBUG, "[%s][%d] select error! errno=[%d]:[%s]",
						__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
		}
		else if (ret == 0)
		{
			pub_xml_deltree(xmlcfg);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			soapcall_destroy();
			lsn_set_err(vars, ERR_LSNOUT);
			uLog(SW_LOG_ERROR, "[%s][%d] timeout!", __FILE__, __LINE__);
			return -1;
		}
		break;
	}

	ret = soap_client_response(g_soap, vars, xmlcfg);
	if (ret != SOAP_OK)
	{
		pub_log_error("[%s][%d] soap_client_response error!", __FILE__, __LINE__);
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		soapcall_destroy();
		return -1;
	}
	soapcall_destroy();

	pub_buf_init(&locbuf);
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
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] decrypt error!", __FILE__, __LINE__);
			return -1;
		}
	}
	pub_buf_clear(&locbuf);
	pub_xml_deltree(xmlcfg);
	pub_log_info("[%s][%d] Soap call success!", __FILE__, __LINE__);

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
	uLog(SW_LOG_DEBUG, "[%s][%d] soapcall success!", __FILE__, __LINE__);

	return 0;
}

