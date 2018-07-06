#if !defined(__XML_TREE_H__)
#define __XML_TREE_H__
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__XML_STACK__)

/* ���ܺ�: �ڵ����ͬʱ�к��ӽڵ����ֵ */
/* #define __XML_BOTH_CHILD_AND_VALUE_SUPPORT__ */

#define XML_MAX_DEPTH      (32)     /* XML���������� */

#define XML_MARK_BEGIN     "<"      /* �ڵ㿪ʼ */
#define XML_MARK_BEGIN_LEN  (1)     /* �ڵ㿪ʼ ���� */
#define XML_MARK_END1      "/>"     /* �ڵ���� */
#define XML_MARK_END1_LEN   (2)     /* �ڵ���� ���� */
#define XML_MARK_END2      "</"     /* �ڵ����2 */
#define XML_MARK_END2_LEN   (2)     /* �ڵ����2 ���� */
#define XML_VERS_BEGIN     "<?xml " /* �汾��ʼ */
#define XML_VERS_BEGIN_LEN  (6)     /* �汾��ʼ ���� */
#define XML_VERS_END       "?>"     /* �汾���� */
#define XML_VERS_END_LEN    (2)     /* �汾���� ���� */
#define XML_NOTE_BEGIN     "<!--"   /* ע�Ϳ�ʼ */
#define XML_NOTE_BEGIN_LEN  (4)     /* ע�Ϳ�ʼ ���� */
#define XML_NOTE_END       "-->"    /* ע�ͽ��� */
#define XML_NOTE_END_LEN    (3)     /* ע�ͽ��� ���� */
/* XML_NOTE_END = (XML_NOTE_END1 + XML_NOTE_END2)  */
#define XML_NOTE_END1      "--"     /* ע�ͽ���1 */
#define XML_NOTE_END1_LEN   (2)     /* ע�ͽ���1 ���� */
#define XML_NOTE_END2      '>'      /* ע�ͽ���2 */

#define XML_ROOT_NAME       "ROOT"  /* ���ڵ����� */
#define XML_ROOT_NAME_SIZE  (5)     /* ���ڵ����� SIZE */

#define XML_NODE_HAS_NONE   (0)             /* ɶ��û�� */
#define XML_NODE_HAS_CHILD  (0x00000001)    /* �к��ӽڵ� */
#define XML_NODE_HAS_ATTR   (0x00000002)    /* �����Խڵ� */
#define XML_NODE_HAS_VALUE  (0x00000004)    /* �нڵ�ֵ */

/* XML�ڵ����� */
typedef enum
{
    XML_NODE_ROOT,     /* ���ڵ� */
    XML_NODE_CHILD,    /* ���ӽڵ� */
    XML_NODE_ATTR,     /* ���Խڵ� */
    XML_NODE_UNKNOW,   /* δ֪�ڵ� */
    XML_NODE_TYPE_TOTAL = XML_NODE_UNKNOW     /* �ڵ������� */
}xml_node_type_t;


/* XML�ڵ� */
typedef struct _xml_node_t
{

    char *name;                 /* �ڵ��� */
    char *value;                /* �ڵ�ֵ */
    xml_node_type_t type;       /* �ڵ����� */

    struct _xml_node_t *next;   /* �ֵܽڵ����� */
    struct _xml_node_t *firstchild;  /* ���ӽڵ�����ͷ: ���Խڵ�+���ӽڵ� */
    struct _xml_node_t *tail;   /* ���ӽڵ�����β # ����/�޸�XML��ʱʹ�� # ��߲���Ч�� */
    struct _xml_node_t *parent; /* ���׽ڵ� */

    int flag;                   /* ��¼�ڵ��Ƿ��к���(XML_NODE_HAS_CHILD)������(XML_NODE_HAS_ATTR)���ڵ�ֵ(XML_NODE_HAS_VALUE) */
    struct _xml_node_t *temp;   /* ��ʱָ��: ����XML��ʱ�����Ч��(��������£���ָ��ֵ��Ч) */    
}xml_node_t;

