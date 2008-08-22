/* 
 * $Id: ioutil.c,v 1.9 2008/08/22 10:05:55 hito Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#ifndef WINDOWS
#include <unistd.h>
#else
#include <io.h>
#include <dir.h>
#include <windows.h>
#endif
#include "object.h"
#include "nstring.h"
#include "jnstring.h"
#include "ioutil.h"

#define TRUE  1
#define FALSE 0

static void (* ShowProgressFunc)(int, char *, double) = NULL;

#if EOF == -1
static char C_type_buf[257];

void
char_type_buf_init(void)
{
  memset(C_type_buf, 0, sizeof(C_type_buf));
  C_type_buf[0] = 1;
  C_type_buf['\r' + 1] = 1;
  C_type_buf['\n' + 1] = 1;
  C_type_buf['\0' + 1] = 1;
}
#endif

#ifndef WINDOWS
void changefilename(char *name)
{
  int i;

  if (name==NULL) return;
  for (i=0;name[i]!='\0';i++)
    if ((name[i]=='\\') && (!niskanji2(name,i))) name[i]=DIRSEP;
/*
  if ((name[0]==DIRSEP) && (name[1]==DIRSEP)) {
    for (i=2;(name[i]!='\0') && (name[i]!=DIRSEP);i++);
    if (name[i]==DIRSEP) i++;
  }
  j=i;
  if (isalpha(name[j]) && (name[j+1]==':'))
    for (i=j+2;name[i]!='\0';i++) name[j++]=name[i];
*/
}
#else
void changefilename(char *name)
{
  int i;
/*  char disk[4];
  DWORD csensitive; */

  if (name==NULL) return;
  for (i=0;name[i]!='\0';i++)
    if ((name[i]=='\\') && (!niskanji2(name,i))) name[i]=DIRSEP;
/*
  if ((name[0]==DIRSEP) && (name[1]==DIRSEP)) return;
  if (isalpha(name[0]) && (name[1]==':')) {
    name[0]=tolower(name[0]);
    disk[0]=name[0];
    disk[1]=name[1];
    disk[2]='\\';
    disk[3]='\0';
    GetVolumeInformation(disk,NULL,0,NULL,NULL,&csensitive,NULL,0);
  } else {
    GetVolumeInformation(NULL,NULL,0,NULL,NULL,&csensitive,NULL,0);
  }
  if ((csensitive & FS_CASE_SENSITIVE)==0) {
    for (i=0;name[i]!='\0';i++) {
      if (niskanji(name[i])) i++;
      else name[i]=tolower(name[i]);
    }
  }
*/
}

void unchangefilename(char *name)
{
  int i;

  if (name==NULL) return;
  for (i=0;name[i]!='\0';i++)
    if ((name[i]==DIRSEP) && (!niskanji2(name,i))) name[i]='\\';
}
#endif

void pathresolv(char *name)
{
  int j,k;

  if (name==NULL) return;
  changefilename(name);
  j=k=0;
  while (name[k]!='\0') {
    if ((name[k]==DIRSEP) && (name[k+1]=='.')) {
      if (name[k+2]==DIRSEP) k+=2;
      else if ((name[k+2]=='.') && (name[k+3]==DIRSEP)) {
        if (j!=0) j--;
        for (;(name[j]!=DIRSEP) && (j!=0);j--);
        k+=3;
      } else {
        name[j]=name[k];
        j++;
        k++;
      }
    } else {
      name[j]=name[k];
      j++;
      k++;
    }
  }
  name[j]='\0';
}

