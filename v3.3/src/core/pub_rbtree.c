/*********************************************************************
 *** 版 本 号: v2.2
 *** 程序作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 所属模块: 
 *** 程序名称: pub_rbtree
 *** 程序作用: 红黑树基本操作
 *** 函数列表:
 ***		pub_rbtree_insert		向红黑树中插入节点
 ***		pub_rbtree_insert_value		插入的规则函数	
 ***		pub_rbtree_delete		从红黑树中删除节点	
 ***		pub_rbtree_left_rotate		红黑树左旋转操作
 ***		pub_rbtree_right_rotate		红黑树右旋转操作
 ***		
 ***			
 *** 使用注意:
 ***	该hash结构是只读的，仅在初始创建时可以给出保存在其中的key-val
 ***	对儿，然后就只能进行“增删改查”操作了。
 *** 修改记录:
 *** 	修改作者:       
 *** 	修改时间:       
 *** 	修改内容:
 ***                    
 ***                    
 ***                    
 ********************************************************************/
#include "pub_rbtree.h"

/*
 * The red-black tree code is based on the algorithm described in
 * the "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 */

static void pub_rbtree_left_rotate(sw_rbtree_node_t **root,
	sw_rbtree_node_t *sentinel, sw_rbtree_node_t *node);
static void pub_rbtree_right_rotate(sw_rbtree_node_t **root,
	sw_rbtree_node_t *sentinel, sw_rbtree_node_t *node);


/******************************************************************************
 *** 函数名称: pub_rbtree_insert
 *** 函数功能: 向树中插入元素
 *** 函数作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: tree	向该树进行插入
 ***	参数2：node	插入的节点
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 无
 *** 注意事项: 
 ******************************************************************************/
void
pub_rbtree_insert(sw_thread_volatile sw_rbtree_t *tree,
	sw_rbtree_node_t *node)
{
	sw_rbtree_node_t  **root, *temp, *sentinel;

	/* a binary tree insert */

	root = (sw_rbtree_node_t **) &tree->root;
	sentinel = tree->sentinel;

	if (*root == sentinel) 
	{
		node->parent = NULL;
		node->left = sentinel;
		node->right = sentinel;
		pub_rbt_black(node);
		*root = node;

		return;
	}

	tree->insert(*root, node, sentinel);
	/* re-balance tree */
	while (node != *root && pub_rbt_is_red(node->parent)) 
	{

		if (node->parent == node->parent->parent->left) 
		{
			temp = node->parent->parent->right;

			if (pub_rbt_is_red(temp)) 
			{
				pub_rbt_black(node->parent);
				pub_rbt_black(temp);
				pub_rbt_red(node->parent->parent);
				node = node->parent->parent;

			} 
			else 
			{
				if (node == node->parent->right) 
				{
					node = node->parent;
					pub_rbtree_left_rotate(root, sentinel, node);
				}

				pub_rbt_black(node->parent);
				pub_rbt_red(node->parent->parent);
				pub_rbtree_right_rotate(root, sentinel, node->parent->parent);
			}

		} 
		else 
		{
			temp = node->parent->parent->left;
			
			if (pub_rbt_is_red(temp)) 
			{
  				pub_rbt_black(node->parent);
				pub_rbt_black(temp);
				pub_rbt_red(node->parent->parent);
				node = node->parent->parent;

			} 
			else 
			{
				if (node == node->parent->left) 
				{
					node = node->parent;
					pub_rbtree_right_rotate(root, sentinel, node);
				}

				pub_rbt_black(node->parent);
				pub_rbt_red(node->parent->parent);
				pub_rbtree_left_rotate(root, sentinel, node->parent->parent);
			}
		}
	}

	pub_rbt_black(*root);
}


/******************************************************************************
 *** 函数名称: pub_rbtree_insert_value
 *** 函数功能: 向树中插入元素的规则函数
 *** 函数作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: temp	插入节点的父节点
 ***	参数2：node	插入的节点
 ***	参数3：sentinel	叶子节点
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 无
 *** 注意事项: 
 ******************************************************************************/
