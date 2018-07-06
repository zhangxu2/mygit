#ifndef __PUB_XSD_H__
#define __PUB_XSD_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include "pub_string.h"
#include "pub_xml.h"
#include "pub_regex.h"

sw_int_t pub_xsd_check(char *sFileName, char *sBuffer);
extern sw_int_t pub_xsd_check_glob();
extern sw_int_t pub_xsd_check_mprdt();
extern sw_int_t pub_xsd_check_chan();
extern sw_int_t pub_xsd_check_mlsn();
extern sw_int_t pub_xsd_check_prdt(char *prdtname);

#endif

