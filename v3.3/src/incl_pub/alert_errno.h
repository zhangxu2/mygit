#if !defined(__ALERT_ERRNO_H__)
#define __ALERT_ERRNO_H__

/*********************************************************************************************

                                警告: 以下配置请勿随意修改!!!

 **********************************************************************************************/

#define ERRNO_MAX_LEN       (5)         /* 错误码的最大长度 */
#define ERRNO_BEGIN         (1)         /* 错误码开始值 */
#define ERRNO_END           (99999)     /* 错误码结束值 */
#define ALERT_LIB_PATH      "txlib"
#define ALERT_CFG_PATH      "cfg/alert"		/* 预警配置路径 */
#define ALERT_PERRNO_CFG_PATH "cfg/alert"	/* 平台错误码配置路径 */
#define ALERT_OERRNO_CFG_PATH   "cfg/alert"	/* 交易错误码配置路径 */


/* 错误码种类 */
typedef enum
{
	ERRNO_TYPE_PLAT,
	ERRNO_TYPE_OPR,
	ERRNO_TYPE_TOTAL,
	ERRNO_TYPE_UNKNOW = ERRNO_TYPE_TOTAL,
}ERRNO_TYPE_ENUM;


/* 设置各模块错误码 最大个数 */
#define ERRNO_PLAT_MAX_NUM   (30000)     /* 平台错误码 最大个数 */
#define ERRNO_OPR_MAX_NUM   (20000)     /* 交易错误码 最大个数 */
#define ERRNO_RSV_MAX_NUM   (59999)     /* 预留错误码 最大个数 */

/* 计算各模块错误码 范围 */
#define ERRNO_PLAT_BEGIN     (ERRNO_BEGIN)                         /* 平台错误码 起始值  20001 */


#define ERRNO_PLAT_END       (ERRNO_PLAT_BEGIN + ERRNO_PLAT_MAX_NUM - 1)   /* 平台错误码 起始值  30000 */

#define ERRNO_OPR_BEGIN     (ERRNO_PLAT_END + 1)                         /* 交易错误码 起始值 30001 */
#define ERRNO_OPR_END       (ERRNO_OPR_BEGIN + ERRNO_OPR_MAX_NUM - 1)   /* 交易错误码 起始值 40000 */

#define ERRNO_RSV_BEGIN     (ERRNO_OPR_END + 1)                         /* 预留错误码 起始值 40001 */
#define ERRNO_RSV_END       (ERRNO_RSV_BEGIN + ERRNO_RSV_MAX_NUM - 1)   /* 预留错误码 起始值 99999 */


/*********************************************************************************************

                                定义各模块错误码
                                       
                  注意: 在此配置的值必须与错误码配置文件保持一致，否则将会出现异常

 **********************************************************************************************/
/* 系统监控错误码 定义 */
/* 起始值:ERRNO_SYSMNT_BEGIN(00001) */
#define ERR_MEM		(00001)
#define ERR_CPU		(00002)


/* ===请在此行之上按顺序依次递增=== */
/* 注意: 最大值不能超过ERRNO_SYSMNT_END(10000) */
/*****************************************************************************/
/* 平台监控错误码 定义 */
/* 起始值:ERRNO_PLATMNT_BEGIN(10001) */
#define ERR_TX_CNT	(10001)
#define ERR_MTYPE_USED	(10002)
#define ERR_PROC_MEM	(10003)
#define ERR_CORE	(10004)



/* ===请在此行之上按顺序依次递增=== */
/* 注意: 最大值不能超过ERRNO_PLATMNT_END(20000) */
/*****************************************************************************/
/* 平台错误码 定义 */
/* 起始值:ERRNO_PLAT_BEGIN(20001) */
#define ERR_CALLOC_FAILED	(20001)     	/* calloc失败 */
#define ERR_MALLOC_FAILED	(20002)    	/* malloc失败 */
#define ERR_REALLOC_FAILED	(20003)		/*realloc失败 */
#define ERR_MTYPE_FAILED	(20004)		/*mtype失败*/
#define ERR_TRCNO_FAILED	(20005)		/*traceno失败*/
#define ERR_DATE_FAILED		(20006)		/*date失败*/
#define ERR_PROC_FAILED		(20007)		/*proc失败*/
#define ERR_OFTEN_RESTART_FAILED (20008)	/*重启间隔时间太短*/
#define ERR_CNT_RESTART_FAILED	(20009)		/*重启达到最大次数*/
#define ERR_DBCONN_FAILED	(20010)		/*连接数据库失败*/
#define ERR_TCPSS_FAILED    (20011)     /* TCPSS失败 */
#define ERR_TCPSC_FAILED    (20011)     /* TCPSC失败 */
#define ERR_TCPLC_FAILED    (20013)     /* TCPLC失败 */
#define ERR_CONMAX_FAILED	
#define ERR_RECV_FAILED
#define ERR_ENC_FAILED
#define ERR_PKGIN_FAILED
#define ERR_RECVFILE_FAILED
#define ERR_ROUTE_FAILED
#define ERR_REGTRACE_FAILED
#define ERR_UNSERISE_FAILED
#define ERR_FINDFD_FAILED
#define	ERR_BYMTYPE_INDEX_FAILED
#define ERR_PKGOUT_FAILED
#define ERR_DEC_FAILED
#define ERR_SEND_FAILED
#define ERR_CONN_FAILED
#define ERR_RECOVLINK_FAILED
#define ERR_DELLINK_FAILED
#define ERR_SAVELINK_FAILED


/* ===请在此行之上按顺序依次递增=== */
/* 注意: 最大值不能超过ERRNO_PLAT_END(30000) */
/*****************************************************************************/
/* 交易错误码 */
/* 起始值:ERRNO_OPR_BEGIN(30001) */
#define ERR_DB_INSERT       (30001)     /* 插入数据失败 */
#define ERR_DB_UPDATE       (30002)     /* 更新数据失败 */
#define ERR_DB_SELECT       (30003)     /* 查询数据失败 */
#define ERR_DB_DELETE       (30004)     /* 删除数据失败 */
#define ERR_DEFAULT_SVC_OPR	(30005)
#define ERR_COUNT_SVC_OPR 	(30006)
#define ERR_FLOW_FAILED		(30007)


/* ===请在此行之上按顺序依次递增=== */
/* 注意: 最大值不能超过ERRNO_OPR_END(40000) */
/****************************************************************************/

#endif /*__ALERT_ERRNO_H__*/
