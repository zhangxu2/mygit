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


/* �̹߳������� */
typedef struct _thread_worker_t
{
	void *(*process)(void *arg);  /* �̵߳��ýӿ� */
	void *arg;                    /* �ӿڲ��� */
	struct _thread_worker_t *next;/* ��һ���ڵ� */
}thread_worker_t;

/* �̳߳� */
typedef struct
{
	pthread_mutex_t queue_lock;   /* ���л����� */
	pthread_cond_t queue_ready;   /* �����ٽ��� */

	thread_worker_t *head;        /* ����ͷ */
	int shutdown;                 /* �Ƿ��������߳� */
	pthread_t *threadid;          /* �߳�ID���� ����̬����ռ� */
	int num;                      /* ʵ�ʴ������̸߳��� */
	int queue_size;               /* �������е�ǰ��С */
}thread_pool_t;


extern int thread_pool_init(thread_pool_t **pool, int num);
extern int thread_pool_add_worker(thread_pool_t *pool, void *(*process)(void *arg), void *arg);
extern int thread_pool_keepalive(thread_pool_t *pool);
extern int thread_pool_destroy(thread_pool_t *pool);

#endif /*__THREAD_POOL_H__*/
