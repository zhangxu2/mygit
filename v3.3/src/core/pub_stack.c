#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "pub_stack.h"
#include "pub_log.h"

#undef MIN_NODES
#define MIN_NODES 4

int pub_stack_set_cmp_func(sqstack_t *st, stack_comp_pt c)
{
	if (st == NULL)
	{
		return -1;
	}

	if (st->comp != c)
	{
		st->sorted = 0;
	}
	st->comp = c;

	return 0;
}

int pub_stack_set_pop_free_func(sqstack_t *st, stack_pop_free_pt f)
{
	if (st == NULL)
	{
		return -1;
	}
	st->pop_free = f;
	
	return 0;
}

sqstack_t *pub_stack_dup(sqstack_t *st)
{
	char	**s;
	sqstack_t	*ret;
	
	if (st == NULL)
	{
		return NULL;
	}

	ret = pub_stack_new(st->comp, st->pop_free);
	if (ret == NULL)
	{
		goto err;
	}

	s = (char **)realloc((char *)ret->data, (unsigned int)sizeof(char *)*st->num_alloc);
	if (s == NULL)
	{
		pub_log_error("[%s][%d] realloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		goto err;
	}
	ret->data = s;
	ret->num = st->num;
	memcpy(ret->data, st->data, sizeof(char *)*st->num);
	ret->sorted = st->sorted;
	ret->num_alloc = st->num_alloc;
	ret->comp = st->comp;

	return ret;
err:
	if (ret)
	{
		pub_stack_free(ret);
	}

	return NULL;
}

sqstack_t *pub_stack_new_null(void)
{
	return pub_stack_new(NULL, NULL);
}

sqstack_t *pub_stack_new(stack_comp_pt c, stack_pop_free_pt f)
{
	int	i;
	sqstack_t	*ret;
	
	ret = calloc(1, sizeof(sqstack_t));
	if (ret == NULL)
	{
		printf("[%s][%d] Calloc error! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}
	
	ret->data = calloc(1, sizeof(char *)*MIN_NODES);
	if (ret->data == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}
	
	for (i = 0; i < MIN_NODES; i++)
	{
		ret->data[i] = NULL;
	}
	ret->comp = c;
	ret->pop_free = f;
	ret->num_alloc = MIN_NODES;
	ret->num = 0;
	ret->sorted = 0;

	return ret;
}

int pub_stack_insert(sqstack_t *st, void *data, int loc)
{
	char **s;
	
	if (st == NULL)
	{
		return -1;
	}

	if (st->num_alloc <= st->num + 1)
	{
		s = realloc((char *)st->data, sizeof(char *)*st->num_alloc*2);
		if (s == NULL)
		{
			pub_log_error("[%s][%d] realloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		st->data = s;
		st->num_alloc *= 2;
	}
	if ((loc >= (int)st->num) || (loc < 0))
	{
		st->data[st->num]=data;
	}
	else
	{
		int	i;
		char	**f, **t;

		f = st->data;
		t = &(st->data[1]);
		for (i = st->num; i >= loc; i--)
		{
			t[i] = f[i];
		}
			
#ifdef undef
		memmove(&(st->data[loc+1]), &(st->data[loc]), sizeof(char *)*(st->num-loc));
#endif
		st->data[loc] = data;
	}
	st->num++;
	st->sorted = 0;

	return st->num;
}

void *pub_stack_delete_ptr(sqstack_t *st, void *p)
{
	int	i = 0;

	for (i = 0; i < st->num; i++)
	{
		if (st->data[i] == p)
		{
			return pub_stack_delete(st, i);
		}
	}
	return NULL;
}

void *pub_stack_delete(sqstack_t *st, int loc)
{
	int	i = 0;
	int	j = 0;
	char	*ret = NULL;

	if (!st || (loc < 0) || (loc >= st->num))
	{
		return NULL;
	}

	ret = st->data[loc];
	if (loc != st->num - 1)
	{
		j = st->num - 1;
		for (i = loc; i < j; i++)
		{
			st->data[i] = st->data[i + 1];
		}
	}
	st->num--;

	return ret;
}

int pub_stack_find(sqstack_t *st, void *data)
{
	int	i = 0;

	if (st == NULL)
	{
		return -1;
	}

	if (st->comp == NULL)
	{
		for (i = 0; i < st->num; i++)
		{
			if (st->data[i] == data)
			{
				return i;
			}
		}
		return -1;
	}
	
	return -1;
}

int pub_stack_push(sqstack_t *st, void *data)
{
	return pub_stack_insert(st, data, st->num);
}

int pub_stack_unshift(sqstack_t *st, void *data)
{
	return pub_stack_insert(st, data, 0);
}

void *pub_stack_shift(sqstack_t *st)
{
	if (st == NULL)
	{
		return NULL;
	}
	if (st->num <= 0)
	{
		return NULL;
	}
	return pub_stack_delete(st, 0);
}

void *pub_stack_pop(sqstack_t *st)
{
	if (st == NULL)
	{
		return NULL;
	}

	if (st->num <= 0)
	{
		return NULL;
	}
	return pub_stack_delete(st, st->num - 1);
}

void *pub_sqstack_top(sqstack_t *st)
{
	return pub_stack_value(st, st->num - 1);
}

void pub_stack_zero(sqstack_t *st)
{
	if (st == NULL)
	{
		return;
	}

	if (st->num <= 0)
	{
		return;
	}

	memset((char *)st->data, 0, sizeof(st->data) * st->num);
	st->num = 0;
	
	return;
}

void pub_stack_pop_free(sqstack_t *st)                                                                                 
{                                                                                                                          
	int	i = 0;

	if (st == NULL)
	{
		return;                                                                                                    
	}

	for (i = 0; i < st->num; i++)
	{
		if (st->data[i] != NULL && st->pop_free != NULL)
		{
			st->pop_free(st->data[i]);
		}
	}
        pub_stack_free(st);
}

void pub_stack_free(sqstack_t *st)
{
	if (st == NULL)
	{
		return;
	}

	if (st->data != NULL)
	{
		free(st->data);
	}
	free(st);
	
	return;
}

int pub_stack_num(const sqstack_t *st)
{
	if (st == NULL)
	{
		return -1;
	}

	return st->num;
}

void *pub_stack_value(const sqstack_t *st, int i)
{
	if (!st || (i < 0) || (i >= st->num))
	{
		return NULL;
	}

	return st->data[i];
}

void *pub_stack_set(sqstack_t *st, int i, void *value)
{
	if (!st || (i < 0) || (i >= st->num))
	{
		return NULL;
	}
	return st->data[i] = value;
}

void pub_stack_sort(sqstack_t *st)
{
	if (st && !st->sorted)
	{
		qsort(st->data, st->num, sizeof(char *), st->comp);
		st->sorted = 1;
	}
}

int pub_stack_is_sorted(const sqstack_t *st)
{
	if (!st)
	{
		return 1;
	}
	return st->sorted;
}

