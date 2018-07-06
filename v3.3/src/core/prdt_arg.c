#include "prdt_arg.h"
#include "pub_vars.h"

#if defined(__PRDT_ARG__)

/* ��Ʒ������������Ϣ */
prdt_arg_cntx_t g_prdt_arg_cntx;

static int prdt_arg_env_check(void);

/******************************************************************************
 **��������: prdt_arg_init
 **��    ��: ��Ʒ������ʼ��
 **�������: 
 **     addr: �ڴ��ַ
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.11 #
 ******************************************************************************/
int prdt_arg_init(void *addr)
{
	int ret = 0;
	char gflock[FILE_PATH_MAX_LEN] = {0},
	     pflock[FILE_PATH_MAX_LEN] = {0};

	/* 1. Check environment */
	ret = prdt_arg_env_check();
	if(0 != ret)
	{
		pub_log_error("[%s][%d] Environment is incorrect!", __FILE__, __LINE__);
		return -1;
	}

	snprintf(gflock, sizeof(gflock),
			"%s/%s/%s", getenv("SWWORK"), PRDT_ARG_FLOCK_DIR, PRDT_GRP_FLOCK);
	snprintf(pflock, sizeof(pflock),
			"%s/%s/%s", getenv("SWWORK"), PRDT_ARG_FLOCK_DIR, PRDT_ARG_FLOCK);

	/* 2. Init group information */
	ret = group_init(prdt_arg_gcntx(), addr,
			PRDT_GRP_NUM, PRDT_GRP_SIZE, gflock, pflock);
	if(0 != ret)
	{
		pub_log_error("[%s][%d] Init group failed!", __FILE__, __LINE__);
		return -1;
	}

	/* 2. Link group memory */
	ret = prdt_arg_link(addr);
	if(0 != ret)
	{
		pub_log_error("[%s][%d] Link paramter of product failed!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

/******************************************************************************
 **��������: prdt_arg_galloc
 **��    ��: �����Ʒ������
 **�������: 
 **     name: ����
 **     gid: ��ID
 **     pgid: ����ID
 **     remark: ��ע��Ϣ
 **�������: NONE
 **��    ��: ������IDX
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.10 #
 ******************************************************************************/
int prdt_arg_galloc(const char *name,
		unsigned int gid, unsigned int pgid, const char *remark)
{
	int gidx = -1;
	group_sum_t *sum = NULL;
	group_cntx_t *cntx = prdt_arg_gcntx();

	sum = (group_sum_t *)cntx->addr;
	if(cntx->gfd <= 0)
	{
		cntx->gfd = open(sum->gflock, O_CREAT|O_WRONLY, 0777);
		if(cntx->gfd < 0)
		{
			pub_log_error("[%s][%d] can not open [%s], errmsg:[%d][%s]",
					__FILE__, __LINE__, sum->gflock, errno, strerror(errno));
			return -1;
		}
	}

	if(cntx->pfd <= 0)
	{
		cntx->pfd = open(sum->pflock, O_CREAT|O_WRONLY, 0777);
		if(cntx->gfd < 0)
		{
			close(cntx->gfd);
			pub_log_error("[%s][%d] errmsg:[%d]%d", __FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
	}

	gidx = group_alloc(cntx, name, gid, pgid, remark);
	if(gidx < 0)
	{
		pub_log_error("[%s][%d] Alloc group failed!", __FILE__, __LINE__);
		return gidx;
	}
	prdt_arg_set_gidx(gidx);

	return gidx;
}

/******************************************************************************
 **��������: prdt_arg_link
 **��    ��: ���Ӳ�Ʒ����
 **�������: 
 **     addr: ��Ʒ������ַ
 **�������: NONE
 **��    ��: 0:success  !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.10 #
 ******************************************************************************/
int prdt_arg_link(void *addr)
{
	group_cntx_t *gcntx = NULL;

	gcntx = prdt_arg_gcntx();
	gcntx->addr = addr;

	return 0;
}

/******************************************************************************
 **��������: prdt_arg_env_check
 **��    ��: �������������
 **�������: 
 **     sum: �����������Ϣ
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
static int prdt_arg_env_check(void)
{
	const char *swwork = NULL;
	char path[FILE_PATH_MAX_LEN] = {0};

	swwork = getenv("SWWORK");
	if(NULL == swwork)
	{
		pub_log_error("[%s][%d] Did't configure environment variable $SWWORK!", __FILE__, __LINE__);
		return -1;
	}

	snprintf(path, sizeof(path), "%s/%s/", swwork, PRDT_ARG_FLOCK_DIR);

	return Mkdir(path, 0777);
}

int prdt_get_value_by_name(char *name, char *value)
{
	prdt_arg_t      *ptr = NULL;

	if (name == NULL || name[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error! name is null!", __FILE__, __LINE__);
		return -1;
	}

	ptr = (prdt_arg_t *)prdt_arg_get(name);
	if (ptr != NULL)
	{	
		strcpy(value, ptr->value);
	}
	else
	{	
		value[0] = '\0';
	}

	return 0;
}

int prdt_automap(char *argname)
{
	int	i = 0;
	int	idx = 0;
	char	varname[256];
	prdt_arg_t	*arg = NULL;

	for (i = 0; i < PARAM_HASH_NUM; i++)
	{
		idx = i;
		while ((arg = prdt_arg_getbyindex(idx, argname)) != NULL)
		{
			memset(varname, 0x0, sizeof(varname));
			sprintf(varname, "#%s", arg->name);
			set_zd_data(varname, arg->value);
			uLog(SW_LOG_INFO, "[%s][%d] arg:[%s]=[%s]", __FILE__, __LINE__, varname, arg->value);
			idx = -1;
		}
	}

	return 0;
}

#endif /*__PRDT_ARG__*/
