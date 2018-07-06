#ifndef __DFS_PLIST_H__
#define __DFS_PLIST_H__

#include "dfs_pool.h"

#define DFS_PLIST_START_HEAD	1
#define DFS_PLIST_START_TAIL 2

#define EQUAL	0
#define LESS	1
#define GREATER	2


typedef int (*dfs_plist_sort_pt)(void *, void *);
typedef int (*dfs_plist_match_pt)(void *, void *);
typedef int (*dfs_plist_release_pt)(dfs_pool_t *pool, void *);
typedef void *(*dfs_plist_dup_pt)(void *ptr);

typedef struct dfs_plist_node_s
{
	void	*data;
	struct dfs_plist_node_s	*prev;
	struct dfs_plist_node_s	*next;
}dfs_plist_node_t;

typedef struct
{
	dfs_pool_t	*pool;
	unsigned int	size;
	unsigned int	data_size;
	dfs_plist_node_t	*head;
	dfs_plist_node_t	*tail;
	dfs_plist_node_t	**curr;
	dfs_plist_dup_pt	dup;
	dfs_plist_match_pt	match;
	dfs_plist_release_pt release;
	dfs_plist_sort_pt sort;
}dfs_plist_t;

typedef struct
{
	int	direction;
	dfs_plist_node_t	*next;
}dfs_plist_iter_t;


dfs_plist_t *dfs_plist_create(dfs_pool_t *pool, int data_size);
void dfs_plist_release(dfs_plist_t *list);
int dfs_plist_addnode_head(dfs_plist_t *list, void *data);
int dfs_plist_addnode_tail(dfs_plist_t *list, void *data);
int dfs_plist_insert_node(dfs_plist_t *list, dfs_plist_node_t *old_node, void *data, int after);
dfs_plist_iter_t *dfs_plist_get_iter(dfs_plist_t *list, int direction);
void dfs_plist_release_iter(dfs_plist_t *list, dfs_plist_iter_t *iter);
dfs_plist_node_t *dfs_plist_next(dfs_plist_iter_t *iter);
dfs_plist_node_t *dfs_plist_search_key(dfs_plist_t *list, void *key);
void dfs_plist_set_match(dfs_plist_t *list, dfs_plist_match_pt match);
void dfs_plist_set_release(dfs_plist_t *list, dfs_plist_release_pt release);
void *dfs_plist_pop_head(dfs_plist_t *list);
void *dfs_plist_head(dfs_plist_t *list);
void *dfs_plist_tail(dfs_plist_t *list);
void dfs_plist_rewind(dfs_plist_t *list, dfs_plist_iter_t *iter);
void dfs_plist_rewind_tail(dfs_plist_t *list, dfs_plist_iter_t *iter);
int dfs_plist_length(dfs_plist_t *list);
dfs_plist_t *dfs_plist_dup(dfs_plist_t *orig);
dfs_plist_node_t *dfs_plist_tail_search_key(dfs_plist_t *list, void *key);
void dfs_plist_set_sort(dfs_plist_t *list, dfs_plist_sort_pt sort);
int dfs_plist_insert_sort(dfs_plist_t *list, void *data);
int dfs_plist_del_node(dfs_plist_t *list, dfs_plist_node_t *node);

#endif

