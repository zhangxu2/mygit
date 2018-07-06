#ifndef __PUB_FIFO_H__
#define __PUB_FIFO_H__

#include "pub_type.h"
#include "pub_log.h"

SW_PUBLIC sw_int32_t pub_fifo_open(char *fifofile);
SW_PUBLIC sw_int_t pub_fifo_clear(int fifoid,char *file);
SW_PUBLIC sw_int_t pub_fifo_put(int fd );
SW_PUBLIC sw_int_t pub_fifo_get(int fifofd);
SW_PUBLIC sw_int_t pub_fifo_close(int fifofd);
SW_PUBLIC sw_int_t pub_fifo_del(char *file);
SW_PUBLIC sw_int_t check_fd_read(int fd);
SW_PUBLIC sw_int_t check_fd_write(int fd);


#endif
