#include "pub_route.h"
#include "mtype.h"
#include "pub_alog.h"
#include "pub_proc.h"

sw_int_t cmd_tpye_change(sw_cmd_t *cmd);
sw_int_t route_alloc(sw_loc_vars_t *var,sw_task_hd_t **head)
{
	char	*ptr = NULL;
	sw_int32_t	len = 0;

	if (var == NULL || head == NULL)
	{
		pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
		return SW_ERR;			
	}

	ptr = var->get_var_addr(var, SW_ROUTE_VALUDE, &len);
	if (ptr == NULL)
	{
		pub_log_info("[%s][%d] ptr is null!", __FILE__, __LINE__);
		len = sizeof(sw_task_hd_t)+sizeof(sw_cmd_t)*SW_MAX_CMD;
		ptr = var->get_null(var,SW_ROUTE_VALUDE,len);
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
			return SW_ERR;
		}
		pub_mem_memzero(ptr,len);
		*head = (sw_task_hd_t*)ptr;
		(*head)->task_curr = 0;
		(*head)->task_max = SW_MAX_CMD;
		(*head)->cmd =(sw_cmd_t *) ( ptr+sizeof(sw_task_hd_t));
	}
	else
	{
		*head = (sw_task_hd_t*)ptr;
		(*head)->cmd =(sw_cmd_t *) ( ptr+sizeof(sw_task_hd_t));
	}

	return SW_OK;
}

