#include "common.h"
#include "pub_type.h"
#include "pub_buf.h"

/******************************************************************************
 **函数名称: Mkdir
 **功    能: 创建文件夹路径
 **输入参数: 
 **     dir: 文件夹路径
 **     mode: 权值
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.06 #
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
 **函数名称: Mkdir2
 **功    能: 创建文件夹路径(注意: 路径中存在文件名)
 **输入参数: 
 **     fname: 文件夹路径
 **     mode: 权值
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **     如: fname="/home/oracle/etc/lsn.log"执行后创建的路径为"/home/oracle/etc/"
 **作    者: # Qifeng.zou # 2013.11.08 #
 ******************************************************************************/
int Mkdir2(const char *fname, mode_t mode)
{
    const char *p = fname;
    char dir[FILE_PATH_MAX_LEN] = {0};

    p += strlen(fname);

    p = strrchr(fname, '/');
    if(NULL == p)
    {
        return 0;   /* 在当前路径下，因此不必创建路径 */
    }

    /* 去除文件名，只保留文件路径 */
    memcpy(dir, fname, p - fname);

    return Mkdir(dir, mode);
}

/******************************************************************************
 **函数名称: Hash
 **功    能: 计算Hash值
 **输入参数: 
 **     str: 需要被处理的字串
 **输出参数: NONE
 **返    回: hash值
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.27 #
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
 **函数名称: file_try_lock
 **功    能: 尝试加锁
 **输入参数: 
 **     fd: 文件描述符
 **     type: 锁类型(读:F_RDLCK, 写:F_WRLCK, 解锁:F_UNLCK)
 **     whence: 相对位置(SEEK_SET, CURR_SET, END_SET)
 **     start: 其实位置
 **     len: 加锁长度
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **     F_SETLK 设置文件锁定的状态。
 **         此时flcok 结构的l_type 值必须是F_RDLCK、F_WRLCK或F_UNLCK。
 **         如果无法建立锁定，则返回-1，错误代码为EACCES 或EAGAIN。
 **作    者: # Qifeng.zou # 2013.09.06 #
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
 **函数名称: lock
 **功    能: 阻塞加写锁
 **输入参数: 
 **     fd: 文件描述符
 **     type: 锁类型(读:F_RDLCK, 写:F_WRLCK, 解锁:F_UNLCK)
 **     whence: 相对位置(SEEK_SET, CURR_SET, END_SET)
 **     offset: 偏移位置
 **     len: 加锁长度
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **     F_SETLKW 与F_SETLK作用相似，
 **         但是无法建立锁定时，此调用会一直等到锁定动作成功为止。
 **         若在等待锁定的过程中被信号中断时，会立即返回-1，错误代码为EINTR。
 **作    者: # Qifeng.zou # 2013.09.06 #
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
 **函数名称: Readn
 **功    能: 读取N个字节
 **输入参数: 
 **     fd: 文件描述符
 **     n: 读取字节个数
 **输出参数: 
 **     buff: 缓存
 **返    回: 读取字节个数
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.16 #
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
 **函数名称: Writen
 **功    能: 写入N个字节
 **输入参数: 
 **     fd: 文件描述符
 **     buff: 缓存
 **     n: 写入长度
 **输出参数: NONE
 **返    回: 写入字节个数
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.16 #
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
 **函数名称: Open
 **功    能: 打开文件
 **输入参数: 
 **     fpath: 文件路径
 **     flags: 打开标识
 **     mode: 权限设置
 **输出参数: NONE
 **返    回: 文件描述符
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.16 #
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
 **函数名称: Sleep
 **功    能: 睡眠指定时间
 **输入参数: 
 **     seconds: 秒
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.25 #
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
 **函数名称: Rename
 **功    能: 重命名文件
 **输入参数: 
 **     oldpath: 原文件名
 **     newpath: 新文件名
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.31 #
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
 **函数名称: Msgrcv
 **功    能: 接收消息队列的信息
 **输入参数: 
 **     seconds: 秒
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.25 #
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
 **函数名称: Msgsnd
 **功    能: 发送消息队列的信息
 **输入参数: 
 **     seconds: 秒
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.10.25 #
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
 **函数名称: daemon_init
 **功    能: 启动精灵进程
 **输入参数: 
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.11.06 #
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
 **函数名称: System
 **功    能: 执行系统命令
 **输入参数: 
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.11.06 #
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

/* 将文件内容读入缓存 # Maweiwei # 2013.11.11 */
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

/* 环境变量参数表 */
static char g_EnvTable[ENV_TOTAL][ENV_MAX_LEN];

char (*g_EnvTablePtr)[ENV_MAX_LEN] = g_EnvTable;

/******************************************************************************
 **函数名称: SetEnv
 **功    能: 设置环境变量的值
 **输入参数: 
 **     type: 环境变量类型， 取值范围:ENV_SWWORK ~ ENV_UNKNOWN.(env_table_e)
 **     value: 变量值
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.11.20 #
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
 **函数名称: InitEnv
 **功    能: 初始化环境变量
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 0:success !0:failed
 **实现描述: 
 **     依次设置各环境变量的值
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.11.20 #
 ******************************************************************************/
void InitEnv(void)
{
    SetWorkDir();
    SetHomeDir();
}
