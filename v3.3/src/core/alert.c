#include "alert.h"
#include "pub_log.h"
#include "alert_proc.h"

#if defined(__ALERT_SUPPORT__)

static sw_int_t alert_cfg_parse(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_level(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_switch(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_remote(sw_xmltree_t *xml, alert_cfg_t *cfg);
static sw_int_t alert_parse_remotesvr(sw_xmltree_t *xml, alert_remote_t *remote);

/* 获取预警配置文件绝对路径 */
#define AlertGetCfgPath(fname, size) \
	snprintf(fname, size, "%s/%s/%s", getenv("SWWORK"), ALERT_CFG_PATH, ALERT_CFG_FILE);


/* 预警队列ID */
static int g_alert_msqid = -1;

/* 预警开关标识 */
static bool g_alert_switch = true;
#define alert_set_switch(b) (g_alert_switch = b)
#define alert_get_switch()  (g_alert_switch)
/******************************************************************************
 **函数名称: alert_set_msqid
 **功    能: 设置预警队列ID
 **输入参数:
 **      msqid: 队列ID
 **输出参数:
 **返    回: VOID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.13 #
 ******************************************************************************/
void alert_set_msqid(int msqid)
{
	g_alert_msqid = msqid;
}

/******************************************************************************
 **函数名称: alert_get_msqid
 **功    能: 获取预警队列ID
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 预警队列ID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.13 #
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
 **函数名称: alert_link
 **功    能: 打开预警队列
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 预警队列ID
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.15 #
 ******************************************************************************/
int alert_link(void)
{
	key_t key = 0;
	sw_int_t ret = SW_ERR;
	sw_int32_t msqid = 0;
	alert_cfg_t cfg;

	memset(&cfg, 0, sizeof(cfg));

	/* 1. 加载配置文件 */
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

	/* 2. 打开或新建预警队列 */
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
 **函数名称: alert_errno_get_mtype
 **功    能: 获取错误码类型(平台错误码或交易错误码)
 **输入参数:
 **      _errno: 错误码
 **输出参数:
 **返    回: 错误码类型，其取值范围在ALERT_TYPE_ENUM中定义
 **实现描述: 
 **     查找预警信息类型，其取值范围在ALERT_TYPE_ENUM中定义
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.13 #
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
 **函数名称: alert_msg
 **功    能: 设置错误码和备注信息(平台/交易调用接口)
 **输入参数:
 **      _errno: 错误码
 **      format: 格式化输出
 **      ...: 可变参数
 **输出参数:
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 查找预警信息类型，其取值范围在ALERT_TYPE_ENUM中定义
 **     2. 设置错误码和备注信息
 **     3. 将预警信息写入预警队列
 **注意事项: 
 **  	使用指针进行强制转换，减少数据复制，提高运行效率
 **作    者: # Qifeng.zou # 2013.04.13 #
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

	/* 1. 查找预警信息类型 */
	msqmsg.mtype = alert_errno_get_mtype(_errno);
	if(ALERT_TYPE_UNKNOW == msqmsg.mtype)
	{
		pub_log_error("[%s][%d] Error code is out of range![%d]",
				__FILE__, __LINE__, _errno);
		return -1;
	}

	/* 2. 设置错误码和备注信息 */
	mtext = (alert_error_t *)msqmsg.mtext;
	mtext->_errno = _errno;
	va_start(args, format);
	vsnprintf(mtext->remark, sizeof(mtext->remark), format, args);
	va_end(args);

	/* 3. 将预警信息写入预警队列 */
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
 **函数名称: alert_msq_get_key
 **功    能: 获取预警消息队列KEY值
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 预警消息队列KEY值
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.07.03 #
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
 **函数名称: alert_msq_creat
 **功    能: 创建预警消息队列
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 消息队列ID
 **实现描述: 
 **		1. 获取预警队列KEY
 **		2. 打开or创建消息队列
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.07.03 #
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
 **函数名称: alert_cfg_load
 **功    能: 加载预警配置
 **输入参数: NONE
 **输出参数: 
 **      cfg: 预警配置
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **      1. 加载预警配置文件
 **      2. 提取预警配置信息
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
sw_int_t alert_cfg_load(alert_cfg_t *cfg)
{
	sw_int_t ret = SW_ERR;
	sw_xmltree_t *xml = NULL;
	char fname[FILE_NAME_MAX_LEN] = {0};

	AlertGetCfgPath(fname, sizeof(fname));

	/* 1. 加载预警配置文件 */
	xml = pub_xml_crtree(fname);
	if(NULL == xml)
	{
		pub_log_error("[%s][%d] Load cycle configure file failed![%s]",
				__FILE__, __LINE__, fname);
		return SW_ERR;
	}

	/* 2. 提取预警配置信息 */
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
 **函数名称: alert_cfg_parse
 **功    能: 提取预警配置信息
 **输入参数: 
 **      xml: 预警配置树
 **      cfg: 预警配置结构体
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **      1. 加载预警配置文件
 **      2. 获取预警级别
 **      3. 获取监控开关配置
 **      4. 获取远程服务配置
 **注意事项: 
 **      因预警进程的启动早于平台工作进程，因此，可不用过多考虑效率!
 **作    者: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
static sw_int_t alert_cfg_parse(sw_xmltree_t *xml, alert_cfg_t *cfg)
{
	sw_int_t ret = SW_ERR;

	/* 1. 预警开关 */
	ret = alert_parse_switch(xml, cfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get alert switch failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 2. 预警级别: 非空 */
	ret = alert_parse_level(xml, cfg);
	if(SW_OK != ret)
	{
		pub_log_error("[%s][%d] Get alert level failed!", __FILE__, __LINE__);
		return SW_ERR;
	}

	/* 3. 监控开关: 非空 */

	/* 4. 远程服务配置 */
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
 **函数名称: alert_parse_switch
 **功    能: 提取预警开关配置
 **输入参数: 
 **      xml: 预警配置树
 **      cfg: 预警配置结构体
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **      因预警进程的启动早于平台工作进程，因此，可不用过多考虑效率!
 **作    者: # Qifeng.zou # 2013.07.03 #
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
 **函数名称: alert_parse_level
 **功    能: 提取预警级别配置
 **输入参数: 
 **      xml: 预警配置树
 **      cfg: 预警配置结构体
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **      因预警进程的启动早于平台工作进程，因此，可不用过多考虑效率!
 **作    者: # Qifeng.zou # 2013.04.11 #
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
		 **函数名称: alert_parse_monitor
		 **功    能: 提取预警监控开关配置
		 **输入参数: 
		 **      xml: 预警配置树
		 **      cfg: 预警配置结构体
		 **输出参数: NONE
		 **返    回: 0:成功 !0:失败
		 **实现描述: 
		 **注意事项: 
		 **      因预警进程的启动早于平台工作进程，因此，可不用过多考虑效率!
		 **作    者: # Qifeng.zou # 2013.04.11 #
		 ******************************************************************************/


		/* 1. 系统监控开关 */


		/* 2. 平台监控开关 */

		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 **函数名称: alert_parse_remote
 **功    能: 提取预警远程服务配置
 **输入参数: 
 **      xml: 预警配置树
 **      cfg: 预警配置结构体
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **      因预警进程的启动早于平台工作进程，因此，可不用过多考虑效率!
 **作    者: # Qifeng.zou # 2013.04.11 #
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
		/* 1. 申请动态空间 */
		new_remote = (alert_remote_t *)calloc(1, sizeof(alert_remote_t));
		if(NULL == new_remote)
		{
			pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
			return SW_ERR;
		}
		new_remote->next = NULL;

		/* 2. 获取远程服务配置 */
		xml->current = node;
		ret = alert_parse_remotesvr(xml, new_remote);
		if(SW_OK != ret)
		{
			free(new_remote), new_remote=NULL;
			pub_log_error("[%s][%d] Get remote server information failed!idx:%d",
					__FILE__, __LINE__, idx);
			return SW_ERR;
		}

		/* 3. 加入远程配置链表 */
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
 **函数名称: alert_parse_remotesvr
 **功    能: 提取预警远程服务项配置
 **输入参数: 
 **      xml: 预警配置树
 **      cfg: 预警配置结构体
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **      因预警进程的启动早于平台工作进程，因此，可不用过多考虑效率!
 **作    者: # Qifeng.zou # 2013.04.11 #
 ******************************************************************************/
static sw_int_t alert_parse_remotesvr(sw_xmltree_t *xml, alert_remote_t *remote)
{
	sw_xmlnode_t *node = NULL;

	/* 1. USE: 是否启用配置。非空 */
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

	/* 2. 获取预警级别 */
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

	/* 3. IP: 远程IP地址。USE开启，则非空，否则可空 */
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

	/* 4. PORT: 远程端口。USE开启，则非空，否则可空 */
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

	/* 5. LIBNAME: 组包动态链接库名。可空 */
	node = pub_xml_locnode(xml, "LIBNAME");
	if(NULL == node)
	{
		pub_log_error("[%s][%d] Didn't configure LIBNAME!", __FILE__, __LINE__);
	}
	else if(NULL != node->value)
	{
		snprintf(remote->libname, sizeof(remote->libname), "%s", node->value);
	}

	/* 6. FUNC: 组包函数名。 可空 */
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
