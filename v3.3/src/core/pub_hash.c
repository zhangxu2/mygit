#include "pub_hash.h"
#include "pub_log.h"

/* Internal functions */
static struct hash_node **hash_get_bucket(struct hash *p_hash, void *p_key);
static struct hash_node *hash_get_node_by_key(struct hash *p_hash, void *p_key);

struct hash *hash_alloc(unsigned int buckets, unsigned int key_size,
           unsigned int value_size, hashfunc_t hash_func)
{
	unsigned int	size = 0;
	struct hash	*p_hash = NULL;
	
	p_hash = calloc(1, sizeof(*p_hash));
	if (p_hash == NULL)
	{
		perror("calloc");
		return NULL;
	}

	p_hash->buckets = buckets;
	p_hash->key_size = key_size;
	p_hash->value_size = value_size;
	p_hash->hash_func = hash_func;
	
	size = sizeof(struct hash) * buckets;
	p_hash->p_nodes = calloc(1, size);
	if (p_hash->p_nodes == NULL)
	{
		perror("calloc");
		free(p_hash);
		return NULL;
	}
	
	return p_hash;
}

static struct hash_node **hash_get_bucket(struct hash *p_hash, void *p_key)
{
	unsigned int	bucket = -1;
	
	bucket = p_hash->hash_func(p_hash->buckets, p_key);
	if (bucket >= p_hash->buckets)
	{
		pub_log_error("[%s][%d] Can not find bucket by key!\n", __FILE__, __LINE__);
		return NULL;
	}
	
	return &(p_hash->p_nodes[bucket]);
}

static struct hash_node *hash_get_node_by_key(struct hash *p_hash, void *p_key)
{
	struct hash_node	*p_node = NULL;
	struct hash_node	**p_bucket = NULL;
	
	p_bucket = hash_get_bucket(p_hash, p_key);
	if (p_bucket == NULL)
	{
		pub_log_error("[%s][%d] Get bucket error!\n", __FILE__, __LINE__);
		return NULL;
	}
	
	p_node = *p_bucket;
	while (p_node != NULL && strcmp(p_key, p_node->p_key) != 0)
	{
		p_node = p_node->next;
	}
	
	return p_node;
}

void *hash_lookup_entry(struct hash *p_hash, void *p_key)
{
	struct hash_node	*p_node = NULL;
	
	p_node = hash_get_node_by_key(p_hash, p_key);
	if (p_node == NULL)
	{
		return NULL;
	}
	
	return p_node->p_value;
}

int hash_add_entry(struct hash *p_hash, void *p_key, void *p_value)
{
	struct hash_node	**p_bucket = NULL;
	struct hash_node	*p_new_node = NULL;
	
	if (hash_lookup_entry(p_hash, p_key))
	{
		pub_log_error("[%s][%d] duplicate hash key!\n", __FILE__, __LINE__);
		return -1;
	}
	
	p_bucket = hash_get_bucket(p_hash, p_key);
	if (p_bucket == NULL)
	{
		pub_log_error("[%s][%d] Get bucket error!\n", __FILE__, __LINE__);
		return -1;
	}
	
	p_new_node = calloc(1, sizeof(*p_new_node));
	if (p_new_node == NULL)
	{
		pub_log_error("[%s][%d] Calloc erorr! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	p_new_node->prev = NULL;
	p_new_node->next = NULL;
	p_new_node->p_key = calloc(1, p_hash->key_size);
	if (p_new_node->p_key == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	memcpy(p_new_node->p_key, p_key, p_hash->key_size);
	
	p_new_node->p_value = calloc(1, p_hash->value_size);
	if (p_new_node->p_value == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	memcpy(p_new_node->p_value, p_value, p_hash->value_size);
	
	if (!*p_bucket)
	{
		*p_bucket = p_new_node;
	}
	else
	{
		p_new_node->next = *p_bucket;
		(*p_bucket)->prev = p_new_node;
		*p_bucket = p_new_node;
	}
	
	return 0;
}

int hash_free_entry(struct hash *p_hash, void *p_key)
{
	struct hash_node 	*p_node = NULL;
	struct hash_node	**p_bucket = NULL;
	
	p_node = hash_get_node_by_key(p_hash, p_key);
	if (p_node == NULL)
	{
		pub_log_error("[%s][%d] Hash node not found!\n", __FILE__, __LINE__);
		return -1;
	}
	free(p_node->p_key);
	free(p_node->p_value);
	
	if (p_node->prev != NULL)
	{
		p_node->prev->next = p_node->next;
	}
	else
	{
		p_bucket = hash_get_bucket(p_hash, p_key);
		*p_bucket = p_node->next;
	}
	
	if (p_node->next != NULL)
	{
		p_node->next->prev = p_node->prev;
	}
	
	free(p_node);
	
	return 0;
}

int hash_free_hash(struct hash *p_hash)
{
	size_t	i = 0;
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
	
	return 0;
}
