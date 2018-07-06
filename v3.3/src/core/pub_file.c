#include "pub_file.h"
#include "pub_mem.h"

sw_int_t aix_mkdirp(char *path, int mode)
{
	char	*ptr = path + 1;
	
	while (*ptr != '\0')
	{
		if (*ptr != '/')
		{
			ptr++;
			continue;
		}
		*ptr = '\0';
		
		if (mkdir(path, mode) < 0)
		{
			if (errno != EEXIST)
			{
				return -1;
			}
		}
		*ptr = '/';
		ptr++;
	}

	if (mkdir(path, mode) < 0)
	{
		if (errno != EEXIST)
		{
			return -1;
		}
	}
	
	return 0;
}

char *get_last_name(char *fullname, char *name)
{
	char    *p = NULL;
	char    *ptr = fullname + strlen(fullname) - 1;

	while (ptr > fullname && *ptr != '/')
	{
		ptr--;
	}

	if (*ptr == '/')
	{
		ptr++;
	}

	p = ptr;
	while (*ptr != '\0')
	{
		*name++ = *ptr++;
	}
	*name = '\0';

        return p;
}


sw_int_t pub_file_check_dir(sw_char_t *path)
{
	DIR*	dir = NULL;
	
	if ((dir = opendir(path)) != NULL)
	{
		closedir(dir);
		return SW_OK;
	}
	
	return (aix_mkdirp(path, 0777));
}

ssize_t
pub_file_read(sw_file_t *file, u_char *buf, size_t size, off_t offset)
{
	ssize_t  n;

#if (PUB_HAVE_PREAD)

	n = pread(file->fd, buf, size, offset);

	if (n == -1) 
	{
        	return SW_ERROR;
	}

#else

	if (file->sys_offset != offset) 
	{
		if (lseek(file->fd, offset, SEEK_SET) == -1) 
		{
            		return SW_ERROR;
        	}

        	file->sys_offset = offset;
    	}

	n = read(file->fd, buf, size);

	if (n == -1) 
	{
        	return SW_ERROR;
	}

	file->sys_offset += n;

#endif

	file->offset += n;

	return n;
}

ssize_t
pub_file_write(sw_file_t *file, u_char *buf, size_t size, off_t offset)
{
	ssize_t  n, written;

	written = 0;

#if (PUB_HAVE_PWRITE)

	for ( ;; ) 
	{
		n = pwrite(file->fd, buf + written, size, offset);

		if (n == -1) 
		{
            		return SW_ERROR;
        	}

		file->offset += n;
		written += n;

		if ((size_t) n == size) 
		{
			return written;
		}

		offset += n;
		size -= n;
	}

#else

	if (file->sys_offset != offset) 
	{
        	if (lseek(file->fd, offset, SEEK_SET) == -1) 
		{
            		return SW_ERROR;
        	}

		file->sys_offset = offset;
	}

	for ( ;; ) 
	{
        	n = write(file->fd, buf + written, size);

        	if (n == -1) 
		{
            		return SW_ERROR;
        	}

		file->offset += n;
		written += n;

        	if ((size_t) n == size) 
		{
            		return written;
        	}

		size -= n;
	}
#endif
}

/*sw_int_t
sw_file_settime(u_char *name, sw_fd_t fd, time_t s)
{
	struct timeval  tv[2];

	tv[0].tv_sec = ngx_time();
	tv[0].tv_usec = 0;
	tv[1].tv_sec = s;
	tv[1].tv_usec = 0;

	if (utimes((char *) name, tv) != -1) 
	{
        	return SW_OK;
    	}

	return SW_ERROR;
}*/

sw_int_t
pub_create_path(sw_file_t *file, sw_path_t *path)
{
	size_t      pos;
	sw_err_t   err;
	sw_uint_t  i;

	pos = path->name.len;

	for (i = 0; i < 3; i++) 
	{
        	if (path->level[i] == 0) 
		{
            		break;
        	}

        	pos += path->level[i] + 1;

        	file->name.data[pos] = '\0';

        	if (pub_create_dir(file->name.data, 0700) == SW_FILE_ERROR) 
		{
            		err = sw_errno;
            		if (err != SW_EEXIST) 
			{
                		return SW_ERROR;
            		}
        	}

        	file->name.data[pos] = '/';
	}

	return SW_OK;
}

sw_err_t
pub_create_full_path(u_char *dir, sw_uint_t access)
{
	u_char     *p, ch;
	sw_err_t   err;

	err = 0;

	p = dir + 1;

	for ( /* void */ ; *p; p++) 
	{
        	ch = *p;

        	if (ch != '/') 
		{
			continue;
        	}

		*p = '\0';

		if (pub_create_dir(dir, access) == SW_FILE_ERROR) 
		{
			err = sw_errno;

			switch (err) 
			{
            			case SW_EEXIST:
					err = 0;
            			case SW_EACCES:
                			break;

            			default:
                			return err;
            		}
        	}

        	*p = '/';
	}

	return err;
}

