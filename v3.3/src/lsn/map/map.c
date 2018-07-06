#include "pub_computexp.h"
#include "pub_xml.h"

int g_send_flag = 0;

#define MAX_PRDT_NUM	30

typedef struct
{
	int	use;
	char	prdt[128];
	sw_xmltree_t	*xml;
}prdt_errinfo_t;


static char *lrtrim(char *str)
{
	char    *ptr = str;
	char    *tptr = str + strlen(str) - 1;
	char    *destr = str;

	while (str <= tptr)
	{
		if (*tptr != ' ' && *tptr != '\t')
		{
			tptr++;
			*tptr = '\0';
			break;
		}
		tptr--;
	}
	while (*str != '\0')
	{
		if (*str != ' ' && *str != '\t')
		{
			break;
		}
		str++;
	}
	while (*str != '\0')                                                                                                       
	{                                                                                                                          
		*ptr++ = *str++;                                                                                                   
	}                                                                                                                          
	*ptr = '\0';                                                                                                               
														   
	return destr;                                                                                                              
}

int check_include(char *var, char *check_var)
{
	char	*p = NULL;
	char	*ptr = NULL;
	char	value[512];
	
	memset(value, 0x0, sizeof(value));
	p = value;
	ptr = check_var;
	while (*ptr != '\0')
	{
		if (*ptr != ' ')
		{
			*p++ = *ptr++;
			continue;
		}
		
		*p = '\0';
		if (strcmp(value, var) == 0)
		{
			return 0;
		}
		ptr++;
		memset(value, 0x0, sizeof(value));
		p = value;
	}
	if (strcmp(value, var) == 0)
	{
		return 0;
	}
	
	return -1;
}

static int check_single(sw_loc_vars_t *locvar, char *check_var)
{
	char	*p = NULL;
	char	*ptr = NULL;
	char	sign[4];
	char	name[128];
	char	value[256];
	char	check_value[256];
	
	memset(sign, 0x0, sizeof(sign));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	memset(check_value, 0x0, sizeof(check_value));
	
	ptr = check_var;
	p = name;
	while (*ptr != '\0' && *ptr != '=' && *ptr != '?')
	{
		*p++ = *ptr++;
	}
	*p = '\0';
	
	if (*ptr == '\0')
	{
		pub_log_error("[%s][%d] CHECK表达式[%s]有误!",
			__FILE__, __LINE__, check_var);
		return -1;
	}
	sign[0] = *ptr;
	ptr++;
	strcpy(check_value, ptr);
	
	loc_get_zd_data(locvar, name, value);

	if (sign[0] == '=')
	{
		if (check_include(value, check_value) == 0)
		{
			return 0;
		}
		return -1;
	}
	
	if (sign[0] == '?')
	{
		if (check_include(value, check_value) == 0)
		{
			return -1;
		}
		return 0;
	}
	
	return -1;
}

int check_mult(sw_loc_vars_t *locvar, sw_xmlnode_t *check_node)
{
	sw_xmlnode_t	*node = NULL;
	
	node = check_node;
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}
		if (check_single(locvar, node->value) != 0)
		{
			return -1;
		}
		node = node->next;
	}
	
	return 0;
}

