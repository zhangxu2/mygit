#include "pub_db.h"
#include <errno.h>
#include <syslog.h>
#include "pub_xml.h"
#include "pub_buf.h"

#include "db_intern_prot.h"

/* 数据库操作对象 */
static db_cycle_t g_db_obj;

/* 数据库回调函数指针 */
static const db_cb_ptr_t *g_db_cb_ptr = &g_db_obj.access.cbfunc.func;

#if defined(__DB_CONNECT_POOL__)
/* 是否支持连接池 */
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
 **函数名称: pub_db_init
 **功    能: 初始化数据库配置信息
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **      1. 加载数据库配置文件
 **      2. 提取配置文件中的有效信息
 **      3. 根据数据库类型，设置回调函数
 **      4. 创建数据库连接池
 **注意事项: 
 **作    者: # Zhanglei & Qifeng.zou # 2012.08.21 #
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
 **函数名称: pub_db_release
 **功    能: 释放数据库操作的所有数据
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Zhanglei & Qifeng.zou # 2012.08.21 #
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
 **函数名称: pub_db_open
 **功    能: 连接数据库
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Zhanglei & Qifeng.zou # 2012.08.21 #
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
 **函数名称: pub_db_close
 **功    能: 关闭数据库的连接
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou # 2012.08.21 #
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
 **函数名称: pub_db_commit
 **功    能: 提交事务操作
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_commit(void)
{
    return g_db_cb_ptr->db_commit(g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_rollback
 **功    能: 回滚事务操作
 **输入参数:
 **      obj: 数据库操作对象
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_rollback(void)
{
    return g_db_cb_ptr->db_rollback(g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_nquery
 **功    能: 执行非查找操作
 **输入参数:
 **      sql: 序执行的SQL语句
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int pub_db_nquery(const char *sql)
{
    return g_db_cb_ptr->db_nquery(sql, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_exec
 **功    能: 执行指定的SQL语句命令(扩展)
 **输入参数:
 **      cmd: SQL命令类型
 **      table: 表名
 **      param1: SQL参数列表1
 **      param2: SQL参数列表2
 **输出参数:
 **返    回: 返回值的含义随cmd的不同而不同
 **实现描述: 
 **注意事项: 
 **      1. 列表1和列表2所代表的含义随着CMD的命令而不同
 **      2. 返回值的含义随cmd的不同而不同
 **作    者: # Qifeng.zou  # 2012.08.27 #
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
 **函数名称: pub_db_update_by_rowid
 **功    能: 通过rowid更新数据库数据
 **输入参数:
 **      sql: UPDATE [tablename] SET [colname=value] [, ...]
 **      rowid: 行号
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **     UPDATE student
 **         SET id='0001', name='zouqifeng'
 **         WHERE rowid=x‘0A000000000083C0000’等类似语句
 **作    者: # Qifeng.zou  # 2013.08.20 #
 ******************************************************************************/
int pub_db_update_by_rowid(const char *_sql, const char *rowid)
{
    return g_db_cb_ptr->db_update_by_rowid(_sql, rowid, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_mquery
 **功    能: 执行查找操作(多条查询结果)
 **输入参数:
 **      sql: 序执行的SQL语句
 **      rows: 每次返回的行数
 **输出参数:
 **返    回: 返回列的数据
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
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
 **函数名称: pub_db_squery
 **功    能: 执行返回零行或一行的查找语句
 **输入参数:
 **      sql: 序执行的SQL语句
 **输出参数:
 **返    回: 返回列的数据
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_squery(const char *sql)
{
    return g_db_cb_ptr->db_squery(sql, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_fetch
 **功    能: 获取一条数据
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
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
 **函数名称: pub_db_cclose
 **功    能: 关闭游标
 **输入参数:
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
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
 **函数名称: pub_db_get_data
 **功    能: 获取一列数据
 **输入参数:
 **      rowno: 行号(取值范围: 1~X)
 **      colno: 列号(取值范围: 1~Y)
 **输出参数:
 **返    回: 返回获取的数据
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
char *pub_db_get_data(const char *alias, int rowno, int colno)
{
    return g_db_cb_ptr->db_get_data(alias, rowno, colno, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_get_data_by_name
 **功    能: 获取一列数据
 **输入参数:
 **      rowno: 行号(取值范围: 1~N)
 **      name: Column name 
 **输出参数:
 **返    回: 返回获取的数据
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.27 #
 ******************************************************************************/
char *pub_db_get_data_by_name(const char *alias, int rowno, const char *name)
{
    return g_db_cb_ptr->db_get_data_by_name(alias, rowno, name, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_get_data_and_name
 **功    能: 获取一列数据及列名
 **输入参数:
 **     rowno: 行号(取值范围: 1~X)
 **     colno: 列号(取值范围: 1~Y)
 **     size: 列名缓存尺寸
 **输出参数:
 **     name: 存放获取的列名
 **返    回: 返回获取的数据
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou  # 2013.07.02 #
 ******************************************************************************/
char *pub_db_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size)
{
    return g_db_cb_ptr->db_get_data_and_name(alias, rowno, colno, name, size, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_get_type
 **功    能: 获取指定列的数据类型
 **输入参数:
 **      colno: 列号
 **输出参数:
 **返    回: 数据类型
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei & Qifeng.zou  # 2012.08.21 #
 ******************************************************************************/
int pub_db_get_type(const char *alias, int colno)
{
    return g_db_cb_ptr->db_get_type(alias, colno, g_db_obj.access.cntx);
}

/******************************************************************************
 **函数名称: pub_db_set_alias
 **功    能: 设置别名
 **输入参数:
 **     name: 别名
 **     value: 别名值
 **     length: 别名值长度
 **输出参数:
 **返    回: 0:success !0:false
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
int pub_db_set_alias(const char *name, const void *value, int length)
{
    return g_db_cb_ptr->db_set_alias(g_db_obj.access.cntx, name, value, length);
}

/******************************************************************************
 **函数名称: pub_db_get_alias
 **功    能: 通过别名获取值
 **输入参数:
 **      name: 别名
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
const void *pub_db_get_alias(const char *name)
{
    return g_db_cb_ptr->db_get_alias(g_db_obj.access.cntx, name);
}

/******************************************************************************
 **函数名称: pub_db_delete_alias
 **功    能: 删除指定别名
 **输入参数:
 **      name: 别名
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou  # 2013.08.13 #
 ******************************************************************************/
int pub_db_delete_alias(const char *name)
{
    return g_db_cb_ptr->db_delete_alias(g_db_obj.access.cntx, name);
}

/******************************************************************************
 **函数名称: pub_db_delete_all_alias
 **功    能: 删除所有别名
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou  # 2013.08.13 #
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
		pub_log_error("[%s][%d] 未配置DATABASE!", __FILE__, __LINE__);
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
