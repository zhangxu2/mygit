/*************************************************
  文 件 名:  flw_sp2073.c                        **
  功能描述:  配置文件冲突检测                    **
  作    者:  邹佩                                **
  完成日期:  20160803                            **
 *************************************************/
#include "agent_comm.h"

static char dbconf[5][32] = {{"DBNAME"},{"DBSID"},{"DBUSER"},{"DBPASSWD"},{"DBTYPE"}};

sw_int_t lsn_nodecmp(char bp_lis[][256], char in_lis[][256], int bp_cnt, int in_cnt)
{
	int i = 0;
	int j = 0;
	
	if (bp_cnt != in_cnt)
	{
		return -1;
	}

	for (i = 0; i < bp_cnt; i++)
	{
		
		for (j = 0; j < in_cnt; j++)
		{
			pub_log_info("[%s][%d] %s....%s", __FILE__, __LINE__, bp_lis[i], bp_lis[j]);
			if(strcmp(bp_lis[i], in_lis[j]) == 0)
			{
				break;
			}
		}
		
		if (j == in_cnt)
		{
			return -1;
		}
	}
	
	return 0;
}

int file_cmp(const char *path1, const char *path2)
{
	int	fd = 0;
	char	*buffer1 = NULL;
	char	*buffer2 = NULL;
	struct stat	f_stat1;
	struct stat	f_stat2;

	memset(&f_stat1, 0x0, sizeof(f_stat1));
	if(stat(path1, &f_stat1) == -1) 
	{   
		pub_log_info("[%s][%d]  file [%s] is not exist", __FILE__, __LINE__, path1);
		return -2; 
	}   

	memset(&f_stat2, 0x0, sizeof(f_stat2));
	if(stat(path2, &f_stat2) == -1)
	{
		pub_log_info("[%s][%d]  file [%s] is not exist", __FILE__, __LINE__, path2);
		return -2;
	}

	if (f_stat1.st_size != f_stat2.st_size)
	{
		pub_log_info("[%s][%d] file[%s]size[%lu] is different from file [%s] size [%lu]",

			__FILE__, __LINE__, path1, f_stat1.st_size, path2, f_stat2.st_size);
		return -1;
	}

	fd = open(path1, O_RDONLY);
	if(fd < 0)
	{
		pub_log_error("[%s][%d] open file[%s]error errno[%d]:[%s]",
			__FILE__ , __LINE__,path1,  errno, strerror(errno));
		return -1;
	}
	buffer1 = mmap(NULL, f_stat1.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (buffer1 == NULL)
	{
		pub_log_error("[%s][%d] mmap file[%s]error errno[%d]:[%s]",
			__FILE__ , __LINE__,path1,  errno, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	fd = open(path2, O_RDONLY);
	if(fd < 0)
	{
		pub_log_error("[%s][%d] open file[%s]error errno[%d]:[%s]",
			__FILE__ , __LINE__,path2,  errno, strerror(errno));
		munmap(buffer1, f_stat1.st_size);
		return -1;
	}
	buffer2 = mmap(NULL, f_stat2.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (buffer2 == NULL)
	{
		pub_log_error("[%s][%d] mmap file[%s]error errno[%d]:[%s]",
			__FILE__ , __LINE__,path2,  errno, strerror(errno));
		close(fd);
		munmap(buffer1, f_stat1.st_size);
		return -1;
	}
	close(fd);

	if (memcmp(buffer1, buffer2, f_stat1.st_size) != 0)
	{
		pub_log_info("[%s][%d] file[%s] is different from file [%s]",
			__FILE__, __LINE__, path1, path2);
		munmap(buffer1, f_stat1.st_size);
		munmap(buffer2, f_stat2.st_size);
		return -1;
	}

	munmap(buffer1, f_stat1.st_size);
	munmap(buffer2, f_stat2.st_size);
	return 0;
}

static int list_i = 0;
static int value_j = 0;
int node_cmp(sw_loc_vars_t *vars, sw_xmltree_t *ins_xml, sw_xmltree_t *ora_xml, int add, char *node_name)
{
	int 	find = 0;
	int 	diff = 0;
	char	buf[128];
	char	path[256];
	char	value1[64];
	char	value2[64];
	char	name[64];
	char	child_name[64];
	sw_xmlnode_t    *node_ins2 = NULL;
	sw_xmlnode_t    *node_ora2 = NULL;
	sw_xmlnode_t    *node_ins3 = NULL;
	sw_xmlnode_t    *node_ora3 = NULL;
	
	loc_set_zd_data(vars, "#value", "");

	memset(value1, 0x0, sizeof(value1));
	node_ins2 = pub_xml_locnode(ins_xml, node_name);
	if (node_ins2 != NULL && node_ins2->value != NULL)
	{
		strncpy(value1, node_ins2->value, sizeof(value1)-1);
	}

	memset(value2, 0x0, sizeof(value2));
	if (add == 0)
	{
		node_ora2 = pub_xml_locnode(ora_xml, node_name);
		if (node_ora2 != NULL && node_ora2->value != NULL)
		{
			strncpy(value2, node_ora2->value, sizeof(value2)-1);
		}
	}	

	if (strcmp(value1, value2) != 0)
	{
		if (value2[0] == '\0')
		{
			memset(path, 0x0, sizeof(path));
			memset(buf, 0x0, sizeof(buf));
			sprintf(path, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).CFTS.CFT(%d).PKGCFT", list_i, value_j);
			snprintf(buf, sizeof(buf)-1, "%s:%s", node_name, value1);
			loc_set_zd_data(vars, path, buf);
		}
		else
		{
			memset(path, 0x0, sizeof(path));
			memset(buf, 0x0, sizeof(buf));
			sprintf(path, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).CFTS.CFT(%d).PKGCFT", list_i, value_j);
			snprintf(buf, sizeof(buf)-1, "%s:%s", node_name, value1);
			loc_set_zd_data(vars, path, buf);
			memset(path, 0x0, sizeof(path));
			memset(buf, 0x0, sizeof(buf));
			sprintf(path, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).CFTS.CFT(%d).PTCFT", list_i, value_j);
			snprintf(buf, sizeof(buf)-1, "%s:%s", node_name, value2);
			loc_set_zd_data(vars, path, buf);
		}

		value_j ++;
		return 1;
	}

	if (strlen(value1) != 0)
	{
			loc_set_zd_data(vars, "#value", value1);
			return 0;
	}

	/*检测部分节点下有同名多节点的情况*/
	diff = 0;
	if ( node_ins2 != NULL && node_ins2->firstchild != NULL)
	{
		node_ins3 = node_ins2->firstchild;
		while(node_ins3 != NULL)
		{
			memset(child_name, 0x0, sizeof(child_name));
			strncpy(child_name, node_ins3->name, sizeof(child_name)-1);
			memset(name, 0x0, sizeof(name));
			snprintf(name, sizeof(name)-1, "%s.%s", node_name, child_name);
			pub_log_info("[%s][%d]同名多节点[%s]", __FILE__, __LINE__, name);

			memset(value1, 0x0, sizeof(value1));
			if (node_ins3->value == NULL)
			{
				node_ins3 = node_ins3->next;
				continue;
			}
			strncpy(value1, node_ins3->value, sizeof(value1)-1);

			find = 0;
			memset(value2, 0x0, sizeof(value2));
			if (add == 0 && node_ora2 != NULL && node_ora2->firstchild != NULL)
			{
				node_ora3 = node_ora2->firstchild;			
				while(node_ora3 != NULL)
				{
					if (strcmp(node_ora3->name, child_name) == 0 && node_ora3->value != NULL && strcmp(node_ora3->value, value1) == 0)
					{
						find = 1;
						break;
					}
					node_ora3 = node_ora3->next;
				}
			}	

			if (find == 0)
			{
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).CFTS.CFT(%d).PKGCFT", list_i, value_j);
				memset(buf, 0x0, sizeof(buf));
				snprintf(buf, sizeof(buf)-1, "%s:%s", name, value1);
				loc_set_zd_data(vars, path, buf);
				diff = 1;
				value_j ++;
			}
			node_ins3 = node_ins3->next;
		}
	}

	return diff;
}

int check_lsn(sw_loc_vars_t *vars, char *ins_path)
{
	int 	result = 0;
	int 	add = 0;
	int 	diff = 0;
	char	ora_path[256];
	char 	ins_chl[256];
	char 	ora_chl[256];
	char 	ins_lsn[256];
	char 	ora_lsn[256];
	char	lsn_name[64];
	char	buf[128];
	char	tmp[128];
	char	path[256];
	char	path1[256];
	sw_xmltree_t    *ins_lsn_xml = NULL;
	sw_xmltree_t    *ora_lsn_xml = NULL;
	sw_xmltree_t    *ins_chl_xml = NULL;
	sw_xmltree_t    *ora_chl_xml = NULL;
	sw_xmlnode_t    *node = NULL;
	sw_xmlnode_t    *node_ins_chl1 = NULL;
	sw_xmlnode_t    *node_ins_lsn1 = NULL;
	sw_xmlnode_t    *node_ora_chl1 = NULL;
	sw_xmlnode_t    *node_ora_lsn1 = NULL;

	memset(ora_path, 0x0, sizeof(ora_path));
	snprintf(ora_path, sizeof(ora_path)-1, "%s", getenv("SWWORK"));

	memset(ins_chl, 0x0, sizeof(ins_chl));
	snprintf(ins_chl, sizeof(ins_chl)-1, "%s/cfg/channels.xml", ins_path);
	ins_chl_xml = pub_xml_crtree(ins_chl);
	if (ins_chl_xml == NULL)
	{
		pub_log_error("[%s][%d] %s 建树失败", __FILE__, __LINE__, ins_chl);
		return -1;
	}

	memset(ins_lsn, 0x0, sizeof(ins_lsn));
	snprintf(ins_lsn, sizeof(ins_lsn)-1, "%s/cfg/listener.lsn", ins_path);
	ins_lsn_xml = pub_xml_crtree(ins_lsn);
	if (ins_lsn_xml == NULL)
	{
		pub_log_error("[%s][%d] %s 建树失败", __FILE__, __LINE__, ins_lsn);
		pub_xml_deltree(ins_chl_xml);
		return -1;
	}

	memset(ora_chl, 0x0, sizeof(ora_chl));
	snprintf(ora_chl, sizeof(ora_chl)-1, "%s/cfg/channels.xml", ora_path);
	ora_chl_xml = pub_xml_crtree(ora_chl);
	if (ora_chl_xml == NULL)
	{
		pub_log_error("[%s][%d] %s 建树失败", __FILE__, __LINE__, ora_chl);
		pub_xml_deltree(ins_lsn_xml);
		pub_xml_deltree(ins_chl_xml);
		return -1;
	}

	memset(ora_lsn, 0x0, sizeof(ora_lsn));
	snprintf(ora_lsn, sizeof(ora_lsn)-1, "%s/cfg/listener.lsn", ora_path);
	ora_lsn_xml = pub_xml_crtree(ora_lsn);
	if (ora_lsn_xml == NULL)
	{
		pub_log_error("[%s][%d] %s 建树失败", __FILE__, __LINE__, ora_lsn);
		pub_xml_deltree(ins_lsn_xml);
		pub_xml_deltree(ins_chl_xml);
		pub_xml_deltree(ora_chl_xml);
		return -1;
	}

	node = pub_xml_locnode(ins_chl_xml,".DFISBP.CHANNEL");
	list_i = 0;
	while(node != NULL)
	{
		add = 0;
		diff = 0;
		value_j = 0;
		ins_chl_xml->current = node;
		node_ins_chl1 = pub_xml_locnode(ins_chl_xml, "LISTEN");
		if (node_ins_chl1 == NULL || node_ins_chl1->value == NULL)
		{
			pub_log_error("[%s][%d]  查找节点LISTEN失败", __FILE__, __LINE__);
			pub_xml_deltree(ins_lsn_xml);
			pub_xml_deltree(ins_chl_xml);
			pub_xml_deltree(ora_lsn_xml);
			pub_xml_deltree(ora_chl_xml);
			return -1;	
		}
		memset(lsn_name, 0x0, sizeof(lsn_name));
		strncpy(lsn_name, node_ins_chl1->value, sizeof(lsn_name) - 1);

		node_ins_lsn1 = agt_xml_search(ins_lsn_xml, ".DFISBP.SWLISTEN", "NAME", lsn_name);
		if (node_ins_lsn1 == NULL)
		{
			pub_log_error("[%s][%d]  查找节点[%s]失败", __FILE__, __LINE__, lsn_name);
			pub_xml_deltree(ins_lsn_xml);
			pub_xml_deltree(ins_chl_xml);
			pub_xml_deltree(ora_lsn_xml);
			pub_xml_deltree(ora_chl_xml);
			return -1;	
		}

		node_ora_chl1 = agt_xml_search(ora_chl_xml, ".DFISBP.CHANNEL", "LISTEN", lsn_name);
		if (node_ora_chl1 == NULL)
		{
			pub_log_info("[%s][%d] 侦听%s为新增", __FILE__, __LINE__, lsn_name);
			add = 1;
		}

		if (add  == 0)
		{
			node_ora_lsn1 = agt_xml_search(ora_lsn_xml, ".DFISBP.SWLISTEN", "NAME", lsn_name);
			if (node_ora_lsn1 == NULL )
			{
				pub_log_error("[%s][%d]  查找节点[%s]失败", __FILE__, __LINE__, lsn_name);
				pub_xml_deltree(ins_lsn_xml);
				pub_xml_deltree(ins_chl_xml);
				pub_xml_deltree(ora_lsn_xml);
				pub_xml_deltree(ora_chl_xml);
				return -1;	
			}
		}

		ins_chl_xml->current = node;
		if (add == 0)
		{
			ora_chl_xml->current = node_ora_chl1;
		}
		diff |= node_cmp(vars, ins_chl_xml, ora_chl_xml, add, "STATUS");
		diff |= node_cmp(vars, ins_chl_xml, ora_chl_xml, add, "PRODUCT");

		ins_lsn_xml->current = node_ins_lsn1;
		if (add == 0)
		{
			ora_lsn_xml->current = node_ora_lsn1;
		}
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "ACTIVEMSG");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCESSER.INIT");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCESSER.DESTROY");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCESSER.EXTCEVT");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCESSER.INTEREVT");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCESSER.TIMEOUTEVT");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCESSER.TRANSCODE");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "CONMAX");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCMIN");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PROCMAX");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "TIMEOUT");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "DATA");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "PKGCFG");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "FILEDEAL");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "SVRMAP");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "MAPCFG");
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#value", buf);
		if (buf[0] != '\0')
		{
			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, "%s/cfg/common/%s", ins_path, buf);
			memset(path1, 0x0, sizeof(path1));
			snprintf(path1, sizeof(path1)-1, "%s/cfg/common/%s", ora_path, buf);
			if ((result = file_cmp(path, path1)) != 0)
			{
				if (result == -2)
				{
					pub_log_error("[%s][%d] check_lsn error MAPCFG file %s is not exist", __FILE__, __LINE__, buf);
					pub_xml_deltree(ins_lsn_xml);
					pub_xml_deltree(ins_chl_xml);
					pub_xml_deltree(ora_lsn_xml);
					pub_xml_deltree(ora_chl_xml);
					return -1;
					
				}
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTEN.SWLISTEN(%d).CFTS.CFT(%d).PKGCFT", list_i, value_j);
				memset(tmp, 0x0, sizeof(tmp));
				snprintf(tmp, sizeof(tmp)-1, "MAPCFG:%s(更新)",  buf);
				loc_set_zd_data(vars, path, tmp);
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTEN.SWLISTEN(%d).CFTS.CFT(%d).PTCFT", list_i, value_j);
				memset(tmp, 0x0, sizeof(tmp));
				snprintf(tmp, sizeof(tmp)-1, "MAPCFG:%s",  buf);
				loc_set_zd_data(vars, path, tmp);
				value_j ++;
				diff |= 1;
			}
		}
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "SVRMAPCFG");
		memset(buf, 0x0, sizeof(buf));
		loc_get_zd_data(vars, "#value", buf);
		if (buf[0] != '\0')
		{
			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, "%s/cfg/common/%s", ins_path, buf);
			memset(path1, 0x0, sizeof(path1));
			snprintf(path1, sizeof(path1)-1, "%s/cfg/common/%s", ora_path, buf);
			if ((result = file_cmp(path, path1)) != 0)
			{
				if (result == -2)
				{
					pub_log_error("[%s][%d] check_lsn error SVRMAPCFG file %s is not exist", __FILE__, __LINE__, buf);
					pub_xml_deltree(ins_lsn_xml);
					pub_xml_deltree(ins_chl_xml);
					pub_xml_deltree(ora_lsn_xml);
					pub_xml_deltree(ora_chl_xml);
					return -1;
				}

				memset(path, 0x0, sizeof(path));
				pub_log_info("[%s][%d]SVRMAPCFG:%s", __FILE__, __LINE__, buf);
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTEN.SWLISTEN(%d).CFTS.CFT(%d).PKGCFT", list_i, value_j);
				memset(tmp, 0x0, sizeof(tmp));
				snprintf(tmp, sizeof(tmp)-1, "SVRMAPCFG:%s(更新)",  buf);
				loc_set_zd_data(vars, path, tmp);
				memset(path, 0x0, sizeof(path));
				pub_log_info("[%s][%d]SVRMAPCFG:%s", __FILE__, __LINE__, buf);
				memset(path, 0x0, sizeof(path));
				snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTEN.SWLISTEN(%d).CFTS.CFT(%d).PTCFT", list_i, value_j);
				memset(tmp, 0x0, sizeof(tmp));
				snprintf(tmp, sizeof(tmp)-1, "SVRMAPCFG:%s", buf, buf);
				loc_set_zd_data(vars, path, tmp);
				value_j ++;
				diff |= 1;
			}
		}
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "COMM.GROUP.LOCAL.IP");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "COMM.GROUP.LOCAL.PORT");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "COMM.GROUP.REMOTE.IP");
		diff |= node_cmp(vars, ins_lsn_xml, ora_lsn_xml, add, "COMM.GROUP.REMOTE.PORT");

		if (diff)
		{
			if (add)
			{
			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).PKGNAME", list_i);
			memset(buf, 0x0, sizeof(buf));
				snprintf(buf, sizeof(buf)-1, "%s(新增)", lsn_name);
			loc_set_zd_data(vars, path, buf);
			}
			else
			{
			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).PKGNAME", list_i);
			memset(buf, 0x0, sizeof(buf));
				
				snprintf(buf, sizeof(buf)-1, "%s(修改)",  lsn_name);
			loc_set_zd_data(vars, path, buf);
			memset(path, 0x0, sizeof(path));
			snprintf(path, sizeof(path)-1, ".TradeRecord.Response.SWLISTENS.SWLISTEN(%d).BPNAME", list_i);
			memset(buf, 0x0, sizeof(buf));
				
				snprintf(buf, sizeof(buf)-1, "%s", lsn_name);
			loc_set_zd_data(vars, path, buf);
			}
			list_i ++;
		}

		node = node->next;
	}

	pub_xml_deltree(ins_lsn_xml);
	pub_xml_deltree(ins_chl_xml);
	pub_xml_deltree(ora_lsn_xml);
	pub_xml_deltree(ora_chl_xml);
	return 0;
}

