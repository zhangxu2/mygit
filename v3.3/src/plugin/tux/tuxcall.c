#include "lsn_pub.h"
#include "atmi.h"

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
	
	return 0;
}

int tuxcall()
{
	int	ret = 0;
	int	port = 0;
	char 	tmp[128];
	char	buf[128];
	char	ip[32];
	char	prdt[32];
	char	envbuf[64];
	char	xmlname[64];
	char	enclib[64];
	char	encfun[64];
	char	buftype[32];
	char	filefun[64];
	char	mapcfg[32];
	char	service[32];
	char	*sendbuf = NULL;
	char	*recvbuf = NULL;
	long	sendlen = 0;
	long	recvlen = 0;
	sw_buf_t	locbuf;
	sw_xmltree_t	*xml = NULL;
	sw_xmltree_t	*xmlcfg = NULL;
	sw_loc_vars_t	*vars = NULL;
	sw_pkg_cache_list_t	pkg_cache;
	
	memset(buf, 0x0, sizeof(buf));
	memset(ip, 0x0, sizeof(ip));
	memset(prdt, 0x0, sizeof(prdt));
	memset(envbuf, 0x0, sizeof(envbuf));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(enclib, 0x0, sizeof(enclib));
	memset(encfun, 0x0, sizeof(encfun));
	memset(buftype, 0x0, sizeof(buftype));
	memset(filefun, 0x0, sizeof(filefun));
	memset(mapcfg, 0x0, sizeof(mapcfg));
	memset(service, 0x0, sizeof(service));
	memset(&pkg_cache, 0x0, sizeof(pkg_cache));
	
	uLog(SW_LOG_DEBUG, "[%s][%d] tuxcall begin...", __FILE__, __LINE__);
	vars = pub_get_global_vars();
	loc_get_zd_data(vars, "$listen", buf);
	uLog(SW_LOG_DEBUG, "[%s][%d] $listen=[%s]", __FILE__, __LINE__, buf);
	
	memset(prdt, 0x0, sizeof(prdt));
	loc_get_zd_data(vars, "$product", prdt);
	uLog(SW_LOG_DEBUG, "[%s][%d] $product=[%s]", __FILE__, __LINE__, prdt);
	
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, "#sw_port", buf);
	port = atoi(buf);
	loc_get_zd_data(vars, "#sw_ip", ip);
	loc_get_zd_data(vars, "#sw_xmltree", xmlname);
	loc_get_zd_data(vars, "#sw_buftype", buftype);
	loc_get_zd_data(vars, "#sw_enc_lib", enclib);
	loc_get_zd_data(vars, "#sw_enc_fun", encfun);
	loc_get_zd_data(vars, "#sw_file_fun", filefun);
	loc_get_zd_data(vars, "#sw_mapcfg", mapcfg);
	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, "#sw_service", buf);
	if (buf[0] == '#' || buf[0] == '$')
	{
		loc_get_zd_data(vars, buf, service);
	}
	else
	{
		strncpy(service, buf, sizeof(service) - 1);
	}
	
	if (buftype[0] == '\0')
	{
		strcpy(buftype, "STRING");
	}
	
	if (ip[0] == '\0' || port <= 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Error: IP=[%s] PORT=[%d]", __FILE__, __LINE__, ip, port);
		return -1;
	}
	
	clear_vars(vars);
	memset(envbuf, 0x0, sizeof(envbuf));
	if (ip[0] == '#' || ip[0] == '$')
	{
		memset(tmp, 0x0, sizeof(tmp));
		loc_get_zd_data(vars, ip, tmp);
		char *p = tmp;
		char *q = NULL;
		q = strchr(p, ':');
		if (q == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] ip config=[%s] format error, usage[IP:PORT]", __FILE__, __LINE__, tmp);
			return -1;
		}
		port = 0;
		port = atoi(q+1);
		p[q-p] = '\0';
		sprintf(envbuf, "WSNADDR=//%s:%d", p, port);
	}
	else
	{
		sprintf(envbuf, "WSNADDR=//%s:%d", ip, port);
	}

	ret = putenv(envbuf);
	if (ret < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] putenv [%s] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, envbuf, errno, strerror(errno));
		return -1;
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
	pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] Pack:[%d]", 
		__FILE__, __LINE__, locbuf.len);   

	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, O_SEND);
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
		pub_log_bin(SW_LOG_DEBUG, locbuf.data, locbuf.len, "[%s][%d] after enc:[%d]", 
			__FILE__, __LINE__, locbuf.len);
	}
	
	if (tpinit((TPINIT *)NULL) == -1)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] tpinit error! tperrno=[%d]:[%s]", 
			__FILE__, __LINE__, tperrno, tpstrerror(tperrno));
		return -1;
	}
	
	sendbuf = (char *)tpalloc(buftype, NULL, MAX_PACK_LEN);
	if (sendbuf == NULL)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] tpalloc error! tperrno=[%d]:[%s]",
			__FILE__, __LINE__, tperrno, tpstrerror(errno));
		tpterm();
		return -1;
	}
	memcpy(sendbuf, locbuf.data, locbuf.len);
	sendlen = locbuf.len;
	pub_buf_clear(&locbuf);

	recvbuf = (char *)tpalloc(buftype, NULL, MAX_PACK_LEN);
	if (recvbuf == NULL)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		uLog(SW_LOG_ERROR, "[%s][%d] tpalloc error! tperrno=[%d]:[%s]",
			__FILE__, __LINE__, tperrno, tpstrerror(errno));
		tpfree(sendbuf);
		tpterm();
		return -1;
	}

	if (filefun[0] != '\0')
	{
		ret = run_file_fun(filefun, vars, O_SEND, 0, NULL);
		if (ret < 0)
		{
			pub_xml_deltree(xmlcfg);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			uLog(SW_LOG_ERROR, "[%s][%d] send file error!", __FILE__, __LINE__);
			tpfree(sendbuf);
			tpfree(recvbuf);
			tpterm();
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] send file success!", __FILE__, __LINE__);
	}
	
	recvlen = 0;
	ret = tpcall(service, sendbuf, sendlen, &recvbuf, &recvlen, (long)0);
	if (ret < 0)
	{
		pub_xml_deltree(xmlcfg);
		if (mapcfg[0] != '\0')
		{
			pub_xml_deltree(xml);
		}
		uLog(SW_LOG_ERROR, "[%s][%d] tpcall error! tperrno=[%d]:[%s]",
			__FILE__, __LINE__, tperrno, tpstrerror(tperrno));
		tpfree(sendbuf);
		tpfree(recvbuf);
		tpterm();
		return -1;
	}
	pub_log_bin(SW_LOG_DEBUG, recvbuf, recvlen, "[%s][%d] recv msg:[%d]", 
		__FILE__, __LINE__, recvlen);
	memset(&locbuf, 0x0, sizeof(locbuf));
	pub_buf_init(&locbuf);
	pub_buf_chksize(&locbuf, recvlen);
	memcpy(locbuf.data, recvbuf, recvlen);
	locbuf.len = recvlen;
	tpfree(sendbuf);
	tpfree(recvbuf);
	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, O_RECV);
		if (ret < 0)
		{
			pub_xml_deltree(xmlcfg);
			if (mapcfg[0] != '\0')
			{
				pub_xml_deltree(xml);
			}
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			tpterm();
			pub_buf_clear(&locbuf);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] After dec: msg=[%s] len=[%d]", __FILE__, __LINE__, recvbuf, recvlen);
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
		tpterm();
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

	if (filefun[0] != '\0')
	{
		ret = run_file_fun(filefun, vars, O_RECV, 0, NULL);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] send file error!", __FILE__, __LINE__);
			tpterm();
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] send file success!", __FILE__, __LINE__);
	}
	tpterm();
	
	uLog(SW_LOG_DEBUG, "[%s][%d] tuxcall success!", __FILE__, __LINE__);
	
	return 0;
}