#ifndef WINDOWS
char *getfullpath(char *name)
{
  char *s,*cwd;
  int top,j;

  if (name==NULL) return NULL;
  changefilename(name);
  if ((name[0]==DIRSEP) && (name[1]==DIRSEP)) {
    if ((s=memalloc(strlen(name)+1))==NULL) return NULL;
    strcpy(s,name);
  } else {
    if (isalpha(name[0]) && name[1]==':') top=2;
    else top=0;
    if (name[top]==DIRSEP) {
      if ((s=memalloc(strlen(name)+1))==NULL) return NULL;
      strcpy(s,name+top);
    } else {
      if ((cwd=ngetcwd())==NULL) return NULL;
      if ((s=memalloc(strlen(cwd)+strlen(name)+2))==NULL) {
        memfree(cwd);
        return NULL;
      }
      strcpy(s,cwd);
      j=strlen(cwd);
      if ((cwd[0]!='\0') && (cwd[strlen(cwd)-1]!=DIRSEP)) s[j++]=DIRSEP;
      strcpy(s+j,name+top);
      memfree(cwd);
    }
  }
  pathresolv(s);
  return s;
}
#else
char *getfullpath(char *name)
{
  char buf[MAXPATH],*s;

  if (name==NULL) return NULL;
  unchangefilename(name);
  if (GetFullPathName(name,MAXPATH,buf,&s)==0) return NULL;
  if ((s=memalloc(strlen(buf)+1))==NULL) return NULL;
  strcpy(s,buf);
  changefilename(s);
  return s;
}
#endif

char *getrelativepath(char *name)
{
  char *s,*cwd,*cwd2;
  int i,j,depth,top;

  if (name==NULL) return NULL;
  changefilename(name);
  if ((name[0]==DIRSEP) && (name[1]==DIRSEP)) {
    if ((s=memalloc(strlen(name)+1))==NULL) return NULL;
    strcpy(s,name);
    pathresolv(s);
  } else {
    top=0;
#ifndef WINDOWS
    if (name[0]==DIRSEP) {
#else
    if (isalpha(name[0]) && (name[1]==':')) top=2;
    if ((name[top]==DIRSEP)
        && ((top!=2) || ((toupper(name[0])-'A')==getdisk()))) {
#endif
      if ((cwd=ngetcwd())==NULL) return NULL;
#ifdef WINDOWS
      for (j=2;cwd[j]!='\0';j++) cwd[j-2]=cwd[j];
      cwd[j-2]='\0';
#endif
      if ((cwd2=memalloc(strlen(cwd)+2))==NULL) return NULL;
      strcpy(cwd2,cwd);
      memfree(cwd);
      i=strlen(cwd2);
      if ((i==0) || (cwd2[i-1]!=DIRSEP)) {
        cwd2[i]=DIRSEP;
        cwd2[i+1]='\0';
      }
      for (i=0;(cwd2[i]!='\0') && (name[i+top]!='\0');i++)
        if (cwd2[i]!=name[i+top]) break;
      if (i>0) i--;
      for (;niskanji2(cwd2,i) || (cwd2[i]!=DIRSEP);i--);
      depth=0;
      for (j=strlen(cwd2);j!=i;j--)
        if (!niskanji2(cwd2,j) && (cwd2[j]==DIRSEP)) depth++;
      memfree(cwd2);
      if ((s=nstrnew())==NULL) return NULL;
      if (depth==0) {
        if ((s=nstrcat(s,"./"))==NULL) return NULL;
      } else {
        for (j=0;j<depth;j++)
          if ((s=nstrcat(s,"../"))==NULL) return NULL;
      }
      if ((s=nstrcat(s,name+i+top+1))==NULL) return NULL;
    } else {
      if ((s=memalloc(strlen(name)+1))==NULL) return NULL;
      strcpy(s,name);
      pathresolv(s);
    }
  }
  return s;
}

#if 0
} /* dummy */
#endif

char *getbasename(char *name)
{
  char *s;
  int i;

  if (name==NULL) return NULL;
  changefilename(name);
  for (i=strlen(name);(name[i]!=DIRSEP) && (name[i]!=':') && (i!=0);i--);
  if ((name[i]==DIRSEP) || (name[i]==':')) i++;
  if ((s=memalloc(strlen(name)-i+1))==NULL) return NULL;
  strcpy(s,name+i);
  return s;
}

