#ifndef __EXPRESS_H__
#define __EXPRESS_H__
#define CTYPE_OPERATOR 0	/****���������****/
#define CTYPE_STRING   1	/****�ַ�������****/
#define CTYPE_NUMBER   2	/****��������****/
#define MAX_OPERATOR_CNT	64	/****������������****/
#define MAX_DATA_CNT		128	/****������ݸ���****/
typedef struct _t_ctree_node{
	char type;	/***0�����,1����****/
	char *data;	/***����****/
}CTNODE;
/*****************�������������******************/
#define DECLARE_MAX_ITEM  128		/***DECLARE������			****/
#define MAX_EXPRESS_LENGTH 512		/***���ʽ��󳤶�			****/
#define FLOW_MAX_ITEM  1000		/***BEGIN-END������			****/
#define SENT_TYPE_DO   0		/***�������				****/
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
	short step[3];			/*���浱ǰ�����������Ϣ*/
	short curstep;			/*��ǰ���ڵĲ�������*/
	short curmaxstep;		/*��ǰ���Ĳ�����*/
	short curlevel;			/*��ǰ���ڲ���ļ���*/
}STEPINFO;
/**end**/

typedef struct{
	short type;
	short line;			/**����������ļ��еľ����к�****/
	short step;			/*��ǰ���ڵĲ���*add by duke 20110515*/
	char  out[33];			/**SET���ĸ�ֵ����,����GOTO��Ŀ��****/
	char  express[MAX_EXPRESS_LENGTH];/***SET���=����ı��ʽ��DO��������,
						����CHECK������ı��ʽ,����GOTO����IF�������ʽ,����LABEL���ı��**/
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
