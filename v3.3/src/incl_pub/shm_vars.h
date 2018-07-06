#if !defined(__SHM_VARS_H__)
#define __SHM_VARS_H__

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include "common.h"
#include "pub_type.h"
#include "pub_cfg.h"
#include "shm_slab.h"

#if defined(__SHM_VARS_SUPPORT__)

/******************************************************************************/
/* 宏开关配置 */

/* 变量池扩展类型 */
#define VARS_EXP_TYPE_FILE  (-1)        /* 使用文件扩展 */
#define VARS_EXP_TYPE_SHM   (-2)        /* 使用共享内存扩展 */

#define VARS_MEM_ALLOC_SLAB (-1)        /* SLAB算法实现内存分配 */
#define VARS_MEM_ALLOC_MOVE (-2)        /* 偏移算法实现内存分配 */

#define CFG_VARS_EXP_TYPE    (VARS_EXP_TYPE_FILE)       /* 配置扩展类型 */
#define CFG_VARS_MEM_ALLOC   (VARS_MEM_ALLOC_SLAB)      /* 配置内存分配算法 */


/* 依据扩展配置类型，开启功能宏 */
#if (defined(CFG_VARS_EXP_TYPE) && (VARS_EXP_TYPE_FILE == CFG_VARS_EXP_TYPE))
    #define __VARS_EXP_FILE__           /* 变量池超大时: 支持文件存储变量池数据 */
#elif (defined(CFG_VARS_EXP_TYPE) && (VARS_EXP_TYPE_SHM == CFG_VARS_EXP_TYPE))
    #define __VARS_EXP_SHM__            /* 变量池超大时: 支持扩展共享内存存储变量池数据 */
#endif

#if (defined(CFG_VARS_MEM_ALLOC) && (VARS_MEM_ALLOC_SLAB == CFG_VARS_MEM_ALLOC))
    #define __VARS_MEM_ALLOC_MOVE__     /* 支持SLAB内存分配算法 */
#endif
/******************************************************************************/

#define VARS_NODE_NUM       (301)       /* 变量池 节点数 */
#define VAR_NAME_MAX_LEN    (64)        /* 变量名 最大长度 */
#define VARS_MAX_SIZE       (327680)    /* 单个变量池尺寸: 320KB */
#define VARS_DEF_MAX_NUM    (1000)      /* 默认变量池最大个数 */

#define VARS_PRINT_MAX_LEN  (1024)
#define SYS_CMD_MAX_LEN     (1024)      /* 系统命令 最大长度 */
#if !defined(FILE_NAME_MAX_LEN)
#define FILE_NAME_MAX_LEN   (256)       /* 文件名的最大长度 */
#endif

#if defined(__VARS_EXP_FILE__)
    #define VARS_DIR_MODE   (0777)      /* 目录权限 */
    #define VARS_EXP_SIZE   (32768)     /* 扩展文件缓存增长基数: 32K (getpagesize() = 4k) */
#elif defined(__VARS_EXP_SHM__)
    #define VARS_SHM_BASE_KEY   (0x12345678)    /* 共享内存基础KEY */
    #define VARS_EXP_SIZE   (524288)    /* 扩展共享内存增长基数: 512K */
    #define VARS_IPC_KEY_INVAILD    (0) /* IPC键值非法值 */
#endif

#define VAR_TYPE_CHAR       ('a')       /* 变量类型: 字符类型 */
#define VAR_TYPE_BIN        ('b')       /* 变量类型: 二进制类型 */

/* 变量池节点 */
typedef struct
{
    char name[VAR_NAME_MAX_LEN];        /* 变量名 [注意: 第一位是否为'\0'可用来判断该节点是否被占用] */
    int type;                           /* 数据类型: VAR_TYPE_CHAR-字符 VAR_TYPE_BIN-二进制 */
    size_t value;                       /* 变量值 */
    size_t length;                      /* 变量值的长度 */
    int index;                          /* for repeat var */
    size_t size;                        /* 分配给VALUE的空间大小 */
    size_t next;                        /* 下一个节点 */
#if defined(SOLARIS) || defined(HPUX)
}__attribute__ ((packed))vars_node_t;
#else
}vars_node_t;
#endif

