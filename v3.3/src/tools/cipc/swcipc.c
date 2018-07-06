#include "uni_svr.h"
int main()
{
	int	i = 0;
	int	ret = -1;
	char	log_path[64];
	uni_msipc_t	msipc[UNI_MAX_IPC_CNT];
	
	for (i = 0; i < UNI_MAX_IPC_CNT; i++)
	{
		memset(&msipc[i], 0x00, sizeof(uni_msipc_t));
	}
	
	memset(log_path, 0x00, sizeof(log_path));
	sprintf(log_path, "%s/log/syslog/swclear.log", getenv("SWWORK"));
	ret = pub_log_chglog(SW_LOG_CHG_DBGFILE, log_path);
	if (ret < 0)
	{
		pub_log_error("[%s][%d]pub_log_chglog", __FILE__, __LINE__);
		return -1;
	}
	
	ret = uni_get_ipc_info(msipc);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] Get IPC info error!", __FILE__, __LINE__);
		return -1;
	}
	
	uni_clear_ipc_source(msipc);
	
	return 0;
}

