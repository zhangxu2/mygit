#include "alert.h"
#include "pub_log.h"
#include "alert_proc.h"

#if defined(__ALERT_SUPPORT__)

static sw_int_t alert_cfg_parse(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_level(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_switch(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_remote(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_remotesvr(sw_xmltree_t *xml, alert_remote_t *remote);

/* ��ȡԤ�������ļ�����·�� */
#define AlertGetCfgPath(fname, size) \
	snprintf(fname, size, "%s/%s/%s", getenv("SWWORK"), ALERT_CFG_PATH, ALERT_CFG_FILE);


/* Ԥ������ID */
static int g_alert_msqid = -1;

/* Ԥ�����ر�ʶ */
static bool g_alert_switch = true;
#define alert_set_switch(b) (g_alert_switch = b)
#define alert_get_switch()  (g_alert_switch)
/******************************************************************************
 **��������: alert_set_msqid
 **��    ��: ����Ԥ������ID
 **�������:
 **      msqid: ����ID
 **�������:
 **��    ��: VOID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
void alert_set_msqid(int msqid)
{
	g_alert_msqid = msqid;
}

/******************************************************************************
 **��������: alert_get_msqid
 **��    ��: ��ȡԤ������ID
 **�������: NONE
 **�������: NONE
 **��    ��: Ԥ������ID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
int alert_get_msqid(void)
{
	return g_alert_msqid;
}

int alert_check_msqid(void)
{
	int	ret = 0;
	struct msqid_ds	msqds;

	if (g_alert_msqid < 0)
	{
		return -1;
	}

	memset(&msqds, 0x0, sizeof(msqds));
	ret = msgctl(g_alert_msqid, IPC_STAT, &msqds);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] msgctl [%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, g_alert_msqid, errno, strerror(errno));
		return -1;
	}

	return 0;
}

/******************************************************************************
 **��������: alert_link
 **��    ��: ��Ԥ������
 **�������: NONE
 **�������: NONE
 **��    ��: Ԥ������ID
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.15 #
 ******************************************************************************/
int alert_link(void)
{
	key_t key = 0;
	sw_int_t ret = SW_ERR;
	sw_int32_t msqid = 0;
	alert_cfg_t cfg;

	memset(&cfg, 0, sizeof(cfg));

	/* 1. ���������ļ� */
	ret = alert_cfg_load(&cfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Load alert configuration failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	if(SWITCH_OFF == cfg._switch)
	{
		alert_set_switch(false);
		return SW_OK;
	}

	/* 2. �򿪻��½�Ԥ������ */
	msqid = alert_msq_creat();
	if(msqid < 0)
	{
		pub_log_error(
				"[%s][%d] Open or create msq failed! key:[%d]", __FILE__, __LINE__, key);
		return SW_ERR;
	}

	alert_set_msqid(msqid);

	return SW_OK;
}

/******************************************************************************
 **��������: alert_errno_get_mtype
 **��    ��: ��ȡ����������(ƽ̨��������״�����)
 **�������:
 **      _errno: ������
 **�������:
 **��    ��: ���������ͣ���ȡֵ��Χ��ALERT_TYPE_ENUM�ж���
 **ʵ������: 
 **     ����Ԥ����Ϣ���ͣ���ȡֵ��Χ��ALERT_TYPE_ENUM�ж���
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
static int alert_errno_get_mtype(int _errno)
{
	if((_errno >= ERRNO_BEGIN) && (_errno < ERRNO_RSV_BEGIN))
	{
		if((_errno >= ERRNO_OPR_BEGIN) && (_errno <= ERRNO_OPR_END))
		{
			return ALERT_TYPE_OPRERR;
		}
		else if (_errno >= ERRNO_PLAT_BEGIN && _errno <= ERRNO_PLAT_END)
		{
			return ALERT_TYPE_PLATERR;
		}
		else
		{
			return ALERT_TYPE_UNKNOW;
		}

	}

	return ALERT_TYPE_UNKNOW;
}

/******************************************************************************
 **��������: alert_msg
 **��    ��: ���ô�����ͱ�ע��Ϣ(ƽ̨/���׵��ýӿ�)
 **�������:
 **      _errno: ������
 **      format: ��ʽ�����
 **      ...: �ɱ����
 **�������:
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     1. ����Ԥ����Ϣ���ͣ���ȡֵ��Χ��ALERT_TYPE_ENUM�ж���
 **     2. ���ô�����ͱ�ע��Ϣ
 **     3. ��Ԥ����Ϣд��Ԥ������
 **ע������: 
 **  	ʹ��ָ�����ǿ��ת�����������ݸ��ƣ��������Ч��
 **��    ��: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
int alert_msg(int _errno, const char *format, ...)
{
	int ret = 0, msqid = 0;
	va_list args;
	alert_msq_t msqmsg;
	alert_error_t *mtext = NULL;

	memset(&msqmsg, 0, sizeof(msqmsg));
	memset(&mtext, 0, sizeof(mtext));

	if(!alert_get_switch())
	{
		pub_log_debug("[%s][%d] Alert was switch off!", __FILE__, __LINE__);
		return SW_OK;
	}

	msqid = alert_get_msqid();
	if(msqid < 0)
	{
		pub_log_error("[%s][%d] Alert msqid exception! msqid:[%d]",
				__FILE__, __LINE__, msqid);
		return -1;
	}

	/* 1. ����Ԥ����Ϣ���� */
	msqmsg.mtype = alert_errno_get_mtype(_errno);
	if(ALERT_TYPE_UNKNOW == msqmsg.mtype)
	{
		pub_log_error("[%s][%d] Error code is out of range![%d]",
				__FILE__, __LINE__, _errno);
		return -1;
	}

	/* 2. ���ô�����ͱ�ע��Ϣ */
	mtext = (alert_error_t *)msqmsg.mtext;
	mtext->_errno = _errno;
	va_start(args, format);
	vsnprintf(mtext->remark, sizeof(mtext->remark), format, args);
	va_end(args);

	/* 3. ��Ԥ����Ϣд��Ԥ������ */
	do
	{
		ret = msgsnd(msqid, &msqmsg, sizeof(alert_error_t), IPC_NOWAIT);
		if(ret < 0)
		{
			if(EINTR == errno)
			{
				continue;
			}

			pub_log_error("[%s][%d] Send alert msg failed! msqid:%d[%d] errmsg:[%d][%s]",
					__FILE__, __LINE__, msqid, _errno, errno, strerror(errno));
			return -1;
		}

		break;
	}while(1);

	return 0;
}

/******************************************************************************
 **��������: alert_msq_get_key
 **��    ��: ��ȡԤ����Ϣ����KEYֵ
 **�������: NONE
 **�������: NONE
 **��    ��: Ԥ����Ϣ����KEYֵ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
sw_int32_t alert_msq_get_key(void)
{
	int fd = 0;
	char fname[FILE_NAME_MAX_LEN] = {0};

	snprintf(fname, sizeof(fname), "%s/cfg/alert/%s", getenv("SWWORK"), ALERT_MSQ_KEY_FILE);
	fd = open(fname, O_WRONLY | O_CREAT, 0644);
	if (fd < 0)
	{
		pub_log_error("[%s][%d]create  file  %s error", __FILE__, __LINE__, fname);
		return SW_ERROR;
	}
	close(fd);

	return ftok(fname, 0x56);
}

/******************************************************************************
 **��������: alert_msq_creat
 **��    ��: ����Ԥ����Ϣ����
 **�������: NONE
 **�������: NONE
 **��    ��: ��Ϣ����ID
 **ʵ������: 
 **		1. ��ȡԤ������KEY
 **		2. ��or������Ϣ����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
sw_int32_t alert_msq_creat(void)
{
	key_t key = -1;
	sw_int_t msqid = -1;

	key = alert_msq_get_key();
	if(key < 0)
	{
		pub_log_error("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	msqid = pub_msq_creat(key, 0);
	if(msqid < 0)
	{
		pub_log_error("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	return msqid;
}

/******************************************************************************
 **��������: alert_cfg_load
 **��    ��: ����Ԥ������
 **�������: NONE
 **�������: 
 **      cfg: Ԥ������
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **      1. ����Ԥ�������ļ�
 **      2. ��ȡԤ��������Ϣ
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
sw_int_t alert_cfg_load(alert_cfg_t *cfg)
{
	sw_int_t ret = SW_ERR;
	sw_xmltree_t *xml = NULL;
	char fname[FILE_NAME_MAX_LEN] = {0};

	AlertGetCfgPath(fname, sizeof(fname));

	/* 1. ����Ԥ�������ļ� */
	xml = pub_xml_crtree(fname);
	if(NULL == xml)
	{
		pub_log_error("[%s][%d] Load cycle configure file failed![%s]",
				__FILE__, __LINE__, fname);
		return SW_ERR;
	}

	/* 2. ��ȡԤ��������Ϣ */
	ret = alert_cfg_parse(xml, cfg);
	if(SW_OK != ret)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] Get configure message failed![%s]",
				__FILE__, __LINE__, fname);
		return SW_ERR;
	}

	pub_log_info("[%s][%d] Load configure message success![%s]",
			__FILE__, __LINE__, fname);

	pub_xml_deltree(xml);
	return SW_OK;
}