/* 变量池参数信息 */
typedef struct
{
    int shmid;                          /* 共享内存ID */
#if defined(__VARS_RAND_ALLOC__)
    int semid;                          /* 信号量ID */
#endif /*__VARS_RAND_ALLOC__*/
    int vars_total;                     /* 变量池总数 */
    size_t unit_size;                   /* 单个变量池大小 */

    void *addr;                         /* 共享内存首地址: 索引段+变量池段 */
#if defined(__VARS_RAND_ALLOC__)
    int *bitmap;                        /* 索引段首地址 */
#endif /*__VARS_RAND_ALLOC__*/
}vars_args_t;

#if defined(__VARS_EXP_FILE__)
typedef struct
{
    size_t size;                        /* 文件块大小 */
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    size_t offset;                      /* 当前偏移量: 相对start_offset计算 */
#endif
    size_t start_offset;                /* 起始偏移量: 相对数据块首地址计算 */
    size_t end_offset;                  /* 结束偏移量: 相对数据块首地址计算 */
}vars_fblk_head_t;

/* 变量池文件扩展 */
typedef struct _vars_fblk_t
{
    vars_fblk_head_t head;              /* 缓存头部信息 */
    char *fbuff;                        /* 文件缓存块 */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    shm_slab_pool_t *pool;              /* SLAB内存池 */
#endif
    int isdirty;                        /* 是否为脏数据: 是否需要进行同步 */
    
    struct _vars_fblk_t *next;          /* 下一个 */
}vars_fblk_t;

#elif defined(__VARS_EXP_SHM__)
/* 扩展共享内存头部信息 */
typedef struct _vars_exp_head_t
{
    key_t next_key;                     /* 下一块共享内存KEY值 */
    
    size_t size;                        /* 共享内存大小 */
    size_t offset;                      /* 偏移量: 相对start_offset计算 */
    size_t start_offset;                /* 起始偏移量: 相对数据块起始地址计算 */
    size_t end_offset;                  /* 结束偏移量: 相对数据块起始地址计算 */
}vars_exp_head_t;

/* 扩展内存信息在进程中链表 */
typedef struct _vars_exp_t
{
    int shmkey;                         /* 扩展内存KEY值 */
    int shmid;                          /* 扩展共享内存ID */
    void *addr;                         /* 扩展共享内存地址 */
    char *data;                         /* 扩展共享内存数据块 */
    vars_exp_head_t *head;              /* 扩展共享内存头部 */
	
    struct _vars_exp_t *next;           /* 下一块扩展共享内存地址 */
}vars_exp_t;
#endif /*__VARS_EXP_SHM__*/

/* 变量池头部信息 */
typedef struct
{
#if defined(__VARS_EXP_FILE__)
    int fflag;                          /* 文件存储标识 */
#elif defined(__VARS_EXP_SHM__)
    key_t next_key;                     /* 下一块共享内存key值 */
#endif
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    size_t offset;                      /* 数据块使用偏移量 */
#endif
    vars_node_t node[VARS_NODE_NUM];    /* 变量池节点 */
}vars_head_t;

/* 单个变量池对象基本信息 */
typedef struct
{
    int vid;                            /* 变量池ID */
    void *addr;                         /* 单个变量池地址 */
    vars_head_t *head;                  /* 变量池头部信息 */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    shm_slab_pool_t *pool;              /* SLAB内存池对象 */
#endif /*__VARS_MEM_ALLOC_SLAB__*/
    void *data;                         /* 数据块首地址 */
    size_t data_size;                   /* 数据块空间大小 */
#if defined(__VARS_EXP_FILE__)
    vars_fblk_t *fblk;                  /* 文件缓存块链表: 当单个变量池空间不足时，使用文件缓存进行扩展 */
#elif defined(__VARS_EXP_SHM__)
    vars_exp_t *expand;                 /* 下一块共享内存地址 */
#endif
}vars_object_t;

