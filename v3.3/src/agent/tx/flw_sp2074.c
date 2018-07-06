/*************************************************
  Œƒ º˛ √˚:  flw_sp2074.c                        **
  π¶ƒ‹√Ë ˆ:  ∞≤◊∞                        		     **
  ◊˜    ’ﬂ:  ◊ﬁ≈Â                                **
  ÕÍ≥…»’∆⁄:  20160802                            **
 *************************************************/
#include "agent_comm.h"

Node* insert_node(Node *head, char *name)
{
	Node *p = (Node*)calloc(sizeof(Node), 1);
	if (p == NULL)
	{
		pub_log_error("[%s][%d] malloc error",__FILE__, __LINE__);
		return NULL;
	}

	if (head == NULL)
	{
		strcpy(p->name,name);
		p->value = 0;
		p->next = NULL;
		return p;
	}

	Node *q = head;
	while (q->next != NULL)
	{
		q = q->next;
	}

	strcpy(p->name,	name);
	p->value = 0;
	p->next = NULL;
	q->next = p;

	return head;
}

static int get_dbtype(char *dbtype)
{
	int	get_flag = 0;
	int	result = 0;
	char	*swwork = NULL;
	char	cfg_path[256];
	char	line[256];
	char	*tmpstr1 = NULL;
	char	*tmpstr2 = NULL;
	FILE	*fp = NULL;

	if (dbtype == NULL)
	{
		pub_log_error("err: %s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	swwork = getenv("SWWORK");
	if (swwork == NULL)
	{
		pub_log_error("err: %s, %d, No env SWWORK!", __FILE__, __LINE__);
		return -1;
	}

	memset(cfg_path, 0x00, sizeof(cfg_path));
	sprintf(cfg_path, "%s/cfg/agentcfg/database.cfg", swwork);
	result = access(cfg_path, F_OK);
	if (result != 0)
	{
		pub_log_error("err: %s, %d, access file[%s] error[%d][%s]."
				, __FILE__, __LINE__, cfg_path, errno, strerror(errno));
		return -1;
	}

	fp = fopen(cfg_path, "rb");
	if (fp == NULL)
	{
		pub_log_error("err: %s, %d, fopen file[%s] error[%d][%s]."
				, __FILE__, __LINE__, cfg_path, errno, strerror(errno));
		return -1;
	}

	while(!feof(fp))
	{
		memset(line, 0x00, sizeof(line));
		fgets(line, sizeof(line) - 1, fp);
		line[strlen(line) - 1] = '\0';

		if (line[0] == '#')
		{
			continue;
		}

		tmpstr1 = strstr(line, "DB_TYPE");
		if (tmpstr1 == NULL)
		{
			continue;
		}
		else
		{
			tmpstr1 = tmpstr1 + strlen("DB_TYPE");
			tmpstr2 = strstr(tmpstr1, "oracle");
			if (tmpstr2 != NULL)
			{
				strcpy(dbtype, "oracle");
				get_flag = 1;
				break;
			}

			tmpstr2 = strstr(tmpstr1, "informix");
			if (tmpstr2 != NULL)
			{
				strcpy(dbtype, "informix");
				get_flag = 1;
				break;
			}
			tmpstr2 = strstr(tmpstr1, "db2");
			if (tmpstr2 != NULL)
			{
				strcpy(dbtype, "db2");
				get_flag = 1;
				break;
			}
		}
	}

	fclose(fp);

	if (get_flag == 0)
	{
		pub_log_error("err: %s, %d, Do not get dbtype!", __FILE__, __LINE__);
		return -1;
	}

	return 0;
}

static int get_dbcmd_prefix(char *prefix)
{
	int	result = 0;
	char	dbtype[32];
	char	sid[32];
	char	db_usr[32];
	char	db_passwd[32];
	char	db_name[32];
	char	*tmpstr = NULL;

	if (prefix == NULL)
	{
		pub_log_error("err: %s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	memset(dbtype, 0x00, sizeof(dbtype));
	result = get_dbtype(dbtype);
	if (result != 0)
	{
		pub_log_error("err: %s, %d, get_dbtype error!", __FILE__, __LINE__);
		return -1;
	}

	pub_log_info("%s, %d, dbtype[%s]", __FILE__, __LINE__, dbtype);
	if (strcmp(dbtype, "oracle") == 0)
	{
		memset(db_usr, 0x00, sizeof(db_usr));
		tmpstr = getenv("DB_USER");
		if (tmpstr == NULL)
		{
			pub_log_error("err: %s, %d, No env DB_USER!", __FILE__, __LINE__);
			return -1;			
		}
		else
		{
			strcpy(db_usr, tmpstr);
		}

		memset(db_passwd, 0x00, sizeof(db_passwd));
		tmpstr = getenv("DB_PASSWD");
		if (tmpstr == NULL)
		{
			pub_log_error("err: %s, %d, No env DB_PASSWD!", __FILE__, __LINE__);
			return -1;			
		}
		else
		{
			strcpy(db_passwd, tmpstr);
		}
		
		memset(sid, 0x00, sizeof(sid));
		tmpstr = getenv("ORACLE_SID");
		if (tmpstr == NULL)
		{
			pub_log_info("%s, %d, No env ORACLE_SID!", __FILE__, __LINE__);
			sprintf(prefix, "sqlplus %s/%s  < ", db_usr, db_passwd);
		}
		else
		{
			strcpy(sid, tmpstr);
			sprintf(prefix, "sqlplus %s/%s@%s  < ", db_usr, db_passwd, sid);
		}
	}
	else if (strcmp(dbtype, "informix") == 0)
	{
		memset(db_name, 0x00, sizeof(db_name));
		tmpstr = getenv("DBNAME");
		if (tmpstr == NULL)
		{
			pub_log_error("err: %s, %d, No env DBNAME!\n", __FILE__, __LINE__);
			return -1;			
		}
		else
		{
			strcpy(db_name, tmpstr);
		}
		
		memset(sid, 0x0, sizeof(sid));
		tmpstr = getenv("INFORMIXSERVER");
		if (tmpstr == NULL)
		{
			pub_log_info("%s, %d, No env INFORMIXSERVER!\n", __FILE__, __LINE__);
			sprintf(prefix, "dbaccess %s < ", db_name);
		}
		else
		{
			strcpy(sid, tmpstr);
			sprintf(prefix, "dbaccess %s@%s < ", db_name, sid);
		}
	}
	else if (strcmp(dbtype, "db2") == 0)
	{
		memset(db_name, 0x00, sizeof(db_name));
		tmpstr = getenv("DBNAME");
		if (tmpstr == NULL)
		{
			pub_log_error("err: %s, %d, No env DBNAME!\n", __FILE__, __LINE__);
			return -1;			
		}
		else
		{
			strcpy(db_name, tmpstr);
		}
		
		memset(db_usr, 0x00, sizeof(db_usr));
		tmpstr = getenv("DB_USER");
		if (tmpstr == NULL)
		{
			pub_log_info("%s, %d, No env DB_USER!", __FILE__, __LINE__);
		}
		else
		{
			strcpy(db_usr, tmpstr);
		}

		memset(db_passwd, 0x00, sizeof(db_passwd));
		tmpstr = getenv("DB_PASSWD");
		if (tmpstr == NULL)
		{
			pub_log_info("%s, %d, No env DB_PASSWD!", __FILE__, __LINE__);
		}
		else
		{
			strcpy(db_passwd, tmpstr);
		}

		if (strlen(db_usr) == 0 || strlen(db_passwd) == 0)
		{
			sprintf(prefix, "db2 connect to %s;db2 -tvf  ", db_name);
		}
		else
		{
			sprintf(prefix, "db2 connect to %s user %s using %s;db2 -tvf ", db_name, db_usr, db_passwd);
		}
	}
	else
	{
		pub_log_error("err: %s, %d, Unknown dbtype[%s]!\n", __FILE__, __LINE__,dbtype);
		return -1;
	}

	return 0;
}

int pri_str_split(char *sBuf, char *value, char item[][256], int cnt)
{
	int	flag = 0;
	int	j = 0;
	char	*pr = NULL;
	char	*start = NULL;
	
	if (sBuf == NULL)
	{   
		return -1; 
	}   

	pr = sBuf;
	if (*pr == '(')
	{
		pr++;
	}
	
	while(*pr != '\0')
	{
		if (*pr != '\t' && *pr != ' ' && flag == 0)
		{
			start = pr;
			flag = 1;
		}
		else if ((*pr == '\t' || *pr == ' ') && flag == 1 )
		{
			flag = 0;
			strncpy(item[j], start, pr-start);
			j++;
		}
		
		if (j >= cnt)
		{
			break;
		}
		pr++;
	}
	
	if (*pr == '\0')
	{   
		strncpy(item[j], start, pr-start);
	}

	return 0;
}

Node* node_free(Node *head)
{
	Node *p = NULL;
	
	while (head != NULL)
	{
		p = head;
		head = head->next;
		free(p);
	}
	
	return NULL;
}

static void remove_tmp_file(void)
{
	sw_char_t path[512];

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/flock/RESTORELIST.lock", getenv("HOME"));
	remove(path);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/alert.xml", getenv("HOME"));
	remove(path);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/.tabxml.xml", getenv("HOME"));
	remove(path);


	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/.file.xml", getenv("HOME"));
	remove(path);

	memset(path, 0x0, sizeof(path));
	snprintf(path, sizeof(path)-1, "%s/tmp/flock/CHECK_DEPLOY_CONFLICT.loc", getenv("HOME"));
	remove(path);
	return;
}

int del_lockfile()
{
	char  file_lock[256];
	memset(file_lock, 0x0, sizeof(file_lock));
	sprintf(file_lock, "%s/tmp/.lock_file", getenv("HOME"));
	remove(file_lock);
	remove_tmp_file();
	return 0;
}

int cheack_sametab(FILE *fp, char *tablename, Node *head, int *tabfilct, sw_loc_vars_t *vars)
{
	int	result = -1;
	int	i = 0;
	int	flag = 0;
	int	oracntzd = 0;
	int	j = 0;
	int cols = 0;
	int	rows = 0;
	char sql[256];
	char *ptr = NULL;
	char colum_name[32];
	char rowname[32];
	char datavalue[32];
	char datanull[32];
	char datalength[32];
	char datapre[32];
	char datasca[32];
	char Sbuf[256];
	char bufvar[128];
	char path[128];
	char item[5][256];
	Node  *q = NULL;

	if (strlen(tablename) == 0)
	{
		return 0;
	}

	if (fp == NULL || head == NULL)
	{
		pub_log_error("[%s][%d] fp, head is null", __FILE__, __LINE__);
		return -1;
	}

	for (i = 0; i < strlen(tablename); i++)
	{    
		tablename[i] = toupper( tablename[i] ); 
	}   

	memset(sql, 0x00, sizeof(sql));
	sprintf(sql, "select TABLE_NAME from USER_TAB_COLS where TABLE_NAME='%s'", tablename);
	result = pub_db_squery(sql);
	if (result < 0)
	{
		pub_log_error("[%s][%d] √ñ¬¥√ê√ê¬≤√©√ë¬Ø√ì√Ø¬æ√§√ä¬ß¬∞√ú! sql=[%s]", __FILE__, __LINE__, sql);
		return -1;
	}
	else if (result == 0)
	{
		oracntzd = 0;
		flag = 0;
		fprintf(fp, "Create Table %s\n", tablename);
		pub_log_info("[%s][%d] √ê√Ç√î√∂¬±√≠ %s", __FILE__, __LINE__, tablename);
		while(head != NULL)
		{
			if(strstr(head->name, "Drop Table") == NULL)
			{
				memset(item, 0x0, sizeof(item));
				fprintf(fp, "%s\n", head->name);
				if (strstr(head->name, ");") != NULL)
				{
					flag = 1;
				}
				if (flag == 0)
				{
					pub_log_debug("[%s][%d] --------------q->name=%s", __FILE__, __LINE__, head->name);
					pri_str_split(head->name, " " ,item, 4);
					pub_mem_memzero(path, sizeof(path));
					sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PKGName", *tabfilct,oracntzd);
					memset(bufvar,0x00,sizeof(bufvar));
					sprintf(bufvar,"%s",item[0]);
					loc_set_zd_data(vars, path,bufvar);
					pub_mem_memzero(path, sizeof(path));
					sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PKGType", *tabfilct,oracntzd);
					memset(bufvar,0x00,sizeof(bufvar));
					sprintf(bufvar,"%s|%s", item[1], Sbuf);
					loc_set_zd_data(vars, path,bufvar);
					oracntzd++;
				}
			}
			head = head->next;
		}
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Name", *tabfilct);
		loc_set_zd_data(vars, path, tablename);
		pub_mem_memzero(path, sizeof(path));
		sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Type", *tabfilct);
		loc_set_zd_data(vars, path, "–¬‘ˆ");
		
		(*tabfilct)++;
	}
	else
	{
		sprintf(sql, "select COLUMN_NAME,DATA_TYPE ,NULLABLE,DATA_LENGTH,DATA_PRECISION,DATA_SCALE from USER_TAB_COLS where TABLE_NAME='%s'", tablename);
		pub_log_debug("[%s][%d] =[%s]",__FILE__,__LINE__,sql);
		cols = pub_db_mquery(tablename, sql, 100);
		if (cols <= 0) 
		{    
			pub_log_error("[%s][%d] ¬≤√©√ë¬Ø√ä√Ω¬æ√ù¬ø√¢√ä¬ß¬∞√ú!", __FILE__, __LINE__);
			return SW_ERROR;
		}    

		rows = pub_db_mfetch(tablename);
		if (rows <0)
		{    
			pub_log_error("[%s][%d] ¬≤√©√ë¬Ø√ä√Ω¬æ√ù¬ø√¢√ä¬ß¬∞√ú!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		for (i = 0; i < rows; i++)
		{
			for (j = 0; j < cols; j++)
			{
				memset(rowname, 0x0, sizeof(rowname));
				ptr = pub_db_get_data_and_name(tablename, i + 1, j + 1, rowname, sizeof(rowname));
				pub_str_ziphlspace(ptr);
				if (strcmp(rowname,"COLUMN_NAME") == 0)
				{
					strcpy(colum_name,ptr);
				}

				if (strcmp(rowname, "DATA_TYPE") == 0)
				{
					strcpy(datavalue, ptr);
				}
				if (strcmp(rowname, "NULLABLE") == 0)
				{
					strcpy(datanull, ptr);
				}
				if (strcmp(rowname, "DATA_LENGTH") == 0)
				{
					strcpy(datalength, ptr);
				}
				if (strcmp(rowname, "DATA_PRECISION") == 0)
				{
					strcpy(datapre, ptr);
				}
				if (strcmp(rowname, "DATA_SCALE") == 0)
				{
					strcpy(datasca, ptr);
				}
			}

			if (strlen(datapre) == 0)
			{
				sprintf(Sbuf,"%s(%s)",datavalue,datalength);
			}
			else if ( strlen(datapre) != 0 && datasca[0] == '0')
			{
				sprintf(Sbuf,"%s(%s)",datavalue,datapre);
			}
			else
			{
				sprintf(Sbuf,"%s(%s,%s)",datavalue,datapre,datasca);
			}
			q = head;
			while (q != NULL)
			{
				if (strstr(q->name, "drop") != NULL)
				{
					q = q->next;
					q->value = 1;
					continue;
				}
				memset(&item, 0x0, sizeof(item));
				pub_log_info("[%s][%d] q->name= %s", __FILE__, __LINE__, q->name);
				if (q->name[0] == '(')
				{
					pri_str_split(q->name+1, " " ,item, 4);
				}
				else if (strncasecmp(q->name,");", 2) == 0)
				{
					pub_log_info("[%s][%d] q->name= %s", __FILE__, __LINE__, q->name);
					break;
				}
				else
				{
					/*√ì√ê√ó¬¢√ä√ç√ä¬±√ê√®√í¬™√ó¬¢√í√¢,¬¥√Ω√ê√û¬∏√Ñ*/
					pri_str_split(q->name, " ", item, 4);
				}
				if (item[1][strlen(item[1]) -1] == ',')
				{
					item[1][strlen(item[1]) -1] = '\0';
				}
				pub_log_info("[%s][%d] clos=%s, item[0]=%s, item[1] = %s, sBuf=%s", __FILE__, __LINE__,colum_name ,item[0],item[1], Sbuf);
				if (strcasecmp(item[0], colum_name) == 0 )
				{
					if (strcasecmp(item[1],  Sbuf) != 0)
					{
						if (strncasecmp(item[1], datavalue, strlen(datavalue)) != 0)
						{
							pub_log_error("[%s][%d] colum value is diffenct [%s][%s]", __FILE__, __LINE__, item[1], datavalue);
							return -1;
						}
						fprintf(fp, "alter table %s modify %s %s;\n", tablename, colum_name, item[1]);
						pub_mem_memzero(path, sizeof(path));
						sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PKGName", *tabfilct,oracntzd);
						memset(bufvar,0x00,sizeof(bufvar));
						sprintf(bufvar,"%s", colum_name);
						loc_set_zd_data(vars, path,bufvar);
						pub_mem_memzero(path, sizeof(path));
						sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PTName", *tabfilct,oracntzd);
						memset(bufvar,0x00,sizeof(bufvar));
						sprintf(bufvar,"%s", colum_name);
						loc_set_zd_data(vars, path,bufvar);
						pub_mem_memzero(path, sizeof(path));
						sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PKGType", *tabfilct,oracntzd);
						memset(bufvar,0x00,sizeof(bufvar));
						sprintf(bufvar,"%s", item[1]);
						loc_set_zd_data(vars, path,bufvar);
						pub_mem_memzero(path, sizeof(path));
						sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PTType", *tabfilct,oracntzd);
						memset(bufvar,0x00,sizeof(bufvar));
						sprintf(bufvar,"%s", Sbuf);
						loc_set_zd_data(vars, path,bufvar);
						flag = 1;
						oracntzd++;
					}
					q->value = 1;
					break;
				}
				q = q->next;
			}
		}

		q = head;
		while (q != NULL)
		{
			pub_log_info("[%s][%d] q->name=%s , q->value=%d", __FILE__, __LINE__, q->name, q->value);
			if (strncasecmp(q->name, ");", 2) == 0)
			{
				break;
			}
			else if (q->value != 1)
			{
				memset(item, 0x0, sizeof(item));
				if( q->name[0] == '(')
				{
					pri_str_split(q->name+1, " " ,item, 2);
				}
				else
				{
					pri_str_split(q->name, " " ,item, 2);
				}
			}
			else
			{
				q = q->next;
				continue;
			}
			
			if (item[1][strlen(item[1]) -1] == ',')
			{
				item[1][strlen(item[1]) -1] = '\0';
			}
			fprintf(fp, "alter table %s add %s %s;\n", tablename, item[0], item[1]);
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PKGName", *tabfilct,oracntzd);
			memset(bufvar,0x00,sizeof(bufvar));
			sprintf(bufvar,"%s", item[0]);
			loc_set_zd_data(vars, path,bufvar);
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Fields.Field(%d).PKGType", *tabfilct,oracntzd);
			memset(bufvar,0x00,sizeof(bufvar));
			sprintf(bufvar,"%s", item[1]);
			loc_set_zd_data(vars, path,bufvar);
			flag = 1;
			oracntzd++;
			q = q->next;
		}

		if (flag == 1)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Name", *tabfilct);
			loc_set_zd_data(vars, path, tablename);
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.ConflictTabs.ConflictTab(%d).Type", *tabfilct);
			loc_set_zd_data(vars, path, "∏¸–¬");
		
			(*tabfilct)++;
		}
	}

	return 0;
}


int  check_dbdatd(char *install_dir, sw_loc_vars_t *vars)
{
	int	flag = 0;
	int	oracnt = 0;
	char mdic[256];
	char mbase[256];
	char buf[256];
	char item[4][256];
	char buftmp[256];
	char tmp_baseData[256];
	char tmp_dictionary[256];
	char file_path[256];
	struct dirent	*dir_entry = NULL;
	DIR	*ptr_dir = NULL;
	Node	*head = NULL, *p = NULL;
	FILE	*fp = NULL, *fp_base = NULL, *fp_tmp = NULL;

	memset(tmp_baseData, 0x00, sizeof(tmp_baseData));
	memset(tmp_dictionary, 0x00, sizeof(tmp_dictionary));
	sprintf(tmp_baseData, "%s/tmp/baseDatatmp.sql", getenv("HOME"));
	sprintf(tmp_dictionary, "%s/tmp/dictionarytmp.sql", getenv("HOME"));

	fp_base = fopen(tmp_baseData, "w");
	if (fp_base == NULL )
	{
		pub_log_error("[%s][%d] fpoen file [%s] is failed\n", __FILE__, __LINE__, tmp_baseData);
		return -1;
	}

	fp_tmp = fopen(tmp_dictionary, "w");
	if (fp_tmp == NULL )
	{
		fclose(fp_base);
		pub_log_error("[%s][%d] fpoen file [%s] is failed\n", __FILE__, __LINE__, tmp_dictionary);
		return -1;
	}

	sprintf(file_path, "%s/products", install_dir);
	ptr_dir = opendir(file_path);
	if (ptr_dir == NULL)
	{   
		fclose(fp_base);
		fclose(fp_tmp);
		pub_log_error("[%s][%d] open dir [%s] is failed!\n", __FILE__, __LINE__, file_path);
		return -1; 
	}
	
	while ((dir_entry = readdir(ptr_dir)) != NULL)
	{
		if (dir_entry->d_name[0] == '.')
		{   
			continue;
		} 
		memset(mdic, 0x00, sizeof(mdic));
		sprintf(mdic, "%s/%s/bin/dictionary.sql", file_path, dir_entry->d_name);

		memset(mbase, 0x00, sizeof(mbase));
		sprintf(mbase, "%s/%s/bin/baseData.sql", file_path, dir_entry->d_name);

		fp = fopen(mbase, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] fpoen file [%s] is failed\n", __FILE__, __LINE__, mbase);
			fclose(fp_base);
			fclose(fp_tmp);
			closedir(ptr_dir);
			return -1;
		}
		
		while (!feof(fp))
		{
			memset(buf,0x00,sizeof(buf));
			fgets(buf,sizeof(buf)-1,fp);
			fprintf(fp_base, "%s", buf);
		}
		fclose(fp);

		fp = fopen(mdic, "r");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d] fpoen file [%s] is failed\n", __FILE__, __LINE__, mdic);
			fclose(fp_base);
			fclose(fp_tmp);
			closedir(ptr_dir);
			return -1;
		}
		
		flag = 0;
		while (!feof(fp))
		{
			memset(buf,0x00,sizeof(buf));
			fgets(buf,sizeof(buf)-1,fp);

			if (buf[strlen(buf)-1]=='\n')
			{    
				buf[strlen(buf)-1]='\0';    
			}    
			if (buf[strlen(buf)-1]=='\r')
			{    
				buf[strlen(buf)-1]='\0';    
			}    
			if (strstr(buf,"Create Table")!=NULL || strstr(buf,"Create VIEW ")!=NULL || strstr(buf,"Create Sequence")!=NULL)
			{   
				pub_log_debug("[%s][%d] buftmp=[%s]",__FILE__,__LINE__,buf);
			}

			if (strncasecmp(buf,"Create Table",12) ==0 )
			{
				memset(buftmp,0x00,sizeof(buftmp));
				pub_log_debug("[%s][%d] buftmp=[%s]",__FILE__,__LINE__, item[2]);
				cheack_sametab(fp_tmp, item[2], head, &oracnt, vars);
				if (head != NULL)
				{
					pub_log_debug("[%s][%d] buftmp=[%s]",__FILE__,__LINE__,buftmp);
					head = node_free(head);
				}
				memset(&item, 0x0, sizeof(item));
				pri_str_split(buf," ", item, 3);
				sprintf(buftmp,"tmp_%s",item[2]);
				pub_log_debug("[%s][%d] buftmp=[%s]",__FILE__,__LINE__, item[2]);
				continue;
			}
			else if (strncasecmp(buf,"Drop Table",10) != 0)
			{
				head = insert_node(head, buf);
				p = head;
			}
		}
		cheack_sametab(fp_tmp, item[2], head, &oracnt, vars);
		head = node_free(head);
		memset(&item, 0x0, sizeof(item));
		fclose(fp);
	}

	fclose(fp_tmp);
	fclose(fp_base);
	closedir(ptr_dir);

	return 0;
}

int update_config(char *install_dir)
{
	int	result = -1;
	char	cmd[256];
	char	install_file[256];
	char	bp_file[256];

	memset(install_file, 0x0, sizeof(install_file));
	memset(bp_file, 0x0, sizeof(bp_file));

	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd, "cp -R %s/cfg %s/", install_dir, getenv("SWWORK"));
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_info("[%s][%d] cmd [%s] error", __FILE__, __LINE__, cmd);
		return -1;
	}
	memset(cmd, 0x0, sizeof(cmd));
	sprintf(cmd,"cp -R %s/products %s/",install_dir,getenv("SWWORK"));
	result = agt_system(cmd);
	if (result != 0)
	{
		pub_log_info("[%s][%d] cmd [%s] error", __FILE__, __LINE__, cmd);
		return -1;
	}
	return 0;	
}

