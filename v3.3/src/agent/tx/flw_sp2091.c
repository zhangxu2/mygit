/*************************************************
  文 件 名:  flw_sp2091.c                        **
  功能描述:  创建产品级目录                       **
  作    者:                                      **
  完成日期:                                      **
 *************************************************/
#include "agent_comm.h"

#define MAX_PATHNAME_LEN 2048

static const char *product_subdires[] = {
    "bin",
    "lib",
    "log",
    "etc",
    "etc/lsncfg",
    "etc/svrcfg",
    "product",
    "plugin",
    "components",
    "src",
    "data",
    "templates",
};

static mode_t get_env_mode(void)
{
	mode_t envmask;

	envmask = umask(0); /* parm 0 is optional */
	umask(envmask);

	return 0777 & ~envmask;
}

static int is_legal_path_name(const char *pathname)
{
	assert(pathname);

	if ('\0' == pathname[0])
	{
		return 0; /* false */
	}
	return 1; /* true */
}

static int is_legal_prdt_name(const char *product_name)
{
	assert(product_name);
	if (!is_legal_path_name(product_name))
	{
		return 0; /* false */
	}

	while('\0' != *product_name)
	{
		if((isalnum(*product_name))
			|| ('-' == *product_name)
			|| ('_' == *product_name))
		{
			product_name++;
			continue;
		}
		else
		{
			return 0; /* false */
		}
		/* do NOT add code here */
	}
	return 1; /* ture */
}

int sp2091(sw_loc_vars_t *vars)
{
	int	rc;
	int	i; /* loop count */
	char    reply[8];
	char    res_msg[256];
	char	path_inxml[256];
	char	product_name[MAX_PATHNAME_LEN];
	char	dirpath_tomake[MAX_PATHNAME_LEN];
	mode_t	curmode; /* permission mode */

	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));

	pub_mem_memzero(product_name, sizeof(product_name));
	sprintf(path_inxml, "%s", ".TradeRecord.Request.ProductName");
	loc_get_zd_data(vars, path_inxml, product_name);

	if (!is_legal_prdt_name(product_name))
	{
		strcpy(reply, "E012");
		pub_log_error("[%s][%d] productname[%s] is illegal", __FILE__, __LINE__, product_name);
		goto ErrExit;
	}
    
	/* when in doubt why set i = -1, think about what if mkdir()
	* failed in the following for loop */
	i = -1; 

	curmode = get_env_mode();
	memset(dirpath_tomake, 0x0, sizeof(dirpath_tomake));
	sprintf(dirpath_tomake, "%s/%s/%s", getenv("SWWORK"), 
		"products", product_name);
	assert(is_legal_path_name(dirpath_tomake));
	rc = mkdir(dirpath_tomake, curmode);
	if (0 == rc)
	{
		for (i = 0; i < sizeof(product_subdires)/sizeof(char *); i++)
		{
			memset(dirpath_tomake, 0x0, sizeof(dirpath_tomake));
			sprintf(dirpath_tomake, "%s/%s/%s/%s", getenv("SWWORK"), 
				"products", product_name, product_subdires[i]);
			assert(is_legal_path_name(dirpath_tomake));
			rc = mkdir(dirpath_tomake, curmode);
			if(-1 == rc)
			{
				break;
			}
		}
		/* do not add code in this position that use i */
	}
    
	/* do not add code in this position that use i */
	if (-1 == rc)
	{
		pub_log_error( "[%s] [%d] mkdir: %s failed because %s" ,
			__FILE__, __LINE__, dirpath_tomake, strerror(errno));

		/* do not add code in this position that use i */
		if (i != -1)
		{
			/* dirpath_tomake maybe confusing here,
			* it's used to rmdir in this if { } block */
			for (--i; i >=0; i--)
			{
				memset(dirpath_tomake, 0x0, sizeof(dirpath_tomake));
				sprintf(dirpath_tomake, "%s/%s/%s/%s", 
					getenv("SWWORK"), "products",
					product_name, product_subdires[i]);
				rc = rmdir(dirpath_tomake);
				if (-1 == rc)
				{
					pub_log_error("[%s] [%d] rmdir: %s failed because %s",
						__FILE__, __LINE__, dirpath_tomake, strerror(errno));
					continue; /* clearly not break */
				}
			}

			memset(dirpath_tomake, 0x0, sizeof(dirpath_tomake));
			sprintf(dirpath_tomake, "%s/%s/%s", getenv("SWWORK"), 
				"products", product_name);
			rc = rmdir(dirpath_tomake);
			if (-1 == rc)
			{
				pub_log_error( 
					"[%s] [%d] rmdir: %s failed because %s",
					__FILE__, __LINE__, dirpath_tomake, 
					strerror(errno));
			}
		}


		pub_log_error("[%s][%d] Transaction Processing Failed", __FILE__, __LINE__);
		strcpy(reply, "E010");
		goto ErrExit;
	}

OkExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "Transaction processes successful", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_OK;
ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}
