/**************************************************************************
 *** �������� : Zhanglei                                                 **
 *** ��    �� : 2012-08-14                                               **
 *** ����ģ�� : ͳһ���ݷ���                                             **
 *** �������� : access_ora.c                                             **
 *** �������� : oracle���ݿ������ع�������(��DBS��������)              **
 *** ʹ��ע�� :                                                          **
 **************************************************************************/

#include "db_ora_opt.h"
#include "pub_log.h"

#define ORA_ERR_MSG_LEN (1024)  /* Max length of error message */

#define db_ora_ret_success(ret) \
    ((OCI_SUCCESS == ret) || (OCI_SUCCESS_WITH_INFO == ret))

static void db_ora_free_conn(alias_pool_t *ap, ora_conn_t *conn);
static int db_ora_pool_cntx_free(ora_cntx_t *cntx);
static int db_ora_result_init(alias_pool_t *ap, ora_conn_t *conn, ora_result_t *rst, int rows);
static int db_ora_result_alloc(alias_pool_t *ap, ora_result_t *rst);
static int db_ora_result_free(alias_pool_t *ap, ora_result_t *rst);

int ora_get_valid_index(ora_cntx_t *cntx)
{
	int	i = 0;
	
	for (i = 0; i < ORA_MAX_CONN; i++)
	{
		if (cntx->conn[i] == NULL)
		{
			return i;
		}
	}
	
	return -1;
}

int ora_get_conn_index(const char *alias, ora_cntx_t *cntx)
{
	int	i = 0;
	
	if (alias == NULL)
	{
		return 0;
	}
	
	for (i = 1; i < ORA_MAX_CONN; i++)
	{
		if (cntx->conn[i] != NULL && strcmp(cntx->conn[i]->alias, alias) == 0)
		{
			return i;
		}
	}
	return -1;
}

/******************************************************************************
 **��������: check_error
 **��    ��: ��ӡ����ԭ��
 **�������:
 **     errhp: ������
 **�������: ��
 **��    ��: ��
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
static void check_error(OCIError *errhp)
{
    sb4 errcode = 0;
    char errmsg[ORA_ERR_MSG_LEN] = {0};

    OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *)NULL,
        &errcode, errmsg, (ub4) sizeof(errmsg), OCI_HTYPE_ERROR);
    uLog(SW_LOG_ERROR, "errmsg:[%d] %s", errcode, errmsg);
}

/******************************************************************************
 **��������: db_ora_priv_collen
 **��    ��: �������ݿ����ֶεĳ���ȷ������ĳ���
 **�������:
 **     type: �ֶ�����
 **     length: �ֶγ���
 **�������: ��
 **��    ��: ����ĳ���
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
static int db_ora_priv_collen(int type, int length)
{
    switch(type)
    {
        case SQLT_INT:
        {
            return 12;
        }
        case SQLT_NUM:
        {
            return 54;
        }
        /*mod by liuyong
        case SQLT_BFLOAT:
        case SQLT_IBFLOAT:
            return 42;*/
        case SQLT_FLT:
        /*case SQLT_BDOUBLE:
        case SQLT_IBDOUBLE:*/
        {
            return 312;
        }
        case SQLT_AFC:
        case SQLT_AVC:
        case SQLT_CHR:
        case SQLT_VCS:
        case SQLT_STR:
        case SQLT_LNG:
        case SQLT_LVC:
        {
            return length * 2 + 1;   
        }
        /* TODO: Oracle needs more than length but how much? */

        case SQLT_CLOB:
        case SQLT_BLOB:
        {
            return length * 2 + 1;   
        }
        /* TODO: Oracle needs more than length but how much? */

        case SQLT_TIME:
        {
            return 9;
        }
        case SQLT_TIME_TZ:
        {
            return 16;
        }
        case SQLT_TIMESTAMP:
        {
            return 20;
        }
        case SQLT_TIMESTAMP_TZ:
        {
            return 27;
        }
        case SQLT_DAT:
        case SQLT_DATE:
        {
            return 11;
        }
        case SQLT_INTERVAL_YM:
        case SQLT_INTERVAL_DS:
        {
            return 24;
        }
        case SQLT_RDD: /* rowid descriptor */
        {
            return 19;
        }
    }
    
    return length + 1;
}

#if defined(__DB_CONNECT_POOL__)
/******************************************************************************
 **��������: db_ora_open_pool
 **��    ��: �������ӳ�
 **�������:
 **    src: ����Դ������Ϣ
 **    _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14
 **��    ��: # Qifeng.zou # 2013.09.13 # ��ֹ�ڴ�й¶ #
 ******************************************************************************/
int db_ora_open_pool(void *src, void **_cntx)
{
    int ret = 0, idx = 0;
    ora_cntx_t *cntx = NULL;
    db_data_src_t *source = NULL;
    char user[DB_USR_NAME_MAX_LEN] = {0},
         pwd[DB_PWD_MAX_LEN] = {0},
         svrname[DB_SVR_NAME_MAX_LEN] = {0};

    /* 1. ���������� */
    cntx = (ora_cntx_t *)calloc(1, sizeof(ora_cntx_t));
    if(NULL == cntx)
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
        return -1;
    }

    do
    {
        /* 2. ��ʼ�������� */
        ret = alias_init(&cntx->ap, ORA_MEM_POOL_SIZE);
        if(0 != ret)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Init alias pool failed!", __FILE__, __LINE__);
            break;
        }

        source = (db_data_src_t *)src;

        memcpy(user, source->usrname, sizeof(user));
        memcpy(pwd, source->passwd, sizeof(pwd));
        memcpy(svrname, source->svrname, sizeof(svrname));

        uLog(SW_LOG_DEBUG, "[%s][%d] user:[%s] pwd:[%s] svrname:[%s]",
            __FILE__, __LINE__, user, pwd, svrname);

        /*  3. ����OCI������ �κ�OCI��������������������������е� */
        ret = OCIEnvCreate(&(cntx->henv),
                    OCI_THREADED, (dvoid *)0,  NULL, NULL, NULL, 0, (dvoid *)0);
        if(!db_ora_ret_success(ret)) 
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Initialize enverionment failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)(cntx->henv),
                    (void**)&(cntx->herror), OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc error handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void *)cntx->henv,
                    (void**)&(cntx->hcpool), OCI_HTYPE_CPOOL, (size_t)0, (dvoid **)0);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR,
                "[%s][%d] Alloc connection pool failed!", __FILE__, __LINE__);
            break;
        }
        
        /* 4. �������ӳ�*/
        ret = OCIConnectionPoolCreate(
                    cntx->henv, cntx->herror, cntx->hcpool,
                    &(cntx->cpool_name), &(cntx->cpool_name_len),
                    (OraText*)svrname, strlen(svrname),
                    ORA_MIN_CONN, ORA_MAX_CONN, ORA_CONN_INCR,
                    (OraText*)user, strlen(user), (OraText*)pwd, strlen(pwd), OCI_DEFAULT);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR,
                "[%s][%d] Create connection pool failed!", __FILE__, __LINE__);
            check_error(cntx->herror);
            break;
        }
        
        for(idx=0; idx<ORA_MAX_CONN; idx++)
        {
            cntx->conn[idx] = NULL;
            cntx->result[idx].result = NULL;
            cntx->result[idx].length = NULL;
            cntx->result[idx].type = NULL;
            cntx->result[idx].name = NULL;
        }

        *_cntx = cntx;
        return 0;
    }while(0);

    /* 5. ��ֹ�ڴ�й¶ */
    if(NULL != cntx)
    {
        db_ora_pool_cntx_free(cntx);
        cntx = NULL;
    }
    
    return -1;
}

