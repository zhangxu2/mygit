#ifndef __PUB_QUEUE_H__
#define __PUB_QUEUE_H__

#include "pub_type.h"

typedef struct sw_queue_s  sw_queue_t;

/** ���еĽڵ㣬Ҳֱ�ӱ�ʾ���С�ע������һ��˫��ѭ������ **/
struct sw_queue_s 
{
	sw_queue_t  *prev;
	sw_queue_t  *next;
};

/** ��ʼ������ q ��ָ���ָ�룬prev �� next �����Լ� **/
#define pub_queue_init(q)                                                     \
	(q)->prev = q;                                                        \
	(q)->next = q

/** ����� h == h ����һ������ô����һ���ն���**/
#define pub_queue_empty(h)                                                    \
	(h == (h)->prev)

/** �� h �������һ�� x **/
#define pub_queue_insert_head(h, x)                                           \
	(x)->next = (h)->next;                                                \
	(x)->next->prev = x;                                                  \
	(x)->prev = h;                                                        \
	(h)->next = x

#define pub_queue_insert_after	pub_queue_insert_head

/** �� h ǰ�����һ�� x **/
#define pub_queue_insert_tail(h, x)                                           \
	(x)->prev = (h)->prev;                                                \
	(x)->prev->next = x;                                                  \
	(x)->next = h;                                                        \
	(h)->prev = x

/** h ��ʾ���У���һ��Ԫ��Ϊ h->next **/
#define pub_queue_head(h)                                                     \
	(h)->next

/** h��ͷ��h����һ������β **/
#define pub_queue_last(h)                                                     \
	(h)->prev

#define pub_queue_sentinel(h)                                                 \
	(h)

/** ���ؽڵ�q����һ�� **/
#define pub_queue_next(q)                                                     \
	(q)->next

/** ���ؽڵ�q��ǰһ�� **/
#define pub_queue_prev(q)                                                     \
	(q)->prev

#if (SW_DEBUG)

/** ɾ��һ���ڵ� **/
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


/** hΪ����ͷ(������ͷָ��)�����ö��д�q�ڵ㽫����(����)
    �ָ�Ϊ��������(����)��q֮��Ľڵ���ɵ��¶��е�ͷ�ڵ�Ϊn **/
#define pub_queue_split(h, q, n)                                              \
	(n)->prev = (h)->prev;                                                \
	(n)->prev->next = n;                                                  \
	(n)->next = q;                                                        \
	(h)->prev = (q)->prev;                                                \
	(h)->prev->next = h;                                                  \
	(q)->prev = n;


/** h��n�ֱ�Ϊ�������е�ָ�룬��ͷ�ڵ�ָ�룬
    �ò�����n����������h����֮�� **/
#define pub_queue_add(h, n)                                                   \
	(h)->prev->next = (n)->next;                                          \
	(n)->next->prev = (h)->prev;                                          \
	(h)->prev = (n)->prev;                                                \
	(h)->prev->next = h;

/** ��ȡ���нڵ����� **/
#define pub_queue_data(q, type, link)                                         \
	(type *) ((u_char *) q - offsetof(type, link))

sw_queue_t *pub_queue_middle(sw_queue_t *queue);
void pub_queue_sort(sw_queue_t *queue,
sw_int_t (*cmp)(const sw_queue_t *, const sw_queue_t *));

#endif /* __PUB_QUEUE_H__ */