/* XML�� */
typedef struct
{
    xml_node_t *root;           /* ���ڵ�: ע��root�ĵ�һ���ӽڵ���������ĸ��ڵ� */
    xml_node_t *current;        /* ��ǰ���ʵĽڵ� */
}xml_tree_t;

/* ����Ľӿ� */
#define xml_child(node) (node->firstchild)
#define xml_parent(node) (node->parent)
#define xml_brother(node) (node->next)
#define xml_current(xml) (xml->current)
#define xml_name(node) (node->name)
#define xml_value(node) (node->value)
#define xml_set_current(xml, node) (xml->current = node)

extern xml_node_t *xml_node_creat(xml_node_type_t type);
extern xml_node_t *xml_node_creat_ext(xml_node_type_t type, const char *name, const char *value);
extern int xml_node_free(xml_node_t *node);

extern xml_tree_t *xml_creat(const char *fname);
extern xml_tree_t *xml_screat(const char *str);
extern xml_tree_t *xml_screat_ext(const char *str, int length);

extern int xml_fwrite(xml_tree_t *xml, const char *fname);
extern int xml_fprint(xml_tree_t *xml, FILE *fp);
extern int xml_sprint(xml_tree_t *xml, char *str);
extern int xml_spack(xml_tree_t *xml, char *str);

extern xml_node_t *xml_search(xml_tree_t *xml, const char *path);
extern xml_node_t *xml_search_ext(xml_tree_t *xml, const char *path, const char *value);

extern xml_node_t *xml_add_node(xml_node_t *node, const char *name, const char *value, int type);
extern xml_node_t *xml_add_attr(xml_node_t *node, const char *name, const char *value);
extern xml_node_t *xml_add_child(xml_node_t *node, const char *name, const char *value);
#define xml_add_brother(node, name, value) xml_add_node((node)->parent, name, value, node->type)
extern int xml_delete_child(xml_node_t *node, xml_node_t *child);
#define xml_delete_brother(node, brother) xml_delete_child((node)->parent, brother)
extern int xml_delete_empty(xml_tree_t *xml);

extern int xml_set_value(xml_node_t *node, const char *value);
extern int xml_node_length(xml_node_t *node);
#define xml_tree_length(xml) xml_node_length(xml->root->firstchild)

extern int _xml_pack_length(xml_node_t *node);
#define xml_pack_length(xml) _xml_pack_length(xml->root)

#define xml_free(xml) {xml_node_free(xml->root); free(xml); xml=NULL;}

/* ��ǵݹ��㷨ʵ�ֵ�XML����ͳһ�ӿ� */
#if 1 

#define SW_NODE_ATTRIB  XML_NODE_ATTR
#define SW_NODE_ELEMENT XML_NODE_CHILD
#define SW_NODE_ROOT    XML_NODE_ROOT

typedef xml_node_t sw_xmlnode_t;
typedef xml_tree_t sw_xmltree_t;

#define pub_xml_crtree(fname)  xml_creat(fname)
#define pub_xml_deltree(xml) xml_free(xml)

#define pub_xml_locnode(xml, path) xml_search(xml, path)
#define pub_xml_locnode_value(xml, path, value) xml_search_ext(xml, path, value)

#define pub_xml_pack(xml, fname) xml_fwrite(xml, fname)
#define pub_xml_pack_ext(xml, str) xml_spack(xml, str)

#define pub_xml_unpack(str) xml_screat(str)
#define pub_xml_unpack_ext(str, len) xml_screat_ext(str, len)
#define pub_xml_crtree_ext(str, len) xml_screat(str)

#define pub_xml_addnode(node, name, value, flag) xml_add_node(node, name, value, flag)
#define pub_xml_delnode(node) xml_node_free(node)
#define pub_xml_set_value(node, value)  xml_set_value(node, value)
#endif

#endif /*__XML_STACK__*/

#endif /*__XML_TREE_H__*/
