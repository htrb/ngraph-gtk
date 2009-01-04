#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
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
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(signum, &act, NULL);
}

int
main(int argc,char **argv)
{
  int fdi, fdo, len;
  char *ptr, buf[256] = {0};

  if (argc < 3) {
    goto End;
  }

  if (! isatty(0) || ! isatty(1)) {
    goto End;
  }

  ptr = ttyname(0);
  if (ptr == NULL) {
    goto End;
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
    goto End;;
  }

  len = snprintf(buf, sizeof(buf) - 1, "%d", getpid()) + 1;
  if (write(fdo, buf, len) < 0) {
    close(fdo);
    goto End;;
  }
  
  close(fdo);
  close(fdi);

  unlink(argv[1]);
  unlink(argv[2]);

  my_signal(SIGINT, SIG_IGN);

#ifdef SIGWINCH
  my_signal(SIGWINCH, SIG_IGN);
#endif

  my_signal(SIGCHLD, sig_handler);

  pause();

  return 0;


 End:
  unlink(argv[1]);
  unlink(argv[2]);

  return 1;
}
