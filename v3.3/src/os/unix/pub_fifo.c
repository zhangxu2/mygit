#include "pub_fifo.h"

/**
函数名:pub_fifo_open
功能  :创建或者连接管道 
参数  :
       nFIFOKey    关键字
       nSize       队列长度(0表示默认)
返回值:
      -1         失败
      >0的数     管道描述符号
注意: 管道实现忽略size参数
**/ 
sw_int32_t pub_fifo_open(char *fifofile)
{
	int	fd = 0;
	int	ret = 0;
	struct stat	st;

	memset(&st, 0x0, sizeof(st));

	fd = open(fifofile, O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		if (errno != ENOENT)
		{
			pub_log_error("[%s][%d] Can not open [%s], errno=[%d]:[%s]",
				__FILE__, __LINE__, fifofile, errno, strerror(errno));
			return SW_ERROR;
		}

		ret = mkfifo(fifofile, 0644);
		if (ret)
		{
			pub_log_error("[%s][%d] mkfifo [%s] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, fifofile, errno, strerror(errno));
			return SW_ERROR;
		}

		fd = open(fifofile, O_RDWR | O_NONBLOCK);
		if (fd < 0)
		{
			pub_log_error("[%s][%d] Open fifo [%s] error, errno=[%d]:[%s]",
				__FILE__, __LINE__, fifofile, errno, strerror(errno));
			return SW_ERROR;
		}
	}

	fstat(fd, &st);
	if (!S_ISFIFO(st.st_mode))
	{
		pub_log_error("[%s][%d] The file [%s] isn't FIFO!", __FILE__, __LINE__, fifofile);
		return SW_ERROR;
	}
	
	return fd;
}

sw_int_t pub_fifo_put(int fd)
{
	int	ret = 0;

	ret = check_fd_write(fd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Check fd [%d] error!", __FILE__, __LINE__, fd);
		return -1;
	}
	
	ret = write(fd, "0", 1);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] Write to fifo [%d] error, errno=[%d]:[%s]",
			__FILE__, __LINE__, fd, errno, strerror(errno));
		return -1;
	}
	
	return 0;
}

sw_int_t pub_fifo_get(int fd)
{
	int	ret = 0;
	char	buf[2];
	
	ret = check_fd_read(fd);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Check fd [%d] error!", __FILE__, __LINE__, fd);
		return -1;
	}
	
	memset(buf, 0x0, sizeof(buf));
	ret = read(fd, buf, 1);
	if (ret <= 0)
	{
		pub_log_error("[%s][%d] Read from fifo [%d] error! errno=[%d]:[%s]",
			__FILE__, __LINE__, fd, errno, strerror(errno));
		return -1;
	}

	return 0;
}

sw_int_t check_fd_read(int fd)
{
	int	ret = 0;
	fd_set	rmask;
	fd_set	emask;
	struct timeval	time_out;
	
	while (1)
	{
		FD_ZERO(&rmask);
		FD_ZERO(&emask);
		FD_SET(fd, &rmask);
		FD_SET(fd, &emask);
		time_out.tv_sec  = 0;
		time_out.tv_usec = 0;
		ret = select(fd + 1, &rmask, 0, &emask, &time_out);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			pub_log_error("[%s][%d] select error! fd=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, fd, errno, strerror(errno));
			return -1;
		}
		else if (ret == 0)
		{
			return 0;
		}
	
		if (FD_ISSET(fd, &emask))
		{
			pub_log_error("[%s][%d] Check fd [%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, fd, errno, strerror(errno));
			return -1;
		}
	
		return 1;
	}

}

sw_int_t check_fd_write(int fd)
{
	int	ret = 0;
	fd_set  wmask;
	fd_set  emask;
	struct timeval	time_out;

	while (1)
	{
		FD_ZERO(&wmask);
		FD_ZERO(&emask);
		FD_SET(fd, &wmask);
		FD_SET(fd, &emask);
		time_out.tv_sec  = 0;
		time_out.tv_usec = 0;
		ret = select(fd + 1, NULL, &wmask, &emask, &time_out);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			pub_log_error("[%s][%d] select error, errno=[%d]:[%s]",
				__FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}
		else if (ret == 0)
		{
			return 0;
		}

		if (FD_ISSET(fd, &emask))
		{
			pub_log_error("[%s][%d] Check fd [%d] error! errno=[%d]:[%s]",
				__FILE__, __LINE__, fd, errno, strerror(errno));
			return -1;
		}
		
		return 1;
	}
}

sw_int_t pub_fifo_close(int fd)
{
	return close(fd);
}

sw_int_t pub_fifo_del(char *file)
{
	return unlink(file);
}

