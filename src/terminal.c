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

int
main(int argc,char **argv)
{
  int fdi, fdo, len;
  char *ptr, buf[256] = {0};
  struct sigaction act;

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

  len = strlen(ptr);
  if (write(fdo, ptr, len + 1) < 0) {
    close(fdo);
    close(fdi);
    goto End;;
  }

  len = snprintf(buf, sizeof(buf) - 1, "%d", getpid());
  if (write(fdo, buf, len + 1) < 0) {
    close(fdo);
    close(fdi);
    goto End;;
  }
  
  close(fdo);
  close(fdi);

  unlink(argv[1]);
  unlink(argv[2]);

  act.sa_handler = SIG_IGN;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, NULL);

#ifdef SIGWINCH
  act.sa_handler = SIG_IGN;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGWINCH, &act, NULL);
#endif

  act.sa_handler = sig_handler;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGCHLD, &act, NULL);

  pause();

  return 0;


 End:
  unlink(argv[1]);
  unlink(argv[2]);

  return 1;
}
