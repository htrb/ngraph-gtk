/*
 * $Id: ioutil.c,v 1.25 2010-04-01 06:08:22 hito Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include "object.h"
#include "nstring.h"
#include "ioutil.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif


static void (* ShowProgressFunc)(int, const char *, double) = NULL;

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

#define is_line_sep(ch) (C_type_buf[ch + 1])
#endif

void
changefilename(char *name)
{
#if WINDOWS
  int i;

  if (name == NULL) {
    return;
  }

  for (i = 0; name[i] != '\0'; i++) {
    if (name[i] == '\\') {
      name[i] = DIRSEP;
    }
  }
#endif  /* WINDOWS */
}

void
path_to_win(char *name)
{
#if WINDOWS
  int i;

  if (name == NULL) {
    return;
  }

  for (i = 0; name[i] != '\0'; i++) {
    if (name[i] == DIRSEP) {
      name[i] = '\\';
    }
  }
#endif  /* WINDOWS */
}

static void
pathresolv(char *name)
{
  int j,k;

  if (name == NULL) {
    return;
  }
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

char *
getfullpath(const char *name)
{
  char *s, *utf8_name;

  if (name == NULL) {
    return NULL;
  }

  utf8_name = get_utf8_filename(name);
  if (utf8_name == NULL) {
    return NULL;
  }

  changefilename(utf8_name);

  if (utf8_name[0] == DIRSEP && utf8_name[1] == DIRSEP) {
    s = utf8_name;
  } else {
    int top;
    if (isalpha(utf8_name[0]) && utf8_name[1] == ':') {
      top = 2;
    } else {
      top = 0;
    }
    if (utf8_name[top] == DIRSEP) {
      s = utf8_name;
    } else {
      char *cwd;
      cwd = ngetcwd();
      if (cwd == NULL) {
	g_free(utf8_name);
	return NULL;
      }
      s = g_strdup_printf("%s%c%s", cwd, DIRSEP, utf8_name + top);
      g_free(cwd);
      g_free(utf8_name);
    }
  }
  pathresolv(s);
  return s;
}

#if WINDOWS
static int
getdisk(void)
{
  char *cwd;
  int drive;

  cwd = g_get_current_dir();
  if (cwd == NULL) {
    return -1;
  }

  drive = toupper(cwd[0]) - 'A';

  g_free(cwd);

  return drive;
}
#endif	/* WINDOWS */

char *
getrelativepath(const char *name)
{
  char *utf8_name, *str;

  if (name == NULL) {
    return NULL;
  }

  utf8_name = get_utf8_filename(name);
  if (utf8_name == NULL) {
    return NULL;
  }

  if ((utf8_name[0]==DIRSEP) && (utf8_name[1]==DIRSEP)) {
    str = utf8_name;
    pathresolv(str);
  } else {
    int top;

    top = 0;
#if WINDOWS
    if (isalpha(utf8_name[0]) && name[1] == ':') {
      top = 2;
    }
#endif	/* WINDOWS */
    if (
#if WINDOWS
	name[top] == DIRSEP && (top != 2 || toupper(name[0]) - 'A' == getdisk())
#else  /* WINDOWS */
	name[0] == DIRSEP
#endif	/* WINDOWS */
	) {
      GString *s;
      char *cwd, *cwd2;
      int i, j, depth;

      cwd = ngetcwd();
      if (cwd == NULL) {
	g_free(utf8_name);
	return NULL;
      }
#if WINDOWS
      for (j = 2; cwd[j] != '\0'; j++) {
	cwd[j - 2] = cwd[j];
      }
      cwd[j - 2] = '\0';
#endif	/* WINDOWS */

      cwd2 = g_malloc(strlen(cwd) + 2);
      if (cwd2 == NULL) {
	g_free(cwd);
	g_free(utf8_name);
	return NULL;
      }
      strcpy(cwd2, cwd);
      g_free(cwd);
      i=strlen(cwd2);
      if ((i==0) || (cwd2[i-1]!=DIRSEP)) {
	cwd2[i]=DIRSEP;
	cwd2[i+1]='\0';
      }

      for (i=0;(cwd2[i]!='\0') && (utf8_name[i+top]!='\0');i++) {
	if (cwd2[i]!=utf8_name[i+top]) {
	  break;
	}
      }

      if (i > 0) {
	i--;
      }
      for (;cwd2[i] != DIRSEP;i--);
      depth=0;
      for (j=strlen(cwd2); j != i; j--) {
	if (cwd2[j] == DIRSEP) {
	  depth++;
	}
      }
      g_free(cwd2);

      s = g_string_sized_new(64);
      if (s == NULL) {
	g_free(utf8_name);
	return NULL;
      }

      if (depth==0) {
	g_string_append(s, "./");
      } else {
	for (j = 0; j < depth; j++) {
	  g_string_append(s, "../");
	}
      }

      g_string_append(s, utf8_name + i + top + 1);
      g_free(utf8_name);
      str = g_string_free(s, FALSE);
    } else {
      str = utf8_name;
      pathresolv(str);
    }
  }

  return str;
}

char *
get_utf8_filename(const char *name)
{
  char *utf8_name;

  if (name == NULL) {
    return NULL;
  }

  if (g_utf8_validate(name, -1, NULL)) {
    utf8_name = g_strdup(name);
  } else {
#if WINDOWS
    utf8_name = g_locale_to_utf8(name, -1, NULL, NULL, NULL);
#else  /* WINDOWS */
    utf8_name = g_filename_to_utf8(name, -1, NULL, NULL, NULL);
#endif	/* WINDOWS */
  }
  if (utf8_name == NULL) {
    /* for compatibility with Ngraph for Windows */
    utf8_name = g_convert(name, -1, "utf-8", "CP932", NULL, NULL, NULL);
  }
  if (utf8_name == NULL) {
    utf8_name = g_strdup(name);
  }
  changefilename(utf8_name);

  return utf8_name;
}

char *
get_localized_filename(const char *name)
{
  char *localized_name;

  if (name == NULL) {
    return NULL;
  }

#if WINDOWS
  if (g_utf8_validate(name, -1, NULL)) {
    localized_name = g_strdup(name);
  } else {
    localized_name = g_locale_to_utf8(name, -1, NULL, NULL, NULL);
  }
#else  /* WINDOWS */
  if (g_utf8_validate(name, -1, NULL)) {
    localized_name = g_filename_from_utf8(name, -1, NULL, NULL, NULL);
  } else {
    localized_name = g_strdup(name);
  }
#endif	/* WINDOWS */

  return localized_name;
}

char *
getbasename(const char *name)
{
  char *basename, *tmp;

  if (name == NULL)
    return NULL;

  tmp = get_utf8_filename(name);
  if (tmp == NULL) {
    return NULL;
  }

  basename = g_path_get_basename(tmp);
  g_free(tmp);

  return basename;
}

char *
getdirname(const char *name)
{
  char *dirname, *tmp;

  if (name == NULL)
    return NULL;

  tmp = get_utf8_filename(name);
  if (tmp == NULL) {
    return NULL;
  }

  dirname = g_path_get_dirname(tmp);
  g_free(tmp);

  return dirname;
}

char *
getextention(char *name)
{
  int i;

  if (name==NULL) return NULL;
  changefilename(name);
  for (i=strlen(name);(name[i]!='.') && (name[i]!=':') && (i!=0);i--);
  if (name[i]=='.') return name+i+1;
  return NULL;
}

char *
getfilename(const char *dir, const char *sep, const char *file)
{
  char *s;
  unsigned int dir_len;

  dir_len = strlen(dir);

  s = g_malloc(dir_len + strlen(sep) + strlen(file) + 1);
  if (s == NULL)
    return NULL;

  strcpy(s, dir);

  if (dir_len > 0 && (s[dir_len - 1] == '/' || s[dir_len - 1] == '\\'))
    s[dir_len - 1]='\0';

  strcat(s, sep);
  strcat(s, file);
  changefilename(s);
  return s;
}

int
findfilename(const char *dir, const char *sep, const char *file)
{
  char *s;
  int find;
  GStatBuf buf;

  if ((s=getfilename(dir,sep,file))==NULL) return FALSE;
  if ((naccess(s,R_OK)==0) && (nstat(s,&buf)==0)) {
    if ((buf.st_mode & S_IFMT)==S_IFREG) find=TRUE;
    else find=FALSE;
  }
  else find=FALSE;
  g_free(s);
  return find;
}

char *
ngetcwd(void)
{
  char *s;

  s = g_get_current_dir();

  changefilename(s);

  return s;
}

char *
nsearchpath(char *path,char *name,int shellscript)
{
  char *cmdname;
  int len;

  if (name==NULL) return NULL;
  if (name[0]=='\0') return NULL;

  cmdname = g_find_program_in_path(name);
  if (cmdname) {
    return cmdname;
  }

  if (strchr(name,DIRSEP)==NULL) {
    char *tok;
    while ((tok=getitok(&path,&len,PATHSEP))!=NULL) {
      g_free(cmdname);
      if ((cmdname=g_malloc(strlen(name)+len+2))==NULL) return NULL;
      strncpy(cmdname,tok,len);
      if (cmdname[len-1]!=DIRSEP) {
        cmdname[len]=DIRSEP;
        len++;
      }
      strcpy(cmdname+len,name);
      if ((!shellscript && (naccess(cmdname,X_OK)==0))
      || (shellscript && (naccess(cmdname,R_OK)==0))) return cmdname;
    }
    if (tok==NULL) {
      g_free(cmdname);
      return NULL;
    }
  } else {
    if (!((!shellscript && (naccess(name,X_OK)==0))
    || (shellscript && (naccess(name,R_OK)==0)))) return NULL;
    if ((cmdname=g_malloc(strlen(name)+1))==NULL) return NULL;
    strcpy(cmdname,name);
    return cmdname;
  }
  return NULL;
}

static int
nscandir(char *dir,char ***namelist, int (*compar)())
{
  unsigned int i;
  GDir *dp;
  const char *ent;
  char **po,**po2;
  unsigned int allocn = 256, alloc = 0;

  dp = g_dir_open(dir, 0, NULL);
  if (dp == NULL) {
    return -1;
  }

  po = g_malloc(allocn * sizeof(char *));
  if (po == NULL) {
    g_dir_close(dp);
    return -1;
  }

  while ((ent = g_dir_read_name(dp))) {
    if (allocn == alloc) {
      allocn += 256;
      po2 = g_realloc(po, allocn * sizeof(char *));
      if (po2 == NULL) {
        for (i = 0; i < alloc; i++) {
	  g_free(po[i]);
	}
        g_free(po);
	g_dir_close(dp);
        return -1;
      }
      po = po2;
    }
    po[alloc] = g_strdup(ent);
    if (po[alloc] == NULL) {
      for (i = 0; i < alloc; i++) {
	g_free(po[i]);
      }
      g_free(po);
      g_dir_close(dp);
      return -1;
    }
    alloc++;
  }
  g_dir_close(dp);

  if (compar != NULL) {
    qsort(po, alloc, sizeof(struct dirent *), compar);
  }

  *namelist = po;
  return alloc;
}

static int
nalphasort(char **a,char **b)
{
  return strcmp(*a,*b);
}

static int
nglob2(char *path,int po,int *num,char ***list)
{
  int i,j,p1,escape;
  char **namelist, *s;

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
    if (naccess(path,R_OK)==0) {
      if ((s=g_malloc(strlen(path)+1))==NULL) return -1;
      strcpy(s,path);
      if (arg_add(list,s)==NULL) return -1;
      (*num)++;
    }
  } else {
    int scannum,len,err;
    char *s1,*s2,*s3,*path2;
    for (;(path[i]!='\0') && (path[i]!=DIRSEP);i++) ;
    s1 = g_malloc(p1 + 1);
    s2 = g_malloc(i - p1 + 1);
    s3 = g_malloc(strlen(path) - i + 1);
    if (s1 == NULL || s2 == NULL || s3 == NULL) {
      g_free(s1);
      g_free(s2);
      g_free(s3);
      return -1;
    }
    strncpy(s1,path,p1);
    s1[p1]='\0';
    strncpy(s2,path+p1,i-p1);
    s2[i-p1]='\0';
    strcpy(s3,path+i);
    if (strlen(s1)==0) scannum=nscandir("./",&namelist,nalphasort);
    else scannum=nscandir(s1,&namelist,nalphasort);
    for (i=0;i<scannum;i++) {
      if (wildmatch(s2,namelist[i],WILD_PATHNAME | WILD_PERIOD)) {
        len=strlen(namelist[i]);
        if ((path2=g_malloc(strlen(s1)+len+strlen(s3)+1))!=NULL) {
          strcpy(path2,s1);
          strcat(path2,namelist[i]);
          err=FALSE;
          if (s3[0]=='\0') {
            if ((s=g_malloc(strlen(path2)+1))!=NULL) {
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
            g_free(path2);
            for (j=i;j<scannum;j++) g_free(namelist[j]);
            if (scannum>0) g_free(namelist);
            g_free(s1);
            g_free(s2);
            g_free(s3);
            return -1;
          }
          g_free(path2);
        }
      }
      g_free(namelist[i]);
    }
    if (scannum>0) g_free(namelist);
    g_free(s1);
    g_free(s2);
    g_free(s3);
  }
  return 0;
}

int
nglob(char *path,char ***namelist)
{
  int num;

  *namelist=NULL;
  num=0;
  if (nglob2(path,0,&num,namelist)==-1) {
    arg_del(*namelist);
    return -1;
  }
  if (num==0) {
    char *s;
    s = g_strdup(path);
    if (s == NULL) return -1;
    if (arg_add(namelist,s)==NULL) return -1;
    num = 1;
  }
  return num;
}

int
fgetline(FILE *fp, char **buf)
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

  if (ch == EOF) {
    *buf = NULL;
    return 1;
  }

  s = nstrnew();
  if (s == NULL) {
    *buf = NULL;
    return -1;
  }

  i = 0;
  while (TRUE) {
#if EOF == -1
    if (is_line_sep(ch)) {
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
      if (s == NULL) {
	*buf = NULL;
	return -1;
      }
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
      if (s == NULL) {
	*buf = NULL;
	return -1;
      }
    }
#endif
    ch = fgetc(fp);
  }
}

