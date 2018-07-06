/*********************************************************************
 *** �� �� ��: v2.2
 *** ��������: yangzhao
 *** ��������: 2013-4-3
 *** ����ģ��: 
 *** ��������: pub_rbtree
 *** ��������: �������������
 *** �����б�:
 ***		pub_rbtree_insert		�������в���ڵ�
 ***		pub_rbtree_insert_value		����Ĺ�����	
 ***		pub_rbtree_delete		�Ӻ������ɾ���ڵ�	
 ***		pub_rbtree_left_rotate		���������ת����
 ***		pub_rbtree_right_rotate		���������ת����
 ***		
 ***			
 *** ʹ��ע��:
 ***	��hash�ṹ��ֻ���ģ����ڳ�ʼ����ʱ���Ը������������е�key-val
 ***	�Զ���Ȼ���ֻ�ܽ��С���ɾ�Ĳ顱�����ˡ�
 *** �޸ļ�¼:
 *** 	�޸�����:       
 *** 	�޸�ʱ��:       
 *** 	�޸�����:
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
 *** ��������: pub_rbtree_insert
 *** ��������: �����в���Ԫ��
 *** ��������: yangzhao
 *** ��������: 2013-4-3
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: tree	��������в���
 ***	����2��node	����Ľڵ�
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ��
 *** ע������: 
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
 *** ��������: pub_rbtree_insert_value
 *** ��������: �����в���Ԫ�صĹ�����
 *** ��������: yangzhao
 *** ��������: 2013-4-3
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: temp	����ڵ�ĸ��ڵ�
 ***	����2��node	����Ľڵ�
 ***	����3��sentinel	Ҷ�ӽڵ�
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ��
 *** ע������: 
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
 *** ��������: pub_rbtree_delete
 *** ��������: ������ɾ���ڵ�
 *** ��������: yangzhao
 *** ��������: 2013-4-3
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: tree	�Ը�������ɾ������
 ***	����2��node	ɾ���Ľڵ�
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ��
 *** ע������: 
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
 *** ��������: pub_rbtree_left_rotate
 *** ��������: ������������ת����
 *** ��������: yangzhao
 *** ��������: 2013-4-3
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: root	���ڵ�
 ***	����2��sentinel	Ҷ�ӽڵ�
 ***	����3��node	�����Ľڵ�
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ��
 *** ע������: 
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
 *** ��������: pub_rbtree_right_rotate
 *** ��������: ������������ת����
 *** ��������: yangzhao
 *** ��������: 2013-4-3
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: root	���ڵ�
 ***	����2��sentinel	Ҷ�ӽڵ�
 ***	����3��node	�����Ľڵ�
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ��
 *** ע������: 
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
