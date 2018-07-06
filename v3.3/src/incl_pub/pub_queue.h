#ifndef __PUB_QUEUE_H__
#define __PUB_QUEUE_H__

#include "pub_type.h"

typedef struct sw_queue_s  sw_queue_t;

/** 队列的节点，也直接表示队列。注意这是一个双向循环队列 **/
struct sw_queue_s 
{
	sw_queue_t  *prev;
	sw_queue_t  *next;
};

/** 初始化队列 q 所指向的指针，prev 和 next 都是自己 **/
#define pub_queue_init(q)                                                     \
	(q)->prev = q;                                                        \
	(q)->next = q

/** 如果表 h == h 的上一个，那么就是一个空队列**/
#define pub_queue_empty(h)                                                    \
	(h == (h)->prev)

/** 向 h 后面插入一个 x **/
#define pub_queue_insert_head(h, x)                                           \
	(x)->next = (h)->next;                                                \
	(x)->next->prev = x;                                                  \
	(x)->prev = h;                                                        \
	(h)->next = x

#define pub_queue_insert_after	pub_queue_insert_head

/** 向 h 前面插入一个 x **/
#define pub_queue_insert_tail(h, x)                                           \
	(x)->prev = (h)->prev;                                                \
	(x)->prev->next = x;                                                  \
	(x)->next = h;                                                        \
	(h)->prev = x

/** h 表示队列，第一个元素为 h->next **/
#define pub_queue_head(h)                                                     \
	(h)->next

/** h是头，h的上一个就是尾 **/
#define pub_queue_last(h)                                                     \
	(h)->prev

#define pub_queue_sentinel(h)                                                 \
	(h)

/** 返回节点q的下一个 **/
#define pub_queue_next(q)                                                     \
	(q)->next

/** 返回节点q的前一个 **/
#define pub_queue_prev(q)                                                     \
	(q)->prev

#if (SW_DEBUG)

/** 删除一个节点 **/
#define pub_queue_remove(x)                                                   \
	(x)->next->prev = (x)->prev;                                          \
	(x)->prev->next = (x)->next;                                          \
	(x)->prev = NULL;                                                     \
	(x)->next = NULL

#else

#define pub_queue_remove(x)                                                   \
	(x)->next->prev = (x)->prev;                                          \
	(x)->prev->next = (x)->next

#endif


/** h为队列头(即链表头指针)，将该队列从q节点将队列(链表)
    分割为两个队列(链表)，q之后的节点组成的新队列的头节点为n **/
#define pub_queue_split(h, q, n)                                              \
	(n)->prev = (h)->prev;                                                \
	(n)->prev->next = n;                                                  \
	(n)->next = q;                                                        \
	(h)->prev = (q)->prev;                                                \
	(h)->prev->next = h;                                                  \
	(q)->prev = n;


/** h、n分别为两个队列的指针，即头节点指针，
    该操作将n队列链接在h队列之后 **/
#define pub_queue_add(h, n)                                                   \
	(h)->prev->next = (n)->next;                                          \
	(n)->next->prev = (h)->prev;                                          \
	(h)->prev = (n)->prev;                                                \
	(h)->prev->next = h;

/** 获取队列节点数据 **/
#define pub_queue_data(q, type, link)                                         \
	(type *) ((u_char *) q - offsetof(type, link))

sw_queue_t *pub_queue_middle(sw_queue_t *queue);
void pub_queue_sort(sw_queue_t *queue,
sw_int_t (*cmp)(const sw_queue_t *, const sw_queue_t *));

#endif /* __PUB_QUEUE_H__ */

