#include "pub_xml_convert.h"

/* UTF-8 Encoding 
 *
 * Send Bug Reports to Chang Zhenghao
 *
 */

#define  DEFAULT_NAME_OF_PKG   "#tx_code"
#define  ANALY_PKG_PATH_1st    ".CBM.ANALYZE.MAP.PACKAGE"
#define  INTEG_PKG_PATH_1st    ".CBM.INTEGRATE.MAP.PACKAGE"
#define  MULTIPLE_NODE_MARK    "(%d)" /* u won't edit it */ 

#define  XNAME_MAX_LEN         (511 + 1)
#define  CHECK_NAME_MAX_LEN    ( 31 + 1)
#define  CHECK_VALUE_MAX_LEN   ( 31 + 1)
#define  SNAME_MAX_LEN         (127 + 1)
#define  MNAME_MAX_LEN         ( 31 + 1)
#define  MNAME_INDEXED_MAX_LEN (127 + 1)
#define  PATH_INDEXED_MAX_LEN  (255 + 1)
#define  NODE_STATE_NAME_LEN     32

extern int pkg_check_mult(sw_loc_vars_t *vars, sw_xmlnode_t *check_node);

/* for example, XNAME = .A.B(%d).C.D */
struct node_state
{
	/* node[0].name = A | node[1].name = B
	 * node[2].name = C | node[3].name = D */
	char name[NODE_STATE_NAME_LEN];

	/* node[i].maxname = the i-th MNAME of config item  */
	char maxname[NODE_STATE_NAME_LEN];

	/* node[0] = .A
	 * node[1] = .A.B
	 * node[2] = .A.B.C
	 * node[3] = .A.B.C.D */
	sw_xmlnode_t * current;

	/* .A.B(index_loop) when A stays its current state */
	int index_loop;

	/* .A(%d).B(max count) the max count of B in .A(0-max)  */
	int max_looped;

	/* contains MULTIPLE_NODE_MARK  */
	unsigned is_multiple:1;
};

enum package_config_type 
{
	/* PCT, package_config_type */

	PCT_ANALYZE,
	PCT_INTEGRATE,
};

static int 
_process_entry( sw_loc_vars_t * vars, 
		sw_xmltree_t  * config_tree, 
		int type                   );

static const char * 
_get_NAME_value_from_vars(
		sw_loc_vars_t * vars, 
		sw_xmlnode_t  * name_node);

static int 
_check_INCLUDE( const char * word, 
		const char * words_seperated_by_spaces);

static int 
_check_CHECKs(  sw_loc_vars_t * vars, 
		sw_xmlnode_t  * check_node_1st);

static size_t 
_strchr_count(  const char * str, 
		      char   ch  );

static size_t 
_strstr_count(  const char *haystack, 
		const char *needle  );

static void 
_clean_path_str(char *path);

static int 
_process_item_each(
		sw_loc_vars_t * vars, 
		sw_xmltree_t  * config_tree, 
		sw_xmlnode_t  * item,
		int type                   );

static void  
_save_unique_node_to_vars(
		sw_loc_vars_t * vars, 
		sw_xmltree_t  * packet_tree, 
		char * xname, 
		char * sname, 
		char * defaul               );

static void
_save_unique_var_to_xml(
		sw_loc_vars_t * vars, 
		char * xname, 
		char * sname, 
		char * defaul, 
		char   attrflag, 
		int    creatflag     );

static int
_save_multiple_var_to_xml(
		sw_loc_vars_t * vars, 
		char * xname, 
		char * sname, 
	  const char * mname, 
		char * defaul, 
		char   attrflag, 
		int    creatflag     );

static void
_init_node_state(
		struct node_state node[], 
		size_t node_count, 
		sw_xmltree_t * packet_tree, 
		const char   * fullpath,
		const char   * maxname);

static void 
_process_nodes( sw_loc_vars_t * vars,
		sw_xmltree_t  * packet_tree,
		struct node_state *nodes,
	       	size_t nodes_count, 
		char * xname, 
		char * sname, 
		char * defaul);

static void
_set_last_node_value_to_vars(
		sw_loc_vars_t * vars,
		const struct node_state *nodes, 
		      size_t nodes_count, 
		const char * sname, 
		      char * defaul);

static int 
_find_1st_multiple_parent_node(
		const struct node_state *nodes,
		size_t child_index);

