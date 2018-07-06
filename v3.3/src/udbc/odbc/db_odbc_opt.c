/******************************************************************************
 *** �������� : Qifeng.zou                                                  ***
 *** ��ʼ���� : 2012.08.21                                                  ***
 *** �������� :                                                             ***
 *** ����ģ�� : ���ݿ�: DB2���ݿ����ģ��                                   ***
 *** �������� : db_odbc_opt.ec                                              ***
 *** �������� : INFORMIX���ݿ����ģ��                                      ***
 *** �޸�˵�� :                                                             ***
 ******************************************************************************/ 
#include "db_odbc_type.h"
#include "pub_log.h"

/* ����ֵ�Ƿ�ɹ� */
#define db_odbc_ret_success(ret) \
	((SQL_SUCCESS == (ret)) || (SQL_SUCCESS_WITH_INFO == (ret)))

static int db_odbc_get_col_desc(odbc_conn_t * conn);
static int db_odbc_get_type(odbc_col_desc_t *desc);
static void db_odbc_free_result(alias_pool_t *ap, odbc_conn_t *conn);
static int db_odbc_bind_row(const char *alias, odbc_cntx_t *cntx, int index, int rowno);
static int db_odbc_rtrim(char *str);

int odbc_get_valid_index(odbc_cntx_t *cntx)
{
	int	i = 0;
	
	for (i = 1; i < ODBC_MAX_CONN; i++)
	{
		if (cntx->conn[i] == NULL)
		{
			return i;
		}
	}
	
	return -1;
}

int odbc_get_conn_index(const char *alias, odbc_cntx_t *cntx)
{
	int	i = 0;
	
	if (alias == NULL)
	{
		return 0;
	}
	
	for (i = 1; i < ODBC_MAX_CONN; i++)
	{
		if (cntx->conn[i] != NULL && strcmp(cntx->conn[i]->alias, alias) == 0)
		{
			return i;
		}
	}
	return -1;
}

/* ���ý�����н������ */
#define db_odbc_set_reqrows(conn, reqrows) \
{\
	if(reqrows <= 0) \
	{ \
		conn->query.req_rows = 1; \
	} \
	else if(reqrows > DB_FETCH_MAX_ROWS) \
	{ \
		conn->query.req_rows = DB_FETCH_MAX_ROWS; \
	} \
	else \
	{ \
		conn->query.req_rows = reqrows; \
	} \
}

#if defined(__DB_CONNECT_POOL__)
/******************************************************************************
 **��������: db_odbc_pcreat
 **��    ��: �������ӳ�
 **�������:
 **      src: ����Դ��Ϣ
 **      cntx: ��������������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.23 #
 ******************************************************************************/
int db_odbc_pcreat(void *source, void **_cntx)
{
	return 0;
}

/******************************************************************************
 **��������: db_odbc_pclose
 **��    ��: �ر����ӳ�
 **�������:
 **      cntx: ��������������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.23 #
 ******************************************************************************/
int db_odbc_pclose(void **_cntx)
{
	return 0;
}
#endif /*__DB_CONNECT_POOL__*/

static void db_odbc_free_cntx(alias_pool_t *ap, odbc_cntx_t *cntx)
{    
	if(cntx->hnqstmt)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, cntx->hnqstmt);
		cntx->hnqstmt = NULL;
	}

	if(cntx->hdbc)
	{
		if(1 == cntx->is_connected)
		{
			SQLDisconnect(cntx->hdbc);
			cntx->is_connected = 0;
		}
        
		SQLFreeHandle(SQL_HANDLE_DBC, cntx->hdbc);
		cntx->hdbc = NULL;
	}

	if(cntx->henv)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, cntx->henv);
		cntx->henv = NULL;
	}
	
	return;
}

/******************************************************************************
 **��������: db_odbc_open
 **��    ��: ���������ݿ�
 **�������:
 **      src: ����Դ��Ϣ
 **      _cntx: ��������������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.24 #
 ******************************************************************************/
