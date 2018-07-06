#include "jmq.h"

int mq_javainit(char *cip, int cport,char *cqmgr, char *cconnchl, int cccsid)
{
	int ret = 0;
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


int mq_javasend(char *cmessage, sw_int_t len, char *msgid, char *corrid, int exptime, int index)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jmethodID	mid;
	jint jexptime = exptime;

	env = g_jmq.env;
	obj = g_jmq.obj;
	mid = g_jmq.send[index].mid;
	pub_log_bin(SW_LOG_INFO, cmessage, len, "recv semd msg[%s][%d] len=[%ld]", __FILE__, __LINE__, len);

	jstring jmsgid = (*env)->NewStringUTF(env,msgid);
	jstring jcorrid = (*env)->NewStringUTF(env, corrid);
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

int mq_javarecv(int index, char *cmessage, char *msgid, char *corrid, int *len)
{
	jint	ret;
	jobject	obj;
	JNIEnv	*env;
	jbyteArray text;
	jmethodID mid;

	env = g_jmq.env;
	obj = g_jmq.obj;
	mid = g_jmq.recv[index].mid;

	jstring jmsgid = (*env)->NewStringUTF(env,msgid);
	jstring jcorrid = (*env)->NewStringUTF(env, corrid);
	text = (jbyteArray)(*env)->CallObjectMethod(env, obj, mid, msgid, corrid);
	if (text == NULL)
	{
		ret = (jint)(*env)->CallIntMethod(env, obj, g_jmq.mid);
		if (ret == 0)
		{
			(*env)->DeleteLocalRef(env, jmsgid); 
			(*env)->DeleteLocalRef(env, jcorrid); 
			return  SW_NO_MSG;
		}
		pub_log_error("[%s][%d] recv message error.", __FILE__, __LINE__);
		return -1;
	}

	(*env)->DeleteLocalRef(env, jmsgid); 
	(*env)->DeleteLocalRef(env, jcorrid); 

	char *str = (char*)(*env)->GetByteArrayElements(env, text, JNI_FALSE);
	jsize size = (*env)->GetArrayLength(env, text);
	memcpy(cmessage, str, size);
	*len = size;

	(*env)->ReleaseByteArrayElements(env, text, str, JNI_COMMIT);
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

int main()
{
	int ret = 0;
	int port = 3001;
	char *ip = "192.168.2.3";
	char *qmgr = "QMA";
	int ccsid = 819;
	char *squeue = "LQ_1";
	char message[1024];
	size_t len = 0;

	ret =  mq_javainit(ip, port, qmgr, NULL, ccsid);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mq_javainit error.", __FILE__, __LINE__);
		return -1;
	}

	ret =  mq_javaopen(squeue, squeue, 0, O_SEND);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mq_java send error.", __FILE__, __LINE__);
		return -1;
	}	
	
	ret =  mq_javaopen(squeue, squeue, 0, O_RECV);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mq_java recv error.", __FILE__, __LINE__);
		return -1;
	}	

	int i = 0;
	
	while(1)
	{
		i++;
		if (i > 1)
		{
			break;
		}
	memset(message, 0x00, sizeof(message));
	memcpy(message, "11111111111111111cÎÒµÄ¼ÎÊ¢1fdwdjwkldddddddddddddddddddd", sizeof(message) - 1);
	len = strlen(message);
	message[26] = '\0';

	pub_log_info("[%s][%d] code g2u before message=[%s]len=[%d]", __FILE__, __LINE__, message, len);
	pub_log_bin(SW_LOG_INFO, message, len, "[%s][%d] len=[%ld]", __FILE__, __LINE__, len);

	ret = mq_javasend(message,len, NULL, NULL, 1000, 0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] mq java send msg error.", __FILE__, __LINE__);
		return -1;
	}
	len = 0;	
	memset(message, 0x00, sizeof(message));	
	ret = mq_javarecv(0, message, NULL, NULL, &len);
	if (ret < 0)
	{
		if (ret == SW_NO_MSG)
		{
			pub_log_info("[%s][%d] no have msg..", __FILE__, __LINE__);
			continue;
		}
		pub_log_error("[%s][%d] mq java recv msg error.", __FILE__, __LINE__);
		return -1;
		
	}
	pub_log_info("[%s][%d] recv java msg:[%s]len=[%d]",__FILE__, __LINE__,message, len);
	}

	mq_javadestroy();
	return 0;
}
