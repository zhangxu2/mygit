#if !defined(__XML_COMM_H__)
#define __XML_COMM_H__

#include "stack.h"
#include "xml_tree.h"

#include "pub_log.h"

#if defined(__XML_STACK__)

#define xml_set_type(node, t) ((node)->type = (t))          /* ���ýڵ����� */
#define xml_set_flag(node, f) ((node)->flag |= (f))     	/* ���ýڵ��־ */
#define xml_unset_flag(node, f) ((node)->flag &= ~(f))  	/* ����ڵ�ĳ����־ */
#define xml_reset_flag(node) ((node)->flag = XML_NODE_HAS_NONE)   /* ���ýڵ��־ */

#define xml_set_attr_flag(node)  xml_set_flag(node, XML_NODE_HAS_ATTR)   /* ���������� */
#define xml_unset_attr_flag(node)    xml_unset_flag(node, XML_NODE_HAS_ATTR) /* ���������� */
#define xml_set_child_flag(node) xml_set_flag(node, XML_NODE_HAS_CHILD)  /* �����к��� */
#define xml_unset_child_flag(node) xml_unset_flag(node, XML_NODE_HAS_CHILD)  /* �����޺��� */
#define xml_set_value_flag(node) xml_set_flag(node, XML_NODE_HAS_VALUE)  /* �����нڵ�ֵ */
#define xml_unset_value_flag(node) xml_unset_flag(node, XML_NODE_HAS_VALUE)  /* �����޽ڵ�ֵ */

#define xml_is_attr(node) (XML_NODE_ATTR == (node)->type)   /* �ڵ��Ƿ�Ϊ���Խڵ� */
#define xml_is_child(node) (XML_NODE_CHILD == (node)->type) /* �ڵ��Ƿ�Ϊ���ӽڵ� */
#define xml_is_root(node) (XML_NODE_ROOT == (node)->type)   /* �ڵ��Ƿ�Ϊ���׽ڵ� */
#define xml_has_value(node) (XML_NODE_HAS_VALUE&(node)->flag) /* �ڵ��Ƿ���ֵ */
#define xml_has_attr(node)  (XML_NODE_HAS_ATTR&(node)->flag)    /* �Ƿ������Խڵ� */
#define xml_has_child(node) (XML_NODE_HAS_CHILD&(node)->flag)   /* �Ƿ��к��ӽڵ� */

#define XML_BEGIN_FLAG      '<'	    /* ��ǩ��ʼ��־"<" */
#define XML_VERS_FLAG       '?'	    /* �汾��Ϣ��־"<?xml " */
#define XML_NOTE_FLAG       '!'	    /* ע����Ϣ��־"<!--" */
#define XML_END_FLAG        '/'	    /* ������־"</XXX>" */
#define STR_END_FLAG        '\0'    /* �ִ������� */

/* ������Ϣ���� */
typedef enum
{
    XML_SUCCESS,
    XML_FAILED = ~0xffff,   /* ʧ�� */
    XML_ERR_CALLOC,         /* callocʧ�� */
    XML_ERR_FORMAT,         /* XML��ʽ���� */
    XML_ERR_STACK,          /* ջ���� */
    XML_ERR_NODE_TYPE,      /* �ڵ����ʹ��� */
    XML_ERR_GET_ATTR,       /* ���Ի�ȡʧ�� */
    XML_ERR_GET_NAME,       /* ��ǩ����ȡʧ�� */
    XML_ERR_MARK_MISMATCH,  /* ��ǩ��ƥ�� */
    XML_ERR_CREAT_NODE,     /* �½��ڵ�ʧ�� */
    XML_ERR_PTR_NULL,       /* ��ָ�� */
    XML_ERR_EMPTY_TREE,     /* ���� */
    XML_ERR_FOPEN           /* fopenʧ�� */
}xml_err_t;

/* �ļ����� �ṹ�� */
typedef struct
{
    const char *str;         /* XML�ִ� */
    const char *ptr;            /* ��ǰ������λ�� */
    int length;
}xml_fparse_t;

typedef struct
{
    char *str;
    char *ptr;
}sprint_t;
#define sprint_init(sp, s) ((sp)->str = (s), (sp)->ptr = s)


extern char *xml_fload(const char *fname);

extern int xml_init(xml_tree_t **xmltree);
extern int xml_parse(xml_tree_t *xmltree, Stack_t *stack, const char *str);
extern int xml_fprint_tree(xml_node_t *root, Stack_t *stack, FILE *fp);
extern int xml_sprint_tree(xml_node_t *root, Stack_t *stack, sprint_t *sp);
extern int xml_pack_tree(xml_node_t *root, Stack_t *stack, sprint_t *sp);

extern int _xml_node_length(xml_node_t *root, Stack_t *stack);
extern int xml_pack_node_length(xml_node_t *root, Stack_t *stack);
    
extern int xml_node_sfree(xml_node_t *node);
extern xml_node_t *xml_free_next(Stack_t *stack, xml_node_t *current);

#endif /*__XML_STACK__*/
#endif /*__XML_COMM_H__*/