void
pub_rbtree_insert_value(sw_rbtree_node_t *temp, sw_rbtree_node_t *node,
	sw_rbtree_node_t *sentinel)
{
	sw_rbtree_node_t  **p;

	for ( ;; ) 
	{

		p = (node->key < temp->key) ? &temp->left : &temp->right;

		if (*p == sentinel) 
		{
			break;
		}

		temp = *p;
	}

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	pub_rbt_red(node);
}

void
pub_rbtree_insert_timer_value(sw_rbtree_node_t *temp, sw_rbtree_node_t *node,
	sw_rbtree_node_t *sentinel)
{
	sw_rbtree_node_t  **p;

	for ( ;; ) 
	{

		/*
		* Timer values
		* 1) are spread in small range, usually several minutes,
		* 2) and overflow each 49 days, if milliseconds are stored in 32 bits.
		* The comparison takes into account that overflow.
		*/

		/*  node->key < temp->key */

		p = ((sw_rbtree_key_int_t) (node->key - temp->key) < 0)
 		? &temp->left : &temp->right;

		if (*p == sentinel) 
		{
			break;
		}

		temp = *p;
	}

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	pub_rbt_red(node);
}

/******************************************************************************
 *** 函数名称: pub_rbtree_delete
 *** 函数功能: 从树中删除节点
 *** 函数作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: tree	对该树进行删除操作
 ***	参数2：node	删除的节点
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 无
 *** 注意事项: 
 ******************************************************************************/
void
pub_rbtree_delete(sw_thread_volatile sw_rbtree_t *tree,
	sw_rbtree_node_t *node)
{
	sw_uint_t           red;
	sw_rbtree_node_t  **root, *sentinel, *subst, *temp, *w;

	/* a binary tree delete */

	root = (sw_rbtree_node_t **) &tree->root;
	sentinel = tree->sentinel;

	if (node->left == sentinel) 
	{
		temp = node->right;
		subst = node;

	} 
	else if (node->right == sentinel) 
	{
		temp = node->left;
		subst = node;

	} 
	else 
	{
		subst = pub_rbtree_min(node->right, sentinel);
	
		if (subst->left != sentinel) 
		{
			temp = subst->left;
		} 
		else 
		{
			temp = subst->right;
		}
	}

	if (subst == *root) 
	{
		*root = temp;
		pub_rbt_black(temp);

		/* DEBUG stuff */
		node->left = NULL;
		node->right = NULL;
		node->parent = NULL;
		node->key = 0;

		return;
	}

	red = pub_rbt_is_red(subst);

	if (subst == subst->parent->left) 
	{
		subst->parent->left = temp;

	} 
	else 
	{
		subst->parent->right = temp;
	}

	if (subst == node) 
	{

		temp->parent = subst->parent;

	} 
	else 
	{

		if (subst->parent == node) 
		{
			temp->parent = subst;

		} 
		else 
		{
			temp->parent = subst->parent;
		}
		subst->left = node->left;
		subst->right = node->right;
		subst->parent = node->parent;
		pub_rbt_copy_color(subst, node);

		if (node == *root) 
		{
			*root = subst;

		} 
		else 
		{
			if (node == node->parent->left) 
			{
				node->parent->left = subst;
			} 
			else 
			{
				node->parent->right = subst;
			}
		}

		if (subst->left != sentinel) 
		{
			subst->left->parent = subst;
		}

		if (subst->right != sentinel) 
		{
			subst->right->parent = subst;
		}
	}

	/* DEBUG stuff */
	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;
	node->key = 0;

	if (red) 
	{
		return;
	}

	/* a delete fixup */
	while (temp != *root && pub_rbt_is_black(temp)) 
	{
		if (temp == temp->parent->left) 
		{
			w = temp->parent->right;

			if (pub_rbt_is_red(w)) 
			{
				pub_rbt_black(w);
				pub_rbt_red(temp->parent);
				pub_rbtree_left_rotate(root, sentinel, temp->parent);
				w = temp->parent->right;
 			}

 			if (pub_rbt_is_black(w->left) && pub_rbt_is_black(w->right)) 
			{
                		pub_rbt_red(w);
				temp = temp->parent;

			} 
			else 
			{
				if (pub_rbt_is_black(w->right)) 
				{
					pub_rbt_black(w->left);
					pub_rbt_red(w);
					pub_rbtree_right_rotate(root, sentinel, w);
					w = temp->parent->right;
				}

 				pub_rbt_copy_color(w, temp->parent);
				pub_rbt_black(temp->parent);
				pub_rbt_black(w->right);
				pub_rbtree_left_rotate(root, sentinel, temp->parent);
				temp = *root;
			}

		} 
		else 
		{
			w = temp->parent->left;

			if (pub_rbt_is_red(w)) 
			{
				pub_rbt_black(w);
				pub_rbt_red(temp->parent);
				pub_rbtree_right_rotate(root, sentinel, temp->parent);
				w = temp->parent->left;
			}

			if (pub_rbt_is_black(w->left) && pub_rbt_is_black(w->right)) 
			{
				pub_rbt_red(w);
				temp = temp->parent;

			} 
			else 
			{
				if (pub_rbt_is_black(w->left)) 
				{
 					pub_rbt_black(w->right);
					pub_rbt_red(w);
					pub_rbtree_left_rotate(root, sentinel, w);
					w = temp->parent->left;
				}

				pub_rbt_copy_color(w, temp->parent);
				pub_rbt_black(temp->parent);
				pub_rbt_black(w->left);
				pub_rbtree_right_rotate(root, sentinel, temp->parent);
				temp = *root;
			}
		}
	}

	pub_rbt_black(temp);
}