int db_odbc_open(void *src, void **_cntx)
{
	int i = 0;
	SQLRETURN ret = 0;
	SQLINTEGER nerror = 0;
	SQLSMALLINT msglen = 0;
	SQLCHAR errmsg[DB_ERR_MSG_MAX_LEN] = {0},
		errstat[ODBC_ERR_STAT_MAX_LEN] = {0};
	odbc_cntx_t *cntx = NULL;
	odbc_conn_t *conn = NULL;
	db_data_src_t *source = (db_data_src_t *)src;
	
	
	/* 1. Alloc space for context */
	cntx = (odbc_cntx_t *)calloc(1, sizeof(odbc_cntx_t));
	if(NULL == cntx)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Memory is not enough!", __FILE__, __LINE__);
		return -1;
	}
	
	do
	{    
		ret = alias_init(&cntx->ap, ODBC_MEM_POOL_SIZE);
		if(0 != ret)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Initialize alais pool failed!", __FILE__, __LINE__);
			break;
		}

		/* 2. Set base data */
		snprintf(cntx->svrname, sizeof(cntx->svrname), "%s", source->svrname);
	
		/* 3. Alloc space for environment handle */
		ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &cntx->henv);
		if(!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alloc environment handle failed!", __FILE__, __LINE__);
			break;
		}
	
		/* 4. Set version attribute */
		ret = SQLSetEnvAttr(cntx->henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
		if(!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Set env attr failed!", __FILE__, __LINE__);
			break;
		}
	
		/* 5. Alloc space for connection handle */
		ret = SQLAllocHandle(SQL_HANDLE_DBC, cntx->henv, &cntx->hdbc);
		if(!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alloc connect handle failed!", __FILE__, __LINE__);
			break;
		}
	
		/* 6. Set login timeout: 5s */
		SQLSetConnectAttr(cntx->hdbc, 
			(SQLINTEGER)SQL_LOGIN_TIMEOUT, 
			(SQLPOINTER)ODBC_CONN_TIMEOUT, (SQLINTEGER)0);
	
	 	/* 7. Connect to data source */
		ret = SQLConnect(cntx->hdbc,
			(SQLCHAR *)source->svrname, SQL_NTS,
			(SQLCHAR *)source->usrname, SQL_NTS, 
			(SQLCHAR *)source->passwd, SQL_NTS);
		if(!db_odbc_ret_success(ret))
		{
			SQLGetDiagRec(SQL_HANDLE_DBC, cntx->hdbc, 1,
				errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
	  	uLog(SW_LOG_ERROR, "[%s][%d] Connect data source failed! errmsg:[%d] %s",
				__FILE__, __LINE__, (int)nerror, (char *)errmsg);
			break;
		}

                ret = SQLAllocHandle(SQL_HANDLE_STMT, cntx->hdbc, &cntx->hnqstmt);
                if(!db_odbc_ret_success(ret))
                {
                        cntx->hnqstmt = NULL;
                        uLog(SW_LOG_ERROR, "[%s][%d] Alloc stmt handle failed!",__FILE__, __LINE__);
                        break;
                }
	
		cntx->is_connected = 1;

		conn = alias_alloc(&cntx->ap, sizeof(odbc_conn_t));
		if (conn == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            		break;
		}

		memset(conn, 0x00, sizeof(odbc_conn_t));
		conn->hqstmt = NULL;
		cntx->conn[0] = conn;
		for (i = 1; i < ODBC_MAX_CONN; i++)
		{
			cntx->conn[i] = NULL;
		}
	
		*_cntx = cntx;
		return 0;
	}while(0);
	
	if (NULL != cntx)
	{
		db_odbc_free_cntx(&cntx->ap, cntx);
		alias_destory(&cntx->ap);
		free(cntx);
		cntx = NULL;
	}
	
	return -1;
}

/******************************************************************************
 **��������: db_odbc_close
 **��    ��: �Ͽ���INFORMIX���ݿ������
 **�������:
 **      _cntx: ��������������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     1. �Ͽ�����
 **     2. �ͷſռ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.24 #
 ******************************************************************************/
int db_odbc_close(void *_cntx)
{
	int idx = 0;
	odbc_conn_t *conn = NULL;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;
	
	for (idx = 0; idx < ODBC_MAX_CONN; idx++)
	{
		conn = cntx->conn[idx];
		if (conn == NULL)
		{
			continue;
		}
			
		db_odbc_free_result(ap, conn);
	}
	
	if (cntx)
	{
		db_odbc_free_cntx(ap, cntx);
		alias_destory(&cntx->ap);
		free(cntx), cntx = NULL;
	}
	
	return 0;
}

/******************************************************************************
 **��������: db_odbc_commit
 **��    ��: �ύ����
 **�������:
 **      _cntx: �������������� 
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **      1. �ع�����
 **      2. ��ʼ�µ�����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.24 #
 ******************************************************************************/
int db_odbc_commit(void *_cntx)
{
	SQLINTEGER ret = 0;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;

	ret = SQLEndTran(SQL_HANDLE_DBC, cntx->hdbc, SQL_COMMIT);
	if(!db_odbc_ret_success(ret))
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Commit transaction failed!", __FILE__, __LINE__);
		return -1;
	}

	ret = SQLSetConnectAttr(cntx->hdbc,
			SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_INTEGER);
	if(!db_odbc_ret_success(ret))
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Set attribute failed!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

/******************************************************************************
 **��������: db_odbc_rollback
 **��    ��: �ع�����
 **�������:
 **      _cntx: ��������������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **      1. �ع�����
 **      2. ��ʼ�µ�����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.24 #
 ******************************************************************************/
