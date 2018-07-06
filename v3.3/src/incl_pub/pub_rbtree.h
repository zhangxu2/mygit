#ifndef __PUB_RBTREE_H__
#define __PUB_RBTREE_H__

#include "pub_config.h"
#include "pub_type.h"

#define sw_thread_volatile	volatile

typedef sw_uint_t  sw_rbtree_key_t;
typedef sw_int_t   sw_rbtree_key_int_t;

typedef struct sw_rbtree_node_s  sw_rbtree_node_t;

struct sw_rbtree_node_s 
{
	sw_rbtree_key_t       key;
	sw_rbtree_node_t     *left;
	sw_rbtree_node_t     *right;
	sw_rbtree_node_t     *parent;
	u_char                 color;
	u_char                 data;
};

typedef struct sw_rbtree_s  sw_rbtree_t;

typedef void (*sw_rbtree_insert_pt) (sw_rbtree_node_t *root,
	sw_rbtree_node_t *node, sw_rbtree_node_t *sentinel);

struct sw_rbtree_s 
{
	sw_rbtree_node_t     *root;
	sw_rbtree_node_t     *sentinel;
	sw_rbtree_insert_pt   insert;
};

#define pub_rbtree_init(tree, s, i)                                           \
	pub_rbtree_sentinel_init(s);                                          \
	(tree)->root = s;                                                     \
	(tree)->sentinel = s;                                                 \
	(tree)->insert = i

void pub_rbtree_insert(sw_thread_volatile sw_rbtree_t *tree,
	sw_rbtree_node_t *node);
void pub_rbtree_delete(sw_thread_volatile sw_rbtree_t *tree,
	sw_rbtree_node_t *node);
void pub_rbtree_insert_value(sw_rbtree_node_t *root, sw_rbtree_node_t *node,
	sw_rbtree_node_t *sentinel);
void pub_rbtree_insert_timer_value(sw_rbtree_node_t *root,
	sw_rbtree_node_t *node, sw_rbtree_node_t *sentinel);

#define pub_rbt_red(node)               ((node)->color = 1)
#define pub_rbt_black(node)             ((node)->color = 0)
#define pub_rbt_is_red(node)            ((node)->color)
#define pub_rbt_is_black(node)          (!pub_rbt_is_red(node))
#define pub_rbt_copy_color(n1, n2)      (n1->color = n2->color)

/* a sentinel must be black */
#define pub_rbtree_sentinel_init(node)  pub_rbt_black(node)

static sw_rbtree_node_t *
pub_rbtree_min(sw_rbtree_node_t *node, sw_rbtree_node_t *sentinel)
{
	while (node->left != sentinel) 
	{
		node = node->left;
	}

	return node;
}

#endif /* __PUB_RBTREE_H__ */

