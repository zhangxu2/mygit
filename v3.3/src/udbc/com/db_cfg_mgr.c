/******************************************************************************
 *** �������� : Qifeng.zou                                                  ***
 *** ��ʼ���� : 2012.08.21                                                  ***
 *** �������� :                                                             ***
 *** ����ģ�� : ���ݿ�: �������ݿ�������Ϣ                                  ***
 *** �������� : db_cfg_mgr.c                                                ***
 *** �������� : �������ݿ�������Ϣ                                          ***
 *** �޸�˵�� :                                                             ***
 ******************************************************************************/ 
#include "pub_db.h"
#include "db_data_type.h"
#include "pub_cfg.h"


#if !defined(__NODB__)
/*******************************************************************************/
/* ��̬���� */
static int db_get_db_type(const char *name);
static int db_set_lib_handle(db_type_t type, int mode, db_access_info_t *access);
static void db_set_ifx_cbname(const db_cb_name_t **name);
static void db_set_ora_cbname(const db_cb_name_t **name);
static void db_set_odbc_cbname(const db_cb_name_t **name);
#define db_set_db2_cbname(name) db_set_odbc_cbname(name)
static int db_set_cb_ptr(db_cycle_t *obj);

/*******************************************************************************/

/******************************************************************************
 **��������: db_get_cfg_info
 **��    ��: �������ļ�����ȡ��Ч��Ϣ
 **�������:
 **      dbcfg: ���ݿ�������Ϣ
 **�������:
 **      source: ����Դ��Ϣ
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.21 #
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
 **��������: db_get_db_type
 **��    ��: ͨ�����ݿ�����������ȡ���ݿ�����
 **�������:
 **      name: ���ݿ�������
 **�������:
 **��    ��: ���ݿ�����
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.21 #
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
 **��������: db_set_lib_handle
 **��    ��: ͨ�����ݿ����ͣ����ö�̬����
 **�������:
 **      type: ���ݿ�����
 **�������:
 **      cb: ��Żص�����
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.21 #
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
 **��������: db_set_cb_func
 **��    ��: ͨ�����ݿ����ͣ����ûص�����
 **�������:
 **      type: ���ݿ�����
 **�������:
 **      cb: ��Żص�����
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **      1. �򿪶�̬��
 **      2. ���ú�����
 **      3. ��ȡ������ַ
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.21 #
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
 **��������: db_set_ora_cbname
 **��    ��: ����oracle�����ݿ�����ĺ�����
 **�������:
 **      name: ������
 **�������:
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # zhanglei # 2012.08.21 #
 ******************************************************************************/
static void db_set_ora_cbname(const db_cb_name_t **name)
{
    static const db_cb_name_t ora_cb_name = 
    {
    #if defined(__DB_CONNECT_POOL__)
        "db_ora_open_pool",         /* �������ӳ� */
        "db_ora_close_pool",        /* �ر����ӳ� */
    #endif /*__DB_CONNECT_POOL__*/
    
        "db_ora_open",              /* �������ݿ� */
        "db_ora_close",             /* �ر����ݿ����� */
        
        "db_ora_mul_query",         /* ִ�в�ѯ��� */
        "db_ora_single_query",      /* ִ��ֻ����������һ������Ĳ�ѯ��� */
        "db_ora_non_query",         /* ִ�зǲ�ѯSQL��� */
        "db_ora_update_by_rowid",   /* ͨ��ROWID�������� */
        
        "db_ora_fetch",             /* ȡ�α�ֵ */
        "db_ora_mfetch",             /* ȡ�α�ֵ */
        "db_ora_clo_fetch",         /* �ر��α� */
	"db_ora_del_all_conn",		/* ɾ������ */

        "db_ora_commit",            /* �ύ���� */
        "db_ora_rollback",          /* �ع����� */
        
        "db_ora_get_col_value",     /* ��ȡ��ֵ[ͨ������] */
        "db_ora_get_value_byname",  /* ��ȡ��ֵ[ͨ������] */
        "db_ora_get_data_and_name", /* ��ȡ��ֵ������ */
        "db_ora_get_col_type",      /* ��ȡ������ */
        
        "db_ora_set_alias",         /* ���ñ���ֵ */
        "db_ora_get_alias",         /* ��ȡ����ֵ */
        "db_ora_delete_alias",      /* ɾ��ָ������ */
        "db_ora_delete_all_alias",   /* ɾ�����б��� */
        "db_ora_conn_detect"	     /*̽�����ݿ������Ƿ�����*/	
    };

    *name = (const db_cb_name_t *)&ora_cb_name;
}

