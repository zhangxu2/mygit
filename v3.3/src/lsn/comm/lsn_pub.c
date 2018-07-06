/*********************************************************************
 *** version : v3.0
 *** author  : 
 *** create  : 
 *** module  : swlsn 
 *** name    : lsn_pub.c
 *** function: 
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/
#include "lsn_pub.h"
#include <dlfcn.h>
#include "alert.h"
#include "pub_proc.h"
#include "pub_code.h"

static int g_rcvtrytimes = 0;
static long g_rcvwaittime = 0;

extern sw_int_t lsn_get_plugin_handle(sw_lsn_cycle_t *cycle, sw_char_t *name, sw_char_t *prdt_name, void **hd, void **df);
extern sw_int_t lsn_cycle_child_destroy(sw_lsn_cycle_t *cycle);
extern int lsn_cache_upd_svrmap(sw_lsn_cycle_t *cycle, int index);

static sw_int32_t lsn_child_register(sw_lsn_cycle_t *cycle, sw_int32_t status, sw_char_t *lsn_name);
static sw_int_t lsn_upd_cfg(sw_lsn_cycle_t *cycle);
static sw_int_t lsn_upd_prdt(sw_lsn_cycle_t *cycle, sw_char_t *prdt);
static sw_int_t lsn_add_prdt(sw_lsn_cycle_t *cycle, sw_char_t *prdt);
static sw_int_t get_all_lsncfg_handle(sw_lsn_cycle_t *cycle, sw_char_t *name);
static sw_int_t lsn_prdt_svrmap_add(sw_lsn_cycle_t *cycle, sw_char_t *prdt);

sw_int_t lsn_get_chl_stat(char *name, int *stat)
{
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));

	sprintf(xmlname, "%s/cfg/channels.xml", getenv("SWWORK"));
	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.CHANNEL");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHANNEL") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "LISTEN");
		if (node1 == NULL || node1->value == NULL)
		{       
			pub_log_error("[%s][%d] Not config [LISTEN]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		if (strcmp(name, node1->value) == 0)
		{       
			break;
		}

		node = node->next;
	}
	if (node == NULL)
	{       
		pub_log_error("[%s][%d] Not find [%s] config!", __FILE__, __LINE__, name);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, "STATUS");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config [STATUS]!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] [%s] status is [%s]", __FILE__, __LINE__, name, node->value);

	*stat = atoi(node->value);

	pub_xml_deltree(xml);
	return SW_OK;
}

/******************************************************************************
 *** name      : lsn_pub_set_lsn_comm
 *** function  : set communication parameter
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
SW_PROTECTED sw_int_t lsn_pub_read_cfg(sw_lsn_cycle_t * cycle, sw_xmltree_t *xml_tree, sw_xmlnode_t *current_node);
sw_int_t lsn_pub_set_lsn_comm(sw_xmltree_t * xml, sw_xmlnode_t *node, sw_lsn_config_t * st)
{
	int	flag = 0;
	sw_xmlnode_t *node1 = NULL, *node2 = NULL;
	sw_xmlnode_t *xml_bak = NULL;
	sw_int32_t index = 0;
	sw_int32_t i = 0;
	sw_int32_t j = 0;

	pub_mem_memzero(&st->comm, sizeof (st->comm));
	for (i = 0; i < MAX_NODE_CHANNEL; i++)
	{
		st->comm.group[i].remote.sockid = -1;
		st->comm.group[i].local.sockid = -1;
		st->comm.group[i].send.qcnt = 0;
		st->comm.group[i].recv.qcnt = 0;
	}

	/* use comm config */
	xml_bak = xml->current;
	xml->current = node;
	node1 = pub_xml_locnode(xml, "GROUP");
	while (node1 != NULL)
	{
		xml->current = node1;

		node2 = pub_xml_locnode(xml, "REMOTE");
		if (node2 != NULL)
		{
			xml->current = node2;
			node2 = pub_xml_locnode(xml, "IP");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_error("[%s][%d] REMOTE not set IP", __FILE__, __LINE__);
				xml->current = xml_bak;
				return SW_ERROR;
			}

			strcpy(st->comm.group[index].remote.ip, node2->value);
			node2 = pub_xml_locnode(xml, "PORT");
			if (node2 == NULL || node2->value == NULL &&
				(st->comm.group[index].remote.ip[0] != '$' && st->comm.group[index].remote.ip[0] != '#'))
			{
				pub_log_error("[%s][%d] REMOTE not set PORT", __FILE__, __LINE__);
				xml->current = xml_bak;
				return SW_ERROR;
			}

			st->comm.group[index].remote.port = atoi(node2->value);
			node2 = pub_xml_locnode(xml, "SERVICE");
			if (node2 != NULL && node2->value != NULL)
			{
				strcpy(st->comm.group[index].remote.service, node2->value);
			}

			st->comm.group[index].remote.use = 1;
		}
		xml->current = node1;
		node2 = pub_xml_locnode(xml, "SNDQ");
		if (node2 != NULL)
		{
			xml->current = node2;
			node2 = pub_xml_locnode(xml, "QMGR");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_info("[%s][%d]SNDQ not set QMGR", __FILE__, __LINE__);
			}
			else
			{
				strcpy(st->comm.group[index].send.qmgr, node2->value);
			}

			node2 = pub_xml_locnode(xml, "PROCCNT");
			if (node2 != NULL || node2->value != NULL)
			{
				st->comm.group[index].send.proc_cnt = atoi(node2->value);
				if (st->comm.group[index].send.proc_cnt > MAX_COMM_PROC_CNT)
				{
					st->comm.group[index].send.proc_cnt = MAX_COMM_PROC_CNT;
				}
			}
			else
			{
				st->comm.group[index].send.proc_cnt = 1;
			}

			j = 0;
			node2 = pub_xml_locnode(xml, "QNAME");
			while (j < MQ_NUM && node2 != NULL)
			{
				if (strcmp(node2->name, "QNAME") != 0)
				{
					node2 = node2->next;
					continue;
				}

				if (node2 == NULL || node2->value == NULL)
				{
					pub_log_info("[%s][%d]SNDQ not set QNAME",__FILE__, __LINE__);
				}
				else
				{
					strcpy(st->comm.group[index].send.qm[j].qname, node2->value);
				}

				node2 = node2->next;
				j++;
			}
			st->comm.group[index].send.qcnt = j;
			pub_log_info("[%s][%d] qmgr:[%s] qcnt:[%d]",
					__FILE__, __LINE__, st->comm.group[index].send.qmgr, st->comm.group[index].send.qcnt);
			flag = 1;
		}
		xml->current = node1;
		node2 = pub_xml_locnode(xml, "RCVQ");
		if (node2 != NULL)
		{
			xml->current = node2;
			node2 = pub_xml_locnode(xml, "QMGR");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_info("[%s][%d]SCVQ not set QMGR", __FILE__, __LINE__);
			}
			else
			{
				strcpy(st->comm.group[index].recv.qmgr, node2->value);
			}

			node2 = pub_xml_locnode(xml, "PROCCNT");
			if (node2 != NULL || node2->value != NULL)
			{
				st->comm.group[index].recv.proc_cnt = atoi(node2->value);
				if (st->comm.group[index].recv.proc_cnt > MAX_COMM_PROC_CNT)
				{
					st->comm.group[index].recv.proc_cnt = MAX_COMM_PROC_CNT;
				}
			}
			else
			{
				st->comm.group[index].recv.proc_cnt = 1;
			}

			j = 0;
			node2 = pub_xml_locnode(xml, "QNAME");
			while( j < MQ_NUM && node2 != NULL)
			{
				if (strcmp(node2->name, "QNAME") != 0)
				{
					node2 = node2->next;
					continue;
				}

				if (node2 == NULL || node2->value == NULL)
				{
					pub_log_info("[%s][%d]RCVQ not set QNAME",
							__FILE__, __LINE__);
				}
				else
				{
					strcpy(st->comm.group[index].recv.qm[j].qname, node2->value);
				}

				node2 = node2->next;
				j++;
			}
			st->comm.group[index].recv.qcnt = j;
			pub_log_info("[%s][%d] qmgr:[%s] qcnt:[%d]",
					__FILE__, __LINE__, st->comm.group[index].recv.qmgr, st->comm.group[index].recv.qcnt);
			flag = 1;
		}
		xml->current = node1;
		node2 = pub_xml_locnode(xml, "LOCAL");
		if (node2 != NULL)
		{
			xml->current = node2;
			node2 = pub_xml_locnode(xml, "IP");
			if (node2 == NULL || node2->value == NULL)
			{
				if (flag == 0)
				{
					pub_log_info("[%s][%d],not find LOCAL.IP,default value,127.0.0.1",
							__FILE__, __LINE__);
					strcpy(st->comm.group[index].local.ip, "127.0.0.1");
				}
			}
			else
			{
				strncpy(st->comm.group[index].local.ip, node2->value,
						sizeof(st->comm.group[index].local.ip) - 1);
			}

			node2 = pub_xml_locnode(xml, "PORT");
			if (node2 != NULL && node2->value != NULL)
			{
				st->comm.group[index].local.port = atoi(node2->value);
			}
			else if (flag == 0)
			{
				pub_log_error("[%s][%d] LOCAL not set PORT", __FILE__, __LINE__);
				xml->current = xml_bak;
				return SW_ERROR;
			}

			node2 = pub_xml_locnode(xml, "SERVICE");
			if (node2 != NULL && node2->value != NULL)
			{
				strcpy(st->comm.group[index].local.service, node2->value);
			}
			st->comm.group[index].local.use = 1;
		}

		xml->current = node1;
		node2 = pub_xml_locnode(xml, "BUFTYPE");
		if (node2 != NULL && node2->value != NULL)
		{
			strcpy(st->comm.group[index].buftype, node2->value);
		}

		st->comm.count++;
		index++;
		node1 = node1->next;
	}

	if (st->comm.count <= 0)
	{
		pub_log_error("[%s][%d]not set IP,not set COMM[%s]\n", __FILE__,
				__LINE__, st->name);
		xml->current = xml_bak;
		return SW_ERROR;
	}

	xml->current = xml_bak;
	return SW_OK;
}

/******************************************************************************
 *** name      : lsn_pub_cfg_init
 *** function  : 
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_cfg_init(sw_lsn_cycle_t *cycle, sw_char_t *lsn_name)
{
	sw_int_t	ret = 0;
	sw_char_t	buf[128];
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;

	memset(buf, 0x0, sizeof(buf));
	memset(xmlname, 0x0, sizeof(xmlname));
	if (cycle == NULL || lsn_name == NULL || lsn_name[0] == '\0')
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(xmlname, "%s/cfg/listener.lsn", cycle->base.work_dir.data);
	pub_log_info("[%s][%d] xmlname=[%s]", __FILE__, __LINE__, xmlname);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.SWLISTEN");
	while (node != NULL)
	{
		if (strcmp(node->name, "SWLISTEN") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] not find [NAME]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (strcmp(node1->value, lsn_name) == 0)
		{
			strncpy(cycle->lsn_conf.name, node1->value, sizeof(cycle->lsn_conf.name) - 1); 
			break;
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] can not find [%s]'s cfg!", __FILE__, __LINE__, lsn_name);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	nodebak = node;

	node = pub_xml_locnode(xml, "COMTYPE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(cycle->lsn_conf.comtype, node->value);
	}

	node = pub_xml_locnode(xml, "SUPER");
	if (node != NULL && node->value != NULL && atoi(node->value) == 1)
	{
		cycle->lsn_conf.issuper = 1;
	}
	else
	{
		cycle->lsn_conf.issuper = 0;
	}

	node = pub_xml_locnode(xml, "FASTCLOSE");
	if (node != NULL && node->value != NULL && node->value[0] == '1')
	{
		cycle->lsn_conf.fastclose = 1;
	}
	else
	{
		cycle->lsn_conf.fastclose = 0;
	}

	node = pub_xml_locnode(xml, "RCVTRYTIMES");
	if (node != NULL && node->value != NULL)
	{
		g_rcvtrytimes = atoi(node->value);
	}
	else
	{
		g_rcvtrytimes = 0;
	}

	node = pub_xml_locnode(xml, "RCVWAITTIME");
	if (node != NULL && node->value != NULL)
	{
		g_rcvwaittime = atol(node->value) * 1000;
	}
	else
	{
		g_rcvwaittime = 0;
	}

	node = pub_xml_locnode(xml, "TIMEOUT");
	if (node == NULL || node->value == NULL)
	{
		cycle->lsn_conf.timeout = 120;
	}
	else
	{
		cycle->lsn_conf.timeout = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "ACTIVETIME");
	if (node == NULL || node->value == NULL)
	{
		cycle->lsn_conf.activetime = 30;
	}
	else
	{
		cycle->lsn_conf.activetime = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "SCANTIME");
	if (node != NULL && node->value != NULL)
	{
		cycle->lsn_conf.scantime = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "DATA");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not find DATA!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	strncpy(cycle->lsn_conf.data, node->value, sizeof (cycle->lsn_conf.data) - 1);

	node = pub_xml_locnode(xml, "SOCKSNDBUFSIZE");
	if (node != NULL && node->value != NULL)
	{
		if (pub_str_rstrncmp(node->value, "M", 1) == 0 || pub_str_rstrncmp(node->value, "m", 1) == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			strncpy(buf, node->value, strlen(node->value) - 1);
			cycle->lsn_conf.socksndbufsize = atoi(buf) * 1024 * 1024;
		}
		else if (pub_str_rstrncmp(node->value, "K", 1) == 0 || pub_str_rstrncmp(node->value, "k", 1) == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			strncpy(buf, node->value, strlen(node->value) - 1);
			cycle->lsn_conf.socksndbufsize = atoi(buf) * 1024;
		}
		else
		{
			cycle->lsn_conf.socksndbufsize = atoi(buf);
		}
	}

	node = pub_xml_locnode(xml, "SOCKRCVBUFSIZE");
	if (node != NULL && node->value != NULL)
	{
		if (pub_str_rstrncmp(node->value, "M", 1) == 0 || pub_str_rstrncmp(node->value, "m", 1) == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			strncpy(buf, node->value, strlen(node->value) - 1);
			cycle->lsn_conf.sockrcvbufsize = atoi(buf) * 1024 * 1024;
		}
		else if (pub_str_rstrncmp(node->value, "K", 1) == 0 || pub_str_rstrncmp(node->value, "k", 1) == 0)
		{
			memset(buf, 0x0, sizeof(buf));
			strncpy(buf, node->value, strlen(node->value) - 1);
			cycle->lsn_conf.sockrcvbufsize = atoi(buf) * 1024;
		}
		else
		{
			cycle->lsn_conf.sockrcvbufsize = atoi(buf);
		}
	}

	node = pub_xml_locnode(xml, "CONMAX");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not find CONMAX!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	cycle->lsn_conf.conmax = atoi(node->value);

	node = pub_xml_locnode(xml, "PROCMAX");
	if (node == NULL || node->value == NULL)
	{
		cycle->lsn_conf.procmax = 10;
	}
	else
	{
		cycle->lsn_conf.procmax = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "PROCMIN");
	if (node == NULL || node->value == NULL)
	{
		cycle->lsn_conf.procmin = 1;
	}
	else
	{
		cycle->lsn_conf.procmin = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "ACTIVEMSG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.activemsg, node->value, sizeof (cycle->lsn_conf.activemsg) - 1);
	}

	node = pub_xml_locnode(xml, "DBID");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.db_conn_name, node->value, sizeof (cycle->lsn_conf.db_conn_name) - 1);
	}
	
	cycle->lsn_conf.out_fd = -1;
	cycle->lsn_conf.process_index = 0;

	node = pub_xml_locnode(xml, "COMM");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not find COMM!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	ret = lsn_pub_set_lsn_comm(xml, node, &cycle->lsn_conf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get comm info error!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}

	ret = lsn_pub_read_cfg(cycle, xml, nodebak);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_pub_read_cfg error!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	pub_xml_deltree(xml);
	pub_log_info("[%s][%d] init lsn cfg success!", __FILE__, __LINE__);

	return SW_OK;
}

/******************************************************************************
 *** name      : lsn_pub_connect
 *** function  : 
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_connect(sw_char_t *addr, sw_int32_t port)
{
	sw_fd_t	sock_id = 0;
	sw_int_t	ret = 0;
	sw_char_t	*ca_server_name;
	sw_int32_t	tmp_port;
	in_addr_t	inter_addr;
	struct hostent	*hp;
	struct sockaddr_in	sock_addr;

	tmp_port = port;
	ca_server_name = addr;
	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id < 0)
	{
		pub_log_error("[%s][%d] Socket error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	pub_mem_memzero((char *)&sock_addr, sizeof (sock_addr));
	hp = gethostbyname(ca_server_name);
	if (hp != NULL)
	{
		inter_addr = *((in_addr_t *) hp->h_addr);
	}
	else
	{
		if ((inter_addr = inet_addr(ca_server_name)) == INADDR_NONE)
		{
			pub_log_error("[%s][%d] inet_addr error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
			close(sock_id);
			return SW_ERROR;
		}
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(tmp_port);

#ifndef LINUX
	if (sizeof (inter_addr) > 4)
	{
		memcpy(((char *) &inter_addr) + 4, (char *) &inter_addr, 4);
		pub_mem_memzero((char *) &inter_addr, 4);
	}
#endif

	sock_addr.sin_addr.s_addr = inter_addr;
	ret = connect(sock_id, (struct sockaddr *)(&sock_addr), sizeof (struct sockaddr_in));
	if (ret)
	{
		pub_log_error("[%s][%d] Connect to [%s]:[%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, addr, port, errno, strerror(errno)); 
		close(sock_id);
		return SW_ERROR;
	}

	return sock_id;
}

/******************************************************************************
 *** name      : lsn_pub_bind
 *** function  : 
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_bind(sw_char_t *addr, sw_int32_t port)
{
	sw_fd_t		sock_id;
	sw_int_t	length = 0;
	struct sockaddr_in	sock_addr;
	struct linger		timeout;
	sw_int_t		ret = SW_ERROR;
	sw_int32_t		flag = 1;

	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id < 0)
	{
		pub_log_error("[%s][%d] Socket error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	ret = setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] setsockopt SO_REUSEADDR error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		close(sock_id);
		return -1;
	}

	length = sizeof (struct sockaddr_in);
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	timeout.l_onoff = 1;	/* 0: not block  1:block */
	timeout.l_linger = 10;	/* block timeout */
	setsockopt(sock_id, SOL_SOCKET, SO_LINGER, (char *) &timeout, sizeof (struct linger));
	ret = bind(sock_id, (struct sockaddr *)&sock_addr, length);
	setsockopt(sock_id, SOL_SOCKET, SO_LINGER, (char *)&timeout, sizeof (struct linger));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Bind [%s]:[%d] error! ERRNO=[%d]:[%s]",
			__FILE__, __LINE__, addr, port, errno, strerror(errno));
		return SW_ERROR;
	}

	ret = listen(sock_id, 128);
	if (ret < 0)

	{
		pub_log_error("listen error [%s][%d][%d]", __FILE__, __LINE__, errno);
		return SW_ERROR;
	}
	return sock_id;
}

