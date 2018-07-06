#include "cycle.h"
#include "pub_type.h"
#include "lsn_prdt.h"
#include "lsn_alink.h"
#include "pub_ares.h"

int alink_struct2buf(char *buf, int flag, void *data)
{
	sw_char_t 	*sep = "<|>";
	sw_char_t	tmp[128];
	memset(tmp, 0x0, sizeof(tmp));

	if (buf == NULL || (flag != 0 && flag != 1) || data == NULL) 
	{
		pub_log_error("[%s][%d] params error!", __FILE__, __LINE__);
		return SW_ERROR;	
	}

	if (flag == 0)
	{
		sw_link_t	*link = NULL;
		link = (sw_link_t *)data;
		sprintf(buf, "use:%d%stype:%d%smtype:%ld%stimeout:%d%ssendtype:%d%snotsend:%d%spkglen:%d%straceno:%lld%s",
			link->use, sep, link->type, sep, link->mtype, sep, link->timeout, sep, 
			link->sendtype, sep, link->notsend, sep, link->pkglen, sep, link->traceno, sep);

		if (link->asktype[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "asktype:%s%s", link->asktype, sep);
			strcat(buf, tmp);
		}

		if (link->keyinfo[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "keyinfo:%s%s", link->keyinfo, sep);
			strcat(buf, tmp);
		}

		if (link->infoset[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "infoset:%s%s", link->infoset, sep);
			strcat(buf, tmp);
		}

		if (link->source[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "source:%s%s", link->source, sep);
			strcat(buf, tmp);
		}

		if (link->pkgtype[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "pkgtype:%s%s", link->pkgtype, sep);
			strcat(buf, tmp);
		}

		if (link->synflag[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "synflag:%s%s", link->synflag, sep);
			strcat(buf, tmp);
		}

		if (link->infoflag[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "infoflag:%s%s", link->infoflag, sep);
			strcat(buf, tmp);
		}

		if (link->saveflag[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "saveflag:%s%s", link->saveflag, sep);
			strcat(buf, tmp);
		}

		if (link->clearflag[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "clearflag:%s%s", link->clearflag, sep);
			strcat(buf, tmp);
		}

		if (link->resflag[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "resflag:%s%s", link->resflag, sep);
			strcat(buf, tmp);
		}

		if (link->qname[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "qname:%s%s", link->qname, sep);
			strcat(buf, tmp);
		}

		if (link->msgid[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "msgid:%s%s", link->msgid, sep);
			strcat(buf, tmp);
		}

		if (link->corrid[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "corrid:%s%s", link->corrid, sep);
			strcat(buf, tmp);
		}

		if (link->sys_date[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "sys_date:%s%s", link->sys_date,sep);
			strcat(buf, tmp);
		}

		if (link->machine[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "machine:%s%s", link->machine, sep);
			strcat(buf, tmp);
		}

		if (link->addr[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "addr:%s%s", link->addr, sep);
			strcat(buf, tmp);
		}

		if (link->lsn_name[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "lsn_name:%s%s", link->lsn_name, sep);
			strcat(buf, tmp);
		}
	}

	if (flag == 1)
	{
		sw_cmd_t	*cmd = NULL;
		cmd = (sw_cmd_t *)data;

		sprintf(buf, "trace_no:%lld#mtype:%ld#timeout:%d#start_line:%d#dst_type:%d#ori_type:%d#msg_type:%d#task_flg:%d#type:%d#",
				cmd->trace_no, cmd->mtype, cmd->timeout, cmd->start_line, cmd->dst_type, cmd->ori_type, cmd->msg_type, cmd->task_flg, cmd->type);
		if (cmd->ori_def[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "ori_def:%s#", cmd->ori_def);
			strcat(buf, tmp);
		}

		if (cmd->lsn_name[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "lsn_name:%s#", cmd->lsn_name);
			strcat(buf, tmp);
		}

		if (cmd->def_name[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "def_name:%s#", cmd->def_name);
			strcat(buf, tmp);
		}

		if (cmd->dst_prdt[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "dst_prdt:%s#", cmd->dst_prdt);
			strcat(buf, tmp);
		}

		if (cmd->dst_svr[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "dst_svr:%s#", cmd->dst_svr);
			strcat(buf, tmp);
		}

		if (cmd->dst_svc[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "dst_svc:%s#", cmd->dst_svc);
			strcat(buf, tmp);
		}

		if (cmd->ori_prdt[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "ori_prdt:%s#", cmd->ori_prdt);
			strcat(buf, tmp);
		}

		if (cmd->ori_svr[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "ori_svr:%s#", cmd->ori_svr);
			strcat(buf, tmp);
		}

		if (cmd->ori_svc[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "ori_svc:%s#", cmd->ori_svc);
			strcat(buf, tmp);
		}

		if (cmd->sys_date[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "sys_date:%s#", cmd->sys_date);
			strcat(buf, tmp);
		}

		if (cmd->udp_name[0] != '\0')
		{
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "udp_name:%s#", cmd->udp_name);
			strcat(buf, tmp);
		}
	}

	return SW_OK;
}

