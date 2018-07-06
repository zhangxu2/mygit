#include "pub_db.h"
#include <errno.h>
#include <syslog.h>
#include "pub_xml.h"
#include "pub_buf.h"

#include "db_intern_prot.h"

/* ���ݿ�������� */
static db_cycle_t g_db_obj;

/* ���ݿ�ص�����ָ�� */
static const db_cb_ptr_t *g_db_cb_ptr = &g_db_obj.access.cbfunc.func;

#if defined(__DB_CONNECT_POOL__)
/* �Ƿ�֧�����ӳ� */
#define db_is_support_pool() ((true == g_db_obj.source.ispool)?1:0)
#endif /*__DB_CONNECT_POOL__*/


#if defined(__NODB__)
int pub_db_init(sw_dbcfg_t *db){ return 0; }
int pub_db_open(void){ return 0; }
int pub_db_close(void){  return 0; }
int pub_db_commit(void){ return 0;}
int pub_db_rollback(void){ return 0; }
int pub_db_nquery(const char *sql) { return 0; }
int pub_db_exec(db_sql_cmd_t cmd, const char *table,
        const char *param1, const char *param2) { return 0; }
int pub_db_update_by_rowid(const char *_sql, const char *rowid){ return 0; }
int pub_db_mquery(const char *alias, const char *sql, int rows){ return 0; }
int pub_db_squery(const char *sql){ return 0; }
int pub_db_fetch(void){ return 0; }
int pub_db_cclose(const char *alias){ return 0; }
char *pub_db_get_data(const char *alias, int rowno, int colno){ return NULL; }
char *pub_db_get_data_by_name(const char *alias, int rowno, const char *name){ return NULL; }
char *pub_db_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size){ return NULL; }
int pub_db_get_type(const char *alias, int colno){ return DB_DTYPE_UNKNOW; }
int pub_db_set_alias(const char *name, const void *value, int length){ return 0; }
const void *pub_db_get_alias(const char *name){ return NULL; }
int pub_db_delete_alias(const char *name){ return 0; }
int pub_db_delete_all_alias(void){ return 0; }
int pub_db_del_all_conn() {return 0;}
int pub_db_mfetch(const char *alias) {return 0;}
int pub_db_conn_detect(){ return 0; }

#else /*__NODB__*/
/******************************************************************************
 **��������: pub_db_init
 **��    ��: ��ʼ�����ݿ�������Ϣ
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **      1. �������ݿ������ļ�
 **      2. ��ȡ�����ļ��е���Ч��Ϣ
 **      3. �������ݿ����ͣ����ûص�����
 **      4. �������ݿ����ӳ�
 **ע������: 
 **��    ��: # Zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_init(sw_dbcfg_t *cfg)
{
    int ret = -1;
    db_cycle_t *obj = &g_db_obj;
    db_data_src_t *source = &obj->source;

    memset(obj, 0, sizeof(db_cycle_t));

    do
    {
        ret = db_get_cfg_info(cfg, source);
        if(ret < 0)
        {
            pub_log_error("[%s][%d] Get configuration failed!", __FILE__, __LINE__);
            break;
        }

        ret = db_set_cb_func(source->dbtype, cfg->mode, obj);
        if(ret < 0)
        {
            pub_log_error("[%s][%d] Set callback failed!", __FILE__, __LINE__);
            break;
        }

        return 0;
    }while(0);

    db_reset_cb_ptr(obj);

    return ret;
}

/******************************************************************************
 **��������: pub_db_release
 **��    ��: �ͷ����ݿ��������������
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_release(void)
{
    db_cycle_t *obj = &g_db_obj;

#if defined(__DB_CONNECT_POOL__)
    if(db_is_support_pool())
    {
        ret = g_db_cb_ptr->db_close_pool(&obj->access.conn);
        if(ret < 0)
        {
            return -1;
        }
    }
#endif /*__DB_CONNECT_POOL__*/

    db_cfg_release(obj);

    return 0;
}

/******************************************************************************
 **��������: pub_db_open
 **��    ��: �������ݿ�
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_open(void)
{
#if defined(__DB_CONNECT_POOL__)
    if(db_is_support_pool())
    {
        return g_db_cb_ptr->db_get_conn(&g_db_obj.source, &g_db_obj.access.cntx);
    }
    else
#endif /*__DB_CONNECT_POOL__*/
    {
        return g_db_cb_ptr->db_open(&g_db_obj.source, &g_db_obj.access.cntx);
    }
}

