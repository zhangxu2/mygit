#include "lsn_pub.h"
#include "http.h"
#include "pub_http.h"
#include <iconv.h>
#include "anet.h"

#if defined(SOLARIS)
#include <sys/filio.h>
#endif

static int	g_http_port = 0;
static char	g_http_host[128];
static char	g_http_path[512];

static int http_set_endpoint(const char *endpoint, char *host, int *port, char *path);
int url_encoded(const char *s, int s_len, char *out, int *len);

sw_int_t http_getsys_date(time_t *time_now, sw_char_t *date);

static int str_to_lower(char *str)
{
	char    *ptr = str;

	while (*ptr != '\0')
	{
		if (*ptr >= 'A' && *ptr <= 'Z')
		{
			*ptr |= 32;
		}
		ptr++;
	}

	return 0;
}

static int str_to_upper(char *str)
{
	char    *ptr = str;

	while (*ptr != '\0')
	{
		if (*ptr >= 'a' && *ptr <= 'z')
		{
			*ptr &= ~32;
		}
		ptr++;
	}                                                                                                                          

	return 0;
}

sw_int_t http_res_err(sw_lsn_cycle_t *cycle, int index, int state)
{
	long	now = 0;
	char	buf[128];
	sw_fd_t	sockid;
	sw_char_t	date[64];
	sw_buf_t	locbuf;

	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	memset(&locbuf, 0x0, sizeof(locbuf));
	pub_buf_init(&locbuf);

	memset(date, 0x0, sizeof(date));
	now = (long)time(NULL);
	http_getsys_date(&now, date);
	strcat(locbuf.data, "HTTP/1.1 400 Bad Request\r\n");
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "Date: %s\r\n", date);
	strcat(locbuf.data, buf);
	strcat(locbuf.data, "Connection: close\r\n\r\n");
	locbuf.len = strlen(locbuf.data);
	pub_log_info("[%s][%d] Response:[%s][%d]", __FILE__, __LINE__, locbuf.data, locbuf.len);

#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		if (SSL_write(sslcount[index].ssl, locbuf.data, locbuf.len) < 0)
		{
			pub_log_error("[%s][%d] SSL_write error!", __FILE__, __LINE__);
			pub_buf_clear(&locbuf);
			return SW_ERROR;
		}
		pub_buf_clear(&locbuf);
		pub_log_info("[%s][%d] Send response success!", __FILE__, __LINE__);
		return 0;
	}
#endif
	sockid = cycle->link_info[index].sockid;
	if (lsn_pub_send(sockid, locbuf.data, locbuf.len) < 0)
	{
		pub_log_error("[%s][%d] Send response error!", __FILE__, __LINE__);
		pub_buf_clear(&locbuf);
		return SW_ERROR;
	}
	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] Send response success!", __FILE__, __LINE__);

	return 0;
}

