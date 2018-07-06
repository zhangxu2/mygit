#include "svc_cycle.h"
#include "alert.h"
#include "pub_computexp.h"
#include "pub_proc.h"

#define SVC_ALERT_EXP_TIME	5 * 60
#define SVC_DEFAULT_MTYPE_TIMEOUT (5*60)

int	g_argload = 1;
time_t	g_arule_time;

extern int flow_prework(sw_char_t *flow_name);
extern int flow_work(int startline);
extern int flow_init(char *flow_name);

static int svc_create_single_svr(sw_svc_cycle_t *cycle, int index);
static int svc_print_factor(sw_loc_vars_t *locvars, sw_cmd_t *cmd, int idx, int flag);
static int svc_create_single_svc(sw_svc_cycle_t *cycle, int index, int id);
static int set_arg(sw_char_t *filename);
static int svc_check_child_status(sw_svc_cycle_t *cycle);
static int svc_send_cmd_to_child(sw_svc_cycle_t *cycle);

static int str_replace_var(char *str, int len)
{
	int	i = 0;
	int	index = 0;
	char	*ptr = str;
	char	p = '\0';
	char	q = '\0';
	char	*exp = NULL;
	char	value[1024];
	char	varname[128];

	if (str == NULL)
	{
		pub_log_info("[%s][%d] Param error, str is null.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (len <= 0)
	{
		pub_log_error("[%s][%d] Param error, size error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	exp = (char *)calloc(1, len + 1);
	if (exp == NULL)
	{
		pub_log_error("[%s][%d] calloc error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	while (*ptr != '\0')
	{
		if (*ptr == '[')
		{
			q = *ptr++;
			p = *ptr;
			if (p == '#' || p == '$')
			{
				i = 0;
				memset(varname, 0x00, sizeof(varname));
				while (*ptr != '\0' && *ptr != ']')
				{
					varname[i++] = *ptr++;
				}

				varname[i] = '\0';
				memset(value, 0x00, sizeof(value));
				get_zd_data(varname, value);
				strcat(exp, value);
				index += strlen(value);

			}
			else
			{
				exp[index++] = q;
				exp[index++] = p;
			}
		}
		else
		{
			exp[index++] = *ptr;
		}
		ptr++;
	}

	exp[index] = '\0';
	strcpy(str, exp);
	free(exp);
	exp = NULL;

	return 0;
}

static int deal_conn_name(char *name)
{
	int	len = 0;
	int	index = 0;
	char	*sep = "|";
	char	*p = name;
	char	*q = NULL;

	if (name[0] == '\0' || name == NULL)
	{
		pub_log_error("[%s][%d] input argv name is null.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	g_dbconn_cnt = 0;
	g_dbconn_curr = 0;
	while((q = strstr(p, sep)) != NULL)
	{
		memset(&g_dbconn_t[index].conn_name, 0x0, sizeof(g_dbconn_t[index].conn_name));
		len = q - p;
		strncpy(g_dbconn_t[index].conn_name, p, len);
		p += len + 1;
		index++;
	}
	if (*p != '\0')
	{
		memset(&g_dbconn_t[index].conn_name, 0x0, sizeof(g_dbconn_t[index].conn_name));
		strncpy(g_dbconn_t[index].conn_name, p, strlen(p));
		index++;
	}
	g_dbconn_cnt = index;

	return SW_OK;
}

sw_int_t svc_init(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	sw_char_t	svc_path[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;	
	sw_xmlnode_t	*node2 = NULL;	
	
	memset(svc_path, 0x0, sizeof(svc_path));
	
	sprintf(svc_path, "%s/products/%s/etc/svrcfg/svrcfg.xml", cycle->base.work_dir.data, cycle->prdt);
	xml = cfg_read_xml(svc_path);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, svc_path);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".SERVERS.ARGLOAD");
	if (node != NULL && node->value[0] == '0')
	{
		g_argload = 0;
		pub_log_info("[%s][%d] g_argload=[%d]", __FILE__, __LINE__, g_argload);
	}

	i = 0;
	node = pub_xml_locnode(xml, ".SERVERS.SERVER");
	while (node != NULL)
	{
		if (strcmp(node->name, "SERVER") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("NAME could not null!", __FILE__, __LINE__);
			pub_xml_deltree(xml)
			return SW_ERROR;
		}
		
		strncpy(cycle->svrs->svr[i].svr_name, node1->value, sizeof(cycle->svrs->svr[i].svr_name) - 1);
		
		node1 = pub_xml_locnode(xml, "MIN");
		if (node1 != NULL && node1->value != NULL)
		{
			cycle->svrs->svr[i].min = atoi(node1->value);
		}
		else
		{
			cycle->svrs->svr[i].min = MIN_PROC_CNT;
		}
		
		node1 = pub_xml_locnode(xml, "MAX");
		if (node1 != NULL && node1->value != NULL)
		{
			cycle->svrs->svr[i].max = atoi(node1->value);
		}
		else
		{
			cycle->svrs->svr[i].max = MAX_PROC_CNT;
		}
		
		node1 = pub_xml_locnode(xml, "SCANTIME");
		if (node1 != NULL && node1->value != NULL)
		{
			cycle->svrs->svr[i].scantime = atoi(node1->value);
		}

		node1 = pub_xml_locnode(xml, "RELOAD");
		if (node1 != NULL && node1->value != NULL)
		{
			cycle->svrs->svr[i].reload = atoi(node1->value);
		}
		else
		{
			cycle->svrs->svr[i].reload = SVC_RELOAD_DEFAULT;
		}
		
		node1 = pub_xml_locnode(xml, "DLCACHE");
		if (node1 != NULL && node1->value != NULL)
		{
			cycle->svrs->svr[i].use_dlcache = atoi(node1->value);
		}
		else
		{
			cycle->svrs->svr[i].use_dlcache = 0;
		}

		node1 = pub_xml_locnode(xml, "DBID");
		if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
		{
			strncpy(cycle->svrs->svr[i].db_conn_name, node1->value, sizeof(cycle->svrs->svr[i].db_conn_name) - 1);
			cycle->svrs->svr[i].use_db = 1;
		}
		else
		{
			cycle->svrs->svr[i].use_db = 0;
		}
		
		node1 = pub_xml_locnode(xml, "DBMODE");
		if (node1 != NULL && node1->value != NULL && strcmp(node1->value, "ESQL") == 0)
		{
			cycle->svrs->svr[i].db_mode = DB_MODE_ESQL;
		}
		else
		{
			cycle->svrs->svr[i].db_mode = DB_MODE_UDBC;
		}
	
		node1 = pub_xml_locnode(xml, "STATUS");
		if (node1 != NULL && node1->value != NULL)
		{
			cycle->svrs->svr[i].status = atoi(node1->value);
		}
		else
		{
			cycle->svrs->svr[i].status = SVC_STATUS_DEFAULT;
		}
		
		node1 = pub_xml_locnode(xml, "SEQSNAME");
		if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0')
		{
			strncpy(cycle->svrs->svr[i].seqs_name, node1->value, sizeof(cycle->svrs->svr[i].seqs_name) - 1);
		}
		else
		{
			if (g_workmode_mp)
			{	
				pub_log_error("[%s][%d] mp workmode must cfg SEQSNAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
			strncpy(cycle->svrs->svr[i].seqs_name, DEFAULT_TRACE_NAME, sizeof(cycle->svrs->svr[i].seqs_name) - 1);
		}

		node1 = pub_xml_locnode(xml, "USELEVEL");
		if (node1 != NULL && node1->value != NULL && node1->value[0] != '\0' && node1->value[0] == '1')
		{
			cycle->svrs->svr[i].use_svrlevel = 1;
		}
		else
		{
			cycle->svrs->svr[i].use_svrlevel = 0;
		}
		
		j = 0;
		node1 = pub_xml_locnode(xml, "SERVICE");
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "SERVICE") != 0)
			{
				node1 = node1->next;
				continue;
			}
			
			xml->current = node1;
			node2 = pub_xml_locnode(xml, "NAME");
			if (node2 == NULL || node2->value == NULL)
			{
				pub_log_error("[%s][%d] NAME could not null!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
			strncpy(cycle->svrs->svr[i].services[j].name, node2->value, sizeof(cycle->svrs->svr[i].services[j].name) - 1);
			
			node2 = pub_xml_locnode(xml, "STATUS");
			if (node2 != NULL && node2->value != NULL)
			{
				cycle->svrs->svr[i].services[j].status = atoi(node2->value);
			}
			else
			{
				cycle->svrs->svr[i].services[j].status = 1;
			}
			
			node2 = pub_xml_locnode(xml, "LIB");
			if (node2 != NULL && node2->value != NULL)
			{
				strncpy(cycle->svrs->svr[i].services[j].lib, node2->value, sizeof(cycle->svrs->svr[i].services[j].lib) - 1);
			}
			
			node2 = pub_xml_locnode(xml, "TRACE");
			if (node2 != NULL && node2->value != NULL && node2->value[0] == '1')
			{
				cycle->svrs->svr[i].services[j].trace_flag = '1';
			}
			else
			{
				cycle->svrs->svr[i].services[j].trace_flag = '0';
			}
			
			j++;
			if (j >= MAX_SVC_CNT)
			{
				pub_log_error("[%s][%d] Too many service[%d]!", __FILE__, __LINE__, j);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
			node1 = node1->next;
		}
		cycle->svrs->svr[i].service_cnt = j;
		
		i++;
		if (i >= MAX_SVR_CNT)
		{
			pub_log_error("[%s][%d] Too many server[%d] max=[%d]", 
				__FILE__, __LINE__, i, MAX_SVR_CNT);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		node = node->next;
	}
	cycle->svrs->head.svr_cnt = i;
	
	return SW_OK;
}

int svc_db_init(sw_svc_cycle_t *cycle, sw_char_t *name, sw_int32_t mode)
{
	int	ret = 0;
	sw_dbcfg_t	db;
	
	memset(&db, 0x0, sizeof(db));
	
	ret = cfg_get_db_conn((sw_cfgshm_hder_t *)cycle->base.shm_cfg, name, &db);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] svc_get_db_name [%s] error!", __FILE__, __LINE__, name);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] get db conn [%s] success! exptime=[%d]", 
		__FILE__, __LINE__, name, db.exptime);
	g_exptime = db.exptime;
	
	db.mode = mode;
	ret = pub_db_init(&db);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_db_init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("[%s][%d] pub_db_init [%s] success!", __FILE__, __LINE__, name);
	
	return SW_OK;
}

int svc_init_db_fun(sw_svc_cycle_t *cycle, int index)
{
	int 	i = 0;
	int 	j = 0;
	int	ret = 0;	
	sw_svr_t	*svr;		
	
	if (index < 0 || index >= MAX_SVR_CNT)	
	{		
		pub_log_error("[%s][%d] param error! index=[%d]", __FILE__, __LINE__, index);	
	}	
	svr = &cycle->svrs->svr[index];		
	if (svr->use_db == 0)
	{
		return SW_OK;
	}

	ret = deal_conn_name(svr->db_conn_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get single db conn error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < g_dbconn_cnt; i++)
	{
		j = (g_dbconn_cnt + g_dbconn_curr + i) % g_dbconn_cnt;
		ret = svc_db_init(cycle, g_dbconn_t[j].conn_name, svr->db_mode);
		if (ret != SW_OK)
		{
			alert_msg(ERR_DBCONN_FAILED, "紧急预警: 初始化数据库[%s]失败,请查看系统日志进行手工解决.", svr->db_conn_name);
			pub_log_error("[%s][%d] svc_db_init [%s] error!", __FILE__, __LINE__, svr->db_conn_name);
			return SW_ERROR;
		}

		ret = pub_db_open();
		if (ret == SW_OK)
		{
			g_dbconn_curr = j + 1;
			g_db_conn.conn = 1;
			g_db_conn.start_time = (long)time(NULL);
			pub_log_info("[%s][%d] Connect db [%s] success!", __FILE__, __LINE__, g_dbconn_t[j].conn_name);
			break;
		}
	}
	if (i == g_dbconn_cnt)
	{
		alert_msg(ERR_DBCONN_FAILED, "紧急预警: 连接数据库失败,请查看系统日志进行手工处理.");
		pub_log_error("[%s][%d] Connect to db error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	svr->db_connect = pub_db_open;
	svr->db_commit = pub_db_commit;
	svr->db_close = pub_db_close;
	svr->db_rollback = pub_db_rollback;
	pub_log_info("[%s][%d] init db fun success!", __FILE__, __LINE__);
	
	return SW_OK;
}

int svc_init_cmp_info(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	hashid = 0;
	sw_char_t	*ptr = NULL;
	sw_char_t	name[64];
	sw_char_t	value[64];
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(g_cmp_info, 0x0, sizeof(g_cmp_info));
	
	sprintf(xmlname, "%s/etc/.compensate.xml", cycle->base.home_dir.data);
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xml tree error! xmlname=[%s]", 
			__FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".COMPENSATE.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] NAME could not null!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		strncpy(name, node1->value, sizeof(name) - 1);
		
		node1 = pub_xml_locnode(xml, "VALUE");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] VALUE could not null!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		strncpy(value, node1->value, sizeof(value) - 1);
		
		hashid = 0;
		ptr = name;
		while (*ptr != '\0')
		{
			hashid += (unsigned char)*ptr;
			ptr++;
		}
		
		i = 0;
		hashid = hashid % MAX_CMP_LEVEL;
		while (g_cmp_info[hashid].err_name[0] != '\0')
		{
			i++;
			hashid++;
			if (hashid == MAX_CMP_LEVEL)
			{
				hashid = 0;
			}
	
			if (i == MAX_CMP_LEVEL)
			{
				pub_log_error("[%s][%d] Have no enough memeory!",
					__FILE__, __LINE__);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
		}
		
		strncpy(g_cmp_info[hashid].err_name, name, sizeof(g_cmp_info[hashid].err_name) - 1);
		strncpy(g_cmp_info[hashid].err_code, value, sizeof(g_cmp_info[hashid].err_code) - 1);
		node1 = pub_xml_locnode(xml, "UP");
		if (node1 != NULL || node1->value != NULL)
		{
			strncpy(g_cmp_info[hashid].up_name, node1->value, sizeof(g_cmp_info[hashid].up_name) - 1);
		}
		node = node->next;
	}
	pub_xml_deltree(xml);
	pub_log_info("[%s][%d] Init cmp info success!", __FILE__, __LINE__);

	return SW_OK;
}

int svc_init_com_param(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;
	int	cols = 0;
	int	gidx = 0;
	int	rows = 0;
	char	sql[256];
	char	*ptr = NULL;
	char	desc[128];
	char	name[128];
	char	arg_name[128];
	char	arg_value[128];
	char	arg_ctgy[128];
	char	arg_bus[128];
	
	if (g_argload == 0)
	{
		pub_log_info("[%s][%d] ARGLOAD=0 不加载参数!", __FILE__, __LINE__);
		return 0;
	}

#if defined __PRDT_ARG__
	prdt_arg_t	arg;

	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		if (cycle->svrs->svr[i].use_db == 1)
		{
			break;
		}
	}
	if (i == cycle->svrs->head.svr_cnt)
	{
		pub_log_info("[%s][%d] 未配置数据库,不进行产品参数加载!", __FILE__, __LINE__);
		return SW_OK;
	}
	pub_log_info("[%s][%d] DBID=[%s]", __FILE__, __LINE__, cycle->svrs->svr[i].db_conn_name);
	
	memset(sql, 0x0, sizeof(sql));
	memset(desc, 0x0, sizeof(desc));
	memset(name, 0x0, sizeof(name));
	memset(arg_name, 0x0, sizeof(arg_name));
	memset(arg_value, 0x0, sizeof(arg_value));
	memset(arg_ctgy, 0x0, sizeof(arg_ctgy));
	memset(arg_bus, 0x0, sizeof(arg_bus));
	ret = deal_conn_name(cycle->svrs->svr[i].db_conn_name);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get single db conn error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	for (i = 0; i < g_dbconn_cnt; i++)
	{
		j = (g_dbconn_cnt + g_dbconn_curr + i) % g_dbconn_cnt;
		ret = svc_db_init(cycle, g_dbconn_t[j].conn_name, DB_MODE_UDBC);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] DB init error! DBID=[%s]", 
				__FILE__, __LINE__, g_dbconn_t[i].conn_name);
			return SW_ERROR;
		}
		
		ret = pub_db_open();
		if (ret == SW_OK)
		{
			g_dbconn_curr = j + 1;
			break;
		}
	}
	if (i == g_dbconn_cnt)
	{
		pub_log_error("[%s][%d] Connect to db error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select prdt_ctgy from app_products where prdt_id = '%s'", cycle->prdt);
	ret = pub_db_squery(sql);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 取产品信息失败! sql=[%s]", __FILE__, __LINE__, sql);
		pub_db_close();
		return SW_ERROR;
	}
	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] 产品信息[%s]不存在!", __FILE__, __LINE__, cycle->prdt);
		pub_db_close();
		return SW_ERROR;
	}
	pub_str_rtrim(ptr);
	pub_log_info("[%s][%d] prdt_ctdy=[%s]", __FILE__, __LINE__, ptr);
	
	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select arg_name, arg_value, arg_ctgy, arg_bus from app_params where arg_ctgy = '%s' order by arg_name", ptr);
	pub_log_info("[%s][%d] SQL=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_mquery("app_params", sql, 500);
	if (cols < 0)
	{
		pub_db_close();
		pub_log_error("[%s][%d] pub_db_mquery error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	sprintf(desc, "%s params", cycle->prdt);
	gidx = prdt_arg_galloc(cycle->prdt, 1, 0, desc);
	if (gidx < 0)
	{
		pub_db_close();
		pub_log_error("[%s][%d] Alloc group faild!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	while (1)
	{
		rows = pub_db_mfetch("app_params");
		if (rows == 0)
		{
			break;
		}
		else if (rows == -1)
		{
			pub_db_close();
			pub_log_error("[%s][%d] mfetch error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		for (i = 0; i < rows; i++)
		{
			memset(name, 0x0, sizeof(name));
			memset(arg_name, 0x0, sizeof(arg_name));
			memset(arg_value, 0x0, sizeof(arg_value));
			memset(arg_ctgy, 0x0, sizeof(arg_ctgy));
			memset(arg_bus, 0x0, sizeof(arg_bus));
			for (j = 0; j < cols; j++)
			{
				memset(name, 0x0, sizeof(name));
				ptr = pub_db_get_data_and_name("app_params", i + 1, j + 1, name, sizeof(name));
				pub_str_rtrim(ptr);
				if (strcmp(name, "ARG_NAME") == 0)
				{
					strncpy(arg_name, ptr, sizeof(arg_name) - 1);
				}

				if (strcmp(name, "ARG_VALUE") == 0)
				{
					strncpy(arg_value, ptr, sizeof(arg_value) - 1);
				}
				
				if (strcmp(name, "ARG_CTGY") == 0)
				{
					strncpy(arg_ctgy, ptr, sizeof(arg_ctgy) - 1);
				}
			
				if (strcmp(name, "ARG_BUS") == 0)
				{
					strncpy(arg_bus, ptr, sizeof(arg_bus) - 1);
				}
			}
			memset(&arg, 0x0, sizeof(arg));
			strncpy(arg.name, arg_name, sizeof(arg.name) - 1);
			strncpy(arg.value, arg_value, sizeof(arg.value) - 1);
			memset(name, 0x0, sizeof(name));
			sprintf(name, "%s_%s_%s", arg.name, arg_ctgy, arg_bus);
			ret = prdt_arg_set(name, &arg, sizeof(arg));
			if (ret != 0)
			{
				pub_db_close();
				pub_log_error("[%s][%d] set prdt arg error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] name=[%s] arg_name=[%s] arg_value=[%s]",
				__FILE__, __LINE__, name, arg_name, arg_value);
		}
	}
	pub_db_close();
	pub_log_info("[%s][%d] Init com param success!", __FILE__, __LINE__);
#endif

	return SW_OK;
}

int svc_backup_pkg(sw_cmd_t *cmd)
{
	return SW_OK;
}

int svc_preanalyze_cmd(sw_cmd_t *cmd)
{
	pub_log_info("[%s][%d] cmd->type=[%d]", __FILE__, __LINE__, cmd->type);
	switch (cmd->type)
	{
		case	SW_TASKREQ:
			set_zd_data("$errcode", "0000");
		case	SW_CALLREQ:
		case	SW_POSTREQ:
		case	SW_LINKREQ:
		case	SW_LINKLSNRES:
		case	SW_POSTLSNRES:
		case	SW_CALLLSNSAFREQ:
			set_zd_data("$errcode", "0000");
			break;
		case	SW_CALLLSNSAFRES:
			break;
		case	SW_CALLRES:
			break;
		case	SW_CALLLSNRES:
			break;
		case	SW_ERRCMD:
			return 1;
		case	SW_TASKERR:
			pub_log_error("[%s][%d] recv error deal cmd, to backup pkg info", __FILE__, __LINE__);
			svc_backup_pkg(cmd);
			mtype_delete(cmd->mtype, 1);
		default:
			pub_log_error("[%s][%d] type=[%d]", __FILE__, __LINE__, cmd->type);
			cmd->type = SW_ERRCMD;
			return SW_ERROR;
	}
	
	return SW_OK;
}

int svc_init_sys(sw_cmd_t *cmd)
{
	int	i = 0;
	int	ret = 0;
	sw_char_t	buf[32];
	sw_char_t	date[32];
	
	memset(buf, 0x0, sizeof(buf));
	memset(date, 0x0, sizeof(date));
	
	ret = seqs_get_sysdate(date);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] seqs_get_sysdate error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	date[8] = '\0';
	set_zd_data("$date", date);

	memset(date, 0x0, sizeof(date));
	pub_time_app_get_time(NULL, 0, date);
	date[4 + 10] = '\0';
	set_zd_data("$time", date + 8);
	
	memset(buf, 0x0, sizeof(buf));
	ret = seqs_new_business_no(SYS_TRACE_NAME, 1, buf);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] 生成平台流水失败!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	set_zd_data("$sys_trace_no", buf);
	pub_log_info("[%s][%d] sys_trace_no=[%s]", __FILE__, __LINE__, buf);

	for (i = 0; i < g_svr->service_cnt; i++)                                                                          
	{
		if (strcmp(g_svr->services[i].name, cmd->def_name) == 0)
		{
			break;
		}
	}
	if (i == g_svr->service_cnt)
	{
		pub_log_info("[%s][%d] Service [%s] not found!", 
			__FILE__, __LINE__, cmd->def_name);
		return SW_ERROR;
	}
	
	if (g_svr->services[i].trace_flag != '1')
	{
		pub_log_info("[%s][%d] 该交易不产生流水! service=[%s]", 
			__FILE__, __LINE__, cmd->def_name);
		return SW_OK;
	}
	
	memset(buf, 0x0, sizeof(buf));
	ret = seqs_new_business_no(g_svr->seqs_name, 1, buf);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] seqs_new_business error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	set_zd_data("$trace_no", buf);
	pub_log_info("[%s][%d] seqs_name=[%s] trace_no=[%s]", 
		__FILE__, __LINE__, g_svr->seqs_name, buf);
	
	return SW_OK;
}

int svc_run_flow(sw_svc_cycle_t *cycle, sw_cmd_t *cmd)
{
	int	i = 0;
	int	j = 0; 
	int	ret = 0;
	int	errflag = 0;
	int	startline = 0;
	char	tmp[128];
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(&g_cmd, 0x0, sizeof(g_cmd));
	memcpy(&g_cmd, cmd, sizeof(g_cmd));
	g_cmd.msg_type = SW_MSG_RES;
	g_cmd.task_flg = SW_FORGET;
	g_cmd.dst_type = ND_LSN;
	if (cmd->type == SW_TASKREQ || cmd->type == SW_CALLLSNRES || cmd->type == SW_CALLRES)
	{
		g_cmd.type = SW_TASKRES;
	}
	else if (cmd->type == SW_CALLREQ)
	{
		g_cmd.type = SW_CALLRES;
	}
	
	if (cmd->mtype <= 0)
	{
		pub_log_error("[%s][%d] mtype error! mtype=[%ld]", __FILE__, __LINE__, cmd->mtype);
		return SW_ERROR;
	}
	
	startline = cmd->start_line;
	if (startline < 0)
	{
		pub_log_error("[%s][%d] param error! startline=[%d]", __FILE__, __LINE__, startline);
		return SW_ERROR;
	}
	
	if (cmd->type == SW_TASKREQ || cmd->type == SW_POSTREQ)
	{
		ret = svc_init_sys(cmd);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] svc_init_sys error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] svc_init_sys success!", __FILE__, __LINE__);
	}
	
	if (cmd->type == SW_TASKREQ || cmd->type == SW_POSTREQ || cmd->type == SW_CALLREQ)
	{
		startline = 0;
	}
	
	memset(g_svc, 0x0, sizeof(g_svc));
	for (i = 0; i < g_svr->service_cnt; i++)
	{
		if (strcmp(g_svr->services[i].name, cmd->def_name) != 0)
		{
			continue;
		}

		if (strlen(g_svr->services[i].lib) > 0)
		{
			strncpy(g_svc, g_svr->services[i].lib, sizeof(g_svc) - 1);
			pub_log_info("[%s][%d] g_svc=[%s] lib=[%s]", __FILE__, __LINE__, g_svc, g_svr->services[i].lib);
		}
	}
	ret = flow_prework(cmd->def_name);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] flow_prework error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (g_svr->use_db == 1)
	{
		if (g_db_conn.conn != 1)
		{
			pub_log_info("[%s][%d] connect db begin...", __FILE__, __LINE__);
			ret = pub_db_open();
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] connect to db error! ret=[%d]", __FILE__, __LINE__, ret);
				return SW_ERROR;
			}
			g_db_conn.conn = 1;
			g_db_conn.start_time = (long)time(NULL);
			pub_log_info("[%s][%d] connect db success!", __FILE__, __LINE__);
		}

		ret = pub_db_conn_detect();
		if (ret <= 0)
		{
			pub_log_error("[%s][%d] Detect db error!", __FILE__, __LINE__);
			for (i = 0; i < g_dbconn_cnt; i++)
			{
				j = (g_dbconn_cnt + g_dbconn_curr + i) % g_dbconn_cnt;
				ret = svc_db_init(cycle, g_dbconn_t[j].conn_name, g_svr->db_mode);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] svc_db_init [%s] error!", __FILE__, __LINE__, g_dbconn_t[j].conn_name);
					return SW_ERROR;
				}

				ret = pub_db_open();
				if (ret == SW_OK)
				{
					g_dbconn_curr = j + 1;
					g_db_conn.conn = 1;
					g_db_conn.start_time = (long)time(NULL);
					pub_log_info("[%s][%d] manster process connect db success!", __FILE__, __LINE__);
					break;
				}
			}
			if (i == g_dbconn_cnt)
			{
				pub_log_error("[%s][%d] Connect to db error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
		}
	}

	ulog_init(cmd->trace_no, g_prdt, cmd->dst_svr, cmd->def_name, g_idx);
	svc_print_factor(pub_get_global_vars(), cmd, g_idx, 0); 
	errflag = 0;
	ret = flow_work(startline);
	if (ret != SW_OK)
	{
		memset(tmp, 0x0, sizeof(tmp));
		get_zd_data("$errcode", tmp);
		if (tmp[0] == '\0' || strcmp(tmp, "0000") == 0)
		{
			set_zd_data("$errcode", "0010");
		}
		errflag = 1;
		pub_log_error("[%s][%d] flow [%s] exec error!", __FILE__, __LINE__, cmd->def_name);
	}
	
	if (errflag == 1)
	{
		if (g_svr->use_db == 1)
		{
			pub_db_rollback();
			pub_db_del_all_conn();
		}
		return SW_ERROR;
	}
	
	if (g_svr->use_db == 1)
	{
		pub_db_commit();
		pub_db_del_all_conn();
	}
	pub_log_info("[%s][%d] flow [%s] exec success!", __FILE__, __LINE__, cmd->def_name);
	
	return SW_OK;
}

sw_int_t svc_trace_reg(sw_cmd_t *cmd, int flag)
{
	sw_int32_t	index = 0;
	sw_char_t	errcode[16];
	sw_char_t	tx_code[32];
	
	memset(errcode, 0x0, sizeof(errcode));
	get_zd_data("$errcode", errcode);
	pub_log_info("[%s][%d] cmd.type=[%d] cmd.mtype=[%ld] errcode=[%s]", 
		__FILE__, __LINE__, cmd->type, cmd->mtype, errcode);

	index = cmd->mtype - 1;
	if (flag == 0)
	{
		if (cmd->type == SW_TASKREQ)
		{
			memset(&g_trace->trace[index], 0x0, sizeof(sw_svc_trace_item_t));
			g_trace->trace[index].mtype = cmd->mtype;
			g_trace->trace[index].trace_no = cmd->trace_no;
			g_trace->trace[index].start_time = pub_time_get_current();
			g_trace->trace[index].status = SW_SVC_DOING;
			g_trace->head.curr_cnt++;
			g_trace->head.total_cnt++;

			memset(tx_code, 0x0, sizeof(tx_code));
			get_zd_data("$tx_code", tx_code);
			strncpy(g_trace->trace[index].tx_code, tx_code, sizeof(g_trace->trace[index].tx_code) - 1);
			strncpy(g_trace->trace[index].lsn_name, cmd->lsn_name, sizeof(g_trace->trace[index].lsn_name) - 1);
			strncpy(g_trace->trace[index].prdt, cmd->dst_prdt, sizeof(g_trace->trace[index].prdt) - 1);
			strncpy(g_trace->trace[index].svr, cmd->dst_svr, sizeof(g_trace->trace[index].svr) - 1);
			strncpy(g_trace->trace[index].svc, cmd->dst_svc, sizeof(g_trace->trace[index].svc) - 1);
		}

		if (cmd->type == SW_CALLREQ)
		{
			g_trace->trace[index].mtype = cmd->mtype;
			g_trace->trace[index].trace_no = cmd->trace_no;
			g_trace->trace[index].start_time = pub_time_get_current();
			g_trace->trace[index].status = SW_SVC_DOING;
			g_trace->trace[index].req_cnt++;
		}
		
		if (cmd->type == SW_CALLLSNRES || cmd->type == SW_CALLRES)
		{
			if (g_trace->trace[index].errcode == 0)
			{
				g_trace->trace[index].errcode = atoi(errcode);
			}
			g_trace->trace[index].req_cnt--;
		}
		
	}
	else
	{
		if (cmd->type == SW_TASKRES)
		{
			g_trace->head.curr_cnt--;
			g_trace->trace[index].end_time = pub_time_get_current();
			g_trace->trace[index].status = SW_SVC_DONE;
			if (g_trace->trace[index].errcode == 0)
			{
				g_trace->trace[index].errcode = atoi(errcode);
			}
		}
		else if (cmd->type == SW_CALLRES)
		{
			g_trace->trace[index].req_cnt--;
		}
		else if (cmd->type == SW_CALLLSNREQ || cmd->type == SW_CALLREQ)
		{
			g_trace->trace[index].req_cnt++;
		}
	}

	return SW_OK;
}

sw_int_t svc_trace_print(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	
	pub_log_debug("[%s][%d] Total:[%d] Current:[%d]",
		__FILE__, __LINE__, g_trace->head.total_cnt, g_trace->head.curr_cnt);
	for (i = 0; i < cycle->list_size; i++)
	{
		if (g_trace->trace[i].status == SW_SVC_DOING || g_trace->trace[i].req_cnt > 0)
		{
			pub_log_debug("[%s][%d] mtype=[%d] trace_no=[%lld] Doing...", 
				__FILE__, __LINE__, g_trace->trace[i].mtype, g_trace->trace[i].trace_no);
		}
	}
	
	return SW_OK;
}

typedef struct
{
	int	type;
	char	zname[512];
	char	ename[512];
}alert_fmt_node_t;

alert_fmt_node_t afmt[] = {
	{1, "大于", ">"},
	{1, "小于", "<"},
	{1, "等于", "=="},
	{1, "不等于", "!="},
	{1, "大于等于", ">="},
	{1, "小于等于", "<="},
	{1, "非", "!"},
	{1, "且", "&&"},
	{1, "并且", "&&"},
	{1, "或", "||"},
	{1, "或者", "||"},
	{1, "加", "+"},
	{1, "减", "-"},
	{1, "乘", "*"},
	{1, "除", "/"},
	{0, "\0", "\0"}
};

static int init_logic_fmt()
{
	alert_fmt_node_t	*ptr = afmt;
	alert_fmt_node_t	*node = NULL;
	
	while (ptr->type != 0 && ptr->zname[0] != '\0' && ptr->ename[0] != '\0')
	{
		node = (alert_fmt_node_t *)calloc(1, sizeof(alert_fmt_node_t));
		if (node == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		strncpy(node->zname, ptr->zname, sizeof(node->zname) - 1);
		strncpy(node->ename, ptr->ename, sizeof(node->ename) - 1);
		node->type = ptr->type;
		pub_stack_push(g_afmt_stack, node);
		
		ptr++;
	}
	
	return 0;
}

static int get_afmt_parm(char *buf, alert_fmt_node_t *node)
{
	int     i = 0;
	char    out[5][512];
	char    *ptr = NULL;
	char    *tmp = NULL;
	const char      *sep = "|";

	memset(out, 0x0, sizeof(out));
	ptr = buf;
	while (*ptr != '\0')
	{
		tmp = strstr(ptr, sep);
		if (tmp == NULL)
		{
			strcpy(out[i], ptr);
			i++;
			break;
		}
		strncpy(out[i], ptr, tmp - ptr);
		ptr = tmp + strlen(sep);                                                                                           
		i++;                                                                                                               
	}                                                                                                                          
	strncpy(node->zname, out[0], sizeof(node->zname) - 1);
	strncpy(node->ename, out[1], sizeof(node->ename) - 1);

	return 0;
}

static void afmt_stack_sort(sqstack_t *st)
{
	if (st && !st->sorted)
	{
		int     i = 0;
		int     j = 0;
		int     exchange = 1;
		int     num = st->num;
		void    *tmp = NULL;
		alert_fmt_node_t  *node1 = NULL;
		alert_fmt_node_t  *node2 = NULL;

		for (i = 0; i < num - 1 && exchange; i++)
		{
			exchange = 0;
			for (j = 0; j < num - i - 1; j++)
			{
				node1 = (alert_fmt_node_t *)pub_stack_value(st, j);
				node2 = (alert_fmt_node_t *)pub_stack_value(st, j + 1);
				if (strcmp(node1->zname, node2->zname) < 0)
				{
					tmp = st->data[j];
					st->data[j] = st->data[j + 1];
					st->data[j + 1] = tmp;
					exchange = 1;
				}
			}
		}
		st->sorted = 1;                                                                                                    
	}
}

static int svc_init_alert_fmt()
{
	int	len = 0;
	FILE	*fp = NULL;
	char	line[1024];
	char	filename[128];
	alert_fmt_node_t	*node = NULL;
	
	memset(line, 0x0, sizeof(line));
	memset(filename, 0x0, sizeof(filename));
	
	sprintf(filename, "%s/products/%s/etc/alert_fmt.cfg", getenv("SWWORK"), g_prdt);
	if (access(filename, F_OK) < 0)
	{
		return 0;
	}
	
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	
	g_afmt_stack = pub_stack_new(NULL, NULL);
	if (g_afmt_stack == NULL)
	{
		pub_log_error("[%s][%d] pub_stack_new error!", __FILE__, __LINE__);
		fclose(fp);
		return -1;
	}
	
	while (1)
	{
		memset(line, 0x0, sizeof(line));
		if (fgets(line, sizeof(line), fp) == NULL)
		{
			break;
		}
		
		len = strlen(line);
		if (line[len - 1] == '\n')
		{
			line[len - 1] = '\0';
		}
		len = strlen(line);
		if (line[len - 1] == '\r')
		{
			line[len - 1] = '\0';
		}
		pub_log_info("[%s][%d] line=[%s]", __FILE__, __LINE__, line);
		if (strncmp(line, "EXP:", 4) != 0 && strncmp(line, "KEY:", 4) != 0)
		{
			continue;
		}
		
		node = (alert_fmt_node_t *)calloc(1, sizeof(alert_fmt_node_t));
		if (node == NULL)
		{
			pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			fclose(fp);
			return -1;
		}
		
		get_afmt_parm(line + 4, node);
		pub_stack_push(g_afmt_stack, node);	
	}
	if (init_logic_fmt() < 0)
	{
		pub_log_error("[%s][%d] init logic fmt error!", __FILE__, __LINE__);
		return -1;
	}
	afmt_stack_sort(g_afmt_stack);
	
	return 0;
}

int replace_key(char *exp, char *aexp)
{
	int	i = 0;
	int	num = 0;
	char	*ap = aexp;
	char	*ptr = exp;
	alert_fmt_node_t	*node = NULL;

	if (exp == NULL || aexp == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	while (*ptr != '\0')
	{
		if (!(*ptr & 0x80))
		{
			*ap++ = *ptr++;
			continue;
		}
		
		num = pub_stack_num(g_afmt_stack);
		for (i = 0; i < num; i++)
		{
			node = (alert_fmt_node_t *)pub_stack_value(g_afmt_stack, i);
			if (strncmp(ptr, node->zname, strlen(node->zname)) == 0)
			{
				strncpy(ap, node->ename, strlen(node->ename));
				ap += strlen(node->ename);
				ptr += strlen(node->zname);
				break;
			}
		}
		if (i == num)
		{
			pub_log_error("[%s][%d] Unknown express [%s]!", __FILE__, __LINE__, ptr);
			return -1;
		}
	}
	*ap = '\0';
	
	return 0;
}

static sw_int_t svc_compute_exp(sw_loc_vars_t *locvar, char *exp, char *value)
{
	int	ret = 0;
	char	aexp[1024];
	static int	first = 1;
	
	if (locvar == NULL || exp == NULL || value == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	if (first == 1)
	{
		ret = svc_init_alert_fmt();
		if (ret < 0)
		{
			pub_log_error("[%s][%d] init alert fmt error!", __FILE__, __LINE__);
			return -1;
		}
		first = 0;
		pub_log_info("[%s][%d] init alert fmt success!", __FILE__, __LINE__);
	}
	
	memset(aexp, 0x0, sizeof(aexp));
	pub_log_info("[%s][%d] exp=[%s]", __FILE__, __LINE__, exp);
	replace_key(exp, aexp);
	pub_log_info("[%s][%d] aexp=[%s]", __FILE__, __LINE__, aexp);
	
	ret = compute_exp(locvar, aexp, value);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Compute exp[%s] error!", __FILE__, __LINE__, aexp);
		return -1;
	}
	
	return 0;
}

static int svc_print_alert_msg(sw_cmd_t cmd, sw_int64_t alertno)
{
	int	fd = 0;
	int	ret = 0;
	char	dir[128];
	char	date[32];
	char	lsn[64];
	char	code[16];
	char	time_buf[32];
	char	line[1024];
	char	filename[128];
	char	txcode[64];
	memset(dir, 0x0, sizeof(dir));
	memset(date, 0x0, sizeof(date));
	memset(line, 0x0, sizeof(line));
	memset(filename, 0x0, sizeof(filename));
	pub_time_getdate(date, 1);
	sprintf(dir, "%s/tmp/monitor/%s", getenv("SWWORK"), date);
	ret = access(dir, W_OK);
	if (ret < 0)
	{
		if (errno == ENOENT)
		{
			ret = pub_file_check_dir(dir);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] mkdir [%s] error! errno=[%d]:[%s]",
					__FILE__, __LINE__, dir, errno, strerror(errno));
				return SW_ERROR;
			}
		}
		else
		{
			pub_log_error("[%s][%d] access [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, dir, errno, strerror(errno));
			return SW_ERROR;
		}
	}
	sprintf(filename, "%s/mon_error.log", dir);
	fd = open(filename, O_WRONLY | O_CREAT, 0777);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}
	pub_lock_fd(fd);
	lseek(fd, 0, SEEK_END);
	memset(lsn, 0x00, sizeof(lsn));
	memset(code, 0x00, sizeof(code));
	memset(line, 0x0, sizeof(line));
	memset(time_buf, 0x00, sizeof(time_buf));
	memset(txcode, 0x0, sizeof(txcode));
	get_zd_data("$current_lsn", lsn);
	get_zd_data("$errcode", code);
	get_zd_data("#txcode", txcode);
	pub_time_getdate(time_buf, 2);
	sprintf(line, "预警编号:[%lld] 发起渠道:[%s] 交易码:[%s] 平台流水:[%lld] 错误码:[%s] MTYPE:[%ld] 交易时间:[%s] 交易日期:[%s] 产品名称:[%s] SVR:[%s] SVC:[%s]", 
			alertno, lsn, txcode, cmd.trace_no, code, cmd.mtype, time_buf, date, cmd.dst_prdt, cmd.dst_svr, cmd.dst_svc);
	pub_log_info("[%s]", line);
	strcat(line, "\n");
	write(fd, line, strlen(line));
	close(fd);
	return SW_OK;
}
sw_int_t svc_alert_rule(sw_cmd_t cmd)
{
	int	ret = 0;
	int	level = 0;
	char	msg[2048];
	char	value[128];
	char	xmlname[128];
	static int first = 1;
	sw_int64_t	cur_time = 0ll;
	struct stat 	filestat;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	pub_log_info("[%s][%d] cmd.type=[%d]", __FILE__, __LINE__, cmd.type);
	if (cmd.type != SW_TASKRES)
	{
		return 0;
	}
	
	memset(value, 0x0, sizeof(value));
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/alert_rule.xml", getenv("SWWORK"), g_prdt);
	if (access(xmlname, F_OK) < 0)
	{
		return 0;
	}
	memset(&filestat, 0x00, sizeof(filestat));
	if (stat(xmlname, &filestat) < 0)
	{
		pub_log_error("[%s][%d] get file stat error.", __FILE__, __LINE__);
		return -1;
	}
	if (first == 0)
	{
		if (g_arule_time != filestat.st_mtime)
		{
			first = 1;
			pub_xml_deltree(g_arule_xml);
			pub_log_info("[%s][%d] file already modify,need recover load.", __FILE__, __LINE__);
		}
	}
	
	if (first == 1)
	{
		g_arule_xml = pub_xml_crtree(xmlname);
		if (g_arule_xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, xmlname);
			return -1;
		}

		first = 0;
		g_arule_time = filestat.st_mtime;
		pub_log_info("[%s][%d] Create alert rule tree success!", __FILE__, __LINE__);
	}
	xml = g_arule_xml;

	node = pub_xml_locnode(xml, ".ALERT.BUSINESS.ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") != 0)
		{
			node = node->next;
			continue;
		}
		xml->current = node;
	
		node1 = pub_xml_locnode(xml, "EXP");
		if (node1 == NULL || node1->value == NULL)
		{
			node = node->next;
			continue;
		}
		
		memset(value, 0x0, sizeof(value));
		ret = svc_compute_exp(pub_get_global_vars(), node1->value, value);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] compute [%s] error!", __FILE__, __LINE__, node1->value);
			node = node->next;
			continue;
		}
		if (strcmp(value, "1") == 0)
		{
			node1 = pub_xml_locnode(xml, "MSG");
			if (node1 == NULL || node1->value == NULL)
			{
				pub_log_error("[%s][%d] Not config MSG!", __FILE__, __LINE__);
				node = node->next;
				continue;
			}
			memset(msg, 0x00, sizeof(msg));
			memset(value, 0x00, sizeof(value));
			sprintf(value, "|%s|%lld|", cmd.sys_date, cmd.trace_no);
			strncpy(msg + strlen(value), node1->value, strlen(node1->value));	
			ret = str_replace_var(msg + strlen(value), sizeof(msg));
			if (ret < 0)
			{
				pub_log_error("[%s][%d] replace var to str error.", __FILE__, __LINE__);
				return SW_ERROR;
			}

			memcpy(msg, value, strlen(value));
			cur_time = pub_time_get_current();
			pub_log_debug("[%s][%d] msg========[%s]", __FILE__, __LINE__, msg);
			node2 = pub_xml_locnode(xml, "ALERTNO");
			if (node2 == NULL || node2->value == NULL)
			{
				level = ERR_DEFAULT_SVC_OPR;
				alert_msg(level, "[MSGID:%lld][PRDT:%s] 业务用户[%s]产品[%s]%s", cur_time, g_prdt, getenv("USER"), g_prdt, msg);
			}
			else
			{
				level = atoi(node2->value);
				alert_msg(level, "[MSGID:%lld][PRDT:%s]%s", cur_time, g_prdt, msg);	
			}
			svc_print_alert_msg(cmd, cur_time);
			pub_log_info("[%s][%d] PRDT:[%s] 业务用户[%s]产品[%s][%s] level=[%d] msg=[%s]",
				__FILE__, __LINE__, g_prdt, getenv("USER"), g_prdt, node1->value, level, msg);
		}
		
		node = node->next;
	}
	
	return SW_OK;
}