sw_int_t lsn_set_fd_noblock(sw_fd_t fd)
{
	if ( fd < 0)
	{
		pub_log_error("[%s][%d] lsn_set noblock argument error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	sw_int32_t flags;
	sw_int_t ret;

	flags = fcntl(fd, F_GETFL, 0);
	ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if(ret)
	{
		return SW_ERROR;
	}
	return SW_OK;
}
/******************************************************************************
 *** name      : lsn_pub_accept
 *** function  : recv a connect
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_fd_t lsn_pub_accept(sw_fd_t sockid)
{
	sw_fd_t		accept_id;
	socklen_t	length;
	struct sockaddr_in	sock_addr;

	pub_mem_memzero(&sock_addr, sizeof(struct sockaddr_in));
	length = sizeof(struct sockaddr_in);

	accept_id = accept(sockid, (struct sockaddr *)&sock_addr, &length);

	return accept_id;
}

/******************************************************************************
 *** name      : lsn_pub_find_free_fd
 *** function  :  find the available free space, used to store receives the link info


 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_find_free_fd(sw_lsn_cycle_t *cycle, sw_fd_t accept_id)
{
	sw_int32_t	i = 0;
	sw_int32_t	index = 0;
	sw_int32_t	wait_cnt = 0;

	if (accept_id < 0)
	{
		pub_log_error("[%s][%d]input param error fd=%d", __FILE__, __LINE__, accept_id);
		return SW_ERROR;
	}

	wait_cnt = cycle->lsn_conf.conmax;
	if (cycle->link_info == NULL || wait_cnt <= 0)
	{
		pub_log_error("[%s][%d]stored space error,addr=[%x],cnt=[%d]",
				__FILE__, __LINE__, cycle->link_info, wait_cnt);
		return SW_ERROR;
	}

	index = accept_id % wait_cnt;
	if ((cycle->link_info[index].use <= 0) && (cycle->link_info[index].sockid < 0))
	{
		return index;
	}
	else
	{
		for (i = 0; i < wait_cnt; i++)
		{
			if ((cycle->link_info[i].use <= 0) && (cycle->link_info[i].sockid < 0))

			{
				return i;
			}
		}
	}
	return SW_ERROR;
}

sw_int_t lsn_find_index_by_fd(sw_lsn_cycle_t *cycle, sw_fd_t fd)
{
	sw_int_t	i = 0;

	if (cycle == NULL || fd < 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	i = fd % cycle->lsn_conf.conmax;
	if (cycle->link_info[i].sockid == fd)
	{
		return i;
	}

	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].sockid == fd)
		{
			return i;
		}
	}

	return SW_ERROR;
}

sw_int_t lsn_find_index_by_mtype(sw_lsn_cycle_t *cycle, sw_int_t mtype)
{
	int	index = 0;

	if (cycle == NULL || mtype <= 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	for (index = 0; index < cycle->lsn_conf.conmax; index++)
	{
		if (cycle->link_info[index].sockid > 0 && cycle->link_info[index].mtype == mtype)
		{
			return index;
		}
	}

	pub_log_error("[%s][%d] Can not find old fd! mtype=[%d]",
			__FILE__, __LINE__, mtype);
	return -1;
}

sw_fd_t lsn_get_fd_by_index(sw_lsn_cycle_t *cycle, int index)
{
	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	if (index >= cycle->lsn_conf.conmax)
	{
		pub_log_error("[%s][%d] Index error! index=[%d]", __FILE__, __LINE__, index);
		return -1;
	}

	return cycle->link_info[index].sockid;
}

/******************************************************************************
 *** name      : lsn_pub_find_old_fd
 *** function  : 
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_find_old_fd(sw_lsn_cycle_t *cycle, sw_int_t mtype, sw_fd_t *fd)
{
	sw_int32_t	i = 0;
	sw_int32_t	wait_cnt = 0;

	if (mtype <= 0 || fd == NULL)
	{
		pub_log_error("[%s][%d] Param error! mtype=[%d] Fd=[%d]",
				__FILE__, __LINE__, mtype, fd);
		return -1;
	}

	wait_cnt = cycle->lsn_conf.conmax;
	for (i = 0; i < wait_cnt; i++)
	{
		if (cycle->link_info[i].sockid > 0 && cycle->link_info[i].mtype == mtype)
		{
			*fd = cycle->link_info[i].sockid;
			break;
		}
	}
	if (i == wait_cnt)
	{
		pub_log_error("[%s][%d] Can not find old fd, mtype=[%d]", __FILE__, __LINE__, mtype);
		return -1;
	}

	return i;
}

/******************************************************************************
 *** name      : lsn_pub_close_fd
 *** function  : close fd
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_close_fd(sw_lsn_cycle_t *cycle, sw_int32_t index)
{
	if (cycle->link_info == NULL || index < 0 || index >= cycle->lsn_conf.conmax)
	{
		pub_log_error("[%s][%d] Param error! index=[%d]", __FILE__, __LINE__, index);
		return SW_ERROR;
	}

	close(cycle->link_info[index].sockid);
	pub_log_info("[%s][%d] CLOSE FD:[%d]", __FILE__, __LINE__, cycle->link_info[index].sockid);

	cycle->link_info[index].sockid = -1;
	cycle->link_info[index].mtype = -1;
	cycle->link_info[index].trace_no = 0;
	cycle->link_info[index].start_time = 0;
	cycle->link_info[index].use = 0;

	return SW_OK;
}

/******************************************************************************
 *** name      : lsn_pub_recv_len
 *** function  : recieve data 
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_recv_len(sw_fd_t sockid, sw_char_t *buf, sw_int32_t length)
{
	int	ret = 0;
	int	times = 0;
	int	trytimes = 0;
	int	sub_len = 0;
	fd_set	readmask;
	struct timeval	timeout;

	trytimes = g_rcvtrytimes > 0 ? g_rcvtrytimes : 3;
	errno = 0;
	while (1)
	{
		FD_ZERO(&readmask);
		FD_SET(sockid, &readmask);

		memset(&timeout, 0x0, sizeof(timeout));
		timeout.tv_sec = 0;
		timeout.tv_usec = g_rcvwaittime > 0 ? g_rcvwaittime : 500000;
		ret = select(sockid + 1, &readmask, NULL, NULL, &timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			pub_log_error("[%s][%d] select [%d] error, errno=[%d]:[%s]",
					__FILE__, __LINE__, sockid, errno, strerror(errno));
			return ret;
		}
		else if (ret == 0)
		{
			times++;
			if (times > trytimes)
			{
				pub_log_error("[%s][%d] Retry [%d] times, recv:[%d]:[%d]",
						__FILE__, __LINE__, times, sub_len, length);
				return sub_len;
			}
			continue;
		}

		ret = recv(sockid, buf + sub_len, length - sub_len, 0);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			pub_log_error("[%s][%d] 接收长度不足! times=[%d][%d][%d]",
					__FILE__, __LINE__, times, sub_len, length);
			return sub_len;
		}
		else if (ret == 0)
		{
			pub_log_error("[%s][%d] 对端链接已关闭! times=[%d][%d][%d]",
					__FILE__, __LINE__, times, sub_len, length);
			return sub_len;
		}
		else
		{
			sub_len += ret;
			if (sub_len >= length)
			{
				return length;
			}
		}
	}

	return 0;
}

sw_int_t lsn_pub_recv_odata(sw_fd_t sockid, sw_char_t *buf, sw_int32_t length)
{
	int	ret = 0;
	int	times = 0;
	int	trytimes = 0;
	int	sub_len = 0;
	fd_set	readmask;
	struct timeval	timeout;

	trytimes = g_rcvtrytimes > 0 ? g_rcvtrytimes : 3;
	errno = 0;
	while (1)
	{
		FD_ZERO(&readmask);
		FD_SET(sockid, &readmask);

		memset(&timeout, 0x0, sizeof(timeout));
		timeout.tv_sec = 0;
		timeout.tv_usec = g_rcvwaittime > 0 ? g_rcvwaittime : 500000;
		ret = select(sockid + 1, &readmask, NULL, NULL, &timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			pub_log_error("[%s][%d] select [%d] error, errno=[%d]:[%s]",
					__FILE__, __LINE__, sockid, errno, strerror(errno));
			return ret;
		}
		else if (ret == 0)
		{
			times++;
			if (times > trytimes)
			{
				pub_log_info("[%s][%d] Retry [%d] times, recv:[%d]:[%d]",
						__FILE__, __LINE__, times, sub_len, length);
				return sub_len;
			}
			continue;
		}

		ret = recv(sockid, buf + sub_len, length - sub_len, 0);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			pub_log_error("[%s][%d] 接收长度不足! times=[%d][%d][%d]",
					__FILE__, __LINE__, times, sub_len, length);
			return sub_len;
		}
		else if (ret == 0)
		{
			pub_log_error("[%s][%d] 对端链接已关闭! times=[%d][%d][%d]",
					__FILE__, __LINE__, times, sub_len, length);
			return sub_len;
		}
		else
		{
			sub_len += ret;
/*			if (sub_len >= length)
			{
				return length;
			}*/
			return sub_len;
		}
	}

	return 0;
}

/******************************************************************************
 *** name      : lsn_pub_send
 *** function  : send data to client
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_send(sw_fd_t sock_id, sw_char_t * buf, sw_int32_t length)
{
	sw_int32_t	sub_len = 0;
	sw_int32_t	buf_len = 0;
	sw_int32_t	times = 0;

	if (sock_id < 0 || length <= 0 || buf == NULL)
	{
		pub_log_error("[%s][%d] input param error, sockid=[%d] len=[%d] buf=[%s]",
				__FILE__, __LINE__, sock_id, length, buf);
		return SW_ERROR;
	}

	errno = 0;
	while (1)
	{
		buf_len = send(sock_id, buf + sub_len, length - sub_len, 0);
		if (buf_len < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			if (errno == EAGAIN)
			{
				times++;
				pub_log_info("[%s][%d] times=[%d] errno=[%d]:[%s]", 
						__FILE__, __LINE__, times, errno, strerror(errno));
				usleep(100000);
				continue;
			}
			pub_log_error("[%s][%d] send error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));

			return buf_len;
		}
		else if (buf_len == 0)
		{
			times++;
			if (times > 3)
			{
				pub_log_error("[%s][%d]send pkg to net error[%s]",
						__FILE__, __LINE__, strerror(errno));
				return SW_ERROR;
			}
			usleep(100000);
			continue;
		}
		sub_len += buf_len;
		if (sub_len >= length)
		{
			pub_log_info("[%s][%d] sub_len=[%d]", __FILE__, __LINE__, sub_len);
			return SW_OK;
		}
	}
	return SW_ERROR;
}

/******************************************************************************
 *** name      : lsn_pub_recv
 *** function  : recieve data
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_recv(sw_char_t * data, sw_fd_t sock_id, sw_buf_t * pkg_vars)
{
	if (data[0] == '\0')
	{
		pub_log_error("[%s][%d]data format error  = [%s]!", __FILE__,
				__LINE__, data);
		return SW_ERROR;
	}

	return (lsn_pub_recv_ext(sock_id, pkg_vars, data));
}

/******************************************************************************
 *** name      : lsn_pub_getformat_func
 *** function  : get a func form the format string
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_getformat_func(sw_char_t *format, sw_int32_t index, sw_char_t *fun_name, sw_char_t *args)
{
	sw_int32_t	i = 0;
	sw_int32_t	count = 0;
	sw_char_t	*ptr = NULL;

	while (format[i] != '\0')
	{
		if (format[i] == '(')
		{
			if (count == index)
			{
				break;
			}
			count++;
		}
		i++;
	}

	if (i == 0)
	{
		pub_log_error("data format define error[%s]\n", format);
		return SW_ERROR;
	}

	if (format[i] == '\0')
	{
		return 1;
	}

	ptr = format + i - 1;
	*fun_name = *ptr;
	i = 0;
	ptr += 2;

	while (*ptr != ')' && *ptr != '\0')
	{
		args[i] = *ptr;
		i++;
		ptr++;
	}
	args[i] = '\0';

	return 0;
}

/******************************************************************************
 *** name      : deal_hex_data
 *** function  : process using \x hexadecimal data format
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    :
 ***	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : i:success  -1:fail
 *** notice    : 
 *** modified  :
 ***	author :
 ***	date   :
 ***	content: 
 ******************************************************************************/
sw_int_t deal_hex_data(sw_char_t * str, sw_char_t * buf)
{
	sw_int32_t i, j;

	i = j = 0;
	buf[0] = '\0';
	while (str[i] != '\0')
	{
		if (str[i] == '\\' && (str[i + 1] == 'x' || str[i + 1] == 'X'))
		{
			sw_uchar_t ch1, ch2;
			ch1 = str[i + 2];
			ch2 = str[i + 3];

			if (ch1 >= 'A' && ch1 <= 'F')
			{
				ch1 = ch1 - 'A' + 10;
			}
			else if (ch1 >= 'a' && ch1 <= 'f')
			{
				ch1 = ch1 - 'a' + 10;
			}
			else if (ch1 >= '0' && ch1 <= '9')
			{
				ch1 = ch1 - '0';
			}
			else
			{
				ch1 = 0;
			}

			if (ch2 >= 'A' && ch2 <= 'F')
			{
				ch2 = ch2 - 'A' + 10;
			}
			else if (ch2 >= 'a' && ch2 <= 'f')
			{
				ch2 = ch2 - 'a' + 10;
			}
			else if (ch2 >= '0' && ch2 <= '9')
			{
				ch2 = ch2 - '0';
			}
			else
			{
				ch2 = 0;
			}

			buf[j] = (char) (ch1 * 16 + ch2);
			i += 4;
			j++;
		}
		else if (str[i] == '\\' && str[i + 1] == '\\')
		{
			buf[j] = '\\';
			i += 2;
			j++;
		}
		else
		{
			buf[j] = str[i];
			i++;
			j++;
		}
	}
	buf[j] = '\0';
	return (j);
}

/******************************************************************************
 *** name      : check_fd_is_write
 *** function  : check whether the FIFO is free to write
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    :
 ***	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 0:not write -1:fail 1:write
 *** notice    : 
 *** modified  :
 ***	author :
 ***	date   :
 ***	content:  non-blocking mode, return immediately after judgment
 ******************************************************************************/
sw_int_t check_fd_is_write(sw_fd_t sock_id)
{
	sw_int_t	ret;
	fd_set		rmask;
	fd_set		rmask_err;
	struct timeval	time_out;

	time_out.tv_sec = 0;
	time_out.tv_usec = 0;

	FD_ZERO(&rmask);
	FD_ZERO(&rmask_err);
	FD_SET((sw_uint_t) sock_id, &rmask);
	FD_SET((sw_uint_t) sock_id, &rmask_err);

	if (sock_id < 0)
	{
		pub_log_error("[%s][%d]check FIFO state error[%d]!", __FILE__,
				__LINE__, sock_id);
		return SW_ERROR;
	}

	while (1)
	{
		ret = select(sock_id + 1, 0, &rmask, &rmask_err, &time_out);
		if (ret > 0)
		{
			if (FD_ISSET(sock_id, &rmask_err))
			{
				pub_log_error("[%s][%d]check FIFO state error[%d]!",
						__FILE__, __LINE__, errno);
				return SW_ERROR;
			}
			return (1);
		}
		else if (ret == 0)
		{
			return (0);
		}
		else
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				pub_log_error("[%s][%d]check FIFO state error[%d]!",
						__FILE__, __LINE__, errno);
				return SW_ERROR;
			}
		}
	}

	return (0);
}

static long str_atol(char *line, int n)
{
	long	value = 0;

	if (n <= 0)
	{
		return 0;
	}

	while ((*line == ' ' || *line == '\t' || *line == '\r' || *line == 'n') && n--)
	{
		line++;
	}

	for (value = 0; *line != '\0' && n--; line++)
	{
		if (*line < '0' || *line > '9')
		{
			break;
		}

		value = value * 10 + (*line - '0');
	}

	return value;
}