/******************************************************************************
 **��������: db_ora_get_conn
 **��    ��: �����ӳ���ȡһ�����ݿ�����
 **�������:
 **        src: ����Դ������Ϣ
 **        _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 **��    ��: # Qifeng.zou # 2013.09.13 # ��ֹ�ڴ�й¶ #
 ******************************************************************************/
int db_ora_get_conn(void *src, void *_cntx)
{
    int ret = 0, idx = 0, max = 0;
    db_data_src_t *source = NULL;
    char user[DB_USR_NAME_MAX_LEN] = {0},
         pwd[DB_PWD_MAX_LEN] = {0},
         svrname[DB_SVR_NAME_MAX_LEN] = {0};
    ora_conn_t *conn = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    source = (db_data_src_t *)src;
    
    conn = (ora_conn_t *)alias_alloc(ap, sizeof(ora_conn_t));
    if(NULL == conn)
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
        return -1;
    }

    memcpy(user, source->usrname, sizeof(user));
    memcpy(pwd, source->passwd, sizeof(pwd));
    memcpy(svrname, source->svrname, sizeof(svrname));

    do
    {
        ret = OCIHandleAlloc((void*)cntx->henv,
                    (void**)&(conn->herror), OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc error handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)cntx->henv,
                    (void**)&(conn->hstmt), OCI_HTYPE_STMT, (size_t)0, 0);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc statement handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCILogon2(cntx->henv,
                    conn->herror, &(conn->hsvc),
                    user, strlen(user), pwd, strlen(pwd),
                    cntx->cpool_name, cntx->cpool_name_len, OCI_CPOOL);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Logon failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }
        
        conn->pid = getpid();
        conn->hdfn = NULL;
        conn->ind = NULL;

        max = ORA_MAX_CONN - 1;
        for(idx=0; idx<ORA_MAX_CONN; idx++)
        {
            if(NULL == cntx->conn[idx])
            {
                cntx->conn[idx] = conn;
                break;
            }
            if(max == idx)
            {
                sleep(1);
                idx = 0;
            }
        }
        return 0;
    }while(0);

    /* ��ֹ�ڴ�й¶ */
    if(conn->herror)
    {
        OCIHandleFree((dvoid *)conn->herror, OCI_HTYPE_ERROR);
        conn->herror = NULL;
    }

    if(conn->hstmt)
    {
        OCIHandleFree((dvoid *)conn->hstmt, OCI_HTYPE_STMT);
        conn->hstmt = NULL;
    }

    if(conn->hsvc)
    {
        OCIHandleFree((dvoid *)conn->hsvc, OCI_HTYPE_SVCCTX);
        conn->hsvc = NULL;
    }

    return -1;
}

/******************************************************************************
 **��������: db_ora_put_conn
 **��    ��: �ͷ�һ�����ݿ����ӵ����ӳ�
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_put_conn(void *_cntx)
{
    int ret = 0, idx = 0;
    ora_conn_t *conn = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    

    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            break;
        }
    }

    ret = OCILogoff(conn->hsvc, conn->herror);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Logoff failed!", __FILE__, __LINE__);
        check_error(conn->herror);
        return -1;
    }
    
    if(NULL != conn)
    {
        db_ora_free_conn(&cntx->ap, cntx->conn[idx]);
        cntx->conn[idx] = NULL;
        conn = NULL;
    }

    return 0;
}

/******************************************************************************
 **��������: db_ora_close_pool
 **��    ��: �ر����ӳ�
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 **��    ��: # Qifeng.zou # 2013.09.13 # ��ֹ�ڴ�й¶ #
 ******************************************************************************/
int db_ora_close_pool(void *_cntx)
{
    int ret = 0;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;


    ret = OCIConnectionPoolDestroy(cntx->hcpool, cntx->herror, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Destory connection pool failed!", __FILE__, __LINE__);
        check_error(cntx->herror);
        return -1;
    }

    return db_ora_pool_cntx_free(cntx);
}
#endif /*__DB_CONNECT_POOL__*/

/******************************************************************************
 **��������: db_ora_pool_cntx_free
 **��    ��: �ͷ����ӳ�������
 **�������:
 **      cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **     �����ͷŶ�̬�������
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.13 #
 ******************************************************************************/
static int db_ora_pool_cntx_free(ora_cntx_t *cntx)
{
    int idx = 0;

#if defined(__DB_CONNECT_POOL__)    
    if(cntx->hcpool)
    {
        OCIHandleFree((dvoid *)cntx->hcpool, OCI_HTYPE_CPOOL);
        cntx->hcpool = NULL;
    }
    
    if(cntx->herror)
    {
        OCIHandleFree((dvoid *)cntx->herror, OCI_HTYPE_ERROR);
        cntx->herror = NULL;
    }
    
    if(cntx->henv)
    {
        OCIHandleFree(cntx->henv, OCI_HTYPE_ENV);
        cntx->henv = NULL;
    }

    if(cntx->cpool_name)
    {
        free(cntx->cpool_name);
        cntx->cpool_name = NULL;
    }
#endif /*__DB_CONNECT_POOL__*/

    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(NULL != cntx->conn[idx])
        {
            db_ora_free_conn(&cntx->ap, cntx->conn[idx]);
            cntx->conn[idx] = NULL;
        }
    }
    
    if(cntx)
    {
        alias_destory(&cntx->ap);
        free(cntx);
    }
    
    return 0;
}

