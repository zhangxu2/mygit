#include "pub_buf.h"
#include "shm_vars.h"
#include <ctype.h>
#if defined(__VARS_MEM_ALLOC_SLAB__)
#include "shm_slab.h"
#endif /*__VARS_MEM_ALLOC_SLAB__*/

#if defined(__SHM_VARS_SUPPORT__)

#define VARS_SERAIL_DIGIT_MAX_LEN   (32)
#define VARS_SERAIL_NAME_LEN    (2)
#define VARS_SERAIL_INDEX_LEN   (4)
#define VARS_SERIAL_TYPE_LEN    (1)
#define VARS_SERIAL_LEN_MAX_LEN (16)
#define VARS_SERIAL_SLOT_LEN    (512)


#if defined(__VARS_RAND_ALLOC__)
union semun
{
    int val;                            /* for SETVAL */
    struct semid_ds *buf;               /* for IPC_STAT and IPC_SET */
    ushort *array;                      /* for GETALL and SETALL */
};
#endif /*__VARS_RAND_ALLOC__*/

#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
/* ����ҳ��С */
static int g_vars_page_size = 0;
#define vars_set_page_size(size) (g_vars_page_size = (size))        /* ����ҳ��С */
#define vars_get_page_size()    (g_vars_page_size)                  /* ��ȡҳ��С */
#endif

/* ȫ�ֱ����ز�����Ϣ: ��ʼ���󣬲��ɸ��� */
static vars_args_t g_vars_args;

/* �����ڴ�ID */
#define vars_set_shmid(id) (g_vars_args.shmid = (id))               /* ���ù����ڴ�ID */
#define vars_get_shmid() (g_vars_args.shmid)                        /* ��ȡ�����ڴ�ID */

#if defined(__VARS_RAND_ALLOC__)
/* �ź���ID */
#define vars_set_semid(id) (g_vars_args.semid = (id))               /* �����ź���ID */
#define vars_get_semid() (g_vars_args.semid)                        /* ��ȡ�ź���ID */
#endif /*__VARS_RAND_ALLOC__*/

/* ���������� */
#define vars_set_total(num) (g_vars_args.vars_total = (num))        /* ���ñ��������� */
#define vars_get_total() (g_vars_args.vars_total)                   /* ��ȡ���������� */
/* �����ص�Ԫ */
#define vars_set_unit_size(usize)   (g_vars_args.unit_size = usize) /* ���ñ����ص�Ԫ��С */
#define vars_get_unit_size()   (g_vars_args.unit_size)              /* ���ñ����ص�Ԫ��С */
/* �ڴ��ַ */
#define vars_set_addr(addr) (g_vars_args.addr = (addr))             /* ���ù����ڴ��׵�ַ */
#define vars_get_addr() (g_vars_args.addr)                          /* ��ȡ�����ڴ��׵�ַ */

#if defined(__VARS_RAND_ALLOC__)
#define vars_bitmap_len()  (g_vars_args.vars_total * sizeof(int))   /* �����γ��� */
#define vars_set_bitmap_addr(addr) (g_vars_args.bitmap = (addr))    /* �����������׵�ַ */
/* �������� */
#define vars_set_used(vid)   (g_vars_args.bitmap[vid] = 1)          /* ���ñ����ر�ռ�� */
#define vars_reset_used(vid)   (g_vars_args.bitmap[vid] = 0)        /* ���ñ����ر�ռ�� */
#define vars_is_used(vid)   (g_vars_args.bitmap[vid])               /* ��ȡ�����ر�ռ�� */
#endif /*__VARS_RAND_ALLOC__*/

/* �жϽڵ��Ƿ�ռ�� */
#define VarsIsNodeUsed(node)    ('\0' != node->name[0])
/* �жϱ������Ƿ�Ϸ� */
#define VarsIsNameValid(name) (('#'==name[0]) || ('$'==name[0]))
/* �жϱ����ؿռ��Ƿ��㹻 */
#define VarsIsShmEnough(vars, size) ((vars->head->offset+(size)) <= vars->data_size)

#define vars_get_node_mod(name) (Hash(name)%VARS_NODE_NUM)          /* ������ģֵ */

#if !defined(__VARS_MEM_ALLOC_SLAB__)
#define vars_reset_offset(vars) (vars->head->offset = 0)            /* ����ƫ���� */
#endif

#if defined(__VARS_EXP_FILE__)
static void vars_exp_dir_creat(void);
void *vars_offset_to_addr(const vars_object_t *vars, size_t offset);
static void *vars_exp_offset_to_addr(const vars_object_t *vars, size_t offset);
static void *vars_exp_block_alloc(vars_object_t *vars, size_t size, size_t *offset);
#if defined(__VARS_MEM_ALLOC_SLAB__)
static int vars_exp_block_free(vars_object_t *vars, void *p);
#endif /*__VARS_MEM_ALLOC_SLAB__*/
static sw_int_t vars_exp_load(vars_object_t *vars);
static sw_int_t vars_exp_link_free(vars_fblk_t *head);
static sw_int_t vars_exp_destory(vars_object_t *vars);
static sw_int_t vars_exp_clone(vars_object_t *src_vars, int dst_vid);

/* �ͷŵ����ļ���ڵ� */
#define vars_exp_free(fblk) \
{ \
    if(NULL != fblk->fbuff) \
    { \
        free(fblk->fbuff); \
        fblk->fbuff = NULL; \
    } \
    free(fblk), fblk=NULL; \
}

/* ��ȡ�����ػ����ļ��� */
#define vars_exp_get_fdir(fdir, size) \
            snprintf(fdir, size, "%s/tmp/vars", getenv("SWWORK"))
#define vars_exp_get_fname(vid, fname, size) \
            snprintf(fname, size, "%s/tmp/vars/%d.vars", getenv("SWWORK"), vid)

/* �ļ���ʶ */
#define vars_set_fflag(vars)    (vars->head->fflag = 1)             /* �����ļ���ʶ */
#define vars_reset_fflag(vars)  (vars->head->fflag = 0)             /* �����ļ���ʶ */
#define vars_get_fflag(vars)    (vars->head->fflag)                 /* �����ļ���ʶ */

/* �����ݱ�ʶ */
#define vars_set_dirty(fblk)    (fblk->isdirty = 1)                 /* ���������ݱ�ʶ */
#define vars_reset_dirty(fblk)  (fblk->isdirty = 0)                 /* ���������ݱ�ʶ */
#define vars_get_dirty(fblk)    (fblk->isdirty)                     /* ��ȡ�����ݱ�ʶ */

#elif defined(__VARS_EXP_SHM__)

#define vars_reset_next_key(vars) (vars->head->next_key = VARS_IPC_KEY_INVAILD)

void *vars_offset_to_addr(const vars_object_t *vars, size_t offset);
static void *vars_exp_offset_to_addr(vars_object_t *vars, size_t offset);
static void *vars_exp_block_alloc(vars_object_t *vars, int size, int *offset);
static int vars_exp_load(vars_object_t *vars);
static int vars_exp_link_free(vars_exp_t *head);
static int vars_exp_destory(vars_object_t *vars);

#define vars_exp_set_next_key(exp, key) (exp->head->next_key = key)
#define vars_exp_reset_next_key(exp) (exp->head->next_key = VARS_IPC_KEY_INVAILD)
#define vars_exp_get_next_key(exp)   (exp->head->next_key)

#else /*__VARS_EXP_FILE__*/
#define vars_offset_to_addr(vars, offset) ((vars)->data + (offset)) /* ƫ����ת��Ϊ�ڴ��ַ */
#endif /*__VARS_EXP_FILE__*/

/* ��̬�������� */
static int vars_shm_creat(key_t key, int num);
#if defined(__VARS_RAND_ALLOC__)
static int vars_sem_creat(key_t key, int num);
#endif /*__VARS_RAND_ALLOC__*/
static int vars_shm_size(int num, int usize);

#if defined(__VARS_RAND_ALLOC__)
/******************************************************************************
 **��������: Random
 **��    ��: ����(�Ǹ�)�������
 **�������: NONE
 **�������: NONE
 **��    ��: �Ǹ��������
 **ʵ������: 
 **     1. ��ȡ��ǰʱ��
 **     2. �����������
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
sw_int32_t Random(void)
{
    struct timeval cur_time;

    memset(&cur_time, 0, sizeof(cur_time));

    gettimeofday(&cur_time, NULL);

    return ((random()*cur_time.tv_usec)&0x7FFFFFFF);
}

/******************************************************************************
 **��������: vars_lock
 **��    ��: ����ָ������������
 **�������:
 **      vid: ����������ID
 **      flag: �ź���sem_flg��ֵ(IPC_NOWAIT or SEM_UNDO)
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **      ����Ҫ��vars_unlock()���ʹ��
 **��    ��: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
static sw_int32_t vars_lock(int vid, int flag)
{
    int semid = 0;
    struct sembuf sem_buf;

    memset(&sem_buf, 0, sizeof(sem_buf));

    semid = vars_get_semid();

    sem_buf.sem_num = vid;
    sem_buf.sem_op = -1;
    sem_buf.sem_flg = flag;

    return semop(semid, &sem_buf, 1);
}

/******************************************************************************
 **��������: vars_unlock
 **��    ��: ����ָ������������
 **�������:
 **      vid: ����������ID
 **      flag: �ź���sem_flg��ֵ(IPC_NOWAIT or SEM_UNDO)
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **      ����Ҫ��vars_lock()���ʹ��
 **��    ��: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
static sw_int32_t vars_unlock(int vid, int flag)
{
    int semid = 0;
    struct sembuf sem_buf;

    memset(&sem_buf, 0, sizeof(sem_buf));

    semid = vars_get_semid();

    sem_buf.sem_num = vid;
    sem_buf.sem_op = +1;
    sem_buf.sem_flg = flag;

    return semop(semid, &sem_buf, 1);
}
#endif /*__VARS_RAND_ALLOC__*/

/******************************************************************************
 **��������: vars_creat
 **��    ��: ���������ؼ�
 **�������:
 **      swcfg: ƽ̨���ò���
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     1. ���������ڴ�
 **     2. �����ź�����
 **     3. ��ȡ�����ڴ��ַ
 **     4. ����ȫ�ֱ���
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.23 #
 ******************************************************************************/