/******************************************************************************
 **��������: alert_cfg_parse
 **��    ��: ��ȡԤ��������Ϣ
 **�������: 
 **      xml: Ԥ��������
 **      cfg: Ԥ�����ýṹ��
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **      1. ����Ԥ�������ļ�
 **      2. ��ȡԤ������
 **      3. ��ȡ��ؿ�������
 **      4. ��ȡԶ�̷�������
 **ע������: 
 **      ��Ԥ�����̵���������ƽ̨�������̣���ˣ��ɲ��ù��࿼��Ч��!
 **��    ��: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
static sw_int_t alert_cfg_parse(sw_xmltree_t *xml, alert_cfg_t *cfg)
{
	sw_int_t ret = SW_ERR;

	/* 1. Ԥ������ */
	ret = alert_parse_switch(xml, cfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get alert switch failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. Ԥ������: �ǿ� */
	ret = alert_parse_level(xml, cfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get alert level failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. ��ؿ���: �ǿ� */

	/* 4. Զ�̷������� */
	ret = alert_parse_remote(xml, cfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get remote server configuration failed!",
				__FILE__, __LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **��������: alert_parse_switch
 **��    ��: ��ȡԤ����������
 **�������: 
 **      xml: Ԥ��������
 **      cfg: Ԥ�����ýṹ��
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **      ��Ԥ�����̵���������ƽ̨�������̣���ˣ��ɲ��ù��࿼��Ч��!
 **��    ��: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
static sw_int_t alert_parse_switch(sw_xmltree_t *xml, alert_cfg_t *cfg)
{
	sw_xmlnode_t *node = NULL;

	node = pub_xml_locnode(xml, ".ALERT.SWITCH");
	if((NULL == node)
			|| (NULL == node->value) || ('\0' == node->value[0]))
	{
		pub_log_info("[%s][%d] Didn't configure switch!", __FILE__, __LINE__);
		cfg->_switch = SWITCH_OFF;
		return SW_OK;
	}

	cfg->_switch = atoi(node->value);
	if((SWITCH_OFF != cfg->_switch) && (SWITCH_ON != cfg->_switch))
	{
		pub_log_info("[%s][%d] Didn't switch on switch!", __FILE__, __LINE__);
		cfg->_switch = SWITCH_OFF;
		return SW_OK;
	}

	return SW_OK;
}

/******************************************************************************
 **��������: alert_parse_level
 **��    ��: ��ȡԤ����������
 **�������: 
 **      xml: Ԥ��������
 **      cfg: Ԥ�����ýṹ��
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **      ��Ԥ�����̵���������ƽ̨�������̣���ˣ��ɲ��ù��࿼��Ч��!
 **��    ��: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
static sw_int_t alert_parse_level(sw_xmltree_t *xml, alert_cfg_t *cfg)
{
	sw_xmlnode_t *node = NULL;

	node = pub_xml_locnode(xml, ".ALERT.LEVEL");
	if((NULL == node)
			|| (NULL == node->value) || ('\0' == node->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure alert level!", __FILE__, __LINE__);
		return SW_ERR;
	}

	cfg->level = atoi(node->value);
	if((cfg->level < ERR_LEVEL_PROMPT) || (cfg->level >= ERR_LEVEL_TOTAL))
	{
		pub_log_error("[%s][%d] Alert level is not right!", __FILE__, __LINE__);


		/******************************************************************************
		 **��������: alert_parse_monitor
		 **��    ��: ��ȡԤ����ؿ�������
		 **�������: 
		 **      xml: Ԥ��������
		 **      cfg: Ԥ�����ýṹ��
		 **�������: NONE
		 **��    ��: 0:�ɹ� !0:ʧ��
		 **ʵ������: 
		 **ע������: 
		 **      ��Ԥ�����̵���������ƽ̨�������̣���ˣ��ɲ��ù��࿼��Ч��!
		 **��    ��: # Qifeng.zou # 2013.04.11 #
		 ******************************************************************************/


		/* 1. ϵͳ��ؿ��� */


		/* 2. ƽ̨��ؿ��� */

		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **��������: alert_parse_remote
 **��    ��: ��ȡԤ��Զ�̷�������
 **�������: 
 **      xml: Ԥ��������
 **      cfg: Ԥ�����ýṹ��
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **      ��Ԥ�����̵���������ƽ̨�������̣���ˣ��ɲ��ù��࿼��Ч��!
 **��    ��: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
static sw_int_t alert_parse_remote(sw_xmltree_t *xml, alert_cfg_t *cfg)
{
	sw_int_t ret = SW_ERR, idx = 0;
	sw_xmlnode_t *node = NULL;
	alert_remote_t *remote=NULL, *new_remote=NULL;

	node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Didn't configure remote server!", __FILE__, __LINE__);
		return SW_OK;
	}

	while(NULL != node)
	{
		idx++;
		/* 1. ���붯̬�ռ� */
		new_remote = (alert_remote_t *)calloc(1, sizeof(alert_remote_t));
		if(NULL == new_remote)
		{
			pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
			return SW_ERR;
		}
		new_remote->next = NULL;

		/* 2. ��ȡԶ�̷������� */
		xml->current = node;
		ret = alert_parse_remotesvr(xml, new_remote);
		if(SW_OK != ret)
		{
			free(new_remote), new_remote=NULL;
			pub_log_error("[%s][%d] Get remote server information failed!idx:%d",
					__FILE__, __LINE__, idx);
			return SW_ERR;
		}

		/* 3. ����Զ���������� */
		if(NULL == remote)
		{
			cfg->remote = new_remote;
			remote = cfg->remote;
		}
		else
		{
			remote->next = new_remote;
			remote = new_remote;
		}

		pub_log_info("[%s][%d] Get remote server information success!ipaddr:%s[%d]",
				__FILE__, __LINE__, remote->ipaddr, remote->port);
		node = node->next;
	}

	return SW_OK;
}

/******************************************************************************
 **��������: alert_parse_remotesvr
 **��    ��: ��ȡԤ��Զ�̷���������
 **�������: 
 **      xml: Ԥ��������
 **      cfg: Ԥ�����ýṹ��
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **      ��Ԥ�����̵���������ƽ̨�������̣���ˣ��ɲ��ù��࿼��Ч��!
 **��    ��: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
static sw_int_t alert_parse_remotesvr(sw_xmltree_t *xml, alert_remote_t *remote)
{
	sw_xmlnode_t *node = NULL;

	/* 1. USE: �Ƿ��������á��ǿ� */
	node = pub_xml_locnode(xml, "SWITCH");
	if((NULL == node)
			|| (NULL == node->value) || ('\0' == node->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure SWITCH!", __FILE__, __LINE__);
		return SW_ERR;
	}
	remote->isuse = atoi(node->value);
	if((SWITCH_OFF!=remote->isuse) && (SWITCH_ON!=remote->isuse))
	{
		pub_log_error("[%s][%d] Switch configuration is not right!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. ��ȡԤ������ */
	node = pub_xml_locnode(xml, "LEVEL");
	if((NULL == node)
			|| (NULL == node->value) || ('\0' == node->value[0]))
	{
		pub_log_error("[%s][%d] Didn't configure LEVEL!", __FILE__, __LINE__);
		return SW_ERR;
	}
	remote->level = atoi(node->value);
	if((remote->level < ERR_LEVEL_PROMPT) && (remote->level >= ERR_LEVEL_TOTAL))
	{
		pub_log_error("[%s][%d] Level configuration is not right!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. IP: Զ��IP��ַ��USE��������ǿգ�����ɿ� */
	node = pub_xml_locnode(xml, "IP");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Didn't configure the IP address of remote server!",
				__FILE__, __LINE__);
	}
	else if(NULL != node->value)
	{
		snprintf(remote->ipaddr, sizeof(remote->ipaddr), "%s", node->value);
	}

	/* 4. PORT: Զ�̶˿ڡ�USE��������ǿգ�����ɿ� */
	node = pub_xml_locnode(xml, "PORT");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Didn't configure the port of remote server!",
				__FILE__, __LINE__);
	}
	if(NULL != node->value)
	{
		remote->port = atoi(node->value);
	}

	/* 5. LIBNAME: �����̬���ӿ������ɿ� */
	node = pub_xml_locnode(xml, "LIBNAME");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Didn't configure LIBNAME!", __FILE__, __LINE__);
	}
	else if(NULL != node->value)
	{
		snprintf(remote->libname, sizeof(remote->libname), "%s", node->value);
	}

	/* 6. FUNC: ����������� �ɿ� */
	node = pub_xml_locnode(xml, "FUNCNAME");
	if(NULL == node)
	{
		pub_log_debug("[%s][%d] Didn't configure FUNCNAME!", __FILE__, __LINE__);
	}
	else if(NULL != node->value)
	{
		snprintf(remote->funcname, sizeof(remote->funcname), "%s", node->value);
	}

	return SW_OK;
}

int alert_err(char *fmt, ...)
{
	va_list	ap;
	char	errmsg[1024];

	memset(errmsg, 0x0, sizeof(errmsg));
	va_start(ap, fmt);
	vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);

	alert_msg(51001, errmsg);

	return 0;
}

int alert_crit(char *fmt, ...)
{
	va_list	ap;
	char	errmsg[1024];

	memset(errmsg, 0x0, sizeof(errmsg));
	va_start(ap, fmt);
	vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);

	alert_msg(51002, errmsg);

	return 0;
}

int alert_emerg(char *fmt, ...)
{
	va_list	ap;
	char	errmsg[1024];

	memset(errmsg, 0x0, sizeof(errmsg));
	va_start(ap, fmt);
	vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);

	alert_msg(6000, errmsg);

	return 0;
}

#else

int alert_link(void)
{
	return 0;
}

int alert_msg(int _errno, const char *format, ...)
{
	return 0;
}

int alert_emerg(char *fmt, ...)
{
	return 0;
}

int alert_crit(char *fmt, ...)
{
	return 0;
}

int alert_err(char *fmt, ...)
{
	return 0;
}

#endif /*__ALERT_SUPPORT__*/
