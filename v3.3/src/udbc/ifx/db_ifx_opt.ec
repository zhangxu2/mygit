/******************************************************************************
 *** 程序作者 : Qifeng.zou                                                  ***
 *** 开始日期 : 2012.08.21                                                  ***
 *** 结束日期 :                                                             ***
 *** 所属模块 : 数据库: INFORMIX数据库操作模块                              ***
 *** 程序名称 : db_ifx_opt.ec                                               ***
 *** 程序作用 : INFORMIX数据库操作模块                                      ***
 *** 修改说明 :                                                             ***
 ******************************************************************************/ 
#include "db_ifx_gport.h"
#include "pub_type.h"
#include "pub_log.h"

EXEC SQL INCLUDE sqlca;
EXEC SQL INCLUDE sqlda;
EXEC SQL INCLUDE sqlhdr;
EXEC SQL INCLUDE decimal;
EXEC SQL INCLUDE locator;
EXEC SQL INCLUDE varchar;
EXEC SQL INCLUDE datetime;
EXEC SQL INCLUDE sqlstype;
EXEC SQL INCLUDE sqltypes;
EXEC SQL INCLUDE sqlstype;


EXEC SQL DEFINE SQL_USR_NAME_MAX_LEN 64;  /* User name max len */
EXEC SQL DEFINE SQL_PASSWD_MAX_LEN 64;    /* Password max len */
EXEC SQL DEFINE SQL_DB_NAME_MAX_LEN 64;   /* Database name max len */
EXEC SQL DEFINE SQL_CONN_NAME_MAX_LEN 64; /* Connect name max len */
EXEC SQL DEFINE SQL_STMT_MAX_LEN 1024;    /* SQL statement max len */

#define __DB_IFX_TRANS_SUPPORT__
#define __DB_IFX_FETARRSIZE_ZERO__

/*pthread_mutex_t db_ifx_mutex = PTHREAD_MUTEX_INITIALIZER;*/

static int db_ifx_link_sqlda(ifx_sqlda_t *sqlda, int rows);
static int db_ifx_init_sqlda(ifx_dbconn_t *conn, ifx_sqlda_t *sqlda);
static int db_ifx_set_sqltype(struct sqlvar_struct *sqlvar);
static int db_ifx_init_sqldata(ifx_dbconn_t *conn, struct sqlvar_struct *sqlvar, int alloc_num);
static int db_ifx_alloc_loc(ifx_dbconn_t *conn, struct sqlvar_struct *sqlvar, int alloc_num);
static int db_ifx_free_sqlda(ifx_sqlda_t *result);
static char *db_ifx_get_value(struct sqlvar_struct *sqlvar, ifx_dbconn_t *conn);
static int db_ifx_get_type(struct sqlvar_struct *sqlvar);

static int db_ifx_scfetch(ifx_dbconn_t *conn);
static int db_ifx_scclose(ifx_dbconn_t *conn);

static int db_ifx_init_result(ifx_dbconn_t *conn, ifx_sqlda_t *sqlda);
static void db_ifx_free_result(ifx_dbconn_t *conn);
static void db_ifx_free_all_result(ifx_cntx_t *cntx);

static int db_ifx_exec_squery(ifx_dbconn_t *conn);
static void db_ifx_set_fetarrsize(ifx_dbconn_t *conn, ifx_sqlda_t *sqlda);

/* Release context */
#define db_ifx_free_cntx(cntx) \
{ \
	if(NULL != cntx) \
	{ \
		db_ifx_free_all_result(cntx); \
		free(cntx), cntx = NULL; \
	} \
}

#if defined(__DB_CONNECT_POOL__)
/******************************************************************************
 **函数名称: db_ifx_pcreat
 **功    能: Create connection pool
 **输入参数:
 **      src: data source
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.23 #
 ******************************************************************************/
int db_ifx_pcreat(void *src, void **cntx)
{
	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_pclose
 **功    能: Close connection pool
 **输入参数:
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.23 #
 ******************************************************************************/
int db_ifx_pclose(void **cntx)
{
	return 0;
}
#endif /*__DB_CONNECT_POOL__*/


static ifx_dbconn_t *ifx_get_valid_conn(const char *alias, ifx_cntx_t *cntx)
{
	int     i = 0;
	int	idx = -1;
	ifx_dbconn_t	*conn = NULL;

	for (i = 1; i < IFX_MAX_CONN; i++)
	{
		if (cntx->conn[i].used == 0)
		{
			idx = i;
			break;
		}
	}
	if (i == IFX_MAX_CONN)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] No enough space to save conn info!", __FILE__, __LINE__);
		return NULL;
	}
	conn = &cntx->conn[idx];
	conn->used = 1;
	memset(conn->alias, 0x0, sizeof(conn->alias));
	memcpy(conn->alias, alias, sizeof(conn->alias) - 1);
	conn->ap = &cntx->ap;

	return conn;
}

static ifx_dbconn_t *ifx_get_alias_conn(const char *alias, ifx_cntx_t *cntx)
{
	int     i = 0;
	int	idx = -1;
	ifx_dbconn_t	*conn = NULL;

	if (alias == NULL)
	{
		idx = 0;
	}
	else
	{
		for (i = 1; i < IFX_MAX_CONN; i++)
		{
			if (strcmp(cntx->conn[i].alias, alias) == 0)
			{
				idx = i;
				break;
			}
		}
		if (i == IFX_MAX_CONN)
		{
			return NULL;
		}
	}
	conn = &cntx->conn[idx];

	return conn;
}

/******************************************************************************
 **函数名称: db_ifx_open
 **功    能: Connect to database
 **输入参数:
 **      src: Data source information
 **      _cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.14 #
 ******************************************************************************/