int
fgetnline(FILE *fp, char *buf, int len)
{
/*
  rcode: 0 noerror
        -1 fatal error
         1 EOF
*/
  int rcode;
  char *ptr;

  buf[0] = '\0';

  rcode = fgetline(fp, &ptr);
  if (rcode)
    return rcode;

  strncpy(buf, ptr, len);
  buf[len - 1] = '\0';

  g_free(ptr);

  return 0;
}

int
nfgetc(FILE *fp)
{
  int ch;

  do {
    ch=fgetc(fp);
  } while (ch=='\r');
  return ch;
}

FILE *
nfopen(const char *filename, const char *mode)
{
  FILE *fp;
  char *tmp;

  if (filename == NULL)
    return NULL;

  tmp = get_localized_filename(filename);
  if (tmp == NULL) {
    return NULL;
  }

  fp = g_fopen(tmp, mode);
  g_free(tmp);

  return fp;
}

int
nisatty(int fd)
{
  return isatty(fd);
}

int
nstat(const gchar *filename, GStatBuf *buf)
{
  int r;
  char *tmp;

  if (filename == NULL || buf == NULL)
    return -1;

  tmp = get_localized_filename(filename);
  if (tmp == NULL) {
    return -1;
  }

  r = g_stat(tmp, buf);
  g_free(tmp);

  return r;
}

