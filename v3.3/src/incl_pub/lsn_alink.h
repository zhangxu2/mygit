#ifndef __ALINK_H__
#define __ALINK_H__

sw_int_t alink_save_linkinfo(sw_int_t sendtype, sw_link_t *link, sw_cmd_t *cmd);
sw_int_t alink_delete_linkinfo(sw_link_t *link);
sw_int_t alink_load_linkinfo_by_key(sw_int_t sendtype, sw_link_t *link);

#endif
