#ifndef __PUB_TYPE_H__
#define __PUB_TYPE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <time.h>
#include <iconv.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/timeb.h>
#include <sys/timeb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define SW_OK          0
#define SW_ERROR      -1
#define SW_ERR        -1
#define SW_AGAIN      -2
#define SW_BUSY       -3
#define SW_DONE       -4
#define SW_DECLINED   -5
#define SW_ABORT      -6
#define SW_EXIST	1

#define LINK_ERR    -1
#define SHM_MISS    -2
#define TCPIP	1
#define TUXEDO	2
#define CICS	3
#define TEASY	4
#define MQ	5
#define TLQ	6
#define SOAP	7
#define HTTP	8

#define SW_ENC	0 /*** ���� ***/
#define SW_DEC	1 /*** ���� ***/

#define SW_SWITCH_ON	1
#define SW_SWITCH_OFF	0

#define SW_FILE_RDONLY          O_RDONLY
#define SW_FILE_WRONLY          O_WRONLY
#define SW_FILE_RDWR            O_RDWR
#define SW_FILE_CREATE_OR_OPEN  O_CREAT
#define SW_FILE_OPEN            0
#define SW_FILE_TRUNCATE        O_CREAT|O_TRUNC
#define SW_FILE_APPEND          O_WRONLY|O_APPEND
#define SW_FILE_NONBLOCK        O_NONBLOCK

#define SW_FILE_DEFAULT_ACCESS  0644
#define SW_FILE_OWNER_ACCESS    0600

#define SW_INVALID_FILE         -1
#define SW_FILE_ERROR           -1

#define SW_INVALID_FD		-1

#define	SW_PUBLIC		extern
#define SW_PROTECTED		static

#define true (1)
#define	false (0)
#define bool char
/*esw type rename*/

typedef u_char		sw_uchar_t;
typedef char		sw_char_t;
typedef int             sw_fd_t;
typedef int16_t		sw_int16_t;
typedef	uint16_t	sw_uint16_t;
typedef int32_t		sw_int32_t;
typedef uint32_t	sw_uint32_t;
typedef long long	sw_int64_t;
typedef unsigned long long	sw_uint64_t;
typedef int		sw_int_t;
typedef unsigned int	sw_uint_t;
typedef int             sw_err_t;
typedef uid_t		sw_uid_t;
typedef gid_t		sw_gid_t;
typedef int		sw_lck_t;
typedef time_t		sw_time_t;
typedef int		sw_flag_t;
typedef long long	sw_tans_t;
typedef struct stat     sw_file_info_t;

#define SW_CONF_UNSET_UINT  (sw_uint_t) -1

#define pub_stderr               STDERR_FILENO

#define LF     (u_char) 10
#define CR     (u_char) 13
#define CRLF   "\x0d\x0a"

#define SW_PTR_SIZE	4

#if (SW_PTR_SIZE == 4)
#define SW_INT_T_LEN   SW_INT32_LEN
#else
#define SW_INT_T_LEN   SW_INT64_LEN
#endif

#define SW_INT32_LEN   sizeof("-2147483648") - 1
#define SW_INT64_LEN   sizeof("-9223372036854775808") - 1

#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define SW_MAX_UINT32_VALUE  (uint32_t) 0xffffffffLL
#else
#define SW_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#endif

#define SW_MAX_INT32_VALUE   (uint32_t) 0x7fffffff

#define SW_ENOENT	ENOENT

#define MSGBUF_MAXLEN 	128*1024
#define MAX_FILE_LENGTH  (8129*1024)
#define MIN_RESTART_TIME	300
#define SW_NAME_LEN		32	/*max length of process name*/

#define ERRCODE_LEN	4