int db_ifx_open(void *src, void **_cntx)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char svrname[SQL_DB_NAME_MAX_LEN] = {0},      /* server name */
	     cnname[SQL_CONN_NAME_MAX_LEN] = {0},     /* connection name */
	     usrname[SQL_USR_NAME_MAX_LEN] = {0},     /* user name */
	     passwd[SQL_PASSWD_MAX_LEN] = {0};        /* password */
	EXEC SQL END DECLARE SECTION;
	int	i = 0;
	int ret = 0;
	ifx_cntx_t *cntx = NULL;
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	db_data_src_t *source = (db_data_src_t *)src;

	/* 1. Alloc for context */
	cntx = (ifx_cntx_t *)calloc(1, sizeof(ifx_cntx_t));
	if(NULL == cntx)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
				__FILE__, __LINE__, errno, strerror(errno)); 
		return -1;
	}

	/* 2. Initialize alias pool */
	ret = alias_init(&cntx->ap, IFX_MEM_POOL_SIZE);
	if(0 != ret)
	{
		free(cntx), cntx=NULL;
		uLog(SW_LOG_ERROR, "[%s][%d] Initialize alias pool failed!", __FILE__, __LINE__);
		return -1;
	}
	
	for (i = 0; i < IFX_MAX_CONN; i++)
	{
		memset(&cntx->conn[i], 0x0, sizeof(cntx->conn[i]));
		cntx->conn[i].used = 0;
	}
	
	*_cntx = cntx;

	/* 3. Set database connection information */
	cntx->pid = getpid();
	cntx->tid = pthread_self();
	snprintf(cntx->cnname, sizeof(cntx->cnname), "ifxdb:%d:%d", cntx->pid, cntx->tid);
	snprintf(cnname, sizeof(cnname), "%s", cntx->cnname);

	snprintf(svrname, sizeof(svrname), "%s", source->svrname);
	snprintf(cntx->svrname, sizeof(cntx->svrname), "%s", source->svrname);
	snprintf(usrname, sizeof(usrname), "%s", source->usrname);
	snprintf(passwd, sizeof(passwd), "%s", source->passwd);

	/* 4. Connect to database */
	if (usrname[0] != '\0' && passwd[0] != '\0')
	{
		EXEC SQL CONNECT TO :svrname AS :cnname USER :usrname USING :passwd;    
	}
	else
	{
		EXEC SQL CONNECT TO :svrname AS :cnname;
	}
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] cnname:%s sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, cntx->cnname, sqlca.sqlcode, errmsg);

		EXEC SQL DISCONNECT :cnname;

		db_ifx_free_cntx(cntx);

		return -1;
	}

	EXEC SQL SET CONNECTION :cnname;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] cnname:%s sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, cntx->cnname, sqlca.sqlcode, errmsg);

		return -1;
	}

	EXEC SQL SET LOCK MODE TO WAIT 5;
#if defined(__DB_IFX_TRANS_SUPPORT__)
	EXEC SQL BEGIN WORK;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] cnname:%s sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, cntx->cnname, sqlca.sqlcode, errmsg);

		return -1;
	}
	EXEC SQL  set isolation to committed read last committed;
	if (sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] cnname:%s sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, cntx->cnname, sqlca.sqlcode, errmsg);
		return -1;
	}
	uLog(SW_LOG_INFO, "[%s][%d] Open [%s] success!", __FILE__, __LINE__, cntx->cnname);
#endif /*__DB_IFX_TRANS_SUPPORT__*/

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_close
 **功    能: Disconnect with database
 **输入参数:
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **      1. Commit transaction
 **      2. Disconnect with database
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.14 #
 ******************************************************************************/
int db_ifx_close(void *_cntx)
{
	EXEC SQL BEGIN DECLARE SECTION; 
	char cnname[SQL_CONN_NAME_MAX_LEN] = {0};
	EXEC SQL END DECLARE SECTION; 

	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;


	snprintf(cnname, sizeof(cnname), "%s", cntx->cnname);

#if defined(__DB_IFX_TRANS_SUPPORT__)
	EXEC SQL COMMIT WORK;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] cnname:%s sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, cntx->cnname, sqlca.sqlcode, errmsg);

		return -1;
	}
#endif /*__DB_IFX_TRANS_SUPPORT__*/

	/* Disconnect */
	EXEC SQL DISCONNECT :cnname;    
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] cnname:%s sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, cntx->cnname, sqlca.sqlcode, errmsg);

		return -1;
	}

	db_ifx_free_cntx(cntx);

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_commit
 **功    能: Commit transaction
 **输入参数:
 **      cntx: Context of connection 
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **      1. Rollback transaction
 **      2. Begin new transaction
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.14 #
 ******************************************************************************/
int db_ifx_commit(void *_cntx)
{
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;


#if defined(__DB_IFX_TRANS_SUPPORT__)
	EXEC SQL COMMIT WORK;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] sqlcode:%d errmsg:%s", __FILE__, __LINE__, sqlca.sqlcode, errmsg);

		return -1;
	}

	EXEC SQL BEGIN WORK;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] sqlcode:%d errmsg:%s", __FILE__, __LINE__, sqlca.sqlcode, errmsg);

		return -1;
	}    
#endif /*__DB_IFX_TRANS_SUPPORT__*/

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_rollback
 **功    能: Rollback transaction
 **输入参数:
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **      1. Rollback transaction
 **      2. Begin new transaction
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.14 #
 ******************************************************************************/
int db_ifx_rollback(void *_cntx)
{
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;


#if defined(__DB_IFX_TRANS_SUPPORT__)
	EXEC SQL ROLLBACK WORK;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s", __FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}

	EXEC SQL BEGIN WORK;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s", __FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}    
#endif /*__DB_IFX_TRANS_SUPPORT__*/

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_nquery
 **功    能: Execute non-query SQL statement
 **输入参数:
 **      sql: Non-query SQL statement
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.14 #
 ******************************************************************************/
int db_ifx_nquery(const char *sql, void *_cntx)
{
	EXEC SQL BEGIN DECLARE SECTION; 
	char sql_stmt[SQL_STMT_MAX_LEN] = {0};
	EXEC SQL END DECLARE SECTION; 

	int rows = 0;
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;

	snprintf(sql_stmt, sizeof(sql_stmt), "%s", sql);

	EXEC SQL PREPARE NON_QUERY_SQL FROM :sql_stmt;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] sqlcode:%d errmsg:%s[%s]",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);
		return -1;
	}

	/* Execute non-query sql statement */
	EXEC SQL EXECUTE NON_QUERY_SQL;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR, "[%s][%d] sqlcode:%d errmsg:%s[%s]",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);
		return -1;
	}
	rows = sqlca.sqlerrd[2];
	pub_log_info("[%s][%d] processed rows=[%d]", __FILE__, __LINE__, rows);

	return rows;
}

/******************************************************************************
 **函数名称: db_ifx_update_by_rowid
 **功    能: 通过rowid更新数据库数据
 **输入参数:
 **     sql: UPDATE [tablename] SET [colname=value] [, ...]
 **     rowid: 行号
 **     _cntx: Context of connection
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **     1. 拼接SQL语句
 **     2. 执行SQL语句
 **注意事项: 
 **     UPDATE student
 **         SET id='0001', name='zouqifeng'
 **         WHERE rowid='510'等类似语句
 **作    者: # Qifeng.zou  # 2013.08.21 #
 ******************************************************************************/
int db_ifx_update_by_rowid(const char *_sql, const char *rowid, void *_cntx)
{
	char sql[DB_SQL_MAX_LEN] = {0};

	snprintf(sql, sizeof(sql), "%s WHERE rowid='%s'", _sql, rowid);

	return db_ifx_nquery(sql, _cntx);
}

/******************************************************************************
 **函数名称: db_ifx_mquery
 **功    能: Execute query SQL statement(Mul-line query)
 **输入参数:
 **      sql: Query SQL statement
 **      req_rows: 请求每次结果集中的行数
 **      _cntx: Context of connection
 **输出参数:
 **返    回: Number of column
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.15 #
 ******************************************************************************/
