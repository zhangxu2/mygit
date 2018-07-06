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
/* 缓存页大小 */
static int g_vars_page_size = 0;
#define vars_set_page_size(size) (g_vars_page_size = (size))        /* 设置页大小 */
#define vars_get_page_size()    (g_vars_page_size)                  /* 获取页大小 */
#endif

/* 全局变量池参数信息: 初始化后，不可更改 */
static vars_args_t g_vars_args;

/* 共享内存ID */
#define vars_set_shmid(id) (g_vars_args.shmid = (id))               /* 设置共享内存ID */
#define vars_get_shmid() (g_vars_args.shmid)                        /* 获取共享内存ID */

#if defined(__VARS_RAND_ALLOC__)
/* 信号量ID */
#define vars_set_semid(id) (g_vars_args.semid = (id))               /* 设置信号量ID */
#define vars_get_semid() (g_vars_args.semid)                        /* 获取信号量ID */
#endif /*__VARS_RAND_ALLOC__*/

/* 变量池总数 */
#define vars_set_total(num) (g_vars_args.vars_total = (num))        /* 设置变量池总数 */
#define vars_get_total() (g_vars_args.vars_total)                   /* 获取变量池总数 */
/* 变量池单元 */
#define vars_set_unit_size(usize)   (g_vars_args.unit_size = usize) /* 设置变量池单元大小 */
#define vars_get_unit_size()   (g_vars_args.unit_size)              /* 设置变量池单元大小 */
/* 内存地址 */
#define vars_set_addr(addr) (g_vars_args.addr = (addr))             /* 设置共享内存首地址 */
#define vars_get_addr() (g_vars_args.addr)                          /* 获取共享内存首地址 */

#if defined(__VARS_RAND_ALLOC__)
#define vars_bitmap_len()  (g_vars_args.vars_total * sizeof(int))   /* 索引段长度 */
#define vars_set_bitmap_addr(addr) (g_vars_args.bitmap = (addr))    /* 设置索引段首地址 */
/* 索引操作 */
#define vars_set_used(vid)   (g_vars_args.bitmap[vid] = 1)          /* 设置变量池被占用 */
#define vars_reset_used(vid)   (g_vars_args.bitmap[vid] = 0)        /* 重置变量池被占用 */
#define vars_is_used(vid)   (g_vars_args.bitmap[vid])               /* 获取变量池被占用 */
#endif /*__VARS_RAND_ALLOC__*/

/* 判断节点是否被占用 */
#define VarsIsNodeUsed(node)    ('\0' != node->name[0])
/* 判断变量名是否合法 */
#define VarsIsNameValid(name) (('#'==name[0]) || ('$'==name[0]))
/* 判断变量池空间是否足够 */
#define VarsIsShmEnough(vars, size) ((vars->head->offset+(size)) <= vars->data_size)

#define vars_get_node_mod(name) (Hash(name)%VARS_NODE_NUM)          /* 变量名模值 */

#if !defined(__VARS_MEM_ALLOC_SLAB__)
#define vars_reset_offset(vars) (vars->head->offset = 0)            /* 重置偏移量 */
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

/* 释放单个文件块节点 */
#define vars_exp_free(fblk) \
{ \
    if(NULL != fblk->fbuff) \
    { \
        free(fblk->fbuff); \
        fblk->fbuff = NULL; \
    } \
    free(fblk), fblk=NULL; \
}

/* 获取变量池缓存文件名 */
#define vars_exp_get_fdir(fdir, size) \
            snprintf(fdir, size, "%s/tmp/vars", getenv("SWWORK"))
#define vars_exp_get_fname(vid, fname, size) \
            snprintf(fname, size, "%s/tmp/vars/%d.vars", getenv("SWWORK"), vid)

/* 文件标识 */
#define vars_set_fflag(vars)    (vars->head->fflag = 1)             /* 设置文件标识 */
#define vars_reset_fflag(vars)  (vars->head->fflag = 0)             /* 重置文件标识 */
#define vars_get_fflag(vars)    (vars->head->fflag)                 /* 设置文件标识 */

/* 脏数据标识 */
#define vars_set_dirty(fblk)    (fblk->isdirty = 1)                 /* 设置脏数据标识 */
#define vars_reset_dirty(fblk)  (fblk->isdirty = 0)                 /* 重置脏数据标识 */
#define vars_get_dirty(fblk)    (fblk->isdirty)                     /* 获取脏数据标识 */

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
#define vars_offset_to_addr(vars, offset) ((vars)->data + (offset)) /* 偏移量转换为内存地址 */
#endif /*__VARS_EXP_FILE__*/

/* 静态函数声明 */
static int vars_shm_creat(key_t key, int num);
#if defined(__VARS_RAND_ALLOC__)
static int vars_sem_creat(key_t key, int num);
#endif /*__VARS_RAND_ALLOC__*/
static int vars_shm_size(int num, int usize);

