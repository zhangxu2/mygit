#include "agent_tcpf.h"

int	g_file_socket;

static int filesock_wait_write(int sockid, int waitseconds)
{
	int	ret = 0;
	fd_set	rmask;
	struct	timeval time_out, *ptr;
	
	FD_ZERO(&rmask);
	FD_SET((unsigned int)sockid,&rmask);
	if(waitseconds >= 0)
	{
		time_out.tv_sec  = waitseconds ;
		time_out.tv_usec = 0;
	}
	else
	{
		time_out.tv_sec  = 0 ;
		time_out.tv_usec = 200000;
	}
	ptr = &time_out;
	
	/*循环等待，直到socket空闲可写*/
	while(1)
	{
		ret = select(sockid + 1, 0, &rmask, 0, ptr);
		if(ret > 0)
		{
			break;
		}
		else if(ret == 0)
		{ 
			break;
		}
		else
		{
			if(errno == EINTR)
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}
	
	return ret;
}

static int filesock_wait_read(int sockid, int waitseconds)
{
	int	ret = 0;
	fd_set	rmask;
	struct	timeval time_out, *ptr;
	
	FD_ZERO(&rmask);
	FD_SET((unsigned int)sockid, &rmask);
	if(waitseconds >= 0)
	{
		time_out.tv_sec  = waitseconds ;
		time_out.tv_usec = 0;
	}
	else
	{
		time_out.tv_sec  = 0 ;
		time_out.tv_usec = 200000;
	}
	ptr = &time_out;
	ret = 0;
	/*循环等待，直到socket空闲可读*/
	while(1)
	{
		ret = select(sockid + 1, &rmask, 0, 0, ptr);
		if(ret > 0)
		{
			break;
		}
		else if(ret == 0)
		{
			/*超时*/
			printf("time out :%d \n", ret);
			break;
		}
		else
		{
			if(errno == EINTR)
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}
	return ret;
}

static int net_getpeer(int sockid, char *ipaddr)
{
	unsigned	int len = 0;
	struct sockaddr_in	sock_addr;
	
	len = sizeof(sock_addr);
	if (ipaddr) 
	{
		if (getpeername(sockid, (struct sockaddr *)&sock_addr, &len))
		{
			return -1;
		}
		strcpy(ipaddr,(char *)inet_ntoa(sock_addr.sin_addr));
	}
	return 0;
}

static int filesock_send(int sockid, char *buf, int length, int timeout)
{
	int	ret = 0;
	int	tollen = 0;
	int	sublen = 0;
	int	shouldlen = 0;
	
	shouldlen = length;
	while(tollen < shouldlen)
	{
		if(timeout <= 0)
		{
			ret = filesock_wait_write(sockid, WAITSECONDS);
		}
		else
		{
			ret = filesock_wait_write(sockid, timeout);
		}
		
		if(ret <= 0)
		{
			pub_log_error("%s,%d,等待发送数据错误,errno=%d.",__FILE__,__LINE__, errno);
			return -1;
		}
		sublen = send(sockid, buf + tollen, shouldlen - tollen, 0);
		if(sublen <= 0)
		{
			pub_log_error("%s,%d,发送数据错误,errno=%d",__FILE__,__LINE__,errno);
			return -1;
		}
		tollen += sublen;
	}
	return 0;
}

static int filesock_recv(int sockid, char *buf, int length, int timeout)
{
	int	ret = 0;
	int	tollen = 0;
	int	sublen = 0;
	int	shouldlen = 0;
	
	shouldlen = length;
	while(tollen < shouldlen)
	{
		if(timeout <= 0)
		{
			ret = filesock_wait_read(sockid, WAITSECONDS);
		}
		else
		{
			ret = filesock_wait_read(sockid, timeout);
		}
		if(ret <= 0)
		{
			pub_log_error("%s,%d,等待接收数据错误,errno=[%d][%s],ret==[%d]"
				,__FILE__,__LINE__,errno,strerror(errno),ret);
			return -1;
		}
		sublen = recv(sockid, buf + tollen, shouldlen - tollen, 0);
		if(sublen <= 0)
		{
			pub_log_error("%s,%d,接收数据错误,errno=%d,len=%d",__FILE__,__LINE__,errno,sublen);
			return -1;
		} 
		
		tollen += sublen;
	}
	
	return 0;
}

int istack_init(sw_istack_t *stack)
{
	if (stack == NULL)
	{
		pub_log_error("%s, %d, Param error!\n",__FILE__,__LINE__);
		return -1;
	}

	stack->index = -1;
	stack->data = (int*)malloc(sizeof(int) * RANGE);
	if (stack->data == NULL)
	{
		pub_log_error("%s, %d, malloc error!\n",__FILE__,__LINE__);
		return -1;
	}

	stack->size = RANGE;

	stack->fp = fopen(stack->filename, "wb");
	if (stack->fp == NULL)
	{
		pub_log_error("%s, %d, fopen file[%s] error[%d][%s].\n"
			, __FILE__, __LINE__, stack->filename, errno, strerror(errno));
		free(stack->data);
		stack->data = NULL;
		stack->index = -1;
		stack->size = 0;

		return -1;
	}

	return 0;
}

int istack_destory(sw_istack_t *stack)
{
	if (stack == NULL)
	{
		pub_log_error("%s, %d, Param error!\n",__FILE__,__LINE__);
		return -1;
	}

	if (stack->fp != NULL)
	{
		fclose(stack->fp);
		stack->fp = NULL;
	}

	if (stack->data != NULL)
	{
		free(stack->data);
		stack->data = NULL;
	}

	stack->size = 0;
	stack->index = -1;

	return 0;
}

int istack_push(sw_istack_t *stack, int inode)
{
	int	*tmp = NULL;

	if (stack == NULL || inode < 0)
	{
		pub_log_error("%s, %d, Param error!\n",__FILE__,__LINE__);
		return -1;
	}

	if (stack->data == NULL)
	{
		pub_log_error("%s, %d, Stack data error!\n",__FILE__,__LINE__);
		return -1;		
	}

	if ((stack->index + 1) == stack->size)
	{
		tmp = (int *)malloc(sizeof(int) * (stack->size + RANGE));
		if (tmp == NULL)
		{
			pub_log_error("%s, %d, malloc error!\n",__FILE__,__LINE__);
			return -1;
		}

		memcpy(tmp, stack->data, (sizeof(int) * stack->size));
		free(stack->data);
		stack->data = tmp;
	}

	stack->index++;
	stack->data[stack->index] = inode;

	return 0;
}

int in_istack(sw_istack_t *stack, int inode)
{
	int	i = 0;

	if (stack == NULL || inode < 0)
	{
		pub_log_error("%s, %d, Param error!\n",__FILE__,__LINE__);
		return -1;
	}

	if (stack->data == NULL)
	{
		pub_log_error("%s, %d, Stack data error!\n",__FILE__,__LINE__);
		return -1;		
	}

	for (i = 0; i <= stack->index; i++)
	{
		if (inode == stack->data[i])
		{
			return 1;
		}
	}

	return 0;
}

int istack_pop(sw_istack_t *stack)
{
	if (stack == NULL)
	{
		pub_log_error("%s, %d, Param error!\n",__FILE__,__LINE__);
		return -1;
	}

	if (stack->data == NULL)
	{
		pub_log_error("%s, %d, Stack data error!\n",__FILE__,__LINE__);
		return -1;		
	}

	if (stack->index < 0)
	{
		pub_log_error("%s, %d, Opt error!\n",__FILE__,__LINE__);
		return -1;		
	}

	stack->index--;

	return 0;
}

int read_paths(char* name, sw_istack_t *stack)
{
	int	result = -1;
	int	child = 0;
	char	path[256];
	DIR	*dir = NULL;
	struct stat	buf;
	struct dirent	*dt = NULL;
	
	if (name == NULL || stack == NULL)
	{
		pub_log_error("%s, %d, Param error!\n",__FILE__,__LINE__);
		return -1;		
	}

	memset(&buf, 0x00, sizeof(buf));
	result = stat(name, &buf);
	if (result != 0)
	{
		pub_log_error("%s, %d, stat [%s] error[%d][%s]!\n"
			,__FILE__,__LINE__,name, errno, strerror(errno));
		return -1;
	}

	memset(path, 0x00, sizeof(path));
	if (S_ISDIR(buf.st_mode))
	{
		/*sprintf(path, "%s/", name);
		  printf("dir:%s\n", path);*/
	}
	else
	{
		stack->cnt += 1;
		pub_log_debug("%08d inode[%d] file:%s\n", stack->cnt, buf.st_ino, name);

		result = fprintf(stack->fp, "F:%s\n", name);
		if (result < 0)
		{
			pub_log_error("%s, %d, fprintf error[%d][%s].\n"
				, __FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}

		return 0;
	}

	result = in_istack(stack, buf.st_ino);
	if (result == 1)
	{
		pub_log_debug("warning: [%s] recursive directory loop!\n", name);
		return 0;
	}

	result = istack_push(stack, buf.st_ino);
	if (result != 0)
	{
		pub_log_error("%s, %d, istack_push path[%s] inode[%d] error.\n"
			, __FILE__, __LINE__, name, buf.st_ino);
		return -1;		
	}

	dir = opendir(name);
	if (dir == NULL)
	{
		pub_log_error("%s, %d, opendir [%s] error[%d][%s]\n"
			, __FILE__, __LINE__, name, errno, strerror(errno));
		istack_pop(stack);
		return -1;
	}

	while ((dt = readdir(dir)) != NULL)
	{
		if (strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0 )
		{
			continue;
		}

		child = 1;

		memset(path, 0x00, sizeof(path));
		sprintf(path, "%s/%s", name, dt->d_name);

		result = read_paths(path, stack);
		if (result != 0)
		{
			pub_log_error("%s, %d, read_paths [%s] error!\n",__FILE__,__LINE__,path);
			closedir(dir);
			istack_pop(stack);
			return -1;
		}
	}

	if (child == 0)
	{
		stack->cnt += 1;
		sprintf(path, "%s/", name);
		pub_log_debug("%08d inode[%d] dir:%s\n", stack->cnt, buf.st_ino, path);

		result = fprintf(stack->fp, "D:%s\n", path);
		if (result < 0)
		{
			pub_log_error("%s, %d, fprintf error[%d][%s].\n"
				, __FILE__, __LINE__, errno, strerror(errno));
			closedir(dir);
			istack_pop(stack);

			return -1;	
		}
	}

	closedir(dir);
	istack_pop(stack);

	return 0;
}

int mk_trunk(char* name, char** trunk, int* len, u_char end_flag, char* prefix)
{
	int	result = -1;
	int	file_len = 0;
	int 	trunk_len = 0;
	FILE*	fp = NULL;

	if (name == NULL || trunk == NULL || prefix == NULL)
	{
		pub_log_error("%s, %d, Param error!\n", __FILE__,__LINE__);
		return -1;
	}

	if (name[0] == 'D')
	{
		file_len = 0;
	}
	else if (name[0] == 'F')
	{
		fp = fopen(name + (strlen("F:")), "rb");
		if (fp == NULL)
		{
			pub_log_error("%s, %d, fopen[%s] error[%d][%s]!\n"
				, __FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}

		/*获取文件长度*/
		fseek(fp, 0, SEEK_END);
		file_len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	trunk_len = FNAME_LEN + FCONTENT_LEN + file_len + FLAG_LEN;

	*trunk = malloc(trunk_len);
	if ((*trunk) == NULL)
	{
		pub_log_error("%s, %d, malloc error!\n", __FILE__, __LINE__);

		if (fp != NULL)
		{
			fclose(fp);
			fp = NULL;
		}

		return -1;
	}

	memset((*trunk), 0x00, trunk_len);
	if (name[0] == 'D')
	{
		sprintf((char*)(*trunk), "D:%s", (name + strlen(prefix) + FTYPE_LEN));
	}
	else if (name[0] == 'F')
	{
		sprintf((char*)(*trunk), "F:%s", (name + strlen(prefix) + FTYPE_LEN));
	}

	pub_log_debug("%s, %d, [%s]\n", __FILE__,__LINE__,(*trunk));

	sprintf((char*)(*trunk) + FNAME_LEN, "%08d", file_len);
	if (file_len != 0)
	{
		result = fread(((char*)(*trunk) + FNAME_LEN + FCONTENT_LEN), file_len, 1, fp);
		if (ferror(fp))
		{
			pub_log_error("%s, %d, fread error[%d][%s].\n"
				, __FILE__, __LINE__, errno, strerror(errno));
			fclose(fp);
			free(*trunk);
			(*trunk) = NULL;

			return -1;
		}
	}

	*((char*)(*trunk) + FNAME_LEN + FCONTENT_LEN + file_len) = (u_char)end_flag;

	*len = trunk_len;

	return 0;
}

int send_files_net(int file_num, char* file_list, char* prefix, int sock_id)
{
	int	result = -1;
	int	i = 0;
	int	len = 0;
	char	line[FNAME_LEN];
	char	*trunk = NULL;
	FILE	*fp = NULL;
	u_char	end = END_FLAG;
	

	if (file_list == NULL || file_num <= 0 || prefix == NULL || sock_id < 0)
	{
		pub_log_error("%s, %d, Param error!\n", __FILE__, __LINE__);
		return -1;
	}

	result = access(file_list, F_OK);
	if (result != 0)
	{
		pub_log_error("%s, %d, access file[%s] error[%d][%s]!"
			, __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	fp = fopen(file_list, "rb");
	if (fp == NULL)
	{
		pub_log_error("%s, %d, fopen file[%s] error[%d][%s]!"
			, __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	for (i = 1; i <= file_num; i++)
	{
		memset(line, 0x00, sizeof(line));
		fgets(line, (FNAME_LEN - 1), fp);

		line[strlen(line) - 1] = '\0';

		pub_log_debug("%s, %d, line[%s].",__FILE__,__LINE__,line);

		if (i != file_num)
		{
			end = NOT_END;
		}
		else
		{
			end = END_FLAG;
		}

		result = mk_trunk(line, &trunk, &len, end, prefix);
		if (result != 0)
		{
			pub_log_error("%s, %d, mk_trunk file[%s] error!"
				,__FILE__, __LINE__, line);
			fclose(fp);
			return -1;
		}

		/*pub_log_bin(SW_LOG_DEBUG, trunk, len, "%s, %d, trunk: "
		  , __FILE__,__LINE__);*/

		result = filesock_send(sock_id, trunk, len, -1);
		if (result != 0)
		{
			pub_log_error("%s, %d, filesock_send [%s] error!", __FILE__, __LINE__, line);
			free(trunk);
			fclose(fp);

			return -1;
		}

		free(trunk);
		trunk = NULL;		
	}

	fclose(fp);

	return 0;
}

int get_prefix(char *filename, char* prefix)
{
	int	i = 0;
	int	j = 0;
	char	*tmp = NULL;
	char	*first = NULL;
	char	*last = NULL;

	if (filename == NULL || prefix == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	/*名字内不包含/,是相对路径*/
	tmp = strstr(filename, "/");
	if (tmp == NULL)
	{
		strcpy(prefix, "");
		return 0;
	}

	/*判断是否为根目录*/
	i = 0;
	j = 0;
	while(filename[i++] != '\0')
	{
		if (filename[j] == '/')
		{
			j++;
		}
	}

	if (j == strlen(filename))
	{
		pub_log_debug("%s, %d, It is /", __FILE__, __LINE__);
		strcpy(prefix, "");
		return 0;
	}

	/*普通嵌套目录*/
	first = &filename[0];
	last = &filename[strlen(filename) - 1];

	while((*last) == '/')
	{
		last--;
	}

	while((*last) != '/')
	{
		last--;
	}

	tmp = first;
	i = 0;
	while( tmp <= last)
	{
		prefix[i++] = *tmp;
		tmp++;
	}

	prefix[i] = '\0';

	return 0;
}

int recv_files(FILE *fin, char* dir)
{
	int	len = 0;
	int	result = -1;
	char	fname[FNAME_LEN];
	char	flen[FCONTENT_LEN];
	char	prefix[FNAME_LEN];
	char	*data = NULL;
	char	end = 0;
	FILE	*fp = NULL;
	
	if (fin == NULL || dir == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	result = access(dir, F_OK);
	if (result != 0)
	{
		pub_log_error("%s, %d, access dir[%s] error[%d][%s]!"
			, __FILE__, __LINE__, dir, errno, strerror(errno));
		return -1;
	}

	result = chdir(dir);
	if (result != 0)
	{
		pub_log_error("%s, %d, chdir dir[%s] error[%d][%s]!"
			, __FILE__, __LINE__, dir, errno, strerror(errno));
		return -1;		
	}

	while (1)
	{
		memset(fname, 0x00, sizeof(fname));
		fread(fname, FNAME_LEN, 1, fin);
		if (ferror(fin))
		{
			pub_log_error("%s, %d, fread error[%d][%s]!"
				, __FILE__, __LINE__, errno, strerror(errno));
			return -1;			
		}

		if (fname[0] == 'D')
		{
			/*创建目录*/
			result = pub_file_check_dir(fname + strlen("D:"));
			if (result != 0)
			{
				pub_log_error("%s, %d, create dir[%s] fail!"
					, __FILE__, __LINE__, fname);
				return -1;
			}

			memset(flen, 0x00, sizeof(flen));
			fread(flen, FCONTENT_LEN, 1, fin);
			if (ferror(fin))
			{
				pub_log_error("%s, %d, fread error[%d][%s]!"
					, __FILE__, __LINE__, errno, strerror(errno));
				return -1;			
			}
		}
		else if (fname[0] == 'F')
		{
			/*接收文件*/
			memset(prefix, 0x00, sizeof(prefix));
			result = get_prefix(fname + strlen("F:"), prefix);
			if (result != 0)
			{
				pub_log_error("%s, %d, get_prefix from [%s] error!"
					, __FILE__, __LINE__, fname);
				return -1;
			}

			pub_log_debug("%s, %d, fname[%s] prefix[%s]"
				, __FILE__, __LINE__, fname, prefix);

			result = pub_file_check_dir(prefix);
			if (result != 0)
			{
				pub_log_error("%s, %d, mkpath dir[%s] error!"
					,__FILE__, __LINE__, prefix);
				return -1;
			}

			fp = fopen(fname + strlen("F:"), "wb");
			if (fp == NULL)
			{
				pub_log_error("%s, %d, fopen file[%s] error[%d][%s]!"
					,__FILE__, __LINE__, fname, errno, strerror(errno));
				return -1;				
			}

			memset(flen, 0x00, sizeof(flen));
			fread(flen, FCONTENT_LEN, 1, fin);
			if (ferror(fin))
			{
				pub_log_error("%s, %d, fread file[%s] error[%d][%s]!"
					, __FILE__, __LINE__, fname, errno, strerror(errno));
				fclose(fp);
				return -1;			
			}

			len = atoi(flen);
			data = malloc(len);
			if (data == NULL)
			{
				pub_log_error("%s, %d, malloc error[%d][%s]!"
					, __FILE__, __LINE__, errno, strerror(errno));
				fclose(fp);
				return -1;
			}

			fread(data, len, 1, fin);
			if (ferror(fin))
			{
				pub_log_error("%s, %d, fread file[%s] error[%d][%s]!"
					, __FILE__, __LINE__, fname, errno, strerror(errno));
				fclose(fp);
				return -1;			
			}

			fwrite(data, len, 1, fp);
			if (ferror(fin))
			{
				pub_log_error("%s, %d, fread file[%s] error[%d][%s]!"
					, __FILE__, __LINE__, fname, errno, strerror(errno));
				fclose(fp);
				return -1;			
			}

			fclose(fp);
			free(data);
			data = NULL;
		}
		else
		{
			pub_log_error("%s, %d, Error, unknown ftype[%s]!"
				, __FILE__, __LINE__, fname, errno, strerror(errno));
			return -1;
		}

		fread(&end, FLAG_LEN, 1, fin);
		if (ferror(fin))
		{
			pub_log_error("%s, %d, fread file[%s] error[%d][%s]!"
				, __FILE__, __LINE__, fname, errno, strerror(errno));
			fclose(fp);
			return -1;			
		}

		if ((u_char)end == END_FLAG)
		{
			break;
		}
		else if ((u_char)end == NOT_END)
		{
			continue;
		}
		else
		{
			pub_log_error("%s, %d, Unknown end flag[%02x]!"
				, __FILE__, __LINE__, (u_char)end);
		}
	}

	return 0;
}

int recv_files_net(int sock_id, char* dir)
{
	int	len = 0;
	int	result = -1;
	char	fname[FNAME_LEN];
	char	flen[FCONTENT_LEN];
	char	*data = NULL;
	char	end = 0;
	char	cmd[256];
	char	prefix[FNAME_LEN];
	FILE	*fp = NULL;
	
	if (sock_id <= 0 || dir == NULL)
	{
		pub_log_error("%s, %d, Param error!", __FILE__, __LINE__);
		return -1;
	}

	result = access(dir, F_OK);
	if (result != 0)
	{
		pub_log_error("%s, %d, access dir[%s] error[%d][%s]!"
			, __FILE__, __LINE__, dir, errno, strerror(errno));
		return -1;
	}

	result = chdir(dir);
	if (result != 0)
	{
		pub_log_error("%s, %d, chdir dir[%s] error[%d][%s]!"
			, __FILE__, __LINE__, dir, errno, strerror(errno));
		return -1;		
	}

	while (1)
	{
		memset(fname, 0x00, sizeof(fname));
		result = filesock_recv(sock_id, fname, FNAME_LEN, -1);
		if (result < 0)
		{
			pub_log_error("[%s][%d] Recv error! errno=[%d]:[%s]\n",__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}

		if (fname[0] == 'D')
		{
			/*创建目录*/
			result = pub_file_check_dir(fname + strlen("D:"));
			if (result != 0)
			{
				pub_log_error("%s, %d, create dir[%s] fail!"
					, __FILE__, __LINE__, fname);
				return -1;
			}

			pub_log_debug("%s, %d, fname[%s]"
				, __FILE__, __LINE__, fname);

			memset(flen, 0x00, sizeof(flen));
			result = filesock_recv(sock_id, flen, FCONTENT_LEN, -1);
			if (result < 0)
			{
				pub_log_error("[%s][%d] Recv error! errno=[%d]:[%s]\n",__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}
		}
		else if (fname[0] == 'F')
		{
			/*接收文件*/
			memset(prefix, 0x00, sizeof(prefix));
			result = get_prefix(fname + strlen("F:"), prefix);
			if (result != 0)
			{
				pub_log_error("%s, %d, get_prefix from [%s] error!"
					, __FILE__, __LINE__, fname);
				return -1;
			}

			pub_log_debug("%s, %d, fname[%s] prefix[%s]"
				, __FILE__, __LINE__, fname, prefix);

			if (strlen(prefix) != 0)
			{
				result = pub_file_check_dir(prefix);
				if (result != 0)
				{
					pub_log_error("%s, %d, mkpath dir[%s] error!"
						,__FILE__, __LINE__, prefix);
					return -1;
				}
			}

			memset(flen, 0x00, sizeof(flen));
			result = filesock_recv(sock_id, flen, FCONTENT_LEN, -1);
			if (result < 0)
			{
				pub_log_error("[%s][%d] Recv error! errno=[%d]:[%s]\n",__FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}

			len = atoi(flen);
			pub_log_debug("%s, %d, fname[%s] len[%d]"
				, __FILE__, __LINE__, fname, len);

			if (len == 0)
			{
				memset(cmd, 0x00, sizeof(cmd));
				sprintf(cmd, "touch %s", fname + strlen("F:"));
				result = system(cmd);
				if (result != 0 && errno != ECHILD)
				{
					pub_log_error("%s, %d, run cmd[%s] error!"
						, __FILE__, __LINE__, cmd);
				}

				pub_log_debug("%s, %d, create file[%s] ok!"
					, __FILE__, __LINE__, fname);
			}
			else
			{
				fp = fopen(fname + strlen("F:"), "wb");
				if (fp == NULL)
				{
					pub_log_error("%s, %d, fopen file[%s] error[%d][%s]!"
						,__FILE__, __LINE__, fname, errno, strerror(errno));
					return -1;				
				}

				data = malloc(len);
				if (data == NULL)
				{
					pub_log_error("%s, %d, malloc error[%d][%s]!"
						, __FILE__, __LINE__, errno, strerror(errno));
					fclose(fp);
					return -1;
				}

				result = filesock_recv(sock_id, data, len, -1);
				if (result < 0)
				{
					pub_log_error("[%s][%d] Recv error! errno=[%d]:[%s]\n",__FILE__, __LINE__, errno, strerror(errno));
					return -1;
				}

				fwrite(data, len, 1, fp);
				if (ferror(fp))
				{
					pub_log_error("%s, %d, fread file[%s] error[%d][%s]!"
						, __FILE__, __LINE__, fname, errno, strerror(errno));
					fclose(fp);
					return -1;			
				}

				fclose(fp);
				free(data);
				data = NULL;
			}

		}
		else
		{
			pub_log_error("%s, %d, Error, unknown ftype[%s]!"
				, __FILE__, __LINE__, fname, errno, strerror(errno));
			return -1;
		}

		result = filesock_recv(sock_id, &end, FLAG_LEN, -1);
		if (result < 0)
		{
			pub_log_error("[%s][%d] Recv error! errno=[%d]:[%s]\n"
				,__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}

		if ((u_char)end == END_FLAG)
		{
			break;
		}
		else if ((u_char)end == NOT_END)
		{
			continue;
		}
		else
		{
			pub_log_error("%s, %d, Unknown end flag[%02x]!"
				, __FILE__, __LINE__, (u_char)end);
			fclose(fp);
			return -1;
		}
	}

	return 0;
}

int new_7f_snd(sw_loc_vars_t *vars)
{	
	int	result = 0;
	int	time_val;
	char	file_path[FNAME_LEN];
	char	*tmp = NULL;
	char	prefix[FNAME_LEN];
	char	cmd[256];
	char	del_send_path[256];
	sw_istack_t	stack;

	memset(file_path, 0x0, sizeof(file_path));
	loc_get_zd_data(vars, "$send_path", file_path);
	if (strlen(file_path) == 0)
	{
		pub_log_error("%s, %d, No $file_path!", __FILE__, __LINE__);
		return -1;
	}

	pub_log_debug( "[%s][%d] file_path=[%s]", __FILE__, __LINE__, file_path);

	time_val = time(NULL);
	tmp = getenv("SWWORK");
	if (tmp == NULL)
	{
		pub_log_error("%s, %d, No env SWWORK!", __FILE__, __LINE__);
		return -1;
	}

	memset(&stack, 0x00, sizeof(stack));
	sprintf(stack.filename, "%s/dat/%d.flist", tmp, time_val);
	pub_log_debug("%s, %d, file list[%s].", __FILE__, __LINE__, stack.filename);

	result = istack_init(&stack);
	if (result != 0)
	{
		pub_log_error("%s, %d, istack_init error!", __FILE__, __LINE__);
		return -1;
	}

	/*获取要发送的文件清单*/
	result = read_paths(file_path, &stack);
	if (result != 0)
	{
		pub_log_error("%s, %d, read_paths from [%s] error!"
			, __FILE__, __LINE__, file_path);
		istack_destory(&stack);

		return -1;
	}

	istack_destory(&stack);

	memset(prefix, 0x00, sizeof(prefix));
	result = get_prefix(file_path, prefix);
	if (result != 0)
	{
		pub_log_error("%s, %d, get_prefix file_path[%s] prefix[%s] error!"
			, __FILE__, __LINE__, file_path, prefix);
		return -1;
	}

	/*发送文件清单中的文件和空目录*/
	result = send_files_net(stack.cnt, stack.filename, prefix, g_file_socket);
	if (result != 0)
	{
		pub_log_error("%s, %d, send_files_net error!", __FILE__,__LINE__);
		return -1;
	}

	unlink(stack.filename);

	memset(del_send_path, 0x00, sizeof(del_send_path));
	loc_get_zd_data(vars, "$del_send_path", del_send_path);
	if (strlen(del_send_path) != 0)
	{
		memset(cmd, 0x00, sizeof(cmd));
		sprintf(cmd, "rm -rf %s", del_send_path);
		pub_log_debug("%s, %d, cmd[%s]", __FILE__, __LINE__, cmd);
		system(cmd);
	}
	return 0;
}

int new_7f_rcv(sw_loc_vars_t *vars)
{
	int	result = -1;
	char	dir_path[FNAME_LEN];
	char	*env = NULL;
	long	time_val = 0;

	env = getenv("SWWORK");
	if (env == NULL)
	{
		pub_log_error("%s, %d, No SWWORK!", __FILE__, __LINE__);
		return -1;
	}

	memset(dir_path, 0x00, sizeof(dir_path));
	time_val = time(NULL);
	sprintf(dir_path, "%s/dat/rcv_%d", env, (int)time_val);

	result = pub_file_check_dir(dir_path);
	if (result != 0)
	{
		pub_log_error("%s, %d, mkpath[%s] fail!", __FILE__, __LINE__, dir_path);
		loc_set_zd_data(vars, "$rcv_files_error", "1");
		return -1;

	}

	/*接受文件夹*/
	result = recv_files_net(g_file_socket, dir_path);
	if (result != 0)
	{
		pub_log_error("%s, %d, recv_files error!", __FILE__, __LINE__);
		loc_set_zd_data(vars, "$rcv_files_error", "1");
		return -1;
	}

	loc_set_zd_data(vars, "$rcv_path", dir_path);
	pub_log_debug("%s, %d, receive files to dir[%s].", __FILE__, __LINE__, dir_path);

	return 0;
}

int new_7f(sw_loc_vars_t *vars, int flag, int sock_id, void *param)
{	

	char	file_path[256];
	char	type[8];

	memset(file_path, 0x0, sizeof(file_path));
	memset(type, 0x0, sizeof(type));

	loc_get_zd_data(vars, ".TradeRecord.Header.System.FileFlag", type);
	if (strlen(type) == 0)
	{
		pub_log_debug( "[%s][%d] FileFlag值未设置！", __FILE__, __LINE__);
		return 0;
	}

	if (type[0] != '0' && type[0] != '1')
	{
		pub_log_debug( "[%s][%d] Unknown file flag[%s]!", __FILE__, __LINE__, type);
		return 0;	
	}

	/***1-发送 0-接收***/	
	g_file_socket = sock_id;
	pub_log_debug( "[%s][%d] FileFlag =[%s] flag=[%d]!", __FILE__, __LINE__,type, flag);
	if (flag == 0)
	{
		if(type[0] != '1')
		{
			return 0;
		}

		return new_7f_snd(vars);
	}
	else
	{
		/*if(type[0] != '0')
		{
			return 0;
		}*/

		return new_7f_rcv(vars);
	}
}
