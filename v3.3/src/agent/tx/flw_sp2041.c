/************************************************************************************
 文 件 名:  flw_sp2041.c                                                           **
  功能描述: 获取平台指定产品的所有进程信息、启停产品、启停产品中的服务或侦听       **
  作    者:                                                                        **
  完成日期: 20160608                                                               **
 ************************************************************************************/
#include "agent_comm.h"
#include "pub_proc.h"

#define CHECK_SAFE_UNPASS -2

static char rescode[16];
static char resmsg[1024];

static int query_prdtinfo(sw_loc_vars_t *vars);
static int manage_prdt(sw_loc_vars_t *vars, char *option);
static int manage_proc_in_prdt(sw_loc_vars_t *vars, char *option);
static int check_opt_security(sw_loc_vars_t *vars, int type);


int sp2041(sw_loc_vars_t *vars)
{
	int 	ret;
	char 	option[4];

	memset(option, 0x0, sizeof(option));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", option);
	if (strlen(option) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.Option can not be null", __FILE__, __LINE__);
		strncpy(resmsg, ".TradeRecord.Request.Option can not be null", sizeof(resmsg));
		strcpy(rescode, "E012");
		goto ErrExit;
	}

	if (strcmp(option, "L") == 0)
	{
		pub_log_info("[%s][%d] 查询平台所有产品的所有进程信息", __FILE__, __LINE__);
		ret = query_prdtinfo(vars);
		if (ret == SW_ERROR)
		{
			goto ErrExit;
		}
	}
	else if (strcmp(option, "S") == 0)
	{
		/*启动产品*/
		pub_log_info("[%s][%d] 启动产品", __FILE__, __LINE__);
		ret = manage_prdt(vars, "start");
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] 启动产品失败", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(option, "T") == 0)
	{
		/*停止产品*/
		pub_log_info("[%s][%d] 停止产品", __FILE__, __LINE__);
		ret = manage_prdt(vars, "stop");
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] 停止产品失败", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(option, "SP") == 0)
	{
		/*启动侦听或服务*/
		pub_log_info("[%s][%d] 启动侦听或服务", __FILE__, __LINE__);
		ret = manage_proc_in_prdt(vars, "s");
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] 启动侦听或服务失败", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(option, "TP") == 0)
	{
		/*停止侦听或服务*/
		pub_log_info("[%s][%d] 停止侦听或服务", __FILE__, __LINE__);
		ret = manage_proc_in_prdt(vars, "stop");
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] 停止侦听或服务失败", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else if (strcmp(option, "CT") == 0)
	{
		/*停止进程安全性检查*/
		ret = check_opt_security(vars, 1);
		if (ret == SW_ERROR)
		{
			pub_log_error("[%s][%d] check_opt_security error", __FILE__, __LINE__);
			goto ErrExit;
		}
	}
	else 
	{
		pub_log_error("[%s][%d] 未知请求", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		goto ErrExit;
	}

	goto OkExit;

OkExit:
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", resmsg);
	pub_log_debug("[%s][%d] [%s] Trade 2041 Success![END][OK]", __FILE__, __LINE__, __FUNCTION__);

	return SW_OK;
ErrExit:

	pub_log_debug("[%s][%d] [%s]Trade 2041 Fails!", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(rescode, resmsg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", rescode);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", resmsg);

	return SW_ERROR;
}

int get_chnl_type(char *lsnname, char *type)
{
	char 	file[128] = {0};
	sprintf(file, "%s/cfg/listener.lsn",getenv("SWWORK"));

	if (!pub_file_exist(file))
	{
		pub_log_error("[%s][%d] Not find file [%s]!", __FILE__, __LINE__, file);
		return SW_ERROR;
	}

	sw_xmltree_t *xml = cfg_read_xml(file);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! file=[%s]", __FILE__, __LINE__, file);
		return SW_ERROR;
	}

	sw_xmlnode_t *node1 = NULL;
	sw_xmlnode_t *node  = pub_xml_locnode(xml, ".DFISBP.SWLISTEN");
	while(node != NULL)
	{
		if (strcmp(node->name, "SWLISTEN") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config LSNNAME!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return SW_ERROR;
		}
		if (strcmp(node1->value, lsnname) == 0)
		{
			node1 = pub_xml_locnode(xml, "COMTYPE");
			if (node1 != NULL && node1->value != NULL)
			{
				strcpy(type, node1->value);
			}
			break;
		}
		node = node->next;
	}

	if (node == NULL)
	{
		pub_log_error("[%s][%d] can not find type of chnl[%s]", __FILE__, __LINE__, lsnname);
		return SW_ERROR;
	}
	return SW_OK;
}


int query_prdtinfo(sw_loc_vars_t *vars)
{
	int 	loop     = 0;
	int 	prdt_idx = 0;
	int 	proc_idx = 0;
	int 	page_cnt = 0;
	int 	page_idx = 0;
	int 	page_sum = 0;
	char  	buf[1024];
	char 	line[1024];
	char    cname[128];
	char	status[32];
	char	sts[32];
	char 	*p    = NULL;
	char 	*var  = NULL;
	char 	*tmp  = NULL;
	char 	*pstr = NULL;
	FILE 	*pf   = NULL;

	loc_get_zd_int(vars, ".TradeRecord.Request.PageIndex", &page_idx);
	loc_get_zd_int(vars, ".TradeRecord.Request.PageCount", &page_cnt);
	if (page_cnt <= 0)
	{
		strcpy(rescode, "E012");
		pub_log_error("[%s][%d] PageCount can not be zero", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (page_idx < 1)
	{
		page_idx = 1;
	}

	int begin  = (page_idx - 1) * page_cnt;
	int end    = page_idx * page_cnt; 
	bool legal = false;
	pub_log_debug("[%s][%d] page_cnt[%d] page_idx[%d] begin[%d] end[%d]", __FILE__, __LINE__, page_cnt, page_idx, begin, end);

	pf = popen("swadmin l -pp", "r");
	if (pf == NULL)
	{
		strcpy(rescode, "E010");
		pub_log_error("[%s][%d] popen[swadmin l] error[%d][%s]."
				, __FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}
	fseek(pf, 0, SEEK_SET);

	while (!feof(pf))
	{
		memset(line, 0x00, sizeof(line));
		pstr = fgets(line, sizeof(line), pf);
		if (pstr == NULL)
		{
			if (feof(pf))
			{
				pub_log_debug("[%s][%d] end of file", __FILE__, __LINE__);
				break;
			}

			pub_log_error("[%s][%d] fgets return NULL", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}

		if (line[0] == '\n' && strlen(line) == 1)
		{
			continue;
		}

		legal = prdt_idx >= begin && prdt_idx < end;
	
		p = strtok(line, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtoks return NULL", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			if ((var = strstr(p, "sw")) != NULL)
			{
				pub_log_debug("[%s][%d] process name:[%s]", __FILE__, __LINE__, var);
				if (legal)
				{
					memset(buf, 0x00, sizeof(buf));
					sprintf(buf, ".TradeRecord.Response.Products.Product(%d).ProcInfos.ProcInfo(%d).Proc", loop, proc_idx);
					loc_set_zd_data(vars, buf, var);
				}
			}
			else if ((var = strstr(p, "PRODUCT:")) != NULL)
			{
				tmp = strchr((const char *)p, ':');
				if (tmp != NULL)
				{
					if (legal)
					{
						pub_log_debug("[%s][%d] ProductName[%s] loop[%d]", __FILE__, __LINE__, tmp + 1, loop);
						memset(buf, 0x00, sizeof(buf));
						sprintf(buf, ".TradeRecord.Response.Products.Product(%d).Name", loop);
						loc_set_zd_data(vars, buf, tmp+1);
						memset(cname, 0x0, sizeof(cname));
						agt_get_pdt_name(tmp + 1, cname, sizeof(cname) - 1);
						memset(buf, 0x00, sizeof(buf));
						sprintf(buf, ".TradeRecord.Response.Products.Product(%d).PrdtCName", loop);
						if (strlen(cname) != 0)
						{
							loc_set_zd_data(vars, buf, cname);
						}
						else
						{
							loc_set_zd_data(vars, buf, "");
						}
					}
				}
				p = strtok(NULL, " ");
				if (p == NULL)
				{
					pub_log_error("[%s][%d] strtoks return NULL", __FILE__, __LINE__);
					pclose(pf);
					strcpy(rescode, "E010");
					return SW_ERROR;
				}
				tmp = strstr(p, "STATUS:");
				if (tmp != NULL)
				{
					if (legal)
					{
						pub_log_debug("[%s][%d] ProductStatus[%s]", __FILE__, __LINE__, tmp);
						memset(status, 0x0, sizeof(status));
						memset(sts, 0x0, sizeof(sts));
						if (strcmp(tmp + 7, "NORMAL") == 0)
						{
							strcpy(status, "正常");
							strcpy(sts, "0");
						}
						else if (strcmp(tmp + 7, "STOPED") == 0)
						{
							strcpy(status, "停止");
							strcpy(sts, "1");
						}
						else if (strcmp(tmp + 7, "ABNORMAL") == 0  || strcmp(tmp + 7, "ABORTED") == 0)
						{
							strcpy(status, "异常");
							strcpy(sts, "1");
						}
						else if (strcmp(tmp + 7, "OFF") == 0)
						{
							strcpy(status, "未启用");
							strcpy(sts, "2");
						}
						else
						{
							strcpy(status, "未知状态");
							strcpy(sts, "2");
						}
						memset(buf, 0x00, sizeof(buf));
						sprintf(buf, ".TradeRecord.Response.Products.Product(%d).PrdtStatus", loop);
						loc_set_zd_data(vars, buf, status);
						memset(buf, 0x00, sizeof(buf)); 
						sprintf(buf, ".TradeRecord.Response.Products.Product(%d).PrdtSts", loop);
						loc_set_zd_data(vars, buf, sts);
						loop++;
					}
				}
				else
				{
					strcpy(rescode, "E010");
					return SW_ERROR;
				}

				prdt_idx++;
				proc_idx = 0;
				continue;
			}
			else
			{
				continue;
			}
		}

		p = strtok(NULL, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtok return NULL!", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			if (legal)
			{
				pub_log_debug("[%s][%d] process type:[%s]", __FILE__, __LINE__, p);
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.Products.Product(%d).ProcInfos.ProcInfo(%d).Type", prdt_idx, proc_idx);
				loc_set_zd_data(vars, buf, p);
			}
		}

		p = strtok(NULL, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtok return NULL!", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			if (legal)
			{
				pub_log_debug("[%s][%d] process pid:[%s]", __FILE__, __LINE__, p);
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.Products.Product(%d).ProcInfos.ProcInfo(%d).Pid", prdt_idx, proc_idx);
				loc_set_zd_data(vars, buf, p);
			}
		}

		p = strtok(NULL, " ");
		if (p == NULL)
		{
			pub_log_error("[%s][%d] strtok return NULL!", __FILE__, __LINE__);
			pclose(pf);
			strcpy(rescode, "E010");
			return SW_ERROR;
		}
		else
		{
			if (legal)
			{
				p[strlen(p)-1] = '\0';
				pub_log_debug("[%s][%d] process status:[%s]", __FILE__, __LINE__, p);
				
				if (strcmp(p, "NORMAL") == 0)
				{
					strcpy(status, "正常");
					strcpy(sts, "0");
				}
				else if (strcmp(p, "NORMAL(ON)") == 0)
				{
					strcpy(status, "正常(开启)");
					strcpy(sts, "0");
				}
				else if (strcmp(p, "NORMAL(OFF)") == 0)
				{
					strcpy(status, "正常(关闭)");
					strcpy(sts, "1");
				}
				else if (strcmp(p, "STOPED") == 0)
				{
					strcpy(status, "停止");
					strcpy(sts, "1");
				}
				else if (strcmp(p, "ABORTED") == 0 || strcmp(p, "ABNORMAL") == 0)
				{
					strcpy(status, "异常");
					strcpy(sts, "1");
				}
				else
				{
					strcpy(status, "未知");
					strcpy(sts, "1");
					pub_log_error("[%s][%d] unknown status", __FILE__, __LINE__);
				}

				memset(buf, 0x0, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.Products.Product(%d).ProcInfos.ProcInfo(%d).ProcStatus", prdt_idx, proc_idx);
				loc_set_zd_data(vars, buf, status);
				
				memset(buf, 0x0, sizeof(buf));
				sprintf(buf, ".TradeRecord.Response.Products.Product(%d).ProcInfos.ProcInfo(%d).ProcSts", prdt_idx, proc_idx);
				loc_set_zd_data(vars, buf, sts);
			}
		}
		proc_idx++;
	}

	if (prdt_idx%page_cnt != 0)
	{
		page_sum = prdt_idx/page_cnt + 1;
	}
	else
	{
		page_sum = prdt_idx/page_cnt;
	}
	pub_log_debug("[%s][%d] count[%d] page_sum[%d]", __FILE__, __LINE__, prdt_idx, page_sum);
	loc_set_zd_int(vars, ".TradeRecord.Response.PageSum", page_sum);
	loc_set_zd_int(vars, ".TradeRecord.Response.Cnt", prdt_idx);
	pclose(pf);
	return SW_OK;
}


int manage_proc_in_prdt(sw_loc_vars_t *vars, char *option)
{
	int 	ret;
	char 	cmd[256];
	char 	prdt_name[MAX_NAME_LEN];
	char 	proc_name[MAX_NAME_LEN];

	pub_log_info("[%s][%d] 管理产品进程",__FILE__,__LINE__);

	memset(prdt_name, 0x0, sizeof(prdt_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.Name", prdt_name);
	if (strlen(prdt_name) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.Name can not be null", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}

	memset(proc_name, 0x0, sizeof(proc_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.ProcName", proc_name);
	if (strlen(proc_name) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.ProcName can not be null", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}

	memset(cmd,     0x0, sizeof(cmd));
	if (strncmp(proc_name, "swlsn_", 6) == 0)
	{
		sprintf(cmd, "swadmin %s -pl %s %s", option, prdt_name, proc_name + 6);
	}
	else if (strncmp(proc_name, "swsvcman_", 9) == 0)
	{
		sprintf(cmd, "swadmin %s -s %s", option, proc_name + 9);
	}
	else
	{
		pub_log_info("[%s][%d] unknown process[%s]", __FILE__,__LINE__, proc_name);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}

	ret =  agt_system(cmd);
	if( ret != SW_OK )
	{
		pub_log_error("[%s][%d] execute [%s] error",__FILE__,__LINE__,cmd);
		strcpy(rescode, "E010");
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] execute cmd[%s] success", __FILE__,__LINE__,cmd);
	return SW_OK;
}

int check_opt_security(sw_loc_vars_t *vars, int type)
{
	char    buf[256] = {0};
	char   	chnl_type[128]   = {0};
	char 	client_chnl[128] = {0};
	char 	server_chnl[128] = {0};
	char 	prdt_name[MAX_NAME_LEN];
	char 	proc_name[MAX_NAME_LEN];

	strcpy(client_chnl, "TCPSC TPCLA TUXSC TUXLC SOAPSC HTTPSC MQ");
	strcpy(server_chnl, "TCPSS, TCPLA, SOAPSS, HTTPSS, MQ");

	pub_log_info("[%s][%d] 产品进程操作安全性检查",__FILE__,__LINE__);

	memset(prdt_name, 0x0, sizeof(prdt_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.Name", prdt_name);
	if (strlen(prdt_name) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.Name can not be null", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}

	memset(proc_name, 0x0, sizeof(proc_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.ProcName", proc_name);
	if (strlen(proc_name) == 0)
	{
		pub_log_error("[%s][%d] .TradeRecord.Request.ProcName can not be null", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}

	if (strncmp(proc_name, "swlsn_", 6) == 0)
	{
		memset(chnl_type, 0x0, sizeof(chnl_type));
		get_chnl_type(proc_name + 6, chnl_type);
		pub_log_debug("[%s][%d] chnl_type[%s]", __FILE__, __LINE__, chnl_type);		
		if (strlen(chnl_type) != 0)
		{
			if (type == 1 && strstr(client_chnl, chnl_type))
			{
				sprintf(resmsg, "[%s]为发起渠道，为保证交易安全性，请确认是否已停止相应的接收渠道。|是否确认停止%s渠道？", 
						proc_name + 6, proc_name + 6);
				strcpy(rescode, "E045");
				pub_log_debug("[%s][%d] resmsg[%s]", __FILE__, __LINE__, resmsg);
				return CHECK_SAFE_UNPASS;
			}
			else if (type ==0 && strstr(server_chnl, chnl_type))
			{
				sprintf(resmsg, "[%s]为接收渠道，为保证交易安全性，请确认是否已启动相关的发送渠道和服务。|是否确认启动%s渠道？", 
						proc_name + 6, proc_name + 6);
				strcpy(rescode, "E045");
				return CHECK_SAFE_UNPASS;
			}
			pub_log_debug("[%s][%d] [%s] check passed", __FILE__, __LINE__, proc_name);
		}
		else
		{
			pub_log_error("[%s][%d] get channel[%s] type error", __FILE__, __LINE__, proc_name + 6);
			memset(buf, 0x0, sizeof(buf));
			strcpy(rescode, "E012");
			return SW_ERROR;
		}
	}
	else if (strncmp(proc_name, "swsvcman_", 9) == 0)
	{
		sprintf(buf, "请确认是否已停止与服务%s相关的渠道。|是否确认停止服务%s？", proc_name, proc_name);
		strcpy(resmsg, buf);
		return CHECK_SAFE_UNPASS;
	}
	else
	{
		pub_log_info("[%s][%d] unknown process[%s]", __FILE__,__LINE__, proc_name);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}

	sprintf(resmsg, "请确认是否停止%s？", proc_name);
	pub_log_debug("[%s][%d] resmsg[%s]", __FILE__, __LINE__, resmsg);
	return SW_OK;
}

int manage_prdt(sw_loc_vars_t *vars, char *option)
{
	int 	ret;
	char 	cmd[128];
	char 	prdt_name[MAX_NAME_LEN];

	pub_log_info("[%s][%d] 管理产品 %s", __FILE__, __LINE__, option);
	memset(cmd, 0x0, sizeof(cmd));
	memset(prdt_name, 0x0, sizeof(prdt_name));
	loc_get_zd_data(vars, ".TradeRecord.Request.Name", prdt_name);
	if (strlen(prdt_name) == 0)
	{
		pub_log_error("[%s][%d] 产品名不能为空", __FILE__, __LINE__);
		strcpy(rescode, "E012");
		return SW_ERROR;
	}
	sprintf(cmd, "swadmin %s -p %s", option, prdt_name);
	ret = agt_system(cmd);
	if( ret != SW_OK )
	{
		pub_log_error("[%s][%d] execute [%s] error",__FILE__,__LINE__,cmd);
		strcpy(rescode, "E010");
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] execute cmd[%s] success", __FILE__,__LINE__,cmd);
	return SW_OK;
}

