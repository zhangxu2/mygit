#ifndef __PUB_FILTER_H__
#define __PUB_FILTER_H__

#include <unistd.h>

extern int pub_filter_init();
extern int pub_is_filter(char *name);
extern int pub_filter_free();

#endif