int
naccess(const gchar *filename, int mode)
{
  int r;
  char *tmp;

  if (filename == NULL)
    return -1;

  tmp = get_localized_filename(filename);
  if (tmp == NULL) {
    return -1;
  }

  r =  g_access(tmp, mode);
  g_free(tmp);

  return r;
}

int
nchdir(const gchar *path)
{
  int r;
  char *tmp;

  if (path == NULL)
    return -1;

  tmp = get_localized_filename(path);
  if (tmp == NULL) {
    return -1;
  }

  r =  g_chdir(tmp);
  g_free(tmp);

  return r;
}

int
nopen(const char *path,int access,int mode)
{
  int r;
  char *tmp;

  if (path == NULL)
    return -1;

  tmp = get_localized_filename(path);
  if (tmp == NULL) {
    return -1;
  }

  r =  g_open(tmp, access, mode | O_BINARY);
  g_free(tmp);

  return r;
}

void
nclose(int fd)
{
  close(fd);
}

void
nlseek(int fd,long offset,int fromwhere)
{
  lseek(fd,offset,fromwhere);
}

int
nread(int fd,char *buf,unsigned len)
{
  return read(fd,buf,len);
}

int
nwrite(int fd,char *buf,unsigned len)
{
  return write(fd,buf,len);
}