int db_ifx_mquery(const char *alias, const char *sql, int req_rows, void *_cntx)
{
	EXEC SQL BEGIN DECLARE SECTION; 
	char sql_stmt[SQL_STMT_MAX_LEN] = {0};
	char	query_sql[128];
	char	query_cursor[128];
	EXEC SQL END DECLARE SECTION; 

	int	ret = 0;
	int	colnum = 0;
	char	errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_dbconn_t	*conn = NULL;
	ifx_sqlda_t	*sqlda = NULL;
	ifx_cntx_t	*cntx = (ifx_cntx_t *)_cntx;

	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		conn = ifx_get_valid_conn(alias, cntx);
		if (conn == NULL)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] No enough space to save conn info!", __FILE__, __LINE__);
			return -1;
		}
	}

	/* 1. Avoid memory leak */
	if(1 == conn->isalloced)
	{
		db_ifx_free_result(conn);
	}

	/* 2. 设置每次获取结果行数 */
	conn->req_rows = req_rows;
	conn->is_nodata = 0;

	snprintf(sql_stmt, sizeof(sql_stmt), "%s", sql);

	/* Step 1: PREPARE query statement */
	memset(query_sql, 0x0, sizeof(query_sql));
	sprintf(query_sql, "prepare_%s", alias);
	EXEC SQL PREPARE :query_sql FROM :sql_stmt;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s [%s]",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);
		return -1;
	}

	/* Step 2: 使用describe函数完成两个功能：
	   1. 为sqlda分配空间， 
	   2. 获取语句信息，并存放在ifx_sqlda_t结构中。*/
	EXEC SQL DESCRIBE :query_sql INTO sqlda;
	if(0 != sqlca.sqlcode)
	{
		db_ifx_free_sqlda(sqlda);

		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s [%s]",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);
		return -1;
	}

	do
	{
		/* Step 3: Initialize result set */
		ret = db_ifx_init_result(conn, sqlda);
		if(ret < 0)
		{
			break;
		}

		/* Step 4: 为PREPARE的SELECT语句声明和打开游标 */
		memset(query_cursor, 0x0, sizeof(query_cursor));
		sprintf(query_cursor, "cursor_%s", alias);
		EXEC SQL DECLARE :query_cursor CURSOR WITH HOLD FOR :query_sql;
		if(0 != sqlca.sqlcode)
		{
			rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
			uLog(SW_LOG_ERROR,
					"[%s][%d] sqlcode:%d errmsg:%s [%s]",
					__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);
			break;
		}

		EXEC SQL OPEN :query_cursor;
		if(0 != sqlca.sqlcode)
		{
			rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
			uLog(SW_LOG_ERROR,
					"[%s][%d] sqlcode:%d errmsg:%s",
					__FILE__, __LINE__, sqlca.sqlcode, errmsg);
			db_ifx_cclose(alias, (void*)cntx);
			break;
		}

		colnum = sqlda->sqld;
		db_ifx_free_sqlda(sqlda);
		sqlda = NULL;

		return colnum;
	}while(0);

	db_ifx_free_sqlda(sqlda);
	sqlda = NULL;
	db_ifx_free_result(conn);

	return -1;
}

/******************************************************************************
 **函数名称: db_ifx_fetch
 **功    能: 取出一个动态查询sql游标的一条记录
 **输入参数:
 **     _cntx: Context of connection
 **输出参数:
 **返    回: Number of row.
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.15 #
 ******************************************************************************/
int db_ifx_fetch(void *_cntx)
{
	int idx = 0;
	ifx_sqlda_t *current = NULL;
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = &cntx->conn[0];

	if(conn->is_nodata)
	{
		return 0;
	}

	conn->rows = 0;
	current = (ifx_sqlda_t *)conn->result;

	for(idx=1; idx<=conn->req_rows; idx++, current++, conn->rows++)
	{
		/* 1: 执行fetch操作，将一行数据存放在sqlda结构中 */
		EXEC SQL FETCH QUERY_CURSOR USING DESCRIPTOR current;
		if(SQLNOTFOUND == sqlca.sqlcode)
		{
			conn->is_nodata = 1;

			return conn->rows;
		}
		else if(0 != sqlca.sqlcode)
		{
			rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
			uLog(SW_LOG_ERROR,
					"[%s][%d] sqlcode:%d errmsg:%s",
					__FILE__, __LINE__, sqlca.sqlcode, errmsg);
			return -1;
		}
	}

	return conn->rows;
}

int db_ifx_mfetch(const char *alias, void *_cntx)
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	query_cursor[128];
	EXEC SQL END DECLARE SECTION; 
	int idx = 0;
	ifx_sqlda_t *current = NULL;
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;

	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get conn [%s] info error!", __FILE__, __LINE__, alias);
		return -1;
	}

	if(conn->is_nodata)
	{
		return 0;
	}

	conn->rows = 0;
	current = (ifx_sqlda_t *)conn->result;

	memset(query_cursor, 0x0, sizeof(query_cursor));
	sprintf(query_cursor, "cursor_%s", alias);
	for(idx=1; idx<=conn->req_rows; idx++, current++, conn->rows++)
	{
		/* 1: 执行fetch操作，将一行数据存放在sqlda结构中 */
		EXEC SQL FETCH :query_cursor USING DESCRIPTOR current;
		if(SQLNOTFOUND == sqlca.sqlcode)
		{
			conn->is_nodata = 1;

			return conn->rows;
		}
		else if(0 != sqlca.sqlcode)
		{
			rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
			uLog(SW_LOG_ERROR,
					"[%s][%d] sqlcode:%d errmsg:%s",
					__FILE__, __LINE__, sqlca.sqlcode, errmsg);
			return -1;
		}
	}

	return conn->rows;
}

/******************************************************************************
 **函数名称: db_ifx_cclose
 **功    能: Release cursor
 **输入参数:
 **     _cntx: Context of connection
 **输出参数:
 **返    回: 0: success  !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.15 #
 ******************************************************************************/
int db_ifx_cclose(const char *alias, void *_cntx) 
{ 
	EXEC SQL BEGIN DECLARE SECTION;
	char	query_cursor[128];
	EXEC SQL END DECLARE SECTION; 
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get conn [%s] info error!", __FILE__, __LINE__, alias);
		return -1;
	}

	/* 释放结果集 */
	conn->is_nodata = 1;

	db_ifx_free_result(conn);
	conn->used = 0;
	memset(conn->alias, 0x0, sizeof(conn->alias));
	/* 关闭游标 */
	memset(query_cursor, 0x0, sizeof(query_cursor));
	sprintf(query_cursor, "cursor_%s", alias);
	EXEC SQL CLOSE :query_cursor;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}

	EXEC SQL FREE :query_cursor;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_squery
 **功    能: 单行查询
 **输入参数:
 **      sql: Query SQL statement
 **      _cntx: Context of connection
 **输出参数:
 **返    回: Number of column.
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.27 #
 ******************************************************************************/
