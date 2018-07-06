#include "pub_log.h"
#include "pub_xml.h"
#include "pub_hvar.h"
#include "pub_buf.h"

char *zip_space(char *str)
{
	char	*ptr;
	char	*destr;
	
	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
		{
			*ptr++ = *str;
		}
		str++;
	}
	*ptr = '\0';

	return destr;
}

char *zip_head(char *str)
{
	char	*ptr;
	char	*destr;
	
	ptr = str;
	destr = ptr;
	while (*str != '\0')
	{
		if ((*str != ' ') && (*str != '\t'))
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

char *zip_tail(char *str)
{
	char	*ptr;
	char	*destr;

	destr = str;
	ptr = str + strlen(str) - 1;
	while (str < ptr)
	{
		if ((*ptr != ' ') && (*ptr != '\t'))
		{
			break;
		}
		*ptr = '\0';
		ptr--;
	}
	if (*ptr == ' ' || *ptr == '\t')
	{
		*ptr = '\0';
	}
	else
	{
		ptr++;
		*ptr = '\0';
	}
	
	return destr;
}

char *msstrtok(char *instr, char *outstr, char *delimiter)
{
	char *tmpstr;
	
	if (memcmp(instr, delimiter, strlen(instr)) == 0)
	{
		return(NULL);
	}
	if (instr == NULL || strlen(instr) == 0)
	{
		return NULL;
	}
	tmpstr = strstr(instr, delimiter);
	if (tmpstr != NULL)
	{
		memcpy(outstr, instr, strlen(instr) - strlen(tmpstr));
		return (strstr(instr, delimiter) + strlen(delimiter));
	}
	else
	{
		memcpy(outstr, instr, strlen(instr));
		return NULL;
	}
}

int asc_to_bcd(char *asc, char *bcd, int length)
{
	int i;
	for(i=0;i<length/2;i++)
	{
		unsigned char ch1,ch2;
		ch1=(unsigned char)asc[i*2];
		ch2=(unsigned char)asc[i*2+1];
		if(ch1>='a' && ch1<='f')
			ch1=ch1-'a'+0xa;
		else if(ch1>='A' && ch1<='F')
			ch1=ch1-'A'+0xa;
		else
			ch1=ch1-'0';

		if(ch2>='a' && ch2<='f')
			ch2=ch2-'a'+0xa;
		else if(ch2>='A' && ch2<='F')
			ch2=ch2-'A'+0xa;
		else
			ch2=ch2-'0';
		bcd[i]=(ch1<<4)|ch2;
	}
	return(0);
}

/****bcd->asc***/                                                                                                                  
int bcd_to_asc(char *bcd,char *asc,int length)
{
	int	i;
	for(i=0;i<length;i++)
	{
		unsigned char ch;
		ch=(unsigned char)bcd[i];
		ch=ch>>4;
		if(ch>=10){
			asc[2*i]=ch-10+'A';
		}else{
			asc[2*i]=ch+'0';
		}

		ch=(unsigned char)bcd[i];
		ch=ch &0x0f;
		if(ch>=10){
			asc[2*i+1]=ch-10+'A';
		}else{
			asc[2*i+1]=ch+'0';
		}
	}
	return(0);                                                                                                                 
}

unsigned int var_key(unsigned int buckets, void *p_key)
{
	unsigned int    bucket = 0;
	char    *ptr = (char *)p_key;

	while (*ptr != '\0')
	{
		bucket += (unsigned char)(*ptr);
		ptr++;
	}
													   
	return bucket % buckets;                                                                                                   
}

int comvar_init(sw_hvar_t *vars)
{
	vars->comvar = hash_alloc(MAX_DISTINCT_ITEM, MAX_VARNAME_LEN, sizeof(var_node_t), var_key);
	if (vars->comvar == NULL)
	{
		pub_log_error("[%s][%d] Var alloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	return 0;
}

int comvar_free(sw_hvar_t *vars, char *name)
{
	var_node_t	*pvnode = NULL;
	
	if (vars == NULL || vars->comvar == NULL)
	{
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}
	
	pvnode = hash_lookup_entry(vars->comvar, name);
	if (pvnode != NULL && pvnode->value != NULL)
	{
		free(pvnode->value);
	}
	hash_free_entry(vars->comvar, name);
	
	return 0;
}

int comvar_free_hash(struct hash *p_hash)
{
	unsigned int	i = 0;
	var_node_t	*pvnode = NULL;
	struct hash_node	*p_node = NULL;
	struct hash_node	*p_node1 = NULL;
	struct hash_node	**p_bucket = NULL;
	
	if (p_hash == NULL)
	{
		return -1;
	}
	
	for (i = 0; i < p_hash->buckets; i++)
	{
		p_bucket = &(p_hash->p_nodes[i]);
		p_node = *p_bucket;
		while (p_node != NULL)
		{
			p_node1 = p_node;
			p_node = p_node->next;
			if (p_node1->p_value != NULL)
			{
				pvnode = (var_node_t *)p_node1->p_value;
				if (pvnode->value != NULL)
				{
					free(pvnode->value);
				}
				free(p_node1->p_value);
			}
			
			if (p_node1->p_key != NULL)
			{
				free(p_node1->p_key);
			}
			free(p_node1);
		}
	}
	
	if (p_hash->p_nodes != NULL)
	{
		free(p_hash->p_nodes);
	}
	free(p_hash);
	p_hash = NULL;
	
	return 0;
}

int comvar_clear(sw_hvar_t *vars)
{
	if (vars == NULL || vars->comvar == NULL)
	{	
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}
	comvar_free_hash(vars->comvar);	
	vars->comvar = NULL;

	return 0;
}

int comvar_print(sw_hvar_t *vars)
{
	unsigned int	i = 0;
	var_node_t	*pvnode = NULL;
	struct hash	*p_hash = NULL;
	struct hash_node	*p_node = NULL;
	struct hash_node	*p_node1 = NULL;
	struct hash_node	**p_bucket = NULL;
	
	if (vars == NULL || vars->comvar == NULL)
	{
		pub_log_error("[%s][%d] vars is null!", __FILE__, __LINE__);
		return -1;
	}
	p_hash = vars->comvar;

	for (i = 0; i < p_hash->buckets; i++)
	{
		p_bucket = &(p_hash->p_nodes[i]);
		p_node = *p_bucket;
		while (p_node != NULL)
		{
			p_node1 = p_node;
			p_node = p_node->next;
			if (p_node1->p_value != NULL)
			{
				pvnode = (var_node_t *)p_node1->p_value;
				if (pvnode->value != NULL)
				{
					pub_log_info("[%s][%d] name=[%s] value=[%s]", 
						__FILE__, __LINE__, pvnode->name, pvnode->value);
				}
			}
		}
	}
	
	return 0;
}

int comvar_serialize(sw_hvar_t *vars, sw_buf_t *buf)
{
	unsigned int	i = 0;
	char	tmp[16];
	size_t	offset = buf->len;
	var_node_t	*pvnode = NULL;
	struct hash	*p_hash = NULL;
	struct hash_node	*p_node = NULL;
	struct hash_node	*p_node1 = NULL;
	struct hash_node	**p_bucket = NULL;
	
	if (vars == NULL || vars->comvar == NULL)
	{
		pub_log_error("[%s][%d] vars is null!", __FILE__, __LINE__);
		return -1;
	}
	
	offset = buf->len;
	buf_append(buf, "00000000", 8);
	p_hash = vars->comvar;
	for (i = 0; i < p_hash->buckets; i++)
	{
		p_bucket = &(p_hash->p_nodes[i]);
		p_node = *p_bucket;
		while (p_node != NULL)
		{
			p_node1 = p_node;
			p_node = p_node->next;
			if (p_node1->p_value != NULL)
			{
				pvnode = (var_node_t *)p_node1->p_value;
				buf_format_append(buf, "!%02d%s%c%04d", 
					strlen(pvnode->name), pvnode->name, pvnode->type, pvnode->length);
				buf_append(buf, pvnode->value, pvnode->length);
			}
		}
	}
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08zd", buf->len - offset);
	memcpy(buf->data + offset, tmp, 8);
	
	return 0;
}

int comvar_unserialize(sw_hvar_t *vars, char *str)
{
	int	length = 0;
	long	offset = 0;
	long	total = 0;
	char	tmp[32];
	char	name[MAX_VARNAME_LEN];
	
	if (vars == NULL || vars->comvar == NULL)
	{	
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}
	
	memset(tmp, 0x0, sizeof(tmp));
	memcpy(tmp, str, 8);
	total = atol(tmp);
	offset = 8;
	
	while (offset < total)
	{
		/*** 叹号 ***/
		offset += 1;
		
		/*** 两位变量名长度 ***/
		memset(tmp, 0x0, sizeof(tmp));
		memcpy(tmp, str + offset, 2);
		offset += 2;
		length = atoi(tmp);
		
		/*** 变量名 ***/
		memset(name, 0x0, sizeof(name));
		memcpy(name, str + offset, length);
		offset += length;
		if (name[0] != '$' && name[0] != '#')
		{
			pub_log_error("[%s][%d] 发现非法变量[%s]", __FILE__, __LINE__, name);
			return -1;
		}
		
		/*** 变量类型 ***/
		memset(tmp, 0x0, sizeof(tmp));
		memcpy(tmp, str + offset, 1);
		offset += 1;
		
		/*** 四位变量值长度 ***/
		memset(tmp, 0x0, sizeof(tmp));
		memcpy(tmp, str + offset, 4);
		offset += 4;
		length = atoi(tmp);
		
		/*** 变量值 ***/
		var_set_bin(vars, name, str + offset, length);
		offset += length;
	}
	
	return offset;
}

int comvar_set_value(sw_hvar_t *vars, char *name, char *value, size_t size, char type)
{
	int	ret = 0;
	size_t	value_size = size;
	var_node_t	*pvnode = NULL;
	var_node_t	var_node;
	
	memset(&var_node, 0x0, sizeof(var_node));
	
	if (vars == NULL || vars->comvar == NULL)
	{
		pub_log_error("[%s][%d] vars is null!", __FILE__, __LINE__);
		return -1;
	}
	
	if (name == NULL || name[0] == '\0' || (name[0] != '$' && name[0] != '#'))
	{
		pub_log_error("[%s][%d] name [%s] error!", __FILE__, __LINE__, name);
		return -1;
	}
	
	if (type == 'a' || type == 'A')
	{
		value_size = strlen(value);
	}
	
	pvnode = hash_lookup_entry(vars->comvar, name);
	if (pvnode != NULL)
	{
		free(pvnode->value);
		pvnode->value = (char *)calloc(1, value_size + 1);
		if (pvnode->value == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, value_size, errno, strerror(errno));
			return -1;
		}
		memcpy(pvnode->value, value, value_size);
		pvnode->length = value_size;
		pvnode->type = type;

		return 0;
	}
	
	pvnode = &var_node;
	memcpy(pvnode->name, name, sizeof(pvnode->name) - 1);
	pvnode->value = (char *)calloc(1, value_size + 1);
	if (pvnode->value == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, value_size, errno, strerror(errno));
		return -1;
	}
	memcpy(pvnode->value, value, value_size);
	pvnode->length = value_size;
	pvnode->type = type;
	
	ret = hash_add_entry(vars->comvar, name, pvnode);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] add var error!", __FILE__, __LINE__);
		return -1;
	}
	
	return 0;
}