int map_in(sw_loc_vars_t *locvar, sw_xmltree_t *xml)
{
	int	len = 0;
	int	ret = 0;
	int	zip_item = 1;
	int	zip_package = 1;
	char	aname[128];
	char	bname[128];
	char	value[1024];
	char	tx_code[32];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(aname, 0x0, sizeof(aname));
	memset(bname, 0x0, sizeof(bname));
	memset(value, 0x0, sizeof(value));
	memset(tx_code, 0x0, sizeof(tx_code));

	loc_get_zd_data(locvar, "#tx_code", tx_code);
	pub_log_info("[%s][%d] tx_code=[%s]", __FILE__, __LINE__, tx_code);
	
	node = pub_xml_locnode(xml, ".MAP.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (check_mult(locvar, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}

		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (check_include(tx_code, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		zip_package = zip_item = 1;	
		node1 = pub_xml_locnode(xml, "ZIP");
		if (node1 != NULL && node1->value != NULL && node1->value[0] == '0')
		{
			zip_package = 0;
		}	
	
		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			memset(aname, 0x0, sizeof(aname));
			memset(bname, 0x0, sizeof(bname));	
			memset(value, 0x0, sizeof(value));
			xml->current = node1;
			node2 = pub_xml_locnode(xml, "MAPFLAG");
			if (node2 != NULL && node2->value != NULL)
			{
				if (node2->value[0] == '2')
				{
					node1 = node1->next;
					continue;
				}
			}

			node2 = pub_xml_locnode(xml, "CHECK");
			if (node2 != NULL)
			{
				if (check_mult(locvar, node2) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(xml, "ANAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_info("[%s][%d] Not config ANAME!", __FILE__, __LINE__);
				return -1;
			}
			strncpy(aname, node2->value, sizeof(aname) - 1);
			
			node2 = pub_xml_locnode(xml, "BNAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_info("[%s][%d] Not config BNAME!", __FILE__, __LINE__);
				return -1;
			}
			strncpy(bname, node2->value, sizeof(bname) - 1);
			
			node2 = pub_xml_locnode(xml, "ZIP");
			if (node2 != NULL && node2->value != NULL && node2->value[0] == '0')
			{
				zip_item = 0;
			}
			else
			{
				zip_item = zip_package;
			}
			
			node2 = pub_xml_locnode(xml, "TYPE");
			if (node2 != NULL && node2->value != NULL && (node2->value[0] == 'b' || node2->value[0] == 'B'))
			{
				memset(value, 0x0, sizeof(value));
				len = loc_get_data_len(locvar, aname, value);
				if (len > 0)
				{
					loc_set_zd_data_len(locvar, bname, value, len);
					pub_log_bin(SW_LOG_INFO, value, len, "[%s][%d] B型变量:[%s]=[%s]", __FILE__, __LINE__, bname, aname);
				}
			}
			else
			{
				ret = compute_exp(locvar, aname, value);
				if (ret < 0)
				{
					pub_log_info("[%s][%d] Compute_exp error! Exp=[%s]", __FILE__, __LINE__, aname);
					return -1;
				}
				if (zip_item == 1)
				{
					lrtrim(value);
				}
				loc_set_zd_data(locvar, bname, value);
				
				if (pub_is_filter(aname))
				{
					char buf[2048];
					memset(buf, 0x0, sizeof(buf));
					memset(buf, '*', strlen(value));
					pub_log_debug("[%s][%d] [%s][%s]==>[%s]", __FILE__, __LINE__, aname, buf, bname);
				}
				else
				{
					pub_log_debug("[%s][%d] [%s][%s]==>[%s]", __FILE__, __LINE__, aname, value, bname);
				}
			}
			node1 = node1->next;
		}
		node = node->next;
	}
	pub_log_info("[%s][%d] Map in success!", __FILE__, __LINE__);
	
	return 0;
}

int map_out(sw_loc_vars_t *locvar, sw_xmltree_t *xml)
{
	int	len = 0;
	int	ret = 0;
	int	zip_item = 1;
	int	zip_package = 1;
	char	aname[128];
	char	bname[128];
	char	value[1024];
	char	tx_code[32];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(aname, 0x0, sizeof(aname));
	memset(bname, 0x0, sizeof(bname));
	memset(value, 0x0, sizeof(value));
	memset(tx_code, 0x0, sizeof(tx_code));

	loc_get_zd_data(locvar, "#tx_code", tx_code);
	pub_log_info("[%s][%d] tx_code=[%s]", __FILE__, __LINE__, tx_code);
	
	node = pub_xml_locnode(xml, ".MAP.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (check_mult(locvar, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}

		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (check_include(tx_code, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		zip_package = zip_item = 1;
		node1 = pub_xml_locnode(xml, "ZIP");
		if (node1 != NULL && node1->value != NULL && node1->value[0] == '0')
		{
			zip_package = 0;
		}	

		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			memset(aname, 0x0, sizeof(aname));
			memset(bname, 0x0, sizeof(bname));	
			memset(value, 0x0, sizeof(value));
			xml->current = node1;
			node2 = pub_xml_locnode(xml, "MAPFLAG");
			if (node2 != NULL && node2->value != NULL)
			{
				if (node2->value[0] == '1')
				{
					node1 = node1->next;
					continue;
				}
			}
			
			node2 = pub_xml_locnode(xml, "CHECK");
			if (node2 != NULL)
			{
				if (check_mult(locvar, node2) != 0)
				{
					node1 = node1->next;
					continue;
				}
			}

			node2 = pub_xml_locnode(xml, "ANAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_info("[%s][%d] Not config ANAME!", __FILE__, __LINE__);
				return -1;
			}
			strncpy(aname, node2->value, sizeof(aname) - 1);
			
			node2 = pub_xml_locnode(xml, "BNAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_info("[%s][%d] Not config BNAME!", __FILE__, __LINE__);
				return -1;
			}
			strncpy(bname, node2->value, sizeof(bname) - 1);
			
			node2 = pub_xml_locnode(xml, "ZIP");
			if (node2 != NULL && node2->value != NULL && node2->value[0] == '0')
			{
				zip_item = 0;
			}
			else
			{
				zip_item = zip_package;
			}

			node2 = pub_xml_locnode(xml, "TYPE");
			if (node2 != NULL && node2->value != NULL && (node2->value[0] == 'b' || node2->value[0] == 'B'))
			{
				memset(value, 0x0, sizeof(value));
				len = loc_get_data_len(locvar, bname, value);
				if (len > 0)
				{
					loc_set_zd_data_len(locvar, aname, value, len);
					pub_log_bin(SW_LOG_INFO, value, len, "[%s][%d] B型变量:[%s]=[%s]", __FILE__, __LINE__, aname, bname);
				}
			}
			else
			{
				ret = compute_exp(locvar, bname, value);
				if (ret < 0)
				{
					pub_log_info("[%s][%d] Compute_exp error! Exp=[%s]", __FILE__, __LINE__, aname);
					return -1;
				}
				if (zip_item == 1)
				{
					lrtrim(value);
				}
				loc_set_zd_data(locvar, aname, value);
				
				if (pub_is_filter(bname))
				{
					char buf[2048];
					memset(buf, 0x0, sizeof(buf));
					memset(buf, '*', strlen(value));
					pub_log_debug("[%s][%d] [%s][%s]==>[%s]", __FILE__, __LINE__, bname, buf, aname);
				}
				else
				{
					pub_log_debug("[%s][%d] [%s][%s]==>[%s]", __FILE__, __LINE__, bname, value, aname);
				}
			}
			node1 = node1->next;
		}
		node = node->next;
	}
	pub_log_info("[%s][%d] Map out success!", __FILE__, __LINE__);
	
	return 0;
}

int map(sw_loc_vars_t *locvar, sw_xmltree_t *mapcfg, int flag)
{
	g_send_flag = flag;
	if (flag == O_SEND)
	{
		return map_out(locvar, mapcfg);
	}
	
	return map_in(locvar, mapcfg);
}

int maperrcode(sw_loc_vars_t *locvar, char *from, char *to, char *err)
{
	int	i = 0;
	int	xflag = 0;
	char	chnl[128];
	char	prdt[128];
	char	errmsg[128];
	char	fromcode[32];
	char	tocode[32];
	char	filename[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	static int first = 1;
	static prdt_errinfo_t g_errinfo[MAX_PRDT_NUM];
	
	memset(chnl, 0x0, sizeof(chnl));
	memset(prdt, 0x0, sizeof(prdt));
	memset(fromcode, 0x0, sizeof(fromcode));
	memset(tocode, 0x0, sizeof(tocode));
	memset(filename, 0x0, sizeof(filename));
	memset(errmsg, 0x0, sizeof(errmsg));
	
	if (locvar == NULL || from == NULL || to == NULL)
	{
		pub_log_error("[%s][%d] maperr param error!", __FILE__, __LINE__);
		return -1;
	}
	
	loc_get_zd_data(locvar, "$current_lsn", chnl);
	pub_log_info("[%s][%d] current_lsn=[%s]", __FILE__, __LINE__, chnl);
	
	loc_get_zd_data(locvar, "$product", prdt);
	pub_log_info("[%s][%d] prdt===[%s]", __FILE__, __LINE__, prdt);
	
	if (first)
	{
		memset(&g_errinfo, 0x0, sizeof(g_errinfo));
		first = 0;
		pub_log_info("[%s][%d] first...", __FILE__, __LINE__);
	}
	
	xflag = 0;
	sprintf(filename, "%s/products/%s/etc/maperrcode.xml", getenv("SWWORK"), prdt);
	for (i = 0; i < MAX_PRDT_NUM; i++)
	{
		pub_log_info("[%s][%d] xmltree[%d].use=[%d][%s] prdt=[%s]",
			__FILE__, __LINE__, i, g_errinfo[i].use, g_errinfo[i].prdt, prdt);
		if (g_errinfo[i].use == 1 && strcmp(g_errinfo[i].prdt, prdt) == 0)
		{
			xml = g_errinfo[i].xml;
			pub_log_info("[%s][%d] xmltree is already cached!", __FILE__, __LINE__);
			break;
		}

		if (g_errinfo[i].use == 0)
		{
			xml = pub_xml_crtree(filename);
			if (xml == NULL)
			{
				pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, filename);
				return -1;
			}
			pub_log_info("[%s][%d] create xmltree [%s] success!", __FILE__, __LINE__, filename);
			g_errinfo[i].xml = xml;
			strncpy(g_errinfo[i].prdt, prdt, sizeof(g_errinfo[i].prdt) - 1);
			g_errinfo[i].use = 1;
			break;
		}
	}
	if (i == MAX_PRDT_NUM)
	{
		xml = pub_xml_crtree(filename);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, filename);
			return -1;
		}
		xflag = 1;
		pub_log_info("[%s][%d] create xmltree [%s] success!", __FILE__, __LINE__, filename);
	}
	
	node = pub_xml_locnode(xml, ".maperrcode.channel");
	while (node != NULL)
	{
		if (strcmp(node->name, "channel") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "lsnname");
		if (node1 != NULL && node1->value != NULL)
		{
			if (check_include(chnl, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		break;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Can not find chnl [%s]'s config!", __FILE__, __LINE__, chnl);
		if (xflag == 1)
		{
			pub_xml_deltree(xml);
		}
		if (g_send_flag == O_SEND)
		{
			if (strlen(from) == 4)
			{
				strcpy(to, from);
			}
			else if (strlen(from) == 7)
			{
				strncpy(to, from + 3, 4);
			}
			else
			{
				strcpy(to, "9999");
			}
		}
		else
		{
			if (strlen(from) == 4)
			{
				sprintf(to, "BPE%s", from);
			}
			else
			{
				strcpy(to, "BPE9999");
			}
		}
		
		if (err != NULL)
		{
			if (err[0] == '#' || err[0] == '$')
			{
				loc_set_zd_data(locvar, err, "交易处理失败");
			}
			else
			{
				strcpy(err, "交易处理失败");
			}
		}
		return -1;
	}
	
	node = pub_xml_locnode(xml, "entry");
	while (node != NULL)
	{
		if (strcmp(node->name, "entry") != 0)
		{
			node = node->next;
			continue;
		}
		
		memset(fromcode, 0x0, sizeof(fromcode));
		memset(tocode, 0x0, sizeof(tocode));
		xml->current = node;
		node1 = pub_xml_locnode(xml, "fromcode");
		if (node1 == NULL || node1->value == NULL)
		{
			node = node->next;
			continue;
		}
		strncpy(fromcode, node1->value, sizeof(fromcode) - 1);
	
		node1 = pub_xml_locnode(xml, "tocode");
		if (node1 == NULL || node1->value == NULL)
		{
			node = node->next;
			continue;
		}
		strncpy(tocode, node1->value, sizeof(tocode) - 1);
	
		if (g_send_flag == O_SEND)
		{
			if (strcmp(fromcode, from) != 0)
			{
				node = node->next;
				continue;
			}
		}
		else
		{
			if (strcmp(tocode, from) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		break;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Can not find errcode [%s]'s map config!", 
			__FILE__, __LINE__, from);
		if (xflag == 1)
		{
			pub_xml_deltree(xml);
		}
		
		if (g_send_flag == O_SEND)
		{
			if (strlen(from) == 4)
			{
				strcpy(to, from);
			}
			else if (strlen(from) == 7)
			{
				strncpy(to, from + 3, 4);
			}
			else
			{
				strcpy(to, "9999");
			}
		}
		else
		{
			if (strlen(from) == 4)
			{
				sprintf(to, "BPE%s", from);
			}
			else
			{
				strcpy(to, "BPE9999");
			}
		}

		if (err != NULL)
		{
			if (err[0] == '#' || err[0] == '$')
			{
				loc_set_zd_data(locvar, err, "交易处理失败");
			}
			else
			{
				strcpy(err, "交易处理失败");
			}
		}
		return -1;
	}
	
	if (g_send_flag == O_SEND)
	{
		strcpy(to, tocode);
	}
	else
	{
		strcpy(to, fromcode);
	}
	
	if (err != NULL)
	{	
		memset(errmsg, 0x0, sizeof(errmsg));
		node = pub_xml_locnode(xml, "desc");
		if (node != NULL && node->value != NULL)
		{
			strncpy(errmsg, node->value, sizeof(errmsg) - 1);
		}
		
		if (err[0] == '#' || err[0] == '$')
		{
			loc_set_zd_data(locvar, err, errmsg);
			pub_log_info("[%s][%d] err=[%s]", __FILE__, __LINE__, errmsg);
		}
		else
		{
			strcpy(err, errmsg);
		}
	}
	
	if (xflag == 1)
	{
		pub_xml_deltree(xml);
	}
	pub_log_info("[%s][%d] from=[%s] to=[%s] desc=[%s]", __FILE__, __LINE__, from, to, errmsg);
	
	return 0;
}

