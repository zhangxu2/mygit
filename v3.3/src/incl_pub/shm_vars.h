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
/* �꿪������ */

/* ��������չ���� */
#define VARS_EXP_TYPE_FILE  (-1)        /* ʹ���ļ���չ */
#define VARS_EXP_TYPE_SHM   (-2)        /* ʹ�ù����ڴ���չ */

#define VARS_MEM_ALLOC_SLAB (-1)        /* SLAB�㷨ʵ���ڴ���� */
#define VARS_MEM_ALLOC_MOVE (-2)        /* ƫ���㷨ʵ���ڴ���� */

#define CFG_VARS_EXP_TYPE    (VARS_EXP_TYPE_FILE)       /* ������չ���� */
#define CFG_VARS_MEM_ALLOC   (VARS_MEM_ALLOC_SLAB)      /* �����ڴ�����㷨 */


/* ������չ�������ͣ��������ܺ� */
#if (defined(CFG_VARS_EXP_TYPE) && (VARS_EXP_TYPE_FILE == CFG_VARS_EXP_TYPE))
    #define __VARS_EXP_FILE__           /* �����س���ʱ: ֧���ļ��洢���������� */
#elif (defined(CFG_VARS_EXP_TYPE) && (VARS_EXP_TYPE_SHM == CFG_VARS_EXP_TYPE))
    #define __VARS_EXP_SHM__            /* �����س���ʱ: ֧����չ�����ڴ�洢���������� */
#endif

#if (defined(CFG_VARS_MEM_ALLOC) && (VARS_MEM_ALLOC_SLAB == CFG_VARS_MEM_ALLOC))
    #define __VARS_MEM_ALLOC_MOVE__     /* ֧��SLAB�ڴ�����㷨 */
#endif
/******************************************************************************/

#define VARS_NODE_NUM       (301)       /* ������ �ڵ��� */
#define VAR_NAME_MAX_LEN    (64)        /* ������ ��󳤶� */
#define VARS_MAX_SIZE       (327680)    /* ���������سߴ�: 320KB */
#define VARS_DEF_MAX_NUM    (1000)      /* Ĭ�ϱ����������� */

#define VARS_PRINT_MAX_LEN  (1024)
#define SYS_CMD_MAX_LEN     (1024)      /* ϵͳ���� ��󳤶� */
#if !defined(FILE_NAME_MAX_LEN)
#define FILE_NAME_MAX_LEN   (256)       /* �ļ�������󳤶� */
#endif

#if defined(__VARS_EXP_FILE__)
    #define VARS_DIR_MODE   (0777)      /* Ŀ¼Ȩ�� */
    #define VARS_EXP_SIZE   (32768)     /* ��չ�ļ�������������: 32K (getpagesize() = 4k) */
#elif defined(__VARS_EXP_SHM__)
    #define VARS_SHM_BASE_KEY   (0x12345678)    /* �����ڴ����KEY */
    #define VARS_EXP_SIZE   (524288)    /* ��չ�����ڴ���������: 512K */
    #define VARS_IPC_KEY_INVAILD    (0) /* IPC��ֵ�Ƿ�ֵ */
#endif

#define VAR_TYPE_CHAR       ('a')       /* ��������: �ַ����� */
#define VAR_TYPE_BIN        ('b')       /* ��������: ���������� */

/* �����ؽڵ� */
typedef struct
{
    char name[VAR_NAME_MAX_LEN];        /* ������ [ע��: ��һλ�Ƿ�Ϊ'\0'�������жϸýڵ��Ƿ�ռ��] */
    int type;                           /* ��������: VAR_TYPE_CHAR-�ַ� VAR_TYPE_BIN-������ */
    size_t value;                       /* ����ֵ */
    size_t length;                      /* ����ֵ�ĳ��� */
    int index;                          /* for repeat var */
    size_t size;                        /* �����VALUE�Ŀռ��С */
    size_t next;                        /* ��һ���ڵ� */
#if defined(SOLARIS) || defined(HPUX)
}__attribute__ ((packed))vars_node_t;
#else
}vars_node_t;
#endif

/* �����ز�����Ϣ */
typedef struct
{
    int shmid;                          /* �����ڴ�ID */
#if defined(__VARS_RAND_ALLOC__)
    int semid;                          /* �ź���ID */
#endif /*__VARS_RAND_ALLOC__*/
    int vars_total;                     /* ���������� */
    size_t unit_size;                   /* ���������ش�С */

    void *addr;                         /* �����ڴ��׵�ַ: ������+�����ض� */
#if defined(__VARS_RAND_ALLOC__)
    int *bitmap;                        /* �������׵�ַ */
#endif /*__VARS_RAND_ALLOC__*/
}vars_args_t;