#if defined(__VARS_RAND_ALLOC__)
/******************************************************************************
 **函数名称: Random
 **功    能: 生成(非负)随机数字
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 非负随机数字
 **实现描述: 
 **     1. 获取当前时间
 **     2. 生成随机数字
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
sw_int32_t Random(void)
{
    struct timeval cur_time;

    memset(&cur_time, 0, sizeof(cur_time));

    gettimeofday(&cur_time, NULL);

    return ((random()*cur_time.tv_usec)&0x7FFFFFFF);
}

/******************************************************************************
 **函数名称: vars_lock
 **功    能: 加锁指定变量池索引
 **输入参数:
 **      vid: 变量池索引ID
 **      flag: 信号量sem_flg的值(IPC_NOWAIT or SEM_UNDO)
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **      其需要和vars_unlock()配对使用
 **作    者: # Qifeng.zou # 2013.04.24 #
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
 **函数名称: vars_unlock
 **功    能: 解锁指定变量池索引
 **输入参数:
 **      vid: 变量池索引ID
 **      flag: 信号量sem_flg的值(IPC_NOWAIT or SEM_UNDO)
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **      其需要和vars_lock()配对使用
 **作    者: # Qifeng.zou # 2013.04.24 #
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
 **函数名称: vars_creat
 **功    能: 创建变量池集
 **输入参数:
 **      swcfg: 平台配置参数
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 创建共享内存
 **     2. 创建信号量集
 **     3. 获取共享内存地址
 **     4. 设置全局变量
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.23 #
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
    
    /* 1. 创建共享内存 */
    usize = (params->vars_max_size > VARS_MAX_SIZE)? params->vars_max_size: VARS_MAX_SIZE;
    size = vars_shm_size(params->vars_max_num, usize);
    shmid = vars_shm_creat(params->vars_shm_key, size);
    if(shmid < 0)
    {
        return -1;
    }

#if defined(__VARS_RAND_ALLOC__)
    /* 2. 创建信号量集 */
    semid = vars_sem_creat(params->vars_sem_key, params->vars_max_num);
    if(semid < 0)
    {
        return -1;
    }
#endif /*__VARS_RAND_ALLOC__*/

    /* 2. 获取共享内存地址 */
    addr = shmat(shmid, NULL, 0);
    if((void*)-1 == addr)
    {
        return -1;
    }

    /* 4. 设置全局参数 */
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
 **函数名称: vars_shm_init
 **功    能: 创建变量池共享内存
 **输入参数:
 **      swcfg: 平台配置参数
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 创建共享内存
 **     2. 获取共享内存地址
 **     3. 设置全局变量
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.23 #
 ******************************************************************************/