char *comvar_get_null(sw_hvar_t *vars, char *name, size_t size)
{
	int	ret = 0;
	char	type = 'b';
	size_t	value_size = size;
	var_node_t	*pvnode = NULL;
	var_node_t	var_node;
	
	memset(&var_node, 0x0, sizeof(var_node));
	
	if (vars == NULL || vars->comvar == NULL)
	{
		pub_log_error("[%s][%d] vars is null!", __FILE__, __LINE__);
		return NULL;
	}
	
	if (name == NULL || name[0] == '\0' || (name[0] != '$' && name[0] != '#'))
	{
		pub_log_error("[%s][%d] name [%s] error!", __FILE__, __LINE__, name);
		return NULL;
	}
	
	pvnode = hash_lookup_entry(vars->comvar, name);
	if (pvnode != NULL)
	{
		free(pvnode->value);
		pvnode->value = (char *)calloc(1, value_size + 1);
		if (pvnode->value == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, value_size, errno, strerror(errno));
			return NULL;
		}
		pvnode->length = value_size;
		pvnode->type = type;

		return pvnode->value;
	}
	
	pvnode = &var_node;
	memcpy(pvnode->name, name, sizeof(pvnode->name) - 1);
	pvnode->value = (char *)calloc(1, value_size + 1);
	if (pvnode->value == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, value_size, errno, strerror(errno));
		return NULL;
	}
	pvnode->length = value_size;
	pvnode->type = type;
	
	ret = hash_add_entry(vars->comvar, name, pvnode);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] add var error!", __FILE__, __LINE__);
		return NULL;
	}
	
	return pvnode->value;
}

