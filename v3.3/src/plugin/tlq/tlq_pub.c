#include "lsn_pub.h"
#include "tlq_api.h"
#include "tlq_error.h"
#include "tlq_pub.h"

int tlq_conn(TLQ_ID *gid)
{
	int	ret = 0;
	TLQError	err;
	
	memset(&err, 0x0, sizeof(err));
	ret = Tlq_Conn(gid, &err);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Tlq_Conn error!", __FILE__, __LINE__);
		pub_log_error("[%s][%d] errno is : %d", __FILE__, __LINE__, err.tlq_errno);
		pub_log_error("[%s][%d] syserr is : %d", __FILE__, __LINE__, err.sys_errno);
		pub_log_error("[%s][%d] errstr is : %s", __FILE__, __LINE__, err.errstr);
		return -1;
	}
	pub_log_info("[%s][%d] Tlq_conn success!", __FILE__, __LINE__);
	
	return 0;
}

int tlq_open(TLQ_ID *gid, TLQ_QCUHDL *qcuid, char *qcu_name)
{
	int	ret = 0;
	TLQError	err;
	
	memset(&err, 0x0, sizeof(err));
	ret = Tlq_OpenQCU(gid, qcuid, qcu_name, &err);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Tlq_OpenQCU error!", __FILE__, __LINE__);
		pub_log_error("[%s][%d] errno is : %d", __FILE__, __LINE__, err.tlq_errno);
		pub_log_error("[%s][%d] syserr is : %d", __FILE__, __LINE__, err.sys_errno);
		pub_log_error("[%s][%d] errstr is : %s", __FILE__, __LINE__, err.errstr);
		return -1;
	}
	
	pub_log_info("[%s][%d] Tlq_OpenQCU success!", __FILE__, __LINE__);
	return 0;
}

int tlq_putmsg(TLQ_ID *gid, TLQ_QCUHDL *qcuid, char *send_name, char *ptr, int len, u_char *msgid, u_char *corrid)
{
	int	ret = 0;
	TLQError        err;
	TLQMSG_INFO     msg;
	TLQMSG_OPT      msgopt;
	
	memset(&err, 0x0, sizeof(err));
	Tlq_InitMsgInfo(&msg);
	Tlq_InitMsgOpt(&msgopt);
	msg.MsgSize = len;
	msg.MsgType = BUF_MSG;
	msg.Persistence = 1;
	msg.Priority = 5;
	msg.Expiry = 1000;
	if (msgid != NULL && msgid[0] != '\0')
	{
		memcpy(msg.MsgId, msgid, sizeof(msg.MsgId) - 1);
	}
	if (corrid != NULL && corrid[0] != '\0')
	{
		memcpy(msg.CorrMsgId, corrid, sizeof(msg.CorrMsgId) - 1);
	}
	strcpy(msgopt.QueName, send_name);
	ret = Tlq_PutMsg(gid, qcuid, &msg, &msgopt, NULL, ptr, &err);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Tlq_PutMsg error!", __FILE__, __LINE__);
		pub_log_error("[%s][%d] errno is : %d", __FILE__, __LINE__, err.tlq_errno);
		pub_log_error("[%s][%d] syserr is : %d", __FILE__, __LINE__, err.sys_errno);
		pub_log_error("[%s][%d] errstr is : %s", __FILE__, __LINE__, err.errstr);
		return -1;
	}
	if (msgid != NULL)
	{
		memcpy(msgid, msg.MsgId, sizeof(msg.MsgId));
	}
	pub_log_info("[%s][%d] Tlq_PutMsg success!", __FILE__, __LINE__);
	
	return 0;
}