#define SW_TRACE_LEN    12
#define SID_LEN 	32
#define NAME_LEN  	32
#define DATE_LEN	16
#define USER_LEN	32
#define PASSWD_LEN	32
#define PATH_LEN	256
#define FORMAT_LEN	32
#define SQN_LEN		32
#define ACTIVEMSG_LEN 128
#define	MAX_SVR_LEVEL_CNT   (8)
#define FILE_NAME_MAX_LEN   (256)       /* Max length of file/path name */
#define PATH_NAME_MAX_LEN   FILE_NAME_MAX_LEN   /* Max length of path name */
#define FILE_PATH_MAX_LEN   FILE_NAME_MAX_LEN   /* Max length of path name */
#define SYS_CMD_MAX_LEN     (1024)      /* Max length of system command */
#define ERR_MSG_MAX_LEN     (1024)      /* Max length of error message */
#define FUNC_NAME_MAX_LEN   (256)       /* Max length of function name */
#define LIB_NAME_MAX_LEN    (256)       /* Max length of library name */
#define IP_ADDR_MAX_LEN     (64)        /* Max length of IP address */

/*udp command*/
#define SW_TIMEOUT	99		/*timeout*/
#define SW_ERRCMD	999		/*unknown cmd type*/
#define SW_RESCMD	800		/*got cmd ack*/
#define SW_REERES	9900		/*exception ack*/
#define SW_ZERO		0		

/*swadmin*/
#define SW_MSTARTONE	401		/*cmd type for start one listen/service*/
#define SW_MSTOPONE	402		/*cmd type for stop one listen / service*/
#define SW_MSTARTALL	403		/*cmd type for start all listen / service*/
#define SW_MSTOPALL	404		/*cmd type for stop all listen / service*/
#define SW_MSTOPIALL	405		/*cmd type for stop all listen / service immediately*/
#define SW_MSTOPFALL	406		/*cmd type for stop all listen / service forcibly*/
#define SW_MSTOPIONE	407		/*cmd type for stop one listen / service immediately*/
#define SW_MSTOPFONE	408		/*cmd type for stop one listen / service forcibly*/
#define SW_MSTOPSELF	409		/*cmd type for stop myself*/
#define SW_MSTOPISELF	410		/*cmd type for stop myself immediately.*/
#define SW_WRITELOG	411			
#define SW_MSTARTTASK	412 		/*cmd type for run a job manually.*/
#define SW_MSTARTALLTASK 413		/*cmd type for run all job manually.*/
#define SW_MSTOPTASK	414		/*cmd type for stop a job manually.*/
#define SW_MEXITSELF	415             /*cmd type for EXIT a process */
#define SW_MSTOPSVR 	416     /*cmd type for stop server in special product*/
#define SW_MSTARTSVR	417     /*cmd type for start server in special product*/


#define SW_ADD_PRDT	451		/*cmd type for add one product*/
#define SW_UPD_PRDT	452		/*cmd type for upd all product*/
#define SW_DEL_PRDT	453		/*** CMD TYPE FOR DELETE ONE PRDT ***/
#define SW_STOP_PRDT	454		/*cmd type for stop one product*/
#define SW_START_PRDT	455		/*cmd type for start one product*/
#define SW_ADD_SVR	456             /*** CMD TYPE FOR ADD ONE SVR ***/
#define SW_UPD_SVR	457             /*** CMD TYPE FOR UPDATE ONE SVR ***/
#define SW_DEL_SVR	458             /*** CMD TYPE FOR DELETE ONE SVR ***/
#define SW_STOP_SVR	459		/*** CMD TYPE FOR STOP ONE SERVER ***/
#define SW_START_SVR	460		/*** CMD TYPE FOR STOP ONE SERVER ***/
#define SW_ADD_SVC	461             /*** CMD TYPE FOR ADD ONE SVC ***/
#define SW_UPD_SVC	462             /*** CMD TYPE FOR UPDATE ONE SVC ***/
#define SW_DEL_SVC	463             /*** CMD TYPE FOR DELETE ONE SVC ***/
#define SW_STOP_SVC	464 		/*** CMD TYPE FOR STOP ONE SERVICE ***/
#define SW_START_SVC	465 		/*** CMD TYPE FOR STOP ONE SERVICE ***/
#define SW_UPD_LSN	466		/*** CMD TYPE FOR UPDATE ONE LSN ***/
#define SW_ARG_MODIFY	467		/*** MODIFY PRDT ARG***/
#define SW_GET_LSN_ROUTE_IN_PRDT 468  /*get lsn status in product*/
#define SW_GET_LOG 469

