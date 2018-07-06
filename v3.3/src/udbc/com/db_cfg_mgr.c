/******************************************************************************
 *** 程序作者 : Qifeng.zou                                                  ***
 *** 开始日期 : 2012.08.21                                                  ***
 *** 结束日期 :                                                             ***
 *** 所属模块 : 数据库: 加载数据库配置信息                                  ***
 *** 程序名称 : db_cfg_mgr.c                                                ***
 *** 程序作用 : 加载数据库配置信息                                          ***
 *** 修改说明 :                                                             ***
 ******************************************************************************/ 
#include "pub_db.h"
#include "db_data_type.h"
#include "pub_cfg.h"


#if !defined(__NODB__)
/*******************************************************************************/
/* 静态函数 */
static int db_get_db_type(const char *name);
static int db_set_lib_handle(db_type_t type, int mode, db_access_info_t *access);
static void db_set_ifx_cbname(const db_cb_name_t **name);
static void db_set_ora_cbname(const db_cb_name_t **name);
static void db_set_odbc_cbname(const db_cb_name_t **name);
#define db_set_db2_cbname(name) db_set_odbc_cbname(name)
static int db_set_cb_ptr(db_cycle_t *obj);

/*******************************************************************************/

/******************************************************************************
 **函数名称: db_get_cfg_info
 **功    能: 从配置文件中提取有效信息
 **输入参数:
 **      dbcfg: 数据库配置信息
 **输出参数:
 **      source: 数据源信息
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int db_get_cfg_info(sw_dbcfg_t *dbcfg, db_data_src_t *source)
{
    source->dbtype = db_get_db_type(dbcfg->dbtype);

    switch(source->dbtype)
    {
        case DB_TYPE_ORACLE:
        {
            snprintf(source->svrname, sizeof(source->svrname), "%s", dbcfg->dbsid);
            break;
        }
        case DB_TYPE_INFORMIX:
        {
            snprintf(source->svrname,
                sizeof(source->svrname), "%s@%s", dbcfg->dbname, dbcfg->dbsid);
            break;
        }
        case DB_TYPE_DB2:
        {
            snprintf(source->svrname, sizeof(source->svrname), "%s", dbcfg->dbname);
            break;
        }
        case DB_TYPE_UNKOWN:
        {
            pub_log_error("[%s][%d] Unknown type of database!", __FILE__, __LINE__);
            return -1;
        }
    }
    
    snprintf(source->usrname, sizeof(source->usrname), "%s", dbcfg->dbuser);
    snprintf(source->passwd, sizeof(source->passwd), "%s", dbcfg->dbpasswd);

    return 0;
}

/******************************************************************************
 **函数名称: db_get_db_type
 **功    能: 通过数据库类型名，获取数据库类型
 **输入参数:
 **      name: 数据库类型名
 **输出参数:
 **返    回: 数据库类型
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
static int db_get_db_type(const char *name)
{
    if(0 == strcasecmp(name, DB_NAME_ORACLE))
    {
        return DB_TYPE_ORACLE;
    }
    else if(0 == strcasecmp(name, DB_NAME_INFORMIX))
    {
        return DB_TYPE_INFORMIX;
    }
    else if(0 == strcasecmp(name, DB_NAME_DB2))
    {
        return DB_TYPE_DB2;
    }

    return DB_TYPE_UNKOWN;
}

/******************************************************************************
 **函数名称: db_set_lib_handle
 **功    能: 通过数据库类型，设置动态库句柄
 **输入参数:
 **      type: 数据库类型
 **输出参数:
 **      cb: 存放回调函数
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
static int db_set_lib_handle(db_type_t type, int mode, db_access_info_t *access)
{
    char libpath[FILE_NAME_MAX_LEN] = {0};

    switch(type)
    {
        case DB_TYPE_ORACLE:
        {
            if (mode)
            {
                snprintf(libpath, sizeof(libpath),
                    "%s/lib/%s", getenv("SWHOME"), DB_LIB_ORACLE_PC);
            }
            else
            {
                snprintf(libpath, sizeof(libpath),
                    "%s/lib/%s", getenv("SWHOME"), DB_LIB_ORACLE);
            }
            break;
        }
        case DB_TYPE_INFORMIX:
        {
            snprintf(libpath, sizeof(libpath),
                "%s/lib/%s", getenv("SWHOME"), DB_LIB_INFORMIX);
            break;
        }
        case DB_TYPE_DB2:
        {
            snprintf(libpath, sizeof(libpath),
                "%s/lib/%s", getenv("SWHOME"), DB_LIB_DB2);
            break;
        }
        default:
        {
            pub_log_error("[%s][%d] Unknown database type![%d]", __FILE__, __LINE__, type);
            return -1;
        }
    }

    access->handle = (void *)dlopen(libpath, RTLD_LAZY|RTLD_GLOBAL);
    if(NULL == access->handle)
    {
        pub_log_error("[%s][%d] %s![%s]", __FILE__, __LINE__, dlerror(), libpath);
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: db_set_cb_func
 **功    能: 通过数据库类型，设置回调函数
 **输入参数:
 **      type: 数据库类型
 **输出参数:
 **      cb: 存放回调函数
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **      1. 打开动态库
 **      2. 设置函数名
 **      3. 获取函数地址
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
int db_set_cb_func(db_type_t type, int mode, db_cycle_t *obj)
{
    int ret = -1;

    ret = db_set_lib_handle(type, mode, &obj->access);
    if(ret < 0)
    {
        pub_log_error("[%s][%d] Set lib handle failed!", __FILE__, __LINE__);
        return ret;
    }

    switch(type)
    {
        case DB_TYPE_ORACLE:
        {
            db_set_ora_cbname((const db_cb_name_t **)&obj->access.cbfunc.name);
            break;
        }
        case DB_TYPE_INFORMIX:
        {
            db_set_ifx_cbname((const db_cb_name_t **)&obj->access.cbfunc.name);
            break;
        }
        case DB_TYPE_DB2:
        {
            db_set_db2_cbname((const db_cb_name_t **)&obj->access.cbfunc.name);
            break;
        }
        default:
        {
            pub_log_error("[%s][%d] Type of database is incorrect!", __FILE__, __LINE__);
            return -1;
        }
    }

    ret = db_set_cb_ptr(obj);
    if(ret < 0)
    {
        pub_log_error("[%s][%d] Set callback failed!", __FILE__, __LINE__);
        return ret;
    }

    return 0;
}

/******************************************************************************
 **函数名称: db_set_ora_cbname
 **功    能: 设置oracle的数据库操作的函数名
 **输入参数:
 **      name: 函数名
 **输出参数:
 **返    回: VOID
 **实现描述: 
 **注意事项: 
 **作    者: # zhanglei # 2012.08.21 #
 ******************************************************************************/
