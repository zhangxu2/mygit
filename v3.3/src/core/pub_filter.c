#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "pub_hash.h"
#include "pub_log.h"
#include "pub_vars.h"
#include "pub_filter.h"

#define MAX_NAME_LEN	64
#define MAX_DISTINCT_ITEM	512

static int	g_filter = 0;
static struct hash	*g_filter_bucket = NULL;

typedef struct
{
	int	length;
	char	name[MAX_NAME_LEN];
}filter_node_t;

static int filter_push(char *name);

unsigned int filter_var_key(unsigned int buckets, void *p_key)
{
	unsigned int	bucket = 0;
	char	*ptr = (char *)p_key;

	while (*ptr != '\0')
	{
		bucket += (unsigned char)(*ptr);
		ptr++;
	}

	return bucket % buckets;
}

int pub_filter_init()
{
	int	len = 0;
	int	ret = 0;
	FILE	*fp = NULL;
	char	line[256];
	char	filename[128];

	memset(line, 0x0, sizeof(line));
	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/cfg/common/filter_cfg.txt", getenv("SWWORK"));
	ret = access(filename, F_OK);
	if (ret < 0)
	{
		pub_log_info("[%s][%d] 过滤文件[%s]不存在,不进行过滤!", __FILE__, __LINE__, filename);
		return 0;
	}
	
	g_filter_bucket = hash_alloc(MAX_DISTINCT_ITEM, MAX_NAME_LEN, sizeof(filter_node_t), filter_var_key);
	if (g_filter_bucket == NULL)
	{
		pub_log_error("[%s][%d] Hash alloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	
	while (1)
	{
		memset(line, 0x0, sizeof(line));
		if (fgets(line, sizeof(line), fp) == NULL)
		{
			break;
		}
		
		len = strlen(line);
		if (line[len - 1] == '\n' || line[len - 1] == 'r')
		{
			line[len - 1] = '\0';
		}
		pub_log_info("过滤变量:[%s]", line);
		filter_push(line);
	}
	fclose(fp);
	
	g_filter = 1;
	
	return 0;
}

int pub_filter_free()
{
	unsigned int	i = 0;
	struct hash	*p_hash = NULL;
	struct hash_node	*p_node = NULL;
	struct hash_node	*p_node1 = NULL;
	struct hash_node	**p_bucket = NULL;
	
	if (g_filter_bucket == NULL)
	{
		return -1;
	}
	
	p_hash = g_filter_bucket;

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

static int filter_push(char *name)
{
	int	ret = 0;
	filter_node_t	*phnode = NULL;
	filter_node_t	hnode;
	
	memset(&hnode, 0x0, sizeof(hnode));
	
	phnode = hash_lookup_entry(g_filter_bucket, name);
	if (phnode != NULL)
	{
		pub_log_info("[%s][%d] %s already exist!", __FILE__, __LINE__, name);
		return 0;
	}

	phnode = &hnode;
	memcpy(phnode->name, name, sizeof(phnode->name) - 1);
        ret = hash_add_entry(g_filter_bucket, name, phnode);
        if (ret != 0)
        {
		pub_log_error("[%s][%d] Add filter node error!", __FILE__, __LINE__);
                return -1;
        }
	
	return 0;
}

int pub_filter_print()
{
	unsigned int	i = 0;
	filter_node_t	*pvnode = NULL;
	struct hash	*p_hash = NULL;
	struct hash_node	*p_node = NULL;
	struct hash_node	*p_node1 = NULL;
	struct hash_node	**p_bucket = NULL;
	
	if (g_filter_bucket == NULL)
	{
		pub_log_error("[%s][%d] g_filter_bucket is null!", __FILE__, __LINE__);
		return -1;
	}
	
	p_hash = g_filter_bucket;

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
				pvnode = (filter_node_t *)p_node1->p_value;
				if (pvnode->name != NULL)
				{
					pub_log_info("[%s][%d] name=[%s]", __FILE__, __LINE__, pvnode->name);
				}
			}
		}
	}

	return 0;
}

int pub_is_filter(char *name)
{
	filter_node_t	*pnode = NULL;

	if (g_filter == 0 || g_filter_bucket == NULL)
	{
		return 0;
	}
	
	pnode = hash_lookup_entry(g_filter_bucket, name);
	if (pnode != NULL)
	{
		return 1;
	}
	
	return 0;
}