int db_odbc_rollback(void *_cntx)
{
	SQLINTEGER ret = 0;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;

	ret = SQLEndTran(SQL_HANDLE_DBC, cntx->hdbc, SQL_ROLLBACK);
	if (!db_odbc_ret_success(ret))
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Rollback transaction failed!", __FILE__, __LINE__);
		return -1;
	}

	ret = SQLSetConnectAttr(cntx->hdbc,
		SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_INTEGER);
	if (!db_odbc_ret_success(ret))
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Set attribute failed!", __FILE__, __LINE__);
		return -1;
	}
    
	return 0;
}

/******************************************************************************
 **��������: db_odbc_nquery
 **��    ��: ִ�зǲ�ѯ�Ķ�̬SQL���
 **�������:
 **      sql: Ҫִ�еķǲ�ѯ��sql���
 **      _cntx: ���ݿ�����������
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.24 #
 ******************************************************************************/
int db_odbc_nquery(const char *sql, void *_cntx)
{
	SQLRETURN ret = 0;
	SQLINTEGER rows = 0;
	SQLINTEGER nerror = 0;
	SQLSMALLINT msglen = 0;
	SQLCHAR errmsg[DB_ERR_MSG_MAX_LEN] = {0},
			errstat[ODBC_ERR_STAT_MAX_LEN] = {0};
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
		
	do
	{
		/* 1. Prepare SQL statement */
		ret = SQLPrepare(cntx->hnqstmt, (SQLCHAR *)sql, SQL_NTS);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Prepare SQL failed! [%s]", __FILE__, __LINE__, sql);
			break;
		}

		ret = SQLExecute(cntx->hnqstmt);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Execute SQL failed! [%s]", __FILE__, __LINE__, sql);
			break;
		}

		ret = SQLRowCount(cntx->hnqstmt, &rows);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] get sql exec rows error.", __FILE__, __LINE__);
			break;
		}

		return rows;
	}while(0);

	SQLGetDiagRec(SQL_HANDLE_STMT, cntx->hnqstmt, 1,
		errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
	uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s",
		__FILE__, __LINE__, (int)nerror, (char *)errmsg);
	return -1;
}

/******************************************************************************
 **��������: db_odbc_update_by_rowid
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
 **         WHERE rowid=x'0A000000000083C0000'���������
 **��    ��: # Qifeng.zou  # 2013.08.21 #
 ******************************************************************************/
int db_odbc_update_by_rowid(const char *_sql, const char *rowid, void *_cntx)
{
	char sql[DB_SQL_MAX_LEN] = {0};

	snprintf(sql, sizeof(sql), "%s WHERE rowid=x'%s'", _sql, rowid);
	return db_odbc_nquery(sql, _cntx);
}

/******************************************************************************
 **��������: db_odbc_mquery
 **��    ��: ִ��һ����̬��ѯsql(���в�ѯ)
 **�������:
 **      sql: Ҫִ�еĲ�ѯ��sql���
 **      req_rows: ����ÿ�ν�����е�����
 **      _cntx: �����ӵ���������Ϣ
 **�������:
 **��    ��: �����е���Ŀ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.24 #
 ******************************************************************************/
