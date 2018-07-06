#include "common.h"
#include "pub_type.h"
#include "pub_buf.h"

/******************************************************************************
 **��������: Mkdir
 **��    ��: �����ļ���·��
 **�������: 
 **     dir: �ļ���·��
 **     mode: Ȩֵ
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
int Mkdir(const char *dir, mode_t mode)
{
    int ret = 0, len = 0;
    const char *p = dir;
    struct stat file_stat;
    char part[FILE_PATH_MAX_LEN] = {0};

    memset(&file_stat, 0, sizeof(file_stat));

    ret = stat(dir, &file_stat);
    if(0 == ret)
    {
        if(S_ISDIR(file_stat.st_mode))
        {
            return 0;
        }
        return -1;  /* Exist, but not directory */
    }

    p = strchr(p, '/');
    while(NULL != p)
    {
        len = p - dir + 1;
        memcpy(part, dir, len);

        ret = stat(part, &file_stat);
        if(0 == ret)
        {
            if(S_ISDIR(file_stat.st_mode))
            {
                p++;
                p = strchr(p, '/');
                continue;
            }
            return -1;  /* Exist, but not directory */
        }
        
        ret = mkdir(part, mode);
        if(0 != ret)
        {
            if(EEXIST != errno)
            {
                return -1;
            }
            /* Exist, then continue */
        }
        p++;
        p = strchr(p, '/');
    }

    mkdir(dir, mode);
    return 0;
}

/******************************************************************************
 **��������: Mkdir2
 **��    ��: �����ļ���·��(ע��: ·���д����ļ���)
 **�������: 
 **     fname: �ļ���·��
 **     mode: Ȩֵ
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **     ��: fname="/home/oracle/etc/lsn.log"ִ�к󴴽���·��Ϊ"/home/oracle/etc/"
 **��    ��: # Qifeng.zou # 2013.11.08 #
 ******************************************************************************/
int Mkdir2(const char *fname, mode_t mode)
{
    const char *p = fname;
    char dir[FILE_PATH_MAX_LEN] = {0};

    p += strlen(fname);

    p = strrchr(fname, '/');
    if(NULL == p)
    {
        return 0;   /* �ڵ�ǰ·���£���˲��ش���·�� */
    }

    /* ȥ���ļ�����ֻ�����ļ�·�� */
    memcpy(dir, fname, p - fname);

    return Mkdir(dir, mode);
}

/******************************************************************************
 **��������: Hash
 **��    ��: ����Hashֵ
 **�������: 
 **     str: ��Ҫ��������ִ�
 **�������: NONE
 **��    ��: hashֵ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
unsigned int Hash(const char *str)
{
    const char *p = str;
    unsigned int hash = 5381;

    while(*p)
    {
        hash += (hash << 5) + (*p++);
    }

    return (hash & 0x7FFFFFFF);
}

/******************************************************************************
 **��������: file_try_lock
 **��    ��: ���Լ���
 **�������: 
 **     fd: �ļ�������
 **     type: ������(��:F_RDLCK, д:F_WRLCK, ����:F_UNLCK)
 **     whence: ���λ��(SEEK_SET, CURR_SET, END_SET)
 **     start: ��ʵλ��
 **     len: ��������
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **     F_SETLK �����ļ�������״̬��
 **         ��ʱflcok �ṹ��l_type ֵ������F_RDLCK��F_WRLCK��F_UNLCK��
 **         ����޷������������򷵻�-1���������ΪEACCES ��EAGAIN��
 **��    ��: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
int file_try_lock(int fd, int type, int whence, int start, int len)
{
    struct flock arg;

    arg.l_type = type;
    arg.l_whence = whence;
    arg.l_start = start;
    arg.l_len = len;
    
    return fcntl(fd, F_SETLK, &arg);
}

/******************************************************************************
 **��������: lock
 **��    ��: ������д��
 **�������: 
 **     fd: �ļ�������
 **     type: ������(��:F_RDLCK, д:F_WRLCK, ����:F_UNLCK)
 **     whence: ���λ��(SEEK_SET, CURR_SET, END_SET)
 **     offset: ƫ��λ��
 **     len: ��������
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **     F_SETLKW ��F_SETLK�������ƣ�
 **         �����޷���������ʱ���˵��û�һֱ�ȵ����������ɹ�Ϊֹ��
 **         ���ڵȴ������Ĺ����б��ź��ж�ʱ������������-1���������ΪEINTR��
 **��    ��: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
int file_lock(int fd, int type, int whence, int offset, int len)
{
    struct flock arg;

    arg.l_type = type;
    arg.l_whence = whence;
    arg.l_start = offset;
    arg.l_len = len;
    
    return fcntl(fd, F_SETLKW, &arg);
}

/******************************************************************************
 **��������: Readn
 **��    ��: ��ȡN���ֽ�
 **�������: 
 **     fd: �ļ�������
 **     n: ��ȡ�ֽڸ���
 **�������: 
 **     buff: ����
 **��    ��: ��ȡ�ֽڸ���
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.16 #
 ******************************************************************************/
