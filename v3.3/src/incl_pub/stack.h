#if !defined(__STACK_H__)
#define __STACK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

/* ջ���Ż����� */
#define __STACK_OPTIMIZE__

/* ͨ��ջ���� */
typedef struct
{
    void **base;    /* ջ����ַ */
    void **top;     /* ջ����ַ */
    int size;       /* ջ�Ĵ�С */
}Stack_t;

extern int stack_init(Stack_t *stack, int size);

#if defined(__STACK_OPTIMIZE__)
/* �ͷ�ջ */
#define stack_free(stack) \
( \
    free((stack)->base), \
    (stack)->base = NULL, \
    (stack)->top = NULL, \
    (stack)->size = 0 \
)

/* ѹջ */
#define stack_push(stack, node) \
( \
    (((stack)->top-(stack)->base) >= (stack)->size)? (-1): \
    ( \
        *((stack)->top) = (node), \
        (stack)->top++, 0 \
    ) \
)

/* ��ջ */
#define stack_pop(stack) \
(   \
    ((stack)->base == (stack)->top)? (-1): \
    ( \
        (stack)->top--, \
        *((stack)->top) = NULL, 0 \
    ) \
)
#else   /*__STACK_OPTIMIZE__*/
extern void stack_free(Stack_t *stack);
extern int stack_push(Stack_t *stack, void *node);
extern int stack_pop(Stack_t * stack);
#endif /*__STACK_OPTIMIZE__*/

#define stack_isempty(stack) (((stack)->base == (stack)->top)? true : false) /* ջ�Ƿ�Ϊ�� */
#define stack_gettop(stack) (((stack)->base == (stack)->top)? NULL: *((stack)->top-1)) /* ȡջ��Ԫ�� */
#define stack_depth(stack) ((stack)->top - (stack)->base)  /* ջ��ǰ��� */
#define stack_maxdepth(stack) ((stack)->size)   /* ջ������ */

#endif /*__STACK_H__*/