int db_ifx_squery(const char *sql, void *_cntx)
{
	EXEC SQL BEGIN DECLARE SECTION; 
	char sql_stmt[SQL_STMT_MAX_LEN] = {0};
	EXEC SQL END DECLARE SECTION; 

	int ret = -1,
	    colnum = 0;
	ifx_sqlda_t *sqlda = NULL;
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = &cntx->conn[0];
	conn->ap = &cntx->ap;

	/* Avoid memory leak */
	if(1 == conn->isalloced)
	{
		db_ifx_free_result(conn);
	}

	snprintf(sql_stmt, sizeof(sql_stmt), "%s", sql);

	conn->req_rows = 1;
	conn->is_nodata = 0;

	/* Step 1. PREPARE查询语句 */
	EXEC SQL PREPARE SINGLE_QUERY_SQL FROM :sql_stmt;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s [%s]",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);

		return -1;
	}

	/* Step 2. 使用describe函数完成两个功能：
	   1. 为sqlda分配空间， 
	   2. 获取语句信息，并存放在ifx_sqlda_t结构中。*/
	EXEC SQL DESCRIBE SINGLE_QUERY_SQL INTO sqlda;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s [%s]",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg, sql);

		db_ifx_free_sqlda(sqlda);
		sqlda = NULL;
		return -1;
	}

	/* Step 3: Initialize result set */
	ret = db_ifx_init_result(conn, sqlda);
	if(ret < 0)
	{
		db_ifx_free_sqlda(sqlda);
		sqlda = NULL;
		db_ifx_free_result(conn);
		return -1;
	}

	/* Step 4. 执行单行查找操作 */
	ret = db_ifx_exec_squery(conn);
	if(ret < 0)
	{
		db_ifx_free_sqlda(sqlda);
		sqlda = NULL;
		db_ifx_free_result(conn);
		return -1;
	}
	else if(SQLNOTFOUND == ret)
	{
		db_ifx_free_sqlda(sqlda);
		sqlda = NULL;
		db_ifx_free_result(conn);
		return 0;
	}

	colnum = sqlda->sqld;
	db_ifx_free_sqlda(sqlda);
	sqlda = NULL;

	return colnum;
}

/******************************************************************************
 **函数名称: db_ifx_exec_squery
 **功    能: 执行单行查询
 **输入参数:
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.11.01 #
 ******************************************************************************/
static int db_ifx_exec_squery(ifx_dbconn_t *conn)
{
	int ret = 0;
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};

	/* Step1. 声明游标 */
	EXEC SQL DECLARE SINGLE_QUERY_CURSOR CURSOR WITH HOLD FOR SINGLE_QUERY_SQL;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}

	/* Step2. 打开游标 */
	EXEC SQL OPEN SINGLE_QUERY_CURSOR;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);
		db_ifx_scclose(conn);
		return -1;
	}

	/* Step3. 获取数据 */
	ret = db_ifx_scfetch(conn);
	if(0==ret || SQLNOTFOUND==ret)
	{
		db_ifx_scclose(conn);
		return ret;
	}

	/* Step4. 关闭和释放游标 */
	db_ifx_scclose(conn);

	return -1;
}

/******************************************************************************
 **函数名称: db_ifx_scfetch
 **功    能: 取数据(单行查询)
 **输入参数:
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.27 #
 ******************************************************************************/
int db_ifx_scfetch(ifx_dbconn_t *conn)
{
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};
	ifx_sqlda_t *sqlda = conn->result;

	EXEC SQL FETCH SINGLE_QUERY_CURSOR USING DESCRIPTOR sqlda;
	if(SQLNOTFOUND == sqlca.sqlcode)
	{
		conn->rows = 0;
		return sqlca.sqlcode;
	}
	else if(0 != sqlca.sqlcode)
	{
		conn->rows = 0;

		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}

	conn->rows = 1;

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_scclose
 **功    能: Close cursor(Single line query)
 **输入参数:
 **      cntx: Context of connection
 **输出参数:
 **返    回: 0: success !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.27 #
 ******************************************************************************/
static int db_ifx_scclose(ifx_dbconn_t *conn)
{
	char errmsg[DB_ERR_MSG_MAX_LEN] = {0};


	EXEC SQL CLOSE SINGLE_QUERY_CURSOR;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);    
		return -1;
	}

	EXEC SQL FREE SINGLE_QUERY_SQL;
	if(0 != sqlca.sqlcode)
	{
		rgetmsg(sqlca.sqlcode, errmsg, sizeof(errmsg));
		uLog(SW_LOG_ERROR,
				"[%s][%d] sqlcode:%d errmsg:%s",
				__FILE__, __LINE__, sqlca.sqlcode, errmsg);
		return -1;
	}

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_get_data_by_idx
 **功    能: Get value by index.
 **输入参数:
 **      rowno: Row No.(range: 1~X)
 **      colno: Column No.(range: 1~Y)
 **      cntx: Context of connection
 **输出参数:
 **返    回: Address of value.
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
char *db_ifx_get_data_by_idx(const char *alias, int rowno, int colno, void *_cntx)
{
	ifx_sqlda_t *current = NULL;
	struct sqlvar_struct *sqlvar = NULL;
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get conn [%s] info error!", __FILE__, __LINE__, alias);
		return NULL;
	}

	if(NULL == conn->result
			|| NULL == conn->result->sqlvar
			|| colno <= 0
			|| colno > conn->result->sqld
			|| rowno <= 0
			|| rowno > conn->rows)
	{
		uLog(SW_LOG_ERROR,
				"[%s][%d] Coordinate is out of range! rowno:%d colno:%d",
				__FILE__, __LINE__, rowno, colno);
		return NULL;
	}

	/* 1. 定位行 */
	current = conn->result+rowno-1;

	/* 2. 定位列 */
	sqlvar = current->sqlvar+colno-1;

	/* 3. 返回结果 */
	return db_ifx_get_value(sqlvar, conn);
}

/******************************************************************************
 **函数名称: db_ifx_get_data_by_name
 **功    能: Get value by name.
 **输入参数:
 **      rowno: Row No.(range: 1~X)
 **      name: Name of column
 **      cntx: Context of connection
 **输出参数:
 **返    回: Address of value.
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
char *db_ifx_get_data_by_name(const char *alias, int rowno, const char *name, void *_cntx)
{
	int idx = 0;
	ifx_sqlda_t *current = NULL;
	struct sqlvar_struct *sqlvar = NULL;
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get conn [%s] info error!", __FILE__, __LINE__, alias);
		return NULL;
	}

	if(NULL == conn->result
			|| NULL == conn->result->sqlvar
			|| rowno <= 0
			|| rowno > conn->rows)
	{
		uLog(SW_LOG_ERROR,
				"[%s][%d] rowno is incorrect! rowno:%d name:%s",
				__FILE__, __LINE__, rowno, name);

		return NULL;
	}

	/* 1. 定位行 */
	current = conn->result+rowno-1;

	/* 2. 定位列 */
	sqlvar = current->sqlvar;
	for(idx=0; idx<current->sqld; idx++)
	{
		if(0 == strcasecmp(name, sqlvar->sqlname))
		{
			break;
		}
		sqlvar++;
	}

	if(idx == current->sqld)
	{
		uLog(SW_LOG_ERROR,
				"[%s][%d] Didn't find column [%s]!", __FILE__, __LINE__, name);

		return NULL;
	}

	/* 3. 返回结果 */
	return db_ifx_get_value(sqlvar, conn);
}