/******************************************************************************
 *** name      : lsn_pub_recv_ext
 *** function  : message write the cache
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn cycle object
name : module name
module_type : module type
err_log : error log name
dbg_log : debug log name
 *** outputs   : 
 *** return    : 1:success  0:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_recv_ext(sw_fd_t sock_id, sw_buf_t * pkg_vars, sw_char_t * data)
{
	sw_int32_t	num = 0;
	sw_int32_t	size = 0;
	sw_int_t	ret = SW_ERROR;
	sw_int32_t	len = 0;	/* func L variable */
	sw_int32_t	index = 0;
	sw_int32_t	total_len = 0;
	sw_int32_t	once_len = 0;
	sw_char_t	*buf;
	sw_char_t	args[32];
	sw_char_t	func_format;

	if (data[0] == '\0' || data == NULL || pkg_vars == NULL || sock_id < 0)
	{
		pub_log_error("[%s][%d]data format error  = [%s]!", __FILE__,
				__LINE__, data);
		return SW_ERROR;
	}

	buf = pkg_vars->data;
	pkg_vars->len = 0;
	while (1)
	{
		pub_mem_memzero(args, sizeof (args));
		ret = lsn_pub_getformat_func(data, index, &func_format, args);
		if (ret < 0)
		{
			return SW_ERROR;
		}
		else if (ret > 0)
		{
			break;
		}
		else
		{
			/* obtain a func,receives a message on it */
			if (func_format == 'H')
			{
				/* H func receive host sequence numbers for length */
				num = atoi(args);
				if (num <= 0)
				{
					pub_log_error("[%s][%d]recv data error!",
							__FILE__, __LINE__);
					return SW_ERROR;
				}

				once_len = lsn_pub_recv_len(sock_id, buf + total_len, num);
				if (once_len != num)
				{
					pub_log_error(
							"[%s][%d]recv data error[%d][%d]!", __FILE__,
							__LINE__, once_len, errno);
					return SW_ERROR;
				}

				len = pub_code_hosttonum(buf + total_len, num);
				if (len < 0)
				{
					pub_log_error("[%s][%d]recv data error[%d][%d]!", 
							__FILE__, __LINE__, once_len, errno);
					return SW_ERROR;
				}
				pub_log_info("H func calculate the length[%d]", len);

				size = len + num + 1;
				pkg_vars->len += once_len;
				if (pub_buf_chksize(pkg_vars, size) != SW_OK)
				{
					pub_log_error("[%s][%d] update buf error! size=[%d]", 
							__FILE__, __LINE__, pkg_vars->size);
					return SW_ERROR;
				}
				buf = pkg_vars->data;
				total_len += once_len;
			}
			else if (func_format == 'X')
			{
				/* X func receive the hex numbers for length */
				num = atoi(args);
				if (num <= 0)
				{
					pub_log_error("[%s][%d]recv data error!",
							__FILE__, __LINE__);
					return SW_ERROR;
				}

				once_len = lsn_pub_recv_len(sock_id, buf + total_len, num);
				if (once_len != num)
				{
					pub_log_error(
							"[%s][%d]recv data error[%d][%d]!", __FILE__,
							__LINE__, once_len, errno);
					return SW_ERROR;
				}

				len = pub_code_hex2int((u_char *)(buf + total_len), num);
				if (len < 0)
				{
					pub_log_error("[%s][%d]recv data error[%d][%d]!", 
							__FILE__, __LINE__, once_len, errno);
					return SW_ERROR;
				}
				pub_log_info("X func calculate the length[%d]", len);

				size = len + num + 1;
				pkg_vars->len += once_len;
				if (pub_buf_chksize(pkg_vars, size) != SW_OK)
				{
					pub_log_error("[%s][%d] update buf error! size=[%d]", 
							__FILE__, __LINE__, pkg_vars->size);
					return SW_ERROR;
				}
				buf = pkg_vars->data;
				total_len += once_len;
			}
			else if (func_format == 'N')
			{

				/* N func receive the network order numbers for length */
				num = atoi(args);
				if (num <= 0)
				{
					pub_log_error("[%s][%d]recv data error!",
							__FILE__, __LINE__);
					return SW_ERROR;
				}

				once_len = lsn_pub_recv_len(sock_id, buf + total_len, num);
				if (once_len != num)
				{
					pub_log_error(
							"[%s][%d]recv data error[%d][%d]!", __FILE__,
							__LINE__, once_len, errno);
					return SW_ERROR;
				}

				len = pub_code_nettonum(buf + total_len, num);
				if (len < 0)
				{
					pub_log_error(
							"[%s][%d]recv data error[%d][%d]!", __FILE__,
							__LINE__, once_len, errno);
					return SW_ERROR;
				}

				pub_log_info("N func calculate the length[%d]", len);
				size = len + num + 1;
				pkg_vars->len += once_len;
				if (pub_buf_chksize(pkg_vars, size) != SW_OK)
				{
					pub_log_error("[%s][%d] update buf error! size=[%d]", 
							__FILE__, __LINE__, pkg_vars->size);
					return SW_ERROR;
				}
				buf = pkg_vars->data;
				total_len += once_len;
			}
			else if (func_format == 'A')
			{
				/* A func recieve ASCII for length */
				sw_int32_t i;
				num = atoi(args);
				if (num <= 0)
				{
					pub_log_error("[%s][%d]recv data error!",
							__FILE__, __LINE__);
					return SW_ERROR;
				}

				once_len = lsn_pub_recv_len(sock_id, buf + total_len, num);
				if (once_len != num)
				{
					pub_log_error(
							"[%s][%d]recv data error[%d][%s]!", __FILE__,
							__LINE__, once_len, strerror(errno));
					return SW_ERROR;
				}

				for (i = 0; i < num; i++)
				{
					if ((buf[total_len + i] > '9' || buf[total_len + i] < '0')
							&& buf[total_len + i] != ' ')
					{
						pub_log_bin(SW_LOG_ERROR, buf + total_len, num,
								"recv length of the data error");
						return SW_ERROR;
					}
				}

				len = str_atol(buf + total_len, num);
				if (len < 0)
				{
					pub_log_error("[%s][%d]recv data error[%d][%d]!\n",
							__FILE__, __LINE__, once_len, errno);
					return SW_ERROR;
				}

				pub_log_info("A func calculate the length[%d]", len);
				size = len + num + 1;
				pkg_vars->len += once_len;
				if (pub_buf_chksize(pkg_vars, size) != SW_OK)
				{
					pub_log_error("[%s][%d] update buf error! size=[%d]", 
							__FILE__, __LINE__, pkg_vars->size);
					return SW_ERROR;
				}
				buf = pkg_vars->data;
				total_len += once_len;
			}
			else if (func_format == 'R')
			{
				/* R func recieve specified length */
				sw_char_t *ptr;
				ptr = pub_str_strchr(args, '+');
				if (ptr == NULL)
				{
					ptr = pub_str_strchr(args, '-');
					if (ptr == NULL)
					{

						/* no arithmetic operators */
						if (args[0] == 'L')
						{
							num = len;
						}
						else
						{
							num = atoi(args);
						}
					}
					else
					{
						/* subtraction */
						if (args[0] != 'L')
						{
							pub_log_error(
									"[%s][%d]currently supports only begin with L subtract!\n",
									__FILE__, __LINE__);
							return SW_ERROR;
						}
						num = len - atoi(ptr + 1);
					}
				}
				else
				{	/* addition */
					if (args[0] != 'L')
					{
						pub_log_error(
								"[%s][%d]currently supports only begin with L add operation !\n",
								__FILE__, __LINE__);
						return SW_ERROR;
					}
					num = len + atoi(ptr + 1);
				}

				if (num < 0)
				{
					pub_log_error("[%s][%d]recv data error!",
							__FILE__, __LINE__);
					return SW_ERROR;
				}
				else if (num == 0)
				{
				}
				else
				{
					pub_log_info("[%s][%d] recv len=[%d]", __FILE__, __LINE__, num);
					once_len = lsn_pub_recv_len(sock_id, buf + total_len, num);
					if (once_len != num)
					{
						pub_log_error(
								"[%s][%d]recv data error[%d][%d]!",
								__FILE__, __LINE__, once_len, errno);
						return SW_ERROR;
					}
					total_len += once_len;
				}
				pub_log_info("[%s][%d] R fun recv len=[%d]", __FILE__, __LINE__, num);
			}
			else if (func_format == 'T')
			{
				/* T func recieve specified character of the end of symbols */
				sw_int32_t i;
				sw_int32_t tail_cnt = 0;
				sw_char_t tail_buf[33];
				pub_mem_memzero(tail_buf, sizeof (tail_buf));
				i = 0;

				while (args[i] != '\0')
				{
					if (args[i] != '\\'
							|| (args[i + 1] != 'x' && args[i + 1] != 'X'))
					{
						tail_buf[tail_cnt] = args[i];
						tail_cnt++;
						i++;
					}
					else
					{
						sw_char_t tmp[5];
						if (args[i + 2] == '\0' || args[i + 3] == '\0')
						{
							pub_log_error(
									"[%s][%d]data format error[%d][%d]!",
									__FILE__, __LINE__, once_len,
									errno);
							return SW_ERROR;
						}
						pub_mem_memcpy(tmp, args + i, 4);
						tmp[0] = '0';
						tmp[4] = '\0';
						tail_buf[tail_cnt] = strtol(tmp, NULL, 0);
						pub_log_error(
								"[%s][%d]the end character is[%s][%d]\n",
								__FILE__, __LINE__, tmp,
								tail_buf[tail_cnt]);
						tail_cnt++;
						i += 4;
					}
				}

				once_len = lsn_pub_recv_len(sock_id, buf + total_len, tail_cnt);
				if (once_len != tail_cnt)
				{
					pub_log_error(
							"[%s][%d]recv data error[%d][%d]!", __FILE__,
							__LINE__, once_len, errno);
					return SW_ERROR;
				}

				if (pub_mem_memcmp(buf + total_len, tail_buf, tail_cnt) == 0)
				{
					total_len += once_len;
				}
				else
				{
					total_len += once_len;
					while (1)
					{
						once_len =
							lsn_pub_recv_len(sock_id, buf + total_len,
									1);
						if (once_len != 1)
						{
							pub_log_error(
									"[%s][%d]recv data error[%d][%d]!",
									__FILE__, __LINE__, once_len,
									errno);
							return SW_ERROR;
						}

						if (pub_mem_memcmp
								(buf + total_len + 1 - tail_cnt, tail_buf,
								 tail_cnt) == 0)
						{
							total_len++;
							break;
						}
						total_len++;
					}
				}
			}
			else if (func_format == 'O')
			{
				/* O func recv only one */
				num = atoi(args);
				if (num <= 0)
				{
					pub_log_error("[%s][%d]recv data error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				pub_log_info("[%s][%d] O(%s)计算长度=[%d]", __FILE__, __LINE__, args, num);
				if (pub_buf_chksize(pkg_vars, num) != SW_OK)
				{
					pub_log_error("[%s][%d] update buf error! size=[%d]",
							__FILE__, __LINE__, pkg_vars->size);
					return SW_ERROR;
				}

				buf = pkg_vars->data;
				once_len = lsn_pub_recv_odata(sock_id, buf + total_len, num);
				if (once_len < 0)
				{
					pub_log_error("[%s][%d]recv data error[%d][%s]!", 
							__FILE__, __LINE__, once_len, strerror(errno));
					return SW_ERROR;
				}
				total_len += once_len;
			}
			else if (func_format == 'S')
			{
				/*** 每次接收固定大小的数据,如果接收的数据小于这个固定值,那么认为接收数据结束 ***/
				int	ret = 0;
				char resbuf[] = "<?xml version=\"1.0\" encoding=\"GB2312\"?><RETURNINFO RESULT=\"SUCCESS\" COMMAND=\"XXXX\" INFORMATION=\"XXXX\"></RETURNINFO>";
				size = atoi(args);
				pub_log_debug("[%s][%d] S函数,size=[%d]", __FILE__, __LINE__, size);
				while (1)
				{
					if (pub_buf_chksize(pkg_vars, size) != SW_OK)
					{
						pub_log_error("[%s][%d] update buf error! size=[%d]",
								__FILE__, __LINE__, pkg_vars->size);
						return SW_ERROR;
					}
					buf = pkg_vars->data;
					once_len = lsn_pub_recv_len(sock_id, buf + total_len, size);
					if (once_len < 0)
					{
						if (errno == EINTR)
						{
							continue;
						}
						pub_log_error("[%s][%d] recv error! errno=[%d]:[%s]",
								__FILE__, __LINE__, errno, strerror(errno));
						return SW_ERROR;
					}
					pub_log_info("[%s][%d] recv data len:[%d]", __FILE__, __LINE__, once_len);
					if (once_len == size)
					{
						ret = lsn_pub_send(sock_id, resbuf, strlen(resbuf));
						if (ret < 0)
						{
							pub_log_error("[%s][%d] send resbuf error! errno=[%d]:[%s]",
									__FILE__, __LINE__, errno, strerror(errno));
							return SW_ERROR;
						}
					}
					total_len += once_len;

					if (once_len < size)
					{
						pub_log_info("[%s][%d] once_len=[%d]", __FILE__, __LINE__, once_len);
						break;
					}
				}
			}
		}
		index++;
	}
	pkg_vars->len = total_len;
	pkg_vars->data[pkg_vars->len] = '\0';
	return pkg_vars->len;
}

/******************************************************************************
 *** name      : 
 *** function  : 
 *** author    : 
 *** create    : 
 *** call lists: 
 *** inputs    : 
 *** 	  cycle : lsn  object
 *** outputs   : 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t lsn_pub_deal_timeout(sw_lsn_cycle_t * cycle)
{
	sw_int32_t	i = 0;
	sw_time_t	now_time = -1;
	sw_int32_t	timeout = 0;
	sw_cmd_t	cmd;
	sw_int_t	ret = 0;
	sw_char_t	buf[64];
	sw_loc_vars_t	vars;

	memset(buf, 0x0, sizeof(buf));
	if (cycle == NULL)
	{
		pub_log_info("[%s][%d] Param is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		if (cycle->link_info[i].sockid >= 0 && cycle->link_info[i].use == 1)
		{
			now_time = (sw_int_t)time(NULL);
			timeout = cycle->link_info[i].timeout;
			if (timeout == 0)
			{
				timeout = cycle->lsn_conf.timeout;
			}
			pub_log_debug("[%s][%d] timeout===[%d]", __FILE__, __LINE__, timeout);
			if (cycle->link_info[i].start_time != 0 && now_time - cycle->link_info[i].start_time > timeout)
			{
				pub_log_info("[%s][%d]Socket[%d][%d][%d] TIMEOUT!", 
						__FILE__, __LINE__, i, cycle->link_info[i].sockid, cycle->link_info[i].trace_no);
				if (cycle->link_info[i].cmd_type == SW_CALLLSNREQ)
				{
					pub_log_info("[%s][%d] CALLLSN LSNOUT! mtype=[%d] traceno=[%lld]",
							__FILE__, __LINE__, cycle->link_info[i].mtype, cycle->link_info[i].trace_no);
					ret = pub_loc_vars_alloc(&vars, SHM_VARS);
					if (ret == SW_ERROR)
					{
						pub_log_bend("[%s][%d] pub_loc_vars_alloc error!",
								__FILE__, __LINE__);
						continue;
					}
					pub_log_info("[%s][%d] vars alloc success! mtype=[%d]traceno=[%lld]", 
							__FILE__, __LINE__, cycle->link_info[i].mtype, cycle->link_info[i].trace_no);

					memset(buf, 0x0, sizeof(buf));
					sprintf(buf, "shm%08d", cycle->link_info[i].mtype);
					ret = vars.unserialize(&vars, buf);
					if (ret == SW_ERROR)
					{
						vars.free_mem(&vars);
						pub_log_bend("[%s][%d] vars unserialize error! mtype=[%d]",
								__FILE__, __LINE__, cycle->link_info[i].mtype);
						continue;
					}
					char	chnl[64];
					memset(chnl, 0x0, sizeof(chnl));
					loc_get_zd_data(&vars, "$listen", chnl);
					alog_set_sysinfo(cycle->link_info[i].mtype, cycle->link_info[i].cmd.sys_date,
							cycle->link_info[i].cmd.trace_no, chnl);
					pub_log_info("[%s][%d] vars unserialize success! mtype=[%d]",
							__FILE__, __LINE__, cycle->link_info[i].mtype);
					lsn_set_err(&vars, ERR_LSNOUT);
					memcpy(&cmd, &cycle->link_info[i].cmd, sizeof(sw_cmd_t));
					lsn_deal_err(cycle, &vars, &cmd);
					vars.free_mem(&vars);
				}
				select_del_event(cycle->lsn_fds, cycle->link_info[i].sockid);
				lsn_pub_close_fd(cycle, i);
				pub_log_bend("[%s][%d] close link socket[%d]", __FILE__, __LINE__, i);
				continue;
			}
		}
	}
	return SW_OK;
}

int get_proc_index(char *proc_name, int *index)
{
	char    *ptr = NULL;
	char    *tmp = NULL;
	char    buf[32];

	memset(buf, 0x0, sizeof(buf));
	ptr = proc_name;
	while (*ptr != '_' && *ptr != '\0')
	{
		ptr++;
	}                                                                                                                                  
	if (*ptr == '\0')
	{                                                                                                                                  
		return -1;
	}
	ptr++;
	tmp = buf;
	while (*ptr != '\0')
	{
		if (*ptr >= '0' && *ptr <= '9')
		{
			*tmp++ = *ptr++;
			continue;
		}
		break;
	}
	*tmp = '\0';
	*index = atoi(buf);

	return 0;                                                                                                                          
}

sw_int_t lsn_handle_timeout(sw_lsn_cycle_t *cycle)
{
	int	j = 0;
	int	index = 0;
	int	ret = 0;
	long	lnow = 0;
	sw_int32_t	sendtype = -1;
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;

	grp_svr = procs_get_svr_by_name(cycle->base.name.data, NULL);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] procs_get_svr_by_name error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] svr=[%s]", __FILE__, __LINE__, grp_svr->svrname);

	for (j = 0; j < grp_svr->svc_curr; j++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}

		ret = pub_proc_checkpid(proc_info.pid);
		if (ret != SW_OK && proc_info.restart_cnt >= 0)
		{
			pub_log_info("[%s][%d] Process [%d] abnormal! name=[%s]", 
					__FILE__, __LINE__, proc_info.pid, proc_info.name);
			index = proc_info.proc_index;
			pub_log_info("[%s][%d] name=[%s] index=[%d]", __FILE__, __LINE__, proc_info.name, index);
			lnow = (long)time(NULL);
			if (lnow - proc_info.starttime < MIN_RESTART_TIME)
			{
				alert_msg(ERR_OFTEN_RESTART_FAILED, "紧急预警:进程[%s]5分钟之内重启多次,请查看渠道日志进行手工处理!", proc_info.name);
				pub_log_error("[%s][%d] [%s][%d] Time is less than 5 minutes, could not restart!",
						__FILE__, __LINE__, proc_info.name, proc_info.pid);
				proc_info.status = SW_S_ABNORMAL;
				proc_info.restart_cnt = -1;
				ret = procs_lsn_register(&proc_info);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
				}
				continue;
			}
			if (proc_info.restart_cnt == 0)
			{
				alert_msg(ERR_CNT_RESTART_FAILED, "紧急预警:进程[%s]重启次数过多,请查看渠道日志进行手工处理!", proc_info.name);
				pub_log_error("[%s][%d] [%s][%d] restart_cnt=[%d] Could not restart!",
						__FILE__, __LINE__, proc_info.name, proc_info.pid, proc_info.restart_cnt);
				proc_info.status = SW_S_ABNORMAL;
				proc_info.restart_cnt--;
				ret = procs_lsn_register(&proc_info);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
				}
				continue;
			}

			alert_msg(ERR_PROC_FAILED, "紧急预警:进程[%s]异常系统准备重启,请手动查看渠道日志.?",proc_info.name);
			proc_info.status = SW_S_ABNORMAL;
			proc_info.restart_cnt--;
			ret = procs_lsn_register(&proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
				continue;
			}

			if (strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0)
			{
				if (proc_info.type == ND_LSN_RCV)
				{
					sendtype = O_RECV;
				}
				else
				{
					sendtype = O_SEND;
				}
				ret = lsn_create_single_child_mq(cycle, proc_info.group_index, proc_info.proc_index, sendtype);
			}
			else if (strcmp(cycle->lsn_conf.comtype, "TCPLA") == 0)
			{
				if (proc_info.type == ND_LSN_RCV)
				{
					sendtype = O_RECV;
				}
				else
				{
					sendtype = O_SEND;
				}
				ret = lsn_create_single_child_la(cycle, proc_info.group_index, sendtype);
			}
			else if (strcmp(cycle->lsn_conf.comtype, "TCPLC") == 0)
			{    
				ret = lsn_create_child_tcplc(cycle);
			}
			else
			{
				ret = lsn_create_single_child_common(cycle, index);
			}
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] create single child error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
		pub_log_debug("[%s][%d] name=[%s] pid=[%d] type=[%d] status=[%d] p_index=[%d] g_index=[%d] busy=[%d]",
				__FILE__, __LINE__, proc_info.name, proc_info.pid, proc_info.type, 
				proc_info.status, proc_info.proc_index, proc_info.group_index, proc_info.busy_flag);
	}

	return SW_OK;
}

sw_int32_t lsn_get_libfunc_handle(sw_lsn_cycle_t *cycle, sw_char_t *name, sw_char_t *outlib, sw_char_t *outfunc)
{
	sw_char_t	buf[256];
	sw_char_t	libso[256];
	sw_char_t	*ptr = NULL;

	if (cycle == NULL || name == NULL || outlib == NULL || outfunc == NULL || strlen(name) == 0)
	{
		pub_log_error("[%s][%d] Get lib func param error!", __FILE__, __LINE__);
		return -1;
	}

	memset(buf, 0x0, sizeof(buf));
	memset(libso, 0x0, sizeof(libso));

	strcpy(buf, name);
	ptr = strchr(buf, '/');
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] [%s] Format error! [lib/fun]", 
				__FILE__, __LINE__, buf);
		return -1; 
	}
	strncpy(outlib, buf, ptr - buf);
	strcpy(outfunc, ptr + 1);
	pub_log_info("[%s][%d] LIB=[%s] FUNC=[%s]", __FILE__, __LINE__, outlib, outfunc);

	if (cycle->handle == NULL)
	{
		sprintf(libso, "%s/plugin/%s", getenv("SWWORK"), outlib);
		if (!pub_file_exist(libso))
		{
			sprintf(libso, "%s/plugin/%s", getenv("SWHOME"), outlib);
			if (!pub_file_exist(libso))
			{
				pub_log_error("[%s][%d] libso[%s] not found", __FILE__, __LINE__, libso);
				return SW_ERROR;
			}
		}

		cycle->handle = (void *)dlopen(libso, RTLD_LAZY|RTLD_GLOBAL);
		if (cycle->handle == NULL)
		{
			pub_log_error("[%s][%d]dlopen [%s] error=[%d][%s]"
					, __FILE__, __LINE__, libso, errno, dlerror());
			return SW_ERROR;
		}        
	}
	return SW_OK;
}

sw_int_t lsn_pub_read_cfg(sw_lsn_cycle_t * cycle, sw_xmltree_t *xml_tree, sw_xmlnode_t *current_node)
{
	sw_int_t	ret = 0;
	sw_char_t	filename[256];
	sw_char_t	filefunc[256];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;;

	pub_mem_memzero(filename, sizeof(filename));
	pub_mem_memzero(filefunc, sizeof(filefunc));

	node1 = NULL;
	xml_tree->current = current_node;

	if (current_node == NULL || cycle == NULL)
	{
		pub_log_error("[%s][%d] input param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml_tree, "PKGCFG");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Can not find PKGCFG!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(cycle->lsn_conf.pkgcfg, node->value, sizeof(cycle->lsn_conf.pkgcfg) - 1);

	node = pub_xml_locnode(xml_tree, "FILEDIR");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.filedir, node->value, sizeof(cycle->lsn_conf.filedir) - 1);
	}

	node = pub_xml_locnode(xml_tree, "FILEDEAL");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.filedeal, node->value, sizeof(cycle->lsn_conf.filedeal) - 1);
	}

	node = pub_xml_locnode(xml_tree, "MAPDEAL");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.mapdeal, node->value, sizeof(cycle->lsn_conf.mapdeal) - 1);
	}

	node = pub_xml_locnode(xml_tree, "MAPCFG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.mapcfg, node->value, sizeof(cycle->lsn_conf.mapcfg) - 1);
	}

	node = pub_xml_locnode(xml_tree, "FACTORANAYLY");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.factoranayly, node->value, sizeof(cycle->lsn_conf.factoranayly) - 1);
	}

	node = pub_xml_locnode(xml_tree, "SVRMAP");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.svrmap, node->value, sizeof(cycle->lsn_conf.svrmap) - 1);
	}

	node = pub_xml_locnode(xml_tree, "SVRMAPCFG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.svrmapcfg, node->value, sizeof(cycle->lsn_conf.svrmapcfg) - 1);
	}

	node = pub_xml_locnode(xml_tree, "DENYSERVCIE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.denyservice, node->value, sizeof(cycle->lsn_conf.denyservice) - 1);
	}

	node = pub_xml_locnode(xml_tree, "DENYCFG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.denycfg, node->value, sizeof(cycle->lsn_conf.denycfg) - 1);
	}

	node = pub_xml_locnode(xml_tree, "STARTFUNC");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.startfunc, node->value, sizeof(cycle->lsn_conf.startfunc) - 1);
	}

	node = pub_xml_locnode(xml_tree, "LOADLIB");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cycle->lsn_conf.loadlib, node->value, sizeof(cycle->lsn_conf.loadlib) - 1);
	}

	node1 =  pub_xml_locnode(xml_tree, "PROCESSER");
	if (node1 == NULL)
	{
		return SW_ERROR;
	}
	xml_tree->current = node1;

	cycle->handle = NULL;

	node1 = pub_xml_locnode(xml_tree, "EXTCEVT");
	if (node1 != NULL && node1->value != NULL)
	{
		strncpy(cycle->lsn_conf.extcevt, node1->value, sizeof(cycle->lsn_conf.extcevt));
	}

	node1 = pub_xml_locnode(xml_tree, "TRANSCODE");
	if (node1 == NULL || node1->value == NULL || pub_str_strlen(node1->value) == 0)
	{
		cycle->handler.des_handler = NULL;
	}
	else
	{
		strncpy(cycle->lsn_conf.transcode, node1->value, sizeof(cycle->lsn_conf.transcode));
		memset(filename, 0x0, sizeof(filename));
		memset(filefunc, 0x0, sizeof(filefunc));
		ret =  lsn_get_libfunc_handle(cycle, node1->value, filename, filefunc);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] get des fun error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		cycle->handler.des_handler = (sw_des_handle_pt)dlsym(cycle->handle, filefunc);
		if (cycle->handler.des_handler == NULL)

		{
			pub_log_error("[%s][%d]dlsym error=[%s]", __FILE__, __LINE__,  dlerror());
			return SW_ERROR;
		}
		cycle->handle = NULL;
	}

	node1 = pub_xml_locnode(xml_tree, "PKGDEAL");
	if (node1 != NULL && node1->value != NULL)
	{
		strncpy(cycle->lsn_conf.pkgdeal, node1->value, sizeof(cycle->lsn_conf.pkgdeal));
	}

	node1 = pub_xml_locnode(xml_tree, "KEEPALIVE");
	if (node1 != NULL && node1->value != NULL)
	{
		strncpy(cycle->lsn_conf.keepalive, node1->value, sizeof(cycle->lsn_conf.keepalive));
	}

	node1 = pub_xml_locnode(xml_tree, "HTTPSCFG");
	if (node1 != NULL && node1->value != NULL)
	{
		strncpy(cycle->lsn_conf.httpscfg, node1->value, sizeof(cycle->lsn_conf.httpscfg) - 1);
	}

	node1 = pub_xml_locnode(xml_tree, "INIT");
	if (node1 == NULL || node1->value == NULL || pub_str_strlen(node1->value) == 0)
	{
		cycle->handler.init_handler = NULL;
	}
	else
	{
		strncpy(cycle->lsn_conf.init, node1->value, sizeof(cycle->lsn_conf.init));

		pub_mem_memzero(filename, sizeof (filename));
		pub_mem_memzero(filefunc, sizeof (filefunc));


		ret =  lsn_get_libfunc_handle(cycle, node1->value, filename, filefunc);
		cycle->handler.init_handler = (sw_lsn_deal_handle_pt)dlsym(cycle->handle, filefunc);
		if (cycle->handler.init_handler == NULL)
		{
			pub_log_error("[%s][%d]dlsym error=[%s]node value=[%s]lib=[%s]func=[%s]", 
					__FILE__, __LINE__,dlerror(),node1->value,filename,filefunc);
			return SW_ERROR;
		}
	}

	node1 = pub_xml_locnode(xml_tree, "TIMEOUTEVT");
	if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
	{
		pub_log_info("[%s][%d],not find timeout func", __FILE__,
				__LINE__);
		cycle->handler.timeout_handler = NULL; 
	}
	else
	{
		strncpy(cycle->lsn_conf.timeoutevt, node1->value, sizeof(cycle->lsn_conf.timeoutevt));

		pub_mem_memzero(filename, sizeof (filename));
		pub_mem_memzero(filefunc, sizeof (filefunc));

		ret = lsn_get_libfunc_handle(cycle, node1->value, filename, filefunc);
		cycle->handler.timeout_handler = (sw_event_handler_pt)dlsym(cycle->handle, filefunc);
		if (cycle->handler.timeout_handler == NULL)

		{
			pub_log_error("[%s][%d]dlsym error=[%s]",
					__FILE__, __LINE__, dlerror());
			return SW_ERROR;
		}
	}
	node1 = pub_xml_locnode(xml_tree, "INTEREVT");
	if (node1 == NULL || node1->value == NULL || pub_str_strlen(node1->value) == 0)
	{
		pub_log_error("[%s][%d],not find INTEREVT",
				__FILE__, __LINE__);
		return SW_ERROR;
	}
	else
	{
		strncpy(cycle->lsn_conf.interevt, node1->value, sizeof(cycle->lsn_conf.interevt));

		pub_mem_memzero(filename, sizeof (filename));
		pub_mem_memzero(filefunc, sizeof (filefunc));

		ret =  lsn_get_libfunc_handle(cycle, node1->value, filename, filefunc);
		cycle->handler.deal_pkg_handler = (sw_event_handler_pt) dlsym(cycle->handle, filefunc);
		if (cycle->handler.deal_pkg_handler == NULL)

		{
			pub_log_error("[%s][%d]dlsym error=[%s]",
					__FILE__, __LINE__,  dlerror());
			return SW_ERROR;
		}
	}

	node1 = pub_xml_locnode(xml_tree, "DESTROY");
	if (node1 == NULL || node1->value == NULL || pub_str_strlen(node1->value) == 0)
	{
		pub_log_info("[%s][%d],not find destroy func", __FILE__,
				__LINE__);
		cycle->handler.destroy_handler = NULL;
	}
	else
	{
		pub_mem_memzero(filename, sizeof (filename));
		pub_mem_memzero(filefunc, sizeof (filefunc));
		strncpy(cycle ->lsn_conf.destroy, node1->value, sizeof(cycle->lsn_conf.destroy));
		ret = lsn_get_libfunc_handle(cycle, node1->value, filename, filefunc);
		cycle->handler.destroy_handler = (sw_lsn_deal_handle_pt)dlsym(cycle->handle, filefunc);
		if (cycle->handler.destroy_handler == NULL)
		{
			pub_log_error("[%s][%d]dlsym error=[%s]",
					__FILE__, __LINE__,  dlerror());
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] destroy_handler[%s]", __FILE__, __LINE__, filefunc);
	}

	return SW_OK;
}

sw_int_t lsn_prdt_cfg_upd(sw_lsn_cycle_t *cycle, char *name, int index)
{
	int	ret = 0;
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_route_t      *route = NULL;
	sw_lsn_prdt_cfg_t	*cfg = NULL;

	if (cycle == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/lsncfg/lsncfg.xml", cycle->base.work_dir.data, name);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		return SW_OK;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.SWLISTEN");
	while (node != NULL)
	{
		if (strcmp(node->name, "SWLISTEN") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		if (strcmp(node1->value, cycle->lsn_conf.name) == 0)
		{
			break;
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_info("[%s][%d] Not find [%s][%s] lsn config!", 
				__FILE__, __LINE__, name, cycle->lsn_conf.name);
		pub_xml_deltree(xml);
		return SW_OK;
	}
	if (index < 0)
	{
		cfg = &cycle->routes->route[cycle->routes->head.route_cnt].prdt_cfg;
		route = &cycle->routes->route[cycle->routes->head.route_cnt];
	}
	else
	{
		cfg = &cycle->routes->route[index].prdt_cfg;
		route = &cycle->routes->route[index];
	}
	memset(cfg, 0x0, sizeof(sw_lsn_prdt_cfg_t));
	node = pub_xml_locnode(xml, "PREANALYZE.CODECONVERT");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->codeconvert, node->value, sizeof(cfg->codeconvert) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.TYPEANALYZE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->typeanalyze, node->value, sizeof(cfg->typeanalyze) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.PKGPREANALYZE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->preanalyze, node->value, sizeof(cfg->preanalyze) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.GENSYNRES");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->synres, node->value, sizeof(cfg->synres) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.RECOVERSET");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->recover, node->value, sizeof(cfg->recover) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.AFTERDEAL");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->afterdeal, node->value, sizeof(cfg->afterdeal) - 1);
	}

	pub_xml_deltree(xml);

	pub_log_info("[%s][%d] updata lsncfg plugin handle begin...", __FILE__, __LINE__);
	if (route->prdt_cfg.codeconvert[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.codeconvert, route->name,
				&route->handle.code_handle, (void *)&route->fun.code_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.codeconvert);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.typeanalyze[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.typeanalyze, route->name,
				&route->handle.type_handle, (void *)&route->fun.type_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.typeanalyze);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.preanalyze[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.preanalyze, route->name,
				&route->handle.prean_handle, (void *)&route->fun.prean_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.preanalyze);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.synres[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.synres, route->name,
				&route->handle.synres_handle, (void *)&route->fun.synres_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.synres);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.recover[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.recover, route->name,
				&route->handle.recov_handle, (void *)&route->fun.recov_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.recover);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.afterdeal[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.afterdeal, route->name,
				&route->handle.after_handle, (void *)&route->fun.after_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.recover);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] updata lsncfg plugin handle success", __FILE__, __LINE__);
	return SW_OK;
}

sw_int_t lsn_route_cfg_upd(sw_lsn_cycle_t *cycle, char *name, int index) 
{
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;	
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	sw_route_t	*route = NULL;
	sw_check_t	*check = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));
	if (cycle == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(xmlname, "%s/products/%s/etc/lsncfg/route.xml", cycle->base.work_dir.data, name);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] Not find xml [%s]!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".PRODUCTS.PRODUCT");
	while (node != NULL)
	{
		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "LSNNAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config LSNNAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (strcmp(cycle->lsn_conf.name, node1->value) == 0)
		{
			break;
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_info("[%s][%d] Can not find lsn [%s] route config!", 
				__FILE__, __LINE__, cycle->lsn_conf.name);
		pub_xml_deltree(xml);
		return SW_OK;
	}
	nodebak = xml->current;
	if (index < 0)
	{
		route = &cycle->routes->route[cycle->routes->head.route_cnt];
	}
	else
	{
		route = &cycle->routes->route[index];
	}
	route->an.check_cnt = 0;
	route->in.check_cnt = 0;
	strncpy(route->name, name, sizeof(route->name) - 1);
	node = pub_xml_locnode(xml, "GATE");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config GATE!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	if (strcasecmp(node->value, "on") == 0)
	{
		route->gate = 1;
	}
	else
	{
		route->gate = 0;
	}
	node = pub_xml_locnode(xml, "ANALYZE.CHECK");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}

		check = &route->an.check[route->an.check_cnt];
		xml->current = node;
		node1 = pub_xml_locnode(xml, "BEGIN");
		if (node1 != NULL && node1->value != NULL)
		{
			check->begin = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			check->length = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->value, node1->value, sizeof(check->value) - 1);
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->name, node1->value, sizeof(check->name) - 1);
		}

		node1 = pub_xml_locnode(xml, "LIB");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->lib, node1->value, sizeof(check->lib) - 1);
		}

		node1 = pub_xml_locnode(xml, "FUNCTION");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->func, node1->value, sizeof(check->func) - 1);
		}
		route->an.check_cnt++;
		node = node->next;
	}	
	pub_log_info("[%s][%d] an.check_cnt=[%d]", __FILE__, __LINE__, route->an.check_cnt);

	xml->current = nodebak;
	node = pub_xml_locnode(xml, "INTEGRATE.CHECK");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}

		check = &route->in.check[route->in.check_cnt];
		xml->current = node;
		node1 = pub_xml_locnode(xml, "BEGIN");
		if (node1 != NULL && node1->value != NULL)
		{
			check->begin = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			check->length = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->value, node1->value, sizeof(check->value) - 1);
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->name, node1->value, sizeof(check->name) - 1);
		}

		node1 = pub_xml_locnode(xml, "LIB");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->lib, node1->value, sizeof(check->lib) - 1);
		}

		node1 = pub_xml_locnode(xml, "FUNCTION");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->func, node1->value, sizeof(check->func) - 1);
		}
		route->in.check_cnt++;
		node = node->next;
	}	
	pub_log_info("[%s][%d] an.check_cnt=[%d]", __FILE__, __LINE__, route->an.check_cnt);

	pub_xml_deltree(xml);
	pub_log_info("[%s][%d] lsn_route_cfg_init [%s] success!", __FILE__, __LINE__, name);

	return SW_OK;
}

sw_int_t lsn_prdt_stat_init(sw_lsn_cycle_t *cycle, char *name)
{
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));
	if (cycle == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/products.xml", cycle->base.work_dir.data);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.PRODUCT");
	while (node != NULL)
	{
		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		if (strcmp(node1->value, name) == 0)
		{
			break;
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not find [%s]'s status config!", __FILE__, __LINE__, name);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, "STATUS");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config STATUS!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	cycle->routes->route[cycle->routes->head.route_cnt].status = atoi(node->value);
	pub_log_info("[%s][%d] route[%d].stat=[%d]", __FILE__, __LINE__, 
			cycle->routes->head.route_cnt, cycle->routes->route[cycle->routes->head.route_cnt].status);

	pub_xml_deltree(xml);
	pub_log_info("[%s][%d] init prdt [%s] status success!", __FILE__, __LINE__, name);

	return SW_OK;
}

sw_int_t lsn_route_cfg_init(sw_lsn_cycle_t *cycle, char *name, int index) 
{
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;	
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*nodebak = NULL;
	sw_route_t	*route = NULL;
	sw_check_t	*check = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));
	if (cycle == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(xmlname, "%s/products/%s/etc/lsncfg/route.xml", cycle->base.work_dir.data, name);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] Not find xml [%s]!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".PRODUCTS.PRODUCT");
	while (node != NULL)
	{
		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "LSNNAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config LSNNAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (strcmp(cycle->lsn_conf.name, node1->value) == 0)
		{
			break;
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_info("[%s][%d] Can not find lsn [%s] route config!", 
				__FILE__, __LINE__, cycle->lsn_conf.name);
		pub_xml_deltree(xml);
		return SW_OK;
	}
	nodebak = xml->current;
	if (index < 0)
	{
		route = &cycle->routes->route[cycle->routes->head.route_cnt];
	}
	else
	{
		route = &cycle->routes->route[index];
	}

	route->an.check_cnt = 0;
	route->in.check_cnt = 0;
	strncpy(route->name, name, sizeof(route->name) - 1);
	node = pub_xml_locnode(xml, "GATE");
	if (node == NULL || node->value == NULL)
	{
		pub_log_error("[%s][%d] Not config GATE!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	if (strcasecmp(node->value, "on") == 0)
	{
		route->gate = 1;
	}
	else
	{
		route->gate = 0;
	}
	node = pub_xml_locnode(xml, "ANALYZE.CHECK");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}

		check = &route->an.check[route->an.check_cnt];
		xml->current = node;
		node1 = pub_xml_locnode(xml, "BEGIN");
		if (node1 != NULL && node1->value != NULL)
		{
			check->begin = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			check->length = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->value, node1->value, sizeof(check->value) - 1);
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->name, node1->value, sizeof(check->name) - 1);
		}

		node1 = pub_xml_locnode(xml, "LIB");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->lib, node1->value, sizeof(check->lib) - 1);
		}

		node1 = pub_xml_locnode(xml, "FUNCTION");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->func, node1->value, sizeof(check->func) - 1);
		}
		route->an.check_cnt++;
		node = node->next;
	}	
	pub_log_info("[%s][%d] an.check_cnt=[%d]", __FILE__, __LINE__, route->an.check_cnt);

	xml->current = nodebak;
	node = pub_xml_locnode(xml, "INTEGRATE.CHECK");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHECK") != 0)
		{
			node = node->next;
			continue;
		}

		check = &route->in.check[route->in.check_cnt];
		xml->current = node;
		node1 = pub_xml_locnode(xml, "BEGIN");
		if (node1 != NULL && node1->value != NULL)
		{
			check->begin = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "LENGTH");
		if (node1 != NULL && node1->value != NULL)
		{
			check->length = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->value, node1->value, sizeof(check->value) - 1);
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->name, node1->value, sizeof(check->name) - 1);
		}

		node1 = pub_xml_locnode(xml, "LIB");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->lib, node1->value, sizeof(check->lib) - 1);
		}

		node1 = pub_xml_locnode(xml, "FUNCTION");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(check->func, node1->value, sizeof(check->func) - 1);
		}
		route->in.check_cnt++;
		node = node->next;
	}	
	pub_log_info("[%s][%d] an.check_cnt=[%d]", __FILE__, __LINE__, route->an.check_cnt);

	pub_xml_deltree(xml);
	pub_log_info("[%s][%d] lsn_route_cfg_init [%s] success!", __FILE__, __LINE__, name);

	return SW_OK;
}

sw_int_t lsn_prdt_cfg_init(sw_lsn_cycle_t *cycle, char *name, int index)
{
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_lsn_prdt_cfg_t	*cfg = NULL;

	if (cycle == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/lsncfg/lsncfg.xml", cycle->base.work_dir.data, name);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		return SW_OK;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.SWLISTEN");
	while (node != NULL)
	{
		if (strcmp(node->name, "SWLISTEN") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		if (strcmp(node1->value, cycle->lsn_conf.name) == 0)
		{
			break;
		}
		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_info("[%s][%d] Not find [%s][%s] lsn config!", 
				__FILE__, __LINE__, name, cycle->lsn_conf.name);
		pub_xml_deltree(xml);
		return SW_OK;
	}

	if (index < 0)
	{
		cfg = &cycle->routes->route[cycle->routes->head.route_cnt].prdt_cfg;
	}
	else
	{
		cfg = &cycle->routes->route[index].prdt_cfg;
	}

	memset(cfg, 0x0, sizeof(sw_lsn_prdt_cfg_t));
	node = pub_xml_locnode(xml, "PREANALYZE.CODECONVERT");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->codeconvert, node->value, sizeof(cfg->codeconvert) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.TYPEANALYZE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->typeanalyze, node->value, sizeof(cfg->typeanalyze) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.PKGPREANALYZE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->preanalyze, node->value, sizeof(cfg->preanalyze) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.GENSYNRES");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->synres, node->value, sizeof(cfg->synres) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.RECOVERSET");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->recover, node->value, sizeof(cfg->recover) - 1);
	}

	node = pub_xml_locnode(xml, "PREANALYZE.AFTERDEAL");
	if (node != NULL && node->value != NULL)
	{
		strncpy(cfg->afterdeal, node->value, sizeof(cfg->afterdeal) - 1);
	}

	pub_xml_deltree(xml);

	return SW_OK;
}


sw_int_t lsn_route_init(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));

	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < MAX_ROUTE_NUM; i++)
	{
		cycle->routes->route[i].an.check_cnt = 0;
		cycle->routes->route[i].in.check_cnt = 0;
	}
	sprintf(xmlname, "%s/cfg/channels.xml", cycle->base.work_dir.data);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] not find!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.CHANNEL");
	while (node != NULL)
	{
		if (strcmp(node->name, "CHANNEL") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "LISTEN");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config [LISTEN]!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		if (strcmp(cycle->lsn_conf.name, node1->value) == 0)
		{
			break;
		}

		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not find [%s] config!", __FILE__, __LINE__, cycle->lsn_conf.name);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, "PRODUCT.NAME");
	while (node != NULL)
	{
		if (strcmp(node->name, "NAME") != 0)
		{
			node = node->next;
			continue;
		}
		pub_log_info("[%s][%d] prdt=[%s]", __FILE__, __LINE__, node->value);

		if (cycle->routes->head.route_cnt >= MAX_ROUTE_NUM)
		{
			pub_log_error("[%s][%d] No enough space to save route info! route_cnt=[%d] MAX_ROUTE_NUM=[%d]",
					__FILE__, __LINE__, cycle->routes->head.route_cnt, MAX_ROUTE_NUM);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		ret = lsn_prdt_stat_init(cycle, node->value);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_prdt_stat_init error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (cycle->routes->route[cycle->routes->head.route_cnt].status != 1)
		{
			node = node->next;
			continue;
		}

		ret = lsn_route_cfg_init(cycle, node->value, -1);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] init [%s] route config error!", __FILE__, __LINE__, node->value);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		ret = lsn_prdt_cfg_init(cycle, node->value, -1);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] init [%s] cfg config error!", __FILE__, __LINE__, node->value);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		cycle->routes->head.route_cnt++;
		node = node->next;
	}
	pub_log_info("[%s][%d] lsn_route_init success!", __FILE__, __LINE__);
	pub_xml_deltree(xml);

	return 0;
}

sw_int_t lsn_deal_cmd(sw_lsn_cycle_t *cycle)
{
	int     ret = 0;
	sw_cmd_t        cmd;
	sw_char_t	udp_name[64];

	memset(&cmd, 0x0, sizeof(cmd));
	ret = udp_recv(cycle->udp_fd, (char *)&cmd, sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] FD=[%d] recv failed,errno=[%d]:[%s]ret=[%d]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno), ret);
		return SW_ERROR;
	}

	memset(udp_name, 0x0, sizeof(udp_name));
	strncpy(udp_name, cmd.udp_name, sizeof(udp_name)-1);
	pub_log_info("[%s][%d] udp_name=[%s]", __FILE__, __LINE__, udp_name);

	pub_log_info("[%s][%d] type=[%d]", __FILE__, __LINE__, cmd.type);
	switch (cmd.type)
	{
		case SW_MSTOPSELF:
		case SW_MSTOPISELF:
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char*)&cmd, sizeof(cmd), udp_name);
			pub_log_info("[%s][%d] stop myself!", __FILE__, __LINE__);
			if (cycle->handler.destroy_handler != NULL)
			{
				cycle->handler.destroy_handler(cycle);
			}
			lsn_cycle_child_destroy(cycle);
			exit(0);
			break;
		case SW_UPD_LSN:
			ret = lsn_upd_cfg(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Update lsn cfg error!", __FILE__, __LINE__);
			}
			pub_log_info("[%s][%d] Update lsn!", __FILE__, __LINE__);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char*)&cmd, sizeof(cmd), udp_name);
			break;
		case SW_UPD_PRDT:
			ret = lsn_upd_prdt(cycle, cmd.dst_prdt);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Update prdt error!", __FILE__, __LINE__);
			}
			pub_log_info("[%s][%d] Update lsn prdt[%s]!", __FILE__, __LINE__, cmd.dst_prdt);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char*)&cmd, sizeof(cmd), udp_name);
			break;
		case SW_ADD_PRDT:
			ret = lsn_add_prdt(cycle, cmd.dst_prdt);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Add prdt [%s] error!",
						__FILE__, __LINE__, cmd.dst_prdt);
			}
			ret = get_all_lsncfg_handle(cycle, cmd.dst_prdt);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Add prdt [%s] error!",
						__FILE__, __LINE__, cmd.dst_prdt);
			}       
			ret = lsn_prdt_svrmap_add(cycle, cmd.dst_prdt);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Add prdt [%s] error!",
						__FILE__, __LINE__, cmd.dst_prdt);
			}
			pub_log_info("[%s][%d] add  prdt[%s]!", __FILE__, __LINE__, cmd.dst_prdt);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char*)&cmd, sizeof(cmd), udp_name);
			break;
		default:
			cmd.type = SW_ERRCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name , "%s" , cycle->base.name.data);
			udp_send(cycle->udp_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), udp_name);
			break;
	} 

	return SW_OK;                                                                                                                      
}

sw_int_t lsn_child_work_ext(sw_lsn_cycle_t *cycle, sw_int32_t index)
{
	int	i = 0;
	sw_int_t	ret = 0;
	sw_fd_list_t    fd_list;
	sw_char_t	lsn_name[64];
	sw_char_t	log_name[64];
	sw_char_t	log_path[128];

	memset(lsn_name, 0x0, sizeof(lsn_name));
	memset(log_name, 0x0, sizeof(log_name));
	memset(log_path, 0x0, sizeof(log_path));

	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d]child work argument error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(lsn_name, "%s_%d_rcv", cycle->lsn_conf.name, index);
	if (strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0)
	{
		if (cycle->lsn_conf.comm.count > 1)
		{
			memset(lsn_name, 0x0, sizeof(lsn_name));
			sprintf(lsn_name, "%s_%d_%d_rcv", cycle->lsn_conf.name, cycle->lsn_conf.group_index, index);
		}
	}
	sprintf(log_name, "%s.log", lsn_name);
	cycle->lsn_conf.process_index = index;

	pub_mem_memzero(log_path, sizeof(log_path));
	sprintf(log_path, "%s/%s", cycle->base.log->log_path.data, log_name);
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_log_chglog debug path error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(g_eswid, 0x0, sizeof(g_eswid));
	if (pub_cfg_get_eswid(g_eswid) < 0)
	{
		pub_log_error("[%s][%d] Get sysid error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] sysid===[%s]", __FILE__, __LINE__, g_eswid);

	cycle->udp_fd = udp_bind(lsn_name);
	if (cycle->udp_fd <= 0)
	{
		pub_log_error("[%s][%d] udp_bind error! fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno));
		return SW_ERROR;
	}

	ret = lsn_child_register(cycle, SW_S_START, lsn_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Register error! lsn_name=[%s]", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}

	cycle->link_list = (sw_link_list_t *)cycle->link_shm.addr;
	if (strcmp(cycle->lsn_conf.comtype, "MQ") != 0 &&
			strcmp(cycle->lsn_conf.comtype, "TLQ") != 0 &&
			strcmp(cycle->lsn_conf.comtype, "TCPLA") != 0)
	{
		cycle->link_info = (sw_link_info_t *)(cycle->sock_shm.addr + sizeof(sw_link_info_t) * cycle->lsn_conf.conmax * index); 
		for (i = 0; i < cycle->lsn_conf.conmax; i++)
		{
			memset(&cycle->link_info[i], 0x0, sizeof(sw_link_info_t));
			cycle->link_info[i].sockid = -1;
			cycle->link_info[i].use = 0;
			pub_mem_memzero(&cycle->link_info[i].route, sizeof(sw_route_t));
		}
	}

	pub_mem_memzero(&fd_list, sizeof (fd_list));
	fd_list.fd = cycle->udp_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)lsn_deal_cmd;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Child: udp_name=[%s] udp_fd=[%d]", __FILE__, __LINE__, lsn_name, cycle->udp_fd);
	pub_log_end("[%s][%d] worker begin...", __FILE__, __LINE__);

	memset(&fd_list, 0x0, sizeof(fd_list));
	fd_list.fd = 999;
	fd_list.data = (void *)cycle;
	ret = cycle->handler.deal_pkg_handler(&fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] deal_pkg_handler error!", __FILE__, __LINE__);
		return SW_OK;
	}
	return SW_OK;
}

sw_int_t lsn_child_work(sw_lsn_cycle_t *cycle, sw_int32_t index)
{
	int	msg_in = 0;
	int	msg_out = 0;
	int	timer = 0;
	sw_int32_t	i = 0;
	sw_int_t	ret = 0;
	sw_fd_list_t	*fd_work;
	sw_int_t	fd_out = 0;
	sw_int_t	recv_cnt = 0;
	sw_fd_list_t    fd_list;
	sw_char_t	lsn_name[64];
	sw_char_t	log_name[64];
	sw_char_t	log_path[128];

	memset(lsn_name, 0x0, sizeof(lsn_name));
	memset(log_name, 0x0, sizeof(log_name));
	memset(log_path, 0x0, sizeof(log_path));

	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d]child work argument error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(lsn_name, "%s_%d", cycle->lsn_conf.name, index);

	/*** MQ OR TLQ ***/
	if (strcmp(cycle->lsn_conf.comtype, "TCPLC") == 0 )
	{    
		memset(lsn_name, 0x0, sizeof(lsn_name));
		sprintf(lsn_name, "%s_lsn", cycle->lsn_conf.name);
		sprintf(log_name, "%s.log", lsn_name);
	}    
	else if (strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0)
	{
		memset(lsn_name, 0x0, sizeof(lsn_name));
		if (cycle->lsn_conf.comm.count > 1)
		{
			sprintf(lsn_name, "%s_%d_%d_snd", cycle->lsn_conf.name, cycle->lsn_conf.group_index, index);
		}
		else
		{
			sprintf(lsn_name, "%s_%d_snd", cycle->lsn_conf.name, index);
		}
		sprintf(log_name, "%s.log", lsn_name);
	}
	else if (strcmp(cycle->lsn_conf.comtype, "TCPLA") == 0 || pub_str_strrncmp(cycle->lsn_conf.comtype, "LC", 2) == 0)
	{
		memset(lsn_name, 0x0, sizeof(lsn_name));
		sprintf(lsn_name, "%s_%d_snd", cycle->lsn_conf.name, index);
		sprintf(log_name, "%s.log", lsn_name);
	}
	else
	{
		sprintf(log_name, "%s_lsn.log", lsn_name);
	}
	cycle->lsn_conf.process_index = index;

	pub_mem_memzero(log_path, sizeof(log_path));
	sprintf(log_path, "%s/%s", cycle->base.log->log_path.data, log_name);
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_log_chglog debug path error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(g_eswid, 0x0, sizeof(g_eswid));
	if (pub_cfg_get_eswid(g_eswid) < 0)
	{
		pub_log_error("[%s][%d] Get sysid error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] sysid===[%s]", __FILE__, __LINE__, g_eswid);

	cycle->udp_fd = udp_bind(lsn_name);
	if (cycle->udp_fd <= 0)
	{
		pub_log_error("[%s][%d] udp_bind error! fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno));
		return SW_ERROR;
	}

	fd_out = msg_trans_create(cycle->base.global_path, IPC_PRIVATE, 0, &msg_out);
	if (fd_out <= 0)
	{
		pub_log_error("[%s][%d] msg_trans_create error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	if (pub_str_strrncmp(cycle->lsn_conf.comtype, "LC", 2) == 0)
	{
		cycle->lsn_conf.in_fd = msg_trans_create(cycle->base.global_path, IPC_PRIVATE, 0, &msg_in);
		if (fd_out <= 0)
		{
			pub_log_error("[%s][%d] msg_trans_create error! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		cycle->lsn_conf.in_msgid = msg_in;
	}

	cycle->lsn_conf.out_fd = fd_out;
	cycle->lsn_conf.out_msgid = msg_out;
	pub_log_info("[%s][%d] lsn_name=[%s] msgid=[%d]", __FILE__, __LINE__, lsn_name, cycle->lsn_conf.out_msgid);

	/*** 对于TCPLA,如果连接远端服务失败,直接将进程状态置为SW_S_READY ***/
	if (strcmp(cycle->lsn_conf.comtype, "TCPLA") == 0 && cycle->lsn_conf.comm.group[cycle->lsn_conf.group_index].remote.sockid < 0)
	{
		ret = lsn_child_register(cycle, SW_S_READY, lsn_name);
	}
	else
	{
		ret = lsn_child_register(cycle, SW_S_START, lsn_name);
	}
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Register error! lsn_name=[%s]", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}

	cycle->link_list = (sw_link_list_t *)cycle->link_shm.addr;
	cycle->link_list->head.cur_pos = 0;
	cycle->link_list->head.link_cnt = cycle->list_size;
	for (i = 0; i < cycle->list_size; i++)
	{
		cycle->link_list->list[i].use = 0;
	}

	if (strcmp(cycle->lsn_conf.comtype, "MQ") != 0 &&
			strcmp(cycle->lsn_conf.comtype, "TLQ") != 0 &&
			strcmp(cycle->lsn_conf.comtype, "TCPLA") != 0)
	{
		cycle->link_info = (sw_link_info_t *)(cycle->sock_shm.addr + sizeof(sw_link_info_t) * cycle->lsn_conf.conmax * index); 
		for (i = 0; i < cycle->lsn_conf.conmax; i++)
		{
			memset(&cycle->link_info[i], 0x0, sizeof(sw_link_info_t));
			cycle->link_info[i].sockid = -1;
			cycle->link_info[i].use = 0;
			pub_mem_memzero(&cycle->link_info[i].route, sizeof(sw_route_t));
		}
	}

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->udp_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)lsn_deal_cmd;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Child: udp_name=[%s] udp_fd=[%d]", __FILE__, __LINE__, lsn_name, cycle->udp_fd);

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->lsn_conf.out_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = cycle->handler.deal_pkg_handler;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] fd_out=[%d]", __FILE__, __LINE__, cycle->lsn_conf.out_fd);
	pub_log_info("[%s][%d] child work info: conmax=[%d] cycle=[%x] link_info=[%x] fd=[%d]",
			__FILE__, __LINE__, cycle->lsn_conf.conmax, cycle, cycle->link_info, cycle->lsn_fd);

	pub_log_end("[%s][%d] worker begin...", __FILE__, __LINE__);
	while (1)
	{
		if (cycle->handler.timeout_handler != NULL)
		{
			cycle->handler.timeout_handler(cycle);
		}

		timer = cycle->lsn_conf.scantime > 0 ? cycle->lsn_conf.scantime : 1000;
		recv_cnt = select_process_events(cycle->lsn_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error! errno=[%d]:[%s]", 
					__FILE__, __LINE__, errno, strerror(errno));
			continue;
		}
		else if (recv_cnt == 0)
		{	
			if (getppid() == 1)
			{
				pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
				lsn_cycle_child_destroy(cycle);
				exit(0);
			}

			if (procs_update_process_busystatus(NULL, cycle->base.name.data, lsn_name, PROC_S_IDLE) != SW_OK)
			{
				pub_log_error("[%s][%d] update [%s] busystats error!",
						__FILE__, __LINE__, lsn_name);
			}
			continue;
		}

		for (i = 0; i < recv_cnt; i++)
		{
			if (fd_work[i].fd == cycle->udp_fd)
			{
				ret = fd_work[i].event_handler(fd_work[i].data);
			}
			else
			{
				ret = fd_work[i].event_handler(&fd_work[i]);
			}

			if (ret != SW_DELETE)
			{
				pub_log_bend("\0");
			}

			if (ret == SW_DELETE)
			{
				pub_log_end("\0");
				select_del_event(cycle->lsn_fds, fd_work[i].fd);
				continue;
			}
		}
	}

	return SW_OK;
}

sw_int_t lsn_child_work_normal(sw_lsn_cycle_t *cycle, sw_int32_t index)
{
	int	msg_out = 0;
	sw_int32_t	i = 0;
	sw_int_t	ret = 0;
	sw_int_t	fd_out = 0;
	sw_fd_list_t    fd_list;
	sw_char_t	lsn_name[64];
	sw_char_t	log_name[64];
	sw_char_t	log_path[128];

	memset(lsn_name, 0x0, sizeof(lsn_name));
	memset(log_name, 0x0, sizeof(log_name));
	memset(log_path, 0x0, sizeof(log_path));

	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d]child work argument error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(lsn_name, "%s_%d", cycle->lsn_conf.name, index);
	sprintf(log_name, "%s_lsn.log", lsn_name);
	cycle->lsn_conf.process_index = index;

	pub_mem_memzero(log_path, sizeof(log_path));
	sprintf(log_path, "%s/%s", cycle->base.log->log_path.data, log_name);
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_log_chglog debug path error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	memset(g_eswid, 0x0, sizeof(g_eswid));
	if (pub_cfg_get_eswid(g_eswid) < 0)
	{
		pub_log_error("[%s][%d] Get sysid error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] sysid===[%s]", __FILE__, __LINE__, g_eswid);

	cycle->udp_fd = udp_bind(lsn_name);
	if (cycle->udp_fd <= 0)
	{
		pub_log_error("[%s][%d] udp_bind error! fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno));
		return SW_ERROR;
	}

	fd_out = msg_trans_create(cycle->base.global_path, IPC_PRIVATE, 0, &msg_out);
	if (fd_out <= 0)
	{
		pub_log_error("[%s][%d] msg_trans_create error! errno=[%d]:[%s]", 
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	cycle->lsn_conf.out_fd = fd_out;
	cycle->lsn_conf.out_msgid = msg_out;
	pub_log_info("[%s][%d] lsn_name=[%s] msgid=[%d]", __FILE__, __LINE__, lsn_name, cycle->lsn_conf.out_msgid);

	ret = lsn_child_register(cycle, SW_S_START, lsn_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Register error! lsn_name=[%s]", __FILE__, __LINE__, lsn_name);
		return SW_ERROR;
	}

	cycle->link_list = (sw_link_list_t *)cycle->link_shm.addr;
	cycle->link_list->head.cur_pos = 0;
	cycle->link_list->head.link_cnt = cycle->list_size;
	for (i = 0; i < cycle->list_size; i++)
	{
		cycle->link_list->list[i].use = 0;
	}

	cycle->link_info = (sw_link_info_t *)(cycle->sock_shm.addr + sizeof(sw_link_info_t) * cycle->lsn_conf.conmax * index); 
	for (i = 0; i < cycle->lsn_conf.conmax; i++)
	{
		memset(&cycle->link_info[i], 0x0, sizeof(sw_link_info_t));
		cycle->link_info[i].sockid = -1;
		cycle->link_info[i].use = 0;
		pub_mem_memzero(&cycle->link_info[i].route, sizeof(sw_route_t));
	}

	pub_mem_memzero(&fd_list, sizeof(fd_list));
	fd_list.fd = cycle->udp_fd;
	fd_list.data = (void *)cycle;
	fd_list.event_handler = (sw_event_handler_pt)lsn_deal_cmd;
	ret = select_add_event(cycle->lsn_fds, &fd_list);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error! fd=[%d]", __FILE__, __LINE__, fd_list.fd);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Child: udp_name=[%s] udp_fd=[%d]", __FILE__, __LINE__, lsn_name, cycle->udp_fd);
	pub_log_end("[%s][%d] worker begin...", __FILE__, __LINE__);

	memset(&fd_list, 0x0, sizeof(fd_list));                                                                               
	fd_list.fd = 999;                                                                                                     
	fd_list.data = (void *)cycle;                                                                                         
	ret = cycle->handler.deal_pkg_handler(&fd_list);                                                                      
	if (ret != SW_OK)                                                                                                     
	{                                                                                                                     
		pub_log_error("[%s][%d] deal_pkg_handler error!", __FILE__, __LINE__);                                        
		return SW_OK;                                                                                                 
	}            

	return SW_OK;
}

sw_int_t lsn_create_single_child_common(sw_lsn_cycle_t *cycle, sw_int_t index)
{
	pid_t	pid;
	sw_int_t	ret = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		ret = lsn_child_work(cycle, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_child_work error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_single_child_normal(sw_lsn_cycle_t *cycle, sw_int_t index)
{
	pid_t	pid;
	sw_int_t	ret = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		ret = lsn_child_work_normal(cycle, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_child_work error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_single_child_mq(sw_lsn_cycle_t *cycle, int group_index, int proc_index, int sendtype)
{
	pid_t	pid;
	int i = 0;
	sw_int_t	ret = 0;

	memset(&g_mqcfg, 0x0, sizeof(g_mqcfg));
	memset(&g_mqcfg_ext, 0x00, sizeof(g_mqcfg_ext));
	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		cycle->lsn_conf.sendtype = sendtype;
		cycle->lsn_conf.group_index = group_index;
		if (sendtype == O_SEND)
		{
			memcpy(&g_mqcfg, &cycle->lsn_conf.comm.group[group_index].send, sizeof(g_mqcfg));
			memcpy(&g_mqcfg_ext, &cycle->lsn_conf.comm.group[group_index].recv, sizeof(g_mqcfg_ext));
		}
		else
		{
			memcpy(&g_mqcfg_ext, &cycle->lsn_conf.comm.group[group_index].send, sizeof(g_mqcfg_ext));
			memcpy(&g_mqcfg, &cycle->lsn_conf.comm.group[group_index].recv, sizeof(g_mqcfg));
		}
		if (cycle->handler.init_handler != NULL && 
				(strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "MQLA") == 0))
		{
			ret = cycle->handler.init_handler(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("%s, %d, init error", __FILE__, __LINE__);
				exit(1);
			}

			for (i = 0; i < MAX_MACHINE_NUM; i++)
			{
				g_mp_addr[i].use = 0;
				g_mp_addr[i].port = 0;
				g_mp_addr[i].sockid = -1;
				memset(g_mp_addr[i].ip, 0x0, sizeof(g_mp_addr[i].ip));
			}
		}

		if (sendtype == O_RECV)
		{
			ret = lsn_child_work_ext(cycle, proc_index);
		}
		else
		{
			ret = lsn_child_work(cycle, proc_index);
		}

		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] lsn_child_work_ext error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child_mq(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	int	index = 0;
	int	ret = 0;

	pub_log_debug("[%s][%d] group cnt:[%d]", __FILE__, __LINE__, cycle->lsn_conf.comm.count);
	for (i = 0; i < cycle->lsn_conf.comm.count; i++)
	{
		pub_log_info("[%s][%d] send qmgr=[%s]", __FILE__, __LINE__, cycle->lsn_conf.comm.group[i].send.qmgr);
		if (strlen(cycle->lsn_conf.comm.group[i].send.qmgr) > 0)
		{
			pub_log_info("[%s][%d] send proc_cnt=[%d]", 
					__FILE__, __LINE__, cycle->lsn_conf.comm.group[i].send.proc_cnt);
			for (j = 0; j < cycle->lsn_conf.comm.group[i].send.proc_cnt; j++)
			{
				ret = lsn_create_single_child_mq(cycle, i, j, O_SEND);
				if (ret == SW_ERROR)
				{
					pub_log_error("[%s][%d] create child error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				index++;
			}
		}

		pub_log_info("[%s][%d] recv qmgr=[%s]", __FILE__, __LINE__, cycle->lsn_conf.comm.group[i].recv.qmgr);
		if (strlen(cycle->lsn_conf.comm.group[i].recv.qmgr) > 0)
		{
			pub_log_info("[%s][%d] recv proc_cnt=[%d]", 
					__FILE__, __LINE__, cycle->lsn_conf.comm.group[i].recv.proc_cnt);
			for (j = 0; j < cycle->lsn_conf.comm.group[i].recv.proc_cnt; j++)
			{
				ret = lsn_create_single_child_mq(cycle, i, j, O_RECV);
				if (ret == SW_ERROR)
				{
					pub_log_error("[%s][%d] create child error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				index++;
			}
		}
	}
	pub_log_info("[%s][%d] Create child success! proc_cnt:[%d]", __FILE__, __LINE__, index);

	return 0;
}

sw_int_t lsn_create_single_child_la(sw_lsn_cycle_t *cycle, int group_index, int sendtype)
{
	pid_t	pid;
	sw_int_t	ret = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		cycle->lsn_conf.sendtype = sendtype;
		cycle->lsn_conf.group_index = group_index;
		if (sendtype == O_RECV)
		{
			ret = lsn_child_work_ext(cycle, group_index);
		}
		else
		{
			ret = lsn_child_work(cycle, group_index);
		}

		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] lsn_child_work_ext error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child_la(sw_lsn_cycle_t *cycle)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;

	for (i = 0; i < cycle->lsn_conf.comm.count; i++)
	{
		pub_log_info("[%s][%d] id=[%d]", __FILE__, __LINE__, i);
		if (cycle->lsn_conf.comm.group[i].local.use == 1)
		{
			ret = lsn_create_single_child_la(cycle, i, O_RECV);
			if (ret == SW_ERROR)
			{
				pub_log_error("[%s][%d] create child error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}

		if (cycle->lsn_conf.comm.group[i].remote.use == 1)
		{
			ret = lsn_create_single_child_la(cycle, i, O_SEND);
			if (ret == SW_ERROR)
			{
				pub_log_error("[%s][%d] create child error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_single_child_lc(sw_lsn_cycle_t *cycle, int index)
{
	pid_t	pid;
	sw_int_t	ret = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		cycle->lsn_conf.process_index = index;
		ret = cycle->handler.init_handler(cycle);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] init error!", __FILE__, __LINE__);
			exit(1);
		}
	
		ret = lsn_child_work_normal(cycle, index);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] lsn_child_work_ext error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child_lc(sw_lsn_cycle_t *cycle)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;

	for (i = 0; i < cycle->lsn_conf.procmin; i++)
	{
		ret = lsn_create_single_child_lc(cycle, i);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] create child error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child_tcplc(sw_lsn_cycle_t *cycle)
{
	pid_t   pid;
	sw_int_t    ret = 0;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] fork error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		cycle->lsn_conf.process_index = 0;
		ret = cycle->handler.init_handler(cycle);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] init error!", __FILE__, __LINE__);
			exit(1);
		}
		ret = lsn_child_work(cycle, 0);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] lsn_child_work_ext error!", __FILE__, __LINE__);
			exit(1);
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child_common(sw_lsn_cycle_t *cycle)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;

	for (i = 0; i < cycle->lsn_conf.procmin; i++)
	{
		pub_log_debug("[%s][%d] id=[%d]", __FILE__, __LINE__, i);
		ret = lsn_create_single_child_common(cycle, i);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_create_single error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child_normal(sw_lsn_cycle_t *cycle)
{
	sw_int_t	i = 0;
	sw_int_t	ret = 0;

	for (i = 0; i < cycle->lsn_conf.procmin; i++)
	{
		pub_log_debug("[%s][%d] id=[%d]", __FILE__, __LINE__, i);
		ret = lsn_create_single_child_normal(cycle, i);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] lsn_create_single error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

sw_int_t lsn_create_child(sw_lsn_cycle_t *cycle)
{
	sw_int_t	ret = 0;

	if (strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 || strcmp(cycle->lsn_conf.comtype, "TLQLA") == 0)
	{
		ret = lsn_create_child_mq(cycle);
	}
	else if (strcmp(cycle->lsn_conf.comtype, "TCPLA") == 0)
	{
		ret = lsn_create_child_la(cycle);
	}	
	else if (strcmp(cycle->lsn_conf.comtype, "TCPLC") == 0)
	{
		ret = lsn_create_child_tcplc(cycle);
	}
	else if (pub_str_strrncmp(cycle->lsn_conf.comtype, "LC", 2) == 0)
	{
		ret = lsn_create_child_lc(cycle);
	}
	else if (strcmp(cycle->lsn_conf.comtype, "SUNSS") == 0)
	{
		ret = lsn_create_child_normal(cycle);
	}
	else
	{
		ret = lsn_create_child_common(cycle);
	}

	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Create child error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] Create lsn success!", __FILE__, __LINE__);

	return SW_OK;
}

int lsn_main(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	int	times = 0;
	int	recv_cnt = 0;
	sw_int32_t	i = 0;
	sw_int64_t	timer = 0;
	sw_fd_list_t	*fd_work;
	sw_fd_set_t     *fd_set = NULL;
	sw_fd_list_t    *fd_list = NULL;

	if (cycle->handler.init_handler != NULL && strcmp(cycle->lsn_conf.comtype, "MQLA") != 0 
			&& strcmp(cycle->lsn_conf.comtype, "TLQLA") != 0 && pub_str_strrncmp(cycle->lsn_conf.comtype, "LC", 2) != 0)
	{
		ret = cycle->handler.init_handler(cycle);
		if (ret != SW_OK)
		{
			pub_log_error("%s, %d, init error", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	if (g_workmode_mp)
	{
		if (cycle->handler.init_handler != NULL && strcmp(cycle->lsn_conf.comtype, "MQLA") == 0 
				&& cycle->lsn_conf.comm.group[0].local.ip[0] != '\0' && cycle->lsn_conf.comm.group[0].local.port > 0)
		{
			ret = cycle->handler.init_handler(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("%s, %d, init error", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
	}

	fd_set = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_set_t));
	if (fd_set == NULL)
	{
		pub_log_error("[%s][%d] pub_pool_palloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = select_init(fd_set);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	fd_list = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_list_t));
	if (fd_list == NULL)
	{
		pub_log_error("[%s][%d] pub_pool_palloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	fd_list->fd = cycle->udp_fd;                                                                        
	fd_list->data = cycle;
	fd_list->event_handler = (sw_event_handler_pt)lsn_handle_control_cmd;                                                                                             
	ret = select_add_event(fd_set, fd_list);                                                                                        
	if (ret != SW_OK)                                                                                                               
	{                                                                                                                                  
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);                                       
		return SW_ERROR;                                                                                                           
	}

	cycle->lsn_conf.sendtype = -1;
	ret = lsn_create_child(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] lsn_create_child error!", __FILE__, __LINE__);
		return SW_DECLINED;
	}
	
	times = 0;
	while (1)
	{
		sleep(5);
		if (procs_check_svr_status(cycle->base.name.data, NULL) == SW_OK)
		{
			break;
		}
		
		times++;
		if (times > 12)
		{
			pub_log_error("[%s][%d] All child abnormal!", __FILE__, __LINE__);
			return SW_DECLINED;
		}
	}

	while (1)
	{
		timer = 60000;
		recv_cnt = select_process_events(fd_set, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error!", __FILE__, __LINE__);
			continue;
		}
		else if (recv_cnt == 0)
		{
			ret = lsn_handle_timeout(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] lsn_handle_timeout error!", __FILE__, __LINE__);
			}
			/***
			  pub_log_info("[%s][%d] No data arrive!", __FILE__, __LINE__);
			 ***/
		}
		else
		{
			pub_log_info("[%s][%d] Data arrived! ret=[%d]", __FILE__, __LINE__, recv_cnt);
			for (i = 0; i < recv_cnt; i++)
			{
				pub_log_info("[%s][%d] fd=[%d]", __FILE__, __LINE__, fd_work[i].fd);
				ret = fd_work[i].event_handler((sw_lsn_cycle_t *)fd_work[i].data);
				if (ret == SW_ABORT)
				{
					pub_log_info("[%s][%d] Recv exit cmd! Exit!", __FILE__, __LINE__);
					return SW_OK;
				}
				else if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] event_handler error!", __FILE__, __LINE__);
				}
			}
		}
	}

	return SW_OK;
}

sw_int_t lsn_child_update_stat(sw_lsn_cycle_t *cycle, sw_int32_t status)
{
	sw_int_t	ret = 0;
	sw_proc_info_t	proc_info;

	memset(&proc_info, 0x0, sizeof(proc_info));
	pub_log_info("[%s][%d] proc_name=[%s] status=[%d]", __FILE__, __LINE__, cycle->proc_name, status);
	ret = procs_get_proces_info(NULL, cycle->base.name.data, cycle->proc_name, &proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get proc info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] ori status=[%d]", __FILE__, __LINE__, proc_info.status);
	if (proc_info.status == status)
	{
		return SW_OK;
	}

	proc_info.status = status;
	ret = procs_lsn_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] register [%s] error!", 
				__FILE__, __LINE__, cycle->proc_name);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int32_t lsn_child_register(sw_lsn_cycle_t *cycle, sw_int32_t status, sw_char_t *lsn_name)
{
	sw_int_t	ret = 0;
	sw_proc_info_t	proc_info;

	pub_mem_memzero(&proc_info, sizeof(proc_info));	
	ret = procs_get_proces_info(NULL, cycle->base.name.data, lsn_name, &proc_info);
	if (ret == SW_ERROR)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		strncpy(proc_info.name, lsn_name, sizeof(proc_info.name) - 1);
		strcpy(proc_info.svr_name, cycle->base.name.data);
		proc_info.pid = getpid();
		pub_log_info("[%s][%d] lsn_name=[%s] sendtype=[%d]", 
				__FILE__, __LINE__, lsn_name, cycle->lsn_conf.sendtype);
		if (cycle->lsn_conf.sendtype == O_SEND)
		{
			proc_info.type = ND_LSN_SND;
		}
		else if (cycle->lsn_conf.sendtype == O_RECV)
		{
			proc_info.type = ND_LSN_RCV;
		}
		else
		{
			proc_info.type = ND_LSN;
		}
		proc_info.proc_index = cycle->lsn_conf.process_index;
		proc_info.group_index = cycle->lsn_conf.group_index;
		proc_info.mqid = cycle->lsn_conf.out_msgid;
		proc_info.status = status;
		proc_info.restart_type = LIMITED_RESTART;
	}
	else
	{
		pub_log_info("[%s][%d] proc [%s] already exist! name=[%s] svr_name=[%s]", \
				__FILE__, __LINE__, lsn_name, proc_info.name, proc_info.svr_name);
		proc_info.pid = getpid();
		proc_info.status = status;
		proc_info.mqid = cycle->lsn_conf.out_msgid;
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	pub_log_info("[%s][%d] Child:[%s] msgid=[%d]", __FILE__, __LINE__, proc_info.name, proc_info.mqid);
	ret = procs_lsn_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_lsn_register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(cycle->proc_name, lsn_name);
	pub_log_info("[%s][%d] [%s] register success!", __FILE__, __LINE__, lsn_name);
	return SW_OK;
}

sw_int32_t lsn_father_register(sw_lsn_cycle_t *cycle, sw_int32_t status)
{
	if ( cycle == NULL || status < 0)
	{
		pub_log_error("[%s][%d] Lsn father register param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sw_int32_t	ret = 0;
	sw_proc_info_t	proc_info;

	pub_mem_memzero(&proc_info, sizeof(proc_info));
	ret = procs_get_sys_by_name(cycle->base.name.data, &proc_info);
	if (ret != SW_OK)
	{
		strcpy(proc_info.name, cycle->base.name.data);
		proc_info.pid = getpid();
		proc_info.type = ND_LSN;
		proc_info.status = status;
		proc_info.shmid = cycle->shm.id;
		proc_info.restart_type = LIMITED_RESTART;
		proc_info.lockid = cycle->semid;
	}
	else
	{
		proc_info.pid = getpid();
		proc_info.status = status;
		proc_info.shmid = cycle->shm.id;
		proc_info.lockid = cycle->semid;
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	ret = procs_sys_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	else
	{
		pub_log_info("[%s][%d] name=[%s] register success, iStatus=[%d],PID=[%d]",__FILE__,
				__LINE__, cycle->base.name.data, proc_info.status, proc_info.pid);
	}
	return SW_OK;
}

sw_int_t lsn_handle_control_cmd(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	sw_cmd_t	cmd;

	memset(&cmd, 0x0, sizeof(cmd));
	ret = udp_recv(cycle->udp_fd, (char *)&cmd, sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] FD=[%d] recv failed,errno=[%d]:[%s]ret=[%d]",
				__FILE__, __LINE__, cycle->udp_fd, errno, strerror(errno), ret);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] upd_name=[%s] type=[%d]", __FILE__, __LINE__, cmd.udp_name, cmd.type);
	ret = lsn_pub_analyze_udpcmd(cycle, &cmd);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] lsn_pub_analyze_udpcmd error!", __FILE__, __LINE__);
		return ret;
	}
	else if (ret == SW_ABORT)
	{
		return SW_ABORT;
	}

	return SW_OK;
}

int lsn_send_cmd_to_child(sw_lsn_cycle_t *cycle, sw_cmd_t *pcmd)
{
	int	j = 0;
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;

	grp_svr = procs_get_svr_by_name(cycle->base.name.data, NULL);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] procs_get_svr_by_name error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] prdt=[%s] svr=[%s]", __FILE__, __LINE__, grp_svr->prdtname, grp_svr->svrname);

	for (j = 0; j < grp_svr->svc_curr; j++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d]",
				__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);
		memset(&cmd, 0x0, sizeof(cmd));
		memcpy(&cmd, pcmd, sizeof(sw_cmd_t));
		memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
		sprintf(cmd.udp_name, "%s", cycle->base.name.data);
		pub_log_info("[%s][%d] cmd.udp_name=[%s]", __FILE__, __LINE__, cmd.udp_name);
		udp_send(cycle->udp_fd, (char*)&cmd, sizeof(cmd), proc_info.name);
		memset(&cmd, 0x0, sizeof(cmd));
		udp_recv(cycle->udp_fd, (sw_char_t*)&cmd, sizeof(cmd));
		pub_log_info("[%s][%d] recv [%s]'s response!", __FILE__, __LINE__, proc_info.name);
	}

	return SW_OK;
}

int lsn_mod_prdt_file_status(char *xmlname, char *lsnname, char *stat)
{
	int	ret = 0;
	int	oldlen = 0;
	int	newlen = 0;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".PRODUCTS.PRODUCT");
	while (node != NULL)
	{
		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "LSNNAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config LSNNAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		if (strcmp(node1->value, lsnname) != 0)
		{
			node = node->next;
			continue;
		}

		node1 = pub_xml_locnode(xml, "GATE");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config GATE!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}

		newlen = strlen(stat);
		oldlen = strlen(node1->value);
		if (newlen > oldlen)
		{
			pub_log_info("[%s][%d] 超长,重新分配!", __FILE__, __LINE__);
			free(node1->value);
			node1->value = (char *)calloc(1, newlen + 1);
			if (node1->value == NULL)
			{
				pub_xml_deltree(xml);
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
						__FILE__, __LINE__, errno, strerror(errno));
				return SW_ERROR;
			}
		}
		else
		{
			memset(node1->value, 0x0, oldlen);
		}
		memcpy(node1->value, stat, newlen);
		pub_log_debug("[%s][%d] Update stat [%s] success!", __FILE__, __LINE__, stat);
		break;
	}

	ret = pub_xml_pack(xml, xmlname);
	if (ret < 0)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] xml pack error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_xml_deltree(xml);
	return SW_OK;
}

sw_int_t lsn_is_prdt_exist(sw_lsn_cycle_t *cycle, sw_char_t *prdt)
{
	int	i = 0;

	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, prdt) == 0)
		{
			return SW_OK;
		}
	}

	return SW_ERROR;
}

sw_int_t lsn_set_prdt_status(sw_lsn_cycle_t *cycle, sw_char_t *prdt, int status)
{
	int	i = 0;
	sw_int_t	ret = 0;
	sw_char_t	stat[8];
	sw_char_t	xmlname[128];

	memset(stat, 0x0, sizeof(stat));
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/lsncfg/route.xml", cycle->base.work_dir.data, prdt);
	pub_log_info("[%s][%d] xmlname=[%s]", __FILE__, __LINE__, xmlname);
	if (status == SW_SWITCH_OFF)
	{
		strcpy(stat, "off");
	}
	else
	{
		strcpy(stat, "on");
	}

	ret = lsn_mod_prdt_file_status(xmlname, cycle->lsn_conf.name, stat);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Update lsn [%s] file status error!", __FILE__, __LINE__, cycle->lsn_conf.name);
		return SW_ERROR;
	}

	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, prdt) == 0)
		{
			cycle->routes->route[i].gate = status;
			return SW_OK;
		}
	}
	pub_log_error("[%s][%d] Can not find prdt [%s]!", __FILE__, __LINE__, prdt);

	return SW_ERROR;
}

sw_int_t lsn_add_prdt(sw_lsn_cycle_t *cycle, sw_char_t *prdt)
{
	sw_int_t	ret = 0;

	pub_log_info("[%s][%d] add prdt [%s] begin...", __FILE__, __LINE__, prdt);
	ret = lsn_prdt_stat_init(cycle, prdt);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get prdt [%s] status error!", __FILE__, __LINE__, prdt);
		return SW_ERROR;
	}
	ret = lsn_route_cfg_init(cycle, prdt, -1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] add prdt route[%s] error!", __FILE__, __LINE__, prdt);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] add prdt route success!", __FILE__, __LINE__);

	ret = lsn_prdt_cfg_init(cycle, prdt, -1);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] add [%s] lsnccf error!", __FILE__, __LINE__, prdt);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] add lsncfg success!", __FILE__, __LINE__);
	pub_log_info("[%s][%d] add prdt [%s] success!", __FILE__, __LINE__, prdt);

	return SW_OK;
}