sw_int_t http_res_head(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, char *head, sw_int_t *headlen, sw_int_t content_len)
{
	int	i = 0;	
	int	num = 0;
	long	now = 0;
	char	date[64];
	char	tmp[128];
	char	prop_name[128];
	char	prop_value[256];
	char	http_method[8];
	char	http_action[512];
	sw_buf_t	*head_buffer = NULL;

	memset(http_method, 0x0, sizeof(http_method));
	memset(http_action, 0x0, sizeof(http_action));
	loc_get_zd_data(locvar, "#http_method", http_method);
	if (http_method[0] == '\0')
	{
		strncpy(http_method, "POST", sizeof(http_method) - 1);
	}

	loc_get_zd_data(locvar, "#http_action", http_action);
	if (http_action[0] == '\0')
	{
		strncpy(http_action, g_http_path, sizeof(http_action) - 1);
	}

	head_buffer = buf_new();
	if (head_buffer == NULL)
	{
		pub_log_error("[%s][%d] Alloc buffer error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	buf_format_append(head_buffer, "HTTP/1.1 200 OK%s", CRLF);
	memset(date, 0x00, sizeof(date));
	now = time(NULL);
	http_getsys_date(&now, date);
	buf_format_append(head_buffer, "Date: %s%s", date, CRLF);
	buf_format_append(head_buffer, "Connection: keep-alive%s", CRLF);
	buf_format_append(head_buffer, "Accept: */*%s", CRLF);
	buf_format_append(head_buffer, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.71%s", CRLF);
	buf_format_append(head_buffer, "Accept-Encoding: deflate, sdch%s", CRLF);
	buf_format_append(head_buffer, "Accept-Language: zh-CN,zh;q=0.8%s", CRLF);

	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(locvar, "#http_propertys", tmp);
	num = atoi(tmp);
	for (i = 0; i < num; i++)
	{
		memset(tmp, 0x0, sizeof(tmp));
		memset(prop_name, 0x0, sizeof(prop_name));
		memset(prop_value, 0x0, sizeof(prop_value));
		sprintf(tmp, "#http_property(%d)", i);
		loc_get_zd_data(locvar, tmp, prop_name);
		loc_get_zd_data(locvar, prop_name, prop_value);
		pub_log_info("[%s][%d] property[%d] [%s]=[%s]", __FILE__, __LINE__, i, prop_name, prop_value);
		if (prop_name[0] != '\0')
		{
			buf_format_append(head_buffer, "%s: %s%s", prop_name + 1, prop_value, CRLF);
		}
	}
	
	if (content_len > 0)
	{
		/*modify by songxc 20170923 增加Content-Type标签*/
		buf_format_append(head_buffer, "Content-Type: %s%s", "application/xml;charset=utf-8", CRLF);
		buf_format_append(head_buffer, "Content-Length: %d%s", content_len, CRLF);
	}
	buf_format_append(head_buffer, "%s", CRLF);
	strcpy(head, head_buffer->data);
	*headlen = head_buffer->len;
	buf_release(head_buffer);

	return 0;
}

sw_int_t http_req_head(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, char *head, sw_int_t *headlen, sw_int_t content_len)
{
	int	ret = 0;
	int	i = 0;	
	int	num = 0;
	char	tmp[128];
	char	prop_name[128];
	char	prop_value[256];
	char	http_method[8];
	char	http_action[512];
	char	http_params[2048*8];
	char	endpoint[128];
	sw_buf_t	*head_buffer = NULL;

	g_http_port = 0;
	memset(g_http_host, 0x0, sizeof(g_http_host));
	memset(g_http_path, 0x0, sizeof(g_http_path));
	memset(endpoint, 0x0, sizeof(endpoint));

	if (cycle->lsn_conf.comm.group[0].remote.ip[0] == '#' || cycle->lsn_conf.comm.group[0].remote.ip[0] == '$')
	{
		memset(tmp, 0x0, sizeof(tmp));
		loc_get_zd_data(locvar, cycle->lsn_conf.comm.group[0].remote.ip, tmp);
		strncpy(g_http_host, tmp, sizeof(g_http_host) - 1);
	}
	else
	{
		strncpy(g_http_host, cycle->lsn_conf.comm.group[0].remote.ip, sizeof(g_http_host) - 1);
	}

	g_http_port = cycle->lsn_conf.comm.group[0].remote.port;
	if (strncmp(g_http_host, "http", 4) == 0)
	{
		strncpy(endpoint, g_http_host, sizeof(endpoint) - 1);
		memset(g_http_host, 0x0, sizeof(g_http_host));
		g_http_port = 0;
		ret = http_set_endpoint(endpoint, g_http_host, &g_http_port, g_http_path);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] http set endpoint error! endpoint=[%s]",
					__FILE__, __LINE__, endpoint);
			return SW_ERROR;
		}
	}
	else
	{
		g_http_path[0] = '/';
		g_http_path[1] = '\0';
	}

	memset(http_method, 0x0, sizeof(http_method));
	memset(http_action, 0x0, sizeof(http_action));
	loc_get_zd_data(locvar, "#http_method", http_method);
	if (http_method[0] == '\0')
	{
		strncpy(http_method, "POST", sizeof(http_method) - 1);
	}

	loc_get_zd_data(locvar, "#http_action", http_action);
	if (http_action[0] == '\0')
	{
		strncpy(http_action, g_http_path, sizeof(http_action) - 1);
	}

	head_buffer = buf_new();
	if (head_buffer == NULL)
	{
		pub_log_error("[%s][%d] Alloc buffer error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(http_params, 0x0, sizeof(http_params));
	loc_get_zd_data(locvar, "#http_params", http_params);
	if (strcmp(http_method, "GET") == 0)
	{
		if (http_params[0] != '\0')
		{
			buf_format_append(head_buffer, "GET %s?%s HTTP/1.1%s", http_action, http_params, CRLF);
		}
		else
		{
			buf_format_append(head_buffer, "GET %s HTTP/1.1%s", http_action, CRLF);
		}
	}
	else
	{
		if (http_params[0] != '\0')
		{
			buf_format_append(head_buffer, "POST %s?%s HTTP/1.1%s", http_action, http_params, CRLF);
		}
		else
		{
			buf_format_append(head_buffer, "POST %s HTTP/1.1%s", http_action, CRLF);
		}
	}
	buf_format_append(head_buffer, "Host: %s%s", g_http_host, CRLF);
	buf_format_append(head_buffer, "Connection: keep-alive%s", CRLF);
	buf_format_append(head_buffer, "Accept: */*%s", CRLF);
	buf_format_append(head_buffer, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.71%s", CRLF);
	buf_format_append(head_buffer, "Accept-Encoding: deflate, sdch%s", CRLF);
	buf_format_append(head_buffer, "Accept-Language: zh-CN,zh;q=0.8%s", CRLF);

	memset(tmp, 0x0, sizeof(tmp));
	loc_get_zd_data(locvar, "#http_propertys", tmp);
	num = atoi(tmp);
	for (i = 0; i < num; i++)
	{
		memset(tmp, 0x0, sizeof(tmp));
		memset(prop_name, 0x0, sizeof(prop_name));
		memset(prop_value, 0x0, sizeof(prop_value));
		sprintf(tmp, "#http_property(%d)", i);
		loc_get_zd_data(locvar, tmp, prop_name);
		loc_get_zd_data(locvar, prop_name, prop_value);
		pub_log_info("[%s][%d] property[%d] [%s]=[%s]", __FILE__, __LINE__, i, prop_name, prop_value);
		if (prop_name[0] != '\0')
		{
			buf_format_append(head_buffer, "%s: %s%s", prop_name + 1, prop_value, CRLF);
		}
	}
	
	if (content_len > 0)
	{
		buf_format_append(head_buffer, "Content-Length: %d%s", content_len, CRLF);
	}
	buf_format_append(head_buffer, "%s", CRLF);
	strcpy(head, head_buffer->data);
	*headlen = head_buffer->len;
	buf_release(head_buffer);

	return 0;
}

sw_int_t http_getsys_date(time_t *time_now, sw_char_t *date)
{
	int	len = 0;
	sw_char_t *ptr_p;
	struct tm *time;

	if (time_now == NULL || date == NULL)
	{
		pub_log_error("[%s][%d]func http_getsys_date input error! ",__FILE__,__LINE__);
		return -1;
	}

	time = gmtime(time_now);
	if (time == NULL)
	{
		pub_log_error("[%s][%d] gmtime error [%d]",__FILE__,__LINE__,errno);
		return -1;
	}

	ptr_p = asctime(time);
	if (ptr_p != NULL)
	{
		len = strlen(ptr_p);
		if (ptr_p[len - 1] == '\n' || ptr_p[len - 1] == '\r')
		{
			ptr_p[len - 1] = '\0';
		}
		pub_log_info("[%s][%d] ptr_p=[%s][%d]", __FILE__, __LINE__, ptr_p, strlen(ptr_p));
		strcpy(date, ptr_p);
		return 0;
	}

	return -1;
}

int http_unpack_head(sw_loc_vars_t *vars, sw_buf_t *pkgbuf)
{
	int	i = 0;
	int	lines = 0;
	int	chunked = 0;
	int	headlen = 0;
	int	spaces = 0;
	int	xmlheadlen = 0;
	char	*p = NULL;
	char	*q = NULL;
	char	*ptr = NULL;
	char	uri[512];
	char	method[64];
	char	httpver[64];
	char	name[256];
	char	value[1024];
	char	xmlhead[128];
	sw_buf_t	locbuf;

	memset(uri, 0x0, sizeof(uri));
	memset(method, 0x0, sizeof(method));
	memset(httpver, 0x0, sizeof(httpver));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));	
	memset(&locbuf, 0x0, sizeof(locbuf));
	pub_buf_init(&locbuf);
	if (pub_buf_chksize(&locbuf, pkgbuf->len) != SW_OK)
	{
		pub_log_error("[%s][%d] update buf error! size=[%d]",
				__FILE__, __LINE__, pkgbuf->len);
		pub_buf_clear(&locbuf);
		return -1;
	}
	memcpy(locbuf.data, pkgbuf->data, pkgbuf->len);
	locbuf.len = pkgbuf->len;
	ptr = strstr(locbuf.data, "\r\n\r\n");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] http package format error!", __FILE__, __LINE__);
		pub_log_bin(SW_LOG_ERROR, locbuf.data, locbuf.len, "[%s][%d] Http package format error! len=[%d]",
				__FILE__, __LINE__, locbuf.len);
		pub_buf_clear(&locbuf);
		return -1;
	} 
	headlen = ptr - locbuf.data + 4;
	pub_log_bin(SW_LOG_INFO, locbuf.data, headlen, "[%s][%d] Http head:[%d]",
			__FILE__, __LINE__, headlen);
	lines = 0;
	ptr = locbuf.data;
	while (*ptr != '\0')
	{
		if (strncmp(ptr, "\r\n", 2) == 0)
		{
			break;
		}

		p = strstr(ptr, "\r\n");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] http package format error!", __FILE__, __LINE__);
			pub_log_bin(SW_LOG_ERROR, locbuf.data, locbuf.len, "[%s][%d] Http package format error! len=[%d]",
					__FILE__, __LINE__, locbuf.len);
			pub_buf_clear(&locbuf);
			return -1;
		}
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		memcpy(value, ptr, p - ptr);
		if (lines == 0)
		{
			pub_log_info("[%s][%d] First line=[%s]", __FILE__, __LINE__, value);
			q = method;
			i = 0;
			while (value[i] != '\0')
			{
				if (value[i] == ' ')
				{
					*q = '\0';
					spaces++;
					if (spaces == 1)
					{
						q = uri;
					}
					else if (spaces == 2)
					{
						q = httpver;
					}
					else
					{
						pub_log_error("[%s][%d] Too many space!", __FILE__, __LINE__);
						break;
					}
				} 
				else
				{
					*q++ = value[i];
				}
				i++;
			}
			loc_set_zd_data(vars, "#http-method", method);
			pub_log_info("[%s][%d] 拆出变量[#http-method]=[%s]", __FILE__, __LINE__, method);
			loc_set_zd_data(vars, "#http-uri", uri);
			pub_log_info("[%s][%d] 拆出变量[#http-uri]=[%s]", __FILE__, __LINE__, uri);
			loc_set_zd_data(vars, "#http-version", httpver);
			pub_log_info("[%s][%d] 拆出变量[#http-version]=[%s]", __FILE__, __LINE__, httpver);

			lines++;
			ptr = p + 2;
			continue;
		}

		q = strstr(value, ": ");
		if (q != NULL)
		{
			memset(name, 0x0, sizeof(name));
			strcpy(name, "#http-");
			memcpy(name + 6, value, q - value);
			str_to_lower(name);
			loc_set_zd_data(vars, name, q + 2);
			if (strcmp(name, "#http-transfer-encoding") == 0 && strcmp(q + 2, "chunked") == 0)
			{
				chunked = 1;
			}
			pub_log_info("[%s][%d] 拆出变量[%s]=[%s]", __FILE__, __LINE__, name, q + 2);
		}
		lines++;
		ptr = p + 2;
	}
	
	xmlheadlen = 0;
	memset(pkgbuf->data, 0x0, pkgbuf->size);
	if (locbuf.data[headlen] == '<' && strncmp(locbuf.data + headlen, "<?xml", 5) != 0)
	{
		memset(xmlhead, 0x0, sizeof(xmlhead));
		sprintf(xmlhead, "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n");
		xmlheadlen = strlen(xmlhead);
		memcpy(pkgbuf->data, xmlhead, xmlheadlen);
	}

	if (chunked)
	{
		int	first = 0;
		int	count = 0;
		int	buflen = 0;
		int	tmplen = 0;
		char	*endptr = NULL;
		char	tmp[32];
		
		count = headlen;
		buflen = xmlheadlen;
		ptr = strstr(locbuf.data + count, "\r\n");
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] Format error!", __FILE__, __LINE__);
			memcpy(pkgbuf->data + xmlheadlen, locbuf.data + headlen, locbuf.len - headlen);
			pkgbuf->len = locbuf.len - headlen + xmlheadlen;
		}
		
		memset(tmp, 0x0, sizeof(tmp));
		tmplen = ptr - locbuf.data - count;
		memcpy(tmp, locbuf.data + count, tmplen);
		pkgbuf->len = strtol(tmp, &endptr, 16);
		pub_log_info("[%s][%d] chunked len:[%d]", __FILE__, __LINE__, pkgbuf->len);
		memcpy(pkgbuf->data + buflen, ptr + 2, pkgbuf->len);
		buflen += pkgbuf->len;
		count += pkgbuf->len + 4 + tmplen;
		
		while(1)
		{
			ptr = strstr(locbuf.data + count, "\r\n");
			if (ptr != NULL && memcmp(locbuf.data + count - 2, "\r\n", 2) == 0)
			{
				memset(tmp, 0x0, sizeof(tmp));
				tmplen = ptr - locbuf.data - count;
				memcpy(tmp, locbuf.data + count, tmplen);
				pkgbuf->len = strtol(tmp, &endptr, 16);
				if (pkgbuf->len  == 0)
				{
					pub_log_info("[%s][%d] deal pkg end", __FILE__, __LINE__);
					break;
				}
				pub_log_info("[%s][%d] chunked len:[%d]", __FILE__, __LINE__, pkgbuf->len);
				memcpy(pkgbuf->data + buflen, ptr + 2, pkgbuf->len);
				count += pkgbuf->len + 4 + tmplen;
				buflen += pkgbuf->len;
			}
		}
		pkgbuf->len = buflen;
		pkgbuf->data[pkgbuf->len] = '\0';
	}
	else
	{
		memcpy(pkgbuf->data + xmlheadlen, locbuf.data + headlen, locbuf.len - headlen);
		pkgbuf->len = locbuf.len - headlen + xmlheadlen;
	}
	pub_log_bin(SW_LOG_INFO, pkgbuf->data, pkgbuf->len, "[%s][%d] Http body:[%d]",
			__FILE__, __LINE__, pkgbuf->len);

	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] http unpack head success!", __FILE__, __LINE__);

	return 0;
}