sw_int_t vars_creat(sw_vars_cfg_t *params)
{
    void *addr = NULL;
    int shmid=0, size=0, usize=0;
#if defined(__VARS_RAND_ALLOC__)
	int semid = 0;
    vars_node_t *node = NULL;
#endif /*__VARS_RAND_ALLOC__*/
    
    memset(&g_vars_args, 0, sizeof(g_vars_args));
    
    /* 1. ���������ڴ� */
    usize = (params->vars_max_size > VARS_MAX_SIZE)? params->vars_max_size: VARS_MAX_SIZE;
    size = vars_shm_size(params->vars_max_num, usize);
    shmid = vars_shm_creat(params->vars_shm_key, size);
    if(shmid < 0)
    {
        return -1;
    }

#if defined(__VARS_RAND_ALLOC__)
    /* 2. �����ź����� */
    semid = vars_sem_creat(params->vars_sem_key, params->vars_max_num);
    if(semid < 0)
    {
        return -1;
    }
#endif /*__VARS_RAND_ALLOC__*/

    /* 2. ��ȡ�����ڴ��ַ */
    addr = shmat(shmid, NULL, 0);
    if((void*)-1 == addr)
    {
        return -1;
    }

    /* 4. ����ȫ�ֲ��� */
    vars_set_unit_size(usize);
    vars_set_shmid(shmid);
#if defined(__VARS_RAND_ALLOC__)
    vars_set_semid(semid);
#endif /*__VARS_RAND_ALLOC__*/
    vars_set_total(params->vars_max_num);
    vars_set_addr(addr);
#if defined(__VARS_RAND_ALLOC__)
    vars_set_bitmap_addr(addr);
#endif /*__VARS_RAND_ALLOC__*/

#if defined(__VARS_EXP_FILE__)
    vars_set_page_size(VARS_EXP_SIZE);
    vars_exp_dir_creat();
#elif defined(__VARS_EXP_SHM__)
    vars_set_page_size(VARS_EXP_SIZE);
#endif
    
    return 0;
}

/******************************************************************************
 **��������: vars_shm_init
 **��    ��: ���������ع����ڴ�
 **�������:
 **      swcfg: ƽ̨���ò���
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     1. ���������ڴ�
 **     2. ��ȡ�����ڴ��ַ
 **     3. ����ȫ�ֱ���
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.23 #
 ******************************************************************************/
sw_int_t vars_shm_init(sw_syscfg_t *syscfg)
{
    void *addr = NULL;
    int shmid=0;
    size_t size=0, usize=0;
    
    memset(&g_vars_args, 0, sizeof(g_vars_args));
    
    /* 1. ���������ڴ� */
    usize = (syscfg->share_pool_size > VARS_MAX_SIZE)? syscfg->share_pool_size: VARS_MAX_SIZE;
    size = vars_shm_size(syscfg->session_max, usize);

    /*create shm_vars*/
    if (SW_INVALID_IPC_ID == syscfg->vars_shmid)
    {
        shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | 0666);
        if (-1 == shmid)
        {
            pub_log_error("[%s][%d] shmget error, size[%d]."
            		,__FILE__,__LINE__,size);
            return -1;
        }

        syscfg->vars_shmid = shmid;
    }
    

    /* 2. ��ȡ�����ڴ��ַ */
    addr = shmat(syscfg->vars_shmid, NULL, 0);
    if((void*)-1 == addr)
    {
	    pub_log_error("[%s][%d] errmsg:[%d]%s! [%d].",
             __FILE__, __LINE__, errno, strerror(errno), syscfg->vars_shmid);
        return -1;
    }

    /* 4. ����ȫ�ֲ��� */
    vars_set_unit_size(usize);
    vars_set_shmid(shmid);
    vars_set_total(syscfg->session_max);
    vars_set_addr(addr);

#if defined(__VARS_EXP_FILE__)
    vars_set_page_size(VARS_EXP_SIZE);
    vars_exp_dir_creat();
#elif defined(__VARS_EXP_SHM__)
    vars_set_page_size(VARS_EXP_SIZE);
#endif
    
    return 0;	
}

/******************************************************************************
 **��������: vars_destory
 **��    ��: ����/�ͷű����ؼ�
 **�������: NONE
 **�������: NONE
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     1. �����ź�����
 **     2. ���ٹ����ڴ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_destory(void)
{
    void *addr = NULL;
    int shmid = 0;

#if defined(__VARS_RAND_ALLOC__)
	int semid = 0;

    semid = vars_get_semid();

    /* 1. �����ź����� */
    semctl(semid, 0, IPC_RMID);
#endif /*__VARS_RAND_ALLOC__*/
    

    /* 2. ���ٹ����ڴ� */
    addr = vars_get_addr();
    if((NULL == addr) || ((void *)-1 == addr))
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }
    
    shmdt(addr), addr=NULL;
    
    shmid = vars_get_shmid();
    if(shmid <= 0)
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

/******************************************************************************
 **��������: vars_shm_size
 **��    ��: ���㹲���ڴ�Ĵ�С(����+������)
 **�������:
 **      num: ����������
 **      size: ���������ش�С
 **�������:
 **��    ��: �����ڴ��С
 **ʵ������: 
 **      1. ����������ռ�ռ�
 **      2. �����������ռ�ռ�
 **
 **  �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **ע������: 
 **      1. ������: num��sizeof(int)
 **      2. �����ض�: num�������ض���
 **��    ��: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
static sw_int32_t vars_shm_size(int num, int usize)
{
    int size = 0;

#if defined(__VARS_RAND_ALLOC__)
    /* 1. ����������ռ�ռ� */
    size = num*sizeof(int);
#endif /*__VARS_RAND_ALLOC__*/

    /* 2. �����������ռ�ռ� */
    size += num * usize;

    return size;
}

/******************************************************************************
 **��������: vars_shm_creat
 **��    ��: ���������ڴ������
 **�������:
 **      key: �����ڴ�key
 **      num: �����ظ���
 **�������:
 **��    ��: �����ڴ�ID
 **ʵ������: 
 **     1. �ж��Ƿ��Ѵ��������ڴ�
 **     2. �����ڣ��򴴽������ڴ�
 **ע������: 
 **  �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **��    ��: # Qifeng.zou # 2013.04.22 #
 ******************************************************************************/
static sw_int32_t vars_shm_creat(key_t key, int size)
{
    int shmid = 0;
    void *addr = NULL;


    /* 1. �ж��Ƿ��Ѵ��������ڴ� */
    shmid = shmget(key, 0, 0);
    if(shmid < 0)
    {
        if(ENOENT != errno)
        {
            pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
            return -1;
        }
        /* �����ڴ治����, ��������д��� */
    }
    else
    {
        return shmid;
    }

    /* 2. �����ڣ��򴴽������ڴ� */    
    shmid = shmget(key, size, IPC_CREAT|0666);
    if(shmid < 0)
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }
    
    /* 3. ��չ����ڴ� */
    addr = shmat(shmid, NULL, 0);
    if(NULL == addr)
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    memset(addr, 0, size);

    shmdt(addr), addr=NULL;
    
    return shmid;
}

#if defined(__VARS_RAND_ALLOC__)
/******************************************************************************
 **��������: vars_sem_creat
 **��    ��: �����������ź���
 **�������:
 **      key: �ź�����key
 **      num: �ź�������(������ظ���һ��)
 **�������:
 **��    ��: �ź�����ID
 **ʵ������: 
 **     1. �ж��Ƿ��Ѵ����ź�����
 **     2. �����ڣ��򴴽��ź�����
 **     3. �����ź���������Ա�ĳ�ʼֵ
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.23 #
 ******************************************************************************/
static int vars_sem_creat(key_t key, int num)
{
    union semun sem;
    int vid=0, semid=0;


    /* 1. �ж��Ƿ��Ѵ����ź����� */
    semid = semget(key, 0, 0);
    if(semid < 0)
    {
        if(ENOENT != errno)
        {
            pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
            return -1;
        }
        /* �ź��������ڣ���������д��� */
    }
    else
    {
        return semid;
    }

    /* 2. �����ڣ��򴴽��ź����� */
    semid = semget(key, num, IPC_CREAT|0666);
    if(semid < 0)
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    /* 3. �����ź���������Ա�ĳ�ʼֵ */
    for(vid=0; vid<num; vid++)
    {
        sem.val = 1;
        semctl(semid, vid, SETVAL, sem);
    }
    
    return semid;
}

/******************************************************************************
 **��������: vars_valloc
 **��    ��: ����һ�����õı�����
 **�������: 
 **     vars: �����ض���
 **�������: NONE
 **��    ��: ����������ID
 **ʵ������: 
 **     1. ͨ�����ֵ����һ��������ID
 **     2. ����˱������ѱ�ռ�ã��������һ�������أ�ֱ���ҵ�һ��δ��ռ�õı�����
 **ע������:  
 **��    ��: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
int vars_valloc(vars_object_t *vars)
{
    int ret=0, vid=0, loop=0, maxloop=0;
    vars_args_t *args = &g_vars_args;

    vid = Random()%vars_get_total();
    maxloop = vars_get_total();

    do
    {
        loop++;
        if(vid >= vars_get_total())
        {
            vid %= vars_get_total();
        }
        
        
        if(vars_is_used(vid))
        {
            vid++;
            continue;
        }

        ret = vars_lock(vid, IPC_NOWAIT);
        if(ret < 0)
        {
            if(ENOENT != errno)
            {
                pub_log_error("[%s][%d] Lock vars[%d] failed!", __FILE__, __LINE__, vid);
                return -1;
            }
            vid++;
            continue;
        }

        if(vars_is_used(vid))
        {
            vars_unlock(vid, IPC_NOWAIT);
            vid++;
            continue;
        }

        vars_set_used(vid);

        vars_unlock(vid, IPC_NOWAIT);

        ret = vars_object_creat(vid, vars);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Create object failed!", __FILE__, __LINE__);
            return -1;
        }
        
        return 0;
    }while(loop <= maxloop);   /* ��ֹ��ѭ��: ѭ��һȦ���˳� */

    pub_log_error("[%s][%d] Alloc vars failed!", __FILE__, __LINE__);
    return -1;
}
#endif /*__VARS_RAND_ALLOC__*/

/******************************************************************************
 **��������: vars_vfree
 **��    ��: �ͷű�����
 **�������: 
 **     vars: �����ض���
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     �ͷű����ض�������Ķ�̬�ռ��
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
sw_int_t vars_vfree(vars_object_t *vars)
{
#if defined(__VARS_RAND_ALLOC__)
    if(!vars_is_used(vars->vid))
    {
        vars_object_free(vars);
        return 0;
    }
#endif /*__VARS_RAND_ALLOC__*/

#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
    /* 1. ������չ�ռ����� */
    vars_exp_destory(vars);
#endif

    /* 2. ���ñ�����ռ�ñ�ʶ */
#if defined(__VARS_RAND_ALLOC__)
    vars_lock(vars->vid, SEM_UNDO);
