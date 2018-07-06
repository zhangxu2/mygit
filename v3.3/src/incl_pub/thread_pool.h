#if !defined(__THREAD_POOL_H__)
#define __THREAD_POOL_H__

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>


/* 线程工作内容 */
typedef struct _thread_worker_t
{
	void *(*process)(void *arg);  /* 线程调用接口 */
	void *arg;                    /* 接口参数 */
	struct _thread_worker_t *next;/* 下一个节点 */
}thread_worker_t;

/* 线程池 */
typedef struct
{
	pthread_mutex_t queue_lock;   /* 队列互斥锁 */
	pthread_cond_t queue_ready;   /* 队列临界锁 */

	thread_worker_t *head;        /* 队列头 */
	int shutdown;                 /* 是否已销毁线程 */
	pthread_t *threadid;          /* 线程ID数组 —动态分配空间 */
	int num;                      /* 实际创建的线程个数 */
	int queue_size;               /* 工作队列当前大小 */
}thread_pool_t;


extern int thread_pool_init(thread_pool_t **pool, int num);
extern int thread_pool_add_worker(thread_pool_t *pool, void *(*process)(void *arg), void *arg);
extern int thread_pool_keepalive(thread_pool_t *pool);
extern int thread_pool_destroy(thread_pool_t *pool);

#endif /*__THREAD_POOL_H__*/
