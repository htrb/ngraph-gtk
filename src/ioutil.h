/* 
 * $Id: ioutil.h,v 1.3 2008/06/10 04:21:33 hito Exp $
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

#include <sys/stat.h>
#include <dirent.h>

#ifndef WINDOWS
#define DIRSEP '/'
#define PATHSEP ":"
#define CONFSEP "/"
#define CONFTOP "/."
typedef int HANDLE;
#define NOHANDLE -1
#define NFMODE 384
#else
#define DIRSEP '/'
#define PATHSEP ";"
#define CONFSEP "/"
#define CONFTOP "/_"
typedef void *HANDLE;
#define NOHANDLE NULL
#define NFMODE S_IREAD|S_IWRITE
#endif

void changefilename(char *name);
#ifdef WINDOWS
void unchangefilename(char *name);
#endif
char *getfullpath(char *name);
char *getrelativepath(char *name);
char *getbasename(char *name);
char *getextention(char *name);
char *getfilename(char *dir,char *sep,char *file);
int findfilename(char *dir,char *sep,char *file);
char *ngetcwd(void);
char *nsearchpath(char *path,char *name,int shellscript);
int nglob(char *path,char ***namelist);
int fgetline(FILE *fp,char **buf);
int fgetnline(FILE *fp,char *buf,int len);
int nfgetc(FILE *fp);
int nisatty(HANDLE fd);
FILE *nfopen(char *filename,const char *mode);
HANDLE nopen(char *path,int access,int mode);
void nclose(HANDLE fd);
HANDLE nredirect(int fd,HANDLE newfd);
void nredirect2(int fd,HANDLE savefd);
void nlseek(HANDLE fd,long offset,int fromwhere);
int nread(HANDLE fd,char *buf,unsigned len);
int nwrite(HANDLE fd,char *buf,unsigned len);
HANDLE stdinfd(void);
HANDLE stdoutfd(void);
HANDLE stderrfd(void);
void set_progress_func(void (* func)(int, char *, double));
void set_progress(int pos, char *msg, double fraction);