int tlq_getmsg(TLQ_ID *gid, TLQ_QCUHDL *qcuid, char *recv_name, char **msgtext, long *len, u_char *msgid, u_char *corrid, int wait)
{
	int	ret = 0;
	TLQError        err;
	TLQMSG_INFO     msg;
	TLQMSG_OPT      msgopt;
	
	if (gid == NULL || qcuid == NULL || recv_name == NULL || recv_name[0] == '\0')
	{
		pub_log_error("[%s][%d] tlq_getmsg param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(&err, 0x0, sizeof(err));
	Tlq_InitMsgInfo(&msg);
	Tlq_InitMsgOpt(&msgopt);

	msgopt.AckMode = TLQACK_AUTO;
	strcpy(msgopt.QueName, recv_name);
	msgopt.WaitInterval = wait;
	if (msgid != NULL && msgid[0] != '\0')
	{
		memcpy(msg.MsgId, msgid, sizeof(msg.MsgId) - 1);
	}
	if (corrid != NULL && corrid[0] != '\0')
	{
		memcpy(msg.CorrMsgId, corrid, sizeof(msg.CorrMsgId) - 1);
	}
	ret = Tlq_GetMsg(gid, qcuid, &msg, &msgopt, msgtext, &err);
	if (ret < 0 && err.tlq_errno != TL_ERR_NO_MESSAGE && err.tlq_errno != TL_ERR_NOTMATCH)
	{
		pub_log_error("[%s][%d] Tlq_GetMsg error!", __FILE__, __LINE__);
		pub_log_error("[%s][%d] errno is : %d", __FILE__, __LINE__, err.tlq_errno);
		pub_log_error("[%s][%d] syserr is : %d", __FILE__, __LINE__, err.sys_errno);
		pub_log_error("[%s][%d] errstr is : %s", __FILE__, __LINE__, err.errstr);
		return -1;
	}
	else if (err.tlq_errno == TL_ERR_NO_MESSAGE || err.tlq_errno == TL_ERR_NOTMATCH)
	{
		/***
		pub_log_info("[%s][%d] NO MESSAGE! errno=[%d]", __FILE__, __LINE__, err.tlq_errno);
		***/
		return -2;
	}

	*len = msg.MsgSize;
	if (msgid != NULL)
	{
		memcpy(msgid, msg.MsgId, sizeof(msg.MsgId));
	}
	if (corrid != NULL)
	{
		memcpy(corrid, msg.CorrMsgId, sizeof(msg.CorrMsgId));
	}
	pub_log_info("[%s][%d] Tlq_GetMsg success!", __FILE__, __LINE__);
	
	return 0;
}

int tlq_close(TLQ_ID *gid, TLQ_QCUHDL *qcuid)
{
	int	ret = 0;
	TLQError        err;
	
	ret = Tlq_CloseQCU(gid, qcuid, &err);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Tlq_CloseQCU error!", __FILE__, __LINE__);
		pub_log_error("[%s][%d] errno is : %d", __FILE__, __LINE__, err.tlq_errno);
		pub_log_error("[%s][%d] syserr is : %d", __FILE__, __LINE__, err.sys_errno);
		pub_log_error("[%s][%d] errstr is : %s", __FILE__, __LINE__, err.errstr);
		return -1;
	}
	pub_log_info("[%s][%d] Tlq_CloseQCU success!", __FILE__, __LINE__);
	
	return 0;
}

int tlq_disconn(TLQ_ID *gid)
{
	int	ret = 0;
	TLQError	err;
	
	ret = Tlq_DisConn(gid, &err);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Tlq_DisConn error!", __FILE__, __LINE__);
		pub_log_error("[%s][%d] errno is : %d", __FILE__, __LINE__, err.tlq_errno);
		pub_log_error("[%s][%d] syserr is : %d", __FILE__, __LINE__, err.sys_errno);
		pub_log_error("[%s][%d] errstr is : %s", __FILE__, __LINE__, err.errstr);
		return -1;
	}
	pub_log_info("[%s][%d] Tlq_DisConn success!", __FILE__, __LINE__);

	return 0;
}

#ifdef __SW_USE_JMS__

