/*
 * $Id: shellcm.c,v 1.29 2010-03-04 08:30:16 hito Exp $
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
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <math.h>

#define USE_HASH 1

#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "mathfn.h"
#include "shell.h"
#include "shell_error.h"
#include "shellcm.h"

#include "math/math_equation.h"

#define ERR 128

int
cmcd(struct nshell *nshell,int argc,char **argv)
{
  char *home;

  if (argv[1]!=NULL) {
    if (nchdir(argv[1])!=0) {
      sherror3(argv[0],ERRNODIR,argv[1]);
      return ERR;
    }
  } else {
    if ((home=getval(nshell,"HOME"))!=NULL) {
      if (nchdir(home)!=0) {
        sherror3(argv[0],ERRNODIR,home);
        return ERR;
      }
    }
  }
  return 0;
}

int
cmecho(struct nshell *nshell,int argc,char **argv)
{
  int i, nbr;

  nbr = (argc > 1 && strcmp(argv[1], "-n") == 0);

  for (i = (nbr) ? 2 : 1; i < argc; i++) {
    printfstdout("%s",argv[i]);
    if (i != (argc - 1))
      printfstdout(" ");
  }

  if (! nbr)
    putstdout("");

  return 0;
}

int
cmbasename(struct nshell *nshell,int argc,char **argv)
{
  char *bname;

  if (argc < 2) {
    return 1;
  }

  if (argv[1] == NULL) {
    return 1;
  }

  bname = getbasename(argv[1]);
  if (bname == NULL) {
    return 1;
  }

  if (argc > 2) {
    int len, ext_len;
    len = strlen(bname);
    ext_len = strlen(argv[2]);

    if (ext_len < len && strcmp(bname + len - ext_len, argv[2]) == 0) {
      bname[len - ext_len] = '\0';
    }
  }

  putstdout(bname);
  g_free(bname);

  return 0;
}

int
cmdirname(struct nshell *nshell,int argc,char **argv)
{
  char *tmp;

  if (argc < 2) {
    return 1;
  }

  if (argv[1] == NULL)
    return 1;

  tmp = getdirname(argv[1]);

  putstdout(tmp);
  g_free(tmp);

  return 0;
}

int
cmseq(struct nshell *nshell, int argc, char **argv)
{
  int i, f, l;
  double x, first, last, inc;
  char *endptr;

  switch (argc) {
  case 0:  case 1:
    sherror4(argv[0], ERRSMLARG);
    return ERR;
  case 2:
    f = 0;
    i = 0;
    l = 1;
    first = 1.0;
    inc = 1.0;
    break;
  case 3:
    f = 1;
    i = 0;
    l = 2;
    inc = 1.0;
    break;
  default:
    f = 1;
    i = 2;
    l = 3;
    break;
  }

  if (f) {
    errno = 0;
    first= strtod(argv[f], &endptr);
    if (errno || endptr == argv[f]) {
      sherror4(argv[0], ERRNUMERIC);
      return ERR;
    }
  }

  if (l) {
    errno = 0;
    last= strtod(argv[l], &endptr);
    if (errno || endptr == argv[l]) {
      sherror4(argv[0], ERRNUMERIC);
      return ERR;
    }
  }

  if (i) {
    errno = 0;
    inc= strtod(argv[i], &endptr);
    if (errno || endptr == argv[i]) {
      sherror4(argv[0], ERRNUMERIC);
      return ERR;
    }
  }

  for (x = first; (inc < 0) ? (x >= last) : (x <= last); x += inc) {
    printfstdout("%G\n", x);
  }
  return 0;
}

int
cmeval(struct nshell *nshell,int argc,char **argv)
{
  GString *s;
  int i,rcode;

  s = g_string_sized_new(128);
  if (s == NULL) {
    return ERR;
  }

  for (i = 1; i < argc; i++) {
    g_string_append(s, argv[i]);
    g_string_append_c(s, ' ');
  }

  rcode = cmdexecute(nshell, s->str);
  g_string_free(s, TRUE);

  if (rcode != 0 && rcode != 1) {
    return ERR;
  }

  return 0;
}

int
cmexit(struct nshell *nshell,int argc,char **argv)
{
  int a;
  char *endptr;

  if (argc>2) {
    sherror4(argv[0],ERREXTARG);
    return ERREXTARG;
  } else if (argc==2) {
    a=strtol(argv[1],&endptr,10);
    if (endptr[0]!='\0') {
      sherror3(argv[0],ERRNUMERIC,argv[1]);
      return ERRNUMERIC;
    } else {
      nshell->quit=TRUE;
      return a;
    }
  } else {
    nshell->quit=TRUE;
    return nshell->status;
  }
}

#if USE_HASH
static int
printexp(struct nhash *h, void *data)
{
  printfstdout("%.256s\n", h->key);
  return 0;
}
#endif

int
cmexport(struct nshell *nshell,int argc,char **argv)
{
#if USE_HASH
  if (argv[1] == NULL) {
    nhash_each(nshell->exproot, printexp, NULL);
  } else {
    int i;
    for (i = 1; i < argc; i++) {
      if (addexp(nshell, argv[i]) == NULL)
	return ERR;
    }
  }
  return 0;
#else
  struct explist *valcur;

  if (argv[1]==NULL) {
    valcur=nshell->exproot;
    while (valcur!=NULL) {
      printfstdout("%.256s\n",valcur->val);
      valcur=valcur->next;
    }
    return 0;
  } else {
    int i;
    for (i=1;i<argc;i++)
      if (addexp(nshell,argv[i])==NULL) return ERR;
    return 0;
  }
#endif
}

int
cmpwd(struct nshell *nshell,int argc,char **argv)
{
  char *s;

  s = ngetcwd();
  if (s == NULL) {
    return ERR;
  }

  putstdout(s);
  g_free(s);

  return 0;
}

static int
print_val(struct nhash *h, void *data)
{
  struct vallist *val;

  val = (struct vallist *) h->val.p;
  if (!val->func)
    printfstdout("%.256s=%.256s\n", val->name, (char *)(val->val));

  return 0;
}

static int
print_func(struct nhash *h, void *data)
{
  struct vallist *val;
  struct cmdlist *cmdcur;
  struct prmlist *prmcur;

  val = (struct vallist *) h->val.p;
  if (val->func) {
    printfstdout("%.256s=()\n", val->name);
    putstdout("{");
    cmdcur = val->val;
    while (cmdcur) {
      prmcur = cmdcur->prm;
      printfstdout("    ");
      while (prmcur) {
	if (prmcur->str)
	  printfstdout("%.256s ", prmcur->str);
	prmcur = prmcur->next;
      }
      printfstdout("\n");
      cmdcur = cmdcur->next;
    }
    putstdout("}");
  }
  return 0;
}

int
cmset(struct nshell *nshell,int argc,char **argv)
{
#if USE_HASH
  char *s;
  int j, r, ops;

  if (argc < 2) {
    nhash_each(nshell->valroot, print_val, NULL);
    nhash_each(nshell->valroot, print_func, NULL);
    return 0;
  }
  for (j = 1; j < argc; j++) {
    s = argv[j];
    if (s[0] == '-' && s[1] == '-' && s[2] == '\0') {
      j++;
      if (j == argc) {
	r = set_shell_args(nshell, j, nshell->argv[0], argc, argv);
	if (r) {
	  r = ERR;
	}
	return r;
      }
      break;
    } else if (s[0] == '-' && s[1] == '\0') {
      nshell->optionv = FALSE;
      nshell->optionx = FALSE;
      j++;
      break;
    } else if (s[0] == '-' || s[0] == '+') {
      if (s[1] == '\0' || strchr("efvx", s[1]) == NULL) {
	sherror3(argv[0], ERRILOPS, s);
	return ERRILOPS;
      }
    } else {
      break;
    }
  }
  if (j != argc) {
    r = set_shell_args(nshell, j, nshell->argv[0], argc, argv);
    if (r) {
      return ERR;
    }
  }
  for (j = 1 ; j < argc; j++) {
    s = argv[j];
    if (s[0] == '-') {
      ops=TRUE;
    } else if (s[0]=='+') {
      ops=FALSE;
    } else {
      break;
    }
    switch (s[1]) {
    case 'e':
      nshell->optione = ops;
      break;
    case 'f':
      nshell->optionf = ops;
      break;
    case 'v':
      nshell->optionv = ops;
      break;
    case 'x':
      nshell->optionx = ops;
      break;
    }
  }
#else
  struct vallist *valcur;
  struct cmdlist *cmdcur;
  struct prmlist *prmcur;
  char *s;
  unsigned int n;
  int j,ops;
  char **argv2;
  int argc2;

  if (argc<2) {
    valcur=nshell->valroot;
    while (valcur!=NULL) {
      if (!valcur->func)
        printfstdout("%.256s=%.256s\n",valcur->name,(char *)(valcur->val));
      valcur=valcur->next;
    }
    valcur=nshell->valroot;
    while (valcur!=NULL) {
      if (valcur->func) {
        printfstdout("%.256s=()\n",valcur->name);
        putstdout("{");
        cmdcur=valcur->val;
        while (cmdcur!=NULL) {
          prmcur=cmdcur->prm;
          printfstdout("    ");
          while (prmcur!=NULL) {
            if (prmcur->str!=NULL) printfstdout("%.256s ",prmcur->str);
            prmcur=prmcur->next;
          }
          printfstdout("\n");
          cmdcur=cmdcur->next;
        }
        putstdout("}");
      }
      valcur=valcur->next;
    }
  } else {
    for (j=1;j<argc;j++) {
      s=argv[j];
      if ((s[0]=='-') && (s[1]=='-')) {
	n = strlen(argv[j]);
#if 0
        for (i = 1; i <= n; i++) {
	  argv[j][i-1]=argv[j][i];
	}
#else
	memmove(argv[j], argv[j] + 1, sizeof(**argv) * n);
#endif
        break;
      } else if ((s[0]=='-') || (s[0]=='+')) {
        if ((s[1]=='\0') || (strchr("efvx",s[1])==NULL)) {
          sherror3(argv[0],ERRILOPS,s);
          return ERRILOPS;
        }
      } else break;
    }
    if (j!=argc) {
      argv2=NULL;
      if ((s=g_malloc(strlen((nshell->argv)[0])+1))==NULL) return ERR;
      strcpy(s,(nshell->argv)[0]);
      if (arg_add(&argv2,s)==NULL) {
        g_free(s);
        arg_del(argv2);
        return ERR;
      }
      for (;j<argc;j++) {
        if ((s=g_malloc(strlen(argv[j])+1))==NULL) return ERR;
        strcpy(s,argv[j]);
        if (arg_add(&argv2,s)==NULL) {
          g_free(s);
          arg_del(argv2);
          return ERR;
        }
      }
      argc2=getargc(argv2);
      arg_del(nshell->argv);
      nshell->argv=argv2;
      nshell->argc=argc2;
    }
    for (j=1;j<argc;j++) {
      s=argv[j];
      if (s[0]=='-') ops=TRUE;
      else if (s[0]=='+') ops=FALSE;
      else break;
      switch (s[1]) {
      case 'e':
        nshell->optione=ops;
        break;
      case 'f':
        nshell->optionf=ops;
        break;
      case 'v':
        nshell->optionv=ops;
        break;
      case 'x':
        nshell->optionx=ops;
        break;
      }
    }
  }
#endif
  return 0;
}

int
cmshift(struct nshell *nshell,int argc,char **argv)
{
  int i,a;
  char *arg,*endptr;

  if (argc>2) {
    sherror4(argv[0],ERREXTARG);
    return ERREXTARG;
  } else if (argc==2) {
    arg=argv[1];
    a=strtol(arg,&endptr,10);
    if (endptr[0]!='\0') {
      sherror3(argv[0],ERRNUMERIC,arg);
      return ERRNUMERIC;
    }
  } else a=1;

  if (a<0) a=0;
  if ((a+1)>=nshell->argc) a=nshell->argc-1;
  for (i=a+1;i<nshell->argc;i++) {
    g_free(nshell->argv[i-a]);
    nshell->argv[i-a]=nshell->argv[i];
    nshell->argv[i]=NULL;
  }
  nshell->argc-=a;
  return 0;
}

int
cmtype(struct nshell *nshell,int argc,char **argv)
{
  struct prmlist *prm2;
  struct cmdlist *cmdcur;
  int j;
  char *cmdname;
  shell_proc proc;

  for (j=1;j<argc;j++) {
    int i;
    i = check_cpcmd(argv[j]);
    if (i >= 0) {
      printfstdout("%.256s is a shell keyword.\n", argv[j]);
    } else if ((cmdcur=getfunc(nshell,argv[j]))!=NULL) {
      printfstdout("%.256s is a function.\n",argv[j]);
      printfstdout("%.256s=()\n",argv[j]);
      putstdout("{");
      while (cmdcur!=NULL) {
        prm2=cmdcur->prm;
        printfstdout("    ");
        while (prm2!=NULL) {
          if (prm2->str!=NULL) printfstdout("%.256s ",prm2->str);
          prm2=prm2->next;
        }
        printfstdout("\n");
        cmdcur=cmdcur->next;
      }
      putstdout("}");
    } else if ((strcmp0(".",argv[j])==0)
    || (strcmp0("break",argv[j])==0)
    || (strcmp0("continue",argv[j])==0)
    || (strcmp0("return",argv[j])==0)) {
      printfstdout("%.256s is a shell built in.\n",argv[j]);
    } else {
      proc = check_cmd(argv[j]);
      if (proc) {
        printfstdout("%.256s is a shell built in.\n",argv[j]);
      } else {
        cmdname=nsearchpath(getval(nshell,"PATH"),argv[j],FALSE);
        if (cmdname==NULL) {
          sherror3(argv[0],ERRCFOUND,argv[j]);
        } else {
          printfstdout("%.256s is %.256s.\n",argv[j],cmdname);
	}
        g_free(cmdname);
      }
    }
  }
  return 0;
}

int
cmunset(struct nshell *nshell,int argc,char **argv)
{
  int i;

  for (i=1;i<argc;i++) {
    if ((strcmp0(argv[i],"PATH")!=0) && (strcmp0(argv[i],"PS1")!=0)
    && (strcmp0(argv[i],"PS2")!=0) && (strcmp0(argv[i],"IFS")!=0)) {
      if (!delval(nshell,argv[i])) return ERR;
   } else {
      sherror3(argv[0],ERRUNSET,argv[i]);
      return ERR;
    }
  }
  return 0;
}

static void
objdisp(struct objlist *root,struct objlist *parent,int *tab)
{
  int i;
  struct objlist *objcur;

  (*tab)++;
  objcur=root;
  while (objcur!=NULL) {
    if (objcur->parent==parent) {
      for (i=1;i<*tab;i++) printfstdout("\t");
      printfstdout("%.256s\n",objcur->name);
      objdisp(objcur,objcur,tab);
    }
    objcur=objcur->next;
  }
  (*tab)--;
}

static void
dispfield(struct objlist *obj,char *name)
{
  int j;
  enum ngraph_object_field_type ftype;
  char perm[4],type[10];
  char **enumlist;

  ftype=chkobjfieldtype(obj,name);
  switch (ftype) {
  case NVOID:    strcpy(type,"void"); break;
  case NBOOL:    strcpy(type,"bool"); break;
#ifdef USE_NCHAR
  case NCHAR:    strcpy(type,"char"); break;
#endif
  case NINT:     strcpy(type,"int"); break;
  case NDOUBLE:  strcpy(type,"double"); break;
  case NSTR:     strcpy(type,"char*"); break;
  case NPOINTER: strcpy(type,"void*"); break;
  case NIARRAY:  strcpy(type,"int[]"); break;
  case NDARRAY:  strcpy(type,"double[]"); break;
  case NSARRAY:  strcpy(type,"char*[]"); break;
  case NENUM:    strcpy(type,"enum("); break;
  case NOBJ:     strcpy(type,"obj"); break;
#ifdef USE_LABEL
  case NLABEL:   strcpy(type,"label"); break;
#endif
  case NVFUNC:   strcpy(type,"void("); break;
  case NBFUNC:   strcpy(type,"bool("); break;
#ifdef USE_NCHAR
  case NCFUNC:   strcpy(type,"char("); break;
#endif
  case NIFUNC:   strcpy(type,"int("); break;
  case NDFUNC:   strcpy(type,"double("); break;
  case NSFUNC:   strcpy(type,"char*("); break;
  case NIAFUNC:  strcpy(type,"int[]("); break;
  case NDAFUNC:  strcpy(type,"double[]("); break;
  case NSAFUNC:  strcpy(type,"char*[]("); break;
  default:      strcpy(type,"unknown"); break;
  }
  if (chkobjperm(obj,name) & NREAD) perm[0]='r';
  else perm[0]='-';
  if (chkobjperm(obj,name) & NWRITE) perm[1]='w';
  else perm[1]='-';
  if (chkobjperm(obj,name) & NEXEC) perm[2]='x';
  else perm[2]='-';
  perm[3]='\0';
  printfstdout("%3s %16.256s  %.256s",
                   (char *)perm,(char *)name,(char *)type);
  if (ftype>=NVFUNC) {
    const char *alist;
    if ((alist=chkobjarglist(obj,name))!=NULL) {
      if (alist[0]=='\0') printfstdout(" void");
      else
        for (j=0;alist[j]!='\0';j++) {
          switch (alist[j]) {
          case 'b': printfstdout(" bool"); break;
#ifdef USE_NCHAR
          case 'c': printfstdout(" char"); break;
#endif
          case 'i': printfstdout(" int"); break;
          case 'd': printfstdout(" double"); break;
          case 's': printfstdout(" char*"); break;
          case 'p': printfstdout(" void*"); break;
          case 'o': printfstdout(" obj"); break;
          }
          if (alist[j+1]=='a') {
            printfstdout("[]");
            j++;
          }
        }
    }
    printfstdout(" )");
  } else if (ftype==NENUM) {
    if ((enumlist=(char **)chkobjarglist(obj,name))!=NULL) {
      for (j=0;enumlist[j] && enumlist[j][0];j++) printfstdout(" %s",enumlist[j]);
      printfstdout(" )");
    }
  }
  printfstdout("\n");
}

int
cmobject(struct nshell *nshell,int argc,char **argv)
{
  struct objlist *obj;
  int i,j,tab;

  if (argc<2) {
    tab=0;
    objdisp(chkobjroot(),NULL,&tab);
    return 0;
  } else {
    if ((obj=getobject(argv[1]))==NULL) return ERR;
  }
  if (argc==2) {
    printfstdout("object: %.256s\n",chkobjectname(obj));
    if (chkobjectalias(obj)!=NULL)
      printfstdout("alias: %.256s\n",chkobjectalias(obj));
    else
      printfstdout("alias:\n");
    printfstdout("version: %.256s\n",chkobjver(obj));
    if (chkobjparent(obj)!=NULL)
      printfstdout("parent: %.256s\n",chkobjectname(chkobjparent(obj)));
    else
      printfstdout("parent: (null)\n");
    printfstdout("object id: %d\n",chkobjectid(obj));
    printfstdout("number of fields: %d\n",chkobjfieldnum(obj));
    printfstdout("size of instance: %d\n",chkobjsize(obj));
    printfstdout("current instance: %d\n",chkobjcurinst(obj));
    printfstdout("last instance id: %d\n",chkobjlastinst(obj));
    for (i=0;i<chkobjfieldnum(obj);i++) {
      char *name;
      name=chkobjfieldname(obj,i);
      dispfield(obj,name);
    }
  }
  for (i=2;i<argc;i++) {
    if (argv[i][0]=='-') {
      if (strcmp0(argv[i]+1,"name")==0) printfstdout("%.256s\n",chkobjectname(obj));
      else if (strcmp0(argv[i]+1,"version")==0) printfstdout("%.256s\n",chkobjver(obj));
      else if (strcmp0(argv[i]+1,"parent")==0) {
        if (chkobjparent(obj)!=NULL)
          printfstdout("%.256s\n",chkobjectname(chkobjparent(obj)));
        else
          printfstdout("(null)\n");
      } else if (strcmp0(argv[i]+1,"id")==0) printfstdout("%d\n",chkobjectid(obj));
      else if (strcmp0(argv[i]+1,"field")==0) printfstdout("%d\n",chkobjfieldnum(obj));
      else if (strcmp0(argv[i]+1,"size")==0) printfstdout("%d\n",chkobjsize(obj));
      else if (strcmp0(argv[i]+1,"current")==0) printfstdout("%d\n",chkobjcurinst(obj));
      else if (strcmp0(argv[i]+1,"last")==0) printfstdout("%d\n",chkobjlastinst(obj));
      else if (strcmp0(argv[i]+1,"instance")==0) printfstdout("%d\n",chkobjlastinst(obj)+1);
      else if (strcmp0(argv[i]+1,"instances")==0) {
        for (j=0;j<=chkobjlastinst(obj);j++) {
          printfstdout("%d",j);
          if (j!=chkobjlastinst(obj)) printfstdout(" ");
          else printfstdout("\n");
        }
      } else {
        sherror3(argv[0],ERROPTION,argv[i]);
        return ERROPTION;
      }
    } else break;
  }
  for (;i<argc;i++) {
    if (getobjfield(obj,argv[i])==-1) return ERR;
    dispfield(obj,argv[i]);
  }
  return 0;
}

static void
dispparent(struct objlist *parent,int noinst)
{
  struct objlist *ocur;

  ocur=chkobjroot();
  while (ocur!=NULL) {
    if (chkobjparent(ocur)==parent) {
      if ((chkobjlastinst(ocur)!=-1) || (!noinst))
        putstdout(chkobjectname(ocur));
      dispparent(ocur,noinst);
    }
    ocur=ocur->next;
  }
}

int
cmderive(struct nshell *nshell,int argc,char **argv)
{
  struct objlist *obj;
  int i,noinst;

  noinst=FALSE;
  for (i=1;i<argc;i++) {
    if (argv[i][0]=='-') {
      if (strcmp0(argv[i]+1,"instance")==0) noinst=TRUE;
      else {
        sherror3(argv[0],ERROPTION,argv[i]);
        return ERROPTION;
      }
    } else break;
  }
  if (i==argc) {
    if ((obj=getobject("object"))==NULL) return ERR;
    if ((chkobjlastinst(obj)!=-1) || (!noinst))
      putstdout(chkobjectname(obj));
    dispparent(obj,noinst);
  } else {
    for (;i<argc;i++) {
      if ((obj=getobject(argv[i]))==NULL) return ERR;
      if ((chkobjlastinst(obj)!=-1) || (!noinst))
        putstdout(chkobjectname(obj));
      dispparent(obj,noinst);
    }
  }
  return 0;
}

int
cmnew(struct nshell *nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,anum,id;
  char *oname;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  }
  arrayinit(&iarray,sizeof(int));
  getobjiname(argv[1], &oname, NULL);
  if (getobjilist(argv[1],&obj,&iarray,FALSE,NULL)) {
    g_free(oname);
    return ERR;
  }
  anum=arraynum(&iarray);
  arraydel(&iarray);
  if (anum!=0) {
    sherror3(argv[0],ERRNEWINST,argv[1]);
    arraydel(&iarray);
    g_free(oname);
    return ERRNEWINST;
  }
  id = newobj_alias(obj, oname);
  g_free(oname);
  if (id == -1) {
    return ERR;
  }
  for (i=2;i<argc;i++) {
    if (sputfield(obj,id,argv[i])!=0) {
      delobj(obj,id);
      return ERR;
    }
  }
  return 0;
}

int
cmdel(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,j,id,anum,*adata;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  } else if (argc>2) {
    sherror4(argv[0],ERRMANYARG);
    return ERRMANYARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  if (anum==0) {
    sherror4(argv[0],ERRNONEINST);
    arraydel(&iarray);
    return ERRNONEINST;
  }
  adata=arraydata(&iarray);
  for (i=0;i<anum;i++)
    for (j=1;j<anum;j++)
      if (adata[j-1]<adata[j]) {
        id=adata[j-1];
        adata[j-1]=adata[j];
        adata[j]=id;
      }
  j=0;
  for (i=1;i<anum;i++)
    if (adata[i]!=adata[j]) {
      j++;
      adata[j]=adata[i];
    }
  for (i=0;i<=j;i++)
    if (delobj(obj,adata[i])==-1) {
      arraydel(&iarray);
      return ERR;
    }
  arraydel(&iarray);
  return 0;
}

int
cmexist(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int anum, quiet = FALSE;
  char *objname;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  } else if (argc == 3) {
    if (strcmp(argv[1], "-q")) {
      sherror4(argv[0],ERRVALUE);
      return ERRVALUE;
    } else {
      quiet = TRUE;
      objname = argv[2];
    }
  } else if (argc>3) {
    sherror4(argv[0],ERRMANYARG);
    return ERRMANYARG;
  } else {
    objname = argv[1];
  }

  arrayinit(&iarray,sizeof(int));
  if (chkobjilist(objname,&obj,&iarray,TRUE,NULL)) anum=0;
  else {
    anum=arraynum(&iarray);
    arraydel(&iarray);
  }
  if (! quiet)
    printfstdout("%d\n",anum);

  if (anum==0) return ERRNONEINST;
  return 0;
}

static void
put_field_str(const char *valstr, int escape)
{
  if (valstr == NULL) {
    return;
  }
  if (escape) {
    char *tmp;
    tmp = g_strescape(valstr, NULL);
    if (tmp == NULL) {
      return;
    }
    putstdout(tmp);
    g_free(tmp);
  } else {
    putstdout(valstr);
  }
}

int
cmget(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  char *field,*valstr;
  int i,j,k,l,anum,len,*adata;
  int nowrite,nofield,noid,quote,perm,multi,escape;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  adata=arraydata(&iarray);
  if (anum==0) {
    sherror4(argv[0],ERRNONEINST);
    arraydel(&iarray);
    return ERRNONEINST;
  }
  nowrite=FALSE;
  nofield=FALSE;
  noid=FALSE;
  quote=FALSE;
  escape=FALSE;
  for (j=2;j<argc;j++) {
    if (argv[j][0]=='-') {
      if (strcmp0(argv[j]+1,"write")==0) nowrite=TRUE;
      else if (strcmp0(argv[j]+1,"field")==0) nofield=TRUE;
      else if (strcmp0(argv[j]+1,"id")==0) noid=TRUE;
      else if (strcmp0(argv[j]+1,"quote")==0) quote=TRUE;
      else if (strcmp0(argv[j]+1,"escape")==0) escape=TRUE;
      else {
        sherror3(argv[0],ERROPTION,argv[j]);
        arraydel(&iarray);
        return ERROPTION;
      }
    } else break;
  }
  if (anum==1) multi=FALSE;
  else multi=TRUE;
  for (l=0;l<anum;l++) {
    int id;
    id=adata[l];
    if (j==argc) {
      for (i=0;i<chkobjfieldnum(obj);i++) {
        field=chkobjfieldname(obj,i);
        perm=chkobjperm(obj,field);
        if (((perm&NREAD)==NREAD) && ((perm&NEXEC)==0)
        && (!nowrite || ((perm&NWRITE)==NWRITE))) {
          if (sgetfield(obj,id,field,&valstr,FALSE,FALSE,quote)!=0) {
            arraydel(&iarray);
            return ERR;
          }
          if (multi && !noid) printfstdout("%d: ",id);
          if (!nofield) printfstdout("%.256s:",field);
	  if (valstr) {
	    put_field_str(valstr, escape);
	    g_free(valstr);
	  }
        }
      }
    } else {
      for (k=j;k<argc;k++) {
        field=argv[k];
        if (!nowrite || ((chkobjperm(obj,field)&NWRITE)==NWRITE)) {
          if (sgetfield(obj,id,field,&valstr,FALSE,FALSE,quote)!=0) {
            arraydel(&iarray);
            return ERR;
          }
          if (multi && !noid) printfstdout("%d: ",id);
          if (!nofield) {
            field=getitok2(&field,&len,":=");
            printfstdout("%.256s:",field);
            g_free(field);
          }
	  if (valstr) {
	    put_field_str(valstr, escape);
	    g_free(valstr);
	  }
        }
      }
    }
  }
  arraydel(&iarray);
  return 0;
}

int
cmput(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  int i,j,anum,*adata;
  struct narray iarray;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  adata=arraydata(&iarray);
  if (anum==0) {
    sherror4(argv[0],ERRNONEINST);
    arraydel(&iarray);
    return ERRNONEINST;
  }
  if (argc==2) {
    sherror4(argv[0],ERRNOFIELD);
    arraydel(&iarray);
    return ERRNOFIELD;
  }
  for (i=0;i<anum;i++) {
    int id;
    id=adata[i];
    for (j=2;j<argc;j++) {
      if (sputfield(obj,id,argv[j])!=0) {
        arraydel(&iarray);
        return ERR;
      }
    }
  }
  arraydel(&iarray);
  return 0;
}

int
cmdup(struct nshell *nshell, int argc, char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i, anum, *adata;

  if (argc < 2) {
    sherror4(argv[0], ERROBJARG);
    return ERROBJARG;
  }

  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1], &obj, &iarray, TRUE, NULL)) {
    return ERR;
  }

  anum = arraynum(&iarray);
  adata = arraydata(&iarray);
  if (anum == 0) {
    sherror4(argv[0],ERRNONEINST);
    arraydel(&iarray);
    return ERRNONEINST;
  }

  for (i = 0; i < anum; i++) {
    int sid, did;
    sid = adata[i];
    did = newobj(obj);
    if (did < 0) {
      arraydel(&iarray);
      return ERR;
    }

    if (copy_obj_field(obj, did, sid, NULL)) {
      arraydel(&iarray);
      return ERR;
    }
  }

  arraydel(&iarray);
  return 0;
}

int
cmcpy(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,j,anum,*adata;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  adata=arraydata(&iarray);
  if (anum<2) {
    sherror4(argv[0],ERRTWOINST);
    arraydel(&iarray);
    return ERRTWOINST;
  }
  for (i=1;i<anum;i++) {
    int sid, did;
    sid=adata[0];
    did=adata[i];
    if (argc == 2) {
      if (copy_obj_field(obj, did, sid, NULL)) {
	arraydel(&iarray);
	return ERR;
      }
    } else {
      for (j = 2; j < argc; j++) {
        if (copyobj(obj, argv[j], did, sid)==-1) {
          arraydel(&iarray);
          return ERR;
        }
      }
    }
  }
  arraydel(&iarray);
  return 0;
}

int
cmmove(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int anum,id1,id2,*adata;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  } else if (argc>2) {
    sherror4(argv[0],ERRMANYARG);
    return ERRMANYARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  adata=arraydata(&iarray);
  if (anum!=2) {
    sherror4(argv[0],ERRTWOINST);
    arraydel(&iarray);
    return ERRTWOINST;
  }
  id1=adata[0];
  id2=adata[1];
  arraydel(&iarray);
  if (strcmp0(argv[0],"move")==0) {
    if (moveobj(obj,id2,id1)==-1) return ERR;
  } else if (strcmp0(argv[0],"exch")==0) {
    if (exchobj(obj,id2,id1)==-1) return ERR;
  }
  return 0;
}

int
cmmovetop(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,anum,top,rcode,*adata;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  } else if (argc>2) {
    sherror4(argv[0],ERRMANYARG);
    return ERRMANYARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  adata=arraydata(&iarray);
  if (strcmp0(argv[0],"movetop")==0) top=0;
  else if (strcmp0(argv[0],"moveup")==0) top=1;
  else if (strcmp0(argv[0],"movedown")==0) top=2;
  else top=3;
  for (i=0;i<anum;i++) {
    if (top==0) rcode=movetopobj(obj,adata[i]);
    else if (top==1) rcode=moveupobj(obj,adata[i]);
    else if (top==2) rcode=movedownobj(obj,adata[i]);
    else rcode=movelastobj(obj,adata[i]);
    if (rcode==-1) {
      arraydel(&iarray);
      return ERR;
    }
  }
  arraydel(&iarray);
  return 0;
}

int
cmexe(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,j,anum,*adata;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,TRUE,NULL)) return ERR;
  anum=arraynum(&iarray);
  adata=arraydata(&iarray);
  if (anum==0) {
    sherror4(argv[0],ERRNONEINST);
    arraydel(&iarray);
    return ERRNONEINST;
  }
  if (argc==2) {
    sherror4(argv[0],ERRNOFIELD);
    arraydel(&iarray);
    return ERRNOFIELD;
  }
  for (j=0;j<anum;j++) {
    int id;
    id=adata[j];
    for (i=2;i<argc;i++) {
      if (sexefield(obj,id,argv[i])!=0) {
        arraydel(&iarray);
        return ERR;
      }
    }
  }
  arraydel(&iarray);
  return 0;
}

int
cmdexpr(struct nshell*nshell,int argc,char **argv)
{
  int rcode,ecode;
  double vd;
  int i;
  char *err_msg;
  GString *s;

  if (argc<1) {
    sherror4(argv[0],ERRSMLARG);
    return ERRSMLARG;
  }

  s = g_string_sized_new(64);
  if (s == NULL) {
    return ERR;
  }

  for (i = 1; i < argc; i++) {
    g_string_append(s, argv[i]);
  }

  ecode = str_calc(s->str, &vd, &rcode, &err_msg);
  g_string_free(s, TRUE);

  if (ecode) {
    if (err_msg) {
      printfstderr("shell: %s\n", err_msg);
      g_free(err_msg);
    } else {
      sherror4(argv[0],ecode);
    }
    return ecode;
  }

  if (rcode == MATH_VALUE_NAN) {
    putstdout("nan");
    return ERR;
  } else if (rcode == MATH_VALUE_UNDEF) {
    putstdout("undefined");
    return ERR;
  }

  if (argv[0][0] == 'd') {
    printfstdout("%.15e\n", vd);
  } else {
    printfstdout("%.0f\n", round(vd));
  }

  return 0;
}

int
cmread(struct nshell *nshell,int argc,char **argv)
{
  int c,len;
  char *po;
  GString *s;

  s = g_string_sized_new(64);
  if (s == NULL) {
    return ERR;
  }

  while (TRUE) {
    c = getstdin();
    if (c == '\n' || c == EOF) {
      break;
    }
    g_string_append_c(s, c);
  }

  if (argc == 1) {
    addval(nshell, "REPLY", s->str);
  } else {
    char *ifs;
    int i;
    po = s->str;
    ifs = getval(nshell, "IFS");
    for (i = 1; i < argc; i++) {
      char *s2;
      s2 = getitok2(&po, &len, ifs);
      if (s2) {
        addval(nshell, argv[i], s2);
        g_free(s2);
      } else {
        addval(nshell, argv[i], "");
      }
    }
  }
  g_string_free(s, TRUE);

  return (c == EOF) ? ERR : 0;
}

int
cmwhich(struct nshell*nshell,int argc,char **argv)
{
  int i, r, start, quiet;
  char *path;

  if (strcmp0(argv[1], "-q")) {
    start = 1;
    quiet = FALSE;
  } else {
    start = 2;
    quiet = TRUE;
  }

  if (argc < 1 + start) {
    sherror4(argv[0], ERRSMLARG);
    return ERRSMLARG;
  }

  r = 0;
  for (i = start; i < argc; i++) {
    if (check_cmd(argv[i])) {
      if (! quiet) {
	printfstdout("%s: shell built-in command\n", argv[i]);
      }
    } else {
      path = g_find_program_in_path(argv[i]);
      if (path == NULL) {
	r = 1;
      } else {
	if (! quiet) {
	  putstdout(path);
	}
	g_free(path);
      }
    }
  }

  return r;
}