static int
_is_1st_multiple_node(
		const struct node_state *nodes, 
		size_t nodes_count,
		size_t index_to_check);

static void 
_save_nodes_maxlooped_to_vars(
		sw_loc_vars_t *vars,
		struct node_state *nodes,
		size_t nodes_count);

static int
_get_mname_value_from_vars(
		sw_loc_vars_t *vars,
		const char * mname,
		       int * value,
		size_t value_size);
static void
_get_indexed_sname_value(
		char * out,
		char * sname,
		int  * index,
		size_t index_count);

static void
_set_xml_path_by_index(
		const char * xname, 
		const int  * index,
		size_t index_count,
		char * out);



sw_int_t pub_xml_convert_analyze( sw_loc_vars_t *pstlocvar, sw_xmltree_t *pxml)
{
	return (sw_int_t) _process_entry(pstlocvar, pxml, PCT_ANALYZE);
}

sw_int_t pub_xml_convert_integrate(sw_loc_vars_t *pstlocvar, sw_xmltree_t *pxml)
{
	return (sw_int_t) _process_entry(pstlocvar, pxml, PCT_INTEGRATE);
}


static int _process_entry(
		sw_loc_vars_t *vars,
	       	sw_xmltree_t *config_tree,
	       	int type)
{
	sw_xmlnode_t *package;

	if (vars == NULL)
	{
		pub_log_error("[%s][%d] vars == NULL", __FILE__, __LINE__);
		return -1;
	}
	if (config_tree == NULL)
	{
		pub_log_error("[%s][%d] config_xml == NULL", __FILE__, __LINE__);
		return -1;
	}

	if (type == PCT_ANALYZE)
		package = pub_xml_locnode(config_tree, ANALY_PKG_PATH_1st);
	else if (type == PCT_INTEGRATE)
		package = pub_xml_locnode(config_tree, INTEG_PKG_PATH_1st);
	else
		assert(0);

	for ( ; package != NULL; package = package->next)
	{
		if (0 == strcmp(package->name, "PACKAGE"))
			config_tree->current = package;
		else
			continue;

		sw_xmlnode_t * subnode;
		const char   * name_value;

		subnode = pub_xml_locnode(config_tree, "NAME");
		name_value = _get_NAME_value_from_vars(vars, subnode);
		if (name_value == NULL)
		{
			if (NULL != pub_xml_locnode(config_tree, "INCLUDE"))
				name_value = "#tx_code";
		}

		subnode = pub_xml_locnode(config_tree, "INCLUDE");
		if (subnode != NULL)
		{
			if (subnode->value == NULL || (subnode->value)[0] == '\0')
			{
				pub_log_info("[%s][%d] config item [INCLUDE] isn't correct", __FILE__, __LINE__);
				continue;
			}

			if (0 !=  _check_INCLUDE(name_value, subnode->value))
			{
				continue;
			}
		}	

		subnode = pub_xml_locnode(config_tree, "CHECK");
		if (subnode != NULL)
		{
			if (0 != pkg_check_mult(vars, subnode))
				continue;
		}

		sw_xmlnode_t *item = pub_xml_locnode(config_tree, "ITEM");
		while(item != NULL)
		{
			if (0 != _process_item_each(vars, config_tree, item, type))
			{
				pub_log_error("[%s][%d] config item error", __FILE__, __LINE__);
				return -1;
			}

			item = item->next;
		}
	}

	return 0;
}


static const char * 
_get_NAME_value_from_vars(
		sw_loc_vars_t *vars, 
		sw_xmlnode_t *name_node)
{
	const char *value;
	int size;

	if (   name_node != NULL 
	    && name_node->value 
	    && name_node->value[0] != '\0')
	{
		value = vars->get_var_addr(vars, name_node->value, &size);
	}
	else
	{
		value = vars->get_var_addr(vars, DEFAULT_NAME_OF_PKG, &size);
	}

	return value;
}


static int _check_INCLUDE(
		const char *word, 
		const char *words_seperated_by_spaces)
{
	char *words;
	char *saveptr;
	char *token;

	words = strdup(words_seperated_by_spaces);
	token = strtok_r(words, " ", &saveptr);

	if (0 == strcmp(word, token))
	{
		free(words);
		return 0;
	}

	for (;;) 
	{
		token = strtok_r(NULL, " ", &saveptr);
		if (token == NULL)
			break;

		if (0 == strcmp(word, token))
		{
			free(words);
			return 0;
		}
	}

	free(words);
	return -1;
}


