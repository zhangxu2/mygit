#ifndef __PUB_SQSTACK_H__
#define __PUB_SQSTACK_H__

typedef int (*stack_comp_pt)(const void *, const void *);
typedef int (*stack_pop_free_pt)(void *);
typedef struct stack_s
{
	int	num;
	char	**data;
	int	sorted;
	int	num_alloc;
	stack_comp_pt	comp;
	stack_pop_free_pt	pop_free;
}sqstack_t;

int pub_stack_num(const sqstack_t *);
void *pub_stack_value(const sqstack_t *, int);
void *pub_stack_set(sqstack_t *, int, void *);
sqstack_t *pub_stack_new(stack_comp_pt c, stack_pop_free_pt f);
sqstack_t *pub_stack_new_null(void);
void pub_stack_free(sqstack_t *);
void pub_stack_pop_free(sqstack_t *st);
int pub_stack_insert(sqstack_t *sk, void *data, int where);
void *pub_stack_delete(sqstack_t *st, int loc);
void *pub_stack_delete_ptr(sqstack_t *st, void *p);
int pub_stack_find(sqstack_t *st, void *data);
int pub_stack_find_ex(sqstack_t *st, void *data);
int pub_stack_push(sqstack_t *st, void *data);
int pub_stack_unshift(sqstack_t *st, void *data);
void *pub_stack_shift(sqstack_t *st);
void *pub_stack_pop(sqstack_t *st);
void pub_stack_zero(sqstack_t *st);
int pub_stack_set_cmp_func(sqstack_t *st, stack_comp_pt c);
int pub_stack_set_pop_free_func(sqstack_t *st, stack_pop_free_pt f);
sqstack_t *pub_stack_dup(sqstack_t *st);
void pub_stack_sort(sqstack_t *st);
int pub_stack_is_sorted(const sqstack_t *st);
void *pub_sqstack_top(sqstack_t *st);

#endif
