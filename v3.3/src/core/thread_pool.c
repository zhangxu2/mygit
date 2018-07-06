/*********************************************************************
 *** Module: Thread pool 
 *** File  : thread_pool.c
 *** Desc  : 
 *** Note  :
 *** Author: # Qifeng.zou # 2012.12.26 #
 ********************************************************************/
#include <signal.h>
#include <pthread.h>
#include "thread_pool.h"
#include "pub_log.h"

static int thread_create_detach(thread_pool_t *pool, int idx);
static void *thread_routine(void *arg);

/******************************************************************************
 ** Name : thread_pool_init
 ** Desc : Initalize thread pool
 ** Input: 
 **     num: The num of threads in pool
 ** Output: 
 **     pool: The object of thread pool
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Alloc thread pool space, and initalize it.
 **     2. Create the num of threads
 ** Note :
 ** Author: # Qifeng.zou # 2012.12.26 #
 ******************************************************************************/
int thread_pool_init(thread_pool_t **pool, int num)
{
	int idx=0, ret=0;

	/* 1. 分配线程池空间，并初始化 */
	*pool = (thread_pool_t*)calloc(1, sizeof(thread_pool_t));
	if(NULL == *pool)
	{
		return -1;
	}

	pthread_mutex_init(&((*pool)->queue_lock), NULL);
	pthread_cond_init(&((*pool)->queue_ready), NULL);
	(*pool)->head = NULL;
	(*pool)->queue_size = 0;
	(*pool)->shutdown = 0;
	(*pool)->threadid = (pthread_t*)calloc(1, num*sizeof(pthread_t));
	if(NULL == (*pool)->threadid)
	{
		free(*pool);
		(*pool) = NULL;
		return -1;
	}

	/* 2. 创建指定数目的线程 */
	for(idx=0; idx<num; idx++)
	{
		ret = thread_create_detach(*pool, idx);
		if(0 != ret)
		{
			return -1;
		}
		(*pool)->num++;
	}

	return 0;
}

/******************************************************************************
 ** Name : thread_pool_add_worker
 ** Desc : Register routine callback function
 ** Input: 
 **     pool: The object of thread-pool
 **     process: Callback function
 **     arg: The paramter of callback function
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Create new task node
 **     2. Add callback function into work queue
 **     3. Wakeup waitting process 
 ** Note :
 ** Author: # Qifeng.zou # 2012.12.26 #
 ******************************************************************************/
int thread_pool_add_worker(thread_pool_t *pool, void *(*process)(void *arg), void *arg)
{
	thread_worker_t *worker=NULL, *member=NULL;

	/* 1. 创建新任务节点 */
	worker = (thread_worker_t*)calloc(1, sizeof(thread_worker_t));
	if(NULL == worker)
	{
		return -1;
	}

	worker->process = process;
	worker->arg = arg;
	worker->next = NULL;

	/* 2. 将回调函数加入工作队列 */
	pthread_mutex_lock(&(pool->queue_lock));

	member = pool->head;
	if(NULL != member)
	{
		while(NULL != member->next)
		{
			member = member->next;
		}
		member->next = worker;
	}
	else
	{
		pool->head = worker;
	}

	pool->queue_size++;

	pthread_mutex_unlock(&(pool->queue_lock));

	/* 3. 唤醒正在等待的线程 */
	pthread_cond_signal(&(pool->queue_ready));

	return 0;
}

/******************************************************************************
 ** Name : thread_pool_keepalive
 ** Desc : Keepalive thread
 ** Input: 
 **     pool: The object of thread-pool
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Judge the thread whether exist?
 **     2. The thread was dead if it's not exist.
 ** Note :
 ** Author: # Qifeng.zou # 2012.12.26 #
 ******************************************************************************/
int thread_pool_keepalive(thread_pool_t *pool)
{
	int idx=0, ret=0;

 	for(idx=0; idx<pool->num; idx++)
	{
		ret = pthread_kill(pool->threadid[idx], 0);
		if(ESRCH == ret)
		{
			ret = thread_create_detach(pool, idx);
			if(ret < 0)
			{
				return -1;
			}
		}
	}

	return 0;
}

/******************************************************************************
 ** Name : thread_pool_destroy
 ** Desc : Destroy thread pool
 ** Input: 
 **     pool: The object of thread-pool
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Close thread-pool flag
 **     2. Wakeuup all the threads which is waitting
 **     3. Wait all threads end of run
 **     4. Release all threads space
 ** Note :
 ** Author: # Qifeng.zou # 2012.12.26 #
 ******************************************************************************/
int thread_pool_destroy(thread_pool_t *pool)
{
	int idx=0, ret=0;
	thread_worker_t *member = NULL;


	if(0 != pool->shutdown)
	{
		return -1;
	}

	/* 1. 设置关闭线程池标志 */
	pool->shutdown = 1;

	/* 2. 唤醒所有等待的线程 */
	pthread_cond_broadcast(&(pool->queue_ready));

	/* 3. 等待线程结束 */
	while(idx < pool->num)
	{
		ret = pthread_kill(pool->threadid[idx], 0);
		if(ESRCH == ret)
		{
			idx++;
			continue;
		}
		pthread_cancel(pool->threadid[idx]);
		idx++;
	}

	/* 4. 释放线程池对象空间 */
	free(pool->threadid);
	pool->threadid = NULL;

	while(NULL != pool->head)
	{
		member = pool->head;
		pool->head = member->next;
		free(member);
	}

	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));
	free(pool);
	
	return 0;
}

/******************************************************************************
 ** Name : thread_create_detach
 ** Desc : Create detach thread
 ** Input: 
 **     pool: The object of thread-pool
 **     idx: The index of thead
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 **     1. Close thread-pool flag
 **     2. Wakeuup all the threads which is waitting
 **     3. Wait all threads end of run
 **     4. Release all threads space
 ** Note :
 ** Author: # Qifeng.zou # 2013.01.10 #
 ******************************************************************************/
static int thread_create_detach(thread_pool_t *pool, int idx)
{
	int ret = 0;
	pthread_attr_t attr;

	do
	{
		ret = pthread_attr_init(&attr);
		if(0 != ret)
		{
			pthread_attr_destroy(&attr);
			return -1;
		}
		
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if(0 != ret)
		{
			pthread_attr_destroy(&attr);
			return -1;
		}
		
		ret = pthread_attr_setstacksize(&attr, 0x800000);
		if (ret < 0)
		{       
			return -1;
		}
		
		ret = pthread_create(&(pool->threadid[idx]), &attr, thread_routine, pool);
		if(0 != ret)
		{
			pthread_attr_destroy(&attr);
			if(EINTR == errno)
			{
				continue;
			}
			
			return -1;
		}
		pthread_attr_destroy(&attr);
        break;
	}while(1);

	return 0;
}

/******************************************************************************
 ** Name : thread_routine
 ** Desc : The entry of routine
 ** Input: 
 **     arg: the parmater of this function
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.12.26 #
 ******************************************************************************/
static void *thread_routine(void *arg)
{
	thread_worker_t *worker = NULL;
	thread_pool_t *pool = (thread_pool_t*)arg;

	while(1)
	{
		pthread_mutex_lock(&(pool->queue_lock));
		while((0 == pool->shutdown)
			&& (0 == pool->queue_size))
		{
			pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
		}

		if(0 != pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->queue_lock));
			pthread_exit(NULL);
		}

		pool->queue_size--;
		worker = pool->head;
		pool->head = worker->next;
		pthread_mutex_unlock(&(pool->queue_lock));

		(*(worker->process))(worker->arg);

		free(worker);
		worker = NULL;
	}
}
