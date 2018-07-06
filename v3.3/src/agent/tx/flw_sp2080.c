/*************************************************
  文 件 名:  flw_sp2080.c                        **
  功能描述:  激活预警                            **
  作    者:  linpangei                           **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

int sp2080(sw_loc_vars_t *vars)
{
	int 	ret = -1;
	char	buf[256];
	char	res_msg[256];
	char	reply[8];
	char	xmlname[128];
	char	ip[32];
	char	port[32];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(res_msg, 0x00, sizeof(res_msg));
	memset(reply, 0x00, sizeof(reply));
	memset(buf, 0x00, sizeof(buf));

	ret = pub_cfg_get_eswid(buf);
	if (ret == -1)
	{
		pub_log_error("[%s][%d]get pub_cfg_get_eswid error", __FILE__, __LINE__);
		strcpy(reply, "E042");
		goto ErrExit;
	}

	loc_set_zd_data(vars, ".TradeRecord.Response.Ptid", buf);

	memset(ip, 0x0, sizeof(ip));
	loc_get_zd_data(vars, ".TradeRecord.Request.IpAddr", ip);
	if (strlen(ip) == 0)
	{
		goto OkExit;
	}

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/cfg/alert/alert.xml", getenv("SWWORK"));
	if (!pub_file_exist(xmlname))
	{
		pub_log_error("[%s][%d] xml [%s] is not exist!", __FILE__, __LINE__, xmlname);
		strcpy(reply, "E009");
		goto ErrExit;
	}

	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 建树失败! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		strcpy(reply, "E045");
		goto ErrExit;
	}

	node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.IP");
	if (node == NULL)
	{
		pub_log_error("[%s][%d]  NO .ALERT.REMOTE.SERVER.IP", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		strcpy(reply, "E026");
		goto ErrExit;
	}
	ret = pub_xml_set_value(node, ip);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_xml_set_value error!");
		pub_xml_deltree(xml);
		goto ErrExit;
	}

	memset(port, 0x0, sizeof(port));
	strcpy(port, "9002");
	node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.PORT");
	if (node == NULL)
	{
		pub_log_error("[%s][%d]  NO .ALERT.REMOTE.SERVER.PORT", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		strcpy(reply, "E026");
		goto ErrExit;
	}
	ret = pub_xml_set_value(node, port);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_xml_set_value error!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		goto ErrExit;
	}

	ret = pub_xml_pack(xml, xmlname);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] pub_xml_pack error!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		goto ErrExit;
	}
	pub_xml_deltree(xml);

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return 0;

ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}