/******************************************************************************
 **函数名称: db_ifx_get_data_and_name
 **功    能: Get value and name by index.
 **输入参数:
 **     rowno: Row No.(range: 1~X)
 **     colno: Column No.(range: 1~Y)
 **     size: Size of name buffer.
 **     cntx: Context of connection
 **输出参数:
 **     name: Name of column.
 **返    回: Address of value.
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
char *db_ifx_get_data_and_name(const char *alias, int rowno, int colno, char *name, int size, void *_cntx)
{
	ifx_sqlda_t *current = NULL;
	struct sqlvar_struct *sqlvar = NULL;
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get conn [%s] info error!", __FILE__, __LINE__, alias);
		return NULL;
	}

	if(NULL == conn->result
			|| NULL == conn->result->sqlvar
			|| colno <= 0
			|| colno > conn->result->sqld
			|| rowno <= 0
			|| rowno > conn->rows)
	{
		uLog(SW_LOG_ERROR,
				"[%s][%d] Coordinate is out of range! rowno:%d colno:%d",
				__FILE__, __LINE__, rowno, colno);
		return NULL;
	}

	/* 1. 定位行 */
	current = conn->result+rowno-1;

	/* 2. 定位列 */
	sqlvar = current->sqlvar+colno-1;

	/* 3. 返回结果 */
	snprintf(name, size, "%s", sqlvar->sqlname);

	return db_ifx_get_value(sqlvar, conn);
}

/******************************************************************************
 **函数名称: db_ifx_get_data_type
 **功    能: 获取指定列的数据类型
 **输入参数:
 **      row: 行号(range: 1~X)
 **      col: 列号(range: 1~Y)
 **      cntx: 数据库连接的上下文
 **输出参数:
 **返    回: 0: success  !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
int db_ifx_get_data_type(const char *alias, int col, void *_cntx)
{
	int idx = 0;
	ifx_sqlda_t *current = NULL;
	struct sqlvar_struct *sqlvar = NULL;
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	ifx_dbconn_t	*conn = NULL;
	
	conn = ifx_get_alias_conn(alias, cntx);
	if (conn == NULL)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] Get conn [%s] info error!", __FILE__, __LINE__, alias);
		return -1;
	}

	if(NULL == conn->result
			|| NULL == conn->result->sqlvar
			|| col <= 0
			|| col > conn->result->sqld)
	{
		uLog(SW_LOG_ERROR,
				"[%s][%d] Paramter is incorrect! col:%d result:%p",
				__FILE__, __LINE__, col, conn->result);
		return DB_DTYPE_UNKNOW;
	}

	current = conn->result;
	sqlvar = current->sqlvar+col-1;

	return db_ifx_get_type(sqlvar);
}

/******************************************************************************
 **函数名称: db_ifx_init_result
 **功    能: 初始化结果集
 **输入参数:
 **     cntx: Context of connection
 **     sqlda: 存放了查找结果的基本信息
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **      参数sqlda已通过"EXEC SQL DESCRIBE QUERY_SQL INTO sqlda"分配了空间和数据
 **作    者: # Qifeng.zou # 2012.11.08 #
 ******************************************************************************/
static int db_ifx_init_result(ifx_dbconn_t *conn, ifx_sqlda_t *sqlda)
{
	int	idx = 0;
	int	ret = -1, row_idx = 0;
	size_t sqlvar_alloc_size = 0, row_size = 0;
	ifx_sqlda_t *current = NULL;
	struct sqlvar_struct *sqlvar = NULL;
	struct sqlvar_struct *sqlold, *sqlnew = NULL;
	alias_pool_t *ap = conn->ap;

	memset(conn->result, 0, sizeof(conn->result));

	conn->isalloced = 1;
	db_ifx_set_fetarrsize(conn, sqlda);

	sqlvar_alloc_size = sqlda->sqld*sizeof(struct sqlvar_struct);

	/* 1. 初始化结果集行 */
	current = conn->result;
	for(row_idx=0; row_idx<conn->req_rows; row_idx++, current++)
	{
		memcpy(current, sqlda, sizeof(ifx_sqlda_t));

		sqlvar = (struct sqlvar_struct *)alias_alloc(ap, sqlvar_alloc_size);
		if(NULL == sqlvar)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!", 
					__FILE__, __LINE__, errno, strerror(errno)); 
			return -1;
		}
		memcpy(sqlvar, sqlda->sqlvar, sqlvar_alloc_size);

		sqlnew = sqlvar;
		sqlold = sqlda->sqlvar;
		for (idx = 0; idx < sqlda->sqld; idx++, sqlnew++, sqlold++)
		{
			size_t  len = 0;
			len = strlen(sqlold->sqlname);
			sqlnew->sqlname = (char *)alias_alloc(ap, len + 1);
			if (sqlnew->sqlname == NULL)
			{
				uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
						__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			memset(sqlnew->sqlname, 0x0, len + 1);
			memcpy(sqlnew->sqlname, sqlold->sqlname, len);
		}
		current->sqlvar = sqlvar;
		current->desc_next = NULL;

		/* 2. 初始化列--初试化sqlda结构，如：为列分配空间，改变数据类型等。*/
		row_size = db_ifx_init_sqlda(conn, current);
		if(row_size < 0)
		{
			return -1;
		}
	}

	/* 3. 申请结果转换的空间 */
	conn->convert = (char *)alias_alloc(ap, conn->convert_size);
	if(NULL == conn->convert)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
				__FILE__, __LINE__, errno, strerror(errno)); 
		return -1;
	}

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_free_result
 **功    能: 释放结果查找空间
 **输入参数:
 **      cntx: 上下文信息
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.11.08 #
 ******************************************************************************/
