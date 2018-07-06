#include <sys/mman.h>
#include <time.h>
#include "agent_comm.h"

/*字符串大写转小写*/
char *agt_str_tolower(char *str)
{
	char	*ptr = str;

	while (*ptr != '\0')
	{
		if (*ptr >= 'A' && *ptr <= 'Z')
		{
			*ptr = *ptr + ('a' - 'A');
		}
		ptr++;
	}

	return str;
}

/*执行命令， 忽视信号SIGCHLD*/
int agt_system(char *cmd)
{
	int	result = -1;

	signal(SIGCHLD,SIG_DFL);
	result = system(cmd);
	signal(SIGCHLD,SIG_IGN);

	return result;
}

/*执行命令， 结果保存在buf中*/
int agt_popen(char *cmd, char *buf)
{
	int	len = 0;
	char	buf_tmp[128];
	FILE	*fp = NULL;

	pub_mem_memzero(buf_tmp, sizeof(buf_tmp));

	strcat (cmd, " 2>/dev/null");
	fp = popen(cmd, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] POPEN error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	fgets(buf_tmp, sizeof(buf_tmp) - 1, fp);
	pclose(fp);

	pub_str_trim(buf_tmp);
	len = pub_str_strlen(buf_tmp);
	if (buf_tmp[len - 1] == '\n')
	{
		buf_tmp[len - 1] = '\0';
	}

	strcpy(buf, buf_tmp);

	return SW_OK;
}

/*按照分隔符拆分字符串*/
int agt_str_split(char *buf, char *type, char item[][256], int cnt)
{
	int	i = 0;
	char	*pr = NULL;

	if (buf == NULL)
	{
		return -1;
	}

	i = 0;

	while ((pr = strsep(&buf, type)) != NULL && i < cnt)
	{
		strncpy(item[i], pr, 256-1);
		i++;
	}

	return 0;
}