int db_odbc_mquery(const char *alias, const char *sql, int req_rows, void *_cntx)
{
	SQLRETURN ret = 0;
	SQLINTEGER nerror = 0;
	SQLSMALLINT msglen = 0;
	SQLCHAR errmsg[DB_ERR_MSG_MAX_LEN] = {0},
		errstat[ODBC_ERR_STAT_MAX_LEN] = {0};
	int	index = 0;
	odbc_conn_t *conn = NULL;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_query_t *query = NULL;

	index = odbc_get_conn_index(alias, cntx);
	if (index == -1)
	{
		index = odbc_get_valid_index(cntx);
		if (index == -1)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] No enough space to save conn info!", __FILE__, __LINE__);
			return -1;
		}
		
		odbc_conn_t *contmp = NULL;
		contmp = (odbc_conn_t *)alias_alloc(&cntx->ap, sizeof(odbc_conn_t));
		if (contmp == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alias alloc error!", __FILE__, __LINE__);
			return -1;
		}
		
		memset(contmp, 0x0, sizeof(odbc_conn_t));
		memset(contmp->alias, 0x00, sizeof(contmp->alias));
		strncpy(contmp->alias, alias, strlen(alias));
		contmp->hqstmt = NULL;
		cntx->conn[index] = contmp;
	}

	conn = cntx->conn[index];
	db_odbc_free_result(&cntx->ap, conn);
	query = &conn->query;
	query->is_nodata = 0;
	db_odbc_set_reqrows(conn, req_rows);
	query->cols = 0;

	do
	{
		ret = SQLAllocHandle(SQL_HANDLE_STMT, cntx->hdbc, &conn->hqstmt);
		if (!db_odbc_ret_success(ret))
		{
			conn->hqstmt = NULL;
			uLog(SW_LOG_ERROR, "[%s][%d] Alloc stmt handle failed!",__FILE__, __LINE__);
			break;
		}

		/* 1. Execute SQL statement */
		ret = SQLExecDirect(conn->hqstmt, (SQLCHAR *)sql, SQL_NTS);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR,
				"[%s][%d] Execute SQL failed! [%s]", __FILE__, __LINE__, sql);
			break;
		}
		
		/* 2. Alloc space for columns */
		ret = SQLNumResultCols(conn->hqstmt, (SQLPOINTER)&query->cols);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Compute cols failed! [%s]", __FILE__, __LINE__, sql);
			break;
		}
		
		/* 3. Get desc of columns */
		query->col_desc = 
			(odbc_col_desc_t *)alias_alloc(&cntx->ap, query->cols * sizeof(odbc_col_desc_t));
		if (NULL == query->col_desc)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alloc desc failed!", __FILE__, __LINE__);
			break;
		}
		
		ret = db_odbc_get_col_desc(conn);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Get desc failed!", __FILE__, __LINE__);
			break;
		}

		return query->cols;
	}while(0);
    
	SQLGetDiagRec(SQL_HANDLE_STMT, conn->hqstmt, 1,
		errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
	uLog(SW_LOG_ERROR, "[%s][%d] Execute SQL failed! errmsg:[%d] %s [%s]",
		__FILE__, __LINE__, (int)nerror, (char *)errmsg, sql);

	db_odbc_free_result(&cntx->ap, conn);
	alias_free(&cntx->ap, conn);
	conn = NULL;
	
	return -1;
}

/******************************************************************************
 **��������: db_odbc_fetch
 **��    ��: ȡ��һ����̬��ѯsql�α��һ����¼
 **�������:
 **      _cntx: ���ݿ�����������
 **�������:
 **��    ��: ��ȡ������
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.15 #
 ******************************************************************************/
int db_odbc_fetch(void *_cntx)
{
	int idx = 0;
	SQLRETURN ret = 0;
	SQLINTEGER nerror = 0;
	SQLSMALLINT msglen = 0;
	SQLCHAR errmsg[DB_ERR_MSG_MAX_LEN] = {0},
		errstat[ODBC_ERR_STAT_MAX_LEN] = {0};
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_conn_t *conn = cntx->conn[0];
	odbc_query_t *query = &conn->query;

	if (query->is_nodata)
	{
		return 0;
	}

	query->rows = 0;
	for (idx = 0; idx < query->req_rows; idx++)
	{
		ret = db_odbc_bind_row(NULL, cntx, 0, idx);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Bind row failed!", __FILE__, __LINE__);
			break;
		}

		ret = SQLFetch(conn->hqstmt);
		switch(ret)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			{
				query->rows++;
				break;
			}
			case SQL_NO_DATA:
			{
				query->is_nodata = 1;

				/* END OF FETCH */
				return query->rows;
			}
			case SQL_ERROR:
			default:
			{
				SQLGetDiagRec(SQL_HANDLE_STMT, conn->hqstmt, 1,
					errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
				uLog(SW_LOG_ERROR, "[%s][%d] Execute SQL failed! errmsg:[%d] %s",
					__FILE__, __LINE__, (int)nerror, (char *)errmsg);
				return -1;
			}
		}
	}

	if (idx == query->req_rows)
	{
		return query->rows;
	}

	SQLGetDiagRec(SQL_HANDLE_STMT, conn->hqstmt, 1,
		errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
	uLog(SW_LOG_ERROR, "[%s][%d] Execute SQL failed! errmsg:[%d] %s",
		__FILE__, __LINE__, (int)nerror, (char *)errmsg);

	return -1;
}

