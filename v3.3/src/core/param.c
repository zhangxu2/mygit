#include "param.h"
#include "common.h"

#define param_node_is_used(param) ('\0' != param->name[0])  /* 参数节点被占用 */
#define group_idx(name, max) (Hash(name)%max)               /* 组名哈希值 */
#define param_idx(name) (Hash(name)%PARAM_HASH_NUM)         /* 参数名哈希值 */

#define group_wrlock(fd) file_wrlock(fd)                    /* 组写锁 */
#define group_rdlock(fd) file_rdlock(fd)                    /* 组读锁 */
#define group_bit_wrlock(fd, idx) fbit_wrlock(fd, idx)      /* 给指定组加写锁 */
#define group_bit_rdlock(fd, idx) fbit_rdlock(fd, idx)      /* 给指定组加读锁 */
#define group_try_wrlock(fd) file_try_wrlock(fd)            /* 尝试组写锁 */
#define group_try_rdlock(fd) file_try_rdlock(fd)            /* 尝试组读锁 */
#define group_unlock(fd) file_unlock(fd)                    /* 组解锁 */
#define group_bit_unlock(fd, idx) fbit_unlock(fd, idx)      /* 给指定组解锁 */

#define param_wrlock(fd, idx) fbit_wrlock(fd, idx)          /* 给指定组参数写锁 */
#define param_rdlock(fd, idx) fbit_rdlock(fd, idx)          /* 给指定组参数读锁 */
#define param_try_wrlock(fd, idx) fbit_try_wrlock(fd, idx)  /* 尝试给指定组参数写锁 */
#define param_try_rdlock(fd, idx) fbit_try_rdlock(fd, idx)  /* 尝试给指定组参数读锁 */
#define param_unlock(fd, idx) fbit_unlock(fd, idx)          /* 给指定组参数解锁 */