/*cmd type*/
#define SW_TASKERR 		911	
#define SW_TASKREQ		605   	
#define SW_TASKRES		606   	
#define SW_CALLREQ		607   	
#define SW_CALLRES		608   	
#define SW_POSTREQ		609   	
#define SW_POSTRES		610   	
#define SW_LINKREQ		611   	
#define SW_CALLLSNREQ		612   	
#define SW_CALLLSNRES		613   	
#define SW_LINKLSNREQ		614   	
#define SW_LINKLSNRES		615   	
#define SW_POSTLSNREQ		616   	
#define SW_POSTLSNRES		617   	
#define SW_LINKNULL		618				
#define SW_CALLLSNSAFREQ        619     /**/
#define SW_CALLLSNSAFRES        620     /**/
#define SW_SAFTRIGER            621     /**/
#define SW_SAFREQ               622     /**/
#define SW_SAFRES               623     /**/
#define SW_CALLRESGRPERR        624     /*callӦ���͵��˴����group*/
#define SW_CALLLSNRESGRPERR     625     /*calllsnӦ���͵��˴����group*/                                                                 
#define SW_PKGRESCLEAR          626     /*������Դ����ָ��      */                                                                         
#define SW_DO                   627     /*��ˮ�����DO��� add by songshixin20120813*/                                                     
#define SW_TIME_OUT             633
#define SW_TASKDENY             640     /* ����ܾ����� */

/*process status*/
#define SW_S_START		100             /*start normally*/
#define SW_S_STOPED		101             /*stop normally*/
#define SW_S_KILL		102             /*stop forcibly*/
#define SW_S_ABNORMAL		999             /*process's status is abnormal*/
#define SW_S_ERR		900             /*not running*/
#define SW_S_NREACH		103		/*Can not reach, socket maybe abnormal.*/
#define SW_S_STOPPING		104
#define SW_S_READY		105
#define SW_S_EXITING		106
#define SW_S_EXITED		107
#define SW_S_ABORTED		108


/*server status*/
#define SW_ND_READY	1
#define SW_ND_WORK	2
#define SW_ND_CUTDOWN	3
#define SW_ND_STOP	4
#define SW_ND_END	5
#define SW_ND_CONNECT	6
#define SW_ND_RELOAD   7

/*DFIS BP status*/
#define SW_ALL_START   1001
#define SW_ALL_STOP	2001
#define SW_DATE_CHG	3001
#define SW_ABNORMAL 	4001

#define SW_FIFO_STATUS  -1 
/*saf status*/
#define ST_INIT          0              /*��ʼ״̬              */
#define ST_INIT2         8              /*��ʼ״̬2             */
#define ST_NOFILE        9              /*Ҫת�����ļ���ʧ      */
#define ST_NOSAF        10              /*����Ҫת���ļ�¼      */
#define ST_SND           1              /*�ѷ���                */
#define ST_SNDE          2              /*���͹���������ʧ��    */
#define ST_END           3              /*�������              */
#define ST_ENDE          4              /*������ɣ�������ʧ��  */
#define ST_REQ           5              /*ĳ������������        */
#define ST_RES           6              /*ĳ����Ӧ������        */
#define ST_DENO          7              /*ĳ���̴������        */
#define ST_RESEND	11		/*�ط�״̬,���calllsn���������	*/

