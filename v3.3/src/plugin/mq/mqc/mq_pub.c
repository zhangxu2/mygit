#include "lsn_pub.h"
#include "mq_pub.h"

extern msg_format_enum_t g_msg_format;

int mq_conn(MQHCONN *h_conn, char *qmgr_name)
{
	MQLONG	compcode = 0;
	MQLONG  reason = 0;
	
	if (qmgr_name == NULL || qmgr_name[0] == '\0')
	{
		pub_log_error("[%s][%d] qmgr_name is null!", __FILE__, __LINE__);
		return -1;
	}

	MQCONN(qmgr_name, h_conn, &compcode, &reason);
	if (compcode == MQCC_FAILED)
	{
		pub_log_error("[%s][%d] MQCONN error! compcode=[%d] reason code=[%d]",
			__FILE__, __LINE__, compcode, reason);
		return -1;
	}

	return 0;
}

int mqm_open(MQHCONN h_conn, MQHOBJ *h_obj, char *qname, int flag)
{
	MQLONG	compcode = 0;
	MQLONG  reason = 0;
	MQLONG	o_options = 0;
	MQOD	od = {MQOD_DEFAULT};
	
	if (qname == NULL || qname[0] == '\0')
	{
		pub_log_error("[%s][%d] qname is null!", __FILE__, __LINE__);
		return -1;
	}

	strncpy(od.ObjectName, qname, (int)MQ_Q_NAME_LENGTH);
	if (flag == O_SEND)
	{
		o_options = MQOO_FAIL_IF_QUIESCING | MQOO_OUTPUT;
	}
	else if (flag == O_RECV)
	{
		o_options = MQOO_FAIL_IF_QUIESCING + MQOO_INPUT_AS_Q_DEF;
	}
	else
	{
		pub_log_error("[%s][%d] flag error! flag=[%d]", __FILE__, __LINE__, flag);
		return -1;
	}

	MQOPEN(h_conn, &od, o_options, h_obj, &compcode, &reason);
	if (compcode == MQCC_FAILED)
	{
		pub_log_error("[%s][%d] MQOPEN error! compcode=[%d] reason=[%d]",
			__FILE__, __LINE__, compcode, reason);
		return -1;
	}
	pub_log_info("[%s][%d] open [%s] queue [%s] success!", 
		__FILE__, __LINE__, flag == O_SEND ? "send" : "recv", qname);

	return 0;
}

int mq_putmsg(MQHCONN h_conn, MQHOBJ h_obj, char *send_buf, int length, MQBYTE24 msgid, MQBYTE24 corrid, long exp_time)
{
	char	*ptr = NULL;
	MQLONG  compcode = 0;
	MQLONG  reason = 0;
	MQMD	md = {MQMD_DEFAULT};
	MQPMO	pmo = {MQPMO_DEFAULT};
	
	if (send_buf == NULL || send_buf[0] == '\0')
	{
		pub_log_error("[%s][%d] send buffer is null!", __FILE__, __LINE__);
		return -1;
	}
	
	if (length <= 0)
	{
		pub_log_error("[%s][%d] Error: length=[%d]", __FILE__, __LINE__, length);
		return -1; 
	}

	pmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
	
	if (msgid == NULL || msgid[0] == '\0')
	{
		memcpy(md.MsgId, MQMI_NONE, sizeof(md.MsgId));
	}
	else
	{
		memcpy(md.MsgId, msgid, sizeof(md.MsgId));
	}
	
	if (corrid != NULL && corrid[0] != '\0')
	{
		memcpy(md.CorrelId, corrid, sizeof(md.CorrelId));
	}
	else
	{
		memcpy(md.CorrelId, MQCI_NONE, sizeof(md.CorrelId));
	}

	if (exp_time > 0)
	{
		md.Expiry = exp_time;
	}
	
	ptr = getenv("MQENCODE");
	if (ptr != NULL)
	{
		md.Encoding = atoi(ptr);
	}
	
	if (g_msg_format == MSG_STRING)
	{
		memcpy(md.Format, MQFMT_STRING, (size_t)MQ_FORMAT_LENGTH);
	}
	else
	{
		memcpy(md.Format, MQFMT_NONE, (size_t)MQ_FORMAT_LENGTH);
	}
	MQPUT(h_conn, h_obj, &md, &pmo, length, send_buf, &compcode, &reason);
	if (compcode != MQCC_OK)
	{
		pub_log_error("[%s][%d] MQPUT error! compcode=[%d] reason code=[%d]",
			__FILE__, __LINE__, compcode, reason);
		return -1;
	}

	if (msgid != NULL)
	{
		memcpy(msgid, md.MsgId, sizeof(md.MsgId));
	}

	return 0;
}