static int _check_CHECKs(
		sw_loc_vars_t *vars, 
		sw_xmlnode_t *check_node_1st)
{
	sw_xmlnode_t *node = check_node_1st;

	while(node != NULL)
	{
		const char * expression;
		char name[CHECK_NAME_MAX_LEN];
		const char *sign;
		char value_from_expression[CHECK_VALUE_MAX_LEN];
		const char *value_from_vars;

		if (0 != strcmp(node->name, "CHECK"))
			break;

		expression = node->value;
		if (expression == NULL || expression[0] == '\0')
		{
			pub_log_info("[%s][%d] config item [CHECK] isn't correct", __FILE__, __LINE__);
			pub_log_error("[%s][%d] config item [CHECK] isn't correct", __FILE__, __LINE__);

			break;
		}

		/* ignore sign */
		if (   strlen(expression) 
		   > (CHECK_NAME_MAX_LEN+CHECK_VALUE_MAX_LEN))
		{
			pub_log_error("[%s][%d] config item [CHECK] isn't correct", __FILE__, __LINE__);

			return -1;
		}

		sign = strchr(expression, '=');
		if (sign == NULL)
			sign = strchr(expression, '?'); /* ? means != */
		if (sign == NULL)
		{
			pub_log_error("[%s][%d] config item [CHECK] isn't correct", __FILE__, __LINE__);
			return -1;
		}

		size_t name_len = sign - expression;
		memcpy(name, expression, name_len);
		name[name_len] = '\0';
		pub_str_zipspace(name);

		size_t value_exp_len = strlen(expression) - (sign + 1 - expression);
		memcpy(value_from_expression, sign+1, value_exp_len);
		value_from_expression[value_exp_len] = '\0';
		pub_str_zipspace(value_from_expression);

		int len_should_ignore;
		value_from_vars = vars->get_var_addr(vars, name, &len_should_ignore);
		if (value_from_vars == NULL)
		{
			pub_log_error("[%s][%d] config item [CHECK] vars.getvalue(%s) == NULL", name, __FILE__, __LINE__);

			return -1;
		}

		if (*sign == '=')
		{
			if (0 != strcmp(value_from_expression, value_from_vars))
				return -1;
		}
		else
		{
			if (0 == strcmp(value_from_expression, value_from_vars))
				return -1;
		}

		node = node->next;
	}

	return 0;
}


static size_t 
_strchr_count(const char * str, char ch)
{
	size_t count = 0;

	while (str && (str = strchr(str, ch)))
	{
		count++;
		str++;
	}

	return count;
}


static size_t 
_strstr_count(const char *haystack, const char *needle)
{
	size_t count = 0;
	const char *p = haystack; /* good for reading */

	while(p && (p = strstr(p, needle)))
	{
		count++;
		p += strlen(needle);
	}

	return count;
}