sw_int_t pub_create_pathes(sw_path_t      *path, sw_uid_t user)
{
	sw_err_t         err;
	
	if (pub_create_dir(path->name.data, 0700) == SW_FILE_ERROR) 
	{
		err = sw_errno;
		if (err != SW_EEXIST) 
		{
        		return SW_ERROR;
    		}
	}

	if (user == (sw_uid_t) SW_CONF_UNSET_UINT) 
	{
    		return SW_OK;
	}

#if !(PUB_WIN32)
	{
        sw_file_info_t   fi;

        if (pub_file_info((const char *) path->name.data, &fi)
            == SW_FILE_ERROR)
        {
		return SW_ERROR;
        }

        if (fi.st_uid != user) 
	{
        	if (chown((const char *) path->name.data, user, -1) == -1) 
		{
                	return SW_ERROR;
		}
        }

        if ((fi.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR))
                                                  != (S_IRUSR|S_IWUSR|S_IXUSR))
        {
        	fi.st_mode |= (S_IRUSR|S_IWUSR|S_IXUSR);

       		if (chmod((const char *) path->name.data, fi.st_mode) == -1) 		{
                	return SW_ERROR;
		}
        }
        }
#endif
	

	return SW_OK;
}


sw_int_t
pub_open_dir(sw_str_t *name, sw_dir_t *dir)
{
	dir->dir = opendir((const char *) name->data);

	if (dir->dir == NULL) 
	{
        	return SW_ERROR;
	}

	dir->valid_info = 0;

	return SW_OK;
}

sw_int_t
pub_read_dir(sw_dir_t *dir)
{
	dir->de = readdir(dir->dir);

	if (dir->de) 
	{
#if (PUB_HAVE_D_TYPE)
        	dir->type = dir->de->d_type;
#else
        	dir->type = 0;
#endif
        	return SW_OK;
	}

	return SW_ERROR;
}

sw_int_t
pub_open_glob(sw_glob_t *gl)
{
	int  n;

	n = glob((char *) gl->pattern, GLOB_NOSORT, NULL, &gl->pglob);

	if (n == 0) 
	{
        	return SW_OK;
	}

#ifdef GLOB_NOMATCH

	if (n == GLOB_NOMATCH && gl->test) 
	{
        	return SW_OK;
	}

#endif

	return SW_ERROR;
}

sw_int_t
pub_read_glob(sw_glob_t *gl, sw_str_t *name)
{
	size_t  count;

#ifdef GLOB_NOMATCH
	count = (size_t) gl->pglob.gl_pathc;
#else
	count = (size_t) gl->pglob.gl_matchc;
#endif

	if (gl->n < count) 
	{
        	name->len = (size_t) strlen(gl->pglob.gl_pathv[gl->n]);
        	name->data = (char *) gl->pglob.gl_pathv[gl->n];
        	gl->n++;

        	return SW_OK;
	}

	return SW_DONE;
}

void
pub_close_glob(sw_glob_t *gl)
{
    globfree(&gl->pglob);
}

sw_err_t
pub_trylock_fd(sw_fd_t fd)
{
	struct flock  fl;

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &fl) == -1) 
	{
        	return sw_errno;
	}

	return 0;
}

sw_err_t
pub_lock_fd(sw_fd_t fd)
{
	struct flock  fl;

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLKW, &fl) == -1) 
	{
        	return sw_errno;
    	}

	return 0;
}

sw_err_t
pub_unlock_fd(sw_fd_t fd)
{
	struct flock  fl;

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &fl) == -1) 
	{
        	return  sw_errno;
	}

	return 0;
}

sw_int_t
pub_file_exist(sw_char_t *filename)
{
	struct stat	st;

	if (stat(filename, &st) < 0)
	{
		return 0;
	}
	if ((st.st_mode & S_IFMT) != S_IFREG)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/* Judge the file whether is directory */
bool file_isdir(const char *fname)
{
	int ret = 0;
	struct stat _fstat;

	ret = stat(fname,&_fstat);
	if(ret < 0)
	{
		return false;
	}

	if(0 == S_ISDIR(_fstat.st_mode))
	{
		return false;
	}

	return true;
}

#if (PUB_HAVE_STATFS)

size_t
pub_fs_bsize(u_char *name)
{
	struct statfs  fs;

	if (statfs((char *) name, &fs) == -1) 
	{
        	return 512;
	}

	if ((fs.f_bsize % 512) != 0) 
	{
        	return 512;
    	}

	return (size_t) fs.f_bsize;
}

#elif (PUB_HAVE_STATVFS)

size_t
pub_fs_bsize(u_char *name)
{
	struct statvfs  fs;

	if (statvfs((char *) name, &fs) == -1) 
	{
        	return 512;
    	}

	if ((fs.f_frsize % 512) != 0) 
	{
        	return 512;
    	}

	return (size_t) fs.f_frsize;
}

#else

size_t
pub_fs_bsize(u_char *name)
{
	return 512;
}

#endif