sw_int_t svc_deal_task(sw_svc_cycle_t *cycle)
{
	int	len = 0;
	int	ret = 0;
	long	mtype = 0;
	char	tmp[128];
	sw_cmd_t	cmd;
	sw_int32_t	timeout = 0;
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(&cmd, 0x0, sizeof(cmd));
	
	pub_log_debug("[%s][%d] Deal task request begin...", __FILE__, __LINE__);
	ret = msg_trans_rcv(cycle->cmd_fd, (char *)&cmd, &mtype, &len);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] msg_trans_rcv error! ret=[%d]", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}
	cmd_print(&cmd);
	
	ret = pub_vars_alloc(SHM_VARS);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_vars_alloc error!", __FILE__, __LINE__, ret);
		return SW_ERROR;
	}

	sprintf(tmp, "shm%08ld", cmd.mtype);
	ret = pub_vars_unserialize(tmp);
	if (ret != SW_OK)
	{
		pub_vars_free();
		pub_log_error("[%s][%d] pub_vars_unserialize error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	trace_insert(pub_get_global_vars(), &cmd, TRACE_IN);
	svc_trace_reg(&cmd, 0);
	
	timeout = cmd.timeout > 0 ? cmd.timeout : SVC_DEFAULT_MTYPE_TIMEOUT;
	if (cmd.type == SW_TASKREQ && mtype_check_timeout(cmd.mtype, timeout) == TASK_TIMEOUT)
	{
		pub_log_error("[%s][%d] ERROR:MTYPE=[%d] traceno=[%ld] TIMEOUT!",
			__FILE__, __LINE__, cmd.timeout, cmd.trace_no);
		cmd.type = SW_TASKRES;
		cmd.msg_type = SW_MSG_RES;
		cmd.task_flg = SW_FORGET;
		cmd.dst_type = ND_LSN;
		ret = route_snd_dst(pub_get_global_vars(), cycle->base.global_path, &cmd);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] send cmd error!", __FILE__, __LINE__);
		}
		pub_vars_free();
		return SW_OK;
	}

	ret = svc_preanalyze_cmd(&cmd);
	if (ret == SW_ERROR)
	{
		pub_log_error("[%s][%d] task type [%d] error!", __FILE__, __LINE__, cmd.type);
		cmd.type = SW_ERRCMD;
		cmd.msg_type = SW_MSG_RES;
		cmd.task_flg = SW_FORGET;
		ret = route_snd_dst(pub_get_global_vars(), cycle->base.global_path, &cmd);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] send cmd error!", __FILE__, __LINE__);
		}
		pub_vars_free();
		return SW_ERROR;
	}
	else if (ret == 1)
	{
		pub_vars_free();
		pub_log_error("[%s][%d] recv invalid cmd[%d]!", __FILE__, __LINE__, cmd.type);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] Run flow begin...", __FILE__, __LINE__);
	g_complete = 0;
	
	set_zd_data("$errcode1", "0000");
	ret = svc_run_flow(cycle, &cmd);
	if (ret != SW_OK)
	{
		memset(tmp, 0x0, sizeof(tmp));
		get_zd_data("$errcode", tmp);
		if (tmp[0] == '\0' || strcmp(tmp, "0000") == 0)
		{
			set_zd_data("$errcode", "0001");
		}
		pub_log_error("[%s][%d] run flow error!", __FILE__, __LINE__);
	}
	else
	{
		pub_log_info("[%s][%d] run flow success! type=[%d]", __FILE__, __LINE__, cmd.type);
	}
	svc_print_factor(pub_get_global_vars(), &g_cmd, g_idx, 1);
	ulog_release();
	pub_log_debug("[%s][%d] Run flow over!", __FILE__, __LINE__);
	memset(tmp, 0x0, sizeof(tmp));
	get_zd_data("$errcode1", tmp);
	if (strcmp(tmp, "0000") != 0)
	{		
		set_zd_data("$errcode", tmp);
	}
	svc_trace_reg(&g_cmd, 1);
	svc_alert_rule(g_cmd);
	
	pub_log_debug("[%s][%d] Send cmd begin...", __FILE__, __LINE__);

	trace_insert(pub_get_global_vars(), &g_cmd, TRACE_OUT);
	ret = route_snd_dst(pub_get_global_vars(), cycle->base.global_path, &g_cmd);
	if (ret == SW_ERROR)
	{
		pub_vars_free();
		pub_log_error("[%s][%d] send cmd error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_vars_free();
	pub_log_debug("[%s][%d] Send cmd success!", __FILE__, __LINE__);
	pub_log_debug("[%s][%d] Deal task request success!", __FILE__, __LINE__);
	
	return SW_OK;
}

sw_int_t svc_deal_cmd(sw_svc_cycle_t *cycle)
{
	int	ret = 0;
	sw_char_t	udp_name[64];
	sw_cmd_t	cmd;
	
	memset(&cmd, 0x0, sizeof(cmd));
	ret = udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] upd_recv error! fd=[%d] errno=[%d]:[%s]", 
			__FILE__, __LINE__, cycle->cmd_fd, errno, strerror(errno));
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
			/***
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			***/
			pub_log_debug("[%s][%d] stop myself!", __FILE__, __LINE__);
			svc_cycle_child_destroy(cycle);
			exit(0);
			break;
		case SW_MEXITSELF:
			pub_log_debug("[%s][%d]	Recv exit cmd!", __FILE__, __LINE__);
			pub_log_debug("[%s][%d] Total:[%d] Current:[%d]", 
				__FILE__, __LINE__, g_trace->head.total_cnt, g_trace->head.curr_cnt);
			if (g_trace->head.curr_cnt == 0)
			{
				ret = svc_child_register(cycle, SW_S_EXITED);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] Child register error!", __FILE__, __LINE__);
				}
				pub_log_debug("[%s][%d] All task finished, exit!", __FILE__, __LINE__);
				svc_cycle_child_destroy(cycle);
				exit(0);
			}
			ret = svc_child_register(cycle, SW_S_EXITING);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Child register error!", __FILE__, __LINE__);
				svc_cycle_child_destroy(cycle);
				exit(1);
			}
			g_exiting = 1;
			svc_trace_print(cycle);
			break;
		default:
			cmd.type = SW_ERRCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name , "%s" , cycle->base.name.data);
			udp_send(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), udp_name);
			break;
	}

	return SW_OK;
}