sw_int_t http_recv_ssl(sw_loc_vars_t *locvar, sw_buf_t *locbuf, int index)
{
#if defined(SW_USE_OPENSSL)
	if (!g_is_ssl)
	{
		return 0;
	}
	int	width = 0;
	int	toread = 0;
	SSL	*ssl;
	fd_set	readfds;
	sw_int_t	ret = 0;
	sw_int_t	sslerr = 0;
	sw_int_t	length = 0;
	struct timeval	timeout;

	if (locvar == NULL || locbuf == NULL || index < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	ssl = sslcount[index].ssl;
	width = SSL_get_fd(ssl) + 1;
	while (1)
	{
		toread = 0;
		ret = ioctl(SSL_get_fd(ssl), FIONREAD, &toread);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ioctl error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		if (toread == 0)
		{
			break;
		}
again:
		if (pub_buf_chksize(locbuf, toread) < 0)
		{
			pub_log_error( "[%s][%d] pub_buf_chksize error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		ret = SSL_read(ssl, locbuf->data + length, toread);
		if (ret == 0)
		{
			pub_log_info("[%s][%d] SSL_read 0!", __FILE__, __LINE__);
			break;
		}
		sslerr = SSL_get_error(ssl, ret);
		if (sslerr == SSL_ERROR_NONE)
		{
			length += ret;
			locbuf->len = length;
			pub_log_info("[%s][%d] toread=[%d] ret=[%d] pending=[%d]", 
					__FILE__, __LINE__, toread, ret, SSL_pending(ssl));
			if (SSL_pending(ssl))
			{
				pub_log_info("[%s][%d] SSl_read again! SSL_pending=[%d]",
						__FILE__, __LINE__, SSL_pending(ssl));
				goto again;
			}

			FD_ZERO(&readfds);
			FD_SET(SSL_get_fd(ssl), &readfds);
			memset(&timeout, 0x0, sizeof(timeout));
			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			ret = select(width, &readfds, NULL, NULL, &timeout);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] select error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}
			else if (ret > 0)
			{
				pub_log_info("[%s][%d] SSL_read again!", __FILE__, __LINE__);
				goto again;
			}
		}
		else if (sslerr == SSL_ERROR_WANT_WRITE || sslerr == SSL_ERROR_WANT_READ)
		{
			pub_log_info("[%s][%d] Read BLOCK!", __FILE__, __LINE__);
		}
		else if (sslerr == SSL_ERROR_SYSCALL || sslerr == SSL_ERROR_SSL)
		{
			pub_log_error("[%s][%d] SSL_read error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		else if (sslerr == SSL_ERROR_ZERO_RETURN)
		{
			pub_log_error("[%s][%d] SSL_read done!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		else
		{
			pub_log_error("[%s][%d] sslerr=[%d]", __FILE__, __LINE__, sslerr);
			return SW_ERROR;
		}
	}
	if (length <= 0)
	{
		pub_log_error("[%s][%d] SSL no data to read!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] SSL read msg:[%s][%d]", __FILE__, __LINE__, locbuf->data, locbuf->len);

	ret = http_unpack_head(locvar, locbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Unpack head error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Package body:[%s][%d]", __FILE__, __LINE__, locbuf->data, locbuf->len);

	return locbuf->len;
#endif
	return 0;
}

sw_int_t http_recv(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf, int index)
{
	int	toread = 0;
	int	width = 0;
	sw_fd_t	sockid;
	fd_set	readfds;
	sw_int_t	ret = 0;
	sw_int_t	length = 0;
	struct timeval	timeout;

#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		return http_recv_ssl(locvar, locbuf, index);
	}
#endif
	memset(&timeout, 0x0, sizeof(timeout));
	sw_int32_t  scantime = cycle->lsn_conf.scantime;
	if (scantime == 0)
	{
		timeout.tv_sec  = 1;
		timeout.tv_usec = 0;
	}
	else 
	{
		timeout.tv_sec  = 0;
		timeout.tv_usec = scantime * 1000;
	}

	sockid = cycle->link_info[index].sockid;
	width = sockid + 1;
	while (1)
	{
		FD_ZERO(&readfds);                                                                                 
		FD_SET(sockid, &readfds);                                                                 
		ret = select(width, &readfds, NULL, NULL, &timeout);                                               
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			pub_log_error("[%s][%d] select error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		else if (ret == 0)
		{
			pub_log_info("[%s][%d] Recv data over! size=[%d]",
					__FILE__, __LINE__, length);
			break;
		}

		toread = 0;
		ret = ioctl(sockid, FIONREAD, &toread);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ioctl error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] toread=[%d]", __FILE__, __LINE__, toread);
		if (toread == 0)
		{
			break;
		}

		if (pub_buf_chksize(locbuf, toread) != SW_OK)
		{
			pub_log_error("[%s][%d] Update buffer error! size=[%d]", 
					__FILE__, __LINE__, length + toread);
			return SW_ERROR;
		}

		ret = lsn_pub_recv_len(sockid, locbuf->data + length, toread);
		if (ret != toread)
		{
			pub_log_error("[%s][%d] lsn_pub_recv error! toread=[%d] ret=[%d]",
					__FILE__, __LINE__, toread, ret);
			pub_log_bin(SW_LOG_INFO, locbuf->data, length, "[%s][%d] Recv data:[%d]",
					__FILE__, __LINE__, length);
			return SW_ERROR;
		}
		pub_log_debug("[%s][%d] recv:[%s][%d]", __FILE__, __LINE__, locbuf->data + length, toread);
		length += toread;
		locbuf->len = length;
	}
	if (length <= 0)
	{
		pub_log_error("[%s][%d] No data to read!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_bin(SW_LOG_INFO, locbuf->data, locbuf->len, "[%s][%d] Recv data:[%d]", __FILE__, __LINE__, locbuf->len);

	ret = http_unpack_head(locvar, locbuf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Unpack head error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Package body:[%s][%d]", __FILE__, __LINE__, locbuf->data, locbuf->len);

	return locbuf->len;
}

int http_add_head(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf, int flag)
{
	int	ret = 0;
	int	xmlheadlen = 0;
	int	bodylen = locbuf->len;
	char	*p = NULL;
	char	*s = NULL;
	char	head[2048*8];
	sw_int_t	headlen = 0;
	sw_buf_t	tmpbuf;

	char xml_head[128];
	memset(xml_head, 0x00, sizeof(xml_head));
	loc_get_zd_data(locvar, "$sw_xml_headflag", xml_head);

	p = strstr(locbuf->data, "<?xml");
	if (p != NULL && xml_head[0] == '\0')
	{
		s = strstr(p, "?>");
		if (s != NULL)
		{
			sw_buf_t	*tb = NULL;

			xmlheadlen = s - locbuf->data + 2;
			tb = buf_new_string(locbuf->data + xmlheadlen, locbuf->len - xmlheadlen);
			if (tb == NULL)
			{
				pub_log_error("[%s][%d] Alloc buffer error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			buf_refresh(locbuf);
			if (buf_update_string(locbuf, tb->data, tb->len) < 0)
			{
				pub_log_error("[%s][%d] Update buffer error!", __FILE__, __LINE__);
				buf_release(tb);
				return SW_ERROR;
			}
			buf_release(tb);
			bodylen = locbuf->len;
		}
		pub_log_info("[%s][%d] data:[%s][%d][%d]",
			__FILE__, __LINE__, locbuf->data, locbuf->len, strlen(locbuf->data));
	}
	
	memset(head, 0x0, sizeof(head));
	if (flag == 1)
	{
		ret = http_res_head(cycle, locvar, head, &headlen, bodylen);		
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Pack res head error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		ret = http_req_head(cycle, locvar, head, &headlen, bodylen);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Pack req head error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	pub_buf_init(&tmpbuf);
	if (pub_buf_chksize(&tmpbuf, bodylen + headlen) != SW_OK)
	{
		pub_log_error("[%s][%d] Update buf error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	memcpy(tmpbuf.data, head, headlen);
	tmpbuf.len = headlen + bodylen;
	memcpy(tmpbuf.data + headlen, locbuf->data, bodylen);

	memset(locbuf->data, 0x0, locbuf->size);
	locbuf->len = 0;
	if (pub_buf_chksize(locbuf, bodylen + headlen) != SW_OK)
	{
		pub_log_error("[%s][%d] Update buf error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	memcpy(locbuf->data, tmpbuf.data, tmpbuf.len);
	locbuf->len = tmpbuf.len;
	pub_buf_clear(&tmpbuf);

	pub_log_bin(SW_LOG_INFO, locbuf->data, locbuf->len, "[%s][%d] After add head:[%d]",
			__FILE__, __LINE__, locbuf->len);

	return 0;
}

sw_int_t http_find_free(sw_lsn_cycle_t *cycle)
{
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		sw_int_t	i = 0;
		sw_int_t	cnt = 0;

		cnt = cycle->lsn_conf.conmax;
		for (i = 0; i < cnt; i++)
		{
			if (sslcount[i].used == 0)
			{
				break;
			}
		}
		if (i == cnt)
		{
			pub_log_error("[%s][%d] There is no free ssl!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		return i;
	}
#endif
	return -1;
}

sw_int_t http_find_ssl_by_fd(sw_lsn_cycle_t *cycle, sw_int_t sockid)
{
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		sw_int_t	i = 0;
		sw_int_t	cnt = 0;

		cnt = cycle->lsn_conf.conmax;
		for (i = 0; i < cnt; i++)
		{
			if(sslcount[i].sock == sockid)
			{
				break;
			}
		}
		if (i == cnt)
		{
			pub_log_error("[%s][%d] Can not find [%s]'s ssl info!", __FILE__, __LINE__, sockid);
			return SW_ERROR;
		}
		return i;
	}
#endif
	return -1;
}

sw_int_t http_clear_ssl(sw_int_t index)
{
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		if (sslcount[index].ssl != NULL)
		{
			SSL_shutdown(sslcount[index].ssl);
			SSL_free(sslcount[index].ssl);
			sslcount[index].ssl = NULL;
		}
		sslcount[index].used = 0;
		sslcount[index].sock = 0;
	}
#endif

	return SW_OK;
}

sw_int_t http_close_fd(sw_lsn_cycle_t *cycle, int index)
{
	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	select_del_event(cycle->lsn_fds, cycle->link_info[index].sockid);
	lsn_pub_close_fd(cycle, index);
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		if (sslcount[index].ssl != NULL)
		{
			SSL_free(sslcount[index].ssl);
			sslcount[index].ssl = NULL;
		}
	}
#endif

	return 0;
}

sw_int_t httpss_init_ssl(sw_lsn_cycle_t *cycle)
{
#if defined(SW_USE_OPENSSL)	
	if (g_is_ssl)
	{
		sw_int_t	ret = 0;
		sw_int_t	wait_cnt = 0;
		sw_char_t	server_file[512];
		sw_char_t	server_key[512];
		sw_char_t	server_crt[512];

		memset(server_file, 0x00, sizeof(server_file));
		memset(server_key, 0x00, sizeof(server_key));
		memset(server_crt, 0x00, sizeof(server_crt));

		memset(&sslcycle, 0x00, sizeof(sslcycle));
		sprintf(server_crt, "%s/cert/%s/server.crt", getenv("SWWORK"), cycle->lsn_conf.name);
		sprintf(server_key, "%s/cert/%s/server.key", getenv("SWWORK"), cycle->lsn_conf.name);
		sslcycle.cafile = server_crt;
		sslcycle.keyfile = server_key;

		SSL_load_error_strings();
		SSLeay_add_ssl_algorithms();
		sslcycle.ctx = SSL_CTX_new(SSLv23_server_method());
		if (sslcycle.ctx == NULL)
		{
			pub_log_error("[%s][%d]SSL_CTX_new falied!", __FILE__, __LINE__);  
			return SW_ERROR;
		}

		ret = SSL_CTX_use_certificate_file(sslcycle.ctx, sslcycle.cafile, SSL_FILETYPE_PEM);
		if (ret <= 0)
		{
			pub_log_error("[%s][%d]certificate erro, crt[%s]", __FILE__, __LINE__, sslcycle.cafile);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}

		ret = SSL_CTX_use_PrivateKey_file(sslcycle.ctx, sslcycle.keyfile, SSL_FILETYPE_PEM);
		if (ret <=0 )
		{
			pub_log_error("[%s][%d]PrivateKey erro, key[%s]", __FILE__, __LINE__, sslcycle.keyfile);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}

		ret = SSL_CTX_check_private_key(sslcycle.ctx);
		if (ret <= 0)
		{
			pub_log_error("[%s][%d]check_private erro", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}

		wait_cnt = cycle->lsn_conf.conmax;

		sslcount = NULL;
		sslcount = (SSLCount *)malloc(sizeof(SSLCount) * wait_cnt);
		if (sslcount == NULL)
		{
			pub_log_error("[%s][%d] malloc SSLCount failed! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		memset(sslcount, 0x00, sizeof(SSLCount) * wait_cnt);
	}
#endif
	return 0;
}

sw_int_t http_destroy_ssl(sw_lsn_cycle_t *cycle)
{
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		SSL_CTX_free(sslcycle.ctx);
		free(sslcount);
	}
#endif
	return 0;
}

sw_int_t http_send(sw_lsn_cycle_t *cycle, sw_char_t *buf, sw_int_t length, int index)
{
	sw_fd_t	sockid;
	sw_int_t	ret = 0;

	if (cycle == NULL || buf == NULL || length <= 0 || index < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		ret = SSL_write(sslcount[index].ssl, buf, length);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] SSL_write error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] SSL_write success!", __FILE__, __LINE__);
		pub_log_info("[%s][%d] SSL_write:[%s][%d][%d]", __FILE__, __LINE__, buf, length, ret);
		return SW_OK;
	}
#endif
	sockid = cycle->link_info[index].sockid;
	ret = lsn_pub_send(sockid, buf, length);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] lsn_pub_send error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Send message success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t http_accept_ssl(sw_lsn_cycle_t *cycle, sw_fd_t acceptfd, int index)
{
#if defined(SW_USE_OPENSSL)
	int	times = 0;

	if (g_is_ssl)
	{
		sw_int_t	ret = 0;
		sw_char_t	*ptr = NULL;
		X509	*client_cert;

		sslcount[index].ssl = SSL_new(sslcycle.ctx);
		if (sslcount[index].ssl == NULL)
		{
			pub_log_error("[%s][%d] SSL_new error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		SSL_set_fd(sslcount[index].ssl, acceptfd);
		errno = 0;
		while (1)
		{
			ret = SSL_accept(sslcount[index].ssl);
			if (ret == -1)
			{
				if (errno == EINTR)
				{
					continue;
				}
				if (errno == EAGAIN)
				{
					times++;
					if (times > 3)
					{
						pub_log_error("[%s][%d] ret = [%d] SSL_accept error!", __FILE__, __LINE__,ret);
						http_clear_ssl(index);
						ERR_print_errors_fp(stderr);
						return SW_ERROR;
					}
					usleep(100000);
					continue;
				}
				pub_log_error("[%s][%d] ret = [%d] SSL_accept error!", __FILE__, __LINE__,ret);
				http_clear_ssl(index);
				ERR_print_errors_fp(stderr);
				return SW_ERROR;
			}
			break;
		}
		pub_log_debug("[%s][%d] accept sucess, ssl[%X]", 
				__FILE__, __LINE__, sslcount[index].ssl);
		client_cert = SSL_get_peer_certificate(sslcount[index].ssl);
		if (client_cert != NULL)
		{
			ptr = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] X509_NAME_oneline error!", __FILE__, __LINE__);
				http_clear_ssl(index);
				ERR_print_errors_fp(stderr);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] subject:[%s]", __FILE__, __LINE__, ptr);
			OPENSSL_free(ptr);
			ptr = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] X509_NAME_oneline error!", __FILE__, __LINE__);
				http_clear_ssl(index);
				ERR_print_errors_fp(stderr);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] Issuer:[%s]", __FILE__, __LINE__, ptr);
			OPENSSL_free(ptr);
			X509_free (client_cert);
		}
		else
		{
			pub_log_info("[%s][%d] Client dose not have certificate!", __FILE__, __LINE__);
		}
		pub_log_info("[%s][%d] SSL accept success!", __FILE__, __LINE__);
	}
#endif
	return 0;
}

sw_int_t httpsc_init_ssl(sw_lsn_cycle_t *cycle)
{
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		sw_int_t	wait_cnt = 0;
		sw_char_t	cacert[512];
		sw_char_t	clikey[512];
		sw_char_t	clicrt[512];

		memset(&sslcycle, 0x00, sizeof(sslcycle));

		SSL_library_init();
		SSL_load_error_strings();
		sslcycle.ctx = SSL_CTX_new((SSL_METHOD *)TLSv1_client_method());
		if (sslcycle.ctx == NULL)
		{
			pub_log_error("[%s][%d] SSL_CTX_new error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		
		memset(cacert, 0x00, sizeof(cacert));
		snprintf(cacert, sizeof(cacert) - 1, "%s/cert/%s/ca.crt", getenv("SWWORK"), cycle->lsn_conf.name);
		
		memset(clikey, 0x00, sizeof(clikey));
		snprintf(clikey, sizeof(clikey) - 1, "%s/cert/%s/client.key", getenv("SWWORK"), cycle->lsn_conf.name);
			
		memset(clicrt, 0x00, sizeof(clicrt));
		snprintf(clicrt, sizeof(clicrt) - 1, "%s/cert/%s/client.crt", getenv("SWWORK"), cycle->lsn_conf.name);
		
		if (pub_file_exist(cacert) && pub_file_exist(clikey) && pub_file_exist(clicrt))
		{
			/* 制定证书验证方式,客户端验证服务端证书，SSL_VERIFY_NONE 暂时设为不验证，用于openssl0.9.8测试*/
			SSL_CTX_set_verify(sslcycle.ctx, SSL_VERIFY_PEER, NULL);
	
			/* 为SSL会话加载CA证书*/
			SSL_CTX_load_verify_locations(sslcycle.ctx, cacert, NULL);
			
			/* 为SSL会话加载用户证书 */
			if (0 == SSL_CTX_use_certificate_file(sslcycle.ctx, clicrt, SSL_FILETYPE_PEM))
			{ 
				ERR_print_errors_fp (stderr);
				pub_log_error("[%s][%d] load user cert[%s] error.", __FILE__, __LINE__, clicrt);
				return SW_ERROR;
			}
				
			/* 为SSL会话加载用户私钥 */
			if (0 == SSL_CTX_use_PrivateKey_file(sslcycle.ctx, clikey, SSL_FILETYPE_PEM))
			{
				ERR_print_errors_fp (stderr);
				pub_log_error("[%s][%d] load user key[%s] error.", __FILE__, __LINE__, clikey);
				return SW_ERROR;
			}
				
			/* 验证私钥和证书是否相符 */
			if (!SSL_CTX_check_private_key(sslcycle.ctx))
			{
				pub_log_error("[%s][%d] Private key does not match the certificate public key.", __FILE__, __LINE__);
				return SW_ERROR;
			}
				
			pub_log_info("[%s][%d] load client cert ok.", __FILE__, __LINE__);
		}

		wait_cnt = cycle->lsn_conf.conmax;
		sslcount = (SSLCount *)malloc(sizeof(SSLCount) * wait_cnt);
		if (sslcount == NULL)
		{
			pub_log_error("[%s][%d] Malloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
	}
#endif
	return SW_OK;
}

sw_int_t http_connect(sw_lsn_cycle_t *cycle)
{
	sw_fd_t	sockid;
	int	ret = 0;
	char	errmsg[128];

	memset(errmsg, 0x0, sizeof(errmsg));

	sockid = anet_tcp_connect(errmsg, g_http_host, g_http_port);
	if (sockid < 0)
	{
		pub_log_error("[%s][%d] connect [%s]:[%d] error! errmsg:[%s]",
				__FILE__, __LINE__, g_http_host, g_http_port, errmsg);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] connect [%s]:[%d] success! sockid=[%d]",
			__FILE__, __LINE__, g_http_host, g_http_port, sockid);
#if defined(SW_USE_OPENSSL)
	if (g_is_ssl)
	{
		char	*ptr = NULL;
		X509	*server_cert;

		sslcount[sockid].ssl = SSL_new(sslcycle.ctx);
		if (sslcount[sockid].ssl == NULL)
		{
			pub_log_error("[%s][%d] SSL_new error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		SSL_set_fd(sslcount[sockid].ssl, sockid);
		ret = SSL_connect(sslcount[sockid].ssl);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] SSL_connect error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] SSL_connect success!", __FILE__, __LINE__);
		pub_log_info("[%s][%d] SSL connection using [%s]!", __FILE__, __LINE__, SSL_get_cipher(sslcount[sockid].ssl));
		sslcount[sockid].sock = sockid;
		sslcount[sockid].used = 1;

		server_cert = SSL_get_peer_certificate(sslcount[sockid].ssl);
		if (server_cert == NULL)
		{
			pub_log_error("[%s][%d] SSL_get_peer_certificate error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}

		ptr = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);                                                           
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] X509_get_subject_name error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] subject:[%s]", __FILE__, __LINE__, ptr);
		OPENSSL_free(ptr);

		ptr = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] X509_get_issuer_name error!", __FILE__, __LINE__);
			ERR_print_errors_fp(stderr);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] issuer:[%s]", __FILE__, __LINE__, ptr);
		OPENSSL_free(ptr);
		X509_free(server_cert);
		return sockid;
	}
#endif
	return sockid;
}

int http_append_value(sw_buf_t *buffer, char *key, char *value, int len)
{
	if (buffer->len > 0)
	{
		buf_append(buffer, "&", 1);
	}
	buf_append(buffer, key, strlen(key));
	buf_append(buffer, "=", 1);
	buf_append(buffer, value, len);

	return 0;
}

int http_append_and_obj(sw_buf_t *buffer, sw_loc_vars_t *vars, char *vname, char *key)
{
	int     len = 0;
	int     large = 0;
	char    *pv = NULL;
	char    value[1024];

	large = 0;
	memset(value, 0x0, sizeof(value));
	pv = value;
	len = vars->get_field_len(vars, vname);
	if (len > 1024)
	{
		pv = (char *)calloc(1, len + 1);
		if (pv == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]\n",
					__FILE__, __LINE__, len, errno, strerror(errno));
			return -1;
		}
		large = 1;
	}
	else if (len == 0)
	{
		return 0;
	}

	loc_get_zd_data(vars, vname, pv);
	if (buffer->len > 0)
	{
		buf_append(buffer, "&", 1);
	}
	buf_append(buffer, key, strlen(key));
	buf_append(buffer, "=", 1);
	buf_append(buffer, pv, len);
	if (large)
	{
		free(pv);
	}

	return 0;
}

int http_append_and_encode(sw_buf_t *buffer, sw_loc_vars_t *vars, char *vname, char *key)
{
	int     len = 0;
	int     large = 0;
	int	enlen = 0;
	char    *pv = NULL;
	char    value[1024];
	char	encoded[1024*4];

	large = 0;
	memset(value, 0x0, sizeof(value));
	pv = value;
	len = vars->get_field_len(vars, vname);
	if (len > 1024)
	{
		pv = (char *)calloc(1, len + 1);
		if (pv == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]\n",
					__FILE__, __LINE__, len, errno, strerror(errno));
			return -1;
		}
		large = 1;
	}
	else if (len == 0)
	{
		return 0;
	}

	loc_get_zd_data(vars, vname, pv);
	if (buffer->len > 0)
	{
		buf_append(buffer, "&", 1);
	}
	buf_append(buffer, key, strlen(key));
	buf_append(buffer, "=", 1);
	memset(encoded, 0x0, sizeof(encoded));
	url_encoded(pv, len, encoded, &enlen);
	buf_append(buffer, encoded, enlen);
	if (large)
	{
		free(pv);
	}

	return 0;
}

int http_append_value_encode(sw_buf_t *buffer, char *key, char *value, int len)
{
	int	enlen = 0;
	char	encoded[1024*4];

	memset(encoded, 0x0, sizeof(encoded));
	url_encoded(value, len, encoded, &enlen);

	if (buffer->len > 0)
	{
		buf_append(buffer, "&", 1);
	}
	buf_append(buffer, key, strlen(key));
	buf_append(buffer, "=", 1);
	buf_append(buffer, encoded, enlen);

	return 0;
}

int http_tag_cmp(const char *s, const char *t)
{
	for (;;)
	{
		int     c1 = *s;
		int     c2 = *t;

		if (!c1 || c1 == '"')
		{
			break;
		}
		if (c2 != '-')
		{
			if (c1 != c2)
			{
				if (c1 >= 'A' && c1 <= 'Z')
				{
					c1 += 'a' - 'A';
				}
				if (c2 >= 'A' && c2 <= 'Z')
				{
					c2 += 'a' - 'A';
				}
			}
			if (c1 != c2)
			{
				if (c2 != '*')
				{
					return 1;
				}
				c2 = *++t;
				if (!c2)
				{
					return 0;
				}
				if (c2 >= 'A' && c2 <= 'Z')
				{
					c2 += 'a' - 'A';
				}
				for (;;)
				{
					c1 = *s;
					if (!c1 || c1 == '"')
					{
						break;
					}
					if (c1 >= 'A' && c1 <= 'Z')
					{
						c1 += 'a' - 'A';
					}
					if (c1 == c2 && !http_tag_cmp(s + 1, t + 1))
					{
						return 0;
					}
					s++;
				}
				break;
			}
		}
		s++;
		t++;
	}
	if (*t == '*' && !t[1])
	{
		return 0;
	}

	return *t;
}

static int http_set_endpoint(const char *endpoint, char *host, int *port, char *path)
{
	size_t  i = 0, n = 0;
	const char      *s = NULL;

	host[0] = '\0';
	path[0] = '/';
	path[1] = '\0';
	*port = 80;

	if (!endpoint || !*endpoint)
	{
		pub_log_error("[%s][%d] endpoint is null!", __FILE__, __LINE__);
		return -1;
	}
#ifdef SW_USE_OPENSSL 
	if (!http_tag_cmp(endpoint, "https:*"))
	{
		*port = 443;
	}
#endif
	s = strchr(endpoint, ':');
	if (s && s[1] == '/' && s[2] == '/')
	{
		s += 3;
	}
	else
	{
		s = endpoint;
	}
	n = strlen(s);
	for (i = 0; i < n; i++)
	{
		host[i] = s[i];
		if (s[i] == '/' || s[i] == ':')
		{
			break;
		}
	}
	host[i] = '\0';
	if (s[i] == ':')
	{
		*port = (int)strtol(s + i + 1, NULL, 10);
		for (i++; i < n; i++)
		{
			if (s[i] == '/')
			{
				break;
			}
		}
	}
	if (i < n && s[i])
	{
		strcpy(path, s + i);
	}

	return 0;
}

static const char hex_chars[] = "0123456789abcdef";

/* everything except: ! ( ) * - . / 0-9 A-Z _ a-z */
static const char encoded_chars_rel_uri[] = {
	/*
	   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
	1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0,  /*  20 -  2F space " # $ % & ' + , */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  /*  30 -  3F : ; < = > ? */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F @ */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  /*  50 -  5F [ \ ] ^ */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F ` */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,  /*  70 -  7F { | } ~ DEL */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  80 -  8F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  90 -  9F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  A0 -  AF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  B0 -  BF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  C0 -  CF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  D0 -  DF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  E0 -  EF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  F0 -  FF */
};

int url_encoded(const char *s, int s_len, char *out, int *len)
{
	int	d_len, ndx;
	unsigned char   *ds, *d;
	const char      *map = encoded_chars_rel_uri;

	for (ds = (unsigned char *)s, d_len = 0, ndx = 0; ndx < s_len; ds++, ndx++)
	{
		if (map[*ds])
		{
			d_len += 3;
		}
		else
		{
			d_len++;
		}
	}

	for (ds = (unsigned char *)s, d = (unsigned char *)out, d_len = 0, ndx = 0; ndx < s_len; ds++, ndx++)
	{
		if (map[*ds])
		{
			d[d_len++] = '%';
			d[d_len++] = hex_chars[((*ds) >> 4) & 0x0F];
			d[d_len++] = hex_chars[(*ds) & 0x0F];
		}
		else
		{
			d[d_len++] = *ds;
		}
	}
	out[d_len] = '\0';
	*len = d_len;

	return 0;
}


static char hex2int(unsigned char hex)
{
	hex = hex - '0';
	if (hex > 9)
	{
		hex = (hex + '0' - 1) | 0x20;
		hex = hex - 'a' + 11;
	}
	if (hex > 15)
	{
		hex = 0xFF;
	}

	return hex;
}

static int urldecode_internal(char *url, int *len, int is_query)
{
	char    *dst;
	const char      *src;
	unsigned char   high, low;

	if (url == NULL)
	{
		pub_log_error("[%s][%d] url is null!", __FILE__, __LINE__);
		return -1;
	}

	src = (const char*)url;
	dst = (char*)url;
	while (*src != '\0')
	{
		if (is_query && *src == '+')
		{
			*dst = ' ';
		}
		else if (*src == '%')
		{
			*dst = '%';
			high = hex2int(*(src + 1));
			if (high != 0xFF) {
				low = hex2int(*(src + 2));
				if (low != 0xFF)
				{
					high = (high << 4) | low;
					if (high < 32 || high == 127)
					{
						high = '_';
					}
					*dst = high;
					src += 2;
				}
			}
		}
		else
		{
			*dst = *src;
		}

		dst++;
		src++;
	}

	*dst = '\0';
	*len = (dst - url) + 1;

	return 0;
}

int urldecode_path(char *url, int *len)
{
	return urldecode_internal(url, len, 0);
}

int urldecode_query(char *url, int *len)
{
	return urldecode_internal(url, len, 1);
}

