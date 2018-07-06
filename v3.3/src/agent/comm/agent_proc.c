#include "agent_pub.h"
int agt_check_pid(int pid)
{
	int	ret = 0;

	ret = kill(pid, 0);
	if (ret == 0)
	{
		return 0;
	}
	pub_log_error("[%s][%d] PID:[%d] dead!", __FILE__, __LINE__, pid);

	return -1;
}

int agt_get_procs(sw_agt_procs_t* agt_procs)
{
	int	i = 0;
	int	result = 0;
	char	*tmp = NULL;
	char	xml_path[256];
	sw_xmltree_t	*xml_tree = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, no env SWWORK.",__FILE__,__LINE__);
		return -1;
	}

	pub_mem_memzero(xml_path, sizeof(xml_path));
	sprintf(xml_path, "%s/cfg/agentcfg/%s", tmp, PROCS_FILE_NAME);
	result = access(xml_path, F_OK);
	if (result != 0)
	{
		pub_log_debug("%s, %d, access[%s] error[%d][%s]."
				,__FILE__,__LINE__, xml_path, errno, strerror(errno));
		return 0;
	}

	xml_tree = pub_xml_crtree(xml_path);
	if (xml_tree == NULL)
	{
		pub_log_error("%s, %d, pub_xml_crtree[%s] error.",__FILE__,__LINE__, xml_path);
		return -1;
	}

	node = pub_xml_locnode(xml_tree, ".PROCS.CNT");
	if (node == NULL || node->value == NULL || strlen(node->value) == 0)
	{
		pub_log_error("%s, %d, .PROCS.CNT not set in [%s]."
				,__FILE__,__LINE__, xml_path);
		pub_xml_deltree(xml_tree);
		return -1;
	}

	agt_procs->count = atoi(node->value);

	if (agt_procs->count == 0)
	{
		pub_xml_deltree(xml_tree);
		return 0;
	}

	node = pub_xml_locnode(xml_tree, ".PROCS.PROC");
	if (node == NULL)
	{
		pub_log_error("%s, %d, .PROCS.PROC not set in [%s]."
				,__FILE__,__LINE__, xml_path);
		pub_xml_deltree(xml_tree);
		return -1;	
	}

	for (i = 0; i < agt_procs->count && node != NULL; i++)
	{	
		xml_tree->current = node;

		/*get pid*/
		node1 = pub_xml_locnode(xml_tree, "PID");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).PID not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		agt_procs->proc[i].pid = atoi(node1->value);

		/*get stat*/
		node1 = pub_xml_locnode(xml_tree, "STAT");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).STAT not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		agt_procs->proc[i].stat = atoi(node1->value);

		/*get binary*/
		node1 = pub_xml_locnode(xml_tree, "BINARY");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).BINARY not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		strcpy(agt_procs->proc[i].binary, node1->value);

		/*get name*/
		node1 = pub_xml_locnode(xml_tree, "NAME");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).NAME not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		strcpy(agt_procs->proc[i].name, node1->value);

		/*get restart cnt*/
		node1 = pub_xml_locnode(xml_tree, "RESTARTCNT");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).RESTARTCNT not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		agt_procs->proc[i].restartcnt = atoi(node1->value);

		/*get proc type*/
		node1 = pub_xml_locnode(xml_tree, "TYPE");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).TYPE not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		agt_procs->proc[i].type = atoi(node1->value);

		/*get proc index*/
		node1 = pub_xml_locnode(xml_tree, "PROCINDEX");
		if (node1 == NULL || node1->value == NULL || strlen(node1->value) == 0)
		{
			pub_log_error("%s, %d, .PROCS.PROC(%d).PROCINDEX not set in [%s]."
					,__FILE__,__LINE__, i, xml_path);
			pub_xml_deltree(xml_tree);
			return -1;	
		}

		agt_procs->proc[i].proc_idx = atoi(node1->value);
		node = node->next;
	}

	if (i != agt_procs->count)
	{
		pub_log_error("%s, %d, agt_procs->count[%d] is not eq the number of PROC[%d]."
				, __FILE__, __LINE__, agt_procs->count, i);
		pub_xml_deltree(xml_tree);
		return -1;
	}

	pub_xml_deltree(xml_tree);

	return 0;
}

