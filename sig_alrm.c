#include	"ping.h"

void
sig_alrm(int signo)
{
	(*pr->fsend)();

	alarm(1);
	return;
}

int
sig_alrm2()
{
	return 1;
}