#if defined(__VARS_EXP_FILE__)
typedef struct
{
    size_t size;                        /* �ļ����С */
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    size_t offset;                      /* ��ǰƫ����: ���start_offset���� */
#endif
    size_t start_offset;                /* ��ʼƫ����: ������ݿ��׵�ַ���� */
    size_t end_offset;                  /* ����ƫ����: ������ݿ��׵�ַ���� */
}vars_fblk_head_t;

/* �������ļ���չ */
typedef struct _vars_fblk_t
{
    vars_fblk_head_t head;              /* ����ͷ����Ϣ */
    char *fbuff;                        /* �ļ������ */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    shm_slab_pool_t *pool;              /* SLAB�ڴ�� */
#endif
    int isdirty;                        /* �Ƿ�Ϊ������: �Ƿ���Ҫ����ͬ�� */
    
    struct _vars_fblk_t *next;          /* ��һ�� */
}vars_fblk_t;

#elif defined(__VARS_EXP_SHM__)
/* ��չ�����ڴ�ͷ����Ϣ */
typedef struct _vars_exp_head_t
{
    key_t next_key;                     /* ��һ�鹲���ڴ�KEYֵ */
    
    size_t size;                        /* �����ڴ��С */
    size_t offset;                      /* ƫ����: ���start_offset���� */
    size_t start_offset;                /* ��ʼƫ����: ������ݿ���ʼ��ַ���� */
    size_t end_offset;                  /* ����ƫ����: ������ݿ���ʼ��ַ���� */
}vars_exp_head_t;

/* ��չ�ڴ���Ϣ�ڽ��������� */
typedef struct _vars_exp_t
{
    int shmkey;                         /* ��չ�ڴ�KEYֵ */
    int shmid;                          /* ��չ�����ڴ�ID */
    void *addr;                         /* ��չ�����ڴ��ַ */
    char *data;                         /* ��չ�����ڴ����ݿ� */
    vars_exp_head_t *head;              /* ��չ�����ڴ�ͷ�� */
	
    struct _vars_exp_t *next;           /* ��һ����չ�����ڴ��ַ */
}vars_exp_t;
#endif /*__VARS_EXP_SHM__*/

/* ������ͷ����Ϣ */
typedef struct
{
#if defined(__VARS_EXP_FILE__)
    int fflag;                          /* �ļ��洢��ʶ */
#elif defined(__VARS_EXP_SHM__)
    key_t next_key;                     /* ��һ�鹲���ڴ�keyֵ */
#endif
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    size_t offset;                      /* ���ݿ�ʹ��ƫ���� */
#endif
    vars_node_t node[VARS_NODE_NUM];    /* �����ؽڵ� */
}vars_head_t;

/* ���������ض��������Ϣ */
typedef struct
{
    int vid;                            /* ������ID */
    void *addr;                         /* ���������ص�ַ */
    vars_head_t *head;                  /* ������ͷ����Ϣ */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    shm_slab_pool_t *pool;              /* SLAB�ڴ�ض��� */
#endif /*__VARS_MEM_ALLOC_SLAB__*/
    void *data;                         /* ���ݿ��׵�ַ */
    size_t data_size;                   /* ���ݿ�ռ��С */
#if defined(__VARS_EXP_FILE__)
    vars_fblk_t *fblk;                  /* �ļ����������: �����������ؿռ䲻��ʱ��ʹ���ļ����������չ */
#elif defined(__VARS_EXP_SHM__)
    vars_exp_t *expand;                 /* ��һ�鹲���ڴ��ַ */
#endif
}vars_object_t;

/* ������������Ϣ */
typedef struct
{
	int vars_max_size;                  /* ���������ؿռ� */
	int  vars_max_num;                  /* �����ظ��� */
	key_t vars_shm_key;                 /* �����ڴ�KEY */
	key_t vars_sem_key;                 /* �ź�����KEY */
}sw_vars_cfg_t;

/* ���л����� */
typedef struct
{
    char *buf;                          /* �����׵�ַ */
    size_t buflen;                      /* ����ռ��С */
    size_t offset;                      /* ��ǰƫ�� */
}vars_serial_t;

/* �����л����� */
typedef struct
{
    const char *buf;                    /* �����׵�ַ */
    const char *ptr;                    /* ��ǰƫ�� */
}vars_unserial_t;

/* ����/���� �����ؼ� */
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

/* ����ͬ�� */
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