sw_int_t vars_shm_init(sw_syscfg_t *syscfg)
{
    void *addr = NULL;
    int shmid=0;
    size_t size=0, usize=0;
    
    memset(&g_vars_args, 0, sizeof(g_vars_args));
    
    /* 1. 创建共享内存 */
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
    

    /* 2. 获取共享内存地址 */
    addr = shmat(syscfg->vars_shmid, NULL, 0);
    if((void*)-1 == addr)
    {
	    pub_log_error("[%s][%d] errmsg:[%d]%s! [%d].",
             __FILE__, __LINE__, errno, strerror(errno), syscfg->vars_shmid);
        return -1;
    }

    /* 4. 设置全局参数 */
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
 **函数名称: vars_destory
 **功    能: 销毁/释放变量池集
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 销毁信号量集
 **     2. 销毁共享内存
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_destory(void)
{
    void *addr = NULL;
    int shmid = 0;

#if defined(__VARS_RAND_ALLOC__)
	int semid = 0;

    semid = vars_get_semid();

    /* 1. 销毁信号量集 */
    semctl(semid, 0, IPC_RMID);
#endif /*__VARS_RAND_ALLOC__*/
    

    /* 2. 销毁共享内存 */
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
 **函数名称: vars_shm_size
 **功    能: 计算共享内存的大小(索引+变量池)
 **输入参数:
 **      num: 变量池总数
 **      size: 单个变量池大小
 **输出参数:
 **返    回: 共享内存大小
 **实现描述: 
 **      1. 计算索引所占空间
 **      2. 计算变量池所占空间
 **
 **  共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **注意事项: 
 **      1. 索引段: num个sizeof(int)
 **      2. 变量池段: num个变量池对象
 **作    者: # Qifeng.zou # 2013.04.24 #
 ******************************************************************************/
static sw_int32_t vars_shm_size(int num, int usize)
{
    int size = 0;

#if defined(__VARS_RAND_ALLOC__)
    /* 1. 计算索引所占空间 */
    size = num*sizeof(int);
#endif /*__VARS_RAND_ALLOC__*/

    /* 2. 计算变量池所占空间 */
    size += num * usize;

    return size;
}

/******************************************************************************
 **函数名称: vars_shm_creat
 **功    能: 创建共享内存变量池
 **输入参数:
 **      key: 共享内存key
 **      num: 变量池个数
 **输出参数:
 **返    回: 共享内存ID
 **实现描述: 
 **     1. 判断是否已创建共享内存
 **     2. 不存在，则创建共享内存
 **注意事项: 
 **  共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **作    者: # Qifeng.zou # 2013.04.22 #
 ******************************************************************************/
static sw_int32_t vars_shm_creat(key_t key, int size)
{
    int shmid = 0;
    void *addr = NULL;


    /* 1. 判断是否已创建共享内存 */
    shmid = shmget(key, 0, 0);
    if(shmid < 0)
    {
        if(ENOENT != errno)
        {
            pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
            return -1;
        }
        /* 共享内存不存在, 在下面进行创建 */
    }
    else
    {
        return shmid;
    }

    /* 2. 不存在，则创建共享内存 */    
    shmid = shmget(key, size, IPC_CREAT|0666);
    if(shmid < 0)
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }
    
    /* 3. 清空共享内存 */
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
 **函数名称: vars_sem_creat
 **功    能: 创建变量池信号量
 **输入参数:
 **      key: 信号量集key
 **      num: 信号量总数(与变量池个数一致)
 **输出参数:
 **返    回: 信号量集ID
 **实现描述: 
 **     1. 判断是否已创建信号量集
 **     2. 不存在，则创建信号量集
 **     3. 设置信号量集各成员的初始值
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.23 #
 ******************************************************************************/
static int vars_sem_creat(key_t key, int num)
{
    union semun sem;
    int vid=0, semid=0;


    /* 1. 判断是否已创建信号量集 */
    semid = semget(key, 0, 0);
    if(semid < 0)
    {
        if(ENOENT != errno)
        {
            pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
            return -1;
        }
        /* 信号量不存在，在下面进行创建 */
    }
    else
    {
        return semid;
    }

    /* 2. 不存在，则创建信号量集 */
    semid = semget(key, num, IPC_CREAT|0666);
    if(semid < 0)
    {
        pub_log_error("[%s][%d] errmsg:[%d]%s!", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    /* 3. 设置信号量集各成员的初始值 */
    for(vid=0; vid<num; vid++)
    {
        sem.val = 1;
        semctl(semid, vid, SETVAL, sem);
    }
    
    return semid;
}

/******************************************************************************
 **函数名称: vars_valloc
 **功    能: 申请一个可用的变量池
 **输入参数: 
 **     vars: 变量池对象
 **输出参数: NONE
 **返    回: 变量池索引ID
 **实现描述: 
 **     1. 通过随机值计算一个变量池ID
 **     2. 如果此变量池已被占用，则查找下一个变量池，直到找到一个未被占用的变量池
 **注意事项:  
 **作    者: # Qifeng.zou # 2013.04.24 #
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
    }while(loop <= maxloop);   /* 防止死循环: 循环一圈，退出 */

    pub_log_error("[%s][%d] Alloc vars failed!", __FILE__, __LINE__);
    return -1;
}
#endif /*__VARS_RAND_ALLOC__*/

/******************************************************************************
 **函数名称: vars_vfree
 **功    能: 释放变量池
 **输入参数: 
 **     vars: 变量池对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     释放变量池对象申请的动态空间等
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.04.24 #
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
    /* 1. 销毁扩展空间数据 */
    vars_exp_destory(vars);
#endif

    /* 2. 重置变量池占用标识 */
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
 **函数名称: vars_vclone
 **功    能: 克隆变量池引用对象的数据至指定变量池中
 **输入参数: 
 **     src_vars: 变量池引用对象
 **     dst_vid: 目标变量池ID
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 克隆共享内存中的数据
 **     2. 克隆扩展空间中的数据
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.15 #
 ******************************************************************************/
sw_int_t vars_vclone(vars_object_t *src, int dst_vid)
{
    int ret = 0;
    vars_object_t dst;

    /* 1. 获取目标变量池引用对象 */
    ret = vars_object_creat(dst_vid, &dst);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Create object failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 2. 拷贝变量池至目标变量池 */
    /* 2.1 拷贝共享内存 */
    vars_vclear(&dst);

    memcpy(dst.head, src->head, sizeof(vars_head_t));     /* 克隆变量池头 */
#if defined(__VARS_MEM_ALLOC_SLAB__)
    memcpy(dst.pool, src->pool, src->pool->pool_size);
#else
    memcpy(dst.data, src->data, src->head->offset);  /* 克隆变量池数据块 */
#endif

#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
    /* 2.2 拷贝扩展空间数据 */
    vars_exp_clone(src, dst_vid);
#endif /*__VARS_EXP_FILE__ || __VARS_EXP_SHM__*/

    return 0;
}

/******************************************************************************
 **函数名称: vars_vmerge_link
 **功    能: 变量池合并
 **输入参数: 
 **     src: 源变量池
 **     head: 源变量池中的哈希表头
 **     dst: 目的变量池
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 遍历哈希链表
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.06 #
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
 **函数名称: vars_vmerge
 **功    能: 合并变量池
 **输入参数: 
 **     src: 源变量池引用对象
 **     dst_vid: 目标变量池ID
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 合并共享内存中的数据
 **     2. 合并扩展空间中的数据
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.09.06 #
 ******************************************************************************/
sw_int_t vars_vmerge(const vars_object_t *src, int dst_vid)
{
    int ret = 0, idx = 0;
    vars_object_t dst;

    memset(&dst, 0, sizeof(dst));

    /* 1. 获取目标变量池引用对象 */
    ret = vars_object_creat(dst_vid, &dst);
    if(0 != ret)
    {
        pub_log_error("[%s][%d] Create object failed!", __FILE__, __LINE__);
        return -1;
    }

    /* 2. 依次合并变量池数据 */
    for(idx=0; idx<VARS_NODE_NUM; idx++)
    {
        ret = vars_vmerge_link(src, &src->head->node[idx], &dst);
        if(0 != ret)
        {
            pub_log_error("[%s][%d] Merge link failed!", __FILE__, __LINE__);
            return -1;
        }
    }

    /* 3. 同步数据并释放引用对象空间 */
    vars_sync(&dst);
    
    vars_object_free(&dst);
    
    return 0;
}

/******************************************************************************
 **函数名称: vars_object_creat
 **功    能: 创建变量池实例化对象
 **输入参数: 
 **     vid: 变量池ID
 **输出参数: 
 **     vars: 变量池对象
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **  1) 共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) 单个变量池的结构如下所示:
 **     | 文件 |偏移量|<---    MOD个变量池节点    --->|<--         数据块        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            数据块           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **作    者: # Qifeng.zou # 2013.04.27 #
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
    vars->addr = (char *)vars_get_addr()            /* 共享内存首地址 */
                  #if defined(__VARS_RAND_ALLOC__)
                    + vars_bitmap_len()     /* 跳过索引段长度 */
                  #endif /*__VARS_RAND_ALLOC__*/
                    + vid*vars_get_unit_size(); /* 跳过前面的变量池 */
    vars->head = (vars_head_t *)vars->addr; /* 变量池头部信息 */
    vars->data = (void *)((char *)vars->addr + sizeof(vars_head_t));    /* 数据块起始地址 */
    vars->data_size = vars_get_unit_size() - sizeof(vars_head_t);   /* 数据块初始空间 */ 

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
    /* 加载扩展空间数据 */
    return vars_exp_load(vars);
#else
    return 0;
#endif
}

#if defined(__VARS_EXP_FILE__)
/******************************************************************************
 **函数名称: vars_clear
 **功    能: 清空变量池中所有对象，但不释放对变量池的占用
 **输入参数: 
 **     vars: 变量池对象
 **输出参数: NONE
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项:  
 **作    者: # Qifeng.zou # 2013.06.14 #
 ******************************************************************************/
sw_int_t vars_vclear(vars_object_t *vars)
{
    /* 1. 释放扩展空间 */
    vars_exp_destory(vars);

    vars_exp_link_free(vars->fblk);
    vars->fblk = NULL;

    /* 2. 重置共享内存数据 */
    vars_reset_fflag(vars);
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    vars_reset_offset(vars);
#endif /*!__VARS_MEM_ALLOC_SLAB__*/
    memset(vars->addr, 0, vars_get_unit_size());
    
    return 0;
}

/******************************************************************************
 **函数名称: vars_object_free
 **功    能: 释放变量池实例化对象的空间，但不释放变量池的空间和文件
 **输入参数: 
 **     vars: 变量池对象
 **输出参数: NONE
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项:  
 **作    者: # Qifeng.zou # 2013.05.09 #
 ******************************************************************************/
sw_int_t vars_object_free(vars_object_t *vars)
{
    vars_exp_link_free(vars->fblk);
    
    vars->fblk = NULL;
    return 0;
}

#elif defined(__VARS_EXP_SHM__)
/******************************************************************************
 **函数名称: vars_clear
 **功    能: 清空变量池中所有对象，但不释放对变量池的占用
 **输入参数: 
 **     vars: 变量池对象
 **输出参数: NONE
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项:  
 **作    者: # Qifeng.zou # 2013.06.14 #
 ******************************************************************************/
sw_int_t vars_vclear(vars_object_t *vars)
{
    /* 1. 销毁扩展空间 */
    vars_exp_destory(vars);

    vars_exp_link_free(vars->expand);
    vars->expand = NULL;

    /* 2. 重置标识 */
    vars_reset_next_key(vars);
#if !defined(__VARS_MEM_ALLOC_SLAB__)
    vars_reset_offset(vars);
#endif /*!__VARS_MEM_ALLOC_SLAB__*/
    memset(vars->addr, 0, vars_get_unit_size());
    return 0;
}

/******************************************************************************
 **函数名称: vars_object_free
 **功    能: 释放变量池实例化对象的空间，但不释放变量池的空间和文件
 **输入参数: 
 **     vars: 变量池对象
 **输出参数: NONE
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项:  
 **作    者: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
sw_int_t vars_object_free(vars_object_t *vars)
{
    vars_exp_link_free(vars->expand);
    
    vars->expand = NULL;
    return 0;
}
#endif /*__VARS_EXP_FILE__*/
    
/******************************************************************************
 **函数名称: vars_block_alloc
 **功    能: 从变量池空间中申请制定大小的空间
 **输入参数: 
 **     vars: 变量池对象
 **     size: 申请空间大小
 **输出参数: NONE
 **返    回: 申请的空间偏移量
 **实现描述: 
 **     1. 从共享内存中分配空间
 **     2. 从文件缓存中分配空间
 **注意事项:  
 **作    者: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
static void *vars_block_alloc(vars_object_t *vars, int size, size_t *offset)
{
    void *addr = NULL;

    *offset = -1;

    /* 1. 从共享内存中分配空间 */
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
    /* 2. 从扩展空间中分配空间 */
    return vars_exp_block_alloc(vars, size, offset);
#else
    return NULL;
#endif    
}

#if defined(__VARS_MEM_ALLOC_SLAB__)
/******************************************************************************
 **函数名称: vars_block_free
 **功    能: 释放变量池中内存块的空间
 **输入参数:
 **      vars: 变量池对象
 **      p: 释放的地址对象
 **输出参数: NONE
 **返    回: 0:success  !0:failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.07.18 #
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
 **函数名称: vars_node_best_unused
 **功    能: 获取最适合的未使用的节点 [最佳匹配算法]
 **输入参数: 
 **     node: 变量池对象
 **     best: 已选择的最适合的节点
 **     size: 申请的空间大小
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     使用[最佳匹配算法]，进一步提高空间利用率
 **注意事项: 
 **     1. 当best比申请的size小时
 **         1.1 如果node比best还小时，则选择node节点
 **         1.2 如果node比size还大时，则选择node节点
 **     2. 当best比申请的size大时
 **         2.1 如果node比best还小时，但比size大时，则选择node节点
 **         2.2 如果node比size还小时，则选择best节点
 **作    者: # Qifeng.zou # 2013.05.06 #
 ******************************************************************************/
static vars_node_t *vars_node_best_unused(vars_node_t *node, vars_node_t *best, size_t size)
{
    if(NULL == best)
    {
        return node;
    }

    if(size == best->size)
    {
        return best;        /* 已经找到了最合适的节点 */
    }

    /* 1. 当best比申请的size小时 */
    if(best->size < size)
    {
        if((node->size >= size)
            || (node->size < best->size))
        {
            return node;
        }

        return best;
    }

    /* 2. 当best比申请的size大时 */
    if((node->size < best->size) && (node->size >= size))
    {
        return node;
    }

    return best;
}
#endif /*__VARS_MEM_ALLOC_SLAB__*/

/******************************************************************************
 **函数名称: vars_set_revariable
 **功    能: 设置变量信息
 **输入参数: 
 **     vars: 变量池对象 
 **     name: 变量名
 **     type: 变量类型 VAR_TYPE_CHAR-字符  VAR_TYPE_BIN-二进制
 **     value: 变量值
 **     length: 变量值长度
 **     index: 重复变量索引
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **  1) 共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) 单个变量池的结构如下所示:
 **     | 文件 |偏移量|<---    MOD个变量池节点    --->|<--         数据块        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            数据块           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **作    者: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_set_revariable(vars_object_t *vars,
    const char *name, int type, const char *value, int length, int index)
{
    int mod=0;
    vars_node_t *node = NULL,   /* 变量池节点地址 */
                *new_node = NULL,   /* 新建节点 */
                *best = NULL;   /* 空闲节点 */
    void *dst_value = NULL;
    size_t offset = 0;
    size_t size = 0;

    /* 变量名合法性判断 */
    if(!VarsIsNameValid(name))
    {
        return -1;
    }
    
    mod = vars_get_node_mod(name);
    node = &vars->head->node[mod];
    size = length+1;

    /* 1. 申请节点空间[支持可重复利用] */
    do
    {
    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.1 未使用时，查找下一个节点 */
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

        /* 1.2 是否与查找的name一致 */
        if((index == node->index)
            && (0 == strcmp(name, node->name)))
        {
            break;  /* 找到name节点 */
        }

        /* 1.3 是否有下一个节点 */
        if(0 != node->next)
        {
            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
            continue;
        }

    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.4 空闲节点再利用 */
        if(NULL != best)
        {
            node = best;
            break;
        }
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        
        /* 1.5 分配新节点空间 */
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

    /* 2. 设置节点数据信息 */
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
 **函数名称: vars_get_null
 **功    能: 从变量池中申请固定长度内存
 **输入参数: 
 **     vars: 变量池对象 
 **     name: 内存块名
 **     length: 内存块长度
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **  1) 共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) 单个变量池的结构如下所示:
 **     | 文件 |偏移量|<---    MOD个变量池节点    --->|<--         数据块        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            数据块           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **作    者: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_char_t *vars_get_null(vars_object_t *vars, const char *name, int length)
{
    int mod=0;
    vars_node_t *node = NULL,   /* 变量池节点地址 */
                *new_node = NULL,   /* 新建节点 */
                *best = NULL;   /* 空闲节点 */
    void *dst_value = NULL;
    size_t offset = 0;
    size_t size = 0;

    /* 变量名合法性判断 */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Var name is invalid! name:%s", __FILE__, __LINE__, name);
        return NULL;
    }
    
    mod = vars_get_node_mod(name);
    node = &vars->head->node[mod];
    size = length+1;

    /* 1. 申请节点空间[支持可重复利用] */
    do
    {
    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.1 未使用时，查找下一个节点 */
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

        /* 1.2 是否与查找的name一致 */
        if((0 == node->index)
            && (0 == strcmp(name, node->name)))
        {
            break;  /* 找到name节点 */
        }

        /* 1.3 是否有下一个节点 */
        if(0 != node->next)
        {
            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);
            continue;
        }

    #if !defined(__VARS_MEM_ALLOC_SLAB__)
        /* 1.4 空闲节点再利用 */
        if(NULL != best)
        {
            node = best;
            break;
        }
    #endif /*__VARS_MEM_ALLOC_SLAB__*/
        
        /* 1.5 分配新节点空间 */
        new_node = (vars_node_t *)vars_block_alloc(vars, sizeof(vars_node_t), &offset);
        if(NULL == new_node)
        {
            return NULL;
        }

        node->next = offset;
        node = new_node;
        break;
    }while(1);

    /* 2. 设置节点数据信息 */
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
 **函数名称: vars_get_revariable
 **功    能: 获取变量值
 **输入参数: 
 **     vars: 变量池对象 
 **     name: 变量名
 **     index: 重复变量索引
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **  1) 共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) 单个变量池的结构如下所示:
 **     | 文件 |偏移量|<---    MOD个变量池节点    --->|<--         数据块        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            数据块           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **作    者: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
