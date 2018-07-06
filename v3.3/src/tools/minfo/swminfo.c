#include "cycle.h"
#include "pub_type.h"
#include "mtype.h"
#include "sem_lock.h"
#include "signal.h"

int	type = 0;
sw_trace_item_t	*trace_item;

extern int ares_count_str(char *buf, char *substr);

int ares_mtype_print(int sockfd, char *buf, int filefd)
{
	int	cnt = 0;
	int	ret = 0;
	char	tmp[2048];
	char	*p = NULL;
	char	*ptr = NULL;
	
	if (buf == NULL)
	{
		pub_log_error("[%s][%d] Input buffer is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ret = ares_comm(sockfd, buf, 0, MTYPE_PRINT);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares get mtype info error!",
			__FILE__, __LINE__);
		return SW_ERROR;
	}

	cnt = ares_count_str(buf, ARES_SEP);
	if (cnt != 2 && cnt != 3)
	{
		pub_log_error("[%s][%d] Ares response result error!",
			__FILE__, __LINE__);
		return SW_ERROR;
	}
	
	ptr = strstr(buf, ARES_SEP);
	if (ptr != NULL)
	{
		memset(tmp, 0x0, sizeof(tmp));
		memcpy(tmp, buf, ptr - buf);
		printf("\t%s\n", tmp);
	}
		
	if (cnt == 3)
	{
		ptr += ARES_SEP_LEN;
		ptr = strstr(ptr, ARES_SEP);
		if (ptr != NULL)
		{
			ptr += ARES_SEP_LEN;
			p = ptr;
			ptr = strstr(p, ARES_SEP);
			if (ptr != NULL)
			{
				write(filefd, p, ptr - p);
			}
		}
	}

	return SW_OK;
}

void mtype_print(int fd, sw_mtype_t *addr)
{
	if (g_mtype_in_ares)
	{
		int	ret = 0;
		char	buf[256*1024];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_mtype_print(g_ares_fd, buf, fd);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] mtype_print result deal error!", __FILE__, __LINE__);
			return ;
		}
		
		return ;
	}

	int     i = 0;
	int	len = 0;
	int     used = 0;
	int     idle = 0;
	char	tmp[128];
	char	line[2048];
	sw_mtype_head_t *head;
	sw_mtype_node_t *item;
	sw_int32_t      lock_id;
	sw_int64_t	now = 0;

	head = &addr->head;
	item = &addr->first;
	
	now = pub_time_get_current();
	lock_id = head->lock_id;
	sem_mutex_lock(lock_id);
	for (i = 0; i < head->mtype_max; i++)
	{
		switch (item->flag)
		{
			case MTYPE_TAIL:
			case MTYPE_IDLE:
			{
				idle++;
				break;
			}
			case MTYPE_USED:
			{
				used++;
				break;
			}
			default:
			{
				break;
			}
		}
		item++;
	}
	if (used > 0)
	{
		memset(tmp, 0x0, sizeof(tmp));
		pub_change_time(now, tmp, 5);
		memset(line, 0x0, sizeof(line));
		sprintf(line, "--------------------------------Used[%d][%s]-------------------------------\n", used, tmp);
		len = strlen(line);
		write(fd, line, len);
		item = &addr->first;
		for (i = 0; i < head->mtype_max; i++)
		{
			if (item->flag == MTYPE_USED)
			{
				memset(line, 0x0, sizeof(line));
				memset(tmp, 0x0, sizeof(tmp));
				pub_change_time(trace_item[i].start_time, tmp, 5);
				sprintf(line, "MTYPE:[%d] TRACE_NO:[%lld] BUSI_NO:[%lld] PRDT:[%s] CHNL:[%s] TXCODE:[%s] SERVER:[%s] SERVICE:[%s] STARTTIME:[%s]\n", 
				i + 1, trace_item[i].trace_no, trace_item[i].busi_no, trace_item[i].prdt_name, 
				trace_item[i].chnl, trace_item[i].tx_code, trace_item[i].server, trace_item[i].service, tmp);
				len = strlen(line);
				write(fd, line, len);
				
			}
			item++;
		}
		memset(line, 0x0, sizeof(line));
		sprintf(line, "-----------------------------------------------------------------------------------------\n");
		len = strlen(line);
		write(fd, line, len);
	}
	printf("\t%-14d%-14d%-14d%-14d%-14d\n", head->mtype_max, used, idle, head->succ_cnt, head->fail_cnt);
	sem_mutex_unlock(lock_id);
}

void mtype_out()
{
	type = -1;
}

int main(int argc, const char *argv[])
{
	int	n = 1;
	int	fd = 0;
	int	ret = 0;
	int	time = 0;
	char	filename[128];
	sw_mtype_t      *addr = NULL;
	sw_cycle_t      cycle;
	sw_trace_info_t	*trace_info = NULL;
	
	memset(filename, 0x0, sizeof(filename));
	pub_mem_memzero(&cycle, sizeof(cycle));
	ret = cycle_init(&cycle, "minfo", ND_ADM, "error.log", "swinfo.log", NULL);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, mtype_cycle_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = cycle_link_shm_run(&cycle);
	if (ret != SW_OK)
	{
		pub_log_error("%s, %d, mtype_run_link error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (!g_mtype_in_ares)
	{
		addr = (sw_mtype_t *)mtype_get_addr();
		trace_info = trace_get_addr();
		trace_item = trace_info->trace_item;
	}
	if (argc == 2)
	{
		n = -1;
		time = atoi(argv[1]);
		if (time == 0)
		{
			return -1;
		}
	}

	if (argc >= 3)
	{
		n = atoi(argv[2]);
		time = atoi(argv[1]);
		if (time == 0)
		{
			return -1;
		}
	}
	
	sprintf(filename, "%s/log/syslog/swminfo.txt", getenv("SWWORK"));
	fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd < 0)
	{
		printf("Can not open file [%s]! errno=[%d]:[%s]\n", filename, errno, strerror(errno));
		return -1;
	}
	signal(SIGINT, mtype_out);
	printf("\t%-14s%-14s%-14s%-14s%-14s\n", "TOTAL", "USED", "IDEL", "SECCESS", "FAIL");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	for (; n != 0; n--)
	{
		mtype_print(fd, addr);
		sleep(time);
		if (type == -1)
		{
			break;
		}
	}
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	close(fd);
	
	cycle_destory(&cycle);
	return 0;
}