int db_odbc_mfetch(const char *alias, void *_cntx)
{
	int idx = 0;
	SQLRETURN ret = 0;
	SQLINTEGER nerror = 0;
	SQLSMALLINT msglen = 0;
	SQLCHAR errmsg[DB_ERR_MSG_MAX_LEN] = {0},
		errstat[ODBC_ERR_STAT_MAX_LEN] = {0};
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_conn_t *conn = NULL;
	odbc_query_t *query = NULL;
	
	int	index = 0;
	index = odbc_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return -1;
	}
		
	conn = cntx->conn[index];
	query = &conn->query;
		
	if (query->is_nodata)
	{
		return 0;
	}

	query->rows = 0;
	for (idx = 0; idx < query->req_rows; idx++)
	{
		ret = db_odbc_bind_row(alias, cntx, index, idx);
		if (ret < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Bind row failed!", __FILE__, __LINE__);
			break;
		}

		ret = SQLFetch(conn->hqstmt);
		switch(ret)
		{
			case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
			{
				query->rows++;
				break;
			}
			case SQL_NO_DATA:
			{
				query->is_nodata = 1;

				/* END OF FETCH */
				return query->rows;
			}
			case SQL_ERROR:
			default:
			{
				SQLGetDiagRec(SQL_HANDLE_STMT, conn->hqstmt, 1,
					errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
				uLog(SW_LOG_ERROR, "[%s][%d] Execute SQL failed! errmsg:[%d] %s",
					__FILE__, __LINE__, (int)nerror, (char *)errmsg);
				return -1;
			}
		}
	}

	if(idx == query->req_rows)
	{
		return query->rows;
	}

	SQLGetDiagRec(SQL_HANDLE_STMT, conn->hqstmt, 1,
		errstat, &nerror, errmsg, sizeof(errmsg), &msglen);
	uLog(SW_LOG_ERROR, "[%s][%d] Execute SQL failed! errmsg:[%d] %s",
		__FILE__, __LINE__, (int)nerror, (char *)errmsg);

	return -1;
}

/******************************************************************************
 **��������: db_odbc_cclose
 **��    ��: �ͷ��α�
 **�������:
 **      _cntx: ���ݿ�������
 **�������:
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **ע������: DB2�����ͷ��α�
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
int db_odbc_cclose(const char *alias, void *_cntx) 
{
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	int i = 0;
		
	for (i = 0; i < ODBC_MAX_CONN; i++)
	{
		if (cntx->conn[i] != NULL && strcmp(cntx->conn[i]->alias, alias) == 0)
		{
			db_odbc_free_result(&cntx->ap, cntx->conn[i]);
			alias_free(&cntx->ap, cntx->conn[i]);
			cntx->conn[i] = NULL;
			return 0;
		}
	}
	
	uLog(SW_LOG_ERROR, "[%s][%d]not find [%s] cursor", __FILE__, __LINE__, alias);
	return -1;
}

/******************************************************************************
 **��������: db_odbc_squery
 **��    ��: ���в�ѯ
 **�������:
 **      sql: Ҫִ�еĲ�ѯ��sql���
 **      _cntx: �����ӵ���������Ϣ
 **�������:
 **��    ��: ��������
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.27 #
 ******************************************************************************/
int db_odbc_squery(const char *sql, void *_cntx)
{
	int cols = 0, rows = 0;

	cols = db_odbc_mquery(NULL, sql, 1, _cntx);
	if (cols < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Execute query failed!", __FILE__, __LINE__);
		return -1;
	}

	rows = db_odbc_fetch(_cntx);
	if (rows < 0)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Fetch data failed!", __FILE__, __LINE__);
		return -1;
	}
	else if(0 == rows)
	{
		return 0;
	}

	return cols;
}

/******************************************************************************
 **��������: db_odbc_get_data_by_idx
 **��    ��: ͨ��ָ���е��кţ���ȡָ���е�ֵ
 **�������:
 **      rowno: �к�(ȡֵ��Χ: 1~X)
 **      colno: �к�(ȡֵ��Χ: 1~Y)
 **      _cntx: ���ݿ����ӵ�������
 **�������:
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
char *db_odbc_get_data_by_idx(const char *alias, int rowno, int colno, void *_cntx)
{
	odbc_row_data_t *row_data = NULL;
	odbc_col_data_t *col_data = NULL;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_query_t *query = NULL;

	int	index = 0;
	
	index = odbc_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return NULL;
	}
	query = &(cntx->conn[index]->query);

	if(colno <= 0
		|| colno > query->cols
		|| rowno <= 0
		|| rowno > query->rows)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Out of range! rowno:%d colno:%d",
			__FILE__, __LINE__, rowno, colno);
		return NULL;
	}

	/* 1. ��λ�� */
	row_data = query->row_data + rowno - 1;

	/* 2. ��λ�� */
	col_data = row_data->col_data + colno - 1;
	
	db_odbc_rtrim(col_data->buffer);
	return col_data->buffer;
}

