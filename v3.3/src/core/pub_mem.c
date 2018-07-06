#include "pub_mem.h"
#include <malloc.h>

sw_uint_t  pub_pagesize= 1024;

void *pub_mem_alloc(size_t size)
{
    return malloc(size);
}


void *pub_mem_calloc(size_t size)
{
	void  *p;
	
	p = pub_mem_alloc(size);
	
	if (p) 
	{
		pub_mem_memzero(p, size);
	}
	
	return p;
}


#if (SW_HAVE_POSIX_MEMALIGN)

void *pub_mem_memalign(size_t alignment, size_t size)
{
	void  *p;
	int    err;
	
	err = posix_memalign(&p, alignment, size);
	
	if (err) 
	{
		p = NULL;
	}
	return p;
}

#elif (SW_HAVE_MEMALIGN)

void *pub_mem_memalign(size_t alignment, size_t size)
{
	void  *p;
	
	p = memalign(alignment,size);
	if (p == NULL) 
	{
	}
	return p;
}

#endif