/******************************************************************************
 **��������: db_ora_open
 **��    ��: ��ʼ��һ�����ݿ�����
 **�������:
 **      src: ����Դ������Ϣ
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_open(void *src, void **_cntx)
{
    int i = 0;
    int ret = -1;
    ora_cntx_t *cntx = NULL;
    ora_conn_t *conn = NULL;
    db_data_src_t *source = NULL;
    char user[DB_USR_NAME_MAX_LEN] = {0},
         pwd[DB_PWD_MAX_LEN] = {0},
         svrname[DB_SVR_NAME_MAX_LEN] = {0};

    do
    {
        /* 1. Alloc memory for context */
        cntx = (ora_cntx_t *)calloc(1, sizeof(ora_cntx_t));
        if(NULL == cntx)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            break;
        }

        /* 2. Initialize alias pool */
        ret = alias_init(&cntx->ap, ORA_MEM_POOL_SIZE);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Init alias pool failed!", __FILE__, __LINE__);
            break;
        }
        
        conn = (ora_conn_t *)alias_alloc(&cntx->ap, sizeof(ora_conn_t));
        if(NULL == conn)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            break;
        }

        source = (db_data_src_t *)src;

        memcpy(user, source->usrname, sizeof(user));
        memcpy(pwd, source->passwd, sizeof(pwd));
        memcpy(svrname, source->svrname, sizeof(svrname));

        /* ����OCI������ �κ�OCI��������������������������е�*/
        ret = OCIEnvCreate(&conn->henv,
                    OCI_THREADED, (dvoid *)0,  NULL, NULL, NULL, 0, (dvoid *)0);
        if(!db_ora_ret_success(ret)) 
        {
            pub_log_error("[%s][%d] Create environment handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)conn->henv,
                    (void**)&(conn->herror), OCI_HTYPE_ERROR, (size_t)0, 0);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Alloc error handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)conn->henv,
                    (void**)&(conn->hsvr), OCI_HTYPE_SERVER, (size_t)0, 0);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Alloc server handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)conn->henv,
                    (void**)&(conn->hsvc), OCI_HTYPE_SVCCTX, (size_t)0, 0);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Alloc service handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)conn->henv,
                    (void**)&(conn->hssn), OCI_HTYPE_SESSION, (size_t)0, 0);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Alloc session handle failed!", __FILE__, __LINE__);
            break;
        }

        ret = OCIHandleAlloc((void*)conn->henv,
                    (void**)&(conn->hstmt), OCI_HTYPE_STMT, (size_t)0, 0);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Alloc statement handle failed!", __FILE__, __LINE__);
            break;
        }
        
        /*  �������ݿ� */
        ret = OCIServerAttach((OCIServer*)conn->hsvr,
                    (OCIError*)conn->herror, svrname, strlen(svrname), (ub4)OCI_DEFAULT);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Attach server handle failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }

        /*  ���ûỰ���� */
        ret = OCIAttrSet((void*)conn->hsvc,OCI_HTYPE_SVCCTX,
                    (void*)conn->hsvr, 0, OCI_ATTR_SERVER, conn->herror);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Set server attribute of service failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }

        ret = OCIAttrSet((void*)conn->hssn, OCI_HTYPE_SESSION,
                    (void*)user, strlen(user), OCI_ATTR_USERNAME, conn->herror);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Set user attribute of session failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }

        ret = OCIAttrSet((void*)conn->hssn, OCI_HTYPE_SESSION,
                    (void*)pwd, strlen(pwd), OCI_ATTR_PASSWORD, conn->herror);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Set password attribute of session failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }
        
        /*  ��ʼ�Ự */
        ret = OCISessionBegin((OCISvcCtx*)conn->hsvc,
                    conn->herror, conn->hssn, OCI_CRED_RDBMS, OCI_DEFAULT);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Begin session failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }

        /*  ���ûỰ���� */
        ret = OCIAttrSet((void*)conn->hsvc, OCI_HTYPE_SVCCTX,
                    (void*)conn->hssn, 0, OCI_ATTR_SESSION, conn->herror);
        if(!db_ora_ret_success(ret))
        {
            pub_log_error("[%s][%d] Set session attribute of service failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            break;
        }
        
        conn->pid = getpid();
        conn->hdfn = NULL;
        conn->ind = NULL;

        cntx->conn[0] = conn;
        for (i = 0; i < ORA_MAX_CONN; i++)
        {
        	cntx->result[i].result = NULL;
        	cntx->result[i].length = NULL;
        	cntx->result[i].type = NULL;
       		cntx->result[i].name = NULL;
        }
	for (i = 1; i < ORA_MAX_CONN; i++)
	{
		cntx->conn[i] = NULL;
	}
	
        *_cntx = cntx;
        return 0;
    }while(0);

    if(NULL != conn)
    {
        db_ora_free_conn(&cntx->ap, conn);
        conn = NULL;
    }

    if(NULL != cntx)
    {
        alias_destory(&cntx->ap);
        free(cntx);
        cntx = NULL;
    }
    return -1;
}


/******************************************************************************
 **��������: db_ora_close
 **��    ��: �ر�һ�����ݿ�����
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_close(void *_cntx)
{
    int idx = 0;
    ora_conn_t *conn = NULL;
    ora_result_t *rst = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        conn = cntx->conn[idx];
	if (conn == NULL)
	{
		continue;
	}
        rst = &cntx->result[idx];

        OCISessionEnd((OCISvcCtx*)conn->hsvc, conn->herror, conn->hssn, OCI_DEFAULT);
        OCIServerDetach((OCIServer*)conn->hsvr, conn->herror, OCI_DEFAULT);

        db_ora_free_conn(ap, conn);
        cntx->conn[idx] = NULL;

        db_ora_result_free(ap, rst);
    }

    
    if(cntx)
    {
        alias_destory(&cntx->ap);
        free(cntx), cntx = NULL;
    }

    return 0;
}

/******************************************************************************
 **��������: db_ora_free_conn
 **��    ��: �ͷ����Ӷ���
 **�������:
 **      conn: ���Ӷ���
 **�������: ��
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.14 #
 ******************************************************************************/
