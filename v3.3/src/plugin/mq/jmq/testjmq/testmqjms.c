#include <jni.h>
#include <stdio.h>
#include <stdlib.h>  
#include <unistd.h>
#include <string.h>
#include "mq_pub.h"

#if 0
typedef struct
{
	jobject	obj;
	jclass	cls;
	JavaVM	*jvm;
	JNIEnv	*env;
	jmethodID	send_mid;
	jmethodID	recv_mid;
} mqjms_t;

static mqjms_t	g_mqjms;

#define O_CNT		5
#define O_SEND	0
#define O_RECV	1

int mq_jmsinit(char *pip,int iport, char *pqmgr, char *csendqueue, char *crecvqueue, int flag)
{
	printf("ip[%s] port[%d] qmgr[%s]\n", pip,iport, pqmgr);	
	int	ret = 0;
	static jobject	obj;
	static jclass	cls;
	static JavaVM	*jvm;
	static JNIEnv	*env;
	jmethodID	mid;
	JavaVMInitArgs	vm_args;
	JavaVMOption	options[3];

	options[0].optionString = "-Djava.compiler=NONE";  
	char classpath[1024] = "-Djava.class.path=";
	char *env_classpath = getenv("CLASSPATH");
	if (env_classpath)
	{
		options[1].optionString = strcat(classpath, env_classpath);  
	}
	
	fprintf(stdout, "vm_args init....\n");
	vm_args.version = JNI_VERSION_1_6;  
	vm_args.nOptions = 2;  
	vm_args.options = options;  
	vm_args.ignoreUnrecognized = JNI_TRUE;  

	fprintf(stdout, "ready create JVM\n");
	ret = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);  
	if (ret < 0)  
	{  
		fprintf(stderr, "Can't create Java VM\n");  
		return -1;
	}

	fprintf(stdout, "ready find class\n");
	cls = (*env)->FindClass(env, "mqjms/MqJms");
	if (!cls)
	{
		fprintf(stderr, "Can't found class mqJms\n");
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	fprintf(stderr, "Find class success!\n");

	mid = (*env)->GetMethodID(env, cls, "<init>", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");  
	if (!mid)
	{
		fprintf(stderr, "Can't found Constructor method!\n");
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	fprintf(stderr, "found constructor method!\n");

	jint port = iport;
	jstring ip = (*env)->NewStringUTF(env, pip);
	jstring qmgr = (*env)->NewStringUTF(env, pqmgr);
	jstring jSendQueue = NULL;
	jstring jRecvQueue = NULL;

	fprintf(stdout, "ip[%s] port[%d] qmgr[%s]\n", pip,iport, pqmgr);	
	if (csendqueue != NULL)
	{
		jSendQueue = (*env)->NewStringUTF(env, csendqueue);
	}

	if (crecvqueue != NULL)
	{
		jRecvQueue = (*env)->NewStringUTF(env, crecvqueue);
	}

	printf("ready create New Object....\n");
	obj = (*env)->NewObject(env, cls, mid, port, ip, qmgr, jSendQueue, jRecvQueue);
	if (!obj)
	{
		fprintf(stderr, "[%s][%d] NewObject error!\n", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	fprintf(stderr, "[%s][%d] New object success!\n", __FILE__, __LINE__);

	mid = (*env)->GetMethodID(env, cls, "init", "()I");  
	if (!mid)
	{
		fprintf(stderr, "[%s][%d] Get init method error!\n", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	fprintf(stderr, "[%s[%d] Get init method success!\n", __FILE__, __LINE__);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid);  
	if (ret < 0)
	{
		fprintf(stderr, "[%s][%d] Call init method error!\n", __FILE__, __LINE__);
		(*jvm)->DestroyJavaVM(jvm);  
		return -1;
	}
	fprintf(stderr, "[%s[%d] Call init method success!\n", __FILE__, __LINE__);

	memset(&g_mqjms, 0x0, sizeof(g_mqjms));
	g_mqjms.env = env;
	g_mqjms.cls = cls;
	g_mqjms.obj = obj;

	if (flag != O_RECV)
	{
		mid = (*env)->GetMethodID(env, cls, "initSend", "()I");  
		if (!mid)
		{
			fprintf(stderr, "[%s][%d] Get initSend method error!\n", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		fprintf(stderr, "[%s[%d] Get initSend method success!\n", __FILE__, __LINE__);

		ret = (jint)(*env)->CallIntMethod(env, obj, mid);  
		if (ret < 0)
		{
			fprintf(stderr, "[%s][%d] Call initSend method error!\n", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		fprintf(stderr, "[%s][%d] Call initSend method success!\n", __FILE__, __LINE__);

		mid = (*env)->GetMethodID(env, cls, "sendMessage","(Ljava/lang/String;)I");  
		if (!mid)
		{
			fprintf(stderr, "Can't found method sendMessage!\n");
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		g_mqjms.send_mid = mid;
		fprintf(stderr, "[%s][%d] Get sendMessage method success!\n", __FILE__, __LINE__);
	}
	
	if (flag != O_SEND)
	{
		mid = (*env)->GetMethodID(env, cls, "initRecv", "()I");  
		if (!mid)
		{
			fprintf(stderr, "[%s][%d] Get initRecv method error!\n", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		fprintf(stderr, "[%s[%d] Get initRecv method success!\n", __FILE__, __LINE__);

		ret = (jint)(*env)->CallIntMethod(env, obj, mid);  
		if (ret < 0)
		{
			fprintf(stderr, "[%s][%d] Call initRecv method error!\n", __FILE__, __LINE__);
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		fprintf(stderr, "[%s][%d] Call initRecv method success!\n", __FILE__, __LINE__);
		mid = (*env)->GetMethodID(env, cls, "recvMessage","(I)Ljava/lang/String;");  
		if (!mid)
		{
			fprintf(stderr, "Can't found method recvMessage!\n");
			(*jvm)->DestroyJavaVM(jvm);  
			return -1;
		}
		g_mqjms.recv_mid = mid;
		fprintf(stderr, "[%s][%d] Get recvMessage method success!\n", __FILE__, __LINE__);
	}
	fprintf(stderr, "[%s][%d] Jms init success!\n", __FILE__, __LINE__);
	
	return 0;
}

int mq_jmssend(char *cmessage)
{
	jint	ret = 0;
	jobject	obj;
	JNIEnv	*env;
	jmethodID	mid;
	jstring message;
	
	env = g_mqjms.env;
	obj = g_mqjms.obj;
	mid = g_mqjms.send_mid;
	message = (*env)->NewStringUTF(env, cmessage);

	ret = (jint)(*env)->CallIntMethod(env, obj, mid, message);  
	if (ret < 0)
	{
		fprintf(stderr, "[%s][%d] Send message error, ret=[%d]\n", __FILE__, __LINE__, ret);
		return -1;
	}

	return 0;
}

int mq_jmsrecv(int timeout, char *cmessage, long *len)
{
	int	ret = 0;
	long	rlen = 0;
	jobject	obj;
	JNIEnv	*env;
	jstring	text;
	jint	jtimeout = timeout;
	jmethodID	mid;
	
	env = g_mqjms.env;
	obj = g_mqjms.obj;
	mid = g_mqjms.recv_mid;

	text = (jstring)(*env)->CallObjectMethod(env, obj, mid, jtimeout);
	if (text == NULL)
	{
		fprintf(stderr, "Recv message error!\n");
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
	JavaVM	*jvm = g_mqjms.jvm;

	(*jvm)->DestroyJavaVM(jvm);
	
	fprintf(stderr, "[%s][%d] DestroyJavaVM!", __FILE__, __LINE__);
	
	return 0;
}
#endif

#define O_SEND_RECV 2
msg_format_enum_t g_msg_format;

int main(int argc, char **argv)
{
	int	i = 0;
	int	ret = 0;
	int	onlysend = 1;
	int	onlyrecv = 1;
	long	len = 0;
	char	*jsendqueue = "LQ_1";
	char 	*qmgr = "QMA";
	char	message[1024];
	int	port = 3001;
	char ip[128]="192.168.2.3";

	if (argc == 2)
	{
		if (strcmp(argv[1], "send") == 0)
		{
			onlyrecv = 0;
		}

		if (strcmp(argv[1], "recv") == 0)
		{
			onlysend = 0;
		}
	}
	else
	{
		onlysend = 0;
		onlyrecv = 0;
	}

	if (onlysend)
	{
		ret = mq_jmsinit(ip, port, qmgr, jsendqueue, NULL, O_SEND);
	}
	else if (onlyrecv)
	{
		ret = mq_jmsinit(ip, port, qmgr, NULL,jsendqueue,  O_RECV);
	}
	else
	{
		ret = mq_jmsinit(ip, port, qmgr,jsendqueue, jsendqueue,  O_SEND_RECV);
	}
	if (ret < 0)
	{
		fprintf(stderr, "[%s][%d] jms init error!\n", __FILE__, __LINE__);
		exit(1);
	}

	if (onlysend)
	{
		i = 0;
		memset(message, 0x0, sizeof(message));
		while(1)
		{
			i++;
			sprintf(message, "jni jms mqifo %d message", i);
			ret = mq_jmssend(message);
			if (ret < 0)
			{
				printf("[%s][%d] Send message error!\n", __FILE__, __LINE__);
				return -1;
			}
			printf("[%s][%d] Send message success!\n", __FILE__, __LINE__);
		}
	}

	if (onlyrecv)
	{
		while(1)
		{
			len = 0;
			memset(message, 0x0, sizeof(message));
			ret = mq_jmsrecv(4000, message, &len);
			if (ret < 0)
			{
				printf("[%s][%d] recv message error!\n", __FILE__, __LINE__);
				return -1;
			}
			printf("[%s][%d] Recv message [%s][%d] success!\n", __FILE__, __LINE__, message, len);
		}
	}
	return 0;
}