char *getextention(char *name)
{
  int i;

  if (name==NULL) return NULL;
  changefilename(name);
  for (i=strlen(name);(name[i]!='.') && (name[i]!=':') && (i!=0);i--);
  if (name[i]=='.') return name+i+1;
  return NULL;
}

char *getfilename(char *dir,char *sep,char *file)
{
  char *s;
  if ((s=memalloc(strlen(dir)+strlen(sep)+strlen(file)+1))==NULL) return NULL;
  strcpy(s,dir);
  if ((strlen(dir)>0) && ((s[strlen(dir)-1]=='/') || s[strlen(dir)-1]=='\\'))
    s[strlen(dir)-1]='\0';
  strcat(s,sep);
  strcat(s,file);
  changefilename(s);
  return s;
}

int findfilename(char *dir,char *sep,char *file)
{
  char *s;
  int find;
  struct stat buf;

  if ((s=getfilename(dir,sep,file))==NULL) return FALSE;
  if ((access(s,04)==0) && (stat(s,&buf)==0)) {
    if ((buf.st_mode & S_IFMT)==S_IFREG) find=TRUE;
    else find=FALSE;
  }
  else find=FALSE;
  memfree(s);
  return find;
}

char *ngetcwd()
{
  char *buf,*s;
  size_t size;

  size=0;
  buf=NULL;
  do {
    memfree(buf);
    size+=256;
    if ((buf=memalloc(size))==NULL) return NULL;
    s=getcwd(buf,size);
  } while ((s==NULL) && (size<=10240));
  if (size>10240) {
    memfree(buf);
    return NULL;
  }
  changefilename(s);
  return s;
}

#ifndef WINDOWS

char *nsearchpath(char *path,char *name,int shellscript)
{
  char *cmdname,*tok;
  int len;

  if (name==NULL) return NULL;
  if (name[0]=='\0') return NULL;
  cmdname=NULL;
  if (strchr(name,DIRSEP)==NULL) {
    while ((tok=getitok(&path,&len,PATHSEP))!=NULL) {
      memfree(cmdname);
      if ((cmdname=memalloc(strlen(name)+len+2))==NULL) return NULL;
      strncpy(cmdname,tok,len);
      if (cmdname[len-1]!=DIRSEP) {
        cmdname[len]=DIRSEP;
        len++;
      }
      strcpy(cmdname+len,name);
      if ((!shellscript && (access(cmdname,01)==0))
      || (shellscript && (access(cmdname,04)==0))) return cmdname;
    }
    if (tok==NULL) {
      memfree(cmdname);
      return NULL;
    }
  } else {
    if (!((!shellscript && (access(name,01)==0))
    || (shellscript && (access(name,04)==0)))) return NULL;
    if ((cmdname=memalloc(strlen(name)+1))==NULL) return NULL;
    strcpy(cmdname,name);
    return cmdname;
  }
  return NULL;
}

int nselectdir(char *dir,struct dirent *ent)
{
  char *s;
  struct stat sbuf;

  if ((s=nstrnew())==NULL) return 0;
  s=nstrcat(s,dir);
  if (s[strlen(s)+1]!='/') s=nstrccat(s,'/');
  s=nstrcat(s,ent->d_name);
  stat(s,&sbuf);
  memfree(s);
  if ((sbuf.st_mode & S_IFMT)==S_IFDIR) return 1;
  return 0;
}

int nselectfile(char *dir,struct dirent *ent)
{
  char *s;
  struct stat sbuf;

  if ((s=nstrnew())==NULL) return 0;
  s=nstrcat(s,dir);
  if (s[strlen(s)+1]!='/') s=nstrccat(s,'/');
  s=nstrcat(s,ent->d_name);
  stat(s,&sbuf);
  memfree(s);
  if ((sbuf.st_mode & S_IFMT)==S_IFREG) return 1;
  return 0;
}

#else

#define ADDEXENUM 6

char *addexechar[ADDEXENUM]={
  ".com",
  ".exe",
  ".bat",
  ".COM",
  ".EXE",
  ".BAT",
};

