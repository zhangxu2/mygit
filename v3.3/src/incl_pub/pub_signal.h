#ifndef __PUB_SIGNAL_H__
#define __PUB_SIGNAL_H__

#include "pub_type.h"
#include <signal.h>

void pub_signal_nozombie();
void pub_signal_ignore();
void  pub_signal_send(sw_int32_t pid, sw_int32_t sig);
#endif