/******************************************************************************
 **��������: db_odbc_get_data_by_name
 **��    ��: ͨ��ָ���е���������ȡ��ֵ
 **�������:
 **      rowno: �к�(ȡֵ��Χ: 1~X)
 **      name: ����
 **      _cntx: ���ݿ����ӵ�������
 **�������:
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
char *db_odbc_get_data_by_name(const char *alias,int rowno, const char *name, void *_cntx)
{
	int idx = 0;
	odbc_col_desc_t *desc = NULL;
	odbc_row_data_t *row_data = NULL;
	odbc_col_data_t *col_data = NULL;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_query_t *query = NULL;

	int	index = 0;
	
	index = odbc_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return NULL;
	}
	query = &(cntx->conn[index]->query);

	if(rowno <= 0
		|| rowno > query->rows)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Out of range! rowno:%d name:%s",
			__FILE__, __LINE__, rowno, name);
		return NULL;
	}

	/* 1. ��λ�� */
	row_data = query->row_data+rowno-1;

	/* 2. ��λ�� */
	col_data = row_data->col_data;
	desc = &query->col_desc[0];
	for (idx = 0; idx < query->cols; idx++, desc++)
	{
		if (0 == strcasecmp(name, desc->name))
		{
			break;
		}
	}

	if (idx == query->cols)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Didn't find column [%s]!",
			__FILE__, __LINE__, name);
		return NULL;
	}

	col_data += idx;

	/* 3. ���ؽ�� */
	db_odbc_rtrim(col_data->buffer);
    
	return col_data->buffer;
}

/******************************************************************************
 **��������: db_odbc_get_data_and_name
 **��    ��: ͨ��ָ�����У���ȡ��ֵ������
 **�������:
 **     rowno: �к�(ȡֵ��Χ: 1~X)
 **     colno: �к�(ȡֵ��Χ: 1~Y)
 **     size: ��������ĳߴ�
 **     _cntx: ���ݿ����ӵ�������
 **�������:
 **     name: ����
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
char *db_odbc_get_data_and_name(const char *alias,int rowno, int colno, char *name, int size, void *_cntx)
{
	odbc_col_desc_t *desc = NULL;
	odbc_row_data_t *row_data = NULL;
	odbc_col_data_t *col_data = NULL;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_query_t *query = NULL;
	
	int	index = 0;
	
	index = odbc_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return NULL;
	}
	query = &(cntx->conn[index]->query);

	if (colno <= 0
		|| colno > query->cols
		|| rowno <= 0
		|| rowno > query->rows)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Out of range! rowno:%d colno:%d",
			__FILE__, __LINE__, rowno, colno);
		return NULL;
	}

	/* 1. ��λ�� */
	row_data = query->row_data+rowno-1;

	/* 2. ��λ�� */
	col_data = row_data->col_data+colno-1;

	desc = query->col_desc+colno-1;  
	snprintf(name, size, "%s", desc->name);
	db_odbc_rtrim(col_data->buffer);

	return col_data->buffer;
}

/******************************************************************************
 **��������: db_odbc_get_data_type
 **��    ��: ��ȡָ���е���������
 **�������:
 **      col: �к�(ȡֵ��Χ: 1~Y)
 **      _cntx: ���ݿ����ӵ�������
 **�������:
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
int db_odbc_get_data_type(const char *alias, int col, void *_cntx)
{
	odbc_col_desc_t *desc = NULL;
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	odbc_query_t *query = NULL;

	int	index = 0;
	
	index = odbc_get_conn_index(alias, cntx);
	if (index == -1)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Can not find [%s]'s conn info!", __FILE__, __LINE__, alias);
		return -1;
	}
	query = &(cntx->conn[index]->query);

	if ((col <= 0) || (col > query->cols))
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Out of range! col:%d",
			__FILE__, __LINE__, col);
		return DB_DTYPE_UNKNOW;
	}

	desc = query->col_desc + col;  
	return db_odbc_get_type(desc);
}

/******************************************************************************
 **��������: db_odbc_free_result
 **��    ��: �ͷŽ�����ҿռ�
 **�������:
 **      cntx: ��������Ϣ
 **�������:
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
static void db_odbc_free_result(alias_pool_t *ap, odbc_conn_t *conn)
{
	int row = 0, col = 0;
	odbc_row_data_t *row_data = NULL;
	odbc_col_data_t *col_data = NULL;
	odbc_query_t *query = &conn->query;

	if (conn->hqstmt)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, conn->hqstmt);
		conn->hqstmt = NULL;
	}

	/* 1. Release query result */
	row_data = &query->row_data[0];
	for (row = 0; row < DB_FETCH_MAX_ROWS; row++, row_data++)
	{
		if (NULL == row_data->col_data)
		{
			break;
		}

		col_data = row_data->col_data;
		for (col = 0; col< query->cols; col++, col_data++)
		{
			if (NULL != col_data->buffer)
			{
				alias_free(ap, col_data->buffer);
				col_data->buffer = NULL;
			}
		}

		alias_free(ap, row_data->col_data);
		row_data->col_data = NULL;
	}

	alias_free(ap, query->col_desc);
	query->col_desc = NULL;

	return;
}