char *nsearchpath(char *path,char *name,int shellscript)
{
  char *cmdname,*tok;
  int i,k,len,len0;
  char *path3;
  char *path2;
  int pathlen;

  if (name==NULL) return NULL;
  if (name[0]=='\0') return NULL;
  changefilename(name);
  cmdname=NULL;
  if (strchr(name,DIRSEP)==NULL) {
    if (path==NULL) pathlen=0;
    else pathlen=strlen(path);
    if ((path2=memalloc(pathlen+4))==NULL) return NULL;
    strcpy(path2,".;");
    if (path!=NULL) strcat(path2,path);
    path3=path2;
    while ((tok=getitok(&path3,&len0,PATHSEP))!=NULL) {
      if (tok[0]!='\0') {
        memfree(cmdname);
        if ((cmdname=memalloc(strlen(name)+len0+6))==NULL) {
          memfree(path2);
          return NULL;
        }
        if ((strchr(name,'.')==NULL) && !shellscript) {
          for (k=0;k<ADDEXENUM;k++) {
            len=len0;
            strncpy(cmdname,tok,len);
            if ((cmdname[len-1]!=DIRSEP) && (cmdname[len-1]!='\\')) {
              cmdname[len]=DIRSEP;
              len++;
            }
            strcpy(cmdname+len,name);
            len+=strlen(name);
            strcpy(cmdname+len,addexechar[k]);
            unchangefilename(cmdname);
            if (access(cmdname,04)==0) {
              memfree(path2);
              return cmdname;
            }
          }
        } else {
          len=len0;
          strncpy(cmdname,tok,len);
          if ((cmdname[len-1]!=DIRSEP) && (cmdname[len-1]!='\\')) {
            cmdname[len]=DIRSEP;
            len++;
          }
          strcpy(cmdname+len,name);
          len+=strlen(name);
          unchangefilename(cmdname);
          if (access(cmdname,04)==0) {
            memfree(path2);
            return cmdname;
          }
        }
      }
    }
    if (tok==NULL) memfree(path2);
  } else {
    if ((cmdname=memalloc(strlen(name)+6))==NULL) memfree(path2);
    strcpy(cmdname,name);
    changefilename(cmdname);
    len=strlen(cmdname);
    for (i=len-1;(i>0) && (cmdname[i]!=DIRSEP);i--);
    if ((strchr(cmdname+i,'.')==NULL) && !shellscript) {
      for (k=0;k<ADDEXENUM;k++) {
        strcpy(cmdname+len,addexechar[k]);
        unchangefilename(cmdname);
        if (access(cmdname,04)==0) {
          return cmdname;
        }
      }
    } else {
      unchangefilename(cmdname);
      if (access(cmdname,04)==0) {
        return cmdname;
      }
    }
  }
  memfree(cmdname);
  return NULL;
}

#endif

int nscandir(char *dir,char ***namelist,
             int (*select)(char *dir,struct dirent *ent),
             int (*compar)())
{
  int i;
  DIR *dp;
  struct dirent *ent;
  char **po,**po2;
  unsigned int allocn=256,alloc=0;

  if ((dp=opendir(dir))==NULL) return -1;
  if ((po=malloc(allocn*sizeof(char *)))==NULL) return -1;
  while ((ent=readdir(dp))!=NULL) {
    if ((select!=NULL) && ((*select)(dir,ent)==0)) continue;
#ifndef WINDOWS
    if (ent->d_ino==0) continue;
#endif
    if (allocn==alloc) {
      if ((po2=realloc(po,(allocn+=256)*sizeof(char *)))==NULL) {
        for (i=0;i<alloc;i++) free(po[i]);
        free(po);
        return -1;
      }
      po=po2;
    }
    if ((po[alloc]=malloc(strlen(ent->d_name)+1))==NULL) {
      for (i=0;i<alloc;i++) free(po[i]);
      free(po);
      return -1;
    }
    strcpy(po[alloc],ent->d_name);
    alloc++;
  }
  closedir(dp);
  if (compar!=NULL) qsort(po,alloc,sizeof(struct dirent *),compar);
  *namelist=po;
  return alloc;
}

