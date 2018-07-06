#ifndef _PUB_HASH_H
#define _PUB_HASH_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef unsigned int (*hashfunc_t)(unsigned int, void*);

struct hash_node
{
	void	*p_key;
	void	*p_value;
	struct hash_node	*prev;
	struct hash_node	*next;
};

struct hash
{
	unsigned int buckets;
	unsigned int key_size;
	unsigned int value_size;
	hashfunc_t	hash_func;
	struct hash_node	**p_nodes;
};

typedef struct hash hash_t;

struct hash *hash_alloc(unsigned int buckets, unsigned int key_size, unsigned int value_size, hashfunc_t hash_func);
void *hash_lookup_entry(struct hash *p_hash, void *p_key);
int hash_add_entry(struct hash *p_hash, void *p_key, void *p_value);
int hash_free_entry(struct hash *p_hash, void *p_key);
unsigned int hash_key_info(unsigned int buckets, void *p_key);
int hash_free_hash(struct hash *p_hash);

#endif
