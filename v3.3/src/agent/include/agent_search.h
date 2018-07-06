#ifndef __SW_SEACH_H__
#define __SW_SEACH_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include "pub_type.h"
#include "pub_log.h"

#define PATH_SIZE 512
#define FILE_SIZE 512
#define LINE_SIZE 1024*4
#define NODE_SIZE 512
#define CNT 6
#define FILE_NUM	16
#define NO 0
#define FIND 1

int get_file(char *log_dir, char *log_file, char *chl);
int rand_file();
int get_log(char *date, char *fls);
int log_search(char *chl, char *cdate, char *ctime, char *fls);

#endif