sw_int_t route_add_cmd(sw_task_hd_t *head, sw_cmd_t *cmd)
{
	if (head == NULL || cmd == NULL)
	{
		pub_log_error("[%s][%d] input param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (head->task_curr >= head->task_max)
	{
		pub_log_error("[%s][%d] no space store task! max[%d] ",__FILE__,__LINE__,head->task_max);
		return SW_ERR;
	}
	cmd_print(cmd);
	pub_mem_memcpy(head->cmd+head->task_curr,cmd,sizeof(sw_cmd_t));
	cmd_print(head->cmd + head->task_curr);
	head->task_curr++;

	return SW_OK;

}

sw_int_t route_del_cur_cmd(sw_task_hd_t *head)
{
	if (head == NULL )
	{
		pub_log_error("[%s][%d] input param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (head->task_curr <= 0)
	{
		pub_log_error("[%s][%d] no task  need delete!",__FILE__,__LINE__);
		return SW_ERR;
	}
	head->task_curr--;
	pub_mem_memzero(head->cmd+head->task_curr,sizeof(sw_cmd_t));

	return SW_OK;
}

sw_int_t route_recov_cmd(sw_task_hd_t *head,sw_cmd_t *cmd)
{
	sw_int32_t	curr = 0;

	if (cmd == NULL)
	{
		pub_log_error("[%s][%d] input param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	curr = head->task_curr;
	if (curr < 1)
	{
		pub_log_error("[%s][%d] no task analy!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (cmd->msg_type != SW_MSG_RES )
	{
		pub_log_info("[%s][%d] cmd is request msg, need not recovery",
				__FILE__,__LINE__);
		return SW_OK;
	}

	cmd_print(&head->cmd[curr-1]);
	pub_mem_memcpy(cmd->dst_prdt,head->cmd[curr-1].ori_prdt,sizeof(cmd->ori_prdt));
	pub_mem_memcpy(cmd->dst_svc,head->cmd[curr-1].ori_svc,sizeof(cmd->ori_svc));
	pub_mem_memcpy(cmd->dst_svr,head->cmd[curr-1].ori_svr,sizeof(cmd->ori_svr));
	pub_mem_memcpy(cmd->ori_prdt,head->cmd[curr-1].dst_prdt,sizeof(cmd->dst_prdt));
	pub_mem_memcpy(cmd->ori_svc,head->cmd[curr-1].dst_svc,sizeof(cmd->dst_svc));
	pub_mem_memcpy(cmd->ori_svr,head->cmd[curr-1].dst_svr,sizeof(cmd->dst_svr));			
	cmd->type = head->cmd[curr-1].type;
	if (curr >= 2)
	{
		pub_mem_memcpy(cmd->def_name,head->cmd[curr-2].def_name,sizeof(cmd->def_name));			
	}

	if (curr == 1 && head->cmd[curr - 1].task_flg == SW_DEL)
	{
		cmd->task_flg = SW_DEL;
	}
	cmd->dst_type = head->cmd[curr-1].ori_type;
	cmd->start_line = head->cmd[curr-1].start_line;
	cmd_print(cmd);

	route_del_cur_cmd(head);

	return SW_OK;
}

sw_int_t route_snd_dst(sw_loc_vars_t *var,sw_global_path_t *path,sw_cmd_t *cmd)
{
	sw_int_t	ret = 0;
	sw_int_t	type = 0;
	sw_int32_t	fd = 0;
	sw_int32_t	level = 0;
	sw_char_t 	comtype[16];
	sw_char_t 	svcname[64];
	sw_buf_t	locbuf;
	sw_task_hd_t	*head = NULL;
	sw_proc_info_t	proc;

	if (cmd == NULL  || (cmd->msg_type != SW_MSG_REQ && cmd->msg_type != SW_MSG_RES))
	{
		pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	level = cmd->level;
	ret = route_alloc(var, &head);
	if (ret != SW_OK || head == NULL)
	{
		pub_log_error("[%s][%d] can not load cmd list!",__FILE__,__LINE__);
		return SW_ERR;
	}
	pub_log_info("[%s][%d] head.task_curr=[%d] head.task_max=[%d]",
			__FILE__, __LINE__, head->task_curr, head->task_max);
	pub_mem_memzero(&proc,sizeof(sw_proc_info_t));
	if (cmd->msg_type == SW_MSG_RES)
	{
		pub_log_info("[%s][%d] msg_type=[%d] task_flg=[%d] lsn_name=[%s]",
				__FILE__, __LINE__, cmd->msg_type, cmd->task_flg, cmd->lsn_name);
		type = cmd->type;
		ret = route_recov_cmd(head,cmd);
		if (ret != SW_OK )
		{
			pub_log_error("[%s][%d] route_recov_cmd error!",__FILE__,__LINE__);
			return SW_ERR;
		}

		if (type == SW_LINKNULL)
		{
			cmd->type = type;
		}
		if (cmd->task_flg == SW_DEL)
		{
			pub_log_info("[%s][%d] task DELETE! mtype=[%d]", __FILE__, __LINE__, cmd->mtype);
			var->destroy(var);
			mtype_delete(cmd->mtype, 0);
			return SW_OK;
		}
		route_cmd_tpye_change(cmd);
	}

	pub_log_info("[%s][%d] dst_svr=[%s] dst_prdt=[%s] dst_svc=[%s] dst_type=[%d]",
			__FILE__, __LINE__, cmd->dst_svr, cmd->dst_prdt, cmd->dst_svc, cmd->dst_type);
	if (cmd->msg_type == SW_MSG_REQ || cmd->dst_type != ND_LSN)
	{
		if (cmd->dst_type == ND_SAF)
		{
			ret = procs_get_sys_by_name(cmd->dst_svr,&proc);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_get_dst_proc error!",__FILE__,__LINE__);
				return SW_ERR;
			}
		}
		else
		{
			ret = procs_get_dst_proc(cmd->dst_svr,cmd->dst_prdt, cmd->msg_type, &proc);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_get_dst_proc error!",__FILE__,__LINE__);
				return SW_ERR;
			}
		}
		pub_mem_memzero(cmd->dst_svc,sizeof(cmd->dst_svc));
		strncpy(cmd->dst_svc,proc.name,sizeof(cmd->dst_svc)-1);
	}
	else 
	{
		ret = procs_get_proces_info(cmd->dst_prdt,cmd->dst_svr,cmd->dst_svc,&proc);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] procs_get_dst_proc error!",__FILE__,__LINE__);
			return SW_ERR;
		}
	}

	if (cmd->task_flg == SW_STORE || cmd->type == SW_POSTREQ)
	{
		ret = route_add_cmd(head,cmd);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] route_add_cmd error!",__FILE__,__LINE__);
			return SW_ERR;
		}
	}

	fd = -1;
	pub_log_debug("[%s][%d] proc.name=[%s] pid=[%d] mqid=[%d] type=[%d]", 
			__FILE__, __LINE__, proc.name, proc.pid, proc.mqid, proc.type);

	if (proc.type == ND_LSN_RCV)
	{
		memset(comtype, 0x0, sizeof(comtype));
		loc_get_zd_data(var, "$comtype", comtype);
		pub_log_debug("[%s][%d] comtype=[%s]", __FILE__, __LINE__, comtype);
		if (comtype[0] != '\0' && strncmp(comtype, "TCPLA", 5) == 0)
		{
			memset(svcname, 0x00, sizeof(svcname));
			snprintf(svcname, sizeof(svcname), "%s_%d_snd", cmd->dst_svr + 6, proc.proc_index);
			pub_log_debug("[%s][%d] dst svcname=[%s]", __FILE__, __LINE__, svcname);

			pub_mem_memzero(&proc,sizeof(sw_proc_info_t));
			ret = procs_get_proces_info(cmd->dst_prdt, cmd->dst_svr, svcname, &proc);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_get_dst_proc error!",__FILE__,__LINE__);
				return SW_ERR;
			}
			pub_log_info("[%s][%d] dst_svc=========[%s]", __FILE__, __LINE__, proc.name);
			if (proc.status != SW_S_START || proc.pid <= 0 || pub_proc_checkpid(proc.pid) != SW_OK)
			{
				pub_mem_memzero(&proc,sizeof(sw_proc_info_t));
				ret = procs_get_dst_proc(cmd->dst_svr, cmd->dst_prdt, cmd->msg_type, &proc);
				if (ret != SW_OK)
				{
					pub_log_error("[%s][%d] procs_get_dst_proc error!",__FILE__,__LINE__);
					return SW_ERR;
				}
				pub_mem_memzero(cmd->dst_svc,sizeof(cmd->dst_svc));
				strncpy(cmd->dst_svc,proc.name,sizeof(cmd->dst_svc)-1);
			}
		}
		else
		{
			memset(&proc, 0x0, sizeof(proc));
			ret = procs_get_dst_proc(cmd->dst_svr, cmd->dst_prdt, cmd->msg_type, &proc);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] procs_get_dst_proc error!",__FILE__,__LINE__);
				return SW_ERR;
			}
			pub_mem_memzero(cmd->dst_svc,sizeof(cmd->dst_svc));
			strncpy(cmd->dst_svc,proc.name,sizeof(cmd->dst_svc)-1);
			pub_log_debug("[%s][%d] dst_svc=========[%s]", __FILE__, __LINE__, cmd->dst_svc);
		}
	}

	sw_int32_t	mqid;

	if (level > 0 && proc.type == ND_SVC && proc.mqids[level - 1] > 0)
	{
		mqid = proc.mqids[level - 1];
		fd = msg_load_fd_by_mqid(mqid);
		pub_log_info("[%s][%d] level=[%d] mqid=[%d] fd=[%d]", __FILE__, __LINE__, level, mqid, fd);

	}
	else
	{
		mqid = proc.mqid;
		fd = msg_load_fd_by_mqid(mqid);
	}
	if (fd < 0)
	{
		fd = msg_trans_open(path, mqid);
		if (fd < 0)
		{
			pub_log_error("[%s][%d] msg_trans_open error!",__FILE__,__LINE__);
			return SW_ERR;
		}
	}

	if (cmd->type != SW_POSTLSNREQ && cmd->type != SW_POSTREQ)
	{
		if (g_use_alog)
		{
			alog_update_count();
		}
	}
	pub_buf_init(&locbuf);
	var->serialize(var, &locbuf);
	pub_buf_clear(&locbuf);
	pub_log_info("[%s][%d] serialize success!", __FILE__, __LINE__);
	ret = msg_trans_send(fd,(char*)cmd,cmd->mtype,sizeof(sw_cmd_t));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] msg_trans_send error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	return SW_OK;
}

sw_int_t route_cmd_tpye_change(sw_cmd_t *cmd)
{
	if (cmd == NULL)
	{
		pub_log_error("[%s][%d] input param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	switch(cmd->type)
	{
		case SW_TASKREQ:
			cmd->type = SW_TASKRES;
			break;

		case SW_CALLLSNREQ:
			cmd->type = SW_CALLLSNRES;
			break;

		case SW_CALLREQ:
			cmd->type = SW_CALLRES;
			break;

		case SW_POSTLSNREQ:
			cmd->type = SW_POSTLSNRES;
			break;

		case SW_POSTREQ:
			cmd->type = SW_POSTRES;
			break;

		case SW_LINKLSNREQ:
			cmd->type = SW_LINKLSNRES;			
			break;

		case SW_LINKREQ:
			break;
		case SW_LINKNULL:
			break;
		default:
			break;
	}

	return SW_OK;
}

sw_int_t route_free(sw_loc_vars_t *var)
{
	if (var == NULL)
	{
		pub_log_error("[%s][%d] param error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	var->remove_var(var, SW_ROUTE_VALUDE);

	return SW_OK;
}

