#ifndef __PUB_XML_H__
#define __PUB_XML_H__

#include "pub_type.h"
#include "pub_log.h"
#include "pub_mem.h"

#if defined(__XML_STACK__)
#include "xml_tree.h"

#else /*!__XML_STACK__*/

#define SW_S_CHILD 		0
#define SW_S_BROTHER 		1
#define SW_S_ALL_END 		-1
#define SW_S_NOTALL_END 	-2

/*为了模拟oracle 9i的解析器,需要区别属性和子节点*/
#define SW_NODE_ATTRIB 	0		/*属性节点	*/
#define SW_NODE_ELEMENT 	1		/*子节点	*/
#define SW_NODE_ROOT 		2		/*根节点	*/

#define XML_READ_BUF_MAX 	512
#define ITEM_NAME_LEN		128
#define ITEM_VALUE_LEN		1024
#define XML_LINE_LEN		4096

typedef struct sw_xmlnode_s sw_xmlnode_t;
struct sw_xmlnode_s
{
	sw_xmlnode_t *next;
	sw_xmlnode_t *firstchild;
	sw_xmlnode_t *parent;
	char *name;
	char *value;
	int node_type;/*结点类型，为模拟oracle 9i解析器用*/
};

typedef struct sw_xmltree_s sw_xmltree_t;

struct sw_xmltree_s
{
	sw_xmlnode_t *root;
	sw_xmlnode_t *current;
};

#define XML_FILE_TYPE 1 
#define XML_MEM_TYPE  0 

typedef struct
{
	FILE *fp;
	char *start;
	char *current;
	int  length;
}sw_xmlmem_t;

SW_PUBLIC void pub_xml_delnode(sw_xmlnode_t *pstNode);
#define pub_xml_deltree(pxml) {pub_xml_delnode(pxml->root);free( pxml);pxml=NULL;}

SW_PUBLIC sw_int_t pub_xml_calc_len(sw_xmlnode_t *pnode_cur, int level);
#define xml_pack_length(xml) (xml == NULL ? 0 : pub_xml_calc_len(xml->root, 0))

SW_PUBLIC sw_int_t pub_xml_prt(sw_xmlnode_t *pstNode);
SW_PUBLIC sw_int_t pub_xml_getpath(sw_xmlnode_t *pnode,char *path);
SW_PUBLIC sw_int_t pub_xml_get_nodevalue(sw_xmltree_t *pstXml,char *psMark,char *psNodeName,char *psValue);
SW_PUBLIC sw_int_t pub_xml_pack(sw_xmltree_t *pstXml,char *psPack);
SW_PUBLIC sw_int_t pub_xml_pack_ext(sw_xmltree_t *pxml,char *pack);
SW_PUBLIC sw_xmlnode_t *pub_xml_addnode(sw_xmlnode_t *node,char *name,char *value,int flg);

SW_PUBLIC sw_xmltree_t *pub_xml_unpack(char *psPack);
SW_PUBLIC sw_xmltree_t *pub_xml_unpack_ext(char *pack,int iLen);
SW_PUBLIC sw_xmltree_t *pub_xml_crtree(char *psFileName);
SW_PUBLIC sw_xmltree_t *pub_xml_crtree_ext(char *pack,int iLen);
SW_PUBLIC sw_xmlnode_t *pub_xml_locnode(sw_xmltree_t *pstXml,char *psLoc);
SW_PUBLIC sw_xmlnode_t *pub_xml_locnode_value(sw_xmltree_t *xmltree,char *targ,char *value);
SW_PUBLIC sw_xmlnode_t *pub_xml_crnode(sw_xmltree_t *pstXml,char *psName,int iFlag);
SW_PUBLIC sw_int_t pub_xml_set_value(sw_xmlnode_t *node, const char *value);
SW_PUBLIC sw_int_t pub_xml_pack_node(sw_xmltree_t *pxml, sw_xmlnode_t *node, char *pack);

#endif /*!__XML_STACK__*/
#endif /* _FORXML_H */