#endif /*__VARS_RAND_ALLOC__*/

    memset(vars->addr, 0, vars_get_unit_size());
#if defined(__VARS_RAND_ALLOC__)
    vars_reset_used(vars->vid);

    vars_unlock(vars->vid, SEM_UNDO);
#endif /*__VARS_RAND_ALLOC__*/

    vars_object_free(vars);
    
    return 0;
}

/******************************************************************************
 **��������: vars_vclone
 **��    ��: ��¡���������ö����������ָ����������
 **�������: 
 **     src_vars: ���������ö���
 **     dst_vid: Ŀ�������ID
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     1. ��¡�����ڴ��е�����
 **     2. ��¡��չ�ռ��е�����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.06.15 #
 ******************************************************************************/
sw_int_t vars_vclone(vars_object_t *src, int dst_vid)
{
    int ret = 0;
    vars_object_t dst;

    /* 1. ��ȡĿ����������ö��� */
    ret = vars_object_creat(dst_vid, &dst);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Create object failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 2. ������������Ŀ������� */
    /* 2.1 ���������ڴ� */
    vars_vclear(&dst);

    memcpy(dst.head, src->head, sizeof(vars_head_t));     /* ��¡������ͷ */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    memcpy(dst.pool, src->pool, src->pool->pool_size);
#else
    memcpy(dst.data, src->data, src->head->offset);  /* ��¡���������ݿ� */
#endif

#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
    /* 2.2 ������չ�ռ����� */
    vars_exp_clone(src, dst_vid);
#endif /*__VARS_EXP_FILE__ || __VARS_EXP_SHM__*/

    return 0;
}

/******************************************************************************
 **��������: vars_vmerge_link
 **��    ��: �����غϲ�
 **�������: 
 **     src: Դ������
 **     head: Դ�������еĹ�ϣ��ͷ
 **     dst: Ŀ�ı�����
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     1. ������ϣ����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
int vars_vmerge_link(const vars_object_t *src, const vars_node_t *head, vars_object_t *dst)
{
    int ret = 0;
    const char *value = NULL;
    const vars_node_t *node = head;

    while(NULL != node)
    {
        if(!VarsIsNodeUsed(node))
        {
        #if defined(__VARS_MEM_ALLOC_SLAB__)
            if(node != head)
            {
                return 0;
            }
        #endif /*__VARS_MEM_ALLOC_SLAB__*/
        
            if(0 == node->next)
            {
                return 0;
            }
            node = vars_offset_to_addr(src, node->next);
            continue;
        }

        value = vars_offset_to_addr(src, node->value);

        ret = vars_set_revariable(dst,
                    node->name, node->type, value, node->length, node->index);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Merge [%s] failed!", __FILE__, __LINE__, node->name);
            return -1;
        }

        if(0 == node->next)
        {
            return 0;
        }

        node = vars_offset_to_addr(src, node->next);
    }
    return 0;
}

/******************************************************************************
 **��������: vars_vmerge
 **��    ��: �ϲ�������
 **�������: 
 **     src: Դ���������ö���
 **     dst_vid: Ŀ�������ID
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     1. �ϲ������ڴ��е�����
 **     2. �ϲ���չ�ռ��е�����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
sw_int_t vars_vmerge(const vars_object_t *src, int dst_vid)
{
    int ret = 0, idx = 0;
    vars_object_t dst;

    memset(&dst, 0, sizeof(dst));

    /* 1. ��ȡĿ����������ö��� */
    ret = vars_object_creat(dst_vid, &dst);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Create object failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 2. ���κϲ����������� */
    for(idx=0; idx<VARS_NODE_NUM; idx++)
    {
        ret = vars_vmerge_link(src, &src->head->node[idx], &dst);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Merge link failed!", __FILE__, __LINE__);
            return -1;
        }
    }

    /* 3. ͬ�����ݲ��ͷ����ö���ռ� */
    vars_sync(&dst);
    
    vars_object_free(&dst);
    
    return 0;
}

/******************************************************************************
 **��������: vars_object_creat
 **��    ��: ����������ʵ��������
 **�������: 
 **     vid: ������ID
 **�������: 
 **     vars: �����ض���
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **  1) �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) ���������صĽṹ������ʾ:
 **     | �ļ� |ƫ����|<---    MOD�������ؽڵ�    --->|<--         ���ݿ�        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            ���ݿ�           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_object_creat(int vid, vars_object_t *vars)
{
#if defined(__VARS_MEM_ALLOC_SLAB__)
    int ret = 0;
#endif

    if((vid < 0) || (vid > vars_get_total()))
    {
        pub_log_error("[%s][%d] Vid is out of range! vid:%d", __FILE__, __LINE__, vid);
        return -1;
    }
    
    vars->vid = vid;
    vars->addr = (char *)vars_get_addr()            /* �����ڴ��׵�ַ */
                  #if defined(__VARS_RAND_ALLOC__)
                    + vars_bitmap_len()     /* ���������γ��� */
                  #endif /*__VARS_RAND_ALLOC__*/
                    + vid*vars_get_unit_size(); /* ����ǰ��ı����� */
    vars->head = (vars_head_t *)vars->addr; /* ������ͷ����Ϣ */
    vars->data = (void *)((char *)vars->addr + sizeof(vars_head_t));    /* ���ݿ���ʼ��ַ */
    vars->data_size = vars_get_unit_size() - sizeof(vars_head_t);   /* ���ݿ��ʼ�ռ� */ 

#if defined(__VARS_MEM_ALLOC_SLAB__)
    vars->pool = (shm_slab_pool_t *)vars->data;
    vars->pool->pool_size = vars->data_size;
    ret = shm_slab_init(vars->pool);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Slab init failed!", __FILE__, __LINE__);
        return -1;
    }
#endif /*__VARS_MEM_ALLOC_SLAB__*/

#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
    /* ������չ�ռ����� */
    return vars_exp_load(vars);
#else
    return 0;
#endif
}

#if defined(__VARS_EXP_FILE__)
/******************************************************************************
 **��������: vars_clear
 **��    ��: ��ձ����������ж��󣬵����ͷŶԱ����ص�ռ��
 **�������: 
 **     vars: �����ض���
 **�������: NONE
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������:  
 **��    ��: # Qifeng.zou # 2013.06.14 #
 ******************************************************************************/
sw_int_t vars_vclear(vars_object_t *vars)
{
    /* 1. �ͷ���չ�ռ� */
    vars_exp_destory(vars);

    vars_exp_link_free(vars->fblk);
    vars->fblk = NULL;

    /* 2. ���ù����ڴ����� */
    vars_reset_fflag(vars);
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    vars_reset_offset(vars);
#endif /*!__VARS_MEM_ALLOC_SLAB__*/
    memset(vars->addr, 0, vars_get_unit_size());
    
    return 0;
}

/******************************************************************************
 **��������: vars_object_free
 **��    ��: �ͷű�����ʵ��������Ŀռ䣬�����ͷű����صĿռ���ļ�
 **�������: 
 **     vars: �����ض���
 **�������: NONE
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������:  
 **��    ��: # Qifeng.zou # 2013.05.09 #
 ******************************************************************************/
sw_int_t vars_object_free(vars_object_t *vars)
{
    vars_exp_link_free(vars->fblk);
    
    vars->fblk = NULL;
    return 0;
}

#elif defined(__VARS_EXP_SHM__)
/******************************************************************************
 **��������: vars_clear
 **��    ��: ��ձ����������ж��󣬵����ͷŶԱ����ص�ռ��
 **�������: 
 **     vars: �����ض���
 **�������: NONE
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������:  
 **��    ��: # Qifeng.zou # 2013.06.14 #
 ******************************************************************************/
sw_int_t vars_vclear(vars_object_t *vars)
{
    /* 1. ������չ�ռ� */
    vars_exp_destory(vars);

    vars_exp_link_free(vars->expand);
    vars->expand = NULL;

    /* 2. ���ñ�ʶ */
    vars_reset_next_key(vars);
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    vars_reset_offset(vars);
#endif /*!__VARS_MEM_ALLOC_SLAB__*/
    memset(vars->addr, 0, vars_get_unit_size());
    return 0;
}

/******************************************************************************
 **��������: vars_object_free
 **��    ��: �ͷű�����ʵ��������Ŀռ䣬�����ͷű����صĿռ���ļ�
 **�������: 
 **     vars: �����ض���
 **�������: NONE
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������:  
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
sw_int_t vars_object_free(vars_object_t *vars)
{
    vars_exp_link_free(vars->expand);
    
    vars->expand = NULL;
    return 0;
}
#endif /*__VARS_EXP_FILE__*/
    
/******************************************************************************
 **��������: vars_block_alloc
 **��    ��: �ӱ����ؿռ��������ƶ���С�Ŀռ�
 **�������: 
 **     vars: �����ض���
 **     size: ����ռ��С
 **�������: NONE
 **��    ��: ����Ŀռ�ƫ����
 **ʵ������: 
 **     1. �ӹ����ڴ��з���ռ�
 **     2. ���ļ������з���ռ�
 **ע������:  
 **��    ��: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
static void *vars_block_alloc(vars_object_t *vars, int size, size_t *offset)
{
    void *addr = NULL;

    *offset = -1;

    /* 1. �ӹ����ڴ��з���ռ� */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    addr = shm_slab_alloc(vars->pool, size);
    if(NULL != addr)
    {
        *offset = addr - (void *)vars->pool;
        return addr;
    }
#else /*__VARS_MEM_ALLOC_SLAB__*/
    if(VarsIsShmEnough(vars, size))
    {
        *offset = vars->head->offset;
        addr = (char *)vars->data + *offset;
        vars->head->offset += size;

        memset(addr, 0, size);
        
        return addr;
    }
#endif /*__VARS_MEM_ALLOC_SLAB__*/

#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
    /* 2. ����չ�ռ��з���ռ� */
    return vars_exp_block_alloc(vars, size, offset);
#else
    return NULL;
#endif    
}

#if defined(__VARS_MEM_ALLOC_SLAB__)
/******************************************************************************
 **��������: vars_block_free
 **��    ��: �ͷű��������ڴ��Ŀռ�
 **�������:
 **      vars: �����ض���
 **      p: �ͷŵĵ�ַ����
 **�������: NONE
 **��    ��: 0:success  !0:failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.18 #
 ******************************************************************************/
int vars_block_free(vars_object_t *vars, void *p)
{
    int ret = 0;

    if((p > vars->data) && (p < vars->data+vars->data_size))
    {
        ret = shm_slab_free(vars->pool, p);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Free memory failed! [%p] [%p] [%p]",
                __FILE__, __LINE__, vars->data, vars->data + vars->data_size, p);
            return -1;
        }
        return 0;
    }

