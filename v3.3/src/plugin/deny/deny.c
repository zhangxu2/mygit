#include "lsn_pub.h"
#include "pub_xml.h"

int deny(sw_loc_vars_t *vars, sw_buf_t *locbuf, sw_xmltree_t *xml)
{
	pub_log_debug("[%s][%d] deny.....", __FILE__, __LINE__);
	
	loc_set_zd_data(vars, "#sys2", "@@@@");
	loc_set_zd_data(vars, "#sys3", "!!!!");
	loc_set_zd_data(vars, "#sys4", "%%%%");
	pub_log_debug("[%s][%d] deny end!", __FILE__, __LINE__);

	return SW_OK;
}