int get_index_by_fd(sw_svc_cycle_t *cycle, int fd)
{
	int i = 0;
	for (i = 0; i < MAX_SVR_LEVEL_CNT; i++)
	{
		if (cycle->fd_out[i] == fd)
		{
			return i;
		}
	}

	return -1;
}

static void svc_svrlvl_event_deal(sw_svc_cycle_t *cycle, sw_fd_list_t *fd_work, int recv_cnt, int udp_fd)
{
	int	cnt = 0;
	int	i = 0, j = 0;
	int	ret = 0;
	int	index = 0;
	sw_svc_cycle_t	*tmp = NULL;
	
	for (i = SVR_LEVEL_URGENT; i >= SVR_LEVEL_DEFAULT; i--)
	{
		for (j = 0; j < recv_cnt; j++)
		{
			if (udp_fd != fd_work[j].fd)
			{
				tmp = (sw_svc_cycle_t *)fd_work[j].data;
				index = get_index_by_fd(tmp, fd_work[j].fd);
				pub_log_debug("[%s][%d] index=[%d]", __FILE__, __LINE__, index);
				if (tmp->level[index] == i)
				{
					pub_log_debug("[%s][%d] tmp->cmd_fd=[%d]fd_work.fd=[%d]", __FILE__, __LINE__, tmp->cmd_fd, fd_work[j].fd);
					ret = fd_work[j].event_handler((sw_svc_cycle_t *)fd_work[j].data);
					if (ret != SW_OK)
					{
						pub_log_error("[%s][%d] event_handler error!", __FILE__, __LINE__);
					}
					cnt++;
					pub_log_bend("\0");
				}
			}
			else
			{
				ret = fd_work[j].event_handler((sw_svc_cycle_t *)fd_work[j].data);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] event_handler error!", __FILE__, __LINE__);
				}
				pub_log_bend("\0");
			}
		}
		
		if (cnt == recv_cnt)
		{
			pub_log_info("[%s][%d]deal svrlvl event end [%d]===[%d]", __FILE__, __LINE__, cnt, recv_cnt);
			break;
		}
	}
}