size_t comvar_get_value(sw_hvar_t *vars, char *name, char *value)
{
	var_node_t	*pvnode = NULL;
	
	if (vars == NULL || vars->comvar == NULL)
	{	
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}	
	
	pvnode = hash_lookup_entry(vars->comvar, name);
	if (pvnode != NULL)
	{
		memcpy(value, pvnode->value, pvnode->length);
		return pvnode->length;
	}
	value[0] = '\0';
	
	return -1;
}

size_t comvar_get_field_len(sw_hvar_t *vars, char *name)
{
	var_node_t	*pvnode = NULL;
	
	if (vars == NULL || vars->comvar == NULL)
	{	
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}
	
	pvnode = hash_lookup_entry(vars->comvar, name);
	if (pvnode != NULL)
	{
		return pvnode->length;
	}
	
	return 0;
}

char *comvar_get_var_addr(sw_hvar_t *vars, char *name, int *len)
{
	var_node_t	*pvnode = NULL;
	
	if (vars == NULL || vars->comvar == NULL)
	{	
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return NULL;
	}
	
	pvnode = hash_lookup_entry(vars->comvar, name);
	if (pvnode != NULL)
	{
		*len = pvnode->length;
		return pvnode->value;
	}
	
	return NULL;
}

static sw_xmlnode_t *extvar_get_node_by_exp(sw_xmltree_t *pxml, sw_xmlnode_t *pnode, char *express)
{
	sw_xmlnode_t	*pnode1 = NULL;
	sw_xmlnode_t	*pnode2 = NULL;
	sw_xmlnode_t	*pnode_bak = NULL;

	if (express[0] >= '0' && express[0] <= '9')
	{
		/****常量索引****/
		int	i = 0;
		int	index = atoi(express);

		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (i == index)
			{
				break;
			}
			pnode1 = pnode1->next;
			i++;
		}
		if (pnode1 == NULL)
		{
			/****未找到指定结点****/
			return NULL;
		}
		return pnode1;
	}
	else
	{
		/****表达式****/
		size_t	com_pos = 0;
		int	equal_flag = 0;/**相等标志,0-相等,1-不等****/
		char	loc_buf[128];

		pnode_bak = pxml->current;
		pxml->current = pnode;
		while (express[com_pos] != '\0')
		{
			if (express[com_pos] == '!' || express[com_pos] == '=')
			{
				break;
			}
			com_pos++;
		}
		if (express[com_pos] == '!' && express[com_pos+1] != '=')
		{
			pxml->current = pnode_bak;
			return NULL;
		}
		if (com_pos > sizeof(loc_buf) - 1)
		{
			pxml->current = pnode_bak;
			return NULL;
		}
		memcpy(loc_buf, express, com_pos);
		loc_buf[com_pos] = '\0';
		if (express[com_pos] == '!')
		{
			com_pos += 2;
			equal_flag = 1;
		}
		else
		{
			com_pos++;
			equal_flag = 0;
		}
		pnode1 = pnode;
		while (pnode1 != NULL)
		{
			pxml->current = pnode1;
			if (strcmp(pnode1->name, pnode->name) != 0)
			{
				pnode1 = pnode1->next;
				continue;
			}
			pnode2 = pub_xml_locnode(pxml, loc_buf);
			if (pnode2 == NULL || pnode2->value == NULL)
			{
				pnode1 = pnode1->next;
				continue;
			}
			if (equal_flag == 0)
			{
				if (strcmp(pnode2->value, express + com_pos) == 0)
				{
					/****相等,找到****/
					break;
				}
				else
				{
					pnode1 = pnode1->next;
					continue;
				}
			}
			else if (equal_flag == 1)
			{
				if (strcmp(pnode2->value, express + com_pos) != 0)
				{
					/****不等,找到****/
					break;
				}
				else
				{
					pnode1 = pnode1->next;
					continue;
				}
			}
			pnode1 = pnode1->next;
		}
		pxml->current = pnode_bak;
		if (pnode1 != NULL)
		{
			return pnode1;
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}

static sw_xmlnode_t *extvar_get_var(sw_hvar_t *vars, char *name, int creat)
{
	int	pos = 0;
	int	pre = 0;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node_bak = NULL;

	if (vars == NULL || name == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return NULL;
	}
	
	if (vars->tree == NULL)
	{
		vars->tree = pub_xml_unpack(NULL);
	}
	
	if (vars->tree == NULL)
	{
		return NULL;
	}
	
	if (name[0]=='.')
	{
	 	vars->tree->current = vars->tree->root;
  	}
  	
	pos = pre = 0;
	node_bak = vars->tree->current;
	while (name[pos] != '\0')
	{
		if(name[pos] == '(')
		{
			while (name[pos] != '\0')
			{
				if(name[pos] != ')')
				{
					pos++;
					continue;
				}
				else
				{
					break;
				}
			}
			
			if (name[pos]!=')')
			{
				vars->tree->current = node_bak;
				return NULL;
			}
		}
		
		pos++;
		if (name[pos] == '.' || name[pos] == '\0')
		{
			char	var_name[1024];
			char	experss[1024];
			if (name[pos-1]==')')
			{

				int	i = pre;
				
				while (i < pos)
				{
					if(name[i]=='(')
					{
						break;
					}
					
					var_name[i-pre] = name[i];
					i++;
				}
				
				if(i < pos)
				{
					var_name[i - pre] = '\0';
					memcpy(experss, name + i + 1, pos - i - 2);
					experss[pos - i - 2] = '\0';
				}
				else
				{
					vars->tree->current = node_bak;
					return NULL;
				}
			}
			else
			{
				memcpy(var_name, name + pre, pos - pre);
				var_name[pos - pre] = '\0';
				experss[0] = '\0';
			}
			
			zip_space(experss);
			node1 = pub_xml_locnode(vars->tree, var_name);
			if(node1 != NULL && experss[0] != '\0')
			{
				node1 = extvar_get_node_by_exp(vars->tree, node1, experss);
			}
			
			if (node1 == NULL)
			{
				if (name[pos] == '\0' && creat)
				{
					if (var_name[0] != '.')
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name, "",SW_NODE_ELEMENT);
						vars->tree->current = node_bak;
					}
					else
					{
						node2 = pub_xml_addnode(vars->tree->current, var_name + 1, "",SW_NODE_ELEMENT);
						vars->tree->current = node2;
					}
					
					return node2;
				}
		
				vars->tree->current = node_bak;
				
				return NULL;
			}
			
			vars->tree->current = node1;
			
			if(name[pos] == '\0')
			{
				break;
			}
			
			pos++;
			pre = pos;
		}
	}
	
	node1 = vars->tree->current;
	vars->tree->current = node_bak;
	
	return node1;
}

