#include "svc_cycle.h"

#define MAX_IMPORT_CNT	10
#define FLOW_VAR	"$flowcode"
#define FLOW_EXP	"[$flowcode]"

extern int nComputeExpress(char *express,int length,char *outbuf);
extern int nExtendEqual(char *s1,char *s2);

static int flow_call(sw_char_t *svr_name, sw_char_t *svc_name, int id);
static int flow_link(sw_char_t *svr_name, sw_char_t *svc_name);
static int flow_postsvc(sw_char_t *svr_name, sw_char_t *svc_name);
static int flow_calllsn(sw_char_t *lsn_name, int id);
static int flow_linklsn(sw_char_t *lsn_name);
static int flow_postlsn(sw_char_t *lsn_name);
static int flow_continue(int id);
static int flow_import(FILE *fpw, char *flow);


int	line_cnt = 0;
int	list_cnt = 0;
char	file_list[MAX_IMPORT_CNT][128];
/************************************
 去掉字符串中的前导空格和后导空格,
 并且将多个连续的空格压缩成一个空格 
************************************/
char *flow_zip_space(char *str)
{
	int	iQFlag = 0;
	char	ch = 0;
	char	*ptr = str;
	char	*dst = str;
	
	while (*ptr != '\0')
	{
		if (*ptr != ' ' && *ptr != '\t' && *ptr != 0x0a && *ptr != 0x0d)
		{
			break;
		}
		ptr++;
	}
	while (*ptr != '\0')
	{
		if (strncmp(ptr, "//", 2) == 0)
		{
			break;
		}
		if (*ptr == '"')
		{
			if (iQFlag == 0)
			{
				iQFlag = 1;
			}
			else
			{
				iQFlag = 0;
			}
		}
		if (*ptr == '\t' && iQFlag == 0)
		{
			*ptr = ' ';
		}
		if (iQFlag == 1)
		{
			*str++ = *ptr++;
			continue;
		}
		if (*ptr != ' ')
		{
			ch = *ptr;
			*str++ = *ptr++;
			continue;
		}

		if (ch == ' ')
		{
			ptr++;
			continue;
		}
		ch = *ptr;
		*str++ = *ptr++;
	}
	if (ch == ' ' || ch == 0x0d || ch == 0x0a)
	{
		str--;
	}
	*str = '\0';
	if (iQFlag == 1)
	{
		pub_log_error("[%s][%d] 表达式[%s]格式有误! 缺半个单引号!", 
			__FILE__, __LINE__, dst);
		return NULL;
	}
	
	return dst;
}

static int flow_import(FILE *fpw, char *flow)
{
	int	i = 0;
	int	len = 0;
	int	result = 0;
	FILE	*fpr = NULL;
	char	*ptr = NULL;
	char	*str = NULL;
	char	buf[512];
	char	read_file[128];
	char	read_line[1024];
	char	import_file[128];
	
	memset(read_file, 0x0, sizeof(read_file));
	memset(read_line, 0x0, sizeof(read_line));
	sprintf(read_file, "%s/%s/%s.def", g_svr->flow_path, g_svr->svr_name, flow);
	fpr = fopen(read_file, "r");
	if (fpr == NULL)
	{
		pub_log_error("[%s][%d] Can not open [%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, read_file, errno, strerror(errno));
		return -1;
	}
	
	for (i = 0; i < list_cnt; i++)
	{
		if (strcmp(file_list[i], flow) == 0)
		{
			pub_log_error("[%s][%d] 死循环! flow=[%s]", 
				__FILE__, __LINE__, flow);
			fclose(fpr);
			return -1;
		}
	}
	
	if (list_cnt >= MAX_IMPORT_CNT)
	{
		pub_log_error("[%s][%d] Too many import block!", 
			__FILE__, __LINE__);
		fclose(fpr);
		return -1;
	}
	strcpy(file_list[list_cnt++], flow);
	
	
	memset(read_line, 0x0, sizeof(read_line));
	while (1)
	{
		memset(buf, 0x0, sizeof(buf));
		if (fgets(buf, sizeof(buf) - 1, fpr) == NULL)
		{
			break;
		}
		len = strlen(buf);
		if (buf[len - 1] == '\n')
		{
			buf[len - 1] = '\0';
		}
		
		if (buf[len - 2] == '\\')
		{
			buf[len - 2] = '\0';
			strcat(read_line, buf);
			continue;
		}
		strcat(read_line, buf);
		if (flow_zip_space(read_line) == NULL)
		{
			fclose(fpr);
			pub_log_error("[%s][%d] 预编译失败! read_line=[%s]", 
				__FILE__, __LINE__, read_line);
			return -1;
		}

		if (strlen(read_line) < 2)
		{
			continue;
		}
		
		len = strlen(read_line);
		if (read_line[len - 1] == ':')
		{
			fclose(fpr);
			pub_log_error("[%s][%d] Import块不能出现跳转语句![%s]",
				 __FILE__, __LINE__, read_line);	
			return -1;
		}
		
		if (strncmp(read_line, "GOTO ", 5) == 0)
		{
			fclose(fpr);
			pub_log_error("[%s][%d] Import块不能出现跳转语句![%s]",
				 __FILE__, __LINE__, read_line);	
			return -1;
		}
		
		if (strcmp(read_line, "DECLARE") == 0 || 
			strcmp(read_line, "BEGIN") == 0 ||
			strcmp(read_line, "END") == 0)	
		{
			continue;
		}
		
		if (strncmp(read_line, "IMPORT ", 7) == 0)
		{
			memset(import_file, 0x0, sizeof(import_file));
			str = import_file;
			ptr = read_line + 7;
			while (*ptr != '\0' && *ptr != '.')
			{
				*str++ = *ptr++;
			}
			*str = '\0';
			result = flow_import(fpw, import_file);
			if (result < 0)
			{
				fclose(fpr);
				pub_log_error("[%s][%d] Import [%s] error!", 
					__FILE__, __LINE__, import_file);
				return -1;
			}
		}
		else
		{
			strcat(read_line, "\n");
			fputs(read_line, fpw);
			line_cnt++;
		}
		memset(read_line, 0x0, sizeof(read_line));
	}
	fclose(fpr);
	
	return 0;
}
		
static int flow_analyze(sw_char_t *flow_name)
{
	int	result = 0;
	int	iLen = 0;
	FILE	*fpr = NULL;
	FILE	*fpw = NULL;
	char	*ptr = NULL;
	char	*str = NULL;
	char	flow_path[128];
	char	read_buf[256];
	char	write_file[128];
	char	read_line[1024];
	char	import_file[128];
	
	memset(flow_path, 0x0, sizeof(flow_path));
	memset(read_buf, 0x0, sizeof(read_buf));
	memset(read_line, 0x0, sizeof(read_line));
	memset(import_file, 0x0, sizeof(import_file));
	memset(file_list, 0x0, sizeof(file_list));
	list_cnt = 0;
	
	sprintf(flow_path, "%s/%s/%s.def", g_svr->flow_path, g_svr->svr_name, flow_name);
	fpr = fopen(flow_path, "r");
	if (fpr == NULL)
	{
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, flow_name, errno, strerror(errno));
		return -1;
	}
	
	memset(write_file, 0x0, sizeof(write_file));
	sprintf(write_file, "%s/common/bin/%s_%s_import_%d.tmp", 
		g_svr->flow_path, g_svr->svr_name, flow_name, g_idx);
	fpw = fopen(write_file, "w");
	if (fpw == NULL)
	{
		fclose(fpr);
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, write_file, errno, strerror(errno));
		return -1;
	}
	
	line_cnt = 0;
	while (1)
	{
		if (fgets(read_buf, sizeof(read_buf) - 1, fpr) == NULL)
		{
			break;
		}
		iLen = strlen(read_buf);
		if (read_buf[iLen - 1] == '\n')
		{
			read_buf[iLen - 1] = '\0';
		}
		if (read_buf[iLen - 2] == '\\')
		{
			read_buf[iLen - 2] = '\0';
			strcat(read_line, read_buf);
			continue;
		}
		strcat(read_line, read_buf);
		if (flow_zip_space(read_line) == NULL)
		{
			fclose(fpr);
			fclose(fpw);
			pub_log_error("[%s][%d] 预分析失败! read_line=[%s]",
				__FILE__, __LINE__, read_line);
			return -1;
		}

		if (strlen(read_line) < 2)
		{
			continue;
		}
		strcat(read_line, "\n");
		if (strncmp(read_line, "IMPORT ", 7) == 0)
		{
			list_cnt = 0;
			memset(file_list, 0x0, sizeof(file_list));
			memset(import_file, 0x0, sizeof(import_file));
			str = import_file;
			ptr = read_line + 7;
			while (*ptr != '\0' && *ptr != '.')
			{
				*str++ = *ptr++;
			}
			*str = '\0';
			strcpy(file_list[list_cnt++], flow_name);
			result = flow_import(fpw, import_file);
			if (result < 0)
			{
				fclose(fpr);
				fclose(fpw);
				pub_log_error("[%s][%d] Import [%s] error!",
					__FILE__, __LINE__, import_file);
				return -1;
			}
		}
		else
		{
			fputs(read_line, fpw);
			line_cnt++;
		}
		memset(read_line, 0x0, sizeof(read_line));
	}
	pub_log_info("预编译[%s]完成! 共[%d]行!", flow_name, line_cnt);
	fclose(fpr);
	fclose(fpw);

	return 0;
}