/*将时间输出为制定格式的字符串*/
int agt_date_format(long time, char *date, char *type)
{
	struct tm	dt;

	memset(&dt, 0x0, sizeof(dt));
	localtime_r(&time, &dt);
	if (strcmp(type, "A") == 0)
	{
		sprintf(date, "%04d%02d%02d", 1900 + dt.tm_year, 1 + dt.tm_mon, dt.tm_mday);
	}
	else if (strcmp(type, "B") == 0)
	{
		sprintf(date, "%02d:%02d:%02d", dt.tm_hour, dt.tm_min, dt.tm_sec);
	}
	else if (strcmp(type, "C") == 0)
	{
		sprintf(date, "%04d%02d%02d%02d%02d%02d", 1900 + dt.tm_year, 1 + dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
	}
	else if (strcmp(type, "D") == 0)
	{
		sprintf(date, "%04d-%02d-%02d-%02d:%02d", 1900 + dt.tm_year, 1 + dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min);
	}
	else if (strcmp(type, "E") == 0)
	{
		sprintf(date, "%02d%02d%02d", dt.tm_hour, dt.tm_min, dt.tm_sec);
	}
	else if (strcmp(type, "F") == 0)
	{
		sprintf(date, "%04d%02d%02d-%02d%02d%02d", 1900 + dt.tm_year, 1 + dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
	}
	return 0;
}

/*判断处理码成功或失败*/
int agt_check_stat(char *result)
{
	if (result == NULL || strlen(result) == 0)
	{
		return -1;
	}

	if (strstr(result, "0000") == NULL)
	{
		return -1;
	}

	return 0;
}

/*获取产品中文名*/
int agt_get_pdt_name(char *pdt, char *pdtname, int len)
{
	int 	res = 0;
	char	xml_path[512];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL, *node1 = NULL;

	memset(xml_path, 0x00, sizeof(xml_path));
	sprintf(xml_path, "%s/cfg/products.xml", getenv("SWWORK"));

	res = access(xml_path, F_OK);
	if (res)
	{
		strcpy(pdtname, pdt);
		return -1;
	}

	xml = pub_xml_crtree(xml_path);
	if (xml == NULL)
	{
		strcpy(pdtname, pdt);
		return -1;
	}

	node = pub_xml_locnode(xml, ".DFISBP.PRODUCT");
	while (node != NULL)
	{
		node1 = node->firstchild;
		while (node1 != NULL)
		{
			if (node1->name != NULL && strlen(node1->name) > 0)
			{
				if (strcmp(node1->name, "CNNAME") == 0)
				{
					if (node1->value != NULL && strlen(node1->value) > 0)
					{
						strncpy(pdtname, node1->value, len);
						if (strcmp(pdt, node1->next->next->value) == 0)
						{
							pub_log_debug("[%s][%d]pdtname=[%s]", __FILE__,__LINE__, pdtname);	
							pub_xml_deltree(xml);
							return 0;
						}
						memset(pdtname,0x00,strlen(pdtname));
					}
				}
			}
			node1 = node1->next;
		}
		node = node->next;
	}

	strcpy(pdtname, pdt);
	pub_xml_deltree(xml);

	return -1;
}

/*获取渠道中文名*/
int agt_get_lsn_name(char *lsn, char *lsnname)
{
	int	res = -1;
	char	xml_path[512];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL, *node1 = NULL;

	memset(xml_path, 0x00, sizeof(xml_path));
	sprintf(xml_path, "%s/cfg/channels.xml", getenv("SWWORK"));

	res = access(xml_path, F_OK);
	if (res)
	{
		pub_log_error("[%s][%d]the chl=[%s] is not exist!\n", __FILE__, __LINE__, xml_path);
		strcpy(lsnname, lsn);
		return -1;
	}

	xml = pub_xml_crtree(xml_path);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",__FILE__, __LINE__, xml_path);
		strcpy(lsnname, lsn);
		return -1;
	}

	node = pub_xml_locnode(xml, ".DFISBP.CHANNEL");
	while (node != NULL)
	{
		node1 = node->firstchild;
		while (node1 != NULL)
		{
			if (node1->name != NULL && strlen(node1->name) > 0)
			{
				if (strcmp(node1->name, "CNNAME") == 0)
				{
					if (node1->value != NULL && strlen(node1->value) > 0)
					{
						strcpy(lsnname, node1->value);
						if (strcmp(lsn, node1->next->next->value) == 0)
						{
							pub_log_debug("[%s][%d]lsnname=[%s]",__FILE__,__LINE__,lsnname);	
							pub_xml_deltree(xml);
							return 0;
						}
						memset(lsnname,0x00,strlen(lsnname));
					}
				}
			}
			node1 = node1->next;
		}
		node = node->next;
	}

	strcpy(lsnname, lsn);
	pub_xml_deltree(xml);

	return -1;
}

/*设置出错信息*/
int agt_error_info(char *code, char *desc)
{
	if (code == NULL || desc == NULL)
	{
		pub_log_error("[%s][%d] input parameter error, please cheack!", __FILE__, __LINE__);
		return -1;
	}

	if (strlen(code) == 0)
	{
		strcpy(code, "E999");
	}	

	if (strlen(desc) > 0)
	{
		return 0;
	}

	int	ret = 0;
	int	flag = 0;
	char	err_info[256];
	char	buf[512];
	char	*s = NULL;
	char	*e = NULL;
	FILE	*pf = NULL;

	memset(err_info, 0x0, sizeof(err_info));
	sprintf(err_info, "%s/cfg/agentcfg/common/agent_error.xml", getenv("SWWORK"));

	ret = access(err_info, F_OK);
	if (ret)
	{
		pub_log_error ("[%s][%d] not exsist the file [%s]\n", __FILE__, __LINE__, err_info);
		strcpy(desc, "交易失败");
		return -1;
	}

	pf = fopen(err_info, "r");
	if (pf == NULL)
	{
		pub_log_error("[%s][%d] fpoen file [%s] is failed, errno[%d]\n", __FILE__, __LINE__, err_info, errno);
		strcpy(desc, "交易失败");
		return -1;
	}

	while(!feof(pf))
	{
		memset(buf, 0x00, sizeof(buf));
		fgets(buf, sizeof(buf), pf);

		if (strstr(buf, code) != NULL )
		{
			flag = 1;
		}
		if ((flag == 1) && (strstr(buf, "DESC") != NULL))
		{
			s = strstr(buf, "=");
			e = strstr(s, "/");
			strncpy(desc, s + 2, e - s -3);
			fclose(pf);
			return 0;
		}
	}

	fclose(pf);
	strcpy(desc, "交易失败");
	return -1;
}

/*获取目录下所有文件并排序*/
struct dirent **agt_scan_dir(int *n,  char *path)
{
	struct dirent **namelist;

	(*n) = scandir(path, &namelist, 0, alphasort);
	if ((*n) < 0)
	{
		pub_log_error("[%s][%d]scandir error", __FILE__, __LINE__);
		return NULL;
	}

	return namelist;
}

/*释放文件列表*/
int agt_free_namelist(struct dirent **namelist, int n)
{
	int i;
	for(i = 0; i < n; i++)
	{
		free(namelist[i]);
	}

	free(namelist);

	return 0;
}

/*隐藏账号*/
int agt_hide_account(char *account)
{
	int	len = strlen(account);
	int	i = 0;

	if(len <= 10)
	{
		return SW_OK;
	}
	for(i = 6; i < len-4; i++)
	{
		account[i] = '*';
	}

	return SW_OK;
}

/*解析monitor.log文件中的TOTAL信息*/
int agt_get_trace_info(char *str, sw_trace_item_t *trace_item, char *ptid)
{
	int	num = 0;
	char	item[32][256];

	memset(item, 0x00, sizeof(item));
	agt_str_split(str + 6, "|", item, 32); 

	memset(trace_item, 0x0, sizeof(sw_trace_item_t));
	strncpy(trace_item->tx_code, item[0], sizeof(trace_item->tx_code));
	trace_item->trace_no = atoll(item[1]);
	pub_str_ziphlspace(item[2]);
	strncpy(trace_item->server, item[2], sizeof(trace_item->server));
	pub_str_ziphlspace(item[3]);
	strncpy(trace_item->service, item[3], sizeof(trace_item->service));
	strncpy(trace_item->start_date, item[4], sizeof(trace_item->start_date));
	trace_item->start_time = atoll(item[5]);
	trace_item->end_time = atoll(item[6]);
	strncpy(trace_item->tx_respcd, item[7], sizeof(trace_item->tx_respcd));
	strncpy(trace_item->prdt_name, item[8], sizeof(trace_item->prdt_name));
	strncpy(trace_item->tx_amt, item[9], sizeof(trace_item->tx_amt));
	strncpy(trace_item->dr_ac_no, item[10], sizeof(trace_item->dr_ac_no));
	strncpy(trace_item->cr_ac_no, item[11], sizeof(trace_item->cr_ac_no));
	strncpy(trace_item->dr_ac_name, item[12], sizeof(trace_item->dr_ac_name));
	strncpy(trace_item->cr_ac_name, item[13], sizeof(trace_item->cr_ac_name));
	strncpy(trace_item->fee_amt, item[14], sizeof(trace_item->fee_amt));
	strncpy(trace_item->fee_ac_no, item[15], sizeof(trace_item->fee_ac_no));
	strncpy(trace_item->ct_ind, item[16], sizeof(trace_item->ct_ind));
	strncpy(trace_item->dc_ind, item[17], sizeof(trace_item->dc_ind));
	strncpy(trace_item->chnl, item[18], sizeof(trace_item->chnl));
	strncpy(trace_item->time, item[19], sizeof(trace_item->time));
	strncpy(trace_item->sys_errcode, item[20], sizeof(trace_item->sys_errcode));
	trace_item->busi_no = atoll (item[22]);
	strncpy(trace_item->resflag, item[23], sizeof(trace_item->resflag));
	strcpy(ptid, item[24]);

	if (agt_check_stat(trace_item->tx_respcd) == 0)
	{
		trace_item->flag = 1;
	}
	else
	{
		trace_item->flag = 0;
	}

	if (strlen(item[21]) == 0)
	{
		if (trace_item->flag == 1)
		{
			strcpy(trace_item->tx_errmsg, "交易成功");
		}
		else
		{
			strcpy(trace_item->tx_errmsg, "交易失败");
		}
	}
	else
	{
		strncpy(trace_item->tx_errmsg, item[21], sizeof(trace_item->tx_errmsg) - 1);
	}

	if (strlen(trace_item->tx_code) == 0)
	{
		strncpy(trace_item->tx_code, "other", sizeof(trace_item->tx_code)-1);
	}

	if (strlen(trace_item->tx_respcd) == 0)
	{
		strncpy(trace_item->tx_respcd, "other", sizeof(trace_item->tx_respcd)-1);
	}
	if (strlen(trace_item->prdt_name) == 0)
	{
		strncpy(trace_item->prdt_name, "other", sizeof(trace_item->prdt_name));
	}
	if (strlen(trace_item->chnl) == 0)
	{
		strncpy(trace_item->chnl, "other", sizeof(trace_item->chnl));
	}

	return 0;
}

/*将字符串时间转为秒数*/
long agt_date_to_time(char *date)
{
	char	tmp[32];
	time_t	cur;
	struct tm	tim;

	if (strchr(date, '-')  == NULL)
	{
		memset(tmp, 0x0, sizeof(tmp));
		memcpy(tmp, date, 4);
		strcat(tmp, "-");
		memcpy(tmp+5, date+4, 2);
		strcat(tmp, "-");
		memcpy(tmp+8, date+6, 2);
		memset(date, 0x0, 11);
		memcpy(date, tmp, 10);
	}	
	memset(&tim, 0x00, sizeof(tim));
	cur = time(NULL);
	localtime_r(&cur, &tim);
	memset(tmp, 0x00, sizeof(tmp));
	sprintf(tmp, "%s-00:00:00",date);
	strptime(tmp, "%Y-%m-%d-%H:%M:%S", &tim);
	return mktime(&tim);
}

/*释放point链表*/
int agt_free_point(struct point *phead)
{
	struct point	*cur = NULL;

	while (phead != NULL)
	{
		cur = phead;

		phead = phead->next;
		if (cur->pdt != NULL)
		{
			agt_free_point(cur->pdt);
		}
		free(cur);
	}

	return 0;
}

/*插入point链表*/
struct point *agt_insert_point(struct point *phead, char *name, int right, int response, long long d_time, char *pdt)
{
	struct point	*ppoint = NULL, *ptmp = NULL;

	if (phead == NULL)
	{
		ppoint = (struct point *)malloc(sizeof(struct point));
		if (ppoint == NULL)
		{
			pub_log_error("[%s][%d] malloc is failed!\n", __FILE__, __LINE__);
			return NULL;
		}

		phead = ppoint;
		memset(ppoint, 0x00, sizeof(struct point));
		strncpy(ppoint->name, name, sizeof(ppoint->name)-1);
	}
	else
	{
		for (ppoint = phead; ppoint != NULL; ppoint = ppoint->next)
		{
			if (strcmp(ppoint->name, name) == 0)
			{
				break;
			}
			ptmp = ppoint;
		}

		if (ppoint == NULL)
		{
			ppoint = (struct point *)malloc(sizeof(struct point));
			if (ppoint == NULL)
			{
				pub_log_error("[%s][%d] malloc is failed!\n", __FILE__, __LINE__);
				return NULL;
			}
			memset(ppoint, 0x00, sizeof(struct point));
			strncpy(ppoint->name, name, sizeof(ppoint->name)-1);

			ptmp->next = ppoint;
		}
	}

	ppoint->num++;
	if (right)
	{
		ppoint->right++;
	}

	ppoint->d_time += d_time;

	ppoint->response += response;

	if (pdt != NULL && strlen(pdt) > 0)
	{
		ppoint->pdt = agt_insert_point(ppoint->pdt, pdt, right, response, d_time, NULL);
	}

	return phead;
}

/*链表排序*/
struct point *agt_rank_point(struct point *phead, agt_point_cmp_pt point_cmp)
{
	if (phead == NULL || point_cmp == NULL)
	{
		pub_log_debug("[%s][%d]input error", __FILE__, __LINE__);
		return phead;		
	}
	
	int	n = 0;
	int	i = 0, j = 0;
	struct point	*ptmp = NULL;
	struct point	**pppoint = NULL;

	for (ptmp = phead, n = 0; ptmp != NULL; ptmp = ptmp->next)
	{
		++n;	
	}		

	pppoint = calloc(n, sizeof(struct point *));
	if (pppoint == NULL)
	{
		pub_log_error("[%s][%d]calloc error", __FILE__, __LINE__);
		return phead;	
	}

	for (ptmp = phead, i = 0; ptmp != NULL; ptmp = ptmp->next,  ++i)
	{
		pppoint[i] = ptmp;	
	}

	for(i = 0; i < n - 1; ++i)
	{
		for(j = i + 1; j < n; ++j)
		{
			if (point_cmp(pppoint[i], pppoint[j]) < 0)
			{
				ptmp = pppoint[i];
				pppoint[i] = pppoint[j];
				pppoint[j] = ptmp;	
			}	
		}
	}

	phead = pppoint[0];
	for (i = 0; i < n - 1; ++i)
	{
		pppoint[i]->next = pppoint[i+1];
	}
	
	pppoint[n - 1]->next = NULL;
	free(pppoint);		
	return phead;	
}

/*按数目比较*/
int agt_point_cmp_num(struct point *p1, struct point *p2)
{
	return p1->num - p2->num;
}

/*成功率*/
float agt_get_avgright(int right, int sum)
{
	float	avg = 0;

	if (sum > 0)
	{
		avg = (float)right / sum * 100;
	}

	if (right > 0 && avg <= 0.01)
	{
		avg = 0.01;
	}
	else if (right < sum && avg >= 99.99 )
	{
		avg = 99.99;
	}

	return avg;	
}

/*平均时间*/
float agt_get_avgtime(long long d_time, int total)
{
	if (total > 0)
	{
		return (float)d_time/(total * 1000);
	}
	else
	{
		return 0;
	}
}

/*由精确到微秒的时间得到小时*/
static int get_hour(long long time)
{
	time_t	curt = 0;
	struct tm	sttm;

	curt = time / 1000000;
	localtime_r(&curt, &sttm);
	return sttm.tm_hour;
}

/*倒着截取一行信息*/
static char *cut(char *begin, char *end)
{
	char	*p = NULL;

	if(begin >= end)
	{
		return NULL;
	}

	for (p = end - 1; p != begin && *p != '\n'; --p)
	{
	}
	if(p != begin)
	{
		*p = '\0';
		++p;
	}

	return p;
}

/*按照条件查询交易信息*/
int agt_get_trc(sw_loc_vars_t *vars, int page_idx, int page_cnt, struct search *searchs, int flags, agt_set_trace_pt set_trace_info)
{
	int	ret = 0;
	int	cnt = 0;
	int	page_sum = 0;
	int	high = 0, low = 0;
	int	flag = 0;
	int	i, n = 0;
	int	num = 0;
	int	fd = 0;
	int	right = 0;
	int	wrong = 0;
	int quit = 0;
	char	sts[8];
	char	today[32];
	char	log_path[256];
	char	sbuf[1024];
	char	file_time[128];
	char	dir[512];
	char	*ptmp = NULL, *p1 = NULL, *p2 = NULL;
	char	*buffer = NULL;
	char	ptid[32];

	long long	total_time = 0;
	size_t	file_size = 0;
	time_t	curt = 0;
	sw_trace_item_t	trace_info;
	struct dirent	**namelist;
	struct stat	f_stat;
	static  int first_use = 1;
	static	unit_t	*pp[7] = {NULL};

	if (page_idx == 0)
	{
		page_idx = 1;
	}

	low = (page_idx - 1)*page_cnt;
	high = page_idx*page_cnt - 1;
	pub_log_debug("[%s][%d]low[%d]high[%d]", __FILE__, __LINE__, low, high);

	if (strlen(searchs->TxDate) == 0)
	{
		memset(today, 0x00, sizeof(today));
		curt = time(NULL);
		agt_date_format(curt, today, "A");
		strncpy(searchs->TxDate, today, sizeof(searchs->TxDate));
	}

	if (strlen(searchs->StartTime) == 0  && strlen(searchs->EndTime) == 0)
	{
		sprintf(searchs->StartTime, "%s", "000000");
		sprintf(searchs->EndTime, "%s", "240000");
	}

	pub_log_debug("[%s][%d]start[%s]end[%s]", __FILE__, __LINE__, searchs->StartTime, searchs->EndTime);
	memset(dir, 0x00, sizeof(dir));
	sprintf(dir, "%s/tmp/monitor/%s", getenv("SWWORK"), searchs->TxDate);

	ret = access(dir, F_OK);
	if (ret)
	{
		pub_log_error("[%s][%d] not exist file [%s]\n", __FILE__, __LINE__, dir);
		return -2;
	}
	
	namelist = agt_scan_dir(&n, dir);
	if (namelist == NULL)
	{
		return -2;
	}

	flag = 1;
	i = n;
	num = 0;

	if (first_use)
	{
	pp[2] = get_chnl_info();
	pp[0] = get_prdt_info();
	pp[1] = get_svr_info(pp[0]);
	pp[3] = get_txcode_info(pp[0]);
	first_use = 0;
	}
	while (i > 0 || flag)
	{

		if (flag)
		{
			memset(log_path, 0x00, sizeof(log_path));
			sprintf(log_path, "%s/monitor.log", dir);
			flag = 0;
		}
		else
		{
			--i;
			if (strncmp(namelist[i]->d_name, "monitor_", strlen("monitor_")) != 0)
			{
				continue;
			}

			memset(log_path, 0x00, sizeof(log_path));
			sprintf(log_path, "%s/%s", dir, namelist[i]->d_name);
		}
		pub_log_debug("[%s][%d]filename[%s]", __FILE__, __LINE__, log_path);
		ret = access(dir, F_OK);
		if (ret)
		{
			pub_log_error("[%s][%d] not exist file [%s]\n", __FILE__, __LINE__, log_path);
			continue;
		}

		if (stat(log_path, &f_stat) == -1)
		{

			pub_log_error("[%s][%d] not exist file [%s]\n", __FILE__, __LINE__, log_path);
			agt_free_namelist(namelist, n);
			return -1;
		}
		file_size = f_stat.st_size;
		if(file_size <= 0)
		{
			continue;
		}

		fd = open(log_path, O_RDWR);
		if (fd < 0)
		{
			pub_log_error("[%s][%d] open file [%s] is failed!\n", __FILE__, __LINE__, log_path);
			agt_free_namelist(namelist, n);
			return -1;
		}

		buffer = mmap(NULL, file_size+1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		if (buffer == NULL)
		{
			pub_log_error("[%s][%d] errno[%d]:[%s]", __FILE__ , __LINE__, errno, strerror(errno));
			agt_free_namelist(namelist, n);
			close(fd);
			return -1;
		}
		close(fd);

		p2 = buffer + file_size;
		while ((p1 = cut(buffer, p2)) != NULL)
		{
			memset(sbuf, 0x00, sizeof(sbuf));
			strncpy(sbuf, p1, sizeof(sbuf) - 1);
			p2 = p1;

			if ((ptmp = strstr(sbuf, "TOTAL:")) != NULL)
			{
				memset(&trace_info, 0x00, sizeof(trace_info));
				memset(ptid, 0x0, sizeof(ptid));
				agt_get_trace_info(ptmp, &trace_info, ptid);
				if (strcmp(searchs->flag, "S") == 0)
				{
					memset(file_time, 0x00, sizeof(file_time));
					curt = trace_info.end_time / 1000000;
					agt_date_format(curt, file_time, "E");
					if (strcmp(file_time, searchs->StartTime) < 0)
					{
						break;
					}
					else if (strcmp(file_time, searchs->EndTime) > 0)
					{
						continue;
					}
					if (!((searchs->TxCode[0] == '\0' || strcmp(trace_info.tx_code, searchs->TxCode) == 0) && \
						(searchs->Lsn[0] == '\0' || strcmp(trace_info.chnl, searchs->Lsn) == 0) & \
						(searchs->Prdt[0] == '\0' || strcmp(trace_info.prdt_name, searchs->Prdt) == 0) && \
						(searchs->Svr[0] == '\0' || strstr(trace_info.server, searchs->Svr) != NULL)  && \
						(searchs->Svc[0] == '\0' || strstr(trace_info.service, searchs->Svc) != NULL)  && \
						(searchs->PlatFlow[0] == '\0' || trace_info.trace_no == atoll(searchs->PlatFlow)) && \
						(searchs->TxAmount[0] == '\0' || (atof(trace_info.tx_amt) - atof(searchs->TxAmount) <= 0.01  && \
							atof(trace_info.tx_amt) - atof(searchs->TxAmount) >= -0.01)) && \
						(searchs->RetCode[0] == '\0' || strstr(trace_info.tx_respcd, searchs->RetCode) != NULL) && \
						(searchs->RetCodeNo[0] == '\0' || strstr(trace_info.tx_respcd, searchs->RetCodeNo) == NULL)  
					))
					{
						continue;
					}
				}
				memset(sts, 0x00, sizeof(sts));
				if (agt_check_stat(trace_info.tx_respcd) == 0)
				{
					strcpy(sts, "1");
				}
				else
				{
					strcpy(sts, "0");
				}
				if (cnt >= low && cnt <= high &&(flags || strcmp(sts, "0") == 0))
				{
					set_trace_info(vars, &trace_info, num, pp, ptid);
					++num;
				}
				if (strcmp(sts, "0") == 0)
				{
					wrong++;
				}
				else
				{
					right++;
				}
				total_time += trace_info.end_time  - trace_info.start_time;
				if (flags || strcmp(sts, "0") == 0)
				{
					cnt++;
				}
				if (cnt > high + 1 && flags)
				{
					quit = 1;
					break;
				}
			}
		}
		munmap(buffer, file_size+1);
		buffer = NULL;
		if (quit)
		{
			break;
		}
	}

	agt_free_namelist(namelist, n);

	page_sum = 0;
	if (cnt % page_cnt == 0)
	{
		page_sum = cnt/page_cnt;
	}
	else
	{
		page_sum = cnt/page_cnt + 1;
	}

	memset(sbuf, 0x0 ,sizeof(sbuf));
	sprintf(sbuf, "%d", page_sum);
	loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", sbuf);

	memset(sbuf, 0x0 ,sizeof(sbuf));
	sprintf(sbuf, "%d", cnt);
	loc_set_zd_data(vars, ".TradeRecord.Response.Cnt", sbuf);
	if (num > 0)
	{
		memset(sbuf, 0x0, sizeof(sbuf));
		sprintf(sbuf, "%d", num - 1);
		loc_set_zd_data(vars, "#MAXALLF", sbuf);
	}

	pub_log_debug("[%s][%d]cnt[%d]page_sum[%d]", __FILE__, __LINE__, cnt, page_sum);
	if (page_idx > page_sum && strcmp(searchs->flag, "N") == 0 )
	{
		pub_log_error("[%s][%d]page_idx[%d] is out of range!\n",__FILE__, __LINE__,  page_idx);
		return -1;
	}

	if (flags == 0)
	{
		memset(sbuf, 0x0, sizeof(sbuf));
		sprintf(sbuf, "%d %d %.2f%% %.3f", wrong, right,\
		agt_get_avgright(right, wrong+right), agt_get_avgtime(total_time, right+wrong));
		loc_set_zd_data(vars, ".TradeRecord.Response.Total", sbuf);
	}

	return 0;
}

/*获取内存状态*/
int agt_get_mem_stats(mem_stats_t *mem_stats)
{

#ifdef LINUX

	sg_mem_stats *mem_info;

	mem_info = sg_get_mem_stats();
	if (mem_info == NULL)
	{
		return -1;
	}

	mem_stats->total = mem_info->total;
	mem_stats->free = mem_info->free;
	mem_stats->used = mem_info->used;
	mem_stats->cache = mem_info->cache;
#endif

#ifdef AIX

	sw_int64_t	pagesize;
	perfstat_memory_total_t mem;

	if ((pagesize = sysconf(_SC_PAGESIZE)) == -1)
	{
		return SW_ERROR;
	}

	if (perfstat_memory_total(NULL, &mem, sizeof(perfstat_memory_total_t), 1) != 1) 
	{
		return SW_ERROR;
	}

	mem_stats->total = ((long long) mem.real_total) * pagesize;
	mem_stats->free = ((long long) mem.real_free)  * pagesize;
	mem_stats->used = ((long long) mem.real_inuse) * pagesize;
	mem_stats->cache = ((long long) mem.numperm) * pagesize;
#endif

	return SW_OK;
}

/*获取网络状态*/
network_stats_t *agt_get_network_stats(int *num)
{
	network_stats_t	*pnetwork_stats;

#ifndef AIX

	int	i = 0;
	int	entries = 0;
	network_stats_t	*ptr = NULL;
	sg_network_io_stats	*network_info = NULL;

	network_info = sg_get_network_io_stats(&entries);
	if (network_info == NULL)
	{
		pub_log_error("[%s][%d] sg_get_network_io_stats error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	pnetwork_stats = (network_stats_t *)calloc(entries, sizeof(network_stats_t));
	if (pnetwork_stats == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ptr = pnetwork_stats;
	for (i = 0; i < entries; i++)
	{
		strncpy(ptr->interface_name, network_info->interface_name, sizeof(ptr->interface_name) - 1);
		ptr->tx = network_info->tx;
		ptr->rx = network_info->rx;
		ptr->opackets = network_info->ipackets;
		ptr->ipackets = network_info->opackets;
		ptr->oerrors = network_info->ierrors;
		ptr->ierrors = network_info->oerrors;
		ptr->collisions = network_info->collisions;
		ptr->systime = network_info->systime;
		ptr++;
		network_info++;
	}

	*num = entries;
#endif

#ifdef AIX

	int	i = 0;
	int	ret = 0;
	int	tot = 0;
	perfstat_id_t	first;
	network_stats_t	*ptr = NULL;
	perfstat_netinterface_t	*statp = NULL;

	/* check how many perfstat_netinterface_t structures are available */
	tot = perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);
	if (tot < 0)
	{
		pub_log_error("[%s][%d] perfstat_netinterface error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	statp = (perfstat_netinterface_t *)calloc(tot, sizeof(perfstat_netinterface_t));
	if (statp == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	strcpy(first.name, FIRST_NETINTERFACE);

	ret = perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), tot);
	if (ret <= 0) 
	{
		free(statp);
		pub_log_error("[%s][%d] perfstat_netinterface error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	pnetwork_stats = (network_stats_t *)calloc(ret, sizeof(network_stats_t));
	if (pnetwork_stats == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ptr = pnetwork_stats;
	for (i = 0; i < ret; i++)
	{
		strncpy(ptr->interface_name, statp[i].name, sizeof(ptr->interface_name) - 1);
		ptr->tx = statp[i].obytes;
		ptr->rx = statp[i].ibytes;
		ptr->opackets = statp[i].opackets;
		ptr->ipackets = statp[i].ipackets;
		ptr->oerrors = statp[i].oerrors;
		ptr->ierrors = statp[i].ierrors;
		ptr->collisions = statp[i].collisions;
		ptr->systime = time(NULL);

		ptr++;
	}

	free(statp);
	statp = NULL;
	*num = ret;
#endif

	return pnetwork_stats;
}

/*获取CPU状态*/
cpu_stat_t *agt_get_cpu_stat(int *num)
{
	int	i = 0;
	int	cpus = 0;

#ifdef AIX
	int	ret = 0;
	perfstat_id_t	*id = NULL;
	perfstat_cpu_t	*cpu = NULL;
#endif

#ifdef LINUX
	int	cpu_stat = 0;
	FILE	*fp = NULL;
	char	buf[256];
#endif
	cpu_stat_t	*cpu_struct = NULL;
	cpu_stat_t	*cpu_bak = NULL;

#ifdef AIX	
	cpus = perfstat_cpu(NULL, NULL, sizeof(perfstat_cpu_t), 0);
	if (cpus <= 0)
	{
		pub_log_error("[%s][%d] perfstat_cpu error!errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	id = (perfstat_id_t *)calloc(1, sizeof(perfstat_id_t));
	if (id == NULL)
	{
		pub_log_error("[%s][%d] calloc errno! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	strcpy(id->name, """");

	cpu = (perfstat_cpu_t *)calloc(cpus, sizeof(perfstat_cpu_t));
	if (cpu == NULL)
	{
		free(id);
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ret = perfstat_cpu(id, cpu, sizeof(perfstat_cpu_t), cpus);
	if (ret < 0)
	{
		free(id);
		free(cpu);
		pub_log_error("[%s][%d] perfstat_cpu error! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}
	cpus = ret;

	cpu_struct = (cpu_stat_t *)calloc(cpus, sizeof(cpu_stat_t));
	if (cpu_struct == NULL)
	{
		free(id);
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}
	cpu_bak = cpu_struct;


	for (i = 0; i < cpus; i++)
	{
		strncpy(cpu_struct->desc, cpu[i].name, sizeof(cpu_struct->desc) - 1);
		cpu_struct->user = cpu[i].user;
		cpu_struct->kernel = cpu[i].sys;
		cpu_struct->idle = cpu[i].idle;
		cpu_struct->iowait = cpu[i].wait;
		cpu_struct->total = cpu[i].user + cpu[i].sys + cpu[i].idle + cpu[i].wait;
		cpu_struct++;
	}
	free(id);
	free(cpu);
#endif

#ifdef LINUX
	fp = popen("cat /proc/stat|grep cpu[0-9]|wc -l|awk '{print $1}'", "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] popen error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	pub_mem_memzero(buf, sizeof(buf));
	fgets(buf, sizeof(buf) - 1, fp);
	pclose(fp);

	cpus = atoi(buf);
	pub_log_debug("[%s][%d] cpus=[%d]\n", __FILE__, __LINE__, cpus);

	cpu_struct = (cpu_stat_t *)calloc(cpus, sizeof(cpu_stat_t));
	if (cpu_struct == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}
	cpu_bak = cpu_struct;

	fp = popen("cat /proc/stat|grep cpu[0-9]", "r");
	if (fp == NULL)
	{
		free(cpu_struct);
		pub_log_error("[%s][%d] popen error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	for (i = 0; i < cpus; i++)
	{
		pub_mem_memzero(buf, sizeof(buf));
		fgets(buf, sizeof(buf) - 1, fp);
		if (strlen(buf) < 2)
		{
			continue;
		}


		cpu_stat = sscanf(buf, "%s %lld %lld %lld %lld %lld", 
			cpu_struct->desc, &cpu_struct->user, &cpu_struct->nice, 
			&cpu_struct->kernel, &cpu_struct->idle, &cpu_struct->iowait);

		if (cpu_stat < 5 || cpu_stat > 6)
		{
			pub_log_error("[%s][%d] error! cpu_stat=[%d]\n"
				, __FILE__, __LINE__, cpu_stat);
			free(cpu_struct);
			pclose(fp);
			return NULL;
		}

		if (cpu_stat == 5)
		{
			cpu_struct->iowait = 0;
		}
		cpu_struct->total = cpu_struct->user + cpu_struct->nice + cpu_struct->kernel + cpu_struct->idle;
		cpu_struct++;
	}
	pclose(fp);
#endif

	*num = cpus;

	return cpu_bak;
}

/*获取进程信息*/
process_stats_t *agt_get_process_stat(int *num)
{
	process_stats_t	*process_stat = NULL;

#ifndef AIX

	int	i = 0;
	int	entries = 0;
	process_stats_t	*ptr = NULL;
	sg_process_stats	*process_info = NULL;

	process_info = sg_get_process_stats(&entries);
	if (process_info == NULL)
	{
		pub_log_error("[%s][%d] sg_get_process_stats error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	process_stat = (process_stats_t *)calloc(entries, sizeof(process_stats_t));
	if (process_stat == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ptr = process_stat;
	for (i = 0; i < entries; i++)
	{
		if (process_info->process_name != NULL)
		{
			strncpy(ptr->process_name, process_info->process_name, sizeof(ptr->process_name) - 1);
		}
		if (process_info->proctitle != NULL)
		{
			strncpy(ptr->proctitle, process_info->proctitle, sizeof(ptr->proctitle) - 1);
		}

		ptr->pid = process_info->pid;
		ptr->parent = process_info->parent;
		ptr->pgid = process_info->pgid;
		ptr->uid = process_info->uid;
		ptr->euid = process_info->euid;
		ptr->gid = process_info->gid;
		ptr->egid = process_info->egid;
		ptr->proc_size = process_info->proc_size;
		ptr->proc_resident = process_info->proc_resident;
		ptr->time_spent = process_info->time_spent;
		ptr->cpu_percent = process_info->cpu_percent;
		ptr->nice = process_info->nice;

		switch (process_info->nice)
		{
			case SG_PROCESS_STATE_RUNNING:
				ptr->state = PROCESS_STATE_RUNNING;
				break;
			case SG_PROCESS_STATE_SLEEPING:
				ptr->state = PROCESS_STATE_SLEEPING;
				break;
			case SG_PROCESS_STATE_STOPPED:
				ptr->state = PROCESS_STATE_STOPPED;
				break;
			case SG_PROCESS_STATE_ZOMBIE:
				ptr->state = PROCESS_STATE_STOPPED;
				break;
			default:
				break;
		}

		ptr++;
		process_info++;
	}

	*num = entries;
#endif

#ifdef AIX

#define PROCS_TO_FETCH  1000
#define	TVALU_TO_SEC(x)	((x).tv_sec + ((double)((x).tv_usec) / 1000000.0))
#define	TVALN_TO_SEC(x)	((x).tv_sec + ((double)((x).tv_usec) / 1000000000.0))

	int	i;
	int	fetched = 0;
	int	zombie = 0;
	int	done = 0;
	int	ncpus;
	double	now_time;
	long long	pagesize;
	pid_t	index = 0;
	time_t	utime, stime;
	char	cmndline[ARG_MAX];
	char	comm[ARG_MAX];
	struct timeval	now_tval;
	struct procentry64	*procs = NULL;
	struct procentry64	curproc_for_getargs;
	process_stats_t	*ptr = NULL;

	ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (ncpus == -1) 
	{
		ncpus = 1; /* sysconf error - assume 1 */
	}

	if ((pagesize = sysconf(_SC_PAGESIZE)) == -1) 
	{
		return NULL;
	}

	procs = (struct procentry64 *) malloc(sizeof(*procs) * PROCS_TO_FETCH);
	if (procs == NULL) 
	{
		return NULL;
	}

	gettimeofday(&now_tval, 0);
	now_time = TVALU_TO_SEC(now_tval);

	/* keep on grabbing chunks of processes until getprocs returns a smaller
	   block than we asked for */

	fetched = getprocs64(procs, sizeof(*procs), NULL, 0, &index, PROCS_TO_FETCH);
	process_stat = (process_stats_t *)calloc(fetched, sizeof(process_stats_t));
	if (process_stat == NULL)
	{
		pub_log_error("[%s][%d] calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return NULL;
	}

	ptr = process_stat;
	for (i = 0; i < fetched; i++) 
	{
		struct procentry64	*pi = procs + i;

		zombie = 0;

		/* set a descriptive name for the process state */
		switch( pi->pi_state ) 
		{
			case SSLEEP:
				ptr->state = PROCESS_STATE_SLEEPING;
				break;
			case SRUN:
				ptr->state = PROCESS_STATE_RUNNING;
				break;
			case SZOMB:
				ptr->state = PROCESS_STATE_ZOMBIE;
				zombie = 1;
				break;
			case SSTOP:
				ptr->state = PROCESS_STATE_STOPPED;
				break;
			case SACTIVE:
				ptr->state = PROCESS_STATE_RUNNING;
				break;
			case SIDL:
			default:
				ptr->state = PROCESS_STATE_UNKNOWN;
				break;
		}

		if (zombie) 
		{
			utime = pi->pi_utime;
			stime = pi->pi_stime;
		} 
		else 
		{
			utime = TVALN_TO_SEC(pi->pi_ru.ru_utime) + TVALN_TO_SEC(pi->pi_cru.ru_utime);
			stime = TVALN_TO_SEC(pi->pi_ru.ru_stime) + TVALN_TO_SEC(pi->pi_cru.ru_stime);
		}

		ptr->pid = pi->pi_pid;
		ptr->parent = pi->pi_ppid;
		ptr->pgid = pi->pi_pgrp;
		ptr->uid = pi->pi_cred.crx_ruid;
		ptr->euid = pi->pi_cred.crx_uid;
		ptr->gid = pi->pi_cred.crx_rgid;
		ptr->egid = pi->pi_cred.crx_gid;
		ptr->proc_size = pi->pi_size;
		ptr->proc_resident = pi->pi_drss + pi->pi_trss; /* XXX might be wrong, see P::PT */
		ptr->time_spent = utime + stime;
		ptr->cpu_percent = (((double)(utime + stime) * 100) / ( now_time - pi->pi_start )) / ncpus;
		ptr->nice = pi->pi_nice;

		/* determine comm & cmndline */
		if ( (pi->pi_flags & SKPROC) == SKPROC ) 
		{
			if ( pi->pi_pid == 0 )
			{
				snprintf(comm, ARG_MAX, "kproc (swapper)");
				snprintf(cmndline, ARG_MAX, "kproc (swapper)");
			} 
			else 
			{
				snprintf(comm, ARG_MAX, "kproc (%s)", pi->pi_comm);
				snprintf(cmndline, ARG_MAX, "kproc (%s)", pi->pi_comm);
			}
		}
		else 
		{
			snprintf(comm, ARG_MAX, "%s", pi->pi_comm);
			curproc_for_getargs.pi_pid = pi->pi_pid;
			if ( getargs(&curproc_for_getargs, sizeof(curproc_for_getargs), cmndline, ARG_MAX) < 0 ) 
			{
				snprintf(cmndline, ARG_MAX, "%s", pi->pi_comm);
			} 
			else 
			{
				done = 0;
				/* replace NUL characters in command line with spaces */

				char *c = cmndline;

				while ( ! done ) 
				{
					if ( *c == '\0' ) 
					{
						if( *(c+1) == '\0' ) 
						{
							done = 1;
						} 
						else 
						{
							*c++ = ' ';
						}
					} 
					else 
					{
						++c;
					}
				}
			}
		}

		strncpy(ptr->process_name, comm, sizeof(ptr->process_name) - 1);
		strncpy(ptr->proctitle, cmndline, sizeof(ptr->proctitle) - 1);

		ptr++;
	}

	*num = fetched;
	free(procs);
#endif

	return process_stat;
}

/*获取流水日志*/
int agt_get_trace_log(sw_loc_vars_t *vars, struct search *requ_info, int flag, log_buf_t *log)
{
	int 	fd;
	int 	len = 0;
	size_t	file_size;
	char	path[256];
	char	type[10];
	char	svc_name[256];
	char	file_name[256];
	char	*buffer = NULL;
	char	*p = NULL;
	char	*q = NULL;
	char	send_file[256];
	char	item[4][256];
	char	time[32];
	DIR 	*dp = NULL;
	FILE 	*fp = NULL;
	struct dirent	*entry;
	struct stat	f_stat;

	memset(send_file, 0x0, sizeof(send_file));

	memset(svc_name, 0x0, sizeof(svc_name));
	snprintf(svc_name, sizeof(svc_name), "%s_%s", requ_info->Svc, requ_info->TxDate);

	memset(type, 0x0, sizeof(type));
	if (flag == 1)
	{
		strcpy(type, ".pkg");
		snprintf(send_file, sizeof(send_file), "%s/dat/%s_%ld_pkg", getenv("SWWORK"), requ_info->TxDate, atoll(requ_info->PlatFlow));
	}
	else if (flag == 0)
	{
		snprintf(send_file, sizeof(send_file), "%s/dat/tra%s_%012ld.log", getenv("SWWORK"), requ_info->TxDate, atoll(requ_info->PlatFlow));
		strcpy(type, ".log");
	}
	
	if (log != NULL)
	{
		len = log->postion[log->index];
	}

	pub_log_debug("[%s][%d] send_file=[%s]", __FILE__, __LINE__, send_file);
	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path), "%s/log/%s/%s", getenv("SWWORK"), requ_info->Prdt, requ_info->Svr);

	pub_log_debug("[%s][%d] found path=[%s]", __FILE__, __LINE__, path);
	dp = opendir(path);
	if (dp == NULL)
	{
		pub_log_error("[%s][%d] opendir %s error", __FILE__, __LINE__, path);
		return -1;
	}

	while ((entry = readdir(dp)) != NULL)
	{
		if (strncmp(entry->d_name, svc_name, strlen(svc_name)) == 0)
		{
			if (strstr(entry->d_name, type) != NULL)
			{
				memset(file_name, 0x0, sizeof(file_name));
				sprintf(file_name, "%s/%s", path, entry->d_name);
				if (stat(file_name, &f_stat) == -1)
				{
					pub_log_debug("[%s][%d] get file stat error", __FILE__, __LINE__);
					closedir(dp);
					return -1;
				}

				pub_log_debug("[%s][%d]file_name=%s", __FILE__, __LINE__, file_name);
				file_size = f_stat.st_size;
				if (file_size <= 0)
				{
					continue;
				}

				fd = open(file_name,  O_RDWR);
				if (fd < 0)
				{
					pub_log_error("[%s][%d] open error", __FILE__, __LINE__);
					closedir(dp);
					return -1;
				}

				buffer = mmap(NULL, file_size+1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
				if (buffer == NULL)
				{
					pub_log_error("[%s][%d] mmap error", __FILE__, __LINE__);
					closedir(dp);
					close(fd);
					return -1;
				}
				close(fd);
				buffer[file_size] = '\n';
				p = buffer;

				while (1)
				{
					q = strchr(p+1, '\n');
					*q = '\0';
					memset(time, 0x0, sizeof(time));
					memset(&item, 0x0, sizeof(item));
					strncpy(time, p, sizeof(time) - 1);
					agt_str_split(time, "|", item, 4);
					if (strcmp(item[0], requ_info->PlatFlow) == 0)
					{
						if (log != NULL)
						{
							if (log->index == 40 || strlen(log->msg_buf)+ q-p > 1024*1024)
							{
								pub_log_error("[%s][%d] trace log is full!", __FILE__, __LINE__);
								munmap(buffer, file_size+1);
								closedir(dp);
								return 0;
							}
							*q = '\n';
							char *str = log->msg_buf;
							memcpy(str+len, p, q - p + 1);
							if (log->time[log->index][0] == '\0')
							{
								pub_log_debug("[%s][%d] time=%s", __FILE__, __LINE__, item[1]);
								strncpy(log->time[log->index], item[1], sizeof(log->time[log->index]));
							}
							len += q-p+1;
						}
						else
						{
							if (fp == NULL)
							{
								fp = fopen(send_file, "w+");
								if (fp == NULL)
								{
									pub_log_debug("[%s][%d] open file[%s] error, errno[%d] errmsg[%s]", __FILE__, __LINE__, send_file, errno, strerror(errno));
									munmap(buffer, file_size+1);
									closedir(dp);
									return -1;
								}
								fd = fileno(fp);
								pub_lock_fd(fd);

							}
							fprintf(fp, "%s\n", p);
						}
					}
					p = q + 1;
					if (p - buffer >= file_size )
					{
						break;
					}

				}

				munmap(buffer, file_size+1);
				buffer = NULL;
			}

		}
	}
	closedir(dp);
	if (log == NULL)
	{
		if (fp == NULL)
		{
			pub_log_debug("[%s][%d] not find ", __FILE__, __LINE__);
			return -1;
		}
		pub_unlock_fd(fd);
		fclose(fp);
	}
	else
	{
		log->postion[log->index + 1] = len + log->postion[log->index] + 1;
		log->index++;
	}

	return 0;
}

/*链表插入*/
static Proc_Node_t *insert_proc(Proc_Node_t *head, proc_top_t *proc_top,  int *num)
{
	Proc_Node_t	*p = NULL;
	Proc_Node_t	*q = head;
	Proc_Node_t	*pre = head;

	p = (Proc_Node_t *)malloc(sizeof(Proc_Node_t));
	if( p == NULL )
	{
		pub_log_error("[%s][%d] malloc error",__FILE__,__LINE__);
		return NULL;	
	}

	strncpy(p->proc.user, proc_top->user, sizeof(p->proc.user)-1);
	p->proc.pid = proc_top->pid;
	p->proc.cpu = proc_top->cpu;
	p->proc.a_mem = proc_top->a_mem;
	p->proc.thcount = proc_top->thcount;
	p->proc.v_mem = proc_top->v_mem;
	strcpy(p->proc.sts,  proc_top->sts);
	strcpy(p->proc.name, proc_top->name);
	strcpy(p->proc.etime, proc_top->etime);
	if( head == NULL )
	{
		(*num)++;
		head = p ;
		p->next = NULL;	
		return head;
	}

	while(q != NULL)
	{
		pre = q;
		q = q->next;	
	}

	pre->next = p;
	p->next = NULL;	
	(*num)++;	

	return head;
}

/*链表释放*/
static int destory_list( Proc_Node_t *head )
{
	Proc_Node_t	*p = NULL;

	if (head == NULL)
	{
		return 0;	
	}

	while (head != NULL)
	{
		p = head;
		head = head->next;
		free(p);	
	}	
	return 0;
}

/*进程状态*/
static void agt_proc_stat(char *cn, char *en)
{
	int	i = 0;

	switch (en[0])
	{
		case 'A':
		case 'R':
			strcpy(cn, "运行");
			break;
		case 'S':
			strcpy(cn, "休眠");
			break;
		case 'T':
			strcpy(cn, "停止");
			break;
		case 'Z':
			strcpy(cn, "僵死");
			break;
		case 'W':
			strcpy(cn, "交换");
			break;
		case 'O':
			strcpy(cn, "不存在");
			break;
		case 'X':
			strcpy(cn, "死亡");
			break;
		case 'D':
			strcpy(cn, "不可中断");
			break;
		default:
			strcpy(cn, "未知");
			break;
	}

	i = 1;
	while (en[i] != '\0')
	{
		switch (en[1])
		{
			case '<':
				strcat(cn, "（高优先级）");
				break;
			case 'N':
			case 'n':
				strcat(cn, "（低优先级）");
				break;
			case 'L':
				strcat(cn, "（分页被锁内存）");
				break;
			case 'l':
				strcat(cn, "（多进程）");
				break;
			case '+':
				strcat(cn, "（后台）");
				break;
			default:
				break;
		}
		i++;
	}

	return;
}

/*设置应答信息*/
static int set_vars_value(sw_loc_vars_t *vars, Proc_Node_t *head, int flg)
{
	int	index = 0;
	char	path[256];
	char	tmp[256];
	char	cnsts[64];
	Proc_Node_t	*p = head;

	while (p != NULL)
	{
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).Usr", index);
		loc_set_zd_data(vars, path, p->proc.user);


		memset(path, 0x00, sizeof(path));
		memset(tmp,0x00,sizeof(tmp));
		sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).Pid", index);
		sprintf(tmp,"%d",p->proc.pid);
		loc_set_zd_data(vars, path, tmp);

		memset(cnsts, 0x0, sizeof(cnsts));
		agt_proc_stat(cnsts, p->proc.sts);
		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).Sts", index);
		loc_set_zd_data(vars, path, cnsts);

		if (flg == 0)
		{
			memset(path, 0x00, sizeof(path));
			memset(tmp,0x00,sizeof(tmp));
			sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).Thcount", index);
			sprintf(tmp,"%d",p->proc.thcount);
			loc_set_zd_data(vars, path, tmp);

			memset(path, 0x00, sizeof(path));
			sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).Etime", index);
			loc_set_zd_data(vars, path, p->proc.etime);


			memset(path, 0x00, sizeof(path));
			memset(tmp,0x00,sizeof(tmp));
			sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).CPU", index);
			sprintf(tmp,"%0.2f",p->proc.cpu);
			loc_set_zd_data(vars, path, tmp);
		}
		else if (flg == 1)	
		{
			memset(path, 0x00, sizeof(path));
			memset(tmp,0x00,sizeof(tmp));
			sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).AMem", index);
			sprintf(tmp,"%0.2f",p->proc.a_mem);
			loc_set_zd_data(vars, path, tmp);


			memset(path, 0x00, sizeof(path));
			memset(tmp,0x00,sizeof(tmp));
			sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).VMem", index);
			sprintf(tmp,"%ld",p->proc.v_mem);
			loc_set_zd_data(vars, path, tmp);
		}

		memset(path, 0x00, sizeof(path));
		sprintf(path, ".TradeRecord.Response.Procs.Proc(%d).Name", index);
		loc_set_zd_data(vars, path, p->proc.name);
		index++;
		p = p->next;	
	}
	return 0;
}

/*字符串分割函数 strtok和strsep的区别是strsep严格匹配一个strtok可以匹配多个*/
static int str_split(char *sbuf, char item[][256], int cnt)
{
	int	i = 0;
	char	temp[512];
	char	*pr = NULL;
	
	if (sbuf == NULL)
	{
		return -1; 
	}

	memset(temp, 0x00, sizeof(temp));
	strncpy(temp, sbuf, sizeof(temp)-1);
	if (temp[strlen(temp) - 1] == '\n')
	{
		temp[strlen(temp) -1] = '\0';
	}
	memset(item, 0x00, sizeof(item));
	pr = strtok(temp, " ");
	strncpy(item[i++], pr, sizeof(item[i]) - 1);

	while ((pr = strtok(NULL, " ")) != NULL && i < cnt)
	{
		strncpy(item[i], pr, sizeof(item[i]) - 1);
		i++;
	}

	return 0;
}

/*按CPU或MEM排序查找进程信息*/
int get_proc_sort(sw_loc_vars_t *vars, char *type)
{
	int	i = 0;
	int	flg = 0;
	int	num  = 0;
	int	flag = 0;
	int	result = 0;
	char	res_msg[256];
	char	reply[8];	
	char	script[256];
	char	line[512];
	char	*tmp = NULL;
	char	item[12][256];
	FILE	*fp = NULL;
	proc_top_t	proc_top;
	Proc_Node_t	*head = NULL;

	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(script, sizeof(script));

	if (strlen(type) == 0)
	{
		pub_log_error("%s, %d, No .TradeRecord.Request.ResType!",__FILE__,__LINE__);
		return -1;
	}

	if (strcmp(type, "CPU") != 0 && strcmp(type, "MEM") != 0)
	{
		pub_log_error("%s, %d, Unknown resource type!",__FILE__,__LINE__);
		return -1;		
	}
	
	if (strcmp(type,"MEM") == 0)
	{
		flg = 1;
	}

	tmp = getenv("SWHOME");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, No SWHOME!",__FILE__,__LINE__);
		return -1;			
	}

	pub_mem_memzero(script, sizeof(script));
	sprintf(script, "%s/sbin/proc_inf.sh", tmp);

	result = access(script, F_OK);
	if (result != 0)
	{
		pub_log_error("%s, %d, access file [%s] error[%d][s]!"
			,__FILE__,__LINE__, script, errno, strerror(errno));
		return -1;		
	}

	pub_mem_memzero(script, sizeof(script));
	sprintf(script, "sh %s/sbin/proc_inf.sh %s", tmp, type);

	pub_log_debug("[%s][%d] the cmd[%s]", __FILE__, __LINE__, script);

	fp = popen(script, "r");
	if (fp == NULL)
	{
		pub_log_error("%s, %d, popen[%s] error[%d][%s]!", __FILE__, __LINE__, script, errno, strerror(errno));
		return -1;
	}
	
	while (!feof(fp))
	{
		memset(line, 0x00, sizeof(line));
		fgets(line, sizeof(line) - 1, fp);
		if (strlen(line) == 0)
		{
			break;
		}

		memset(item, 0x0, sizeof(item));
		str_split(line, item, 9);
		memset(&proc_top,0x0,sizeof(proc_top));

		strncpy(proc_top.user, item[0], sizeof(proc_top.user) - 1);
		proc_top.pid = atoi(item[1]);
		proc_top.cpu = atof(item[2]);
		proc_top.thcount = atoi(item[3]);
		proc_top.a_mem = atof(item[4]);
		proc_top.v_mem = atol(item[5]);
		strncpy(proc_top.etime, item[6], sizeof(proc_top.etime));
		strncpy(proc_top.sts, item[7], sizeof(proc_top.sts) - 1);
		strncpy(proc_top.name,item[8], sizeof(proc_top.name) - 1);
		head = insert_proc(head, &proc_top, &num);
		if (head == NULL)
		{
			pub_log_error("[%s][%d] insert_proc error", __FILE__, __LINE__);
			pclose(fp);
			destory_list( head );
			return -1;	
		}

		i++;
		while (line[strlen(line) - 1] != '\n')
		{
			fgets(line, sizeof(line) - 1, fp);
			if (strlen(line) == 0)
			{
				break;
			}
		}
	}
	pclose(fp);

	result = set_vars_value(vars, head, flg);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] set_vars_value error",__FILE__,__LINE__);
		destory_list( head );
		return -1;
	}

	result = destory_list(head);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] destory_list error",__FILE__,__LINE__);
		return -1;	
	}
	pub_log_debug("[%s][%d] sum=[%d]",__FILE__,__LINE__,i);
	loc_set_zd_int(vars, "#MAXALL", i);
	return 0;
}

/*unit*/
/*链表插入*/
unit_t *insert_unit(unit_t *head, unit_t *unit)
{
	unit_t	*ptmp = NULL, *punit = NULL;

	punit = malloc (sizeof(unit_t));
	if (punit == NULL)
	{
		return NULL;
	}
	memset(punit, 0x0, sizeof(unit_t));
	memcpy(punit, unit, sizeof(unit_t));
	punit->next = NULL;

	if (head == NULL)
	{
		return punit;
	}

	ptmp = head;
	while (ptmp->next != NULL)
	{
		ptmp = ptmp->next;
	}

	ptmp->next = punit;

	return head;
}

/*链表更新*/
unit_t *update_unit(unit_t *head, char *name)
{
	unit_t	*punit = NULL, unit;
	punit = head;
	while (punit != NULL)
	{
		if (strcmp(punit->name, name) == 0)
		{
			punit->cnt++;
			return head;
		}
		punit = punit->next;
	}

	memset(&unit, 0x0, sizeof(unit));
	strncpy(unit.name, name, sizeof(unit.name) - 1);
	unit.cnt = 1;

	return insert_unit(head, &unit);
}

/*链表更新状态*/
unit_t *update_unit_status(unit_t *head, char *name, int hour, int status)
{
	unit_t	*punit = NULL, unit;
	punit = head;
	while (punit != NULL)
	{
		if (strcmp(punit->name, name) == 0)
		{
			if (status)
			{
				punit->right[hour]++;
			}
			else
			{
				punit->wrong[hour]++;
			}
			punit->cnt++;
			return head;
		}
		punit = punit->next;
	}

	memset(&unit, 0x0, sizeof(unit));
	strncpy(unit.name, name, sizeof(unit.name)-1);
	unit.cnt ++;
	if (status)
	{
		unit.right[hour]++;
	}
	else
	{
		unit.wrong[hour]++;
	}

	return insert_unit(head, &unit);
}

/*链表释放*/
int free_unit(unit_t *head)
{
	if (head == NULL)
	{
		return 0;
	}
	unit_t	*punit = NULL;
	while (head != NULL)
	{
		punit = head;
		head = head->next;
		free(punit);
	}

	return 0;
}

/*获取产品信息*/
unit_t *get_prdt_info(void)
{
	int 	status = 0;
	char 	sPath[256];
	unit_t	unit, *punit = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(sPath, 0x0, sizeof(sPath));
	snprintf(sPath, sizeof(sPath)-1, "%s/cfg/products.xml", getenv("SWWORK"));

	if (access(sPath, F_OK) != 0)
	{
		pub_log_error("[%s][%d] file %s is not exist", __FILE__, __LINE__, sPath);
		return NULL;
	}

	xml = pub_xml_crtree(sPath);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! sPath=[%s]", __FILE__, __LINE__, sPath);
		return NULL;
	}

	node = pub_xml_locnode(xml, ".DFISBP.PRODUCT");
	while (node != NULL)
	{
		memset(&unit, 0x0, sizeof(unit));
		node1 = node->firstchild;
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "CNNAME") == 0)
			{
				if (node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(unit.cnname, node1->value, sizeof(unit.cnname)-1);
				}
			}

			if (strcmp(node1->name, "NAME") == 0)
			{
				if (node1->value == NULL || node1->value[0] == '\0')
				{
					pub_log_error("[%s][%d]%s node NAME has no value", __FILE__, __LINE__, sPath);
					free_unit(punit);
					return NULL;
				}
				strncpy(unit.name, node1->value, sizeof(unit.name)-1);
			}

			if (strcmp(node1->name, "STATUS") == 0)
			{
				if (node1->value == NULL || node1->value[0] == '\0')
				{
					pub_log_error("[%s][%d]%s node STATUS has no value", __FILE__, __LINE__, sPath);
					free_unit(punit);
					return NULL;
				}
				status = atoi(node1->value);
			}
			node1 = node1->next;
		}
		if (status)
		{
			punit = insert_unit(punit, &unit);
		}
		node = node->next;
	}

	pub_xml_deltree(xml);
	return punit;
}

/*获取服务信息*/
unit_t *get_svr_info(unit_t *pproduct)
{
	char 	sPath[256];
	unit_t	unit, *punit = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	while(pproduct != NULL)
	{
		memset(sPath, 0x0, sizeof(sPath));
		snprintf(sPath, sizeof(sPath)-1, "%s/products/%s/etc/svrcfg/svrcfg.xml", getenv("SWWORK"), pproduct->name);
		pproduct = pproduct->next;	

		if (access(sPath, F_OK) != 0)
		{
			pub_log_error("[%s][%d] file %s is not exist", __FILE__, __LINE__, sPath);
			continue;
		}

		xml = pub_xml_crtree(sPath);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] 建树失败! sPath=[%s]", __FILE__, __LINE__, sPath);
			continue;
		}

		node = pub_xml_locnode(xml, ".SERVERS.SERVER");
		while (node != NULL)
		{
			memset(&unit, 0x0, sizeof(unit));
			node1 = node->firstchild;
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "CNNAME") == 0)
				{
					if (node1->value != NULL && node1->value[0] != '\0')
					{
						strncpy(unit.cnname, node1->value, sizeof(unit.cnname)-1);
					}
				}

				if (strcmp(node1->name, "NAME") == 0)
				{
					if (node1->value == NULL || node1->value[0] == '\0')
					{
						pub_log_error("[%s][%d]%s node NAME has no value", __FILE__, __LINE__, sPath);
						free_unit(punit);
						return NULL;
					}
					strncpy(unit.name, node1->value, sizeof(unit.name)-1);
				}
				node1 = node1->next;
			}
			punit = insert_unit(punit, &unit);		
			node = node->next;
		}
		pub_xml_deltree(xml);
	}

	return punit;
}

/*获取渠道信息*/
unit_t *get_chnl_info(void)
{
	int 	status = 0;
	char 	path[256];
	unit_t	unit, *punit = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/cfg/channels.xml", getenv("SWWORK"));

	if (access(path, F_OK) != 0)
	{
		pub_log_debug("[%s][%d] file %s is not exist", __FILE__, __LINE__, path);
		return NULL;
	}

	xml = pub_xml_crtree(path);
	if (xml == NULL)
	{
		pub_log_debug("[%s][%d] 建树失败! path=[%s]", __FILE__, __LINE__, path);
		return NULL;
	}

	node = pub_xml_locnode(xml, ".DFISBP.CHANNEL");
	while (node != NULL)
	{
		memset(&unit, 0x0, sizeof(unit));
		node1 = node->firstchild;
		while (node1 != NULL)
		{
			if (strcmp(node1->name, "CNNAME") == 0)
			{
				if (node1->value != NULL && node1->value[0] != '\0')
				{
					strncpy(unit.cnname, node1->value, sizeof(unit.cnname)-1);
				}
			}

			if (strcmp(node1->name, "LISTEN") == 0)
			{
				if (node1->value == NULL || node1->value[0] == '\0')
				{
					pub_log_debug("[%s][%d]%s node NAME has no value", __FILE__, __LINE__, path);
					free_unit(punit);
					return NULL;
				}
				strncpy(unit.name, node1->value, sizeof(unit.name)-1);
			}

			if (strcmp(node1->name, "STATUS") == 0)
			{
				if (node1->value == NULL || node1->value[0] == '\0')
				{
					pub_log_debug("[%s][%d]%s node STATUS has no value", __FILE__, __LINE__, path);
					free_unit(punit);
					return NULL;
				}
				status = atoi(node1->value);
			}
			node1 = node1->next;
		}
		if (status)
		{
			punit = insert_unit(punit, &unit);
		}
		node = node->next;
	}

	pub_xml_deltree(xml);
	return punit;
}

/*获取交易码信息*/
unit_t *get_txcode_info(unit_t *pproduct)
{
	char	path[256];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	unit_t	unit, *punit = NULL;

	pub_log_debug("[%s][%d][%s]", __FILE__, __LINE__, __FUNCTION__);
	while (pproduct != NULL)
	{
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, "%s/products/%s/etc/maptxcode.xml", getenv("SWWORK"), pproduct->name);
		pproduct = pproduct->next;

		if (access(path, F_OK) != 0)
		{
			pub_log_debug("[%s][%d] file %s is not exist", __FILE__, __LINE__, path);
			continue;
		}

		xml = pub_xml_crtree(path);
		if (xml == NULL)
		{
			pub_log_debug("[%s][%d] 建树失败! path=[%s]", __FILE__, __LINE__, path);
			continue;
		}

		node = pub_xml_locnode(xml, ".DFISBP.TXCODE");
		while (node != NULL)
		{
			memset(&unit, 0x0, sizeof(unit));
			node1 = node->firstchild;
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "CNNAME") == 0)
				{
					if (node1->value != NULL && node1->value[0] != '\0')
					{
						strncpy(unit.cnname, node1->value, sizeof(unit.cnname)-1);
					}
				}

				if (strcmp(node1->name, "NAME") == 0)
				{
					if (node1->value == NULL || node1->value[0] == '\0')
					{
						pub_log_debug("[%s][%d]%s node NAME has no value", __FILE__, __LINE__, path);
						free_unit(punit);
						return NULL;
					}
					strncpy(unit.name, node1->value, sizeof(unit.name)-1);
				}
				node1 = node1->next;
			}
			punit = insert_unit(punit, &unit);		
			node = node->next;
		}
		pub_xml_deltree(xml);
	}
	return punit;
}

/*获取返回码信息*/
unit_t *get_retcode_info(unit_t *pproduct)
{
	char 	path[256];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	unit_t	unit, *punit = NULL;

	while (pproduct != NULL)
	{
		memset(path, 0x0, sizeof(path));
		snprintf(path, sizeof(path)-1, "%s/products/%s/etc/maperrcode.xml", getenv("SWWORK"), pproduct->name);
		pproduct = pproduct->next;
		if (access(path, F_OK) != 0)
		{
			pub_log_debug("[%s][%d] file %s is not exist", __FILE__, __LINE__, path);
			continue;
		}

		xml = pub_xml_crtree(path);
		if (xml == NULL)
		{
			pub_log_debug("[%s][%d] 建树失败! path=[%s]", __FILE__, __LINE__, path);
			continue;
		}

		node = pub_xml_locnode(xml, ".maperrcode.channel");
		while (node != NULL)
		{
			node1 = node->firstchild;
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "entry") != 0)
				{
					node1 = node1->next;
					continue;
				}

				memset(&unit, 0x0, sizeof(unit));
				node2 = node1->firstchild;

				while (node2 != NULL)
				{
					if (strcmp(node2->name, "desc") == 0)
					{
						if (node2->value != NULL && node2->value[0] != '\0')
						{
							strncpy(unit.cnname, node2->value, sizeof(unit.cnname)-1);
						}
					}

					if (strcmp(node2->name, "tocode") == 0)
					{
						if (node2->value == NULL || node2->value[0] == '\0')
						{
							pub_log_debug("[%s][%d]%s node tocode has no value", __FILE__, __LINE__, path);
							free_unit(punit);
							return NULL;
						}
						strncpy(unit.name, node2->value, sizeof(unit.name)-1);
					}
					node2 = node2->next;
				}
				punit = insert_unit(punit, &unit);		
				node1 = node1->next;
			}
			node = node->next;
		}
		pub_xml_deltree(xml);
	}

	return punit;
}

/*获取响应信息*/
unit_t *get_resflag_info(void)
{
	unit_t	unit, *punit = NULL;

	memset(&unit, 0x0, sizeof(unit));
	strcpy(unit.cnname, "未响应");
	strcpy(unit.name, "0");
	punit = insert_unit(punit, &unit);

	memset(&unit, 0x0, sizeof(unit));
	strcpy(unit.cnname, "响应");
	strcpy(unit.name, "1");
	punit = insert_unit(punit, &unit);

	return punit;
}

/*获取返回信息*/
unit_t *get_return_info(void)
{
	unit_t	unit, *punit = NULL;

	memset(&unit, 0x0, sizeof(unit));
	strcpy(unit.cnname, "失败");
	strcpy(unit.name, "0");
	punit = insert_unit(punit, &unit);

	memset(&unit, 0x0, sizeof(unit));
	strcpy(unit.cnname, "成功");
	strcpy(unit.name, "1");
	punit = insert_unit(punit, &unit);

	return punit;
}

/*时间格式转换*/
int time_adjus(char *time, char *stime)
{
	if (strlen(time) == 0)
	{
		return -1;
	}
	while (*time != '\0')
	{
		if (*time == ':')
		{
			time++;
			continue;
		}
		if (*time == '.')
		{
			break;
		}
		*stime++ = *time++;
	}
	stime = '\0';
	return 0;
}

/*搜索XML节点*/
sw_xmlnode_t *agt_xml_search(sw_xmltree_t *xml, char *path, char *name, char *value)
{
	if (xml == NULL || path == NULL || name == NULL || value == NULL)
	{
		pub_log_error("[%s][%d] input param is error!", __FILE__, __LINE__);
		return NULL;
	}

	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	node = pub_xml_locnode(xml, path);
	while (node != NULL)
	{
		node1 = node->firstchild;
		while (node1 != NULL)
		{
			if (strcmp(node1->name, name) == 0 && node1->value != NULL &&  strcmp(node1->value, value) == 0)
			{
				pub_log_debug("[%s][%d]the NAME[%s] node[%p]", __FILE__, __LINE__, node1->value, node);
				return node;
			}
			node1 = node1->next;
		}
		node = node->next;
	}

	return NULL;
}

/*删除XML节点*/
int agt_remove_node(sw_xmlnode_t *node)
{
	sw_xmlnode_t	*parent = NULL;
	sw_xmlnode_t	*brother1 = NULL, *brother2 = NULL;

	parent = node->parent;

	brother1 = brother2 = parent->firstchild;
	while (brother1 != node)
	{
		brother2 = brother1;
		brother1 = brother1->next;
	}
	if (brother1 == brother2)
	{
		parent->firstchild = node->next;
	}
	else 
	{
		brother2->next = node->next;
	}

	node->next = NULL;
	node->parent = NULL;

	pub_xml_delnode(node);
	return 0;
}

/*特殊字符转换*/
int change_ral(char *ral, int flag)
{
	if (ral == NULL || strlen(ral) == 0)
	{
		pub_log_error("[%s][%d] input param is error!", __FILE__, __LINE__);
		return -1;
	}

	if (flag == 0)
	{
		if (strcmp(ral, "dy") == 0 || strcmp(ral, "&gt;") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, ">");
		}
		else if (strcmp(ral, "xy") == 0 || strcmp(ral, "&lt;") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "<");
		}
		else if (strcmp(ral, "dy=") == 0 || strcmp(ral, "&gt;=") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, ">=");
		}
		else if (strcmp(ral, "xy=") == 0 || strcmp(ral, "&lt;=") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "<=");
		}
		else if (strcmp(ral, "并且") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "&&");
		}
		else if (strcmp(ral, "或者") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "||");
		}
		else if (strcmp(ral, "非") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "!");
		}
	}
	else if (flag == 1)
	{
		if (strcmp(ral, ">") == 0 || strcmp(ral, "&gt;") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "dy");
		}
		else if (strcmp(ral, "<") == 0 || strcmp(ral, "&lt;") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "xy");
		}
		else if (strcmp(ral, ">=") == 0 || strcmp(ral, "&gt;=") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "dy=");
		}
		else if (strcmp(ral, "<=") == 0 || strcmp(ral, "&lt;=") == 0)
		{
			memset(ral, 0x00, strlen(ral));
			strcpy(ral, "xy=");
		}
	}

	return 0;
}

/*插入*/
Node* insert(Node *head, char *name, int value)
{
	Node	*p = (Node*)malloc(sizeof(Node));
	strcpy(p->name, name);

	p->value = value;
	p->next  =NULL;
	p->next = head->next;
	head->next = p;

	return head;
}

/*查看链表信息*/
void view(Node* head)
{
	Node	*p = head->next;
	while (p != NULL)
	{
		pub_log_debug("[%s][%d]value=[%d]\n", __FILE__, __LINE__, p->name);
		p = p->next;
	}
}

/*释放链表*/
void destory(Node* head)
{
	Node	*p;
	while (head != NULL)
	{
		p = head->next;
		free(head);
		head = p;
	}
}

/*预警规则转换*/
int pack_exp(sw_xmltree_t *xml, char *rule, char *tmp)
{
	char	*p = NULL;
	char	buf[512];
	char	c_key[64];
	char	c_ral[16];
	char	c_value[64];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	p = strtok(rule, " ");
	pub_log_debug("[%s][%d] rule=%s. p=%s", __FILE__, __LINE__, rule, p);
	while (p != NULL )
	{
		if (strcmp(p,"(") == 0 || strcmp(p,")") == 0)
		{
			strcat(tmp, p); 
		}
		else if (strcmp(p, "并且") == 0 || strcmp(p, "或者") == 0 || strcmp(p, "非") == 0)
		{
			change_ral(p, 0);
			strcat(tmp, " ");
			strcat(tmp, p);
			strcat(tmp, " ");
		}
		else
		{
			pub_log_debug("[%s][%d]the search[%s]", __FILE__, __LINE__, p);
			node = agt_xml_search(xml, ".ALERT.CONDITION.TERM", "NAME", p);
			if (node == NULL)
			{
				pub_log_error("[%s][%d] not find the condition[%s]!", __FILE__, __LINE__, tmp);
				return SW_ERROR;

			}

			node1 = node->firstchild;
			while (node1 != NULL)
			{
				if (strcmp(node1->name, "KEY") == 0)
				{
					memset(c_key, 0x00, sizeof(c_key));
					strcpy(c_key, node1->value);
				}
				else if (strcmp(node1->name, "RELATION") == 0)
				{
					memset(c_ral, 0x00, sizeof(c_ral));
					strcpy(c_ral, node1->value);
					change_ral(c_ral, 0);
					pub_xml_set_value(node1, c_ral);
					pub_log_debug("[%s][%d] c_ral=[%s] ", __FILE__, __LINE__, node1->value);
				}
				else if (strcmp(node1->name, "VALUE") == 0)
				{
					memset(c_value, 0x00, sizeof(c_value));
					strcpy(c_value, node1->value);
				}
				node1 = node1->next;
			}
			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "%s%s\'%s\'", c_key, c_ral, c_value);
			pub_log_debug("[%s][%d]the Rule[%s]", __FILE__, __LINE__, buf);
			strcat(tmp, buf);
		}

		p = strtok(NULL, " ");
	}

	pub_log_debug("[%s][%d]the Rule[%s]", __FILE__, __LINE__, tmp);
	return SW_OK;
}

/*获取命令状态*/
static int get_cmd_status(char *opt, char *status)
{
	int	eflag = 0;
	int	result = SW_ERROR;
	char	out[256];
	char	*home = NULL;
	char	line[256];


	FILE	*fp = NULL;
	if (opt == NULL || status == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(out, sizeof(out));
	sprintf(out, "%s/tmp/%s.out", getenv("HOME"), opt);
	pub_log_debug("%s, %d, the out file is [%s]", __FILE__, __LINE__, out);

	result = access(out, F_OK);
	if (result)
	{
		pub_log_error("%s, %d, access file[%s] error[%d][%s]"
			, __FILE__, __LINE__, out, errno, strerror(errno));
		return SW_OK;
	}

	fp = fopen(out, "rb");
	if (fp == NULL)
	{
		pub_log_error("%s, %d, fopen file[%s] error[%d][%s]!"
			, __FILE__, __LINE__, out, errno, strerror(errno));
		return SW_ERROR;
	}

	while (!feof(fp))
	{
		pub_mem_memzero(line, sizeof(line));
		fgets(line, sizeof(line) - 1, fp);

		if (strncmp(line, SCRIPT_ERR, strlen(SCRIPT_ERR)) == 0)
		{
			strcpy(status, "err");
			pub_log_debug("%s, %d, line[%s]", __FILE__, __LINE__, line);
			eflag = 1;
		}
	}

	fclose(fp);

	if (eflag == 0)
	{
		strcpy(status, "ok");
	}

	return SW_OK;
}

/*检查产品发布是否进行*/
static int check_running(char *cmd_type)
{
	int	fd = -1;
	int	oder;
	int	not_running = 2;
	char	flock_dir[1024];
	char	flock_file[1024];

	memset(flock_dir, 0x0, sizeof(flock_dir));
	sprintf(flock_dir, "%s/tmp/flock", getenv("HOME"));
	pub_file_check_dir(flock_dir);

	memset(flock_file, 0x00, sizeof(flock_dir));
	sprintf(flock_file, "%s/%s.lock", flock_dir, cmd_type);
	pub_log_debug("%s,%d,flock_file=[%s]", __FILE__, __LINE__, flock_file);

	oder = -1;
	oder = pub_file_exist(flock_file);
	if ( oder )
	{
		pub_log_debug("[%s][%d][%s] is running!", __FILE__, __LINE__, cmd_type);
		return 1;
	}
	else
	{
		pub_log_debug("[%s][%d]cmd_type=[%s] is not running!", __FILE__, __LINE__, cmd_type);
		if (strcmp(cmd_type, "RESTORELIST") == 0)
		{
			pub_log_debug("[%s][%d] mkdiring ",__FILE__,__LINE__);
			fd = open(flock_file, O_WRONLY|O_CREAT);
			if (fd < 0)
			{
				pub_log_error( "[%s][%d]open file [%s] fail!", __FILE__, __LINE__, flock_file);
				return -1;
			}
			close(fd);
		}

		return 2;
	}
}

/*检查安装状态*/
int agt_check_inst_stat(sw_loc_vars_t *vars, char *opt, char *chk_run)
{
	int	result = -1;
	char	status[128];

	memset(status, 0x0, sizeof(status));

	if (strlen(opt) == 0)
	{
		pub_log_error("[%s][%d] opt is null", __FILE__, __LINE__);
		return -1;
	}
	pub_log_debug("%s, %d, 开始进行[%s]处理!", __FILE__, __LINE__, opt);
	result = check_running(opt);
	if (result == CMD_RUNNING)
	{
		pub_log_debug("%s, %d, 备份恢复正在进行!", __FILE__, __LINE__);
		loc_set_zd_data(vars, ".TradeRecord.Response.Running", "1");

		return 1;
	}
	else if (result == SW_ERROR)
	{
		pub_log_error("%s, %d, check_running[%s] error!", __FILE__, __LINE__, opt);
		return -1;
	}

	if (strlen(chk_run) != 0 && chk_run[0] == '1')
	{
		result = get_cmd_status(opt, status);
		if (result != SW_OK)
		{
			pub_log_error("%s, %d, get_cmd_status cmd[%s] error!"
				,__FILE__, __LINE__, opt);
			return -1;
		}

		loc_set_zd_data(vars, ".TradeRecord.Response.Running", "0");
		loc_set_zd_data(vars, ".TradeRecord.Response.Status", status);
		return 1;
	}

	return 0;
}

/*执行命令前检查*/
int pre_cmd(char *cmd_type)
{
	int	ret;
	int	fd;
	char	cmd[256];
	char	flock_dir[1024];
	char	lock_file[1024];

	memset(flock_dir, 0x0, sizeof(flock_dir));
	sprintf(flock_dir, "%s/tmp/flock", getenv("HOME"));
	ret = pub_file_check_dir(flock_dir);
	if ( ret < 0)
	{
		pub_log_debug("[%s][%d] %s not exist and creat error!", __FILE__, __LINE__, flock_dir);
		return -1;
	}

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "%s/tmp/cmd_out", getenv("HOME"));
	ret = pub_file_check_dir(cmd);
	if (ret < 0)
	{
		pub_log_error( "[%s][%d] %s is not exist and creat it error fail!", __FILE__, __LINE__, lock_file);
		return -1;
	}

	memset(lock_file, 0x00, sizeof(lock_file));
	sprintf(lock_file, "%s/%s.lock", flock_dir, cmd_type);
	pub_log_debug("%s,%d:lock_file=[%s]", __FILE__, __LINE__, lock_file);
	ret = pub_file_exist(lock_file);
	if (ret == 1)
	{
		pub_log_debug("[%s][%d][%s] is running!", __FILE__, __LINE__, cmd_type);
		return -2;
	}

	fd = open(lock_file, O_WRONLY|O_CREAT);
	if (fd < 0)
	{
		pub_log_error( "[%s][%d]open file [%s] fail!", __FILE__, __LINE__, lock_file);
		return -1;
	}

	close(fd);

	pub_log_debug("[%s][%d]执行备份!", __FILE__, __LINE__);
	return 0;
}

/*备份文件*/
int agt_back_file(char *filename)
{
	int	ret = -1;
	char	cmd[512];
	char	back_file[128];
	time_t	tm;

	memset(cmd, 0x0, sizeof(cmd));
	memset(back_file, 0x0, sizeof(back_file));

	time( &tm );
	sprintf(back_file, "%s/tmp/%s%ld.bak", getenv("SWWORK"), filename, tm);

	sprintf(cmd, "cp -rf %s/cfg/%s %s", getenv("SWWORK"), filename, back_file);
	pub_log_debug("[%s][%d] cmd[%s]", __FILE__, __LINE__, cmd);
	ret = agt_system(cmd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] %s error", __FILE__, __LINE__, cmd);
		return -1;
	}

	return 0;
}

/*备份配置*/
int restore(char *id)
{
	int	result = -1;	
	char	cmd[256];
	char	*home = NULL;
	char	*swwork = NULL;
	char	filename[256];
	if (id == NULL)
	{
		pub_log_error("err: %s, %d, Param error!\n", __FILE__, __LINE__);
		return -1;
	}

	home = getenv("HOME");
	swwork = getenv("SWWORK");

	memset(filename,0x00,sizeof(filename));
	sprintf(filename,"%s/tmp/bp_backup%s", home, id);
	result = access(filename,F_OK);
	if (result)
	{
		pub_log_error("%s file not exit !!\n", filename);
		return -1;	
	}
	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "cp -R %s/tmp/bp_backup%s/cfg %s/tmp", home, id, swwork);
	pub_log_debug("%s, %d, cmd[%s]\n", __FILE__,__LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_error("cp -R %s/tmp/bp_backup%s/cfg %s/  faile!!\n", home, id, swwork);	
		return -1;
	}

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "rm -Rf  %s/tmp/cfg/agentcfg", swwork);
	pub_log_debug("%s, %d, cmd[%s]\n", __FILE__,__LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_error("[%s][%d] %s error ", __FILE__, __LINE__, cmd);
		return -1;
	}
	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "cp -R %s/tmp/cfg/* %s/cfg/", swwork, swwork);
	pub_log_debug("%s, %d, cmd[%s]\n", __FILE__,__LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_error(" %s  faile!!\n", cmd);	
		return -1;
	}

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "rm -Rf %s/tmp/cfg", swwork);
	pub_log_debug("%s, %d, cmd[%s]\n", __FILE__,__LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_error(" %s  faile!!\n", cmd);	
		return -1;
	}

	memset(cmd, 0x00, sizeof(cmd));
	sprintf(cmd, "cp -R %s/tmp/bp_backup%s/products %s/", home, id, swwork);
	pub_log_debug("%s, %d, cmd[%s]\n", __FILE__,__LINE__, cmd);
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_error("cp -R %s/tmp/bp_backup%s/products %s/  faile!!\n", home, id, swwork);	
		return -1;
	}

	return 0;
}

/*命令执行后*/
int post_cmd(char *cmd_type)
{
	char	lock_file[1024];

	memset(lock_file, 0x00, sizeof(lock_file));
	sprintf(lock_file, "%s/tmp/flock/%s.lock", getenv("HOME"), cmd_type);
	pub_log_debug("[%s][%d]完成%s!", __FILE__, __LINE__, cmd_type);
	remove(lock_file);
	return 0;

}

/*字符串解析*/
int agt_str_parse(char *sbuf, char item[][256], int cnt)
{
	int	i = 0;
	char	*ptr = NULL;
	char	*p = NULL;

	if (sbuf == NULL)
	{
		return -1;
	}

	i = 0;

	while ((ptr = strsep(&sbuf, "|")) != NULL && i < cnt)
	{  
		if ((p = strchr(ptr, ':')) == NULL)
		{
			p = ptr;
		}
		else
		{
			p += 1;
		}
		strncpy(item[i], p, 256-1);
		i++; 
	} 

	return 0;
}

/*保存产品数据库列表*/
Node *xml_pack(sw_xmltree_t *xml, Node *head, char *deploy)
{
	char 	path[218];	
	char	*in_line = NULL;
	char	*bp_line = NULL;
	FILE	*fp;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node3 = NULL;

	memset(path,0x00,sizeof(path));
	sprintf(path, "%s/tmp/.dbconfig.xml", getenv("HOME"));
	fp = fopen(path, "a+");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] fopen %s failed", __FILE__, __LINE__, path);
		return NULL;
	}

	node = pub_xml_locnode(xml, ".TradeRecord.Request");
	node1 = node->firstchild;
	while (node1 != NULL)
	{
		xml->current = node1;
		pub_log_debug("[%s][%d] %s node->value[%s]", __FILE__, __LINE__, deploy, node1->name);
		if (strcmp(deploy, "LISTEN") == 0)
		{
			if (strcmp(node1->name,"PRODUCT") == 0)
			{
				node2 = pub_xml_locnode(xml,"PTNAME");
				if (node2 != NULL && node2->value != NULL)
				{
					if (node2->value[0] == '(')
					{
						memset(path, 0x00, sizeof(path));
						sprintf(path, "%s", node2->value + strlen("(更新)"));
						head = insert(head, path, 1);
					}
					else
					{
						pub_log_debug("[%s][%d] node2->value=%s", __FILE__, __LINE__, node2->value);
						head = insert(head, node2->value, 1);
					}
				}
				node2 = NULL;
				node2 = pub_xml_locnode(xml, "PKGNAME");
				if (node2 != NULL && node2->value != NULL)
				{
					if (node2->value[0] == '(')
					{
						memset(path, 0x00, sizeof(path));
						sprintf(path, "%s", node2->value + strlen("(更新)"));
						head = insert(head, path, 0);
					}
					else
					{
						pub_log_debug("[%s][%d] node2->value=%s", __FILE__, __LINE__, node2->value);
						head = insert(head, node2->value, 0);
					}
				}
			}
			if (strcmp(node1->name, "DATABASE") == 0)
			{
				node2 = pub_xml_locnode(xml,"PKGDBID");
				if (node2 != NULL && node2->value != NULL)
				{
					if ( node2->value[0] == '(')
					{
						memset(path, 0x00, sizeof(path));
						sprintf(path, "%s|0", node2->value + strlen("(更新)"));
						fprintf(fp, "%s\n", path);
					}
					else
					{
						memset(path, 0x00, sizeof(path));
						sprintf(path, "%s|0", node2->value);
						fprintf(fp, "%s\n", path);
					}
				}
				node2 = NULL;
				node2 = pub_xml_locnode(xml, "PTDBID");
				if (node2 != NULL && node2->value != NULL)
				{
					if ( node2->value[0] == '(')
					{
						memset(path, 0x00, sizeof(path));
						sprintf(path, "%s|1", node2->value + strlen("(更新)"));
						fprintf(fp, "%s\n", path);
					}
					else
					{
						memset(path,0x00,sizeof(path));
						sprintf(path, "%s|1", node2->value);
						fprintf(fp, "%s\n", path);
					}
				}
			}
		}
		if (strcmp(deploy,"REPLOY") == 0)
		{
			pub_log_debug("[%s][%d] node1->name[%s]", __FILE__, __LINE__, node1->name);
			if (strcmp(node1->name, "SWLISTENS") == 0)
			{
				node2 = node1->firstchild;
				while (node2 != NULL)
				{
					pub_log_debug("[%s][%d] node1->name[%s]", __FILE__, __LINE__, node2->name);
					xml->current = node2;
					node3 = pub_xml_locnode(xml,"PTNAME");
					if (node3 != NULL && node3->value != NULL)
					{
						head = insert(head, node3->value, 1);
					}

					node3 = pub_xml_locnode(xml, "PKGNAME");
					if (node3 != NULL && node3->value != NULL)
					{
						head = insert(head, node3->value, 0);
					}
					node2 = node2->next;
				}
			}
		}
		node1 = node1->next;
	}
	fclose(fp);
	return head;
}


Node *same_deploy(char *install_dir, char *deploy, Node *head)
{
	char 	path[256];
	FILE 	*fp = NULL;
	char	line[1024];
	char	lable[256];
	char	*name=NULL;
	char	*value=NULL;
	char	*home = NULL;

	home = getenv("HOME");
	if (home == NULL)
	{
		pub_log_error("[%s][%d] not env home exist!",__FILE__,__LINE__);
		return NULL;	
	}
	if (strcmp(deploy, LSN_DEPLOY) == 0)
	{
		memset(path, 0x00, sizeof(path));
		sprintf(path, "%s/tmp/.listener.lsn", home);
	}
	if (strcmp(deploy, PRDT_DEPLOY) == 0 || strcmp(deploy,PRDT_LSN) == 0)
	{
		memset(path, 0x00, sizeof(path));
		sprintf(path, "%s/tmp/.products.xml", home);
	}
	
	if (strcmp(deploy, DB_DEPLOY) == 0)
	{
		memset(path, 0x00, sizeof(path));
		sprintf(path, "%s/tmp/.dbconfig.xml", home);
	}

	fp = fopen(path,"r");
	if (fp == NULL)
	{
		pub_log_error("%s, %d,no %s exist!", __FILE__, __LINE__,path);
		return NULL;	
	}	

	memset(line, 0x00, sizeof(line));
	while (fgets(line, 1024, fp) != NULL)
	{
		memset(lable, 0x00, sizeof(lable));
		strncpy(lable, line, strlen(line) - 1);
		if (strcmp(deploy, PRDT_LSN) == 0)
		{
			name = strtok(lable, "|");
			head = insert(head, lable, 4);
			head = insert(head, lable, 5);
		}
		else
		{
			name = strtok(lable, "|");
			value = strtok(NULL, "|");
			if (atoi(value) == 3)
			{
				head = insert(head, name, 1);
			}
			else
			{
				head = insert(head, name, atoi(value));	
			}
		}
		memset(line, 0x00, sizeof(line));	
	}
	fclose(fp);
	return head;
}

/*脚本类型*/
void agt_get_exec_type(int exec_type, char *buf)
{
	switch (exec_type)
	{
		case SCRIPT_JOB:
			strcpy(buf, "脚本");
			return;
		case BIN_JOB:
			strcpy(buf, "二进制");
			return;
		default:
			strcpy(buf, "未知");
			return;
	}
}

/*中文星期*/
int agt_trans_week(char *cweek, char *eweek)
{
	strcpy(eweek, "星期:");
	while (*cweek != '\0')
	{
		switch (*cweek)
		{
			case '1':
				strcat(eweek, "一,");
				break;
			case '2':
				strcat(eweek, "二,");
				break;
			case '3':
				strcat(eweek, "三,");
				break;
			case '4':
				strcat(eweek, "四,");
				break;
			case '5':
				strcat(eweek, "五,");
				break;
			case '6':
				strcat(eweek, "六,");
				break;
			case '7':
				strcat(eweek, "日,");
				break;
			default:
				;
		}
		cweek++;
	}
	eweek[strlen(eweek) - 1] = '\0';

	return 0;
}

/*判断平台工作模式 是否开启日志服务器,资源服务器*/
int judge_workmode(void)
{
	char	file_name[256];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(file_name, 0x0, sizeof(file_name));
	sprintf(file_name, "%s/cfg/%s", getenv("SWWORK"), SW_SYSCFG_FILE);

	xml = pub_xml_crtree(file_name);
	if (xml == NULL)
	{
		pub_log_debug("[%s][%d] cretree xml [%s] error", __FILE__, __LINE__);
		return -1;
	}

	node = pub_xml_locnode(xml, ".SWCFG.ALOG.USE");
	if (node != NULL && node->value != NULL && strcmp(node->value, "1") == 0)
	{
		pub_xml_deltree(xml);
		return 2;
	}

	pub_xml_deltree(xml);
	return 1;
}

/*统计当日交易*/
int count_monitor(struct total *total, unit_t *pproduct, unit_t *pchannel)
{
	int 	i = 0;
	int 	flag = 0;
	int 	file_num = 0;
	int 	size = 0;
	int 	hour = 0;
	int 	status = 0;
	time_t	cur = 0; 
	char 	buf[1024];
	char	*ptmp = NULL;
	char	sDate[32];
	char	path[256];
	char	filename[256];
	char	bakname[256];
	char	ptid[32];
	FILE 	*fp = NULL;
	sw_trace_item_t	trace_item;
	struct dirent	**namelist = NULL;

	cur = time(NULL);	
	memset(sDate, 0x0, sizeof(sDate));
	agt_date_format(cur, sDate, "A");

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/monitor/%s", getenv("SWWORK"), sDate);

	if (0 != access(path, F_OK))
	{
		pub_log_debug ("[%s][%d] file [%s] not exist", __FILE__, __LINE__, path);
		return 0;
	}

	memset(bakname, 0x0, sizeof(bakname));
	sprintf(bakname, "monitor_%s", total->save_time);

	size = total->end_size;
	i = file_num = 0;		
	flag = 1;
	namelist = agt_scan_dir(&file_num, path);	
	while (i < file_num || flag)
	{
		if (i < file_num)
		{	
			if (strncmp(namelist[i]->d_name, "monitor_", strlen("monitor_")) != 0)
			{
				i++;
				continue;
			}

			if (strcmp(namelist[i]->d_name, bakname) <= 0)
			{
				i++;
				continue;
			}
			memset(filename, 0x00, sizeof(filename));
			snprintf(filename, sizeof(filename)-1, "%s/%s", path, namelist[i]->d_name);
			i++;
		}
		else
		{	
			memset(filename, 0x00, sizeof(filename));
			snprintf(filename, sizeof(filename)-1, "%s/monitor.log", path);				
			flag = 0;
		}

		if (access(filename, F_OK) != 0)
		{
			continue;
		}

		fp = fopen(filename, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] open file[%s] is faild", __FILE__, __LINE__, filename);
			agt_free_namelist(namelist, file_num);

			return -1;
		}	
		pub_log_debug("[%s][%d]FILE_PATH:%s",__FILE__, __LINE__,  filename);
		fseek(fp, size, SEEK_SET);
		size = 0;
		while (!feof(fp))
		{
			memset(buf, 0x00, sizeof(buf));
			fgets(buf, sizeof(buf), fp);

			if ((ptmp = strstr(buf, "TOTAL:")) == NULL)
			{
				continue;
			}
			memset(&trace_item, 0x00, sizeof(trace_item));
			memset(ptid, 0x0, sizeof(ptid));
			agt_get_trace_info(ptmp, &trace_item, ptid);

			total->d_time += trace_item.end_time - trace_item.start_time;
			hour = get_hour(trace_item.end_time);	
			if (agt_check_stat(trace_item.tx_respcd) == 0)
			{
				status = 1;
				total->right++;
			}
			else
			{
				status = 0;
				total->wrong++;
			}

			if (trace_item.resflag[0] ==  '1')
			{
				total->reaction++;
			}

			pproduct = update_unit_status(pproduct, trace_item.prdt_name, hour, status);
			pchannel = update_unit_status(pchannel, trace_item.chnl, hour, status);

		}
		total->end_size = ftell(fp);
		fclose(fp);									
		pub_log_debug("[%s][%d]read file %s  success",__FILE__, __LINE__,  filename);
	}
	agt_free_namelist(namelist, file_num);

	total->num = total->right + total->wrong;
	memset(total->save_time, 0x0, sizeof(total->save_time));
	agt_date_format(cur, total->save_time, "E");

	return 0;
}

/*获取保存的统计信息*/
int agt_get_monitor_data(struct total *total, unit_t *pproduct, unit_t *pchannel)
{
	int 	i = 0;
	int 	fd = 0;
	int 	*p = NULL;
	char	buf[1024];
	char	path[1024];
	FILE	*fp = NULL;
	time_t	cur = 0;
	struct 	tm	tm;
	char	item[32][256];
	unit_t	*punit = NULL, *phead = NULL;
	unit_t	unit;

	if (pproduct == NULL || pchannel == NULL)
	{
		pub_log_error("[%s][%d] pproduct or pchannel is null", __FILE__, __LINE__);
		return -1;
	}

	cur = time(NULL);
	localtime_r(&cur, &tm);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/monitor/%04d%02d%02d/.count_monitor", 
			getenv("SWWORK"), tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);

	if (access(path, F_OK) != 0)
	{
		pub_log_error("[%s][%d] %s is not exist", __FILE__, __LINE__, path);
		return 0;
	}

	fp = fopen(path, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d]open file error[%s]", __FILE__, __LINE__, path);
		return -1;
	}
	fd = fileno(fp);
	pub_lock_fd(fd);

	memset(buf, 0x0, sizeof(buf));
	fgets(buf, sizeof(buf)-1, fp);
	memset(item, 0x0, sizeof(item));
	agt_str_parse(buf, item, 32);
	strncpy(total->save_time, item[0], sizeof(total->save_time));
	total->num = atoi(item[1]);
	total->right = atoi(item[2]);
	total->wrong = atoi(item[3]);
	total->reaction = atoi(item[4]);
	total->d_time = atoll(item[5]);
	total->end_size = atoll(item[6]);
	while (!feof(fp))
	{
		memset(buf, 0x0, sizeof(buf));
		fgets(buf, sizeof(buf)-1, fp);

		if (strlen(buf) < 2)
		{
			continue;
		}
		
		if (strncmp(buf, "PRODUCT:", strlen("PRODUCT:")) == 0)
		{
			phead = pproduct;
			continue;
		}

		if (strncmp(buf, "CHANNEL:", strlen("CHANNEL:")) == 0)
		{
			phead = pchannel;
			continue;
		}

		memset(item, 0x0, sizeof(item));
		agt_str_parse(buf, item, 32);

		for (punit = phead; punit != NULL; punit = punit->next)
		{
			if (strcmp(punit->name, item[0]) == 0)
			{
				break;
			}
		}
		if (punit == NULL)
		{
			memset(&unit, 0x0, sizeof(unit));
			strncpy(unit.name, item[0], sizeof(unit.name));
			insert_unit(phead, &unit);
			punit = &unit;
		}

		if (strcmp(item[1], "RIGHT") == 0)
		{
			p = punit->right;
		}
		else
		{
			p = punit->wrong;
		}
		for (i = 0; i < 24; ++i)
		{
			p[i] = atoi(item[i+2]);
		}
	}

	pub_unlock_fd(fd);
	fclose(fp);
	return 0;
}

/*保存统计信息*/
int agt_save_monitor_data(struct total *total, unit_t *pproduct, unit_t *pchannel)
{
	int	i = 0;
	int	fd = 0;
	char	buf[1024];
	char	path[1024];
	FILE	*fp = NULL;
	time_t	cur = 0;
	struct tm	tm;

	if (pproduct == NULL || pchannel == NULL)
	{
		pub_log_error("[%s][%d] pproduct or pchannel is null", __FILE__, __LINE__);
		return -1;
	}

	cur = time(NULL);
	localtime_r(&cur, &tm);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/monitor/%04d%02d%02d/.count_monitor", 
			getenv("SWWORK"), tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
	pub_log_debug("[%s][%d]path[%s]", __FILE__, __LINE__, path);
	fp = fopen(path, "w");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d]open file [%s] error", __FILE__, __LINE__, path);
		return 0;
	}
	fd = fileno(fp);
	pub_lock_fd(fd);

	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "SAVETIME:%s|TOTAL:%d|RIGHT:%d|WRONG:%d|REACTION:%d|D_TIME:%lld|END_SIZE:%d", 
			total->save_time, total->num, total->right, total->wrong, total->reaction, total->d_time, total->end_size);

	fprintf(fp, "%s\n", buf);

	fprintf(fp, "PRODUCT:\n");
	while (pproduct != NULL)
	{
		fprintf(fp, "%s|RIGHT|", pproduct->name);
		for (i = 0; i < 24; ++i)
		{
			fprintf(fp, "%d|", pproduct->right[i]);
		}
		fprintf(fp, "\n");

		fprintf(fp, "%s|WRONG|", pproduct->name);
		for (i = 0; i < 24; ++i)
		{
			fprintf(fp, "%d|", pproduct->wrong[i]);
		}
		fprintf(fp, "\n");

		pproduct = pproduct->next;
	}

	fprintf(fp, "CHANNEL:\n");
	while (pchannel != NULL)
	{
		fprintf(fp, "%s|RIGHT|", pchannel->name);
		for (i = 0; i < 24; ++i)
		{
			fprintf(fp, "%d|", pchannel->right[i]);
		}
		fprintf(fp, "\n");

		fprintf(fp, "%s|WRONG|", pchannel->name);
		for (i = 0; i < 24; ++i)
		{
			fprintf(fp, "%d|", pchannel->wrong[i]);
		}
		fprintf(fp, "\n");

		pchannel = pchannel->next;
	}

	pub_unlock_fd(fd);
	fclose(fp);
	return 0;
}

/*预警级别中文*/
char *agt_level_name(char *level)
{
	switch (level[0])
	{
		case '0':
			return "提示";
		case '1':
			return "警告";
		case '2':
			return "严重";
		case '3':	
			return "紧急";
		default:
			return "未知";
	}
}

/*查询数据字典*/
int set_app_data_dic_trans(char *code_id, char *code_value, char *code_desc)
{
	int	cols = 0;
	int	rows = 0;
	char	sql[1024];
	char	*ptr = NULL;
	char	name[256];

	memset(name, 0x0, sizeof(name));
	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select code_desc from app_data_dic where code_id='%s' and code_value='%s'", code_id, code_value);
	pub_log_debug("[%s][%d] sql=[%s]", __FILE__, __LINE__, sql);
	cols = pub_db_squery(sql);
	if (cols <= 0)
	{
		pub_log_error("[%s][%d]数据库查询失败", __FILE__, __LINE__);
		return -1;
	}

	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	pub_str_rtrim(ptr);
	strcpy(code_desc, ptr);

	return 0;
}

/*自增序列查询值*/
int agt_creat_code(char *sequence, char* rate_no)
{
	int	ret = 0;
	int	cols = 0;
	char	*ptr = NULL;
	char	name[128];
	char	sql[1024];
	sw_dbcfg_t	dbcfg;

	memset(&dbcfg, 0x00, sizeof(dbcfg));
	ret = agt_get_db_cfg(&dbcfg);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get db cfg error.", __FILE__, __LINE__);
		return -1;
	}

	memset(sql, 0x00, sizeof(sql));
	if (strcmp(dbcfg.dbtype, "INFORMIX") == 0)
	{
		snprintf(sql, sizeof(sql)-1,  "select first 1 %s.nextval from systables", sequence);
	}
	else if (strcmp(dbcfg.dbtype, "ORACLE") == 0)
	{
		snprintf(sql, sizeof(sql)-1,  "select %s.nextval from dual", sequence);
	}
	else
	{
		pub_log_error("[%s][%d] unknown dbtype %s", __FILE__, __LINE__, dbcfg.dbtype);
		return -1;
	}

	pub_log_debug("[%s][%d]sql[%s]", __FILE__, __LINE__, sql);

	cols = pub_db_squery(sql);
	if (cols <= 0)
	{
		pub_log_error("[%s][%d] 生成序列号失败！", __FILE__, __LINE__);
		return -1;
	}

	memset(name, 0x00, sizeof(name));
	ptr = pub_db_get_data_and_name(NULL, 1, 1, name, sizeof(name));
	pub_str_ziphlspace(ptr);

	sprintf(rate_no, "%04d", atoi(ptr));
	return 0;
}


int record_oper_log(sw_loc_vars_t *vars, char *err_code, char *req_msg)
{
	return 0;
}

/*探测表是否存在*/
int agt_table_detect(char *table_name)
{
	char 	sql[256];

	memset(sql, 0x0, sizeof(sql));
	sprintf(sql, "select 1 from %s", table_name);

	pub_log_debug("[%s][%d] sql=%s", __FILE__, __LINE__, sql);
	return pub_db_squery(sql);
}

/*将查询结果用分隔符连接起来，替代oracle中的wm_concat函数*/
int agt_wm_concat(char *sql, char *columns)
{
	int	cols = 0;
	int	rows = 0;
	int	i = 0;
	char	*ptr = NULL;
	char	name[64];

	cols = pub_db_mquery("agt_wm_concat", sql, 1);
	if (cols < 0)
	{
		pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
		return -1;
	}

	rows = pub_db_mfetch("agt_wm_concat");
	if (rows < 0)
	{
		pub_log_error("[%s][%d] 查询数据库失败!", __FILE__, __LINE__);
		return -1;
	}
	else if (rows == 0)
	{
		pub_db_cclose("agt_wm_concat");
		pub_log_debug("[%s][%d] not data.", __FILE__, __LINE__);
		return 0;
	}

	while (1)
	{
		for (i = 0; i < rows; i++)
		{
			memset(name, 0x0, sizeof(name));
			ptr = pub_db_get_data_and_name("agt_wm_concat", i + 1, 1, name, sizeof(name));
			pub_str_ziphlspace(ptr);
			pub_log_debug("[%s][%d] col_name=[%s] value=[%s]", __FILE__, __LINE__, name, ptr);

			strcat(columns, ptr);
			strcat(columns, ",");
		}
		rows = pub_db_mfetch("agt_wm_concat");
		if (rows == 0)
		{
			pub_db_cclose("agt_wm_concat");
			pub_log_debug("[%s][%d] Fetch over!", __FILE__, __LINE__);
			columns[strlen(columns) - 1] = '\0';
			break;
		}
		else if (rows < 0)
		{
			pub_db_cclose("agt_wm_concat");
			pub_log_error("[%s][%d] Fetch error!", __FILE__, __LINE__);
			return -1;
		}
	}
	return 0;
}

