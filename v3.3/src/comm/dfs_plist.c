#include "dfs_plist.h"
#include "dfs_pool.h"
#include "pub_log.h"

dfs_plist_t *dfs_plist_create(dfs_pool_t *pool, int data_size)
{
	dfs_plist_t	*list = NULL;
	
	list = dfs_palloc(pool, sizeof(dfs_plist_t));
	if (list)
	{
		list->size = 0;
		list->data_size = data_size;
		list->head = NULL;
		list->tail = NULL;
		list->dup = NULL;
		list->match = NULL;
		list->release = NULL;
		list->sort = NULL;
		list->curr = &(list->head);
		list->pool = pool;
	}
	
	return list;
}

void dfs_plist_set_match(dfs_plist_t *list, dfs_plist_match_pt match)
{
	if (list)
	{
		list->match = match;
	}
	
	return ;
}

void dfs_plist_set_release(dfs_plist_t *list, dfs_plist_release_pt release)
{
	if (list)
	{
		list->release = release;
	}
	
	return ;
}

void dfs_plist_release(dfs_plist_t *list)
{
	unsigned int	size = 0;
	dfs_plist_node_t	*current = NULL, *next = NULL;
	
	size = list->size;
	current = list->head;
	while (size--)
	{
		next = current->next;
		if (current->data != NULL)
		{
			if (list->release != NULL)
			{
				list->release(list->pool, current->data);
			}
			if (current->data != NULL)
			{
				dfs_pfree(list->pool, current->data);
			}
		}
		dfs_pfree(list->pool, current);
		current = next;
	}
	dfs_pfree(list->pool, list);
	
	return ;
}

int dfs_plist_addnode_head(dfs_plist_t *list, void *data)
{
	dfs_plist_node_t	*node = NULL;
	
	node = dfs_palloc(list->pool, sizeof(dfs_plist_node_t));
	if (node == NULL)
	{
		return -1;
	}
	
	node->data = dfs_palloc(list->pool, list->data_size);
	if (node->data == NULL)
	{
		dfs_pfree(list->pool, node);
		return -1;
	}
	memcpy(node->data, data, list->data_size);

	if (list->size == 0)
	{
		node->prev = node->next = NULL;
		list->head = list->tail = node;
	}
	else
	{
		node->prev = NULL;
		node->next = list->head;
		list->head->prev = node;
		list->head = node;
	}
	list->size++;
	
	return 0;
}

int dfs_plist_addnode_tail(dfs_plist_t *list, void *data)
{
	dfs_plist_node_t	*node = NULL;
	
	node = dfs_palloc(list->pool, sizeof(dfs_plist_node_t));
	if (node == NULL)
	{
		return -1;
	}
	
	node->data = dfs_palloc(list->pool, list->data_size);
	if (node->data == NULL)
	{
		dfs_pfree(list->pool, node);
		return -1;
	}
	memcpy(node->data, data, list->data_size);

	if (list->size == 0)
	{
		node->prev = node->next = NULL;
		list->head = list->tail = node;
	}
	else
	{
		node->next = NULL;
		node->prev = list->tail;
		list->tail->next = node;
		list->tail = node;
	}
	list->size++;
	
	return 0;
}

int dfs_plist_insert_node(dfs_plist_t *list, dfs_plist_node_t *old_node, void *data, int after)
{
	dfs_plist_node_t	*node = NULL;
	
	node = dfs_palloc(list->pool, sizeof(dfs_plist_node_t));
	if (node == NULL)
	{
		return -1;
	}
	
	node->data = dfs_palloc(list->pool, list->data_size);
	if (node->data == NULL)
	{
		dfs_pfree(list->pool, node);
		return -1;
	}
	memcpy(node->data, data, list->data_size);

	if (after)
	{
		node->prev = old_node;
		node->next = old_node->next;
		if (list->tail == old_node)
		{
			list->tail = node;
		}
	}
	else
	{
		node->next = old_node;
		node->prev = old_node->prev;
		if (list->head == old_node)
		{
			list->head = node;
		}
	}
	
	if (node->prev != NULL)
	{
		node->prev->next = node;
	}
	
	if (node->next != NULL)
	{
		node->next->prev = node;
	}
	
	list->size++;
	
	return 0;
}