static void db_ifx_free_result(ifx_dbconn_t *conn)
{
	int idx = 0;
	int i = 0, j = 0,
	    dealloc_num = 0;
	loc_t *loc = NULL;
	ifx_sqlda_t *current = NULL;
	struct sqlvar_struct *sqlvar = NULL;
	alias_pool_t *ap = conn->ap;

	if (conn->isalloced == 0)
	{
		return ;
	}

	/* 1. 释放与结果集相关变量的空间 */
	if(NULL != conn->convert)
	{
		alias_free(ap, conn->convert);
		conn->convert = NULL;
	}
	conn->convert_size = 0;

	/* 2. 释放结果集数据空间 */
	current = conn->result;
	for(idx=0; idx<DB_FETCH_MAX_ROWS; idx++, current++)
	{
		sqlvar = current->sqlvar;
		if (sqlvar == NULL)
		{
			continue;
		}

		for(i=0; i<current->sqld; i++, sqlvar++)
		{
			if(CLOCATORTYPE == sqlvar->sqltype)
			{
#if defined(__DB_IFX_FETARRSIZE_ZERO__)
				dealloc_num = 1;
#else  /*__DB_IFX_FETARRSIZE_ZERO__*/
				dealloc_num = (0 == FetArrSize)? 1: FetArrSize;
#endif /*__DB_IFX_FETARRSIZE_ZERO__*/

				loc = (loc_t*)sqlvar->sqldata;
				for(j=0; j<dealloc_num; j++)
				{
					alias_free(ap, loc->loc_buffer);
					loc->loc_buffer = NULL;

					loc++;
				}
			}

			if (sqlvar->sqlname != NULL)
			{
				alias_free(ap, sqlvar->sqlname);
				sqlvar->sqlname = NULL;
			}

			if(NULL != sqlvar->sqlind)
			{
				alias_free(ap, sqlvar->sqlind);
				sqlvar->sqlind = NULL;
			}

			if (sqlvar->sqldata != NULL)
			{
				alias_free(ap, sqlvar->sqldata);
				sqlvar->sqldata = NULL;
			}
		}
		if (current->sqlvar)
		{
			alias_free(ap, current->sqlvar);
		}
	}
	conn->isalloced = 0;
}

/******************************************************************************
 **函数名称: db_ifx_init_sqlda
 **功    能: 初始化ifx_sqlda_t类型变量
 **输入参数:
 **     cntx: Context
 **     sqlda: 被初始化对象
 **输出参数:
 **返    回: >=0: 一行记录的长度  <0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.15 #
 ******************************************************************************/
static int db_ifx_init_sqlda(ifx_dbconn_t *conn, ifx_sqlda_t *sqlda)
{
	int ret = 0,
	    idx = 0,
	    msg_len = 0,
	    row_size = 0,
	    alloc_num = 0;
	struct sqlvar_struct *sqlvar = NULL;


	/* 1. 获取一行数据的长度 */
	sqlvar = sqlda->sqlvar;
	for(idx=0; idx<sqlda->sqld; idx++, sqlvar++)
	{
		/* 为col->sqllen 重新赋值，该值是在C下的大小。
		   如：在数据库中的字符串，在C中应该多一个字节空间来存放NULL的结束符 */
		sqlvar->sqllen = rtypmsize(sqlvar->sqltype, sqlvar->sqllen);
	}

#if defined(__DB_IFX_FETARRSIZE_ZERO__)
	alloc_num = 1;
#else /*__DB_IFX_FETARRSIZE_ZERO__*/
	alloc_num = (0 == FetArrSize)? 1: FetArrSize;
#endif /*__DB_IFX_FETARRSIZE_ZERO__*/


	/* 3. 设置sqlvar_struct结构中的数据类型为相应的C的数据类型 */
	sqlvar = sqlda->sqlvar;
	for(idx=0; idx<sqlda->sqld; idx++, sqlvar++)
	{
		ret = db_ifx_set_sqltype(sqlvar);
		if(ret < 0)
		{
			return ret;
		}

		ret = db_ifx_init_sqldata(conn, sqlvar, alloc_num);
		if(ret < 0)
		{
			return ret;
		}
	}

	return msg_len;
}

/******************************************************************************
 **函数名称: db_ifx_set_sqltype
 **功    能: 设置数据类型
 **输入参数:
 **      sqlvar: 存放列信息
 **      alloc_num: 分配空间的块数
 **输出参数:
 **返    回: >=0: 一行记录的长度  <0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.19 #
 ******************************************************************************/
static int db_ifx_set_sqltype(struct sqlvar_struct *sqlvar)
{
	int ret = 0;

	switch(sqlvar->sqltype)
	{
		case SQLBOOL:
			{
				sqlvar->sqltype = CBOOLTYPE;
				break;
			}
		case SQLSMINT:
			{
				sqlvar->sqltype = CSHORTTYPE;
				break;
			}
		case SQLINT:
			{
				sqlvar->sqltype = CINTTYPE;
				break;
			}
		case SQLINT8:
		case SQLSERIAL:
		case SQLSERIAL8:
			{
				sqlvar->sqltype = CINT8TYPE;
				break;
			}
		case SQLBIGSERIAL:
		case SQLINFXBIGINT:
			{
				sqlvar->sqltype = CBIGINTTYPE;
				break;
			}
		case SQLDECIMAL:
			{
				sqlvar->sqltype = CDECIMALTYPE;
				break;
			}
		case SQLSMFLOAT:
			{
				sqlvar->sqltype = CFLOATTYPE;
				break;
			}
		case SQLFLOAT:
			{
				sqlvar->sqltype = CDOUBLETYPE;
				break;
			}
		case SQLCHAR:
			{
				sqlvar->sqltype = CCHARTYPE;
				break;
			}
		case SQLNCHAR:
			{
				sqlvar->sqltype = CFIXCHARTYPE;
				break;
			}
		case SQLVCHAR:
		case SQLNVCHAR:
			{
				sqlvar->sqltype = CVCHARTYPE;
				break;
			}
		case SQLLVARCHAR:
			{
				sqlvar->sqltype = CLVCHARTYPE;
				break;
			}
		case SQLMONEY:
			{
				sqlvar->sqltype = CMONEYTYPE;
				break;
			}
		case SQLINTERVAL:
			{
				sqlvar->sqltype = CINVTYPE;
				break;
			}
		case SQLDATE:
			{
				sqlvar->sqltype = CDATETYPE;
				break;
			}
		case SQLDTIME:
			{
				sqlvar->sqltype = CDTIMETYPE;
				break;
			}
		case SQLROW:
			{
				sqlvar->sqltype = CROWTYPE;
				break;
			}
		case SQLSET:
		case SQLLIST:
		case SQLMULTISET:
		case SQLCOLLECTION:
			{
				sqlvar->sqltype = CCOLLTYPE;
				break;
			}
		case SQLTEXT:
		case SQLBYTES:
			{
				sqlvar->sqltype = CLOCATORTYPE;
				break;
			}
		default: /* Other data type */
			{
				uLog(SW_LOG_ERROR, "[%s][%d] 出现不支持的数据类型! type:%d!", __FILE__, __LINE__, sqlvar->sqltype);
				return -1;
			}
	}

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_init_sqldata
 **功    能: 通过数据类型，初始化sqlvar->sqldata
 **输入参数:
 **      sqlvar: Store column description information
 **      alloc_num: 分配空间的块数
 **输出参数:
 **返    回: >=0: 一行记录的长度  <0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.10.31 #
 ******************************************************************************/
static int db_ifx_init_sqldata(ifx_dbconn_t *conn, struct sqlvar_struct *sqlvar, int alloc_num)
{
	int ret = 0, alloc_size = 0;
	alias_pool_t *ap = conn->ap;

	/* 1. 为指示符变量申请空间 */
	sqlvar->sqlind = (short *)alias_alloc(ap, alloc_num * sizeof(short));
	if(NULL == sqlvar->sqlind)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
				__FILE__, __LINE__, errno, strerror(errno)); 
		return -1;
	}

	/* 2. 为存放非TEXT 和BLOB的数据类型的sqldata申请空间.
	   注意: 申请的地址是(char *)，在输出数据时，要按照相应的数据类型做转换 */
	if(CLOCATORTYPE != sqlvar->sqltype)
	{
		alloc_size = alloc_num*sqlvar->sqllen;
		if(alloc_size > conn->convert_size)
		{
			conn->convert_size = alloc_size;
		}

		sqlvar->sqldata = (char*)alias_alloc(ap, alloc_size);
		if(NULL == sqlvar->sqldata)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
					__FILE__, __LINE__, errno, strerror(errno)); 
			return -1;
		}
		return 0;
	}

	/* 3. 为TEXT和BLOB的数据类型的sqldata申请空间 */
	return db_ifx_alloc_loc(conn, sqlvar, alloc_num);
}