static void db_set_ora_cbname(const db_cb_name_t **name)
{
    static const db_cb_name_t ora_cb_name = 
    {
    #if defined(__DB_CONNECT_POOL__)
        "db_ora_open_pool",         /* 创建连接池 */
        "db_ora_close_pool",        /* 关闭连接池 */
    #endif /*__DB_CONNECT_POOL__*/
    
        "db_ora_open",              /* 连接数据库 */
        "db_ora_close",             /* 关闭数据库连接 */
        
        "db_ora_mul_query",         /* 执行查询语句 */
        "db_ora_single_query",      /* 执行只返回零条或一条结果的查询语句 */
        "db_ora_non_query",         /* 执行非查询SQL语句 */
        "db_ora_update_by_rowid",   /* 通过ROWID更新数据 */
        
        "db_ora_fetch",             /* 取游标值 */
        "db_ora_mfetch",             /* 取游标值 */
        "db_ora_clo_fetch",         /* 关闭游标 */
	"db_ora_del_all_conn",		/* 删除链接 */

        "db_ora_commit",            /* 提交事务 */
        "db_ora_rollback",          /* 回滚事务 */
        
        "db_ora_get_col_value",     /* 获取列值[通过坐标] */
        "db_ora_get_value_byname",  /* 获取列值[通过列名] */
        "db_ora_get_data_and_name", /* 获取列值和列名 */
        "db_ora_get_col_type",      /* 获取列类型 */
        
        "db_ora_set_alias",         /* 设置别名值 */
        "db_ora_get_alias",         /* 获取别名值 */
        "db_ora_delete_alias",      /* 删除指定别名 */
        "db_ora_delete_all_alias",   /* 删除所有别名 */
        "db_ora_conn_detect"	     /*探测数据库连接是否正常*/	
    };

    *name = (const db_cb_name_t *)&ora_cb_name;
}

