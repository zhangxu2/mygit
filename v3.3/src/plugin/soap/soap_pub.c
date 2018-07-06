#include "soap_pub.h"
#include "pub_log.h"
#include "pub_vars.h"
#include "pub_xml.h"
#include "pub_buf.h"

#if defined(SOLARIS)
#include <sys/filio.h>
#endif

#define MAX_SOAP_NAME_LEN	128
#define MAX_SOAP_TYPE_LEN	64
#define MAX_SOAP_VALUE_LEN	1024

typedef struct item_t
{
	int	level;
	int	loop;
	int	absflag;
	int	loopcnt;
	char	*value;
	char	name[MAX_SOAP_NAME_LEN];
	char	nsprefix[MAX_SOAP_NAME_LEN];
	char	tname[MAX_SOAP_NAME_LEN];
	char	mname[MAX_SOAP_NAME_LEN];
	char	type[MAX_SOAP_TYPE_LEN];
	char	loopvar[MAX_SOAP_NAME_LEN];
	char	path[MAX_SOAP_NAME_LEN*2];
}item_t;

static int get_com_item(sw_xmltree_t *xml, item_t *item);
static int save_file(const char *filename, const char *buf, int len);
extern int soap_deal_req(struct soap *soap, sw_loc_vars_t *vars);
SOAP_FMAC5 int SOAP_FMAC6 soap_server_unpack(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *tag);
SOAP_FMAC3 int SOAP_FMAC4 soap_put_header(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml);

#define MAX_FILE_NAME_LEN 1024
static sw_lsn_cycle_t *g_cycle = NULL;

struct file_node 
{
	char 	*varname;       /* the variable name which store the file name */
	char 	*filename;      /* the real file name */
	char 	*type;		/* content type */
	char 	*ptr;		/* data pointer for base64 */
	size_t	size;		/* data size for base64 */
	char	*cid;		/* cid for mtom */
	struct file_node	*next; 
};

struct file_array
{
	struct file_node	*flist_head;
	struct file_node	*flist_tail;
	int	size;
};

static struct file_array g_farray;

sw_int_t lsn_pub_not_close_fd(sw_lsn_cycle_t *cycle, sw_int32_t index)
{
	if (cycle->link_info == NULL || index < 0 || index >= cycle->lsn_conf.conmax)
	{    
		pub_log_error("[%s][%d] Param error! index=[%d]", __FILE__, __LINE__, index);
		return SW_ERROR;
	}    

	pub_log_info("[%s][%d] NEEDNOT CLOSE FD:[%d]", __FILE__, __LINE__, cycle->link_info[index].sockid);

	cycle->link_info[index].sockid = -1;
	cycle->link_info[index].mtype = -1;
	cycle->link_info[index].trace_no = 0; 
	cycle->link_info[index].start_time = 0; 
	cycle->link_info[index].use = 0; 

	return SW_OK;
}

void set_gcycle(sw_lsn_cycle_t *cycle)
{
	g_cycle = cycle;
}

char *format_dir(char *dir)
{
	char 	*p = dir;
	char 	*d = dir;
	int 	 slash_flag = 0;

	if (!dir)
		return NULL;

	while (*p)	
	{
		if (*p == '/')
		{
			if (slash_flag) {
				p++;
				continue;
			}
			slash_flag = 1;
			*d++ = *p;
		} 
		else
		{
			slash_flag = 0;
			*d++ = *p;
		}
		p++;
	}
	if (p == d) 
		return dir;
	*d = '\0';
	return dir;
}

static void fnode_reset(struct file_node *fnode)
{
	if (!fnode)
		return;
	fnode->varname = NULL;
	fnode->filename = NULL;
	fnode->type = NULL;
	fnode->ptr = NULL;
	fnode->size = 0;
	fnode->cid = NULL;
	fnode->next = NULL;
}

static void farray_reset(struct file_array *farray)
{
	if (!farray)
		return;
	farray->flist_head = NULL;
	farray->flist_tail = NULL;
	farray->size = 0;
}

static void farray_add_to_tail(struct file_array *farray, struct file_node *fnode)
{
	if (!farray || !fnode)
		return;

	if (farray->flist_head == NULL)
	{
		farray->flist_tail = farray->flist_head = fnode;
		farray->size = 1;
		return;
	}
	farray->flist_tail->next = fnode;
	farray->flist_tail = fnode;
	farray->size++;
}

static struct file_node *soap_new_fnode(struct soap *soap)
{
	struct file_node *node;
	node = soap_malloc(soap, sizeof(struct file_node));
	fnode_reset(node);
	return node;

}

static struct file_node *soap_new_fnode_by_args(struct soap *soap, 
	const char *fvarname, const char *cid, const char *type, char *ptr, size_t size)
{
	struct file_node *node = soap_new_fnode(soap);
	node->varname = soap_strdup(soap, fvarname);
	node->cid = soap_strdup(soap, cid);
	node->type = soap_strdup(soap, type);
	node->ptr = ptr;
	node->size = size;
	return node;
}

static struct file_node *farray_find_fnode_by_cid(struct file_array *array, const char *cid)
{
	struct file_node *tmp = NULL;

	for (tmp = array->flist_head; tmp; tmp = tmp->next)
	{
		if (tmp->cid && strcmp(cid, tmp->cid) == 0)
			break;
	}
	return tmp;
}

static const char *farray_find_fvarname_by_cid(struct file_array *array, const char *cid)
{
	struct file_node *node = NULL;
	if ((node = farray_find_fnode_by_cid(array, cid)))
	{
		return node->varname;
	}
	return NULL;
}

static void after_get_mime(struct soap *soap, struct x__Data *fdata, const char *varname)
{
	if (!fdata)
		return;

	struct file_node *node = NULL;
	struct _xop__Include *in = &fdata->xop__Include;

	if (fdata->xmime5__contentType && !strcmp(fdata->xmime5__contentType, "base64"))
	{ /* it's a base64  file */
		node = soap_new_fnode_by_args(soap, varname, in->id, "base64", in->__ptr, in->__size);
	}
	else 
	{ /* mime data  */
		node = soap_new_fnode_by_args(soap, varname, in->id, in->type, in->__ptr, in->__size);
	}
	farray_add_to_tail(&g_farray, node);		
}

static int deal_base64_file_node(struct soap *soap, sw_loc_vars_t *vars, struct file_node *node)
{
	char fname[MAX_FILE_NAME_LEN + 1];
	char stored_fname[MAX_FILE_NAME_LEN + 1];
	int len = 0;

	memset(fname, 0x00, sizeof(fname));
	loc_get_zd_data(vars, node->varname, fname);
	if (!fname || fname[0] == '\0' || !node->ptr)
		return SW_ERROR;

	memset(stored_fname, 0x00, sizeof(stored_fname));
	sprintf(stored_fname, "%s/dat/%s/%s", getenv("SWWORK"), g_cycle->lsn_conf.filedir, fname);

	format_dir(stored_fname);

	if (save_file(stored_fname, node->ptr, node->size) != SW_OK)	
	{
		pub_log_error("[%s][%d] save file [%s] failed [%s]",
				__FILE__, __LINE__, stored_fname, strerror(errno));

		return SW_ERROR;

	}

	node->filename = soap_strdup(soap, stored_fname);
	pub_log_info("[%s][%d] save file [%s] success, type=[%s]", 
			__FILE__, __LINE__, node->filename, node->type);
	loc_set_zd_data(vars, "$fileflag", "1");

	return SW_OK;

}

static int after_unpack_for_base64(struct soap *soap, sw_loc_vars_t *vars, struct file_array *array)
{
	if (!array)
	{
		pub_log_error("[%s][%d] array === nil", __FILE__, __LINE__);
		return SW_ERROR;
	}

	struct file_node *node = NULL;
	for (node = array->flist_head; node; node = node->next)
	{
		if (node->type && !strcmp(node->type, "base64"))
		{
			/* TODO: 
			 * if error occurs  when we save the file,
			 * reutn 'SW_ERROR' for some error dealing
			 * */
			deal_base64_file_node(soap, vars, node);
		}
	}
	return SW_OK;
}

void print_farray()
{
	struct file_node *node = NULL;
	fprintf(stderr, "\nnode count ============>%d\n", g_farray.size);
	for (node = g_farray.flist_head; node; node = node->next)
	{
		fprintf(stderr, "node->varname  =[%s]\n", !node->varname ? "null" : node->varname);
		fprintf(stderr, "node->filename =[%s]\n", !node->filename ? "null" : node->filename);
		fprintf(stderr, "node->type     =[%s]\n", !node->type ? "null" : node->type);
		fprintf(stderr, "node->ptr      =[%p]\n", node->ptr);
		fprintf(stderr, "node->size     =[%d]\n", node->size);
		fprintf(stderr, "node->cid      =[%s]\n", !node->cid ? "null" : node->cid);
	}
}

