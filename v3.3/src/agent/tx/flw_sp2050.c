/*************************************************
 文 件 名:  flw_sp2050.c                        **
 功能描述:  设置任务监控                        **
 作    者:  linpanfei                           **
 完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

typedef struct
{
	char	run_type[8];
	char	manual[8];
	int 	exec_type;
	char	no[8];
	char	name[64];
	char	timeout[32];
	char	exec_name[64];
	char	time[64];
	char	week[64];
	char	date[512];
}sw_job_info_t;

static int judge_type(char *file)
{
	int 	irc = 0;
	char	ch = 0;
	FILE 	*fp = NULL;
	struct	stat ststat;

	irc = access(file, F_OK);
	if (irc < 0)
	{
		pub_log_error("[%s][%d] file not exist[%s]!", __FILE__, __LINE__, file);
		return -1;
	}

	memset(&ststat, 0x00, sizeof(ststat));
	stat(file, &ststat);
	if (S_ISREG(ststat.st_mode) == 0)
	{
		pub_log_error("[%s][%d] file is not txt[%s]!", __FILE__, __LINE__, file);
		return -1;
	}

	fp = fopen(file, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] popen error!", __FILE__, __LINE__);
		return -1;
	}

	while (!feof(fp))
	{
		ch = fgetc(fp);
		if (ch == '\0')
		{
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	return 0;
}

static int find_file(char *cmd, char *path)
{
	int 	i = 0;
	char	line[1024];
	char	file[256];
	FILE	*fp = NULL;

	memset(file, 0x00, sizeof(file));
	memset(line, 0x00, sizeof(line));
	sprintf(file, "type -p %s ", cmd);

	fp = popen(file, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] popen error!", __FILE__, __LINE__);
		return -1;
	}

	memset(line, 0x00, sizeof(line));
	if (fgets(line, sizeof(line) - 1, fp) != NULL)
	{
		line[strlen(line) - 1] = '\0';
		if (strlen(line) == 0)
		{
			pclose(fp);
			return -1;
		}
		for (i = 0; i < strlen(line); i++)
		{
			if (line[i] == '/')
			{
				break;
			}
		}
		strcpy(path, line + i);
		pclose(fp);
		return 0;
	}

	pclose(fp);
	return -1;
}

int get_request_info(sw_loc_vars_t *vars, sw_job_info_t *pjob)
{
	char	buf[64];
	char	eweek[128];
	char	file_path[256];

	memset(eweek, 0x0, sizeof(eweek));
	pub_log_debug("[%s][%d] add Task information processing begin..", __FILE__, __LINE__);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskNo", pjob->no);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskName", pjob->name);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.IsManual", pjob->manual);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskTimeType", pjob->run_type);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskTimeValue", pjob->time);
	if (strlen(pjob->time) == 0)
	{
		strcpy(pjob->time, "1111");
	}
	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TimeOut", pjob->timeout);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskWeek", pjob->week);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskDate", pjob->date);

	loc_get_zd_data(vars, ".TradeRecord.Request.SysTaskInfo.TaskRunName", pjob->exec_name);
	if (strlen(pjob->exec_name) != 0 )
	{
		memset(buf, 0x00, sizeof(buf));
		memset(file_path, 0x00, sizeof(file_path));

		sscanf(pjob->exec_name, "%s", buf);
		if (find_file(buf, file_path) != 0)
		{
			pub_log_error("[%s][%d] can not find file %s", __FILE__, __LINE__, pjob->exec_name);
			return -1;
		}
		if (judge_type(file_path) == -1)
		{
			pjob->exec_type = BIN_JOB;
		}
		else
		{
			pjob->exec_type = SCRIPT_JOB;
		}

	}

	return 0;
}
int set_response_info(sw_loc_vars_t *vars, sw_job_info_t *pjob, int i)
{
	char	path[256];
	char	eweek[64];
	char	buf[64];

	pub_mem_memzero(eweek, sizeof(eweek));

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskNo", i);
	loc_set_zd_data(vars, path, pjob->no);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskName", i);
	loc_set_zd_data(vars, path, pjob->name);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).IsManual", i);
	loc_set_zd_data(vars, path, pjob->manual);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TimeOut", i);
	loc_set_zd_data(vars, path, pjob->timeout);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskTimeValue", i);
	loc_set_zd_data(vars, path, pjob->time);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskRunName", i);
	loc_set_zd_data(vars, path, pjob->exec_name);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskType", i);
	memset(buf, 0x0, sizeof(buf));
	agt_get_exec_type(pjob->exec_type, buf);
	loc_set_zd_data(vars, path, buf);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskWeek", i);
	loc_set_zd_data(vars, path, pjob->week);

	pub_mem_memzero(path, sizeof(path));
	agt_trans_week(pjob->week, eweek);
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskChWeek", i);
	loc_set_zd_data(vars, path, eweek);
	pub_mem_memzero(path, sizeof(path));

	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskDate", i);
	loc_set_zd_data(vars, path, pjob->date);

	pub_mem_memzero(path, sizeof(path));
	sprintf(path, ".TradeRecord.Response.SysTaskInfos.SysTaskInfo(%d).TaskTimeType", i);
	loc_set_zd_data(vars, path, pjob->run_type);

	return 0;
}

int get_from_xml(sw_xmlnode_t *node, sw_job_info_t *pjob)
{
	sw_xmlnode_t	*node1 = NULL;

	node1 = node->firstchild;
	while (node1 != NULL)
	{
		if (strcmp(node1->name, "NO") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->no, node1->value, sizeof(pjob->no) - 1);
		}

		if (strcmp(node1->name, "NAME") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->name, node1->value, sizeof(pjob->name) - 1);
		}

		if (strcmp(node1->name, "MANUAL") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->manual, node1->value, sizeof(pjob->manual) - 1);
		}

		if (strcmp(node1->name, "TIME") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->time, node1->value, sizeof(pjob->time) - 1);
		}

		if (strcmp(node1->name, "WEEK") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->week, node1->value, sizeof(pjob->week) - 1);
		}

		if (strcmp(node1->name, "DATE") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->date, node1->value, sizeof(pjob->date) - 1);
		}

		if (strcmp(node1->name, "RUNTYPE") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->run_type, node1->value, sizeof(pjob->run_type) - 1);
		}

		if (strcmp(node1->name, "EXEC") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->exec_name, node1->value, sizeof(pjob->exec_name) - 1);
		}

		if (strcmp(node1->name, "EXECTYPE") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			pjob->exec_type = atoi(node1->value);
		}

		if (strcmp(node1->name, "TIMEOUT") == 0 && node1->value != NULL && strlen(node1->value) != 0)
		{
			strncpy(pjob->timeout, node1->value, sizeof(pjob->timeout) - 1);
		}

		node1 = node1->next;
	}

	return 0;
}

int del_from_xml(sw_xmltree_t *xml, char *taskno)
{
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*nodetmp = NULL;

	node = pub_xml_locnode(xml, ".JOBS");
	node1 = node->firstchild;
	nodetmp = NULL;
	while (node1 != NULL)
	{
		xml->current = node1;
		node2 = pub_xml_locnode(xml, "NO");
		if (node2->value == NULL || strlen(node2->value) == 0)
		{
			return -1;
		}

		if (strcmp(taskno, node2->value) == 0)
		{
			if (nodetmp == NULL)
			{
				node->firstchild = node1->next;
				node1->next = NULL;
			}
			else
			{
				nodetmp->next = node1->next;
				node1->next = NULL;
			}

			return 0;
		}

		nodetmp = node1;
		node1 = node1->next;
	}

	return -1;
}

int add_to_xml(sw_xmltree_t *xml, sw_job_info_t *pjob)
{
	char	buf[32];
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;

	node = pub_xml_locnode(xml, ".JOBS");
	node1 = node->firstchild;
	while (node1 != NULL)
	{
		xml->current = node1;
		node2 = pub_xml_locnode(xml, "NO");
		if (node2 == NULL  || node2->value == NULL)
		{
			return -1;
		}

		if (strcmp(node2->value, pjob->no) == 0)
		{
			return -1;
		}

		node1 = node1->next;
	}

	node = pub_xml_locnode(xml, ".JOBS");

	node1 = pub_xml_addnode(node, "JOB", "", SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "NO", pjob->no, SW_NODE_ATTRIB);
	pub_xml_addnode(node1, "NAME", pjob->name, SW_NODE_ATTRIB);
	pub_xml_addnode(node1, "MANUAL", pjob->manual, SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "TIME", pjob->time, SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "TIMEOUT", pjob->timeout, SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "EXEC", pjob->exec_name, SW_NODE_ELEMENT);
	memset(buf, 0x0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "%d", pjob->exec_type);
	pub_xml_addnode(node1, "EXECTYPE", buf, SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "RUNTYPE", pjob->run_type, SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "DATE", pjob->date, SW_NODE_ELEMENT);
	pub_xml_addnode(node1, "WEEK", pjob->week, SW_NODE_ELEMENT);

	return 0;
}

/*2050*/
int sp2050(sw_loc_vars_t *vars)
{
	int 	i = 0;
	int 	no = 0;
	int 	ret = -1;
	char	option[2];
	char	buf[1024];
	char	reply[16];
	char	res_msg[256];
	char	cmd[512];
	char	taskno[512];
	char	file_path[512];
	char	xmlname[128];
	FILE	*fp = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_job_info_t	stjob;

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(res_msg, sizeof(res_msg));
	pub_log_debug("[%s][%d]Task information processing functions begin to execute...", __FILE__, __LINE__);

	pub_mem_memzero(option, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);
	pub_log_info("[%s][%d] option == [%s]", __FILE__, __LINE__, option);

	memset(&stjob, 0x0, sizeof(stjob));
	ret = get_request_info(vars, &stjob);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] do get_request_info error!", __FILE__, __LINE__);
		goto ErrExit;
	}

	if (option[0] != 'S'  && option[0] != 'D' && strlen(stjob.exec_name) == 0)
	{
		pub_log_error("[%s][%d] exec_name is null", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

	if (option[0] == 'F')
	{
		pub_log_debug("[%s][%d] option 'F' begin..", __FILE__, __LINE__);

		if (strlen(stjob.exec_name) == 0)
		{
			pub_log_error("[%s][%d] exec_name is null", __FILE__, __LINE__);
			strcpy(reply, "E012");
			goto ErrExit;
		}

		memset(file_path, 0x00, sizeof(file_path));
		ret = find_file(stjob.exec_name, file_path);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] can not find file %s", __FILE__, __LINE__, stjob.exec_name);
			strcpy(reply, "E009");
			goto ErrExit;
		}

		ret = judge_type(file_path);
		if (ret == -1)
		{
			pub_log_error("[%s][%d] judge type of file %s error", __FILE__, __LINE__, stjob.exec_name);
			strcpy(reply, "E009");
			goto ErrExit;
		}
		else if (ret == 1)
		{
			pub_log_error("[%s][%d]  %s 文件为二进制  不支持查看", __FILE__, __LINE__, stjob.exec_name);
			strcpy(reply, "E024");
			goto ErrExit;
		}

		pub_log_debug("[%s][%d] find the shell path [%s]", __FILE__, __LINE__, file_path);
		loc_set_zd_data(vars, "$send_path", file_path);
		loc_set_zd_data(vars, ".TradeRecord.Header.System.FileFlag", "1");

		goto OkExit;
	}

	if (option[0] == 'X')
	{
		fp = popen(stjob.exec_name, "r");
		if (fp == NULL)
		{
			sprintf(res_msg, "run cmd=[%s] faild!", cmd);
			strcpy(reply, "E010");
			goto ErrExit;
		}

		while (!feof(fp))
		{
			memset(buf, 0x00, sizeof(buf));
			if (fgets(buf, sizeof(buf) - 1, fp) == NULL)
			{
				break;
			}

			if (strlen(res_msg) + strlen(buf) >= sizeof(res_msg))
			{
				break;
			}

			strcat(res_msg, buf);
		}

		if (strlen(res_msg) == 0)
		{
			sprintf(res_msg, "run cmd=[%s] success!", cmd);
		}

		pclose(fp);

		pub_log_debug("[%s][%d] perform task[%s]success", __FILE__, __LINE__, stjob.exec_name);
		goto OkExit;
	}

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/job.xml", getenv("SWWORK"));
	ret = access(xmlname, F_OK);
	if (ret && errno == ENOENT)
	{
		pub_log_debug("[%s][%d]file is not exist,begin to create empty tree!", __FILE__, __LINE__);
		xml = pub_xml_unpack_ext(NULL, 0);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] create XML tree error!", __FILE__, __LINE__);
			strcpy(reply, "E009");
			goto ErrExit;
		}
	}
	else if (ret)
	{
		pub_log_error("[%s][%d] wrong file[%s]permissions! errno=[%d]:[%s]",
				__FILE__, __LINE__, xmlname, errno, strerror(errno));
		strcpy(reply, "E009");
		goto ErrExit;
	}
	else
	{
		xml = pub_xml_crtree(xmlname);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d]according to XML file[%s]create tree error!",
					__FILE__, __LINE__, xmlname);
			strcpy(reply, "E045");
			goto ErrExit;
		}
	}

	node = pub_xml_locnode(xml, ".JOBS");
	if (node == NULL)
	{
		node = pub_xml_addnode(xml->root, "JOBS", "", SW_NODE_ROOT);
		if (node == NULL)
		{
			pub_log_error("[%s][%d]add child node error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			strcpy(reply, "E025");
			goto ErrExit;
		}
	}

	if (option[0] == 'S')
	{
		int 	cnt = 0;
		int 	page_cnt = 0;
		int 	page_index = 0;
		int 	page_sum = 0;

		pub_mem_memzero(buf, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageCount", buf);
		page_cnt = atoi(buf);
		if (page_cnt == 0)
		{
			page_cnt = 1;
		}

		pub_mem_memzero(buf, sizeof(buf));
		loc_get_zd_data(vars, ".TradeRecord.Request.PageIndex", buf);
		page_index = atoi(buf);
		pub_log_debug("[%s][%d] idxidx=[%d]", __FILE__, __LINE__, page_index);

		pub_log_debug("[%s][%d] search Task information processing begin..", __FILE__, __LINE__);

		node1 = pub_xml_locnode(xml, ".JOBS.JOB");
		if (node1 == NULL)
		{
			pub_xml_deltree(xml);
			strcpy(reply, "E026");
			pub_log_error("[%s][%d]find no job!", __FILE__, __LINE__);
			goto OkExit;
		}

		memset(taskno, 0x0, sizeof(taskno));
		strncpy(taskno, stjob.no, sizeof(taskno) - 1);
		i = 0;
		cnt = 0;
		while (node1 != NULL)
		{
			memset(&stjob, 0x00, sizeof(stjob));
			get_from_xml(node1, &stjob);
			cnt++;
			if ( page_cnt == 1 || (cnt > (page_index - 1) * page_cnt && cnt <= page_index * page_cnt) )
			{
				if (strlen(taskno) == 0 || taskno[0] == '=' || strcmp(stjob.no, taskno) == 0)
				{
					set_response_info(vars, &stjob, i++);
				}
			}
			node1 = node1->next;
		}

		pub_xml_deltree(xml);

		if (cnt % page_cnt == 0)
		{
			page_sum = cnt / page_cnt;
		}
		else
		{
			page_sum = cnt / page_cnt + 1;
		}
		pub_mem_memzero(buf, sizeof(buf));
		sprintf(buf, "%d", page_sum);
		loc_set_zd_data(vars, ".TradeRecord.Response.PageSum", buf);

		pub_mem_memzero(buf, sizeof(buf));
		sprintf(buf, "%d", cnt);
		loc_set_zd_data(vars, ".TradeRecord.Response.Cnt", buf);

		pub_log_debug("[%s][%d] search Task information processing success", __FILE__, __LINE__);
		goto OkExit;
	}

	if (option[0] == 'M' || option[0] == 'A')
	{
		memset(buf, 0x00, sizeof(buf));
		memset(file_path, 0x00, sizeof(file_path));

		sscanf(stjob.exec_name, "%s", buf);
		ret = find_file(buf, file_path);
		if (ret != 0)
		{
			pub_log_error("[%s][%d] can not find file %s", __FILE__, __LINE__, stjob.exec_name);
			strcpy(reply, "E009");
			goto ErrExit;
		}
	}

	if (option[0] == 'M' || option[0] == 'D')
	{
		if (strlen(stjob.no) == 0)
		{
			pub_log_error("[%s][%d] resquset taskno is null", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			strcpy(reply, "E012");
			goto ErrExit;
		}

		ret = del_from_xml(xml, stjob.no);
		if (ret < 0)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] not found taskno=[%s]", __FILE__, __LINE__, stjob.no);
			strcpy(reply, "E026");
			goto ErrExit;
		}
	}

	if (option[0] == 'M' || option[0] == 'A')
	{
		if (option[0] == 'A')
		{
			node = pub_xml_locnode(xml, ".JOBS.JOB");
			while (node != NULL)
			{
				xml->current = node;
				node1 = pub_xml_locnode(xml, "NO");
				if (node1 != NULL && node1->value != NULL)
				{
					if (no <= atoi(node1->value))
					{
						no = atoi(node1->value) + 1;
					}
				}
				node = node->next;
			}
			memset(stjob.no, 0x0, sizeof(stjob.no));
			sprintf(stjob.no, "%d", no);
		}

		ret = add_to_xml(xml, &stjob);
		if (ret < 0)
		{
			pub_log_error("[%s][%d]  taskno=[%s] error", __FILE__, __LINE__, stjob.no);
			pub_xml_deltree(xml);
			strcpy(reply, "E038");
			goto ErrExit;
		}
	}

	ret = pub_xml_pack(xml, xmlname);
	if (ret)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] pub_xml_pack error! ",
				__FILE__, __LINE__, errno, strerror(errno));
		strcpy(reply, "E027");
		goto ErrExit;
	}
	pub_xml_deltree(xml);

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	if (strlen(res_msg) == 0)
	{
		strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	}
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	if (option[0] != 'S')
	{
		ret = record_oper_log(vars, reply, res_msg);
		if(ret < 0)
		{
			pub_log_error("[%s][%d] 操作登记流水错误", __FILE__, __LINE__);
			goto ErrExit;
		}
	}

	return 0;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}

