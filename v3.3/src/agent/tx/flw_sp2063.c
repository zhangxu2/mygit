/**************************************************
  文 件 名:  flw_sp2063.c                        **
  功能描述:  预警级别管理                        **
  作    者:  邹佩                                **
  完成日期:  20160802                            **
 *************************************************/
#include "agent_comm.h"

int sp2063(sw_loc_vars_t *vars)
{
	char 	opt[8];
	char	reply[8];
	char	level[16];
	char	res_msg[256];
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	memset(opt, 0x00, sizeof(opt));
	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	pub_log_info("[%s][%d]操作标识opt[%s]", __FILE__, __LINE__, opt);

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

	if (opt[0] == 'S')
	{
		node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.LEVEL");
		if (node == NULL || node->value == NULL || node->value[0] == '\0')
		{
			pub_log_error("[%s][%d]  NO .ALERT.REMOTE.SERVER.LEVEL", __FILE__, __LINE__);
			strcpy(reply, "E026");
			pub_xml_deltree(xml);
			goto ErrExit;
		}
		
		strncpy(level, node->value, sizeof(level) - 1);
		loc_set_zd_data(vars, ".TradeRecord.Response.Alert.LEVEL", level);
		loc_set_zd_data(vars, ".TradeRecord.Response.Alert.CNLEVEL", agt_level_name(level));
		pub_xml_deltree(xml);
		
		goto OkExit;
	}
	else if (opt[0] == 'M')
	{
		memset(level, 0x0, sizeof(level));
		loc_get_zd_data(vars, ".TradeRecord.Request.Alert.LEVEL", level);
		if (strlen(level) == 0)
		{
			pub_log_error("[%s][%d] level is null", __FILE__, __LINE__);
			strcpy(reply, "E012");
			pub_xml_deltree(xml);
			goto ErrExit;
		}
	
		node = pub_xml_locnode(xml, ".ALERT.REMOTE.SERVER.LEVEL");
		if (node == NULL)
		{
			pub_log_error("[%s][%d]  NO .ALERT.REMOTE.SERVER.LEVEL", __FILE__, __LINE__);
			strcpy(reply, "E026");
			pub_xml_deltree(xml);
			goto ErrExit;
		}
		
		pub_xml_set_value(node, level);
		pub_xml_pack(xml, xmlname);
		pub_xml_deltree(xml);

		goto OkExit;
	}
	else
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d]操作标识[%s]有误!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		goto ErrExit;
	}

OkExit:
	pub_log_info("[%s][%d] [%s]OK EXIT!", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", "Transaction processes successful");

	return 0;

ErrExit:
	pub_log_info("[%s][%d] [%s]ERR EXIT!", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return -1;
}