int dfs_plist_del_node(dfs_plist_t *list, dfs_plist_node_t *node)
{
	if (list == NULL || node == NULL)
	{
		return -1;
	}
	
	if (node->prev != NULL)
	{
		node->prev->next = node->next;
	}
	else
	{
		list->head = node->next;
	}
	
	if (node->next != NULL)
	{
		node->next->prev = node->prev;
	}
	else
	{
		list->tail = node->prev;
	}
	
	if (node->data != NULL)
	{
		dfs_pfree(list->pool, node->data);
		node->data = NULL;
	}
	dfs_pfree(list->pool, node);
	list->size--;

	return 0;
}

dfs_plist_iter_t *dfs_plist_get_iter(dfs_plist_t *list, int direction)
{
	dfs_plist_iter_t	*iter = NULL;
	
	iter = dfs_palloc(list->pool, sizeof(dfs_plist_iter_t));
	if (iter == NULL)
	{
		return NULL;
	}
	
	if (direction == DFS_PLIST_START_HEAD)
	{
		iter->next = list->head;
	}
	else
	{
		iter->next = list->tail;
	}
	iter->direction = direction;
	
	return iter;
}

void dfs_plist_release_iter(dfs_plist_t *list, dfs_plist_iter_t *iter)
{
	if (iter != NULL)
	{
		dfs_pfree(list->pool, iter);
	}
	
	return ;
}

void dfs_plist_rewind(dfs_plist_t *list, dfs_plist_iter_t *iter)
{
	iter->next = list->head;
	iter->direction = DFS_PLIST_START_HEAD;
	
	return ;
}

void dfs_plist_rewind_tail(dfs_plist_t *list, dfs_plist_iter_t *iter)
{
	iter->next = list->tail;
	iter->direction = DFS_PLIST_START_TAIL;
	
	return ;
}

dfs_plist_node_t *dfs_plist_next(dfs_plist_iter_t *iter)
{
	dfs_plist_node_t	*current = iter->next;
	
	if (current != NULL)
	{	
		if (iter->direction == DFS_PLIST_START_HEAD)
		{
			iter->next = current->next;
		}
		else
		{
			iter->next = current->prev;
		}
	}
	return current;
}

dfs_plist_node_t *dfs_plist_search_key(dfs_plist_t *list, void *key)
{
	dfs_plist_iter_t	*iter = NULL;
	dfs_plist_node_t	*node = NULL;
	
	iter = dfs_plist_get_iter(list, DFS_PLIST_START_HEAD);
	while ((node = dfs_plist_next(iter)) != NULL)
	{
		if (list->match != NULL)
		{
			if (list->match(node->data, key))
			{
				dfs_plist_release_iter(list, iter);
				return node;
			}
		}
		else
		{
			if (node->data == key)
			{
				dfs_plist_release_iter(list, iter);
				return node;
			}
		}
	}
	dfs_plist_release_iter(list, iter);
	
	return NULL;
}

dfs_plist_node_t *dfs_plist_index(dfs_plist_t *list, int index)
{
	dfs_plist_node_t	*node = NULL;
	
	if (index < 0)
	{
		index = (-index) - 1;
		node = list->tail;
		while (index--)
		{
			node = node->prev;
		}
	}
	else
	{
		node = list->head;
		while (index--)
		{
			node = node->next;
		}
	}
	
	return node;
}

void *dfs_plist_head(dfs_plist_t *list)
{
	return (void *)(list->head);	
}

void *dfs_plist_tail(dfs_plist_t *list)
{
	return (void *)(list->tail);
}

void *dfs_plist_pop_head(dfs_plist_t *list)
{
	void    *ptr = NULL;
	dfs_plist_node_t     *node = NULL;

	if (list == NULL || list->head == NULL)
	{
		return NULL;
	}

	node = list->head;
	if (node->prev != NULL)
	{
		node->prev->next = node->next;
	}
	else
	{
		list->head = node->next;
	}

	if (node->next != NULL)
	{
		node->next->prev = node->prev;
	}
	else
	{
		list->tail = node->prev;
	}
	list->head = node->next;                                                                                                   
	list->size--;                                                                                                              
	ptr = node->data;

	dfs_pfree(list->pool, node);
	
	return ptr;
}

