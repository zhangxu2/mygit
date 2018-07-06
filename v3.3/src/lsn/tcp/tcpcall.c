#include "lsn_pub.h"

int clear_vars(sw_loc_vars_t *vars)
{
	loc_var_remove(vars, "#sw_port");
	loc_var_remove(vars, "#sw_ip");
	loc_var_remove(vars, "#sw_xmltree");
	loc_var_remove(vars, "#sw_enc_lib");
	loc_var_remove(vars, "#sw_enc_fun");
	loc_var_remove(vars, "#sw_data");
	loc_var_remove(vars, "#sw_file_fun");
	loc_var_remove(vars, "#sw_mapcfg");
	loc_var_remove(vars, "#sw_timeout");
	
	return 0;
}

int tcpcall()
{
	int	ret = 0;
	int	port = 0;
	int	sockid = 0;
	int 	index = -1;
	char 	*p = NULL;
	char 	*q = NULL;
	char	buf[128];
	char	ip[32];
	char	data[32];
	char	prdt[32];
	char	xmlname[64];
	char	enclib[64];
	char	encfun[64];
	char	filefun[64];
	char	mapcfg[32];
	fd_set	readfds;
	sw_buf_t	locbuf;
	sw_xmltree_t	*xml = NULL;
	sw_xmltree_t	*xmlcfg = NULL;
	sw_loc_vars_t	*vars = NULL;
	struct  timeval	timeout;
	static int first_use = 1;
	sw_pkg_cache_list_t	*pkg_cache = NULL;
	static sw_pkg_cache_list_t	xmlcfg_cache;
	static sw_pkg_cache_list_t	mapcfg_cache;
	
	memset(buf, 0x0, sizeof(buf));
	memset(ip, 0x0, sizeof(ip));
	memset(data, 0x0, sizeof(data));
	memset(prdt, 0x0, sizeof(prdt));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(enclib, 0x0, sizeof(enclib));
	memset(encfun, 0x0, sizeof(encfun));
	memset(filefun, 0x0, sizeof(filefun));
	memset(mapcfg, 0x0, sizeof(mapcfg));
	memset(&timeout, 0x0, sizeof(timeout));
	if (first_use)
	{
		memset(&xmlcfg_cache, 0x0, sizeof(xmlcfg_cache));
		memset(&mapcfg_cache, 0x0, sizeof(mapcfg_cache));
		first_use = 0;
	}	
	pub_log_debug("[%s][%d] tcpcall begin...", __FILE__, __LINE__);
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
	loc_get_zd_data(vars, "#sw_enc_lib", enclib);
	loc_get_zd_data(vars, "#sw_enc_fun", encfun);
	loc_get_zd_data(vars, "#sw_data", data);
	loc_get_zd_data(vars, "#sw_file_fun", filefun);
	loc_get_zd_data(vars, "#sw_mapcfg", mapcfg);
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
	extvars_clear();
	
	if (ip[0] == '\0' || port <= 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Error: IP=[%s] PORT=[%d]", __FILE__, __LINE__, ip, port);
		return -1;
	}
	uLog(SW_LOG_DEBUG, "[%s][%d] IP:[%s] PORT:[%d]", __FILE__, __LINE__, ip, port);
	
	if (xmlname[0] == '\0')
	{
		uLog(SW_LOG_ERROR, "[%s][%d] xmlname is null!", __FILE__, __LINE__);
		return -1;
	}
	
	index = get_xmltree_from_cache(&xmlcfg_cache, xmlname);
	if (index < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Create xml tree error! xmlname=[%s]",
			__FILE__, __LINE__, xmlname);
		return -1;
	}
	xmlcfg = xmlcfg_cache.processor[index].xml;
	if (xmlcfg_cache.processor[index].handle == NULL)
	{
		xmlcfg_cache.processor[index].handle = malloc(sizeof(sw_pkg_cache_list_t));
		if (xmlcfg_cache.processor[index].handle == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] malloc error!", __FILE__, __LINE__);
			return -1;
		}
		memset(xmlcfg_cache.processor[index].handle, 0x0, sizeof(sw_pkg_cache_list_t));

	}

	pkg_cache = xmlcfg_cache.processor[index].handle;

	if (mapcfg[0] != '\0')
	{
		index = get_xmltree_from_cache(&mapcfg_cache, mapcfg);
		if (index < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, mapcfg);
			return -1;
		}
		xml = mapcfg_cache.processor[index].xml;
		
		ret = map(vars, xml, O_SEND);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] map error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] map success!", __FILE__, __LINE__);
	}
	
	pub_buf_init(&locbuf);
	ret = pkg_out(vars, &locbuf, xmlcfg, pkg_cache, prdt);
	if (ret <= 0)
	{
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] pack out error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	locbuf.len = ret;
	uLog(SW_LOG_DEBUG, "[%s][%d] after pack, len=[%d] pkg=[%s]", __FILE__, __LINE__, locbuf.len, locbuf.data);
	
	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, SW_ENC);
		if (ret < 0)
		{
			pub_buf_clear(&locbuf);
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] after enc, len=[%d] pkg=[%s]", __FILE__, __LINE__, locbuf.len, locbuf.data);
	}

	if (ip[0] == '$' || ip[0] == '#')
	{
		memset(buf, 0x00, sizeof(buf));
		loc_get_zd_data(vars, ip, buf);
		p = buf;
		q = strchr(buf, ':');
		if (q == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] ip/port[%s] configure format error, usage[IP:PORT]", __FILE__, __LINE__, buf);
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		port = 0;
		port = atoi(q+1);
 		p[q-p] = '\0';
		sockid = lsn_pub_connect(p, port);
	}
	else
	{
		sockid = lsn_pub_connect(ip, port);
	}
	
	if (sockid <= 0)
	{
		lsn_set_err(vars, ERR_CONNECT);
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] Connect [%s]:[%d] error! errno=[%d]:[%s]", 
			__FILE__, __LINE__, ip, port, errno, strerror(errno));
		return -1;
	}
	
	ret = lsn_pub_send(sockid, locbuf.data, locbuf.len);
	if (ret < 0)
	{
		lsn_set_err(vars, ERR_SEND);
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] send msg error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1; 
	}
	pub_buf_clear(&locbuf);
	extvars_clear();
	
	if (filefun[0] != '\0')
	{
		ret = run_file_fun(filefun, vars, O_SEND, sockid, NULL);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] send file error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] send file success!", __FILE__, __LINE__);
	}
	
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
				close(sockid);
				uLog(SW_LOG_ERROR, "[%s][%d] select error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
		}
		else if (ret == 0)
		{
			close(sockid);
			lsn_set_err(vars, ERR_LSNOUT);
			uLog(SW_LOG_ERROR, "[%s][%d] timeout!", __FILE__, __LINE__);
			return -1;
		}
		break;
	}
	
	pub_buf_init(&locbuf);
	ret = lsn_pub_recv(data, sockid, &locbuf);
	if (ret < 0)
	{
		lsn_set_err(vars, ERR_RECV);
		close(sockid);
		pub_buf_clear(&locbuf);
		uLog(SW_LOG_ERROR, "[%s][%d] recv msg error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	locbuf.len = ret;
	uLog(SW_LOG_DEBUG, "[%s][%d] recv msg:[%s] len=[%d]", __FILE__, __LINE__, locbuf.data, locbuf.len);
	
	if (enclib[0] != '\0' && encfun[0] != '\0')
	{
		ret = run_enc_fun(vars, &locbuf, enclib, encfun, SW_DEC);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] encrypt error!", __FILE__, __LINE__);
			close(sockid);
			pub_buf_clear(&locbuf);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] After dec, msg:[%s] len=[%d]", __FILE__, __LINE__, locbuf.data, locbuf.len);
	}
	
	xmlcfg->current = xmlcfg->root;
	ret = pkg_in(vars, &locbuf, xmlcfg, pkg_cache, prdt);
	if (ret < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] pack in error! ret=[%d]", __FILE__, __LINE__, ret);
		close(sockid);
		pub_buf_clear(&locbuf);
		return -1;
	}

	pub_buf_clear(&locbuf);
	
	if (filefun[0] != '\0')
	{
		ret = run_file_fun(filefun, vars, O_RECV, sockid, NULL);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] send file error!", __FILE__, __LINE__);
			close(sockid);
			pub_buf_clear(&locbuf);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] send file success!", __FILE__, __LINE__);
	}
	close(sockid);

	if (mapcfg[0] != '\0')
	{
		xml->current = xml->root;
		ret = map(vars, xml, O_RECV);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] map error!", __FILE__, __LINE__);
			return -1;
		}
		uLog(SW_LOG_DEBUG, "[%s][%d] map success!", __FILE__, __LINE__);
	}
	pub_log_debug("[%s][%d] tcpcall success!", __FILE__, __LINE__);
	
	return 0;
}
