#include "agent_adm.h"

int agt_adm_start();
int agt_adm_stop();
int agt_adm_list();
int agt_adm_clean();
int agt_adm_help();
int agt_adm_quit();
int agt_adm_unknow();

static const agt_cmd_handler_t g_agt_cmd_array[] = 
{
	{
		AGENT_START,
		"start/s",
		"start agent",
		agt_adm_start
	},
	{
		AGENT_STOP,
		"stop",
		"stop agent",
		agt_adm_stop
	},
	{
		AGENT_LIST,
		"list/l",
		"list agent",
		agt_adm_list
	},
	{
		AGENT_CLEAN,
		"clean",
		"clean agent",
		agt_adm_clean
	},
	{
		AGENT_HELP,
		"help/h",
		"help agent",
		agt_adm_help
	},
	{
		AGENT_QUIT,
		"quit/q",
		"quit agent",
		agt_adm_quit
	},
	{
		AGENT_UNKNOW,
		"unknow",
		"unknow cmd",
		agt_adm_unknow
	}
};

int agt_adm_start()
{
	int	ret = 0;
	int	use = 0;
	int	type = 0;
	char	*ptr = NULL;
	char	name[64];
	char	xmlname[64];
	char	binary[32];
	char	filename[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL, *node1 = NULL;

	agt_procs_exist_clean();
	memset(filename, 0x00, sizeof(filename));
	ptr = getenv("SWWORK");
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d], No env SWWORK.",__FILE__,__LINE__);
		return -1;
	}
	else
	{
		snprintf(filename, sizeof(filename), "%s/cfg/agentcfg/agent.xml", ptr);
	}

	xml = pub_xml_crtree(filename);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xmltree error.", __FILE__, __LINE__);
		return -1;
	}

	node = pub_xml_locnode(xml, ".CFG.AGENT");
	while(node != NULL)
	{
		if (strcmp(node->name, "AGENT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		use = 0;
		type = 0;
		memset(name, 0x00, sizeof(name));
		node1 = pub_xml_locnode(xml, "USE");
		if (node1 != NULL && node1->value != NULL)
		{
			use = atoi(node1->value);
		}
		else
		{
			use = 0;
		}

		if (use == 0)
		{
			pub_log_info("[%s][%d] not use,next node.", __FILE__, __LINE__);
			node = node->next;
			continue;
		}

		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 != NULL && node1->value != NULL)
		{
			strncpy(name, node1->value, strlen(node1->value));
		}
		else
		{
			pub_log_info("[%s][%d] not cfg NAME,next node.", __FILE__, __LINE__);
			node = node->next;
			continue;
		}

		pub_log_debug("[%s][%d] name=[%s]", __FILE__, __LINE__, name);
		memset(binary, 0x00, sizeof(binary));
		strncpy(binary, PROC_NAME_AGT, sizeof(binary));
		ret = agt_scan_node(name);
		if (ret == 0)
		{
			ret = agt_start_node(binary, name);
			if (ret < 0)
			{
				printf("Start [%s] fail\n",name);
			}
			else
			{
				printf("Start [%s] sucess.\n",name);
			}

			usleep(100000);
		}
		else
		{
			pub_log_info("[%s][%d]Process [%s] is running.", __FILE__, __LINE__,name);
		}

		node = node->next;
	}

	pub_xml_deltree(xml);
	pub_mem_memzero(binary, sizeof(binary));
	pub_mem_memzero(name, sizeof(name));
	strncpy(binary, PROC_NAME_AGTPOL, sizeof(binary));
	strncpy(name, PROC_NAME_AGTPOL, sizeof(name));
	ret = agt_scan_node(name);
	if (ret == 0)
	{
		ret = agt_start_node(binary, name);
		if (ret < 0)
		{
			printf("Start [%s] fail!\n", name);                        }
		else
		{
			printf("Start [%s] success!\n", name);
		}
	}
	else
	{
		printf("process [%s] is running!\n", name);
	}

	return 0;
}