int check_dbconf(sw_loc_vars_t *vars)
{
	int i = 0;
	int diff_nu = 0;
	int same_nu = 0;
	int result = -1;
	char install_dir[128];
	char in_prdt_path[128];
	char bp_prdt_path[128];
	char item_bp[5][256];
	char item_in[5][256];
	char buf[256];
	char path[256];
	sw_xmltree_t    *in_xml = NULL;
	sw_xmltree_t    *bp_xml = NULL;
	sw_xmlnode_t    *in_node = NULL;
	sw_xmlnode_t    *in_node1 = NULL;
	sw_xmlnode_t    *bp_node1 = NULL;
	sw_xmlnode_t    *in_node2 = NULL;
	sw_xmlnode_t    *bp_node2 = NULL;
	memset(item_in, 0x0, sizeof(item_in));
	memset(item_bp, 0x0, sizeof(item_bp));

	loc_get_zd_data(vars, ".TradeRecord.Request.InstallDir", install_dir);
	sprintf(in_prdt_path, "%s/cfg/dbconfig.xml", install_dir);

	sprintf(bp_prdt_path, "%s/cfg/dbconfig.xml", getenv("SWWORK"));

	if (strlen(install_dir) == 0)
	{    
		pub_log_error("%s, %d, No .TradeRecord.Request.InstallDir", __FILE__, __LINE__);
		return  -1;
	} 
	
	result = access(bp_prdt_path, R_OK);
	if (result != 0)
	{   
		pub_log_error("%s, %d, 平台无数据库配置文件 %s", __FILE__, __LINE__, bp_prdt_path);
		return -1;
	}
	
	result = access(in_prdt_path, R_OK);
	if (result != 0)
	{
		pub_log_error("%s, %d, 无数据库配置文件 %s", __FILE__, __LINE__,in_prdt_path);
		return 0;
	}

	in_xml = pub_xml_crtree(in_prdt_path);
	bp_xml = pub_xml_crtree(bp_prdt_path);
	in_node = pub_xml_locnode(in_xml,".DBCONFIG.DATABASE");
	while (in_node != NULL)
	{
		memset(&item_bp, 0x0, sizeof(item_bp));
		memset(&item_in, 0x0, sizeof(item_in));
		in_xml->current = in_node;
		in_node1 = pub_xml_locnode(in_xml,"DBID");
		if (in_node1 == NULL || in_node1->value == NULL || strlen(in_node1->value) == 0)
		{
			pub_log_error("[%s][%d] 安装包产品配置文件错误!", __FILE__, __LINE__);
			pub_xml_deltree(in_xml);
			pub_xml_deltree(bp_xml);
			return -1;
		
		}
		for (i = 0; i < 5;i++)
		{
			in_node2 = pub_xml_locnode(in_xml, dbconf[i]);
			if (in_node2 == NULL || in_node2->value == NULL)
			{
				pub_log_info("[%s][%d] 数据库配置文件错误", __FILE__, __LINE__);
				strcpy(item_bp[i], "");
			}
			else
			{
				strcpy(item_in[i], in_node2->value);
			}		
		}
		bp_node1 = agt_xml_search(bp_xml, ".DBCONFIG.DATABASE", "DBID", in_node1->value);
		if (bp_node1 == NULL)
		{
			pub_mem_memzero(buf, sizeof(buf));
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.DATABASE(%d).PKGDBID",same_nu);
			sprintf(buf, "(新增)%s", in_node1->value);
			loc_set_zd_data(vars, path, buf);
			in_node = in_node->next;
			for (i = 0; i < 5; i++)
			{
				pub_mem_memzero(path, sizeof(path));
				sprintf(path, ".TradeRecord.Response.DATABASE(%d).CFTS.CFT(%d).PKGCFT",same_nu, i);
				sprintf(buf,"%s:%s", dbconf[i],item_in[i]);
				loc_set_zd_data(vars, path, buf);
			}
			same_nu++;
			continue;
		}
		bp_node2 = bp_node1->firstchild;
		while(bp_node2 != NULL)
		{
			pub_log_info("[%s][%d] bp_node2_>name=%s", __FILE__, __LINE__,bp_node2->name);
			if (strcmp(bp_node2->name, "DBNAME") == 0)
			{
				strcpy(item_bp[0], bp_node2->value);
			}
			if (strcmp(bp_node2->name, "DBSID") == 0)
			{
				strcpy(item_bp[1], bp_node2->value);
			}
			if (strcmp(bp_node2->name, "DBUSER") == 0)
			{
				strcpy(item_bp[2], bp_node2->value);
			}
			if (strcmp(bp_node2->name, "DBPASSWD") == 0)
			{
				strcpy(item_bp[3], bp_node2->value);
			}
			if (strcmp(bp_node2->name, "DBTYPE") == 0)
			{
				strcpy(item_bp[4], bp_node2->value);
			}
			bp_node2 = bp_node2->next;
		
		}
		result = lsn_nodecmp(item_in, item_bp, 5, 5);
		if(result == -1)
		{
			pub_mem_memzero(buf, sizeof(buf));
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.DATABASE(%d).PKGDBID",same_nu);
			sprintf(buf, "(更新)%s", in_node1->value);
			loc_set_zd_data(vars, path, buf);
			pub_mem_memzero(buf, sizeof(buf));
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.DATABASE(%d).PTDBID",same_nu);
			sprintf(buf, "%s", in_node1->value);
			loc_set_zd_data(vars, path, buf);
			diff_nu = 0; 
			for(i = 0; i < 5; i++)
			{
				if(strcmp(item_bp[i], item_in[i] ) != 0)
				{
					pub_mem_memzero(path, sizeof(path));
					sprintf(path, ".TradeRecord.Response.DATABASE(%d).CFTS.CFT(%d).PKGCFT",same_nu, diff_nu);
					memset(buf,0x00,sizeof(buf));
					sprintf(buf,"%s:%s", dbconf[i],item_in[i]);
					loc_set_zd_data(vars, path, buf);
					pub_mem_memzero(path, sizeof(path));
					sprintf(path, ".TradeRecord.Response.DATABASE(%d).CFTS.CFT(%d).PTCFT",same_nu, diff_nu);
					memset(buf,0x00,sizeof(buf));
					sprintf(buf,"%s:%s",  dbconf[i],item_bp[i]);
					loc_set_zd_data(vars, path, buf);
					diff_nu++;
				}
			}
			same_nu++;
		}
		in_node = in_node->next;
	}
	pub_xml_deltree(in_xml);
	pub_xml_deltree(bp_xml);
	return 0;
	
}