/******************************************************************************
 **��������: db_set_ifx_cbname
 **��    ��: ����informix�����ݿ�����ĺ�����
 **�������:
 **      name: ������
 **�������:
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
static void db_set_ifx_cbname(const db_cb_name_t **name)
{
    static const db_cb_name_t ifx_cb_name = 
    {
    #if defined(__DB_CONNECT_POOL__)
        "db_ifx_pcreat",            /* �������ӳ� */
        "db_ifx_pclose",            /* �ر����ӳ� */
    #endif /*__DB_CONNECT_POOL__*/
    
        "db_ifx_open",              /* �������ݿ� */
        "db_ifx_close",             /* �ر����ݿ����� */

        "db_ifx_mquery",            /* ִ�в�ѯ��� */
        "db_ifx_squery",            /* ִ��ֻ����������һ������Ĳ�ѯ��� */
        "db_ifx_nquery",            /* ִ�зǲ�ѯSQL��� */
        "db_ifx_update_by_rowid",   /* ͨ��ROWID�������� */

        "db_ifx_fetch",             /* ȡ�α�ֵ */
        "db_ifx_mfetch",             /* ȡ�α�ֵ */
        "db_ifx_cclose",            /* �ر��α� */
	"db_ifx_del_all_conn",      /* ɾ������ */
        "db_ifx_commit",            /* �ύ���� */
        "db_ifx_rollback",          /* �ع����� */
        
        "db_ifx_get_data_by_idx",   /* ��ȡ��ֵ[ͨ������] */
        "db_ifx_get_data_by_name",  /* ��ȡ��ֵ[ͨ������] */
        "db_ifx_get_data_and_name", /* ��ȡ��ֵ������ */
        "db_ifx_get_data_type",     /* ��ȡ������ */
        
        "db_ifx_set_alias",         /* ���ñ���ֵ */
        "db_ifx_get_alias",         /* ��ȡ����ֵ */
        "db_ifx_delete_alias",      /* ɾ��ָ������ */
        "db_ifx_delete_all_alias",   /* ɾ�����б��� */
        "db_ifx_conn_detect"	     /*̽�����ݿ������Ƿ�����*/
    };

    *name = (const db_cb_name_t *)&ifx_cb_name;
}

/******************************************************************************
 **��������: db_set_odbc_cbname
 **��    ��: ����ODBC�����ݿ�����ĺ�����
 **�������:
 **      name: ������
 **�������:
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.01 #
 ******************************************************************************/
static void db_set_odbc_cbname(const db_cb_name_t **name)
{
    static const db_cb_name_t odbc_cb_name = 
    {
    #if defined(__DB_CONNECT_POOL__)
        "db_odbc_pcreat",           /* �������ӳ� */
        "db_odbc_pclose",           /* �ر����ӳ� */
    #endif /*__DB_CONNECT_POOL__*/
    
        "db_odbc_open",             /* �������ݿ� */
        "db_odbc_close",            /* �ر����ݿ����� */
        
        "db_odbc_mquery",           /* ִ�в�ѯ��� */
        "db_odbc_squery",           /* ִ��ֻ����������һ������Ĳ�ѯ��� */
        "db_odbc_nquery",           /* ִ�зǲ�ѯSQL��� */
        "db_odbc_update_by_rowid",  /* ͨ��ROWID�������� */

        "db_odbc_fetch",            /* ȡ�α�ֵ */
        "db_odbc_mfetch",            /* ȡ�α�ֵ */
        "db_odbc_cclose",           /* �ر��α� */
	"db_odbc_del_all_conn",		/* ɾ������ */

        "db_odbc_commit",           /* �ύ���� */
        "db_odbc_rollback",         /* �ع����� */
        
        "db_odbc_get_data_by_idx",  /* ��ȡ��ֵ[ͨ������] */
        "db_odbc_get_data_by_name", /* ��ȡ��ֵ[ͨ������] */
        "db_odbc_get_data_and_name",/* ��ȡ��ֵ������ */
        "db_odbc_get_data_type",    /* ��ȡ������ */
        
        "db_odbc_set_alias",        /* ���ñ���ֵ */
        "db_odbc_get_alias",        /* ��ȡ����ֵ */
        "db_odbc_delete_alias",     /* ɾ��ָ������ */
        "db_odbc_delete_all_alias",  /* ɾ�����б��� */
        "db_odbc_conn_detect"	     /*̽�����ݿ������Ƿ�����*/
    };

    *name = (const db_cb_name_t *)&odbc_cb_name;
}

/******************************************************************************
 **��������: db_set_cb_ptr
 **��    ��: �������ݿ�����Ļص�����ָ��
 **�������:
 **      func: ������Ϣ
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.21 #
 ******************************************************************************/