#if defined(__VARS_EXP_FILE__)
    return vars_exp_block_free(vars, p);
#else /*__VARS_EXP_FILE__*/
    pub_log_error("[%s][%d] Free memory failed! [%p]", __FILE__, __LINE__, p);
    return -1;
#endif /*__VARS_MEM_ALLOC_SLAB__*/
}
#endif /*__VARS_MEM_ALLOC_SLAB__*/

#if !defined(__VARS_MEM_ALLOC_SLAB__)
/******************************************************************************
 **��������: vars_node_best_unused
 **��    ��: ��ȡ���ʺϵ�δʹ�õĽڵ� [���ƥ���㷨]
 **�������: 
 **     node: �����ض���
 **     best: ��ѡ������ʺϵĽڵ�
 **     size: ����Ŀռ��С
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     ʹ��[���ƥ���㷨]����һ����߿ռ�������
 **ע������: 
 **     1. ��best�������sizeСʱ
 **         1.1 ���node��best��Сʱ����ѡ��node�ڵ�
 **         1.2 ���node��size����ʱ����ѡ��node�ڵ�
 **     2. ��best�������size��ʱ
 **         2.1 ���node��best��Сʱ������size��ʱ����ѡ��node�ڵ�
 **         2.2 ���node��size��Сʱ����ѡ��best�ڵ�
 **��    ��: # Qifeng.zou # 2013.05.06 #
 ******************************************************************************/
static vars_node_t *vars_node_best_unused(vars_node_t *node, vars_node_t *best, size_t size)
{
    if(NULL == best)
    {
        return node;
    }

    if(size == best->size)
    {
        return best;        /* �Ѿ��ҵ�������ʵĽڵ� */
    }

    /* 1. ��best�������sizeСʱ */
    if(best->size < size)
    {
        if((node->size >= size)
            || (node->size < best->size))
        {
            return node;
        }

        return best;
    }

    /* 2. ��best�������size��ʱ */
    if((node->size < best->size) && (node->size >= size))
    {
        return node;
    }

    return best;
}
#endif /*__VARS_MEM_ALLOC_SLAB__*/

/******************************************************************************
 **��������: vars_set_revariable
 **��    ��: ���ñ�����Ϣ
 **�������: 
 **     vars: �����ض��� 
 **     name: ������
 **     type: �������� VAR_TYPE_CHAR-�ַ�  VAR_TYPE_BIN-������
 **     value: ����ֵ
 **     length: ����ֵ����
 **     index: �ظ���������
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **  1) �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) ���������صĽṹ������ʾ:
 **     | �ļ� |ƫ����|<---    MOD�������ؽڵ�    --->|<--         ���ݿ�        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            ���ݿ�           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_set_revariable(vars_object_t *vars,
    const char *name, int type, const char *value, int length, int index)
{
    int mod=0;
    vars_node_t *node = NULL,   /* �����ؽڵ��ַ */
                *new_node = NULL,   /* �½��ڵ� */
                *best = NULL;   /* ���нڵ� */
    void *dst_value = NULL;
    size_t offset = 0;
    size_t size = 0;

    /* �������Ϸ����ж� */
    if(!VarsIsNameValid(name))
    {
        return -1;
    }
    
    mod = vars_get_node_mod(name);
    node = &vars->head->node[mod];
    size = length+1;

    /* 1. ����ڵ�ռ�[֧�ֿ��ظ�����] */
    do
    {
    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.1 δʹ��ʱ��������һ���ڵ� */
        if(!VarsIsNodeUsed(node))
        {
            best = vars_node_best_unused(node, best, size);
            
            if(0 != node->next)
            {
                node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
                continue;
            }

            node = best;
            break;
        }
    #endif /*__VARS_MEM_ALLOC_SLAB__*/

        /* 1.2 �Ƿ�����ҵ�nameһ�� */
        if((index == node->index)
            && (0 == strcmp(name, node->name)))
        {
            break;  /* �ҵ�name�ڵ� */
        }

        /* 1.3 �Ƿ�����һ���ڵ� */
        if(0 != node->next)
        {
            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
            continue;
        }

    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.4 ���нڵ������� */
        if(NULL != best)
        {
            node = best;
            break;
        }
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        
        /* 1.5 �����½ڵ�ռ� */
        new_node = (vars_node_t *)vars_block_alloc(vars, sizeof(vars_node_t), &offset);
        if(NULL == new_node)
        {
            pub_log_error("[%s][%d] Alloc block failed!", __FILE__, __LINE__);
            return -1;
        }

        node->next = offset;
        node = new_node;
        break;
    }while(1);

    /* 2. ���ýڵ�������Ϣ */
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->type = type;
    
    if(node->size < size)
    {
        dst_value = vars_block_alloc(vars, size, &offset);
        if(NULL == dst_value)
        {
            pub_log_error("[%s][%d] Alloc block failed!", __FILE__, __LINE__);
            return -1;
        }
        
        node->value = offset;
        node->size = size;
    }
    else
    {
        dst_value = vars_offset_to_addr(vars, node->value);
	memset(dst_value, 0x0, node->size);
    }
    
    memcpy(dst_value, value, length);

    node->length = length;
    node->index = index;

    return 0;
}

/******************************************************************************
 **��������: vars_get_null
 **��    ��: �ӱ�����������̶������ڴ�
 **�������: 
 **     vars: �����ض��� 
 **     name: �ڴ����
 **     length: �ڴ�鳤��
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **  1) �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) ���������صĽṹ������ʾ:
 **     | �ļ� |ƫ����|<---    MOD�������ؽڵ�    --->|<--         ���ݿ�        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            ���ݿ�           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_char_t *vars_get_null(vars_object_t *vars, const char *name, int length)
{
    int mod=0;
    vars_node_t *node = NULL,   /* �����ؽڵ��ַ */
                *new_node = NULL,   /* �½��ڵ� */
                *best = NULL;   /* ���нڵ� */
    void *dst_value = NULL;
    size_t offset = 0;
    size_t size = 0;

    /* �������Ϸ����ж� */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Var name is invalid! name:%s", __FILE__, __LINE__, name);
        return NULL;
    }
    
    mod = vars_get_node_mod(name);
    node = &vars->head->node[mod];
    size = length+1;

    /* 1. ����ڵ�ռ�[֧�ֿ��ظ�����] */
    do
    {
    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.1 δʹ��ʱ��������һ���ڵ� */
        if(!VarsIsNodeUsed(node))
        {
            best = vars_node_best_unused(node, best, size);
            
            if(0 != node->next)
            {
                node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
                continue;
            }

            node = best;
            break;
        }
    #endif /*__VARS_MEM_ALLOC_SLAB__*/

        /* 1.2 �Ƿ�����ҵ�nameһ�� */
        if((0 == node->index)
            && (0 == strcmp(name, node->name)))
        {
            break;  /* �ҵ�name�ڵ� */
        }

        /* 1.3 �Ƿ�����һ���ڵ� */
        if(0 != node->next)
        {
            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
            continue;
        }

    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.4 ���нڵ������� */
        if(NULL != best)
        {
            node = best;
            break;
        }
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        
        /* 1.5 �����½ڵ�ռ� */
        new_node = (vars_node_t *)vars_block_alloc(vars, sizeof(vars_node_t), &offset);
        if(NULL == new_node)
        {
            return NULL;
        }

        node->next = offset;
        node = new_node;
        break;
    }while(1);

    /* 2. ���ýڵ�������Ϣ */
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->type = VAR_TYPE_BIN;
    
    if(node->size < size)
    {
        dst_value = vars_block_alloc(vars, size, &offset);
        if(NULL == dst_value)
        {
            pub_log_error("[%s][%d] Alloc block failed!", __FILE__, __LINE__);
            return NULL;
        }
        
        node->value = offset;
        node->size = size;
    }
    else
    {
        dst_value = vars_offset_to_addr(vars, node->value);
    }
    

    node->length = length;
    node->index = 0;

    return dst_value;
}

/******************************************************************************
 **��������: vars_get_revariable
 **��    ��: ��ȡ����ֵ
 **�������: 
 **     vars: �����ض��� 
 **     name: ������
 **     index: �ظ���������
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **  1) �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) ���������صĽṹ������ʾ:
 **     | �ļ� |ƫ����|<---    MOD�������ؽڵ�    --->|<--         ���ݿ�        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            ���ݿ�           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
const void *vars_get_revariable(vars_object_t *vars, const char *name, int index, int *length)
{
    unsigned int mod = 0;
    vars_node_t *node = NULL;   /* �����ؽڵ��ַ */


    /* �������Ϸ����ж� */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Var name is invaliled! [%s]", __FILE__, __LINE__, name);
        return NULL;
    }

    mod = vars_get_node_mod(name);
    node = (vars_node_t *)(vars->head->node + mod);

    /* 1. ����ָ���Ľڵ� */
    do
    {
        if(!VarsIsNodeUsed(node)
            || (index != node->index)
            || (0 != strcmp(name, node->name)))
        {
            if(0 == node->next)
            {
                return NULL;    /* ����ʧ�� */
            }

            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
            if(NULL == node)
            {
                return NULL;
            }
            
            continue;
        }

        *length = node->length;
        
        return vars_offset_to_addr(vars, node->value);
    }while(1);

    return NULL;
}


/******************************************************************************
 **��������: vars_delete_revariable
 **��    ��: ɾ��ָ������
 **�������: 
 **     vars: �����ض��� 
 **     name: ������
 **     index: �ظ���������
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **  1) �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) ���������صĽṹ������ʾ:
 **     | �ļ� |ƫ����|<---    MOD�������ؽڵ�    --->|<--         ���ݿ�        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            ���ݿ�           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_delete_revariable(vars_object_t *vars, const char *name, int index)
{
    unsigned int mod = 0;
    vars_node_t *head = NULL, *node = NULL;   /* �����ؽڵ��ַ */
    void *value = NULL;
#if defined(__VARS_MEM_ALLOC_SLAB__)
    int ret = 0;
    vars_node_t *prev = NULL;
    shm_slab_pool_t *pool = NULL;
