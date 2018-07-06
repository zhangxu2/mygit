#include "uni_svr.h"

int ipc_infor_printf(uni_msipc_t *msipc)
{
	int i = 0, shm_nm = 0, msg_nm = 0, sem_nm = 0; 
	
	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		if (msipc[i].used == 0)
		{
			break;
		}
		
		if (msipc[i].uni_shm.shmid > 0)
		{
			shm_nm++;
		}
		else if(msipc[i].uni_msg.mqid > 0)
		{
			msg_nm++;
		}
		else if(msipc[i].uni_sem.semid > 0)
		{
			sem_nm++;
		}
	}
	
	system("clear");
	printf("\n");
	printf("-------------------------------------------------IPC资源监控-----------------------------------------------------\n");
	printf("共享内存数量:%d       消息队列数量:%d        信号灯数量:%d      \n", shm_nm, msg_nm, sem_nm);
	printf("-----------------------------------------------------------------------------------------------------------------\n");
	printf("共享内存(SHM)相关信息：\n");
	printf("-----------------------------------------------------------------------------------------------------------------\n");
	printf("  SHMID      SHMSIZE      CREATEPID      SHM_CTIME       SHM_NATTCH     SHM_LPID     SHM_PNAME       \n");
	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		if (msipc[i].used == 0)
		{
			break;
		}
		if (msipc[i].uni_shm.shmid > 0)
		{
			printf("%-12d %-12d %-14d %-16d %-13d %-12d %-14s \n" \
				, msipc[i].uni_shm.shmid \
				, msipc[i].uni_shm.shm_size \
				, msipc[i].uni_shm.shm_cpid \
				, msipc[i].uni_shm.shm_ctime \
				, msipc[i].uni_shm.shm_nattch \
				, msipc[i].uni_shm.shm_lpid \
				, msipc[i].uni_shm.pname);
		}
	}
	printf("-----------------------------------------------------------------------------------------------------------------\n");
	printf("消息队(MSG)列相关信息: \n");
	printf("-----------------------------------------------------------------------------------------------------------------\n");
	printf(" MSGID       MSG_MAX      MSG_CBYTE      MSG_QNUM        MSG_LTIME      MSG_PID      MSG_PNAME       \n");
	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		if (msipc[i].used == 0)
		{
			break;
		}
		if (msipc[i].uni_msg.mqid > 0)
		{
			printf("%-12d %-12d %-14d %-16d %-13d %-12d %-14s \n" \
				, msipc[i].uni_msg.mqid \
				, msipc[i].uni_msg.msg_qbytes \
				, msipc[i].uni_msg.msg_cbytes \
				, msipc[i].uni_msg.msg_qnum \
				, msipc[i].uni_msg.msg_stime \
				, msipc[i].uni_msg.msg_pid \
				, msipc[i].uni_msg.pname);
		}
	}
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("信号灯(SEM)相关信息：\n");
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf(" SEMID        SEMOTIME     SEMCTIME      SEM_NSEMS  \n");
	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		if (msipc[i].used == 0)
		{
			break;
		}
		if (msipc[i].uni_sem.semid > 0)
		{
			printf("%-12d %-12d %-14d %-10d   \n" \
			, msipc[i].uni_sem.semid \
			, msipc[i].uni_sem.sem_otime \
			, msipc[i].uni_sem.sem_ctime \
			, msipc[i].uni_sem.sem_nsems);
		}
		
	}
	printf("------------------------------------------------------------------------------------------------------------------\n");
	return 0;
}

int main(int argc, char * argv[])
{
	int	i = 0;
	int	ret = 0;
	int	num = 0;
	int	time = 0;
	char	quite;
	char	log_path[64];
	fd_set	fds;
	struct timeval	timeout;
	uni_msipc_t	msipc[UNI_MAX_IPC_CNT];
	
	memset(&timeout, 0x00, sizeof(timeout));
	
	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		memset(&msipc[i], 0x00, sizeof(uni_msipc_t));
	}
	
	memset(log_path, 0x00, sizeof(log_path));
	sprintf(log_path, "%s/log/syslog/swmipc.log", getenv("SWWORK"));
	pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	
	num = 1;
	time = 0;
	if (argc == 2)
	{
		time = atoi(argv[1]);
		num = 50000;
	}
	else if (argc >= 3)
	{
		time = atoi(argv[1]);
		num = atoi(argv[2]);
	}
	
	for (i = 0; i < num; i++)
	{
		memset(msipc, 0x0, sizeof(uni_msipc_t) * UNI_MAX_IPC_CNT);
		ret = uni_get_ipc_info(msipc);
		if (ret < 0)
		{
			printf("Get IPC info error!\n");
			return -1;
		}
		ipc_infor_printf(msipc);
		
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		memset(&timeout, 0x0, sizeof(timeout));
		timeout.tv_sec = time;
		if (select(FD_SETSIZE, &fds, NULL, NULL, &timeout) > 0)
		{
			if (FD_ISSET(0, &fds))
			{
				quite = getchar();
				if (quite == 'q')
				{
					return 0;
				}
			}
		}
	}
	
	return 0;
}
