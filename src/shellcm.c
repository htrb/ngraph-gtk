/* 
 * $Id: shellcm.c,v 1.7 2008/10/17 06:43:02 hito Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifndef WINDOWS
#include <unistd.h>
#else
#include <dos.h>
#include <dir.h>
#endif
#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "mathcode.h"
#include "mathfn.h"
#include "shell.h"
#include "shellcm.h"

#define TRUE  1
#define FALSE 0

#define ERR 128

int cmcd(struct nshell *nshell,int argc,char **argv)
{
  char *home;

  if (argv[1]!=NULL) {
    if (chdir(argv[1])!=0) {
      sherror3(argv[0],ERRNODIR,argv[1]);
      return ERR;
    }
  } else {
    if ((home=getval(nshell,"HOME"))!=NULL) {
      if (chdir(home)!=0) {
        sherror3(argv[0],ERRNODIR,home);
        return ERR;
      }
    }
  }
  return 0;
}

int cmecho(struct nshell *nshell,int argc,char **argv)
{
  int i;

  for (i=1;i<argc;i++) {
    printfstdout("%.256s",argv[i]);
    if (i!=(argc-1)) printfstdout(" ");
  }
  putstdout("");
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

  for (x = first; x <= last; x += inc) {
    printfstdout("%G\n",x);
  }
  return 0;
}

int cmeval(struct nshell *nshell,int argc,char **argv)
{
  char *s;
  int i,rcode;

  if ((s=nstrnew())==NULL) return ERR;
  for (i=1;i<argc;i++) {
    if ((s=nstrcat(s,argv[i]))==NULL) return ERR;
    if ((s=nstrccat(s,' '))==NULL) return ERR;
  }
  rcode=cmdexecute(nshell,s);
  memfree(s);
  if ((rcode!=0) && (rcode!=1)) return ERR;
  return 0;
}

int cmexit(struct nshell *nshell,int argc,char **argv)
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

int cmexport(struct nshell *nshell,int argc,char **argv)
{
  int i;
  struct explist *valcur;

  if (argv[1]==NULL) {
    valcur=nshell->exproot;
    while (valcur!=NULL) {
      printfstdout("%.256s\n",valcur->val);
      valcur=valcur->next;
    }
    return 0;
  } else {
    for (i=1;i<argc;i++)
      if (addexp(nshell,argv[1])==NULL) return ERR;
    return 0;
  }
}

int cmpwd(struct nshell *nshell,int argc,char **argv)
{
  char *s;

  if ((s=ngetcwd())==NULL) return ERR;
  putstdout(s);
  memfree(s);
  return 0;
}

int cmset(struct nshell *nshell,int argc,char **argv)
{
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
      if ((s=memalloc(strlen((nshell->argv)[0])+1))==NULL) return ERR;
      strcpy(s,(nshell->argv)[0]);
      if (arg_add(&argv2,s)==NULL) {
        memfree(s);
        arg_del(argv2);
        return ERR;
      }
      for (;j<argc;j++) {
        if ((s=memalloc(strlen(argv[j])+1))==NULL) return ERR;
        strcpy(s,argv[j]);
        if (arg_add(&argv2,s)==NULL) {
          memfree(s);
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
  return 0;
}

int cmshift(struct nshell *nshell,int argc,char **argv)
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
    memfree(nshell->argv[i-a]);
    nshell->argv[i-a]=nshell->argv[i];
    nshell->argv[i]=NULL;
  }
  nshell->argc-=a;
  return 0;
}

int cmtype(struct nshell *nshell,int argc,char **argv)
{
  struct prmlist *prm2;
  struct cmdlist *cmdcur;
  int i,j;
  char *cmdname;

  for (j=1;j<argc;j++) {
    for (i=1;i<CPCMDNUM;i++)
      if (strcmp0(cpcmdtable[i],argv[j])==0) break;
    if (i!=CPCMDNUM) {
      printfstdout("%.256s is a shell keyword.\n",argv[j]);
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
      printfstdout("%.256s is a shell builtin.\n",argv[j]);
    } else {
      for (i=0;i<CMDNUM;i++)
        if (strcmp0(argv[j],cmdtable[i].name)==0) break;
      if (i!=CMDNUM) {
        printfstdout("%.256s is a shell builtin.\n",argv[j]);
      } else {
        cmdname=nsearchpath(getval(nshell,"PATH"),argv[j],FALSE);
        if (cmdname==NULL) {
          sherror3(argv[0],ERRCFOUND,argv[j]);
        } else {
          printfstdout("%.256s is %.256s.\n",argv[j],cmdname);
	}
        memfree(cmdname);
      }
    }
  }
  return 0;
}

int cmunset(struct nshell *nshell,int argc,char **argv)
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

void objdisp(struct objlist *root,struct objlist *parent,int *tab)
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

void dispfield(struct objlist *obj,char *name)
{
  int j,ftype;
  char perm[4],type[10];
  char *alist;
  char **enumlist;

  ftype=chkobjfieldtype(obj,name);
  switch (ftype) {
  case NVOID:    strcpy(type,"void"); break;
  case NBOOL:    strcpy(type,"bool"); break;
  case NCHAR:    strcpy(type,"char"); break;
  case NINT:     strcpy(type,"int"); break;
  case NDOUBLE:  strcpy(type,"double"); break;
  case NSTR:     strcpy(type,"char*"); break;
  case NPOINTER: strcpy(type,"void*"); break;
  case NIARRAY:  strcpy(type,"int[]"); break;
  case NDARRAY:  strcpy(type,"double[]"); break;
  case NSARRAY:  strcpy(type,"*char[]"); break;
  case NENUM:    strcpy(type,"enum("); break;
  case NOBJ:     strcpy(type,"obj"); break;
  case NLABEL:   strcpy(type,"label"); break;
  case NVFUNC:   strcpy(type,"void("); break;
  case NBFUNC:   strcpy(type,"bool("); break;
  case NCFUNC:   strcpy(type,"char("); break;
  case NIFUNC:   strcpy(type,"int("); break;
  case NDFUNC:   strcpy(type,"double("); break;
  case NSFUNC:   strcpy(type,"*char("); break;
  case NIAFUNC:  strcpy(type,"int[]("); break;
  case NDAFUNC:  strcpy(type,"double[]("); break;
  case NSAFUNC:  strcpy(type,"*char[]("); break;
  default:      strcpy(type,"unknown"); break;
  }
  if (chkobjperm(obj,name) & NREAD) perm[0]='r';
  else perm[0]='-';
  if (chkobjperm(obj,name) & NWRITE) perm[1]='w';
  else perm[1]='-';
  if (chkobjperm(obj,name) & NEXEC) perm[2]='x';
  else perm[2]='-';
  perm[3]='\0';
  printfstdout("%3s %15.256s    %.256s",
                   (char *)perm,(char *)name,(char *)type);
  if (ftype>=NVFUNC) {
    if ((alist=chkobjarglist(obj,name))!=NULL) {
      if (alist[0]=='\0') printfstdout(" void");
      else
        for (j=0;alist[j]!='\0';j++) {
          switch (alist[j]) {
          case 'b': printfstdout(" bool"); break;
          case 'c': printfstdout(" char"); break;
          case 'i': printfstdout(" int"); break;
          case 'd': printfstdout(" double"); break;
          case 's': printfstdout(" char *"); break;
          case 'p': printfstdout(" void *"); break;
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
      for (j=0;enumlist[j]!=NULL;j++) printfstdout(" %s",enumlist[j]);
      printfstdout(" )");
    }
  }
  printfstdout("\n");
}

int cmobject(struct nshell *nshell,int argc,char **argv)
{
  struct objlist *obj;
  int i,j,tab;
  char *name;

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

void dispparent(struct objlist *parent,int noinst)
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

int cmderive(struct nshell *nshell,int argc,char **argv)
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

int cmnew(struct nshell *nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,anum,id;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(argv[1],&obj,&iarray,FALSE,NULL)) return ERR;
  anum=arraynum(&iarray);
  arraydel(&iarray);
  if (anum!=0) {
    sherror3(argv[0],ERRNEWINST,argv[1]);
    arraydel(&iarray);
    return ERRNEWINST;
  }
  if ((id=newobj(obj))==-1) return ERR;
  for (i=2;i<argc;i++) {
    if (sputfield(obj,id,argv[i])!=0) {
      delobj(obj,id);
      return ERR;
    }
  }
  return 0;
}

int cmdel(struct nshell*nshell,int argc,char **argv)
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

int cmexist(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int anum;

  if (argc<2) {
    sherror4(argv[0],ERROBJARG);
    return ERROBJARG;
  } else if (argc>2) {
    sherror4(argv[0],ERRMANYARG);
    return ERRMANYARG;
  }
  arrayinit(&iarray,sizeof(int));
  if (chkobjilist(argv[1],&obj,&iarray,TRUE,NULL)) anum=0;
  else {
    anum=arraynum(&iarray);
    arraydel(&iarray);
  }
  printfstdout("%d\n",anum);
  if (anum==0) return ERRNONEINST;
  return 0;
}

int cmget(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  char *field,*valstr;
  int i,j,k,l,id,anum,len,*adata;
  int nowrite,nofield,noid,quote,perm,multi;

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
  for (j=2;j<argc;j++) {
    if (argv[j][0]=='-') {
      if (strcmp0(argv[j]+1,"write")==0) nowrite=TRUE;
      else if (strcmp0(argv[j]+1,"field")==0) nofield=TRUE;
      else if (strcmp0(argv[j]+1,"id")==0) noid=TRUE;
      else if (strcmp0(argv[j]+1,"quote")==0) quote=TRUE;
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
    id=adata[l];
    if (j==argc) {
      for (i=0;i<chkobjfieldnum(obj);i++) {
        field=chkobjfieldname(obj,i);
        perm=chkobjperm(obj,field);
        if (((perm&NREAD)==1) && ((perm&NEXEC)==0)
        && (!nowrite || ((perm&NWRITE)==1))) {
          if (sgetfield(obj,id,field,&valstr,FALSE,FALSE,quote)!=0) {
            arraydel(&iarray);
            return ERR;
          }
          if (multi && !noid) printfstdout("%d: ",id);
          if (!nofield) printfstdout("%.256s:",field);
          putstdout(valstr);
          memfree(valstr);
        }
      }
    } else {
      for (k=j;k<argc;k++) {
        field=argv[k];
        if (!nowrite || ((chkobjperm(obj,field)&NWRITE)==1)) {
          if (sgetfield(obj,id,field,&valstr,FALSE,FALSE,quote)!=0) {
            arraydel(&iarray);
            return ERR;
          }
          if (multi && !noid) printfstdout("%d: ",id);
          if (!nofield) {
            field=getitok2(&field,&len,":=");
            printfstdout("%.256s:",field);
            memfree(field);
          }
          putstdout(valstr);
          memfree(valstr);
        }
      }
    }
  }
  arraydel(&iarray);
  return 0;
}

int cmput(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  char *image;
  int i,j,id,anum,*adata;
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
    id=adata[i];
    if ((image=saveobj(obj,id))==NULL) {
      arraydel(&iarray);
      return ERR;
    }
    for (j=2;j<argc;j++) {
      if (sputfield(obj,id,argv[j])!=0) {
        restoreobj(obj,id,image); 
        arraydel(&iarray);
        return ERR;
      }
    }
    memfree(image);
  }
  arraydel(&iarray);
  return 0;
}

int cmcpy(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,j,anum,sid,did,*adata;
  char *field;
  int perm,type;

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
    sid=adata[0];
    did=adata[i];
    if (argc==2) {
      for (j=0;j<chkobjfieldnum(obj);j++) {
        field=chkobjfieldname(obj,j);
        perm=chkobjperm(obj,field);
        type=chkobjfieldtype(obj,field);
        if (((perm&NREAD)!=0) && ((perm&NWRITE)!=0) && (type<NVFUNC)) {
          if (copyobj(obj,field,did,sid)==-1) {
            arraydel(&iarray);
            return ERR;
          }
        }
      }
    } else {
      for (j=2;j<argc;j++) 
        if (copyobj(obj,argv[j],did,sid)==-1) {
          arraydel(&iarray);
          return ERR;
        }
    }
  }
  arraydel(&iarray);
  return 0;
}

int cmmove(struct nshell*nshell,int argc,char **argv)
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

int cmmovetop(struct nshell*nshell,int argc,char **argv)
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

int cmexe(struct nshell*nshell,int argc,char **argv)
{
  struct objlist *obj;
  struct narray iarray;
  int i,j,id,anum,*adata;

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
str_calc(char *str, double *val, int *r)
{
  int rcode, ecode = 0, i;
  char *code;
  double memory[MEMORYNUM];
  char memorystat[MEMORYNUM];

  rcode = mathcode(str, &code, NULL, NULL, NULL, NULL, 
		   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		   FALSE, FALSE, FALSE, FALSE, FALSE);

  if (rcode != MCNOERR) {
    switch (rcode) {
    case MCSYNTAX:
      ecode = ERRMSYNTAX;
      break;
    case MCILLEGAL:
      ecode = ERRMILLEGAL;
      break;
    case MCNEST:
      ecode = ERRMNEST;
      break;
    default:
      ecode = ERRUNKNOWNSH;
    }
    return ecode;
  }
  for (i = 0; i < MEMORYNUM; i++) {
    memory[i] = 0;
    memorystat[i] = MNOERR;
  }
  rcode = calculate(code, 1, 
		    0, MNOERR, 0, MNOERR, 0, MNOERR, 
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		    NULL, NULL, 
		    memory, memorystat, 
		    NULL, NULL, 
		    NULL, NULL, 
		    NULL, NULL, NULL, 
		    NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, val);
  memfree(code);

  if (rcode == MSERR) {
    ecode = ERRMSYNTAX;
  } else if (rcode == MERR) {
    ecode = ERRMFAT;
  }

  if (ecode) {
    return ecode;
  }

  *r = rcode;

  return 0;
}

int cmdexpr(struct nshell*nshell,int argc,char **argv)
{
  int rcode,ecode;
  double vd;
  int i;
  char *s;

  if (argc<1) {
    sherror4(argv[0],ERRSMLARG);
    return ERRSMLARG;
  }
  if ((s=nstrnew())==NULL) return ERR;
  for (i=1;i<argc;i++)
    if ((s=nstrcat(s,argv[i]))==NULL) return ERR;

  ecode = str_calc(s, &vd, &rcode);
  memfree(s);

  if (ecode) {
    sherror4(argv[0],ecode);
    return ecode;
  }

  if (rcode==MNAN) {
    putstdout("nan");
    return ERR;
  } else if (rcode==MUNDEF) {
    putstdout("undifined");
    return ERR;
  }

  if (argv[0][0]=='d') printfstdout("%.15e\n",vd);
  else printfstdout("%d\n",nround(vd));
  return 0;
}

int cmread(struct nshell *nshell,int argc,char **argv)
{
  int c,i,len;
  char *s,*po,*s2,*ifs;

  if ((s=nstrnew())==NULL) return ERR;
  while (TRUE) {
    c=getstdin();
    if ((c=='\n') || (c==EOF)) break;
    if ((s=nstrccat(s,c))==NULL) return ERR;
  }
  if (argc==1) {
    addval(nshell,"REPLY",s);
  } else {
    po=s;
    ifs=getval(nshell,"IFS");
    for (i=1;i<argc;i++) {
      if ((s2=getitok2(&po,&len,ifs))!=NULL) {
        addval(nshell,argv[i],s2);
        memfree(s2);
      } else {
        addval(nshell,argv[i],"");
      }
    }
  }
  memfree(s);
  if (c==EOF) return ERR;
  else return 0;
}