int update_dbsql()
{
	int  result = -1;
	char cmd[256];
	char prefix[256];
	char baseDatatmp[256];
	char dictionarytmp[256];

	memset(prefix, 0x0, sizeof(prefix));
	memset(dictionarytmp, 0x0, sizeof(dictionarytmp));
	memset(baseDatatmp, 0x0, sizeof(baseDatatmp));


	sprintf(baseDatatmp, "%s/tmp/baseDatatmp.sql", getenv("HOME"));
	sprintf(dictionarytmp, "%s/tmp/dictionarytmp.sql", getenv("HOME"));

	memset(cmd,0x00,sizeof(cmd));
	result = get_dbcmd_prefix(prefix);
	if (result != 0)
	{
		pub_log_error("err: %s, %d, get_dbcmd_prefix error!", __FILE__, __LINE__);
		return -1;
	}
	sprintf(cmd, "%s  %s 1>>/dev/null 2>>/dev/null", prefix, baseDatatmp);
	result = agt_system(cmd);
	if (result == -1)
	{
		pub_log_error("[%s][%d] system √ñ¬¥√ê√ê√ä¬ß¬∞√ú",__FILE__,__LINE__);
		return -1;
	}
		pub_log_info("[%s][%d] system %s", __FILE__,__LINE__, cmd);

	memset(cmd,0x00,sizeof(cmd));
	sprintf(cmd, "%s  %s 1>>/dev/null 2>>/dev/null", prefix, dictionarytmp);
	result = agt_system(cmd);
	if (result == -1)
	{
		pub_log_error("[%s][%d] system √ñ¬¥√ê√ê√ä¬ß¬∞√ú",__FILE__,__LINE__);
		return -1;
	}
	pub_log_info("[%s][%d] system %s", __FILE__,__LINE__, cmd);

	return 0;
}