const void *vars_get_revariable(vars_object_t *vars, const char *name, int index, int *length)
{
    unsigned int mod = 0;
    vars_node_t *node = NULL;   /* 变量池节点地址 */


    /* 变量名合法性判断 */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Var name is invaliled! [%s]", __FILE__, __LINE__, name);
        return NULL;
    }

    mod = vars_get_node_mod(name);
    node = (vars_node_t *)(vars->head->node + mod);

    /* 1. 查找指定的节点 */
    do
    {
        if(!VarsIsNodeUsed(node)
            || (index != node->index)
            || (0 != strcmp(name, node->name)))
        {
            if(0 == node->next)
            {
                return NULL;    /* 查找失败 */
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
 **函数名称: vars_delete_revariable
 **功    能: 删除指定变量
 **输入参数: 
 **     vars: 变量池对象 
 **     name: 变量名
 **     index: 重复变量索引
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **  1) 共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) 单个变量池的结构如下所示:
 **     | 文件 |偏移量|<---    MOD个变量池节点    --->|<--         数据块        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            数据块           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **作    者: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int_t vars_delete_revariable(vars_object_t *vars, const char *name, int index)
{
    unsigned int mod = 0;
    vars_node_t *head = NULL, *node = NULL;   /* 变量池节点地址 */
    void *value = NULL;
#if defined(__VARS_MEM_ALLOC_SLAB__)
    int ret = 0;
    vars_node_t *prev = NULL;
    shm_slab_pool_t *pool = NULL;
#endif /*__VARS_MEM_ALLOC_SLAB__*/

    /* 1. 变量名合法性判断 */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Vars name is invalied! [%s]", __FILE__, __LINE__, name);
        return -1;
    }

    mod = vars_get_node_mod(name);
    head = (vars_node_t *)(vars->head->node + mod);
    node = head;
    
    /* 2. 查找并删除指定的节点 */
    do
    {
        if(!VarsIsNodeUsed(node)
            || (index != node->index)
            || (0 != strcmp(name, node->name)))
        {
            if(0 == node->next)
            {
                return -1;    /* 查找失败 */
            }
        #if defined(__VARS_MEM_ALLOC_SLAB__)
            prev = node;
        #endif /*__VARS_MEM_ALLOC_SLAB__*/
            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);    /* 查找下一个 */
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
        /* 清空变量名和值等 [注:不修改value、size、next的值] */
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
 **函数名称: vars_get_field_len
 **功    能: 获取变量长度
 **输入参数: 
 **     vars: 变量池对象 
 **     name: 变量名
 **     index: 重复变量索引
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **  1) 共享内存的结构如下所示:
 **     |<--  索引段  -->|<--                       变量池段                     -->|
 **     -----------------------------------------------------------------------------
 **     |                |                                                          |
 **     |      索引段    |                          变量池段                        |
 **     |                |                                                          |
 **     -----------------------------------------------------------------------------
 **
 **  2) 单个变量池的结构如下所示:
 **     | 文件 |偏移量|<---    MOD个变量池节点    --->|<--         数据块        -->|
 **     -----------------------------------------------------------------------------
 **     |      |      |       |       |     |         |                             |
 **     | fflag|offset|NODE[0]|NODE[1]| ... |NODE[m-1]|            数据块           |
 **     |      |      |       |       |     |         |                             |
 **     -----------------------------------------------------------------------------
 **     ^      ^      ^                               ^
 **     |      |      |                               |
 **    addr  offset  node                            data
 **作    者: # Qifeng.zou # 2013.04.27 #
 ******************************************************************************/