int nalphasort(char **a,char **b)
{
  return strcmp(*a,*b);
}

int nglob2(char *path,int po,int *num,char ***list)
{
  int i,j,p1,escape,scannum,len,err;
  char *s,*s1,*s2,*s3,*path2;
  char **namelist;

  if (path==NULL) return 0;
  p1=po;
  escape=FALSE;
  for (i=po;path[i]!='\0';i++) {
    if (escape) escape=FALSE;
    else if (path[i]=='\\') escape=TRUE;
    else if (path[i]==DIRSEP) p1=i+1;
    else if ((path[i]=='?') || (path[i]=='*')) break;
    else if (path[i]=='[') {
      for (j=i+1;(path[j]!='\0') && (path[j]!=']');j++) ;
      if (path[j]==']') {
        i=j;
        break;
      }
    }
  }
  if (path[i]=='\0') {
    if (access(path,04)==0) {
      if ((s=memalloc(strlen(path)+1))==NULL) return -1;
      strcpy(s,path);
      if (arg_add(list,s)==NULL) return -1;
      (*num)++;
    }
  } else {
    for (;(path[i]!='\0') && (path[i]!=DIRSEP);i++) ;
    s1 = memalloc(p1 + 1);
    s2 = memalloc(i - p1 + 1);
    s3 = memalloc(strlen(path) - i + 1);
    if (s1 == NULL || s2 == NULL || s3 == NULL) {
      memfree(s1);
      memfree(s2);
      memfree(s3);
      return -1;
    }
    strncpy(s1,path,p1);
    s1[p1]='\0';
    strncpy(s2,path+p1,i-p1);
    s2[i-p1]='\0';
    strcpy(s3,path+i);
    if (strlen(s1)==0) scannum=nscandir("./",&namelist,NULL,nalphasort);
    else scannum=nscandir(s1,&namelist,NULL,nalphasort);
    for (i=0;i<scannum;i++) {
      if (wildmatch(s2,namelist[i],WILD_PATHNAME | WILD_PERIOD)) {
        len=strlen(namelist[i]);
        if ((path2=memalloc(strlen(s1)+len+strlen(s3)+1))!=NULL) {
          strcpy(path2,s1);
          strcat(path2,namelist[i]);
          err=FALSE;
          if (s3[0]=='\0') {
            if ((s=memalloc(strlen(path2)+1))!=NULL) {
              strcpy(s,path2);
              if (arg_add(list,s)!=NULL) {
                (*num)++;
              } else err=TRUE;
            } else err=TRUE;
          } else {
            strcat(path2,s3);
            if (nglob2(path2,p1+len,num,list)==-1) err=TRUE;
          }
          if (err) {
            memfree(path2);
            for (j=i;j<scannum;j++) free(namelist[j]);
            if (scannum>0) free(namelist);
            memfree(s1);
            memfree(s2);
            memfree(s3);
            return -1;
          }
          memfree(path2);
        }
      }
      free(namelist[i]);
    }
    if (scannum>0) free(namelist);
    memfree(s1);
    memfree(s2);
    memfree(s3);
  }
  return 0;
}

int nglob(char *path,char ***namelist)
{
  int num;
  char *s;

  *namelist=NULL;
  num=0;
  if (nglob2(path,0,&num,namelist)==-1) {
    arg_del(*namelist);
    return -1;
  }
  if (num==0) {
    if ((s=memalloc(strlen(path)+1))==NULL) return -1;
    strcpy(s,path);
    if (arg_add(namelist,s)==NULL) return -1;
    return 1;
  }
  return num;
}

