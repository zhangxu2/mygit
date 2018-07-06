#ifndef __PUB_XML_CONVERT_H__
#define __PUB_XML_CONVERT_H__

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdarg.h>

#include "pub_log.h"
#include "pub_vars.h"

SW_PUBLIC sw_int_t pub_xml_convert_integrate( sw_loc_vars_t *pstlocvar, sw_xmltree_t *pxml);
SW_PUBLIC sw_int_t pub_xml_convert_analyze( sw_loc_vars_t *pstlocvar, sw_xmltree_t *pxml);

#endif
