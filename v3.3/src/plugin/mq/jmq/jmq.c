#include "jmq.h"

extern msg_format_enum_t g_msg_format;

int mq_javainit(char *cip, int cport,char *cqmgr, char *cconnchl, int cccsid)
{
	int	ret = 0;
	jobject	obj;
	jclass	cls;
	JavaVM	*jvm;
	JNIEnv	*env;
	jmethodID	mid;
	JavaVMInitArgs	vm_args;
	JavaVMOption	options[3];
	static int	first = 1;

	if (!first)
	{
		pub_log_info("[%s][%d] Already init!", __FILE__, __LINE__);
		return 0;
	}
	options[0].optionString = "-Djava.compiler=NONE";  
	char classpath[4096] = "-Djava.class.path=";
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

	cls = (*env)->FindClass(env, "mqjava/MqBase");
	if (!cls)
	{
		pub_log_error("[%s][%d] Can't found class MqJava!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	pub_log_info("[%s][%d] Find class success!", __FILE__, __LINE__);


	mid = (*env)->GetMethodID(env, cls, "<init>", "(IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");  
	if (!mid)
	{
		pub_log_info("[%s][%d] Can't found Constructor method!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s][%d] Found constructor method!", __FILE__, __LINE__);

	jint port = cport;
	jint pid = getpid();
	jint ccsid = cccsid; 
	jstring ip = (*env)->NewStringUTF(env, cip);
	jstring qmgr = (*env)->NewStringUTF(env, cqmgr);
	jstring connchl = (*env)->NewStringUTF(env, cconnchl);

	pub_log_info("[%s][%d] ready create New Object.", __FILE__, __LINE__);
	obj = (*env)->NewObject(env, cls, mid, pid, port, ccsid, ip, qmgr, connchl);
	if (!obj)
	{
		pub_log_error("[%s][%d] Create NewObject error!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	(*env)->DeleteLocalRef(env, ip); 
	(*env)->DeleteLocalRef(env, qmgr); 
	(*env)->DeleteLocalRef(env, connchl); 
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

	mid = (*env)->GetMethodID(env, cls, "getMsgflag", "()I");
	if (!mid)
	{
		pub_log_error("[%s][%d] Get getMsgflag method error!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}

	memset(&g_jmq, 0x0, sizeof(g_jmq));
	g_jmq.env = env;
	g_jmq.cls = cls;
	g_jmq.obj = obj;
	g_jmq.mid = mid;
	g_jmq.jvm = jvm;

	first = 0;
	pub_log_info("[%s][%d] Javainit success!", __FILE__, __LINE__);

	return 0;
}

int mq_javaopen(char *queue, char *queue_ext, int index, int flag)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jclass	cls;
	jmethodID	mid;
	jstring jQueue = NULL;
	jstring jRecvQueue = NULL;
	jstring jSendQueue = NULL;

	if (queue == NULL)
	{
		pub_log_error("[%s][%d] input queue is null.", __FILE__, __LINE__);
		return -1;
	}

	env = g_jmq.env;
	obj = g_jmq.obj;
	cls = g_jmq.cls;

	jQueue = (*env)->NewStringUTF(env, queue);
	if (flag != O_SEND)
	{
		jRecvQueue = (*env)->NewStringUTF(env, queue_ext);
		mid = (*env)->GetMethodID(env, cls, "initRecv", "(Ljava/lang/String;Ljava/lang/String;)I");  
		if (!mid)
		{
			pub_log_error("[%s][%d] Get initRecv method error!", __FILE__, __LINE__);
			mq_javadestroy();
			return -1;
		}
		pub_log_info("[%s[%d] Get initRecv method success!", __FILE__, __LINE__);

		ret = (jint)(*env)->CallIntMethod(env, obj, mid, jQueue, jRecvQueue);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Call initRecv method error!", __FILE__, __LINE__);
			mq_javadestroy();
			return -1;
		}
		pub_log_info("[%s][%d] Call initRecv method success!", __FILE__, __LINE__);
		mid = (*env)->GetMethodID(env, cls, "recvMessage","(Ljava/lang/String;Ljava/lang/String;)[B");  
		if (!mid)
		{
			pub_log_error("[%s][%d] Can't found method recvMessage!", __FILE__, __LINE__);
			mq_javadestroy();
			return -1;
		}

		g_jmq.recv[index].mid = mid;
		(*env)->DeleteLocalRef(env, jRecvQueue); 
		pub_log_info("[%s][%d] Get recvMessage method success!", __FILE__, __LINE__);

	}

	if (flag != O_RECV)
	{
		jSendQueue = (*env)->NewStringUTF(env, queue_ext);
		mid = (*env)->GetMethodID(env, cls, "initSend", "(Ljava/lang/String;Ljava/lang/String;)I");
		if (!mid)
		{
			pub_log_error("[%s][%d] Get initSend method error!", __FILE__, __LINE__);
			mq_javadestroy();
			return -1;
		}
		pub_log_info("[%s[%d] Get initSend method success!", __FILE__, __LINE__);

		ret = (jint)(*env)->CallIntMethod(env, obj, mid, jQueue, jSendQueue);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Call initSend method error!", __FILE__, __LINE__);
			mq_javadestroy();
			return -1;
		}
		pub_log_info("[%s][%d] Call initSend method success!", __FILE__, __LINE__);

		mid = (*env)->GetMethodID(env, cls, "sendMessage","([BLjava/lang/String;Ljava/lang/String;I)I");
		if (!mid)
		{
			pub_log_error("[%s][%d] Can't found method sendMessage!", __FILE__, __LINE__);
			mq_javadestroy();
			return -1;
		}

		(*env)->DeleteLocalRef(env, jSendQueue); 
		g_jmq.send[index].mid = mid;
		pub_log_info("[%s][%d] Get sendMessage method success!", __FILE__, __LINE__);
	}

	(*env)->DeleteLocalRef(env, jQueue); 

	return 0;
}

int mq_javafmt(int flag)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jclass  cls;
	jint	jflag = flag;
	jmethodID	mid;

	env = g_jmq.env;
	obj = g_jmq.obj;
	cls = g_jmq.cls;

	mid = (*env)->GetMethodID(env, cls, "setFmtFlag", "(I)V");
	if (!mid)
	{
		pub_log_error("[%s][%d] Get setFmtFlag method error!", __FILE__, __LINE__);
		mq_javadestroy();
		return -1;
	}
	pub_log_info("[%s[%d] Get setFmtFlag method success!", __FILE__, __LINE__);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid, jflag);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Call setFmtFlag method error!", __FILE__, __LINE__);
		mq_javadestroy();
		return -1;
	}
	pub_log_info("[%s][%d] Call setFmtFlag method success!", __FILE__, __LINE__);

	return SW_OK;
}


int mq_javasend(char *cmessage, sw_int_t len, u_char *msgid, u_char *corrid, int exptime, int index)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jmethodID	mid;
	jint jexptime = exptime;

	env = g_jmq.env;
	obj = g_jmq.obj;
	mid = g_jmq.send[index].mid;
	pub_log_bin(SW_LOG_DEBUG, cmessage, len, "recv send msg[%s][%d] len=[%d]", __FILE__, __LINE__, len);

	jstring jmsgid = (*env)->NewStringUTF(env, (char *)msgid);
	jstring jcorrid = (*env)->NewStringUTF(env, (char *)corrid);
	jbyte* byte = (jbyte*)cmessage;
	jbyteArray bytearray = (*env)->NewByteArray(env,len); 
	(*env)->SetByteArrayRegion(env, bytearray, 0, len, byte);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid, bytearray, jmsgid, jcorrid, jexptime);  
	if (ret < 0)
	{
		(*env)->DeleteLocalRef(env, jmsgid); 
		(*env)->DeleteLocalRef(env, jcorrid); 
		(*env)->DeleteLocalRef(env, bytearray); 
		pub_log_error("[%s][%d] Send message error, ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}

	(*env)->DeleteLocalRef(env, jmsgid); 
	(*env)->DeleteLocalRef(env, jcorrid); 
	(*env)->DeleteLocalRef(env, bytearray); 

	return 0;
}

int mq_javarecv(int index, char *cmessage, u_char *msgid, u_char *corrid, int *len)
{
	jint	ret;
	jobject	obj;
	JNIEnv	*env;
	jbyteArray text;
	jmethodID mid;

	env = g_jmq.env;
	obj = g_jmq.obj;
	mid = g_jmq.recv[index].mid;

	jstring jmsgid = (*env)->NewStringUTF(env, (char *)msgid);
	jstring jcorrid = (*env)->NewStringUTF(env, (char *)corrid);
	text = (jbyteArray)(*env)->CallObjectMethod(env, obj, mid, msgid, corrid);
	if (text == NULL)
	{
		ret = (jint)(*env)->CallIntMethod(env, obj, g_jmq.mid);
		if (ret == 0)
		{
			(*env)->DeleteLocalRef(env, jmsgid); 
			(*env)->DeleteLocalRef(env, jcorrid); 
			(*env)->DeleteLocalRef(env, text);
			return  SW_NO_MSG;
		}
		(*env)->DeleteLocalRef(env, jmsgid); 
		(*env)->DeleteLocalRef(env, jcorrid); 
		(*env)->DeleteLocalRef(env, text);
		pub_log_error("[%s][%d] recv message error.", __FILE__, __LINE__);
		return -1;
	}

	(*env)->DeleteLocalRef(env, jmsgid); 
	(*env)->DeleteLocalRef(env, jcorrid); 

	jbyte *str = (jbyte *)(*env)->GetByteArrayElements(env, text, JNI_FALSE);
	if (str != NULL)
	{
		jsize size = (*env)->GetArrayLength(env, text);
		memcpy(cmessage, str, size);
		*len = size;
		(*env)->ReleaseByteArrayElements(env, text, str, JNI_ABORT);
	}
	
	(*env)->DeleteLocalRef(env, text); 

	return 0;
}

int mq_javadestroy()
{
	JavaVM	*jvm = g_jmq.jvm;

	(*jvm)->DestroyJavaVM(jvm);

	pub_log_info("[%s][%d] Destroy JavaVM!", __FILE__, __LINE__);

	return 0;
}

int mq_jmsinit(char *cip, int cport, char *cqmgr, char *csendqueue, char *crecvqueue, int flag)
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
	char classpath[4096] = "-Djava.class.path=";
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

	cls = (*env)->FindClass(env, "mqjms/MqJms");
	if (!cls)
	{
		pub_log_error("[%s][%d] Can't found class TlqJms!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	pub_log_info("[%s][%d] Find class success!", __FILE__, __LINE__);


	mid = (*env)->GetMethodID(env, cls, "<init>", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");  
	if (!mid)
	{
		pub_log_info("[%s][%d] Can't found Constructor method!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s][%d] Found constructor method!", __FILE__, __LINE__);

	jint port = cport;
	jstring ip = (*env)->NewStringUTF(env, cip);
	jstring qmgr = (*env)->NewStringUTF(env, cqmgr);
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

	pub_log_info("[%s][%d] ready create New Object.", __FILE__, __LINE__);
	obj = (*env)->NewObject(env, cls, mid, port, ip, qmgr, jSendQueue, jRecvQueue);
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

	memset(&g_jmq, 0x0, sizeof(g_jmq));
	g_jmq.env = env;
	g_jmq.cls = cls;
	g_jmq.obj = obj;
	g_jmq.jvm = jvm;

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
		g_jmq.send[0].mid = mid;
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
		g_jmq.recv[0].mid = mid;
		pub_log_info("[%s][%d] Get recvMessage method success!", __FILE__, __LINE__);
	}

	pub_log_info("[%s][%d] Jmsinit success!", __FILE__, __LINE__);
	first = 0;

	return 0;
}

int mq_jmssend(char *cmessage)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jmethodID	mid;
	jstring message;

	env = g_jmq.env;
	obj = g_jmq.obj;
	mid = g_jmq.send[0].mid;
	message = (*env)->NewStringUTF(env, cmessage);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid, message);  
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send message error, ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}

	return 0;
}

int mq_jmsrecv(int timeout, char *cmessage, int *len)
{
	long	begin = 0;
	long	end = 0;
	jobject	obj;
	JNIEnv	*env;
	jint	jtimeout = timeout;
	jstring	text;
	jmethodID	mid;

	env = g_jmq.env;
	obj = g_jmq.obj;
	mid = g_jmq.recv[0].mid;

	begin = (long)time(NULL);
	text = (jstring)(*env)->CallObjectMethod(env, obj, mid, jtimeout);
	end = (long)time(NULL);
	if (text == NULL)
	{
		if (end - begin >= SW_RECV_TIMEOUT)
		{
			return SW_NO_MSG;
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

int mq_jmsdestroy()
{
	JavaVM	*jvm = g_jmq.jvm;

	(*jvm)->DestroyJavaVM(jvm);

	pub_log_info("[%s][%d] Destroy JavaVM!", __FILE__, __LINE__);

	return 0;
}


