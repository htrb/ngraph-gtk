#ifndef _SHELL_HEADER
#define _SHELL_HEADER
/* 
 * $Id: shell.h,v 1.14 2010/03/04 08:30:16 hito Exp $
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
#include "ioutil.h"

#define SHELLBUFSIZE 4096

#define ERRUEXPEOF	100
#define ERRBADSUB	101
#define ERRSYNTAX	102
#define ERRUEXPTOK	103
#define ERRIDENT	104
#define ERRCFOUND	105
#define ERRSYSTEM	106
#define ERREXTARG	107
#define ERRSMLARG	108
#define ERRMANYARG	109
#define ERRNOFILE	110
#define ERREOF		111
#define ERRNOFIL	112
#define ERRREAD		113
#define ERROPEN		114
#define ERROBJARG	115
#define ERRINSTARG	116
#define ERRNEWINST	117
#define ERRCMFIELD	118
#define ERRVALUE	119
#define ERRTWOINST	120
#define ERRNONEINST	121
#define ERRNOFIELD	122
#define ERRNUMERIC	123
#define ERROPTION	124
#define ERRILOPS	125
#define ERRUNSET	126
#define ERRNODIR	127
#define ERRMSYNTAX	128
#define ERRMILLEGAL	129
#define ERRMNEST	130
#define ERRMARG		131
#define ERRMFAT		132
#define ERRTESTNEST	133
#define ERRTESTSYNTAX	134
#define ERRSECURITY	135
#define ERRMEMORY	136
#define ERRUNKNOWNSH	137

struct prmlist;
struct prmlist {
  struct prmlist *next;
  char *str;
  int prmno;
  int quoted;
};

struct cmdlist;
struct cmdlist {
  struct cmdlist *next;
  int cmdno;
  int cmdend;
  struct prmlist *prm;
  void *done;
  char *pipefile;
};

struct vallist;
struct vallist {
  struct vallist *next;
  char *name;
  void *val;
  int func;
  int arg;
};

struct explist;
struct explist {
  char *val;
  struct explist *next;
};

struct nshell {
  struct objlist *obj;
#if USE_HASH
  NHASH exproot, valroot;
#else
  struct vallist *valroot;
  struct explist *exproot;
#endif
  int argc;
  char **argv;
  int cmdexec;
  int status;
  int quit;
  int options;
  int optionf;
  int optione;
  int optionv;
  int optionx;
  int fd;
  char *readbuf;
  int readpo;
  int readbyte;
  int deleted;
  int (*sgetstdin)();
  int (*sputstdout)(char *s);
  int (*sprintfstdout)(char *fmt,...);
};

extern int CMDNUM;

typedef int (* shell_proc)(struct nshell *nshell,int argc,char **argv);

struct cmdtabletype {
  char *name;
  shell_proc proc;
};
extern struct cmdtabletype cmdtable[];

extern int CPCMDNUM;
extern char *cpcmdtable[];

shell_proc check_cmd(char *name);
int check_cpcmd(char *name);
int init_cmd_tbl(void);

int eval_script(const char *script, int security);
char *addval(struct nshell *nshell,char *name,char *val);
char *addexp(struct nshell *nshell,char *name);
int delval(struct nshell *nshell,char *name);
char *getval(struct nshell *nshell,char *name);
struct cmdlist *getfunc(struct nshell *nshell,char *name);
void setshhandle(struct nshell *shell, int fd);
int getshhandle(struct nshell *nshell);
int cmdexecute(struct nshell *nshell, const char *cline);
struct nshell *newshell(void);
void delshell(struct nshell *nshell);
void sherror(int code);
void sherror2(int code,char *mes);
void sherror3(char *cmd,int code,char *mes);
void sherror4(char *cmd,int code);
void shellsavestdio(struct nshell *nshell);
void shellrestorestdio(struct nshell *nshell);
int setshelloption(struct nshell *nshell,char *opt);
int getshelloption(struct nshell *nshell,char opt);
void setshellargument(struct nshell *nshell,int argc,char **argv);
int printfconsole(char *fmt,...);
void ngraphenvironment(struct nshell *nshell);
int msleep(int ms);
void set_security(int state);
void set_environ(char **env);
int set_signal(int signal, int flags, void (*handler)(int));
void set_childhandler(void);
void unset_childhandler(void);
#ifdef WINDOWS
typedef struct {
  int pid;
  int errlevel;
  int done;
} WaitPidParam;

int waitpid(WaitPidParam *pWP);
#endif	/* WINDOWS */
#endif
