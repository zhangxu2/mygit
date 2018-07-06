#include <stdio.h>
#include "pub_mem.h"
#include "pub_queue.h"


#define Max_Num 6

typedef struct my_point_s  my_point_t;

struct my_point_s
{
	int x;
	int y;
};

typedef struct my_point_queue_s  my_point_queue_t;

struct my_point_queue_s
{
	my_point_t point;
	sw_queue_t queue;
};


void dump_queue_from_head(sw_queue_t *que)
{
	sw_queue_t *q = pub_queue_head(que);

	printf("(0x%x: (0x%x, 0x%x)) <==> \n", que, que->prev, que->next);

	for (; q != pub_queue_sentinel(que); q = pub_queue_next(q))
	{
		my_point_queue_t *point = pub_queue_data(q, my_point_queue_t, queue);
		printf("(0x%x: (%-2d, %-2d), 0x%x: (0x%x, 0x%x)) <==> \n", point, point->point.x,  
			point->point.y, &point->queue, point->queue.prev, point->queue.next);
	}
}

void dump_queue_from_tail(sw_queue_t *que)
{
	sw_queue_t *q = pub_queue_last(que);

	printf("(0x%x: (0x%x, 0x%x)) <==> \n", que, que->prev, que->next);

	for (; q != pub_queue_sentinel(que); q = pub_queue_prev(q))
	{
		my_point_queue_t *point = pub_queue_data(q, my_point_queue_t, queue);
		printf("(0x%x: (%-2d, %-2d), 0x%x: (0x%x, 0x%x)) <==> \n", point, point->point.x,  
			point->point.y, &point->queue, point->queue.prev, point->queue.next);
	}
}

/** sort from small to big **/
sw_int_t my_point_cmp(const sw_queue_t* lhs, const sw_queue_t* rhs)
{
	my_point_queue_t *pt1 = pub_queue_data(lhs, my_point_queue_t, queue);
	my_point_queue_t *pt2 = pub_queue_data(rhs, my_point_queue_t, queue);

	if (pt1->point.x < pt2->point.x)
	{
		return 0;
	}
	else if (pt1->point.x > pt2->point.x)
	{
		return 1;
	}
	else if (pt1->point.y < pt2->point.y)
	{
		return 0;
	}
	else if (pt1->point.y > pt2->point.y)
	{
		return 1;
	}
	return 1;
}



int main()
{
	sw_pool_t *pool = NULL;
	sw_queue_t *myque = NULL;
	my_point_queue_t *point;
	my_point_t points[Max_Num] = {
		{10, 1}, {20, 9}, {5, 9}, {90, 80}, {5, 3}, {50, 20}
	};
	int i;

	pool = pub_pool_create(1024);


	myque = pub_pool_palloc(pool, sizeof(sw_queue_t));
	pub_queue_init(myque);
   
	
	for (i = 0; i < Max_Num; i++)
	{
		point = (my_point_queue_t*) pub_pool_palloc(pool, sizeof(my_point_queue_t));
		point->point.x = points[i].x;
		point->point.y = points[i].y;
		pub_queue_init(&point->queue);
   
		pub_queue_insert_head(myque, &point->queue);
	}

	dump_queue_from_tail(myque);
	printf("\n");
	
	printf("--------------------------------\n");
	printf("sort the queue:\n");
	printf("--------------------------------\n");
	pub_queue_sort(myque, my_point_cmp);
	dump_queue_from_head(myque);
	printf("\n");
 
	pub_pool_destroy(pool);
	return 0;
}