int mq_getmsg(MQHCONN h_conn, MQHOBJ h_obj, char *recv_buf, MQLONG *length, MQBYTE24 msgid, MQBYTE24 corrid, long wait_time)
{
	int	ret = 0;
	long	readlen = 0;
	char	*ptr = NULL;
	MQLONG	len = 0;
	MQLONG	compcode = 0;
	MQLONG	reason = 0;
	MQMD	md = {MQMD_DEFAULT};
	MQGMO	gmo= {MQGMO_DEFAULT};

	if (recv_buf == NULL)
	{
		pub_log_error("[%s][%d] recv buffer is null!", __FILE__, __LINE__);
		return -1;
	}
	
	if (msgid  == NULL || msgid[0] == '\0')
	{
		memcpy(md.MsgId, MQMI_NONE, sizeof(md.MsgId));
	}
	else
	{
		memcpy(md.MsgId, msgid, sizeof(md.MsgId));
	}
	
	if (corrid != NULL && corrid[0] != '\0')
	{
		memcpy(md.CorrelId, corrid, sizeof(md.CorrelId));
	}
	else
	{
		memcpy(md.CorrelId, MQCI_NONE, sizeof(md.CorrelId));
	}

	gmo.Options =  MQGMO_WAIT | MQGMO_NO_SYNCPOINT | MQGMO_CONVERT ;

	/*** 为了解决2110的问题 ***/
	ptr = getenv("MQENCODE");
	if (ptr != NULL)
	{
		md.Encoding = atoi(ptr);
	}
	else
	{
		md.Encoding = MQENC_NATIVE;
	}
	md.CodedCharSetId = MQCCSI_Q_MGR;

	if (wait_time < 0)
	{
		gmo.WaitInterval = MQWI_UNLIMITED;
	}
	else
	{
		gmo.WaitInterval = wait_time;
	}
	
	if (*length <= 0)
	{
		readlen = MAX_PACK_LEN;
	}
	else
	{
		readlen = *length;
	}
	
	MQGET(h_conn, h_obj, &md, &gmo, readlen, recv_buf, &len, &compcode, &reason);
	if (compcode != MQCC_OK)
	{
		if (reason == MQRC_NO_MSG_AVAILABLE)
		{
			return -2;
		}
		else if (reason == MQRC_TRUNCATED_MSG_FAILED)
		{
			char	*ptr = NULL;
			FILE	*fp = NULL;
			char	filename[128];
			struct timeval	tm;
			
			ptr = (char *)calloc(1, len + 1);
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
					__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
			pub_log_info("[%s][%d] Largebuffer len=[%d]", __FILE__, __LINE__, len);
			MQGET(h_conn, h_obj, &md, &gmo, len,  ptr, length, &compcode, &reason);
			if (compcode != MQCC_OK)
			{
				free(ptr);
				pub_log_error("[%s][%d] MQGET error! compcode=[%d] reason=[%d]",
					__FILE__, __LINE__, compcode, reason);
				return -1;
			}
			
			memset(filename, 0x0, sizeof(filename));
			memset(&tm, 0x0, sizeof(tm));
			gettimeofday(&tm, NULL);
			sprintf(filename, "%s/ext/%d%ld%ld", getenv("SWWORK"), 
				getpid(), tm.tv_sec, tm.tv_usec);
			pub_log_info("[%s][%d] Largebuff, filename=[%s]", __FILE__, __LINE__, filename);
			fp = fopen(filename, "wb");
			if (fp == NULL)
			{
				free(ptr);
				pub_log_error("[%s][%d]打开文件[%s]失败! errno=[%d]:[%s]",
					__FILE__, __LINE__, filename, errno, strerror(errno));
				return -1;
			}
			fwrite(ptr, len, 1, fp);
			fclose(fp);
			free(ptr);
				
			len = strlen(filename);
			memset(recv_buf, 0x0, MAX_PACK_LEN);
			sprintf(recv_buf, "%s%s", LARGE_HEAD, filename);
			*length = len + LARGE_HEAD_LEN;
			pub_log_info("[%s][%d] 大报文保存文件信息=[%s]", __FILE__, __LINE__, recv_buf);
		}
		else if (reason == MQRC_CONNECTION_BROKEN)
		{
			pub_log_error("[%s][%d] MQGET: MQRC_CONNECTION_BROKEN! CompCode=[%d] Reason Code=[%d]",
				__FILE__, __LINE__, compcode, reason);
			return -3;
		}
		else if (reason != MQRC_FORMAT_ERROR && reason != MQRC_NOT_CONVERTED)
		{
			pub_log_error("[%s][%d] MQGET:CompCode=[%d] Reason Code=[%d]",
				__FILE__, __LINE__, compcode, reason);
			return -1;
		}
		else
		{
			pub_log_info("[%s][%d] MQGET:CompCode=[%d] Reason Code=[%d]",
				__FILE__, __LINE__, compcode, reason);
		}
	}
	*length = len;

	if (msgid != NULL)
	{
		memcpy(msgid, md.MsgId, sizeof(md.MsgId));
	}
	
	if (corrid != NULL)
	{
		memcpy(corrid, md.CorrelId, sizeof(md.CorrelId));
	}
	
	ret = mq_commit(h_conn);
	if (ret)
	{
		pub_log_error("[%s][%d] mq_commit error!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

int mq_freeobj(MQHCONN h_conn, MQHOBJ *h_obj)
{
	MQLONG	compcode = 0;
	MQLONG	reason = 0;
	MQLONG	c_options;

	c_options = MQCO_NONE;
	MQCLOSE(h_conn, h_obj, c_options, &compcode, &reason);
	if (reason != MQRC_NONE)
	{
		pub_log_error("[%s][%d] MQCLOSE error! reson code=[%d]",
			__FILE__, __LINE__, reason);
		return -1;
	}
	h_obj = NULL;

	return 0;
}

int mq_freeconn(MQHCONN *h_conn)
{
	MQLONG	compcode = 0;
	MQLONG	reason = 0;

	MQDISC(h_conn, &compcode, &reason);
	if (reason != MQRC_NONE)
	{
		pub_log_error("[%s][%d] MQDISC error! reason code=[%d]",
			__FILE__, __LINE__, reason);
		return -1;
	}
	h_conn = NULL;

	return 0;
}

int mq_commit(MQHCONN h_conn)
{
	MQLONG	compcode = 0;
	MQLONG	reason = 0;

	MQCMIT(h_conn, &compcode, &reason);
	if (reason != MQRC_NONE)
	{
		pub_log_error("[%s][%d] MQCMIT error! compcode=[%d] reason code=[%d]",
			__FILE__, __LINE__, compcode, reason);
		return -1;
	}
	return 0;
}
