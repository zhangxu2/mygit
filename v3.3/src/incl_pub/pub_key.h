#ifndef __PUB_KEY_H__
#define __PUB_KEY_H__

#include "pub_type.h"
#include "pub_log.h"

#define KEY_INFO_FILE ".keyinfo.file"

enum IPC_TYPE
{
	SW_IPC_SHM,
	SW_IPC_MSG,
	SW_IPC_SEM,
	sw_all_ipc_type
};
struct sw_key_s
{
	int key_no;			/* primarykey of the key_file_info */
	int ipc_id;			
	enum IPC_TYPE id_type;	

};
typedef struct sw_key_s sw_key_t;


extern int pub_key_load(const char *file, key_t *key, int num);

#endif
