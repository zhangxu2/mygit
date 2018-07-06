/*********************************************************************
 *** 版 本 号: v2.2
 *** 程序作者: yangzhao
 *** 生成日期: 2013-3-26
 *** 所属模块: 
 *** 程序名称: pub_queue
 *** 程序作用: 队列基本结构
 *** 函数列表:
 ***		pub_queue_middle		获取中间节点，若队列有奇数个(除头节点外)节点，
 ***					则返回中间的节点；若队列有偶数个节点，则返回
 ***					后半个队列的第一个节点。
 ***		pub_queue_sort		队列排序，从第一个节点开始遍历，
 ***					依次将当前节点(q)插入前面已经排好序的队列。	
 ***		
 ***			
 *** 使用注意:
 ***	1.队列操作只对链表指针进行简单的修改指向操作，并不负责节
 ***	  点数据空间的分配。因此，在使用队列结构时，要自己
 ***	  定义数据结构并分配空间，且在其中包含一个sw_queue_t的
 ***	  指针或者对象，当需要获取队列节点数据时，使用pub_queue_data宏。
 ***
 ***	2.使用队列排序函数，需要自己定义排序规则函数。
 *** 修改记录:
 *** 	修改作者:       
 *** 	修改时间:       
 *** 	修改内容:
 ***                    
 ***                    
 ***                    
 ********************************************************************/
#include "pub_queue.h"

/******************************************************************************
 *** 函数名称: pub_queue_middle
 *** 函数功能: 获取中间节点
 *** 函数作者: yangzhao
 *** 生成日期: 2013-3-26
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: queue	需要操作的队列
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 队列指针
 *** 注意事项: 
 ******************************************************************************/
sw_queue_t *
pub_queue_middle(sw_queue_t *queue)
{
	sw_queue_t  *middle, *next;

	middle = pub_queue_head(queue);

	if (middle == pub_queue_last(queue)) 
	{
		return middle;
	}

	next = pub_queue_head(queue);

	for ( ;; ) 
	{
		middle = pub_queue_next(middle);

		next = pub_queue_next(next);

		if (next == pub_queue_last(queue)) 
		{
			return middle;
		}

		next = pub_queue_next(next);

		if (next == pub_queue_last(queue)) 
		{
			return middle;
		}
	}
}


/******************************************************************************
 *** 函数名称: pub_queue_middle
 *** 函数功能: 获取中间节点
 *** 函数作者: yangzhao
 *** 生成日期: 2013-3-26
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: queue	需要操作的队列
 ***	参数2：cmp()	排序规则函数，根据需要自己定义
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 无
 *** 注意事项:
 ***	1.排序函数如果返回值大于1则进行插入排序 
 ******************************************************************************/
void
pub_queue_sort(sw_queue_t *queue,
	sw_int_t (*cmp)(const sw_queue_t *, const sw_queue_t *))
{
	sw_queue_t  *q, *prev, *next;

	q = pub_queue_head(queue);

	if (q == pub_queue_last(queue)) 
	{
		return;
	}

	for (q = pub_queue_next(q); q != pub_queue_sentinel(queue); q = next) 
	{

		prev = pub_queue_prev(q);
		next = pub_queue_next(q);

		pub_queue_remove(q);

		do 
		{
			if (cmp(prev, q) <= 0) 
			{
				break;
			}

			prev = pub_queue_prev(prev);

		} while (prev != pub_queue_sentinel(queue));

		pub_queue_insert_after(prev, q);
	}
}