/******************************************************************************
 **��������: db_odbc_get_type
 **��    ��: ��struct sqlda����ȡ��������
 **�������:
 **      desc: �е�������Ϣ
 **�������:
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
static int db_odbc_get_type(odbc_col_desc_t *desc)
{
	switch (desc->type)
	{
		case SQL_BIT:
		{
			return DB_DTYPE_BOOL;
		}
		case SQL_SMALLINT:
		{
			return DB_DTYPE_SMINT;
		}
		case SQL_INTEGER:
		{
			return DB_DTYPE_INT;
		}
		case SQL_BIGINT:
		{
			return DB_DTYPE_BIGINT;
		}
		case SQL_DECIMAL:
		case SQL_NUMERIC:
		{
			return DB_DTYPE_DECIMAL;
		}
		case SQL_REAL:
		{
			return DB_DTYPE_REAL;
		}
		case SQL_FLOAT:
		{
			return DB_DTYPE_SMFLOAT;
		}
		case SQL_DOUBLE:
		{
			return DB_DTYPE_FLOAT;
		}
		case SQL_CHAR:
		{
			return DB_DTYPE_CHAR;
		}
		case SQL_VARCHAR:
		{
			return DB_DTYPE_VCHAR;
		}
		case SQL_TYPE_DATE:
		{
			return DB_DTYPE_DATE;
		}
		case SQL_TYPE_TIME:
		{
			return DB_DTYPE_TIME;
		}
		case SQL_TYPE_TIMESTAMP:
		{
			return DB_DTYPE_TIMESTAMP;
		}
		case SQL_LONGVARCHAR:
		{
			return DB_DTYPE_CLOB;
		}
		case SQL_LONGVARBINARY:
		{
			return DB_DTYPE_BLOB;
		}
	} 

	return DB_DTYPE_UNKNOW;
}

/******************************************************************************
 **��������: db_odbc_get_col_length
 **��    ��: ��ȡָ���еĳ���
 **�������:
 **     hstmt: SQL�����
 **     col: �к�
 **     type: ��������
 **�������:
 **��    ��: �������͵ĳ���
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
static SQLLEN db_odbc_get_col_length(SQLHSTMT hstmt, SQLSMALLINT col, SQLSMALLINT type)
{
	SQLLEN len = 0;
	SQLRETURN ret = 0;

	switch(type)
	{
		case SQL_SMALLINT:
		{
			return 7;
		}
		case SQL_INTEGER:
		{
			return 12;
		}
		case SQL_BIGINT:
		{
			return 22;
		}        
		case SQL_DECIMAL:
		case SQL_NUMERIC:
		{
			return 18;
		}
		case SQL_REAL:
		{
			return 42;
		}
		case SQL_DOUBLE:
		case SQL_FLOAT:
		{
			return 312;
		}
		case SQL_DATE:
		case SQL_TYPE_DATE:
		{
			return 11;
		}
		case SQL_TIME:
		case SQL_TYPE_TIME:
		{
			return 9;
		}
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
		{
			return 24;
		}
		case SQL_VARBINARY:
		{
			ret = SQLColAttribute(hstmt, col, SQL_COLUMN_LENGTH, NULL, 0, NULL, &len);
			if (!db_odbc_ret_success(ret))
			{
				uLog(SW_LOG_ERROR, "[%s][%d] Get length of column failed", __FILE__, __LINE__);
				return -1;
			}
			return (2 * len + 1);
		}
	}

	ret = SQLColAttribute(hstmt, col, SQL_COLUMN_LENGTH, NULL, 0, NULL, &len);
	if (!db_odbc_ret_success(ret))
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get length of column failed", __FILE__, __LINE__);
		return -1;
	}

	return len + 1;
}

/******************************************************************************
 **��������: db_odbc_get_col_desc
 **��    ��: ��ȡ�е�������Ϣ
 **�������:
 **     cntx: ������
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
static int db_odbc_get_col_desc(odbc_conn_t * conn)
{
	int idx = 0;
	SQLRETURN ret = 0;
	odbc_col_desc_t *desc = NULL;

	desc = &(conn->query.col_desc[0]);
	for (idx = 0; idx < conn->query.cols; idx++, desc++)
	{
		ret = SQLDescribeCol(conn->hqstmt,
			(SQLUSMALLINT)idx + 1,
			(SQLCHAR *)desc->name,
			(SQLSMALLINT)sizeof(desc->name),
			(SQLSMALLINT *)&desc->name_len,
			(SQLSMALLINT *)&desc->type,
			(SQLULEN *)&desc->precision,
			(SQLSMALLINT *)&desc->scale,
			(SQLSMALLINT *)&desc->nullable);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Get describtion of column failed! idx:[%d]",
				__FILE__, __LINE__, idx);
			return -1;
		}

		/* Get length of column buffer */
		desc->buflen = db_odbc_get_col_length(conn->hqstmt, idx + 1, desc->type);
		if (desc->buflen < 0)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Get length of column failed! idx:[%d]",
				__FILE__, __LINE__, idx);
			return -1;
		}
	}

	return 0;
}