static int _process_item_each(
		sw_loc_vars_t *vars, 
		sw_xmltree_t *config_tree, 
		sw_xmlnode_t *item,
		int type)
{
	sw_xmltree_t *packet_tree; 
	sw_xmlnode_t *node;

	/* remove const to avoid warnngs */
	char *xname;
	char *sname;
	char *defaul;
	const char *mname;
	char attrflag = '0';
	int createflag = 0;

	config_tree->current = item;

	node = pub_xml_locnode(config_tree, "XNAME");
	if (node == NULL)
		return -1;
	xname = node->value;
	if (xname == NULL || xname[0] != '.')
		return -1;

	node = pub_xml_locnode(config_tree, "SNAME");
	if (node == NULL)
		return -1;
	sname = node->value;
	if (sname == NULL || sname[0] == '\0')
		return -1;
	
	node = pub_xml_locnode(config_tree, "DEFAULT");
	if (node != NULL)
	{
		defaul = node->value; 

		if (defaul == NULL)
			return -1;
	}
	else
	{
		defaul = NULL;
	}

	node = pub_xml_locnode(config_tree, "MNAME");
	if (node == NULL)
	{
		if (NULL != strstr(xname, MULTIPLE_NODE_MARK))
			return -1;
		else
			mname = NULL;
	}
	else
	{
		mname = node->value;
		if (mname == NULL || mname[0] == '\0')
			return -1;

		if (_strchr_count(mname, '#') != _strstr_count(xname, MULTIPLE_NODE_MARK))
			return -1;

		if (NULL != strstr(mname, "##"))
			return -1;

		if (NULL != strstr(mname, "# "))
			return -1;
	}

	if (type == PCT_INTEGRATE)
	{
		node = pub_xml_locnode(config_tree, "ATTR");
		if (node != NULL && node->value && node->value[0] == '1')
			attrflag = '1';
		else
			attrflag = '0';

		node =pub_xml_locnode(config_tree, "CRETFLAG");
		if (node != NULL && node->value &&  node->value[0] == '1')
			createflag = 1;
		else
			createflag = 0;
	}

	packet_tree = vars->tree;

	if (NULL == strstr(xname, MULTIPLE_NODE_MARK)) 
	{
		if (type == PCT_ANALYZE)
			_save_unique_node_to_vars(vars, packet_tree, xname, sname, defaul);
		else if (type == PCT_INTEGRATE)
			_save_unique_var_to_xml(vars, xname, sname, defaul, attrflag, createflag);
		else
			assert(0);
	}
	else
	{
		if (mname == NULL)
			return -1;

		if (type == PCT_ANALYZE)
		{
			struct node_state *nodes;
			size_t node_count;

			node_count = _strchr_count(xname, '.');
			nodes = malloc(sizeof (struct node_state) * node_count);

			_init_node_state(nodes, node_count, packet_tree, xname, mname);
			_process_nodes(vars, packet_tree, nodes, node_count, xname, sname, defaul); 
			_save_nodes_maxlooped_to_vars(vars, nodes, node_count);

			free(nodes);
		}
		else if (type == PCT_INTEGRATE)
		{
			return _save_multiple_var_to_xml(vars, xname, sname, mname, defaul, attrflag, createflag);
		}
		else
			assert(0);
	}

	return 0;
}


static void 
_save_unique_node_to_vars(
		sw_loc_vars_t *vars, 
		sw_xmltree_t *packet_tree, 
		char * xname, 
		char * sname, 
		char * defaul)
{
	sw_xmlnode_t *node;

	if (defaul == NULL)
	{
		node = pub_xml_locnode(packet_tree, xname);
		if (node != NULL && node->value)
		{
			vars->set_var(vars, sname, 'a', node->value, strlen(node->value));
			pub_log_info("[%s]=>[%s]=[%s]", xname, sname, 
					node->value ? node->value : "null");   
		}
	}
	else
	{
		vars->set_var(vars, sname, 'a', defaul, strlen(defaul));
		pub_log_info("[%s]=>[%s]=[%s]", xname, sname, defaul);
	}
}


static void
_save_unique_var_to_xml(
		sw_loc_vars_t *vars, 
		char *xname, 
		char *sname, 
		char *defaul, 
		char attrflag, 
		int createflag)
{
	if (defaul == NULL)
	{
		char *value;
		int size;

		/* value can be null */
		value = vars->get_var_addr(vars, sname, &size);

		set_zdxml_data(vars, xname, value, attrflag, createflag);
		if (value != NULL)
		{
			pub_log_info("[%s][%d] 映射变量:[%s]=[%s]", 
				__FILE__, __LINE__, xname, value);
		}
	}
	else
	{
		/* TODO-2
		 *
		 * pub_xml_convert_integrate() takes around 0.25 ms (10 items)
		 * without the following function that saves data to xml in my 
		 * 1st test case, or it takes at least double.
		 *
		 * I do NOT think there is room for improment without algorithm
		 * changing of my code(require rewriting all related functions). 
		 * If u r willing to optimize performance, consider rewriting 
		 * the folloing function, set_zdxml_data (/src/core/newvars.c)  
		 *
		 * BUGS set_zdxml_data():
		 *    see bugs in test config file 
		 *
		 */

		set_zdxml_data(vars, xname, defaul, attrflag, createflag);
		pub_log_info("[%s][%d] 映射变量:[%s]=[%s][%d] attr=[%c]", 
			__FILE__, __LINE__, xname, defaul, createflag, attrflag);
	}
}


