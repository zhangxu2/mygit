#ifndef __PUB_FILE_H__
#define __PUB_FILE_H__

#include <sys/stat.h>
#include "pub_type.h"
#include "pub_errno.h"
#include "pub_string.h"

typedef struct sw_file_s sw_file_t;
struct sw_file_s
{
	sw_fd_t		fd;
	sw_str_t	name;
	sw_file_info_t	info;
	
	off_t		offset;
	off_t		sys_offset;

	unsigned	valid_info:1;
	unsigned	directio:1;
};


typedef time_t (*sw_path_manager_pt) (void *data);
typedef void (*sw_path_loader_pt) (void *data);

typedef struct
{
	sw_str_t		name;
	size_t			len;
	size_t			level[3];

	sw_path_manager_pt	manager;
	sw_path_loader_pt	loader;
	void			*data;

	u_char			*conf_file;
	sw_uint_t		line;
} sw_path_t;

typedef struct
{
	DIR		*dir;
	struct dirent	*de;
	sw_file_info_t	info;

	unsigned	type:8;
	unsigned	valid_info:1;
	
} sw_dir_t;

typedef struct
{
	size_t		n;
	glob_t		pglob;
	u_char		*pattern;
	
	sw_uint_t	test;
}sw_glob_t;

#define pub_file_open(name,mode,create,access) \
	open((const char *)name,mode|create,access)

#define pub_file_open_n		"open()"
#define pub_file_close		close
#define pub_file_close_n	"close()"
#define pub_file_delete(name)	unlink((const char *) name)
#define pub_file_delete_n	"unlink()"

ssize_t pub_file_read(sw_file_t *file, u_char *buf, size_t size, off_t offset);
#if (SW_HAVE_PREAD)
#define pub_file_read_n		"pread()"
#else
#define pub_file_read_n		"read()"
#endif

ssize_t pub_file_write(sw_file_t *file, u_char *buf, size_t size,off_t offset);

#define pub_read_fd		read
#define pub_read_fd_n		"read()"

#define pub_write_fd_n		"write()"
#define pub_write_console	pub_write_fd

#define pub_linefeed(p)          *p++ = LF;
#define PUB_LINEFEED_SIZE        1
#define PUB_LINEFEED             "\x0a"


#define pub_file_rename(o, n)	rename((const char *) o, (const char *) n)
#define pub_file_rename_n	"rename()"

#define pub_file_change_access(n, a)	chmod((const char *) n, a)
#define pub_file_change_access_n	"chmod()"

sw_int_t pub_file_settime(u_char *name, sw_fd_t fd, time_t s);
#define pub_file_settime_n	"utimes()"


#define pub_file_info(file, sb)	stat((const char *) file, sb)
#define pub_file_info_n          "stat()"

#define pub_fd_info(fd, sb)      fstat(fd, sb)
#define pub_fd_info_n            "fstat()"

#define pub_link_info(file, sb)  lstat((const char *) file, sb)
#define pub_link_info_n          "lstat()"

#define pub_is_dir(sb)           (S_ISDIR((sb)->st_mode))
#define pub_is_file(sb)          (S_ISREG((sb)->st_mode))
#define pub_is_link(sb)          (S_ISLNK((sb)->st_mode))
#define pub_is_exec(sb)          (((sb)->st_mode & S_IXUSR) == S_IXUSR)
#define pub_file_access(sb)      ((sb)->st_mode & 0777)
#define pub_file_size(sb)        (sb)->st_size
#define pub_file_fs_size(sb)     ((sb)->st_blocks * 512)
#define pub_file_mtime(sb)       (sb)->st_mtime
#define pub_file_uniq(sb)        (sb)->st_ino

#define pub_realpath(p, r)       realpath((char *) p, (char *) r)
#define pub_realpath_n           "realpath()"
#define pub_getcwd(buf, size)    (getcwd((char *) buf, size) != NULL)
#define pub_getcwd_n             "getcwd()"
#define pub_path_separator(c)    ((c) == '/')

#define PUB_MAX_PATH             PATH_MAX 
#define PUB_DIR_MASK_LEN         0

sw_int_t pub_open_dir(sw_str_t *name, sw_dir_t *dir);
#define pub_open_dir_n           "opendir()"


#define pub_close_dir(d)         closedir((d)->dir)
#define pub_close_dir_n          "closedir()"

sw_int_t pub_read_dir(sw_dir_t *dir);
#define pub_read_dir_n           "readdir()"


#define pub_create_dir(name, access) mkdir((const char *) name, access)
#define pub_create_dir_n         "mkdir()"


#define pub_delete_dir(name)     rmdir((const char *) name)
#define pub_delete_dir_n         "rmdir()"

#define pub_change_dir(path)	chdir((const char*) path)

#define pub_dir_access(a)        (a | (a & 0444) >> 2)


#define pub_de_name(dir)         ((u_char *) (dir)->de->d_name)
#if (PUB_HAVE_D_NAMLEN)
#define pub_de_namelen(dir)      (dir)->de->d_namlen
#else
#define pub_de_namelen(dir)      strlen((dir)->de->d_name)
#endif

#define pub_de_info_n            "stat()"
#define pub_de_link_info(name, dir)  lstat((const char *) name, &(dir)->info)
#define pub_de_link_info_n       "lstat()"

#if (PUB_HAVE_D_TYPE)
/*
 * some file systems (e.g. XFS on Linux and CD9660 on FreeBSD)
 * do not set dirent.d_type
 */
#define pub_de_is_dir(dir)                                                   \
    (((dir)->type) ? ((dir)->type == DT_DIR) : (S_ISDIR((dir)->info.st_mode)))
#define pub_de_is_file(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_REG) : (S_ISREG((dir)->info.st_mode)))
#define pub_de_is_link(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_LNK) : (S_ISLNK((dir)->info.st_mode)))

#else

#define pub_de_is_dir(dir)       (S_ISDIR((dir)->info.st_mode))
#define pub_de_is_file(dir)      (S_ISREG((dir)->info.st_mode))
#define pub_de_is_link(dir)      (S_ISLNK((dir)->info.st_mode))

#endif

#define pub_de_access(dir)       (((dir)->info.st_mode) & 0777)
#define pub_de_size(dir)         (dir)->info.st_size
#define pub_de_fs_size(dir)      ((dir)->info.st_blocks * 512)
#define pub_de_mtime(dir)        (dir)->info.st_mtime

sw_int_t pub_open_glob(sw_glob_t *gl);
#define pub_open_glob_n          "glob()"
sw_int_t pub_read_glob(sw_glob_t *gl, sw_str_t *name);
void pub_close_glob(sw_glob_t *gl);

sw_err_t pub_trylock_fd(sw_fd_t fd);
sw_err_t pub_lock_fd(sw_fd_t fd);
sw_err_t pub_unlock_fd(sw_fd_t fd);
sw_int_t pub_file_exist(sw_char_t * filename);
sw_int_t pub_file_check_dir(sw_char_t *path);
sw_int_t aix_mkdirp(char *path, int mode);

#define pub_trylock_fd_n         "fcntl(F_SETLK, F_WRLCK)"
#define pub_lock_fd_n            "fcntl(F_SETLKW, F_WRLCK)"
#define pub_unlock_fd_n          "fcntl(F_SETLK, F_UNLCK)"

size_t pub_fs_bsize(u_char *name);


#define pub_set_stderr(fd)       dup2(fd, STDERR_FILENO)
#define pub_set_stderr_n         "dup2(STDERR_FILENO)"

char *get_last_name(char *fullname, char *name);

#endif