int agt_set_procs(sw_agt_procs_t* agt_procs)
{
	int	i = 0;
	int	result = 0;
	char	*tmp = NULL;
	char	xml_path[256];
	FILE*	fp = NULL;

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, no env SWWORK.",__FILE__,__LINE__);
		return -1;
	}

	pub_mem_memzero(xml_path, sizeof(xml_path));
	sprintf(xml_path, "%s/cfg/agentcfg/%s", tmp, PROCS_FILE_NAME);
	fp = fopen(xml_path, "w");
	if (fp == NULL)
	{
		pub_log_error("%s, %d, open file[%s] error.",__FILE__,__LINE__,xml_path);
		return -1;
	}

	fprintf(fp, "<?xml version= \"1.0\"  encoding= \"GBK\" ?>\n");
	fprintf(fp, "<PROCS>\n");

	for (i = 0;i < agt_procs->count; i++)
	{
		fprintf(fp, "\t<PROC>\n");

		fprintf(fp, "\t\t<PID>%d</PID>\n", agt_procs->proc[i].pid);
		fprintf(fp, "\t\t<TYPE>%d</TYPE>\n", agt_procs->proc[i].type);
		fprintf(fp, "\t\t<STAT>%d</STAT>\n", agt_procs->proc[i].stat);
		fprintf(fp, "\t\t<BINARY>%s</BINARY>\n", agt_procs->proc[i].binary);
		fprintf(fp, "\t\t<NAME>%s</NAME>\n", agt_procs->proc[i].name);
		fprintf(fp, "\t\t<RESTARTCNT>%d</RESTARTCNT>\n", agt_procs->proc[i].restartcnt);
		fprintf(fp, "\t\t<PROCINDEX>%d</PROCINDEX>\n", agt_procs->proc[i].proc_idx);
		fprintf(fp, "\t</PROC>\n");
#if 0
		pub_log_debug("<PID>%d</PID>\n", agt_procs->proc[i].pid);
		pub_log_debug("<STAT>%d</STAT>\n", agt_procs->proc[i].stat);
		pub_log_debug("<TYPE>%d</TYPE>\n", agt_procs->proc[i].type);
		pub_log_debug("<BINARY>%s</BINARY>\n", agt_procs->proc[i].binary);
		pub_log_debug("<NAME>%s</NAME>\n", agt_procs->proc[i].name);
		pub_log_debug("<RESTARTCNT>%d</RESTARTCNT>\n", agt_procs->proc[i].restartcnt);
		pub_log_debug("<PROCINDEX>%d</PROCINDEX>\n", agt_procs->proc[i].proc_idx);
#endif
	}

	fprintf(fp,"\t<CNT>%d</CNT>\n", agt_procs->count);
	fprintf(fp, "</PROCS>\n");

	fclose(fp);

	return 0;
}

int agt_clean_procs()
{
	int	result = 0;
	char	xml_path[256];
	char	*tmp = NULL;

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, no env SWWORK.",__FILE__,__LINE__);
		return -1;
	}

	pub_mem_memzero(xml_path, sizeof(xml_path));
	sprintf(xml_path, "%s/cfg/agentcfg/%s", tmp, PROCS_FILE_NAME);

	result = unlink(xml_path);
	if (result != 0)
	{
		pub_log_debug("%s, %d, access[%s] error[%d][%s]."
				,__FILE__,__LINE__, xml_path, errno, strerror(errno));
		return -1;
	}

	return 0;
}