static void
_init_node_state(struct node_state nodes[], 
		size_t nodes_count, 
		sw_xmltree_t *packet_tree, 
		const char *fullpath,
		const char *maxname)
{
	size_t	i = 0;
	char path[XNAME_MAX_LEN]; 
	const char *path_end;

	/* you will debug with fun if the following line is commented */
	memset(nodes, 0x00, sizeof(struct node_state) * nodes_count);

	/* skip '.' first */
	path_end = fullpath + 1;
	for (i = 0; i < nodes_count; i++)
	{
		size_t path_len;

		path_end = strchr(path_end, '.');
		if (path_end == NULL)
		{
			path_end = fullpath + strlen(fullpath); 
		}

		path_len = path_end - fullpath;
		memcpy(path, fullpath, path_len);
		path[path_len] = '\0';

		const char *name_begin;
		const char *name_end;

		name_begin = 1 + strrchr(path, '.');
		name_end = strstr(name_begin, MULTIPLE_NODE_MARK);

		if (name_end == NULL)
		{
			strncpy(nodes[i].name, name_begin, NODE_STATE_NAME_LEN);
		} 
		else
		{
			strncpy(nodes[i].name, name_begin, name_end - name_begin);
			nodes[i].name[name_end - name_begin] = '\0';

			nodes[i].is_multiple = 1;
		}

		/* remove %d in path */
		_clean_path_str(path);

		/* then locate it */
		nodes[i].current = pub_xml_locnode(packet_tree, path);

		path_end++;
	}

	/* TODO-3
	 *
	 * the following for loop is similar to 
	 *      _get_mname_value_from_vars()
	 * refactor it !
	 */

	for (i = 0; i < nodes_count; i++)
	{
		if ( ! nodes[i].is_multiple)
			continue;

		const char *maxname_end;
		size_t maxname_len;

		maxname_end = strchr(maxname, ' ');

		if (maxname_end == NULL)
		{
			maxname_end = strchr(maxname + 1, '#');
			if (maxname_end == NULL)
				maxname_end = maxname + strlen(maxname);
		}

		maxname_len = maxname_end - maxname;

		memcpy(nodes[i].maxname, maxname, maxname_len);
		nodes[i].maxname[maxname_len] = 0;

		maxname = strchr(maxname_end, '#') ;
	}

	return;
}


static void _clean_path_str(char *path)
{
	size_t skip_len = strlen(MULTIPLE_NODE_MARK);
	char * move_start;

	while((path = strstr(path, MULTIPLE_NODE_MARK)))
	{
		move_start = path + skip_len;
		memmove(path, move_start, strlen(move_start)+1);
	}
}


static void 
_process_nodes( sw_loc_vars_t *vars,
		sw_xmltree_t *packet_tree,
		struct node_state *nodes,
	       	size_t nodes_count, 
		char *xname, 
		char *sname, 
		char *defaul)
{

	for (;;)
	{
		struct node_state *last_node = &nodes[nodes_count -1];

		if (last_node->current != NULL) 
		{
			/* if lastnode is what we need then process it, or move to next WITHOUT
			 * other state changing, that's what 'bool match' flags on */
			bool match = (0 == strcmp(last_node->current->name, last_node->name));

			if (match)
				_set_last_node_value_to_vars(vars, nodes, nodes_count, sname, defaul);

			if (last_node->is_multiple) /* xname=".A.B [ (%d) or not] .Z(%d)" */
			{
				last_node->current = last_node->current->next;

				if (match)
				{
					(last_node->index_loop)++;

					/* if XNAME = A.B.C...Z(%d) and Z is the only multiple node
					 * the following code works(only under this circumstance) */
					if (last_node->max_looped < last_node->index_loop)
						last_node->max_looped = last_node->index_loop;
				}

				continue;
			}
			else  /* xname=".A.B [ (%d) or not] .Z" */
			{
				/* to avoid a useless step*/
				last_node->current = NULL; 
			}
		}

		/* updating nodes' state from here, i.e.
		 * move the first non-null multiple parent node to node->next,
		 * then renew its childrent to the exact path */
		
		int pi;  /* pi, parent index */

		/* _find_1st_multiple_parent_node() returns the first index of
		 * parent of node[pi], so it's the last node's index initially */
		pi = nodes_count - 1;

		while (-1 != (pi = _find_1st_multiple_parent_node(nodes, pi)))
		{
			if (nodes[pi].current == NULL) 
			{
				if (_is_1st_multiple_node(nodes, nodes_count, pi))
				{
					if (nodes[pi].max_looped < nodes[pi].index_loop)
						nodes[pi].max_looped = nodes[pi].index_loop;

					nodes[pi].index_loop = 0;

					/* there r 2 'return' inside this function, 
					 * the other one follows this while loop closely*/  
					return;
				}
				else
				{
					continue;
				}
			}

			nodes[pi].index_loop++;
			if (nodes[pi].max_looped < nodes[pi].index_loop)
				nodes[pi].max_looped = nodes[pi].index_loop;

			nodes[pi].current = nodes[pi].current->next;

			/* what if config item is .A.B(%d).C
			 * and packet is  .A.B(0).C
			 *                .A.B(1).C
			 *                .A.X
			 *                .A.B(2).C 
			 * the following for loop handles it */
			for (;;)
			{
				if (nodes[pi].current == NULL)
					break;

				if (0 == strcmp(nodes[pi].current->name, nodes[pi].name))
					break;
				else
					nodes[pi].current = nodes[pi].current->next;
			}

			if (nodes[pi].current == NULL)
				continue;	

			size_t	ci; /* child index*/

			for (ci = pi + 1; ci < nodes_count; ci++)
			{
				if (nodes[ci].is_multiple)
				{
					if (nodes[ci].max_looped < nodes[ci].index_loop)
						nodes[ci].max_looped = nodes[ci].index_loop;

					nodes[ci].index_loop = 0;
				}

				packet_tree->current = nodes[ci-1].current;
				nodes[ci].current = pub_xml_locnode(packet_tree, nodes[ci].name);
			}

			/* all nodes's state is processed, start again */
			goto NEXT_FOR;
		}

		/* pi == -1, the very first multiple parent's state is of end, 
		 * so just break (and then return) */
		break; /* == return, but makes it clearly */

NEXT_FOR:
		continue;
	}

	/* be careful when adding code(not recommended)
	 * behind this implicit 'return' */
}