int tlq_jmsinit(char *curl, char *cfactory, char *csendqueue, char *crecvqueue, int flag)
{
	int	ret = 0;
	static int	first = 1;
	jobject	obj;
	jclass	cls;
	JavaVM	*jvm;
	JNIEnv	*env;
	jmethodID	mid;
	JavaVMInitArgs	vm_args;
	JavaVMOption	options[3];

	if (!first)
	{
		pub_log_info("[%s][%d] Already init!", __FILE__, __LINE__);
		return 0;
	}
	options[0].optionString = "-Djava.compiler=NONE";  
	char classpath[1024] = "-Djava.class.path=";
	char *env_classpath = getenv("CLASSPATH");
	if (env_classpath)
	{
		options[1].optionString = strcat(classpath, env_classpath);  
	}

	vm_args.version = JNI_VERSION_1_6;  
	vm_args.nOptions = 2;  
	vm_args.options = options;  
	vm_args.ignoreUnrecognized = JNI_TRUE;  

	ret = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);  
	if (ret < 0)  
	{  
		pub_log_error("[%s][%d] Can't create Java VM!", __FILE__, __LINE__);  
		return -1;
	}

	cls = (*env)->FindClass(env, "tlqjms/TlqJms");
	if (!cls)
	{
		pub_log_error("[%s][%d] Can't found class TlqJms!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	pub_log_info("[%s][%d] Find class success!", __FILE__, __LINE__);

	mid = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");  
	if (!mid)
	{
		pub_log_info("[%s][%d] Can't found Constructor method!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s][%d] Found constructor method!", __FILE__, __LINE__);

	jstring url = (*env)->NewStringUTF(env, curl);
	jstring factory = (*env)->NewStringUTF(env, cfactory);
	jstring jSendQueue = NULL;
	jstring jRecvQueue = NULL;
	
	if (csendqueue != NULL)
	{
		jSendQueue = (*env)->NewStringUTF(env, csendqueue);
	}
	
	if (crecvqueue != NULL)
	{
		jRecvQueue = (*env)->NewStringUTF(env, crecvqueue);
	}

	obj = (*env)->NewObject(env, cls, mid, url, factory, jSendQueue, jRecvQueue);
	if (!obj)
	{
		pub_log_error("[%s][%d] Create NewObject error!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s][%d] New object success!", __FILE__, __LINE__);

	mid = (*env)->GetMethodID(env, cls, "init", "()I");
	if (!mid)
	{
		pub_log_error("[%s][%d] Get init method error!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s[%d] Get init method success!", __FILE__, __LINE__);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Call init method error!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s][%d] Call init method success!", __FILE__, __LINE__);

	memset(&g_tlqjms, 0x0, sizeof(g_tlqjms));
	g_tlqjms.env = env;
	g_tlqjms.cls = cls;
	g_tlqjms.obj = obj;

	if (flag != O_RECV)
	{
		mid = (*env)->GetMethodID(env, cls, "initSend", "()I");
		if (!mid)
		{
			pub_log_error("[%s][%d] Get initSend method error!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);
			return -1;
		}
		pub_log_info("[%s[%d] Get initSend method success!", __FILE__, __LINE__);

		ret = (jint)(*env)->CallIntMethod(env, obj, mid);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Call initSend method error!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);
			return -1;
		}
		pub_log_info("[%s][%d] Call initSend method success!", __FILE__, __LINE__);

		mid = (*env)->GetMethodID(env, cls, "sendMessage","(Ljava/lang/String;)I");  
		if (!mid)
		{
			pub_log_error("[%s][%d] Can't found method sendMessage!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		g_tlqjms.send_mid = mid;
		pub_log_info("[%s][%d] Get sendMessage method success!", __FILE__, __LINE__);
	}
	
	if (flag != O_SEND)
	{
		mid = (*env)->GetMethodID(env, cls, "initRecv", "()I");  
		if (!mid)
		{
			pub_log_error("[%s][%d] Get initRecv method error!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);
			return -1;
		}
		pub_log_info("[%s[%d] Get initRecv method success!", __FILE__, __LINE__);

		ret = (jint)(*env)->CallIntMethod(env, obj, mid);  
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Call initRecv method error!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);
			return -1;
		}
		pub_log_info("[%s][%d] Call initRecv method success!", __FILE__, __LINE__);
		mid = (*env)->GetMethodID(env, cls, "recvMessage","(I)Ljava/lang/String;");  
		if (!mid)
		{
			pub_log_error("[%s][%d] Can't found method recvMessage!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		g_tlqjms.recv_mid = mid;
		pub_log_info("[%s][%d] Get recvMessage method success!", __FILE__, __LINE__);
	}
	pub_log_info("[%s][%d] Jmsinit success!", __FILE__, __LINE__);
	first = 0;
	
	return 0;
}

int tlq_jmssend(char *cmessage)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jmethodID	mid;
	jstring message;
	
	env = g_tlqjms.env;
	obj = g_tlqjms.obj;
	mid = g_tlqjms.send_mid;
	message = (*env)->NewStringUTF(env, cmessage);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid, message);  
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send message error, ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}

	return 0;
}

int tlq_jmsrecv(int timeout, char *cmessage, long *len)
{
	long	begin = 0;
	long	end = 0;
	jobject	obj;
	JNIEnv	*env;
	jint	jtimeout = timeout;
	jstring	text;
	jmethodID	mid;
	
	env = g_tlqjms.env;
	obj = g_tlqjms.obj;
	mid = g_tlqjms.recv_mid;

	begin = (long)time(NULL);
	text = (jstring)(*env)->CallObjectMethod(env, obj, mid, jtimeout);
	end = (long)time(NULL);
	if (text == NULL)
	{
		if (end - begin >= SW_JMS_RECV_TIMEOUT)
		{
			return SW_JMS_NO_MSG;
		}
		pub_log_error("[%s][%d] Recv message error!", __FILE__, __LINE__);
		return -1;
	}
	jsize size = (*env)->GetStringLength(env, text);
	char *str = (char *)(*env)->GetStringUTFChars(env, text, JNI_FALSE);  
	memcpy(cmessage, str, size);
	(*env)->ReleaseStringUTFChars(env, text, str);
	*len = size;

	return 0;
}

int tlq_jmsdestroy()
{
	JavaVM	*jvm = g_tlqjms.jvm;

	(*jvm)->DestroyJavaVM(jvm);
	
	pub_log_info("[%s][%d] Destroy JavaVM!", __FILE__, __LINE__);
	
	return 0;
}

#endif

