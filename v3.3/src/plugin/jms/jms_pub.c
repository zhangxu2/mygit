#include "lsn_pub.h"
#include "jms_pub.h"

#ifdef __SW_USE_JMS__

int jms_jmsinit(char *cjndifactory, char *curl, char *cconnfactory, char *csendqueue, char *crecvqueue, int flag)
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

	cls = (*env)->FindClass(env, "jmsbase/JMSBase");
	if (!cls)
	{
		pub_log_error("[%s][%d] Can't found class JMSBase!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	pub_log_info("[%s][%d] Find class success!", __FILE__, __LINE__);

	mid = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");  
	if (!mid)
	{
		pub_log_info("[%s][%d] Can't found Constructor method!", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);
		return -1;
	}
	pub_log_info("[%s][%d] Found constructor method!", __FILE__, __LINE__);

	jstring jndifactory = (*env)->NewStringUTF(env, cjndifactory);
	jstring url = (*env)->NewStringUTF(env, curl);
	jstring factory = (*env)->NewStringUTF(env, cconnfactory);
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

	obj = (*env)->NewObject(env, cls, mid, jndifactory, url, factory, jSendQueue, jRecvQueue);
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

	memset(&g_jmscontext, 0x0, sizeof(g_jmscontext));
	g_jmscontext.env = env;
	g_jmscontext.cls = cls;
	g_jmscontext.obj = obj;

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

		mid = (*env)->GetMethodID(env, cls, "sendBytesMessage","(Ljava/lang/String;)I");  
		if (!mid)
		{
			pub_log_error("[%s][%d] Can't found method sendMessage!", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		g_jmscontext.send_mid = mid;
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
		g_jmscontext.recv_mid = mid;
		pub_log_info("[%s][%d] Get recvMessage method success!", __FILE__, __LINE__);
	}
	pub_log_info("[%s][%d] Jmsinit success!", __FILE__, __LINE__);
	first = 0;
	
	return 0;
}

int jms_jmssend(char *cmessage)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jmethodID	mid;
	jstring message;
	
	env = g_jmscontext.env;
	obj = g_jmscontext.obj;
	mid = g_jmscontext.send_mid;
	message = (*env)->NewStringUTF(env, cmessage);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid, message);  
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Send message error, ret=[%d]", __FILE__, __LINE__, ret);
		return -1;
	}

	return 0;
}

int jms_jmsrecv(int timeout, char *cmessage, long *len)
{
	long	begin = 0;
	long	end = 0;
	jobject	obj;
	JNIEnv	*env;
	jint	jtimeout = timeout;
	jstring	text;
	jmethodID	mid;
	
	env = g_jmscontext.env;
	obj = g_jmscontext.obj;
	mid = g_jmscontext.recv_mid;

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

int jms_jmsdestroy()
{
	JavaVM	*jvm = g_jmscontext.jvm;

	(*jvm)->DestroyJavaVM(jvm);
	
	pub_log_info("[%s][%d] Destroy JavaVM!", __FILE__, __LINE__);
	
	return 0;
}

#endif

