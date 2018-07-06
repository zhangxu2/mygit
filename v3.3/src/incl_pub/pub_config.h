/*********************************************************************
 *** 版 本 号: v2.*
 *** 程序作者: yangzhao
 *** 生成日期: 2013-3-19
 *** 所属模块: 公共模块
 *** 程序名称: pub_config.h
 *** 程序作用: 定义全局宏
 *** 函数列表:
 *** 使用注意:
 *** 修改记录:
 *** 	修改作者:
 *** 	修改时间:
 *** 	修改内容:
 ********************************************************************/


#ifndef _PUB_CONFIG_H_INCLUDED_
#define _PUB_CONFIG_H_INCLUDED_


#ifndef SW_HAVE_SO_SNDLOWAT
#define SW_HAVE_SO_SNDLOWAT     1
#endif


#if !(SW_WIN32)


#define sw_random               random

/* TODO: #ifndef */
#define SW_SHUTDOWN_SIGNAL      QUIT
#define SW_TERMINATE_SIGNAL     TERM
#define SW_NOACCEPT_SIGNAL      WINCH
#define SW_RECONFIGURE_SIGNAL   HUP

#if (SW_LINUXTHREADS)
#define SW_REOPEN_SIGNAL        INFO
#define SW_CHANGEBIN_SIGNAL     XCPU
#else
#define SW_REOPEN_SIGNAL        USR1
#define SW_CHANGEBIN_SIGNAL     USR2
#endif

#define sw_cdecl
#define sw_libc_cdecl

#endif

#define SW_ERROR	-1
#define SW_BUSY		-3

#define LF     (u_char) 10
#define CR     (u_char) 13
#define CRLF   "\x0d\x0a"

#define SW_INT32_LEN   sizeof("-2147483648") - 1
#define SW_INT64_LEN   sizeof("-9223372036854775808") - 1

#define SW_PTR_SIZE	4

#if (SW_PTR_SIZE == 4)
#define SW_INT_T_LEN   SW_INT32_LEN
#else
#define SW_INT_T_LEN   SW_INT64_LEN
#endif


#ifndef SW_ALIGNMENT
#define SW_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define sw_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define sw_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


#define sw_abort       abort


/* TODO: platform specific: array[SW_INVALID_ARRAY_INDEX] must cause SIGSEGV */
#define SW_INVALID_ARRAY_INDEX 0x80000000


/* TODO: auto_conf: sw_inline   inline __inline __inline__ */
#ifndef sw_inline
#define sw_inline      inline
#endif

#ifndef INADDR_NONE  /* Solaris */
#define INADDR_NONE  ((unsigned int) -1)
#endif

#ifdef MAXHOSTNAMELEN
#define SW_MAXHOSTNAMELEN  MAXHOSTNAMELEN
#else
#define SW_MAXHOSTNAMELEN  256
#endif


#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define SW_MAX_UINT32_VALUE  (uint32_t) 0xffffffffLL
#else
#define SW_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#endif

#define SW_MAX_INT32_VALUE   (uint32_t) 0x7fffffff


#endif /* _SW_CONFIG_H_INCLUDED_ */