sw_int_t lsn_get_route_in_prdt(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd) 
{
	int	i   = 0;
	sw_route_t	*route = NULL;
	if (cycle == NULL || cmd == NULL || cmd->dst_prdt == NULL || cmd->dst_prdt[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, cmd->dst_prdt) == 0)
		{
			break;
		}
	}
	if (i == cycle->routes->head.route_cnt)
	{
		pub_log_error("[%s][%d] Can not find product[%s] in memory!", __FILE__, __LINE__, cmd->dst_prdt);
		return SW_ERROR;
	}
	route = &cycle->routes->route[i];
	cmd->status = route->gate;

	return SW_OK;
}
sw_int_t lsn_upd_prdt(sw_lsn_cycle_t *cycle, sw_char_t *prdt) 
{
	int	i = 0;
	int	ret = 0;

	if (cycle == NULL || prdt == NULL || prdt[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, prdt) == 0)
		{
			break;
		}
	}
	if (i == cycle->routes->head.route_cnt)
	{
		pub_log_error("[%s][%d] Can not find [%s] in memory!", __FILE__, __LINE__, prdt);
		return SW_ERROR;
	}

	ret = lsn_route_cfg_upd(cycle, prdt, i);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] update route cfg error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] update route cfg success!", __FILE__, __LINE__);

	ret = lsn_prdt_cfg_upd(cycle, prdt, i);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] update prdt cfg error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] update prdt cfg success!", __FILE__, __LINE__);

	if (cycle->lsn_conf.svrmapcfg[0] != '\0')
	{
		ret = lsn_cache_upd_svrmap(cycle, i);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Update svrmap cfg error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] update svrmap cfg success!", __FILE__, __LINE__);
	pub_log_debug("[%s][%d] Update [%s] success!", __FILE__, __LINE__, prdt);

	return SW_OK;
}