/******************************************************************************
 **函数名称: db_set_ifx_cbname
 **功    能: 设置informix的数据库操作的函数名
 **输入参数:
 **      name: 函数名
 **输出参数:
 **返    回: VOID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
static void db_set_ifx_cbname(const db_cb_name_t **name)
{
    static const db_cb_name_t ifx_cb_name = 
    {
    #if defined(__DB_CONNECT_POOL__)
        "db_ifx_pcreat",            /* 创建连接池 */
        "db_ifx_pclose",            /* 关闭连接池 */
    #endif /*__DB_CONNECT_POOL__*/
    
        "db_ifx_open",              /* 连接数据库 */
        "db_ifx_close",             /* 关闭数据库连接 */

        "db_ifx_mquery",            /* 执行查询语句 */
        "db_ifx_squery",            /* 执行只返回零条或一条结果的查询语句 */
        "db_ifx_nquery",            /* 执行非查询SQL语句 */
        "db_ifx_update_by_rowid",   /* 通过ROWID更新数据 */

        "db_ifx_fetch",             /* 取游标值 */
        "db_ifx_mfetch",             /* 取游标值 */
        "db_ifx_cclose",            /* 关闭游标 */
	"db_ifx_del_all_conn",      /* 删除链接 */
        "db_ifx_commit",            /* 提交事务 */
        "db_ifx_rollback",          /* 回滚事务 */
        
        "db_ifx_get_data_by_idx",   /* 获取列值[通过坐标] */
        "db_ifx_get_data_by_name",  /* 获取列值[通过列名] */
        "db_ifx_get_data_and_name", /* 获取列值和列名 */
        "db_ifx_get_data_type",     /* 获取列类型 */
        
        "db_ifx_set_alias",         /* 设置别名值 */
        "db_ifx_get_alias",         /* 获取别名值 */
        "db_ifx_delete_alias",      /* 删除指定别名 */
        "db_ifx_delete_all_alias",   /* 删除所有别名 */
        "db_ifx_conn_detect"	     /*探测数据库连接是否正常*/
    };

    *name = (const db_cb_name_t *)&ifx_cb_name;
}

/******************************************************************************
 **函数名称: db_set_odbc_cbname
 **功    能: 设置ODBC的数据库操作的函数名
 **输入参数:
 **      name: 函数名
 **输出参数:
 **返    回: VOID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.01 #
 ******************************************************************************/
static void db_set_odbc_cbname(const db_cb_name_t **name)
{
    static const db_cb_name_t odbc_cb_name = 
    {
    #if defined(__DB_CONNECT_POOL__)
        "db_odbc_pcreat",           /* 创建连接池 */
        "db_odbc_pclose",           /* 关闭连接池 */
    #endif /*__DB_CONNECT_POOL__*/
    
        "db_odbc_open",             /* 连接数据库 */
        "db_odbc_close",            /* 关闭数据库连接 */
        
        "db_odbc_mquery",           /* 执行查询语句 */
        "db_odbc_squery",           /* 执行只返回零条或一条结果的查询语句 */
        "db_odbc_nquery",           /* 执行非查询SQL语句 */
        "db_odbc_update_by_rowid",  /* 通过ROWID更新数据 */

        "db_odbc_fetch",            /* 取游标值 */
        "db_odbc_mfetch",            /* 取游标值 */
        "db_odbc_cclose",           /* 关闭游标 */
	"db_odbc_del_all_conn",		/* 删除链接 */

        "db_odbc_commit",           /* 提交事务 */
        "db_odbc_rollback",         /* 回滚事务 */
        
        "db_odbc_get_data_by_idx",  /* 获取列值[通过坐标] */
        "db_odbc_get_data_by_name", /* 获取列值[通过列名] */
        "db_odbc_get_data_and_name",/* 获取列值和列名 */
        "db_odbc_get_data_type",    /* 获取列类型 */
        
        "db_odbc_set_alias",        /* 设置变量值 */
        "db_odbc_get_alias",        /* 获取变量值 */
        "db_odbc_delete_alias",     /* 删除指定变量 */
        "db_odbc_delete_all_alias",  /* 删除所有变量 */
        "db_odbc_conn_detect"	     /*探测数据库连接是否正常*/
    };

    *name = (const db_cb_name_t *)&odbc_cb_name;
}

/******************************************************************************
 **函数名称: db_set_cb_ptr
 **功    能: 设置数据库操作的回调函数指针
 **输入参数:
 **      func: 函数信息
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
static int db_set_cb_ptr(db_cycle_t *obj)
{
    void *handle = obj->access.handle;
    const db_cb_name_t *name = obj->access.cbfunc.name;    
    db_cb_ptr_t *func_ptr = &obj->access.cbfunc.func;

    /* 设置db_open */
    func_ptr->db_open = (db_open_func_t)dlsym(handle, name->open);
    if(NULL == func_ptr->db_open)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_close */
    func_ptr->db_close = (db_comm_func_t)dlsym(handle, name->close);
    if(NULL == func_ptr->db_close)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_commit */
    func_ptr->db_commit = (db_comm_func_t)dlsym(handle, name->commit);
    if(NULL == func_ptr->db_commit)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_rollback */
    func_ptr->db_rollback = (db_comm_func_t)dlsym(handle, name->rollback);
    if(NULL == func_ptr->db_rollback)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