static void svc_comm_event_deal(sw_svc_cycle_t *cycle, sw_fd_list_t *fd_work, int recv_cnt)
{
	int i = 0;
	int ret = 0;
	
	for (i = 0; i < recv_cnt; i++)
	{
		pub_log_info("[%s][%d] Data arrived! recv_cnt=[%d] fd=[%d]", 
			__FILE__, __LINE__, recv_cnt, fd_work[i].fd);
		ret = fd_work[i].event_handler((sw_svc_cycle_t *)fd_work[i].data);                                       
		if (ret != SW_OK)                                                                                       
		{                                                                                                          
			pub_log_error("[%s][%d] event_handler error!", __FILE__, __LINE__);                  
		}
		pub_log_bend("\0");
	}
}

int child_work(sw_svc_cycle_t *cycle, int *fd_out)
{
	int	i = 0;
	int	cnt = 0;
	int	ret = 0;
	int	recv_cnt = 0;
	int	udp_fd = 0;
	sw_int64_t	now = 0;
	sw_int64_t	timeout = 0;
	sw_fd_set_t	*fd_set = NULL;
	sw_fd_list_t	*fd_list = NULL;
	sw_fd_list_t	*fd_work = NULL;
	
	pub_log_info("[%s][%d] Child work begin...", __FILE__, __LINE__);
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

	udp_fd = cycle->cmd_fd;
	pub_log_info("[%s][%d] cycle->cmd_fd=[%d] g_svr->use_svrlevel=[%d]",
		__FILE__, __LINE__, cycle->cmd_fd, g_svr->use_svrlevel);

	if (g_svr->use_svrlevel)
	{
		cnt = SVR_LEVEL_ALL;
	}
	else
	{
		cnt = 1;
	}

	fd_list = pub_pool_palloc(cycle->base.pool, sizeof(sw_fd_list_t) * (cnt + 1));
	if (fd_list == NULL)
	{
		pub_log_error("[%s][%d] pub_pool_palloc error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	for (i = 0; i < cnt; i++)
	{
		fd_list[i].fd = fd_out[i];
		cycle->cmd_fd = fd_out[i];

		fd_list[i].data = pub_pool_palloc(cycle->base.pool, sizeof(sw_svc_cycle_t));
		if (fd_list[i].data == NULL)
		{
			pub_log_error("[%s][%d] pub_pool_palloc error! errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return SW_ERROR;
		}
		memcpy(fd_list[i].data,  (void *)cycle, sizeof(sw_svc_cycle_t));
		fd_list[i].event_handler = (sw_event_handler_pt)svc_deal_task;
		ret = select_add_event(fd_set, &fd_list[i]);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}

	fd_list[0].fd = udp_fd;
	cycle->cmd_fd = udp_fd;
	fd_list[0].data = (void *)cycle;
	fd_list->event_handler = (sw_event_handler_pt)svc_deal_cmd;
	ret = select_add_event(fd_set, &fd_list[0]);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] select_add_event error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_end("[%s][%d]  worker begin...", __FILE__, __LINE__);

	while (1)
	{
		timeout = g_svr->scantime > 0 ? g_svr->scantime : 1000;
		recv_cnt = select_process_events(fd_set, &fd_work, timeout);
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
				svc_cycle_child_destroy(cycle);
				exit(0);
			}

			if (procs_get_sys_status(cycle->base.name.data) != SW_S_START)
			{
				pub_log_info("[%s][%d] Father exit!", __FILE__, __LINE__);
				svc_cycle_child_destroy(cycle);
				exit(0);
			}

			if (g_db_conn.conn == 1 && g_svr->exptime > 0)
			{
				now = (long)time(NULL);
				if (now - g_db_conn.start_time > g_svr->exptime)
				{
					pub_db_close();
					g_db_conn.conn = 0;
					pub_log_info("[%s][%d] exptime=[%ld] start=[%ld] now=[%ld]", 
						__FILE__, __LINE__, g_svr->exptime, g_db_conn.start_time, now); 
				}
			}
			
			if (g_exiting == 1)
			{
				if (g_trace->head.curr_cnt == 0)
				{
					ret = svc_child_register(cycle, SW_S_EXITED);
					if (ret != SW_OK)
					{
						pub_log_error("[%s][%d] Child register error!", __FILE__, __LINE__);
					}
					pub_log_debug("[%s][%d] All task finished, EXIT!", __FILE__, __LINE__);
					svc_cycle_child_destroy(cycle);
					exit(0);
				}
			}
			if (procs_update_process_busystatus(cycle->prdt, g_svr->svr_name, cycle->proc_name, PROC_S_IDLE) != SW_OK)
			{
				pub_log_error("[%s][%d] update [%s] busystats error!",
					__FILE__, __LINE__, cycle->proc_name);
			}
			/***
			svc_trace_print(cycle);
			***/
			continue;
		}
		
		pub_log_debug("[%s][%d] Data arrived! recv_cnt=[%d] fd=[%d]", 
			__FILE__, __LINE__, recv_cnt, fd_work[i].fd);
		if (g_svr->use_svrlevel)
		{
			svc_svrlvl_event_deal(cycle, fd_work, recv_cnt, udp_fd);
		}
		else
		{
			svc_comm_event_deal(cycle, fd_work, recv_cnt);
		}
	}

	return SW_OK;
}

int svc_init_flow_info(sw_svc_cycle_t *cycle, int id)
{
	int	index = 0;
	int	ret = 0;
	int	cache_cnt = 0;
	sw_char_t	flow_name[128];
	
	memset(flow_name, 0x0, sizeof(flow_name));
	
	memset(&flow, 0x0, sizeof(flow));
	g_svr = &cycle->svrs->svr[id];
	g_svr->index = id;
	sprintf(g_svr->flow_path, "%s/products/%s/etc/svrcfg", cycle->base.work_dir.data, cycle->prdt);
	sprintf(g_svr->work_path, "%s/products/%s", cycle->base.work_dir.data, cycle->prdt);
	
	pub_log_info("[%s][%d] id====[%d] svc_cnt[%d]", __FILE__, __LINE__, id, g_svr->service_cnt);
	g_nflow = (sw_flow_t *)(cycle->flow_addr + sizeof(sw_flow_t) * MAX_CACHE_CNT * id);
	g_nflow->head.flow_cnt = 0;
	cache_cnt = g_svr->service_cnt > MAX_CACHE_CNT ? MAX_CACHE_CNT : g_svr->service_cnt;
	for (index = 0; index < cache_cnt; index++)
	{
		pub_log_info("[%s][%d] service=[%s]", __FILE__, __LINE__, g_svr->services[index].name);
		ret = flow_init(g_svr->services[index].name);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] flow_init [%s] error!", __FILE__, __LINE__, g_svr->services[index].name);
			return SW_ERROR;
		}
		memcpy(&g_nflow->flow[g_nflow->head.flow_cnt].flow, &flow, sizeof(flow));
		strcpy(g_nflow->flow[g_nflow->head.flow_cnt].name, g_svr->services[index].name);
		g_nflow->flow[g_nflow->head.flow_cnt].upd_time = pub_time_get_current();
		g_nflow->head.flow_cnt++;
	}
	pub_log_info("[%s][%d] init [%s] flow info success!", __FILE__, __LINE__, g_svr->svr_name);
	for (index = cache_cnt; index < g_svr->service_cnt; index++)
	{
		pub_log_info("[%s][%d] No enough space to cache service [%s]!", __FILE__, __LINE__, g_svr->services[index].name);
	}
	
	return SW_OK;
}

sw_int_t svc_work(sw_svc_cycle_t *cycle, sw_int_t index, sw_int_t id)
{
	int i = 0;
	int cnt = 0;
	int	ret = 0;
	int	msg_out = 0;
	sw_fd_t	fd_out = 0;
	sw_int_t	proc_trace_size = 0;
	sw_char_t	udp_name[128];
	sw_char_t	flow_name[128];
	sw_char_t	log_path[128];
	sw_char_t	log_name[64];
	sw_global_path_t	global_path;
	
	memset(&g_db_conn, 0x0, sizeof(g_db_conn));
	memset(udp_name, 0x0, sizeof(udp_name));
	memset(flow_name, 0x0, sizeof(flow_name));
	memset(log_path, 0x0, sizeof(log_path));
	memset(log_name, 0x0, sizeof(log_name));
	memset(&global_path, 0x0, sizeof(global_path));
	
	g_svr = &cycle->svrs->svr[index];
	sprintf(flow_name, "%s_worker_%d", cycle->svrs->svr[index].svr_name, id);
	sprintf(log_name, "%s.log", flow_name);
	g_idx = id;
	pub_log_debug("[%s][%d] svr_name=[%s]", __FILE__, __LINE__, g_svr->svr_name);
	
	memset(udp_name, 0x0, sizeof(udp_name));
	sprintf(udp_name, "%s_%s", cycle->prdt, flow_name);
	cycle->cmd_fd = udp_bind(udp_name);
	if (cycle->cmd_fd <= 0)
	{
		pub_log_error("[%s][%d] upd_bind error! fd=[%d]", __FILE__, __LINE__, cycle->cmd_fd);
		return SW_ERROR;
	}
	
	ret = cfg_set_path(&global_path);	
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] cfg_set_path error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (g_svr->use_svrlevel)
	{
		cnt = SVR_LEVEL_ALL;
	}
	else
	{
		cnt = 1;
	}

	for (i = 0; i < cnt; i++)
	{
		fd_out = msg_out = 0;
		fd_out = msg_trans_create(&global_path, IPC_PRIVATE, 0, &msg_out);
		if (fd_out == SW_ERROR)
		{
			pub_log_error("[%s][%d] msg_trans_create error! fd=[%d]", __FILE__, __LINE__, fd_out);
			return SW_ERROR;
		}
		cycle->fd_out[i] = fd_out;
		cycle->msg_out[i] = msg_out;
		cycle->level[i] = i + 1;
		pub_log_info("[%s][%d] fd_out=[%d] msg_out=[%d] i=[%d]", __FILE__, __LINE__, fd_out, msg_out, i);
	}

	cycle->proc_index = id;
	strncpy(cycle->proc_name, flow_name, sizeof(cycle->proc_name) - 1);
	g_exiting = 0;
	
	g_nflow = (sw_flow_t *)(cycle->flow_addr + sizeof(sw_flow_t) * MAX_CACHE_CNT * index);
	proc_trace_size = (sizeof(sw_svc_trace_t) + sizeof(sw_svc_trace_item_t) * cycle->list_size);
	g_trace = (sw_svc_trace_t *)(cycle->trace_addr + proc_trace_size * MAX_PROC_CNT * index + proc_trace_size * id);
	memset(g_trace, 0x0, proc_trace_size);

	pub_mem_memzero(log_path, sizeof(log_path));
	sprintf(log_path, "%s/%s", cycle->base.log->log_path.data, log_name);
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret != SW_OK)
	{
		pub_log_stderr("pub_log_chglog debug path error.\n");
		return SW_ERROR;
	}

	ret = svc_init_db_fun(cycle, index);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] init db fun error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	g_svr->exptime = g_exptime;
	
	ret = svc_init_cmp_info(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] init cmp info error!", __FILE__, __LINE__);
		pub_log_stderr("[%s][%d] init cmp info error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = svc_child_register(cycle, SW_S_START);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Child register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (g_svr->use_dlcache == 1)
	{
		if (dlcache_init() != SW_OK)
		{
			pub_log_error("[%s][%d] init dlcache error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("[%s][%d] user dlcache, dlcache init success!",
			__FILE__, __LINE__);
	}
	ret = child_work(cycle, cycle->fd_out);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] child work error!", __FILE__, __LINE__);
		if (g_svr->use_dlcache == 1)
		{
			dlcache_clear();
			pub_log_info("[%s][%d] dlcache clear success!", __FILE__, __LINE__);
		}
		pub_log_end("[%s][%d] Child work error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	if (g_svr->use_dlcache == 1)
	{
		dlcache_clear();
		pub_log_info("[%s][%d] dlcache clear success!", __FILE__, __LINE__);
	}
	
	return SW_OK;
}

sw_int_t svc_create_svr(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	
	g_path = cycle->base.global_path;
	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		pub_log_info("[%s][%d] svr_name=[%s]", __FILE__, __LINE__, cycle->svrs->svr[i].svr_name);
		ret = svc_create_single_svr(cycle, i);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] svc_create_singl_svr error! index=[%d]", __FILE__, __LINE__, i);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] Create svr success!", __FILE__, __LINE__);

	return SW_OK;
}


int svc_get_svr_idx_by_name(sw_svc_cycle_t *cycle, const char *svr_name)
{
	if (cycle == NULL || svr_name == NULL)
	{
		pub_log_error("[%s][%d] parameter error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	int i = 0;
	int svr_cnt = cycle->svrs->head.svr_cnt;

	for (i = 0; i < svr_cnt; i++)
	{
		if (strcmp(cycle->svrs->svr[i].svr_name, svr_name) == 0)
		{
			return i;
		}
	}
	pub_log_error("[%s][%d] not find svr[%s]'s info", __FILE__, __LINE__);
	return SW_ERROR;
}

int svc_create_single_svr(sw_svc_cycle_t *cycle, int index)
{
	int	j = 0;
	int	ret = 0;
	sw_svr_grp_t	grp;

	if (cycle->svrs->svr[index].status != 1)
	{
		pub_log_debug("[%s][%d] [%s] unused!", __FILE__, __LINE__, cycle->svrs->svr[index].svr_name);
		return SW_OK;
	}
	
	pub_log_info("[%s][%d] index=[%d] reload=[%d]", __FILE__, __LINE__, index, cycle->svrs->svr[index].reload);
	
	if (cycle->svrs->svr[index].reload != 1)
	{
		ret = svc_init_flow_info(cycle, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] init [%s] flow info error!",
				__FILE__, __LINE__, cycle->svrs->svr[index].svr_name);
			return SW_ERROR;
		}
	}
	
	if (cycle->svrs->svr[index].seqs_name[0] != '\0' && 
		strcmp(cycle->svrs->svr[index].seqs_name, DEFAULT_TRACE_NAME) != 0)
	{
		ret = seqs_add_business_trace(cycle->svrs->svr[index].seqs_name, 0);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] seqs_add_business_trace error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	memset(&grp, 0x0, sizeof(grp));
	grp.svc_max = MAX_PROC_CNT;
	strcpy(grp.svrname, cycle->svrs->svr[index].svr_name);
	strcpy(grp.prdtname, cycle->prdt);
	ret = procs_svr_alloc(&grp);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] procs_svr_alloc success!", __FILE__, __LINE__);
	
	for (j = 0; j < cycle->svrs->svr[index].min; j++)
	{
		ret = svc_create_single_svc(cycle, index, j);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] svc_create_single_svc error! index=[%d] id=[%d]",
				__FILE__, __LINE__, index, j);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] Create svr success! index=[%d]", __FILE__, __LINE__, index);
	
	return SW_OK;
}

