/*
 * $Id: shellux.c,v 1.9 2010-03-04 08:30:16 hito Exp $
 *
 * This file is part of "Ngraph for X11".
 *
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 *
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * "Ngraph for X11" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "nstring.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "shell_error.h"
#include "shellux.h"

int
cmtrue(struct nshell *nshell,int argc,char **argv)
{
  return 0;
}

int
cmfalse(struct nshell *nshell,int argc,char **argv)
{
  return 1;
}

int
cmsleep(struct nshell *nshell,int argc,char **argv)
{
  double a;
  char *arg,*endptr;

  if (argc<2) {
    sherror4(argv[0],ERRSMLARG);
    return ERRSMLARG;
  }
  arg=argv[1];
  a=strtod(arg, &endptr);
  if (endptr[0]!='\0') {
    sherror3(argv[0],ERRNUMERIC,arg);
    return ERRNUMERIC;
  }

  nsleep(a);

  return 0;
}

static int
testexpand(int pre,int *oppo,int *numpo,
	   int numbuf[], char *numbufc[], const char opbuf[], const int prebuf[])
{
  int argnum;
  int d1,d2;
  char *endptr1,*endptr2;
  GStatBuf buf;

  while ((*oppo>=0) && (pre<=prebuf[*oppo])) {
    if (strchr("!nzdefrswx",opbuf[*oppo])!=NULL) argnum=1;
    else argnum=2;
    switch (opbuf[*oppo]) {
    case 'n':
      if (numbufc[*numpo]==NULL) return FALSE;
      numbuf[*numpo]=(numbufc[*numpo][0]=='\0')?FALSE:TRUE;
      numbufc[*numpo]=NULL;
      break;
    case 'z':
      if (numbufc[*numpo]==NULL) return FALSE;
      numbuf[*numpo]=(numbufc[*numpo][0]=='\0')?TRUE:FALSE;
      numbufc[*numpo]=NULL;
      break;
    case 'd':
    case 'f':
    case 'e':
    case 'r':
    case 'w':
    case 'x':
    case 's':
      if (numbufc[*numpo]==NULL) return FALSE;
      if (nstat(numbufc[*numpo],&buf)!=0) {
        numbuf[*numpo]=FALSE;
        numbufc[*numpo]=NULL;
        break;
      }
      switch (opbuf[*oppo]) {
      case 'd':
        numbuf[*numpo]=((buf.st_mode & S_IFMT)==S_IFDIR)?TRUE:FALSE;
        break;
      case 'f':
        numbuf[*numpo]=((buf.st_mode & S_IFMT)==S_IFREG)?TRUE:FALSE;
        break;
      case 'e':
        numbuf[*numpo]=TRUE;
        break;
      case 'r':
        numbuf[*numpo]=((buf.st_mode & S_IREAD)!=0)?TRUE:FALSE;
        break;
      case 'w':
        numbuf[*numpo]=((buf.st_mode & S_IWRITE)!=0)?TRUE:FALSE;
        break;
      case 'x':
        numbuf[*numpo]=((buf.st_mode & S_IEXEC)!=0)?TRUE:FALSE;
        break;
      case 's':
        numbuf[*numpo]=(buf.st_size>0)?TRUE:FALSE;
        break;
      }
      numbufc[*numpo]=NULL;
      break;
    case '!':
      numbuf[*numpo]=(numbuf[*numpo])?FALSE:TRUE;
      numbufc[*numpo]=NULL;
      break;
    case 'o':
      numbuf[*numpo-1]=numbuf[*numpo-1] || numbuf[*numpo];
      numbufc[*numpo]=NULL;
      break;
    case 'a':
      numbuf[*numpo-1]=numbuf[*numpo-1] && numbuf[*numpo];
      numbufc[*numpo]=NULL;
      break;
    case '?':
    case '*':
    case '>':
    case '}':
    case '<':
    case '{':
      if ((numbufc[*numpo-1]==NULL) || (numbufc[*numpo]==NULL)) return FALSE;
      d1=strtol(numbufc[*numpo-1],&endptr1,10);
      d2=strtol(numbufc[*numpo],&endptr2,10);
      if ((endptr1[0]!='\0') || (endptr2[0]!='\0')) return FALSE;
      switch (opbuf[*oppo]) {
      case '?':
        numbuf[*numpo-1]=(d1==d2)?TRUE:FALSE;
        break;
      case '*':
        numbuf[*numpo-1]=(d1!=d2)?TRUE:FALSE;
        break;
      case '>':
        numbuf[*numpo-1]=(d1>d2)?TRUE:FALSE;
        break;
      case '}':
        numbuf[*numpo-1]=(d1>=d2)?TRUE:FALSE;
        break;
      case '<':
        numbuf[*numpo-1]=(d1<d2)?TRUE:FALSE;
        break;
      case '{':
        numbuf[*numpo-1]=(d1<=d2)?TRUE:FALSE;
        break;
      }
      numbufc[*numpo-1]=NULL;
      break;
    case '=':
      if ((numbufc[*numpo-1]==NULL) || (numbufc[*numpo]==NULL)) return FALSE;
      numbuf[*numpo-1]
      =(strcmp2(numbufc[*numpo-1],numbufc[*numpo])==0)?TRUE:FALSE;
      numbufc[*numpo-1]=NULL;
      break;
    case '+':
      if ((numbufc[*numpo-1]==NULL) || (numbufc[*numpo]==NULL)) return FALSE;
      numbuf[*numpo-1]
      =(strcmp2(numbufc[*numpo-1],numbufc[*numpo])==0)?FALSE:TRUE;
      numbufc[*numpo-1]=NULL;
      break;
    }
    (*numpo)+=-argnum+1;
    (*oppo)--;
  }
  return TRUE;
}

int
cmtest(struct nshell *nshell,int argc,char **argv)
{
  int prebuf[20];
  int numbuf[20];
  char *numbufc[20];
  char opbuf[20];
  int oppo,numpo,numeric;
  int i;

  if (argc<2) return 0;
  numpo=-1;
  oppo=-1;
  numeric=FALSE;
  if (strcmp2(argv[argc-1],"]")==0) argc--;
  i=1;
  while (i<argc) {
    if (!numeric) {
      if ((strcmp2(argv[i],"!")==0) && ((i+1)<argc)) {
        if (!testexpand(4,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
          sherror4(argv[0],ERRTESTSYNTAX);
          return 1;
        }
        oppo++;
        if (oppo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        opbuf[oppo]='!';
        prebuf[oppo]=4;
        i++;
      } else if (((strcmp2(argv[i],"-d")==0) || (strcmp2(argv[i],"-e")==0)
		  || (strcmp2(argv[i],"-f")==0) || (strcmp2(argv[i],"-r")==0)
		  || (strcmp2(argv[i],"-s")==0) || (strcmp2(argv[i],"-w")==0)
		  || (strcmp2(argv[i],"-x")==0) || (strcmp2(argv[i],"-z")==0)
		  || (strcmp2(argv[i],"-n")==0))
		 && ((i+1)<argc)
		 && (strcmp2(argv[i+1],"=")!=0) && (strcmp2(argv[i+1],"!=")!=0)) {
	if (!testexpand(6,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
          sherror4(argv[0],ERRTESTSYNTAX);
          return 1;
        }
        oppo++;
        if (oppo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        opbuf[oppo]=argv[i][1];
        prebuf[oppo]=6;
        i++;
      } else if ((strcmp2(argv[i],"(")==0) && ((i+1)<argc)) {
        oppo++;
        if (oppo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        opbuf[oppo]='(';
        prebuf[oppo]=0;
        i++;
      } else {
        numpo++;
        if (numpo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        numbuf[numpo]=strlen(argv[i]);
        numbufc[numpo]=argv[i];
        numeric=TRUE;
        i++;
      }
    } else {
      if (((strcmp2(argv[i],"-eq")==0) || (strcmp2(argv[i],"-ne")==0)
      || (strcmp2(argv[i],"-gt")==0)  || (strcmp2(argv[i],"-ge")==0)
      || (strcmp2(argv[i],"-lt")==0)  || (strcmp2(argv[i],"-le")==0)
      || (strcmp2(argv[i],"=")==0)  || (strcmp2(argv[i],"!=")==0))
      && ((i+1)<argc)) {
        if (!testexpand(5,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
          sherror4(argv[0],ERRTESTSYNTAX);
          return 1;
        }
        oppo++;
        if (oppo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        if (strcmp2(argv[i],"-eq")==0) opbuf[oppo]='?';
        else if (strcmp2(argv[i],"-ne")==0) opbuf[oppo]='*';
        else if (strcmp2(argv[i],"-gt")==0) opbuf[oppo]='>';
        else if (strcmp2(argv[i],"-ge")==0) opbuf[oppo]='}';
        else if (strcmp2(argv[i],"-lt")==0) opbuf[oppo]='<';
        else if (strcmp2(argv[i],"-le")==0) opbuf[oppo]='{';
        else if (strcmp2(argv[i],"=")==0) opbuf[oppo]='=';
        else if (strcmp2(argv[i],"!=")==0) opbuf[oppo]='+';
        prebuf[oppo]=5;
        numeric=FALSE;
        i++;
      } else if (strcmp2(argv[i],")")==0) {
        if (!testexpand(1,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
          sherror4(argv[0],ERRTESTSYNTAX);
          return 1;
        }
        if ((oppo!=-1) && (opbuf[oppo]=='(')) oppo--;
        numeric=TRUE;
        i++;
      } else if ((strcmp2(argv[i],"-a")==0) && ((i+1)<argc)) {
        if (!testexpand(3,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
          sherror4(argv[0],ERRTESTSYNTAX);
          return 1;
        }
        oppo++;
        if (oppo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        opbuf[oppo]=argv[i][1];
        prebuf[oppo]=3;
        numeric=FALSE;
        i++;
      } else if ((strcmp2(argv[i],"-o")==0) && ((i+1)<argc)) {
        if (!testexpand(2,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
          sherror4(argv[0],ERRTESTSYNTAX);
          return 1;
        }
        oppo++;
        if (oppo>=20) {
          sherror4(argv[0],ERRTESTNEST);
          return 1;
        }
        opbuf[oppo]=argv[i][1];
        prebuf[oppo]=2;
        numeric=FALSE;
        i++;
      } else if (strcmp2(argv[i],"]")==0) {
        i++;
      } else {
        sherror4(argv[0],ERRTESTSYNTAX);
        return 1;
      }
    }
  }
  if (!numeric || !testexpand(1,&oppo,&numpo,numbuf,numbufc,opbuf,prebuf)) {
    sherror4(argv[0],ERRTESTSYNTAX);
    return 1;
  }
  if ((oppo>-1) || (numpo!=0)) {
    sherror4(argv[0],ERRTESTSYNTAX);
    return 1;
  }
  if (numbuf[0]) return 0;
  else return 1;
}
