#include <pthread.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include "agent_search.h"
#include "agent_comm.h"
char item[CNT][NODE_SIZE];
static int get_log_from_file(char *filename, char *date, char *fls);

static int my_splits(char *sBuf)
{
	char *pr = NULL;
	int i = 0;
	if (sBuf == NULL)
	{
		return -1;
	}
	
	memset(item, 0x00, sizeof(item));
	while ((pr = strsep(&sBuf, "|")) != NULL)
	{
		memcpy(item[i], pr, strlen(pr));
		i++;
		if (i >= CNT)
		{
			break;
		}
	}
	
	return i;
}

static  int agt_strntailcmp(char *str1, char *str2, int n)
{
	return strncmp(str1 + strlen(str1) - n, str2 + strlen(str2) - n, n);
}

int get_log( char *chl, char *cdate, char *ctime, char *fls)
{
	char log_dir[256];
	char bakname[256];
	char log_file[256];
	int i = 0;
	int file_num = 0;
	struct dirent **namelist = NULL;	

	memset(log_dir, 0x0, sizeof(log_dir));
	
	if (getenv("SWLOG") == NULL)
	{
		snprintf(log_dir, sizeof(log_dir)-1, "%s/log/syslog", getenv("SWWORK"));
	}
	else
	{
		snprintf(log_dir, sizeof(log_dir)-1, "%s", getenv("SWLOG"));
	}
	
	memset(bakname, 0x0, sizeof(bakname));
	snprintf(bakname, sizeof(bakname)-1, "%s%s0000.bak", cdate, ctime);
	
	namelist = agt_scan_dir(&file_num, log_dir);
	if (namelist == NULL)
	{
		pub_log_error("[%s][%d] scan dir error", __FILE__, __LINE__);
		return -1;
	}
	i = 0;
	pub_log_info("[%s][%d] file_num=%d", __FILE__, __LINE__, file_num);
	while(i < file_num)
	{
		pub_log_info("[%s][%d] 搜索文件 %s",__FILE__,__LINE__, namelist[i]->d_name);
		if (strncmp(namelist[i]->d_name, chl, strlen(chl)) != 0 )
		{
			i++;
			continue;
		}
		if (agt_strntailcmp(namelist[i]->d_name, ".log", 4) != 0 &&
				agt_strntailcmp(namelist[i]->d_name, bakname, strlen(bakname)) <= 0)
		{
			i++;
			continue;
		}
		pub_log_info("[%s][%d] 搜索文件 %s",__FILE__,__LINE__, namelist[i]->d_name);
		memset(log_file, 0x0, sizeof(log_file));
		snprintf(log_file, sizeof(log_file)-1, "%s/%s", log_dir, namelist[i]->d_name);
		i++;
		
		if (get_log_from_file(log_file, cdate, fls) == 0)
		{
			break;
		}
	}
	
	 agt_free_namelist(namelist, file_num);
	return SW_OK;
}

int get_log_from_file(char *filename, char *date, char *fls)
{
	FILE *fp = NULL;
	FILE *lf = NULL;
	char line[LINE_SIZE];
	char buf[LINE_SIZE];
	int flag = NO;
	char	tmp[256];
	
	memset(tmp, 0x0, sizeof(tmp));
	sprintf(tmp,"%s/dat/%s_%s.log", getenv("SWWORK"), date, fls);
	pub_log_info("[%s][%d] tmp=[%s]",__FILE__,__LINE__,tmp);
	
	
	lf = fopen(tmp, "w");
	if (lf == NULL)
	{
		pub_log_info("[%s][%d]open file [%s] is failed!", __FILE__, __LINE__, "aa.txt");
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d]open file [%s]\n", __FILE__, __LINE__, filename);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		pub_log_info("[%s][%d]open file [%s] failed!", __FILE__, __LINE__, filename);
		fclose(lf);
		return SW_ERROR;
	}

	while (!feof(fp))
	{
				memset(item, 0x0, sizeof(item));
		memset(line, 0x0, sizeof(line));
		memset(buf, 0x0, sizeof(buf));

		fgets(line, sizeof(line) -1, fp);


		if (line[0] != '@')
		{
			if (flag == FIND)
			{
				fwrite(line, strlen(line), 1, lf);
			}
		}
		else
		{
			strcpy(buf, line);
			my_splits(line);
			if (strcmp(item[2], date) == 0 && strcmp(item[5], fls) == 0)
			{
				pub_log_debug("[%s][%d] buf=[%s]",__FILE__,__LINE__,buf);
				fwrite(buf, strlen(buf), 1, lf);
				flag = FIND;
			}
			else 
			{
				if (flag == FIND)
				{
					break;
				}
			}
		}
	}

	fclose(fp);
	fclose(lf);
	if (flag == FIND)
	{
		pub_log_info("[%s][%d]find trcno[%s] success", __FILE__, __LINE__, fls);
		return SW_OK;
	}
	remove(tmp);
	return SW_ERROR;
}

int log_search(char *chl, char *cdate, char *ctime, char *fls)
{
	int result = 0;
	
	if (chl == NULL || cdate == NULL || ctime ==  NULL || fls == NULL)
	{
		pub_log_info("[%s][%d]the input is NULL", __FILE__, __LINE__);
		return -1;
	}
	pub_log_info("chl:%s, cdate:%s, ctime:%s, fls:%s", chl, cdate, ctime, fls);
	
	
	result = get_log(chl, cdate, ctime, fls);
	if( result != SW_OK )
	{
		pub_log_error("[%s][%d] get_log error",__FILE__,__LINE__);
		return SW_ERROR;	
	}
	
	return SW_OK;
}