static int db_set_cb_ptr(db_cycle_t *obj)
{
    void *handle = obj->access.handle;
    const db_cb_name_t *name = obj->access.cbfunc.name;    
    db_cb_ptr_t *func_ptr = &obj->access.cbfunc.func;

    /* ����db_open */
    func_ptr->db_open = (db_open_func_t)dlsym(handle, name->open);
    if(NULL == func_ptr->db_open)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_close */
    func_ptr->db_close = (db_comm_func_t)dlsym(handle, name->close);
    if(NULL == func_ptr->db_close)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_commit */
    func_ptr->db_commit = (db_comm_func_t)dlsym(handle, name->commit);
    if(NULL == func_ptr->db_commit)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_rollback */
    func_ptr->db_rollback = (db_comm_func_t)dlsym(handle, name->rollback);
    if(NULL == func_ptr->db_rollback)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

#if defined(__DB_CONNECT_POOL__)
    /* ����db_pool */
    func_ptr->db_pool = (db_pool_func_t)dlsym(handle, name->pool);
    if(NULL == func_ptr->db_pool)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }
#endif /*__DB_CONNECT_POOL__*/

#if defined(__DB_CONNECT_POOL__)
    /* ����db_close_pool */
    func_ptr->db_close_pool = (db_comm_func_t)dlsym(handle, name->close_pool);
    if(NULL == func_ptr->db_close_pool)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }
#endif /*__DB_CONNECT_POOL__*/

    /* ����db_mquery */
    func_ptr->db_mquery = (db_mquery_func_t)dlsym(handle, name->mquery);
    if(NULL == func_ptr->db_mquery)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_squery */
    func_ptr->db_squery = (db_squery_func_t)dlsym(handle, name->squery);
    if(NULL == func_ptr->db_squery)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_nquery */
    func_ptr->db_nquery = (db_nquery_func_t)dlsym(handle, name->nquery);
    if(NULL == func_ptr->db_nquery)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_update_by_rowid */
    func_ptr->db_update_by_rowid =
            (db_update_by_rowid_func_t)dlsym(handle, name->update_by_rowid);
    if(NULL == func_ptr->db_update_by_rowid)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_fetch */
    func_ptr->db_fetch = (db_comm_func_t)dlsym(handle , name->fetch);
    if(NULL == func_ptr->db_fetch)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_mfetch */
    func_ptr->db_mfetch = (db_mfetch_func_t)dlsym(handle , name->mfetch);
    if(NULL == func_ptr->db_mfetch)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_cclose */
    func_ptr->db_cclose = (db_clo_fet_func_t)dlsym(handle, name->cclose);
    if(NULL == func_ptr->db_cclose)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_del_all_conn */
    func_ptr->db_del_all_conn = (db_del_all_conn_func_t)dlsym(handle, name->del_all_conn);
    if(NULL == func_ptr->db_del_all_conn)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_get_data */
    func_ptr->db_get_data = (db_get_data_func_t)dlsym(handle, name->get_data);
    if(NULL == func_ptr->db_get_data)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_get_data_by_name */
    func_ptr->db_get_data_by_name =
    (db_get_data_by_name_func_t)dlsym(handle, name->get_data_by_name);
    if(NULL == func_ptr->db_get_data_by_name)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_get_data_and_name */
    func_ptr->db_get_data_and_name = 
    (db_get_data_and_name_func_t)dlsym(handle, name->get_data_and_name);
    if(NULL == func_ptr->db_get_data_and_name)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_get_type */
    func_ptr->db_get_type = (db_get_type_func_t)dlsym(handle, name->get_type);
    if(NULL == func_ptr->db_get_type)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_set_alias */
    func_ptr->db_set_alias = (db_set_alias_func_t)dlsym(handle, name->set_alias);
    if(NULL == func_ptr->db_set_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_get_alias */
    func_ptr->db_get_alias = (db_get_alias_func_t)dlsym(handle, name->get_alias);
    if(NULL == func_ptr->db_get_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_delete_alias */
    func_ptr->db_delete_alias = (db_delete_alias_func_t)dlsym(handle, name->delete_alias);
    if(NULL == func_ptr->db_delete_alias)
    {
        pub_log_error("[%s][%d] %s!", __FILE__, __LINE__, dlerror());
        return -1;
    }

    /* ����db_delete_all_alias */
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
 **��������: db_cfg_release
 **��    ��: �ͷ����ݿ�������Ϣ
 **�������:
 **      obj: ���ݿ����
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.05 #
 ******************************************************************************/
void db_cfg_release(db_cycle_t *obj)
{
    db_access_info_t *access = &obj->access;

    /*dlclose(access->handle);*/
    access->handle = NULL;
}

#if defined(__DB_CONNECT_POOL__)
/******************************************************************************
 **��������: db_dummy_pool
 **��    ��: �������ӳ�(�պ���)
 **�������:
 **     source: ����Դ
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
int db_dummy_pool(void *source, void **cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_close_pool
 **��    ��: �ر����ӳ�(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
int db_dummy_close_pool(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}
#endif /*__DB_CONNECT_POOL__*/

/******************************************************************************
 **��������: db_dummy_open
 **��    ��: �����ݿ�ʧ��(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_open(void *source, void **cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_close
 **��    ��: �����ݿ�ʧ��(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_close(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_mquery
 **��    ��: ������ѯ(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_mquery(const char *alias, const char *sql, int rows, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_squery
 **��    ��: ������ѯ(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_squery(const char *sql,void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_nquery
 **��    ��: �ǲ������
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_nquery(const char *sql, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_update_by_rowid
 **��    ��: ͨ���кŸ������ݿ�(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_update_by_rowid(const char *_sql, const char *rowid, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_fetch
 **��    ��: ȡ����(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_fetch(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_cclose
 **��    ��: �ر��α�(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_cclose(const char *alias, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_commit
 **��    ��: �ύ����(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_commit(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_rollback
 **��    ��: �ع�����(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_rollback(void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_get_data
 **��    ��: ͨ�����к�ȡ����(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static char *db_dummy_get_data(const char *alias, int row, int col, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **��������: db_dummy_get_data_by_name
 **��    ��: ͨ��������ȡ����(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static char *db_dummy_get_data_by_name(const char *alias, int row, const char *name, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **��������: db_dummy_mquery
 **��    ��: ������ѯ(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static char *db_dummy_get_data_and_name(const char *alias, int row, int col, char *name, int size, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **��������: db_dummy_mquery
 **��    ��: ������ѯ(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_get_type(const char *alias, int col, void *cntx)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_set_alias
 **��    ��: ���ñ���(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_set_alias(void *cntx, const char *name, const void *value, int length)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_get_alias
 **��    ��: ͨ������ȡֵ(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static void *db_dummy_get_alias(void *cntx, const char *name)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return NULL;
}

/******************************************************************************
 **��������: db_dummy_delete_alias
 **��    ��: ɾ��ָ������(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
static int db_dummy_delete_alias(void *cntx, const char *name)
{
    pub_log_error("[%s][%d] Didn't connect to database!", __FILE__, __LINE__);
    return -1;
}

/******************************************************************************
 **��������: db_dummy_delete_all_alias
 **��    ��: ɾ�����б���(�պ���)
 **�������:
 **     cntx: ����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
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
 **��������: db_reset_cb_ptr
 **��    ��: �������ݿ�����Ļص�����ָ��
 **�������:
 **      func: ������Ϣ
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.18 #
 ******************************************************************************/
int db_reset_cb_ptr(db_cycle_t *obj)
{
    db_cb_ptr_t *func_ptr = &obj->access.cbfunc.func;

#if defined(__DB_CONNECT_POOL__)
    /* ����db_pool */
    func_ptr->db_pool = db_dummy_pool;

    /* ����db_close_pool */
    func_ptr->db_close_pool = db_dummy_close_pool;
#endif /*__DB_CONNECT_POOL__*/

    /* ����db_open */
    func_ptr->db_open = db_dummy_open;

    /* ����db_close */
    func_ptr->db_close = db_dummy_close;

    /* ����db_mquery */
    func_ptr->db_mquery = db_dummy_mquery;

    /* ����db_squery */
    func_ptr->db_squery = db_dummy_squery;

    /* ����db_nquery */
    func_ptr->db_nquery = db_dummy_nquery;

    /* ����db_update_by_rowid */
    func_ptr->db_update_by_rowid = db_dummy_update_by_rowid;

    /* ����db_fetch */
    func_ptr->db_fetch = db_dummy_fetch;

    /* ����db_cclose */
    func_ptr->db_cclose = db_dummy_cclose;

    /* ����db_commit */
    func_ptr->db_commit = db_dummy_commit;

    /* ����db_rollback */
    func_ptr->db_rollback = db_dummy_rollback;

    /* ����db_get_data */
    func_ptr->db_get_data = db_dummy_get_data;

    /* ����db_get_data_by_name */
    func_ptr->db_get_data_by_name = db_dummy_get_data_by_name;

    /* ����db_get_data_and_name */
    func_ptr->db_get_data_and_name = db_dummy_get_data_and_name;

    /* ����db_get_type */
    func_ptr->db_get_type = db_dummy_get_type;

    /* ����db_set_alias */
    func_ptr->db_set_alias = db_dummy_set_alias;

    /* ����db_get_alias */
    func_ptr->db_get_alias = db_dummy_get_alias;

    /* ����db_delete_alias */
    func_ptr->db_delete_alias = db_dummy_delete_alias;

    /* ����db_delete_all_alias */
    func_ptr->db_delete_all_alias = db_dummy_delete_all_alias;

     func_ptr->db_conn_detect = db_dummy_conn_detect;

	return 0;
}
#endif