sw_int32_t vars_get_field_len(vars_object_t *vars, const char *name, int index)
{
    unsigned int mod = 0;
    vars_node_t *node = NULL;   /* 变量池节点地址 */
    sw_int32_t	len = -1;


    /* 1. 变量名合法性判断 */
    if(!VarsIsNameValid(name))
    {
        pub_log_error("[%s][%d] Vars name is invalied! [%s]", __FILE__, __LINE__, name);
        return -1;
    }

    mod = vars_get_node_mod(name);
    node = (vars_node_t *)(vars->head->node + mod);
    
    /* 2. 查找并删除指定的节点 */
    do
    {
        if(!VarsIsNodeUsed(node)
            || (index != node->index)
            || (0 != strcmp(name, node->name)))
        {
            if(0 == node->next)
            {
                return 0;    /* 查找失败 */
            }

            node = (vars_node_t *)vars_offset_to_addr(vars, node->next);   /* 查找下一个 */
            continue;
        }

        /*获取值长度*/
        len = node->length;
        
        return len;
    }while(1);

    return -1;
}


#if defined(__VARS_EXP_FILE__) || defined(__VARS_EXP_SHM__)
/******************************************************************************
 **函数名称: vars_offset_to_addr
 **功    能: 偏移量转换为地址
 **输入参数:
 **      vars: 变量池对象
 **      offset: 偏移量
 **输出参数:
 **返    回: 数据地址
 **实现描述: 
 **      1. 在共享内存中时，地址=数据块起始地址+偏移量
 **      2. 在扩展中时，地址=vars_exp_offset_to_addr()
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
void *vars_offset_to_addr(const vars_object_t *vars, size_t offset)
{
    /* 1. 在共享内存中时，地址=数据块起始地址+偏移量 */
    if(offset < vars->data_size)
    {
        return ((char *)vars->data + offset); /* 在共享内存中 */
    }

    /* 2. 在扩展空间中计算 */
    return vars_exp_offset_to_addr(vars, offset);
}
#endif /*__VARS_EXP_FILE__ || __VARS_EXP_SHM__*/