static void db_ora_free_conn(alias_pool_t *ap, ora_conn_t *conn)
{
    if(conn->hstmt)
    {
        OCIHandleFree(conn->hstmt, OCI_HTYPE_STMT);
        conn->hstmt = NULL;
    }
    
    if(conn->hssn)
    { 
        OCIHandleFree(conn->hssn, OCI_HTYPE_SESSION);
        conn->hssn = NULL;
    }
    
    if(conn->hsvc)
    {
        OCIHandleFree(conn->hsvc, OCI_HTYPE_SVCCTX);
        conn->hsvc = NULL;
    }
    
    if(conn->hsvr)
    {
        OCIHandleFree(conn->hsvr, OCI_HTYPE_SERVER);
        conn->hsvr = NULL;
    }
    
    if(conn->herror)
    {
        OCIHandleFree(conn->herror, OCI_HTYPE_ERROR);
        conn->herror = NULL;
    }
    
    if(conn->henv)
    {
        OCIHandleFree(conn->henv, OCI_HTYPE_ENV);
        conn->henv = NULL;
    }

    if(conn->hdfn)
    {
        alias_free(ap, conn->hdfn);
        conn->hdfn = NULL;
    }

    if(conn->ind)
    {
        alias_free(ap, conn->ind);
        conn->ind = NULL;
    }
    
    alias_free(ap, conn);
    return;
}

/******************************************************************************
 **��������: db_ora_commit
 **��    ��: �ύ����
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_commit(void *_cntx)
{
    int ret = 0;
    ora_conn_t *conn = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

#if defined(__DB_CONNECT_POOL__)    
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
    conn = cntx->conn[0];
#endif /*__DB_CONNECT_POOL__*/

    ret = OCITransCommit(conn->hsvc, conn->herror, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        pub_log_error("[%s][%d] Commit transaction failed!", __FILE__, __LINE__);
        check_error(conn->herror);
        return -1;
    }
    return 0;
}

/******************************************************************************
 **��������: db_ora_rollback
 **��    ��: �ع�����
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success -1:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_rollback(void *_cntx)
{
    int ret = 0;
    ora_conn_t *conn = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
    conn = cntx->conn[0];
#endif /*__DB_CONNECT_POOL__*/

    ret = OCITransRollback(conn->hsvc, conn->herror, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        pub_log_error("[%s][%d] Rollback transaction failed!", __FILE__, __LINE__);
        check_error(conn->herror);
        return -1;
    }
    return 0;
}

/******************************************************************************
 **��������: db_ora_non_query
 **��    ��: ִ�зǲ�ѯ���sql
 **�������:
 **      sql: sql���
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: 0:success !0: failed
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_non_query(const char *sql, void *_cntx)
{
    int ret = 0, rows = 0;
    ora_conn_t *conn = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;


#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
    conn = cntx->conn[0];
#endif /*__DB_CONNECT_POOL__*/

    ret = OCIStmtPrepare(conn->hstmt,
                conn->herror, (text*)sql, strlen(sql), OCI_NTV_SYNTAX, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR,
            "[%s][%d] Prepare SQL statement failed![%s]", __FILE__, __LINE__, sql);
        check_error(conn->herror);
        return -1;
    }

    ret = OCIStmtExecute(conn->hsvc,
                conn->hstmt, conn->herror, 1, 0, NULL, NULL, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR,
            "[%s][%d] Execute SQL statement failed![%s]", __FILE__, __LINE__, sql);
        check_error(conn->herror);
        return -1;
    }

    OCIAttrGet(conn->hstmt, OCI_HTYPE_STMT, (dvoid *)&rows, (ub4 *) NULL,                                                  
        (ub4)OCI_ATTR_ROWS_FETCHED, conn->herror);

    return rows;
}

/******************************************************************************
 **��������: db_ora_update_by_rowid
 **��    ��: ͨ��rowid�������ݿ�����
 **�������:
 **      sql: UPDATE [tablename] SET [colname=value] [, ...]
 **      rowid: �к�
 **     _cntx: ���ݿ�����������
 **�������:
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     1. ƴ��SQL���
 **     2. ִ��SQL���
 **ע������: 
 **     UPDATE student
 **         SET id='0001', name='zouqifeng'
 **         WHERE rowid='0A000000000083C0000'���������
 **��    ��: # Qifeng.zou  # 2013.08.21 #
 ******************************************************************************/
int db_ora_update_by_rowid(const char *_sql, const char *rowid, void *_cntx)
{
    char sql[DB_SQL_MAX_LEN] = {0};

    snprintf(sql, sizeof(sql), "%s WHERE rowid='%s'", _sql, rowid);

    return db_ora_non_query(sql, _cntx);
}