int check_productcfg(sw_loc_vars_t *vars)
{
	int i = 0, j = 0;
	int lis_bp = 0;
	int lis_in = 0;
	int same_nu = 0;
	int result = -1;
	char in_prdt_path[128];
	char bp_prdt_path[128];
	char buf[256];
	char install_dir[128];
	char path[128];
	char in_chnl_list[512];
	char bp_list[21][256];
	char in_list[21][256];
	char file_svc_in[256];
	char file_svc_bp[256];

	sw_xmltree_t    *in_xml = NULL;
	sw_xmltree_t    *bp_xml = NULL;
	sw_xmlnode_t    *bp_node1 = NULL;
	sw_xmlnode_t    *in_node = NULL;
	sw_xmlnode_t    *in_node1 = NULL;
	sw_xmlnode_t    *in_node2 = NULL;
	sw_xmlnode_t    *bp_node2 = NULL;


	memset(in_prdt_path, 0x0, sizeof(in_prdt_path));
	memset(bp_prdt_path, 0x0, sizeof(bp_prdt_path));
	memset(install_dir, 0x0, sizeof(install_dir));

	loc_get_zd_data(vars, ".TradeRecord.Request.InstallDir", install_dir);
	sprintf(in_prdt_path, "%s/cfg/products.xml", install_dir);

	sprintf(bp_prdt_path, "%s/cfg/products.xml", getenv("SWWORK"));

	if (strlen(install_dir) == 0)
	{    
		pub_log_error("%s, %d, No .TradeRecord.Request.InstallDir", __FILE__, __LINE__);
		return  -1;
	} 
	
	result = access(in_prdt_path, R_OK);
	if(result != 0)
	{
		pub_log_error("%s, %d, 无产品配置文件 %s", __FILE__, __LINE__,in_prdt_path);
		return -1;
	}
	
	result = access(bp_prdt_path, R_OK);
	if(result != 0)
	{   
		pub_log_error("%s, %d, 无产品配置文件 %s", __FILE__, __LINE__, bp_prdt_path);
		return 0;
	}
	
	in_xml = pub_xml_crtree(in_prdt_path);
	if (in_xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml error! xmlname=[%s]", __FILE__, __LINE__, in_prdt_path);
		return -1;
	}
	bp_xml = pub_xml_crtree(bp_prdt_path);
	if (bp_xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml error! xmlname=[%s]", __FILE__, __LINE__, bp_prdt_path);
		return -1;
	}



	in_node = pub_xml_locnode(in_xml,".DFISBP.PRODUCT");
	while(in_node != NULL)
	{
		i = 0;
		j = 0;
		memset(&bp_list, 0x0, sizeof(bp_list));
		memset(&in_list, 0x0, sizeof(in_list));
		in_xml->current = in_node;
		in_node1 = pub_xml_locnode(in_xml,"NAME");
		if(in_node1 == NULL || in_node1->value == NULL || strlen(in_node1->value) == 0)
		{
			pub_log_error("[%s][%d] 安装包产品配置文件错误!", __FILE__, __LINE__);
			pub_xml_deltree(in_xml);
			pub_xml_deltree(bp_xml);
			return -1;
		
		}
		lis_in = 0;
		lis_bp = 0;
		in_node2 = pub_xml_locnode(in_xml, "CHANNEL.LISTEN");
			
		while(in_node2 != NULL)
		{
			if(in_node2->value != NULL && strlen(in_node2->value) != 0)
			{
			pub_log_info("[%s][%d] i = %d  in_list=%s", __FILE__, __LINE__, i, in_node2->value);
				if(i < 20)
				{
					strcpy(in_list[i], in_node2->value);
					i++;
				}
			}
			in_node2=in_node2->next;
		}

		bp_node1 = agt_xml_search(bp_xml, ".DFISBP.PRODUCT", "NAME", in_node1->value);
		if (bp_node1 == NULL)
		{
		pub_mem_memzero(path, sizeof(path));
		pub_mem_memzero(buf, sizeof(buf));

		sprintf(path, ".TradeRecord.Response.PRODUCT(%d).PKGNAME",same_nu);
		sprintf(buf, "(新增)%s", in_node1->value);
		loc_set_zd_data(vars, path, buf);
			in_node = in_node->next;
			memset(in_chnl_list, 0x0, sizeof(in_chnl_list));
			for(lis_in = 0; lis_in < i; lis_in++)
			{
				strcat(in_chnl_list, in_list[lis_in]);
				strcat(in_chnl_list, "|");
			}
			 pub_mem_memzero(path, sizeof(path));
			 sprintf(path, ".TradeRecord.Response.PRODUCT(%d).PKGLSN",same_nu);
			 loc_set_zd_data(vars, path, in_chnl_list);
			same_nu++;
			continue;
		}
		bp_xml->current = bp_node1;
		bp_node2 = pub_xml_locnode(bp_xml,"CHANNEL.LISTEN");
		while(bp_node2 != NULL)
		{	
			pub_log_info("[%s][%d] bo_bnode2= %s", __FILE__,__LINE__, bp_node2->name);
			if (strcmp(bp_node2->name, "LISTEN") == 0)
			{
				if(bp_node2->value != NULL && strlen(bp_node2->value) != 0)
				{
					if(j < 20)
					{
						strcpy(bp_list[j], bp_node2->value);
						j++;
					}
				}
			}
			bp_node2 = bp_node2->next;
		}
		result = lsn_nodecmp(bp_list,in_list, j, i);
		if(result == -1)
		{
		pub_mem_memzero(path, sizeof(path));
		pub_mem_memzero(buf, sizeof(buf));

		sprintf(path, ".TradeRecord.Response.PRODUCT(%d).PKGNAME",same_nu);
		sprintf(buf, "(更新)%s", in_node1->value);
		loc_set_zd_data(vars, path, buf);
		pub_mem_memzero(path, sizeof(path));
		pub_mem_memzero(buf, sizeof(buf));

		sprintf(path, ".TradeRecord.Response.PRODUCT(%d).PTNAME",same_nu);
		sprintf(buf, "%s", in_node1->value);
		loc_set_zd_data(vars, path, buf);
			pub_log_info("[%s][%d] %d, %d", __FILE__, __LINE__, j, i);
			memset(in_chnl_list, 0x0, sizeof(in_chnl_list));
			for(lis_bp = 0; lis_bp < j; lis_bp++)
			{
				pub_log_info("[%s][%d] bp_list=%s", __FILE__, __LINE__, bp_list[lis_bp]);
					strcat(in_chnl_list, bp_list[lis_bp]);
				strcat(in_chnl_list, "|");
			}
					pub_mem_memzero(path, sizeof(path));
					sprintf(path, ".TradeRecord.Response.PRODUCT(%d).PTLSN",same_nu);
					loc_set_zd_data(vars, path, in_chnl_list);
					memset(in_chnl_list, 0x0, sizeof(in_chnl_list));
			for(lis_in = 0; lis_in < i; lis_in++)
			{
				strcat(in_chnl_list, in_list[lis_in]);
				strcat(in_chnl_list,"|");
			}
			memset(file_svc_in, 0x0,sizeof(file_svc_in));
			memset(file_svc_bp, 0x0,sizeof(file_svc_bp));

			sprintf(file_svc_bp, "%s/products/etc/svrcfg/svrcfg.xml", getenv("SWWORK"));
			sprintf(file_svc_in, "%s/products/etc/svrcfg/svrcfg.xml", install_dir);
			
			result = file_cmp(file_svc_in, file_svc_bp);
			if(result == -1)
			{
				strcat(in_chnl_list, "服务有更新|");
			}
			pub_mem_memzero(path, sizeof(path));
			 sprintf(path, ".TradeRecord.Response.PRODUCT(%d).PKGLSN",same_nu);
			 loc_set_zd_data(vars, path, in_chnl_list);
			same_nu++;

		}
		in_node = in_node->next;
	}
	pub_xml_deltree(in_xml);
	pub_xml_deltree(bp_xml);
	return 0;
}

