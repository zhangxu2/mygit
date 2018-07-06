#include "pub_msq.h"
#include "pub_mem.h"

/******************************************************************************
 ** Name : pub_msq_check_size
 ** Desc : �����Ϣ���еĳߴ�
 ** Input: 
 ** 	msqid: ��Ϣ����ID
 **		size: �ߴ�
 ** Output: NONE
 ** Return: 0:���ɹ� !0:���ʧ��
 ** Process:
 ** Note :
 ** Author: # Liteng & Qifeng.zou # 2012.08.01 #
 ******************************************************************************/
sw_int_t pub_msq_check_size(int msqid, int size)
{
	int ret=0, lvl=0;
	struct msqid_ds msqds;

	memset(&msqds, 0, sizeof(msqds));

	ret = msgctl(msqid, IPC_STAT, &msqds);
	if(ret < 0) 
	{
		pub_log_error("%s,%d, msgctl error! errno==[%d]",__FILE__,__LINE__,errno);
		return(SW_ERR);
	}

	lvl=((((msqds.msg_cbytes+size)*1000)/msqds.msg_qbytes+5)/10);
	if (lvl >= IPC_QU_THRESHOLD)
	{
		return SW_ERR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : pub_msq_open
 ** Desc : ͨ��KEY����Ϣ����
 ** Input: 
 ** 	key: ��Ϣ����KEY
 ** Output: NONE
 ** Return: ��Ϣ����ID
 ** Process:
 ** Note :
 ** Author: # Liteng & Qifeng.zou # 2012.08.01 #
 ******************************************************************************/
sw_int_t pub_msq_open(key_t key)
{
	int msqid = -1;


	msqid = msgget(key, 0);
	if(msqid < 0)
	{
		if(ENOENT != errno)
		{
			pub_log_error("[%s][%d],errno=[%s]\n",__FILE__,__LINE__,strerror(errno));
			return SW_ERR;
		}
		return SW_ERR;
	}

	return msqid;
}

/******************************************************************************
 ** Name : pub_msq_creat
 ** Desc : ������Ϣ���У�����Ѵ��ڣ����
 ** Input: 
 ** 	key: ��Ϣ����KEY
 **		size: �ߴ�
 ** Output: NONE
 ** Return: >0:��Ϣ����ID  <0: ʧ��
 ** Process:
 ** Note :
 ** Author: # Liteng # 2012.08.01 #
 ** Modify: # Qifeng.zou # 2013.07.03 #
 ******************************************************************************/
sw_int_t pub_msq_creat(key_t key, int size)
{
	int ret=0, msqid=0;
	struct msqid_ds msqds;

	memset(&msqds, 0, sizeof(msqds));

	/* 1. Create MSQ if not exist */
	msqid = msgget(key, IPC_CREAT|IPC_EXCL|0666);
	if(-1 == msqid)
	{
		/* MSQ exist, Open it */
		msqid = msgget(key, 0);
		if(msqid < 0) 
		{
			pub_log_error("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
			return SW_ERR;
		}
	}
	else if(msqid < 0) 
	{
		pub_log_error("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	/*����������ʱ�����Ƿ���ռ��С*/
	if(size > 0)
	{
		msqds.msg_perm.mode = 000666;
		msqds.msg_perm.uid = getuid();
		msqds.msg_perm.cuid = getuid();
		msqds.msg_perm.gid = getgid();
		msqds.msg_qbytes = size;
		ret = msgctl(msqid, IPC_SET, &msqds);
		if(ret < 0) 
		{
			return SW_ERR;
		}
	}
	return msqid;
}

/******************************************************************************
 ** Name : pub_msq_clear
 ** Desc : ������Ϣ�����е����ݣ��������������д��ָ���ļ�
 ** Input: 
 ** 	msqid: ��Ϣ����ID
 **		file: �������ݴ����ָ�����ļ�
 ** Output: NONE
 ** Return: >0:��Ϣ����ID  <0: ʧ��
 ** Process:
 ** Note :
 ** Author: # Liteng & Qifeng.zou # 2012.08.01 #
 ******************************************************************************/
sw_int_t pub_msq_clear(int msqid, long mtype, const char *file)
{
	FILE *fp = NULL;
	int ret = 0, len = 0;
	char buf[QMSG_MAX_LEN+1] = {0};

	/* clear msg queue */
	fp = fopen(file,"ab");
	if(NULL == fp)
	{
		return SW_ERR;
	}

	do
	{
		ret = pub_msq_get(msqid, buf, &mtype, &len);
		if (ret < 0 || len <=0)
		{
			break;
		}
		fprintf(fp, "\n msqid:[%d] mtype:[%ld] len:[%d]\n", msqid, mtype, len);
		fwrite(buf, len, 1, fp);
	}while(1);

	fclose(fp), fp=NULL;

	return SW_OK;
}

/******************************************************************************
 ** Name : pub_msq_rm
 ** Desc : ɾ����Ϣ����
 ** Input: 
 ** 	msqid: ��Ϣ����ID
 ** Output: NONE
 ** Return: 0:�ɹ�  -1:ʧ��
 ** Process:
 ** Note :
 ** Author: # Liteng & Qifeng.zou # 2010.08.16 #
 ******************************************************************************/
sw_int_t pub_msq_rm(int msqid)
{
	return msgctl(msqid, IPC_RMID, 0);
}

/******************************************************************************
 ** Name : pub_msq_get
 ** Desc : �ӹ�����Ϣ������ȡһ������
 ** Input: 
 ** 	msqid: ��Ϣ����ID
 ** Output: 
 **		buf: ��ȡ����Ϣ����
 **		mtype: Mtypeֵ
 **		len: ���ݳ���
 ** Return: 0:�ɹ�  -1:ʧ��
 ** Process:
 ** Note :
 ** Author: # Liteng & Qifeng.zou # 2010.08.01 #
 ******************************************************************************/
sw_int_t pub_msq_get(int mqid, char *buf, long *mtype,int *len)
{
	int ret = 0;
	sw_qmsg_t st;

	pub_mem_memset(&st, 0, sizeof(st));

	if(NULL == buf)
	{
		pub_log_error("[%s][%d] Buffer is NULL!", __FILE__, __LINE__);
		return SW_ERR;
	}

	ret = msgrcv(mqid, &st, QMSG_MAX_LEN, *mtype, IPC_NOWAIT);
	if(ret < 0)
	{
		if(EINTR == errno)
		{
			return SW_ABORT;
		}
		else if (errno==ENOMSG)
		{
			buf[0] = '\0';
			*len = 0;
			return SW_OK;
		}
		pub_log_error("[%s][%d] errmsg:[%d][%s]", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	*mtype = st.mtype;
	pub_mem_memcpy(buf, st.data, ret);
	buf[ret]='\0';
	*len=ret;

	return SW_OK;
}


/******************************************************************************
 ** Name : pub_msq_put
 ** Desc : д��Ϣ����������
 ** Input: 
 ** 	msqid: ��Ϣ����ID
 **		buf: ��Ϣ����
 **		mtype: Mtypeֵ
 **		len: ���ݳ���
 ** Output: NONE
 ** Return: 0:�ɹ�  -1:ʧ��
 ** Process:
 ** Note :
 ** Author: # Liteng & Qifeng.zou # 2010.08.01 #
 ******************************************************************************/
sw_int_t pub_msq_put(int msqid, const char *buf, int len, int mtype)
{
	int	ret = 0;
	sw_qmsg_t	st;

	pub_mem_memset(&st, 0, sizeof(st));

	if(msqid < 0 || len < 0 || NULL == buf || mtype < 0)
	{
		pub_log_error("Paramter is incorrect! msqid:[%d] mtype:[%d]",
				__FILE__, __LINE__, msqid, mtype);
		return SW_ERR;
	}

	if(len > QMSG_MAX_LEN)
	{
		pub_log_error("Msg is too long! len:[%d]/[%d]", len, QMSG_MAX_LEN);
		return SW_ERR;	
	}

	pub_mem_memcpy(st.data, buf, len);
	st.mtype = mtype;

	ret = msgsnd(msqid, &st, len, IPC_NOWAIT);
	if(ret < 0)
	{
		pub_log_error("[%s][%d] errmsg:[%d]%s", __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}

	return SW_OK;
}