int flow_init(char *flow_name)
{
	FILE	*fp = NULL;
	TSENT	*sent;
	size_t	len = 0;
	sw_char_t	*ptr = NULL;
	int	result = 0;
	int	lineno = 0;
	int	cond_flag = 0;
	int	block_flag = 0;
	sw_char_t	line[1024];
	sw_char_t	filename[128];
	STEPINFO	step_info;
	
	memset(line, 0x0, sizeof(line));
	memset(filename, 0x0, sizeof(filename));
	memset(&step_info, 0x0, sizeof(step_info));
	memset(&flow, 0x0, sizeof(flow));
	
	result = flow_analyze(flow_name);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] [%s] analyze error!", __FILE__, __LINE__, flow_name);
		return SW_ERROR;
	}
	
	sprintf(filename, "%s/common/bin/%s_%s_import_%d.tmp",
		g_svr->flow_path, g_svr->svr_name, flow_name, g_idx);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]", 
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}
	
	lineno = 0;
	block_flag = 0;
	while (1)
	{
		memset(line, 0x0, sizeof(line));
		if (fgets(line, sizeof(line) - 1, fp) == NULL)
		{
			break;
		}
		lineno++;

		if (strlen(line) < 2)
		{
			continue;
		}
		
		len = strlen(line);
		if (line[len - 1] == '\n')
		{
			line[len - 1] = '\0';
		}

		if (strcmp(line, "DECLARE") == 0)
		{
			block_flag = SECTION_DECLARE;
			continue;
		}
		else if (strcmp(line, "BEGIN") == 0)
		{
			block_flag = SECTION_BEGIN;
			continue;
		}
		else if (strcmp(line, "END") == 0)
		{
			break;
		}
		
		if (block_flag == SECTION_BEFORE)
		{
			continue;
		}
		
		if (strncmp(line, "DO ", 3) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] DO sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_DO;
			sent->line = lineno;
			strcpy(sent->express, line + 3);
		}
		else if (strncmp(line, "TUXCALL ", 8) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] TUXCALL sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_TUXC;
			sent->line = lineno;
			strcpy(sent->express, line + 8);
		}
		else if (strncmp(line, "CALL ", 5) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] CALL sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CALL;
			sent->line = lineno;
			strcpy(sent->express, line + 5);
		}
		else if (strncmp(line, "LINK ", 5) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] LINK sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_LINK;
			sent->line = lineno;
			strcpy(sent->express, line + 5);
		}
		else if (strncmp(line, "CALLLSN ", 8) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] CALLLSN sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CALLLSN;
			sent->line = lineno;
			strcpy(sent->express, line + 8);
		}
		else if (strncmp(line, "LINKLSN ", 8) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] LINKLSN sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_LINKLSN;
			sent->line = lineno;
			strcpy(sent->express, line + 8);
		}
		else if (strncmp(line, "POSTLSN ", 8) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] POSTLSN sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_POSTLSN;
			sent->line = lineno;
			strcpy(sent->express, line + 8);
		}
		else if (strncmp(line, "POSTSVC ", 8) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] POSTSVC sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_POSTSVC;
			sent->line = lineno;
			strcpy(sent->express, line + 8);
		}
		else if (strncmp(line, "SET ", 4) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				sent = &flow.decl.sent[flow.decl.count];
				flow.decl.count++;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_SET;
			sent->line = lineno;
			ptr = strtok(line + 4, "=");
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] SET sentence error![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			strcpy(sent->out, ptr);

			ptr = strtok(NULL, "=");
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d] SET sentence error![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			strcpy(sent->express, ptr);
		}
		else if (strncmp(line, "CHECK ", 6) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] CHECK sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CHECK;
			sent->line = lineno;
			strcpy(sent->express, line + 6);
		}
		else if (strncmp(line, "GOTO ", 5) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] GOTO sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_GOTO;
			sent->line = lineno;
			ptr = strtok(line + 5, " ");
			if (ptr == NULL)
			{
				pub_log_error("[%s][%d]GOTO sentence error![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			strcpy(sent->out, ptr);
			
			ptr = strtok(NULL, " ");
			if (ptr != NULL && strcmp(ptr, "IF") == 0)
			{
				ptr = strtok(NULL, " ");
				if (ptr == NULL)
				{
					pub_log_error("[%s][%d]GOTO sentence "
						"error![%d]", __FILE__, __LINE__, lineno);
					fclose(fp);
					return SW_ERROR;
				}
				strcpy(sent->express, ptr);
			}
			else if (ptr != NULL && (*ptr == 'U' || *ptr == 'u'))
			{
				strcpy(sent->express, ptr);
			}
		}
		else if (line[strlen(line) - 1] == ':')
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] STEP sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_LABEL;
			sent->line = lineno;
			strncpy(sent->express, line, strlen(line) - 1);
		}
		else if (strncmp(line, "COMPENSATIONDO", 14) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] COMPENSATIONDO sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CMPDO;
			sent->line = lineno;
			strcpy(sent->out, line + 15);
			sent->step = 999;
		}
		else if (strncmp(line, "COMP_BEGIN", 10) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] COMP_BEGIN sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CMPBEG;
			sent->line = lineno;
		
			step_info.curmaxstep++;
			step_info.curlevel++;
			if (step_info.curlevel > 3)
			{
				pub_log_error("[%s][%d]Too many COMPENSATIONDO",
					__FILE__,__LINE__);
				fclose(fp);
				return SW_ERROR;
			}
			step_info.step[step_info.curlevel - 1] = step_info.curmaxstep;
			step_info.curstep = step_info.step[step_info.curlevel - 1];
			sent->step = step_info.curstep;
			block_flag = SECTION_CMPBEG;
		}
		else if (strncmp(line, "COMP_END", 8) == 0)
		{
			if (block_flag != SECTION_CMPBEG)
			{
				pub_log_error("[%s][%d] COMP_END sent error![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent = &flow.work.sent[flow.work.count];
			flow.work.count++;
			sent->step = step_info.curstep;
			sent->type = SENT_TYPE_CMPEND;
			sent->line = lineno;
			step_info.curlevel--;
			if (step_info.curlevel == 0)
			{
				block_flag = SECTION_BEGIN;
			}
			else
			{
				step_info.curstep = step_info.step[step_info.curlevel - 1];
			}
		}
		else if (strncmp(line, "COND_BEGIN", 10) == 0)
		{
			if (cond_flag == 1)
			{
				pub_log_error("[%s][%d] Not allow cond nested!",
					__FILE__, __LINE__);
				fclose(fp);
				return SW_ERROR;
			}
			
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] COND_BEGIN sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CNDBEG;
			sent->line = lineno;
			step_info.curmaxstep++;
			step_info.curlevel++;
			step_info.curstep = step_info.curmaxstep;
			block_flag = SECTION_CMPBEG;
			sent->step = step_info.curstep;
			cond_flag = 1;
		}
		else if (strncmp(line, "COND_END", 8) == 0)
		{
			if (block_flag != SECTION_CMPBEG)
			{
				pub_log_error("[%s][%d] COMP_END sent error![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent = &flow.work.sent[flow.work.count];
			flow.work.count++;
			sent->type = SENT_TYPE_CNDEND;
			sent->line = lineno;
			cond_flag = 0;
			step_info.curlevel--;
			if (step_info.curlevel == 0)
			{
				block_flag = SECTION_BEGIN;
			}
			else
			{
				step_info.curstep = step_info.step[step_info.curlevel - 1];
			}
		}
		else if (strncmp(line, "EXIT", 4) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] EXIT sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_EXIT;
			sent->line = lineno;
			if (line[4] != '\0')
			{
				strcpy(sent->out, line + 5);
			}
		}
		else if (strncmp(line, "CONDITIONCOMPDO", 15) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] CONDITIONCOMPDO sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_COND;
			sent->line = lineno;
			strcpy(sent->express, line + 16);
			sent->step = 999;
		}
		else if (strncmp(line, "CONTINUE", 8) == 0)
		{
			if (block_flag == SECTION_DECLARE)
			{
				pub_log_error("[%s][%d] CONTINUE sentence could not"
					" in DECLARE section![%d]", __FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			else if (block_flag == SECTION_BEGIN || block_flag == SECTION_CMPBEG)
			{
				sent = &flow.work.sent[flow.work.count];
				flow.work.count++;
				if (block_flag == SECTION_CMPBEG)
				{
					sent->step = step_info.curstep;
				}
			}					
			else
			{
				pub_log_error("[%s][%d] Unknown sentence![%d]",
					__FILE__, __LINE__, lineno);
				fclose(fp);
				return SW_ERROR;
			}
			sent->type = SENT_TYPE_CONTINUE;
			sent->line = lineno;
		}

	}
	fclose(fp);
	pub_log_info("[%s][%d]precompiled success!work=[%d]decl=[%d]line=[%d]",
		__FILE__, __LINE__, flow.work.count, flow.decl.count, lineno);

	return SW_OK;
}

static int flow_do_declare()
{
	int	id = 0;
	TSENT	*sent;
	char	buf[1024];

	for (id = 0; id < flow.decl.count; id++)
	{
		sent = &flow.decl.sent[id];
		if (sent->type != SENT_TYPE_SET)
		{
			pub_log_error("[%s][%d] Must SET in declare section![%d][%s]",
				__FILE__, __LINE__, sent->line, sent->express);
			return SW_ERROR;
		}

		if (nComputeExpress(sent->express, strlen(sent->express), buf) < 0)
		{
			pub_log_error("[%s][%d] Compute exp error![%s][%d]",
				__FILE__, __LINE__, sent->express, sent->line);
			return SW_ERROR;
		}
		set_zd_data(sent->out, buf);
	}
	return SW_OK;
}

static int flow_before_work(sw_char_t *flow_name)
{
	int	result = 0;
	
	result = flow_init(flow_name);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] flow_init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	result = flow_do_declare();
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] Do declare error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	return SW_OK;
}