ssize_t Readn(int fd, void *buff, size_t n)
{
	int	len = 0;
	size_t	left = n;
	char	*ptr = buff;

	while(left > 0)
	{
		len = read(fd, ptr, left);
		if(len < 0)
		{
			if(EINTR == errno)
			{
				continue;
			}
			return -1;
		}
		else if(0 == len)
		{
			break;
		}

		left -= len;
		ptr += len;
	}

	return (n - left);
}

/******************************************************************************
 **��������: Writen
 **��    ��: д��N���ֽ�
 **�������: 
 **     fd: �ļ�������
 **     buff: ����
 **     n: д�볤��
 **�������: NONE
 **��    ��: д���ֽڸ���
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.16 #
 ******************************************************************************/
ssize_t Writen(int fd, const void *buff, size_t n)
{
	int	len = 0;
	size_t	left = n;
	const char	*ptr = buff;

	while(left > 0)
	{
		len = write(fd, ptr, left);
		if(len < 0)
		{
			if(EINTR == errno)
			{
				continue;
			}
			return -1;
		}

		left -= len;
		ptr += len;
	}

	return n;
}

/******************************************************************************
 **��������: Open
 **��    ��: ���ļ�
 **�������: 
 **     fpath: �ļ�·��
 **     flags: �򿪱�ʶ
 **     mode: Ȩ������
 **�������: NONE
 **��    ��: �ļ�������
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.16 #
 ******************************************************************************/
int Open(const char *fpath, int flags, mode_t mode)
{
    int fd = 0;
    
AGAIN:
    fd = open(fpath, flags, mode);
    if(fd < 0)
    {
        if(EINTR == errno)
        {
            goto AGAIN;
        }
        return -1;
    }
    return fd;        
}

/******************************************************************************
 **��������: Sleep
 **��    ��: ˯��ָ��ʱ��
 **�������: 
 **     seconds: ��
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.25 #
 ******************************************************************************/
int Sleep(int seconds)
{
    int left = 0;

    left = seconds;
    do
    {
        left = sleep(left);
    }while(left > 0);

    return 0;
}

/******************************************************************************
 **��������: Rename
 **��    ��: �������ļ�
 **�������: 
 **     oldpath: ԭ�ļ���
 **     newpath: ���ļ���
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.31 #
 ******************************************************************************/
int Rename(const char *oldpath, const char *newpath)
{
    int ret = 0;

AGAIN:
    ret = rename(oldpath, newpath);
    if(ret < 0)
    {
        if(EINTR == errno)
        {
            goto AGAIN;
        }
        return -1;
    }
        return 0;
}

/******************************************************************************
 **��������: Msgrcv
 **��    ��: ������Ϣ���е���Ϣ
 **�������: 
 **     seconds: ��
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.25 #
 ******************************************************************************/
int Msgrcv(int msqid, msgbuf_t *msgbuf, size_t msgsz, int msgtype, int flag)
{
    int ret = 0;

AGAIN:
    ret = msgrcv(msqid, msgbuf, msgsz, msgtype, flag);
    if(ret < 0)
    {
        if(EINTR == errno)
        {
            goto AGAIN;
        }
        return -1;
    }
    return 0;
}