/******************************************************************************
 **��������: db_ora_single_query
 **��    ��: ��ѯһ�������sql
 **�������:
 **      sql: sql���
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: -1 ʧ��    >0 �еĸ���
 **ʵ������: 
 **ע������: select fetch ����
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_single_query(const char *sql, void *_cntx)
{
    int ret = 0, idx = 0, pos = 0;
    ora_conn_t con;
    ora_conn_t *conn = NULL;
    ora_result_t *rst = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    cntx->conn[0]->is_nodata = 0;

#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
    memset(&con, 0x0, sizeof(con));
    memcpy(&con, cntx->conn[0], sizeof(ora_conn_t));
    conn = cntx->conn[0];
    rst = &(cntx->result[0]);
#endif /*__DB_CONNECT_POOL__*/

    /* 1. ׼����ִ��SQL��� */
    ret = OCIStmtPrepare(conn->hstmt,
                conn->herror, (text*)sql, strlen(sql), OCI_NTV_SYNTAX, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR,
            "[%s][%d] Prepare SQL statement failed! [%s]", __FILE__, __LINE__, sql);
        check_error(conn->herror);        
        return -1;
    }

    ret = OCIStmtExecute(conn->hsvc,
                conn->hstmt, conn->herror, 0, 0, NULL, NULL, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR,
            "[%s][%d] Execute SQL statement failed! [%s]", __FILE__, __LINE__, sql);
        check_error(conn->herror);
        return -1;
    }    

    /* 2. Ϊ���������ռ� */
    ret = db_ora_result_init(ap, conn, rst, 1);
    if(0 != ret)
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Init result failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 3. ������� */
    pos = 0;
    for(idx=1; idx<=rst->cols; idx++)
    {
        conn->hdfn[idx-1] = (OCIDefine *)0;

        ret = OCIDefineByPos(
                    conn->hstmt, &(conn->hdfn[idx-1]),
                    conn->herror, idx, (dvoid *)&rst->result[pos],
                    (sword)rst->length[idx-1], (ub2)SQLT_STR,
                    (dvoid *)&(conn->ind[idx-1]), (ub2 *)0, (ub2 *)0, OCI_DEFAULT);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Define by position failed!", __FILE__, __LINE__);
            return -1;
        }
        pos += rst->length[idx-1];
    }

    pos = 0;

    /* 4. ��ȡ��ѯ��� */
    ret = OCIStmtFetch2(conn->hstmt,conn->herror, 1, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
    switch(ret)
    {
        case OCI_SUCCESS:
        case OCI_SUCCESS_WITH_INFO:
        {
            return rst->cols;
        }
        case OCI_NO_DATA:
        {
            return 0;
        }
        default:
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Fetch failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            return -1;
        }
    }
    return 0;
}

/******************************************************************************
 **��������: db_ora_mul_query
 **��    ��: ��ѯ��������sql
 **�������:
 **      sql: sql���
 **      rows: һ�δ����ݿ���ȡ���Ľ��������
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: -1 ʧ��    >0 �еĸ���
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_mul_query(const char *alias, const char *sql, int rows, void *_cntx)
{
    int ret = 0, idx = 0, pos = 0;
    ora_conn_t *conn = NULL;
    ora_result_t *rst = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;


#if defined(__DB_CONNECT_POOL__)
    for(idx=0;  idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
 	int	index = 0;
 
	index = ora_get_conn_index(alias, cntx);
	if (index == -1)
	{
		index = ora_get_valid_index(cntx);
		if (index == -1)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] No enough space to save conn info!", __FILE__, __LINE__);
			return -1;
		}
		ora_conn_t	*con = NULL;
		con = (ora_conn_t *)alias_alloc(&cntx->ap, sizeof(ora_conn_t));
		if (con == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alias alloc error!", __FILE__, __LINE__);
			return -1;
		}
		memset(con, 0x0, sizeof(ora_conn_t));
		memcpy(con, cntx->conn[0], sizeof(ora_conn_t));
		ret = OCIHandleAlloc((void*)con->henv, (void**)&(con->hstmt), OCI_HTYPE_STMT, (size_t)0, 0);
		if (!db_ora_ret_success(ret))
		{
			pub_log_error("[%s][%d] Alloc statement handle error!", __FILE__, __LINE__);
			return -1;
		}
		strcpy(con->alias, alias);
		cntx->conn[index] = con;
		cntx->conn[index]->hdfn = NULL;
		cntx->conn[index]->ind = NULL;
	}
	conn = cntx->conn[index];
	rst = &(cntx->result[index]);
	conn->is_nodata = 0;
#endif /*__DB_CONNECT_POOL__*/

    /* 1. ׼����ִ��SQL��� */
    ret = OCIStmtPrepare(conn->hstmt,
                conn->herror, (text*)sql, strlen(sql), OCI_NTV_SYNTAX, OCI_DEFAULT);    
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR,
            "[%s][%d] Prepare SQL statement failed![%s]", __FILE__, __LINE__, sql);
        check_error(conn->herror);
        return -1;
    }
    
    ret = OCIStmtExecute(conn->hsvc,
                conn->hstmt, conn->herror, 0, 0, NULL, NULL, OCI_DEFAULT);
    if(!db_ora_ret_success(ret))
    {
        uLog(SW_LOG_ERROR,
            "[%s][%d] Execute SQL statement failed![%s]", __FILE__, __LINE__, sql);
        check_error(conn->herror);
        return -1;
    }

    /* 2. ��ʼ������� */
    ret = db_ora_result_init(ap, conn, rst, rows);
    if(0 != ret)
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Init result failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 3. �󶨽���� */
    pos = 0;
    for(idx = 1; idx<=rst->cols; idx++)
    {
        conn->hdfn[idx-1] = (OCIDefine *)0;

        ret = OCIDefineByPos(conn->hstmt, &(conn->hdfn[idx-1]),
                    conn->herror, idx, (dvoid *)&rst->result[pos],
                    (sb4)rst->length[idx-1], (ub2)SQLT_STR,
                    (dvoid *)&(conn->ind[idx-1]), (ub2 *)0, (ub2 *)0, OCI_DEFAULT);
        if(!db_ora_ret_success(ret))
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Define by position failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            return -1;
        }
        pos += rst->length[idx-1];
    }
    
    for(idx=1; idx<=rst->cols; idx++)
    {
        ret = OCIDefineArrayOfStruct(
                    conn->hdfn[idx-1], conn->herror, rst->rowlen, 0, 0, 0);
        if(!db_ora_ret_success(ret)) 
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Define array of struct failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            return -1;
        }
    }
    return rst->cols;
}

/******************************************************************************
 **��������: db_ora_fetch
 **��    ��: �α�ȡ����ѯ�Ľ��
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: -1 ʧ��    >0 ���������
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_fetch(void *_cntx)
{
    sword ret = 0;
    ora_conn_t *conn = NULL;
    ora_result_t *rst = NULL;
    int rows = 0;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

    if(cntx->conn[0]->is_nodata)
    {
        return 0;
    }

#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
    conn = cntx->conn[0];
    rst = &(cntx->result[0]);
#endif /*__DB_CONNECT_POOL__*/
        
    ret = OCIStmtFetch2(conn->hstmt,
                conn->herror, rst->rows, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
    switch(ret)
    {
        case OCI_SUCCESS:
        case OCI_SUCCESS_WITH_INFO:
        {
            return rst->rows;
        }
        case OCI_NO_DATA:
        {
            cntx->conn[0]->is_nodata = 1;
            
            OCIAttrGet(conn->hstmt, OCI_HTYPE_STMT, (dvoid *)&rows, (ub4 *) NULL, 
                    (ub4)OCI_ATTR_ROWS_FETCHED, conn->herror);
            if(0 != rows)
            {
                rst->rows = rows;
                return rst->rows;
            }
            return 0;
        }
        default:
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Fetch failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            return -1;
        }
    }

    return -1;
}