static int flow_read(char *filename)
{
	FILE	*fp = NULL;
	
	if (filename == NULL || filename[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}
	fread(&flow, sizeof(flow), 1, fp);
	fclose(fp);
	
	return SW_OK;
}

static int flow_write(char *filename)
{
	FILE	*fp = NULL;
	
	if (filename == NULL || filename[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open file[%s]! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return SW_ERROR;
	}
	fwrite(&flow, sizeof(flow), 1, fp);
	fclose(fp);
	
	return SW_OK;
}

int flow_prework(sw_char_t *flow_name)
{
	int	index = 0;
	int	result = 0;
	
	if (flow_name == NULL || flow_name[0] == '\0')
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (g_svr->reload == 0)
	{
		pub_log_info("[%s][%d] flow_cnt=[%d]", __FILE__, __LINE__, g_nflow->head.flow_cnt);
		for (index = 0; index < g_nflow->head.flow_cnt; index++)
		{
			if (strcmp(g_nflow->flow[index].name, flow_name) == 0)
			{
				memset(&flow, 0x0, sizeof(flow));
				memcpy(&flow, &g_nflow->flow[index].flow, sizeof(flow));
				pub_log_info("[%s][%d] index=[%d] name=[%s]",
					__FILE__, __LINE__, index, flow_name);
				return SW_OK;
			}
		}
		pub_log_info("[%s][%d] Not found flow [%s]'s info in cache!",
			__FILE__, __LINE__, flow_name);
	}
	result = flow_init(flow_name);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] flow_init error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	pub_log_info("[%s][%d] reload [%s] success!", __FILE__, __LINE__, flow_name);

	return SW_OK;
}

static int flow_get_cmpend(int *iId)
{
	int	next = 0;
	
	for (next = *iId; next < flow.work.count; next++)
	{
		if (flow.work.sent[next].type == SENT_TYPE_CMPEND && flow.work.sent[next + 1].step ==  flow.work.sent[*iId - 1].step)
		{
			*iId = next;
			return 0;
		}
	}
	
	return -1;
}

static int flow_get_cndend(int *id)
{
	int	next = 0;
	
	if (*id <= 0)
	{
		pub_log_error("[%s][%d] Param error! id=[%d]", __FILE__, __LINE__, *id);
		return SW_ERROR;
	}
	for (next = *id; next < flow.work.count; next++)
	{
		if (flow.work.sent[next].type == SENT_TYPE_CNDEND)
		{
			*id = next;
			return SW_OK;
		}
	}
	return SW_ERROR;
}

static int flow_get_errid(sw_char_t *errname, int id)
{
	int	flag = 0;
	int	next = 0;
	int	tag = 0;
		
	if (errname == NULL || errname[0] == '\0' || id <= 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	tag = 0;
	for (next = id; next < flow.work.count; next++)
	{
		if (flow.work.sent[next].type == SENT_TYPE_CMPBEG)
		{
			tag++;
		}
		
		if (flow.work.sent[next].type == SENT_TYPE_CMPEND)
		{
			tag--;			
		}
		if ((flow.work.sent[next].type == SENT_TYPE_CMPDO && strcmp(errname, flow.work.sent[next].out) == 0 && tag == 0) ||
			flow.work.sent[next].step == 0 ||
			(tag == 0 && (flow.work.sent[next].type == SENT_TYPE_DO	|| 
			flow.work.sent[next].type == SENT_TYPE_CALL || 
			flow.work.sent[next].type == SENT_TYPE_CALLLSN)))
		{
			if (flow.work.sent[next].type != SENT_TYPE_CMPDO)
			{
				flag = 1;
			}
			break;
		}
	}
	
	tag = 0;
	if (next >= flow.work.count || flag == 1)
	{
		for (next = id; next > 0; next--)
		{
			if (flow.work.sent[next].type == SENT_TYPE_CMPBEG)
			{
				tag++;
			}
			
			if (flow.work.sent[next].type == SENT_TYPE_CMPEND)
			{
				tag--;			
			}
			
			if ((flow.work.sent[next].type == SENT_TYPE_CMPDO && strcmp(errname, flow.work.sent[next].out) == 0 && tag == 0) ||
				flow.work.sent[next].step == 0 ||
				(tag == 0 && (flow.work.sent[next].type == SENT_TYPE_DO	|| 
				flow.work.sent[next].type == SENT_TYPE_CALL || 
				flow.work.sent[next].type == SENT_TYPE_CALLLSN)))
			{
				break;
			}
		}
	}
	
	if (flow.work.sent[next].type == SENT_TYPE_CMPDO && strcmp(errname, flow.work.sent[next].out) == 0)
	{
		pub_log_info("[%s][%d] step [%s]", __FILE__, __LINE__, flow.work.sent[next].out);
		return next;
	}
	else
	{
		pub_log_error("[%s][%d] Can not find [%s]!", __FILE__, __LINE__, errname);
		return -2;
	}
}

static int flow_set_nextid(int errcode, int *id)
{
	int	i = 0;
	int	result = 0;
	int	hashid = 0;
	sw_char_t	*ptr = NULL;
	sw_char_t	name[64];
	sw_char_t	value[64];
	
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	
	sprintf(value, "%d", errcode);
	hashid = 0;
	ptr = value;
	while (*ptr != '\0')
	{
		hashid += (unsigned char)*ptr;
		ptr++;
	}
	i = 0;
	hashid = hashid % MAX_CMP_LEVEL;
	while (strcmp(g_cmp_info[hashid].err_code, value) != 0)
	{
		i++;
		hashid++;
		if (hashid == MAX_CMP_LEVEL)
		{
			hashid = 0;
		}

		if (i == MAX_CMP_LEVEL)
		{
			pub_log_error("[%s][%d] Can not find [%s]!", __FILE__, __LINE__, value);
			return SW_ERROR;
		}
	}
	pub_log_info("[%s][%d] ERRCODE=[%s] ERRNAME=[%s]", 
		__FILE__, __LINE__, g_cmp_info[hashid].err_code, g_cmp_info[hashid].err_name);
	result = flow_get_errid(g_cmp_info[hashid].err_name, *id);
	if (result == SW_ERROR)
	{
		pub_log_error("[%s][%d] flow_get_errid error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	else if (result == -2)
	{
		if (strcmp(g_cmp_info[hashid].err_name, g_cmp_info[hashid].up_name) == 0)
		{
			pub_log_error("[%s][%d] ERR_NAME=[%s] UP_NAME=[%s] NOT FOUND!",
				__FILE__, __LINE__, g_cmp_info[hashid].err_name, g_cmp_info[hashid].up_name);
			return SW_ERROR;
		}
		while (g_cmp_info[hashid].up_name[0] != '\0')
		{
			memset(value, 0x0, sizeof(value));
			strncpy(value, g_cmp_info[hashid].up_name, sizeof(value) - 1);
			ptr = value;
			hashid = 0;
			while (*ptr != '\0')
			{
				hashid += (unsigned char)*ptr;
				ptr++;
			}
			i = 0;
			hashid = hashid % MAX_CMP_LEVEL;
			while (strcmp(g_cmp_info[hashid].err_name, value) != 0)
			{
				i++;
				hashid++;
				if (hashid == MAX_CMP_LEVEL)
				{
					hashid = 0;
				}

				if (i == MAX_CMP_LEVEL)
				{
					pub_log_error("[%s][%d] Can not find [%s]!", __FILE__, __LINE__, value);
					return SW_ERROR;
				}
			}
			result = flow_get_errid(g_cmp_info[hashid].err_name, *id);
			if (result == SW_ERROR)
			{
				pub_log_error("[%s][%d] flow_get_errid error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			else if (result == -2)
			{
				if (strcmp(g_cmp_info[hashid].err_name, g_cmp_info[hashid].up_name) == 0)
				{
					pub_log_error("[%s][%d] ERR_NAME=[%s] UP_NAME=[%s] NOT FOUND!",
						__FILE__, __LINE__, g_cmp_info[hashid].err_name, g_cmp_info[hashid].up_name);
					return SW_ERROR;
				}
				pub_log_info("[%s][%d] [%s] not found, continue!", 
					__FILE__, __LINE__, g_cmp_info[hashid].err_name);
				continue;
			}
			else
			{
				*id = result;
				pub_log_info("[%s][%d] find id=[%d]", __FILE__, __LINE__, result);
				return SW_OK;
			}
		}
	}
	else
	{
		*id = result;
		pub_log_info("[%s][%d] find id=[%d]", __FILE__, __LINE__, result);
	}
	
	return SW_OK;
}

int get_fun_name(char *exp, char *lib, char *fun)
{
	int	quto = 0;
	char	*p = NULL;
	char	*q = NULL;
	char	*ptr = exp;
	char	buf[128];
	char	libso[128];
	char	funname[64];
	
	memset(libso, 0x0, sizeof(libso));
	memset(funname, 0x0, sizeof(funname));
	p = libso;
	q = funname;
		
	while (*ptr != '\0' && *ptr != ')')
	{
		if (*ptr == '(')
		{
			quto = 1;
			ptr++;
			continue;
		}
		
		if (quto == 0)
		{
			*p++ = *ptr++;
		}
		else
		{
			*q++ = *ptr++;
		}
	}
	*p = '\0';
	*q = '\0';

	if (libso[0] == '#' || libso[0] == '$')
	{
		memset(buf, 0x0, sizeof(buf));
		get_zd_data(libso, buf);
		memset(libso, 0x0, sizeof(libso));
		strncpy(libso, buf, sizeof(libso) - 1);
	}
	
	if (funname[0] == '#' || funname[0] == '$')
	{
		memset(buf, 0x0, sizeof(buf));
		get_zd_data(funname, buf);
		memset(funname, 0x0, sizeof(funname));
		strncpy(funname, buf, sizeof(funname) - 1);
	}
	
	strcpy(lib, libso);
	strcpy(fun, funname);
	
	return 0;
}

static int flow_get_userlib(char *tx_name, char *libname)
{
	int	ret = 0;
	sw_char_t	subprdt[32];
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	
	memset(subprdt, 0x0, sizeof(subprdt));
	memset(xmlname, 0x0, sizeof(xmlname));

	get_zd_data("$subprdt", subprdt);
	if (subprdt[0] == '\0')
	{
		pub_log_debug("[%s][%d] 子产品为空,直接默认!", __FILE__, __LINE__);
		return SW_ENOENT;
	}
	pub_log_debug("[%s][%d] subprdt=[%s]", __FILE__, __LINE__, subprdt);
	
	sprintf(xmlname, "%s/prdtlib.xml", g_svr->flow_path);
	pub_log_info("[%s][%d] xmlname=[%s]", __FILE__, __LINE__, xmlname);
	ret = access(xmlname, F_OK);
	if (ret < 0)
	{
		pub_log_info("[%s][%d] 用户自定义库文件不存在,直接默认!", __FILE__, __LINE__);
		return SW_ENOENT;
	}

	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] 根据XML文件[%s]建树失败!", __FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}
	
	
	node = pub_xml_locnode(xml, ".PRDTLIB.SUBPRDT");
	while (node != NULL)
	{
		if (strcmp(node->name, "SUBPRDT") != 0)
		{
			node = node->next;
			continue;
		}
	
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] 未配置NAME!", __FILE__, __LINE__);
			return SW_ERROR;
		}
	
		if (strcmp(node1->value, subprdt) == 0)
		{
			break;
		}
		
		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_debug("[%s][%d] 未找到[%s]的配置,直接默认!", __FILE__, __LINE__, subprdt);
		return SW_ENOENT;
	}
	
	node = pub_xml_locnode(xml, "ITEM");
	while (node != NULL)
	{
		if (strcmp(node->name, "ITEM") == 0)
		{
			node = node->next;
			continue;
		}
		
		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_xml_deltree(xml);
			pub_log_error("[%s][%d] 未配置NAME!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		
		if (strcmp(node1->value, tx_name) == 0)
		{
			break;
		}

		node = node->next;
	}
	if (node == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_debug("[%s][%d] 未找到[%s]的用户自定义接口[%s]的配置,直接取默认!", 
			__FILE__, __LINE__, subprdt, tx_name);
		return SW_ENOENT;
	}
	
	node = pub_xml_locnode(xml, "LIB");
	if (node == NULL || node->value == NULL)
	{
		pub_xml_deltree(xml);
		pub_log_error("[%s][%d] 未配置LIB!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	strcpy(libname, node->value);
	pub_log_debug("[%s][%d] subprdt=[%s] tx_name=[%s] LIBNAME=[%s]", __FILE__, __LINE__, subprdt, tx_name, libname);
	
	pub_xml_deltree(xml);
	
	return SW_OK;
}

static int replace_var(char *str, char *sstr, char *rstr)
{
	int     len = 0;
	int     len1 = 0;
	int     len2 = 0;
	int     len3 = 0;
	int     offset = 0;
	char    *ptr = NULL;
	char    savestr[1024];

	memset(savestr, 0x0, sizeof(savestr));
	strcpy(savestr, str);
	len1 = strlen(sstr);
	len2 = strlen(rstr);
	while ((ptr = strstr(str, sstr)) != NULL)
	{
		offset = ptr - str;
		len3 = strlen(ptr) - len1;
		len = offset + len2 + len3;
		strncpy(str + offset, rstr, len2);
		strncpy(str + offset + len2, savestr + offset + len1, len3);
		str[len] = '\0';
	}

	return 0;
}

static int reset_errcode()
{
	set_zd_data("$errcode", "0000");
	
	return 0;
}

int flow_work(int startline)
{
	int	i = 0;
	int	j = 0;
	int	id = 0;
	int	len = 0;
	int	next = 0;
	int	result = 0;
	int	err_flag = 0;
	int	should_goto = 0;
	int	(*dlfun)();
	char	*ptr = NULL;
	void	*handle = NULL;
	TSENT	*sent = NULL;
	sw_char_t	buf[1024];
	sw_char_t	libso[128];
	sw_char_t	userlib[128];
	sw_char_t	tx_name[128];
	sw_char_t	fun_name[128];
	sw_char_t	lib_name[128];
	sw_char_t	svr_name[128];
	sw_char_t	svc_name[128];
	sw_char_t	lsn_name[128];
	sw_char_t	errcode[32];
	sw_char_t	errcode_bak[32];
	sw_char_t	check_value[1024];
	
	memset(buf, 0x0, sizeof(buf));
	memset(libso, 0x0, sizeof(libso));
	memset(userlib, 0x0, sizeof(userlib));
	memset(tx_name, 0x0, sizeof(tx_name));
	memset(fun_name, 0x0, sizeof(fun_name));
	memset(lib_name, 0x0, sizeof(lib_name));
	memset(svr_name, 0x0, sizeof(svr_name));
	memset(svc_name, 0x0, sizeof(svc_name));
	memset(lsn_name, 0x0, sizeof(lsn_name));
	memset(errcode, 0x0, sizeof(errcode));
	memset(check_value, 0x0, sizeof(check_value));
	
	if (startline == 0)
	{
		result = flow_do_declare();
		if (result != SW_OK)
		{
			pub_log_error("[%s][%d] flow_do_declare error!", __FILE__, __LINE__);
			return SW_OK;
		}
		pub_log_info("[%s][%d] flow_do_declare success!", __FILE__, __LINE__);
	}
	
	if (startline > 0)
	{
		memset(errcode, 0x0, sizeof(errcode));
		get_zd_data("$errcode", errcode);
		if (atoi(errcode) != 0)
		{
			pub_log_info("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);
			if (flow.work.sent[startline].type != SENT_TYPE_CMPDO)
			{
				pub_log_info("[%s][%d] Next line=[%d] type=[%d]",
					__FILE__, __LINE__, startline, flow.work.sent[startline].type);
				pub_log_error("[%s][%d] Exec flow error! errcode=[%s]", __FILE__, __LINE__, errcode);
				return SW_ERROR;
			}
		}
	}
	
	for (id = startline; id < flow.work.count; id++)
	{
		sent = &flow.work.sent[id];
		switch (sent->type)
		{
		case SENT_TYPE_SET:
			if (nComputeExpress(sent->express, strlen(sent->express), buf) < 0)
			{
				pub_log_error("[%s][%d] [%d] set error![%s][%s]",
					__FILE__, __LINE__, sent->line, sent->out, sent->express);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] SET [%s]=[%s]",
				__FILE__, __LINE__, sent->out, buf);
			set_zd_data(sent->out, buf);
			break;

		case SENT_TYPE_DO:
			memset(buf, 0x0, sizeof(buf));
			memset(libso, 0x0, sizeof(libso));
			memset(tx_name, 0x0, sizeof(tx_name));
			i = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == ' ')
				{
					break;
				}
				tx_name[i] = sent->express[i];
				i++;
			}
			
			ptr = strchr(tx_name, '(');
			if (ptr != NULL)
			{
				memset(fun_name, 0x0, sizeof(fun_name));
				memset(lib_name, 0x0, sizeof(lib_name));
				get_fun_name(tx_name, lib_name, fun_name);
				memset(tx_name, 0x0, sizeof(tx_name));
				strncpy(tx_name, fun_name, sizeof(tx_name) - 1);
				sprintf(libso, "%s/lib/%s.so", g_svr->work_path, lib_name);
			}
			else
			{
				if (pub_str_rstrncmp(tx_name, "[USERLIB]", 9) == 0)
				{
					len = strlen(tx_name);
					tx_name[len - 9] = '\0';
					memset(userlib, 0x0, sizeof(userlib));
					result = flow_get_userlib(tx_name, userlib);
					if (result != SW_OK)
					{
						userlib[0] = '\0';
					}
				}
				
				ptr = strstr(tx_name, FLOW_EXP);
				if (ptr != NULL)
				{
					memset(buf, 0x0, sizeof(buf));
					get_zd_data(FLOW_VAR, buf);
					pub_log_info("[%s][%d] Before replace, tx_name=[%s] flowcode=[%s]",
						__FILE__, __LINE__, tx_name, buf);
					replace_var(tx_name, FLOW_EXP, buf);
					pub_log_info("[%s][%d] After replace, tx_name=[%s]", __FILE__, __LINE__, tx_name);
				}

				if (tx_name[0] == '#' || tx_name[0] == '$')
				{
					memset(buf, 0x0, sizeof(buf));
					strcpy(buf, tx_name);
					memset(tx_name, 0x0, sizeof(tx_name));
					get_zd_data(buf, tx_name);
				}
				
				if (userlib[0] != '\0')
				{
					sprintf(libso, "%s/lib/%s.so", g_svr->work_path, userlib);
				}
				else
				{
					char	tmp[128];
					char	subprdt[64];
					
					memset(tmp, 0x0, sizeof(tmp));
					memset(subprdt, 0x0, sizeof(subprdt));
					get_zd_data("$subprdt", subprdt);	
					pub_log_info("[%s][%d] subprdt=[%s]", __FILE__, __LINE__, subprdt);
					
					if (strlen(g_svc) > 0)
					{
						ptr = strstr(g_svc, FLOW_EXP);
						if (ptr != NULL)
						{
							memset(buf, 0x0, sizeof(buf));
							get_zd_data(FLOW_VAR, buf);
							pub_log_info("[%s][%d] Before replace, flow=[%s] flowcode=[%s]",
								__FILE__, __LINE__, g_svc, buf);
							replace_var(g_svc, FLOW_EXP, buf);
							pub_log_info("[%s][%d] After replace, flow=[%s]", __FILE__, __LINE__, g_svc);
						}
						
						sprintf(tmp, "%s.so", g_svc);
					}
					else
					{
						sprintf(tmp, "flw_%s.so", tx_name);
					}
					
					memset(libso, 0x0, sizeof(libso));
					sprintf(libso, "%s/lib/%s/%s", g_svr->work_path, subprdt, tmp);
					if (subprdt[0] == '\0' || !pub_file_exist(libso))
					{
						memset(libso, 0x0, sizeof(libso));
						sprintf(libso, "%s/lib/%s", g_svr->work_path, tmp);
					}
					pub_log_info("[%s][%d] libso=[%s]", __FILE__, __LINE__, libso);
				}
			}
	
			if (g_svr->use_dlcache == 1)
			{
				dlfun = dlcache_get_dlfun_by_name(libso, tx_name);
				if (dlfun == NULL)
				{
					pub_log_error("[%s][%d] dlcache get dlfun [%s][%s] error!",
						__FILE__, __LINE__, libso, tx_name);
					return SW_ERROR;
				}
			}
			else
			{
				handle = (void *)dlopen(libso, RTLD_NOW | RTLD_GLOBAL);
				if (handle == NULL)
				{
					pub_log_error("[%s][%d] dlopen[%s] error! [%s]",
						__FILE__, __LINE__, libso, dlerror());
					return SW_ERROR;
				}
				
				dlfun = (int (*)())dlsym(handle, tx_name);
				if (dlfun == NULL)
				{
					dlclose(handle);
					pub_log_error("[%s][%d] dlsym[%s][%s] error![%s]",
						__FILE__, __LINE__, libso, tx_name, dlerror());
					return SW_ERROR;
				}
			}
			
			memset(errcode_bak, 0x0, sizeof(errcode_bak));
			get_zd_data("$errcode", errcode_bak);
			result = dlfun();
			if (result == SW_OK)
			{
				err_flag = 0;
				if (g_svr->use_dlcache != 1)
				{
					dlclose(handle);
				}
				pub_log_info("[%s][%d] DO [%s] success!",
					__FILE__, __LINE__, tx_name);
				if (flow.work.sent[id + 1].type == SENT_TYPE_CMPDO)
				{
					reset_errcode();
				}
				
				break;
			}
			if (g_svr->use_dlcache != 1)
			{
				dlclose(handle);
			}
			pub_log_error("[%s][%d] DO [%s] error!", __FILE__, __LINE__, tx_name);
			memset(errcode, 0x0, sizeof(errcode));
			get_zd_data("$errcode", errcode);
			if (strcmp(errcode, errcode_bak) == 0)
			{
				memset(errcode, 0x0, sizeof(errcode));
				sprintf(errcode, "%04d", ERR_DOERR);
				set_zd_data("$errcode", errcode);
				set_zd_data("$errcode1", errcode);
			}
				
			if (flow.work.sent[id + 1].type == SENT_TYPE_CMPDO)
			{
				err_flag = 1;
				break;
			}
			else
			{
				pub_log_error("[%s][%d] DO [%s] error!",
					__FILE__, __LINE__, tx_name);
				return SW_ERROR;
			}
		
		case SENT_TYPE_TUXC:
			reset_errcode();
			memset(buf, 0x0, sizeof(buf));
			memset(libso, 0x0, sizeof(libso));
			memset(tx_name, 0x0, sizeof(tx_name));
			i = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == ' ')
				{
					break;
				}
				tx_name[i] = sent->express[i];
				i++;
			}
			
			if (tx_name[0] == '#' || tx_name[0] == '$')
			{
				memset(buf, 0x0, sizeof(buf));
				strcpy(buf, tx_name);
				memset(tx_name, 0x0, sizeof(tx_name));
				get_zd_data(buf, tx_name);
			}
			
			sprintf(libso, "%s/plugin/%s.so", g_svr->work_path, tx_name);
			handle = (void *)dlopen(libso, RTLD_NOW | RTLD_GLOBAL);
			if (handle == NULL)
			{
				pub_log_error("[%s][%d] dlopen[%s] error! [%s]",
					__FILE__, __LINE__, libso, dlerror());
				return SW_ERROR;
			}
			
			dlfun = (int (*)())dlsym(handle, tx_name);
			if (dlfun == NULL)
			{
				dlclose(handle);
				pub_log_error("[%s][%d] dlsym[%s][%s] error![%s]",
					__FILE__, __LINE__, libso, tx_name, dlerror());
				return SW_ERROR;
			}
			
			result = dlfun();
			if (result == SW_OK)
			{
				err_flag = 0;
				dlclose(handle);
				pub_log_info("[%s][%d] TUXCALL [%s] success!",
					__FILE__, __LINE__, tx_name);
				break;
			}
			dlclose(handle);
			pub_log_error("[%s][%d] TUXCALL [%s] error!",
				__FILE__, __LINE__, tx_name);
			if (flow.work.sent[id + 1].type == SENT_TYPE_CMPDO)
			{
				err_flag = 1;
				break;
			}
			else
			{
				pub_log_error("[%s][%d] TUXCALL [%s] error!",
					__FILE__, __LINE__, tx_name);
				return SW_ERROR;
			}

		case SENT_TYPE_CALL:	
			reset_errcode();
			memset(svr_name, 0x0, sizeof(svr_name));
			memset(svc_name, 0x0, sizeof(svc_name));
			i = 0;
			j = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == '(')
				{
					break;
				}
				svr_name[i] = sent->express[i];
				i++;
			}
			i++;
			j = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == ')')
				{
					break;
				}
				svc_name[j] = sent->express[i];
				j++;
				i++;
			}
			if (sent->express[i] != ')')
			{
				pub_log_error("[%s][%d] Express[%s] error! SVR(SVC)",
					__FILE__, __LINE__, sent->express);
				return SW_ERROR;
			}
			
			pub_log_info("[%s][%d] CALL [%s(%s)]",
				__FILE__, __LINE__, svr_name, svc_name);
			
			result = flow_call(svr_name, svc_name, id);
			if (result == SW_OK)
			{
				err_flag = 0;
				return SW_OK;
			}
			
			if (flow.work.sent[id + 1].type == SENT_TYPE_CMPDO)
			{
				err_flag = 1;
				break;
			}
			else
			{
				pub_log_error("[%s][%d] CALL [%s(%s)] error!",
					__FILE__, __LINE__, svr_name, svc_name);
				return SW_ERROR;
			}
		
		case SENT_TYPE_LINK:
			reset_errcode();
			memset(svr_name, 0x0, sizeof(svr_name));
			memset(svc_name, 0x0, sizeof(svc_name));
			i = 0;
			j = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == '(')
				{
					break;
				}
				svr_name[i] = sent->express[i];
				i++;
			}
			i++;
			j = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == ')')
				{
					break;
				}
				svc_name[j] = sent->express[i];
				j++;
				i++;
			}
			if (sent->express[i] != ')')
			{
				pub_log_error("[%s][%d] Express[%s] error! SVR(SVC)",
					__FILE__, __LINE__, sent->express);
				return SW_ERROR;
			}
			
			pub_log_info("[%s][%d] LINK [%s(%s)]",
				__FILE__, __LINE__, svr_name, svc_name);
			
			result = flow_link(svr_name, svc_name);
			if (result == SW_OK)
			{
				err_flag = 0;
				return SW_OK;
			}
			else
			{
				pub_log_error("[%s][%d] LINK[%s(%s)]error!",
					__FILE__, __LINE__, svr_name, svc_name);
				return SW_ERROR;
			}

		case SENT_TYPE_CALLLSN:	
			reset_errcode();
			memset(lsn_name, 0x0, sizeof(lsn_name));
			if (strlen(sent->express) == 0)
			{
				pub_log_error("[%s][%d] Express error!",
					__FILE__, __LINE__);
				return SW_ERROR;
			}

			strncpy(lsn_name, sent->express, sizeof(lsn_name) - 1);
			pub_log_info("[%s][%d] CALLLSN [%s][%s]",
				__FILE__, __LINE__, sent->express, lsn_name);
			result = flow_calllsn(lsn_name, id);
			if (result == SW_OK)
			{
				return SW_OK;
			}
			
			if (flow.work.sent[id + 1].type == SENT_TYPE_CMPDO)
			{
				err_flag = 1;
				break;
			}
			else
			{
				pub_log_error("[%s][%d] CALLLSN [%s] error!",
					__FILE__, __LINE__, lsn_name);
				return SW_ERROR;
			}
		
		case SENT_TYPE_LINKLSN:
			reset_errcode();
			memset(lsn_name, 0x0, sizeof(lsn_name));
			if (strlen(sent->express) == 0)
			{
				pub_log_error("[%s][%d] Express error!",
					__FILE__, __LINE__);
				return SW_ERROR;
			}
			
			if (sent->express[0] == '#' || sent->express[0] == '$')
			{
				get_zd_data(sent->express, lsn_name);
			}
			else
			{
				strncpy(lsn_name, sent->express, sizeof(lsn_name) - 1);
			}
			pub_log_info("[%s][%d] LINKLSN [%s][%s]",
				__FILE__, __LINE__, sent->express, lsn_name);
			result = flow_linklsn(lsn_name);
			if (result == SW_OK)
			{
				return SW_OK;
			}
			else
			{
				pub_log_error("[%s][%d] LINKLSN [%s] error!",
					__FILE__, __LINE__, lsn_name);
				return SW_ERROR;
			}
		
		case SENT_TYPE_POSTLSN:
			reset_errcode();
			memset(lsn_name, 0x0, sizeof(lsn_name));
			if (strlen(sent->express) == 0)
			{
				pub_log_error("[%s][%d] Express error!",
					__FILE__, __LINE__);
				return SW_ERROR;
			}
			
			if (sent->express[0] == '#' || sent->express[0] == '$')
			{
				get_zd_data(sent->express, lsn_name);
			}
			else
			{
				strncpy(lsn_name, sent->express, sizeof(lsn_name) - 1);
			}
			pub_log_info("[%s][%d] POSTLSN [%s][%s]",
				__FILE__, __LINE__, sent->express, lsn_name);
			result = flow_postlsn(lsn_name);
			if (result != SW_OK)
			{
				pub_log_error("[%s][%d] POSTLSN [%s] error!",
					__FILE__, __LINE__, lsn_name);
				return SW_ERROR;
			}
			break;

		case SENT_TYPE_POSTSVC:	
			reset_errcode();
			memset(svr_name, 0x0, sizeof(svr_name));
			memset(svc_name, 0x0, sizeof(svc_name));
			i = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == '(')
				{
					break;
				}
				svr_name[i] = sent->express[i];
				i++;
			}
			i++;
			j = 0;
			while (sent->express[i] != '\0')
			{
				if (sent->express[i] == ')')
				{
					break;
				}
				svc_name[j] = sent->express[i];
				j++;
				i++;
			}
			if (sent->express[i] != ')')
			{
				pub_log_error("[%s][%d] expres [%s] error!",
					__FILE__, __LINE__, sent->express);
				return SW_ERROR;
			}
			
			result = flow_postsvc(svr_name, svc_name);
			if (result != SW_OK)
			{
				pub_log_error("[%s][%d] POSTSVC [%s(%s)] error!", 
					__FILE__, __LINE__, svr_name, svc_name);
				return SW_ERROR;
			}
			break;

		case SENT_TYPE_CHECK:
			memset(check_value, 0x0, sizeof(check_value));
			if (nComputeExpress(sent->express, strlen(sent->express), check_value) < 0)
			{
				pub_log_error("[s][%d] compute [%s] error!",
					__FILE__, __LINE__, sent->express);
				return SW_ERROR;
			}
			break;
		
		case SENT_TYPE_GOTO:
			should_goto = 0;
			if (strlen(sent->express) == 0)
			{
				should_goto = 1;
			}
			else
			{
				memset(buf, 0x0, sizeof(buf));
				if (nComputeExpress(sent->express, strlen(sent->express), buf) < 0)
				{
					pub_log_error("[%s][%d]express[%s] "
						"error", __FILE__, __LINE__, sent->express);
					return SW_ERROR;
				}
				if (nExtendEqual(check_value, buf))
				{
					should_goto = 1;
				}
			}
			
			if (should_goto == 0)
			{
				break;
			}
			
			for (next = id; next < flow.work.count; next++)
			{
				if (flow.work.sent[next].type == SENT_TYPE_LABEL &&
					strcmp(flow.work.sent[next].express, sent->out) == 0)
				{
					break;
				}
			}
			if (next >= flow.work.count)
			{
				for (next = 0; next < id; next++)
				{
					if (flow.work.sent[next].type == SENT_TYPE_LABEL &&
						strcmp(flow.work.sent[next].express, sent->out) == 0)
					{
						break;
					}
				}
			}
			if (strcmp(flow.work.sent[next].express, sent->out) == 0)
			{
				pub_log_info("[%s][%d] GOTO [%s]", __FILE__, __LINE__, sent->out);
				id = next - 1;
				continue;
			}
			else
			{
				pub_log_error("[%s][%d] Can not goto step [%s]!", __FILE__, __LINE__, sent->out);
				return SW_ERROR;
			}
		
		case SENT_TYPE_LABEL:
			continue;

		case SENT_TYPE_CMPDO:
			memset(errcode, 0x0, sizeof(errcode));
			get_zd_data("$errcode", errcode);
			pub_log_info("[%s][%d] errcode=[%s]", __FILE__, __LINE__, errcode);
			if (err_flag == 0 && strcmp(errcode, "0000") == 0)
			{
				result = flow_get_cmpend(&id);
				if (result != SW_OK)
				{
					pub_log_error("[%s][%d] get cmpend error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				break;
			}
			
			result = flow_set_nextid(atoi(errcode), &id);
			if (result == SW_ERROR)
			{
				pub_log_error("[%s][%d] flow_set_nextid error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			else if (result == -2)
			{
				pub_log_error("[%s][%d] Can not find [%s]!", __FILE__, __LINE__, errcode);
				return SW_ERROR;
			}
			/***
			set_zd_data("$errcode", "0000");
			set_zd_data("$errcode1", "0000");
			***/
			break;

		case SENT_TYPE_CMPEND:
			result = flow_get_cmpend(&id);
			if (result != SW_OK)
			{
				pub_log_error("[%s][%d] get cmpend error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			break;
		
		case SENT_TYPE_EXIT:
			if (sent->out[0] != '\0' && sent->out [0] == '0')
			{
				pub_log_info("[%s][%d] Normal exit!", __FILE__, __LINE__);
				return SW_OK;
			}
			else
			{
				pub_log_info("[%s][%d] Faild exit!", __FILE__, __LINE__);
				return SW_ERROR;
			}

		case SENT_TYPE_CMPBEG:
			break;
		
		case SENT_TYPE_COND:
			memset(check_value, 0x0, sizeof(check_value));
			if (nComputeExpress(sent->express, strlen(sent->express), check_value) < 0)
			{
				pub_log_error("[%s][%d] comput express [%s] error!", __FILE__, __LINE__, sent->express);
				return SW_ERROR;
			}

			if (atoi(check_value) == 0)
			{
				result = flow_get_cndend(&id);
				if (result != SW_OK)
				{
					pub_log_error("[%s][%d] get cndend error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
			}
			break;
		
		case SENT_TYPE_CNDBEG:
		case SENT_TYPE_CNDEND:
			break;
		
		case SENT_TYPE_CONTINUE:
			result = flow_continue(id);
			if (result == SW_ERROR)
			{
				pub_log_error("[%s][%d] flow_continue error!", __FILE__, __LINE__);
				return SW_ERROR;
			}
			else if (result == -2)
			{
				result = flow_get_cmpend(&id);
				if (result != SW_OK)
				{
					pub_log_error("[%s][%d] flow_get_cmpend error!", __FILE__, __LINE__);
					return SW_ERROR;
				}
				continue;
			}
			id = result;
			break;
		default:
			pub_log_error("[%s][%d] [%d] invaild express! [%d][%s]",
				__FILE__, __LINE__, sent->line, sent->type, sent->express);
			return SW_ERROR;
		}
	}
	if (id == flow.work.count)
	{
		g_complete = 1;
	}
	
	return 0;
}

static int flow_call(sw_char_t *svr_name, sw_char_t *svc_name, int id)
{
	if (svr_name == NULL || svc_name == NULL || svr_name[0] == '\0' || svc_name[0] == '\0' || id < 0)
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	g_cmd.type = SW_CALLREQ;
	g_cmd.msg_type = SW_MSG_REQ;
	g_cmd.task_flg = SW_STORE;
	g_cmd.dst_type = ND_SVC;
	strncpy(g_cmd.ori_svr, g_cmd.dst_svr, sizeof(g_cmd.ori_svr) - 1);
	strncpy(g_cmd.ori_svc, g_cmd.dst_svc, sizeof(g_cmd.ori_svc) - 1);
	strncpy(g_cmd.ori_prdt, g_prdt, sizeof(g_cmd.ori_prdt) - 1);
	strncpy(g_cmd.ori_def, g_cmd.def_name, sizeof(g_cmd.ori_def) - 1);
	strncpy(g_cmd.dst_svr, svr_name, sizeof(g_cmd.dst_svr) - 1);
	strncpy(g_cmd.def_name, svc_name, sizeof(g_cmd.def_name) - 1);
	g_cmd.start_line = id + 1;
	g_cmd.dst_prdt[0] = '\0';

	return SW_OK;
}

static int flow_link(sw_char_t *svr_name, sw_char_t *svc_name)
{
	if (svr_name == NULL || svc_name == NULL || svr_name[0] == '\0' || svc_name[0] == '\0')
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	g_cmd.type = SW_LINKREQ;
	g_cmd.msg_type = SW_MSG_REQ;
	g_cmd.task_flg = SW_FORGET;
	g_cmd.dst_type = ND_SVC;
	strncpy(g_cmd.ori_svr, g_cmd.dst_svr, sizeof(g_cmd.ori_svr) - 1);
	strncpy(g_cmd.ori_svc, g_cmd.dst_svc, sizeof(g_cmd.ori_svc) - 1);
	strncpy(g_cmd.ori_prdt, g_cmd.dst_prdt, sizeof(g_cmd.ori_prdt) - 1);
	strncpy(g_cmd.dst_svr, svr_name, sizeof(g_cmd.dst_svr) - 1);
	strncpy(g_cmd.def_name, svc_name, sizeof(g_cmd.def_name) - 1);
	pub_log_info("[%s][%d] flow_link SVR/SVC=[%s(%s)]", __FILE__, __LINE__, svr_name, svc_name);
	
	return SW_OK;
}

static int flow_postsvc(sw_char_t *svr_name, sw_char_t *svc_name)
{
	int	result = 0;
	sw_cmd_t	cmd;
	sw_loc_vars_t	*vars;
	sw_loc_vars_t	new_vars;

	if (svr_name == NULL || svc_name == NULL || svr_name[0] == '\0' || svc_name[0] == '\0')
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(&cmd, 0x0, sizeof(cmd));
	memcpy(&cmd, &g_cmd, sizeof(cmd));
	cmd.msg_type = SW_MSG_REQ;
	cmd.task_flg = SW_DEL;
	cmd.dst_type = ND_SVC;
	strncpy(cmd.ori_svr, g_cmd.dst_svr, sizeof(cmd.ori_svr) - 1);
	strncpy(cmd.ori_svc, g_cmd.dst_svc, sizeof(cmd.ori_svc) - 1);
	strncpy(cmd.ori_prdt, g_cmd.dst_prdt, sizeof(cmd.ori_prdt) - 1);
	strncpy(cmd.dst_svr, svr_name, sizeof(cmd.dst_svr) - 1);
	strncpy(cmd.def_name, svc_name, sizeof(cmd.def_name) - 1);
	cmd.mtype = mtype_new();
	if (cmd.mtype <= 0)
	{
		pub_log_error("[%s][%d] create mtype error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	result = pub_loc_vars_alloc(&new_vars, SHM_VARS);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_alloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	vars = pub_get_global_vars();
	result = vars->clone(vars, cmd.mtype);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, vars.clone error, vid[%ld]." ,__FILE__,__LINE__, cmd.mtype);
		return SW_ERROR;
	}

	result = new_vars.create(&new_vars, cmd.mtype);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, new_vars.create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	cmd.trace_no = seqs_new_trace_no();
	if (cmd.trace_no <= 0)
	{
		pub_log_error("[%s][%d] create trace_no error!", __FILE__, __LINE__);
		mtype_delete(cmd.mtype, 1);
		return SW_ERROR;
	}
	
	result = seqs_get_sysdate(cmd.sys_date);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] get sysdate error!", __FILE__, __LINE__);
		mtype_delete(cmd.mtype, 1);
		return SW_ERROR;
	}
	cmd.type = SW_POSTREQ;
	cmd_print(&cmd);
	result = trace_create_info(&cmd);
	if (result < 0)
	{
		pub_log_error("[%s][%d] Crate trace info error!", __FILE__, __LINE__);
		mtype_delete(cmd.mtype, 1);
		return SW_ERROR;
	}
	route_free(&new_vars);
	result = route_snd_dst(&new_vars, g_path, &cmd);
	if (result == SW_ERROR)
	{
		pub_log_error("[%s][%d] send cmd error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	new_vars.free_mem(&new_vars);
	
	return SW_OK;
}

int get_timeout(char *lsn_name, int *timeout)
{
	int	len = 0;
	char	*ptr = NULL;
	char	*tmp = NULL;
	char	buf[64];

	memset(buf, 0x0, sizeof(buf));
	ptr = lsn_name;
	while (*ptr != '(' && *ptr != '\0')
	{
		ptr++;
	}

	if (*ptr == '\0')
	{
		*timeout = 0;
		if (lsn_name[0] == '#' || lsn_name[0] == '$')
		{
			memset(buf, 0x0, sizeof(buf));
			get_zd_data(lsn_name, buf);
			len = strlen(buf);
			strncpy(lsn_name, buf, len);
			lsn_name[len] = '\0';
			pub_log_info("[%s][%d] lsn_name=[%s]", __FILE__, __LINE__, lsn_name);
		}
		return 0;
	}

	*ptr = '\0';
	ptr++;
	tmp = buf;
	while (*ptr != ')' && *ptr != '\0')
	{
		if (*ptr >= '0' && *ptr <= '9')
		{
			*tmp++ = *ptr++;
			continue;
		}
		break;
	}
	*tmp = '\0';
	if (buf[0] == '#' || buf[0] == '$')
	{
		char	tbuf[128];

		memset(tbuf, 0x0, sizeof(tbuf));
		get_zd_data(buf, tbuf);
		*timeout = atoi(tbuf);
		pub_log_info("[%s][%d] CALLLSN timeout=[%s][%d]", __FILE__, __LINE__, tbuf, *timeout);
	}
	else
	{
		*timeout = atoi(buf);
	}
	
	if (lsn_name[0] == '#' || lsn_name[0] == '$')
	{
		memset(buf, 0x0, sizeof(buf));
		get_zd_data(lsn_name, buf);
		len = strlen(buf);
		strncpy(lsn_name, buf, len);
		lsn_name[len] = '\0';
		pub_log_info("[%s][%d] lsn_name=[%s]", __FILE__, __LINE__, lsn_name);
	}

	return 0;
} 

static int flow_calllsn(sw_char_t *lsn_name, int id)
{
	int	timeout = 0;
	char	buf[128];
	
	memset(buf, 0x0, sizeof(buf));
	if (lsn_name == NULL || id < 0)
	{
		pub_log_error("[%s][%d] param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	if (lsn_name == NULL || lsn_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Calllsn lsnname is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_str_zipspace(lsn_name);
	get_timeout(lsn_name, &timeout);
	pub_log_info("[%s][%d] lsn_name=[%s] timeout=[%d]", __FILE__, __LINE__, lsn_name, timeout);
	if (lsn_name[0] == '\0')
	{
		pub_log_error("[%s][%d] CALLLSN lsnname is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	g_cmd.type = SW_CALLLSNREQ;
	g_cmd.msg_type = SW_MSG_REQ;
	g_cmd.task_flg = SW_STORE;
	g_cmd.dst_type = ND_LSN;
	g_cmd.start_line = id + 1;
	g_cmd.timeout = timeout;
	strncpy(g_cmd.ori_svr, g_cmd.dst_svr, sizeof(g_cmd.ori_svr) - 1);
	strncpy(g_cmd.ori_svc, g_cmd.dst_svc, sizeof(g_cmd.ori_svc) - 1);
	if (g_cmd.dst_prdt[0] != '\0')
	{
		strncpy(g_cmd.ori_prdt, g_cmd.dst_prdt, sizeof(g_cmd.ori_prdt) - 1);
	}
	sprintf(buf, "%s_%s", PROC_NAME_LSN, lsn_name);
	strncpy(g_cmd.dst_svr, buf, sizeof(g_cmd.dst_svr) - 1);
	memset(g_cmd.dst_prdt, 0x0, sizeof(g_cmd.dst_prdt));
	pub_log_info("[%s][%d] flow_calllsn LSNNAME=[%s] g_cmd.lsn_name=[%s]", 
		__FILE__, __LINE__, lsn_name, g_cmd.lsn_name);
	
	return SW_OK;
}

static int flow_linklsn(sw_char_t *lsn_name)
{
	sw_char_t	tmp[64];
	
	if (lsn_name == NULL || lsn_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Linklsn lsnname is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(tmp, 0x0, sizeof(tmp));
	strcpy(tmp, "\"NULL\"");
	
	if (strcmp(lsn_name, tmp) == 0)
	{
		g_cmd.type = SW_LINKNULL;
	}
	else
	{
		g_cmd.type = SW_LINKLSNREQ;
		g_cmd.msg_type = SW_MSG_REQ;
		g_cmd.task_flg = SW_FORGET;
		g_cmd.dst_type = ND_LSN;
		strncpy(g_cmd.ori_svr, g_cmd.dst_svr, sizeof(g_cmd.ori_svr) - 1);
		strncpy(g_cmd.ori_svc, g_cmd.dst_svc, sizeof(g_cmd.ori_svc) - 1);
		strncpy(g_cmd.ori_prdt, g_cmd.dst_prdt, sizeof(g_cmd.ori_prdt) - 1);
		memset(tmp, 0x0, sizeof(tmp));
		sprintf(tmp, "%s_%s", PROC_NAME_LSN, lsn_name);
		strncpy(g_cmd.dst_svr, tmp, sizeof(g_cmd.dst_svr) - 1);
		memset(g_cmd.dst_prdt, 0x0, sizeof(g_cmd.dst_prdt));
	}
	pub_log_info("[%s][%d] flow_linklsn [%s] success!", __FILE__, __LINE__, lsn_name);

	return SW_OK;
}

static int flow_postlsn(sw_char_t *lsn_name)
{
	int	result = 0;
	sw_char_t	buf[128];
	sw_cmd_t	cmd;
	sw_loc_vars_t	*vars;
	sw_loc_vars_t	new_vars;
	
	memset(buf, 0x0, sizeof(buf));
	if (lsn_name == NULL || lsn_name[0] == '\0')
	{
		pub_log_error("[%s][%d] PostLsn lsnname is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	memset(&cmd, 0x0, sizeof(cmd));
	memcpy(&cmd, &g_cmd, sizeof(cmd));
	strncpy(cmd.lsn_name, lsn_name, sizeof(cmd.lsn_name) - 1);
	cmd.mtype = mtype_new();
	if (cmd.mtype <= 0)
	{
		pub_log_error("[%s][%d] create mtype error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = pub_loc_vars_alloc(&new_vars, SHM_VARS);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_vars_alloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	vars = pub_get_global_vars();
	result = vars->clone(vars, cmd.mtype);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, vars.clone error, vid[%ld]." ,__FILE__,__LINE__, cmd.mtype);
		return SW_ERROR;
	}

	result = new_vars.create(&new_vars, cmd.mtype);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, new_vars.create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	cmd.trace_no = seqs_new_trace_no();
	if (cmd.trace_no <= 0)
	{
		pub_log_error("[%s][%d] create trace_no error!", __FILE__, __LINE__);
		mtype_delete(cmd.mtype, 1);
		return SW_ERROR;
	}
	
	result = seqs_get_sysdate(cmd.sys_date);
	if (result != SW_OK)
	{
		pub_log_error("[%s][%d] get sysdate error!", __FILE__, __LINE__);
		mtype_delete(cmd.mtype, 1);
		return SW_ERROR;
	}
	cmd.type = SW_POSTLSNREQ;
	cmd.msg_type = SW_MSG_REQ;
	cmd.task_flg = SW_DEL;
	cmd.dst_type = ND_LSN;
	strncpy(cmd.ori_svr, g_cmd.dst_svr, sizeof(cmd.ori_svr) - 1);
	strncpy(cmd.ori_svc, g_cmd.dst_svc, sizeof(cmd.ori_svc) - 1);
	strncpy(cmd.ori_prdt, g_cmd.dst_prdt, sizeof(cmd.ori_prdt) - 1);
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%s_%s", PROC_NAME_LSN, lsn_name);
	strncpy(cmd.dst_svr, buf, sizeof(cmd.dst_svr) - 1);
	memset(cmd.dst_prdt, 0x0, sizeof(cmd.dst_prdt));
	pub_log_info("[%s][%d] mtype=[%ld] traceno=[%lld]", __FILE__, __LINE__, cmd.mtype, cmd.trace_no);
	trace_create_info(&cmd);
	result = route_snd_dst(&new_vars, g_path, &cmd);
	if (result == SW_ERROR)
	{
		pub_log_error("[%s][%d] send cmd error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	new_vars.free_mem(&new_vars);
	pub_log_info("[%s][%d] flow_postlsn [%s] success!", __FILE__, __LINE__, lsn_name);
	
	return SW_OK;
}

static int flow_continue(int id)
{
	int	i = 0;
	int	tag = 0;
	int	next = 0;
	int	result = 0;
	int	hashid = 0;
	char	*ptr = NULL;
	char	value[64];
	
	for (next = id; next > 0; next--)
	{
		if (flow.work.sent[next].type == SENT_TYPE_CMPBEG)
		{
			tag++;
		}
	
		if (flow.work.sent[next].type == SENT_TYPE_CMPEND)
		{
			tag--;
		}
		
		if (flow.work.sent[next].type == SENT_TYPE_CMPDO && tag == 1)
		{
			break;		
		}
	}
	
	if (next == 0)
	{
		pub_log_error("[%s][%d] continue error! id=[%d]", __FILE__, __LINE__, id);
		return SW_ERROR;
	}
	
	for (hashid = 0; hashid < MAX_CMP_LEVEL; hashid++)
	{
		if (strcmp(g_cmp_info[hashid].err_name, flow.work.sent[next].out) == 0)
		{
			break;
		}
	}
	
	if (hashid == MAX_CMP_LEVEL)
	{
		pub_log_error("[%s][%d] Can not find [%s]!", __FILE__, __LINE__, flow.work.sent[next].out);
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d] err_name=[%s] err_code=[%s] up_name=[%s]", 
		__FILE__, __LINE__, g_cmp_info[hashid].err_name, g_cmp_info[hashid].err_code, g_cmp_info[hashid].up_name);
	while (g_cmp_info[hashid].up_name[0] != '\0')
	{
		memset(value, 0x0, sizeof(value));
		strncpy(value, g_cmp_info[hashid].up_name, sizeof(value) - 1);
		ptr = value;
		hashid = 0;
		while (*ptr != '\0')
		{
			hashid += (unsigned char)*ptr;
			ptr++;
		}
		i = 0;
		hashid = hashid % MAX_CMP_LEVEL;
		while (strcmp(g_cmp_info[hashid].err_name, value) != 0)
		{
			i++;
			hashid++;
			if (hashid == MAX_CMP_LEVEL)
			{
				hashid = 0;
			}
			if (i == MAX_CMP_LEVEL)
			{
				pub_log_error("[%s][%d] Can not find [%s]!", __FILE__, __LINE__, value);
				return SW_ERROR;
			}
		}
		result = flow_get_errid(g_cmp_info[hashid].err_name, next);
		if (result == SW_ERROR)
		{
			pub_log_error("[%s][%d] flow_get_errid error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		else if (result == -2)
		{
			if (strcmp(g_cmp_info[hashid].err_name, g_cmp_info[hashid].up_name) == 0)
			{
				pub_log_error("[%s][%d] ERR_NAME=[%s] UP_NAME=[%s] NOT FOUND!",
					__FILE__, __LINE__, g_cmp_info[hashid].err_name, g_cmp_info[hashid].up_name);
				return SW_ERROR;
			}
			pub_log_info("[%s][%d] [%s] not found, continue!",
				__FILE__, __LINE__, g_cmp_info[hashid].err_name);
			continue;
		}
		else
		{
			pub_log_info("[%s][%d] find id=[%d]", __FILE__, __LINE__, result);
			return result;
		}
	}
	pub_log_error("[%s][%d] Can not find continue line!", __FILE__, __LINE__);
	return SW_ERROR;
}
