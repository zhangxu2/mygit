#ifndef __PUB_ERRNO_H__
#define __PUB_ERRNO_H__

#include "pub_type.h"
#include "pub_math.h"
#include "pub_string.h"

/*auto */
#ifndef SW_SYS_NERR
#define SW_SYS_NERR 100
#endif

#ifndef SW_HAVE_GCC_VARIADIC_MACROS
#define SW_HAVE_GCC_VARIADIC_MACROS 0
#endif


#define SW_EPERM        EPERM
#define SW_ENOENT       ENOENT
#define SW_ENOPATH      ENOENT
#define SW_ESRCH        ESRCH
#define SW_EINTR        EINTR
#define SW_ECHILD       ECHILD
#define SW_ENOMEM       ENOMEM
#define SW_EACCES	EACCES
#define SW_EBUSY	EBUSY
#define SW_EEXIST	EEXIST
#define SW_EXDEV	EXDEV
#define SW_ENOTDIR	ENOTDIR
#define SW_EISDIR	EISDIR
#define SW_EINVAL	EINVAL
#define SW_ENFILE	ENFILE
#define SW_EMFILE	EMFILE
#define SW_ENOSPC	ENOSPC
#define SW_EPIPE	EPIPE
#define SW_EINPROGRESS  EINPROGRESS
#define SW_EADDRINUSE   EADDRINUSE
#define SW_ECONNABORTED ECONNABORTED
#define SW_ECONNRESET   ECONNRESET
#define SW_ENOTCONN     ENOTCONN
#define SW_ETIMEDOUT    ETIMEDOUT
#define SW_ECONNREFUSED ECONNREFUSED
#define SW_ENAMETOOLONG ENAMETOOLONG
#define SW_ENETDOWN     ENETDOWN
#define SW_ENETUNREACH  ENETUNREACH
#define SW_EHOSTDOWN    EHOSTDOWN
#define SW_EHOSTUNREACH EHOSTUNREACH
#define SW_ENOSYS       ENOSYS
#define SW_ECANCELED    ECANCELED
#define SW_EILSEQ       EILSEQ
#define SW_ENOMOREFILES 0

#define sw_errno                  errno
/* 错误类型 */
typedef enum
{
	RET_SUCCESS = 0,					/* 返回成功 */
	RET_CONTINUE,					/* 返回继续 */
	RET_FAILED = ~0xffff,			/* 返回失败 */
	RET_ERR_DLOPEN_FAILED,			/* dlopen()调用失败 */
	RET_ERR_DLSYM_FAILED,			/* dlsym()调用失败 */
	RET_ERR_XML_MISSED_MARK,		/* XML缺失标签或属性 */
	RET_ERR_MALLOC_FAILED,			/* malloc()失败 */
	RET_ERR_ADD_FCACHE_FAILED,		/* 加载文件缓存失败 */
	RET_ERR_UNXML_ALYZ_FAILED,		/* UNXML拆包失败 */
	RET_ERR_UNXML_INGT_FAILED,		/* UNXML组包失败 */
	RET_ERR_XML_ALYZ_FAILED,		/* XML拆包失败 */
	RET_ERR_XML_INGT_FAILED,		/* XML组包失败 */
	RET_ERR_VAR_NAME_INVAILD,		/* 变量名不合法 */
	RET_ERR_GET_MAP_FNAME_FAILED,	/* 获取变量映射文件名失败 */
	RET_ERR_END_OF_STR,				/* 已到字符结尾 */
	RET_ERR_OPT_EXP_IS_NULL,		/* 表达式内容为空 */
	RET_ERR_NOT_PKG_ROUTE_FILE      /* 不是报文拆组包路由文件 */
}ret_err_enum_t;