/* 变量池配置信息 */
typedef struct
{
	int vars_max_size;                  /* 单个变量池空间 */
	int  vars_max_num;                  /* 变量池个数 */
	key_t vars_shm_key;                 /* 共享内存KEY */
	key_t vars_sem_key;                 /* 信号量集KEY */
}sw_vars_cfg_t;

/* 序列化对象 */
typedef struct
{
    char *buf;                          /* 缓存首地址 */
    size_t buflen;                      /* 缓存空间大小 */
    size_t offset;                      /* 当前偏移 */
}vars_serial_t;

/* 反序列化对象 */
typedef struct
{
    const char *buf;                    /* 缓存首地址 */
    const char *ptr;                    /* 当前偏移 */
}vars_unserial_t;

/* 创建/销毁 变量池集 */
SW_PUBLIC sw_int_t vars_creat(sw_vars_cfg_t *params);
SW_PUBLIC sw_int_t vars_destory(void);
SW_PUBLIC sw_int_t vars_shm_init(sw_syscfg_t *syscfg);

#if defined(__VARS_RAND_ALLOC__)
SW_PUBLIC sw_int_t vars_valloc(vars_object_t *vars);
#endif /*__VARS_RAND_ALLOC__*/
SW_PUBLIC sw_int_t vars_vclear(vars_object_t *vars);
SW_PUBLIC sw_int_t vars_vfree(vars_object_t *vars);
SW_PUBLIC sw_int_t vars_vclone(vars_object_t *src, int dst_vid);
SW_PUBLIC sw_int_t vars_vmerge(const vars_object_t *src, int dst_vid);

SW_PUBLIC sw_int_t vars_object_creat(int vid, vars_object_t *vars);
#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
extern sw_int_t vars_object_free(vars_object_t *vars);
#else
#define vars_object_free(vars) (NULL)
#endif

SW_PUBLIC sw_int_t vars_set_revariable(
        vars_object_t *vars, const char *name,
        int type, const char *value, int length, int index);
SW_PUBLIC const void *vars_get_revariable(vars_object_t *vars, const char *name, int index, int* len);
SW_PUBLIC sw_int_t vars_delete_revariable(vars_object_t *vars, const char *name, int index);
SW_PUBLIC sw_int32_t vars_get_field_len(vars_object_t *vars, const char *name, int index);

#define vars_get_variable(vars, name, length)    vars_get_revariable(vars, name, 0, length)
#define vars_set_variable(vars, name, type, value, length) \
			vars_set_revariable(vars, name, type, value, length, 0)
#define vars_delete_variable(vars, name)    vars_delete_revariable(vars, name, 0)
#define vars_set_string(vars, name, value) 	\
			vars_set_revariable(vars, name, VAR_TYPE_CHAR, value, strlen(value), 0);
SW_PUBLIC sw_char_t* vars_get_null(vars_object_t *vars, const char *name, int length);
SW_PUBLIC sw_int_t vars_print(const vars_object_t *vars);

extern sw_int_t vars_serial(const vars_object_t *vars, char *buf, size_t buflen);
extern sw_int_t vars_unserial(vars_object_t *vars, const char *buf);
extern void *vars_offset_to_addr(const vars_object_t *vars, size_t offset);

/* 变量同步 */
#if defined(__VARS_EXP_FILE__)
extern sw_int_t vars_sync(vars_object_t *vars);
#else   /*__VARS_EXP_FILE__*/
#define vars_sync(vars) (NULL)
#endif  /*__VARS_EXP_FILE__*/

#else /*__SHM_VARS_SUPPORT__*/

typedef struct
{
	int vars_max_size;
	int  vars_max_num;
	key_t vars_shm_key;
	key_t vars_sem_key;
}sw_vars_cfg_t;

#define vars_creat(cfg) (NULL)
#define vars_destory()  (NULL)
#endif /*__SHM_VARS_SUPPORT__*/
#endif /*__SHM_VARS_H__*/
