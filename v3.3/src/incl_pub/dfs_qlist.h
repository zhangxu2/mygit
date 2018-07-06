#ifndef __DFS_QLIST_H__
#define __DFS_QLIST_H__

#include "dfs.h"
#include "dfs_slab.h"

#define LIST_START_HEAD	1
#define LIST_START_TAIL 2

#define EQUAL	0
#define LESS	1
#define GREATER	2


typedef int (*dfs_qlist_sort_pt)(void *, void *);
typedef int (*dfs_qlist_match_pt)(void *, void *);
typedef int (*dfs_qlist_release_pt)(dfs_slab_pool_t *pool, void *);
typedef struct dfs_qlist_node_s
{
	void	*data;
	struct dfs_qlist_node_s	*prev;
	struct dfs_qlist_node_s	*next;
}dfs_qlist_node_t;

typedef struct
{
	dfs_slab_pool_t	*pool;
	unsigned int	size;
	unsigned int	data_size;
	dfs_qlist_node_t	*head;
	dfs_qlist_node_t	*tail;
	dfs_qlist_node_t	**curr;
	dfs_qlist_match_pt	match;
	dfs_qlist_release_pt release;
	dfs_qlist_sort_pt	sort;
}dfs_qlist_t;

typedef struct
{
	int	direction;
	dfs_qlist_node_t	*next;
}dfs_qlist_iter_t;


dfs_qlist_t	*g_queue_data;
dfs_qlist_t	*g_cache_queue;
dfs_qlist_t *dfs_qlist_create(dfs_slab_pool_t *pool, int data_size);
void dfs_qlist_release(dfs_qlist_t *list);
int dfs_qlist_addnode_head(dfs_qlist_t *list, void *data);
int dfs_qlist_addnode_tail(dfs_qlist_t *list, void *data);
int dfs_qlist_insert_node(dfs_qlist_t *list, dfs_qlist_node_t *old_node, void *data, int after);
dfs_qlist_iter_t *dfs_qlist_get_iter(dfs_qlist_t *list, int direction);
void dfs_qlist_release_iter(dfs_qlist_t *list, dfs_qlist_iter_t *iter);
dfs_qlist_node_t *dfs_qlist_next(dfs_qlist_iter_t *iter);
dfs_qlist_node_t *dfs_qlist_search_key(dfs_qlist_t *list, void *key);
dfs_qlist_node_t *dfs_qlist_tail_search_key(dfs_qlist_t *list, void *key);
void dfs_qlist_set_match(dfs_qlist_t *list, dfs_qlist_match_pt match);
void dfs_qlist_set_release(dfs_qlist_t *list, dfs_qlist_release_pt release);
void *dfs_qlist_pop_head(dfs_qlist_t *list);
void *dfs_qlist_head(dfs_qlist_t *list);
void *dfs_qlist_tail(dfs_qlist_t *list);
void dfs_qlist_rewind(dfs_qlist_t *list, dfs_qlist_iter_t *iter);
void dfs_qlist_rewind_tail(dfs_qlist_t *list, dfs_qlist_iter_t *iter);
int dfs_qlist_length(dfs_qlist_t *list);
int dfs_qlist_del_node(dfs_qlist_t *list, dfs_qlist_node_t *node);
int dfs_qlist_insert_sort(dfs_qlist_t *list, void *data);
void dfs_qlist_set_sort(dfs_qlist_t *list, dfs_qlist_sort_pt sort);

#endif /*** __dfs_qlist_H__ ***/