/*process type*/
#define ND_LSN    	101 		/*�����ڵ�*/
#define ND_LSN_SND	102
#define ND_LSN_RCV	103
#define ND_SVC       	500 		/*��������*/
#define ND_TASK		30 		/*�������*/
#define ND_SVR	 	40 		/*svr����*/
#define ND_DO		41		/*DO��� add by songshixin20120813*/
#define ND_SAF       	60 		/*ת���ػ�����*/
#define ND_LOG       	61 		/*������־�ػ�����*/
#define ND_POL       	62 		/*ƽ̨�ػ�����*/
#define ND_RES       	64 		/*��Դ�����ػ�����*/
#define ND_JOB		65		/*������job���ý���*/
#define ND_ADM       	70 		/*������*/
#define ND_ALERT	80		/*Ԥ�������ػ�����,add by songshixin 20120801*/
#define ND_AGTADM	50
#define ND_AGT		51
#define ND_AGTPOL	52
#define ND_UNKNOWN  999     /* δ֪�ڵ� */

/* Process name */
#define PROC_NAME_ADM		"swadmin"
#define PROC_NAME_LSN		"swlsn"
#define PROC_NAME_JOB		"swjob"
#define PROC_NAME_MAP		"swmap"
#define PROC_NAME_SVR		"swserv"
#define PROC_NAME_SVC_MAN	"swsvcman"
#define PROC_NAME_FTP		"swftp"
#define PROC_NAME_CLUST	"swclust"
#define PROC_NAME_POL		"swpol"
#define PROC_NAME_LOG		"swlog"
#define PROC_NAME_ALERT	"swalert"
#define PROC_NAME_SAFD		"swsafd"
#define PROC_NAME_ALOGSVR   "swalogsvr"
#define	PROC_NAME_AGTADM	"swagent"
#define	PROC_NAME_AGT		"agentmain"
#define PROC_NAME_AGTPOL	"agentpol"

