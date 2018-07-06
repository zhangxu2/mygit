#include "msg_trans.h"

static int arrFdKeys[SW_FD_MAX];

int msg_trans_open(sw_global_path_t *path, int mqid)
{
	int	fd = 0;
	int	elts = 0;
	char	sFifoName[256];

	if (path == NULL || mqid < 0)
	{
		pub_log_error("[%s][%d] input mqid [%d] error!", __FILE__, __LINE__, mqid);
		return -1;
	}
	/* Open FIFO */
	sprintf(sFifoName,"%s/.x%x.fifo", path->datapath, mqid);
	fd = pub_fifo_open(sFifoName);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] fifo open error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERR;
	}
	/****保存fd和key的对应关系****/ 
	elts = sizeof(arrFdKeys) / sizeof(int);
	if (fd > elts)
	{
		pub_log_error("open max file fd[%d] > [%d]", fd, elts);
		return SW_ERR;
	}
	arrFdKeys[fd] = mqid;

	return fd;
}

int msg_load_fd_by_mqid(int mqid)
{
	int	fd = 0;

	for (fd = 0; fd < SW_FD_MAX; fd++)
	{
		if (arrFdKeys[fd] == mqid)
		{
			break;
		}
	}

	if (fd < SW_FD_MAX )
	{
		return fd;
	}
	return SW_ERR;
}

int msg_trans_create(sw_global_path_t *path, key_t key, int nSize, int*mqid)
{
	int	fd = 0;
	int	elts = 0;
	int	tmp = 0;
	char	sFifoName[256];

	if ( path == NULL )
	{
		pub_log_error("[%s][%d],input argv illegal [%d]",__FILE__,__LINE__,key);
		return SW_ERR;
	}

	/* Open Mq */
	tmp = pub_msq_creat(key, nSize);
	if (tmp < 0 )
	{
		pub_log_error("[%s][%d],errno=[%d]\n",__FILE__,__LINE__,errno);
		return SW_ERR;
	}

	/* Open FIFO */
	sprintf(sFifoName,"%s/.x%x.fifo",path->datapath, tmp);
	fd = pub_fifo_open(sFifoName);
	if(fd<0)
	{
		pub_log_error(" pub_fifo_open error! ");
		pub_msq_rm(tmp);
		return SW_ERR;
	}
	/****保存fd和key的对应关系****/	
	elts = sizeof(arrFdKeys) / sizeof(int);
	if (fd > elts)
	{
		pub_log_error(" open max file fd[%d] > 1024 ",fd);
		pub_msq_rm(tmp);
		pub_fifo_close(fd);
		pub_fifo_del(sFifoName);
		return SW_ERR;
	}
	arrFdKeys[fd] = tmp;
	*mqid = tmp;

	return fd;
}

int msg_trans_rm(sw_global_path_t *path, int fd)
{
	int	ret = 0;
	int	mqid = 0;
	int	elts = 0;
	char	sFifoName[256];

	if( path == NULL || fd <= 0)
	{
		pub_log_error("[%s][%d],input fd [%d] \n",__FILE__,__LINE__,fd);
		return -1;
	}

	mqid = arrFdKeys[fd];	
	if(mqid < 0)
	{
		pub_log_error("%s,%d,get mqid error! [%d]",__FILE__,__LINE__,mqid);
		return SW_ERR;	
	}	
	/* rm Mq */
	ret = pub_msq_rm(mqid);
	if (ret < 0 )
	{
		pub_log_error("[%s][%d],errno=[%d]\n",__FILE__,__LINE__,errno);
		return SW_ERR;
	}
	/*close fifo fd*/
	pub_fifo_close(fd);
	/* Open FIFO */
	sprintf(sFifoName,"%s/.x%x.fifo",path->datapath, mqid);
	fd = pub_fifo_del(sFifoName);
	if(fd<0)
	{
		pub_log_error(" pub_fifo_open error! ");
		return SW_ERR;
	}
	/****保存fd和key的对应关系****/	
	elts = sizeof(arrFdKeys)/sizeof(int);
	if (fd > elts)
	{
		pub_log_error(" open max file fd[%d] > 1024 ",fd);
		return SW_ERR;
	}
	arrFdKeys[fd] = -1;
	pub_log_info("[%s][%d] RM MQID [%d] SUCCESS!", __FILE__, __LINE__, mqid);

	return SW_OK;
}