int check_validity(char *install_dir,	sw_loc_vars_t *vars)
{
	int result = -1;
	result = check_productcfg(vars);
	if(result == -1)
	{
		pub_log_info("[%s][%d] 产品更新变化检测失败", __FILE__, __LINE__);
		return -1;
	}

	result = check_dbconf(vars);
	if(result == -1)
	{
		pub_log_info("[%s][%d] 数据库配置更新变化检测失败", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}



















int sp2073(sw_loc_vars_t *vars)
{
	int	result = 0;
	char	opt[32];
	char	chk_run[32];
	char	filename[256];
	char	status[16];
	char	res_msg[256];
	char	reply[8];
	char	install_dir[256];
	char	produfile[256];

	memset(opt, 0x0, sizeof(opt));
	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	memset(status, 0x0, sizeof(status));
	memset(filename, 0x0, sizeof(filename));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);

	memset(chk_run, 0x0, sizeof(chk_run));
	loc_get_zd_data(vars, ".TradeRecord.Request.ChkRun", chk_run);

	memset(install_dir, 0x0, sizeof(install_dir));
	loc_get_zd_data(vars, ".TradeRecord.Request.InstallDir", install_dir);
	pub_log_debug("%s, %d, 开始检查配置文件冲突!", __FILE__, __LINE__);
	/*检查安装前备份是否在运行*/
	if (strcmp(opt, "CHECK_DEPLOY_CONFLICT") == 0)
	{
		result = agt_check_inst_stat(vars, opt, chk_run);
		if (result < 0)
		{
			sprintf(res_msg, "正在进行配置文件冲突检查");
			strcpy(reply, "E999");
			goto ErrExit;
		}
		else if (result > 0)
		{
			goto OkExit;
		}

		result = check_validity(install_dir, vars);
		if (result == -1)
		{
			pub_log_error("[%s][%d] install is error ", __FILE__, __LINE__);
			strcpy(reply, "E999");	
			strcpy(res_msg, "安装包合法性检测失败");	
			goto ErrExit;
		}
	}
	else if (strcmp(opt, "CHECK_LSN") == 0)
	{
			result = check_lsn(vars, install_dir);
		if (result < 0)
		{
			pub_log_error("[%s][%d]check lsn error", __FILE__, __LINE__);
			strcpy(reply, "E999");
			strcpy(res_msg, "侦听检测失败");
			goto ErrExit;
		}
		goto OkExit;
	}
	else
	{
		pub_log_error("%s, %d, Unknown opt[%s]!", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}
OkExit:

	pub_log_debug("[%s][%d] [%s]OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return SW_OK;

ErrExit:
	agt_error_info(reply, res_msg);
	pub_log_debug("[%s][%d] [%s]ERR EXIT!", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return SW_ERROR;
}
