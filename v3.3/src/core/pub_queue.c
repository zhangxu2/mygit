/*********************************************************************
 *** �� �� ��: v2.2
 *** ��������: yangzhao
 *** ��������: 2013-3-26
 *** ����ģ��: 
 *** ��������: pub_queue
 *** ��������: ���л����ṹ
 *** �����б�:
 ***		pub_queue_middle		��ȡ�м�ڵ㣬��������������(��ͷ�ڵ���)�ڵ㣬
 ***					�򷵻��м�Ľڵ㣻��������ż�����ڵ㣬�򷵻�
 ***					�������еĵ�һ���ڵ㡣
 ***		pub_queue_sort		�������򣬴ӵ�һ���ڵ㿪ʼ������
 ***					���ν���ǰ�ڵ�(q)����ǰ���Ѿ��ź���Ķ��С�	
 ***		
 ***			
 *** ʹ��ע��:
 ***	1.���в���ֻ������ָ����м򵥵��޸�ָ����������������
 ***	  �����ݿռ�ķ��䡣��ˣ���ʹ�ö��нṹʱ��Ҫ�Լ�
 ***	  �������ݽṹ������ռ䣬�������а���һ��sw_queue_t��
 ***	  ָ����߶��󣬵���Ҫ��ȡ���нڵ�����ʱ��ʹ��pub_queue_data�ꡣ
 ***
 ***	2.ʹ�ö�������������Ҫ�Լ����������������
 *** �޸ļ�¼:
 *** 	�޸�����:       
 *** 	�޸�ʱ��:       
 *** 	�޸�����:
 ***                    
 ***                    
 ***                    
 ********************************************************************/
#include "pub_queue.h"

/******************************************************************************
 *** ��������: pub_queue_middle
 *** ��������: ��ȡ�м�ڵ�
 *** ��������: yangzhao
 *** ��������: 2013-3-26
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: queue	��Ҫ�����Ķ���
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ����ָ��
 *** ע������: 
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
 *** ��������: pub_queue_middle
 *** ��������: ��ȡ�м�ڵ�
 *** ��������: yangzhao
 *** ��������: 2013-3-26
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: queue	��Ҫ�����Ķ���
 ***	����2��cmp()	�����������������Ҫ�Լ�����
 *** �������: 
 *** 	����1: ��
 *** �� �� ֵ: ��
 *** ע������:
 ***	1.�������������ֵ����1����в������� 
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
