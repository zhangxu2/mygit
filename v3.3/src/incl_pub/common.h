#if !defined(__COMMON_H__)
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* ��Ϣ���нṹ�� */
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

/* �ļ������ */
extern int file_lock(int fd, int type, int whence, int offset, int len);
#define file_wrlock(fd) file_lock(fd, F_WRLCK, SEEK_SET, 0, 0)  /* д��(ȫ�ļ�) 0: means o to EOF */
#define file_rdlock(fd) file_lock(fd, F_RDLCK, SEEK_SET, 0, 0)  /* ����(ȫ�ļ�) */
#define file_try_wrlock(fd) file_try_lock(fd, F_WRLCK, SEEK_SET, 0, 0) /* ����д��(ȫ�ļ�) */
#define file_try_rdlock(fd) file_try_lock(fd, F_RDLCK, SEEK_SET, 0, 0) /* ���Զ���(ȫ�ļ�) */
#define file_unlock(fd) file_lock(fd, F_UNLCK, SEEK_SET, 0, 0)  /* ����(ȫ�ļ�) */

#define fbit_wrlock(fd, idx) file_lock(fd, F_WRLCK, SEEK_SET, idx, 1)   /* д��(���ֽ�) */
#define fbit_rdlock(fd, idx) file_lock(fd, F_RDLCK, SEEK_SET, idx, 1)   /* ����(���ֽ�) */
#define fbit_try_wrlock(fd, idx) file_try_lock(fd, FD_WRLCK, SEEK_SET, idx, 1) /* ����д��(���ֽ�) */
#define fbit_try_rdlock(fd, idx) file_try_lock(fd, FD_RDLCK, SEEK_SET, idx, 1) /* ���Զ���(���ֽ�) */
#define fbit_unlock(fd, idx) file_lock(fd, F_UNLCK, SEEK_SET, idx, 1)   /* ����(���ֽ�) */

/* ���������Ļ�ȡ������ */
#define ENV_MAX_LEN (256)   /* ��������ֵ�ĳ��� */

/* ������������ */
typedef enum
{
    ENV_SWWORK,             /* ��������SWWORK */
    ENV_SWHOME,             /* ��������SWHOME */

    ENV_UNKNOWN,            /* λ�û������� */
    ENV_TOTAL = ENV_UNKNOWN /* ������������ */
}ENV_VARIABLE_e;

extern char (*g_EnvTablePtr)[ENV_MAX_LEN];    /* ��������������ָ�� */

extern void InitEnv(void);
extern int SetEnv(int type, const char *value);
#define GetEnv(type) (g_EnvTablePtr + type)

#define GetWorkDir() GetEnv(ENV_SWWORK)
#define SetWorkDir() SetEnv(ENV_SWWORK, getenv("SWWORK"))
#define GetHomeDir() GetEnv(ENV_SWHOME)
#define SetHomeDir() SetEnv(ENV_SWHOME, getenv("SWHOME"))

#endif /*__COMMON_H__*/
