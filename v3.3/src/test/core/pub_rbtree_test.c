/*********************************************************************
 *** 版 本 号: v2.2
 *** 程序作者: yangzhao
 *** 生成日期: 2013-4-3
 *** 所属模块: 
 *** 程序名称: pub_rbtree_test
 *** 程序作用: 红黑树测试程序
 *** 修改记录:
 *** 	修改作者:       
 *** 	修改时间:       
 *** 	修改内容:
 ***                    
 ***                    
 ***                    
 ********************************************************************/
#include "pub_rbtree.h"


int main()
{
	sw_rbtree_t *tree;
	sw_rbtree_node_t *root;
	sw_rbtree_node_t *node;
	sw_rbtree_node_t *node2;
	sw_rbtree_node_t *node3;
	sw_rbtree_node_t *node4;
	sw_rbtree_node_t *node5;

	tree = (sw_rbtree_t *) malloc(sizeof(sw_rbtree_t));
	root = (sw_rbtree_node_t *) malloc(sizeof(sw_rbtree_node_t));
	node = (sw_rbtree_node_t *) malloc(sizeof(sw_rbtree_node_t));
	node2 = (sw_rbtree_node_t *) malloc(sizeof(sw_rbtree_node_t));
	node3 = (sw_rbtree_node_t *) malloc(sizeof(sw_rbtree_node_t));
	node4 = (sw_rbtree_node_t *) malloc(sizeof(sw_rbtree_node_t));
	node5 = (sw_rbtree_node_t *) malloc(sizeof(sw_rbtree_node_t));
	
	node->key = 100;
	node2->key = 50;
	node3->key = 150;
	node4->key = 25;
	node5->key = 75;
	
	printf("***************init*****************\n");
	pub_rbtree_init(tree, root, &pub_rbtree_insert_value);
	printf("tree = 0x%x\n", tree);
	printf("  .root = 0x%x\n", tree->root);
	printf("  .sentinel = 0x%x\n", tree->sentinel);
	printf("node = 0x%x\n", node);
	printf("  .key = %d\n", node->key);
	printf("  .left = 0x%x\n", node->left);
	printf("  .right = 0x%x\n", node->right);
	printf("  .parent = 0x%x\n\n", node->parent);
	
	printf("tree->sentinel->key = %d\n", tree->sentinel->key);
	pub_rbtree_insert(tree, node);
	pub_rbtree_insert(tree, node2);
	pub_rbtree_insert(tree, node3);
	pub_rbtree_insert(tree, node4);
	pub_rbtree_insert(tree, node5);

	printf("\n***************insert*****************\n");
	printf("the tree root key is %d\n", tree->root->key);
	printf("the root left key is %d\n", tree->root->left->key);
	printf("the root right key is %d\n", tree->root->right->key);
	printf("the node color is %d(1:red 0:black)\n", node->color);
	
	printf("the node2 color is %d(1:red 0:black)\n", node2->color);
	printf("the node2 parent color is %d(1:red 0:black)\n", node2->parent->color);
	printf("the node2 parent key is %d\n", node2->parent->key);
	printf("the node2 left key is %d\n", node3->left->key);
	printf("the node2 right key is %d\n", node3->right->key);
	
	printf("the node3 color is %d(1:red 0:black)\n", node3->color);
	printf("the node3 parent color is %d(1:red 0:black)\n", node3->parent->color);
	printf("the node3 parent key is %d\n", node3->parent->key);
	printf("the node3 left key is %d\n", node3->left->key);
	printf("the node3 right key is %d\n", node3->right->key);
	
	printf("the node4 color is %d(1:red 0:black)\n", node4->color);
	printf("the node4 parent color is %d(1:red 0:black)\n", node4->parent->color);
	printf("the node4 parent key is %d\n", node4->parent->key);
	
	printf("the node5 color is %d(1:red 0:black)\n", node5->color);
	printf("the node5 parent color is %d(1:red 0:black)\n", node5->parent->color);
	printf("the node5 parent key is %d\n", node5->parent->key);

	
	pub_rbtree_delete(tree, node2);
	printf("\n***************delete*****************\n");
	printf("the tree root key = [%d]\n", tree->root->key);
	printf("the tree root left key = [%d]\n", tree->root->left->key);
	printf("the tree root right key = [%d]\n", tree->root->right->key);
	printf("the tree root left left key = [%d]\n", tree->root->left->left->key);
	printf("the tree root left right key = [%d]\n", tree->root->left->right->key);
	printf("the tree root right left key = [%d]\n", tree->root->right->left->key);
	printf("the tree root right right key = [%d]\n", tree->root->right->right->key);
	
	free(tree);
	free(root);
	free(node);
	free(node2);
	free(node3);
	free(node4);
	free(node5);
	return 0;
}