int msg_trans_rcv(int iFd,char *pBuf,long *mtype,int *piLen)
{

	sw_int32_t	ret = 0;
	sw_int32_t	mqid = 0;

	mqid = arrFdKeys[iFd];
	if(mqid < 0)
	{
		pub_log_error("%s,%d,get mqid error! [%d]",__FILE__,__LINE__, iFd);
		return SW_ERR;	
	}	

	/* read a char for test mq */
	ret = pub_fifo_get(iFd);
	if (ret != SW_OK)
	{
		pub_log_error("%s,%d,read a word from fifo error! fd[%d] err[%d]",__FILE__,__LINE__, iFd,sw_errno);
		return SW_ERR;	

	}

	/****gt msg from mq****/	
	ret =  pub_msq_get(mqid,pBuf,mtype,piLen);
	if (ret != SW_OK)
	{
		pub_log_error("%s,%d,pub_msq_get error! fd[%d] err[%d]",__FILE__,__LINE__, iFd,sw_errno);
		return SW_ERR;
	}
	pub_log_info("%s,%d,  read msg from mq mtype=[%ld] length==[%d] ", __FILE__, __LINE__, *mtype, *piLen);		

	return(SW_OK);	
}


/*(int nMqId,char *pcBuffer,int nLength,int nMtype)*/
int msg_trans_send(int iFd,char *psBuf,long mtype,int iLen)
{
	int	ret = 0;
	int	mqid = 0;
	char	path[256];

	mqid = arrFdKeys[iFd];	
	if (mqid < 0)
	{
		pub_log_error("%s,%d,get mqid error! [%d]",__FILE__,__LINE__,iFd);
		return SW_ERR;	
	}	

	/*放消息之前，先取出队列中残留的同类型消息*/
	memset(path, 0x00, sizeof(path));
	snprintf(path, sizeof(path), "%s/tmp/mqmsg.txt", getenv("SWWORK"));
	ret = pub_msq_clear(mqid, mtype, path);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get msq clear error,errno[%d]", __FILE__, __LINE__, sw_errno);
		return SW_ERR;
	}
	
	ret = pub_msq_put(mqid,psBuf,iLen,mtype);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_msq_put error! errno[%d]",__FILE__,__LINE__,sw_errno);
		return SW_ERR;
	}
	/* 向管道中写入一个字节的信息 */
	ret = pub_fifo_put(iFd);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] pub_fifo_put error!errno[%d]",__FILE__,__LINE__,sw_errno);
		return SW_ERR;
	}	

	return SW_OK;
}

sw_int_t msg_trans_close_all()
{
	sw_int32_t fd;
	for(fd = 0;fd < SW_FD_MAX ;fd++)
	{
		if (arrFdKeys[fd] > 0)
		{
			close(fd);
		}
	}
	return SW_OK;
}

sw_int_t cmd_print(sw_cmd_t *cmd)
{
	pub_log_info("cmdinfo:\n\tsdate:%s|trc:%lld|mtype:%ld|msgtp:%d|taskflag:%d|type:%d|startline:%d|timeout:%d|\n\tdsttype:%d|dstprdt:%s|dstsvr:%s|dstsvc:%s|\n\toriprdt:%s|orisvr:%s|orisvc:%s|udpname:%s|def:%s|lsn:%s|level:%d|", 
			cmd->sys_date,
			cmd->trace_no,
			cmd->mtype,
			cmd->msg_type,
			cmd->task_flg,
			cmd->type,
			cmd->start_line,
			cmd->timeout,
			cmd->dst_type,
			cmd->dst_prdt,
			cmd->dst_svr,
			cmd->dst_svc,
			cmd->ori_prdt,
			cmd->ori_svr,
			cmd->ori_svc,
			cmd->udp_name,
			cmd->def_name,
			cmd->lsn_name,
			cmd->level);

	return 0;
}