/******************************************************************************
 **函数名称: db_ifx_alloc_loc
 **功    能: 为CLOCATORTYPE类型的分配空间
 **输入参数:
 **      sqlvar: 存放列信息
 **      alloc_num: 分配空间的块数
 **输出参数:
 **返    回: >=0: 一行记录的长度  <0: failed
 **实现描述: 
 **注意事项: 
 **      1. 为TEXT和BLOB的数据类型的sqldata申请空间
 **      2. 只有数据类型为TEXT和BLOB时，才执行
 **作    者: # Qifeng.zou # 2012.10.31 #
 ******************************************************************************/
static int db_ifx_alloc_loc(ifx_dbconn_t *conn, struct sqlvar_struct *sqlvar, int alloc_num)
{
	int idx = 0, alloc_size = 0;
	loc_t *loc = NULL;
	alias_pool_t *ap = conn->ap;

	alloc_size = alloc_num*sqlvar->sqllen;
	if(alloc_size > conn->convert_size)
	{
		conn->convert_size = alloc_size;
	}

	/* 1. 为存放TEXT或BYTE列数据申请空间 */
	loc = (loc_t*)alias_alloc(ap, alloc_size);
	if(NULL == loc)
	{
		uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
				__FILE__, __LINE__, errno, strerror(errno)); 
		return -1;                
	}
	sqlvar->sqldata = (char *)loc;

	/* 2. 初试化loc_t结构 */
	byfill(loc, alloc_size, 0);

	for(idx=0; idx<alloc_num; idx++)
	{
		loc->loc_loctype = LOCMEMORY;
		loc->loc_bufsize = IFX_BLOB_SIZE;

		loc->loc_buffer = (char *)alias_alloc(ap, IFX_BLOB_SIZE);
		if(NULL == loc->loc_buffer)
		{
			uLog(SW_LOG_ERROR, "[%s][%d] errmsg:[%d] %s!",
					__FILE__, __LINE__, errno, strerror(errno)); 
			return -1;
		}

		loc->loc_oflags = 0;
		loc++;
	}

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_get_value
 **功    能: Get column value from sqlvar.
 **输入参数:
 **      sqlvar: Description information of specail column.
 **输出参数:
 **返    回: 0: success  !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
static char *db_ifx_get_value(struct sqlvar_struct *sqlvar,  ifx_dbconn_t *conn)
{
	int ret = 0;
	loc_t *loc = NULL;
	char *data = sqlvar->sqldata,
	     *convert = conn->convert,
	     *pconvert = NULL;

	memset(convert, 0, conn->convert_size);

	switch (sqlvar->sqltype)
	{
		case CBOOLTYPE:
			snprintf(convert, conn->convert_size, "%d", *((unsigned char*)data));
			return convert;

		case CSHORTTYPE:
			snprintf(convert, conn->convert_size, "%d", *((short*)data));
			return convert;

		case CINTTYPE:
			snprintf(convert, conn->convert_size, "%d", *((int*)data));
			return convert;

		case CLONGTYPE:
			snprintf(convert, conn->convert_size, "%ld", *((long*)data));
			return convert;

		case CINT8TYPE:
			snprintf(convert, conn->convert_size, "%lld", *((long long*)data));
			return convert;

		case CBIGINTTYPE:
			snprintf(convert, conn->convert_size, "%lld", *((long long*)data));
			return convert;

		case CDECIMALTYPE:
			pconvert = convert;
			dectoasc((dec_t*)data, convert, conn->convert_size, -1);
			/* Note: dectoasc() Left align and fill blank, so must delete blank */
			while('\0' != *pconvert)
			{
				if(isblank(*pconvert))
				{
					*pconvert = '\0';
					break;
				}
				pconvert++;
			}
			return convert;

		case CFLOATTYPE:
			snprintf(convert, conn->convert_size, "%f", (double)(*(float*)data));
			return convert;

		case CDOUBLETYPE:
			snprintf(convert, conn->convert_size, "%f", *((double*)data));
			return convert;

		case CMONEYTYPE:
			snprintf(convert, conn->convert_size, "%d", *(int*)data);
			return convert;

		case CINVTYPE:
			intoasc((intrvl_t*)data, convert);
			return convert;

		case CDATETYPE:
			rfmtdate(*(int*)data, "YYYYMMDD", convert);
			return convert;

		case CDTIMETYPE:
			dttoasc((dtime_t*)data, convert);
			return convert;

		case CROWTYPE:
		case CCOLLTYPE:
			return data;

		case CCHARTYPE:
		case CFIXCHARTYPE:
		case CVCHARTYPE:
		case CLVCHARTYPE:
			return data;

		case CLOCATORTYPE:
			loc = (loc_t *)sqlvar->sqldata;
			return loc->loc_buffer;

		default:
			return NULL;
	} 

	return NULL;
}

/******************************************************************************
 **函数名称: db_ifx_get_type
 **功    能: Get data type from ifx_sqlda_t.
 **输入参数:
 **      result: ifx_sqlda_t存放了查找结果
 **      col: 列号
 **输出参数:
 **返    回: 0: success  !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.22 #
 ******************************************************************************/