/******************************************************************************
 *** 函数名称: pub_rbtree_left_rotate
 *** 函数功能: 二叉树的左旋转操作
 *** 函数作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: root	根节点
 ***	参数2：sentinel	叶子节点
 ***	参数3：node	操作的节点
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 无
 *** 注意事项: 
 ******************************************************************************/
static void
pub_rbtree_left_rotate(sw_rbtree_node_t **root, sw_rbtree_node_t *sentinel,
	sw_rbtree_node_t *node)
{
	printf("left rotate\n");
	sw_rbtree_node_t  *temp;

	temp = node->right;
	node->right = temp->left;

	if (temp->left != sentinel) 
	{
		temp->left->parent = node;
	}

	temp->parent = node->parent;

	if (node == *root) 
	{
		*root = temp;

	} 
	else if (node == node->parent->left) 
	{
		node->parent->left = temp;

	} 
	else 
	{
		node->parent->right = temp;
	}

	temp->left = node;
	node->parent = temp;
}

/******************************************************************************
 *** 函数名称: pub_rbtree_right_rotate
 *** 函数功能: 二叉树的右旋转操作
 *** 函数作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: root	根节点
 ***	参数2：sentinel	叶子节点
 ***	参数3：node	操作的节点
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 无
 *** 注意事项: 
 ******************************************************************************/
static void
pub_rbtree_right_rotate(sw_rbtree_node_t **root, sw_rbtree_node_t *sentinel,
	sw_rbtree_node_t *node)
{
	sw_rbtree_node_t  *temp;

	temp = node->left;
	node->left = temp->right;

	if (temp->right != sentinel) 
	{
		temp->right->parent = node;
	}

	temp->parent = node->parent;

	if (node == *root) 
	{
		*root = temp;

	} 
	else if (node == node->parent->right) 
	{
		node->parent->right = temp;

	} 
	else 
	{
		node->parent->left = temp;
	}

	temp->right = node;
	node->parent = temp;
}