/*错误类型*/
#if 0
#define ERR_BASE   	 1 		/*第一错误码*/
#define ERR_TOPKGOLD   	 3 		/*组包错误(二代支付系统,一代报文)*/
#define ERR_TOPKG    	 4		/*组包错误*/
#define ERR_TOUTF8   	 5		/*to UTF8转码错误*/
#define ERR_CONNECT    	 6		/*连接错误*/
#define ERR_PREDO    	 7		/*预处理错误*/
#define ERR_PUTMQ    	 8 		/*写MQ队列错误*/
#define ERR_LSNOUT    	 9		/*CALLLSN超时*/
#define ERR_DOFLOW	 10		/*FLOW文件执行失败*/
#define ERR_SEND	 11		/*发送消息失败*/
#define ERR_RECV	 12		/*接收消息失败*/
#define ERR_PKGTOS	 13		/*拆包失败*/
#define ERR_CONOUT	 14		/*连接超时*/
#define ERR_SNDFILE	 15		/*发送文件失败*/
#define ERR_RCVFILE	 16		/*接受文件失败*/
#define ERR_SWRCV	 17		/*读队列数据失败*/
#define ERR_SWSND	 18		/*写队列数据失败*/
#define ERR_NOORI	 19		/*未找到原始请求连接*/
#define ERR_RCVPREA	 20		/*来报预分析失败*/
#define ERR_SNDPREA	 21		/*往报预分析失败*/
#define ERR_SYNRES	 22		/*通讯级应答失败*/
#define ERR_RCVPRED	 23		/*来报预处理失败*/
#define ERR_SNDPRED	 24		/*往报预处理失败*/
#define ERR_DELM	 25		/*删除mtype失败*/
#define ERR_SCANDEAL1	 26		/*扫描可用报文处理进程失败（拆包）*/
#define ERR_SCANDEAL2	 27		/*扫描可用报文处理进程失败（组包）*/
#define ERR_NOTOSW	 28		/*未找到拆包配置*/
#define ERR_NOTOPKG	 29		/*未找到组包配置*/
#define ERR_ISFILERCV	 30		/*判断是否有文件接收失败*/
#define ERR_ISFILESND	 31		/*判断是否有文件发送失败*/
#define ERR_SVRSVC	 32		/*SVR/SVC映射失败*/
#define ERR_RCVMAP	 33		/*要素转换失败（来报）*/
#define ERR_SNDMAP	 34		/*要素转换失败（往报）*/
#define ERR_FINDE	 35		/*查找空闲任务失败*/
#define ERR_ADDTASK	 36		/*插入任务表失败*/
#define ERR_GETTASK	 37		/*获取指定任务索引失败*/
#define ERR_DELTASK	 38		/*删除任务表索引失败*/
#define ERR_NOSVR	 39		/*目的svr不存在*/
#define ERR_ERRSVR	 40		/*目的svr异常死掉*/
#define ERR_NOSVC	 41		/*目的svc不存在*/
#define ERR_ERRSVC	 42		/*目的svc异常死掉*/
#define ERR_GETMINFO	 43		/*获取mtype即时任务信息失败*/
#define ERR_LOADSET	 44		/*恢复要素信息失败*/
#define ERR_SAVESET	 45		/*保存要素信息失败*/
#define ERR_TOLOGMAN	 46		/*向日志服务进程发送指令失败*/
#define ERR_DOPREFLOW	 47		/*FLOW文件预分析失败*/
#define ERR_ADDSAFCTL	 48		/*插入转发控制表失败*/
#define ERR_UPDSAFCTL	 49		/*SAF控制表更新失败*/
#define ERR_FINDESVC	 50		/*获取空闲SVC任务索引失败*/
#define ERR_ADDSVC	 51		/*添加SVC任务索引失败*/
#define ERR_GETSVC	 52		/*获取指定SVC任务索引失败*/
#define ERR_DELSVC	 53		/*删除SVC任务表索引失败*/
#define ERR_ERRMAP	 54		/*报文调度异常*/
#define ERR_DBCONN	 55		/*数据库连接失败*/
#define ERR_DOERRFLOW    56		/*执行错误补偿FLOW文件失败*/
#define ERR_DOCONFLOW    57		/*SVC级条件补偿流程执行失败*/
#define ERR_NOLSN    	 58		/*目的侦听不存在*/
#define ERR_DOERR    	 59		/*DO原子交易失败*/
#define ERR_TPINIT	 60		/*应用程序注册错误*/
#define ERR_TPBEGIN	 61		/*申请事务ID失败*/
#define ERR_TPCALL	 62		/*TPCALL失败*/
#define ERR_TPCOMMIT	 63		/*提交事务失败*/
#define ERR_UNSERL	 64		/*反序列化失败*/
#define ERR_SERL	 65		/*序列化失败*/
#define ERR_UNKNOWN	 66		/*序列化失败*/
#define ERR_CICSCALL     67     	/* CICSCALL失败 */
#define ERR_ERRLSN	 68		/*目的侦听异常*/
#endif

#endif

