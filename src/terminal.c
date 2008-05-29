#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc,char **argv)
{
  int fdi[2],fdo[2],len;
  char buf[1];

  if (argc<5) return 0;
  fdo[0]=atoi(argv[1]);
  fdo[1]=atoi(argv[2]);
  fdi[0]=atoi(argv[3]);
  fdi[1]=atoi(argv[4]);
  close(fdo[0]);
  close(fdi[1]);
  if (!isatty(0) || !isatty(1)) {
    close(fdo[1]);
    close(fdi[1]);
    return 1;
  }
  while ((read(fdi[0],buf,1)==1) && (buf[0]!='\0')) {
    printf("%c",buf[0]);
  }
  len=strlen(ttyname(0));
  write(fdo[1],ttyname(0),len+1);
  close(fdo[1]);
  signal(SIGINT,SIG_IGN);
  read(fdi[0],buf,1);
  close(fdi[0]);
  return 0;
}