int svc_create_single_svc(sw_svc_cycle_t *cycle, int index, int id)
{
	pid_t	pid;
	int	ret = 0;
	
	pid = fork();
	if (pid < 0)
	{
		pub_log_error("[%s][%d] Fork error!errno=[%d][%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	else if (pid == 0)
	{
		cycle->svrs->svr[index].index = index;
		sprintf(cycle->svrs->svr[index].flow_path, "%s/products/%s/etc/svrcfg", cycle->base.work_dir.data, cycle->prdt);
		sprintf(cycle->svrs->svr[index].work_path, "%s/products/%s", cycle->base.work_dir.data, cycle->prdt);
		ret = svc_work(cycle, index, id);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] svc_work error", __FILE__, __LINE__);
			exit(1);
		}	
	}
	
	return SW_OK;
}

int get_svr_index_by_name(sw_svc_cycle_t *cycle, char *name)
{
	int	i = 0;
	
	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		if (strcmp(cycle->svrs->svr[i].svr_name, name) == 0)
		{
			return i;
		}
	}
	return -1;
}

int svc_init_list(list_t *l)
{
	*l = (list_t)calloc(1, sizeof(node_t));
	(*l)->next = NULL;
	
	return 0;
}

int svc_add_node(list_t *l, sw_svc_trace_item_t item)
{
	list_t	p = NULL;
	list_t	s = NULL;

	if (*l == NULL)
	{
		pub_log_error("[%s][%d] list is null!", __FILE__, __LINE__);
		return -1;
	}
	
	p = *l;
	while (p->next != NULL)
	{
		p = p->next;
	}
	
	s = (list_t)calloc(1, sizeof(node_t));
	if (s == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]", 
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	s->item = item;
	s->next = p->next;
	p->next = s;
	
	return 0;	
}

int svc_add_node_sort_by_time(list_t *l, sw_svc_trace_item_t *item)
{
	list_t	p = NULL;
	list_t	r = NULL;
	list_t	s = NULL;
	
	if (*l == NULL)
	{
		pub_log_error("[%s][%d] list is null!", __FILE__, __LINE__);
		return -1;
	}
	
	s = (list_t)calloc(1, sizeof(node_t));
	if (s == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	memcpy(&s->item, item, sizeof(sw_svc_trace_item_t));
	
	p = *l;
	while (p->next != NULL)
	{
		r = p->next;
		if (r->item.start_time >= item->start_time)
		{
			break;
		}
		p = p->next;
	}
	s->next = p->next;
	p->next = s;

	return 0;
}

int svc_print_list(list_t l, sw_int64_t alertno)
{
	int	fd = 0;
	int	ret = 0;
	char	dir[128];
	char	date[32];
	char	line[1024];
	char	time_buf[64];
	char	filename[128];
	list_t	p = NULL;
	
	if (l == NULL)
	{
		return 0;
	}
	
	memset(dir, 0x0, sizeof(dir));
	memset(date, 0x0, sizeof(date));
	memset(line, 0x0, sizeof(line));
	memset(filename, 0x0, sizeof(filename));
	memset(time_buf, 0x0, sizeof(time_buf));
	
	pub_time_getdate(date, 1);
	sprintf(dir, "%s/tmp/monitor/%s", getenv("SWWORK"), date);
	ret = access(dir, W_OK);
	if (ret < 0)
	{
		if (errno == ENOENT)
		{
			ret = pub_file_check_dir(dir);
			if (ret < 0)
			{
				pub_log_error("[%s][%d] mkdir [%s] error! errno=[%d]:[%s]",
					__FILE__, __LINE__, dir, errno, strerror(errno));
				return SW_ERROR;
			}
		}
		else
		{
			pub_log_error("[%s][%d] access [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, dir, errno, strerror(errno));
			return SW_ERROR;
		}
	}
	sprintf(filename, "%s/mon_error.log", dir);
	fd = open(filename, O_WRONLY | O_CREAT, 0777);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}
	pub_lock_fd(fd);
	lseek(fd, 0, SEEK_END);
	
	p = l->next;
	while (p != NULL)
	{
		memset(time_buf, 0x0, sizeof(time_buf));
		pub_change_time2(p->item.start_time, time_buf, 0);
		memset(line, 0x0, sizeof(line));
		sprintf(line, "预警编号:[%lld] 发起渠道:[%s] 交易码:[%s] 平台流水:[%lld] 错误码:[%d] MTYPE:[%d] 交易时间:[%s] 交易日期:[%s] 产品名称:[%s] SVR:[%s] SVC:[%s]", 
			alertno, p->item.lsn_name, p->item.tx_code, p->item.trace_no, p->item.errcode, p->item.mtype, time_buf, date, p->item.prdt, p->item.svr, p->item.svc);
		pub_log_info("[%s]", line);
		strcat(line, "\n");
		write(fd, line, strlen(line));
		p = p->next;
	}
	close(fd);
	
	return 0;
}

int svc_destroy_list(list_t *l)
{
	list_t	p = NULL;
	list_t	q = NULL;
	
	q = *l;
	if (q == NULL)
	{
		return 0;
	}
	
	while (q->next != NULL)
	{
		p = q->next;
		q->next = p->next;
		free(p);
		p = NULL;
	}
	free(*l);
	*l = NULL;
	
	return 0;
}

sw_char_t *change_time(sw_int64_t time_value)
{
	sw_int_t        time_sec = 0;
	static sw_char_t	time_buf[32];
	
	memset(time_buf, 0x0, sizeof(time_buf));
	if (time_value < 0 || time_buf == NULL)
	{
		return NULL;
	}

	time_sec =  time_value / 1000000;
	pub_time_change_time((time_t *)&time_sec, time_buf, 4);

        return time_buf;                                                                                                        
} 

int svc_alert(sw_svc_cycle_t *cycle)
{
	int	idxi = 0;
	int	idxj = 0;
	int	idxk = 0;
	char	*addr = NULL;
	char	send_buf[1024];
	double	fail_rate = 0.00;
	static int	use = 0;
	static list_t	list;
	sw_char_t	first_buf[32];
	sw_char_t	last_buf[32];
	sw_int64_t	cur_time = 0ll;
	sw_int64_t	first_time = 0ll;
	sw_int64_t	last_time = 0ll;
	sw_int_t	proc_trace_size = 0;
	sw_int64_t	now_time = 0ll;
	static sw_int64_t	last_alert_time = 0ll;
	static sw_svc_trace_info_t	trace_info;
	sw_svc_trace_t	*traces = NULL;
	sw_svc_trace_head_t	*head = NULL;
	sw_svc_trace_item_t	*item = NULL;
	
	if (use == 0)
	{
		svc_init_list(&list);
		use = 1;
	}

	addr = cycle->trace_addr;
	last_time = trace_info.last_time;
	proc_trace_size = (sizeof(sw_svc_trace_t) + sizeof(sw_svc_trace_item_t) * cycle->list_size);
	for (idxi = 0; idxi < cycle->svrs->head.svr_cnt; idxi++)
	{
		addr = cycle->trace_addr + (MAX_PROC_CNT * proc_trace_size * idxi);
		for (idxj = 0; idxj < MAX_PROC_CNT; idxj++)
		{
			traces = (sw_svc_trace_t *)(addr + proc_trace_size * idxj);
			head = &traces->head;
			if (head->total_cnt == 0)
			{
				continue;
			}
			item = traces->trace;
			for (idxk = 0; idxk < cycle->list_size; idxk++)
			{
				if (item[idxk].status != SW_SVC_DONE || item[idxk].mtype <= 0 || 
					item[idxk].start_time <= trace_info.last_time)
				{
					continue;
				}
				
				if (first_time == 0)
				{
					first_time = item[idxk].start_time;
				}
				
				if (first_time > item[idxk].start_time)
				{
					first_time = item[idxk].start_time;
				}
	
				if (last_time < item[idxk].start_time)
				{
					last_time = item[idxk].start_time;
				}
				
				trace_info.total++;
				if (item[idxk].errcode == 0)
				{
					trace_info.success++;
				}
				else
				{
					trace_info.failed++;
					svc_add_node_sort_by_time(&list, &item[idxk]);
				}
				
				switch (item[idxk].errcode)
				{
					case E_SUCCESS:
						trace_info.err[E_SUCCESS]++;
						break;
					case ERR_LSNOUT:
						trace_info.err[E_LSNOUT]++;
						break;
					case ERR_DOERR:
						trace_info.err[E_DOERR]++;
						break;
					default:
						trace_info.err[E_FAILED]++;
						break;
				}
			}
		}
	}

	if (trace_info.total > 0)
	{
		if (trace_info.last_time < last_time)
		{
			if (trace_info.first_time == 0)
			{
				trace_info.first_time = first_time;
			}
			trace_info.last_time = last_time;
			fail_rate = trace_info.failed * 1.0 / trace_info.total;
			if (trace_info.failed > 1 && fail_rate > 0.05)
			{
				now_time = (long)time(NULL);
				if ((last_alert_time > 0 &&  now_time - last_alert_time > SVC_ALERT_EXP_TIME) || 
					(last_alert_time == 0))
				{
					memset(last_buf, 0x0, sizeof(last_buf));
					memset(first_buf, 0x0, sizeof(first_buf));
					pub_change_time(trace_info.last_time, last_buf, 4);
					pub_change_time(trace_info.first_time, first_buf, 4);
					pub_log_debug("[%s][%d] 交易失败率=[%.2f]", __FILE__, __LINE__, fail_rate);
					memset(send_buf, 0x0, sizeof(send_buf));
					sprintf(send_buf, "紧急预警: 从[%s]到[%s] 业务总笔数:[%d] 成功笔数:[%d] 失败笔数:[%d] 其中 超时笔数:[%d] 原子交易失败笔数:[%d] 异常失败笔数:[%d] 请查看渠道日志进行检查!", 
						first_buf, last_buf, trace_info.total, trace_info.err[E_SUCCESS], 
						trace_info.failed, trace_info.err[E_LSNOUT], trace_info.err[E_DOERR], 
						trace_info.err[E_FAILED]);
					cur_time = pub_time_get_current();
					alert_msg(ERR_COUNT_SVC_OPR, "[MSGID:%lld][PRDT:%s] 业务用户[%s]产品[%s]%s", cur_time, g_prdt, getenv("USER"), g_prdt, send_buf);
					pub_log_debug("[%s][%d] MSGID:[%lld] PRDT:[%s] 业务用户[%s]产品[%s]%s", __FILE__, __LINE__, cur_time, g_prdt, getenv("USER"), g_prdt, send_buf);
					svc_print_list(list,cur_time);
					trace_info.total = 0;
					trace_info.failed = 0;
					trace_info.err[E_SUCCESS] = 0;
					trace_info.err[E_LSNOUT] = 0;
					trace_info.err[E_DOERR] = 0;
					trace_info.err[E_FAILED] = 0;
					trace_info.first_time = trace_info.last_time;
					svc_destroy_list(&list);
					use = 0;
					last_alert_time = now_time;
				}
			}
		}
	}
	
	return SW_OK;
}

int svc_handle_timeout(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;
	int	svr_index = 0;
	long	lnow = 0;
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;

	svc_alert(cycle);

	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		if (cycle->svrs->svr[i].status != 1)
		{
			continue;
		}
		grp_svr = procs_get_svr_by_name(cycle->svrs->svr[i].svr_name, cycle->prdt);
		if (grp_svr == NULL)
		{
			pub_log_error("[%s][%d] procs_get_svr_by_name error! name=[%s] prdt=[%s]", 
				__FILE__, __LINE__, cycle->svrs->svr[i].svr_name, cycle->prdt);
			break;
		}
		/***
		pub_log_info("[%s][%d] prdt=[%s] svr=[%s] curr=[%d]", 
			__FILE__, __LINE__, grp_svr->prdtname, grp_svr->svrname, grp_svr->svc_curr);
		***/
		for (j = 0; j < grp_svr->svc_curr; j++)
		{
			memset(&proc_info, 0x0, sizeof(proc_info));
			ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_get_proces_by_index error! index=[%d]", 
					__FILE__, __LINE__, j);
				break;
			}
			/***
			pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d] restart_cnt=[%d]",
				__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, 
				proc_info.status, proc_info.restart_cnt);
			***/
			if (proc_info.type != ND_SVC)
			{
				continue;
			}
			
			if (proc_info.status == SW_S_EXITED || proc_info.status == SW_S_STOPED)
			{
				pub_log_debug("[%s][%d] Proc:[%s] force quit, do not need to restart!", 
					__FILE__, __LINE__, proc_info.name); 
				continue;
			}

			ret = pub_proc_checkpid(proc_info.pid);
			if (ret != SW_OK && proc_info.restart_cnt >= 0)
			{
				pub_log_info("[%s][%d] Process [%s][%d] index=[%d] abnormal!", 
					__FILE__, __LINE__, proc_info.name, proc_info.pid, j);
				lnow = (long)time(NULL);
				if (lnow - proc_info.starttime < MIN_RESTART_TIME)
				{
					alert_msg(ERR_OFTEN_RESTART_FAILED, "紧急预警:进程[%s]在5分钟内重启多次,请查看渠道日志进行手工处理!", proc_info.name);
					pub_log_error("[%s][%d] [%s][%d] Time is less than 5 minutes, could not restart!",
						__FILE__, __LINE__, proc_info.name, proc_info.pid, proc_info.restart_cnt);
					proc_info.status = SW_S_ABNORMAL;
					proc_info.restart_cnt = -1;
					ret = procs_svr_register(&proc_info);
					if (ret != SW_OK)
					{
						pub_log_error("[%s][%d] procs_svr_register error!", __FILE__, __LINE__);
					}
					continue;
				}
				if (proc_info.restart_cnt == 0)
				{
					alert_msg(ERR_CNT_RESTART_FAILED, "紧急预警:进程[%s]重启次数过多,不能自动重启,请查看渠道日志进行手工处理!", proc_info.name);
					pub_log_error("[%s][%d] [%s][%d] restart_cnt=[%d] Could not restart!",
						__FILE__, __LINE__, proc_info.name, proc_info.pid, proc_info.restart_cnt);
					proc_info.status = SW_S_ABNORMAL;
					proc_info.restart_cnt--;
					ret = procs_svr_register(&proc_info);
					if (ret != SW_OK)
					{
						pub_log_error("[%s][%d] procs_svr_register error!", __FILE__, __LINE__);
					}
					continue;
				}

				alert_msg(ERR_PROC_FAILED, "紧急预警:进程[%s]异常,系统准备重启该进程,请查看渠道日志进行手工处理!", proc_info.name);
				proc_info.status = SW_S_ABNORMAL;
				proc_info.restart_cnt--;
				ret = procs_svr_register(&proc_info);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] procs_svr_register error!", __FILE__, __LINE__);
					continue;
				}
				pub_log_info("[%s][%d] svr=[%s] svc=[%s]",
					__FILE__, __LINE__, cycle->svrs->svr[i].svr_name, cycle->svrs->svr[i].services[j].name);
				svr_index = get_svr_index_by_name(cycle, grp_svr->svrname);
				if (svr_index == -1)
				{
					pub_log_error("[%s][%d] get svr index error! svrname=[%s]",
						__FILE__, __LINE__, grp_svr->svrname);
					continue;
				}
				pub_log_info("[%s][%d] svr_index=[%d] proc_index=[%d]",
					__FILE__, __LINE__, svr_index, proc_info.proc_index);
				ret = svc_create_single_svc(cycle, svr_index, proc_info.proc_index);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] svc_create_single_svc error! index=[%d] id=[%d]",
						__FILE__, __LINE__, i, j);
					return SW_ERROR;
				}
			}
                }
        }

	return SW_OK;
}

