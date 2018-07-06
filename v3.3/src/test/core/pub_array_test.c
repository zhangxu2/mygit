#include <stdio.h>
#include "pub_array.h"


void dump_pool(sw_pool_t* pool)  
{  
	int i = sizeof(sw_pool_t);
	while (pool)  
	{  
		printf("pool = 0x%x\n", pool);  
		printf("  .d\n");  
		printf("    .last = 0x%x\n", pool->d.last);  
		printf("    .end = 0x%x\n", pool->d.end);  
		printf("    .next = 0x%x\n", pool->d.next);  
		printf("    .failed = %d\n", pool->d.failed);  
		printf("  .max = %d\n", pool->max);  
		printf("  .current = 0x%x\n", pool->current);   
		printf("  .large = 0x%x\n", pool->large);  
		printf("  .cleanup = 0x%x\n", pool->cleanup);   
		printf("the pool used memory = %d\n", i);
		printf("available pool memory = %d\n\n", pool->d.end - pool->d.last);  
		pool = pool->d.next;  
	}  
}

void dump_array(sw_array_t *array)
{
	printf("the array size = %d\n", sizeof(sw_array_t));
	printf("array = 0x%x\n", array);  
        printf("  .elts = 0x%x\n", array->elts);  
        printf("  .nelts = %d\n", array->nelts);  
        printf("  .size = %d\n", array->size);  
        printf("  .nalloc = %d\n", array->nalloc);  
        printf("  .pool = 0x%x\n", array->pool);  
}

int main()
{
	sw_array_t *array;
	sw_array_t *a;
	sw_pool_t *pool;
	sw_uint_t n = 1;
	size_t size = 3;
	sw_int_t iRet;
	
	pool = NULL;
	pool = pub_pool_create(102);
	printf("\n************************pool dump************************\n\n");
	dump_pool(pool);
	printf("\n************************pool dump************************\n");

	a = pub_array_create(pool, 40, sizeof(int));
	printf("\n************************create dump************************\n\n");
	dump_array(a);
	printf("\n************************create dump************************\n");
	if (a == NULL)
	{
		printf("array create err!\n");
		return -1;
	}
	
	pub_array_push_n(a, 40);
	printf("\n************************push dump************************\n\n");
	dump_array(a);
        printf("pool->d.last = 0x%x\n", pool->d.last);
        printf("\n************************push dump************************\n\n");
	pub_array_push_n(a, 9);
	pub_array_destroy(a);
	pub_pool_destroy(pool);
	
	return 0;
}