int
nredirect(int fd,int newfd)
{
  int savefd;

  savefd=dup(fd);
  dup2(newfd,fd);
  close(newfd);
  return savefd;
}

void
nredirect2(int fd,int savefd)
{
  dup2(savefd,fd);
  close(savefd);
}

int
stdinfd(void)
{
  return 0;
}

int
stdoutfd(void)
{
  return 1;
}

int
stderrfd(void)
{
  return 2;
}

void
set_progress_func(void (* func)(int, const char *, double))
{
  ShowProgressFunc = func;
}

void
set_progress(int pos, char *msg, double fraction)
{
  if (ShowProgressFunc)
    ShowProgressFunc(pos, msg, fraction);
}

int
n_mkstemp(const char *dir, char *templ, char **name)
{
  char postfix[] = "XXXXXX", *buf, *path;
  int len, fd, path_last;
#ifdef S_IRWXG
  mode_t mask_prev;
#endif

  dir = (dir) ? dir : g_get_tmp_dir();

  path = g_strdup(dir);
  if (path == NULL) {
    return -1;
  }

  len = strlen(path);
  if (len > 1) {
    path_last = path[len - 1];
  } else {
    path_last = '\0';
  }

  changefilename(path);
  buf = g_strdup_printf("%s%s%s%s", path, (path_last == '/') ? "" : "/", CHK_STR(templ), postfix);

  g_free(path);

#ifdef S_IRWXG
  mask_prev = umask(S_IRWXG | S_IRWXO);
#endif

  fd = g_mkstemp(buf);

#ifdef S_IRWXG
  umask(mask_prev);
#endif

  if (fd < 0) {
    g_free(buf);
    buf = NULL;
  }

  *name = buf;

  return fd;
}

FILE *
n_tmpfile(char **name)
{
  int fd;
  FILE *fp;

  fd = n_mkstemp(NULL, "ntmp", name);
  if (fd < 0) {
    return NULL;
  }

#if ! WINDOWS
  if (*name) {
    g_unlink(*name);
  }
#endif

  fp = fdopen(fd, "w+b");
  if (fp == NULL) {
    close(fd);
    if (*name) {
#if WINDOWS
      g_unlink(*name);
#endif
      g_free(*name);
      *name = NULL;
    }
  }

  return fp;
}

void
n_tmpfile_close(FILE *fp, char *name)
{
  if (fp){
    fclose(fp);
  }

  if (name) {
#if WINDOWS
    g_unlink(name);
#endif
    g_free(name);
  }
}