static int 
_find_1st_multiple_parent_node(const struct node_state *nodes, size_t child_index)
{
	int i;

	for (i = child_index - 1; i >= 0; i--)
	{
		if (nodes[i].is_multiple == 1) 
			return i;
	}

	return -1;
}


static int
_is_1st_multiple_node(
		const struct node_state *nodes, 
		size_t nodes_count,
		size_t index_to_check)
{
	int i;

	for (i = 0; i < (int)nodes_count; i++)
	{
		if (nodes[i].is_multiple)
		{
			if (i == (int)index_to_check)
				return true; 
		}
	}

	return false; 
}


static void
_set_last_node_value_to_vars(
		      sw_loc_vars_t *vars,
		const struct node_state *nodes, 
		      size_t nodes_count, 
		const char * sname, 
	 	      char * defaul)
{
	const struct node_state *last_node;
	char   var_name[SNAME_MAX_LEN];
	char * var_value;

	last_node = &nodes[nodes_count -1];
	strcpy(var_name, sname);
	var_value = last_node->current->value;
	
	int i;
	for (i = 0; i < (int)nodes_count; i++)
	{
		if (nodes[i].is_multiple)
		{
			char tmp[16]; 
			/* 100,000,000 uses tmp[0]-tmp[10],
			 * so 16 is enough */

			sprintf(tmp, "(%d)", nodes[i].index_loop);
			strcat(var_name, tmp);
		}
	}

	if (defaul == NULL)
	{
		if (var_value == NULL)
		{
			return ;
		}
		vars->set_var(vars, var_name, 'a', var_value, strlen(var_value));
	}
	else
	{
		vars->set_var(vars, var_name, 'a', defaul, strlen(defaul));
	}

	pub_log_info("[%s]=[%s]", var_name, var_value);   
}


static void _save_nodes_maxlooped_to_vars(
		sw_loc_vars_t *vars,
		struct node_state *nodes,
		size_t nodes_count)
{
	int i;
	char buf[16];

	for (i = 0; i < (int)nodes_count; i++)
	{
		if ( ! nodes[i].is_multiple)
			continue;

		snprintf(buf, sizeof buf, "%d", nodes[i].max_looped);

		vars->set_var(vars, nodes[i].maxname, 'a', buf, strlen(buf));

		pub_log_info("[%s]=[%s]", nodes[i].maxname, buf);   
	}
}