/******************************************************************************
 **��������: pub_db_close
 **��    ��: �ر����ݿ������
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_close(void)
{
#if defined(__DB_CONNECT_POOL__)
    if(db_is_support_pool())
    {
        return g_db_cb_ptr->db_put_conn(&g_db_obj.access.cntx);
    }
    else
#endif /*__DB_CONNECT_POOL__*/
    {
        return g_db_cb_ptr->db_close(g_db_obj.access.cntx);
    }
}

/******************************************************************************
 **��������: pub_db_commit
 **��    ��: �ύ�������
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_commit(void)
{
    return g_db_cb_ptr->db_commit(g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_rollback
 **��    ��: �ع��������
 **�������:
 **      obj: ���ݿ��������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_rollback(void)
{
    return g_db_cb_ptr->db_rollback(g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_nquery
 **��    ��: ִ�зǲ��Ҳ���
 **�������:
 **      sql: ��ִ�е�SQL���
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_nquery(const char *sql)
{
    return g_db_cb_ptr->db_nquery(sql, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_exec
 **��    ��: ִ��ָ����SQL�������(��չ)
 **�������:
 **      cmd: SQL��������
 **      table: ����
 **      param1: SQL�����б�1
 **      param2: SQL�����б�2
 **�������:
 **��    ��: ����ֵ�ĺ�����cmd�Ĳ�ͬ����ͬ
 **ʵ������: 
 **ע������: 
 **      1. �б�1���б�2������ĺ�������CMD���������ͬ
 **      2. ����ֵ�ĺ�����cmd�Ĳ�ͬ����ͬ
 **��    ��: # Qifeng.zou  # 2012.08.27 #
 ******************************************************************************/
int pub_db_exec(db_sql_cmd_t cmd, const char *table, const char *param1, const char *param2)
{
    char sql[DB_SQL_MAX_LEN] = {0};

    switch(cmd)
    {
        case DB_SQL_INSERT:
        {
            if(NULL == param2)
            {
                snprintf(sql, sizeof(sql), "INSERT INTO %s VALUES(%s)", table, param1);
            }
            else
            {
                snprintf(sql, sizeof(sql),
                    "INSERT INTO %s(%s) VALUES(%s)", table, param1, param2);
            }
            
            return g_db_cb_ptr->db_nquery(sql, g_db_obj.access.cntx);
        }
        case DB_SQL_SELECT:
        {
            if(NULL == param2)
            {
                snprintf(sql, sizeof(sql), "SELECT %s FROM %s", param1, table);
            }
            else
            {
                snprintf(sql, sizeof(sql),
                    "SELECT %s FROM %s WHERE %s", param1, table, param2);
            }
            
            return g_db_cb_ptr->db_mquery(NULL, sql, DB_FETCH_MAX_ROWS, g_db_obj.access.cntx);
        }
        case DB_SQL_UPDATE:
        {
            snprintf(sql, sizeof(sql), "UPDATE %s SET %s WHERE %s", table, param1, param2);
            return g_db_cb_ptr->db_nquery(sql, g_db_obj.access.cntx);
        }
        case DB_SQL_DELETE:
        {
            snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE %s", table, param1);
            return g_db_cb_ptr->db_nquery(sql, g_db_obj.access.cntx);
        }
        case DB_SQL_DROP:
        {
            snprintf(sql, sizeof(sql), "DROP TABLE %s", table);
            return g_db_cb_ptr->db_nquery(sql, g_db_obj.access.cntx);
        }
        case DB_SQL_UPDATE_DEC:
        {
            if(NULL == param2)
            {
                snprintf(sql,
                    sizeof(sql), "SELECT %s FROM %s FOR UPDATE", param1, table);
            }
            else
            {
                snprintf(sql, sizeof(sql),
                    "SELECT %s FROM %s WHERE %s FOR UPDATE", param1, table, param2);
            }

            return g_db_cb_ptr->db_mquery(NULL, sql, 1, g_db_obj.access.cntx);
        }
        case DB_SQL_FETCH:
        case DB_SQL_UPDATE_FET:
        {
            return g_db_cb_ptr->db_fetch(g_db_obj.access.cntx);
        }
        case DB_SQL_UPDATE_UPD:
        {
            snprintf(sql, sizeof(sql), "UPDATE %s SET %s", table, param1);
            return g_db_cb_ptr->db_update_by_rowid(sql, param2, g_db_obj.access.cntx);
        }
        case DB_SQL_CLOSE:
        case DB_SQL_UPDATE_CLO:
        {
            return g_db_cb_ptr->db_cclose(NULL, g_db_obj.access.cntx);
        }
        default:
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Unknown command type![%d]", __FILE__, __LINE__, cmd);
            return -1;
        }
    }

    return 0;
}

