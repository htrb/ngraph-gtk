#ifndef _SHELL_HEADER
#define _SHELL_HEADER
/*
 * $Id: shell.h,v 1.14 2010-03-04 08:30:16 hito Exp $
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
  int (*sputstdout)(const char *s);
  int (*sprintfstdout)(const char *fmt,...);
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

void nsleep(double a);
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
int set_shell_args(struct nshell *nshell, int j, const char *argv0, int argc, char **argv);
void setshellargument(struct nshell *nshell,int argc,char **argv);
int printfconsole(const char *fmt,...);
void ngraphenvironment(struct nshell *nshell);
int msleep(int ms);
void set_security(int state);
int get_security(void);
void set_environ(void);
void set_childhandler(void);
void unset_childhandler(void);
int system_bg(char *cmd);
void set_interrupt(void);
int check_interrupt(void);
void reset_interrupt(void);
#if WINDOWS
void show_system_error(void);
#else
int set_signal(int signal, int flags, void (*handler)(int), struct sigaction *oldact);
#endif	/* WINDOWS */
#endif