int fgetline(FILE *fp,char **buf)
{
/*
  rcode: 0 noerror
        -1 fatal error
         1 EOF
*/
  char *s;
  int ch, i;

  /* modified */

  *buf = NULL;
  ch = fgetc(fp);

  if (ch == EOF)
    return 1;

  s = nstrnew();
  if (s == NULL)
    return -1;

  i = 0;
  while (TRUE) {
#if EOF == -1
    if (C_type_buf[ch + 1]) {
      switch (ch) {
      case '\r':
	ch = fgetc(fp);
	if (ch != '\n') {
	  ungetc(ch, fp);
	}
	/* FALLTHRU */
      case '\0':
      case '\n':
      case EOF:
	s[i] = '\0'; /* nstraddchar() is not terminate string */
	*buf = s;
	return 0;
      }
    } else {
      s = nstraddchar(s, i, ch);
      i++;
      if (s == NULL)
	return -1;
    }
#else
    switch (ch) {
    case '\r':
      ch = fgetc(fp);
      if (ch != '\n') {
	ungetc(ch, fp);
      }
      /* FALLTHRU */
    case '\0':
    case '\n':
    case EOF:
      s[i] = '\0'; /* nstraddchar() is not terminate string */
      *buf = s;
      return 0;
    default:
      s = nstraddchar(s, i, ch);
      i++;
      if (s == NULL)
	return -1;
    }
#endif
    ch = fgetc(fp);
  }
}

int fgetnline(FILE *fp,char *buf,int len)
{
/*
  rcode: 0 noerror
         1 EOF
*/
  int i;
  int ch;

  buf[0]='\0';
  ch=fgetc(fp);
  if (ch==EOF) return 1;
  i=0;
  while (TRUE) {
    if ((ch=='\0') || (ch=='\n') || (ch==EOF)) {
      buf[i]='\0';
      return 0;
    }
    if ((i<len-1) && (ch!='\r')) buf[i++]=ch;
    ch=fgetc(fp);
  }
}

int nfgetc(FILE *fp)
{
  int ch;

  do {
    ch=fgetc(fp);
  } while (ch=='\r');
  return ch;
}

FILE *nfopen(char *filename, const char *mode)
{
  FILE *fp;

#ifdef WINDOWS
  unchangefilename(filename);
#endif
  fp=fopen(filename,mode);
  changefilename(filename);
  return fp;
}

#ifndef WINDOWS

int nisatty(int fd)
{
  return isatty(fd);
}

int nopen(char *path,int access,int mode)
{
  return open(path,access,mode);
}

void nclose(int fd)
{
  close(fd);
}

void nlseek(int fd,long offset,int fromwhere)
{
  lseek(fd,offset,fromwhere);
}

int nread(int fd,char *buf,unsigned len)
{
  return read(fd,buf,len);
}

int nwrite(int fd,char *buf,unsigned len)
{
  return write(fd,buf,len);
}

int nredirect(int fd,int newfd)
{
  int savefd;

  savefd=dup(fd);
  dup2(newfd,fd);
  close(newfd);
  return savefd;
}

void nredirect2(int fd,int savefd)
{
  dup2(savefd,fd);
  close(savefd);
}

int stdinfd(void)
{
  return 0;
}

int stdoutfd(void)
{
  return 1;
}

int stderrfd(void)
{
  return 2;
}

#else

int nisatty(HANDLE fd)
{
  DWORD flag;

  if (GetConsoleMode(fd,&flag)) return TRUE;
  return FALSE;
}

HANDLE nopen(char *path,int access,int mode)
{
  DWORD fdwAccess,fdwCreate;
  LONG DistanceToMove;
  HANDLE fd;
  SECURITY_ATTRIBUTES saAttr;
  DWORD fdwAttrsAndFlags;

  if (access&O_RDWR) fdwAccess=GENERIC_READ|GENERIC_WRITE;
  else if (access&O_WRONLY) fdwAccess=GENERIC_WRITE;
  else fdwAccess=GENERIC_READ;
  fdwCreate=0;
  if (access&O_CREAT) {
    if (access&O_TRUNC) fdwCreate=CREATE_ALWAYS;
    else fdwCreate=OPEN_ALWAYS;
  } else fdwCreate=OPEN_EXISTING;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;
  fdwAttrsAndFlags=FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL;
  unchangefilename(path);
  fd=CreateFile(path,fdwAccess,FILE_SHARE_READ,&saAttr,fdwCreate,fdwAttrsAndFlags,NULL);
  changefilename(path);
  if (access&O_APPEND) {
    DistanceToMove=0;
    SetFilePointer(fd,0L,&DistanceToMove,FILE_END);
  }
  return fd;
}