int extvar_get_value(sw_hvar_t *vars, char* name, char* value)
{
	sw_xmlnode_t	*node1 = NULL;

	if (vars == NULL || name == NULL || value == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	node1 = extvar_get_var(vars, name, 0);
	if(node1 != NULL)
	{
		if (NULL != node1->value)
		{
			strcpy(value, node1->value);
		}
		else
		{
			value[0] = '\0';
		}

		return 0;
	}
	else
	{
		return -1;
	}
}

static int extvar_set_ext_var(sw_xmlnode_t *pvar,char *value)
{
	int	ret = 0;

	if (pvar == NULL)
	{
		return -1;
	}

	ret = pub_xml_set_value(pvar, value);
	if (0 != ret)
	{
		pub_log_error("[%s][%d] Set node value failed!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int extvar_set_value(sw_hvar_t *vars, char *var_name, char *value, int creat)
{
	char	*pointer_in = NULL;
	char	path_flag='1';
	char	out[100];
	char	node[256];
	sw_xmlnode_t	*node1 = NULL;
	
	memset(out, 0x0, sizeof(out));
	memset(node, 0x0, sizeof(node));
	
	pointer_in = var_name;
	if (vars == NULL || var_name == NULL || value == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return -1;
	}
	
	if (var_name[0] == '.')
	{
		pointer_in = pointer_in + 1;
		path_flag = '1';	
	}
	
	while (pointer_in != NULL)
	{
		memset(out, 0x0, sizeof(out));
		out[0] = '.';
		pointer_in = msstrtok(pointer_in, out + 1, ".");
		if ('1' == path_flag)
		{
			path_flag = '0';
		}
		strcat(node, out);
		if ((value == NULL || value[0] == '\0') && creat != 1)
		{
			node1 = extvar_get_var(vars, node, 0);
			
		}
		else
		{
			node1 = extvar_get_var(vars, node, 1);
		}
		
		if (node1 == NULL)
		{
			return 0;
		}	
	}
	
	extvar_set_ext_var(node1, value);
	
	return 0;
}

static int extvar_set_ext_var_attr(sw_hvar_t *vars, sw_xmlnode_t *node, char *attr_name, char *attr_value)
{
	int	ret = 0;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	if (vars == NULL || node == NULL || attr_name == NULL || attr_value == NULL || 
		strlen(attr_name) == 0 || strlen(attr_value) == 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	node_bak = vars->tree->current;
	vars->tree->current = node;
	node1 = pub_xml_locnode(vars->tree, attr_name);
	if (node1 == NULL)
	{
		node1 = pub_xml_addnode(vars->tree->current, attr_name, attr_value, SW_NODE_ATTRIB);
		vars->tree->current = node_bak;
		return 0;
	}

	ret = pub_xml_set_value(node1, attr_value);
	if (0 != ret)
	{
		pub_log_error("[%s][%d] Set node value failed!", __FILE__, __LINE__);
		return -1;
	}
	vars->tree->current = node_bak;
	
	return 0;
}

int extvar_set_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value)
{
	char	*pointer_in = NULL;
	char	out[100];
	char	path_flag = '1';
	char	node_buf[256];
	sw_xmlnode_t	*node1 = NULL;
	
	memset(out, 0x0, sizeof(out));
	memset(node_buf, 0x0, sizeof(node_buf));
	
	if (vars == NULL || var_name == NULL || attr_name == NULL || attr_value == NULL || 
		strlen(var_name) == 0 || strlen(attr_name) == 0 || strlen(attr_value) == 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	pointer_in = var_name;
	if (var_name[0] == '.')
	{
		pointer_in = msstrtok(pointer_in, out, ".");
		path_flag = '1';	
	}
	
	while (pointer_in != NULL)
	{
		memset(out, 0x0, sizeof(out));
		out[0] = '.';
		pointer_in = msstrtok(pointer_in, out+1, ".");
		
		if('1' == path_flag)
		{
			path_flag = '0';
		}
		
		strcat(node_buf, out);
		node1 = extvar_get_var(vars, node_buf, 1);
	}
	extvar_set_ext_var_attr(vars, node1, attr_name, attr_value);	
	
	return 0;
}

static char *extvar_get_ext_var_attr(sw_hvar_t *vars, sw_xmlnode_t *node, char *attr)
{
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	if (vars == NULL || node == NULL || attr == NULL)
	{
		return NULL;
	}
	
	node_bak = vars->tree->current;
	vars->tree->current = (sw_xmlnode_t *)node;
	
	node1 = pub_xml_locnode(vars->tree, attr);
	if (node1 == NULL)
	{
		vars->tree->current = node_bak;
		return NULL;
	}
	vars->tree->current = node_bak;
	
	return node1->value;
}

int extvar_get_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value)
{
	char	*tmp = NULL;
	sw_xmlnode_t	*node1 = NULL;

	if (vars == NULL || var_name == NULL || attr_name == NULL || attr_value == NULL || 
		strlen(var_name) == 0 || strlen(attr_name) == 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	node1 = extvar_get_var(vars, var_name, 0);
	if (node1 != NULL)
	{
		tmp = extvar_get_ext_var_attr(vars, node1, attr_name);
		if (tmp != NULL)
		{
			strcpy(attr_value, tmp);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int extvar_serialize(sw_hvar_t *vars, sw_buf_t *buf)
{
	int	ret = 0;
	int	offset = buf->len;	
	int	length = 0;
	char	tmp[32];
	
	memset(tmp, 0x0, sizeof(tmp));
	offset = buf->len;
	buf->len  += 8;
	
	if (vars->tree != NULL)
	{
		length = xml_pack_length(vars->tree);                                                                                         
		if (length < 0)                                                                                                            
		{                                                                                                                          
			pub_log_error("[%s][%d] Get xml len error!", __FILE__, __LINE__);
			return -1;
		}
		
		ret = buf_checksize(buf, length);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] buf_checksize error!", __FILE__, __LINE__);
			return -1;
		}
		pub_xml_pack_ext(vars->tree, buf->data + buf->len);                                                                            
		length = strlen(buf->data + buf->len);
		buf->len += length;
	}
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "%08d", length);
	memcpy(buf->data + offset, tmp, 8);

        return 0;    
}

int extvar_unserialize(sw_hvar_t *vars, char *str)
{
	int	offset = 0;
	long	extlen = 0;
	char	tmp[32];
	
	memset(tmp, 0x0, sizeof(tmp));
	memcpy(tmp, str, 8);
	extlen = atol(tmp);
	offset = 8;
	
	if (extlen > 0)
	{
		vars->tree = pub_xml_unpack_ext(str + offset, extlen);
		if (vars->tree == NULL)
		{
			pub_log_error("[%s][%d] xml unpack error!", __FILE__, __LINE__);
			return -1;
		}
	}
	pub_log_info("[%s][%d] unserialize finished!", __FILE__, __LINE__);
	
	return 0;
}

int extvar_clear(sw_hvar_t *vars)
{
	if (vars->tree != NULL)
	{
		pub_xml_deltree(vars->tree);
		vars->tree = NULL;
	}
	
	return 0;
}

int hvar_init(sw_hvar_t *vars)
{
	int	ret = 0;

	ret = comvar_init(vars);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] comvar init error!", __FILE__, __LINE__);
		return -1;
	}
	vars->tree = NULL;
	
	return 0;
}

int hvar_clear(sw_hvar_t *vars)
{
	if (vars == NULL)
	{
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}
	comvar_clear(vars);
	extvar_clear(vars);
	
	return 0;
}

int hvar_set_value(sw_hvar_t *vars, char *name, char *value, size_t size, char type)
{
	if (vars == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	switch (name[0])
	{
		case '$':
		case '#':
			comvar_set_value(vars, name, value, size, type);
			break;
		case '.':
			extvar_set_value(vars, name, value, 0);
			break;
		default:
			pub_log_error("[%s][%d] name error! name=[%s]", __FILE__, __LINE__, name);
			return -1;
	}
	
	return 0;
}

int hvar_xml_value(sw_hvar_t *vars, char *name, char *value)
{
	if (vars == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (name[0] != '.')
	{
		pub_log_error("[%s][%d] name error! name=[%s]", __FILE__, __LINE__);
		return -1;
	}
	extvar_set_value(vars, name, value, 1);
	
	return 0;
}

size_t hvar_get_value(sw_hvar_t *vars, char *name, char *value)
{
	size_t	len = 0;

	if (vars == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	switch (name[0])
	{
		case '$':
		case '#':
			len = comvar_get_value(vars, name, value);
			break;
		case '.':
			extvar_get_value(vars, name, value);
			len = strlen(value);
			break;
		default:
			pub_log_error("[%s][%d] name error! name=[%s]", __FILE__, __LINE__, name);
			return 0;
	}
	
	return len;	
}

int hvar_remove(sw_hvar_t *vars, char *name)
{
	if (vars == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	if (name[0] == '#' || name[0] == '$')
	{
		return comvar_free(vars, name);
	}
	
	return -1;
}

int hvar_serialize(sw_hvar_t *vars, sw_buf_t *buf)
{
	int	ret = 0;
	
	if (vars == NULL)
	{
		pub_log_error("[%s][%d] Vars is null!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = comvar_serialize(vars, buf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] comvar serialize error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = extvar_serialize(vars, buf);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] extvar serialize error!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] serialize success!", __FILE__, __LINE__);
	
	return 0;
}

int hvar_unserialize(sw_hvar_t *vars, char *str)
{
	int	ret = 0;
	int	offset = 0;
	
	offset = comvar_unserialize(vars, str);
	if (offset < 0)
	{
		pub_log_error("[%s][%d] comvar unserialize error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = extvar_unserialize(vars, str + offset);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] extvar unserialize error!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] unserialize success!", __FILE__, __LINE__);
	
	return 0;
}

int hvar_set_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value)
{
	return extvar_set_attr(vars, var_name, attr_name, attr_value);
}

int hvar_get_attr(sw_hvar_t *vars, char *var_name, char *attr_name, char *attr_value)
{
	return extvar_get_attr(vars, var_name, attr_name, attr_value);
}

int hvar_get_field_len(sw_hvar_t *vars, char *name)
{
	return comvar_get_field_len(vars, name);
}

char *hvar_get_var_addr(sw_hvar_t *vars, char *name, int *len)
{
	return comvar_get_var_addr(vars, name, len);
}

char *hvar_get_null(sw_hvar_t *vars, char *name, int len)
{
	return comvar_get_null(vars, name, len);
}

