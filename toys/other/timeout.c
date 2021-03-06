/* timeout.c - Run command line with a timeout
 *
 * Copyright 2013 Rob Landley <rob@landley.net>
 *
 * No standard

USE_TIMEOUT(NEWTOY(timeout, "<2^vk:s: ", TOYFLAG_BIN))

config TIMEOUT
  bool "timeout"
  default y
  depends on TOYBOX_FLOAT
  help
    usage: timeout [-k LENGTH] [-s SIGNAL] LENGTH COMMAND...

    Run command line as a child process, sending child a signal if the
    command doesn't exit soon enough.

    Length can be a decimal fraction. An optional suffix can be "m"
    (minutes), "h" (hours), "d" (days), or "s" (seconds, the default).

    -s	Send specified signal (default TERM)
    -k	Send KILL signal if child still running this long after first signal
    -v	Verbose
*/

#define FOR_timeout
#include "toys.h"

GLOBALS(
  char *s_signal;
  char *k_timeout;

  int nextsig;
  pid_t pid;
  struct timeval ktv;
  struct itimerval itv;
)

static void handler(int i)
{
  if (toys.optflags & FLAG_v)
    fprintf(stderr, "timeout pid %d signal %d\n", TT.pid, TT.nextsig);
  kill(TT.pid, TT.nextsig);
  
  if (TT.k_timeout) {
    TT.k_timeout = 0;
    TT.nextsig = SIGKILL;
    xsignal(SIGALRM, handler);
    TT.itv.it_value = TT.ktv;
    setitimer(ITIMER_REAL, &TT.itv, (void *)toybuf);
  }
}

// timeval inexplicably makes up a new type for microseconds, despite timespec's
// nanoseconds field (needing to store 1000* the range) using "long". Bravo.
void xparsetimeval(char *s, struct timeval *tv)
{
  long ll;

  tv->tv_sec = xparsetime(s, 6, &ll);
  tv->tv_usec = ll;
}

void timeout_main(void)
{
  // Parse early to get any errors out of the way.
  xparsetimeval(*toys.optargs, &TT.itv.it_value);
  if (TT.k_timeout) xparsetimeval(TT.k_timeout, &TT.ktv);

  TT.nextsig = SIGTERM;
  if (TT.s_signal && -1 == (TT.nextsig = sig_to_num(TT.s_signal)))
    error_exit("bad -s: '%s'", TT.s_signal);

  if (!(TT.pid = XVFORK())) xexec(toys.optargs+1);
  else {
    xsignal(SIGALRM, handler);
    setitimer(ITIMER_REAL, &TT.itv, (void *)toybuf);
    toys.exitval = xwaitpid(TT.pid);
  }
}