#if defined(__DB_CONNECT_POOL__)
    /* 设置db_pool */
    func_ptr->db_pool = (db_pool_func_t)dlsym(handle, name->pool);
    if(NULL == func_ptr->db_pool)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }
#endif /*__DB_CONNECT_POOL__*/

#if defined(__DB_CONNECT_POOL__)
    /* 设置db_close_pool */
    func_ptr->db_close_pool = (db_comm_func_t)dlsym(handle, name->close_pool);
    if(NULL == func_ptr->db_close_pool)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }
#endif /*__DB_CONNECT_POOL__*/

    /* 设置db_mquery */
    func_ptr->db_mquery = (db_mquery_func_t)dlsym(handle, name->mquery);
    if(NULL == func_ptr->db_mquery)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_squery */
    func_ptr->db_squery = (db_squery_func_t)dlsym(handle, name->squery);
    if(NULL == func_ptr->db_squery)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_nquery */
    func_ptr->db_nquery = (db_nquery_func_t)dlsym(handle, name->nquery);
    if(NULL == func_ptr->db_nquery)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_update_by_rowid */
    func_ptr->db_update_by_rowid =
            (db_update_by_rowid_func_t)dlsym(handle, name->update_by_rowid);
    if(NULL == func_ptr->db_update_by_rowid)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_fetch */
    func_ptr->db_fetch = (db_comm_func_t)dlsym(handle , name->fetch);
    if(NULL == func_ptr->db_fetch)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_mfetch */
    func_ptr->db_mfetch = (db_mfetch_func_t)dlsym(handle , name->mfetch);
    if(NULL == func_ptr->db_mfetch)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_cclose */
    func_ptr->db_cclose = (db_clo_fet_func_t)dlsym(handle, name->cclose);
    if(NULL == func_ptr->db_cclose)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_del_all_conn */
    func_ptr->db_del_all_conn = (db_del_all_conn_func_t)dlsym(handle, name->del_all_conn);
    if(NULL == func_ptr->db_del_all_conn)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_get_data */
    func_ptr->db_get_data = (db_get_data_func_t)dlsym(handle, name->get_data);
    if(NULL == func_ptr->db_get_data)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_get_data_by_name */
    func_ptr->db_get_data_by_name =
    (db_get_data_by_name_func_t)dlsym(handle, name->get_data_by_name);
    if(NULL == func_ptr->db_get_data_by_name)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_get_data_and_name */
    func_ptr->db_get_data_and_name = 
    (db_get_data_and_name_func_t)dlsym(handle, name->get_data_and_name);
    if(NULL == func_ptr->db_get_data_and_name)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_get_type */
    func_ptr->db_get_type = (db_get_type_func_t)dlsym(handle, name->get_type);
    if(NULL == func_ptr->db_get_type)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_set_alias */
    func_ptr->db_set_alias = (db_set_alias_func_t)dlsym(handle, name->set_alias);
    if(NULL == func_ptr->db_set_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_get_alias */
    func_ptr->db_get_alias = (db_get_alias_func_t)dlsym(handle, name->get_alias);
    if(NULL == func_ptr->db_get_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_delete_alias */
    func_ptr->db_delete_alias = (db_delete_alias_func_t)dlsym(handle, name->delete_alias);
    if(NULL == func_ptr->db_delete_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* 设置db_delete_all_alias */
    func_ptr->db_delete_all_alias = (db_delete_all_alias_func_t)dlsym(handle, name->delete_all_alias);
    if(NULL == func_ptr->db_delete_all_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    func_ptr->db_conn_detect = (db_conn_detect_func_t)dlsym(handle, name->conn_detect);
    if (NULL == func_ptr->db_conn_detect)
    {
    	pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }
    
    return 0;
}

/******************************************************************************
 **函数名称: db_cfg_release
 **功    能: 释放数据库配置信息
 **输入参数:
 **      obj: 数据库对象
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.05 #
 ******************************************************************************/
void db_cfg_release(db_cycle_t *obj)
{
    db_access_info_t *access = &obj->access;

    /*dlclose(access->handle);*/
    access->handle = NULL;
}

#if defined(__DB_CONNECT_POOL__)
/******************************************************************************
 **函数名称: db_dummy_pool
 **功    能: 创建连接池(空函数)
 **输入参数:
 **     source: 数据源
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
int db_dummy_pool(void *source, void **cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_close_pool
 **功    能: 关闭连接池(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
int db_dummy_close_pool(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}
#endif /*__DB_CONNECT_POOL__*/

/******************************************************************************
 **函数名称: db_dummy_open
 **功    能: 打开数据库失败(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_open(void *source, void **cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_close
 **功    能: 打开数据库失败(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_close(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_mquery
 **功    能: 多条查询(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_mquery(const char *alias, const char *sql, int rows, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_squery
 **功    能: 单条查询(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_squery(const char *sql,void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_nquery
 **功    能: 非插入操作
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_nquery(const char *sql, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_update_by_rowid
 **功    能: 通过行号更新数据库(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_update_by_rowid(const char *_sql, const char *rowid, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_fetch
 **功    能: 取数据(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_fetch(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_cclose
 **功    能: 关闭游标(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_cclose(const char *alias, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_commit
 **功    能: 提交事务(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_commit(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_rollback
 **功    能: 回滚事务(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_rollback(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_get_data
 **功    能: 通过行列号取数据(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static char *db_dummy_get_data(const char *alias, int row, int col, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **函数名称: db_dummy_get_data_by_name
 **功    能: 通过列名获取数据(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static char *db_dummy_get_data_by_name(const char *alias, int row, const char *name, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **函数名称: db_dummy_mquery
 **功    能: 多条查询(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static char *db_dummy_get_data_and_name(const char *alias, int row, int col, char *name, int size, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **函数名称: db_dummy_mquery
 **功    能: 多条查询(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_get_type(const char *alias, int col, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_set_alias
 **功    能: 设置别名(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_set_alias(void *cntx, const char *name, const void *value, int length)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_get_alias
 **功    能: 通过别名取值(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static void *db_dummy_get_alias(void *cntx, const char *name)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **函数名称: db_dummy_delete_alias
 **功    能: 删除指定别名(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_delete_alias(void *cntx, const char *name)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **函数名称: db_dummy_delete_all_alias
 **功    能: 删除所有别名(空函数)
 **输入参数:
 **     cntx: 连接上下文
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_delete_all_alias(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

static int db_dummy_conn_detect(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;		
}

/******************************************************************************
 **函数名称: db_reset_cb_ptr
 **功    能: 重置数据库操作的回调函数指针
 **输入参数:
 **      func: 函数信息
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
int db_reset_cb_ptr(db_cycle_t *obj)
{
    db_cb_ptr_t *func_ptr = &obj->access.cbfunc.func;

#if defined(__DB_CONNECT_POOL__)
    /* 设置db_pool */
    func_ptr->db_pool = db_dummy_pool;

    /* 设置db_close_pool */
    func_ptr->db_close_pool = db_dummy_close_pool;
#endif /*__DB_CONNECT_POOL__*/

    /* 设置db_open */
    func_ptr->db_open = db_dummy_open;

    /* 设置db_close */
    func_ptr->db_close = db_dummy_close;

    /* 设置db_mquery */
    func_ptr->db_mquery = db_dummy_mquery;

    /* 设置db_squery */
    func_ptr->db_squery = db_dummy_squery;

    /* 设置db_nquery */
    func_ptr->db_nquery = db_dummy_nquery;

    /* 设置db_update_by_rowid */
    func_ptr->db_update_by_rowid = db_dummy_update_by_rowid;

    /* 设置db_fetch */
    func_ptr->db_fetch = db_dummy_fetch;

    /* 设置db_cclose */
    func_ptr->db_cclose = db_dummy_cclose;

    /* 设置db_commit */
    func_ptr->db_commit = db_dummy_commit;

    /* 设置db_rollback */
    func_ptr->db_rollback = db_dummy_rollback;

    /* 设置db_get_data */
    func_ptr->db_get_data = db_dummy_get_data;

    /* 设置db_get_data_by_name */
    func_ptr->db_get_data_by_name = db_dummy_get_data_by_name;

    /* 设置db_get_data_and_name */
    func_ptr->db_get_data_and_name = db_dummy_get_data_and_name;

    /* 设置db_get_type */
    func_ptr->db_get_type = db_dummy_get_type;

    /* 设置db_set_alias */
    func_ptr->db_set_alias = db_dummy_set_alias;

    /* 设置db_get_alias */
    func_ptr->db_get_alias = db_dummy_get_alias;

    /* 设置db_delete_alias */
    func_ptr->db_delete_alias = db_dummy_delete_alias;

    /* 设置db_delete_all_alias */
    func_ptr->db_delete_all_alias = db_dummy_delete_all_alias;

     func_ptr->db_conn_detect = db_dummy_conn_detect;

	return 0;
}
#endif
