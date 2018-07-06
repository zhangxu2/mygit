#ifndef __EXPRESS_H__
#define __EXPRESS_H__
#define CTYPE_OPERATOR 0	/****运算符类型****/
#define CTYPE_STRING   1	/****字符串数据****/
#define CTYPE_NUMBER   2	/****数字数据****/
#define MAX_OPERATOR_CNT	64	/****最大运算符个数****/
#define MAX_DATA_CNT		128	/****最大数据个数****/
typedef struct _t_ctree_node{
	char type;	/***0运算符,1数据****/
	char *data;	/***内容****/
}CTNODE;
/*****************工作流解析相关******************/
#define DECLARE_MAX_ITEM  128		/***DECLARE最多语句			****/
#define MAX_EXPRESS_LENGTH 512		/***表达式最大长度			****/
#define FLOW_MAX_ITEM  1000		/***BEGIN-END最多语句			****/
#define SENT_TYPE_DO   0		/***语句类型				****/
#define SENT_TYPE_SET  1
#define SENT_TYPE_CHECK 2
#define SENT_TYPE_GOTO  3
#define SENT_TYPE_LABEL 4
#define SENT_TYPE_CONTINUE 	55
#define SENT_TYPE_TUXC 		56

#define SENT_TYPE_TCPA  6		/*added by liteng,20100801,for mqim		*/
#define SENT_TYPE_CALLLSN 5		/*added by liteng,20101217,for call lsn		*/
#define SENT_TYPE_LINKLSN 18		/*added by liteng,20110119,for link lsn		*/

#define SENT_TYPE_IMPORT 7		/*edit by songshixin 2010.8.24,for fbbm*/
#define SENT_TYPE_CALL   8		/***edit by songshixin 2010.8.24,for fbbm	*/
#define SENT_TYPE_LINK   9		/***edit by songshixin 2010.8.24,for fbbm	*/

#define SENT_TYPE_POSTLSN	31	/*added by liteng,20110507,for post lsn		*/
#define SENT_TYPE_POSTSVC	32	/*added by liteng,20110507,for post svc		*/


/******added by Jiayanbin,20100925.transform*******/
#define	SENT_TYPE_SWITCH 10
#define	SENT_TYPE_MOVE	11
#define SENT_TYPE_COPY	12
#define	SENT_TYPE_REMOVE 13
#define	SENT_TYPE_CASE	14
#define	SENT_TYPE_BREAK	15
#define	SENT_TYPE_DEFAULT 16
#define	SENT_TYPE_CMPBEG 17
#define	SENT_TYPE_CMPEND 19
#define	SENT_TYPE_CMPDO 20
#define SENT_TYPE_EXIT 21
#define SENT_TYPE_COND 22
#define SENT_COND_CHECK 23
#define SENT_TYPE_CNDBEG 24
#define SENT_TYPE_CNDEND 25
/**************************************************/

/*add by duke 20110515*/
typedef struct 
{
	short step[3];			/*保存当前各个级别的信息*/
	short curstep;			/*当前所在的补偿步骤*/
	short curmaxstep;		/*当前最大的步骤编号*/
	short curlevel;			/*当前所在步骤的级别*/
}STEPINFO;
/**end**/

typedef struct{
	short type;
	short line;			/**语句在配置文件中的绝对行号****/
	short step;			/*当前属于的层数*add by duke 20110515*/
	char  out[33];			/**SET语句的赋值变量,或者GOTO的目标****/
	char  express[MAX_EXPRESS_LENGTH];/***SET语句=后面的表达式或DO语句的命令,
						或者CHECK语句后面的表达式,或者GOTO语句的IF条件表达式,或者LABEL语句的标号**/
}TSENT;

typedef struct{
	struct {
		short count;
		TSENT sent[DECLARE_MAX_ITEM];
	}decl;
	struct {
		short count;
		TSENT sent[FLOW_MAX_ITEM];
	}work;
}TFLOW;

/***edit by songshixin 2010.8.24****/
typedef struct _ntflow{
	TFLOW flow;
	long long upd_time;		/*modefied by liteng,20111027,for cache	*/
	char name[256];
	struct timeval time;
}NTFLOW;

/***edit by songshixin 2010.8.24****/
typedef struct namenode
{
	char name[128];
	struct namenode *next;
}TFILENODE;
#endif