#define	ERREVERY	1
#define ERR_BASE         2              /*��һ������*/
#define ERR_PREANA       3              /*Ԥ��������*/
#define ERR_TOPKG        4              /*�������*/
#define ERR_TOUTF8       5              /*to UTF8ת�����*/
#define ERR_CONNECT      6              /*���Ӵ���*/
#define ERR_PREDO        7              /*Ԥ�������*/
#define ERR_PUTMQ        8              /*дMQ���д���*/
#define ERR_LSNOUT       9              /*CALLLSN��ʱ*/
#define ERR_DOFLOW       10             /*FLOW�ļ�ִ��ʧ��*/
#define ERR_SEND         11             /*������Ϣʧ��*/
#define ERR_RECV         12             /*������Ϣʧ��*/
#define ERR_PKGTOS       13             /*���ʧ��*/
#define ERR_CONOUT       14             /*���ӳ�ʱ*/
#define ERR_SNDFILE      15             /*�����ļ�ʧ��*/
#define ERR_RCVFILE      16             /*�����ļ�ʧ��*/
#define ERR_SWRCV        17             /*����������ʧ��*/
#define ERR_SWSND        18             /*д��������ʧ��*/
#define ERR_NOORI        19             /*δ�ҵ�ԭʼ��������*/
#define ERR_RCVPREA      20             /*����Ԥ����ʧ��*/
#define ERR_SNDPREA      21             /*����Ԥ����ʧ��*/
#define ERR_SYNRES       22             /*ͨѶ��Ӧ��ʧ��*/
#define ERR_RCVPRED      23             /*����Ԥ����ʧ��*/
#define ERR_SNDPRED      24             /*����Ԥ����ʧ��*/
#define ERR_DELM         25             /*ɾ��mtypeʧ��*/
#define ERR_SCANDEAL1    26             /*ɨ����ñ��Ĵ������ʧ�ܣ������*/
#define ERR_SCANDEAL2    27             /*ɨ����ñ��Ĵ������ʧ�ܣ������*/
#define ERR_NOTOSW       28             /*δ�ҵ��������*/
#define ERR_NOTOPKG      29             /*δ�ҵ��������*/
#define ERR_ISFILERCV    30             /*�ж��Ƿ����ļ�����ʧ��*/
#define ERR_ISFILESND    31             /*�ж��Ƿ����ļ�����ʧ��*/
#define ERR_SVRSVC       32             /*SVR/SVCӳ��ʧ��*/
#define ERR_RCVMAP       33             /*Ҫ��ת��ʧ�ܣ�������*/
#define ERR_SNDMAP       34             /*Ҫ��ת��ʧ�ܣ�������*/
#define ERR_FINDE        35             /*���ҿ�������ʧ��*/
#define ERR_ADDTASK      36             /*���������ʧ��*/
#define ERR_GETTASK      37             /*��ȡָ����������ʧ��*/
#define ERR_DELTASK      38             /*ɾ�����������ʧ��*/
#define ERR_NOSVR        39             /*Ŀ��svr������*/                                                                                  
#define ERR_ERRSVR       40             /*Ŀ��svr�쳣����*/                                                                                
#define ERR_NOSVC        41             /*Ŀ��svc������*/                                                                                  
#define ERR_ERRSVC       42             /*Ŀ��svc�쳣����*/                                                                                
#define ERR_GETMINFO     43             /*��ȡmtype��ʱ������Ϣʧ��*/                                                                      
#define ERR_LOADSET      44             /*�ָ�Ҫ����Ϣʧ��*/                                                                               
#define ERR_SAVESET      45             /*����Ҫ����Ϣʧ��*/                                                                               
#define ERR_TOLOGMAN     46             /*����־������̷���ָ��ʧ��*/                                                                     
#define ERR_DOPREFLOW    47             /*FLOW�ļ�Ԥ����ʧ��*/                                                                             
#define ERR_ADDSAFCTL    48             /*����ת�����Ʊ�ʧ��*/                                                                             
#define ERR_UPDSAFCTL    49             /*SAF���Ʊ����ʧ��*/                                                                              
#define ERR_FINDESVC     50             /*��ȡ����SVC��������ʧ��*/                                                                        
#define ERR_ADDSVC       51             /*���SVC��������ʧ��*/                                                                            
#define ERR_GETSVC       52             /*��ȡָ��SVC��������ʧ��*/                                                                        
#define ERR_DELSVC       53             /*ɾ��SVC���������ʧ��*/                                                                          
#define ERR_ERRMAP       54             /*���ĵ����쳣*/                                                                                   
#define ERR_DBCONN       55             /*���ݿ�����ʧ��*/                                                                                 
#define ERR_DOERRFLOW    56             /*ִ�д��󲹳�FLOW�ļ�ʧ��*/                                                                       
#define ERR_DOCONFLOW    57             /*SVC��������������ִ��ʧ��*/                                                                      
#define ERR_NOLSN        58             /*Ŀ������������*/                                                                                 
#define ERR_DOERR        59             /*DOԭ�ӽ���ʧ��*/                                                                                 
#define ERR_TPINIT       60             /*Ӧ�ó���ע�����*/                                                                               
#define ERR_TPBEGIN      61             /*��������IDʧ��*/                                                                                 
#define ERR_TPCALL       62             /*TPCALLʧ��*/                                                                                     
#define ERR_TPCOMMIT     63             /*�ύ����ʧ��*/                                                                                   
#define ERR_UNSERL       64             /*�����л�ʧ��*/                                                                                   
#define ERR_SERL         65             /*���л�ʧ��*/                                                                                     
#define ERR_UNKNOWN      66             /*���л�ʧ��*/                                                                                     
#define ERR_CICSCALL     67             /* CICSCALLʧ�� */                                                                                 
#define ERR_ERRLSN       68             /*Ŀ�������쳣*/       

#define MAX_BAK_SIZE  8192*1024

#define SW_MSG_RES              (1001)
#define SW_MSG_REQ              (1002)
#define SW_STORE                (1003)
#define SW_FORGET               (1004)
#define SW_DEL                  (1005)

typedef enum
{
        SVR_LEVEL_DEFAULT = 1,
        SVR_LEVEL_GENERAL,
        SVR_LEVEL_IMPORTANT,
        SVR_LEVEL_URGENT,
        SVR_LEVEL_ALL = SVR_LEVEL_URGENT
} sw_svr_level_t;

typedef enum
{
	SW_TCP_TRANSPORT = 1,
	SW_UNIX_TRANSPORT = 2,
	SW_UDP_TRANSPORT
} sw_transport_t;

#endif