/******************************************************************************
 **��������: Msgsnd
 **��    ��: ������Ϣ���е���Ϣ
 **�������: 
 **     seconds: ��
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.10.25 #
 ******************************************************************************/
int Msgsnd(int msqid, msgbuf_t *msgbuf, size_t msgsz, int flag)
{
    int ret = 0;

AGAIN:
    ret = msgsnd(msqid, msgbuf, msgsz, flag);
    if(ret < 0)
    {
        if(EINTR == errno)
        {
            goto AGAIN;
        }
        return -1;
    }
    return 0;
}

/******************************************************************************
 **��������: daemon_init
 **��    ��: �����������
 **�������: 
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.11.06 #
 ******************************************************************************/
int daemon_init(void)
{
    pid_t pid = 0;

    pid = fork();
    if(pid < 0)
    {
        return -1;
    }
    else if(0 != pid)
    {
        exit(0);
    }

    setsid();
    umask(0);
    return 0;
}

/******************************************************************************
 **��������: System
 **��    ��: ִ��ϵͳ����
 **�������: 
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.11.06 #
 ******************************************************************************/
int System(const char *cmd)
{ 
    int status = -1;
    
    status = system(cmd);
    if(-1 == status)
    {
        return -1;
    }

    if(WIFEXITED(status))
    {
        if(0 == WEXITSTATUS(status))
        {
            return WEXITSTATUS(status);
        }
        else
        {
            return WEXITSTATUS(status);
        }
    }

    return WEXITSTATUS(status);
}

/* ���ļ����ݶ��뻺�� # Maweiwei # 2013.11.11 */
int readfile(const char *filename, sw_buf_t *readbuf)
{
    int ret = 0;
    FILE *fp = NULL;
    struct stat buff;

    if (filename == NULL || filename[0] == '\0' || readbuf == NULL)
    {
        pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
        return -1;
    }
    
    memset(&buff, 0x0, sizeof(buff));
    ret = stat(filename, &buff);
    if (ret < 0)
    {
        pub_log_error("[%s][%d] stat [%s] error! errno=[%d]:[%s]",
            __FILE__, __LINE__, filename, errno, strerror(errno));
        return -1;
    }
    
    if (readbuf->size < buff.st_size)
    {
        ret = pub_buf_update(readbuf, buff.st_size + 1);
        if (ret < 0)
        {
            pub_log_error("[%s][%d] update buf size error! size=[%d]", 
                __FILE__, __LINE__, buff.st_size);
            return -1;
        }
    }
    
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
            __FILE__, __LINE__, filename, errno, strerror(errno));
    }
#if (SW_DES_FILE)
	unsigned char *ptr = NULL;
    ptr = (unsigned char *)calloc(1, buff.st_size + 1);
    if (ptr == NULL)
    {
        fclose(fp);
        pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]", 
            __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }
    fread(ptr, buff.st_size, 1, fp);
    des3_dec(ptr, readbuf->data, buff.st_size);
    free(ptr);
#else
    fread(readbuf->data, buff.st_size, 1, fp);
#endif
    readbuf->len = buff.st_size;
    fclose(fp);
    
    return 0;
}

/* �������������� */
static char g_EnvTable[ENV_TOTAL][ENV_MAX_LEN];

char (*g_EnvTablePtr)[ENV_MAX_LEN] = g_EnvTable;

/******************************************************************************
 **��������: SetEnv
 **��    ��: ���û���������ֵ
 **�������: 
 **     type: �����������ͣ� ȡֵ��Χ:ENV_SWWORK ~ ENV_UNKNOWN.(env_table_e)
 **     value: ����ֵ
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.11.20 #
 ******************************************************************************/
int SetEnv(int type, const char *value)
{
    if((type < 0) || (type >= ENV_UNKNOWN))
    {
        return -1;
    }
	
    snprintf(g_EnvTable[type], sizeof(g_EnvTable[type]), "%s", value);
    return 0;
}

/******************************************************************************
 **��������: InitEnv
 **��    ��: ��ʼ����������
 **�������: NONE
 **�������: NONE
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     �������ø�����������ֵ
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.11.20 #
 ******************************************************************************/
void InitEnv(void)
{
    SetWorkDir();
    SetHomeDir();
}