#endif /*__VARS_MEM_ALLOC_SLAB__*/

    /* 1. �������Ϸ����ж� */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Vars name is invalied! [%s]", __FILE__, __LINE__, name);
        return -1;
    }

    mod = vars_get_node_mod(name);
    head = (vars_node_t *)(vars->head->node + mod);
    node = head;
    
    /* 2. ���Ҳ�ɾ��ָ���Ľڵ� */
    do
    {
        if(!VarsIsNodeUsed(node)
            || (index != node->index)
            || (0 != strcmp(name, node->name)))
        {
            if(0 == node->next)
            {
                return -1;    /* ����ʧ�� */
            }
        #if defined(__VARS_MEM_ALLOC_SLAB__)
            prev = node;
        #endif /*__VARS_MEM_ALLOC_SLAB__*/
            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);    /* ������һ�� */
            continue;
        }

    #if defined(__VARS_MEM_ALLOC_SLAB__)
        value = vars_offset_to_addr(vars, node->value);
        ret = vars_block_free(vars, value);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Free memory failed! [%p]:%s",
                __FILE__, __LINE__, value, value);
            return -1;
        }
        if(head != node)
        {
	        prev->next = node->next;
            ret = vars_block_free(vars, node);
            if(0 != ret)
            {
                pub_log_error("[%s][%d] Free memory failed! [%p]",
                    __FILE__, __LINE__, node);
                return -1;
            }
        }
        else
        {
            memset(node->name, 0, sizeof(node->name));
            node->type = 0;
            node->length = 0;
            node->index = 0;
            node->size = 0;
        }
    #else /*__VARS_MEM_ALLOC_SLAB__*/
        /* ��ձ�������ֵ�� [ע:���޸�value��size��next��ֵ] */
        memset(node->name, 0, sizeof(node->name));
        node->type = 0;
        value = vars_offset_to_addr(vars, node->value);
        memset(value, 0, node->size);
        node->length = 0;
        node->index = 0;
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        return 0;
    }while(1);

    return -1;
}

/******************************************************************************
 **��������: vars_get_field_len
 **��    ��: ��ȡ��������
 **�������: 
 **     vars: �����ض��� 
 **     name: ������
 **     index: �ظ���������
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **  1) �����ڴ�Ľṹ������ʾ:
 **     |<--  ������  -->|<--                       �����ض�                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      ������    |                          �����ض�                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) ���������صĽṹ������ʾ:
 **     | �ļ� |ƫ����|<---    MOD�������ؽڵ�    --->|<--         ���ݿ�        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            ���ݿ�           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **��    ��: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int32_t vars_get_field_len(vars_object_t *vars, const char *name, int index)
{
    unsigned int mod = 0;
    vars_node_t *node = NULL;   /* �����ؽڵ��ַ */
    sw_int32_t	len = -1;


    /* 1. �������Ϸ����ж� */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Vars name is invalied! [%s]", __FILE__, __LINE__, name);
        return -1;
    }

    mod = vars_get_node_mod(name);
    node = (vars_node_t *)(vars->head->node + mod);
    
    /* 2. ���Ҳ�ɾ��ָ���Ľڵ� */
    do
    {
        if(!VarsIsNodeUsed(node)
            || (index != node->index)
            || (0 != strcmp(name, node->name)))
        {
            if(0 == node->next)
            {
                return 0;    /* ����ʧ�� */
            }

            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);   /* ������һ�� */
            continue;
        }

        /*��ȡֵ����*/
        len = node->length;
        
        return len;
    }while(1);

    return -1;
}


#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
/******************************************************************************
 **��������: vars_offset_to_addr
 **��    ��: ƫ����ת��Ϊ��ַ
 **�������:
 **      vars: �����ض���
 **      offset: ƫ����
 **�������:
 **��    ��: ���ݵ�ַ
 **ʵ������: 
 **      1. �ڹ����ڴ���ʱ����ַ=���ݿ���ʼ��ַ+ƫ����
 **      2. ����չ��ʱ����ַ=vars_exp_offset_to_addr()
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
void *vars_offset_to_addr(const vars_object_t *vars, size_t offset)
{
    /* 1. �ڹ����ڴ���ʱ����ַ=���ݿ���ʼ��ַ+ƫ���� */
    if(offset < vars->data_size)
    {
        return ((char *)vars->data + offset); /* �ڹ����ڴ��� */
    }

    /* 2. ����չ�ռ��м��� */
    return vars_exp_offset_to_addr(vars, offset);
}
#endif /*__VARS_EXP_FILE__ || __VARS_EXP_SHM__*/

/******************************************************************************
 **��������: vars_print_link
 **��    ��: ��ӡ���������нڵ�
 **�������:
 **      vars: �����ض���
 **�������:
 **��    ��: 0:success !0:failed
 **ʵ������: 
 **     ͨ��head���α��������и��ڵ㣬�������ӡ
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
sw_int_t vars_print_link(const vars_object_t *vars, const vars_node_t *head)
{
    size_t idx = 0;
    sw_int_t offset=0;
    const char *value = NULL;
    const vars_node_t *node = head;
    char buff[VARS_PRINT_MAX_LEN] = {0};

    while(NULL != node)
    {
        if(!VarsIsNodeUsed(node))
        {
        #if defined(__VARS_MEM_ALLOC_SLAB__)
            if(node != head)
            {
                return 0;
            }
        #endif /*__VARS_MEM_ALLOC_SLAB__*/
        
            if(0 == node->next)
            {
                return 0;
            }
            node = vars_offset_to_addr(vars, node->next);
            continue;
        }

        value = vars_offset_to_addr(vars, node->value);

        if(VAR_TYPE_CHAR == node->type)
        {
            pub_log_info("[%s] = [%s][%d]", node->name, value, node->length);
        }
        else /* VAR_TYPE_BIN */
        {
            pub_log_info("[%s] = [", node->name);
            for(idx=0; idx<node->length; idx++)
            {
                snprintf(buff+offset, sizeof(buff)-offset, "%02x ",value[idx]);
                offset += 3;
                
                if(0 == (idx+1)%16)
                {
                    pub_log_info("%s", buff);
                    
                    memset(buff, 0, sizeof(buff));
                    offset = 0;
                }
            }
            pub_log_info("%s]", buff);
        }

        if(0 == node->next)
        {
            return 0;
        }

        node = vars_offset_to_addr(vars, node->next);
    }
    return 0;
}

/******************************************************************************
 **��������: vars_print
 **��    ��: ��ӡָ���������е����б���
 **�������:
 **      vars: �����ض���
 **�������:
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.06.22 #
 ******************************************************************************/
sw_int_t vars_print(const vars_object_t *vars)
{
    int idx = 0;
	
    for(idx=0; idx<VARS_NODE_NUM; idx++)
    {
        vars_print_link(vars, &(vars->head->node[idx]));
    }
    return 0;
}

/******************************************************************************
 **��������: vars_serial_link
 **��    ��: ���л����������нڵ�
 **�������:
 **     vars: �����ض���
 **     head: ͷ���
 **     buflen: ���л��������ĳ���
 **�������:
 **     buf: ���л�������
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **     ͨ��head���α��������и��ڵ㣬���������л�
 **ע������: 
 **     format: !%02d%s%04d%c%d#XXXXXXXXXXXXX
 **��    ��: # Qifeng.zou # 2013.08.08 #
 ******************************************************************************/
size_t vars_serial_link(const vars_object_t *vars,
    const vars_node_t *head, vars_serial_t *serial)
{
    size_t left = 0;
    sw_int_t length = 0;
    const char *value = NULL;
    const vars_node_t *node = head;

    while(NULL != node)
    {
        if(!VarsIsNodeUsed(node) || strcmp(node->name, "$$REOUTE") == 0)
        {
        #if defined(__VARS_MEM_ALLOC_SLAB__)
            if(node != head)
            {
                return 0;
            }
        #endif /*__VARS_MEM_ALLOC_SLAB__*/

            if(0 == node->next)
            {
                return 0;
            }
            node = vars_offset_to_addr(vars, node->next);
            continue;
        }

        value = vars_offset_to_addr(vars, node->value);

        left = serial->buflen - serial->offset;
        length = snprintf(serial->buf + serial->offset, left,
                    "!%02zd%s%04d%c%zd#",
                    strlen(node->name), node->name,
                    node->index, node->type, node->length);
        
        serial->offset += length;
        if(serial->offset >= serial->buflen)
        {
            pub_log_error("[%s][%d] Buffer space is not enough!", __FILE__, __LINE__);
            return -1;
        }
        
        if(VAR_TYPE_CHAR == node->type)
        {
            left = serial->buflen - serial->offset;
            if(left < node->length)
            {
                pub_log_error("[%s][%d] Buffer space is not enough!", __FILE__, __LINE__);
                return -1;
            }
            
            length = snprintf(serial->buf + serial->offset, left, "%s", value);
            serial->offset += length;
            if(serial->offset > serial->buflen)
            {
                pub_log_error("[%s][%d] Buffer space is not enough!", __FILE__, __LINE__);
                return -1;
            }
        }
        else /* VAR_TYPE_BIN */
        {
            left = serial->buflen - serial->offset;
            if(left < node->length)
            {
                pub_log_error("[%s][%d] Buffer space is not enough!", __FILE__, __LINE__);
                return -1;
            }
            
            memcpy(serial->buf + serial->offset, value, node->length);
            serial->offset += node->length;
            if(serial->offset > serial->buflen)
            {
                pub_log_error("[%s][%d] Buffer space is not enough!", __FILE__, __LINE__);
                return -1;
            }
         }

        if(0 == node->next)
        {
            return 0;
        }

        node = vars_offset_to_addr(vars, node->next);
    }
    return 0;
}

/******************************************************************************
 **��������: vars_serial
 **��    ��: ���л�ָ���������е����б���
 **�������:
 **     vars: �����ض���
 **     buflen: ���л�����������
 **�������:
 **     buf: ���л�������
 **��    ��: >=0: ���ĳ���  <0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.08 #
 ******************************************************************************/
sw_int_t vars_serial(const vars_object_t *vars, char *buf, size_t buflen)
{
    sw_int_t idx = 0, ret = 0;
    vars_serial_t serial;

    memset(&serial, 0, sizeof(serial));

    serial.buf = buf;
    serial.buflen = buflen;
    serial.offset = 0;

    for(idx=0; idx<VARS_NODE_NUM; idx++)
    {
        ret = vars_serial_link(vars, &(vars->head->node[idx]), &serial);
        if(ret < 0)
        {
            pub_log_error("[%s][%d] Vars serial failed! [%d]", __FILE__, __LINE__, idx);
            return ret;
        }
    }
        
    return serial.offset;
}

/******************************************************************************
 **��������: vars_unserial
 **��    ��: ��buf�е����ݽ��з����л�����
 **�������:
 **     vars: �����ض���
 **     buf: �豻�����л����ִ�
 **�������:
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **     �������л��ĸ�ʽ���ַ��������л�����������
 **ע������: 
 **     format: !%02d%s%04d%c%d#XXXXXXXXXXXXX
 **��    ��: # Qifeng.zou # 2013.08.08 #
 ******************************************************************************/