/******************************************************************************
 **��������: db_odbc_bind_row
 **��    ��: ͨ���а���
 **�������:
 **     cntx: ������
 **     rowno: �к�
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
static int db_odbc_bind_row(const char *alias, odbc_cntx_t *cntx, int index, int rowno)
{
	int idx = 0;
	SQLRETURN ret = 0;
	odbc_col_data_t *col_data = NULL;
	odbc_query_t *query = &cntx->conn[index]->query;
	odbc_col_desc_t *desc = NULL;
	odbc_row_data_t *row_data = &query->row_data[rowno];

	/* 1. Point to the first column of special row,
		Alloc memory for all columns if it's NULL */
	col_data = row_data->col_data;
	if (NULL == col_data)
	{
		col_data = (odbc_col_data_t *)alias_alloc(
			&cntx->ap, query->cols * sizeof(odbc_col_data_t));
		if (NULL == col_data)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
			return -1;
		}
		row_data->col_data = col_data;
	}

	/* 2. Bind special column */
	for (idx = 0; idx < query->cols; idx++, col_data++)
	{
		/* 2.1 Get the description of special column */
		desc = cntx->conn[index]->query.col_desc + idx;

		/* 2.2 Get length of data type */
		col_data->buflen = desc->buflen;

		/* 2.3 Alloc space for the buffer of column */
		if (NULL == col_data->buffer)
		{
			col_data->buffer = (SQLPOINTER)alias_alloc(&cntx->ap, col_data->buflen * sizeof(char));
			if (NULL == col_data->buffer)
			{
				uLog(SW_LOG_ERROR, "[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
				return -1;
			}
		}

		/* 2.4 Bind buffer to special column of result */
		ret = SQLBindCol(cntx->conn[index]->hqstmt, idx + 1, SQL_C_CHAR,
			col_data->buffer, col_data->buflen, &col_data->null_ind);
		if (!db_odbc_ret_success(ret))
		{
			uLog(SW_LOG_ERROR, "[%s][%d] Bind column failed!", __FILE__, __LINE__);
			return -1;
		}
	}
	
	return 0;
}

/******************************************************************************
 **��������: db_odbc_rtrim
 **��    ��: ����ɾ���ַ����ո�
 **�������:
 **     str: �豻������ַ���
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.29 #
 ******************************************************************************/
static int db_odbc_rtrim(char *str)
{
	char *p = NULL;
	int idx = 0, len = 0;

	len = strlen(str);
	idx = len - 1;
	p = str + idx;

	for (; idx >= 0; idx--)
	{
		if (' ' == *p)
		{
			*p = '\0';
			p--;
			continue;
		}
		break;
	}

	return 0;
}

/******************************************************************************
 **��������: db_odbc_set_alias
 **��    ��: Set alias
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
int db_odbc_set_alias(void *_cntx, const char *name, const void *value, int length)
{
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_set(ap, name, value, length);
}

/******************************************************************************
 **��������: db_odbc_get_alias
 **��    ��: Get value by alias.
 **�������:
 **     _cntx: ������
 **     name: ����
 **�������:
 **��    ��: ֵ�ĵ�ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
const void *db_odbc_get_alias(void *_cntx, const char *name)
{
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_get(ap, name);
}

/******************************************************************************
 **��������: db_odbc_delete_alias
 **��    ��: Delete special alias.
 **�������:
 **     _cntx: ������
 **     name: ����
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
int db_odbc_delete_alias(void *_cntx, const char *name)
{
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_delete(ap, name);
}

/******************************************************************************
 **��������: db_odbc_delete_all_alias
 **��    ��: Delete all alias.
 **�������:
 **     _cntx: Context of connection.
 **�������:
 **��    ��: 0:�ɹ�  !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
int db_odbc_delete_all_alias(void *_cntx)
{
	odbc_cntx_t *cntx = (odbc_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_delete_all(ap);
}

int db_odbc_conn_detect(void *_cntx)
{
	return db_odbc_squery(ODBC_DB_CONN_DETECT, _cntx);
}

int db_odbc_del_all_conn(void *_cntx)
{
	int i = 0;
	odbc_cntx_t *cntx = _cntx;
	if (cntx == NULL)
	{
		return ;
	}
	
	for (i = 1; i < ODBC_MAX_CONN; i++)
	{
		if (cntx->conn[i] != NULL)
		{
			db_odbc_free_result(&cntx->ap, cntx->conn[i]);
			alias_free(&cntx->ap, cntx->conn[i]);
			cntx->conn[i] = NULL;
		}
	}
	return 0;
}