/******************************************************************************
 **函数名称: vars_print_link
 **功    能: 打印链表中所有节点
 **输入参数:
 **      vars: 变量池对象
 **输出参数:
 **返    回: 0:success !0:failed
 **实现描述: 
 **     通过head依次遍历链表中各节点，并将其打印
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.22 #
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
 **函数名称: vars_print
 **功    能: 打印指定变量池中的所有变量
 **输入参数:
 **      vars: 变量池对象
 **输出参数:
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.22 #
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
 **函数名称: vars_serial_link
 **功    能: 序列化链表中所有节点
 **输入参数:
 **     vars: 变量池对象
 **     head: 头结点
 **     buflen: 序列化缓冲区的长度
 **输出参数:
 **     buf: 序列化缓冲区
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **     通过head依次遍历链表中各节点，并将其序列化
 **注意事项: 
 **     format: !%02d%s%04d%c%d#XXXXXXXXXXXXX
 **作    者: # Qifeng.zou # 2013.08.08 #
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
 **函数名称: vars_serial
 **功    能: 序列化指定变量池中的所有变量
 **输入参数:
 **     vars: 变量池对象
 **     buflen: 序列化缓冲区长度
 **输出参数:
 **     buf: 序列化缓冲区
 **返    回: >=0: 报文长度  <0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.08 #
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
 **函数名称: vars_unserial
 **功    能: 将buf中的数据进行反序列化处理
 **输入参数:
 **     vars: 变量池对象
 **     buf: 需被反序列化的字串
 **输出参数:
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **     根据序列化的格式将字符串反序列化到变量池中
 **注意事项: 
 **     format: !%02d%s%04d%c%d#XXXXXXXXXXXXX
 **作    者: # Qifeng.zou # 2013.08.08 #
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
 **函数名称: vars_unserial_get_digit
 **功    能: 反序列化字串中获取数字
 **输入参数:
 **     ptr: 反序列化字符串
 **     length: 被取数字的长度
 **输出参数:
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **     1. 判断字串合法性
 **     2. 提取数字
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.09 #
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
 **函数名称: vars_unserial_get_name
 **功    能: 反序列化字串中获取变量名
 **输入参数:
 **     ptr: 反序列化字符串
 **     name_len: 变量名的长度
 **输出参数:
 **     name: 变量名缓存
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **     1. 判断字串合法性
 **     2. 提取变量名长度
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.09 #
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
 **函数名称: vars_unserial_get_length
 **功    能: 反序列化字串中获取变量值的长度
 **输入参数:
 **     ptr: 反序列化字符串
 **     length: 被取数字的长度
 **输出参数:
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **     1. 判断字串合法性
 **     2. 提取变量值长度
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.09 #
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
 **函数名称: vars_unserial_parse
 **功    能: 反序列化字串
 **输入参数:
 **     vars: 变量池引用对象
 **     ptr: 被反序列化的字串
 **输出参数:
 **返    回: 0:Success !0:Failed
 **实现描述: 
 **     1. 提取变量名长度
 **     2. 提取变量名
 **     3. 提取变量索引
 **     4. 提取变量值类型
 **     5. 提取变量值长度
 **     6. 提取变量值
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.08.09 #
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
 **函数名称: vars_exp_dir_creat
 **功    能: 创建变量池扩展空间目录
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.10 #
 ******************************************************************************/