static sw_int_t vars_unserial_parse(vars_object_t *vars, vars_unserial_t *uns);
sw_int_t vars_unserial(vars_object_t *vars, const char *buf)
{
    sw_int_t ret = 0;
    vars_unserial_t uns;

    memset(&uns, 0, sizeof(uns));

    uns.buf = buf;
    uns.ptr = buf;
    
    while('\0' != *uns.ptr)
    {
        while(' ' == *uns.ptr)
        {
            uns.ptr++;
        }

        switch(*uns.ptr)
        {
            case '\0':
            {
                return 0;
            }
            case '!':
            {
                uns.ptr++;
                ret = vars_unserial_parse(vars, &uns);
                if(0 != ret)
                {
                    pub_log_error("[%s][%d] Unserial failed!", __FILE__, __LINE__);
                    return -1;
                }
                break;
            }
            default:
            {
                pub_log_error("[%s][%d] Format is incorrect!", __FILE__, __LINE__);
                return -1;
            }
        }
    }
    
    return 0;
}

/******************************************************************************
 **��������: vars_unserial_get_digit
 **��    ��: �����л��ִ��л�ȡ����
 **�������:
 **     ptr: �����л��ַ���
 **     length: ��ȡ���ֵĳ���
 **�������:
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **     1. �ж��ִ��Ϸ���
 **     2. ��ȡ����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.09 #
 ******************************************************************************/
static int vars_unserial_get_digit(vars_unserial_t *uns, int length, int *digit)
{
    sw_int_t idx = 0;
    const char *p = uns->ptr;
    char digit_str[VARS_SERAIL_DIGIT_MAX_LEN] = {0};

    for(idx=0; idx<length; idx++)
    {
        if(!isdigit(*p))
        {
            pub_log_error("[%s][%d] Format is incorrect!", __FILE__, __LINE__);
            return -1;
        }
    }
    
    memcpy(digit_str, uns->ptr, length);

    *digit = atoi(digit_str);
    uns->ptr += length;
    return 0;
}

/******************************************************************************
 **��������: vars_unserial_get_name
 **��    ��: �����л��ִ��л�ȡ������
 **�������:
 **     ptr: �����л��ַ���
 **     name_len: �������ĳ���
 **�������:
 **     name: ����������
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **     1. �ж��ִ��Ϸ���
 **     2. ��ȡ����������
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.09 #
 ******************************************************************************/
static int vars_unserial_get_name(vars_unserial_t *uns, int length, char *name)
{
    int idx = 0;
    const char *p = uns->ptr;

    for(idx=0; idx<length; idx++)
    {
        if('\0' == *p)
        {
            pub_log_error("[%s][%d] Format is incorrect!", __FILE__, __LINE__);
            return -1;
        }
        p++;
    }
    
    memcpy(name, uns->ptr, length);

    uns->ptr += length;
    return 0;
}

/******************************************************************************
 **��������: vars_unserial_get_length
 **��    ��: �����л��ִ��л�ȡ����ֵ�ĳ���
 **�������:
 **     ptr: �����л��ַ���
 **     length: ��ȡ���ֵĳ���
 **�������:
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **     1. �ж��ִ��Ϸ���
 **     2. ��ȡ����ֵ����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.09 #
 ******************************************************************************/
static int vars_unserial_get_length(vars_unserial_t *uns, size_t *length)
{
    const char *p = uns->ptr;
    char digit[VARS_SERAIL_DIGIT_MAX_LEN] = {0};

    while(isdigit(*uns->ptr))
    {
        uns->ptr++;
    }
    
    if('#' != *uns->ptr)
    {
        pub_log_error("[%s][%d] Format is incorrect!", __FILE__, __LINE__);
        return -1;       
    }
    
    memcpy(digit, p, uns->ptr - p);
    *length = (size_t)atoi(digit);
    uns->ptr++;
    return 0;
}

/******************************************************************************
 **��������: vars_unserial_parse
 **��    ��: �����л��ִ�
 **�������:
 **     vars: ���������ö���
 **     ptr: �������л����ִ�
 **�������:
 **��    ��: 0:Success !0:Failed
 **ʵ������: 
 **     1. ��ȡ����������
 **     2. ��ȡ������
 **     3. ��ȡ��������
 **     4. ��ȡ����ֵ����
 **     5. ��ȡ����ֵ����
 **     6. ��ȡ����ֵ
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.08.09 #
 ******************************************************************************/
static sw_int_t vars_unserial_parse(vars_object_t *vars, vars_unserial_t *uns)
{
    vars_node_t node;
    int ret = 0, name_len = 0;
    char *value = NULL, slot[VARS_SERIAL_SLOT_LEN] = {0};

    memset(&node, 0, sizeof(node));
    memset(slot, 0, sizeof(slot));

    /* 1. Get length of name */
    ret = vars_unserial_get_digit(uns, VARS_SERAIL_NAME_LEN, &name_len);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Get length of vars name failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 2. Get name of var */
    ret = vars_unserial_get_name(uns, name_len, node.name);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Get name of vars failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 3. Get index of value */
    ret = vars_unserial_get_digit(uns, VARS_SERAIL_INDEX_LEN, &node.index);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Get index of vars failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 4. Get type of value */
    node.type = (int)*uns->ptr;
    uns->ptr++;
    
    /* 5. Get length of value */
    ret = vars_unserial_get_length(uns, &node.length);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Get the length of value failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 6. Get the value of var */
    if(sizeof(slot) >= node.length + 1)
    {
        value = slot;
    }
    else
    {
        value = calloc(1, node.length + 1);
        if(NULL == value)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            return -1;
        }
    }

    memcpy(value, uns->ptr, node.length);
    uns->ptr += node.length;

    vars_set_revariable(vars, node.name, node.type, value, node.length, node.index);
    
    if(value != slot)
    {
        free(value), value = NULL;
    }

    return 0;
}

#if defined(__VARS_EXP_FILE__)
/******************************************************************************
 **��������: vars_exp_dir_creat
 **��    ��: ������������չ�ռ�Ŀ¼
 **�������: NONE
 **�������: NONE
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.10 #
 ******************************************************************************/
static void vars_exp_dir_creat(void)
{
    char fdir[FILE_NAME_MAX_LEN] = {0};

    vars_exp_get_fdir(fdir, sizeof(fdir));
    
    mkdir(fdir, VARS_DIR_MODE);
}

/******************************************************************************
 **��������: vars_exp_creat
 **��    ��: ����һ����չ�ռ�(�ļ������)
 **�������:
 **      size: ����ռ�Ĵ�С
 **      start_offset: �ļ������ʼƫ����
 **�������:
 **��    ��: ������ļ���ռ��ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
static vars_fblk_t *vars_exp_creat(int start_offset, int size)
{
    int nblock = 0, total_size = 0;
    vars_fblk_t *fblk = NULL;


    /* 1. �����ļ���ڵ� */
    fblk = (vars_fblk_t *)calloc(1, sizeof(vars_fblk_t));
    if(NULL == fblk)
    {
        pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
        return NULL;
    }

    nblock = size/vars_get_page_size();
    if(0 != size%vars_get_page_size())
    {
        nblock++;
    }

    total_size = nblock * vars_get_page_size();
#if defined(__VARS_MEM_ALLOC_SLAB__)
    total_size += shm_slab_head_size(total_size);
#endif
    /* 2. �����ļ����� */
    fblk->fbuff = calloc(1, total_size);
    if(NULL == fblk->fbuff)
    {
        pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
        free(fblk), fblk=NULL;
        return NULL;
    }

    fblk->head.size = total_size;
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    fblk->head.offset = 0;
#endif /*__VARS_MEM_ALLOC_SLAB__*/
    fblk->head.start_offset = start_offset;
    fblk->head.end_offset = start_offset + fblk->head.size - 1;
    fblk->next = NULL;
    vars_reset_dirty(fblk);

    return fblk;
}

/******************************************************************************
 **��������: vars_exp_destory
 **��    ��: ������չ�ռ�(��չ�ļ�)
 **�������:
 **      vars: ������ʵ������
 **�������:
 **��    ��: 0: �ɹ�  !0: ʧ��
 **ʵ������: 
 **     ��ָ���ļ����Ƚض�Ϊ��
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
static sw_int_t vars_exp_destory(vars_object_t *vars)
{
    char fname[FILE_NAME_MAX_LEN] = {0};

    if(!vars_get_fflag(vars))
    {
        return 0;
    }

    vars_exp_get_fname(vars->vid, fname, sizeof(fname));
    truncate(fname, 0);    
    return 0;
}

/******************************************************************************
 **��������: vars_exp_link_free
 **��    ��: �ͷ���չ����(�ļ�����)
 **�������:
 **      fblk: �ļ���������ͷ
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
static sw_int_t vars_exp_link_free(vars_fblk_t *head)
{
    vars_fblk_t *fblk=NULL, *next=NULL;

    fblk = head;
    while(NULL != fblk)
    {
        next = fblk->next;
        vars_exp_free(fblk);
        fblk = next;
    }
    
    return 0;
}

/******************************************************************************
 **��������: vars_exp_block_alloc
 **��    ��: ����չ�ռ�(�ļ�����)�������ڴ�
 **�������:
 **      vars: �����ض���
 **      size: ����ռ�Ĵ�С
 **�������:
 **      offset: ������ݿ���ʼλ�õ�ƫ����
 **��    ��: ����Ŀռ��ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
static void *vars_exp_block_alloc(vars_object_t *vars, size_t size, size_t *offset)
{
    void *addr = NULL;
    vars_fblk_t *fblk = NULL,
                    *fblk2 = NULL;
#if defined(__VARS_MEM_ALLOC_SLAB__)
    int ret = 0;
    shm_slab_pool_t *pool = NULL;
#endif /*__VARS_MEM_ALLOC_SLAB__*/

    fblk = vars->fblk;
    if(NULL == fblk)
    {
        fblk = vars_exp_creat(vars->data_size, size);
        if(NULL == fblk)
        {
            return NULL;
        }

        vars->fblk = fblk;
    #if defined(__VARS_MEM_ALLOC_SLAB__)
        pool = (shm_slab_pool_t *)vars->fblk->fbuff;
        pool->pool_size = vars->fblk->head.size;
        ret = shm_slab_init(pool);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Slab init failed!", __FILE__, __LINE__);
            return NULL;
        }

        vars->fblk->pool = pool;
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
    }

    while(NULL != fblk)
    {        
        /* 1. �жϵ�ǰ�ļ������ռ��Ƿ��㹻 */
    #if defined(__VARS_MEM_ALLOC_SLAB__)
        pub_log_error("[%s][%d] size:%d\n", __FILE__, __LINE__, size);
        addr = shm_slab_alloc(fblk->pool, size);
        if(NULL == addr)
    #else /*__VARS_MEM_ALLOC_SLAB__*/
        if((fblk->head.size - fblk->head.offset) < size)
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        {
            if(NULL != fblk->next)
            {
                fblk = fblk->next;
                continue;
            }

            fblk2 = vars_exp_creat(fblk->head.end_offset+1, size);
            if(NULL == fblk2)
            {
                return NULL;
            }
            
            fblk->next = fblk2;
            fblk = fblk->next;
        #if defined(__VARS_MEM_ALLOC_SLAB__)
            pool = (shm_slab_pool_t *)fblk->fbuff;
            pool->pool_size = fblk->head.size;
            ret = shm_slab_init(pool);
            if(0 != ret)
            {
                return NULL;
            }

            fblk->pool = pool;
        #endif /*__VARS_MEM_ALLOC_SLAB__*/
            continue;
        }

    #if defined(__VARS_MEM_ALLOC_SLAB__)
        *offset = fblk->head.start_offset + (size_t)(addr - (void *)fblk->fbuff);
    #else /*__VARS_MEM_ALLOC_SLAB__*/
        addr = (fblk->fbuff + fblk->head.offset);
        *offset = (fblk->head.start_offset + fblk->head.offset);
        fblk->head.offset += size;
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        vars_set_dirty(fblk);
        vars_set_fflag(vars);
        return addr;
    }

    return NULL;
}