int svc_main(sw_svc_cycle_t *cycle)
{
	int	ret = 0;
	int	times = 0;
	int	recv_cnt = 0;
	sw_int32_t	i = 0;
	sw_int64_t	timer = 0;
	sw_fd_list_t	*fd_work;
	
	pub_log_info("[%s][%d] svc_main begin...", __FILE__, __LINE__);
	ret = svc_init(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] svc_init error!", __FILE__, __LINE__);
		return SW_DECLINED;
	}
	pub_log_info("[%s][%d] fd=[%d]", __FILE__, __LINE__, cycle->cmd_fd);
	
	ret = svc_init_com_param(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] svc_init_com_param error!", __FILE__, __LINE__);
		return SW_DECLINED;
	}
	pub_log_info("[%s][%d] svc_init_com_param success!", __FILE__, __LINE__);
	
	ret = svc_create_svr(cycle);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] svc_create_svr error!", __FILE__, __LINE__);
		return SW_DECLINED;
	}

	while(1)
	{
		sleep(5);
		if (svc_check_child_status(cycle) == SW_OK)
		{
			break;
		}
		
		times++;
		if (times > 12)
		{
			pub_log_error("[%s][%d] Check svr status error!", __FILE__, __LINE__);
			svc_send_cmd_to_child(cycle);
			return SW_DECLINED;
		}
	}

	while (1)
	{
		timer = 60000;
		recv_cnt = select_process_events(cycle->svc_fds, &fd_work, timer);
		if (recv_cnt < 0)
		{
			pub_log_error("[%s][%d] select_process_events error!", __FILE__, __LINE__);
			continue;
		}
		else if (recv_cnt == 0)
		{
			if (procs_get_sys_status(cycle->base.name.data) != SW_S_START)
			{
				pub_log_error("[%s][%d] Process abnormal!", __FILE__, __LINE__);
				return SW_DECLINED;
			}

			ret = svc_handle_timeout(cycle);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] svc_handle_timeout error!", __FILE__, __LINE__);
			}
		}
		else
		{
			pub_log_info("[%s][%d] Data arrived! recv_cnt=[%d]", __FILE__, __LINE__, recv_cnt);
			for (i = 0; i < recv_cnt; i++)
			{
				pub_log_info("[%s][%d] i=[%d]", __FILE__, __LINE__, i);
				ret = fd_work[i].event_handler((sw_svc_cycle_t*)fd_work[i].data);
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

int svc_send_cmd_to_child(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	j = 0;
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_char_t	udp_name[128];
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;

	for (i = 0 ; i < cycle->svrs->head.svr_cnt; i++)
	{
		grp_svr = procs_get_svr_by_name(cycle->svrs->svr[i].svr_name, cycle->prdt);
		if (grp_svr == NULL)
		{
			pub_log_error("[%s][%d] procs_get_svr_by_index error!", __FILE__, __LINE__);
			break;
		}
		pub_log_info("[%s][%d] prdt=[%s] svr=[%s] curr=[%d]", 
			__FILE__, __LINE__, grp_svr->prdtname, grp_svr->svrname, grp_svr->svc_curr);

		for (j = 0; j < grp_svr->svc_curr; j++)
		{
			memset(&proc_info, 0x0, sizeof(proc_info));
			ret = procs_get_proces_by_index(grp_svr, j, &proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
				break;
			}
			if (proc_info.type != ND_SVC)
			{
				continue;
			}
			pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d]",
				__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);

			memset(&cmd, 0x0, sizeof(cmd));
			cmd.type = SW_MSTOPSELF;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			pub_log_info("[%s][%d] cmd.udp_name=[%s]", __FILE__, __LINE__, cmd.udp_name);
			memset(udp_name, 0x0, sizeof(udp_name));
			sprintf(udp_name, "%s_%s", cycle->prdt, proc_info.name);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			pub_log_info("[%s][%d] send cmd to [%s]", __FILE__, __LINE__, proc_info.name);
			/***
			memset(&cmd, 0x0, sizeof(cmd));
			udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(cmd));
			pub_log_info("[%s][%d] recv from [%s]", __FILE__, __LINE__, proc_info.name);
			***/
                }
        }

	return SW_OK;
}

sw_int_t svc_start_svr_in_prdt(sw_svc_cycle_t *cycle, sw_char_t *svr_name)
{
	int	i = 0;
	int	ret = 0;
	int index = -1;
	sw_cmd_t recv_cmd;
	sw_char_t	udp_name[128];
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;
	
	pub_log_info("[%s][%d] svr_name is [%s] prdt=[%s]", __FILE__, __LINE__, svr_name, cycle->prdt);
	grp_svr = procs_get_svr_by_name(svr_name, cycle->prdt);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] get [%s][%s] svr info error!", __FILE__, __LINE__, svr_name, cycle->prdt);
		return SW_ERROR;
	}	

	index = svc_get_svr_idx_by_name(cycle, svr_name);
	if (index < 0)
	{
		pub_log_error("[%s][%d] get server[%s] info in prdt[%s] error!", __FILE__, __LINE__, svr_name, cycle->prdt);
                return SW_ERROR;
	}

	pub_log_info("[%s][%d] svr_name=[%s] prdt=[%s] svc_curr=[%d] index=[%d]", 
		__FILE__, __LINE__, svr_name, cycle->prdt, grp_svr->svc_curr, index);
	for (i = 0; i < grp_svr->svc_curr; i++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, i, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}
		if (proc_info.type != ND_SVC)
		{
			continue;
		}
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d]",
			__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);
	
		ret = pub_proc_checkpid(proc_info.pid);
		if(SW_OK == ret)
		{
			pub_log_info("[%s][%d] process[%s] is already exist", __FILE__, __LINE__, proc_info.name);
			continue;
		}

		ret = svc_create_single_svr(cycle, index);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Create SVR [%s] error!", __FILE__, __LINE__, svr_name);
			return SW_ERROR;
		}
	}
	
	return SW_OK;
}



sw_int_t svc_send_cmd_to_svr_ext(sw_svc_cycle_t *cycle, sw_char_t *svr_name, sw_cmd_t *cmd)
{
	int	i = 0;
	int	ret = 0;
	sw_cmd_t recv_cmd;
	sw_char_t	udp_name[128];
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;
	
	pub_log_info("[%s][%d] svr_name is [%s] prdt=[%s]", __FILE__, __LINE__, svr_name, cycle->prdt);
	grp_svr = procs_get_svr_by_name(svr_name, cycle->prdt);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] get [%s][%s] svr info error!", __FILE__, __LINE__, svr_name, cycle->prdt);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] svr_name=[%s] prdt=[%s] svc_curr=[%d]", 
		__FILE__, __LINE__, svr_name, cycle->prdt, grp_svr->svc_curr);
	for (i = 0; i < grp_svr->svc_curr; i++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, i, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}
		if (proc_info.type != ND_SVC)
		{
			continue;
		}
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d]",
			__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);
	
		ret = pub_proc_checkpid(proc_info.pid);
		if((SW_OK != ret) || (SW_S_START != proc_info.status))
		{
			pub_log_info("[%s][%d] process[%s] is abnormal", __FILE__, __LINE__, proc_info.name);
			continue;
		}
		
		/*register procinfo*/
		proc_info.status = SW_S_STOPED;
		ret = procs_sys_register(&proc_info);
		
		memset(udp_name, 0x0, sizeof(udp_name));
		sprintf(udp_name, "%s_%s", cycle->prdt, proc_info.name);
		udp_send(cycle->cmd_fd, (char *)cmd, sizeof(sw_cmd_t), udp_name);
		memset(&recv_cmd, 0x0, sizeof(sw_cmd_t));
		udp_recv(cycle->cmd_fd, (char *)&recv_cmd, sizeof(sw_cmd_t));
	}
	
	return SW_OK;
}


sw_int_t svc_send_cmd_to_svr(sw_svc_cycle_t *cycle, sw_char_t *svr_name)
{
	int	i = 0;
	int	ret = 0;
	sw_cmd_t	cmd;
	sw_char_t	udp_name[128];
	sw_svr_grp_t	*grp_svr;
	sw_proc_info_t	proc_info;
	
	pub_log_info("[%s][%d] svr_name is [%s] prdt=[%s]", __FILE__, __LINE__, svr_name, cycle->prdt);
	grp_svr = procs_get_svr_by_name(svr_name, cycle->prdt);
	if (grp_svr == NULL)
	{
		pub_log_error("[%s][%d] get [%s][%s] svr info error!", __FILE__, __LINE__, svr_name, cycle->prdt);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] svr_name=[%s] prdt=[%s] svc_curr=[%d]", 
		__FILE__, __LINE__, svr_name, cycle->prdt, grp_svr->svc_curr);
	for (i = 0; i < grp_svr->svc_curr; i++)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		ret = procs_get_proces_by_index(grp_svr, i, &proc_info);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_proces_by_index error!", __FILE__, __LINE__);
			break;
		}
		if (proc_info.type != ND_SVC)
		{
			continue;
		}
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] pid=[%d] type=[%d] status=[%d]",
			__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.pid, proc_info.type, proc_info.status);
		memset(&cmd, 0x0, sizeof(cmd));
		cmd.type = SW_MEXITSELF;
		memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
		sprintf(cmd.udp_name, "%s", cycle->base.name.data);
		memset(udp_name, 0x0, sizeof(udp_name));
		sprintf(udp_name, "%s_%s", cycle->prdt, proc_info.name);
		udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
		memset(&cmd, 0x0, sizeof(cmd));
		udp_recv(cycle->cmd_fd, (sw_char_t *)&cmd, sizeof(cmd));
	}
	
	return SW_OK;
}

