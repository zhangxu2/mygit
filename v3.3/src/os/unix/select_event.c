/*********************************************************************
 *** 版 本 号: v2.2
 *** 程序作者: zhang hailu
 *** 生成日期: 2012-8-2
 *** 所属模块: task_man
 *** 程序名称: taskman
 *** 程序作用: select 封装
 *** 函数列表:
 ***		sw_int32_t select_init(sw_fd_set_t *p)
 ***		void select_clear(sw_fd_set_t *p)
 ***		sw_int32_t select_add_event(sw_fd_set_t *p,sw_fd_list_t *fd_list)
 ***		sw_int32_t select_del_event(sw_fd_set_t *p,sw_fd_list_t *fd_list)
 ***		sw_int32_t select_process_events(sw_fd_set_t *p,sw_fd_list_t *fd_list,sw_uint_t timer)
 ***		sw_int32_t select_repair_fd_sets(sw_fd_set_t *p)
 *** 使用注意:
 *** 修改记录:
 *** 	修改作者:
 *** 	修改时间:
 *** 	修改内容:
 ********************************************************************/
#include "select_event.h"
#include "pub_type.h"
#include "pub_mem.h"
#include "pub_log.h"

sw_int_t select_init(sw_fd_set_t *p)
{
	sw_int32_t	i = 0;

	pub_mem_memzero(p, sizeof(sw_fd_set_t));
	p->fd_cnt = 0;
	p->fd_max = -1;

	FD_ZERO(&p->master_read_fd_set);
	FD_ZERO(&p->worker_read_fd_set);

	p->fd_list = (sw_fd_list_t *)pub_mem_calloc(FD_LIST_MAX*sizeof(sw_fd_list_t));
	if (!p->fd_list)
	{
		pub_log_error("[%s][%d][%s] error![%d][%s]" , 
			__FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
		return SW_ERROR;
	}
	p->fd_list_max = FD_LIST_MAX;
	for (i = 0; i < p->fd_list_max; i++)
	{
		p->fd_list[i].fd = SW_INVALID_FD;
		p->fd_list[i].data = NULL;
	}

	p->fd_work = (sw_fd_list_t *)pub_mem_calloc(FD_LIST_MAX*sizeof(sw_fd_list_t));
	if (!p->fd_work)
	{
		pub_log_error("[%s][%d][%s] error![%d][%s]",
			__FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
		return SW_ERROR;
	}
	p->fd_work_max = FD_LIST_MAX;
	for (i = 0; i < p->fd_work_max; i++)
	{
		p->fd_work[i].fd = SW_INVALID_FD;
		p->fd_work[i].data = NULL;
	}

	return SW_OK;
}

void select_clear(sw_fd_set_t *p)
{
	if (!p)
	{
		return ;
	}
	if (p->fd_list)
	{
		pub_mem_free(p->fd_list);
	}
	if (p->fd_work)
	{
		pub_mem_free(p->fd_work);
	}
	memset(p, 0x00, sizeof(sw_fd_set_t));

	return ;
}

sw_int_t select_add_event(sw_fd_set_t *p,sw_fd_list_t *fd_list)
{
	sw_int32_t idx = 0;
	sw_fd_list_t *tmp = NULL;

	if ( !p ||!fd_list||fd_list->fd < 0)
	{
		pub_log_error("[%s][%d] fun:[%s] input argv error!",__FILE__,__LINE__,__FUNCTION__);
		return SW_ERROR;
	}

	if (fd_list->fd >= p->fd_list_max) /*预分配的空间不够,需要扩大*/
	{

		pub_log_info("[%s][%d]预分配的空间不够,需要扩大cnt=[%d]max=[%d]",
				__FILE__, __LINE__, __FUNCTION__, p->fd_cnt,p->fd_list_max);

		tmp = (sw_fd_list_t *)pub_mem_calloc((p->fd_list_max+FD_LIST_MAX)*sizeof(sw_fd_list_t));
		if ( !tmp )
		{
			pub_log_error("[%s][%d][%s] error![%d][%s]",
				__FILE__,__LINE__,__FUNCTION__,errno,strerror(errno));
			return SW_ERROR;
		}

		p->fd_list_max += FD_LIST_MAX;
		for(idx = 0;idx < p->fd_list_max;idx++)
		{
			tmp[idx].fd = SW_INVALID_FD;
			tmp[idx].data = NULL;
		}

		memcpy(tmp, p->fd_list, p->fd_list_max*sizeof(sw_fd_list_t));
		pub_mem_free(p->fd_list);
		p->fd_list = (sw_fd_list_t *)tmp;
	}

	memcpy(&(p->fd_list[fd_list->fd]), fd_list, sizeof(sw_fd_list_t));

	p->fd_cnt++;
	/*将fd加入监控队列*/
	FD_SET(fd_list->fd,&p->master_read_fd_set);

	if (p->fd_max < fd_list->fd)
	{
		p->fd_max = fd_list->fd;
	}

	return SW_OK;
}

sw_int_t select_del_event(sw_fd_set_t *p,sw_fd_t fd)
{
	if ( !p ||fd < 0)
	{
		pub_log_error("[%s][%d] fun:[%s] input argv error!",__FILE__,__LINE__,__FUNCTION__);
		return SW_ERROR;
	}

	FD_CLR(fd,&p->master_read_fd_set);	

	if (p->fd_max  == fd) 
	{
		p->fd_max -= 1;
	}
	p->fd_cnt--;
	pub_mem_memzero(p->fd_list+fd, sizeof(sw_fd_list_t));
	p->fd_list[fd].fd = SW_INVALID_FD;
	p->fd_list[fd].data = NULL;
	p->fd_list[fd].event_handler = NULL;

	return 0;
}

sw_int_t select_process_events(sw_fd_set_t *p,sw_fd_list_t **fd_list,sw_uint64_t timer)
{
	sw_int32_t	i = 0;
	sw_int32_t	ready,nready;
	sw_err_t	err;
	sw_uint_t	found;
	struct timeval	tv, *tp;

	if (!p || p->fd_max < 0 )
	{
		pub_log_error("[%s][%d] fun:[%s] input argv error!",__FILE__,__LINE__,__FUNCTION__);
		return -1;
	}

	if (timer == SW_TIMER_INFINITE)
	{
		tp = NULL;
	} 
	else
	{
		tv.tv_sec = (sw_int64_t) (timer / 1000);
		tv.tv_usec = (sw_int64_t) ((timer % 1000) * 1000);
		tp = &tv;
	}

	p->worker_read_fd_set = p->master_read_fd_set;

	ready = select(p->fd_max + 1, &p->worker_read_fd_set, NULL,NULL, tp);

	err = (ready == -1) ? sw_errno : 0;
	if (err) 
	{
		if (err == EINTR) 
		{

			pub_log_info("select() failed err:[%d] [%s]", err, strerror(err));

		} 
		else 
		{
			pub_log_error("select() failed err:[%d] [%s]", err, strerror(err));
		}

		if (err == EBADF) 
		{
			select_repair_fd_sets(p);
		}

		return SW_ERROR;
	}

	if (ready == 0)
	{
		if (timer != SW_TIMER_INFINITE)
		{
			return SW_OK;
		}

		pub_log_info("select() returned no events without timeout");

		return SW_ERROR;
	}	

	if (p->fd_work_max <= ready)
	{
		pub_mem_free(p->fd_work);
		p->fd_work = (sw_fd_list_t *)pub_mem_calloc(ready*sizeof(sw_fd_list_t));

		if ( !p->fd_work)
		{
			pub_log_error("[%s][%d][%s] error![%d][%s]",
				__FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
			return SW_ERROR;
		}

		p->fd_work_max = ready;
	}

	pub_mem_memzero(p->fd_work,p->fd_work_max*sizeof(sw_fd_list_t));

	nready = 0;

	for (i = 0; i < p->fd_list_max; i++)
	{
		found = 0;
		if ( p->fd_list[i].fd< 0)
		{
			continue;
		}


		if (FD_ISSET(p->fd_list[i].fd, &p->worker_read_fd_set)) 
		{
			found = 1;
		}

		if (found) 
		{
			memcpy(p->fd_work+nready,p->fd_list+i,sizeof(sw_fd_list_t));
			nready++;			
		}
	}

	if (ready != nready) 
	{
		pub_log_error("select ready != events: %d:%d", ready, nready);
		select_repair_fd_sets(p);
	}

	*fd_list = p->fd_work;

	return nready;
}

sw_int_t select_repair_fd_sets(sw_fd_set_t *p)
{
	sw_fd_t		s = 0;
	socklen_t	len = 0;
	sw_int32_t	n = 0;

	for (s = 0; s <= p->fd_max; s++) 
	{
		if (FD_ISSET(s, &p->master_read_fd_set) == 0)
		{
			n = s;
			continue;
		}

		len = sizeof(sw_int32_t);
		if (getsockopt(s, SOL_SOCKET, SO_TYPE, &n, &len) == -1)
		{
			FD_CLR(s, &p->master_read_fd_set);
			pub_log_error("invalid descriptor #%d in read fd_set", s);
			return -1;
		}
	}
	p->fd_max = n;

	return 0;
}