static sw_int_t get_all_lsncfg_handle(sw_lsn_cycle_t *cycle, sw_char_t *name)
{
	int 	i = 0;
	int     ret = 0;
	sw_route_t      *route = NULL;
	if (cycle == NULL || name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, name) == 0)
		{
			break;
		}
	}
	if (i == cycle->routes->head.route_cnt)
	{
		pub_log_error("[%s][%d] Can not find [%s] in memory!", __FILE__, __LINE__, name);
		return SW_ERROR;
	}
	route = &cycle->routes->route[i];
	pub_log_info("[%s][%d] add lsncfg plugin handle begin...", __FILE__, __LINE__);
	if (route->prdt_cfg.codeconvert[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.codeconvert, route->name,
				&route->handle.code_handle, (void *)&route->fun.code_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.codeconvert);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.typeanalyze[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.typeanalyze, route->name,
				&route->handle.type_handle, (void *)&route->fun.type_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.typeanalyze);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.preanalyze[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.preanalyze, route->name,
				&route->handle.prean_handle, (void *)&route->fun.prean_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.preanalyze);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.synres[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.synres, route->name,
				&route->handle.synres_handle, (void *)&route->fun.synres_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.synres);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.recover[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.recover, route->name,
				&route->handle.recov_handle, (void *)&route->fun.recov_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.recover);
			return SW_ERROR;
		}
	}
	if (route->prdt_cfg.afterdeal[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.afterdeal, route->name,
				&route->handle.after_handle, (void *)&route->fun.after_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!",
					__FILE__, __LINE__, route->prdt_cfg.recover);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] add lsncfg plugin handle success", __FILE__, __LINE__);

	return SW_OK;
}