sw_int_t svc_update_svc_status(sw_svc_cycle_t *cycle, char *svr_name, char *svc_name, int status)
{
	int	i = 0;
	sw_svr_t	*svr = NULL;
	
	if (cycle == NULL || svr_name == NULL || svc_name == NULL)
	{
		return SW_ERROR;
	}
	
	if (status != SW_SWITCH_ON && status != SW_SWITCH_OFF)
	{
		pub_log_error("[%s][%d] Status [%d] error!", __FILE__, __LINE__, status);
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d] svr_cnt=[%d]", __FILE__, __LINE__, cycle->svrs->head.svr_cnt);
	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		pub_log_info("[%s][%d] svr[%d]=[%s] svr_name=[%s]", 
			__FILE__, __LINE__, i, cycle->svrs->svr[i].svr_name, svr_name);
		if (strcmp(cycle->svrs->svr[i].svr_name, svr_name) == 0)
		{
			break;
		}
	}
	if (i == cycle->svrs->head.svr_cnt)
	{
		pub_log_error("[%s][%d] Can not find svr [%s]!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	
	svr = &cycle->svrs->svr[i];
	for (i = 0; i < svr->service_cnt; i++)
	{
		if (strcmp(svr->services[i].name, svc_name) == 0)
		{
			svr->services[i].status = status;
			break;
		}
	}
	if (i == svr->service_cnt)
	{
		pub_log_error("[%s][%d] Can not find svr:[%s] svc:[%s]",
			__FILE__, __LINE__, svr_name, svc_name);
		return SW_ERROR;
	}
	
	if (status == SW_SWITCH_OFF)
	{
		pub_log_info("[%s][%d] stop [%s][%s] success!", __FILE__, __LINE__, svr_name, svc_name);
	}
	else
	{
		pub_log_info("[%s][%d] start [%s][%s] success!", __FILE__, __LINE__, svr_name, svc_name);
	}

	return SW_OK;
}

sw_int_t svc_read_svc_info(sw_svc_cycle_t *cycle, sw_char_t *svr_name, sw_char_t *svc_name, sw_svc_t *svc)
{
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	if (cycle == NULL || svr_name == NULL || svc_name == NULL)
	{
		return SW_ERROR;
	}
	
	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/products/%s/etc/svrcfg/svrcfg.xml", cycle->base.work_dir.data, cycle->prdt);
	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".SERVERS.SERVER");
	while (node != NULL)
	{
		if (strcmp(node->name, "SERVER") != 0)
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
		
		if (strcmp(node1->value, svr_name) == 0)
		{
			break;
		}
	
		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未找到SVR[%s]的配置!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, "SERVICE");
	while (node != NULL)
	{
		if (strcmp(node->name, "SERVICE") != 0)
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
	
		if (strcmp(node1->value, svc_name) == 0)
		{
			break;
		}
	
		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未找到SVC[%s]的配置!", __FILE__, __LINE__, svc_name);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, "LIB");
	if (node != NULL && node->value != NULL)
	{
		strncpy(svc->lib, node->value, sizeof(svc->lib) - 1);
	}
	
	node = pub_xml_locnode(xml, "TRACE");
	if (node != NULL && node->value != NULL)
	{
		if (node->value[0] == '1')
		{
			svc->trace_flag = '1';
		}
		else
		{
			svc->trace_flag = '0';
		}
	}
	
	node = pub_xml_locnode(xml, "STATUS");
	if (node != NULL && node->value != NULL)
	{
		svc->status = atoi(node->value);
	}
	pub_xml_deltree(xml);
	
	return SW_OK;
}

sw_int_t svc_upd_svc(sw_svc_cycle_t *cycle, sw_char_t *svr_name, sw_char_t *svc_name)
{
	int	i = 0;
	int	svrid = 0;
	sw_int_t	ret = 0;
	sw_svc_t	*svc = NULL;
	sw_svr_t	*svr = NULL;
	
	if (cycle == NULL || svr_name == NULL || svc_name == NULL)
	{
		return SW_ERROR;
	}
	
	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		pub_log_info("[%s][%d] svr[%d]=[%s] svr_name=[%s]", 
			__FILE__, __LINE__, i, cycle->svrs->svr[i].svr_name, svr_name);
		if (strcmp(cycle->svrs->svr[i].svr_name, svr_name) == 0)
		{
			break;
		}
	}
	if (i == cycle->svrs->head.svr_cnt)
	{
		pub_log_error("[%s][%d] Can not find svr [%s]!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	svrid = i;
	svr = &cycle->svrs->svr[i];
	for (i = 0; i < svr->service_cnt; i++)
	{
		if (strcmp(svr->services[i].name, svc_name) == 0)
		{
			break;
		}
	}
	if (i == svr->service_cnt)
	{
		pub_log_error("[%s][%d] Can not find svr:[%s] svc:[%s]",
			__FILE__, __LINE__, svr_name, svc_name);
		return SW_ERROR;
	}
	
	svc = &svr->services[i];
	ret = svc_read_svc_info(cycle, svr_name, svc_name, svc);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Read SVR/SVC [%s][%s] info error!", __FILE__, __LINE__, svr_name, svc_name);
		return SW_ERROR;
	}
	
	if (svr->reload == 0)
	{
		ret = svc_init_flow_info(cycle, svrid);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Load flow info error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_debug("[%s][%d] Update [%s][%s] success!", __FILE__, __LINE__, svr_name, svc_name);

	return SW_OK;
}

sw_int_t svc_add_svc(sw_svc_cycle_t *cycle, sw_char_t *svr_name, sw_char_t *svc_name)
{
	int	i = 0;
	int	svrid = 0;
	sw_int_t	ret = 0;
	sw_svc_t	*svc = NULL;
	sw_svr_t	*svr = NULL;
	
	if (cycle == NULL || svr_name == NULL || svc_name == NULL)
	{
		return SW_ERROR;
	}

	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		pub_log_info("[%s][%d] svr[%d]=[%s] svr_name=[%s]", 
			__FILE__, __LINE__, i, cycle->svrs->svr[i].svr_name, svr_name);
		if (strcmp(cycle->svrs->svr[i].svr_name, svr_name) == 0)
		{
			break;
		}
	}
	if (i == cycle->svrs->head.svr_cnt)
	{
		pub_log_error("[%s][%d] Can not find svr [%s]!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	svrid = i;
	
	svr = &cycle->svrs->svr[i];
	if (svr->service_cnt >= MAX_SVC_CNT)
	{
		pub_log_error("[%s][%d] Not enough space for save this svc info! [%s][%s]",
			__FILE__, __LINE__, svr_name, svc_name);
		return SW_ERROR;
	}
	svc = &svr->services[svr->service_cnt];
	ret = svc_read_svc_info(cycle, svr_name, svc_name, svc);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Read SVR/SVC [%s][%s] info error!", __FILE__, __LINE__, svr_name, svc_name);
		return SW_ERROR;
	}
	svr->service_cnt++;
	
	if (svr->reload == 0)
	{
		ret = svc_init_flow_info(cycle, svrid);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Load flow info error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	
	pub_log_debug("[%s][%d] Add [%s][%s] success!", __FILE__, __LINE__, svr_name, svc_name);

	return SW_OK;
}

sw_int_t svc_read_svr_info(sw_svc_cycle_t *cycle, sw_char_t *svr_name, sw_svr_t *svr)
{
	int	i = 0;
	sw_svc_t	*svc = NULL;
	sw_char_t	svc_path[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	if (cycle == NULL || svr_name == NULL || svr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(svc_path, 0x0, sizeof(svc_path));
	sprintf(svc_path, "%s/products/%s/etc/svrcfg/svrcfg.xml", cycle->base.work_dir.data, cycle->prdt);
	xml = cfg_read_xml(svc_path);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, svc_path);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".SERVERS.SERVER");
	while (node != NULL)
	{
		if (strcmp(node->name, "SERVER") != 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("NAME could not null!", __FILE__, __LINE__);
			pub_xml_deltree(xml)
			return SW_ERROR;
		}
		
		if (strcmp(node1->value, svr_name) == 0)
		{
			break;
		}
	
		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未找到SVR[%s]的配置!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, "NAME");
	if (node == NULL || node->value == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未配置NAME!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strncpy(svr->svr_name, node->value, sizeof(svr->svr_name) - 1);
	
	node = pub_xml_locnode(xml, "MIN");
	if (node != NULL && node->value != NULL)
	{
		svr->min = atoi(node->value);
	}
	else
	{
		svr->min = MIN_PROC_CNT;
	}
	
	node = pub_xml_locnode(xml, "MAX");
	if (node != NULL && node->value != NULL)
	{
		svr->max = atoi(node->value);
	}
	else
	{
		svr->max = MAX_PROC_CNT;
	}
	
	node = pub_xml_locnode(xml, "RELOAD");
	if (node != NULL && node->value != NULL)
	{
		svr->reload = atoi(node->value);
	}
	else
	{
		svr->reload = SVC_RELOAD_DEFAULT;
	}
	
	node = pub_xml_locnode(xml, "DBID");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		strncpy(svr->db_conn_name, node->value, sizeof(svr->db_conn_name) - 1);
		svr->use_db = 1;
	}
	else
	{
		svr->use_db = 0;
	}
	
	node = pub_xml_locnode(xml, "STATUS");
	if (node != NULL && node->value != NULL)
	{
		svr->status = atoi(node->value);
	}
	else
	{
		svr->status = SVC_STATUS_DEFAULT;
	}
		
	node = pub_xml_locnode(xml, "SEQSNAME");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		strncpy(svr->seqs_name, node->value, sizeof(svr->seqs_name) - 1);
	}
	else
	{
		if (g_workmode_mp)
		{	
			pub_log_error("[%s][%d] mp workmode must cfg SEQNAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		strncpy(svr->seqs_name, DEFAULT_TRACE_NAME, sizeof(svr->seqs_name) - 1);
	}
	
	i = 0;
	node = pub_xml_locnode(xml, "SERVICE");
	while (node != NULL)
	{
		if (strcmp(node->name, "SERVICE") != 0)
		{
			node = node->next;
			continue;
		}
		
		svc = &svr->services[i];
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] NAME could not null!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		strncpy(svc->name, node1->value, sizeof(svc->name) - 1);
		
		node1 = pub_xml_locnode(xml, "STATUS");
		if (node1 != NULL && node1->value != NULL)
		{
			svc->status = atoi(node1->value);
		}
		else
		{
			svc->status = 1;
		}
		
		node1 = pub_xml_locnode(xml, "LIB");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(svc->lib, node1->value, sizeof(svc->lib) - 1);
		}
		
		node1 = pub_xml_locnode(xml, "TRACE");
		if (node1 != NULL && node1->value != NULL && node1->value[0] == '1')
		{
			svc->trace_flag = '1';
		}
		else
		{
			svc->trace_flag = '0';
		}
		
		i++;
		if (i >= MAX_SVC_CNT)
		{
			pub_log_error("[%s][%d] Too many service[%d]!", __FILE__, __LINE__, i);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		node = node->next;
	}
	svr->service_cnt = i;
	
	return SW_OK;
}

sw_int_t svc_add_svr(sw_svc_cycle_t *cycle, sw_char_t *svr_name)
{
	sw_int_t	ret = 0;
	sw_svr_t	*svr = NULL;
	
	if (cycle == NULL || svr_name == NULL || svr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (cycle->svrs->head.svr_cnt >= MAX_SVR_CNT)
	{
		pub_log_error("[%s][%d] Exceed MAX number of SVR count! [%d][%d]", 
			__FILE__, __LINE__, cycle->svrs->head.svr_cnt, MAX_SVR_CNT);
		return SW_ERROR;
	}
	
	svr = &cycle->svrs->svr[cycle->svrs->head.svr_cnt];
	ret = svc_read_svr_info(cycle, svr_name, svr);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Read SVR [%s] info error!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	
	ret = svc_create_single_svr(cycle, cycle->svrs->head.svr_cnt);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Create SVR [%s] error!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	cycle->svrs->head.svr_cnt++;
	pub_log_debug("[%s][%d] Add SVR [%s] success!", __FILE__, __LINE__, svr_name);
	
	return SW_OK;
}

sw_int_t svc_upd_svr(sw_svc_cycle_t *cycle, sw_char_t *svr_name)
{
	int	i = 0;
	int	min = 0;
	int	svrid = 0;
	sw_int_t	ret = 0;
	sw_cmd_t	cmd;
	sw_svr_t	*svr = NULL;
	sw_svr_t	new_svr;
	sw_char_t	udp_name[128];
	sw_char_t	proc_name[128];
	
	if (cycle == NULL || svr_name == NULL || svr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		pub_log_info("[%s][%d] svr[%d]=[%s] svr_name=[%s]",
			__FILE__, __LINE__, i, cycle->svrs->svr[i].svr_name, svr_name);
		if (strcmp(cycle->svrs->svr[i].svr_name, svr_name) == 0)
		{
			break;
		}
	}
	if (i == cycle->svrs->head.svr_cnt)
	{
		pub_log_error("[%s][%d] Can not find svr [%s]!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	svrid = i;
	svr = &cycle->svrs->svr[i];
	
	memset(&new_svr, 0x0, sizeof(new_svr));
	ret = svc_read_svr_info(cycle, svr_name, &new_svr);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Read SVR [%s] info error!", __FILE__, __LINE__, svr_name);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] Read svr [%s] info success!", __FILE__, __LINE__, svr_name);
	/*** 如果不需要每次都重载,则将DEF加载至内存中 ***/
	if (svr->reload == 1 && new_svr.reload == 0)
	{
		ret = svc_init_flow_info(cycle, svrid);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Load flow info error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	pub_log_debug("[%s][%d] new.seqs_name=[%s] old.seqs_name=[%s]", 
		__FILE__, __LINE__, new_svr.seqs_name, svr->seqs_name);
	/*** 如果流水生成器发生了变化,重新加载流水生成器 ***/
	if (new_svr.seqs_name[0] != '\0' && strcmp(svr->seqs_name, new_svr.seqs_name) != 0 &&
		strcmp(svr->seqs_name, DEFAULT_TRACE_NAME) != 0)
	{
		pub_log_debug("[%s][%d] Add seqs [%s]!", __FILE__, __LINE__, new_svr.seqs_name);
		ret = seqs_add_business_trace(new_svr.seqs_name, 0);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] seqs_add_business_trace error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	}
	min = svr->min;
	
	if (new_svr.min > MAX_PROC_CNT)
	{
		pub_log_error("[%s][%d] 超过最大进程数! MIN=[%d] MAX=[%d]", 
			__FILE__, __LINE__, new_svr.min, MAX_PROC_CNT);
		return SW_ERROR;
	}
	
	pub_log_debug("[%s][%d] new.min=[%d] old.min=[%d]", __FILE__, __LINE__, new_svr.min, svr->min);
	memcpy(svr, &new_svr, sizeof(sw_svr_t));
	svr->index = svrid;
	sprintf(svr->flow_path, "%s/products/%s/etc/svrcfg", cycle->base.work_dir.data, cycle->prdt);
	sprintf(svr->work_path, "%s/products/%s", cycle->base.work_dir.data, cycle->prdt);
	pub_log_debug("[%s][%d] min=[%d] svr->min=[%d]", __FILE__, __LINE__, min, svr->min);
	if (min > svr->min)
	{
		/*** 减少进程 ***/
		for (i = svr->min; i < min; i++)
		{
			pub_log_debug("[%s][%d] 开始停止第[%d]个进程...", __FILE__, __LINE__, i);
			memset(&cmd, 0x0, sizeof(cmd));
			memset(udp_name, 0x0, sizeof(udp_name));
			memset(proc_name, 0x0, sizeof(proc_name));
			sprintf(proc_name, "%s_worker_%d", svr_name, i);
			sprintf(udp_name, "%s_%s", cycle->prdt, proc_name);
			cmd.type = SW_MEXITSELF;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			pub_log_info("[%s][%d] send cmd to [%s]", __FILE__, __LINE__, proc_name);
		}
	}
	else if (min < svr->min)
	{
		/*** 增加进程 ***/
		for (i = min; i < svr->min; i++)
		{
			pub_log_debug("[%s][%d] 开始启动第[%d]个进程...", __FILE__, __LINE__, i);
			ret = svc_create_single_svc(cycle, svrid, i);                                                            
			if (ret != SW_OK)                                                                                        
			{                                                                                                        
				pub_log_error("[%s][%d] create svc error! svrid=[%d] id=[%d]",
					__FILE__, __LINE__, svrid, i);
				return SW_ERROR;
			}
		}
	}
	pub_log_debug("[%s][%d] Update SVR [%s] success!", __FILE__, __LINE__, svr_name);
	
	return SW_OK;
}

sw_int_t svc_handle_control_cmd(sw_svc_cycle_t *cycle)
{
	int	ret = 0;
	sw_char_t	udp_name[64];
	sw_cmd_t	cmd;
	sw_proc_info_t	proc_info;
	
	memset(&proc_info, 0x0, sizeof(proc_info));
	memset(&cmd, 0x0, sizeof(cmd));
	ret = udp_recv(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] upd_recv error! fd=[%d]", __FILE__, __LINE__, cycle->cmd_fd);
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
			pub_log_debug("[%s][%d] Stop SELF!", __FILE__, __LINE__);
			memset(&proc_info, 0x0, sizeof(proc_info));
			strcpy(proc_info.name, cycle->base.name.data);
			proc_info.pid = getpid();
			proc_info.type = ND_SVC;
			proc_info.status = SW_S_STOPPING;
			ret = procs_sys_register(&proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = svc_send_cmd_to_child(cycle);
			pub_log_info("[%s][%d] After send cmd! ret=[%d]", __FILE__, __LINE__, ret);
			memset(&proc_info, 0x0, sizeof(proc_info));
			strcpy(proc_info.name, cycle->base.name.data);
			proc_info.pid = getpid();
			proc_info.type = ND_SVC;
			proc_info.status = SW_S_STOPED;
			ret = procs_sys_register(&proc_info);
			if (ret != SW_OK)
			{
				pub_log_error("%s, %d, procs_register error.", __FILE__, __LINE__);
				return SW_ERROR;
			}
			return SW_ABORT;
			break;
		case SW_MSTOPSVR:
			pub_log_debug("[%s][%d] stop server[%s] in product!", __FILE__, __LINE__, cmd.dst_svr);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			
			char svr_name[64];
			memset(svr_name, 0x0, sizeof(svr_name));
			strncpy(svr_name, cmd.dst_svr, sizeof(svr_name) - 1);
			memset(&cmd, 0x0, sizeof(cmd));
			cmd.type = SW_MSTOPSELF;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			ret = svc_send_cmd_to_svr_ext(cycle, svr_name, &cmd);
			pub_log_info("[%s][%d] after send cmd to server process! ret=[%d]", __FILE__, __LINE__, ret);
			break;
		case SW_MSTARTSVR:
			pub_log_debug("[%s][%d] start server[%s] in product!", __FILE__, __LINE__, cmd.dst_svr);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			svc_start_svr_in_prdt(cycle, cmd.dst_svr);
			break;
		case SW_DEL_SVR:
			pub_log_debug("[%s][%d] Stop SVR:[%s]", __FILE__, __LINE__, cmd.dst_svr);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = svc_send_cmd_to_svr(cycle, cmd.dst_svr);
			pub_log_info("[%s][%d] After send cmd! ret=[%d]", __FILE__, __LINE__, ret);
			break;
		case SW_DEL_SVC:
			pub_log_debug("[%s][%d] Delete SVC [%s][%s]!", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
		case SW_STOP_SVC:
			pub_log_debug("[%s][%d] Stop SVC [%s][%s]!", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = svc_update_svc_status(cycle, cmd.dst_svr, cmd.dst_svc, SW_SWITCH_OFF);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Stop SVC error! SVR/SVC:[%s][%s]",
					__FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			}
			pub_log_info("[%s][%d] After stop svc! [%s][%s]", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			break;
		case SW_START_SVC:
			pub_log_debug("[%s][%d] Start SVC [%s][%s]!", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = svc_update_svc_status(cycle, cmd.dst_svr, cmd.dst_svc, SW_SWITCH_ON);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Start SVC error! SVR/SVC:[%s][%s]",
					__FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			}
			pub_log_info("[%s][%d] After start svc! [%s]/[%s]", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			break;
		case SW_ADD_SVC:
			pub_log_debug("[%s][%d] ADD: SVR=[%s] SVC=[%s]", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = svc_add_svc(cycle, cmd.dst_svr, cmd.dst_svc);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Add SVC error! SVR/SVC:[%s][%s]",
					__FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			}
			pub_log_info("[%s][%d] After add svc! [%s]/[%s]", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			break;
		case SW_UPD_SVC:
			pub_log_debug("[%s][%d] UPDATE: SVR=[%s] SVC=[%s]", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			ret = svc_upd_svc(cycle, cmd.dst_svr, cmd.dst_svc);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Update SVC error! SVR/SVC:[%s][%s]",
					__FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			}
			pub_log_info("[%s][%d] After upd svc [%s]/[%s]!", __FILE__, __LINE__, cmd.dst_svr, cmd.dst_svc);
			break;
		case SW_ADD_SVR:
			pub_log_debug("[%s][%d] ADD SVR:[%s]", __FILE__, __LINE__, cmd.dst_svr);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			svc_add_svr(cycle, cmd.dst_svr);
			pub_log_info("[%s][%d] After update svr [%s]!", __FILE__, __LINE__, cmd.dst_svr);
			break;
		case SW_UPD_SVR:
			pub_log_debug("[%s][%d] Update SVR:[%s]", __FILE__, __LINE__, cmd.dst_svr);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			svc_upd_svr(cycle, cmd.dst_svr);
			pub_log_info("[%s][%d] After upd svr [%s]!", __FILE__, __LINE__, cmd.dst_svr);
			break;
		case SW_ARG_MODIFY:
			pub_log_debug("[%s][%d]recv modify arg cmd!!!", __FILE__, __LINE__);
			cmd.type = SW_RESCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name, "%s", cycle->base.name.data);
			udp_send(cycle->cmd_fd, (char*)&cmd, sizeof(cmd), udp_name);
			
			sw_char_t filepath[256];
			memset(filepath, 0x0, sizeof(filepath));
			sprintf(filepath, "%s/tmp/%s", getenv("SWWORK"), cmd.dst_svr);
			ret = set_arg(filepath);
			if (ret != 0)
			{
				pub_log_error("[%s][%d]set arg error!!!", __FILE__, __LINE__);
			}
			remove(filepath);
			break;
		default:
			cmd.type = SW_ERRCMD;
			memset(cmd.udp_name, 0x0, sizeof(cmd.udp_name));
			sprintf(cmd.udp_name , "%s" , cycle->base.name.data);
			udp_send(cycle->cmd_fd, (sw_char_t*)&cmd, sizeof(sw_cmd_t), udp_name);
			break;
	}

	return SW_OK;
}

sw_int32_t svc_father_register(sw_svc_cycle_t *cycle, sw_int32_t status)
{
	sw_int32_t	ret = 0;
	sw_proc_info_t	proc_info;

	pub_mem_memzero(&proc_info, sizeof(proc_info));
	
	ret = procs_get_sys_by_name(cycle->base.name.data, &proc_info);
	if (ret != SW_OK)
	{
		strcpy(proc_info.name, cycle->base.name.data);
		proc_info.pid = getpid();
		proc_info.type = ND_SVC;
		proc_info.status = status;
		proc_info.shmid = cycle->shm.id;
		proc_info.restart_type = LIMITED_RESTART;
	}
	else
	{
		proc_info.pid = getpid();
		proc_info.status = status;
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	ret = procs_sys_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_sys_register error! name=[%s]", 
			__FILE__, __LINE__, proc_info.name);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] proc:[%s] s:[%d] p:[%d]", 
		__FILE__, __LINE__, proc_info.name, proc_info.status, proc_info.pid);
	
	return SW_OK;
}

sw_int_t svc_child_register(sw_svc_cycle_t *cycle, sw_int32_t status) 
{
	int	ret = 0;
	sw_proc_info_t	proc_info;

	memset(&proc_info, 0x0, sizeof(proc_info));
	ret = procs_get_proces_info(cycle->prdt, g_svr->svr_name, cycle->proc_name, &proc_info);
	if (ret == SW_ERROR)
	{
		memset(&proc_info, 0x0, sizeof(proc_info));
		strcpy(proc_info.name, cycle->proc_name);
		strcpy(proc_info.svr_name, g_svr->svr_name);
		strcpy(proc_info.prdt_name, cycle->prdt);
		proc_info.pid = getpid();
		proc_info.type = ND_SVC;
		proc_info.status = status;
		proc_info.mqid = cycle->msg_out[0];
		proc_info.proc_index = cycle->proc_index;
		proc_info.restart_type = LIMITED_RESTART;
		proc_info.shmid = cycle->shm.id;
		memcpy(&proc_info.mqids, cycle->msg_out, sizeof(proc_info.mqids));
	}
	else
	{
		pub_log_info("[%s][%d] proc [%s] exist!", __FILE__, __LINE__, cycle->proc_name);
		pub_log_info("[%s][%d] name=[%s] svr_name=[%s] prdt_name=[%s]",
			__FILE__, __LINE__, proc_info.name, proc_info.svr_name, proc_info.prdt_name);
		proc_info.pid = getpid();
		proc_info.status = status;
		proc_info.mqid = cycle->msg_out[0];
		proc_info.shmid = cycle->shm.id;
		memcpy(&proc_info.mqids, cycle->msg_out, sizeof(proc_info.mqids));
		if (status == SW_S_START && proc_info.restart_cnt < 0)
		{
			proc_info.restart_cnt = MAX_RESTART_CNT;
		}
	}
	ret = procs_svr_register(&proc_info);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] procs_svr_register error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

int get_bus_factor(sw_loc_vars_t *locvars, sw_buf_t *var_buf, sw_int64_t trace_no)
{
	int	idx = 0;
	int	len = 0;
	int	ret = 0;
	char	tmp[64];
	char	buf[1024];
#if defined(__SHM_VARS_SUPPORT__)
	char	*value = NULL;
	vars_node_t	*head = NULL;
	vars_node_t	*node = NULL;
	vars_object_t	*vars = (vars_object_t *)locvars->data;
#endif
	
#ifndef __SHM_VARS_SUPPORT__
	return 0;
#endif
	for (idx = 0; idx < VARS_NODE_NUM; idx++)
	{
		head = &(vars->head->node[idx]);
		node = head;
		while (NULL != node)
		{
			if (node->name[0] == '\0' || node->name[0] != '#' || 
				node->type != VAR_TYPE_CHAR || strncmp(node->name, "#DB", 3) == 0 ||
				strchr(node->name, '(') != NULL)
			{
				#if defined(__VARS_MEM_ALLOC_SLAB__)
				if(node != head)
				{
					break;
				}
				#endif /*__VARS_MEM_ALLOC_SLAB__*/

				if (0 == node->next)
				{
					break;
				}
				node = vars_offset_to_addr(vars, node->next);
				continue;
			}

			value = vars_offset_to_addr(vars, node->value);

			/***  TRACE_NO|[NAME] = [VALUE]|\n ***/
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "%lld", trace_no);
			len = strlen(tmp) + 1 + strlen(node->name) + 9 + node->length;
			ret = pub_buf_chksize(var_buf, len);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] Check buf size error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			
			if (pub_is_filter(node->name))
			{
				memset(buf, 0x0, sizeof(buf));
				memset(buf, '*', node->length);
				sprintf(var_buf->data + var_buf->len, "%lld|[%s] = [%s]|\n", trace_no, node->name, buf);
			}
			else
			{
				sprintf(var_buf->data + var_buf->len, "%lld|[%s] = [%s]|\n", trace_no, node->name, value);
			}
			len = strlen(var_buf->data + var_buf->len);
			var_buf->len += len;
			if (0 == node->next)
			{
				break;
			}

			node = vars_offset_to_addr(vars, node->next);
		}
	}

	return 0;
}

int print_bus(sw_buf_t *var_buf, char *prdt_name, char *svr_name, char *svc_name, sw_int64_t trace_no, int block)
{
	int	fd = 0;
	int	times = 0;
	off_t	file_size = 0;
	char	buf[256];
	char	date[16];
	char	datetime[32];
	char	filename[128];
	struct tm       *pstm;
	struct timeb    tmb;
	
	memset(date, 0x0, sizeof(date));
	memset(date, 0x0, sizeof(date));
	memset(datetime, 0x0, sizeof(datetime));
	memset(filename, 0x0, sizeof(filename));
	
	memset(&tmb, 0x0, sizeof(tmb));
	ftime(&tmb);
	pstm = localtime(&tmb.time);
	strftime(date, 32, "%Y%m%d", pstm);
	strftime(datetime, 32, "%H%M%S", pstm);
	
	memset(filename, 0x0, sizeof(filename));
	snprintf(filename, sizeof(filename), "%s/log/%s/%s/%s_%s.pkg",
		getenv("SWWORK"), prdt_name, svr_name, svc_name, date);
	fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd == -1)
	{
		pub_log_error("[%s][%d] Can not open file [%s], errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}
	
	times = 0;
	while (lockf(fd, F_LOCK, 0) == -1)
	{
		usleep(100);
		times++;
		if (times > MAX_OPEN_RETRY)
		{
			pub_log_error("[%s][%d] Lock file [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, filename, errno, strerror(errno));
			break;
		}
	}

	file_size = lseek(fd, 0, SEEK_END);
	if (file_size == (off_t)-1)
	{
		pub_log_error("[%s][%d] Lseek [%s] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		close(fd);
		return SW_ERROR;
	}
	write(fd, var_buf->data, var_buf->len);                                                                   
	close(fd);
	
	if (file_size >= MAX_BUS_BAK_SIZE)
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s.%s", filename, datetime);
		if (rename(filename, buf) != 0)
		{
			pub_log_error("[%s][%d] Rename [%s] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, filename, errno, strerror(errno));
			return SW_ERROR;
		}
	}
	
	return SW_OK;
}

int svc_print_factor(sw_loc_vars_t *locvars, sw_cmd_t *cmd, int idx, int flag)
{
	int	len = 0;
	int	ret = 0;
	int	block = 0;
	char	buf[128];
	char	head[128];
	char	svc_name[64];
	char	svr_name[64];
	sw_buf_t	var_buf;
	sw_int64_t	trace_no = 0;

	if (g_applog_level <= SW_LOG_INFO)
	{
		return 0;
	}
	pub_log_info("[%s][%d] 开始打印交易要素... Flag=[%d] type=[%d]", __FILE__, __LINE__, flag, cmd->type);
	
	memset(buf, 0x0, sizeof(buf));
	memset(head, 0x0, sizeof(head));
	memset(svc_name, 0x0, sizeof(svc_name));
	memset(svr_name, 0x0, sizeof(svr_name));

	if (flag == 0)
	{
		if (cmd->type == SW_TASKREQ)
		{
			block = BLOCK_BEGIN;
			sprintf(head, "@@@:TASKREQ [%s]任务请求业务要素:", cmd->lsn_name);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_CALLREQ)
		{
			sprintf(head, "@@@:CALLREQ [%s] CALL请求业务要素:", cmd->dst_svr);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_CALLRES)
		{
			sprintf(head, "@@@:CALLRES [%s] CALL应答业务要素:", cmd->ori_svr);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_CALLLSNRES)
		{
			sprintf(head, "@@@:CALLLSNRES [%s] CALLLSN应答业务要素:", cmd->ori_svr + 6);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_POSTREQ)
		{
			sprintf(head, "@@@:POSTREQ [%s] POST请求业务要素:", cmd->dst_svr);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else
		{
			pub_log_error("@@@:[%s][%d] TYPE====[%d]", __FILE__, __LINE__, cmd->type);
		}
	}
	else
	{
		if (cmd->type == SW_TASKRES)
		{
			block = BLOCK_END;
			sprintf(head, "@@@:TASKRES [%s] 任务应答业务要素:", cmd->lsn_name);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_CALLREQ)
		{
			sprintf(head, "@@@:CALLREQ [%s] CALL请求业务要素:", cmd->dst_svr);
			strncpy(svr_name, cmd->ori_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->ori_def, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_CALLRES)
		{
			sprintf(head, "@@@:CALLRES [%s] CALL应答业务要素:", cmd->dst_svr);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_CALLLSNREQ)
		{
			sprintf(head, "@@@:CALLLSNREQ [%s] CALLLSN请求业务要素:", cmd->dst_svr + 6);
			strncpy(svr_name, cmd->ori_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_LINKNULL)
		{
			sprintf(head, "@@@:LINKNULL [%s] LINKNULL业务要素:", cmd->dst_svr);
			strncpy(svr_name, cmd->dst_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else if (cmd->type == SW_POSTLSNREQ)
		{
			sprintf(head, "@@@:POSTLSNREQ [%s] POSTLSN请求业务要素:", cmd->dst_svr + 6);
			strncpy(svr_name, cmd->ori_svr, sizeof(svr_name) - 1);
			strncpy(svc_name, cmd->def_name, sizeof(svc_name) - 1);
		}
		else
		{
			pub_log_error("[%s][%d] TYPE=[%d]", __FILE__, __LINE__, cmd->type);
		}
	}

	trace_no = cmd->trace_no;
	pub_buf_init(&var_buf);
	if (block == BLOCK_BEGIN)
	{
		var_buf.len += sprintf(var_buf.data + var_buf.len, "%lld|---------------BEGIN---------------|\n", trace_no);
	}
	var_buf.len += sprintf(var_buf.data + var_buf.len, "%lld|%s|\n", trace_no, head);
	get_bus_factor(locvars, &var_buf, trace_no);
	if (block == BLOCK_END)
	{
		memset(buf, 0x0, sizeof(buf));
		len = sprintf(buf, "%lld|----------------END----------------|\n\n\n", trace_no);
		ret = pub_buf_chksize(&var_buf, len);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Check buf size error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		strncpy(var_buf.data + var_buf.len, buf, strlen(buf));
		var_buf.len += strlen(buf);
	}
	print_bus(&var_buf, g_prdt, svr_name, svc_name, trace_no, block);
	pub_buf_clear(&var_buf);
	pub_log_info("[%s][%d] 打印交易要素完成!", __FILE__, __LINE__);

	return 0;
}

typedef struct
{
	sw_char_t	opt[6];
	sw_char_t	arg_ctgy[32];
	sw_char_t	arg_bus[32];
	sw_char_t	arg_name[64];
	sw_char_t	arg_value[64];
}argparams_t;

static void print_argparams(argparams_t arg)
{
	pub_log_info("[%s][%d] opt=[%s] arg_ctgy=[%s] arg_bus=[%s] arg_name=[%s] arg_value=[%s]",
		__FILE__, __LINE__, arg.opt, arg.arg_ctgy, arg.arg_bus, arg.arg_name, arg.arg_value);
}

static sw_int_t get_argparams(sw_char_t *buf, argparams_t *arg)
{
	sw_int32_t	i = 0;
	sw_int32_t	len = 0;
	sw_char_t	out[6][64];
	sw_char_t	*ptr = NULL;
	sw_char_t	*tmp = NULL;
	const sw_char_t	*sep = "|";
	
	memset(out, 0x0, sizeof(out));

	i = 0;
	ptr = buf;
	while (*ptr != '\0')
	{
		tmp = strstr(ptr, sep);
		if (tmp == NULL)
		{
			strcpy(out[i], ptr);
			i++;
			break;
		}
		len = tmp - ptr;
		memcpy(out[i], ptr, len);
		out[i][len] = '\0';
		ptr = tmp + strlen(sep);
		pub_log_info("[%s][%d] field[%d]=[%s][%d]", __FILE__, __LINE__, i, out[i], len);
		i++;
	}
	memcpy(arg->opt, out[0], sizeof(arg->opt) - 1);
	memcpy(arg->arg_ctgy, out[1], sizeof(arg->arg_ctgy) - 1);
	memcpy(arg->arg_bus, out[2], sizeof(arg->arg_bus) - 1);
	memcpy(arg->arg_name, out[3], sizeof(arg->arg_name) - 1);
	memcpy(arg->arg_value, out[4], sizeof(arg->arg_value) - 1);
	
	return 0;
}

int set_arg(sw_char_t *filename)
{
	FILE	*fp = NULL;
	sw_int_t	ret = 0;
	sw_char_t	name[128];
	sw_char_t	line[1024];
	prdt_arg_t	arg;
	argparams_t	params;
	
	memset(name, 0x0, sizeof(name));
	memset(line, 0x0, sizeof(line));
	memset(&arg, 0x0, sizeof(arg));
	memset(&params, 0x0, sizeof(params));
	
	if (filename == NULL || filename[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}

	while (!feof(fp))
	{
		memset(line, 0x0, sizeof(line));
		if (fgets(line, sizeof(line), fp) == NULL)
		{
			break;
		}

		memset(&params, 0x0, sizeof(params));
		get_argparams(line, &params);
		print_argparams(params);
		
		if (params.opt[0] == 'A' || params.opt[0] == 'M')
		{
			memset(&arg, 0x0, sizeof(arg));
			strncpy(arg.name, params.arg_name, sizeof(arg.name) - 1);
			strncpy(arg.value, params.arg_value, sizeof(arg.value) - 1);
			memset(name, 0x0, sizeof(name));
			sprintf(name, "%s_%s_%s", params.arg_name, params.arg_ctgy, params.arg_bus);
			ret = prdt_arg_set(name, &arg, sizeof(arg));
			if (ret != 0)
			{
				pub_log_error("[%s][%d] set prdt arg failed!", __FILE__, __LINE__);
				fclose(fp);
				return -1;
			}
		}
		else if (params.opt[0] == 'D')
		{
			memset(name, 0x0, sizeof(name));
			sprintf(name, "%s_%s_%s", params.arg_name, params.arg_ctgy, params.arg_bus);
			ret = prdt_arg_delete(name);
			if (ret != 0)
			{
				pub_log_error("[%s][%d] delete prdt arg failed!", __FILE__, __LINE__);
				fclose(fp);
				return -1;
			}
		}
		else
		{
			pub_log_error("[%s][%d] invaild opt[%s]!", __FILE__, __LINE__, params.opt);
		}
	}
	fclose(fp);
	pub_log_info("[%s][%d] Set arg success!", __FILE__, __LINE__);

	return 0;
}

static unsigned int libname_key(unsigned int buckets, void *p_key)
{
	unsigned int    bucket = 0;
	char    *ptr = (char *)p_key;

	while (*ptr != '\0')
	{
		bucket += (unsigned char)(*ptr);
		ptr++;
	}

	return bucket % buckets;
}

static hash_t	*g_dlcache = NULL;

int dlcache_init()
{
	g_dlcache = hash_alloc(MAX_DISTINCT_LIB, MAX_LNAME_LEN, sizeof(dlnode_t), libname_key);
	if (g_dlcache == NULL)
	{
		pub_log_error("[%s][%d] Alloc hash cache error!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int dlcache_free(char *libname, char *funname)
{
	dlnode_t      *pnode = NULL;
	char	keyname[MAX_LNAME_LEN];

	if (g_dlcache == NULL)
	{
		pub_log_error("[%s][%d] g_dlcache is null!", __FILE__, __LINE__);
		return -1;
	}

	if (libname == NULL || libname[0] == '\0' || funname == NULL || funname[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error, name is null!", __FILE__, __LINE__);
		return -1;
	}
	
	memset(keyname, 0x0, sizeof(keyname));
	sprintf(keyname, "%s%s", libname, funname);
	pnode = hash_lookup_entry(g_dlcache, keyname);
	if (pnode != NULL && pnode->handle != NULL)
	{
		dlclose(pnode->handle);
	}
	hash_free_entry(g_dlcache, keyname);

	return 0;
}

static int dlcache_free_hash(struct hash *p_hash)
{
	unsigned int	i = 0;
	dlnode_t	*pnode = NULL;
	struct hash_node	*p_node = NULL;
	struct hash_node	*p_node1 = NULL;
	struct hash_node	**p_bucket = NULL;

	if (p_hash == NULL)
	{
		return -1;
	}

	for (i = 0; i < p_hash->buckets; i++)
	{
		p_bucket = &(p_hash->p_nodes[i]);
		p_node = *p_bucket;
		while (p_node != NULL)
		{
			p_node1 = p_node;
			p_node = p_node->next;
			if (p_node1->p_value != NULL)
			{
				pnode = (dlnode_t *)p_node1->p_value;
				if (pnode->handle != NULL)
				{
					dlclose(pnode->handle);
				}
				free(p_node1->p_value);
			}

			if (p_node1->p_key != NULL)
			{
				free(p_node1->p_key);
			}
			free(p_node1);
		}
	}

	if (p_hash->p_nodes != NULL)
	{
		free(p_hash->p_nodes);
	}
	free(p_hash);
	p_hash = NULL;

	return 0;
}

int dlcache_clear()
{
	dlcache_free_hash(g_dlcache);

	return 0;
}

sw_dlfun_pt dlcache_get_dlfun_by_name(char *libname, char *funname)
{
	int	ret = 0;
	dlnode_t      *pnode = NULL;
	dlnode_t      node;
	char	keyname[MAX_LNAME_LEN];
	
	memset(&node, 0x0, sizeof(node));
	memset(keyname, 0x0, sizeof(keyname));
	sprintf(keyname, "%s%s", libname, funname);

	pnode = hash_lookup_entry(g_dlcache, keyname);
	if (pnode != NULL)
	{
		/***
		pub_log_info("[%s][%d] [%s][%s] already cached!",
			__FILE__, __LINE__, libname, funname);
		***/
		return pnode->fun;
	}

	pnode = &node;
	memcpy(pnode->libname, libname, sizeof(pnode->libname) - 1);
	memcpy(pnode->funname, funname, sizeof(pnode->funname) - 1);

	pnode->handle = (void *)dlopen(libname, RTLD_LAZY | RTLD_GLOBAL);
	if (pnode->handle == NULL)
	{
		pub_log_error("[%s][%d] dlopen [%s] error, error:[%s]",
			__FILE__, __LINE__, libname, dlerror());
		return NULL;
	}
	
	pnode->fun = (sw_dlfun_pt)dlsym(pnode->handle, funname);
	if (pnode->fun == NULL)
	{
		pub_log_error("[%s][%d] dlsym [%s][%s] error, error:[%s]",
			__FILE__, __LINE__, libname, funname, dlerror());
		return NULL;
	}

	ret = hash_add_entry(g_dlcache, keyname, pnode);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] add var error!", __FILE__, __LINE__);
		return NULL;
	}

	return pnode->fun;
}

int svc_check_child_status(sw_svc_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	int	err_svrcnt = 0;
	int	enabled = 0;

	for (i = 0; i < cycle->svrs->head.svr_cnt; i++)
	{
		if (cycle->svrs->svr[i].status != 1)
		{
			continue;
		}
		enabled++;
	
		ret = procs_check_svr_status(cycle->svrs->svr[i].svr_name, cycle->prdt);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Check svr [%s] error!",
				__FILE__, __LINE__, cycle->svrs->svr[i].svr_name);
			err_svrcnt++;
		}
	}
	if (err_svrcnt > 0)
	{
		return SW_ERROR;
	}
	
	return SW_OK;
}