static sw_int_t set_link_result(char* buf, sw_link_t *link)
{
	int	i = 0;
	int	k = 0;
	int	cnt = 0;
	sw_link_t	*tmp;
	char	tmpsep[128];
	char	element[64];
	char	*ptr = NULL;
	char	*ptm = NULL;

	ptr = buf;
	tmp = (sw_link_t *)link;
	while (*ptr != '\0')
	{
		if (*ptr != '~')
		{
			element[i++] = *ptr;
			ptr++;
			continue;
		}

		element[i] = '\0';

		k = 0;
		cnt = 0;
		ptm = element;
		while(element[k] != '\0')
		{
			if (element[k] != ':')
			{
				tmpsep[cnt] = element[k];
				k++;
				cnt++;
				continue;
			}

			tmpsep[cnt] = '\0';
			break;
		}

		if (strcmp(tmpsep, "infoset") == 0)
		{
			if (*(ptm+k+1) != '\0')
			{
				strcpy(tmp->infoset, (ptm+k+1));
			}

		}
		else if (strcmp(tmpsep, "type") == 0)
		{
			tmp->type = atoi((ptm+k+1));
		}
		else if (strcmp(tmpsep, "mtype") == 0)
		{
			tmp->mtype = atol((ptm+k+1));
		}
		else if (strcmp(tmpsep, "traceno") == 0)
		{
			tmp->traceno = atoll((ptm+k+1));
		}
		else if (strcmp(tmpsep, "sys_date") == 0)
		{
			strcpy(tmp->sys_date,  ptm + k + 1);
		}
		else if (strcmp(tmpsep, "machine") == 0)
		{
			strcpy(tmp->machine, ptm + k + 1);
		}
		else if (strcmp(tmpsep, "addr") == 0)
		{
			strcpy(tmp->addr,  ptm + k + 1);
		}
		else if (strcmp(tmpsep, "lsn_name") == 0)
		{
			strcpy(tmp->lsn_name,  ptm + k + 1);
		}
		else
		{
			return SW_ERROR;
		}
		i = 0;
		ptr++;
		memset(element, 0x0, sizeof(element));
		memset(tmpsep, 0x0, sizeof(tmpsep));
	}
	return SW_OK;
}

sw_int_t alink_save_linkinfo(sw_int_t sendtype, sw_link_t *link, sw_cmd_t *cmd)
{
	int 	ret = 0;
	char	buf[MAX_BUF_LEN];
	
	memset(buf, 0x0, sizeof(buf));
	alink_struct2buf(buf, 0, link);
	strcat(buf, ARES_SEP);
	alink_struct2buf(buf + strlen(buf), 1, cmd);
	strcat(buf, ARES_SEP);
	sprintf(buf + strlen(buf), "%d", sendtype);
	
	ret = ares_comm(g_ares_fd, buf, strlen(buf), LINKLIST_SAVE);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares save link error!",
			__FILE__, __LINE__);
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t alink_load_linkinfo_by_key(sw_int_t sendtype, sw_link_t *link)
{
	int	ret = 0;
	char	buf[MAX_BUF_LEN];

	memset(buf, 0x0, sizeof(buf));
	alink_struct2buf(buf, 0, link);
	strcat(buf, ARES_SEP);
	sprintf(buf + strlen(buf), "%d", sendtype);
	
	ret = ares_comm(g_ares_fd, buf, strlen(buf), LINKLIST_LOAD);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares load link error!",
			__FILE__, __LINE__);
		return SW_ERROR;
	}

	ret = set_link_result(buf, link);
	if (ret < 0)
	{
		return SW_ERROR;
	}

	return SW_OK;
}

sw_int_t alink_delete_linkinfo(sw_link_t *link)
{
	int	len = 0;
	int	ret = 0;
	char	buf[128];

	memset(buf, 0x0, sizeof(buf));
	
	len = snprintf(buf, sizeof(buf), "%ld%s%s", link->mtype, ARES_SEP, link->keyinfo);
	ret = ares_comm(g_ares_fd, buf, len, LINKLIST_DEL_BY_MTYPE_AND_KEY);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Ares delete link info error, mtype=[%ld]",
			__FILE__, __LINE__, link->mtype);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] delete linkinfo in RES success!", __FILE__, __LINE__);

	return SW_OK;
}