static sw_int_t lsn_prdt_svrmap_add(sw_lsn_cycle_t *cycle, sw_char_t *prdt)
{
	int     i = 0;
	int     ret = 0;
	if (cycle == NULL || prdt == NULL || prdt[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		if (strcmp(cycle->routes->route[i].name, prdt) == 0)
		{
			break;
		}
	}
	if (i == cycle->routes->head.route_cnt)
	{
		pub_log_error("[%s][%d] Can not find [%s] in memory!", __FILE__, __LINE__, prdt);
		return SW_ERROR;
	}
	if (cycle->lsn_conf.svrmapcfg[0] != '\0')
	{
		ret = lsn_cache_upd_svrmap(cycle, i);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] add svrmap cfg error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] add svrmap cfg success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_upd_cfg(sw_lsn_cycle_t *cycle)
{
	int	ret = 0;
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_chnl_t	chnl;
	sw_lsn_config_t	lsn_conf;

	memset(xmlname, 0x0, sizeof(xmlname));
	memset(&chnl, 0x0, sizeof(chnl));
	memset(&lsn_conf, 0x0, sizeof(lsn_conf));
	chnl.cache.pkgdeal = NULL;
	chnl.cache.pkgmap = NULL;
	chnl.cache.svrmap = NULL;
	chnl.fun.file_func = NULL;
	chnl.fun.pkgmap_func = NULL;
	chnl.fun.deny_func = NULL;
	chnl.handle.file_handle = NULL;
	chnl.handle.pkgmap_handle = NULL;
	chnl.handle.deny_handle = NULL;

	sprintf(xmlname, "%s/cfg/listener.lsn", cycle->base.work_dir.data);
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".DFISBP.SWLISTEN");
	while (node != NULL)
	{
		if (strcmp(node->name, "SWLISTEN") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			return SW_ERROR;
		}

		if (strcmp(node1->value, cycle->lsn_conf.name) == 0)
		{
			break;
		}

		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未找到侦听[%s]的配置!", __FILE__, __LINE__, cycle->lsn_conf.name);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, "TIMEOUT");
	if (node != NULL && node->value != NULL)
	{
		lsn_conf.timeout = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "ACTIVETIME");
	if (node != NULL && node->value != NULL)
	{
		lsn_conf.activetime = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "ACTIVEMSG");
	if (node != NULL && node->value != NULL)
	{
		strncpy(lsn_conf.activemsg, node->value, sizeof(lsn_conf.activemsg) - 1);
	}

	node = pub_xml_locnode(xml, "SCANTIME");
	if (node != NULL && node->value != NULL)
	{
		lsn_conf.scantime = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "DATA");
	if (node != NULL && node->value != NULL)
	{
		strncpy(lsn_conf.data, node->value, sizeof(lsn_conf.data) - 1);
	}

	node = pub_xml_locnode(xml, "CONMAX");
	if (node != NULL && node->value != NULL)
	{
		lsn_conf.conmax = atoi(node->value);
	}

	node = pub_xml_locnode(xml, "PKGCFG");
	if (node == NULL || node->value == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未配置PKGCFG!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_debug("[%s][%d] PKGCFG=[%s]", __FILE__, __LINE__, node->value);
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/common/%s", cycle->base.work_dir.data, node->value);
	chnl.cache.pkgdeal = cfg_read_xml(xmlname);
	if (chnl.cache.pkgdeal == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, "MAPCFG");
	if (node != NULL && node->value != NULL)
	{
		memset(xmlname, 0x0, sizeof(xmlname));
		sprintf(xmlname, "%s/cfg/common/%s", cycle->base.work_dir.data, node->value);
		chnl.cache.pkgmap = cfg_read_xml(xmlname);
		if (chnl.cache.pkgmap == NULL)
		{
			pub_xml_deltree(xml);
			pub_xml_deltree(chnl.cache.pkgdeal);
			pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
			return SW_ERROR;
		}
	}

	node = pub_xml_locnode(xml, "FILEDEAL");
	if (node != NULL && node->value != NULL)
	{
		ret = lsn_get_plugin_handle(cycle, node->value, NULL,
				&chnl.handle.file_handle, (void *)&chnl.fun.file_func);
		if (ret != SW_OK)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] Get [%s] handle error!", __FILE__, __LINE__, node->value);
			return SW_ERROR;
		}
	}

	node = pub_xml_locnode(xml, "SVRMAP");
	if (node != NULL && node->value != NULL)
	{
		ret = lsn_get_plugin_handle(cycle, node->value, NULL,
				&chnl.handle.svrmap_handle, (void *)&chnl.fun.svrmap_func);
		if (ret != SW_OK)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] Get [%s] handle error!", __FILE__, __LINE__, node->value);
			return SW_ERROR;
		}         
	}

	node = pub_xml_locnode(xml, "DENYSERVCIE");
	if (node != NULL && node->value != NULL)
	{
		ret = lsn_get_plugin_handle(cycle, node->value, NULL,
				&chnl.handle.deny_handle, (void *)&chnl.fun.deny_func);
		if (ret != SW_OK)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] Get [%s] handle error!", __FILE__, __LINE__, node->value);
			return SW_ERROR;
		}
	}

	node = pub_xml_locnode(xml, "COMM");
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Not config COMM!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = lsn_pub_set_lsn_comm(xml, node, &lsn_conf);
	if (ret != SW_OK)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Get comm info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (lsn_conf.timeout > 0)
	{
		cycle->lsn_conf.timeout = lsn_conf.timeout;
	}

	if (lsn_conf.activetime > 0)
	{
		cycle->lsn_conf.activetime = lsn_conf.activetime;
	}

	if (lsn_conf.activemsg[0] != '\0')
	{
		strcpy(cycle->lsn_conf.activemsg, lsn_conf.activemsg);
	}

	if (chnl.cache.pkgdeal != NULL)
	{
		if (cycle->chnl.cache.pkgdeal != NULL)
		{
			pub_xml_deltree(cycle->chnl.cache.pkgdeal);
		}
		cycle->chnl.cache.pkgdeal = chnl.cache.pkgdeal;
	}

	if (chnl.cache.pkgmap != NULL)
	{
		if (cycle->chnl.cache.pkgmap != NULL)
		{
			pub_xml_deltree(cycle->chnl.cache.pkgmap);
		}
		cycle->chnl.cache.pkgmap = chnl.cache.pkgmap;
	}

	if (chnl.fun.file_func != NULL)
	{
		if (cycle->chnl.fun.file_func != NULL)
		{
			dlclose(cycle->chnl.handle.file_handle);
		}
		cycle->chnl.fun.file_func = chnl.fun.file_func;
		cycle->chnl.handle.file_handle = chnl.handle.file_handle;
	}

	if (chnl.fun.deny_func != NULL)
	{
		if (cycle->chnl.fun.deny_func != NULL)
		{
			dlclose(cycle->chnl.handle.deny_handle);
		}
		cycle->chnl.fun.deny_func = chnl.fun.deny_func;
		cycle->chnl.handle.deny_handle = chnl.handle.deny_handle;
	}

	pub_xml_deltree(xml);
	pub_log_debug("[%s][%d] Update lsn info success!", __FILE__, __LINE__);

	return SW_OK;
}