/******************************************************************************
 **函数名称: group_init
 **功    能: 初始化组基本信息
 **输入参数: 
 **     addr: 组信息首地址
 **     grpnum: 组数
 **     size: 单个组的空间大小
 **     gflock: 组锁文件
 **     pflock: 参数锁文件
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 初始化参数组汇总信息
 **     2. 初始化各组信息
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
int group_init(group_cntx_t *cntx,
		void *addr, int grpnum, size_t size, const char *gflock, const char *pflock)
{
	int ret = 0;
	group_sum_t *sum = NULL;

	if(size < PARAM_GRP_MIN_SIZE)
	{
		pub_log_error("[%s][%d] Group size is too small!", __FILE__, __LINE__);
		return -1;
	}

	/* 2. Group write lock */
	cntx->gfd = open(gflock, O_WRONLY|O_CREAT, 0777);
	if(cntx->gfd < 0)
	{
		pub_log_error(
				"[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	ret = group_wrlock(cntx->gfd);
	if(0 != ret)
	{
		pub_log_error(
				"[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	/* 3. Init group summary information */
	sum = (group_sum_t *)addr;
	sum->grpnum = grpnum;
	sum->size = size;

	snprintf(sum->gflock, sizeof(sum->gflock), "%s", gflock);
	snprintf(sum->pflock, sizeof(sum->pflock), "%s", pflock);

	cntx->pfd = open(sum->pflock, O_WRONLY|O_CREAT, 0777);
	if(cntx->pfd < 0)
	{
		group_unlock(cntx->gfd);
		pub_log_error("[%s][%d] errmsg:[%d]%s!%s",
				__FILE__, __LINE__, errno, strerror(errno), sum->pflock);
		return -1;
	}

	group_unlock(cntx->gfd);

	return 0;
}

/******************************************************************************
 **函数名称: group_alloc
 **功    能: 申请组
 **输入参数: 
 **     cntx: 上下文
 **     name: 组名
 **     gid: 组ID
 **     pgid: 父组ID
 **     remark: 备注信息
 **输出参数: NONE
 **返    回: 组索引IDX
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.09 #
 ******************************************************************************/
int group_alloc(group_cntx_t *cntx, const char *name,
		unsigned int gid, unsigned int pgid, const char *remark)
{
	char *addr = NULL;
	int ret = 0, idx = 0, i = 0, frist = -1;
	size_t pool_size = 0;
	group_t *group = NULL;
	group_head_t *head = NULL;
	group_sum_t *sum = cntx->addr;
	shm_slab_pool_t *pool = NULL;

	idx = group_idx(name, sum->grpnum);

	ret = group_wrlock(cntx->gfd);
	if(0 != ret)
	{
		pub_log_error(
				"[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	addr = (char *)cntx->addr + sizeof(group_sum_t);
	for(i=0; i<sum->grpnum; i++, idx++)
	{
		if(idx >= sum->grpnum)
		{
			idx = 0;
		}

		head = (group_head_t *)((char *)addr + idx*sum->size);

		group = &head->group;

		/* Node wether been used? */
		if(!param_node_is_used(group))
		{
			if(-1 == frist)
			{
				frist = idx;
			}
			continue;
		}

		/* Name wether is same? */
		if(0 == strcmp(name, group->name))
		{
			group->gid = gid;
			group->pgid = pgid;
			snprintf(group->remark, sizeof(group), "%s", remark);
#if 1
			pool = &head->pool;
			pool_size = sum->size - sizeof(group_head_t);
			pool->pool_size = pool_size;
			ret = shm_slab_init(pool);
			if(0 != ret)
			{
				group_unlock(cntx->gfd);
				pub_log_error("[%s][%d] Init slab failed!", __FILE__, __LINE__);
				return -1;
			}
#endif
			group_unlock(cntx->gfd);
			return idx;
		}     
	}

	/* Used frist unused group */
	if(-1 != frist)
	{
		head = (group_head_t *)(addr + frist*sum->size);
		pool = &head->pool;
		group = &head->group;
		pool_size = sum->size - sizeof(group_head_t);

		snprintf(group->name, sizeof(group->name), "%s", name);
		group->gid = gid;
		group->pgid = pgid;
		snprintf(group->remark, sizeof(group), "%s", remark);

		pool->pool_size = pool_size;
		ret = shm_slab_init(pool);
		if(0 != ret)
		{
			group_unlock(cntx->gfd);
			pub_log_error("[%s][%d] Init slab failed!", __FILE__, __LINE__);
			return -1;
		}
	}

	group_unlock(cntx->gfd);

	return frist;
}

/******************************************************************************
 **函数名称: group_free
 **功    能: 释放指定组
 **输入参数: 
 **     cntx: 上下文
 **     gidx: 组索引
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.09 #
 ******************************************************************************/
int group_free(group_cntx_t *cntx, int gidx)
{
	group_t *group = NULL;
	group_sum_t *sum = cntx->addr;
	group_head_t *head = NULL;

	head = (group_head_t *)((char *)cntx->addr + sizeof(group_sum_t) + gidx * sum->size);
	group = &head->group;

	group_bit_wrlock(cntx->gfd, gidx);
	param_wrlock(cntx->pfd, gidx);

	memset(head, 0, sum->size);
	group->idx = gidx;

	param_unlock(cntx->pfd, gidx);
	group_bit_unlock(cntx->gfd, gidx);
	return 0;
}

/******************************************************************************
 **函数名称: param_set
 **功    能: 设置产品参数信息
 **输入参数: 
 **     cntx: 上下文
 **     gidx: 组索引
 **     name: 参数名
 **     value: 参数值
 **     length: 值长度
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.09 #
 ******************************************************************************/
int param_set(group_cntx_t *cntx, int gidx, const char *name, const void *value, int length)
{
	int idx = 0;
	void *alloc = NULL;
	param_node_t *node = NULL, *new_node = NULL;
	group_sum_t *sum = cntx->addr;
	group_head_t *head = NULL;
	shm_slab_pool_t *pool = NULL;

	if(str_isempty(name))
	{
		pub_log_error("[%s][%d] Name is invaild!", __FILE__, __LINE__);
		return -1;
	}

	idx = param_idx(name);

	head = (group_head_t *)((char *)cntx->addr + sizeof(group_sum_t) + gidx * sum->size);
	pool = &head->pool;
	node = &head->params[idx];

	param_wrlock(cntx->pfd, gidx);

	/* 1. 哈希链表头是否被占用? */
	if(!param_node_is_used(node))
	{
		snprintf(node->name, sizeof(node->name), "%s", name);

		alloc = shm_slab_alloc(pool, length + 1);
		if(NULL == alloc)
		{
			param_unlock(cntx->pfd, gidx);
			pub_log_error("[%s][%d] Alloc failed!", __FILE__, __LINE__);
			return -1;
		}
		memcpy(alloc, value, length);
		node->value_offset = (char *)alloc - (char *)cntx->addr;
		node->length = length;
		head->counts++;

		param_unlock(cntx->pfd, gidx);
		return 0;
	}

	/* 2. 哈希链表头已被占用，则遍历链表 */
	do
	{
		/* 1. 找到了参数名一致的节点 */
		if(0 == strcmp(node->name, name))
		{
			if(length != node->length)
			{
				shm_slab_free(pool, (char *)cntx->addr + node->value_offset);

				alloc = shm_slab_alloc(pool, length + 1);
				if(NULL == alloc)
				{
					param_unlock(cntx->pfd, gidx);
					pub_log_error("[%s][%d] Alloc failed!", __FILE__, __LINE__);
					return -1;
				}

				node->value_offset = (char *)alloc - (char *)cntx->addr;
				node->length = length;
			}

			memcpy((char *)cntx->addr + node->value_offset, value, length);

			param_unlock(cntx->pfd, gidx);
			return 0;
		}

		/* 2. 未找到了参数名一致的节点 */
		if(!node->next_offset)
		{
			alloc = shm_slab_alloc(pool, sizeof(param_node_t));
			if(NULL == alloc)
			{
				param_unlock(cntx->pfd, gidx);
				pub_log_error("[%s][%d] Alloc failed!", __FILE__, __LINE__);
				return -1;
			}

			new_node = (param_node_t *)alloc;
			snprintf(new_node->name, sizeof(new_node->name), "%s", name);

			alloc = shm_slab_alloc(pool, length + 1);
			if(NULL == alloc)
			{
				shm_slab_free(pool, new_node);
				param_unlock(cntx->pfd, gidx);
				pub_log_error("[%s][%d] Alloc failed!", __FILE__, __LINE__);
				return -1;
			}

			memcpy(alloc, value, length);
			new_node->length = length;
			new_node->value_offset = (char *)alloc - (char *)cntx->addr;

			node->next_offset = (char *)new_node - (char *)cntx->addr;
			head->counts++;
			param_unlock(cntx->pfd, gidx);
			return 0;
		}

		node = (param_node_t *)((char *)cntx->addr + node->next_offset);
	}while(1);

	param_unlock(cntx->pfd, gidx);
	return 0;
}

/******************************************************************************
 **函数名称: param_get
 **功    能: 获取产品参数信息
 **输入参数: 
 **     cntx: 上下文
 **     gidx: 组索引
 **     name: 参数名
 **输出参数: NONE
 **返    回: 值地址
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.09 #
 ******************************************************************************/
const void *param_get(group_cntx_t *cntx, int gidx, const char *name)
{
	int idx = 0;
	param_node_t *node = NULL, *prev = NULL;
	group_sum_t *sum = cntx->addr;
	group_head_t *head = NULL;

	if(str_isempty(name))
	{
		pub_log_error("[%s][%d] Name is invaild!", __FILE__, __LINE__);
		return NULL;
	}

	idx = param_idx(name);

	head = (group_head_t *)((char *)cntx->addr + sizeof(group_sum_t) + gidx * sum->size);
	node = &head->params[idx];
	prev = node;

	param_rdlock(cntx->pfd, gidx);

	while(param_node_is_used(node) || (prev == node))
	{
		if(0 == strcmp(node->name, name))
		{
			param_unlock(cntx->pfd, gidx);

			return ((char *)cntx->addr + node->value_offset);
		}

		if(!node->next_offset)
		{
			break;
		}

		node = (param_node_t *)((char *)cntx->addr + node->next_offset);
	}

	param_unlock(cntx->pfd, gidx);
	return NULL;
}

/******************************************************************************
 **函数名称: param_get_byindex
 **功    能: 依次遍历参数表中所有参数信息 
 **输入参数: 
 **     cntx: 上下文
 **     gidx: 组索引
 **		idx: 参数顺序号
 **输出参数: 
 **		name: 参数名
 **返    回: 值地址
 **实现描述: 
 **注意事项: 
 **作    者: # Liuyong # 2013.09.09 #
 ******************************************************************************/
void *param_get_byindex(group_cntx_t *cntx, int gidx, int idx, char *name)
{
	param_node_t *node = NULL;
	group_sum_t *sum = cntx->addr;
	group_head_t *head = NULL;
	static	param_node_t	*nodebak = NULL;

	if (idx < 0)
	{
		if (nodebak == NULL || !nodebak->next_offset)
		{
			return NULL;
		}
		node = (param_node_t *)((char *)cntx->addr + nodebak->next_offset);
	}
	else
	{
		if(str_isempty(name))
		{
			pub_log_error("[%s][%d] Name is invaild!", __FILE__, __LINE__);
			return NULL;
		}

		head = (group_head_t *)((char *)cntx->addr + sizeof(group_sum_t) + gidx * sum->size);
		node = &head->params[idx];
	}
	param_rdlock(cntx->pfd, gidx);

	for(;;)
	{
		/* 1. 是否在当前哈希链表中 */
		if(param_node_is_used(node))
		{
			if (pub_str_rstrncmp(node->name, name, strlen(name)) == 0)
			{
				param_unlock(cntx->pfd, gidx);
				nodebak = node;
				return (char *)cntx->addr + node->value_offset;
			}

			if(!node->next_offset)
			{
				break;
			}

			node = (param_node_t *)((char *)cntx->addr + node->next_offset);
		}
		else
		{
			break;
		}

	}
	param_unlock(cntx->pfd, gidx);

	return NULL;
}

/******************************************************************************
 **函数名称: param_delete
 **功    能: 删除指定参数信息
 **输入参数: 
 **     cntx: 上下文
 **     gidx: 组索引
 **     name: 参数名
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.09 #
 ******************************************************************************/
int param_delete(group_cntx_t *cntx, int gidx, const char *name)
{
	int idx = 0;
	size_t offset = 0;
	param_node_t *node = NULL, *prev = NULL;
	group_sum_t *sum = cntx->addr;
	group_head_t *head = NULL;
	shm_slab_pool_t *pool = NULL;

	if(str_isempty(name))
	{
		pub_log_error("[%s][%d] Name is invaild!", __FILE__, __LINE__);
		return -1;
	}

	idx = param_idx(name);

	head = (group_head_t *)((char *)cntx->addr + sizeof(group_sum_t) + gidx * sum->size);
	pool = &head->pool;
	node = &head->params[idx];
	prev = node;

	param_wrlock(cntx->pfd, gidx);

	while(param_node_is_used(node))
	{
		if(0 == strcmp(node->name, name))
		{
			shm_slab_free(pool, (char *)cntx->addr + node->value_offset);            

			if(prev != node)
			{
				prev->next_offset = node->next_offset;
				shm_slab_free(pool, node);
			}
			else
			{
				offset = node->next_offset;

				memset(node, 0, sizeof(param_node_t));

				node->next_offset = offset;
			}
			head->counts--;
			param_unlock(cntx->pfd, gidx);

			return 0;
		}

		if(!node->next_offset)
		{
			break;
		}

		prev = node;
		node = (param_node_t *)((char *)cntx->addr + node->next_offset);
	}

	param_unlock(cntx->pfd, gidx);
	return -1;
}

/******************************************************************************
 **函数名称: param_delete_all
 **功    能: 删除所有参数信息
 **输入参数: 
 **     cntx: 上下文
 **     gidx: 组索引
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.09 #
 ******************************************************************************/
int param_delete_all(group_cntx_t *cntx, int gidx)
{
	size_t left = 0;
	group_sum_t *sum = cntx->addr;
	group_head_t *head = NULL;

	head = (group_head_t *)((char *)cntx->addr + sizeof(group_sum_t) + gidx * sum->size);

	left = sum->size - sizeof(group_head_t);

	param_wrlock(cntx->pfd, gidx);

	head->counts = 0;
	memset(head + 1, 0, left);
	memset(head->params, 0, sizeof(head->params));

	param_unlock(cntx->pfd, gidx);
	return 0;
}

/******************************************************************************
 **函数名称: group_get_total_size
 **功    能: 参数组的所占空间总大小
 **输入参数: 
 **     grpnum: 参数组数
 **     size: 单个组大小
 **输出参数: NONE
 **返    回: 参数组所占空间总大小
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.10 #
 ******************************************************************************/
int group_get_total_size(int grpnum, size_t size)
{
	return sizeof(group_sum_t) + grpnum*size;
}
