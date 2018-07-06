#ifndef __ADM_CMD_HANDLER_H__
#define __ADM_CMD_HANDLER_H__

#include "pub_log.h"
#include "pub_string.h"
#include "seqs.h"
#include "admin.h"
#include "adm_opt.h"
#include "uni_svr.h"
#include "uni_trace.h"

#define  UPD_PACK		"upd_pack.sh"

typedef sw_int_t (*adm_cmd_handler_pt)(sw_adm_cycle_t* cycle);

#define ADM_USE_MAX_LEN	(64)
#define ADM_CMD_DESC_MAX_LEN	(128)

typedef struct sw_adm_cmd_handler_s
{
	sw_int32_t	cmd_type;
	adm_mod_type_t	mod_type;
	adm_cmd_handler_pt	cmd_handle_func;
	sw_char_t	usage[ADM_USE_MAX_LEN];
	sw_char_t	desc[ADM_CMD_DESC_MAX_LEN];
}sw_adm_cmd_handler_t;

#define adm_opt_set_submod_str(opt, mod_str)  \
            snprintf(opt->mod_name, sizeof(opt->mod_name), "%s", mod_str)
            
#define ADM_TOP_DEF_NUM (19)
#define ADM_TOP_MAX_NUM (1000)

#define NODE_TYPE_DESC_MAX_LEN    (128) /* �ڵ�����������Ϣ��󳤶� */

/* �ڵ����������� */
typedef struct
{
    int type;       /* �ڵ�����: ND_LSN ~ ND_XXX */
    char desc[NODE_TYPE_DESC_MAX_LEN];  /* ������Ϣ */
}node_type_desc_t;

#define ROUTE_STATUS_DESC_MAX_LEN   (32)/* ·�ɴ���״̬����������Ϣ��󳤶� */

/* ·��״̬������ */
typedef struct
{
    int status;     /* �ڵ�����: 0-���ڴ�����... 1-�ɹ� 2-�쳣 */
    char desc[ROUTE_STATUS_DESC_MAX_LEN];   /* ������Ϣ */
}route_status_desc_t;

SW_PUBLIC sw_int_t adm_list_version(sw_adm_cycle_t *cycle);
SW_PUBLIC sw_int_t adm_list_prdt_all(sw_adm_cycle_t* cycle);
SW_PUBLIC sw_int_t adm_cmd_handle(sw_adm_cycle_t* cycle, char* line);


#endif /* __ADM_CMD_HANDLER_H__ */
