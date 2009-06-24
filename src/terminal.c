#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
sig_handler(int sig)
{
}

static void
my_signal(int signum, void (* sighandler))
{
  struct sigaction act;

  act.sa_handler = sighandler;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);
  sigaction(signum, &act, NULL);
}

int
main(int argc,char **argv)
{
  int fdi, fdo, len;
  char *ptr, buf[256] = {0};
#ifdef HAVE_SIGSUSPEND
  sigset_t sig_mask;
#endif

  if (argc < 3) {
    return 1;
  }

  if (! isatty(0) || ! isatty(1)) {
    return 1;
  }

  ptr = ttyname(0);
  if (ptr == NULL) {
    return 1;
  }

  fdo = open(argv[1], O_WRONLY);
  fdi = open(argv[2], O_RDONLY);

  while (1) {
    int n;
    n = read(fdi,buf,1);
    if (n != 1) {
      perror("");
      break;
    }
    if (buf[0] == '\0')
      break;
    putchar(buf[0]);
  }
  close(fdi);

  len = strlen(ptr) + 1;
  if (write(fdo, ptr, len) < 0) {
    close(fdo);
    return 1;
  }

  len = snprintf(buf, sizeof(buf) - 1, "%d", getpid()) + 1;
  if (write(fdo, buf, len) < 0) {
    close(fdo);
    return 1;;
  }
  
  close(fdo);
  close(fdi);

  my_signal(SIGTERM, sig_handler);

#ifdef HAVE_SIGSUSPEND

  sigemptyset(&sig_mask);
  sigaddset(&sig_mask, SIGINT);
#ifdef SIGWINCH
  sigaddset(&sig_mask, SIGWINCH);
#endif
  sigsuspend(&sig_mask);

#else  /* HAVE_SIGSUSPEND */

  my_signal(SIGINT, SIG_IGN);
#ifdef SIGWINCH
  my_signal(SIGWINCH, SIG_IGN);
#endif
  pause();

#endif /* HAVE_SIGSUSPEND */

  return 0;
}
