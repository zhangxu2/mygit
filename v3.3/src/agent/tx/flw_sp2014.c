/*************************************************
  文 件 名:  flw_sp2014.c                       **
  功能描述:	 获取平台ID                         **
  作	者:	 薛辉                               **
  完成日期:	 20160801                           **
 *************************************************/
#include "agent_comm.h"

int sp2014(sw_loc_vars_t *vars)
{
	int	i = 0;
	int	ret = -1;
	int	shm_c = 0;
	int	msg_c = 0;
	int	sem_c = 0;
	char	xml[128];
	char	tmp[128];
	char	opt[16];
	char	reply[8];
	char	res_msg[256];
	uni_msipc_t msipc[UNI_MAX_IPC_CNT];

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	pub_log_debug("[%s][%d]this get_ipc_infors ", __FILE__, __LINE__);

	memset(opt, 0x00, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_debug("[%s][%d] opt[%s]", __FILE__, __LINE__,opt);

	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		memset(&msipc[i], 0x00, sizeof(uni_msipc_t));
	}

	ret = uni_get_ipc_info(msipc);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] get_ipc_info error!",__FILE__,__LINE__);
		strcpy(reply, "E044");
		goto ErrExit;
	}

	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		if (msipc[i].used == 0)
		{
			break;
		}

		pub_log_debug("[%s][%d][%d]", __FILE__, __LINE__, i);

		if (msipc[i].uni_shm.shmid > 0)
		{
			if (strcmp(opt, "SHM") == 0)
			{
				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).SHMid", shm_c);
				sprintf(tmp, "%d", msipc[i].uni_shm.shmid);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).SHMsize", shm_c);
				sprintf(tmp, "%d", msipc[i].uni_shm.shm_size);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).PROtime_add", shm_c);
				if (msipc[i].uni_shm.shm_atime > 0)
				{
					pub_time_change_time(&msipc[i].uni_shm.shm_atime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).PROltime", shm_c);
				if (msipc[i].uni_shm.shm_dtime > 0)
				{
					pub_time_change_time(&msipc[i].uni_shm.shm_dtime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).PROtime_modify", shm_c);
				if (msipc[i].uni_shm.shm_ctime > 0)
				{
					pub_time_change_time(&msipc[i].uni_shm.shm_ctime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).Pid", shm_c);
				sprintf(tmp, "%d", msipc[i].uni_shm.shm_cpid);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).SHMlpid", shm_c);
				sprintf(tmp, "%d", msipc[i].uni_shm.shm_lpid);
				loc_set_zd_data(vars, xml, tmp);
				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).SHMnattch", shm_c);
				sprintf(tmp, "%d", msipc[i].uni_shm.shm_nattch);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SHM(%d).PName", shm_c);
				sprintf(tmp, "%s", msipc[i].uni_shm.pname);
				loc_set_zd_data(vars, xml, tmp);
			}
			shm_c++;
		}
		else if (msipc[i].uni_msg.mqid > 0)
		{
			if (strcmp(opt, "MSG") == 0)
			{
				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).Pid", msg_c);
				sprintf(tmp, "%d", msipc[i].uni_msg.msg_pid);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).PName", msg_c);
				sprintf(tmp, "%s", msipc[i].uni_msg.pname);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQId", msg_c);
				sprintf(tmp, "%d", msipc[i].uni_msg.mqid);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQbytes", msg_c);
				sprintf(tmp, "%ld", msipc[i].uni_msg.msg_cbytes);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQltime", msg_c);
				if (msipc[i].uni_msg.msg_stime > 0)
				{
					pub_time_change_time(&msipc[i].uni_msg.msg_stime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQmodify_time", msg_c);
				if (msipc[i].uni_msg.msg_ctime > 0)
				{
					pub_time_change_time(&msipc[i].uni_msg.msg_ctime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQrecvtime", msg_c);
				if (msipc[i].uni_msg.msg_rtime > 0)
				{
					pub_time_change_time(&msipc[i].uni_msg.msg_rtime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQqnum", msg_c);
				sprintf(tmp, "%d", msipc[i].uni_msg.msg_qnum);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQMaxbytes", msg_c);
				sprintf(tmp, "%d", msipc[i].uni_msg.msg_qbytes);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQlsendpid", msg_c);
				sprintf(tmp, "%d", msipc[i].uni_msg.msg_lspid);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.MSG(%d).MQlrecvpid", msg_c);
				sprintf(tmp, "%d", msipc[i].uni_msg.msg_lrpid);
				loc_set_zd_data(vars, xml, tmp);
			}
			msg_c++;
		}
		else if (msipc[i].uni_sem.semid > 0)
		{
			if(strcmp(opt, "SEM") == 0)
			{
				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SEM(%d).SEMid", sem_c);
				sprintf(tmp, "%d", msipc[i].uni_sem.semid);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SEM(%d).SEMltime", sem_c);
				if (msipc[i].uni_sem.sem_otime > 0)
				{
					pub_time_change_time(&msipc[i].uni_sem.sem_otime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SEM(%d).SEMltime_modify", sem_c);
				if (msipc[i].uni_sem.sem_ctime > 0)
				{
					pub_time_change_time(&msipc[i].uni_sem.sem_ctime, tmp, 5);
				}
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SEM(%d).SEMnsems", sem_c);
				sprintf(tmp, "%d", msipc[i].uni_sem.sem_nsems);
				loc_set_zd_data(vars, xml, tmp);

				memset(xml, 0x00, sizeof(xml));
				memset(tmp, 0x00, sizeof(tmp));
				sprintf(xml, ".TradeRecord.Response.IPCS.SEM(%d).SEMNAME", sem_c);
				sprintf(tmp, "%s", msipc[i].uni_sem.pname);
				loc_set_zd_data(vars, xml, tmp);
			}
			sem_c++;
		}
	}

	loc_set_zd_int(vars, ".TradeRecord.Response.SHMCNT", shm_c);
	loc_set_zd_int(vars, ".TradeRecord.Response.MSGCNT", msg_c);
	loc_set_zd_int(vars, ".TradeRecord.Response.SEMCNT", sem_c);

OkExit:
	pub_log_info("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_OK;

ErrExit:
	pub_log_info("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}