#if defined(__VARS_MEM_ALLOC_SLAB__)
/******************************************************************************
 **��������: vars_exp_block_free
 **��    ��: �ͷ���չ��չ�ռ�(�ļ�����)���ڴ��ռ�
 **�������:
 **      vars: �����ض���
 **      p: ���ͷŵ��ڴ���ʼ��ַ
 **�������:
 **��    ��: 0:Success  !0:Failed
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.07.18 #
 ******************************************************************************/
int vars_exp_block_free(vars_object_t *vars, void *p)
{
    int ret = 0;
    vars_fblk_t *fblk = NULL;
    
    fblk = vars->fblk;
    while(NULL != fblk)
    {
        if((p > (void *)fblk->fbuff) && (p < (void *)(fblk->fbuff + fblk->head.size)))
        {
            ret = shm_slab_free(fblk->pool, p);
            if(0 != ret)
            {
                pub_log_error("[%s][%d] Free memory failed! [%p] [%p] [%p]",
                    __FILE__, __LINE__, 
                    fblk->fbuff, fblk->fbuff + fblk->head.size, p);
                return -1;
            }

            pub_log_error("[%s][%d] Free memory success! [%p]", __FILE__, __LINE__, p);
            return 0;
        }
        fblk = fblk->next;
    }

    pub_log_error("[%s][%d] Free memory failed! [%p]", __FILE__, __LINE__, p);
    return -1;
}

#endif /*__VARS_MEM_ALLOC_SLAB__*/

/******************************************************************************
 **��������: vars_exp_offset_to_addr
 **��    ��: ����չ�ռ�(�ļ�����)ƫ����ת��Ϊ�ڴ��ַ
 **�������:
 **      vars: �����ض���
 **      offset: ƫ����(������ݿ��׵�ַ)
 **�������:
 **��    ��: ���ݵ�ַ
 **ʵ������: 
 **  1) �ļ�����������ʾ:
 **     |<--                             �ļ�����                                -->|
 **     -----------------------------------------------------------------------------
 **     |////////////////////////////|                                              |
 **     |\\\\\\\\\\\\\\\\\\\\\\\\\\\\|<--                δʹ�ÿռ�              -->|
 **     |////////////////////////////|                                              |
 **     -----------------------------------------------------------------------------
 **     ^                            ^                                              ^
 **     |                            |                                              |
 ** start_offset                   offset(���start_offset)                     end_offset
 **ע������: 
 **  1) start_offset, offset, end_offset: ��ֵ��������ݿ���ʼ��ַ��ƫ����
 **��    ��: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
static void *vars_exp_offset_to_addr(const vars_object_t *vars, size_t offset)
{
    const vars_fblk_t *fblk = NULL;

    fblk = vars->fblk;

    while(NULL != fblk)
    {
        if((offset >= fblk->head.start_offset)
            && (offset <= fblk->head.end_offset))
        {
            return (fblk->fbuff + (offset - fblk->head.start_offset));
        }

        fblk = fblk->next;
    }

    return NULL;
}

/******************************************************************************
 **��������: vars_sync
 **��    ��: ���ļ������е�����ͬ��������
 **�������:
 **      vars: �����ض���
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
sw_int_t vars_sync(vars_object_t *vars)
{
    int index = 0;
    int ret=0, offset=0;
    FILE *fp = NULL;
    vars_fblk_t *fblk = NULL;
    char fname[FILE_NAME_MAX_LEN] = {0};

    if(!vars_get_fflag(vars))
    {
        return 0;
    }

    vars_exp_get_fname(vars->vid, fname, sizeof(fname));
    
    fp = fopen(fname, "wb+");
    if(NULL == fp)
    {
        pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
            __FILE__, __LINE__, errno, strerror(errno), fname);
        return -1;
    }

    fblk = vars->fblk;
    while(NULL != fblk)
    {
        if(1 || vars_get_dirty(fblk)) /* ��ʱ��Ϊȫ����д�ļ� */
        {
            offset = (fblk->head.start_offset - vars->data_size)
                        + (index * sizeof(vars_fblk_head_t));
            fseek(fp, offset, SEEK_SET);

            /* 1. д�ļ���ͷ */
            ret = fwrite(&fblk->head, sizeof(vars_fblk_head_t), 1, fp);
            if(0 == ret)
            {
                pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
                    __FILE__, __LINE__, errno, strerror(errno), fname);
                fclose(fp), fp=NULL;
                return ret;
            }
            
            /* 2. д�ļ��黺�� */
            ret = fwrite(fblk->fbuff, fblk->head.size, 1, fp);
            if(0 == ret)
            {
                pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
                    __FILE__, __LINE__, errno, strerror(errno), fname);
                fclose(fp), fp=NULL;
                return ret;
            }
            
            vars_reset_dirty(fblk);
        }

        index++;
        fblk = fblk->next;
    }

	fclose(fp);
	fp = NULL;
    return 0;
}

/******************************************************************************
 **��������: vars_exp_load
 **��    ��: ����������չ���ݿ������ڴ�
 **�������:
 **      vars: �����ض���
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     1. �Ƿ��б������浽�ļ���
 **     2. ��ȡ�ļ��������ж��ļ�״̬
 **     3. ���������ļ��ڵ�
 **     4. ���ļ������ڴ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.09 #
 ******************************************************************************/
static sw_int_t vars_exp_load(vars_object_t *vars)
{
    int ret = 0;
    FILE *fp = NULL;
    struct stat file_stat;
    vars_fblk_t *fblk = NULL, *new_fblk = NULL;
    vars_fblk_head_t *head = NULL;
    char fname[FILE_NAME_MAX_LEN] = {0};

    memset(&file_stat, 0, sizeof(file_stat));

    /* 1. �Ƿ��б������浽�ļ��� */
    if(!vars_get_fflag(vars))
    {
        vars->fblk = NULL;
        return 0;
    }

    /* 2. ��ȡ�ļ��������ж��ļ�״̬ */
    vars_exp_get_fname(vars->vid, fname, sizeof(fname));

    ret = stat(fname, &file_stat);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Get file stat information failed!", __FILE__, __LINE__);
        return -1;
    }

    if(0 == file_stat.st_size)
    {
        return 0;
    }

    /* 3. ���ļ������ڴ� */
    fp = fopen(fname, "r");
    if(NULL == fp)
    {
        pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
            __FILE__, __LINE__, errno, strerror(errno), fname);
        return -1;
    }

    while(!feof(fp))
    {
        new_fblk = (vars_fblk_t *)calloc(1, sizeof(vars_fblk_t));
        if(NULL == new_fblk)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            return -1;
        }

        head = &new_fblk->head;

        /* 1. ��ȡ�ļ���ͷ */
        ret = fread(head, sizeof(vars_fblk_head_t), 1, fp);
        if(ferror(fp))
        {
            vars_exp_link_free(fblk);
            vars->fblk = NULL;
            pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
                __FILE__, __LINE__, errno, strerror(errno), fname);
            fclose(fp), fp=NULL;
            return -1;
        }

        if(0 == ret)
        {
            free(new_fblk);
            new_fblk = NULL;
            break;
        }
        
        new_fblk->fbuff = (char *)calloc(1, head->size);
        if(NULL == new_fblk->fbuff)
        {
            pub_log_error("[%s][%d] Alloc memory failed!", __FILE__, __LINE__);
            fclose(fp), fp=NULL;
            return -1;
        }

        if(!feof(fp))
        {            
            /* 2. ��ȡ�ļ������� */
            ret = fread(new_fblk->fbuff, head->size, 1, fp);
            if(ferror(fp))
            {
                vars_exp_link_free(vars->fblk);
                vars->fblk = NULL;
                pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
                    __FILE__, __LINE__, errno, strerror(errno), fname);
                fclose(fp), fp=NULL;
                return -1;
            }
        }

        if(0 == ret)
        {
            free(new_fblk->fbuff);
            free(new_fblk);
            new_fblk = NULL;

            vars_exp_link_free(vars->fblk);
            vars->fblk = NULL;
            fclose(fp), fp=NULL;
			pub_log_error("[%s][%d] read 0 byte", __FILE__, __LINE__);
            return -1;
        }

        if(NULL == fblk)
        {
            fblk = new_fblk;
            vars->fblk = fblk;
        }
        else
        {
            fblk->next = new_fblk;
            fblk = new_fblk;
        }

        fblk->next = NULL;
    }
    fclose(fp), fp=NULL;
    
    return 0;
}

/******************************************************************************
 **��������: vars_exp_clone
 **��    ��: ��¡��չ�ռ�����
 **�������: 
 **     vars: ���������ö���
 **     vid: Ŀ�������ID
 **�������: NONE
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **     1. ��¡�����ڴ��е�����
 **     2. ��¡��չ�ռ��е�����
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.06.15 #
 ******************************************************************************/