sw_int_t lsn_pub_analyze_udpcmd(sw_lsn_cycle_t *cycle, sw_cmd_t *cmd)
{
	sw_int_t	ret = 0;
	sw_char_t	udp_name[64];

	memset(udp_name, 0x0, sizeof(udp_name));

	if (cmd == NULL || cycle == NULL)
	{
		pub_log_error("[%s][%d]input param error", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] udp_name=[%s] type=[%d]",
			__FILE__, __LINE__, cmd->udp_name, cmd->type);

	switch (cmd->type)
	{
		case SW_MSTOPIONE:
		case SW_MSTOPONE:
			/*** STOPPING ***/
			lsn_father_register(cycle, SW_S_STOPPING);
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			pub_mem_memzero(cmd->udp_name, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char *)cmd, sizeof (sw_cmd_t), udp_name);
			/*** SEND CMD TO ALL CHILD ***/
			cmd->type = SW_MSTOPSELF;
			lsn_send_cmd_to_child(cycle, cmd);
			pub_log_info("[%s][%d] send cmd to child success!", __FILE__, __LINE__);
			/*** STOPPED ***/
			lsn_father_register(cycle, SW_S_STOPED);
			pub_log_info("[%s][%d] send response to admin success!", __FILE__, __LINE__);
			return SW_ABORT;
		case SW_STOP_PRDT:
			pub_log_info("[%s][%d] Stop prdt [%s]!", __FILE__, __LINE__, cmd->dst_prdt);
		case SW_DEL_PRDT:
			pub_log_info("[%s][%d] Stop prdt [%s]", __FILE__, __LINE__, cmd->dst_prdt);
			ret = lsn_set_prdt_status(cycle, cmd->dst_prdt, SW_SWITCH_OFF);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] stop prdt [%s] error!", 
						__FILE__, __LINE__, cmd->dst_prdt);
			}
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			pub_mem_memzero(cmd->udp_name, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char *)cmd, sizeof (sw_cmd_t), udp_name);
			pub_log_info("[%s][%d] Stop prdt [%s] success!", 
					__FILE__, __LINE__, cmd->dst_prdt);
			return SW_OK;
		case SW_START_PRDT:
			pub_log_info("[%s][%d] prdt_name=[%s]", __FILE__, __LINE__, cmd->dst_prdt);
			ret = lsn_is_prdt_exist(cycle, cmd->dst_prdt);
			if (ret == SW_ERROR)
			{
				ret = lsn_add_prdt(cycle, cmd->dst_prdt);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] Add prdt [%s] error!", 
							__FILE__, __LINE__, cmd->dst_prdt);
				}
				else
				{
					cycle->routes->head.route_cnt++;
				}
			}
			else
			{
				ret = lsn_set_prdt_status(cycle, cmd->dst_prdt, SW_SWITCH_ON);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] start prdt [%s] error!", 
							__FILE__, __LINE__, cmd->dst_prdt);
				}
			}
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			pub_mem_memzero(cmd->udp_name, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char *)cmd, sizeof(sw_cmd_t), udp_name);
			pub_log_info("[%s][%d] Start prdt [%s] success!", 
					__FILE__, __LINE__, cmd->dst_prdt);
			return SW_OK;
		case SW_ADD_PRDT:
			ret = lsn_add_prdt(cycle, cmd->dst_prdt);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Add prdt [%s] error!", 
						__FILE__, __LINE__, cmd->dst_prdt);
			}
			else
			{
				cycle->routes->head.route_cnt++;
			}
			cmd->type = SW_ADD_PRDT;
			lsn_send_cmd_to_child(cycle, cmd);
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			pub_mem_memzero(cmd->udp_name, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char *)cmd, sizeof(sw_cmd_t), udp_name);
			pub_log_info("[%s][%d] Add prdt [%s] success!", 
					__FILE__, __LINE__, cmd->dst_prdt);
			return SW_OK;
		case SW_UPD_PRDT:
			ret = lsn_upd_prdt(cycle, cmd->dst_prdt);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Update prdt [%s] error!", 
						__FILE__, __LINE__, cmd->dst_prdt);
			}
			cmd->type = SW_UPD_PRDT;
			lsn_send_cmd_to_child(cycle, cmd);
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			pub_mem_memzero(cmd->udp_name, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char *)cmd, sizeof(sw_cmd_t), udp_name);
			pub_log_info("[%s][%d] Update prdt [%s] success!", 
					__FILE__, __LINE__, cmd->dst_prdt);
			return SW_OK;
		case SW_UPD_LSN:
			ret = lsn_upd_cfg(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Update lsn cfg error!", __FILE__, __LINE__);
			}
			cmd->type = SW_UPD_LSN;
			lsn_send_cmd_to_child(cycle, cmd);
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			pub_mem_memzero(cmd->udp_name, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char *)cmd, sizeof(sw_cmd_t), udp_name);
			pub_log_info("[%s][%d] Update lsn success!", __FILE__, __LINE__);
			return SW_OK;
		case SW_GET_LSN_ROUTE_IN_PRDT:
			lsn_get_route_in_prdt(cycle, cmd);
			if (ret == SW_ERROR)
			{
				cmd->status = -1;
				pub_log_error("[%s][%d] get lsn status in product error!", __FILE__, __LINE__);
			}
			pub_log_info("[%s][%d] get lsn status in product[%s] success!", __FILE__, __LINE__, cmd->dst_prdt);
			cmd->type = SW_RESCMD;
			sprintf(udp_name, "%s", cmd->udp_name);
			memset(cmd->udp_name, 0x0, sizeof(cmd->udp_name));
			sprintf(cmd->udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->udp_fd, (char*)cmd, sizeof(sw_cmd_t), udp_name);
			return ret;
		default:
			pub_log_error("[%s][%d] Received unknown command [%d]!",
					__FILE__, __LINE__, cmd->type);
			break;
	}

	return SW_ERROR;
}

