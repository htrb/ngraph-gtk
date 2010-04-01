/* 
 * $Id: ioutil.h,v 1.10 2010/04/01 06:08:22 hito Exp $
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

#ifndef IOUTIL_HEADER
#define IOUTIL_HEADER

#include "common.h"

#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>

#define DIRSEP '/'
#define CONFSEP "/"
#define CONFTOP "/"
#define NOHANDLE -1

#ifndef WINDOWS
#define PATHSEP ":"
#define NFMODE (S_IRUSR | S_IWUSR)
#define NFMODE_NORMAL_FILE (NFMODE | S_IRGRP | S_IROTH)
#else	/* WINDOWS */
#define PATHSEP ";"
#define NFMODE (S_IREAD | S_IWRITE)
#define NFMODE_NORMAL_FILE NFMODE
#endif	/* WINDOWS */

void changefilename(char *name);
void path_to_win(char *name);
void char_type_buf_init(void);
char *getfullpath(const char *name);
char *getrelativepath(const char *name);
char *getbasename(const char *name);
char *getdirname(const char *name);
char *getextention(char *name);
char *getfilename(char *dir,char *sep,char *file);
int findfilename(char *dir,char *sep,char *file);
char *ngetcwd(void);
char *nsearchpath(char *path,char *name,int shellscript);
int nglob(char *path,char ***namelist);
int fgetline(FILE *fp,char **buf);
int fgetnline(FILE *fp,char *buf,int len);
int nfgetc(FILE *fp);
int nisatty(int fd);
FILE *nfopen(const char *filename,const char *mode);
int nopen(const char *path,int access,int mode);
void nclose(int fd);
int nredirect(int fd,int newfd);
void nredirect2(int fd,int savefd);
void nlseek(int fd,long offset,int fromwhere);
int nread(int fd,char *buf,unsigned len);
int nwrite(int fd,char *buf,unsigned len);
int stdinfd(void);
int stdoutfd(void);
int stderrfd(void);
void set_progress_func(void (* func)(int, char *, double));
void set_progress(int pos, char *msg, double fraction);
int n_mkstemp(const char *dir, char *templ, char **name);
int nstat(const gchar *filename, struct stat *buf);
int naccess(const gchar *filename, int mode);
int nchdir(const gchar *path);
char *get_utf8_filename(const char *name);
char *get_localized_filename(const char *name);

#endif	/* IOUTIL_HEADER */
