#ifndef __AGENT_ADM_H__
#define __AGENT_ADM_H__

#include "agent_pub.h"

typedef enum
{
        AGENT_START,
        AGENT_STOP,
        AGENT_LIST,
        AGENT_CLEAN,
        AGENT_HELP,
        AGENT_QUIT,
        AGENT_UNKNOW,
        AGENT_TOTAL
}agt_adm_type_t;

typedef int (*agt_cmd_handler_pt)();
typedef struct agt_cmd_handler_s
{
	int	cmd_type;
	char	usage[64];
	char	desc[128];
	agt_cmd_handler_pt	cmd_handle_func;
}agt_cmd_handler_t;


#endif