int dfs_plist_length(dfs_plist_t *list)
{
	if (list == NULL)
	{
		return -1;
	}
	
	return list->size;
}

dfs_plist_t *dfs_plist_dup(dfs_plist_t *orig)
{
	dfs_plist_t	*copy = NULL;
	dfs_plist_iter_t	*iter = NULL;
	dfs_plist_node_t	*node = NULL;
	
	if (orig == NULL || orig->pool == NULL)
	{
		return NULL;
	}

	copy = dfs_plist_create(orig->pool, orig->data_size);
	if (copy == NULL)
	{
		return NULL;
	}
	
	iter = dfs_plist_get_iter(orig, DFS_PLIST_START_HEAD);
	while ((node = dfs_plist_next(iter)) != NULL)
	{
		void	*data = NULL;
		if (copy->dup)
		{
			data = copy->dup(node->data);
			if (data == NULL)
			{
				dfs_plist_release(copy);
				dfs_plist_release_iter(orig, iter);
				return NULL;
			}
		}
		else
		{
			data = node->data;
		}
	
		if (dfs_plist_addnode_tail(copy, data) < 0)
		{
			dfs_plist_release(copy);
			dfs_plist_release_iter(orig, iter);
			return NULL;
		}
	}
	dfs_plist_release_iter(orig, iter);
	
	return copy;
}


dfs_plist_node_t *dfs_plist_tail_search_key(dfs_plist_t *list, void *key)
{
	dfs_plist_iter_t	*iter = NULL;
	dfs_plist_node_t	*node = NULL;
	
	iter = dfs_plist_get_iter(list,DFS_PLIST_START_TAIL);
	
	while ((node = dfs_plist_next(iter)) != NULL)
	{
		if (list->match != NULL)
		{
			if (list->match(node->data, key))
			{
				dfs_plist_release_iter(list, iter);
				return node;
			}
		}
		else
		{
			if (node->data == key)
			{
				dfs_plist_release_iter(list, iter);
				return node;
			}
		}
	}
	dfs_plist_release_iter(list, iter);
	
	return NULL;
}

void dfs_plist_set_sort(dfs_plist_t *list, dfs_plist_sort_pt sort)
{
	if (list)
	{
		pub_log_info("[%s][%d] list set sort....", __FILE__, __LINE__);
		list->sort = sort;
	}
	
	return ;
}

int dfs_plist_insert_sort(dfs_plist_t *list, void *data)
{
	dfs_plist_node_t	*node = NULL;
	dfs_plist_node_t	*p = NULL;
	
	node = dfs_palloc(list->pool, sizeof(dfs_plist_node_t));
	if (node == NULL)
	{
		pub_log_error("[%s][%d]slab alloc error.", __FILE__, __LINE__);
		return -1;
	}
	
	node->data = dfs_palloc(list->pool, list->data_size);
	if (node->data == NULL)
	{
		dfs_pfree(list->pool, node);
		pub_log_error("[%s][%d]slab alloc error.", __FILE__, __LINE__);
		return -1;
	}

	memcpy(node->data, data, list->data_size);
	if (list->size == 0)
	{
		node->prev = node->next = NULL;
		list->head = list->tail = node;
	}
	else
	{
		if (list->tail == NULL || node == NULL)
		{
			pub_log_error("[%s][%d] list->tail[%x]node[%x]", __FILE__, __LINE__, list->tail, node);
			return -1;
		}
		
		p = list->tail;
		if (list->sort(p->data, data) != GREATER)
		{
			node->next = NULL;
			node->prev = list->tail;
			list->tail->next = node;
			list->tail = node;
			list->size++;
			return 0;
		}

		while (p != NULL)
		{
			if (list->sort(p->data, data) == GREATER)
			{
				p = p->prev;
			}
			else
			{
				node->prev = p;
				node->next = p->next;
				if (list->tail == p)
				{
					list->tail = node;
				}
				if (node->prev != NULL)
				{
					node->prev->next = node;
				}
				
				if (node->next != NULL)
				{
					node->next->prev = node;
				}
				break;
			}
		}
		if (p == NULL)
		{
			node->prev = NULL;
			node->next = list->head;
			list->head->prev = node;
			list->head = node;
		}
	}
	list->size++;	

	return 0;
}