static int db_ifx_get_type(struct sqlvar_struct *sqlvar)
{
	switch (sqlvar->sqltype)
	{
		case CBOOLTYPE:
			return DB_DTYPE_BOOL;

		case CSHORTTYPE:
			return DB_DTYPE_SMINT;

		case CINTTYPE:
			return DB_DTYPE_INT;

		case CLONGTYPE:
			return DB_DTYPE_LONG;

		case CINT8TYPE:
			return DB_DTYPE_LNGLNG;

		case CBIGINTTYPE:
			return DB_DTYPE_BIGINT;

		case CDECIMALTYPE:
			return DB_DTYPE_DECIMAL;

		case CFLOATTYPE:
			return DB_DTYPE_SMFLOAT;

		case CDOUBLETYPE:
			return DB_DTYPE_FLOAT;

		case CCHARTYPE:
			return DB_DTYPE_CHAR;

		case CFIXCHARTYPE:
			return DB_DTYPE_NCHAR;

		case CVCHARTYPE:
			return DB_DTYPE_VCHAR;

		case CLVCHARTYPE:
			return DB_DTYPE_LVCHAR;

		case CMONEYTYPE:
			return DB_DTYPE_MONEY;

		case CINVTYPE:
			return DB_DTYPE_INTERVAL;

		case CDATETYPE:
			return DB_DTYPE_DATE;

		case CDTIMETYPE:
			return DB_DTYPE_DTIME;

		case CROWTYPE:
			return DB_DTYPE_ROW;

		case CCOLLTYPE:
			return DB_DTYPE_COLL;

		case CLOCATORTYPE:
			return DB_DTYPE_LOCATOR;

		default:
			return DB_DTYPE_UNKNOW;
	} 

	return DB_DTYPE_UNKNOW;
}

/******************************************************************************
 **函数名称: db_ifx_free_sqlda
 **功    能: Release memory of ifx_sqlda_t.
 **输入参数:
 **      sqlda: Release object.
 **输出参数:
 **返    回: 0: success  !0: failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.08.15 #
 ******************************************************************************/
static int db_ifx_free_sqlda(ifx_sqlda_t *sqlda)
{
	int i = 0, j = 0,
	    dealloc_num = 0;
	loc_t *loc = NULL;
	struct sqlvar_struct *sqlvar = NULL;

	sqlvar = sqlda->sqlvar;

	for(i=0; i<sqlda->sqld; i++, sqlvar++)
	{
		if(CLOCATORTYPE == sqlvar->sqltype)
		{
			dealloc_num = (0 == FetArrSize)? 1: FetArrSize;

			loc = (loc_t*)sqlvar->sqldata;
			for(j=0; j<dealloc_num; j++)
			{
				free(loc->loc_buffer);
				loc->loc_buffer = NULL;

				loc++;
			}
		}

		if(NULL != sqlvar->sqlind)
		{
			free(sqlvar->sqlind);
			sqlvar->sqlind = NULL;
		}

		free(sqlvar->sqldata);
		sqlvar->sqldata = NULL;
	}

	free(sqlda);

	return 0;
}

/******************************************************************************
 **函数名称: db_ifx_set_fetarrsize
 **功    能: 设置FetArrSize的值
 **输入参数:
 **      cntx: Context
 **      sqlda: 存放行列的基本信息
 **输出参数:
 **返    回: NONE
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2012.11.06 #
 ******************************************************************************/
static void db_ifx_set_fetarrsize(ifx_dbconn_t *conn, ifx_sqlda_t *sqlda)
{
	int idx = 0,
	    length = 0;
	struct sqlvar_struct *sqlvar = NULL;

	/* 1. 获取一行数据的长度 */
	sqlvar = sqlda->sqlvar;
	for(idx=0; idx<sqlda->sqld; idx++, sqlvar++)
	{
		/* msglen变量存放查询数据的所有列的长度和 */
		conn->msglen += sqlvar->sqllen;

		/* 为col->sqllen 重新赋值，该值是在C下的大小。
		   如：在数据库中的字符串，在C中应该多一个字节空间来存放NULL的结束符 */
		length = rtypmsize(sqlvar->sqltype, sqlvar->sqllen);

		/* row_size变量存放了在C程序中的所有列的长度和。
		   这个值是应用程序为存放一行数据所需要申请的内存空间 */
		conn->row_size += length;
	}

	/* 2. 设置FetArrSize值 */
#if defined(__DB_IFX_FETARRSIZE_ZERO__)
	FetArrSize = 0;
	FetBufSize = conn->msglen;
#else  /*__DB_IFX_FETARRSIZE_ZERO__*/
	if(-1 == FetArrSize)
	{
		if(0 == FetBufSize)
		{
			FetBufSize = IFX_FETCH_BUFF_MAX_SIZE;
		}
		FetArrSize = FetBufSize/conn->msglen;
	}
#endif /*__DB_IFX_FETARRSIZE_ZERO__*/
}

/******************************************************************************
 **函数名称: db_ifx_set_alias
 **功    能: Set alias.
 **输入参数:
 **     _cntx: Context of connection.
 **     name: alias
 **     value: Value of alias.
 **     length: Length of value.
 **输出参数:
 **返    回: 0:success  !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
int db_ifx_set_alias(void *_cntx, const char *name, const void *value, int length)
{
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_set(ap, name, value, length);
}

/******************************************************************************
 **函数名称: db_ifx_get_alias
 **功    能: Get value by name.
 **输入参数:
 **     _cntx: Context of connection.
 **     name: alias
 **输出参数:
 **返    回: Address of value.
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
const void *db_ifx_get_alias(void *_cntx, const char *name)
{
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_get(ap, name);
}

/******************************************************************************
 **函数名称: db_ifx_delete_alias
 **功    能: Delete special alias.
 **输入参数:
 **     _cntx: Context of connection.
 **     name: Alais
 **输出参数:
 **返    回: 0:success  !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
int db_ifx_delete_alias(void *_cntx, const char *name)
{
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_delete(ap, name);
}

/******************************************************************************
 **函数名称: db_ifx_delete_all_alias
 **功    能: Delete all alias.
 **输入参数:
 **     _cntx: Context of connection.
 **输出参数:
 **返    回: 0:success  !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.13 #
 ******************************************************************************/
int db_ifx_delete_all_alias(void *_cntx)
{
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;
	alias_pool_t *ap = &cntx->ap;

	return alias_delete_all(ap);
}

int db_ifx_del_all_conn(void *_cntx)
{
	ifx_cntx_t *cntx = (ifx_cntx_t *)_cntx;

	db_ifx_free_all_result(cntx);

	return 0;
}

int db_ifx_conn_detect(void *_cntx)
{
	return db_ifx_squery(IFX_DB_CONN_DETECT, _cntx);
}

static void db_ifx_free_all_result(ifx_cntx_t *cntx)
{
	int	i = 0;
	
	if (cntx == NULL)
	{
		return ;
	}
	
	for (i = 0; i < IFX_MAX_CONN; i++)
	{
		if (cntx->conn[i].used == 1)
		{
			uLog(SW_LOG_INFO, "[%s][%d] Free alias=[%s]", __FILE__, __LINE__, cntx->conn[i].alias);
			db_ifx_free_result(&cntx->conn[i]);
			cntx->conn[i].used = 0;
		}
	}

	return ;
}