int db_ora_mfetch(const char *alias, void *_cntx)
{
    sword ret = 0;
    ora_conn_t *conn = NULL;
    ora_result_t *rst = NULL;
    int rows = 0;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            conn = cntx->conn[idx];
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
	int	index = 0;
	
	index = ora_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return -1;
	}
	conn = cntx->conn[index];
	rst = &(cntx->result[index]);
	if(conn->is_nodata)
	{
		/*uLog(SW_LOG_DEBUG, "[%s][%d] No data!", __FILE__, __LINE__);*/
		return 0;
	}
#endif /*__DB_CONNECT_POOL__*/
        
    ret = OCIStmtFetch2(conn->hstmt,
                conn->herror, rst->rows, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
    switch(ret)
    {
        case OCI_SUCCESS:
        case OCI_SUCCESS_WITH_INFO:
        {
            return rst->rows;
        }
        case OCI_NO_DATA:
        {
            cntx->conn[index]->is_nodata = 1;
            
            OCIAttrGet(conn->hstmt, OCI_HTYPE_STMT, (dvoid *)&rows, (ub4 *) NULL, 
                    (ub4)OCI_ATTR_ROWS_FETCHED, conn->herror);
            if(0 != rows)
            {
                rst->rows = rows;
                return rst->rows;
            }
            return 0;
        }
        default:
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Fetch failed!", __FILE__, __LINE__);
            check_error(conn->herror);
            return -1;
        }
    }

    return -1;
}

/******************************************************************************
 **��������: db_ora_clo_fetch
 **��    ��: �ر��α꣬�ͷŽ�����ռ�
 **�������:
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: -1 ʧ��    0 �ɹ�
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_clo_fetch(const char *alias, void *_cntx) 
{ 
    int idx = 0;
    ora_result_t *rst = NULL;
    alias_pool_t *ap = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

    ap = &cntx->ap;
#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
	idx = ora_get_conn_index(alias, cntx);
	if (idx == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return -1;
	}
	cntx->conn[idx]->is_nodata = 1;
	rst = &(cntx->result[idx]);
#endif /*__DB_CONNECT_POOL__*/
	
	if (idx > 0)
	{
		if(cntx->conn[idx]->hstmt)
		{
			OCIHandleFree(cntx->conn[idx]->hstmt, OCI_HTYPE_STMT);
			cntx->conn[idx]->hstmt = NULL;
		}
	}
   
    if(cntx->conn[idx]->hdfn)
    {
        alias_free(ap, cntx->conn[idx]->hdfn);
        cntx->conn[idx]->hdfn = NULL;
    }
    
    if(cntx->conn[idx]->ind)
    {
        alias_free(ap, cntx->conn[idx]->ind);
        cntx->conn[idx]->ind = NULL;
    }
	if (idx > 0)
	{
		if (cntx->conn[idx] != NULL)
		{
			alias_free(ap, cntx->conn[idx]);
			cntx->conn[idx] = NULL;
		}
	}

    return db_ora_result_free(ap, rst);
}

int db_ora_del_all_conn(void *_cntx) 
{ 
	int	idx = 0;
	ora_result_t	*rst = NULL;
	alias_pool_t	*ap = NULL;
	ora_cntx_t	*cntx = (ora_cntx_t *)_cntx;
	
	ap = &cntx->ap;
	for (idx = 1; idx < ORA_MAX_CONN; idx++)
	{
		if (cntx->conn[idx] == NULL)
		{
			continue;
		}
	
		pub_log_info("[%s][%d] Delete [%s][%d] begin...", __FILE__, __LINE__, cntx->conn[idx]->alias, idx);
		if(cntx->conn[idx]->hstmt)
		{
			OCIHandleFree(cntx->conn[idx]->hstmt, OCI_HTYPE_STMT);
			cntx->conn[idx]->hstmt = NULL;
		}

		if (cntx->conn[idx]->hdfn)
		{
			alias_free(ap, cntx->conn[idx]->hdfn);
			cntx->conn[idx]->hdfn = NULL;
		}

		if (cntx->conn[idx]->ind)
		{
			alias_free(ap, cntx->conn[idx]->ind);
			cntx->conn[idx]->ind = NULL;
		}

		alias_free(ap, cntx->conn[idx]);
		cntx->conn[idx] = NULL;

		rst = &(cntx->result[idx]);
		db_ora_result_free(ap, rst);
	}
	
	return 0;
}


/******************************************************************************
 **��������: db_ora_result_init
 **��    ��: �������Ϣ��ʼ��
 **�������:
 **     ap: �ڴ��
 **     rows: �������Ҫ��ŵ���������
 **�������: 
 **     conn: ����������
 **     rst: �����
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     1. ��ֹ�ڴ�й¶
 **     2. ��ȡ������Ϣ
 **     3. Ϊ���������ռ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.13 #
 ******************************************************************************/