int agt_adm_stop()
{
	int	i = 0;
	int	ret = 0;
	int	count = 0;
	int	option = 0;
	int	status = 0;
	sw_agt_procs_t	procs;

	pub_mem_memzero(&procs, sizeof(procs));
	ret = agt_get_procs(&procs);
	if (ret != 0)
	{
		pub_log_error("%s, %d, agt_get_procs error.",__FILE__,__LINE__);
		return -1;
	}

	count = procs.count;
	for (i = 0;i < count; i++)
	{
		if (procs.proc[i].name[0] != '\0')
		{
			kill(procs.proc[i].pid, 9);
			waitpid(procs.proc[i].pid, &status, option);
			printf("Stop [%s] success!\n", procs.proc[i].name);
			procs.proc[i].stat = SW_S_KILL;
		}
	}

	ret = agt_set_procs(&procs);
	if (ret != 0)
	{
		pub_log_error("%s, %d, agt_set_procs error.",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

int agt_adm_clean()
{
	int	ret = 0;

	ret = agt_clean_procs();
	if (ret != 0)
	{
		printf("clean agt_procs fail!\n");
		pub_log_error("[%s][%d] clean agt_procs fail!", __FILE__, __LINE__);
	}
	else
	{
		printf("clean agt_procs success!\n");
		pub_log_debug("[%s][%d] clean agt_procs success!", __FILE__, __LINE__);
	}

	return 0;
}

int agt_adm_list()
{
	agt_proc_print();
	return 0;
}

int agt_adm_help()
{
	int	idx = 0;
	int	total = 0;

	printf("\tNo.  COMMAND\t\t\t\t DESCRIPTION\n");
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");

	for (idx = 0;idx < AGENT_TOTAL; idx++)
	{
		if(AGENT_UNKNOW == idx)
		{
			continue;
		}

		total++;
		printf("\t[%02d] %-35s %s\n",
				total, g_agt_cmd_array[idx].usage, g_agt_cmd_array[idx].desc);
	}
	printf("\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	printf("\tTOTAL:%d \n", total);

	return 0;
}

int agt_adm_quit()
{
	exit(0);
}

int agt_adm_unknow()
{
	printf("\n\tUnknown command\n");
	return 0;
}

int agt_adm_get_cmdtype(char *args[], int *cmd_type)
{
	if(!strcasecmp(args[0], "start") || !strcasecmp(args[0], "s"))	
	{
		*cmd_type = AGENT_START;
	}
	else if (!strcasecmp(args[0], "stop"))
	{
		*cmd_type = AGENT_STOP;
	}
	else if (!strcasecmp(args[0], "clean"))
	{
		*cmd_type = AGENT_CLEAN;
	}
	else if (!strcasecmp(args[0], "list") || !strcasecmp(args[0], "l"))
	{
		*cmd_type = AGENT_LIST;
	}
	else if (!strcasecmp(args[0], "help") || !strcasecmp(args[0], "h"))
	{
		*cmd_type = AGENT_HELP;
	}
	else if (!strcasecmp(args[0], "quit") || !strcasecmp(args[0], "q"))
	{
		*cmd_type = AGENT_QUIT;
	}
	else
	{
		*cmd_type = AGENT_UNKNOW;
	}

	return 0;
}

int agt_adm_cmd(int argc, char *args[])
{
	int	ret = 0;
	int	cmd_type = 0;
	const agt_cmd_handler_t	*cmd_handler = NULL;

	if (argc < 1)
	{
		return 0;
	}

	ret = agt_adm_get_cmdtype(args, &cmd_type);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get cmd type error.", __FILE__, __LINE__);
		return -1;
	}

	cmd_handler = &g_agt_cmd_array[cmd_type];
	ret = cmd_handler->cmd_handle_func();
	if (ret < 0)
	{
		pub_log_error("[%s][%d] exec function error.", __FILE__, __LINE__);
		return -1;
	}

	return 0;

}

void agt_adm_exit(int sig)
{
	pub_log_error("[%s][%d] swadmin catch signal [%d] and exit.", __FILE__, __LINE__,sig);
	exit(1);
}

int agt_adm_init(const char* err_log, const char* dbg_log)
{
	int	ret = 0;

	ret = agt_set_logpath(err_log, dbg_log);
	if (ret < 0)
	{
		pub_log_stderr("set errlog and dbglog error.\n");
		pub_log_info("[%s][%d] set errlog and dbglog error.", __FILE__, __LINE__);
		return -1;
	}

	signal(SIGTSTP,agt_adm_exit);

	return 0; 
}

int agt_adm_run(int argc, char **argv)
{
	int	i = 0;
	int	ret = 0;
	int	index = 0;
	char	*ptr = NULL;
	char	line[128];
	char	*arg[16];
	fd_set	readfds;
	struct timeval	time_out;

	if (argc >= 2)
	{
		agt_adm_cmd(argc-1, &(argv[1]));
		return 0;
	}

	while(1)
	{
		fprintf(stderr, "%s> ", "DHC-Agent");
		memset(&time_out, 0x00, sizeof(struct timeval));

		while(1)
		{
			time_out.tv_sec = 100;	
			time_out.tv_usec = 0;

			FD_SET(STDIN_FILENO, &readfds);
			ret = select(3, &readfds, NULL, NULL, &time_out);
			if(ret < 0)
			{
				if(errno == EINTR)
				{	
					pub_log_info("[%s][%d] time interpute",__FILE__,__LINE__);
					continue;
				}
				else
				{
					pub_log_error("[%s][%d],select error errno=[%d]",__FILE__,__LINE__,errno);
					continue;
				}
			}
			else if(ret == 0)
			{	
				pub_log_info("[%s][%d] No input data, exit!", __FILE__, __LINE__);
				pub_log_stderr("\nTime out, no input data, exit.\n");
				return 0;
			}
			else
			{
				break;
			}
		}

		i = 0;
		memset(line, 0x00, sizeof(line));
		while (i < sizeof(line))
		{
			ret = read(STDIN_FILENO ,line + i ,1);
			if ( ret <= 0)
			{
				return -1;
			}

			if (line[i] == '\n')
			{
				line[i] = '\0';
				break;
			}
			if (line[i] == '\t')
			{
				line[i] = ' ';
			}
			if (line[i] == ' ' && (i==0 || line[i-1] == ' '))
			{
				continue;
			}

			i++;
		}

		ptr = strtok(line, " ");
		while(ptr != NULL)
		{
			arg[index] = ptr;
			index++;
			ptr = strtok(NULL, " ");
		}

		arg[index] = NULL;
		agt_adm_cmd(index, arg);
		printf("\n");
	}
	return 0;
}

int agt_adm_destroy()
{	
	return 0;
}

int main(int argc, char **argv)
{
	int	ret = 0;

	ret = agt_adm_init("agt_error.log", "swagent.log");
	if(0 != ret)
	{
		pub_log_stderr("[%s][%d] Init swagent cycle failed!\n\n", __FILE__, __LINE__);
		return -1;
	}

	pub_log_info("[%s][%d] swagent running...", __FILE__, __LINE__);
	ret = agt_adm_run(argc, argv);
	if(0 != ret)
	{
		fprintf(stderr, "[%s][%d] Run swagent failed!\n", __FILE__, __LINE__);
		pub_log_error("[%s][%d] Run swagent failed!", __FILE__, __LINE__);
		return -1;
	}

	agt_adm_destroy();

	return 0;
}



