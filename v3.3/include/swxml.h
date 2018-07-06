#ifndef __SWXML_H__
#define __SWXML_H__

#if defined(__XML_STACK__)

/* XML节点类型 */
typedef enum
{
    XML_NODE_ROOT,     /* 根节点 */
    XML_NODE_CHILD,    /* 孩子节点 */
    XML_NODE_ATTR,     /* 属性节点 */
    XML_NODE_UNKNOW,   /* 未知节点 */
    XML_NODE_TYPE_TOTAL = XML_NODE_UNKNOW     /* 节点类型数 */
}xml_node_type_t;

typedef struct _xml_node_t
{
    char *name;
    char *value;
    xml_node_type_t type;

    struct _xml_node_t *next;
    struct _xml_node_t *firstchild;
    struct _xml_node_t *tail;
    struct _xml_node_t *parent;
    int flag;
    struct _xml_node_t *temp;
}xml_node_t;

typedef struct
{
    xml_node_t *root;
    xml_node_t *current;
}xml_tree_t;

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

#else /*!__XML_STACK__*/

typedef struct sw_xmlnode_s	sw_xmlnode_t;

struct sw_xmlnode_s
{
	sw_xmlnode_t *next;
	sw_xmlnode_t *firstchild;
	sw_xmlnode_t *parent;
	char *name;
	char *value;
	int node_type;
};

typedef struct
{
	sw_xmlnode_t *root;
	sw_xmlnode_t *current;
}sw_xmltree_t;

extern void pub_xml_delnode(sw_xmlnode_t *pstNode);
#define pub_xml_deltree(pxml) {pub_xml_delnode(pxml->root);free( pxml);pxml=NULL;}

extern int pub_xml_calc_len(sw_xmlnode_t *pnode_cur, int level);
#define xml_pack_length(xml) (xml == NULL ? 0 : pub_xml_calc_len(xml->root, 0))

extern int pub_xml_prt(sw_xmlnode_t *pstNode);
extern int pub_xml_pack(sw_xmltree_t *pstXml,char *psPack);
extern int pub_xml_pack_ext(sw_xmltree_t *pxml,char *pack);
extern sw_xmlnode_t *pub_xml_addnode(sw_xmlnode_t *node,char *name,char *value,int flg);
extern sw_xmltree_t *pub_xml_unpack(char *psPack);
extern sw_xmltree_t *pub_xml_unpack_ext(char *pack,int iLen);
extern sw_xmltree_t *pub_xml_crtree(char *psFileName);
extern sw_xmltree_t *pub_xml_crtree_ext(char *pack,int iLen);
extern sw_xmlnode_t *pub_xml_locnode(sw_xmltree_t *pstXml,char *psLoc);
extern sw_xmlnode_t *pub_xml_locnode_value(sw_xmltree_t *xmltree,char *targ,char *value);
extern sw_xmlnode_t *pub_xml_crnode(sw_xmltree_t *pstXml,char *psName,int iFlag);
extern int pub_xml_set_value(sw_xmlnode_t *node, const char *value);

#endif /*!__XML_STACK__*/
#endif /* _FORXML_H */
