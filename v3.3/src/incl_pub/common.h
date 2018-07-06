#if !defined(__COMMON_H__)
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* 消息队列结构体 */
typedef struct
{
    int mtype;
    char mtext[1];
}msgbuf_t;

extern int Mkdir(const char *dir, mode_t mode);
extern int Mkdir2(const char *fname, mode_t mode);
extern unsigned int Hash(const char *str);
extern ssize_t Readn(int fd, void *buff, size_t n);
extern ssize_t Writen(int fd, const void *buff, size_t n);
extern int Open(const char *fpath, int flags, mode_t mode);
#define Close(fd) {close(fd), fd = -1;}
#define fClose(fp) {fclose(fp), fp = NULL;}
extern int Sleep(int seconds);
extern int Rename(const char *oldpath, const char *newpath);

extern int Msgrcv(int msqid, msgbuf_t *msgbuf, size_t msgsz, int msgtype, int flag);
extern int Msgsnd(int msqid, msgbuf_t *msgbuf, size_t msgsz, int flag);

extern int daemon_init(void);
extern int System(const char *cmd);

/* 文件锁相关 */
extern int file_lock(int fd, int type, int whence, int offset, int len);
#define file_wrlock(fd) file_lock(fd, F_WRLCK, SEEK_SET, 0, 0)  /* 写锁(全文件) 0: means o to EOF */
#define file_rdlock(fd) file_lock(fd, F_RDLCK, SEEK_SET, 0, 0)  /* 读锁(全文件) */
#define file_try_wrlock(fd) file_try_lock(fd, F_WRLCK, SEEK_SET, 0, 0) /* 尝试写锁(全文件) */
#define file_try_rdlock(fd) file_try_lock(fd, F_RDLCK, SEEK_SET, 0, 0) /* 尝试读锁(全文件) */
#define file_unlock(fd) file_lock(fd, F_UNLCK, SEEK_SET, 0, 0)  /* 解锁(全文件) */

#define fbit_wrlock(fd, idx) file_lock(fd, F_WRLCK, SEEK_SET, idx, 1)   /* 写锁(单字节) */
#define fbit_rdlock(fd, idx) file_lock(fd, F_RDLCK, SEEK_SET, idx, 1)   /* 读锁(单字节) */
#define fbit_try_wrlock(fd, idx) file_try_lock(fd, FD_WRLCK, SEEK_SET, idx, 1) /* 尝试写锁(单字节) */
#define fbit_try_rdlock(fd, idx) file_try_lock(fd, FD_RDLCK, SEEK_SET, idx, 1) /* 尝试读锁(单字节) */
#define fbit_unlock(fd, idx) file_lock(fd, F_UNLCK, SEEK_SET, idx, 1)   /* 解锁(单字节) */

/* 环境变量的获取和设置 */
#define ENV_MAX_LEN (256)   /* 环境变量值的长度 */

/* 环境变量类型 */
typedef enum
{
    ENV_SWWORK,             /* 环境变量SWWORK */
    ENV_SWHOME,             /* 环境变量SWHOME */

    ENV_UNKNOWN,            /* 位置环境变量 */
    ENV_TOTAL = ENV_UNKNOWN /* 环境变量总数 */
}ENV_VARIABLE_e;

extern char (*g_EnvTablePtr)[ENV_MAX_LEN];    /* 环境变量参数表指针 */

extern void InitEnv(void);
extern int SetEnv(int type, const char *value);
#define GetEnv(type) (g_EnvTablePtr + type)

#define GetWorkDir() GetEnv(ENV_SWWORK)
#define SetWorkDir() SetEnv(ENV_SWWORK, getenv("SWWORK"))
#define GetHomeDir() GetEnv(ENV_SWHOME)
#define SetHomeDir() SetEnv(ENV_SWHOME, getenv("SWHOME"))

#endif /*__COMMON_H__*/
