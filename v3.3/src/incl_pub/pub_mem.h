#ifndef __PUB_MEM_H__
#define __PUB_MEM_H__

#include "pub_type.h"
/**** need define
SW_ALIGNMENT
SW_MEMCPY_LIMIT
SW_HAVE_POSIX_MEMALIGN
SW_HAVE_MEMALIGN
***/
#if defined(HPUX)
#define SW_HAVE_POSIX_MEMALIGN  0
#define SW_HAVE_MEMALIGN        1
#else
#define SW_HAVE_POSIX_MEMALIGN  1
#define SW_HAVE_MEMALIGN        0
#endif

extern sw_uint_t  pub_pagesize;
sw_uint_t  pub_pagesize_shift;
sw_uint_t  pub_cacheline_size;

#ifndef SW_ALIGNMENT
#define SW_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define pub_mem_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define pub_mem_align_ptr(p, a) (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define pub_mem_memzero(buf, n)   (void) memset(buf,0x00, n)
#define pub_mem_memset(buf, c, n) (void) memset(buf, c, n)

#if (SW_MEMCPY_LIMIT)

void *pub_mem_memcpy(void *dst, void *src, size_t n);
#define pub_mem_cpymem(dst, src, n) (((u_char *) pub_mem_memcpy(dst, src, n)) + (n))

#else

#define pub_mem_memcpy(dst, src, n) (void) memcpy(dst, src, n)
#define pub_mem_cpymem(dst, src, n) (((u_char*) memcpy(dst, src, n)) + (n))

#endif

#define pub_mem_memmove(dst, src, n) (void) memmove(dst, src, n)
#define pub_mem_movemem(dst, src, n) (((u_char *) memmove(dst, src, n)) + (n))

#define pub_mem_memcmp(s1, s2, n) memcmp((const char *) s1, (const char *) s2, n)

void *pub_mem_alloc(size_t size);
void *pub_mem_calloc(size_t size);

#define pub_mem_free(p)	free(p);\
			p = NULL;

/*
 * Linux has memalign() or posix_memalign()
 * Solaris has memalign()
 * FreeBSD 7.0 has posix_memalign(), besides, early version's malloc()
 * aligns allocations bigger than page size at the page boundary
 */

#if (SW_HAVE_POSIX_MEMALIGN || SW_HAVE_MEMALIGN)

void *pub_mem_memalign(size_t alignment, size_t size);

#else

#define pub_mem_memalign(alignment, size)  pub_mem_alloc(size)

#endif

#endif