int run_enc_fun(sw_loc_vars_t *vars, sw_buf_t *locbuf, char *libname, char *funname, int flag)
{
	int ret = 0; 
	int (* dlfun)(sw_loc_vars_t *,  sw_buf_t *, int);
	static int first_use = 1; 
	static sw_pkg_cache_list_t  handle_cache;

	void    *handle = NULL;

	if (first_use)
	{    
		memset(&handle_cache, 0x0, sizeof(handle_cache));
		first_use = 0; 
	}    

	handle = get_handle_from_cache(&handle_cache, libname);
	if (handle == NULL)
	{    
		uLog(SW_LOG_ERROR, "[%s][%d] dlopen [%s] error! ", __FILE__, __LINE__, libname);
		return -1;
	}    

	dlfun = (int (*)(sw_loc_vars_t *, sw_buf_t *, int))dlsym(handle, funname);
	if (dlfun == NULL)
	{    
		uLog(SW_LOG_ERROR, "[%s][%d] dlsym [%s][%s] error! [%s]", __FILE__, __LINE__, libname, funname, dlerror());
		return -1;
	}

	ret = dlfun(vars, locbuf, flag);
	if (ret < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlfun error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	uLog(SW_LOG_DEBUG, "[%s][%d] run [%s][%s] success!", __FILE__, __LINE__, libname, funname);

	return 0;
}

int run_file_fun(char *funname, sw_loc_vars_t *vars, int flag, int sockid, void *param)
{
	int ret = 0;
	int (* dlfun)(sw_loc_vars_t *, int, int, void *);
	char    *ptr = NULL;
	char    fun[64];
	char    lib[64];
	void    *handle = NULL;
	static int first_use = 1;
	static sw_pkg_cache_list_t  handle_cache;

	if (first_use)
	{
		memset(&handle_cache, 0x0, sizeof(handle_cache));
		first_use = 0;
	}

	memset(fun, 0x0, sizeof(fun));
	memset(lib, 0x0, sizeof(lib));

	ptr = strchr(funname, '/');
	if (ptr == NULL)
	{
		strcpy(fun, funname);
		strcpy(lib, "libfile.so");
	}
	else
	{
		strcpy(fun, ptr + 1);
		strncpy(lib, funname, ptr - funname);
	}

	handle = get_handle_from_cache(&handle_cache, lib);
	if (handle == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlopen [%s] error! ", __FILE__, __LINE__, lib);
		return -1;
	}

	dlfun = (int (*)(sw_loc_vars_t *, int, int, void *))dlsym(handle, fun);
	if (dlfun == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlsym [%s][%s] error! [%s]", __FILE__, __LINE__, lib, funname, dlerror());
		return -1;
	}       
	ret = dlfun(vars, flag, sockid, param);
	if (ret < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] dlfun error! ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}
	uLog(SW_LOG_DEBUG, "[%s][%d] run [%s][%s] success!", __FILE__, __LINE__, lib, funname);

	return 0;
}

int get_xmltree_from_cache(sw_pkg_cache_list_t *cache, char *name)
{
	char path[256];
	sw_xmltree_t *xml = NULL;
	if (cache == NULL || name == NULL || name[0] == '\0')
	{   
		pub_log_error("[%s][%d] get xmltree from cache error", __FILE__, __LINE__);
		return -1;
	}   

	int i = 0;
	for (i = 0; i < cache->cnt; ++i)
	{   
		if (strcmp(cache->processor[i].name, name) == 0)
		{   
			return i;
		}   
	}   

	if (cache->cnt >= PKG_MAX_PROCESSOR_CNT)
	{   
		pub_log_error("[%s][%d] cannot cache xmltree  max[%d]", __FILE__, __LINE__, PKG_MAX_PROCESSOR_CNT);
		return -1;
	}   

	memset(path, 0x0, sizeof(path));
	sprintf(path, "%s/cfg/common/%s", getenv("SWWORK"), name);
	if (pub_file_exist(path) == 0)
	{
		pub_log_error("[%s][%d] file is not exist[%s]", __FILE__, __LINE__, path);
		return -1;
	}
	xml = cfg_read_xml(path);
	if (xml == NULL)
	{   
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, name);
		return -1; 
	}

	strncpy(cache->processor[i].name, name, 128 - 1);
	cache->processor[i].xml = xml;
	cache->cnt ++;

	return i;
}

void *get_handle_from_cache(sw_pkg_cache_list_t *cache, char *name)
{
	char path[256];
	void *handle = NULL;
	if (cache == NULL || name == NULL || name[0] == '\0')
	{   
		pub_log_error("[%s][%d] get xmltree from cache error", __FILE__, __LINE__);
		return NULL;
	}   

	int i = 0;
	for (i = 0; i < cache->cnt; ++i)
	{   
		if (strcmp(cache->processor[i].name, name) == 0)
		{   
			return cache->processor[i].handle;
		}   
	}   

	if (cache->cnt >= PKG_MAX_PROCESSOR_CNT)
	{   
		pub_log_error("[%s][%d] cannot cache handle max[%d]", __FILE__, __LINE__, PKG_MAX_PROCESSOR_CNT);
		return NULL;
	}   

	memset(path, 0x0, sizeof(path));
	sprintf(path, "%s/plugin/%s", getenv("SWWORK"), name);
	if (pub_file_exist(path) == 0)
	{   
		memset(path, 0x0, sizeof(path));
		sprintf(path, "%s/plugin/%s", getenv("SWHOME"), name);
		if (pub_file_exist(path) == 0)
		{
			pub_log_error("[%s][%d] file is not exist[%s]", __FILE__, __LINE__, name);
			return NULL;
		}
	}
	handle = (void *)dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	if (handle == NULL)
	{    
		uLog(SW_LOG_ERROR, "[%s][%d] dlopen [%s] error! [%s]", __FILE__, __LINE__, path, dlerror());
		return NULL;
	}

	strncpy(cache->processor[cache->cnt].name, name, 128 - 1);
	cache->processor[cache->cnt].handle = handle;
	cache->cnt ++;

	return handle;
}
