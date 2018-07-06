#include "pub_signal.h"

void pub_signal_nozombie()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
#ifdef SA_NOCLDWAIT
        sa.sa_flags = SA_NOCLDWAIT;
#else
        sa.sa_flags = 0;
#endif
        sigemptyset(&sa.sa_mask);
        sigaction(SIGCHLD, &sa, NULL);
}

void  pub_signal_send(sw_int32_t pid, sw_int32_t sig)
{
	kill(pid,sig);
}
void pub_signal_ignore()
{
	signal(SIGINT,SIG_IGN);
        signal(SIGQUIT,SIG_IGN);
        signal(SIGHUP,SIG_IGN);
        signal(SIGTERM,SIG_IGN);
        signal(SIGALRM,SIG_IGN);
        signal(SIGABRT,SIG_IGN);
        signal(SIGBUS,SIG_IGN);
        signal(SIGPIPE,SIG_IGN);
        signal(SIGCHLD,SIG_IGN);

        return ;
}

