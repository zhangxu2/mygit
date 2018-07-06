#ifndef __LSN_PRDT_H__
#define __LSN_PRDT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "pub_xml.h"
#include "pub_type.h"
#include "pub_vars.h"
#include "msg_trans.h"

#define	ROUTE_NUM	5
#define MAX_CHECK_CNT	5
#define MAX_NAME_LEN	64

typedef struct
{
	sw_char_t 	name[MAX_NAME_LEN];		/*������ǩ*/
	sw_char_t	codeconvert[MAX_NAME_LEN];	/* ת���� */
	sw_char_t	typeanalyze[MAX_NAME_LEN];	/* �������ͷ������ */
	sw_char_t	preanalyze[MAX_NAME_LEN];	/* ����Ԥ���������� */
	sw_char_t	synres[MAX_NAME_LEN];		/* ͬ��Ӧ������ */
	sw_char_t	recover[MAX_NAME_LEN];		/* Ҫ�ػָ������� */
	sw_char_t	afterdeal[MAX_NAME_LEN];	/* ���ĺ����� */
	sw_int32_t	safctl;                 /*�洢ת������*/
}sw_lsn_prdt_cfg_t;

typedef struct
{
	sw_int32_t	begin;
	sw_int32_t	length;
	sw_char_t	name[64];
	sw_char_t	value[128];
	sw_char_t	lib[64];
	sw_char_t	func[64];
}sw_check_t;

typedef struct
{
	sw_int32_t	check_cnt;
	sw_check_t	check[MAX_CHECK_CNT];
}sw_route_check_t;

typedef struct
{
	int	use;		/*��ǰ�ڵ���Ϣ�Ƿ����	*/
	int	type;
	int	timeout;	/*�Ե�ǰ�ڵ�ĳ�ʱ����	*/
	int	sendtype;
	int	notsend;
	int	pkglen;	/*���Ļ���������*/
	long	mtype;		/*ƽ̨������Ψһ��ʶ	*/
	char	asktype[2];	/*����Ӧ���־��0����1Ӧ��*/
	char	keyinfo[128];	/*ҵ�񼶱���Ψһ��ʶ	*/
	char	infoset[1024];	/*Ҫ�����ָ���Ҫ�ؼ���*/
	char	source[2];	/*������Դ��־,1��ʾ�����ɹ��淢��,2��ʾ��������������*/
	char	pkgtype[2];	/*һ�������ı�־,0��ʾһ������,1��ʾ��������*/
	char	synflag[2];	/*�Ƿ�ΪͨѶ��Ӧ��,0����,1��		*/
	char	infoflag[2];	/*�Ƿ�Ҫ�ָ�Ҫ�ؼ���,1�ָ�,2����	*/
	char	saveflag[2];    /*�Ƿ񱣴���·��Ϣ */
	char	clearflag[2];	/*�Ƿ���յ�ǰ��·��Ϣ,0�����,1���	*/
	char	resflag[2];	/* ���˱������⴦���־ 1:����Ҳ���ԭ��·,��ñʽ��װ�������,����Ӧ���� */
	char	*pkgbuf;	/*���Ļ�����*/
	char	qname[64];	/*mq��������*/
	char	msgid[32];
	char	corrid[32];
	sw_int64_t	traceno;	/*ҵ����ˮ*/ 
	sw_char_t	sys_date[32];
	sw_char_t	machine[64];
	sw_char_t	addr[32];
	sw_char_t	lsn_name[64];
}sw_link_t;

typedef struct sw_link_node_s
{
	int	use;
	long	starttime;
	sw_cmd_t	cmd;
	sw_link_t	info;
}sw_link_node_t;

typedef struct
{
	int	cur_pos;
	int	link_cnt;
}sw_link_head_t;

typedef struct
{
	sw_link_head_t	head;
	sw_link_node_t	list[1];
}sw_link_list_t;
sw_link_list_t	*g_linklist;

typedef  int (*sw_file_fun_pt)(sw_loc_vars_t *, sw_int32_t, sw_fd_t, void *);
typedef  int (*sw_deny_fun_pt)(sw_loc_vars_t *, sw_buf_t *, sw_xmltree_t *);
typedef  int (*sw_svr_fun_pt)(sw_xmltree_t *, sw_loc_vars_t *, sw_char_t *, sw_char_t *, sw_int32_t *);
typedef  int (*sw_code_fun_pt)(sw_char_t *, sw_int_t *, int);
typedef  int (*sw_type_fun_pt)(sw_char_t *, sw_char_t *);
typedef  int (*sw_prean_fun_pt)(sw_loc_vars_t *, sw_link_t *, int);
typedef  int (*sw_synres_fun_pt)(sw_loc_vars_t *, sw_char_t *, sw_int_t *);
typedef  int (*sw_recov_fun_pt)(sw_loc_vars_t *, sw_link_t *);
typedef  int (*sw_after_fun_pt)(sw_loc_vars_t *, sw_char_t *, int);
typedef  int (*sw_pkgmap_fun_pt)(sw_loc_vars_t *, sw_xmltree_t *, int);
typedef  int (*sw_start_fun_pt)(sw_loc_vars_t *, int);

typedef struct
{
	sw_code_fun_pt	code_func;
	sw_type_fun_pt	type_func;
	sw_prean_fun_pt	prean_func;
	sw_recov_fun_pt	recov_func;
	sw_after_fun_pt	after_func;
	sw_synres_fun_pt	synres_func;
}sw_prdt_fun_t;

typedef struct
{
	void	*code_handle;
	void	*type_handle;
	void	*prean_handle;
	void	*synres_handle;
	void	*recov_handle;
	void	*after_handle;
}sw_prdt_handle_t;

typedef struct
{
	sw_xmltree_t	*svrmap;
	sw_xmltree_t	*preana;
}sw_prdt_cache_t;

typedef struct
{
	sw_pkgmap_fun_pt pkgmap_func;
	sw_file_fun_pt	file_func;
	sw_svr_fun_pt	svrmap_func;
	sw_deny_fun_pt	deny_func;
	sw_start_fun_pt	start_func;
}sw_chnl_fun_t;

typedef struct
{
	void	*file_handle;
	void	*pkgmap_handle;
	void	*svrmap_handle;
	void	*deny_handle;
	void	*start_handle;
}sw_chnl_handle_t;

typedef struct
{
	sw_xmltree_t	*pkgdeal;
	sw_xmltree_t	*pkgmap;
	sw_xmltree_t	*svrmap;
	sw_xmltree_t	*deny;
	sw_xmltree_t	*preana;
}sw_chnl_cache_t;

typedef struct
{
	sw_chnl_fun_t	fun;
	sw_chnl_cache_t	cache;
	sw_chnl_handle_t	handle;
}sw_chnl_t;

typedef struct
{
	sw_char_t	name[64];
	sw_int32_t	gate;
	sw_int32_t	status;
	sw_route_check_t	an;
	sw_route_check_t	in;
	sw_lsn_prdt_cfg_t	prdt_cfg;
	sw_prdt_fun_t	fun;
	sw_prdt_cache_t	cache;
	sw_prdt_handle_t	handle;
}sw_route_t;

#endif