/******************************************************************************
 **��������: pub_db_update_by_rowid
 **��    ��: ͨ��rowid�������ݿ�����
 **�������:
 **      sql: UPDATE [tablename] SET [colname=value] [, ...]
 **      rowid: �к�
 **�������:
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **     UPDATE student
 **         SET id='0001', name='zouqifeng'
 **         WHERE rowid=x��0A000000000083C0000�����������
 **��    ��: # Qifeng.zou  # 2013.08.20 #
 ******************************************************************************/
int pub_db_update_by_rowid(const char *_sql, const char *rowid)
{
    return g_db_cb_ptr->db_update_by_rowid(_sql, rowid, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_mquery
 **��    ��: ִ�в��Ҳ���(������ѯ���)
 **�������:
 **      sql: ��ִ�е�SQL���
 **      rows: ÿ�η��ص�����
 **�������:
 **��    ��: �����е�����
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_mquery(const char *alias, const char *sql, int rows)
{
    if((rows < 1) || (rows > DB_FETCH_MAX_ROWS))
    {
        rows = DB_FETCH_MAX_ROWS;
    }
    
    return g_db_cb_ptr->db_mquery(alias, sql, rows, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_squery
 **��    ��: ִ�з������л�һ�еĲ������
 **�������:
 **      sql: ��ִ�е�SQL���
 **�������:
 **��    ��: �����е�����
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_squery(const char *sql)
{
    return g_db_cb_ptr->db_squery(sql, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_fetch
 **��    ��: ��ȡһ������
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_fetch(void)
{
    return g_db_cb_ptr->db_fetch(g_db_obj.access.cntx);
}

int pub_db_mfetch(const char *alias)
{
    return g_db_cb_ptr->db_mfetch(alias, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_cclose
 **��    ��: �ر��α�
 **�������:
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_cclose(const char *alias)
{
    return g_db_cb_ptr->db_cclose(alias, g_db_obj.access.cntx);
}

int pub_db_del_all_conn()
{
    return g_db_cb_ptr->db_del_all_conn(g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_get_data
 **��    ��: ��ȡһ������
 **�������:
 **      rowno: �к�(ȡֵ��Χ: 1~X)
 **      colno: �к�(ȡֵ��Χ: 1~Y)
 **�������:
 **��    ��: ���ػ�ȡ������
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
char *pub_db_get_data(const char *alias, int rowno, int colno)
{
    return g_db_cb_ptr->db_get_data(alias, rowno, colno, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_get_data_by_name
 **��    ��: ��ȡһ������
 **�������:
 **      rowno: �к�(ȡֵ��Χ: 1~N)
 **      name: Column name 
 **�������:
 **��    ��: ���ػ�ȡ������
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.27 #
 ******************************************************************************/
char *pub_db_get_data_by_name(const char *alias, int rowno, const char *name)
{
    return g_db_cb_ptr->db_get_data_by_name(alias, rowno, name, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_get_data_and_name
 **��    ��: ��ȡһ�����ݼ�����
 **�������:
 **     rowno: �к�(ȡֵ��Χ: 1~X)
 **     colno: �к�(ȡֵ��Χ: 1~Y)
 **     size: ��������ߴ�
 **�������:
 **     name: ��Ż�ȡ������
 **��    ��: ���ػ�ȡ������
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou  # 2013.07.02 #
 ******************************************************************************/
char *pub_db_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size)
{
    return g_db_cb_ptr->db_get_data_and_name(alias, rowno, colno, name, size, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_get_type
 **��    ��: ��ȡָ���е���������
 **�������:
 **      colno: �к�
 **�������:
 **��    ��: ��������
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_get_type(const char *alias, int colno)
{
    return g_db_cb_ptr->db_get_type(alias, colno, g_db_obj.access.cntx);
}

/******************************************************************************
 **��������: pub_db_set_alias
 **��    ��: ���ñ���
 **�������:
 **     name: ����
 **     value: ����ֵ
 **     length: ����ֵ����
 **�������:
 **��    ��: 0:success !0:false
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
int pub_db_set_alias(const char *name, const void *value, int length)
{
    return g_db_cb_ptr->db_set_alias(g_db_obj.access.cntx, name, value, length);
}

/******************************************************************************
 **��������: pub_db_get_alias
 **��    ��: ͨ��������ȡֵ
 **�������:
 **      name: ����
 **�������:
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
const void *pub_db_get_alias(const char *name)
{
    return g_db_cb_ptr->db_get_alias(g_db_obj.access.cntx, name);
}

/******************************************************************************
 **��������: pub_db_delete_alias
 **��    ��: ɾ��ָ������
 **�������:
 **      name: ����
 **�������:
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
int pub_db_delete_alias(const char *name)
{
    return g_db_cb_ptr->db_delete_alias(g_db_obj.access.cntx, name);
}

/******************************************************************************
 **��������: pub_db_delete_all_alias
 **��    ��: ɾ�����б���
 **�������: NONE
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
int pub_db_delete_all_alias(void)
{
    return g_db_cb_ptr->db_delete_all_alias(g_db_obj.access.cntx);
}

int pub_db_conn()
{
	int	ret = 0;
	char	xmlname[128];
	char output[100];
	sw_dbcfg_t	dbcfg;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	memset(xmlname, 0x0, sizeof(xmlname));
	memset(&dbcfg, 0x0, sizeof(dbcfg));
	sprintf(xmlname, "%s/cfg/dbconfig.xml", getenv("SWWORK"));
	xml = cfg_read_xml(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
			__FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}
	
	node = pub_xml_locnode(xml, ".DBCONFIG.DATABASE");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] δ����DATABASE!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		return SW_ERROR;
	}
	xml->current = node;
	node = pub_xml_locnode(xml, "DBNAME");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg.dbname, node->value, sizeof(dbcfg.dbname) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBSID");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg.dbsid, node->value, sizeof(dbcfg.dbsid) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBUSER");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg.dbuser, node->value, sizeof(dbcfg.dbuser) - 1);
	}
	
	node = pub_xml_locnode(xml, "DBPASSWD");
	if (node != NULL && node->value != NULL)
	{
		if (node->value != NULL&&strlen(node->value) != 0)
		{
			if (getenv("DB_ENC") != NULL && strcasecmp(getenv("DB_ENC"), "true") == 0)
			{
				memset(output,0x00,sizeof(output));
				pub_des3_dec(node->value, output, strlen(node->value), NULL);
				strncpy(dbcfg.dbpasswd,output,sizeof(dbcfg.dbpasswd)-1);
			}
			else
			{
				strncpy(dbcfg.dbpasswd,node->value,sizeof(dbcfg.dbpasswd)-1);
			}
		}
	}
	
	node = pub_xml_locnode(xml, "DBTYPE");
	if (node != NULL && node->value != NULL)
	{
		strncpy(dbcfg.dbtype, node->value, sizeof(dbcfg.dbtype) - 1);
	}

	node = pub_xml_locnode(xml, "DBMODE");
	if (node != NULL && node->value != NULL && node->value[0] != '\0')
	{
		dbcfg.mode = 1;
	}

	pub_xml_deltree(xml);
	pub_log_debug("[%s][%d] DBNAME:[%s] DBSID:[%s] DBUSER:[%s] DBPWD:[%s] DBTYPE:[%s] DBMODE:[%d]",
		__FILE__, __LINE__, dbcfg.dbname, dbcfg.dbsid, dbcfg.dbuser, dbcfg.dbpasswd, dbcfg.dbtype, dbcfg.mode);
	ret = pub_db_init(&dbcfg);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_db_init error!", __FILE__, __LINE__);
		return -1;
	}
	
	ret = pub_db_open();
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_db_open error!", __FILE__, __LINE__);
		return -1;
	}
	pub_log_debug("[%s][%d] Connect [%s] success!", __FILE__, __LINE__, dbcfg.dbname);
	
	return 0;
}

int pub_db_get_fetrows(const char *osql)
{
	int	ret = 0;
	char	sql[2048];
	char	name[256];
	char	*ptr = NULL;
	
	memset(sql, 0x0, sizeof(sql));
	memset(name, 0x0, sizeof(name));

	if (osql == NULL || osql[0] == '\0')
	{
		pub_log_error("[%s][%d] SQL is null!", __FILE__, __LINE__);
		return -1;
	}
	sprintf(sql, "select count(*) cnt from (%s)", osql);
	ret = pub_db_squery(sql);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] Fetch [%s] error!", __FILE__, __LINE__, sql);
		return -1;
	}

	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	pub_log_info("[%s][%d] name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);
	
	return atoi(ptr);
}

int pub_db_conn_detect()
{
    return g_db_cb_ptr->db_conn_detect(g_db_obj.access.cntx);
}

#endif