static void vars_exp_dir_creat(void)
{
    char fdir[FILE_NAME_MAX_LEN] = {0};

    vars_exp_get_fdir(fdir, sizeof(fdir));
    
    mkdir(fdir, VARS_DIR_MODE);
}

/******************************************************************************
 **函数名称: vars_exp_creat
 **功    能: 创建一个扩展空间(文件缓存块)
 **输入参数:
 **      size: 申请空间的大小
 **      start_offset: 文件块的起始偏移量
 **输出参数:
 **返    回: 分配的文件块空间地址
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.08 #
 ******************************************************************************/
static vars_fblk_t *vars_exp_creat(int start_offset, int size)
{
    int nblock = 0, total_size = 0;
    vars_fblk_t *fblk = NULL;


    /* 1. 创建文件块节点 */
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
    /* 2. 创建文件缓存 */
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
 **函数名称: vars_exp_destory
 **功    能: 销毁扩展空间(扩展文件)
 **输入参数:
 **      vars: 变量池实例对象
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **     将指定文件长度截断为零
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
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
 **函数名称: vars_exp_link_free
 **功    能: 释放扩展链表(文件缓存)
 **输入参数:
 **      fblk: 文件缓存链表头
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
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
 **函数名称: vars_exp_block_alloc
 **功    能: 从扩展空间(文件缓存)中申请内存
 **输入参数:
 **      vars: 变量池对象
 **      size: 申请空间的大小
 **输出参数:
 **      offset: 相对数据块起始位置的偏移量
 **返    回: 分配的空间地址
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.08 #
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
        /* 1. 判断当前文件缓存块空间是否足够 */
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
 **函数名称: vars_exp_block_free
 **功    能: 释放扩展扩展空间(文件缓存)的内存块空间
 **输入参数:
 **      vars: 变量池对象
 **      p: 被释放的内存起始地址
 **输出参数:
 **返    回: 0:Success  !0:Failed
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.07.18 #
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
 **函数名称: vars_exp_offset_to_addr
 **功    能: 将扩展空间(文件缓存)偏移量转换为内存地址
 **输入参数:
 **      vars: 变量池对象
 **      offset: 偏移量(相对数据块首地址)
 **输出参数:
 **返    回: 数据地址
 **实现描述: 
 **  1) 文件缓存如下所示:
 **     |<--                             文件缓存                                -->|
 **     -----------------------------------------------------------------------------
 **     |////////////////////////////|                                              |
 **     |\\\\\\\\\\\\\\\\\\\\\\\\\\\\|<--                未使用空间              -->|
 **     |////////////////////////////|                                              |
 **     -----------------------------------------------------------------------------
 **     ^                            ^                                              ^
 **     |                            |                                              |
 ** start_offset                   offset(相对start_offset)                     end_offset
 **注意事项: 
 **  1) start_offset, offset, end_offset: 其值是相对数据块起始地址的偏移量
 **作    者: # Qifeng.zou # 2013.05.08 #
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
 **函数名称: vars_sync
 **功    能: 将文件缓存中的数据同步到磁盘
 **输入参数:
 **      vars: 变量池对象
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.08 #
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
        if(1 || vars_get_dirty(fblk)) /* 暂时改为全部重写文件 */
        {
            offset = (fblk->head.start_offset - vars->data_size)
                        + (index * sizeof(vars_fblk_head_t));
            fseek(fp, offset, SEEK_SET);

            /* 1. 写文件块头 */
            ret = fwrite(&fblk->head, sizeof(vars_fblk_head_t), 1, fp);
            if(0 == ret)
            {
                pub_log_error("[%s][%d] errmsg:[%d] %s! [%s]",
                    __FILE__, __LINE__, errno, strerror(errno), fname);
                fclose(fp), fp=NULL;
                return ret;
            }
            
            /* 2. 写文件块缓存 */
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
 **函数名称: vars_exp_load
 **功    能: 将变量池扩展数据块载入内存
 **输入参数:
 **      vars: 变量池对象
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 是否有变量保存到文件中
 **     2. 获取文件名，并判断文件状态
 **     3. 创建缓存文件节点
 **     4. 将文件载入内存
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.09 #
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

    /* 1. 是否有变量保存到文件中 */
    if(!vars_get_fflag(vars))
    {
        vars->fblk = NULL;
        return 0;
    }

    /* 2. 获取文件名，并判断文件状态 */
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

    /* 3. 将文件载入内存 */
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

        /* 1. 读取文件块头 */
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
            /* 2. 读取文件缓存体 */
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
 **函数名称: vars_exp_clone
 **功    能: 克隆扩展空间数据
 **输入参数: 
 **     vars: 变量池引用对象
 **     vid: 目标变量池ID
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 克隆共享内存中的数据
 **     2. 克隆扩展空间中的数据
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.15 #
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

        /* 1. 写文件块头 */
        ret = fwrite(&fblk->head, sizeof(vars_fblk_head_t), 1, fp);
        if(0 == ret)
        {
            pub_log_error("[%s][%d] errmsg:[%d] %s! %s",
                __FILE__, __LINE__, errno, strerror(errno), fname);
            fclose(fp), fp=NULL;
            return ret;
        }
        
        /* 2. 写文件块缓存 */
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
 **函数名称: vars_exp_get_shm_key
 **功    能: 获取扩展共享内存KEY值
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 扩展共享内存KEY值
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
 ******************************************************************************/
key_t vars_exp_get_shm_key(void)
{
    struct timeval cur_time;

    memset(&cur_time, 0, sizeof(cur_time));

    gettimeofday(&cur_time, NULL);

    return ((VARS_SHM_BASE_KEY + random()*cur_time.tv_usec)&0x7FFFFFFF);
}

/******************************************************************************
 **函数名称: vars_exp_creat
 **功    能: 创建扩展空间(共享内存)
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 扩展共享内存KEY值
 **实现描述: 
 **     1. 创建扩展共享内存实例对象
 **     2. 创建扩展共享内存
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
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

        /* 共享内存已存在，则返回-1 */
        exp->shmid = shmget(exp->shmkey, total, IPC_CREAT|IPC_EXCL|0666);
        if(exp->shmid < 0)
        {
            if(-1 == exp->shmid)
            {
                continue;   /* 共享内存已存在，请重新申请 */
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
 **函数名称: vars_exp_destory
 **功    能: 销毁扩展空间
 **输入参数:
 **      head: 扩展共享内存信息
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     依次删除扩展共享内存
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
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
 **函数名称: vars_exp_link_free
 **功    能: 释放扩展链表
 **输入参数:
 **      fblk: 文件缓存链表头
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
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
 **函数名称: vars_exp_block_alloc
 **功    能: 从扩展空间(共享内存)中申请内存
 **输入参数:
 **      vars: 变量池对象
 **      size: 申请空间的大小
 **输出参数:
 **      offset: 相对数据块起始位置的偏移量
 **返    回: 分配的空间地址
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.14 #
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
        /* 1. 判断当前文件缓存块空间是否足够 */
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
 **函数名称: vars_exp_offset_to_addr
 **功    能: 将扩展空间(扩展共享内存)偏移量转换为内存地址
 **输入参数:
 **      vars: 变量池对象
 **      offset: 偏移量(相对数据块首地址)
 **输出参数:
 **返    回: 数据地址
 **实现描述: 
 **  1) 文件缓存如下所示:
 **     |<--                           扩展共享内存                              -->|
 **     -----------------------------------------------------------------------------
 **     |          |///////////////////|                                            |
 **     |  扩展头  |\\\\\\\\\\\\\\\\\\\|<--               未使用空间             -->|
 **     |          |///////////////////|                                            |
 **     -----------------------------------------------------------------------------
 **     ^          ^                   ^                                            ^
 **     |          |                   |                                            |
 **   addr    start_offset       offset(相对start_offset)                       end_offset
 **               data
 **注意事项: 
 **  1) start_offset, offset, end_offset: 其值是相对数据块起始地址的偏移量
 **作    者: # Qifeng.zou # 2013.05.14 #
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
 **函数名称: vars_exp_load
 **功    能: 将变量池扩展数据块载入内存
 **输入参数:
 **      vars: 变量池对象
 **输出参数:
 **返    回: 0: 成功 !0: 失败
 **实现描述: 
 **     1. 是否有变量保存到文件中
 **     2. 获取文件名，并判断文件状态
 **     3. 创建缓存文件节点
 **     4. 将文件载入内存
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.05.15 #
 ******************************************************************************/
static sw_int_t vars_exp_load(vars_object_t *vars)
{
    int ret = 0;
    key_t shmkey = 0;
    FILE *fp = NULL;
    vars_exp_t *exp = NULL, *new_exp = NULL;


    /* 1. 是否有扩展共享内存 */
    if(VARS_IPC_KEY_INVAILD == vars->head->next_key)
    {
        return 0;
    }

    /* 2. 打开扩展共享内存 */
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
            /* 出现异常 */
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