/*ºÏ≤È∂˛Ω¯÷∆Œƒº˛∫Õø‚µƒ≥ÂÕª*/
static int check_conflict(sw_loc_vars_t *vars)
{
	int	i = 0; 
	long	time_val = 0; 
	char	line[256];
	char	path[256];
	char	line1[1024];
	char	script[1024];
	char	*swhome = NULL;
	char	install_dir[256];
	char	*swwork = NULL;
	char	xml_path[256];
	FILE	*fp = NULL;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;

	if (vars == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	pub_mem_memzero(install_dir, sizeof(install_dir));
	loc_get_zd_data(vars, ".TradeRecord.Request.InstallDir", install_dir);

	if (strlen(install_dir) == 0)
	{
		pub_log_error("%s, %d, No .TradeRecord.Request.InstallDir", __FILE__, __LINE__);
		return -1;
	}

	swwork = getenv("SWWORK");
	swhome = getenv("SWHOME");
	pub_mem_memzero(xml_path, sizeof(xml_path));
	time_val = time(NULL);
	sprintf(xml_path, "%s/tmp/xml%ld.out", swwork, time_val);
	pub_log_debug("%s, %d, xml_path[%s]", __FILE__, __LINE__, xml_path);

	pub_mem_memzero(script, sizeof(script));
	sprintf(script, "sh %s/sbin/chk_conflict_xml.sh %s %s ", swhome, install_dir, xml_path);
	pub_log_info("%s, %d, script[%s]", __FILE__, __LINE__, script);
	fp = popen(script, "r");
	if (fp == NULL)
	{
		pub_log_error("%s, %d, popen[%s] error[%d][%s]!"
			, __FILE__, __LINE__, script, errno, strerror(errno));
		return -1;
	}

	while (!feof(fp))
	{
		pub_mem_memzero(line, sizeof(line));
		fgets(line, sizeof(line) - 1, fp);
		line[strlen(line) - 1] = '\0';

		if (strncmp(line, SCRIPT_ERR, 3) == 0)
		{
			pub_log_error("%s, %d, run script[%s] error[%s]!"
			,__FILE__,__LINE__, script, line);
			break;
		}
		else if (strncmp(line, "ok", 2) == 0)
		{
			break;
		}
		else
		{
			pub_log_error("%s, %d, Unknown status, run script[%s] error[%s]!"
				,__FILE__,__LINE__,script,line);
			pclose(fp);
			return -1;
		}
	}
	pclose(fp);

	xml = pub_xml_crtree(xml_path);
	if (xml == NULL)
	{
		pub_log_error("%s, %d, create xml from file[%s] error!"
			, __FILE__, __LINE__, xml_path);
		unlink(xml_path);
		return -1;
	}

	i = 0;
	node = pub_xml_locnode(xml, ".Conflicts.Conflict");
	while (node != 0)
	{
		if (node->name != NULL && strcmp(node->name, "Conflict") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;

		node1 = pub_xml_locnode(xml, "pt_path");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.Conflicts.Conflict(%d).pkg_path", i);
			loc_set_zd_data(vars, path, node1->value);
		}
		else
		{
			pub_log_error("%s, %d, No .TradeRecord.Response.Conflicts.Conflict(%d).pkg_path"
				, __FILE__, __LINE__, i);
			pub_xml_deltree(xml);
			unlink(xml_path);
			return -1;
		}

		node1 = pub_xml_locnode(xml, "pkg_path");
		if (node1 != NULL && node1->value != NULL && strlen(node1->value) != 0)
		{
			pub_mem_memzero(path, sizeof(path));
			sprintf(path, ".TradeRecord.Response.Conflicts.Conflict(%d).pt_path", i);
			loc_set_zd_data(vars, path, node1->value);
		}
		else
		{
			pub_log_error("%s, %d, No .TradeRecord.Response.Conflicts.Conflict(%d).pt_path"
				, __FILE__, __LINE__, i);
			pub_xml_deltree(xml);
			unlink(xml_path);
			return -1;
		}
		i++;
		node = node->next;
	}

	pub_xml_deltree(xml);
	unlink(xml_path);
	return 0;
}

int sp2074(sw_loc_vars_t *vars)
{
	int result = -1;
	char opt[32];
	char chk_run[32];
	char install_dir[256];
	char reply[8];
	char res_msg[256];
	
	memset(reply, 0x0, sizeof(reply));
	memset(res_msg, 0x0, sizeof(res_msg));
	memset(install_dir, 0x0, sizeof(install_dir));
	memset(opt, 0x0, sizeof(opt));
	

	loc_get_zd_data(vars, ".TradeRecord.Request.Option", opt);
	loc_get_zd_data(vars, ".TradeRecord.Request.InstallDir", install_dir);
	memset(chk_run, 0x0, sizeof(chk_run));
	loc_get_zd_data(vars, ".TradeRecord.Request.ChkRun", chk_run);

	pub_log_info("%s, %d, opt[%s] chk_run[%s]", __FILE__, __LINE__, opt, chk_run);

	
	if (strcmp(opt, "BACK_DEPLOY") == 0)
	{
		result = check_dbdatd(install_dir, vars);
		if(result == -1)
		{
			strcpy(reply, "E999");	
			strcpy(res_msg, " ˝æ›ø‚±ÌºÏ≤‚ ß∞‹");	
			pub_log_error("[%s][%d] db check error! ", __FILE__, __LINE__);
			goto ErrExit;
		}
		goto OkExit;
	}
	
	if (strcmp(opt, "CHECK_CONFLICT") == 0)
	{
		goto OkExit;
	}
	
	if (strcmp(opt, "INSTALL") == 0)
	{
		char file[256];
		char chk_run[32];
		char id[32];
		FILE *fp = NULL;
		pub_mem_memzero(chk_run, sizeof(chk_run));
		loc_get_zd_data(vars, ".TradeRecord.Request.ChkRun", chk_run);
		if (chk_run[0] == '1') 
		{    
			loc_set_zd_data(vars, ".TradeRecord.Response.Running", "0");
			loc_set_zd_data(vars, ".TradeRecord.Response.Status", "ok");
			goto OkExit;
		}   

		memset(file, 0x0, sizeof(file));
		sprintf(file, "%s/tmp/.back_id", getenv("HOME"));
		if(access(file, F_OK))
		{
			pub_log_error("[%s][%d] backup error", __FILE__, __LINE__);
			strcpy(reply, "E999");
			strcpy(res_msg, "±∏∑›≈‰÷√ ß∞‹....");
			goto ErrExit;
		}
		fp = fopen(file, "r");
		if(fp == NULL)
		{
			pub_log_info("[%s][%d] open file [%s]error", __FILE__, __LINE__, file);
			return -1;
		}

		fgets(id, sizeof(id), fp);
		if (id[strlen(id) -1] == '\n')
		{
			id[strlen(id) -1] = '\0';
		}
		if (strlen(id) == 0)
		{
			pub_log_error("[%s][%d] backup error", __FILE__, __LINE__);
			strcpy(reply, "E999");
			strcpy(res_msg, "±∏∑›≈‰÷√ ß∞‹....");
			goto ErrExit;

		}
		fclose(fp);
		/*√Ö√§√ñ√É√é√Ñ¬º√æ¬∏√º√ê√Ç*/
		result = update_config(install_dir);
		if (result == -1)
		{
			strcpy(reply, "E999");	
			strcpy(res_msg, "∏¸–¬∆ΩÃ®≈‰÷√ ß∞‹");
			restore(id);
			pub_log_error("[%s][%d] update config error! ", __FILE__, __LINE__);
			goto ErrExit;
		}



		/*√ä√Ω¬æ√ù¬ø√¢¬∏√º√ê√Ç*/
		result = update_dbsql();
		if (result == -1)
		{
			restore(id);
			pub_log_error("[%s][%d] update dbconfig error! ", __FILE__, __LINE__);
			strcpy(reply, "E999");	
			strcpy(res_msg, " ˝æ›ø‚∏¸–¬ ß∞‹");	
			goto ErrExit;
		}
		goto OkExit;
	}

	else if(strcmp(opt, "CANCEL")==0)
	{
		del_lockfile();
	}
	else
	{
		pub_log_error("%s, %d, Unknown opt[%s]!", __FILE__, __LINE__);
		strcpy(reply, "E012");
		goto ErrExit;
	}

OkExit:
	del_lockfile();
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "step successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return 0;

ErrExit:
	del_lockfile();
	agt_error_info(reply, res_msg);
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return -1;
}