static sw_int_t vars_exp_clone(vars_object_t *src_vars, int dst_vid)
{
    int index = 0;
    int ret=0, offset=0;
    FILE *fp = NULL;
    vars_fblk_t *fblk = NULL;
    char fname[FILE_NAME_MAX_LEN] = {0};

    if(!vars_get_fflag(src_vars))
    {
        return 0;
    }
    
    vars_exp_get_fname(dst_vid, fname, sizeof(fname));

    fp = fopen(fname, "wb");
    if(NULL == fp)
    {
        pub_log_error("[%s][%d] errmsg:[%d] %s! %s",
            __FILE__, __LINE__, errno, strerror(errno), fname);
        return -1;
    }

    fblk = src_vars->fblk;
    while(NULL != fblk)
    {
        offset = (fblk->head.start_offset - src_vars->data_size)
                    + (index * sizeof(vars_fblk_head_t));
        fseek(fp, offset, SEEK_SET);

        /* 1. д�ļ���ͷ */
        ret = fwrite(&fblk->head, sizeof(vars_fblk_head_t), 1, fp);
        if(0 == ret)
        {
            pub_log_error("[%s][%d] errmsg:[%d] %s! %s",
                __FILE__, __LINE__, errno, strerror(errno), fname);
            fclose(fp), fp=NULL;
            return ret;
        }
        
        /* 2. д�ļ��黺�� */
        ret = fwrite(fblk->fbuff, fblk->head.size, 1, fp);
        if(0 == ret)
        {
            pub_log_error("[%s][%d] errmsg:[%d] %s! %s",
                __FILE__, __LINE__, errno, strerror(errno), fname);
            fclose(fp), fp=NULL;
            return ret;
        }

        index++;
        fblk = fblk->next;
    }

    fclose(fp), fp=NULL;
    return 0;
}

#elif defined(__VARS_EXP_SHM__)
/******************************************************************************
 **��������: vars_exp_get_shm_key
 **��    ��: ��ȡ��չ�����ڴ�KEYֵ
 **�������: NONE
 **�������: NONE
 **��    ��: ��չ�����ڴ�KEYֵ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
key_t vars_exp_get_shm_key(void)
{
    struct timeval cur_time;

    memset(&cur_time, 0, sizeof(cur_time));

    gettimeofday(&cur_time, NULL);

    return ((VARS_SHM_BASE_KEY + random()*cur_time.tv_usec)&0x7FFFFFFF);
}

/******************************************************************************
 **��������: vars_exp_creat
 **��    ��: ������չ�ռ�(�����ڴ�)
 **�������: NONE
 **�������: NONE
 **��    ��: ��չ�����ڴ�KEYֵ
 **ʵ������: 
 **     1. ������չ�����ڴ�ʵ������
 **     2. ������չ�����ڴ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
vars_exp_t *vars_exp_creat(int start_offset, int size)
{
    int total=0, nblock=0;
    vars_exp_t *exp = NULL;

    exp = (vars_exp_t *)calloc(1, sizeof(vars_exp_t));
    if(NULL == exp)
    {
        return NULL;
    }

    total = size + sizeof(vars_exp_head_t);
    
    nblock = total / vars_get_page_size();
    if(0 != (total % vars_get_page_size()))
    {
        nblock++;
    }
    
    total = nblock * vars_get_page_size();

    do
    {
        exp->shmkey = vars_exp_get_shm_key();
        if(VARS_IPC_KEY_INVAILD == exp->shmkey)
        {
            continue;
        }

        /* �����ڴ��Ѵ��ڣ��򷵻�-1 */
        exp->shmid = shmget(exp->shmkey, total, IPC_CREAT|IPC_EXCL|0666);
        if(exp->shmid < 0)
        {
            if(-1 == exp->shmid)
            {
                continue;   /* �����ڴ��Ѵ��ڣ����������� */
            }

            free(exp), exp=NULL;
            return NULL;
        }

        exp->addr = shmat(exp->shmid, NULL, 0);

        memset(exp->addr, 0, total);

        exp->head = (vars_exp_head_t *)exp->addr;
        exp->head->next_key = 0;
        exp->head->size = (total - sizeof(vars_exp_head_t));
        exp->head->offset = 0;
        exp->head->start_offset = start_offset;
        exp->head->end_offset = (start_offset + exp->head->size - 1);
        exp->data = exp->addr + sizeof(vars_exp_head_t);

        return exp;
    }while(1);

    free(exp), exp=NULL;
    return NULL;
}

/******************************************************************************
 **��������: vars_exp_destory
 **��    ��: ������չ�ռ�
 **�������:
 **      head: ��չ�����ڴ���Ϣ
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     ����ɾ����չ�����ڴ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
static sw_int_t vars_exp_destory(vars_object_t *vars)
{
    vars_exp_t *exp=NULL, *next=NULL;
    
    if(NULL == vars->head)
    {
        return 0;
    }

    exp = vars->expand;
    while(NULL != exp)
    {
        next = exp->next;

        shmdt(exp->addr);
        shmctl(exp->shmid, IPC_RMID, NULL);
        free(exp);

        exp = next;
    }
    vars->expand = NULL;
    vars->head->next_key = VARS_IPC_KEY_INVAILD;
    
    return 0;
}

/******************************************************************************
 **��������: vars_exp_link_free
 **��    ��: �ͷ���չ����
 **�������:
 **      fblk: �ļ���������ͷ
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
static sw_int_t vars_exp_link_free(vars_exp_t *head)
{
    vars_exp_t *exp=NULL, *next=NULL;

    exp = head;
    while(NULL != exp)
    {
        next = exp->next;
        free(exp);
        exp = next;
    }
    
    return 0;
}

/******************************************************************************
 **��������: vars_exp_block_alloc
 **��    ��: ����չ�ռ�(�����ڴ�)�������ڴ�
 **�������:
 **      vars: �����ض���
 **      size: ����ռ�Ĵ�С
 **�������:
 **      offset: ������ݿ���ʼλ�õ�ƫ����
 **��    ��: ����Ŀռ��ַ
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
static void *vars_exp_block_alloc(vars_object_t *vars, int size, size_t *offset)
{
    void *addr = NULL;
    vars_exp_t *exp = NULL,
                *new_exp = NULL;

    exp = vars->expand;
    if(NULL == exp)
    {
        exp = vars_exp_creat(vars->data_size, size);
        if(NULL == exp)
        {
            return NULL;
        }

        vars->expand = exp;
        vars->head->next_key = exp->shmkey;
    }

    while(NULL != exp)
    {        
        /* 1. �жϵ�ǰ�ļ������ռ��Ƿ��㹻 */
        if((exp->head->size - exp->head->offset) < size)
        {
            if(NULL != exp->next)
            {
                exp = exp->next;
                continue;
            }

            new_exp = vars_exp_creat(exp->head->end_offset+1, size);
            if(NULL == new_exp)
            {
                return NULL;
            }

            vars_exp_set_next_key(exp, new_exp->shmkey);
            exp->next = new_exp;
            exp = new_exp;
            continue;
        }

        addr = (exp->data + exp->head->offset);
        *offset = (exp->head->start_offset + exp->head->offset);
        exp->head->offset += size;

        return addr;
    }

    return NULL;
}

/******************************************************************************
 **��������: vars_exp_offset_to_addr
 **��    ��: ����չ�ռ�(��չ�����ڴ�)ƫ����ת��Ϊ�ڴ��ַ
 **�������:
 **      vars: �����ض���
 **      offset: ƫ����(������ݿ��׵�ַ)
 **�������:
 **��    ��: ���ݵ�ַ
 **ʵ������: 
 **  1) �ļ�����������ʾ:
 **     |<--                           ��չ�����ڴ�                              -->|
 **     -----------------------------------------------------------------------------
 **     |          |///////////////////|                                            |
 **     |  ��չͷ  |\\\\\\\\\\\\\\\\\\\|<--               δʹ�ÿռ�             -->|
 **     |          |///////////////////|                                            |
 **     -----------------------------------------------------------------------------
 **     ^          ^                   ^                                            ^
 **     |          |                   |                                            |
 **   addr    start_offset       offset(���start_offset)                       end_offset
 **               data
 **ע������: 
 **  1) start_offset, offset, end_offset: ��ֵ��������ݿ���ʼ��ַ��ƫ����
 **��    ��: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
static void *vars_exp_offset_to_addr(const vars_object_t *vars, size_t offset)
{
    const vars_exp_t *exp = NULL;
    const vars_exp_head_t *head = NULL;

    exp = vars->expand;

    while(NULL != exp)
    {
        head = exp->head;
        if((offset >= head->start_offset)
            && (offset <= head->end_offset))
        {
            return (exp->data + (offset - head->start_offset));
        }

        exp = exp->next;
    }

    return NULL;
}

/******************************************************************************
 **��������: vars_exp_load
 **��    ��: ����������չ���ݿ������ڴ�
 **�������:
 **      vars: �����ض���
 **�������:
 **��    ��: 0: �ɹ� !0: ʧ��
 **ʵ������: 
 **     1. �Ƿ��б������浽�ļ���
 **     2. ��ȡ�ļ��������ж��ļ�״̬
 **     3. ���������ļ��ڵ�
 **     4. ���ļ������ڴ�
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.05.15 #
 ******************************************************************************/
static sw_int_t vars_exp_load(vars_object_t *vars)
{
    int ret = 0;
    key_t shmkey = 0;
    FILE *fp = NULL;
    vars_exp_t *exp = NULL, *new_exp = NULL;


    /* 1. �Ƿ�����չ�����ڴ� */
    if(VARS_IPC_KEY_INVAILD == vars->head->next_key)
    {
        return 0;
    }

    /* 2. ����չ�����ڴ� */
    shmkey = vars->head->next_key;
    do
    {
        new_exp = (vars_exp_t *)calloc(1, sizeof(vars_exp_t));
        if(NULL == new_exp)
        {
            return 0;
        }

        new_exp->shmid = shmget(shmkey, 0, 0);
        if(new_exp->shmid < 0)
        {
            /* �����쳣 */
            free(new_exp), new_exp=NULL;
            return -1;
        }

        new_exp->addr = shmat(new_exp->shmid, NULL, 0);
        new_exp->head = (vars_exp_head_t *)new_exp->addr;
        new_exp->data = new_exp->addr + sizeof(vars_exp_head_t);

        if(NULL == vars->expand)
        {
            vars->expand = new_exp;
        }
        else
        {
            exp->next = new_exp;
        }
        exp = new_exp;
        shmkey = exp->head->next_key;
    }while(0 != shmkey);
    
    return 0;
}
#endif /*__VARS_EXP_FILE__*/
#endif /*__SHM_VARS_SUPPORT__*/
