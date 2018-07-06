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
/* �������� */
typedef enum
{
	RET_SUCCESS = 0,					/* ���سɹ� */
	RET_CONTINUE,					/* ���ؼ��� */
	RET_FAILED = ~0xffff,			/* ����ʧ�� */
	RET_ERR_DLOPEN_FAILED,			/* dlopen()����ʧ�� */
	RET_ERR_DLSYM_FAILED,			/* dlsym()����ʧ�� */
	RET_ERR_XML_MISSED_MARK,		/* XMLȱʧ��ǩ������ */
	RET_ERR_MALLOC_FAILED,			/* malloc()ʧ�� */
	RET_ERR_ADD_FCACHE_FAILED,		/* �����ļ�����ʧ�� */
	RET_ERR_UNXML_ALYZ_FAILED,		/* UNXML���ʧ�� */
	RET_ERR_UNXML_INGT_FAILED,		/* UNXML���ʧ�� */
	RET_ERR_XML_ALYZ_FAILED,		/* XML���ʧ�� */
	RET_ERR_XML_INGT_FAILED,		/* XML���ʧ�� */
	RET_ERR_VAR_NAME_INVAILD,		/* ���������Ϸ� */
	RET_ERR_GET_MAP_FNAME_FAILED,	/* ��ȡ����ӳ���ļ���ʧ�� */
	RET_ERR_END_OF_STR,				/* �ѵ��ַ���β */
	RET_ERR_OPT_EXP_IS_NULL,		/* ���ʽ����Ϊ�� */
	RET_ERR_NOT_PKG_ROUTE_FILE      /* ���Ǳ��Ĳ����·���ļ� */
}ret_err_enum_t;


/*��������*/
#if 0
#define ERR_BASE   	 1 		/*��һ������*/
#define ERR_TOPKGOLD   	 3 		/*�������(����֧��ϵͳ,һ������)*/
#define ERR_TOPKG    	 4		/*�������*/
#define ERR_TOUTF8   	 5		/*to UTF8ת�����*/
#define ERR_CONNECT    	 6		/*���Ӵ���*/
#define ERR_PREDO    	 7		/*Ԥ�������*/
#define ERR_PUTMQ    	 8 		/*дMQ���д���*/
#define ERR_LSNOUT    	 9		/*CALLLSN��ʱ*/
#define ERR_DOFLOW	 10		/*FLOW�ļ�ִ��ʧ��*/
#define ERR_SEND	 11		/*������Ϣʧ��*/
#define ERR_RECV	 12		/*������Ϣʧ��*/
#define ERR_PKGTOS	 13		/*���ʧ��*/
#define ERR_CONOUT	 14		/*���ӳ�ʱ*/
#define ERR_SNDFILE	 15		/*�����ļ�ʧ��*/
#define ERR_RCVFILE	 16		/*�����ļ�ʧ��*/
#define ERR_SWRCV	 17		/*����������ʧ��*/
#define ERR_SWSND	 18		/*д��������ʧ��*/
#define ERR_NOORI	 19		/*δ�ҵ�ԭʼ��������*/
#define ERR_RCVPREA	 20		/*����Ԥ����ʧ��*/
#define ERR_SNDPREA	 21		/*����Ԥ����ʧ��*/
#define ERR_SYNRES	 22		/*ͨѶ��Ӧ��ʧ��*/
#define ERR_RCVPRED	 23		/*����Ԥ����ʧ��*/
#define ERR_SNDPRED	 24		/*����Ԥ����ʧ��*/
#define ERR_DELM	 25		/*ɾ��mtypeʧ��*/
#define ERR_SCANDEAL1	 26		/*ɨ����ñ��Ĵ������ʧ�ܣ������*/
#define ERR_SCANDEAL2	 27		/*ɨ����ñ��Ĵ������ʧ�ܣ������*/
#define ERR_NOTOSW	 28		/*δ�ҵ��������*/
#define ERR_NOTOPKG	 29		/*δ�ҵ��������*/
#define ERR_ISFILERCV	 30		/*�ж��Ƿ����ļ�����ʧ��*/
#define ERR_ISFILESND	 31		/*�ж��Ƿ����ļ�����ʧ��*/
#define ERR_SVRSVC	 32		/*SVR/SVCӳ��ʧ��*/
#define ERR_RCVMAP	 33		/*Ҫ��ת��ʧ�ܣ�������*/
#define ERR_SNDMAP	 34		/*Ҫ��ת��ʧ�ܣ�������*/
#define ERR_FINDE	 35		/*���ҿ�������ʧ��*/
#define ERR_ADDTASK	 36		/*���������ʧ��*/
#define ERR_GETTASK	 37		/*��ȡָ����������ʧ��*/
#define ERR_DELTASK	 38		/*ɾ�����������ʧ��*/
#define ERR_NOSVR	 39		/*Ŀ��svr������*/
#define ERR_ERRSVR	 40		/*Ŀ��svr�쳣����*/
#define ERR_NOSVC	 41		/*Ŀ��svc������*/
#define ERR_ERRSVC	 42		/*Ŀ��svc�쳣����*/
#define ERR_GETMINFO	 43		/*��ȡmtype��ʱ������Ϣʧ��*/
#define ERR_LOADSET	 44		/*�ָ�Ҫ����Ϣʧ��*/
#define ERR_SAVESET	 45		/*����Ҫ����Ϣʧ��*/
#define ERR_TOLOGMAN	 46		/*����־������̷���ָ��ʧ��*/
#define ERR_DOPREFLOW	 47		/*FLOW�ļ�Ԥ����ʧ��*/
#define ERR_ADDSAFCTL	 48		/*����ת�����Ʊ�ʧ��*/
#define ERR_UPDSAFCTL	 49		/*SAF���Ʊ����ʧ��*/
#define ERR_FINDESVC	 50		/*��ȡ����SVC��������ʧ��*/
#define ERR_ADDSVC	 51		/*���SVC��������ʧ��*/
#define ERR_GETSVC	 52		/*��ȡָ��SVC��������ʧ��*/
#define ERR_DELSVC	 53		/*ɾ��SVC���������ʧ��*/
#define ERR_ERRMAP	 54		/*���ĵ����쳣*/
#define ERR_DBCONN	 55		/*���ݿ�����ʧ��*/
#define ERR_DOERRFLOW    56		/*ִ�д��󲹳�FLOW�ļ�ʧ��*/
#define ERR_DOCONFLOW    57		/*SVC��������������ִ��ʧ��*/
#define ERR_NOLSN    	 58		/*Ŀ������������*/
#define ERR_DOERR    	 59		/*DOԭ�ӽ���ʧ��*/
#define ERR_TPINIT	 60		/*Ӧ�ó���ע�����*/
#define ERR_TPBEGIN	 61		/*��������IDʧ��*/
#define ERR_TPCALL	 62		/*TPCALLʧ��*/
#define ERR_TPCOMMIT	 63		/*�ύ����ʧ��*/
#define ERR_UNSERL	 64		/*�����л�ʧ��*/
#define ERR_SERL	 65		/*���л�ʧ��*/
#define ERR_UNKNOWN	 66		/*���л�ʧ��*/
#define ERR_CICSCALL     67     	/* CICSCALLʧ�� */
#define ERR_ERRLSN	 68		/*Ŀ�������쳣*/
#endif

#endif