SOAP_FMAC3 int SOAP_FMAC4 soap_getheader(struct soap *soap)
{
	soap->part = SOAP_IN_HEADER;
	soap->header = soap_in_SOAP_ENV__Header(soap, "SOAP-ENV:Header", soap->header, NULL);
	soap->part = SOAP_END_HEADER;
	return soap->header == NULL;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putheader(struct soap *soap)
{
	if (soap->header)
	{	soap->part = SOAP_IN_HEADER;
		if (soap_out_SOAP_ENV__Header(soap, "SOAP-ENV:Header", 0, soap->header, NULL))
			return soap->error;
		soap->part = SOAP_END_HEADER;
	}
	return SOAP_OK;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serializeheader(struct soap *soap)
{
	if (soap->header)
		soap_serialize_SOAP_ENV__Header(soap, soap->header);
}

SOAP_FMAC3 void SOAP_FMAC4 soap_header(struct soap *soap)
{
	if (!soap->header)
	{	if ((soap->header = (struct SOAP_ENV__Header*)soap_malloc(soap, sizeof(struct SOAP_ENV__Header))))
			soap_default_SOAP_ENV__Header(soap, soap->header);
	}
}

SOAP_FMAC3 void SOAP_FMAC4 soap_fault(struct soap *soap)
{
	if (!soap->fault)
	{	soap->fault = (struct SOAP_ENV__Fault*)soap_malloc(soap, sizeof(struct SOAP_ENV__Fault));
		if (!soap->fault)
			return;
		soap_default_SOAP_ENV__Fault(soap, soap->fault);
	}
	if (soap->version == 2 && !soap->fault->SOAP_ENV__Code)
	{	soap->fault->SOAP_ENV__Code = (struct SOAP_ENV__Code*)soap_malloc(soap, sizeof(struct SOAP_ENV__Code));
		soap_default_SOAP_ENV__Code(soap, soap->fault->SOAP_ENV__Code);
	}
	if (soap->version == 2 && !soap->fault->SOAP_ENV__Reason)
	{	soap->fault->SOAP_ENV__Reason = (struct SOAP_ENV__Reason*)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason));
		soap_default_SOAP_ENV__Reason(soap, soap->fault->SOAP_ENV__Reason);
	}
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serializefault(struct soap *soap)
{
	if (soap->fault)
		soap_serialize_SOAP_ENV__Fault(soap, soap->fault);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putfault(struct soap *soap)
{
	if (soap->fault)
		return soap_put_SOAP_ENV__Fault(soap, soap->fault, "SOAP-ENV:Fault", NULL);
	return SOAP_OK;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_getfault(struct soap *soap)
{
	return (soap->fault = soap_get_SOAP_ENV__Fault(soap, NULL, "SOAP-ENV:Fault", NULL)) == NULL;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultcode(struct soap *soap)
{
	soap_fault(soap);
	if (soap->version == 2 && soap->fault->SOAP_ENV__Code)
		return (const char**)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Value;
	return (const char**)&soap->fault->faultcode;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultsubcode(struct soap *soap)
{
	soap_fault(soap);
	if (soap->version == 2)
	{	if (!soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode)
		{	soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = (struct SOAP_ENV__Code*)soap_malloc(soap, sizeof(struct SOAP_ENV__Code));
			soap_default_SOAP_ENV__Code(soap, soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode);
		}
		return (const char**)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;
	}
	return (const char**)&soap->fault->faultcode;
}

SOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultsubcode(struct soap *soap)
{
	soap_fault(soap);
	if (soap->version == 2)
	{	if (soap->fault->SOAP_ENV__Code && soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode && soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode)
			return soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;
		return NULL;
	}
	return soap->fault->faultcode;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultstring(struct soap *soap)
{
	soap_fault(soap);
	if (soap->version == 2)
		return (const char**)&soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text;
	return (const char**)&soap->fault->faultstring;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultdetail(struct soap *soap)
{
	soap_fault(soap);
	if (soap->version == 1)
	{	if (!soap->fault->detail)
		{	soap->fault->detail = (struct SOAP_ENV__Detail*)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
			soap_default_SOAP_ENV__Detail(soap, soap->fault->detail);
		}
		return (const char**)&soap->fault->detail->__any;
	}
	if (!soap->fault->SOAP_ENV__Detail)
	{	soap->fault->SOAP_ENV__Detail = (struct SOAP_ENV__Detail*)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
		soap_default_SOAP_ENV__Detail(soap, soap->fault->SOAP_ENV__Detail);
	}
	return (const char**)&soap->fault->SOAP_ENV__Detail->__any;
}

SOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultdetail(struct soap *soap)
{
	soap_fault(soap);
	if (soap->version == 2 && soap->fault->SOAP_ENV__Detail)
		return soap->fault->SOAP_ENV__Detail->__any;
	if (soap->fault->detail)
		return soap->fault->detail->__any;
	return NULL;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_getindependent(struct soap *soap)
{
	int t;
	if (soap->version == 1)
	{	for (;;)
		{	if (!soap_getelement(soap, &t))
				if (soap->error || soap_ignore_element(soap))
					break;
		}
	}
	if (soap->error == SOAP_NO_TAG || soap->error == SOAP_EOF)
		soap->error = SOAP_OK;
	return soap->error;
}

SOAP_FMAC3 void * SOAP_FMAC4 soap_getelement(struct soap *soap, int *type)
{	(void)type;
	if (soap_peek_element(soap))
		return NULL;
	if (!*soap->id || !(*type = soap_lookup_type(soap, soap->id)))
		*type = soap_lookup_type(soap, soap->href);
	switch (*type)
	{
	case SOAP_TYPE_byte:
		return soap_in_byte(soap, NULL, NULL, "xsd:byte");
	case SOAP_TYPE_int:
		return soap_in_int(soap, NULL, NULL, "xsd:int");
	case SOAP_TYPE_double:
		return soap_in_double(soap, NULL, NULL, "xsd:double");
	case SOAP_TYPE_time:
		return soap_in_time(soap, NULL, NULL, "xsd:dateTime");
	case SOAP_TYPE_PointerTodouble:
		return soap_in_PointerTodouble(soap, NULL, NULL, "xsd:double");
	case SOAP_TYPE_PointerTotime:
		return soap_in_PointerTotime(soap, NULL, NULL, "xsd:dateTime");
	case SOAP_TYPE__QName:
	{	char **s;
		s = soap_in__QName(soap, NULL, NULL, "xsd:QName");
		return s ? *s : NULL;
	}
	case SOAP_TYPE_string:
	{	char **s;
		s = soap_in_string(soap, NULL, NULL, "xsd:string");
		return s ? *s : NULL;
	}
	default:
	{	const char *t = soap->type;
		if (!*t)
			t = soap->tag;
		if (!soap_match_tag(soap, t, "xsd:byte"))
		{	*type = SOAP_TYPE_byte;
			return soap_in_byte(soap, NULL, NULL, NULL);
		}
		if (!soap_match_tag(soap, t, "xsd:int"))
		{	*type = SOAP_TYPE_int;
			return soap_in_int(soap, NULL, NULL, NULL);
		}
		if (!soap_match_tag(soap, t, "xsd:double"))
		{	*type = SOAP_TYPE_double;
			return soap_in_double(soap, NULL, NULL, NULL);
		}
		if (!soap_match_tag(soap, t, "xsd:dateTime"))
		{	*type = SOAP_TYPE_time;
			return soap_in_time(soap, NULL, NULL, NULL);
		}
		if (!soap_match_tag(soap, t, "xsd:QName"))
		{	char **s;
			*type = SOAP_TYPE__QName;
			s = soap_in__QName(soap, NULL, NULL, NULL);
			return s ? *s : NULL;
		}
		if (!soap_match_tag(soap, t, "xsd:string"))
		{	char **s;
			*type = SOAP_TYPE_string;
			s = soap_in_string(soap, NULL, NULL, NULL);
			return s ? *s : NULL;
		}
		t = soap->tag;
	}
	}
	soap->error = SOAP_TAG_MISMATCH;
	return NULL;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_ignore_element(struct soap *soap)
{
	if (!soap_peek_element(soap))
	{	int t;
		DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Unexpected element '%s' in input (level=%u, %d)\n", soap->tag, soap->level, soap->body));
		if (soap->mustUnderstand && !soap->other)
			return soap->error = SOAP_MUSTUNDERSTAND;
		if (((soap->mode & SOAP_XML_STRICT) && soap->part != SOAP_IN_HEADER) || !soap_match_tag(soap, soap->tag, "SOAP-ENV:"))
		{	DBGLOG(TEST, SOAP_MESSAGE(fdebug, "REJECTING element '%s'\n", soap->tag));
			return soap->error = SOAP_TAG_MISMATCH;
		}
		if (!*soap->id || !soap_getelement(soap, &t))
		{	soap->peeked = 0;
			if (soap->fignore)
				soap->error = soap->fignore(soap, soap->tag);
			else
				soap->error = SOAP_OK;
			DBGLOG(TEST, if (!soap->error) SOAP_MESSAGE(fdebug, "IGNORING element '%s'\n", soap->tag));
			if (!soap->error && soap->body)
			{	soap->level++;
				while (!soap_ignore_element(soap))
					;
				if (soap->error == SOAP_NO_TAG)
					soap->error = soap_element_end_in(soap, NULL);
			}
		}
	}
	return soap->error;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putindependent(struct soap *soap)
{
	int i;
	struct soap_plist *pp;
	if (soap->version == 1 && soap->encodingStyle && !(soap->mode & (SOAP_XML_TREE | SOAP_XML_GRAPH)))
		for (i = 0; i < SOAP_PTRHASH; i++)
			for (pp = soap->pht[i]; pp; pp = pp->next)
				if (pp->mark1 == 2 || pp->mark2 == 2)
					if (soap_putelement(soap, pp->ptr, "id", pp->id, pp->type))
						return soap->error;
	return SOAP_OK;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putelement(struct soap *soap, const void *ptr, const char *tag, int id, int type)
{	(void)tag;
	switch (type)
	{
	case SOAP_TYPE_byte:
		return soap_out_byte(soap, tag, id, (const char *)ptr, "xsd:byte");
	case SOAP_TYPE_int:
		return soap_out_int(soap, tag, id, (const int *)ptr, "xsd:int");
	case SOAP_TYPE_double:
		return soap_out_double(soap, tag, id, (const double *)ptr, "xsd:double");
	case SOAP_TYPE_time:
		return soap_out_time(soap, tag, id, (const time_t *)ptr, "xsd:dateTime");
	case SOAP_TYPE_PointerTodouble:
		return soap_out_PointerTodouble(soap, tag, id, (double *const*)ptr, "xsd:double");
	case SOAP_TYPE_PointerTotime:
		return soap_out_PointerTotime(soap, tag, id, (time_t *const*)ptr, "xsd:dateTime");
	case SOAP_TYPE__QName:
		return soap_out_string(soap, tag, id, (char*const*)&ptr, "xsd:QName");
	case SOAP_TYPE_string:
		return soap_out_string(soap, tag, id, (char*const*)&ptr, "xsd:string");
	}
	return SOAP_OK;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_markelement(struct soap *soap, const void *ptr, int type)
{
	(void)soap; (void)ptr; (void)type; /* appease -Wall -Werror */
	switch (type)
	{
	case SOAP_TYPE_PointerTodouble:
		soap_serialize_PointerTodouble(soap, (double *const*)ptr);
		break;
	case SOAP_TYPE_PointerTotime:
		soap_serialize_PointerTotime(soap, (time_t *const*)ptr);
		break;
	case SOAP_TYPE__QName:
		soap_serialize_string(soap, (char*const*)&ptr);
		break;
	case SOAP_TYPE_string:
		soap_serialize_string(soap, (char*const*)&ptr);
		break;
	}
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_byte(struct soap *soap, char *a)
{
	(void)soap; /* appease -Wall -Werror */
#ifdef SOAP_DEFAULT_byte
	*a = SOAP_DEFAULT_byte;
#else
	*a = (char)0;
#endif
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_byte(struct soap *soap, const char *tag, int id, const char *a, const char *type)
{	(void)soap; (void)type; (void)tag; (void)id;
	return soap_outbyte(soap, tag, id, a, type, SOAP_TYPE_byte);
}

SOAP_FMAC3 char * SOAP_FMAC4 soap_in_byte(struct soap *soap, const char *tag, char *a, const char *type)
{	char *p;
	p = soap_inbyte(soap, tag, a, type, SOAP_TYPE_byte);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_byte(struct soap *soap, const char *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_byte);
	if (soap_out_byte(soap, tag?tag:"byte", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 char * SOAP_FMAC4 soap_get_byte(struct soap *soap, char *p, const char *tag, const char *type)
{
	if ((p = soap_in_byte(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_int(struct soap *soap, int *a)
{
	(void)soap; /* appease -Wall -Werror */
#ifdef SOAP_DEFAULT_int
	*a = SOAP_DEFAULT_int;
#else
	*a = (int)0;
#endif
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_int(struct soap *soap, const char *tag, int id, const int *a, const char *type)
{	(void)soap; (void)type; (void)tag; (void)id;
	return soap_outint(soap, tag, id, a, type, SOAP_TYPE_int);
}

SOAP_FMAC3 int * SOAP_FMAC4 soap_in_int(struct soap *soap, const char *tag, int *a, const char *type)
{	int *p;
	p = soap_inint(soap, tag, a, type, SOAP_TYPE_int);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_int(struct soap *soap, const int *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_int);
	if (soap_out_int(soap, tag?tag:"int", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 int * SOAP_FMAC4 soap_get_int(struct soap *soap, int *p, const char *tag, const char *type)
{
	if ((p = soap_in_int(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_double(struct soap *soap, double *a)
{
	(void)soap; /* appease -Wall -Werror */
#ifdef SOAP_DEFAULT_double
	*a = SOAP_DEFAULT_double;
#else
	*a = (double)0;
#endif
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_double(struct soap *soap, const char *tag, int id, const double *a, const char *type)
{	(void)soap; (void)type; (void)tag; (void)id;
	return soap_outdouble(soap, tag, id, a, type, SOAP_TYPE_double);
}

SOAP_FMAC3 double * SOAP_FMAC4 soap_in_double(struct soap *soap, const char *tag, double *a, const char *type)
{	double *p;
	p = soap_indouble(soap, tag, a, type, SOAP_TYPE_double);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_double(struct soap *soap, const double *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_double);
	if (soap_out_double(soap, tag?tag:"double", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 double * SOAP_FMAC4 soap_get_double(struct soap *soap, double *p, const char *tag, const char *type)
{
	if ((p = soap_in_double(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_time(struct soap *soap, time_t *a)
{
	(void)soap; /* appease -Wall -Werror */
#ifdef SOAP_DEFAULT_time
	*a = SOAP_DEFAULT_time;
#else
	*a = (time_t)0;
#endif
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_time(struct soap *soap, const char *tag, int id, const time_t *a, const char *type)
{	(void)soap; (void)type; (void)tag; (void)id;
	return soap_outdateTime(soap, tag, id, a, type, SOAP_TYPE_time);
}

SOAP_FMAC3 time_t * SOAP_FMAC4 soap_in_time(struct soap *soap, const char *tag, time_t *a, const char *type)
{	time_t *p;
	p = soap_indateTime(soap, tag, a, type, SOAP_TYPE_time);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_time(struct soap *soap, const time_t *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_time);
	if (soap_out_time(soap, tag?tag:"dateTime", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 time_t * SOAP_FMAC4 soap_get_time(struct soap *soap, time_t *p, const char *tag, const char *type)
{
	if ((p = soap_in_time(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Fault(struct soap *soap, struct SOAP_ENV__Fault *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_default__QName(soap, &a->faultcode);
	soap_default_string(soap, &a->faultstring);
	soap_default_string(soap, &a->faultactor);
	a->detail = NULL;
	a->SOAP_ENV__Code = NULL;
	a->SOAP_ENV__Reason = NULL;
	soap_default_string(soap, &a->SOAP_ENV__Node);
	soap_default_string(soap, &a->SOAP_ENV__Role);
	a->SOAP_ENV__Detail = NULL;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Fault(struct soap *soap, const struct SOAP_ENV__Fault *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_serialize__QName(soap, &a->faultcode);
	soap_serialize_string(soap, &a->faultstring);
	soap_serialize_string(soap, &a->faultactor);
	soap_serialize_PointerToSOAP_ENV__Detail(soap, &a->detail);
	soap_serialize_PointerToSOAP_ENV__Code(soap, &a->SOAP_ENV__Code);
	soap_serialize_PointerToSOAP_ENV__Reason(soap, &a->SOAP_ENV__Reason);
	soap_serialize_string(soap, &a->SOAP_ENV__Node);
	soap_serialize_string(soap, &a->SOAP_ENV__Role);
	soap_serialize_PointerToSOAP_ENV__Detail(soap, &a->SOAP_ENV__Detail);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Fault(struct soap *soap, const char *tag, int id, const struct SOAP_ENV__Fault *a, const char *type)
{
	const char *soap_tmp_faultcode = soap_QName2s(soap, a->faultcode);
	(void)soap; (void)tag; (void)id; (void)type;
	if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, SOAP_TYPE_SOAP_ENV__Fault), type))
		return soap->error;
	if (soap_out__QName(soap, "faultcode", -1, (char*const*)&soap_tmp_faultcode, ""))
		return soap->error;
	if (soap_out_string(soap, "faultstring", -1, &a->faultstring, ""))
		return soap->error;
	if (soap_out_string(soap, "faultactor", -1, &a->faultactor, ""))
		return soap->error;
	if (soap_out_PointerToSOAP_ENV__Detail(soap, "detail", -1, &a->detail, ""))
		return soap->error;
	if (soap_out_PointerToSOAP_ENV__Code(soap, "SOAP-ENV:Code", -1, &a->SOAP_ENV__Code, ""))
		return soap->error;
	if (soap_out_PointerToSOAP_ENV__Reason(soap, "SOAP-ENV:Reason", -1, &a->SOAP_ENV__Reason, ""))
		return soap->error;
	if (soap_out_string(soap, "SOAP-ENV:Node", -1, &a->SOAP_ENV__Node, ""))
		return soap->error;
	if (soap_out_string(soap, "SOAP-ENV:Role", -1, &a->SOAP_ENV__Role, ""))
		return soap->error;
	if (soap_out_PointerToSOAP_ENV__Detail(soap, "SOAP-ENV:Detail", -1, &a->SOAP_ENV__Detail, ""))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 struct SOAP_ENV__Fault * SOAP_FMAC4 soap_in_SOAP_ENV__Fault(struct soap *soap, const char *tag, struct SOAP_ENV__Fault *a, const char *type)
{
	size_t soap_flag_faultcode = 1;
	size_t soap_flag_faultstring = 1;
	size_t soap_flag_faultactor = 1;
	size_t soap_flag_detail = 1;
	size_t soap_flag_SOAP_ENV__Code = 1;
	size_t soap_flag_SOAP_ENV__Reason = 1;
	size_t soap_flag_SOAP_ENV__Node = 1;
	size_t soap_flag_SOAP_ENV__Role = 1;
	size_t soap_flag_SOAP_ENV__Detail = 1;
	if (soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	a = (struct SOAP_ENV__Fault *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_SOAP_ENV__Fault, sizeof(struct SOAP_ENV__Fault), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default_SOAP_ENV__Fault(soap, a);
	if (soap->body && !*soap->href)
	{
		for (;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if (soap_flag_faultcode && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in__QName(soap, "faultcode", &a->faultcode, ""))
				{	soap_flag_faultcode--;
					continue;
				}
			if (soap_flag_faultstring && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in_string(soap, "faultstring", &a->faultstring, "xsd:string"))
				{	soap_flag_faultstring--;
					continue;
				}
			if (soap_flag_faultactor && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in_string(soap, "faultactor", &a->faultactor, "xsd:string"))
				{	soap_flag_faultactor--;
					continue;
				}
			if (soap_flag_detail && soap->error == SOAP_TAG_MISMATCH)
				if (soap_in_PointerToSOAP_ENV__Detail(soap, "detail", &a->detail, ""))
				{	soap_flag_detail--;
					continue;
				}
			if (soap_flag_SOAP_ENV__Code && soap->error == SOAP_TAG_MISMATCH)
				if (soap_in_PointerToSOAP_ENV__Code(soap, "SOAP-ENV:Code", &a->SOAP_ENV__Code, ""))
				{	soap_flag_SOAP_ENV__Code--;
					continue;
				}
			if (soap_flag_SOAP_ENV__Reason && soap->error == SOAP_TAG_MISMATCH)
				if (soap_in_PointerToSOAP_ENV__Reason(soap, "SOAP-ENV:Reason", &a->SOAP_ENV__Reason, ""))
				{	soap_flag_SOAP_ENV__Reason--;
					continue;
				}
			if (soap_flag_SOAP_ENV__Node && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in_string(soap, "SOAP-ENV:Node", &a->SOAP_ENV__Node, "xsd:string"))
				{	soap_flag_SOAP_ENV__Node--;
					continue;
				}
			if (soap_flag_SOAP_ENV__Role && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in_string(soap, "SOAP-ENV:Role", &a->SOAP_ENV__Role, "xsd:string"))
				{	soap_flag_SOAP_ENV__Role--;
					continue;
				}
			if (soap_flag_SOAP_ENV__Detail && soap->error == SOAP_TAG_MISMATCH)
				if (soap_in_PointerToSOAP_ENV__Detail(soap, "SOAP-ENV:Detail", &a->SOAP_ENV__Detail, ""))
				{	soap_flag_SOAP_ENV__Detail--;
					continue;
				}
			if (soap->error == SOAP_TAG_MISMATCH)
				soap->error = soap_ignore_element(soap);
			if (soap->error == SOAP_NO_TAG)
				break;
			if (soap->error)
				return NULL;
		}
		if (soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Fault *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_SOAP_ENV__Fault, 0, sizeof(struct SOAP_ENV__Fault), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Fault(struct soap *soap, const struct SOAP_ENV__Fault *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_SOAP_ENV__Fault);
	if (soap_out_SOAP_ENV__Fault(soap, tag?tag:"SOAP-ENV:Fault", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Fault * SOAP_FMAC4 soap_get_SOAP_ENV__Fault(struct soap *soap, struct SOAP_ENV__Fault *p, const char *tag, const char *type)
{
	if ((p = soap_in_SOAP_ENV__Fault(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Reason(struct soap *soap, struct SOAP_ENV__Reason *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_default_string(soap, &a->SOAP_ENV__Text);
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Reason(struct soap *soap, const struct SOAP_ENV__Reason *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_serialize_string(soap, &a->SOAP_ENV__Text);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Reason(struct soap *soap, const char *tag, int id, const struct SOAP_ENV__Reason *a, const char *type)
{
	(void)soap; (void)tag; (void)id; (void)type;
	if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, SOAP_TYPE_SOAP_ENV__Reason), type))
		return soap->error;
	if (soap->lang)
		soap_set_attr(soap, "xml:lang", soap->lang, 1);
	if (soap_out_string(soap, "SOAP-ENV:Text", -1, &a->SOAP_ENV__Text, ""))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 struct SOAP_ENV__Reason * SOAP_FMAC4 soap_in_SOAP_ENV__Reason(struct soap *soap, const char *tag, struct SOAP_ENV__Reason *a, const char *type)
{
	size_t soap_flag_SOAP_ENV__Text = 1;
	if (soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	a = (struct SOAP_ENV__Reason *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_SOAP_ENV__Reason, sizeof(struct SOAP_ENV__Reason), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default_SOAP_ENV__Reason(soap, a);
	if (soap->body && !*soap->href)
	{
		for (;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if (soap_flag_SOAP_ENV__Text && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in_string(soap, "SOAP-ENV:Text", &a->SOAP_ENV__Text, "xsd:string"))
				{	soap_flag_SOAP_ENV__Text--;
					continue;
				}
			if (soap->error == SOAP_TAG_MISMATCH)
				soap->error = soap_ignore_element(soap);
			if (soap->error == SOAP_NO_TAG)
				break;
			if (soap->error)
				return NULL;
		}
		if (soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Reason *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_SOAP_ENV__Reason, 0, sizeof(struct SOAP_ENV__Reason), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Reason(struct soap *soap, const struct SOAP_ENV__Reason *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_SOAP_ENV__Reason);
	if (soap_out_SOAP_ENV__Reason(soap, tag?tag:"SOAP-ENV:Reason", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Reason * SOAP_FMAC4 soap_get_SOAP_ENV__Reason(struct soap *soap, struct SOAP_ENV__Reason *p, const char *tag, const char *type)
{
	if ((p = soap_in_SOAP_ENV__Reason(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Detail(struct soap *soap, struct SOAP_ENV__Detail *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	a->__any = NULL;
	a->__type = 0;
	a->fault = NULL;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Detail(struct soap *soap, const struct SOAP_ENV__Detail *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_markelement(soap, a->fault, a->__type);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Detail(struct soap *soap, const char *tag, int id, const struct SOAP_ENV__Detail *a, const char *type)
{
	(void)soap; (void)tag; (void)id; (void)type;
	if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, SOAP_TYPE_SOAP_ENV__Detail), type))
		return soap->error;
	soap_outliteral(soap, "-any", &a->__any, NULL);
	if (soap_putelement(soap, a->fault, "fault", -1, a->__type))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 struct SOAP_ENV__Detail * SOAP_FMAC4 soap_in_SOAP_ENV__Detail(struct soap *soap, const char *tag, struct SOAP_ENV__Detail *a, const char *type)
{
	size_t soap_flag___any = 1;
	size_t soap_flag_fault = 1;
	if (soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	a = (struct SOAP_ENV__Detail *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_SOAP_ENV__Detail, sizeof(struct SOAP_ENV__Detail), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default_SOAP_ENV__Detail(soap, a);
	if (soap->body && !*soap->href)
	{
		for (;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if (soap_flag_fault && soap->error == SOAP_TAG_MISMATCH)
				if ((a->fault = soap_getelement(soap, &a->__type)))
				{	soap_flag_fault = 0;
					continue;
				}
			if (soap_flag___any && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_inliteral(soap, "-any", &a->__any))
				{	soap_flag___any--;
					continue;
				}
			if (soap->error == SOAP_TAG_MISMATCH)
				soap->error = soap_ignore_element(soap);
			if (soap->error == SOAP_NO_TAG)
				break;
			if (soap->error)
				return NULL;
		}
		if (soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Detail *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_SOAP_ENV__Detail, 0, sizeof(struct SOAP_ENV__Detail), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Detail(struct soap *soap, const struct SOAP_ENV__Detail *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_SOAP_ENV__Detail);
	if (soap_out_SOAP_ENV__Detail(soap, tag?tag:"SOAP-ENV:Detail", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Detail * SOAP_FMAC4 soap_get_SOAP_ENV__Detail(struct soap *soap, struct SOAP_ENV__Detail *p, const char *tag, const char *type)
{
	if ((p = soap_in_SOAP_ENV__Detail(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Code(struct soap *soap, struct SOAP_ENV__Code *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_default__QName(soap, &a->SOAP_ENV__Value);
	a->SOAP_ENV__Subcode = NULL;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Code(struct soap *soap, const struct SOAP_ENV__Code *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_serialize__QName(soap, &a->SOAP_ENV__Value);
	soap_serialize_PointerToSOAP_ENV__Code(soap, &a->SOAP_ENV__Subcode);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Code(struct soap *soap, const char *tag, int id, const struct SOAP_ENV__Code *a, const char *type)
{
	const char *soap_tmp_SOAP_ENV__Value = soap_QName2s(soap, a->SOAP_ENV__Value);
	(void)soap; (void)tag; (void)id; (void)type;
	if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, SOAP_TYPE_SOAP_ENV__Code), type))
		return soap->error;
	if (soap_out__QName(soap, "SOAP-ENV:Value", -1, (char*const*)&soap_tmp_SOAP_ENV__Value, ""))
		return soap->error;
	if (soap_out_PointerToSOAP_ENV__Code(soap, "SOAP-ENV:Subcode", -1, &a->SOAP_ENV__Subcode, ""))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 struct SOAP_ENV__Code * SOAP_FMAC4 soap_in_SOAP_ENV__Code(struct soap *soap, const char *tag, struct SOAP_ENV__Code *a, const char *type)
{
	size_t soap_flag_SOAP_ENV__Value = 1;
	size_t soap_flag_SOAP_ENV__Subcode = 1;
	if (soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	a = (struct SOAP_ENV__Code *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_SOAP_ENV__Code, sizeof(struct SOAP_ENV__Code), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default_SOAP_ENV__Code(soap, a);
	if (soap->body && !*soap->href)
	{
		for (;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if (soap_flag_SOAP_ENV__Value && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if (soap_in__QName(soap, "SOAP-ENV:Value", &a->SOAP_ENV__Value, ""))
				{	soap_flag_SOAP_ENV__Value--;
					continue;
				}
			if (soap_flag_SOAP_ENV__Subcode && soap->error == SOAP_TAG_MISMATCH)
				if (soap_in_PointerToSOAP_ENV__Code(soap, "SOAP-ENV:Subcode", &a->SOAP_ENV__Subcode, ""))
				{	soap_flag_SOAP_ENV__Subcode--;
					continue;
				}
			if (soap->error == SOAP_TAG_MISMATCH)
				soap->error = soap_ignore_element(soap);
			if (soap->error == SOAP_NO_TAG)
				break;
			if (soap->error)
				return NULL;
		}
		if (soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Code *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_SOAP_ENV__Code, 0, sizeof(struct SOAP_ENV__Code), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Code(struct soap *soap, const struct SOAP_ENV__Code *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_SOAP_ENV__Code);
	if (soap_out_SOAP_ENV__Code(soap, tag?tag:"SOAP-ENV:Code", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Code * SOAP_FMAC4 soap_get_SOAP_ENV__Code(struct soap *soap, struct SOAP_ENV__Code *p, const char *tag, const char *type)
{
	if ((p = soap_in_SOAP_ENV__Code(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Header(struct soap *soap, struct SOAP_ENV__Header *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Header(struct soap *soap, const struct SOAP_ENV__Header *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Header(struct soap *soap, const char *tag, int id, const struct SOAP_ENV__Header *a, const char *type)
{
	(void)soap; (void)tag; (void)id; (void)type;
	if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, SOAP_TYPE_SOAP_ENV__Header), type))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 struct SOAP_ENV__Header * SOAP_FMAC4 soap_in_SOAP_ENV__Header(struct soap *soap, const char *tag, struct SOAP_ENV__Header *a, const char *type)
{
	if (soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	pub_log_info("[%s][%d] begin in [%s] success!", __FILE__, __LINE__, tag);
	a = (struct SOAP_ENV__Header *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_SOAP_ENV__Header, sizeof(struct SOAP_ENV__Header), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default_SOAP_ENV__Header(soap, a);
	if (soap->body && !*soap->href)
	{
		for (;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if (soap->error == SOAP_TAG_MISMATCH)
				soap->error = soap_ignore_element(soap);
			if (soap->error == SOAP_NO_TAG)
				break;
			if (soap->error)
				return NULL;
		}
		if (soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Header *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_SOAP_ENV__Header, 0, sizeof(struct SOAP_ENV__Header), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Header(struct soap *soap, const struct SOAP_ENV__Header *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_SOAP_ENV__Header);
	if (soap_out_SOAP_ENV__Header(soap, tag?tag:"SOAP-ENV:Header", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Header * SOAP_FMAC4 soap_get_SOAP_ENV__Header(struct soap *soap, struct SOAP_ENV__Header *p, const char *tag, const char *type)
{
	if ((p = soap_in_SOAP_ENV__Header(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerToSOAP_ENV__Reason(struct soap *soap, struct SOAP_ENV__Reason *const*a)
{
	if (!soap_reference(soap, *a, SOAP_TYPE_SOAP_ENV__Reason))
		soap_serialize_SOAP_ENV__Reason(soap, *a);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerToSOAP_ENV__Reason(struct soap *soap, const char *tag, int id, struct SOAP_ENV__Reason *const*a, const char *type)
{
	id = soap_element_id(soap, tag, id, *a, NULL, 0, type, SOAP_TYPE_SOAP_ENV__Reason);
	if (id < 0)
		return soap->error;
	return soap_out_SOAP_ENV__Reason(soap, tag, id, *a, type);
}

SOAP_FMAC3 struct SOAP_ENV__Reason ** SOAP_FMAC4 soap_in_PointerToSOAP_ENV__Reason(struct soap *soap, const char *tag, struct SOAP_ENV__Reason **a, const char *type)
{
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
	if (!a)
		if (!(a = (struct SOAP_ENV__Reason **)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason *))))
			return NULL;
	*a = NULL;
	if (!soap->null && *soap->href != '#')
	{	soap_revert(soap);
		*a = (struct SOAP_ENV__Reason *)soap_malloc(soap, sizeof(struct SOAP_ENV__Reason));
		soap_default_SOAP_ENV__Reason(soap, *a);
		if (!(*a = soap_in_SOAP_ENV__Reason(soap, tag, *a, type)))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Reason **)soap_id_lookup(soap, soap->href, (void**)a, SOAP_TYPE_SOAP_ENV__Reason, sizeof(struct SOAP_ENV__Reason), 0);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerToSOAP_ENV__Reason(struct soap *soap, struct SOAP_ENV__Reason *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_PointerToSOAP_ENV__Reason);
	if (soap_out_PointerToSOAP_ENV__Reason(soap, tag?tag:"SOAP-ENV:Reason", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Reason ** SOAP_FMAC4 soap_get_PointerToSOAP_ENV__Reason(struct soap *soap, struct SOAP_ENV__Reason **p, const char *tag, const char *type)
{
	if ((p = soap_in_PointerToSOAP_ENV__Reason(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerToSOAP_ENV__Detail(struct soap *soap, struct SOAP_ENV__Detail *const*a)
{
	if (!soap_reference(soap, *a, SOAP_TYPE_SOAP_ENV__Detail))
		soap_serialize_SOAP_ENV__Detail(soap, *a);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerToSOAP_ENV__Detail(struct soap *soap, const char *tag, int id, struct SOAP_ENV__Detail *const*a, const char *type)
{
	id = soap_element_id(soap, tag, id, *a, NULL, 0, type, SOAP_TYPE_SOAP_ENV__Detail);
	if (id < 0)
		return soap->error;
	return soap_out_SOAP_ENV__Detail(soap, tag, id, *a, type);
}

SOAP_FMAC3 struct SOAP_ENV__Detail ** SOAP_FMAC4 soap_in_PointerToSOAP_ENV__Detail(struct soap *soap, const char *tag, struct SOAP_ENV__Detail **a, const char *type)
{
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
	if (!a)
		if (!(a = (struct SOAP_ENV__Detail **)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail *))))
			return NULL;
	*a = NULL;
	if (!soap->null && *soap->href != '#')
	{	soap_revert(soap);
		*a = (struct SOAP_ENV__Detail *)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
		soap_default_SOAP_ENV__Detail(soap, *a);
		if (!(*a = soap_in_SOAP_ENV__Detail(soap, tag, *a, type)))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Detail **)soap_id_lookup(soap, soap->href, (void**)a, SOAP_TYPE_SOAP_ENV__Detail, sizeof(struct SOAP_ENV__Detail), 0);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerToSOAP_ENV__Detail(struct soap *soap, struct SOAP_ENV__Detail *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_PointerToSOAP_ENV__Detail);
	if (soap_out_PointerToSOAP_ENV__Detail(soap, tag?tag:"SOAP-ENV:Detail", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Detail ** SOAP_FMAC4 soap_get_PointerToSOAP_ENV__Detail(struct soap *soap, struct SOAP_ENV__Detail **p, const char *tag, const char *type)
{
	if ((p = soap_in_PointerToSOAP_ENV__Detail(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerToSOAP_ENV__Code(struct soap *soap, struct SOAP_ENV__Code *const*a)
{
	if (!soap_reference(soap, *a, SOAP_TYPE_SOAP_ENV__Code))
		soap_serialize_SOAP_ENV__Code(soap, *a);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerToSOAP_ENV__Code(struct soap *soap, const char *tag, int id, struct SOAP_ENV__Code *const*a, const char *type)
{
	id = soap_element_id(soap, tag, id, *a, NULL, 0, type, SOAP_TYPE_SOAP_ENV__Code);
	if (id < 0)
		return soap->error;
	return soap_out_SOAP_ENV__Code(soap, tag, id, *a, type);
}

SOAP_FMAC3 struct SOAP_ENV__Code ** SOAP_FMAC4 soap_in_PointerToSOAP_ENV__Code(struct soap *soap, const char *tag, struct SOAP_ENV__Code **a, const char *type)
{
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
	if (!a)
		if (!(a = (struct SOAP_ENV__Code **)soap_malloc(soap, sizeof(struct SOAP_ENV__Code *))))
			return NULL;
	*a = NULL;
	if (!soap->null && *soap->href != '#')
	{	soap_revert(soap);
		*a = (struct SOAP_ENV__Code *)soap_malloc(soap, sizeof(struct SOAP_ENV__Code));
		soap_default_SOAP_ENV__Code(soap, *a);
		if (!(*a = soap_in_SOAP_ENV__Code(soap, tag, *a, type)))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Code **)soap_id_lookup(soap, soap->href, (void**)a, SOAP_TYPE_SOAP_ENV__Code, sizeof(struct SOAP_ENV__Code), 0);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerToSOAP_ENV__Code(struct soap *soap, struct SOAP_ENV__Code *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_PointerToSOAP_ENV__Code);
	if (soap_out_PointerToSOAP_ENV__Code(soap, tag?tag:"SOAP-ENV:Code", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Code ** SOAP_FMAC4 soap_get_PointerToSOAP_ENV__Code(struct soap *soap, struct SOAP_ENV__Code **p, const char *tag, const char *type)
{
	if ((p = soap_in_PointerToSOAP_ENV__Code(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerTodouble(struct soap *soap, double *const*a)
{
	soap_reference(soap, *a, SOAP_TYPE_double);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerTodouble(struct soap *soap, const char *tag, int id, double *const*a, const char *type)
{
	id = soap_element_id(soap, tag, id, *a, NULL, 0, type, SOAP_TYPE_double);
	if (id < 0)
		return soap->error;
	return soap_out_double(soap, tag, id, *a, type);
}

SOAP_FMAC3 double ** SOAP_FMAC4 soap_in_PointerTodouble(struct soap *soap, const char *tag, double **a, const char *type)
{
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
	if (!a)
		if (!(a = (double **)soap_malloc(soap, sizeof(double *))))
			return NULL;
	*a = NULL;
	if (!soap->null && *soap->href != '#')
	{	soap_revert(soap);
		if (!(*a = soap_in_double(soap, tag, *a, type)))
			return NULL;
	}
	else
	{	a = (double **)soap_id_lookup(soap, soap->href, (void**)a, SOAP_TYPE_double, sizeof(double), 0);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerTodouble(struct soap *soap, double *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_PointerTodouble);
	if (soap_out_PointerTodouble(soap, tag?tag:"double", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 double ** SOAP_FMAC4 soap_get_PointerTodouble(struct soap *soap, double **p, const char *tag, const char *type)
{
	if ((p = soap_in_PointerTodouble(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerTotime(struct soap *soap, time_t *const*a)
{
	soap_reference(soap, *a, SOAP_TYPE_time);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerTotime(struct soap *soap, const char *tag, int id, time_t *const*a, const char *type)
{
	id = soap_element_id(soap, tag, id, *a, NULL, 0, type, SOAP_TYPE_time);
	if (id < 0)
		return soap->error;
	return soap_out_time(soap, tag, id, *a, type);
}

SOAP_FMAC3 time_t ** SOAP_FMAC4 soap_in_PointerTotime(struct soap *soap, const char *tag, time_t **a, const char *type)
{
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
	if (!a)
		if (!(a = (time_t **)soap_malloc(soap, sizeof(time_t *))))
			return NULL;
	*a = NULL;
	if (!soap->null && *soap->href != '#')
	{	soap_revert(soap);
		if (!(*a = soap_in_time(soap, tag, *a, type)))
			return NULL;
	}
	else
	{	a = (time_t **)soap_id_lookup(soap, soap->href, (void**)a, SOAP_TYPE_time, sizeof(time_t), 0);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerTotime(struct soap *soap, time_t *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_PointerTotime);
	if (soap_out_PointerTotime(soap, tag?tag:"dateTime", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 time_t ** SOAP_FMAC4 soap_get_PointerTotime(struct soap *soap, time_t **p, const char *tag, const char *type)
{
	if ((p = soap_in_PointerTotime(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out__QName(struct soap *soap, const char *tag, int id, char *const*a, const char *type)
{
	return soap_outstring(soap, tag, id, a, type, SOAP_TYPE__QName);
}

SOAP_FMAC3 char * * SOAP_FMAC4 soap_in__QName(struct soap *soap, const char *tag, char **a, const char *type)
{	char **p;
	p = soap_instring(soap, tag, a, type, SOAP_TYPE__QName, 2, 0, -1);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put__QName(struct soap *soap, char *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE__QName);
	if (soap_out__QName(soap, tag?tag:"byte", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 char ** SOAP_FMAC4 soap_get__QName(struct soap *soap, char **p, const char *tag, const char *type)
{
	if ((p = soap_in__QName(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_string(struct soap *soap, char **a)
{
	(void)soap; /* appease -Wall -Werror */
#ifdef SOAP_DEFAULT_string
	*a = SOAP_DEFAULT_string;
#else
	*a = (char *)0;
#endif
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_string(struct soap *soap, char *const*a)
{
	soap_reference(soap, *a, SOAP_TYPE_string);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_string(struct soap *soap, const char *tag, int id, char *const*a, const char *type)
{
	return soap_outstring(soap, tag, id, a, type, SOAP_TYPE_string);
}

SOAP_FMAC3 char * * SOAP_FMAC4 soap_in_string(struct soap *soap, const char *tag, char **a, const char *type)
{	char **p;
	p = soap_instring(soap, tag, a, type, SOAP_TYPE_string, 1, 0, -1);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_string(struct soap *soap, char *const*a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_string);
	if (soap_out_string(soap, tag?tag:"byte", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 char ** SOAP_FMAC4 soap_get_string(struct soap *soap, char **p, const char *tag, const char *type)
{
	if ((p = soap_in_string(soap, tag, p, type)))
		if (soap_getindependent(soap))
			return NULL;
	return p;
}

int soap_set_value(struct soap *soap, sw_loc_vars_t *vars, item_t item)
{
	int	alloc = 0;
	int allocbuf = 0;
	char	*p = NULL;
	char	*ptr = NULL;
	size_t	inlen = 0;
	size_t	outlen = 0;
	char	value[MAX_SOAP_VALUE_LEN];
	char	buf[MAX_SOAP_VALUE_LEN];
	char	*tmp = NULL;
	
	ptr = item.value;
	if (ptr == NULL)
	{
		return SW_OK;
	}
	memset(buf, 0x0, sizeof(buf));
	tmp = buf;
	
	alloc = 0;
	memset(value, 0x0, sizeof(value));
	if (ptr[0] != '\0' && strcmp(item.type, "xsd:string") == 0 && !(soap->mode & SOAP_ENC_LATIN))
	{
		inlen = strlen(ptr);
		outlen = inlen * 2;
		if (outlen > MAX_SOAP_VALUE_LEN)
		{
			p = (char *)calloc(1, outlen + 1);
			if (p == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
					__FILE__, __LINE__, outlen, errno, strerror(errno));
				return soap->error = SOAP_ERR;
			}
			alloc = 1;
		}
		else
		{
			p = value;
		}
		if (pub_code_u2g(ptr, &inlen, p, &outlen))
		{
			pub_log_error("[%s][%d] u2g error! name=[%s] value=[%s]",
				__FILE__, __LINE__, item.mname, ptr);
			loc_set_zd_data(vars, item.name, ptr);
			if (pub_is_filter(item.name))
			{
				memset(tmp, '*', inlen);
				pub_log_info("[%s][%d] 拆出变量 [%s]=>[%s]=[%s]", 
					__FILE__, __LINE__, item.mname, item.name, tmp);
			}
			else
			{
				pub_log_info("[%s][%d] 拆出变量 [%s]=>[%s]=[%s]",
					__FILE__, __LINE__, item.mname, item.name, ptr);
			}
		}
		else
		{
			loc_set_zd_data(vars, item.name, p);
			if (pub_is_filter(item.name))
			{
				if (strlen(p) > MAX_SOAP_VALUE_LEN)
				{
					tmp = (char *)calloc(1, strlen(p) + 1);
					if (tmp == NULL)
					{
						pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
							__FILE__, __LINE__, strlen(p) + 1, errno, strerror(errno));
						return soap->error = SOAP_ERR;
					}
					allocbuf = 1;
				}
				
				memset(tmp, '*', strlen(p));
				pub_log_info("[%s][%d] 拆出变量 [%s]=>[%s]=[%s]", 
					__FILE__, __LINE__, item.mname, item.name, tmp);
			}
			else
			{
				pub_log_info("[%s][%d] 拆出变量 [%s]=>[%s]=[%s]",
					__FILE__, __LINE__, item.mname, item.name, p);
			}
		}
		if (alloc)
		{
			free(p);
		}
		
		if (allocbuf)
		{
			free(tmp);
		}
	}
	else
	{
		loc_set_zd_data(vars, item.name, ptr);
		if (pub_is_filter(item.name))
		{
			memset(tmp, '*', strlen(ptr));
			pub_log_info("[%s][%d] 拆出变量 [%s]=>[%s]=[%s]", 
				__FILE__, __LINE__, item.mname, item.name, tmp);
		}
		else
		{
			pub_log_info("[%s][%d] 拆出变量 [%s]=>[%s]=[%s]",
				__FILE__, __LINE__, item.mname, item.name, ptr);
		}
	}

	return SW_OK;
}

int soap_get_value(struct soap *soap, sw_loc_vars_t *vars, item_t item)
{
	int	alloc = 0;
	char	*p = NULL;
	char	*ptr = NULL;
	size_t	inlen = 0;
	size_t	outlen = 0;
	sw_int32_t	len = 0;
	char	value[MAX_SOAP_VALUE_LEN];
	
	alloc = 0;
	memset(value, 0x0, sizeof(value));
	ptr = vars->get_var_addr(vars, item.name, &len);
	if (ptr != NULL)
	{
		if (pub_is_filter(item.name))
		{
			char buf[MAX_SOAP_VALUE_LEN];
			memset(buf, 0x0, sizeof(buf));
			memset(buf, '*', len);
			pub_log_info("[%s][%d] 取平台变量[%s]=[%s][%d]", __FILE__, __LINE__, item.name, buf, len);
		}
		else
		{
			pub_log_info("[%s][%d] 取平台变量[%s]=[%s][%d]", __FILE__, __LINE__, item.name, ptr, len);
		}
		
		if (len > 0 && strcmp(item.type, "xsd:string") == 0 && !(soap->mode & SOAP_ENC_LATIN))
		{
			inlen = len;
			outlen = inlen * 2;
			if (outlen > MAX_SOAP_VALUE_LEN)
			{
				p = (char *)calloc(1, outlen + 1);
				if (p == NULL)
				{
					pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
						__FILE__, __LINE__, outlen, errno, strerror(errno));
					return soap->error = SOAP_ERR;
				}
				alloc = 1;
			}
			else
			{
				p = value;
			}
			if (pub_code_g2u(ptr, &inlen, p, &outlen))
			{
				pub_log_error("[%s][%d] g2u error! name=[%s] value=[%s]",
					__FILE__, __LINE__, item.mname, ptr);
			}
			else
			{
				ptr = p;
			}
			if (soap_out_string(soap, item.mname, -1, &ptr, ""))
			{
				if (alloc)
				{
					free(p);
				}
				return soap->error;
			}
			if (alloc)
			{
				free(p);
			}
		}
		else if (soap_out_string(soap, item.mname, -1, &ptr, ""))
		{
			return soap->error;
		}
	}
	else if (strcmp(item.type, "xsd:string") == 0)
	{
#if 0
		if (soap_element_nil(soap, item.mname))
		{
			return soap->error; 
		}
#endif
		ptr = "";
		if (soap_out_string(soap, item.mname, -1, &ptr, ""))
		{
			return soap->error;
		}
	}
	
	return SOAP_OK;
}

int soap_match_tag_ext(struct soap  *soap, char *var, char *str, int flag)
{
	char    *ptr = NULL;
	char    *ptmp = NULL;
	char	tmp[512];
	char    value[512];
	const char      sep = ' ';
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(value, 0x0, sizeof(value));

	ptr = str;
	ptmp = value;
	while (*ptr != '\0')
	{
		if (*ptr == sep)
		{
			*ptmp = '\0';
			ptr++;
			memset(tmp, 0x0, sizeof(tmp));
			if (flag == 1 && strchr(value, ':') == NULL)
			{
				sprintf(tmp, "%s:%s", SW_DEFAULT_NS, value);
			}
			else
			{
				strcpy(tmp, value);
			}
			if (soap_match_tag(soap, var, tmp) == 0)
			{
				return 0;
			}
			memset(value, 0x0, sizeof(value));
			ptmp = value;
			continue;
		}

		*ptmp++ = *ptr++;
	}
	memset(tmp, 0x0, sizeof(tmp));
	if (flag == 1 && strchr(value, ':') == NULL)
	{
		sprintf(tmp, "%s:%s", SW_DEFAULT_NS, value);
	}
	else
	{
		strcpy(tmp, value);
	}
	if (soap_match_tag(soap, var, tmp) == 0)
	{
		return 0;                                                                                                          
	}                                                                                                                          

	return -1;                                                                                                                 
}

static int set_method(sw_loc_vars_t *vars, const char *method)
{
	char	*ptr = NULL;
	
	if (vars == NULL || method == NULL)
	{
		return SW_ERROR;
	}
	
	ptr = strchr(method, ':');
	if (ptr)
	{
		ptr++;
	}
	else
	{
		ptr = method;
	}
	loc_set_zd_data(vars, "$soapmethod", ptr);
	pub_log_info("[%s][%d] Method=[%s]", __FILE__, __LINE__, ptr);

	return SW_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_request(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	char	firstname[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(firstname, 0x0, sizeof(firstname));
	soap_peek_element(soap);
	DBGLOG(TEST, SOAP_MESSAGE(fdebug, "[%s][%d] tag=[%s]\n", __FILE__, __LINE__, soap->tag));
	pub_log_info("[%s][%d] soap->tag=[%s]", __FILE__, __LINE__, soap->tag);
	
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(firstname, node->value);
	}
	
	node = pub_xml_locnode(xml, ".CBM.ANALYZE.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, firstname, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;                                                                                 
				continue;                                                                                          
			}
		}
		
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			if (!soap_match_tag_ext(soap, soap->tag, node1->value, 1))
			{
				break;
			}
		}
	
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Can not found server [%s]!", __FILE__, __LINE__, soap->tag);
		return soap->error = SOAP_NO_METHOD;
	}
	
	if (soap_server_unpack(soap, vars, xml, soap->tag))
	{
		return soap->error = SOAP_NO_METHOD;
	}

	return soap->error = SOAP_OK;
}

int soap_in_loop(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_xmlnode_t *pnode, item_t uitem)
{
	int	i = 0;
	char	*p = NULL;
	char	*ptr = NULL;
	char	bit[MAX_ITEM_CNT+1];
	char	buf[128];
	char	tmp[128];
	char	name[256];
	item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*snode = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	
	memset(bit, 0x0, sizeof(bit));
	memset(buf, 0x0, sizeof(buf));
	memset(tmp, 0x0, sizeof(tmp));
	memset(name, 0x0, sizeof(name));
	memset(&item, 0x0, sizeof(item));
	memset(bit, '1', MAX_ITEM_CNT);
	nodebak = xml->current;
	
	if (soap_element_begin_in(soap, uitem.mname, 0, ""))
	{
		pub_log_error("[%s][%d] soap_element_begin_in tag=[%s]", __FILE__, __LINE__, uitem.mname);
		xml->current = nodebak;
		return SW_ERROR;
	}

	if (soap->body == NULL)
	{
		pub_log_debug("[%s][%d] struct [%s]'s body is null", __FILE__, __LINE__, uitem.mname);
		return -2;
	}

	if (soap->body && !*soap->href)
	{
		node = pub_xml_locnode(xml, ".CBM.ANALYZE.STRUCT");
		while (node != NULL)
		{
			if (strcmp(node->name, "STRUCT") != 0)
			{
				node = node->next;
				continue;
			}
			xml->current = node;
			node1 = pub_xml_locnode(xml, "NAME");
			if (node1 != NULL && node1->value != NULL)
			{
				if (strcmp(node1->value, uitem.tname) == 0)
				{
					break;
				}
			}
			node = node->next;
		}
		if (node == NULL)
		{
			xml->current = nodebak;
			pub_log_error("[%s][%d] Not defined struct [%s]!",
				__FILE__, __LINE__, uitem.tname);
			return soap->error = SOAP_ERR;
		}
		snode = node;

		while (1)
		{
			soap->error = SOAP_TAG_MISMATCH;
			if (!soap_element_begin_in(soap, soap->tag, 1, NULL))
			{
				i = 0;
				xml->current = snode;
				node = pub_xml_locnode(xml, "ITEM");
				while (node != NULL)
				{
					if (strcmp(node->name, "ITEM") != 0)
					{
						node = node->next;
						continue;
					}
			
					if (bit[i] == '0')
					{
						node = node->next;
						i++;
						continue;
					}
							
					memset(&item, 0x0, sizeof(item));
					xml->current = node;
					if (get_com_item(xml, &item))
					{
						pub_log_error("[%s][%d] Get item info error!", __FILE__, __LINE__);
						return soap->error = SOAP_ERR;
					}
					
					p = strchr(soap->tag, ':');
					if (p != NULL)
					{
						p += 1;
					}
					else
					{
						p = soap->tag;
					}
					if (strcmp(item.mname, p) == 0)
					{
						bit[i] = '0'; 
						break;
					}
					
					node = node->next;
					i++;
				}

				if (node == NULL)
				{
					pub_log_info("[%s][%d] Not config [%s]!", __FILE__, __LINE__, soap->tag);
					if (soap->body)
					{
						soap_element_end_in(soap, soap->tag);
					}
					continue;
				}

				if ((strcmp(item.type, "xsd:file") == 0 && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_OK))
					|| soap->error == SOAP_NO_TAG)
				{
					pub_log_info("[%s][%d] Get the attachment name:[%s]", 
						__FILE__, __LINE__, item.mname);
					struct x__Data *filedata = NULL;
					get_mime_data(soap, &filedata, item.mname);
					if (uitem.loop == 1)
					{
							sprintf(item.path, "%s(%d)", item.name, uitem.loopcnt);
					} 
					else
					{
							strcpy(item.path, item.name);
					}
					after_get_mime(soap, filedata, item.path);
					soap->error = SOAP_TAG_MISMATCH;
					continue;
				}
				
				if (item.absflag == 0 && uitem.absflag == 1)
				{
					item.absflag = 1;
				}
				
				if (strcmp(item.type, "struct") != 0 &&
					(soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG || soap->error == SOAP_OK))
				{
					if (soap_in_string(soap, item.mname, &ptr, "xsd:string"))
					{
						nflag = 1;
						memset(name, 0x0, sizeof(name));
						if (uitem.loop == 0 && uitem.absflag == 0)
						{
							sprintf(name, "%s", item.name);
						}
						else if (uitem.loop == 1 && uitem.absflag == 0)
						{
							sprintf(name, "%s%s", item.name, uitem.path);
						}
						else
						{
							sprintf(name, "%s%s", uitem.path, item.name + 1);
						}
						memset(item.name, 0x0, sizeof(item.name));
						strcpy(item.name, name);
						item.value = ptr;
						if (soap_set_value(soap, vars, item))
						{
							pub_log_error("[%s][%d] soap_set_value error!", __FILE__, __LINE__);
							return soap->error;
						}
					}
					
					soap->error = SOAP_TAG_MISMATCH;
					continue;
				}
				
				if (strcmp(item.type, "struct") == 0)
				{
					item.loopcnt = 0;
AGAIN:
					memset(item.path, 0x0, sizeof(item.path));
					if (uitem.absflag == 1)
					{
						if (item.loop == 1)
						{
							sprintf(item.path, "%s%s(%d)", uitem.path, item.name + 1, item.loopcnt);
						}
						else if (item.loop == 0)
						{
							sprintf(item.path, "%s%s_", uitem.path, item.name + 1);
						}
					}
					else if (item.loop == 1 && item.absflag == 1)
					{
						sprintf(item.path, "%s(%d)", item.name, item.loopcnt);
					}
					else if (item.loop == 1 && item.absflag == 0)
					{
						sprintf(item.path, "(%d)", item.loopcnt);
					}
					if (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_OK)
					{
						soap_revert(soap);
						if (!soap_in_loop(soap, vars, xml, node, item))                         
						{
							if (item.loop == 0)
							{
								continue;
							}
							item.loopcnt++;
							if (!soap_element_begin_in(soap, item.mname, 1, NULL))
							{
								soap->error = SOAP_TAG_MISMATCH;
								goto AGAIN;
							}
						}
					}
				}
				if (item.loopcnt > 0)
				{
					if (item.loopvar[0] != '\0')
					{
						memset(buf, 0x0, sizeof(buf));
						sprintf(buf, "%d", item.loopcnt);
						memset(tmp, 0x0, sizeof(tmp));
						if (uitem.loop == 0 && uitem.absflag == 0)
						{
							sprintf(tmp, "%s", item.loopvar);
						}
						else if (uitem.loop == 1 && uitem.absflag == 0)
						{
							sprintf(tmp, "%s%s", item.loopvar, uitem.path);
						}
						else
						{
							sprintf(tmp, "%s%s", uitem.path, item.loopvar + 1);
						}
						loc_set_zd_data(vars, tmp, buf);
						pub_log_info("[%s][%d] 设置变量[%s]=[%s]", __FILE__, __LINE__, tmp, buf);
					}
				}
				soap->error = SOAP_TAG_MISMATCH;
				continue;
			}
				
			if (soap->error == SOAP_TAG_MISMATCH)
			{
				soap->error = soap_ignore_element(soap);
			}

			if (soap->error == SOAP_NO_TAG)
			{
				break;
			}

			if (soap->error)
			{
				pub_log_info("[%s][%d] name=[%s] error=[%d]", __FILE__, __LINE__, item.mname, soap->error);
				return soap->error;
			}
		}
		if (soap_element_end_in(soap, uitem.mname))
		{
			pub_log_error("[%s][%d] soap_element_end_in [%s] error!", __FILE__, __LINE__, uitem.mname);
			return soap->error;
		}
	}
	pub_log_info("[%s][%d] Unpack struct [%s] success!", __FILE__, __LINE__, uitem.mname);
	xml->current = nodebak;

	return SW_OK;
}

SOAP_FMAC3 int soap_in_unpack_var(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *tag, const char *type, int flag)
{
	int	i = 0;
	int 	len = 0;
	int 	ret = 0;
	char	bit[MAX_ITEM_CNT+1];
	char	*p = NULL;
	char	*ptr = NULL;
	char	buf[128];
	char	ttag[128];
	char	firstname[128];
	char	servertag[128];
	item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	
	memset(bit, 0x0, sizeof(bit));
	memset(buf, 0x0, sizeof(buf));
	memset(ttag, 0x0, sizeof(ttag));
	memset(firstname, 0x0, sizeof(firstname));
	memset(servertag, 0x0, sizeof(servertag));
	memset(&item, 0x0, sizeof(item));
	memset(bit, '1', MAX_ITEM_CNT);

	soap_set_gvars(vars);
	farray_reset(&g_farray);

	nflag = 0;
	if (flag == SW_SOAP_CLI)
	{
		soap_peek_element(soap);
		sprintf(servertag, "%s", soap->tag);
		pub_log_info("[%s][%d] soap->tag=[%s]", __FILE__, __LINE__, soap->tag);
	}
	else
	{
		strcpy(servertag, tag);
	}
	p = strchr(servertag, ':');
	if (p != NULL)
	{
		strcpy(ttag, p + 1);
	}
	else
	{
		strcpy(ttag, servertag);
	}
	
	if (soap_element_begin_in(soap, servertag, 0, type))
	{
		pub_log_error("[%s][%d] soap_element_begin_in [%s] error! soap->error=[%d]",
			__FILE__, __LINE__, servertag, soap->error);
		return soap->error;
	}
	
	if (soap->body && !*soap->href)
	{

		node = pub_xml_locnode(xml, ".CBM.ANALYZE.FIRSTWORK.ITEM.NAME");
		if (node != NULL && node->value != NULL)
		{
			strcpy(firstname, node->value);
		}

		node = pub_xml_locnode(xml, ".CBM.ANALYZE.PACKAGE");
		while (node != NULL)
		{
			if (strcmp(node->name, "PACKAGE") != 0)
			{
				node = node->next;
				continue;
			}

			xml->current = node;	
			node1 = pub_xml_locnode(xml, "INCLUDE");
			if (node1 != NULL && node1->value != NULL)
			{
				if (pkg_check_include(vars, firstname, node1->value) != 0)
				{
					node = node->next;
					continue;
				}
			}
		
			node1 = pub_xml_locnode(xml, "CHECK");
			if (node1 != NULL && node1->value != NULL)
			{
				if (pkg_check_mult(vars, node1) != 0)
				{
					node = node->next;                                                                                 
					continue;                                                                                          
				}
			}
			
			node1 = pub_xml_locnode(xml, "NAME");
			if (node1 != NULL && node1->value != NULL)
			{
				if (flag == SW_SOAP_SVR)
				{
					if (!soap_match_tag_ext(soap, tag, node1->value, 1))
					{
						break;
					}
				}
				else
				{
					len = strlen(ttag) - 8;
					if (len > 0)
					{
						memset(buf, 0x00, sizeof(buf));
						strncpy(buf, ttag + len, 8);
						if (pub_str_strcasecmp(buf, "Response") == 0 && pub_str_strcasecmp(ttag, "Response") != 0)
						{
							ttag[len] = '\0';
						}
					}
					if (pub_str_include(ttag, node1->value) == 0)
					{
						break;
					}
				}
			}
			node = node->next;
		}
		if (node == NULL)
		{
			pub_log_error("[%s][%d] Can not found server [%s]!",
				__FILE__, __LINE__, soap->tag);
			return soap->error = SOAP_NO_METHOD;
		}
		nodebak = xml->current;

		while (1)
		{
			soap->error = SOAP_TAG_MISMATCH;
			if (!soap_element_begin_in(soap, soap->tag, 1, NULL))
			{
				i = 0;
				xml->current = nodebak;
				node = pub_xml_locnode(xml, "ITEM");
				while (node != NULL)
				{
					if (strcmp(node->name, "ITEM") != 0)
					{
						node = node->next;
						continue;
					}
				
					if (bit[i] == '0')
					{
						node = node->next;
						i++;
						continue;
					}
					
					memset(&item, 0x0, sizeof(item));
					xml->current = node;
					if (get_com_item(xml, &item))
					{
						pub_log_error("[%s][%d] Get item info error!", __FILE__, __LINE__);
						return soap->error = SOAP_ERR;
					}
					
					p = strchr(soap->tag, ':');
					if (p != NULL)
					{
						p += 1;
					}
					else
					{
						p = soap->tag;
					}
					if (strcmp(item.mname, p) == 0)
					{
						bit[i] = '0';
						break;
					}
					
					node = node->next;
					i++;	
				}
				
				if (node == NULL)
				{
					pub_log_info("[%s][%d] Not config [%s]!", __FILE__, __LINE__, soap->tag);
					if (soap->body)
					{
						soap_element_end_in(soap, soap->tag);
					}
					continue;
				}

				if ((strcmp(item.type, "xsd:file") == 0 && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_OK))
					|| soap->error == SOAP_NO_TAG)
				{
					pub_log_info("[%s][%d] Get the attachment name:[%s]", 
						__FILE__, __LINE__, item.mname);
					struct x__Data *filedata = NULL;
					get_mime_data(soap, &filedata, item.mname);
					after_get_mime(soap, filedata, item.name);
					soap->error = SOAP_TAG_MISMATCH;
					continue;
				}

				if (strcmp(item.type, "struct") != 0 && 
					(soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG || soap->error == SOAP_OK))
				{
					if (soap_in_string(soap, item.mname, &ptr, "xsd:string"))
					{
						nflag = 1;
						item.value = ptr;
						if (soap_set_value(soap, vars, item))
						{
							pub_log_error("[%s][%d] soap_set_value error!",
								__FILE__, __LINE__);
							return soap->error;
						}
						
						soap->error = SOAP_TAG_MISMATCH;
						continue;
					}
				}

				if (strcmp(item.type, "struct") == 0)
				{
					item.loopcnt = 0;
AGAIN:
					memset(item.path, 0x0, sizeof(item.path));
					if (item.loop == 1 && item.absflag == 1)
					{
						sprintf(item.path, "%s(%d)", item.name, item.loopcnt); 
					}
					else if (item.loop == 0 && item.absflag == 1)
					{
						sprintf(item.path, "%s_", item.name);
					}
					else if (item.loop == 1 && item.absflag == 0)
					{
						sprintf(item.path, "(%d)", item.loopcnt);
					}
					if (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_OK)
					{
						soap_revert(soap);
						ret = soap_in_loop(soap, vars, xml, node, item);
						if (ret == 0)
						{
							if (item.loop == 0)
							{
								continue;
							}
							item.loopcnt++;
							if (!soap_element_begin_in(soap, item.mname, 1, NULL))
							{
								soap->error = SOAP_TAG_MISMATCH;
								goto AGAIN;
							}
						}
						else if (ret == -2)
						{
							memset(buf, 0x0, sizeof(buf));
							sprintf(buf, "%d", item.loopcnt);
							loc_set_zd_data(vars, item.loopvar, buf);
							pub_log_info("[%s][%d] 设置变量[%s]=[%s]",
								__FILE__, __LINE__, item.loopvar, buf);	
							continue;	
						}
					}
				}
				if (item.loopcnt)
				{
					if (item.loopvar[0] != '\0')
					{
						memset(buf, 0x0, sizeof(buf));
						sprintf(buf, "%d", item.loopcnt);
						loc_set_zd_data(vars, item.loopvar, buf);
						pub_log_info("[%s][%d] 设置变量[%s]=[%s]", 
							__FILE__, __LINE__, item.loopvar, buf);
					}
				}
				soap->error = SOAP_TAG_MISMATCH;
				continue;
			}
			
			if (soap->error == SOAP_TAG_MISMATCH)
			{
				soap->error = soap_ignore_element(soap);
			}
			
			if (soap->error == SOAP_NO_TAG)
			{
				break;
			}
			
			if (soap->error)
			{
				pub_log_info("[%s][%d] name=[%s] error=[%d]",
					__FILE__, __LINE__, item.mname, soap->error);
				return soap->error;
			}
		}

		if (soap_element_end_in(soap, servertag))
		{
			pub_log_error("[%s][%d] soap_element_end_in [%s] error!", __FILE__, __LINE__, servertag);
			return soap->error;
		}
	}
	
	if (flag == SW_SOAP_CLI && (soap->mode & SOAP_XML_STRICT) && nflag == 0)
	{
		soap->error = SOAP_OCCURS;
	}
	
	after_unpack_for_base64(soap, vars, &g_farray);
	
	if (g_farray.size != 0 && g_farray.flist_head) 
	{
		struct file_node *node = NULL;
		char filename[MAX_FILE_NAME_LEN * 4 + 1];
		
		memset(filename, 0x00, sizeof(filename));
		for (node = g_farray.flist_head; node; node = node->next)
		{
			char tmpname[MAX_FILE_NAME_LEN + 1];
			memset(tmpname, 0x00, sizeof(tmpname));
			loc_get_zd_data(vars, node->varname, tmpname);
			pub_log_info("[%s][%d] node->varname=[%s], tmpname=[%s]",
					__FILE__, __LINE__, node->varname, tmpname);
			if (tmpname[0] == '\0')
				continue;
			strcat(filename, tmpname);
			strcat(filename, "+");
		}
		if (filename[0] != '\0')
			filename[strlen(filename) - 1] = '\0';
		loc_set_zd_data(vars, "$filename", filename);
		pub_log_info("[%s][%d] $filename = [%s]", __FILE__, __LINE__, filename);
	}
	else
	{
		pub_log_info("[%s][%d] No file", __FILE__, __LINE__);
	}

	return SOAP_OK;
}

SOAP_FMAC3 int soap_unpack_var(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *tag, const char *type)
{
	if (soap_element_begin_in(soap, tag, 1, NULL))
	{
		if (soap->error != SOAP_NO_TAG)
		{
			pub_log_info("[%s][%d] soap_element_begin_in error! soap->error=[%d]",
				__FILE__, __LINE__, soap->error);
		}
		return soap->error;
	}

	if (!soap->null && *soap->href != '#')
	{
		soap_revert(soap);
		if (soap_in_unpack_var(soap, vars, xml, tag, type, SW_SOAP_SVR))
		{
			pub_log_error("[%s][%d] unpack var error! tag=[%s]", __FILE__, __LINE__, tag);
			return soap->error;
		}
		pub_log_info("[%s][%d] soap unpack var success! tag=[%s]", __FILE__, __LINE__, tag);
	}

	return SOAP_OK;
}

int soap_unpack_header(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	int	i = 0;
	char	bit[MAX_ITEM_CNT+1];
	char	*p = NULL;
	char	*ptr = NULL;
	char	buf[128];
	char	servertag[128];
	item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	
	memset(bit, 0x0, sizeof(bit));
	memset(buf, 0x0, sizeof(buf));
	memset(servertag, 0x0, sizeof(servertag));
	memset(&item, 0x0, sizeof(item));
	memset(bit, '1', MAX_ITEM_CNT);
	
	soap_peek_element(soap);
	strcpy(servertag, soap->tag);
	nflag = 0;
	if (soap_element_begin_in(soap, servertag, 0, NULL))
	{
		if (soap->error == SOAP_NO_TAG)
		{
			return soap->error;
		}
		pub_log_error("[%s][%d] soap_element_begin_in [%s] error! soap->error=[%d]",
			__FILE__, __LINE__, servertag, soap->error);
		return soap->error;
	}
	
	if (soap->body && !*soap->href)
	{
		node = pub_xml_locnode(xml, ".CBM.ANALYZE.HEAD");
		while (node != NULL)
		{
			if (strcmp(node->name, "HEAD") != 0)
			{
				node = node->next;
				continue;
			}

			xml->current = node;
			node1 = pub_xml_locnode(xml, "NAME");
			if (node1 != NULL && node1->value != NULL)
			{
				if (!soap_match_tag_ext(soap, soap->tag, node1->value, 1))
				{
					break;
				}
			}
			node = node->next;
		}
		if (node == NULL)
		{
			pub_log_error("[%s][%d] Can not found server [%s]!",
				__FILE__, __LINE__, soap->tag);
			return soap->error = SOAP_NO_METHOD;
		}
		nodebak = xml->current;

		while (1)
		{
			soap->error = SOAP_TAG_MISMATCH;
			if (!soap_element_begin_in(soap, soap->tag, 1, NULL))
			{
				i = 0;
				xml->current = nodebak;
				node = pub_xml_locnode(xml, "ITEM");
				while (node != NULL)
				{
					if (strcmp(node->name, "ITEM") != 0)
					{
						node = node->next;
						continue;
					}
				
					if (bit[i] == '0')
					{
						node = node->next;
						i++;
						continue;
					}
					
					memset(&item, 0x0, sizeof(item));
					xml->current = node;
					if (get_com_item(xml, &item))
					{
						pub_log_error("[%s][%d] Get item info error!", __FILE__, __LINE__);
						return soap->error = SOAP_ERR;
					}
					
					p = strchr(soap->tag, ':');
					if (p != NULL)
					{
						p += 1;
					}
					else
					{
						p = soap->tag;
					}
					if (strcmp(item.mname, p) == 0)
					{
						bit[i] = '0';
						break;
					}
					
					node = node->next;
					i++;	
				}
				
				if (node == NULL)
				{
					pub_log_info("[%s][%d] Not config [%s]!", __FILE__, __LINE__, soap->tag);
					soap_element_end_in(soap, soap->tag);
					continue;
				}

				if (strcmp(item.type, "struct") != 0 && 
					(soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG || soap->error == SOAP_OK))
				{
					if (soap_in_string(soap, item.mname, &ptr, "xsd:string"))
					{
						nflag = 1;
						item.value = ptr;
						if (soap_set_value(soap, vars, item))
						{
							pub_log_error("[%s][%d] soap_set_value error!",
								__FILE__, __LINE__);
							return soap->error;
						}
						
						soap->error = SOAP_TAG_MISMATCH;
						continue;
					}
				}

				if (strcmp(item.type, "struct") == 0)
				{
					item.loopcnt = 0;
AGAIN:
					memset(item.path, 0x0, sizeof(item.path));
					if (item.loop == 1 && item.absflag == 1)
					{
						sprintf(item.path, "%s(%d)", item.name, item.loopcnt); 
					}
					else if (item.loop == 0 && item.absflag == 1)
					{
						sprintf(item.path, "%s_", item.name);
					}
					else if (item.loop == 1 && item.absflag == 0)
					{
						sprintf(item.path, "(%d)", item.loopcnt);
					}
					if (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_OK)
					{
						soap_revert(soap);
						if (!soap_in_loop(soap, vars, xml, node, item))                         
						{
							if (item.loop == 0)
							{
								continue;
							}
							item.loopcnt++;
							if (!soap_element_begin_in(soap, item.mname, 1, NULL))
							{
								soap->error = SOAP_TAG_MISMATCH;
								goto AGAIN;
							}
						}
					}
				}
				if (item.loopcnt)
				{
					if (item.loopvar[0] != '\0')
					{
						memset(buf, 0x0, sizeof(buf));
						sprintf(buf, "%d", item.loopcnt);
						loc_set_zd_data(vars, item.loopvar, buf);
						pub_log_info("[%s][%d] 设置变量[%s]=[%s]", 
							__FILE__, __LINE__, item.loopvar, buf);
					}
				}
				soap->error = SOAP_TAG_MISMATCH;
				continue;
			}
			
			if (soap->error == SOAP_TAG_MISMATCH)
			{
				soap->error = soap_ignore_element(soap);
			}
			
			if (soap->error == SOAP_NO_TAG)
			{
				break;
			}
			
			if (soap->error)
			{
				pub_log_info("[%s][%d] name=[%s] error=[%d]",
					__FILE__, __LINE__, item.mname, soap->error);
				return soap->error;
			}
		}

		if (soap_element_end_in(soap, servertag))
		{
			pub_log_error("[%s][%d] soap_element_end_in [%s] error!", __FILE__, __LINE__, servertag);
			return soap->error;
		}
	}
	
	return SOAP_OK;
}

SOAP_FMAC3 int sw_soap_recv_header(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	pub_log_info("[%s][%d]sw_soap_recv_header begin...", __FILE__, __LINE__);
	if (soap_get_header(soap, vars, xml) && soap->error == SOAP_TAG_MISMATCH)
	{
		soap->error = SOAP_OK;
	}
	if (soap->error == SOAP_OK && soap->fheader)
	{
		soap->error = soap->fheader(soap);
	}
	return soap->error;
}

SOAP_FMAC3 int soap_get_header(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	size_t	soap_flag_header = 1;
	char	tag[128];
	
	soap->part = SOAP_IN_HEADER;
	memset(tag, 0x0, sizeof(tag));
	strcpy(tag, "SOAP-ENV:Header");

	if (soap_element_begin_in(soap, tag, 1, NULL))
	{
		soap->header = NULL;
		return soap->header == NULL;
	}

	soap->header = (struct SOAP_ENV__Header *)soap_id_enter(soap, soap->id, soap->header, 
			SOAP_TYPE_SOAP_ENV__Header, sizeof(struct SOAP_ENV__Header), 0, NULL, NULL, NULL);

	if (!soap->header)
	{
		pub_log_info("[%s][%d]soap->header is null", __FILE__, __LINE__);
		return soap->header == NULL;
	}

	if (soap->body && !*soap->href)
	{
		for (;;)
		{
			soap->error = SOAP_TAG_MISMATCH;
			if (soap_flag_header && soap->error == SOAP_TAG_MISMATCH)
			{
				if (soap_unpack_header(soap, vars, xml))
				{
					soap_flag_header--;
					continue;
				}
			}
			if (soap->error == SOAP_TAG_MISMATCH)
			{
				soap->error = soap_ignore_element(soap);
			}
			if (soap->error == SOAP_NO_TAG)
			{
				break;
			}
			if (soap->error)
			{
				soap->header = NULL;
				return soap->header == NULL;
			}
		}
		if (soap_element_end_in(soap, tag))
		{
			pub_log_error("[%s][%d] soap end in [%s] error, soap->error=[%d]",
					__FILE__, __LINE__, tag, soap->error);
			soap->header = NULL;
			return soap->header == NULL;
		}
	}
	else
	{
		soap->header = (struct SOAP_ENV__Header *)soap_id_forward(soap, soap->href, (void*)soap->header, 
								0, SOAP_TYPE_SOAP_ENV__Header, 0, sizeof(struct SOAP_ENV__Header), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
		{
			soap->header = NULL;
			return soap->header == NULL;
		}
	}
	soap->part = SOAP_END_HEADER;
	return soap->header == NULL;
}

SOAP_FMAC3 int soap_unpack(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *tag, const char *type)
{
	short	soap_flag = 0;
	size_t	soap_flag_unpack = 1;

	for (soap_flag = 0;; soap_flag = 1)
	{
		soap->error = SOAP_TAG_MISMATCH;
		if (soap_flag_unpack && soap->error == SOAP_TAG_MISMATCH)
		{
			if (soap_unpack_var(soap, vars, xml, tag, ""))
			{
				if (soap->error == SOAP_TAG_MISMATCH)
				{
					pub_log_error("[%s][%d] SOAP_TAG_MISMATCH!", __FILE__, __LINE__);
					return soap->error;
				}
				soap_flag_unpack--;
				continue;
			}
		}
		if (soap->error == SOAP_TAG_MISMATCH)                                                                      
		{
			if (soap_flag)                                                                                     
			{
				soap->error = SOAP_OK;                                                                     
				break;                                                                                     
			}                                                                                                  
		}
		if (soap_flag && soap->error == SOAP_NO_TAG)                                                               
		{
			break;
		}
	}
	
	return soap->error;
}

static int get_com_item(sw_xmltree_t *xml, item_t *item)
{
	sw_xmlnode_t	*node = NULL;
	
	if (xml == NULL || item == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	node = pub_xml_locnode(xml, "NAME");
	if (node == NULL || node->value == NULL)
	{	
		pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
		return -1;
	}
	strcpy(item->name, node->value);
	
	node = pub_xml_locnode(xml, "NSPREFIX");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->nsprefix, node->value);
	}

	node = pub_xml_locnode(xml, "MNAME");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config MNAME!", __FILE__, __LINE__);
		return -1;
	}
	strcpy(item->mname, node->value);
	
	node = pub_xml_locnode(xml, "TYPE");
	if (node != NULL && node->value != NULL)
	{
		if (strcmp(node->value, "struct") != 0 && strchr(node->value, ':') == NULL)
		{
			sprintf(item->type, "xsd:%s", node->value);
		}
		else
		{
			strcpy(item->type, node->value);
		}
	}
	else
	{
		strcpy(item->type, "xsd:string");
	}
	
	node = pub_xml_locnode(xml, "TNAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->tname, node->value);
	}
	else if (strcmp(item->type, "struct") == 0)
	{
		strcpy(item->tname, item->mname);
	}
	
	node = pub_xml_locnode(xml, "LOOP");
	if (node != NULL && node->value != NULL && node->value[0] == '1')
	{
		item->loop = 1;
	}
	else
	{
		item->loop = 0;
	}
	
	node = pub_xml_locnode(xml, "LOOPCNT");
	if (node != NULL && node->value != NULL)
	{
		strcpy(item->loopvar, node->value);
	}
	
	node = pub_xml_locnode(xml, "AFLAG");
	if (node != NULL && node->value != NULL && node->value[0] == '0')
	{
		item->absflag = 0;
	}
	else
	{
		item->absflag = 1;
	}

	return 0;
}

int soap_serialize_server(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	int	id = 0;
	char	buf[128];
	char	firstname[128];
	char	soapserver[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(firstname, 0x0, sizeof(firstname));
	memset(soapserver, 0x0, sizeof(soapserver));
	loc_get_zd_data(vars, SOAPSERVER, soapserver);
	pub_log_info("[%s][%d] soapserver=[%s]", __FILE__, __LINE__, soapserver);
	id = 1;
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(firstname, node->value);
	}
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
	
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, firstname, node1->value) != 0)
			{
				id++;
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				id++;
				node = node->next;                                                                                 
				continue;                                                                                          
			}
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			if (!soap_match_tag_ext(soap, soapserver, node1->value, 0))
			{
				break;
			}
		}
		id++;
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Can not found server [%s]!", __FILE__, __LINE__, soapserver);
		return soap->error = SOAP_NO_METHOD;
	}
	
	node = pub_xml_locnode(xml, "NSPREFIX");
	if (node != NULL && node->value != NULL)
	{
		loc_set_zd_data(vars, SOAPNSPREFIX, node->value);
	}
	
	node = pub_xml_locnode(xml, "RTYPE");
	if (node != NULL && node->value != NULL)
	{
		loc_set_zd_data(vars, SOAPRTYPE, node->value);
	}

	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", id);
	loc_set_zd_data(vars, SOAPSERVERID, buf);

	pub_log_info("[%s][%d] %s success!", __FILE__, __LINE__, __FUNCTION__);
	
	return SOAP_OK;
}

int soap_serialize_client(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *addr)
{
	int	id = 0;
	char	buf[128];
	char	firstname[128];
	char	soapserver[128];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(firstname, 0x0, sizeof(firstname));
	memset(soapserver, 0x0, sizeof(soapserver));
	
	node = pub_xml_locnode(xml, ".CBM.NSPREFIX");
	if (node != NULL && node->value != NULL)
	{
		loc_set_zd_data(vars, SOAPNSPREFIX, node->value);
		pub_log_info("[%s][%d] nsprefix=[%s]", __FILE__, __LINE__, node->value);
	}

	node = pub_xml_locnode(xml, ".CBM.NAMESPACE");
	if (node != NULL && node->value != NULL)
	{
		loc_set_zd_data(vars, SOAPNSNAME, node->value);
		pub_log_info("[%s][%d] namespace=[%s]", __FILE__, __LINE__, node->value);
	}

	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(firstname, node->value);
	}
	
	id = 1;
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}
		
		memset(soapserver, 0x0, sizeof(soapserver));
		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, firstname, node1->value) != 0)
			{
				id++;
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				id++;
				node = node->next;                                                                                 
				continue;                                                                                          
			}
		}
		
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			return soap->error = SOAP_ERR;
		}
		strcpy(soapserver, node1->value);
	
		break;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Could not find a suitable package!", __FILE__, __LINE__);
		return soap->error = SOAP_NO_METHOD;
	}
	
	node = pub_xml_locnode(xml, "NSPREFIX");
	if (node != NULL && node->value != NULL)
	{
		loc_set_zd_data(vars, SOAPNSPREFIX, node->value);
	}

	node = pub_xml_locnode(xml, "RTYPE");
	if (node != NULL && node->value != NULL)
	{
		loc_set_zd_data(vars, SOAPRTYPE, node->value);
	}
	
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", id);
	loc_set_zd_data(vars, SOAPSERVERID, buf);
	loc_set_zd_data(vars, SOAPSERVER, soapserver);
	set_method(vars, soapserver);
	pub_log_info("[%s][%d] soapserver=[%s] id=[%s]", __FILE__, __LINE__, soapserver, buf);	

	pub_log_info("[%s][%d] %s success!", __FILE__, __LINE__, __FUNCTION__);
	
	return soap->error = SOAP_OK;
}

int soap_out_loop(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, sw_xmlnode_t *pnode, item_t uitem)
{
	int	i = 0;
	int	tid = 0;
	int	addr = 0;
	char	buf[128];
	char	name[256];
	char	mname[128];
	char	tmp[128];
	char	nsprefix[128];
	item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(name, 0x0, sizeof(name));
	memset(nsprefix, 0x0, sizeof(nsprefix));
	memset(&item, 0x0, sizeof(item));

	loc_get_zd_data(vars, SOAPSERVERID, buf);
	tid = atoi(buf) + 1;
	pub_log_info("[%s][%d] soapserverid=[%s]", __FILE__, __LINE__, buf);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", tid);
	loc_set_zd_data(vars, SOAPSERVERID, buf);
	
	memset(mname, 0x0, sizeof(mname));
	if (uitem.nsprefix[0] != '\0')
	{
		sprintf(mname, "%s:%s", uitem.nsprefix, uitem.mname);
	}
	else
	{
		strcpy(mname, uitem.mname);
	}
	if (soap_element_begin_out(soap, mname, soap_embedded_id(soap, -1, &addr, tid), ""))
	{
		pub_log_error("[%s][%d] soap_element_begin_out error! soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		return soap->error;
	}
	
	nodebak = xml->current;
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.STRUCT");
	while (node != NULL)
	{
		if (strcmp(node->name, "STRUCT") != 0)
		{
			node = node->next;
			continue;
		}
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			if (strcmp(node1->value, uitem.tname) == 0)
			{
				break;
			}
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not defined struct [%s]!", __FILE__, __LINE__, uitem.tname);
		return soap->error = SOAP_ERR;
	}
	
	memset(nsprefix, 0x0, sizeof(nsprefix));
	node = pub_xml_locnode(xml, "NSPREFIX");
	if (node != NULL && node->value != NULL)
	{
		strcpy(nsprefix, node->value);
	}
	
	node = pub_xml_locnode(xml, "ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		memset(&item, 0x0, sizeof(item));
		xml->current = node;
		if (get_com_item(xml, &item))
		{
			pub_log_error("[%s][%d] Get item info error!", __FILE__, __LINE__);
			return soap->error = SOAP_ERR;
		}
		if (item.absflag == 0 && uitem.absflag == 1)
		{
			item.absflag = 1;
		}

		if (strcmp(item.type, "xsd:file") == 0)
		{
			char	fname[1024];

			memset(fname, 0x00, sizeof(fname));
			loc_get_zd_data(vars, item.name, fname);
			if (fname[0] != '\0')
			{
				if (soap_out_attach(soap, vars, item.mname, fname))
				{
					pub_log_error("[%s][%d] soap_out_attach error! soap->error=[%d]",
						__FILE__, __LINE__, soap->error);
					return soap->error;
				}
				pub_log_info("[%s][%d] Send file [%s] success!", __FILE__, __LINE__, fname);
			}
			node = node->next;
			continue;
		}

		if (strcmp(item.type, "struct") != 0)
		{
			memset(name, 0x0, sizeof(name));
			if (uitem.loop == 0 && uitem.absflag == 0)
			{
				sprintf(name, "%s", item.name);
			}
			else if (uitem.loop == 1 && uitem.absflag == 0)
			{
				sprintf(name, "%s%s", item.name, uitem.path);
			}
			else
			{
				sprintf(name, "%s%s", uitem.path, item.name + 1);
			}
			memset(item.name, 0x0, sizeof(item.name));
			strcpy(item.name, name);
			if (nsprefix[0] != '\0')
			{
				memset(tmp, 0x0, sizeof(tmp));
				strcpy(tmp, item.mname);
				memset(item.mname, 0x0, sizeof(item.mname));
				sprintf(item.mname, "%s:%s", nsprefix, tmp);
			}
			if (soap_get_value(soap, vars, item))
			{
				pub_log_error("[%s][%d] soap_get_value error! soap->error=[%d]",
					__FILE__, __LINE__, soap->error);
				xml->current = nodebak;
				return soap->error;
			}
		}
		else
		{
			memset(buf, 0x0, sizeof(buf));
			memset(name, 0x0, sizeof(name));
			if (item.loopvar[0] == '#' || item.loopvar[0] == '$')
			{
				if (uitem.loop == 0 && uitem.absflag == 0)
				{
					sprintf(name, "%s", item.loopvar);
				}
				else if (uitem.loop == 1 && uitem.absflag == 0)
				{
					sprintf(name, "%s%s", item.loopvar, uitem.path);
				}
				else
				{
					sprintf(name, "%s%s", uitem.path, item.loopvar + 1);						
				}
				loc_get_zd_data(vars, name, buf);
				item.loopcnt = atoi(buf);
			}
			else if (item.loopvar[0] != '\0')
			{
				item.loopcnt = atoi(item.loopvar);
			}
			else
			{
				item.loopcnt = 1;
			}
			for (i = 0; i < item.loopcnt; i++)
			{
				memset(item.path, 0x0, sizeof(item.path));
				if (uitem.absflag == 1)
				{
					if (item.loop == 1)
					{
						sprintf(item.path, "%s%s(%d)", uitem.path, item.name + 1, i);
					}
					else if (item.loop == 0)
					{
						sprintf(item.path, "%s%s_", uitem.path, item.name + 1);
					}
				}
				else if (item.loop == 1 && item.absflag == 1)
				{
					sprintf(item.path, "%s(%d)", item.name, i);
				}
				else if (item.loop == 1 && item.absflag == 0)
				{
					sprintf(item.path, "(%d)", i);
				}
				if (soap_out_loop(soap, vars, xml, node, item))
				{
					xml->current = nodebak;
					return soap->error;
				}
			}
		}
	
		node = node->next;
	}
	
	xml->current = nodebak;
	pub_log_info("[%s][%d] soap pack [%s] success!", __FILE__, __LINE__, mname);
	
	return soap_element_end_out(soap, mname);;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_pack(
	struct soap *soap, sw_loc_vars_t *vars, 
	sw_xmltree_t *xml, const char *addr, int flag)
{
	int	i = 0;
	int	id = 0;
	int	tid = 0;
	char	*ptr = NULL;
	char	buf[128];
	char	rtype[8];
	char	nsprefix[32];
	char	servertag[128];
	char	soapserver[128];
	char	firstname[128];
	item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(rtype, 0x0, sizeof(rtype));
	memset(firstname, 0x0, sizeof(firstname));
	memset(nsprefix, 0x0, sizeof(nsprefix));
	memset(servertag, 0x0, sizeof(servertag));
	memset(soapserver, 0x0, sizeof(soapserver));
	memset(&item, 0x0, sizeof(item));
	
	loc_get_zd_data(vars, SOAPSERVER, soapserver);
	loc_get_zd_data(vars, SOAPSERVERID, buf);
	tid = atoi(buf);
	loc_get_zd_data(vars, SOAPNSPREFIX, nsprefix);
	loc_get_zd_data(vars, SOAPRTYPE, rtype);
	pub_log_info("[%s][%d] soapserver=[%s] soapserverid=[%d]", __FILE__, __LINE__, soapserver, tid);
	if (flag == SW_SOAP_CLI)
	{
		ptr = strchr(soapserver, ':');
		if (ptr == NULL)
		{
			if (nsprefix[0] != '\0')
			{
				sprintf(servertag, "%s:%s", nsprefix, soapserver);
			}
			else
			{
				sprintf(servertag, "%s:%s", g_soapns, soapserver);
			}
		}
		else
		{
			strcpy(servertag, soapserver);
		}
		
		id = -1;
		id = soap_element_id(soap, servertag, id, addr, NULL, 0, "", tid);
		if (id < 0)
		{
			pub_log_error("[%s][%d] soap_element_id error! soap->error=[%d]",
				__FILE__, __LINE__, soap->error);
			return soap->error;
		}
	}
	else
	{
		if (rtype[0] == '1')
		{
			ptr = strchr(soapserver, ':');
			if (ptr != NULL)
			{
				if (nsprefix[0] != '\0')
				{
					sprintf(servertag, "%s%s", nsprefix, ptr);
				}
				else
				{
					sprintf(servertag, "%s%s", SW_DEFAULT_NS, ptr);
				}
			}
			else
			{
				sprintf(servertag, "%s", soapserver);
			}
		}
		else
		{
			ptr = strchr(soapserver, ':');
			if (ptr != NULL)
			{
				if (nsprefix[0] != '\0')
				{
					sprintf(servertag, "%s%sResponse", nsprefix, ptr);
				}
				else
				{
					sprintf(servertag, "%s%sResponse", SW_DEFAULT_NS, ptr);
				}
			}
			else
			{
				sprintf(servertag, "%sResponse", soapserver);
			}
		}
		id = soap_embed(soap, (void*)addr, NULL, 0, servertag, tid);
	}
	if (soap_element_begin_out(soap, servertag, soap_embedded_id(soap, id, addr, tid), NULL))
	{
		pub_log_error("[%s][%d] soap_element_begin_out error!", __FILE__, __LINE__);
		return soap->error;
	}
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strcpy(firstname, node->value);
	}
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.PACKAGE");
	while (node != NULL)
	{
		if (strcmp(node->name, "PACKAGE") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, firstname, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;                                                                                 
				continue;                                                                                          
			}
		}
		
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			if (!soap_match_tag_ext(soap, soapserver, node1->value, 0))
			{
				break;
			}
		}
		id++;
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Could not found server [%s]!", __FILE__, __LINE__, soapserver);
		return soap->error = SOAP_NO_METHOD;
	}
	node = pub_xml_locnode(xml, "ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		memset(&item, 0x0, sizeof(item));
		xml->current = node;
		if (get_com_item(xml, &item))
		{
			pub_log_error("[%s][%d] Get item info error!", __FILE__, __LINE__);
			return soap->error = SOAP_ERR;
		}
		
		if (strcmp(item.type, "xsd:file") == 0)
		{
			char	fname[1024];

			memset(fname, 0x00, sizeof(fname));
			loc_get_zd_data(vars, item.name, fname);
			if (fname[0] != '\0')
			{

				if (soap_out_attach(soap, vars, item.mname, fname))
				{
					pub_log_error("[%s][%d] soap_out_attach error! soap->error=[%d]",
						__FILE__, __LINE__, soap->error);
					return soap->error;
				}
				pub_log_info("[%s][%d] Send file [%s] success!", __FILE__, __LINE__, fname);
			}
			node = node->next;
			continue;
		}
		
		if (strcmp(item.type, "struct") != 0)
		{
			if (item.nsprefix[0] != '\0')
			{
				char	tmp[128];
				memset(tmp, 0x0, sizeof(tmp));
				strcpy(tmp, item.mname);
				memset(item.mname, 0x0, sizeof(item.mname));
				sprintf(item.mname, "%s:%s", item.nsprefix, tmp);
				pub_log_info("[%s][%d] item->mname:[%s]", __FILE__, __LINE__, item.mname);
			}

			if (soap_get_value(soap, vars, item))
			{
				return soap->error;
			}
		}
		else
		{
			memset(buf, 0x0, sizeof(buf));
			if (item.loopvar[0] == '#' || item.loopvar[0] == '$')
			{
				loc_get_zd_data(vars, item.loopvar, buf);
				item.loopcnt = atoi(buf);
			}
			else if (item.loopvar[0] != '\0')
			{
				item.loopcnt = atoi(item.loopvar);
			}
			else
			{
				item.loopcnt = 1;
			}
			pub_log_info("[%s][%d] name=[%s] loopcnt=[%s]", __FILE__, __LINE__, item.name, buf);
			for (i = 0; i < item.loopcnt; i++)
			{
				memset(item.path, 0x0, sizeof(item.path));
				if (item.loop == 1 && item.absflag == 1)
				{
					sprintf(item.path, "%s(%d)", item.name, i);
				}
				else if (item.loop == 0 && item.absflag == 1)
				{
					sprintf(item.path, "%s_", item.name);
				}
				else if (item.loop == 1 && item.absflag == 0)
				{
					sprintf(item.path, "(%d)", i);
				}
				if (soap_out_loop(soap, vars, xml, node, item))
				{
					return soap->error;
				}
			}
		}

		node = node->next;
	}
	
	if (soap_element_end_out(soap, servertag))
	{
		pub_log_error("[%s][%d] soap_element_end_out error!", __FILE__, __LINE__);
		return soap->error;
	}
	
	return soap_putindependent(soap);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_response(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *addr)
{
	return soap_out_pack(soap, vars, xml, addr, SW_SOAP_SVR);
}


SOAP_FMAC3 int SOAP_FMAC4 soap_out_request(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *addr)
{
	return soap_out_pack(soap, vars, xml, addr, SW_SOAP_CLI);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_server_unpack(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml, const char *tag)
{
	int	ret = 0;
	
	soap->encodingStyle = NULL;
	loc_set_zd_data(vars, SOAPSERVER, tag);
	set_method(vars, tag);

	if (soap_unpack(soap, vars, xml, tag, NULL))
	{
		pub_log_error("[%s][%d] soap_unpack error! ret=[%d]", __FILE__, __LINE__, ret);
		return soap->error;
	}
	pub_log_info("[%s][%d] soap_unpack success!", __FILE__, __LINE__, tag);

	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	
	pub_log_info("[%s][%d] soap_server_unpack success!", __FILE__, __LINE__);
	return SW_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 soap_server_response(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	int	addr = 0;
	
	pub_log_info("[%s][%d] soap_server_pack begin...", __FILE__, __LINE__);
	soap_serializeheader(soap);
	
	if (soap_serialize_server(soap, vars, xml))
	{
		pub_log_error("[%s][%d] soap_serialize_server error!", __FILE__, __LINE__);
		return soap->error;
	}

	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_put_header(soap, vars, xml)
		 || soap_body_begin_out(soap)
		 || soap_out_response(soap, vars, xml, &addr)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_put_header(soap, vars, xml)
	 || soap_body_begin_out(soap)
	 || soap_out_response(soap, vars, xml, &addr)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;

	pub_log_info("[%s][%d] soap_serve success!", __FILE__, __LINE__);

	return soap_closesock(soap);
}

int soap_pub_destroy(int index)
{
	soap_destroy(soaps[index].soap);
	soap_end(soaps[index].soap);
	soap_done(soaps[index].soap);
	free(soaps[index].soap);
	soaps[index].soap = NULL;
	pub_log_info("[%s][%d] Free soaps[%d]!", __FILE__, __LINE__, index);

	return 0;
}

int soap_reset_nsprefix(struct soap *soap, char *nsprefix)
{
	if (nsprefix == NULL || nsprefix[0] == '\0')
	{
		return 0;
	}
	if (soap->namespaces && !soap->local_namespaces)
	{
		register struct Namespace *ns1;
		for (ns1 = soap->namespaces; ns1->id; ns1++)
		{
			size_t	len = 0;
			if (strcmp(ns1->id, SW_DEFAULT_NS) == 0)
			{
				if (strcmp(ns1->id, g_soapns) != 0)
				{
					len = strlen(nsprefix);
					ns1->id = (char *)SOAP_MALLOC(soap, len + 1);
					if (ns1->id == NULL)
					{
						pub_log_error("[%s][%d] SOAP_MALLOC error! soap->error=[%d]",
							__FILE__, __LINE__, soap->error);
						return SW_ERROR;
					}
					memset(ns1->id, 0x0, len + 1);
					memcpy(ns1->id, nsprefix, len);
				}
			}
		}
	}
	
	return 0;
}

int soap_reset_namespaces(struct soap *soap, char *namespace)
{
	if (namespace == NULL || namespace[0] == '\0')
	{
		return 0;
	}
	if (soap->namespaces && !soap->local_namespaces)
	{
		register struct Namespace *ns1;
		for (ns1 = soap->namespaces; ns1->id; ns1++)
		{
			size_t	len = 0;
			if (strcmp(ns1->id, g_soapns) == 0)
			{
				if (strcmp(ns1->ns, namespace) != 0)
				{
					len = strlen(namespace);
					ns1->ns = (char *)SOAP_MALLOC(soap, len + 1);
					if (ns1->ns == NULL)
					{
						pub_log_error("[%s][%d] SOAP_MALLOC error! soap->error=[%d]",
							__FILE__, __LINE__, soap->error);
						return SW_ERROR;
					}
					memset(ns1->ns, 0x0, len + 1);
					memcpy(ns1->ns, namespace, len);
				}
			}
		}
	}
	
	return 0;
}

int soap_client_request(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	int	addr = 0;
	char	namespace[128];
	char	soap_action[128];
	char	soap_endpoint[128];
	
	memset(namespace, 0x0, sizeof(namespace));
	memset(soap_action, 0x0, sizeof(soap_action));
	memset(soap_endpoint, 0x0, sizeof(soap_endpoint));
	
	loc_get_zd_data(vars, "$soapaction", soap_action);
	pub_log_info("[%s][%d] action=[%s]", __FILE__, __LINE__, soap_action);

	loc_get_zd_data(vars, "$soapendpoint", soap_endpoint);
	pub_log_info("[%s][%d] endpoint=[%s]", __FILE__, __LINE__, soap_endpoint);
	
	soap->encodingStyle = NULL;
	soap_begin(soap);
	soap_serializeheader(soap);
	if (soap_serialize_client(soap, vars, xml, &addr))
	{
		pub_log_error("[%s][%d] serialize client reuqest error! soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		lsn_set_err(vars, ERR_TOPKG);
		return soap->error;
	}
	memset(g_soapns, 0x0, sizeof(g_soapns));
	loc_get_zd_data(vars, SOAPNSPREFIX, g_soapns);
	if (g_soapns[0] != '\0')
	{
		soap_reset_nsprefix(soap, g_soapns);
	}
	else
	{
		strncpy(g_soapns, SW_DEFAULT_NS, sizeof(g_soapns));
	}
	memset(namespace, 0x0, sizeof(namespace));
	loc_get_zd_data(vars, SOAPNSNAME, namespace);
	if (namespace[0] != '\0')
	{
		soap_reset_namespaces(soap, namespace);
	}
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{
		if (soap_envelope_begin_out(soap) || soap_put_header(soap, vars, xml) || soap_body_begin_out(soap))
		{
			pub_log_error("[%s][%d] soap error, soap->error=[%d]",
				__FILE__, __LINE__, soap->error);
			return soap->error;
		}
		
		if (soap_out_request(soap, vars, xml, &addr))
		{
			pub_log_error("[%s][%d] soap pack error! soap->error=[%d]",
				__FILE__,  __LINE__, soap->error);
			lsn_set_err(vars, ERR_TOPKG);
			return soap->error;
		}
		
		if (soap_body_end_out(soap) || soap_envelope_end_out(soap))
		{
			pub_log_error("[%s][%d] soap error, soap->error=[%d]",
				__FILE__, __LINE__, soap->error);
			return soap->error;
		}
	}

	if (soap_end_count(soap))
		return soap->error;

	if (soap_connect(soap, soap_endpoint, soap_action))
	{
		pub_log_error("[%s][%d] soap connect [%s][%s] error! soap->error=[%d]",
			__FILE__, __LINE__, soap_endpoint, soap_action, soap->error);
		lsn_set_err(vars, ERR_CONNECT);
		return soap->error;
	}
	
	if (soap_envelope_begin_out(soap) || soap_put_header(soap, vars, xml) || soap_body_begin_out(soap))
	{
		pub_log_error("[%s][%d] soap error, soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		return soap->error;
	}

	if (soap_out_request(soap, vars, xml, &addr))
	{
		pub_log_error("[%s][%d] soap pack error! soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		lsn_set_err(vars, ERR_TOPKG);
		return soap->error;
	}
	
	if (soap_body_end_out(soap) || soap_envelope_end_out(soap) || soap_end_send(soap))
	{
		pub_log_error("[%s][%d] soap send error, soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		return soap_closesock(soap);
	}
	pub_log_info("[%s][%d] soap connect & send success!", __FILE__, __LINE__);
	
	return 0;
}

int soap_client_response(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	char	soapserver[128];
	
	memset(soapserver, 0x0, sizeof(soapserver));
	loc_get_zd_data(vars, SOAPSERVER, soapserver);
	pub_log_info("[%s][%d] soapserver=[%s]", __FILE__, __LINE__, soapserver);

	if (soap_begin_recv(soap)
			|| soap_envelope_begin_in(soap)
			|| sw_soap_recv_header(soap, vars, xml)
			|| soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_in_unpack_var(soap, vars, xml, soapserver, "", SW_SOAP_CLI);
	if (soap->error == SOAP_TAG_MISMATCH)
	{
		char	*ptr = NULL;
		ptr = strchr(soap->tag, ':');
		if (ptr != NULL)
		{
			ptr++;
		}
		else
		{
			ptr = soap->tag;
		}
		
		if (strcasecmp(ptr, "fault") == 0)
		{
			pub_log_info("[%s][%d] Fault:[%s]",
				__FILE__, __LINE__, ptr);
			soap_in_unpack_var(soap, vars, xml, ptr, "", SW_SOAP_FAULT);
		}
	}
	if (soap->error)
	{
		pub_log_error("[%s][%d] Unpack error! soap->error=[%d]", __FILE__, __LINE__, soap->error);
		lsn_set_err(vars, ERR_PKGTOS);
	}
	if (soap_body_end_in(soap)
		|| soap_envelope_end_in(soap)
		|| soap_end_recv(soap))
		return soap_closesock(soap);
	pub_log_info("[%s][%d] soap client recv success!", __FILE__, __LINE__);

	return soap_closesock(soap);
}

static int save_file(const char *filename, const char *buf, int len)
{
	FILE	*fp = NULL;

	if (!filename || !buf)
	{
		pub_log_error("[%s][%d] filename==>%s, buf==>%p", 
				__FILE__, __LINE__, filename, buf);
		return SW_ERROR;
	}
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}

	while (len)
	{ 
		size_t nwritten = fwrite(buf, 1, len, fp);
		if (!nwritten)
		{ 
			fclose(fp);
			return SW_ERROR;
		}
		len -= nwritten;
		buf += nwritten;
	}
	fclose(fp);

	return SW_OK;
}

static sw_loc_vars_t *g_vars = NULL;

int soap_set_gvars(sw_loc_vars_t *vars)
{
	g_vars = vars;
	return 0;
}

static void 	*mime_read_open(struct soap*, void*, const char*, const char*, const char*);
static void 	 mime_read_close(struct soap*, void*);
static size_t 	 mime_read(struct soap*, void*, char*, size_t);
static void 	*mime_server_write_open(struct soap *soap, void *handle, const char *id, 
					const char *type, const char *description, enum soap_mime_encoding encoding);
static void 	 mime_server_write_close(struct soap *soap, void *handle);
static int 	 mime_server_write(struct soap *soap, void *handle, const char *buf, size_t len);

static void *mime_read_open(struct soap *soap, void *handle, 
	const char *id, const char *type, const char *description)
{
	FILE	*fd = fopen((char *)handle, "rb");
	if (!fd) 
	{
		soap->error = SOAP_ERR;
		soap->errnum = errno;
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, (char *)handle, errno, strerror(errno));
		return NULL;
	}
	soap->error = SOAP_OK;
	return (void*)fd;
}

static size_t mime_read(struct soap *soap, void *handle, char *buf, size_t len)
{ 
	return fread(buf, 1, len, (FILE*)handle);
}

static void mime_read_close(struct soap *soap, void *handle)
{
	fclose((FILE*)handle);
}

static void *mime_server_write_open(struct soap *soap, 
	void *unused_handle, const char *id, const char *type, 
	const char *description, enum soap_mime_encoding encoding)
{ 
#define CID_MAX_LEN  1024
	FILE	*fd = NULL;
	char fname[MAX_FILE_NAME_LEN + 1];
	char stored_fname[MAX_FILE_NAME_LEN + 1];
	char comp_cid[CID_MAX_LEN + 1];
	char tmp_cid[CID_MAX_LEN + 1];

	struct file_node *node = NULL;
	
	int len = 0;

	memset(comp_cid, 0x00, sizeof(comp_cid));
	memset(tmp_cid, 0x00, sizeof(tmp_cid));
	/* 
	 *  cid in farray
	 *  	cid:$realid
	 *  id
	 *  	<$realid>
	 * */
	strncpy(tmp_cid, id + 1, sizeof(tmp_cid) - 1);	
	tmp_cid[strlen(tmp_cid) - 1] = '\0';
	sprintf(comp_cid, "cid:%s", tmp_cid);

	if (g_vars == NULL) 
	{
		pub_log_info("[%s][%d] g_vars is null", __FILE__, __LINE__);
		return NULL;
	}
	node = farray_find_fnode_by_cid(&g_farray, comp_cid);
	if (!node)
			return NULL;
			
	memset(fname, 0x00, sizeof(fname));
	loc_get_zd_data(g_vars, node->varname, fname);
	if (fname[0] == '\0')
			return NULL;

	memset(stored_fname, 0x00, sizeof(stored_fname));
	sprintf(stored_fname, "%s/dat/%s/%s", getenv("SWWORK"), 
						g_cycle->lsn_conf.filedir, fname);

	format_dir(stored_fname);

	fd = fopen(stored_fname, "wb");
	if (!fd)
	{
			pub_log_error("[%s][%d] fopen(\"%s\") failed:%s", 
							__FILE__, __LINE__, stored_fname, strerror(errno));
			return NULL;
	}
	node->filename = soap_strdup(soap, stored_fname);
	
	pub_log_info("[%s][%d] Saving file %s type %s\n", 
					__FILE__, __LINE__, stored_fname, type ? type : "");
	return (void*)fd;
}

static void mime_server_write_close(struct soap *soap, void *handle)
{ 
	fclose((FILE *)handle);
}

static int mime_server_write(struct soap *soap, void *handle, const char *buf, size_t len)
{ 
	FILE	*fd = (FILE *)handle;
	size_t	nwritten = 0;

	while (len)
	{
		nwritten = fwrite(buf, 1, len, fd);
		if (!nwritten)
		{
			soap->errnum = errno;
			return SOAP_EOF;
		}
		len -= nwritten;
		buf += nwritten;
	}
	loc_set_zd_data(g_vars, "$fileflag", "1");

	return SOAP_OK;
}

int soap_set_mime_callback(struct soap *soap)
{
	soap->fmimewriteopen = mime_server_write_open;
	soap->fmimewriteclose = mime_server_write_close;
	soap->fmimewrite = mime_server_write;
	soap->fmimereadopen = mime_read_open;
	soap->fmimereadclose = mime_read_close;
	soap->fmimeread = mime_read;
	
	return 0;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default__xop__Include(struct soap *soap, struct _xop__Include *a)
{	(void)soap;
	(void)soap; /* appease -Wall -Werror */
	a->__size = 0;
	a->__ptr = NULL;
	a->id = NULL;
	a->type = NULL;
	a->options = NULL;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_default_x__Data(struct soap *soap, struct x__Data *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_default__xop__Include(soap, &a->xop__Include);
	soap_default_string(soap, &a->xmime5__contentType);
}

SOAP_FMAC3 struct _xop__Include * SOAP_FMAC4 soap_in__xop__Include(struct soap *soap, const char *tag, struct _xop__Include *a, const char *type)
{
	(void)type; /* appease -Wall -Werror */
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
	if (*soap->type && soap_match_tag(soap, soap->type, type) && soap_match_tag(soap, soap->type, ":base64Binary") && soap_match_tag(soap, soap->type, ":base64"))
	{	soap->error = SOAP_TYPE;
		return NULL;
	}
	a = (struct _xop__Include *)soap_id_enter(soap, soap->id, a, SOAP_TYPE__xop__Include, sizeof(struct _xop__Include), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default__xop__Include(soap, a);
	if (soap->body && !*soap->href)
	{

		a->__ptr = soap_getbase64(soap, &a->__size, 0);
#ifndef WITH_LEANER
		if (soap_xop_forward(soap, &a->__ptr, &a->__size, &a->id, &a->type, &a->options))
			return NULL;
#endif
		if ((!a->__ptr && soap->error) || soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	
#ifndef WITH_LEANER
		if (*soap->href != '#')
		{	if (soap_dime_forward(soap, &a->__ptr, &a->__size, &a->id, &a->type, &a->options))
				return NULL;
		}
		else
#endif
			a = (struct _xop__Include *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE__xop__Include, 0, sizeof(struct _xop__Include), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 struct x__Data * SOAP_FMAC4 soap_in_x__Data(struct soap *soap, const char *tag, struct x__Data *a, const char *type)
{
	size_t soap_flag_xop__Include = 1;
	if (soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	a = (struct x__Data *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_x__Data, sizeof(struct x__Data), 0, NULL, NULL, NULL);
	if (!a)
		return NULL;
	soap_default_x__Data(soap, a);
	if (soap->body && !*soap->href)
	{
		for (;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if (soap_flag_xop__Include && soap->error == SOAP_TAG_MISMATCH)
				if (soap_in__xop__Include(soap, "xop:Include", &a->xop__Include, ""))
				{	soap_flag_xop__Include--;
					continue;
				}
			if (soap->error == SOAP_TAG_MISMATCH)
			{
				soap->error = soap_ignore_element(soap);

			}
			if (soap->error == SOAP_NO_TAG)
			{
				/* if it's a mtom attachment,
				 * the tag format like <tagName> <xop:Include href="cid:****"/></tagName> 
				 * or a 'base64Binary' tag
				 * if it returns SOAP_NO_TAG when get the 'Include' tag
				 * it shows that the data is base64Binary 
				 * */
				if (soap_flag_xop__Include) 
				{
					a->xop__Include.__ptr = soap_getbase64(soap, &a->xop__Include.__size, 0);
					a->xmime5__contentType = soap_strdup(soap, "base64");
				}	
				break;
			}	
			if (soap->error)
				return NULL;
		}
		if (soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct x__Data *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_x__Data, 0, sizeof(struct x__Data), 0, NULL);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	if ((soap->mode & SOAP_XML_STRICT) && (soap_flag_xop__Include > 0))
	{	soap->error = SOAP_OCCURS;
		return NULL;
	}
	return a;
}


SOAP_FMAC3 struct x__Data ** SOAP_FMAC4 soap_in_PointerTox__Data(struct soap *soap, const char *tag, struct x__Data **a, const char *type)
{
#if 0
	if (soap_element_begin_in(soap, tag, 1, NULL))
		return NULL;
#endif
	if (!a)
		if (!(a = (struct x__Data **)soap_malloc(soap, sizeof(struct x__Data *))))
			return NULL;
	*a = NULL;
	if (!soap->null && *soap->href != '#')
	{	soap_revert(soap);
		if (!(*a = soap_in_x__Data(soap, tag, *a, type)))
			return NULL;
	}
	else
	{	a = (struct x__Data **)soap_id_lookup(soap, soap->href, (void**)a, SOAP_TYPE_x__Data, sizeof(struct x__Data), 0);
		if (soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

static int soap_out_xop_include(struct soap *soap, 
		const char *tag, int id, 
		const struct _xop__Include *a, 
		const char *type)
{
#ifndef WITH_LEANER
	id = soap_attachment(soap, tag, id, 
			     a, (struct soap_array*)(void*)&a->__ptr, a->id, a->type, a->options, 
			     1, type, SOAP_TYPE__xop__Include);
#else
	id = soap_element_id(soap, tag, id, a, (struct soap_array*)(void*)&a->__ptr, 1, type, SOAP_TYPE__xop__Include);
#endif
	if (id < 0)
		return soap->error;
	if (soap_element_begin_out(soap, tag, id, type))
		return soap->error;
	if (soap_putbase64(soap, a->__ptr, a->__size))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

int soap_out_attach(struct soap *soap, sw_loc_vars_t *vars, const char *tag, const char *filename)
{
	char buf[20];
	int  tid;
	int  addr = 0;
	struct stat st;
	char fname[1024];
	char fflag[10];

	memset(fflag, 0x0, sizeof(fflag));
	loc_get_zd_data(vars, "$fileflag", fflag);


	memset(buf, 0x0, sizeof(buf));
	loc_get_zd_data(vars, SOAPSERVERID, buf);
	tid = atoi(buf) + 1;
	pub_log_info("[%s][%d] soapserverid=[%s]", __FILE__, __LINE__, buf);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%d", tid);
	loc_set_zd_data(vars, SOAPSERVERID, buf);

	if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, -1, &addr, tid), NULL))
		return soap->error;
	if ((fflag[0] != '1') || (filename[0] == '\0'))
			goto END_TAG;

	memset(fname, 0x0, sizeof(fname));
	sprintf(fname, "%s/dat/%s/%s", getenv("SWWORK"), g_cycle->lsn_conf.filedir, filename);

	pub_log_info("[%s][%d] file [%s] will be sent", __FILE__, __LINE__, fname);

	memset(&st, 0x00, sizeof(st));
	if (stat(fname, &st)) 
	{
		soap->error =  SOAP_ERR;
		pub_log_error("[%s][%d] fstat(%s)error[%s]",
				__FILE__, __LINE__, fname, strerror(errno));
		return soap->error;
	}

	struct _xop__Include  *in  = (struct _xop__Include  *)soap_malloc(soap, sizeof(struct _xop__Include));
	in->__ptr   = (unsigned char *)soap_strdup(soap, fname);	
	in->__size  = st.st_size;
	in->id	    = NULL;	
	in->type    = "*/*";
	in->options = NULL;


	if (soap_out_xop_include(soap, "xop:Include", -1, in, ""))
		return soap->error;
END_TAG:
	if (soap_element_end_out(soap, tag))
		return soap->error;
	return SOAP_OK;

}

struct x__Data ** get_mime_data(struct soap *soap, struct x__Data **item, char *tag)
{
	return	soap_in_PointerTox__Data(soap, tag, item, NULL);	
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_header(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	int	i = 0;
	int	id = 0;
	int	addr = 0;
	int	first = 1;
	int	tid = 0;
	char	buf[128];
	char	tag[128];
	char	header[128];
	char	headerns[32];
	char	firstname[128];
	item_t	item;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(tag, 0x0, sizeof(tag));
	memset(header, 0x0, sizeof(header));
	memset(headerns, 0x0, sizeof(headerns));
	memset(firstname, 0x0, sizeof(firstname));
	memset(&item, 0x0, sizeof(item));
	
	sprintf(tag, "SOAP-ENV:Header");
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.FIRSTWORK.ITEM.NAME");
	if (node != NULL && node->value != NULL)
	{
		strncpy(firstname, node->value, sizeof(firstname) - 1);
	}
	
	node = pub_xml_locnode(xml, ".CBM.INTEGRATE.HEAD");
	while (node != NULL)
	{
		if (strcmp(node->name, "HEAD") != 0)
		{
			node = node->next;
			continue;
		}
	
		xml->current = node;
		memset(header, 0x0, sizeof(header));
		memset(headerns, 0x0, sizeof(headerns));
		node1 = pub_xml_locnode(xml, "NSPREFIX");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(headerns, node1->value, sizeof(headerns) - 1);
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			if (headerns[0] != '\0')
			{
				sprintf(header, "%s:%s", headerns, node1->value);
			}
			else
			{
				strncpy(header, node1->value, sizeof(header) - 1);
			}
		}
	
		node1 = pub_xml_locnode(xml, "INCLUDE");
		if (node1 != NULL && node1->value != NULL)
		{
			if (pkg_check_include(vars, firstname, node1->value) != 0)
			{
				node = node->next;
				continue;
			}
		}

		node1 = pub_xml_locnode(xml, "CHECK");
		if (node1 != NULL)
		{
			if (pkg_check_mult(vars, node1) != 0)
			{
				node = node->next;
				continue;
			}
		}
		
		if (first)
		{
			first = 0;
			memset(buf, 0x0, sizeof(buf));
			loc_get_zd_data(vars, SOAPSERVERID, buf);
			tid = atoi(buf) + 1;
			pub_log_info("[%s][%d] soapserverid=[%s]", __FILE__, __LINE__, buf);
			memset(buf, 0x0, sizeof(buf));
			sprintf(buf, "%d", tid);
			loc_set_zd_data(vars, SOAPSERVERID, buf);
			soap->part = SOAP_IN_HEADER;
			if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, 0, &addr, tid), NULL))
			{
				pub_log_error("[%s][%d] soap begin [%s] out error!",
					__FILE__, __LINE__, tag);
				return soap->error;
			}
		}
		
		if (header[0] != '\0')
		{
			id = -1;
			id = soap_element_id(soap, header, id, &addr, NULL, 0, "", tid);
			if (id < 0)
			{
				pub_log_error("[%s][%d] soap_element_id error! soap->error=[%d]",
					__FILE__, __LINE__, soap->error);
				return soap->error;
			}
			
			if (soap_element_begin_out(soap, header, soap_embedded_id(soap, id, &addr, tid), NULL))
			{
				pub_log_error("[%s][%d] soap_element_begin_out error!", __FILE__, __LINE__);
				return soap->error;
			}
		}

		node1 = pub_xml_locnode(xml, "ITEM");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "ITEM") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			memset(&item, 0x0, sizeof(item));
			xml->current = node1;
			if (get_com_item(xml, &item))
			{
				pub_log_error("[%s][%d] Get item info error!", __FILE__, __LINE__);
				return soap->error = SOAP_ERR;
			}
					
			if (strcmp(item.type, "struct") != 0)
			{
				if (item.nsprefix[0] != '\0')
				{
					char    tmp[128];
					memset(tmp, 0x0, sizeof(tmp));
					strcpy(tmp, item.mname);
					memset(item.mname, 0x0, sizeof(item.mname));
					sprintf(item.mname, "%s:%s", item.nsprefix, tmp);
					pub_log_info("[%s][%d] item->mname:[%s]", __FILE__, __LINE__, item.mname);
				}
				if (soap_get_value(soap, vars, item))
				{
					return soap->error;
				}
			}
			else
			{
				memset(buf, 0x0, sizeof(buf));
				if (item.loopvar[0] == '#' || item.loopvar[0] == '$')
				{
					loc_get_zd_data(vars, item.loopvar, buf);
					item.loopcnt = atoi(buf);
				}
				else if (item.loopvar[0] != '\0')
				{
					item.loopcnt = atoi(item.loopvar);
				}
				else
				{
					item.loopcnt = 1;
				}
				pub_log_info("[%s][%d] name=[%s] loopcnt=[%s]", __FILE__, __LINE__, item.name, buf);
				for (i = 0; i < item.loopcnt; i++)
				{
					memset(item.path, 0x0, sizeof(item.path));
					if (item.loop == 1 && item.absflag == 1)
					{
						sprintf(item.path, "%s(%d)", item.name, i);
					}
					else if (item.loop == 0 && item.absflag == 1)
					{
						sprintf(item.path, "%s_", item.name);
					}
					else if (item.loop == 1 && item.absflag == 0)
					{
						sprintf(item.path, "(%d)", i);
					}
					if (soap_out_loop(soap, vars, xml, node1, item))
					{
						return soap->error;
					}
				}
			}
			
			node1 = node1->next;
		}
		if (header[0] != '\0')
		{
			if (soap_element_end_out(soap, header))
			{
				pub_log_error("[%s][%d] soap_element_end_out error!", __FILE__, __LINE__);
				return soap->error;
			}
		}
	
		node = node->next;
	}
	
	if (!first)
	{
		if (soap_element_end_out(soap, tag))
		{
			pub_log_error("[%s][%d] soap end [%s] out error!", __FILE__, __LINE__, tag);
			return soap->error;
		}
		soap->part = SOAP_END_HEADER;
	
		return soap->error = SOAP_OK;
	}

        return SOAP_OK;
}

int soap_set_xmlns(sw_lsn_cycle_t *cycle, struct soap *soap)
{
	int	cnt = 0;
	size_t	n = 0;
	size_t	len = 0;
	char	id[32];
	char	ns[128];
	char	*ptr = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	cnt = 0;
	xml = cycle->chnl.cache.pkgdeal;
	node = pub_xml_locnode(xml, ".CBM.XMLNS");
	while (node != NULL)
	{
		if (strcmp(node->name, "XMLNS") != 0)
		{
			node = node->next;
			continue;
		}
		pub_log_info("[%s][%d] xmlns:[%s]", __FILE__, __LINE__, node->value);	
		cnt++;
		node = node->next;
	}
	
	if (cnt > 0 && soap->namespaces)
	{
		g_namespaces = (struct Namespace*)SOAP_MALLOC(soap, (cnt + 1) * sizeof(struct Namespace));
		if (g_namespaces == NULL)
		{
			pub_log_error("[%s][%d] SOAP MALLOC error!", __FILE__, __LINE__);
			return soap->error;
		}
		memset(g_namespaces, 0x00, (cnt + 1) * sizeof(struct Namespace));
		
		n = 0;
		node = pub_xml_locnode(xml, ".CBM.XMLNS");
		while (node != NULL)
		{
			if (strcmp(node->name, "XMLNS") != 0)
			{
				node = node->next;
				continue;
			}
			
			memset(id, 0x0, sizeof(id));
			memset(ns, 0x0, sizeof(ns));
			ptr = strchr(node->value, '=');
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] XMLNS [%s] format error!", __FILE__, __LINE__, node->value);
				return soap->error = SOAP_ERR;
			}
			memcpy(id, node->value, ptr - node->value);
			strcpy(ns, ptr + 1);

			len = strlen(id);
			g_namespaces[n].id = (char *)SOAP_MALLOC(soap, len + 1);
			if (g_namespaces[n].id == NULL)
			{
				pub_log_error("[%s][%d] SOAP MALLOC error!", __FILE__, __LINE__);
				return soap->error;
			}
			memset(g_namespaces[n].id, 0x0, len + 1);
			strncpy(g_namespaces[n].id, id, len);
	
			len = strlen(ns);
			g_namespaces[n].ns = (char *)SOAP_MALLOC(soap, len + 1);
			if (g_namespaces[n].ns == NULL)
			{
				pub_log_error("[%s][%d] SOAP MALLOC error!", __FILE__, __LINE__);
				return soap->error;
			}
			memset(g_namespaces[n].ns, 0x0, len + 1);
			strncpy(g_namespaces[n].ns, ns, len);
			
			g_namespaces[n].in = NULL;
			g_namespaces[n].out = NULL;
			n++;

			node = node->next;
		}
		g_namespaces[n].id = NULL;
		g_namespaces[n].ns = NULL;
		g_namespaces[n].in = NULL;
		g_namespaces[n].out = NULL;

		soap->namespaces = g_namespaces;
	}
	
	return SW_OK;
}

int soap_server_begin(struct soap *soap, sw_loc_vars_t *vars, sw_xmltree_t *xml)
{
	soap_begin(soap);
	if (soap_begin_recv(soap) || soap_envelope_begin_in(soap))
	{
		if (soap->error == SOAP_EOF)
		{    
			pub_log_info("[%s][%d] the client has closed",
					__FILE__, __LINE__);
			return soap_closesock(soap);
		}
		pub_log_error("[%s][%d] begin serve error, soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		if (soap->error < SOAP_STOP)
		{
			return soap_send_fault(soap);
		}
		return soap_closesock(soap);
	}

	if (sw_soap_recv_header(soap, vars, xml))
	{
		pub_log_error("[%s][%d] soap get header error, soap->error=[%d]",
			__FILE__, __LINE__, soap->error);
		if (soap->error < SOAP_STOP)
		{
			return soap_send_fault(soap);
		}
		return soap_closesock(soap);
	}
	
	if (soap_body_begin_in(soap))
	{
		if (soap->error < SOAP_STOP)
		{
			return soap_send_fault(soap);
		}
		return soap_closesock(soap);
	}
	return SOAP_OK;
}

static int set_http_host_info(sw_buf_t *locbuf, char *address)
{
	int headlen = 0;
	char value[128];
	char head[1024];
	char buf[1024];
	char *ip = NULL;
	char *p = NULL;
	char *s = NULL;
	char *ptr = NULL;
	sw_buf_t pkgbuf;

	ip = strchr(address, ':');
	if (ip == NULL)
	{
		pub_log_error("[%s][%d] invalid address.", __FILE__, __LINE__);
		return SW_ERROR;	
	}	
	pub_log_info("[%s][%d] ip[%s]", __FILE__, __LINE__, ip + 3);

	ptr = strstr(locbuf->data, "\r\n\r\n");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] http package format error!", __FILE__, __LINE__);
		pub_log_bin(SW_LOG_ERROR, locbuf->data, locbuf->len, "[%s][%d] SOAP package format error! len=[%d]",__FILE__, __LINE__, locbuf->len);
		return -1;
	} 
	
	headlen = ptr - locbuf->data + 4;
	memset(head, 0x00, sizeof(head));
	memcpy(head, locbuf->data, headlen);
	
	ptr = head;
	memset(buf, 0x00, sizeof(buf));
	p = strstr(ptr, "Host: ");
	if (p != NULL)
	{
		s = strstr(p + 6, "\r\n");
		if (s != NULL)
		{
			memset(value, 0x00, sizeof(value));
			memset(buf, 0x00, sizeof(buf));
			sprintf(value, "Host: %s", ip + 3); 
			strncpy(buf, ptr, p - ptr);
			strcat(buf, value);
			strcat(buf, s);	
		}
		else
		{
			pub_log_error("[%s][%d] vaild Host.", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	else
	{
		pub_log_error("[%s][%d] vaild Host.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (headlen != strlen(buf))
	{
		memset(&pkgbuf, 0x0, sizeof(pkgbuf));
		pub_buf_init(&pkgbuf);
		
		if (pub_buf_chksize(&pkgbuf, locbuf->len) != SW_OK)
		{
			pub_log_error("[%s][%d] update buf error! size=[%d]", __FILE__, __LINE__, locbuf->len);
			pub_buf_clear(&pkgbuf);
			return -1;
		}

		memcpy(pkgbuf.data, locbuf->data + headlen, locbuf->len - headlen);
		memset(locbuf->data, 0x00, locbuf->size);	
		strncpy(locbuf->data,buf, strlen(buf));
		strcat(locbuf->data, pkgbuf.data);
		locbuf->len = strlen(locbuf->data);
		pub_buf_clear(&pkgbuf);
	}
	else
	{
		strncpy(locbuf->data, buf, strlen(buf));
	}

	pub_log_info("[%s][%d] soap head deal success, headlen=[%d] buflen=[%d]!", __FILE__, __LINE__, headlen, strlen(buf));
	return 0;
}


static sw_int_t soap_trans_destroy(struct soap *soap)
{
	soap_destroy(soap);
	soap_end(soap);
	soap_done(soap);
	free(soap);
	return SW_OK;	
}

static int get_http_path(sw_buf_t *locbuf, char *path)
{
	char *p = NULL;
	char *ptr = NULL;
	
	ptr = locbuf->data;
	while(*ptr == ' ')
	{
		ptr++;
	}
	
	p = strstr(ptr, " ");
	if (p == NULL)
	{
		pub_log_error("[%s][%d] http format error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	p++;
	ptr = strstr(p, " ");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] http format error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	strncpy(path, p, ptr - p);

	pub_log_info("[%s][%d] http path =[%s]!", __FILE__, __LINE__, path);
	return SW_OK;
}
static sw_int_t soap_pass_recv(struct soap *soap, sw_buf_t *locbuf)
{
	int	toread = 0;
	int	width = 0;
	int	sockid;
	fd_set	readfds;
	sw_int_t	ret = 0;
	sw_int_t	length = 0;
	struct timeval	timeout;

	memset(&timeout, 0x0, sizeof(timeout));
	sockid = soap->socket;
	width = sockid + 1;
	while (1)
	{
		FD_ZERO(&readfds);                                                                                 
		FD_SET(sockid, &readfds);                                                                 
		memset(&timeout, 0x0, sizeof(timeout));                                                            
		timeout.tv_sec = 0;                                                                                
		timeout.tv_usec = 300000;                                                                               
		ret = select(width, &readfds, NULL, NULL, &timeout);                                               
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			pub_log_error("[%s][%d] select error! errno=[%d]:[%s]",__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		else if (ret == 0)
		{
			pub_log_info("[%s][%d] Recv data over! size=[%d]",__FILE__, __LINE__, length);
			break;
		}

		toread = 0;
		ret = ioctl(sockid, FIONREAD, &toread);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] ioctl error! errno=[%d]:[%s]",__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] toread=[%d]", __FILE__, __LINE__, toread);
		if (toread == 0)
		{
			return -2;
		}

		if (pub_buf_chksize(locbuf, toread) != SW_OK)
		{
			pub_log_error("[%s][%d] Update buffer error! size=[%d]", __FILE__, __LINE__, length + toread);
			return SW_ERROR;
		}
	
		ret = lsn_pub_recv_len(sockid, locbuf->data + length, toread);
		if (ret != toread)
		{
			pub_log_error("[%s][%d] lsn_pub_recv error! toread=[%d] ret=[%d]",__FILE__, __LINE__, toread, ret);
			pub_log_bin(SW_LOG_INFO, locbuf->data, length, "[%s][%d] Recv data:[%d]",__FILE__, __LINE__, length);
			return SW_ERROR;
		}
		length += toread;
		locbuf->len = length;
	}
	if (length <= 0)
	{
		pub_log_error("[%s][%d] No data to read!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return locbuf->len;
}

sw_int_t soap_pass_through(struct soap *soap, char *ip, int times)
{
	int	ret = 0;
	char   tmp[256];
	char   path[128];
	char *testlog  = NULL;
	struct soap *mysoap  = NULL;
	fd_set	readfds;
	struct  timeval	timeout;
	sw_buf_t	pkgbuf;
	
	memset(&pkgbuf, 0x0, sizeof(pkgbuf));
	pub_buf_init(&pkgbuf);
	ret = soap_pass_recv(soap, &pkgbuf);
	if (ret <= 0)
	{
		pub_buf_clear(&pkgbuf);
        if (ret == -2)
        {
            pub_log_info("[%s][%d] recv detect info", __FILE__, __LINE__);
            return SW_OK;
        }
		pub_log_error("[%s][%d] recv soap buf error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
		
	mysoap = soap_new1(SOAP_XML_INDENT);
	if (mysoap == NULL)
	{
		pub_buf_clear(&pkgbuf);
		pub_log_error("[%s][%d] create soap error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

#ifdef SOAP_DEBUG
	testlog = (char*)SOAP_MALLOC(mysoap, strlen(soap->logfile[SOAP_INDEX_TEST]) + 1);
	if (testlog == NULL)
	{
		pub_log_error("[%s][%d] error:soap malloc failed", __FILE__, __LINE__);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}

	strncpy(testlog, soap->logfile[SOAP_INDEX_TEST], strlen(soap->logfile[SOAP_INDEX_TEST]));
	testlog[strlen(soap->logfile[SOAP_INDEX_TEST])] = '\0';
	mysoap->logfile[SOAP_INDEX_TEST] = testlog;
#endif

	memset(path, 0x0, sizeof(path));
	if (get_http_path(&pkgbuf, path) != SW_OK)
	{
		pub_log_error("[%s][%d]get http path error", __FILE__, __LINE__);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}
	
	memset(tmp, 0x00, sizeof(tmp));	
	strncpy(tmp, ip, sizeof(tmp));
	if (strlen(path) > 1 && path[0] == '/')
	{
		strcat(tmp, path);
	}
	
	if (tmp && soap_connect(mysoap, tmp, ""))
	{
		pub_log_error("[%s][%d]soap_connect error", __FILE__, __LINE__);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}

	if (!soap_valid_socket(mysoap->socket))
	{
		pub_log_error("[%s][%d]invalid soap socket", __FILE__, __LINE__);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}

	if (set_http_host_info(&pkgbuf, ip) != SW_OK)
	{
		pub_log_error("[%s][%d]get http host info error.", __FILE__, __LINE__);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}

	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "\n===============send dst server request msg==================\n");
	DBGMSG(SENT, tmp, strlen(tmp));
	DBGMSG(SENT, pkgbuf.data, pkgbuf.len);

	if (mysoap->fsend(mysoap, pkgbuf.data, pkgbuf.len))
	{
		pub_log_error("[%s][%d]fsend to dst server error, soap->error=[%d]", __FILE__, __LINE__, mysoap->error);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d trans pkg to dst server success.", __FILE__, __LINE__);
	FD_ZERO(&readfds);
	FD_SET(mysoap->socket, &readfds);
	memset(&timeout, 0x0, sizeof(timeout));
	if (times != 0)
	{
		timeout.tv_sec = times;
	}
	else
	{
		timeout.tv_sec = 30;
	}
	while (1)
	{
		ret = select(mysoap->socket + 3, &readfds, NULL, NULL, &timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				pub_log_error("[%s][%d] interrupt!", __FILE__, __LINE__);
				continue;
			}
			else
			{
				pub_buf_clear(&pkgbuf);
				soap_trans_destroy(mysoap);
				pub_log_error("[%s][%d] select error! errno=[%d]:[%s]",__FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}
		}
		else if (ret == 0)
		{
			pub_buf_clear(&pkgbuf);
			soap_trans_destroy(mysoap);
			pub_log_error("[%s][%d] timeout!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		break;
	}
	
	memset(pkgbuf.data, 0x0, pkgbuf.size);
	ret = soap_pass_recv(mysoap, &pkgbuf);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d]soap_server_begin error, soap->error=[%d]", __FILE__, __LINE__, mysoap->error);
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		return SW_ERROR;
	}

	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "\n===============recv dst server response msg==================\n");
	DBGMSG(RECV, tmp, strlen(tmp));
	DBGMSG(RECV, pkgbuf.data, pkgbuf.len);
	pub_log_info("[%s][%d recv remote response success.", __FILE__, __LINE__);

	if (soap->fsend(soap, pkgbuf.data, pkgbuf.len) != SOAP_OK)
	{
		pub_buf_clear(&pkgbuf);
		soap_trans_destroy(mysoap);
		pub_log_error("[%s][%d]fsend to client error, soap->error=[%d]", __FILE__, __LINE__, soap->error);
		return SW_ERROR;
	}
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp, "\n===============transfer response to client msg==================\n");
	DBGMSG(SENT, tmp, strlen(tmp));
	DBGMSG(SENT, pkgbuf.data, pkgbuf.len);
	pub_buf_clear(&pkgbuf);
	soap_trans_destroy(mysoap);
	pub_log_info("[%s][%d]soap send to client success", __FILE__, __LINE__);
	return SW_OK;
}