static int 
_get_mname_value_from_vars(
		sw_loc_vars_t *vars,
		const char *mname,
		int *value,
		size_t value_size)
{
	int i;

	for (i = 0; i < (int)value_size; i++)
	{
		char maxname[MNAME_MAX_LEN];
		char *maxname_value;
		int maxname_value_len;
		const char *maxname_end;
		size_t maxname_len;

		maxname_end = strchr(mname, ' ');

		if (maxname_end == NULL)
		{
			maxname_end = strchr(mname + 1, '#');
			if (maxname_end == NULL)
				maxname_end = mname + strlen(mname);
		}

		maxname_len = maxname_end - mname;

		memcpy(maxname, mname, maxname_len);
		maxname[maxname_len] = 0;

		maxname_value = vars->get_var_addr(vars, maxname, &maxname_value_len);
		if (maxname_value == NULL)
		{
			pub_log_error("[%s][%d] vars.getvalue(%s) == NULL", __FILE__, __LINE__, maxname);
			return -1;
		}

		value[i] = -1;
		value[i] = atoi(maxname_value);
		if (value[i] == -1)
		{
			pub_log_error("[%s][%d] atoi(%s) failed", __FILE__, __LINE__, maxname_value);
			return -1;
		}

		mname = strchr(maxname_end, '#') ;
	}

	return 0;
}

static void
_get_indexed_sname_value(
		char *out,
		char *sname,
		int *index,
		size_t index_count
		)
{
	int i;

	out[0] = '\0';
	strcat(out, sname);

	for (i = 0; i < (int)index_count; i++)
	{
		char idx[16];

		snprintf(idx, sizeof index, "(%d)", index[i]);

		strcat(out, idx);
	}	
}


static void
_set_xml_path_by_index(const char *xname, const int *index, size_t index_count, char *out)
{
	int i;
	const char *begin;
	const char *end;
	size_t size;
	
	begin = xname;

	for (i = 0; i< (int)index_count; i++)
	{
		char idx[16];

		end = strstr(begin, MULTIPLE_NODE_MARK);
		size = end - begin;
		memcpy(out, begin, size);
		out[size] = 0;

		snprintf(idx, sizeof idx, "(%d)", index[i]);
		strcat(out, idx);

		out = out + strlen(out);

		begin = end + strlen(MULTIPLE_NODE_MARK);
	}

	if (begin != '\0')
		strcat(out, begin);

}

static int
_save_multiple_var_to_xml(
		sw_loc_vars_t *vars, 
		char *xname, 
		char *sname, 
		const char *mname, 
		char *defaul, 
		char attrflag, 
		int creatflag)
{
	size_t multiple_count = 0;
	int *index_max;
	int *index;

	multiple_count = _strchr_count(mname, '#');
	if (multiple_count == 0)
		return -1;

	index_max = malloc(sizeof(int) * multiple_count);
	if (0 !=_get_mname_value_from_vars(vars, mname, index_max, multiple_count))
	{
		free(index_max);
		return -1;
	}

	index = calloc(multiple_count, sizeof (int));

	for (;;)
	{
		int last_index = multiple_count -1;
		if (index[last_index] < index_max[last_index])
		{
			char sname_indexed[MNAME_INDEXED_MAX_LEN];
			char *value_to_xml;
			int size;
			char path[PATH_INDEXED_MAX_LEN];
			char null_char = 0;

			_get_indexed_sname_value(sname_indexed, sname, index, multiple_count);
			_set_xml_path_by_index(xname, index, multiple_count, path);

			value_to_xml = vars->get_var_addr(vars, sname_indexed, &size);
			if (value_to_xml != NULL)
			{
				set_zdxml_data(vars, path, value_to_xml, attrflag, creatflag);
				pub_log_info("set (%s) to %s attr(%c)", 
						value_to_xml, path, attrflag);
			}
			else if (/*value_to_xml == NULL && */ creatflag)
			{
				value_to_xml = &null_char;
				set_zdxml_data(vars, path, value_to_xml, attrflag, creatflag);
				pub_log_info("set (%s) to %s attr(%c) creatflag(%d) ",
						value_to_xml, path, attrflag, creatflag) ;
			}

			index[last_index]++;
			continue;
		}

		int i = 0;

		for (i = last_index - 1; i >= 0; i--)
		{
			index[i]++;
			if (index[i] < index_max[i])
				break;
		}
		if (i == -1)
		{
			free(index);
			free(index_max);
			return 0;
		}

		for (i = i + 1; i < (int)multiple_count; i++)
			index[i] = 0;
	}
}