static int db_ora_result_init(alias_pool_t *ap, ora_conn_t *conn, ora_result_t *rst, int rows)
{
    int ret = -1, cols = 0, idx = 0;
    size_t size = 0;
    ub2 dtype = 0;
    sb2 dlength = 0;
    char *name = NULL;
    ub4 nameSize = 0;
    OCIParam *desc = NULL;

    /* 1. ��ֹ�ڴ�й¶ */
    db_ora_result_free(ap, rst);

    if(conn->ind)
    {
        alias_free(ap, conn->ind);
        conn->ind = NULL;
    }

    if(conn->hdfn)
    {
        alias_free(ap, conn->hdfn);
        conn->hdfn = NULL;
    }

    /* 2. ��ȡ������Ϣ */
    OCIAttrGet (conn->hstmt, (ub4)OCI_HTYPE_STMT,
        (dvoid *) &cols, (ub4 *) 0, (ub4)OCI_ATTR_PARAM_COUNT, conn->herror);

    rst->cols = cols;
    rst->rows = rows;

    /* 3. Ϊ���������ռ� */
    do
    {
        ret = db_ora_result_alloc(ap, rst);
        if(0 != ret)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc result failed!", __FILE__, __LINE__);
            break;
        }
            
        conn->ind = (int *)alias_alloc(ap, sizeof(int)*(cols));
        if(NULL == conn->ind)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory for ind failed!", __FILE__, __LINE__);
            break;
        }
        
        conn->hdfn = (OCIDefine **)alias_alloc(ap, sizeof(OCIDefine *)*cols);
        if(NULL == conn->hdfn)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory for hdfn failed!", __FILE__, __LINE__);
            break;
        }

        rst->rowlen = 0;
        for(idx=1; idx<=cols; idx++)
        {
            /* ����������������� */
            OCIParamGet(conn->hstmt, OCI_HTYPE_STMT, conn->herror, (void **)&desc, idx);
            
            /* ��ȡ��ǰ�ֶε������� */
            OCIAttrGet((dvoid*)desc, OCI_DTYPE_PARAM,
                (dvoid *)&dtype, NULL, OCI_ATTR_DATA_TYPE, conn->herror);
            rst->type[idx-1] = dtype;

            OCIAttrGet((dvoid*)desc, (ub4)OCI_DTYPE_PARAM,
                (dvoid *)&dlength, NULL, OCI_ATTR_DATA_SIZE, conn->herror);
            rst->length[idx-1] = db_ora_priv_collen(dtype, dlength);
            rst->rowlen += rst->length[idx-1];
     
            OCIAttrGet((dvoid*)desc, 
                (ub4)OCI_DTYPE_PARAM, &name, &nameSize, OCI_ATTR_NAME, conn->herror);
            
            rst->name[idx-1] = (char *)alias_alloc(ap, sizeof(char)*nameSize + 1);
            if(NULL == rst->name[idx-1])
            {
                free(desc), desc=NULL;
                uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
                goto RST_INIT_ERROR;
            }
            memset(rst->name[idx-1], 0, nameSize + 1);
            strncpy(rst->name[idx-1], name, nameSize);

            /* �ͷ�desc 
            free(desc), desc=NULL;
		***/
        }

        size = rst->rowlen * rows * sizeof(unsigned char);
        rst->result = (unsigned char *)alias_alloc(ap, size);
        if(NULL == rst->result)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            break;
        }
        
        memset(rst->result, 0, size);

        return 0;
    }while(0);

RST_INIT_ERROR:

    db_ora_result_free(ap, rst);

    if(conn->ind)
    {
        alias_free(ap, conn->ind);
        conn->ind = NULL;
    }

    if(conn->hdfn)
    {
        alias_free(ap, conn->hdfn);
        conn->hdfn = NULL;
    }
    
    return -1;
}

/******************************************************************************
 **��������: db_ora_result_alloc
 **��    ��: Ϊ���������ռ�
 **�������:
 **     ap: �ڴ��
 **�������: 
 **      rst: �����
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.13 #
 ******************************************************************************/
static int db_ora_result_alloc(alias_pool_t *ap, ora_result_t *rst)
{
    do
    {
        rst->length = (int *)alias_alloc(ap, sizeof(int) * (rst->cols));
        if(NULL == rst->length)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory for length failed!", __FILE__, __LINE__);
            break;
        }
        
        rst->type = (int *)alias_alloc(ap, sizeof(int) * (rst->cols));
        if(NULL == rst->type)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory for type failed!", __FILE__, __LINE__);
            break;
        }
        
        rst->name = (char **)alias_alloc(ap, sizeof(char *) * (rst->cols));
        if(NULL == rst->name)
        {
            uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory for name failed!", __FILE__, __LINE__);
            break;
        }

        return 0;
    }while(0);

    if(rst->length)
    {
        alias_free(ap, rst->length);
        rst->length = NULL;
    }

    if(rst->type)
    {
        alias_free(ap, rst->type);
        rst->type = NULL;
    }

    if(rst->name)
    {
        alias_free(ap, rst->name);
        rst->name = NULL;
    }
    
    return -1;
}

/******************************************************************************
 **��������: db_ora_result_free
 **��    ��: �ͷŽ�������ڴ�
 **�������:
 **      rst: �����
 **�������: ��
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.13 #
 ******************************************************************************/
static int db_ora_result_free(alias_pool_t *ap, ora_result_t *rst)
{
    int idx = 0;
    
    if(rst->result)
    {
        alias_free(ap, rst->result);
        rst->result = NULL;
    }
    
    if(rst->length)
    {
        alias_free(ap, rst->length);
        rst->length = NULL;
    }
    
    if(rst->type)
    {
        alias_free(ap, rst->type);
        rst->type = NULL;
    }
        
    if(NULL != rst->name)
    {
        for(idx = 0; idx < rst->cols; idx++)
        {
            if(NULL != rst->name[idx])
            {
                alias_free(ap, rst->name[idx]);
                rst->name[idx] = NULL;
            }
        }
        alias_free(ap, rst->name);
        rst->name = NULL;
    }
    return 0;
}

/******************************************************************************
 **��������: db_ora_get_col_value
 **��    ��: �����кź��е�λ��ȡ���
 **�������:
 **      row: �к�
 **      col: �е�λ��
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: ������׵�ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
char *db_ora_get_col_value(const char *alias, int rows, int col, void *_cntx)
{
    ora_result_t *rst = NULL;
    int idx = 0, pos = 0;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
	int	index = 0;
	
	index = ora_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return NULL;
	}
	rst = &(cntx->result[index]);
#endif /*__DB_CONNECT_POOL__*/

    if((rows < 1) || (rows > rst->rows)
        || (col < 1) || (col > rst->cols))
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Out of range![%d][%d]/[%d][%d]",
            __FILE__, __LINE__, rows, col, rst->rows, rst->cols);
        return NULL;
    }
    
    pos = (rows - 1)*rst->rowlen;
    for(idx = 1; idx < col; idx++)
    {
        pos += rst->length[idx-1];
    }
    
    return (char *)&rst->result[pos];
}