void nclose(HANDLE fd)
{
  CloseHandle(fd);
}

void nlseek(HANDLE fd,long offset,int fromwhere)
{
  LONG DistanceToMove;
  DWORD dwMoveMethod;

  if (fromwhere==SEEK_CUR) dwMoveMethod=FILE_CURRENT;
  else if (fromwhere==SEEK_END) dwMoveMethod=FILE_END;
  else if (fromwhere==SEEK_SET) dwMoveMethod=FILE_BEGIN;
  DistanceToMove=0;
  SetFilePointer(fd,offset,&DistanceToMove,dwMoveMethod);
}

int nread(HANDLE fd,char *buf,unsigned len)
{
  DWORD len2;
  int i,j;

  do {
    if (!ReadFile(fd,buf,len,&len2,NULL)) return -1;
    if (len2==0) return 0;
    j=0;
    for (i=0;i<len2;i++)
      if (buf[i]!='\r') buf[j++]=buf[i];
    if (j!=0) return j;
  } while (j==0);
  return 0;
}

int nwrite(HANDLE fd,char *buf,unsigned len)
{
  DWORD len2;
  int i,j,num;
  char *buf2;

  num=0;
  for (i=0;i<len;i++) if (buf[i]=='\n') num++;
  buf2=memalloc(len+num);
  if (buf2==NULL) return 0;
  j=0;
  for (i=0;i<len;i++) {
    if (buf[i]=='\n') buf2[j++]='\r';
    buf2[j++]=buf[i];
  }
  WriteFile(fd,buf2,j,&len2,NULL);
  memfree(buf2);
  return len2-num;
}

HANDLE nredirect(int fd,HANDLE newfd)
{
  HANDLE savefd;

  switch (fd) {
  case 0:
    savefd=GetStdHandle(STD_INPUT_HANDLE);
    SetStdHandle(STD_INPUT_HANDLE,newfd);
    break;
  case 1:
    savefd=GetStdHandle(STD_OUTPUT_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE,newfd);
    break;
  case 2:
    savefd=GetStdHandle(STD_ERROR_HANDLE);
    SetStdHandle(STD_ERROR_HANDLE,newfd);
    break;
  }
  return savefd;
}

void nredirect2(int fd,HANDLE savefd)
{
  HANDLE oldfd;

  switch (fd) {
  case 0:
    oldfd=GetStdHandle(STD_INPUT_HANDLE);
    CloseHandle(oldfd);
    SetStdHandle(STD_INPUT_HANDLE,savefd);
    break;
  case 1:
    oldfd=GetStdHandle(STD_OUTPUT_HANDLE);
    CloseHandle(oldfd);
    SetStdHandle(STD_OUTPUT_HANDLE,savefd);
    break;
  case 2:
    oldfd=GetStdHandle(STD_ERROR_HANDLE);
    CloseHandle(oldfd);
    SetStdHandle(STD_ERROR_HANDLE,savefd);
    break;
  }
}

HANDLE stdinfd(void)
{
  return GetStdHandle(STD_INPUT_HANDLE);
}

HANDLE stdoutfd(void)
{
  return GetStdHandle(STD_OUTPUT_HANDLE);
}

HANDLE stderrfd(void)
{
  return GetStdHandle(STD_ERROR_HANDLE);
}

#endif

void
set_progress_func(void (* func)(int, char *, double))
{
  ShowProgressFunc = func;
}

void
set_progress(int pos, char *msg, double fraction)
{
  if (ShowProgressFunc)
    ShowProgressFunc(pos, msg, fraction);
}