int agt_get_by_name(char *name, sw_agt_proc_t *proc)
{
	int	i = 0;
	int	fd = 0;
	int	result = 0;
	char	filename[256];
	sw_agt_procs_t	procs;

	if (name == NULL || name[0] == '\0' || proc == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return -1;
	}

	memset(filename, 0x00, sizeof(filename));
	sprintf(filename, "%s/cfg/agentcfg/%s", getenv("SWWORK"), LOCK_FILE_NAME);
	fd = open(filename, O_RDWR | O_CREAT, 07777);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] open file[%s] error.", __FILE__, __LINE__, LOCK_FILE_NAME);
		return -1;
	}

	result = pub_lock_fd(fd);
	if (result != 0)
	{
		pub_log_error("%s, %d, pub_lock_fd[%d] error[%d][%s]."
				, __FILE__, __LINE__, fd, errno, strerror(errno));
		return -1;
	}

	pub_mem_memzero(&procs, sizeof(procs));
	result = agt_get_procs(&procs);
	if (result != 0)
	{
		pub_unlock_fd(fd);
		pub_log_error("%s, %d, agt_get_procs error.",__FILE__,__LINE__);
		return -1;
	}

	for (i = 0; i < procs.count; i++)
	{
		if (strcmp(procs.proc[i].name, name) == 0)
		{
			break;
		}
	}

	if (i == procs.count)
	{
		pub_unlock_fd(fd);
		return NO_DEVEL_PROC;
	}

	pub_mem_memcpy(proc, &(procs.proc[i]), sizeof(sw_agt_proc_t));

	result = pub_unlock_fd(fd);
	if (result != 0)
	{
		pub_log_error("%s, %d, pub_unlock_fd[%d] error[%d][%s]."
				, __FILE__, __LINE__, fd, errno, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int agt_proc_register(sw_agt_proc_t *proc)
{
	int	i = 0;
	int	fd = 0;
	int	result = 0;
	char	filename[256];
	sw_agt_procs_t	procs;

	if (proc == NULL || proc->name[0] == '\0' || proc->binary[0] == '\0')
	{
		pub_log_error("[%s][%d] 注册内容不能为空!", __FILE__, __LINE__);
		return -1;
	}

	memset(filename, 0x00, sizeof(filename));
	sprintf(filename, "%s/cfg/agentcfg/%s", getenv("SWWORK"), LOCK_FILE_NAME);
	fd = open(filename, O_RDWR | O_CREAT, 07777);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] open file[%s] error.", __FILE__, __LINE__, LOCK_FILE_NAME);
		return -1;
	}

	result = pub_lock_fd(fd);
	if (result != 0)
	{
		pub_log_error("%s, %d, pub_lock_fd[%d] error[%d][%s]."
				, __FILE__, __LINE__, fd, errno, strerror(errno));
		return -1;
	}

	pub_log_info("%s, %d, register pid[%d] stat[%d] name[%s] binary[%s]"
			, __FILE__, __LINE__, proc->pid, proc->stat, proc->name, proc->binary);
	pub_mem_memzero(&procs, sizeof(procs));
	result = agt_get_procs(&procs);
	if (result != 0)
	{
		pub_unlock_fd(fd);
		pub_log_error("%s, %d, agt_get_procs error.",__FILE__,__LINE__);
		return -1;
	}

	if (procs.count >= DEVEL_MAX_PROC_NUM)
	{
		pub_unlock_fd(fd);
		pub_log_error("[%s][%d] over max proc[%d], not register.", __FILE__, __LINE__, DEVEL_MAX_PROC_NUM);
		return -1;
	}

	for (i = 0; i < procs.count; i++)
	{
		if ((strcmp(procs.proc[i].name, proc->name) == 0) 
				&& (strcmp(procs.proc[i].binary, proc->binary) == 0))
		{
			break;
		}
	}

	if (i == procs.count)
	{
		pub_log_info("[%s][%d] [%s]未注册,开始注册...", __FILE__, __LINE__, proc->name);
		proc->restartcnt = MAX_RESTART_CNT;
		pub_mem_memcpy((char *)&(procs.proc[i]), (char *)proc, sizeof(sw_agt_proc_t));
		procs.count++;
	}
	else
	{
		procs.proc[i].pid = proc->pid;
		procs.proc[i].stat = proc->stat;
	}

	result = agt_set_procs(&procs);
	if (result != 0)
	{
		pub_unlock_fd(fd);
		pub_log_error("%s, %d, agt_set_procs error.",__FILE__,__LINE__);
		return -1;
	}

	result = pub_unlock_fd(fd);
	if (result != 0)
	{
		pub_log_error("%s, %d, pub_unlock_fd[%d] error[%d][%s]."
				, __FILE__, __LINE__, fd, errno, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}

int agt_scan_node(char *name)
{
	int	result = 0;
	sw_agt_proc_t	agt_proc;

	pub_mem_memzero(&agt_proc, sizeof(agt_proc));
	result = agt_get_by_name(name, &agt_proc);
	if (result == 0)
	{
		if (agt_proc.stat == SW_S_START)
		{
			if (agt_check_pid(agt_proc.pid) == 0)
			{
				return 1;
			}
		}
	}

	return 0;
}

int agt_start_node(char* binary, char *name)
{
	int	result = 0;
	pid_t	pid = -1;

	pid = fork();
	if (pid < 0)
	{
		pub_log_error("%s, %d, vfork error, errno=[%d]:[%s].",
				__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	else if (pid == 0)
	{
		result = execlp(binary, binary, name, NULL);
		if (result < 0)
		{
			printf("Start [%s][%s] fail!\n", binary, name);
		}
		pub_log_error("%s, %d, Start [%d][%s] fail, "
				, __FILE__, __LINE__, errno, strerror(errno));
		exit(0);
	}

	return 0;
}

static int agt_get_proc_stat(int stat, char *stat_buf)
{
	char	tmp[128];
	
	pub_mem_memzero(tmp, sizeof(tmp));

	switch (stat)
	{
		case SW_S_STOPED:
			strcpy(tmp, "正常停止");
			break;
		case SW_S_KILL:
			strcpy(tmp, "强制停止");
			break;
		case SW_S_ERR:
			strcpy(tmp, "未启动");
			break;
		case SW_S_ABNORMAL:
			strcpy(tmp, "异常退出");
			break;
		case SW_S_NREACH:
			strcpy(tmp, "不可到达");
			break;
		default :
			strcpy(tmp, "未知");
			break;
	}

	strcpy(stat_buf, tmp);
	return 0;
}

int agt_proc_print()
{
	int	i = 0;
	int	result = 0;
	char	stat_buf[64];
	sw_agt_procs_t	procs;

	pub_mem_memzero(stat_buf, sizeof(stat_buf));

	printf("\t           agent进程信息\n");
	printf("\t---------------------------------------\n");
	printf("\t进程号     进程名称           进程状态\n");
	printf("\t---------------------------------------\n");

	pub_mem_memzero(&procs, sizeof(procs));
	result = agt_get_procs(&procs);
	if (result != 0)
	{
		pub_log_error("%s, %d, agt_get_procs error.",__FILE__,__LINE__);
		return -1;
	}

	for (i = 0; i < procs.count; i++)
	{
		pub_mem_memzero(stat_buf, sizeof(stat_buf));
		if (procs.proc[i].type != SW_CHILD)
		{
			if (procs.proc[i].stat == SW_S_START)
			{
				if (agt_check_pid(procs.proc[i].pid) == 0)
				{
					strcpy(stat_buf, "运行");
				}
				else
				{
					strcpy(stat_buf, "状态未知");
				}
			}
			else
			{
				agt_get_proc_stat(procs.proc[i].stat, stat_buf);
			}
			printf("\t %-10d%-20s%-20s\n", procs.proc[i].pid, procs.proc[i].name, stat_buf);
		}
	}
	printf("\t---------------------------------------\n");

	return 0;
}

int agt_set_proc_by_name(char *name, sw_agt_proc_t *proc)
{
	int	i = 0;
	int	fd = 0;
	int	result = 0;
	char	filename[256];
	sw_agt_procs_t	procs;

	if (name == NULL || name[0] == '\0' || proc == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return -1;
	}

	memset(filename, 0x00, sizeof(filename));
	sprintf(filename, "%s/cfg/agentcfg/%s", getenv("SWWORK"), LOCK_FILE_NAME);
	fd = open(filename, O_RDWR | O_CREAT, 07777);
	if (fd < 0)
	{
		pub_log_error("[%s][%d] open file[%s] error.", __FILE__, __LINE__, LOCK_FILE_NAME);
		return -1;
	}

	result = pub_lock_fd(fd);
	if (result != 0)
	{
		pub_log_error("%s, %d, pub_lock_fd[%d] error[%d][%s]."
				, __FILE__, __LINE__, fd, errno, strerror(errno));
		return -1;
	}

	pub_mem_memzero(&procs, sizeof(procs));
	result = agt_get_procs(&procs);
	if (result != 0)
	{
		pub_unlock_fd(fd);
		pub_log_error("%s, %d, agt_get_procs error.",__FILE__,__LINE__);
		return -1;
	}

	for (i = 0; i < procs.count; i++)
	{
		if (strcmp(procs.proc[i].name, name) == 0)
		{
			break;
		}
	}

	if (i == procs.count)
	{
		pub_unlock_fd(fd);
		return NO_DEVEL_PROC;
	}

	pub_mem_memcpy(&(procs.proc[i]), proc, sizeof(sw_agt_proc_t));

	result = agt_set_procs(&procs);
	if (result != 0)
	{
		pub_unlock_fd(fd);
		pub_log_error("%s, %d, agt_set_procs error.",__FILE__,__LINE__);
		return -1;
	}

	result = pub_unlock_fd(fd);
	if (result != 0)
	{
		pub_log_error("%s, %d, pub_unlock_fd[%d] error[%d][%s]."
			, __FILE__, __LINE__, fd, errno, strerror(errno));
		return -1;
	}
	return 0;
}
int agt_procs_exist_clean()
{
	int	result = 0;
	char	xml_path[256];
	char	*tmp = NULL;

	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, no env SWWORK.",__FILE__,__LINE__);
		return -1;
	}

	pub_mem_memzero(xml_path, sizeof(xml_path));
	sprintf(xml_path, "%s/cfg/agentcfg/%s", tmp, PROCS_FILE_NAME);
	if (pub_file_exist(xml_path))
	{
		result = unlink(xml_path);
		if (result != 0)
		{
			pub_log_debug("%s, %d, access[%s] error[%d][%s]."
					,__FILE__,__LINE__, xml_path, errno, strerror(errno));
			return -1;
		}
	}

	return 0;
}