/******************************************************************************
 **��������: db_ora_get_col_type
 **��    ��: �����е�λ��ȡ���������
 **�������:
 **      col: �е�λ��
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: ���������
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
int db_ora_get_col_type(const char *alias, int col, void *_cntx)
{
    ora_result_t *rst = NULL;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;

#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
	int	index = 0;
	
	index = ora_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return -1;
	}
	rst = &(cntx->result[index]);
#endif /*__DB_CONNECT_POOL__*/

    if((col < 1) || (col > rst->cols))
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Out of range![%d]/[%d]",
            __FILE__, __LINE__, col, rst->cols);
        return -1;
    }
    
    switch (rst->type[col - 1])
    {
        case SQLT_FLT:
        {
            return DB_DTYPE_FLOAT;
        }
        case SQLT_UIN:
        case SQLT_INT:
        {
            return DB_DTYPE_INT;
        }
        case SQLT_STR:
        case SQLT_CHR:
        {
            return DB_DTYPE_VCHAR2;
        }
        case SQLT_LNG:
        {
            return DB_DTYPE_LONG;
        }
        case SQLT_LVC:
        case SQLT_VCS:
        {
            return DB_DTYPE_VCHAR;
        }
        case SQLT_DAT:
        {
            return DB_DTYPE_DATE;
        }
        case SQLT_AVC:
        case SQLT_AFC:
        {
            return DB_DTYPE_CHAR;
        }
        case SQLT_NUM:
        {
            return DB_DTYPE_NUMBER;
        }
        case SQLT_BIN:
        case SQLT_VBI:
        {
            return DB_DTYPE_RAW;
        }
        case SQLT_LBI:
        case SQLT_LVB:
        {
            return DB_DTYPE_LONGRAW;
        }
        default:
        {
            return DB_DTYPE_UNKNOW;
        }
    }
    
    return DB_DTYPE_UNKNOW;
}

/******************************************************************************
 **��������: db_ora_get_value_byname
 **��    ��: �����кź��е�����ȡ���
 **�������:
 **      row: �к�
 **      col: �е�����
 **      _cntx: ������������Ϣ
 **�������: ��
 **��    ��: ������׵�ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2012.08.14 #
 ******************************************************************************/
char* db_ora_get_value_byname(const char *alias, int rows, const char *name, void *_cntx)
{
    ora_result_t *rst = NULL;
    int idx = 0, j = 0, pos = 0;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;


#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
	int	index = 0;
	
	index = ora_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return NULL;
	}
	rst = &(cntx->result[index]);
#endif /*__DB_CONNECT_POOL__*/


    if((rows > rst->rows) || (rows < 1))
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Out of range! [%d]/[%d]",
            __FILE__, __LINE__, rows, rst->rows);
        return NULL;
    }
    
    for(idx=0; idx < rst->cols; idx++)
    {
        if(0 == strcasecmp(rst->name[idx], name))
        {
            break;
        }
    }
    
    if(idx == rst->cols)
    {
        return NULL;
    }
    
    pos = (rows - 1)*rst->rowlen;
    for(j=1; j <= idx; j++)
    {
        pos += rst->length[j-1];
    }
    
    return (char *)&rst->result[pos];
}

/******************************************************************************
 **��������: db_ora_get_data_and_name
 **��    ��: �����кź��е�����ȡ���
 **�������:
 **     rowno: �к�(ȡֵ��Χ: 1~X)
 **     colno: �к�(ȡֵ��Χ: 1~Y)
 **     size: ��������ߴ�
 **     _cntx: ���ݿ�������������Ϣ
 **�������:
 **     name: ����
 **��    ��: NULL:δ�ҵ���� !NULL:��ѯ����ĵ�ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # ZhangLei # 2013.07.02 #
 ******************************************************************************/
char *db_ora_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size, void *_cntx)
{
    ora_result_t *rst = NULL;
    int idx = 0, pos = 0;
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;


#if defined(__DB_CONNECT_POOL__)
    for(idx=0; idx<ORA_MAX_CONN; idx++)
    {
        if(getpid() == cntx->conn[idx]->pid)
        {
            rst = &(cntx->result[idx]);
            break;
        }
    }
#else /*__DB_CONNECT_POOL__*/
	int	index = 0;
	
	index = ora_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return NULL;
	}
	rst = &(cntx->result[index]);
#endif /*__DB_CONNECT_POOL__*/
    
    if((rowno < 1) || (rowno > rst->rows)
        || (colno < 1) || (colno > rst->cols))
    {
        uLog(SW_LOG_ERROR, "[%s][%d] Out of range! [%d][%d]/[%d][%d]",
            __FILE__, __LINE__, rowno, colno, rst->rows, rst->cols);
        return NULL;
    }

    pos = (rowno - 1)*rst->rowlen;
    for(idx = 1; idx < colno; idx++)
    {
        pos += rst->length[idx-1];
    }
    
    snprintf(name, size, "%s", rst->name[idx-1]);
    return (char *)&rst->result[pos];
}

/******************************************************************************
 **��������: db_ora_set_alias
 **��    ��: ���ñ���
 **�������:
 **     _cntx: ������
 **     name: ����
 **     value: ����ֵ
 **     length: ����ֵ�ĳ���
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
int db_ora_set_alias(void *_cntx, const char *name, const void *value, int length)
{
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    return alias_set(ap, name, value, length);
}

/******************************************************************************
 **��������: db_ora_get_alias
 **��    ��: Get value by alias.
 **�������:
 **     _cntx: ������
 **     name: ����
 **�������:
 **��    ��: ֵ�ĵ�ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.14 #
 ******************************************************************************/
const void *db_ora_get_alias(void *_cntx, const char *name)
{
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    return alias_get(ap, name);
}

/******************************************************************************
 **��������: db_ora_delete_alias
 **��    ��: Delete alias.
 **�������:
 **     _cntx: ������
 **     name: ����
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.14 #
 ******************************************************************************/
int db_ora_delete_alias(void *_cntx, const char *name)
{
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    return alias_delete(ap, name);
}

/******************************************************************************
 **��������: db_ora_delete_all_alias
 **��    ��: Delete all alias.
 **�������:
 **     _cntx: ������
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.14 #
 ******************************************************************************/
int db_ora_delete_all_alias(void *_cntx)
{
    ora_cntx_t *cntx = (ora_cntx_t *)_cntx;
    alias_pool_t *ap = &cntx->ap;

    return alias_delete_all(ap);
}

int db_ora_conn_detect(void *_cntx)
{
    return db_ora_single_query(ORA_DB_CONN_DETECT, _cntx);
}
