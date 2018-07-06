#include "pub_log.h"
#include "pub_type.h"
#include "pub_vars.h"

static sw_int_t strcpy_ext(char *s1, char *s2)
{
	int	i = 0;
	int	j = 0;

	while (s2[i] != '\0')
	{
		if (s2[i] == '>')
		{
			strcpy(s1 + j, "&gt;");
			j+=4;
			i++;
		}
		else if (s2[i] == '<' )
		{
			strcpy(s1+j,"&lt;");
			j+=4;
			i++;
		}
		else if (s2[i] == '&')
		{
			strcpy(s1+j,"&amp;");
			j+=5;
			i++;
		}
		else if (s2[i] == '\'')
		{
			strcpy(s1+j,"&apos;");
			j+=6;
			i++;
		}
		else if (s2[i] == '"')
		{
			strcpy(s1+j,"&quot;");
			j+=6;
			i++;
		}
		else
		{
			s1[j++]=s2[i++];
		}
	}
	s1[j]='\0';

	return 0;
}

int dealpkg(sw_loc_vars_t *ovars, char *buf, sw_int_t *len, int flag)
{
	int	ret = 0;
	int	length = 0;
	int	oxmllen = 0;
	int	cur_len = 0;
	int	cur_xmllen = 0;
	char	*ptr = NULL;
	char	*ptmp = NULL;
	char	*obuf = NULL;
	char	*odbuf = NULL;
	char	*oxmlbuf = NULL;
	char	*cur_xmlbuf = NULL;
	char	label[64];
	char	txcode[32];
	sw_loc_vars_t	vars;
	
	if (flag == SW_DEC)
	{
		pub_log_info("[%s][%d] dealpkg begin...", __FILE__, __LINE__);

		memset(txcode, 0x0, sizeof(txcode));
		loc_get_zd_data(&vars, "#jybh", txcode);
		pub_log_info("[%s][%d] jybh=[%s]", __FILE__, __LINE__, txcode);
		
		ret = pub_loc_vars_alloc(&vars, HEAP_VARS);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] Alloc vars error!", __FILE__, __LINE__);
			return -1;
		}
		
		ret = vars.create(&vars, 0);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] Create var error!", __FILE__, __LINE__);
			vars.free_mem(&vars);
			return -1;
		}
		
		vars.set_attr(&vars, ".E:Envelope", "xmlns:E", "http://schemas.xmlsoap.org/soap/envelope/");
		vars.set_attr(&vars, ".E:Envelope", "xmlns:A", "http://schemas.xmlsoap.org/soap/encoding/");
		vars.set_attr(&vars, ".E:Envelope", "xmlns:s", "http://www.w3.org/2001/XMLSchema-instance");
		vars.set_attr(&vars, ".E:Envelope", "xmlns:y", "http://www.w3.org/2001/XMLSchema");
		vars.set_attr(&vars, ".E:Envelope", "E:encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
	
		vars.set_attr(&vars, ".E:Envelope.E:Body.m:callBusiness", "xmlns:m", "http://webservice.test/");
		vars.set_attr(&vars, ".E:Envelope.E:Body.m:callBusiness.jybh", "s:type", "y:string");
		vars.set_xml_var(&vars, ".E:Envelope.E:Body.m:callBusiness.jybh", txcode, 1);
		vars.set_attr(&vars, ".E:Envelope.E:Body.m:callBusiness.jylsh", "s:type", "y:string");
		vars.set_attr(&vars, ".E:Envelope.E:Body.m:callBusiness.inputxml", "s:type", "y:string");
		vars.set_xml_var(&vars, ".E:Envelope.E:Body.m:callBusiness.inputxml", "aa", 1);
		
		/*** 序列化临时建的树 ***/
		cur_xmllen = xml_pack_length(vars.tree);
		if (cur_xmllen < 0)
		{
			pub_log_error("[%s][%d] Get xml length error!", __FILE__, __LINE__);
			vars.destroy(&vars);
			vars.free_mem(&vars);
			return -1;
		}
		
		cur_xmlbuf = (char *)calloc(1, cur_xmllen);
		if (cur_xmlbuf == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, cur_xmllen, errno, strerror(errno));
			vars.destroy(&vars);
			vars.free_mem(&vars);
			return -1;
		}
		pub_xml_pack_ext(vars.tree, cur_xmlbuf);
		vars.destroy(&vars);
		vars.free_mem(&vars);
		pub_log_info("[%s][%d] xml tree:[%s]", __FILE__, __LINE__, cur_xmlbuf);
		
		/*** 序列化原xml树 ***/
		oxmllen = xml_pack_length(ovars->tree);
		if (oxmllen < 0)
		{
			pub_log_error("[%s][%d] Get xml length error!", __FILE__, __LINE__);
			return -1;
		}
	
		oxmlbuf = (char *)calloc(1, oxmllen * 2);
		if (oxmlbuf == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, oxmllen * 2, errno, strerror(errno));
			return -1;
		}
		pub_xml_pack_ext(ovars->tree, oxmlbuf);	
		pub_log_info("[%s][%d] 替换之前原报文=[%s]", __FILE__, __LINE__, oxmlbuf);
		
		obuf = (char *)calloc(1, oxmllen * 2);
		if (obuf == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, oxmllen * 2, errno, strerror(errno));
			free(oxmlbuf);
			return -1;
		}
		/***
		  <controlxml>
                    <![CDATA[<INPUT><ROWS><ROW><ID>1001</ID><RZM>1001</RZM>  <YKE015>201</YKE015>></ROW></ROWS></INPUT>]]>
                  </controlxml>  
                  <inputxml>
                    <![CDATA[<INPUT><ROWS><ROW><AAC002>440781198007224811</AAC002></ROW></ROWS></INPUT>]]>
                  </inputxml> 
		***/
		ptr = strstr(oxmlbuf, "<controlxml>");
		if (ptr == NULL)
		{	
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, oxmlbuf);
			free(oxmlbuf);
			free(obuf);
			return -1;
		}
		while (*ptr != '>' && *ptr != '\0')
		{
			ptr++;
		}
		if (*ptr == '\0')
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, oxmlbuf);
			free(oxmlbuf);
			free(obuf);
			return -1;
		}
		ptr++;
		
		memset(label, 0x0, sizeof(label));
		strcpy(label, "<input><controlxml>");
		length = strlen(label);
		memcpy(obuf, label, length);
		length = strlen("<![CDATA[");
		memcpy(obuf + cur_len, "<![CDATA[", length);
		cur_len += length;
		ptmp = strstr(ptr, "</controlxml>");
		if (ptmp == NULL)
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, oxmlbuf);
			free(oxmlbuf);
			free(obuf);
			return -1;
		}
		length = ptmp - ptr;
		memcpy(obuf + cur_len, ptr, length);
		cur_len += length;
		memcpy(obuf + cur_len, "]]>", 3);
		cur_len += 3;
		memset(label, 0x0, sizeof(label));
		strcpy(label, "</controlxml>");
		length = strlen(label);
		memcpy(obuf + cur_len, label, length);
		cur_len += length;
		
		ptr = strstr(oxmlbuf, "<inputxml>");
		if (ptr == NULL)
		{	
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, oxmlbuf);
			free(oxmlbuf);
			free(obuf);
			return -1;
		}
		while (*ptr != '>' && *ptr != '\0')
		{
			ptr++;
		}
		if (*ptr == '\0')
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, oxmlbuf);
			free(oxmlbuf);
			free(obuf);
			return -1;
		}
		ptr++;
		
		memset(label, 0x0, sizeof(label));
		strcpy(label, "<inputxml>");
		length = strlen(label);
		memcpy(obuf + cur_len, label, length);
		cur_len += length;
		length = strlen("<![CDATA[");
		memcpy(obuf + cur_len, "<![CDATA[", length);
		cur_len += length;
		ptmp = strstr(ptr, "</inputxml>");
		if (ptmp == NULL)
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, oxmlbuf);
			free(oxmlbuf);
			free(obuf);
			return -1;
		}
		length = ptmp - ptr;
		memcpy(obuf + cur_len, ptr, length);
		cur_len += length;
		memcpy(obuf + cur_len, "]]>", 3);
		cur_len += 3;
		memset(label, 0x0, sizeof(label));
		strcpy(label, "</inputxml></input>");
		length = strlen(label);
		memcpy(obuf + cur_len, label, length);
		cur_len += length;
		pub_log_info("[%s][%d] 添加CDATA格式后的报文=[%s]:[%d]", __FILE__, __LINE__, obuf, cur_len);
		free(oxmlbuf);

		/*** 对原报文中的特殊字符进行替换 ***/
		odbuf = (char *)calloc(1, oxmllen * 5);
		if (odbuf == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, oxmllen * 5, errno, strerror(errno));
			return -1;
		}
		strcpy_ext(odbuf, obuf);
		free(obuf);
		pub_log_info("[%s][%d] 原报文替换之后=[%s]", __FILE__, __LINE__, odbuf);
		
		ptr = strstr(cur_xmlbuf, "<inputxml");
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, cur_xmlbuf);
			free(cur_xmlbuf);
			free(odbuf);
			return -1;
		}
		while (*ptr != '>' && *ptr != '\0')
		{
			ptr++;
		}
		if (*ptr == '\0')
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, cur_xmlbuf);
			free(cur_xmlbuf);
			free(odbuf);
			return -1;
		}
		ptr++;
		
		cur_len = ptr - cur_xmlbuf;
		oxmllen = strlen(odbuf);
		memcpy(buf, cur_xmlbuf, cur_len);
		memcpy(buf + cur_len, odbuf, oxmllen);
		cur_len += oxmllen;

		while (*ptr != '<' && *ptr != '\0')
		{
			ptr++;
		}
		if (*ptr == '\0')
		{
			pub_log_error("[%s][%d] 报文格式有误! xmlbuf=[%s]", __FILE__, __LINE__, cur_xmlbuf);
			free(cur_xmlbuf);
			free(odbuf);
			return -1;
		}
		strcpy(buf + cur_len, ptr);
		cur_len = strlen(buf);
		free(cur_xmlbuf);
		free(odbuf);
		*len = cur_len;
		pub_log_bin(SW_LOG_DEBUG, buf, cur_len, "[%s][%d] 处理后的报文=[%d]", __FILE__, __LINE__, cur_len);
		
		return 0;
	}
	
	return 0;
}
