/*
 * $Id: shell.c,v 1.41 2010-04-01 06:08:23 hito Exp $
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

/*
 * SYNTAX:
 *
 *    name1=val1 name2=val2 ... command arg1 arg2 arg3 ...
 *
 *    # comment
 *
 *    keyword
 *        for name [in word] do list done
 *        case word in [patterm [|pattern] ...list;;] ... esac
 *        if list then list [elif list then list]...[else list] fi
 *        while list do list done
 *        until list do list done
 *        name() { list; } * { } is needed
 *
 *    command replacement
 *        `command`
 *
 *    parameter replacement
 *        name=value [ name=value ] ...
 *        $# $? $num $* $@
 *        ${parameter}
 *        ${parameter:-word}
 *        ${parameter:=word}
 *        ${parameter:?word}
 *        ${parameter:+word}
 *        ${parameter#word}
 *        ${parameter##word}
 *        ${parameter%word}
 *        ${parameter%%word}
 *
 *    object replacement
 *        object:namelist:field=value
 *        ${object:namelist:field=argument}
 *
 *     input & output
 *        < file
 *        << word
 *        > file
 *        >> file
 *
 *     special command
 *        :
 *        .file
 *        break [n]
 *        continue [n]
 *        cd
 *        echo
 *        eval [arg]
 *        exit [n]
 *        export [name]
 *        pwd
 *        quit
 *        read name ...
 *        return [n]
 *        set
 *        shift [n]
 *        test
 *        unset
 *        [
 *        new      object [field:val, ...]
 *        del      object namelist
 *        exist    object namelist
 *        get      object namelist field:argument ...
 *        put      object namelist field:val ...
 *        exe      object namelist field:argument ...
 *        cpy      object sname dnamelist field
 *        move     object sname dname
 *        movetop  object name
 *        moveup   object name
 *        movedown object name
 *        movelast object name
 *        exch     object name1 name2
 *        dexpr
 *        iexpr
 *
 */

#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <glib.h>
#include <unistd.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
static char *Prompt;
static int MultiLine = FALSE;
#endif	/* HAVE_READLINE_READLINE_H */

#if ! WINDOWS
#include <sys/wait.h>
#include <sys/ioctl.h>
#endif	/* WINDOWS */
#include <sys/time.h>

#define USE_HASH 1

#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "shell.h"
#include "shell_error.h"
#include "shellcm.h"
#include "shellux.h"
#include "math/math_equation.h"

#define TEMPPFX "NGS"

static char *cmderrorlist[] = {
  "unexpected EOF.",
  "bad substitution.",
  "syntax error near unexpected token",
  "unexpected token",
  "invalid identifier",
  "command not found",
  "system call",
  "extra arguments.",
  "not enough argument.",
  "too many arguments.",
  "cannot open file",
  "Use \"exit\" to leave the shell.",
  "no such file",
  "I/O error: read file",
  "I/O error: open file",
  "missing object argument.",
  "missing instance argument.",
  "new instance is specified",
  "illegal field identifier",
  "illegal type of argument",
  "illegal number of specified instance.",
  "no instance specified",
  "no field identifier.",
  "non-numeric argument",
  "unknown option",
  "illegal option",
  "cannot unset",
  "no such directory",
  "syntax error for math.",
  "not allowed function for math.",
  "sum() or dif(): deep nest for math.",
  "illegal argument for math.",
  "fatal error for math.",
  "deep nest.",
  "syntax error.",
  "security check.",
  "cannot allocate enough memory.",
  "unknown error.",
};

#define ERRNUM (sizeof(cmderrorlist) / sizeof(*cmderrorlist))

static char **MainEnviron;
static int Security=FALSE;

#define WRITEBUFSIZE 4096

static char writebuf[WRITEBUFSIZE];
static int writepo;
static int Interrupted = FALSE;

static int storeshhandle(struct nshell *nshell,int fd, char **readbuf,int *readbyte,int *readpo);
static void restoreshhandle(struct nshell *nshell,int fd, char *readbuf,int readbyte,int readpo);
static int cmdexec(struct nshell *nshell,struct cmdlist *cmdroot,int namedfunc);
static char *expand(struct nshell *nshell,char *str,int *quote,int *bquote, int ifsexp);

typedef struct {
  int sleep;
  int msec;
} ThreadParam;

gpointer
sleep_thread(gpointer data)
{
  ThreadParam *th;

  th = (ThreadParam *) data;
  msleep(th->msec);
  th->sleep = FALSE;
  return 0;
}

void
nsleep(double a)
{
  ThreadParam th;
  GThread *thread;

  if (a < 0 || a > 100000) {
    return;
  }
  th.sleep = TRUE;
  th.msec = a * 1000;
  if (th.msec < 1) {
    return;
  }
  thread= g_thread_new("sleep", sleep_thread, &th);
#if USE_EVENT_LOOP
  while (th.sleep) {
    eventloop();
    msleep(1);
  }
#endif
  g_thread_join(thread);

  return;
}

void
set_environ(void)
{
  int i, n;
  char **list, **env;

  MainEnviron = NULL;

  env = g_listenv();
  if (env == NULL) {
    MainEnviron = NULL;
    return;
  }

  for (n = 0; env[n]; n++) ;

  list = g_malloc0(sizeof(*list) * (n + 1));
  if (list == NULL) {
    g_strfreev(env);
    return;
  }

  for (i = 0; i < n; i++) {
    char *name;
    const char *val;
    name = env[i];
    val = g_getenv(name);
    list[i] = g_strdup_printf("%s=%s", name, val);
    if (list[i] == NULL) {
      g_strfreev(env);
      g_strfreev(list);
      return;
    }
  }

  MainEnviron = list;

  g_strfreev(env);
}

void
set_security(int state)
{
  Security = state;
}

int
get_security(void)
{
  return Security;
}

static void
unlinkfile(char **file)
{
  if (*file!=NULL) {
    g_unlink(*file);
    g_free(*file);
    *file=NULL;
  }
}

#if ! WINDOWS
int
set_signal(int signal_id, int flags, void (*handler)(int), struct sigaction *oldact)
{
  static struct sigaction act;

  memset(&act, 0, sizeof(act));
  act.sa_handler = handler;
  act.sa_flags = (flags | SA_RESTART);
  sigemptyset(&act.sa_mask);

  return sigaction(signal_id, &act, oldact);
}

static void
childhandler(int sig)
{
  pid_t child_pid;

  do {
    child_pid = waitpid(-1, NULL, WNOHANG);
  } while (child_pid > 0);
}

void
set_childhandler(void)
{
  set_signal(SIGCHLD, SA_NOCLDSTOP, childhandler, NULL);
}

void
unset_childhandler(void)
{
  set_signal(SIGCHLD, 0, SIG_DFL, NULL);
}
#endif	/* WINDOWS */


#if USE_EVENT_LOOP
static int EvLoopActive = FALSE;
static GThread *EvLoopThread = NULL;

static void *
shellevloop(void *ptr)
{
  EvLoopActive = TRUE;
  while (EvLoopActive) {
    eventloop();
    msleep(10);
  }

  return NULL;
}

static void
set_shellevloop(int sig)
{
  if (! has_eventloop())
    return;

  if (EvLoopActive)
    return;

  EvLoopThread = g_thread_new("evloop", shellevloop, NULL);

  if (EvLoopThread == NULL)
    return;

  while (! EvLoopActive) {
    msleep(10);
  }
}

static void
reset_shellevloop(void)
{
  if (EvLoopActive) {
    EvLoopActive = FALSE;
    g_thread_join(EvLoopThread);
  }
}
#endif

static int
shgetstdin(void)
{
  char buf[2];
  int byte;

  if (nisatty(stdinfd())) {
#if USE_EVENT_LOOP
    set_shellevloop(0);
#endif
    do {
      byte=read(stdinfd(),buf,1);
    } while (byte<0);
#if USE_EVENT_LOOP
    reset_shellevloop();
#endif
  } else {
    do {
      byte=read(stdinfd(),buf,1);
      if (byte<=0) return EOF;
    } while (buf[0]=='\r');
  }
  if (byte<=0) return EOF;
  return buf[0];
}

#if HAVE_READLINE_READLINE_H
#if USE_EVENT_LOOP
static int ReadlineLock = FALSE;

static void *
readline_thread(void *prompt)
{
  char *str;

  str = readline(prompt);

  ReadlineLock = FALSE;

  return str;
}

static char *
nreadline(char *prompt)
{
  GThread *thread;

  if (ReadlineLock) {
    return NULL;
  }


  ReadlineLock = TRUE;
  thread= g_thread_new("readline", readline_thread, prompt);
  if (thread == NULL) {
    ReadlineLock = FALSE;
    return NULL;
  }

  while (ReadlineLock) {
    eventloop();
    msleep(10);
  }

  return (char *) g_thread_join(thread);
}
#else
static char *
nreadline(char *prompt)
{
  return readline(prompt);
}
#endif

static void
remove_duplicate_history(char *str)
{
  HIST_ENTRY *entry;
  int pos;

  for(pos = history_length; pos >= 0; pos--) {
    pos = history_search_pos(str, -1, pos);
    if(pos >= 0 && strcmp(history_get(pos + history_base)->line, str) == 0) {
      entry = remove_history(pos);
      if (entry) {
	free_history_entry(entry);
      }
      break;
    }
  }
}
#endif

static int
shget(struct nshell *nshell)
{
  char buf[2];
  int byte;
#ifdef HAVE_READLINE_READLINE_H
  static char *str_ptr = NULL, *line_str = NULL;
#endif

  if (nisatty(nshell->fd)) {
#ifdef HAVE_READLINE_READLINE_H
    if(str_ptr == NULL){
      str_ptr = line_str = nreadline(Prompt);
      if(str_ptr == NULL){
	byte = 0;
      } else if(strlen(str_ptr) > 0) {
	if (MultiLine && history_length > 0) {
	  HIST_ENTRY *entry;

	  entry = remove_history(history_length - 1);
	  remove_duplicate_history(str_ptr);
	  if (entry) {
            char *tmp;
	    tmp = g_strdup_printf("%s\n%s", entry->line, str_ptr);
	    free_history_entry(entry);
	    if (tmp) {
	      remove_duplicate_history(tmp);
	      add_history(tmp);
	      g_free(tmp);
	    }
	  }
	} else {
	  remove_duplicate_history(str_ptr);
	  add_history(str_ptr);
	}
      }
    }
    if(str_ptr != NULL){
      if(*line_str == '\0'){
	free(str_ptr);		/* str_ptr is allocated by readline library */
	str_ptr = line_str = NULL;
	buf[0] = '\0';
	byte = 1;
      }else{
	buf[0] = *line_str++;
	byte = 1;
      }
    }
#else  /* HAVE_READLINE_READLINE_H */
#if USE_EVENT_LOOP
    set_shellevloop(0);
#endif
    do {
      byte=read(nshell->fd,buf,1);
    } while (byte<0);
#if USE_EVENT_LOOP
    reset_shellevloop();
#endif
#endif	/* HAVE_READLINE_READLINE_H */
  } else {
    do {
      byte=read(nshell->fd,buf,1);
      if (byte<=0) return EOF;
    } while (buf[0]=='\r');
  }
  if (byte<=0) return EOF;
  return buf[0];
}

static int
puts_localized(int fd, const char *str)
{
  int len, r;
  char *localized;

  localized = g_locale_from_utf8(str, -1, NULL, NULL, NULL);
  if (localized == NULL) {
    return 0;
  }

  len = strlen(localized);
  r = write(fd, localized, len);
  g_free(localized);

  return r;
}

static int
shputstdout(const char *s)
{
  int len, r;

  len = puts_localized(stdoutfd(), s);

  r = write(stdoutfd(), "\n", 1);
  if (r >= 0)
    len += r;

  return len;
}

#if WINDOWS
static int
shputstderr(const char *s)
{
  int len, r;

  len=strlen(s);
  r = write(stderrfd(),s,len);
  if (r < 0){
    return r;
  }

  r = write(stderrfd(),"\n",1);
  if (r < 0){
    return r;
  }

  return len + 1;
}
#endif	/* WINDOWS */

static int
shprintfstdout(const char *fmt,...)
{
  int len;
  char *buf;
  va_list ap;

  va_start(ap,fmt);
  buf = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  if (buf == NULL) {
    return 0;
  }

  len = puts_localized(stdoutfd(), buf);
  g_free(buf);

  return len;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static int
shprintfstderr(const char *fmt,...)
{
  int len;
  char buf[1024];
  va_list ap;

  va_start(ap,fmt);
  len=vsprintf(buf,fmt,ap);
  va_end(ap);
  write(stderrfd(),buf,len);
  return len;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

struct cmdtabletype cmdtable[] = {
                  {"cd",       cmcd},
                  {"echo",     cmecho},
                  {"basename", cmbasename},
                  {"dirname",  cmdirname},
                  {"seq",      cmseq},
                  {"eval",     cmeval},
                  {"exit",     cmexit},
                  {"export",   cmexport},
                  {"pwd",      cmpwd},
                  {"read",     cmread},
                  {"set",      cmset},
                  {"shift",    cmshift},
                  {"type",     cmtype},
                  {"unset",    cmunset},
                  {":",        cmtrue},
                  {"object",   cmobject},
                  {"derive",   cmderive},
                  {"new",      cmnew},
                  {"exist",    cmexist},
                  {"del",      cmdel},
                  {"get",      cmget},
                  {"put",      cmput},
                  {"cpy",      cmcpy},
                  {"dup",      cmdup},
                  {"move",     cmmove},
                  {"movetop",  cmmovetop},
                  {"moveup",   cmmovetop},
                  {"movedown", cmmovetop},
                  {"movelast", cmmovetop},
                  {"exch",     cmmove},
                  {"exe",      cmexe},
                  {"dexpr",    cmdexpr},
                  {"iexpr",    cmdexpr},
                  {"true",     cmtrue},
                  {"false",    cmfalse},
                  {"[",        cmtest},
                  {"sleep",    cmsleep},
                  {"test",     cmtest},
                  {"which",    cmwhich},
                 };

int CMDNUM = sizeof(cmdtable) / sizeof(*cmdtable);

char *cpcmdtable[] = {
                  ";&|",
                  "if",
                  "then",
                  "else",
                  "elif",
                  "fi",
                  "case",
                  "esac",
                  "for",
                  "while",
                  "until",
                  "do",
                  "done",
                  "{",
                  "}",
};

int CPCMDNUM = sizeof(cpcmdtable) / sizeof(*cpcmdtable);

enum {CPNULL=1,CPIF,CPTHEN,CPELSE,CPELIF,CPFI,CPCASE,CPESAC,
      CPFOR,CPWHILE,CPUNTIL,CPDO,CPDONE,CPBI,CPBO,
      CPPATI,CPPATO,CPFN,CPNO};

enum {PPNULL=0,PPNO,PPSETV,PPSETO,PPSO1,PPSO2,PPSI1,PPSI2,PPPATOR,PPPIPE,
      PPAND,PPEND};

struct cmdstack {
  int cmdno;
  char *val;
  int ival,iftrue,casetrue;
  char *pat;
  struct prmlist *prm;
  struct cmdlist *cmd;
  void *next;
};

static NHASH CmdTblHash, CpCmdTblHash;

int
eval_script(const char *script, int security)
{
  struct nshell *nshell;
  int security_save;

  nshell = newshell();
  if (nshell == NULL)
    return 1;

  security_save = get_security();
  set_security(security);
  ngraphenvironment(nshell);
  cmdexecute(nshell, script);
  set_security(security_save);
  delshell(nshell);

  return 0;
}

int
init_cmd_tbl(void)
{
  int i, r;

  CmdTblHash = nhash_new();
  if (CmdTblHash == NULL)
    return 1;

  CpCmdTblHash = nhash_new();
  if (CpCmdTblHash == NULL)
    return 1;

  for (i = 0; i < CMDNUM; i++) {
    r = nhash_set_ptr(CmdTblHash, cmdtable[i].name, cmdtable[i].proc);
    if (r) {
      goto Err;
    }
  }

  for (i = 1; i < CPCMDNUM; i++) {
    r = nhash_set_int(CpCmdTblHash, cpcmdtable[i], i);
    if (r) {
      goto Err;
    }
  }

  return 0;

 Err:
  nhash_free(CmdTblHash);
  nhash_free(CpCmdTblHash);

  return 1;
}

shell_proc
check_cmd(char *name)
{
  shell_proc proc;
  int r;

  r = nhash_get_ptr(CmdTblHash, name, (void **) &proc);
  if (r)
    return NULL;

  return proc;
}

int
check_cpcmd(char *name)
{
  int r, i;

  r = nhash_get_int(CpCmdTblHash, name, &i);

  if (r)
    i = -1;

  return i;
}

static void
prmfree(struct prmlist *prmroot)
{
  struct prmlist *prmcur;

  if (prmroot==NULL) return;
  prmcur=prmroot;
  while (prmcur!=NULL) {
    struct prmlist *prmdel;
    prmdel=prmcur;
    prmcur=prmcur->next;
    g_free(prmdel->str);
    g_free(prmdel);
  }
}

static void
cmdfree(struct cmdlist *cmdroot)
{
  struct cmdlist *cmdcur;

  cmdcur=cmdroot;
  while (cmdcur!=NULL) {
    struct cmdlist *cmddel;
    prmfree(cmdcur->prm);
    cmddel=cmdcur;
    cmdcur=cmdcur->next;
    g_free(cmddel);
  }
}

static void
cmdstackfree(struct cmdstack *stroot)
{
  struct cmdstack *stcur;

  stcur=stroot;
  while (stcur!=NULL) {
    struct cmdstack *stdel;
    stdel=stcur;
    stcur=stcur->next;
    prmfree(stdel->prm);
    g_free(stdel->pat);		/* hito (mem leak of pat) */
    g_free(stdel);
  }
}

static void *
cmdstackcat(struct cmdstack **stroot,int cmdno)
{
  struct cmdstack *stcur,*stnew;

  if ((stnew=g_malloc(sizeof(struct cmdstack)))==NULL) return NULL;
  stcur=*stroot;
  if (stcur==NULL) *stroot=stnew;
  else {
    while (stcur->next!=NULL) stcur=stcur->next;
    stcur->next=stnew;
  }
  stnew->next=NULL;
  stnew->prm=NULL;
  stnew->cmd=NULL;
  stnew->pat=NULL;		/* hito (mem leak of pat) */
  stnew->cmdno=cmdno;
  return stnew;
}

static struct cmdstack *
cmdstackgetpo(struct cmdstack **stroot)
{
  struct cmdstack *stcur,*stprev;

  stcur=*stroot;
  stprev=NULL;
  while (stcur!=NULL) {
    stprev=stcur;
    stcur=stcur->next;
  }
  return stprev;
}

static int
cmdstackgetlast(struct cmdstack **stroot)
{
  struct cmdstack *stcur,*stprev;

  stcur=*stroot;
  stprev=NULL;
  while (stcur!=NULL) {
    stprev=stcur;
    stcur=stcur->next;
  }
  if (stprev==NULL) return 0;
  else return stprev->cmdno;
}

static void
cmdstackrmlast(struct cmdstack **stroot)
{
  struct cmdstack *stcur,*stprev;

  stcur=*stroot;
  stprev=NULL;
  if (stcur==NULL) return;
  while (stcur->next!=NULL) {
    stprev=stcur;
    stcur=stcur->next;
  }
  if (stprev==NULL) *stroot=NULL;
  else stprev->next=NULL;
  prmfree(stcur->prm);
  g_free(stcur->pat);		/* hito (mem leak of pat) */
  g_free(stcur);
}

static struct vallist *
create_vallist(char *name, char *val)
{
  struct vallist *valnew;

  valnew = g_malloc(sizeof(* valnew));
  if (valnew == NULL)
    return NULL;

  valnew->name = g_strdup(name);
  if (valnew->name == NULL) {
    g_free(valnew);
    return NULL;
  }

  if (val) {
    valnew->val = g_strdup(val);
    if (valnew->val == NULL) {
      g_free(valnew->name);
      g_free(valnew);
      return NULL;
    }
  } else {
    valnew->val = NULL;
  }

  return valnew;
}

static void
free_vallist(struct vallist *val)
{
  if (val == NULL)
    return;

  g_free(val->name);
  if (val->func)
    cmdfree(val->val);
  else
    g_free(val->val);
  g_free(val);
}

char *
addval(struct nshell *nshell,char *name,char *val)
/* addval() returns NULL on error */
{
#if USE_HASH
  struct vallist *valnew, *valcur;
  int hkey;

  valnew = create_vallist(name, val);
  if (valnew == NULL)
    return NULL;

  valnew->func = FALSE;
  valnew->arg = FALSE;

  hkey = nhash_hkey(name);
  nhash_get_ptr_with_hkey(nshell->valroot, name, hkey, (void **) &valcur);
  if (valcur) {
    free_vallist(valcur);
  }

  nhash_set_ptr_with_hkey(nshell->valroot, valnew->name, hkey, valnew);

  return valnew->name;
#else
  struct vallist *valcur,*valprev,*valnew;

  if ((valnew=g_malloc(sizeof(struct vallist)))==NULL) return NULL;
  if ((valnew->name=g_malloc(strlen(name)+1))==NULL) {
    g_free(valnew);
    return NULL;
  }
  if ((valnew->val=g_malloc(strlen(val)+1))==NULL) {
    g_free(valnew->name);
    g_free(valnew);
    return NULL;
  }
  valnew->func=FALSE;
  valnew->arg=FALSE;
  strcpy(valnew->name,name);
  strcpy(valnew->val,val);
  valcur=nshell->valroot;
  valprev=NULL;
  while (valcur!=NULL) {
    if (strcmp2(name,valcur->name)<=0) break;
    valprev=valcur;
    valcur=valcur->next;
  }
  if (valprev==NULL) nshell->valroot=valnew;
  else valprev->next=valnew;
  if (valcur==NULL) {
    valnew->next=NULL;
  } else if (strcmp0(name,valcur->name)==0) {
    valnew->next=valcur->next;
    g_free(valcur->name);
    if (valcur->func) cmdfree(valcur->val);
    else g_free(valcur->val);
    g_free(valcur);
  } else {
    valnew->next=valcur;
  }
  return valnew->name;
#endif
}

static char *
saveval(struct nshell *nshell,char *name,char *val, struct vallist **newvalroot)
/* saveval() returns NULL on error */
{
#if USE_HASH
  struct vallist *valnew, *valcur;
  int hkey;

  valnew = create_vallist(name, val);
  if (valnew == NULL)
    return NULL;

  valnew->func = FALSE;
  valnew->arg = TRUE;

  hkey = nhash_hkey(name);
  nhash_get_ptr_with_hkey(nshell->valroot, name, hkey, (void **) &valcur);

  if (valcur) {
    valcur->next = NULL;
    if (*newvalroot == NULL) {
      *newvalroot = valcur;
    } else {
      (*newvalroot)->next=valcur;
    }
  }

  nhash_set_ptr_with_hkey(nshell->valroot, name, hkey, valnew);

  return valnew->name;
#else
  struct vallist *valcur,*valprev,*valnew;

  if ((valnew=g_malloc(sizeof(struct vallist)))==NULL) return NULL;
  if ((valnew->name=g_malloc(strlen(name)+1))==NULL) {
    g_free(valnew);
    return NULL;
  }
  if ((valnew->val=g_malloc(strlen(val)+1))==NULL) {
    g_free(valnew->name);
    g_free(valnew);
    return NULL;
  }
  valnew->func=FALSE;
  valnew->arg=TRUE;
  strcpy(valnew->name,name);
  strcpy(valnew->val,val);
  valcur=nshell->valroot;
  valprev=NULL;
  while (valcur!=NULL) {
    if (strcmp2(name,valcur->name)<=0) break;
    valprev=valcur;
    valcur=valcur->next;
  }
  if (valprev==NULL) nshell->valroot=valnew;
  else valprev->next=valnew;
  if (valcur==NULL) {
    valnew->next=NULL;
  } else if (strcmp0(name,valcur->name)==0) {
    valnew->next=valcur->next;
    //    valcur->next = *newvalroot;    /* this code may be wrong. */
    valcur->next=NULL;
    if (*newvalroot==NULL) *newvalroot=valcur;
    else (*newvalroot)->next=valcur;
  } else {
    valnew->next=valcur;
  }
  return valnew->name;
#endif
}

static int
delete_save_val(struct nhash *hash, void *data)
{
  struct vallist *val;
  struct nshell *nshell;

  val = (struct vallist *) hash->val.p;
  nshell = (struct nshell *) data;

  if (val->arg) {
    nhash_del(nshell->valroot, val->name);
    free_vallist(val);
  }

  return 0;
}

static void
restoreval(struct nshell *nshell,struct vallist *newvalroot)
/* restoreval() returns NULL on error */
{
#if USE_HASH
  struct vallist *valcur,*valnext, *valcur2;

  nhash_each(nshell->valroot, delete_save_val, nshell);

  valcur = newvalroot;
  while (valcur) {
    int hk;
    valnext = valcur->next;
    hk = nhash_hkey(valcur->name);
    nhash_get_ptr_with_hkey(nshell->valroot, valcur->name, hk, (void **) &valcur2);

    if (valcur2) {
      free_vallist(valcur);
    } else {
      nhash_set_ptr_with_hkey(nshell->valroot, valcur->name, hk, (void *) valcur);
    }
    valcur = valnext;
  }
#else
  struct vallist *valcur,*valprev,*valnext;
  struct vallist *valcur2,*valprev2;

  valcur=nshell->valroot;
  valprev=NULL;
  while (valcur!=NULL) {
    valnext=valcur->next;
    if (valcur->arg==1) {
      if (valprev==NULL) nshell->valroot=valcur->next;
      else valprev->next=valcur->next;
      g_free(valcur->name);
      g_free(valcur->val);
      g_free(valcur);
    } else valprev=valcur;
    valcur=valnext;
  }
  valcur=newvalroot;
  while (valcur!=NULL) {
    valnext=valcur->next;
    valcur2=nshell->valroot;
    valprev2=NULL;
    while (valcur2!=NULL) {
      if (strcmp2(valcur->name,valcur2->name)<=0) break;
      valprev2=valcur2;
      valcur2=valcur2->next;
    }
    if (valcur2==NULL) {
      if (valprev2==NULL) nshell->valroot=valcur;
      else valprev2->next=valcur;
      valcur->next=NULL;
    } else if (strcmp0(valcur->name,valcur2->name)==0) {
      g_free(valcur->name);
      if (valcur->func) cmdfree(valcur->val);
      else g_free(valcur->val);
      g_free(valcur);
    } else {
      if (valprev2==NULL) nshell->valroot=valcur;
      else valprev2->next=valcur;
      valcur->next=valcur2;
    }
    valcur=valnext;
  }
#endif
}

char *
addexp(struct nshell *nshell,char *name)
/* addexp() returns NULL on error */
{
#if USE_HASH
  if (nhash_set_int(nshell->exproot, name, TRUE))
    return NULL;

  return name;
#else
  struct explist *valcur,*valprev,*valnew;
  int cmp;

  valcur=nshell->exproot;
  valprev=NULL;
  while (valcur!=NULL) {
    cmp=strcmp2(name,valcur->val);
    if (cmp==0) return valcur->val;
    else if (cmp<0) break;
    valprev=valcur;
    valcur=valcur->next;
  }
  if ((valnew=g_malloc(sizeof(struct vallist)))==NULL) return NULL;
  if ((valnew->val=g_malloc(strlen(name)+1))==NULL) {
    g_free(valnew);
    return NULL;
  }
  if (valprev==NULL) nshell->exproot=valnew;
  else valprev->next=valnew;
  strcpy(valnew->val,name);
  valnew->next=valcur;
  return valnew->val;
#endif
}

int
delval(struct nshell *nshell,char *name)
{
#if USE_HASH
  struct vallist *val;
  int r, hkey;

  hkey = nhash_hkey(name);
  r = nhash_get_ptr_with_hkey(nshell->valroot, name, hkey, (void **) &val);
  if (r == 0) {
    free_vallist(val);
    nhash_del_with_hkey(nshell->valroot, name, hkey);
    return TRUE;
  }

  return FALSE;
#else
  struct vallist *valcur,*valprev;

  valcur=nshell->valroot;
  valprev=NULL;
  while (valcur!=NULL) {
    if (strcmp0(valcur->name,name)==0) {
      if (valprev==NULL) nshell->valroot=valcur->next;
      else valprev->next=valcur->next;
      g_free(valcur->name);
      if (valcur->func) cmdfree(valcur->val);
      else g_free(valcur->val);
      g_free(valcur);
      return TRUE;
    }
    valprev=valcur;
    valcur=valcur->next;
  }
  return FALSE;
#endif
}

char *
getval(struct nshell *nshell,char *name)
{
#if USE_HASH
  struct vallist *val;

  nhash_get_ptr(nshell->valroot, name, (void **) &val);
  if (val && ! val->func) {
    return val->val;
  }
  return FALSE;
#else
  struct vallist *valcur;

  valcur=nshell->valroot;
  while (valcur!=NULL) {
    if ((strcmp0(valcur->name,name)==0) && (!valcur->func)) return valcur->val;
    valcur=valcur->next;
  }
  return NULL;
#endif
}

static int
getexp(struct nshell *nshell,char *name)
{
#if USE_HASH
  int i, r;

  r = nhash_get_int(nshell->exproot, name, &i);
  return ! r;
#else
  struct explist *valcur;

  valcur=nshell->exproot;
  while (valcur!=NULL) {
    if (strcmp0(valcur->val,name)==0) return TRUE;
    valcur=valcur->next;
  }
  return FALSE;
#endif
}

static char *
newfunc(struct nshell *nshell,char *name)
/* newfunc() returns NULL on error */
{
#if USE_HASH
  struct vallist *valnew, *valcur;
  int hkey;

  valnew = create_vallist(name, NULL);
  if (valnew == NULL)
    return NULL;

  valnew->func = TRUE;
  valnew->arg = FALSE;

  hkey = nhash_hkey(name);
  nhash_get_ptr_with_hkey(nshell->valroot, name, hkey, (void **) &valcur);
  if (valcur) {
    free_vallist(valcur);
  }

  nhash_set_ptr_with_hkey(nshell->valroot, valnew->name, hkey, valnew);

  return valnew->name;
#else
  struct vallist *valcur,*valprev,*valnew;

  if ((valnew=g_malloc(sizeof(struct vallist)))==NULL) return NULL;
  if ((valnew->name=g_malloc(strlen(name)+1))==NULL) {
    g_free(valnew);
    return NULL;
  }
  valnew->func=TRUE;
  valnew->arg=FALSE;
  strcpy(valnew->name,name);
  valnew->val=NULL;
  valcur=nshell->valroot;
  valprev=NULL;
  while (valcur!=NULL) {
    if (strcmp2(name,valcur->name)<=0) break;
    valprev=valcur;
    valcur=valcur->next;
  }
  if (valprev==NULL) nshell->valroot=valnew;
  else valprev->next=valnew;
  if (valcur==NULL) {
    valnew->next=NULL;
  } else if (strcmp0(name,valcur->name)==0) {
    valnew->next=valcur->next;
    g_free(valcur->name);
    if (valcur->func) cmdfree(valcur->val);
    else g_free(valcur->val);
    g_free(valcur);
  } else {
    valnew->next=valcur;
  }
  return valnew->name;
#endif
}

static char *
addfunc(struct nshell *nshell,char *name,struct cmdlist *val)
/* addfunc() returns NULL on error */
{
#if USE_HASH
  struct vallist *valcur;
  struct cmdlist *cmdcur,*cmdprev,*cmdnew;
  struct prmlist *prmcur,*prmprev,*prmnew;
  char *snew;

  nhash_get_ptr(nshell->valroot, name, (void **) &valcur);
  if (valcur == NULL) {
    return NULL;
  }

  if (! valcur->func) {
    return NULL;
  }

  cmdcur = valcur->val;
  cmdprev = NULL;
  while (cmdcur) {
    cmdprev = cmdcur;
    cmdcur = cmdcur->next;
  }

  cmdnew = g_malloc(sizeof(* cmdnew));
  if (cmdnew == NULL) {
    delval(nshell, name);
    return NULL;
  }

  if (cmdprev == NULL) {
    valcur->val = cmdnew;
  } else {
    cmdprev->next = cmdnew;
  }

  *cmdnew = *val;
  cmdnew->prm = NULL;
  cmdnew->next = NULL;
  prmcur = val->prm;
  prmprev = NULL;

  while (prmcur) {
    prmnew = g_malloc(sizeof(* prmnew));
    if (prmnew == NULL) {
      delval(nshell, name);
      return NULL;
    }
    if (prmprev == NULL) {
      cmdnew->prm=prmnew;
    } else {
      prmprev->next=prmnew;
    }
    *prmnew = *prmcur;
    prmnew->next = NULL;
    if (prmcur->str) {
      snew = g_strdup(prmcur->str);
      if (snew == NULL) {
	delval(nshell, name);
	return NULL;
      }
      prmnew->str = snew;
    }
    prmprev = prmnew;
    prmcur = prmcur->next;
  }

  return valcur->name;
#else
  struct vallist *valcur;
  struct cmdlist *cmdcur,*cmdprev,*cmdnew;
  struct prmlist *prmcur,*prmprev,*prmnew;
  char *snew;

  valcur=nshell->valroot;
  while (valcur!=NULL) {
    if ((strcmp0(valcur->name,name)==0) && (valcur->func)) {
      cmdcur=valcur->val;
      cmdprev=NULL;
      while (cmdcur!=NULL) {
        cmdprev=cmdcur;
        cmdcur=cmdcur->next;
      }
      if ((cmdnew=g_malloc(sizeof(struct cmdlist)))==NULL) {
        delval(nshell,name);
        return NULL;
      }
      if (cmdprev==NULL) valcur->val=cmdnew;
      else cmdprev->next=cmdnew;
      *cmdnew=*val;
      cmdnew->prm=NULL;
      cmdnew->next=NULL;
      prmcur=val->prm;
      prmprev=NULL;
      while (prmcur!=NULL) {
        if ((prmnew=g_malloc(sizeof(struct prmlist)))==NULL) {
          delval(nshell,name);
          return NULL;
        }
        if (prmprev==NULL) cmdnew->prm=prmnew;
        else prmprev->next=prmnew;
        *prmnew=*prmcur;
        prmnew->next=NULL;
        if (prmcur->str!=NULL) {
          if ((snew=g_malloc(strlen(prmcur->str)+1))==NULL) {
            delval(nshell,name);
            return NULL;
          }
          strcpy(snew,prmcur->str);
          prmnew->str=snew;
        }
        prmprev=prmnew;
        prmcur=prmcur->next;
      }
      break;
    }
    valcur=valcur->next;
  }
  return valcur->name;
#endif
}

struct cmdlist *
getfunc(struct nshell *nshell,char *name)
{
#if USE_HASH
  struct vallist *val;

  nhash_get_ptr(nshell->valroot, name, (void **) &val);
  if (val && val->func) {
    return val->val;
  }
  return FALSE;
#else
  struct vallist *valcur;

  valcur=nshell->valroot;
  while (valcur!=NULL) {
    if ((strcmp0(valcur->name,name)==0) && (valcur->func)) return valcur->val;
    valcur=valcur->next;
  }
  return NULL;
#endif
}

static int
is_dq_special(int ch)
{
  return (strchr("\"\\`$", ch) != NULL);
}

static int
check_quote(int quote, int ch)
{
  return (!quote || ((quote == '"') && is_dq_special(ch)));
}

static int
check_quote_bquote(int quote, int bquote, int ch)
{
  return (check_quote(quote, ch) || bquote);
}

static char *
gettok(char **s,int *len,int *quote,int *bquote,int *cend,int *escape)
{
  int i;
  char *po,*spo;

  *len = 0;
  if (*s==NULL) return NULL;
  *cend='\0';
  *escape=FALSE;
  po=*s;

  /* remove branck */
  for (i=0;(po[i]!='\0') && !*quote && (strchr(" \t",po[i])!=NULL);i++);
  /* token ends */
  if ((po[i]=='\0') || (!*quote && (po[i]=='#'))) {
    *len=0;
    return NULL;
  }
  spo=po+i;
  if (!*quote && !*bquote) {
    /* check speecial character */
    if (strchr(";&|^<>()",po[i])!=NULL) {
      if (strchr(";&)",po[i])!=NULL) *cend=po[i];
      if ((strchr("<>",po[i])!=NULL) && (po[i]==po[i+1])) {
	i++;
	if (po[i] == '<' && po[i + 1] == '-') {
	  i++;
	}
      }
      if ((po[i]==';') && (po[i]==po[i+1])) i++;
      *s+=(i+1);
      *len=*s-spo;
      return spo;
    }
  }

  for (;(po[i]!='\0') &&
       (*quote || *bquote || *escape || (strchr(";&|^<> \t()",po[i])==NULL));
        i++) {
    /* check escapse */
    if (*escape) *escape=FALSE;
    else if (po[i]=='\\') {
      if (check_quote_bquote(*quote, *bquote, po[i + 1])) *escape=TRUE;
    /* check back quote */
    } else if (*bquote) {
      if (po[i]=='`') *bquote='\0';
    } else if ((po[i]=='`') && (*quote!='\'')) *bquote='`';
    /* check quotation */
    else if ((*quote=='"') || (*quote=='\'')) {
      if (po[i]==*quote) *quote='\0';
    } else if (po[i]=='\'' || po[i]=='"') *quote=po[i];
  }

  if (*escape) i--;
  *s+=i;
  *len=*s-spo;
  return spo;
}

static int
getcmdline(struct nshell *nshell,
	   struct cmdlist **rcmdroot,struct cmdlist *cmd,
	   const char *str,int *istr)
/* getcmdline() returns
     -2: unexpected eof detected
     -1: fatal error
      0: noerror
      1: eof detected */
{
  struct cmdlist *cmdroot,*cmdcur;
  struct prmlist *prmcur,*prmnew;
  int quote,quote2,bquote,bquote2,escape,escape2;
  int cend,l;
  char *spo,*po,*prompt,*tok,*tok2;
  char *ignoreeof,*endptr;
  int ch;
  int i,eofnum;
  int err,eofcount;

  err=-1;
  escape=escape2=FALSE;
  quote=quote2=bquote=bquote2='\0';
  cend='\0';
  cmdroot=NULL;
  cmdcur=NULL;
  prmcur=NULL;
  tok=NULL;
  eofcount=0;
  do {
    g_free(tok);
    tok=NULL;
    if (str==NULL) {
      if ((tok=nstrnew())==NULL) goto errexit;
      if (nisatty(nshell->fd)) {
        if (cmd == NULL && cmdroot == NULL) {
	  prompt = getval(nshell, "PS1");
#ifdef HAVE_READLINE_READLINE_H
	  MultiLine = FALSE;
#endif
	} else {
	  prompt = getval(nshell, "PS2");
#ifdef HAVE_READLINE_READLINE_H
	  MultiLine = TRUE;
#endif
	}
#ifdef HAVE_READLINE_READLINE_H
	Prompt = prompt;
#else
        if (prompt!=NULL) printfconsole("%.256s",prompt);
#endif
      }
      do {
        ch=shget(nshell);
        if (ch==EOF) {
          if (strlen(tok)!=0) {
            if (!nisatty(nshell->fd)) break;
            else printfconsole("%c",(char )0x07);
          } else {
            if (!nisatty(nshell->fd)) {
              if ((cmd!=NULL) || (cmdroot!=NULL)) {
                sherror(ERRUEXPEOF);
                err=-2;
              } else err=1;
              goto errexit;
            } else {
              if ((cmd!=NULL) || (cmdroot!=NULL)) {
                sherror(ERRUEXPEOF);
                err=-2;
                goto errexit;
              }
              eofcount++;
	      ignoreeof = getval(nshell,"IGNOREEOF");
              if (ignoreeof == NULL) {
		eofnum=0;
	      } else {
                if (ignoreeof[0] == '\0') {
		  eofnum = 10;
                } else {
                  eofnum = strtol(ignoreeof,&endptr,10);
                  if (endptr[0] != '\0')
		    eofnum = 10;
                }
              }
              if (eofcount > eofnum) {
                err=1;
                putconsole("exit");
                nshell->quit=TRUE;
                goto errexit;
              } else {
                sherror(ERREOF);
                ch='\0';
              }
            }
          }
        } else {
          eofcount=0;
          if (ch=='\n') ch='\0';
          if ((tok=nstrccat(tok,ch))==NULL) goto errexit;
        }
      } while (ch!='\0');
    } else {
      if (str[*istr]=='\0') {
        if ((cmd!=NULL) || (cmdroot!=NULL)) {
          sherror(ERRUEXPEOF);
          err=-2;
        } else {
	  err=1;
	}
        goto errexit;
      }
      for (i=*istr;(str[i]!='\0') && (str[i]!='\n');i++);
      if ((tok=g_malloc(i-*istr+1))==NULL) {
	goto errexit;
      }
      strncpy(tok,str+*istr,i-*istr);
      tok[i-*istr]='\0';
      if (str[i]=='\n') {
	*istr=i+1;
      } else {
	*istr=i;
      }
    }
    if (! g_utf8_validate(tok, -1, NULL)) {
      char *tmp;

      tmp = g_locale_to_utf8(tok, -1, NULL, NULL, NULL);
      if (tmp) {
	g_free(tok);
	tok = tmp;
      }
    }
    tok2=tok;
    do {
      int len;

      spo=gettok(&tok2,&len,&quote,&bquote,&cend,&escape);
      if (quote2 || bquote2 || escape2) {
        l=strlen(prmcur->str);
        if (quote2 || bquote2) l++;
        if ((po=g_malloc(len+l+1))==NULL) goto errexit;
        strcpy(po,prmcur->str);
        if (quote2 || bquote2) *(po+l-1)='\n';
        if (spo!=NULL) strncpy(po+l,spo,len);
        *(po+len+l)='\0';
        g_free(prmcur->str);
        prmcur->str=po;
      } else if (spo!=NULL) {
        if (cmdroot==NULL) {
          if ((cmdcur=g_malloc(sizeof(struct cmdlist)))==NULL) goto errexit;
          cmdroot=cmdcur;
          cmdcur->next=NULL;
          cmdcur->prm=NULL;
        }
        if ((prmnew=g_malloc(sizeof(struct prmlist)))==NULL) goto errexit;
        if (cmdcur->prm==NULL) cmdcur->prm=prmnew;
        else prmcur->next=prmnew;
        prmcur=prmnew;
        prmcur->next=NULL;
        if ((prmcur->str=g_malloc(len+1))==NULL) goto errexit;
        strncpy(prmcur->str,spo,len);
        prmcur->str[len]='\0';
      }
      if (cend) {
        if ((cmdcur->next=g_malloc(sizeof(struct cmdlist)))==NULL)
          goto errexit;
        cmdcur=cmdcur->next;
        cmdcur->next=NULL;
        cmdcur->prm=NULL;
      }
      quote2=quote;
      bquote2=bquote;
      escape2=escape;
    } while ((spo!=NULL) && (!escape) && (!quote) && (!bquote));
  } while (quote || bquote || escape || (cmdroot==NULL));
  *rcmdroot=cmdroot;
  g_free(tok);
  return 0;

errexit:
  cmdfree(cmdroot);
  g_free(tok);
  *rcmdroot=NULL;
  return err;
}

static char *
quotation(struct nshell *nshell,char *s,int quote)
{
  int i,j,num;
  char *snew,*ifs;

  ifs=getval(nshell,"IFS");
  num=0;
  // NOTE: Keep "\"\\'$" here. Do not add backtick ('`') to avoid unintended
  // side effects, as the current behavior works correctly for quotation prep
  for (i=0;s[i]!='\0';i++) if (strchr("\"\\'$",s[i])!=NULL) num++;
  if ((snew=g_malloc(strlen(s)+num+1))==NULL) return NULL;
  j=0;
  for (i=0;s[i]!='\0';i++) {
    if ((quote!='"') && (ifs!=NULL) && (ifs[0]!='\0')
    && (strchr(ifs,s[i])!=NULL)) snew[j++]=(char )0x01;
    else {
      if (strchr("\"\\'$",s[i])!=NULL) snew[j++]=(char )0x02;
      snew[j++]=s[i];
    }
  }
  snew[j]='\0';
  return snew;
}

static char *
unquotation(char *s,int *quoted)
{
  int escape,quote,i,j;
  char *snew,*po;

  if ((snew=g_malloc(strlen(s)+1))==NULL) return NULL;
  *quoted=FALSE;
  po=s;
  escape=FALSE;
  quote='\0';

  j=0;
  for (i=0;po[i]!='\0';i++) {
    if (escape) {
      escape=FALSE;
      snew[j++]=po[i];
    } else if (po[i]=='\\') {
      if (!quote
      || ((quote=='"') && (strchr("\"\\'$",po[i+1])!=NULL))) escape=TRUE;
      else snew[j++]=po[i];
    } else if (po[i]==(char )0x02) escape=TRUE;
    else if (po[i]==(char )0x01) snew[j++]=' ';
    else if ((quote=='"') || (quote=='\'')) {
      if (po[i]==quote) quote='\0';
      else snew[j++]=po[i];
    } else if (po[i]=='\'' || po[i]=='"') {
      quote=po[i];
      *quoted=TRUE;
    } else snew[j++]=po[i];
  }

  snew[j]='\0';
  return snew;
}

static char *
fnexpand(struct nshell *nshell,char *str)
{
  int escape,str_expand,str_noexpand,quote,i,j,len,num;
  char *po;
  char *s,*s2;
  char **namelist;

  po=str;
  escape=str_expand=str_noexpand=FALSE;
  quote='\0';
  if ((s=nstrnew())==NULL) return NULL;
  if ((s2=nstrnew())==NULL) return NULL;
  len=strlen(po);
  for (i=0;i<=len;i++) {
    if (escape && (po[i]!='\0')) {
      escape=FALSE;
      if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
      if (strchr("*?[",po[i])!=NULL) str_noexpand=TRUE;
    } else if (po[i]=='\\') {
      if (check_quote(quote, po[i+1])) escape=TRUE;
      else if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
    } else if (po[i]==(char )0x02) escape=TRUE;
    else if ((po[i]==(char )0x01) || (po[i]=='\0')) {
      if (strlen(s)!=0) {
        if (!nshell->optionf && str_expand && !str_noexpand) {
          if ((num=nglob(s,&namelist))==-1) goto errexit;
          for (j=0;j<num;j++) {
            if (((s2=nstrcat(s2,(char *)(namelist[j])))==NULL)
            || ((s2=nstrccat(s2,(char )0x01))==NULL)) {
              arg_del(namelist);
              goto errexit;
            }
          }
          arg_del(namelist);
        } else {
          if ((s2=nstrcat(s2,s))==NULL) goto errexit;
          if ((s2=nstrccat(s2,(char )0x01))==NULL) goto errexit;
        }
      }
      str_expand=FALSE;
      if (po[i]==(char )0x01) for (;po[i+1]==(char )0x01;i++);
      g_free(s);
      if ((s=nstrnew())==NULL) goto errexit;
    } else if ((quote=='"') || (quote=='\'')) {
      if (po[i]==quote) quote='\0';
      else if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
      if (strchr("*?[",po[i])!=NULL) str_noexpand=TRUE;
    } else if (po[i]=='\'' || po[i]=='"') quote=po[i];
    else {
      if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
      if (strchr("*?[",po[i])!=NULL) str_expand=TRUE;
    }
  }

  g_free(s);
  g_free(po);
  len=strlen(s2);
  if ((len!=0) && (s2[len-1]==(char )0x01)) s2[len-1]='\0';
  return s2;

errexit:
  g_free(s);
  g_free(po);
  g_free(s2);
  return NULL;
}

static int
wordsplit(struct prmlist *prmcur)
{
  int i,num;
  char *po;
  struct prmlist *prmnew;
  char *s;

  num=0;
  po=prmcur->str;
  if (po==NULL) return 1;
  if ((s=nstrnew())==NULL) return -1;
  for (i=0;po[i]!='\0';i++) {
    if (po[i]==(char )0x02) {
      i++;
      if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
    } else if (po[i]==(char )0x01) {
      prmcur->str=s;
      if ((s=nstrnew())==NULL) goto errexit;
      if ((prmnew=g_malloc(sizeof(struct prmlist)))==NULL) goto errexit;
      prmnew->str=NULL;
      prmnew->next=prmcur->next;
      prmnew->prmno=prmcur->prmno;
      prmcur->next=prmnew;
      prmcur=prmnew;
      num++;
    } else if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
  }
  prmcur->str=s;
  num++;
  g_free(po);
  return num;

errexit:
  g_free(s);
  g_free(po);
  return -1;
}

static char *
wordunsplit(char *str)
{
  int i,j;
  char *po;

  po=str;
  j=0;
  for (i=0;po[i]!='\0';i++)
    if (po[i]==(char )0x01) po[j++]=' ';
    else if (po[i]!=(char )0x02) po[j++]=po[i];
  po[j]='\0';
  return po;
}

struct expand_context {
  int *quote, quote2;
  int *bquote, bquote2;
  int escape;
  char *cmd_buf;			/* command substitution */
  char *param_buf;			/* parameter expansion */
  char *name, *s, *val, valf, valf2;
};

static int
expand_bquote (struct nshell *nshell, struct expand_context *cx, int ch)
{
  if (*cx->bquote) {
    if (ch=='`') {
      *cx->bquote='\0';
      if (cx->param_buf==NULL) {
	/* command substitution */
	int sout, sout2, rcode, byte, n, j;
	char *tmpfil, *c1;
	// if ((sys=getobject("system"))==NULL) return -1; /* fix-me: assigned value of the variable "sys" is never used */
	sout = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &tmpfil);
	if (sout < 0)
	  return -1;
	sout2=nredirect(1,sout);
	rcode=cmdexecute(nshell,cx->cmd_buf);
	if ((rcode!=0) && (rcode!=1)) {
	  nredirect2(1,sout2);
	  g_unlink(tmpfil);
	  g_free(tmpfil);
	  return -1;
	}
	g_free(cx->cmd_buf);
	if ((cx->cmd_buf=nstrnew())==NULL) {
	  nredirect2(1,sout2);
	  g_unlink(tmpfil);
	  g_free(tmpfil);
	  return -1;
	}
	nlseek(stdoutfd(),0L,SEEK_SET);
	while ((byte=nread(stdoutfd(),writebuf,WRITEBUFSIZE-1))>0) {
	  writebuf[byte]='\0';
	  if ((cx->cmd_buf=nstrcat(cx->cmd_buf,writebuf))==NULL) {
	    nredirect2(1,sout2);
	    g_unlink(tmpfil);
	    g_free(tmpfil);
	    return -1;
	  }
	}
	nredirect2(1,sout2);
	g_unlink(tmpfil);
	if (byte==-1) {
	  sherror2(ERRREAD,tmpfil);
	  g_free(tmpfil);
	  return -1;
	}
	g_free(tmpfil);
	n = strlen(cx->cmd_buf);
	for (int u = 0; u < n; u++) {
	  if (cx->cmd_buf[u] == '\n') {
	    cx->cmd_buf[u] = ' ';
	  }
	}
	for (j=strlen(cx->cmd_buf)-1;(j>=0) && (cx->cmd_buf[j]==' ');j--);
	cx->cmd_buf[j+1]='\0';
	if ((c1=quotation(nshell,cx->cmd_buf,*cx->quote))==NULL) return -1;
	cx->s=nstrcat(cx->s,c1);
	g_free(c1);
	if (cx->s==NULL) return -1;
	g_free(cx->cmd_buf);
	cx->cmd_buf=NULL;
      }
    } else if (cx->param_buf==NULL) {
      if (cx->cmd_buf!=NULL) {
	if ((cx->cmd_buf=nstrccat(cx->cmd_buf,ch))==NULL) return -1;
      } else {
	if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
      }
    }
    return 1;
  } else if ((ch=='`') && (*cx->quote!='\'')) {
    *cx->bquote='`';
    if (cx->param_buf==NULL) {
      if ((cx->cmd_buf=nstrnew())==NULL) return -1;
    }
    return 1;
  }
  return 0;
}

static int
expand_escape(struct expand_context *cx, int ch, int chn)
{
  if (cx->escape) {
    if (cx->param_buf==NULL) {
      if (cx->cmd_buf!=NULL) {
	if ((cx->cmd_buf=nstrccat(cx->cmd_buf,ch))==NULL) return -1;
      } else {
	if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
      }
    }
    cx->escape=FALSE;
    return 1;
  }

  if (ch=='\\') {
    if (check_quote_bquote(*cx->quote, *cx->bquote, chn)) {
      cx->escape=TRUE;
      if ((cx->param_buf==NULL) && (cx->cmd_buf==NULL)) {
	if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
      }
    } else if (cx->param_buf==NULL) {
      if (cx->cmd_buf!=NULL) {
	if ((cx->cmd_buf=nstrccat(cx->cmd_buf,ch))==NULL) return -1;
      } else {
	if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
      }
    }
    return 1;
  }

  if (ch==(char )0x02) {
    cx->escape=TRUE;
    if ((cx->param_buf==NULL) && (cx->cmd_buf==NULL)) {
      if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
    }
    return 1;
  }

  return 0; // エスケープ関連ではない文字
}

static int
expand_object_replacement (struct nshell *nshell, struct expand_context *cx)
{
  char *tmp;
  struct narray *sarray;
  char **sdata;
  int snum, dummy;
  if ((tmp=unquotation(cx->param_buf,&dummy))==NULL) return -1;
  g_free(cx->param_buf);
  cx->param_buf=NULL;
  sarray=sgetobj(tmp,FALSE,FALSE,FALSE);
  g_free(tmp);
  sdata=arraydata(sarray);
  snum=arraynum(sarray);
  for (int j=0;j<snum;j++) {
    char *c1;
    if ((c1=quotation(nshell,sdata[j],*cx->quote))==NULL) {
      arrayfree2(sarray);
      return -1;
    }
    cx->s=nstrcat(cx->s,c1);
    g_free(c1);
    if (cx->s==NULL) {
      arrayfree2(sarray);
      return -1;
    }
  }
  arrayfree2(sarray);
  return 1;
}

static int
expand_modifier_default(struct nshell *nshell, struct expand_context *cx, char **str)
{
  int dummy;
  char *c1 = NULL;
  if ((cx->val!=NULL) && ((cx->valf2!=':') || (cx->val[0]!='\0'))) {
    switch (cx->valf) {
    case '-':
    case '=':
    case '?':
      c1 = quotation(nshell,cx->val,cx->quote2);
      break;
    case '+':
      c1 = expand(nshell,cx->param_buf,&cx->quote2,&cx->bquote2,TRUE);
      break;
    }
    *str = c1;
    return c1 ? 0 : -1;
  }

  *str = NULL;
  if (cx->valf == '+') {
    return 0;
  }

  c1 = expand(nshell,cx->param_buf,&cx->quote2,&cx->bquote2,TRUE);
  if (c1 == NULL) {
    return -1;
  }

  switch (cx->valf) {
  case '=':
    g_free(cx->param_buf);
    if ((cx->param_buf=unquotation(c1,&dummy))==NULL) {
      g_free (c1);
      return -1;
    }
    if (addval(nshell,cx->name,cx->param_buf)==NULL) {
      g_free (c1);
      return -1;
    }
    break;
  case '?':
    printfstdout("%.256s: %.256s\n",cx->name,c1);
    g_free (c1);
    return -1;
    break;
  }
  *str = c1;
  return 0;
}

static char *
expand_modifier_prefix (struct nshell *nshell, struct expand_context *cx)
{
  char *c1 = NULL;
  char *c2;
  int k;
  char ch;
  if ((c2=nstrnew())==NULL) return NULL;
  if ((c2=nstrcat(c2,cx->val))==NULL) return NULL;
  if ((c1=expand(nshell,cx->param_buf,&cx->quote2,&cx->bquote2,TRUE))==NULL) {
    g_free(c2);
    return NULL;
  }
  if (cx->valf2=='#') {
    for (k=strlen(c2);k>=0;k--) {
      ch=c2[k];
      c2[k]='\0';
      if (wildmatch(c1,c2,WILD_PATHNAME)) {
	c2[k]=ch;
       break;
      }
      c2[k]=ch;
    }
  } else {
    for (k = 0; k <= (int) strlen(c2); k++) {
      ch=c2[k];
      c2[k]='\0';
      if (wildmatch(c1,c2,WILD_PATHNAME)) {
       c2[k]=ch;
       break;
      }
      c2[k]=ch;
    }
  }
  if (k > (int) strlen(c2)) {
    k=0;
  } else if (k < 0) {
    k = 0;            /* fix-me: is it proper? */
  }
  g_free(c1);
  c1=quotation(nshell,c2+k,cx->quote2);
  g_free(c2);
  return c1;
}

static char *
expand_modifier_suffix (struct nshell *nshell, struct expand_context *cx)
{
  char *c1 = NULL;
  char *c2;
  int k;
  if ((c2=nstrnew())==NULL) return NULL;
  if ((c2=nstrcat(c2,cx->val))==NULL) return NULL;
  if ((c1=expand(nshell,cx->param_buf,&cx->quote2,&cx->bquote2,TRUE))==NULL) {
    g_free(c2);
    return NULL;
  }
  if (cx->valf2=='%') {
    int len;
    len = strlen(c2) - 1;
    for (k = 0; k <= len; k++) {
      if (wildmatch(c1,c2+k,WILD_PATHNAME)) break;
    }
  } else {
    for (k=strlen(c2);k>=0;k--) {
      if (wildmatch(c1,c2+k,WILD_PATHNAME)) break;
    }
  }
  if (k<0) k=strlen(c2);
  c2[k]='\0';
  g_free(c1);
  c1=quotation(nshell,c2,cx->quote2);
  g_free(c2);
  return c1;
}

static int
expand_parameter_modifier(struct nshell *nshell, struct expand_context *cx)
{
  char *c1 = NULL;
  switch (cx->valf) {
  case 'o':
    return expand_object_replacement (nshell, cx);
    break;
  case '-':
  case '=':
  case '?':
  case '+':
    if (expand_modifier_default(nshell, cx, &c1) < 0) {
      return -1;
    }
    break;
  case '#':
  case '%':
    if (cx->val == NULL || cx->val[0] == '\0') {
      break;
    }
    if (cx->valf == '#') {
      c1 = expand_modifier_prefix (nshell, cx);
    } else {
      c1 = expand_modifier_suffix (nshell, cx);
    }
    if (c1 == NULL) {
      return -1;
    }
    break;
  }
  if (c1!=NULL) {
    cx->s=nstrcat(cx->s,c1);
    g_free(c1);
    if (cx->s==NULL) return -1;
  }
  *cx->quote=cx->quote2;
  *cx->bquote=cx->bquote2;
  g_free(cx->param_buf);
  cx->param_buf=NULL;
  return 1;
}

static int
expand_positional(struct nshell *nshell, struct expand_context *cx, char *po, int *index)
{
  int num, j;
  const int i = *index;
  for (j=i+1;(po[j]!='\0') && isdigit(po[j]);j++);
  g_free(cx->name);
  if ((cx->name=g_malloc(j-i))==NULL) return -1;
  strncpy(cx->name,po+i+1,j-i-1);
  cx->name[j-i-1]='\0';
  num=atoi(cx->name);
  if (num<nshell->argc) {
    char *c1;
    if ((c1=quotation(nshell,(nshell->argv)[num],*cx->quote))==NULL)
      return -1;
    cx->s=nstrcat(cx->s,c1);
    g_free(c1);
    if (cx->s==NULL) return -1;
  }
  *index=j-1;
  return 1;
}

static int
expand_special(struct nshell *nshell, struct expand_context *cx, const char *po, int *index)
{
  char *c1;
  const int i = *index;
  switch (po[i+1]) {
  case '#': g_free(cx->name);
    if ((cx->name=g_malloc(12))==NULL) return -1;
    sprintf(cx->name,"%d",nshell->argc-1);
    break;
  case '?': g_free(cx->name);
    if ((cx->name=g_malloc(12))==NULL) return -1;
    sprintf(cx->name,"%d",nshell->status);
    break;
  }
  if ((c1=quotation(nshell,cx->name,*cx->quote))==NULL) return -1;
  cx->s=nstrcat(cx->s,c1);
  g_free(c1);
  if (cx->s==NULL) return -1;
  *index = i + 1;
  return 1;
}

static int
expand_all_args(struct nshell *nshell, struct expand_context *cx, const char *po, int *index)
{
  char ch;
  int j;
  const int i = *index;
  cx->val=getval(nshell,"IFS");
  if (cx->val==NULL) ch='\0';
  else ch=cx->val[0];
  for (j=1;j<nshell->argc;j++) {
    char *c1;
    if ((c1=quotation(nshell,(nshell->argv)[j],*cx->quote))==NULL)
      return -1;
    cx->s=nstrcat(cx->s,c1);
    g_free(c1);
    if (cx->s==NULL) return -1;
    if (j!=(nshell->argc-1)) {
      if ((*cx->quote=='"') && (po[i+1]=='*')) {
	if (ch!='\0') {
	  if (is_dq_special(ch)) {
	    if ((cx->s=nstrccat(cx->s,'\\'))==NULL) return -1;
	  }
	  if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
	}
      } else {
	if ((cx->s=nstrccat(cx->s,(char )0x01))==NULL) return -1;
      }
    }
  }
  *index = i + 1;
  return 1;
}

static int
expand_braced_simple(struct nshell *nshell, struct expand_context *cx, const char *po, int j, int *index)
{
  const int i = *index;
  g_free(cx->name);
  if ((cx->name=g_malloc(j-i-1))==NULL) return -1;
  strncpy(cx->name,po+i+2,j-i-2);
  cx->name[j-i-2]='\0';
  cx->val=getval(nshell,cx->name);
  if (cx->val!=NULL) {
    char *c1;
    if ((c1=quotation(nshell,cx->val,*cx->quote))==NULL) return -1;
    cx->s=nstrcat(cx->s,c1);
    g_free(c1);
    if (cx->s==NULL) return -1;
  }
  *index=j;
  return 1;
}

static int
expand_braced_object(struct expand_context *cx, int *index)
{
  cx->valf='o';
  if ((cx->param_buf=nstrnew())==NULL) return -1;
  *index += 2;
  return 1;
}

static int
expand_braced_modifier(struct nshell *nshell, struct expand_context *cx, const char *po, int j, int *index)
{
  const int i = *index;
  g_free(cx->name);
  if ((cx->name=g_malloc(j-i-1))==NULL) return -1;
  strncpy(cx->name,po+i+2,j-i-2);
  cx->name[j-i-2]='\0';
  cx->val=getval(nshell,cx->name);
  cx->quote2=*cx->quote;
  cx->bquote2=*cx->bquote;
  cx->valf2=' ';
  if (po[j]==':') {
    cx->valf2=':';
    j++;
  }
  if (strchr("-=?+",po[j])!=NULL) {
    cx->valf=po[j];
    j++;
  } else if (strchr("#%",po[j])!=NULL) {
    if (po[j]==po[j+1]) {
      cx->valf2=po[j];
      j++;
    }
    cx->valf=po[j];
    j++;
  } else {
    sherror(ERRBADSUB);
    return -1;
  }
  if ((cx->param_buf=nstrnew())==NULL) return -1;
  *index=j;
  return 1;
}

static int
find_varname_end(const char *po, int start)
{
  int j;
  for (j = start; (po[j]!='\0') && (isalnum(po[j]) || (po[j]=='_')); j++);
  return j;
}

static int
expand_braced(struct nshell *nshell, struct expand_context *cx, const char *po, int *index)
{
  int j;
  const int i = *index;
  j = find_varname_end(po, i + 2);
  if ((po[j]=='\0') || (strchr(":-=?+#%}",po[j])==NULL)
      || (po[j-1]=='{')) {
    sherror(ERRBADSUB);
    return -1;
  }
  if (po[j]=='}') {
    return expand_braced_simple(nshell, cx, po, j, index);
  }
  if ((po[j]==':') && (po[j+1]!='\0') && (strchr("-=?+",po[j+1])==NULL)) {
    return expand_braced_object(cx, index);
  }
  return expand_braced_modifier(nshell, cx, po, j, index);
}

static int
expand_simple(struct nshell *nshell, struct expand_context *cx, const char *po, int *index)
{
  int j;
  const int i = *index;
  j = find_varname_end(po, i + 1);
  g_free(cx->name);
  if ((cx->name=g_malloc(j-i))==NULL) return -1;
  strncpy(cx->name,po+i+1,j-i-1);
  cx->name[j-i-1]='\0';
  if (cx->name[0]=='\0') cx->val="$";
  else cx->val=getval(nshell,cx->name);
  if (cx->val!=NULL) {
    char *c1;
    if ((c1=quotation(nshell,cx->val,*cx->quote))==NULL) return -1;
    cx->s=nstrcat(cx->s,c1);
    g_free(c1);
    if (cx->s==NULL) return -1;
  }
  *index=j-1;
  return 1;
}

static int
expand_variable(struct nshell *nshell, struct expand_context *cx, char *po, int *index)
{
  const int i = *index;
  if ((po[i]=='$') && (po[i+1]!='\0')
      && (*cx->quote!='\'') && (cx->param_buf==NULL)) {
    if (isdigit(po[i+1])) {
      return expand_positional(nshell, cx, po, index);
    }
    if (strchr("#?",po[i+1])!=NULL) {
      return expand_special(nshell, cx, po, index);
    }
    if (strchr("*@",po[i+1])!=NULL) {
      return expand_all_args(nshell, cx, po, index);
    }
    if (po[i+1]=='{') {
      return expand_braced(nshell, cx, po, index);
    }
    /* simple variable substitution */
    return expand_simple(nshell, cx, po, index);
  }
  if ((po[i]=='}') && (*cx->quote!='\'') && (cx->param_buf!=NULL)) {
    return expand_parameter_modifier(nshell, cx);
  }
  return 0;
}

static int
expand_quotation(struct expand_context *cx, int ch)
{
  if ((*cx->quote=='"') || (*cx->quote=='\'')) {
    if (ch==*cx->quote) *cx->quote='\0';
    if (cx->param_buf==NULL) {
      if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
    }
    return 1;
  } else if (ch=='\'' || ch=='"') {
    *cx->quote=ch;
    if (cx->param_buf==NULL) {
      if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
    }
    return 1;
  }
  return 0;
}

static int
expand_ifs (struct expand_context *cx, int ch, char *ifs, int ifsexp)
{
  if (cx->param_buf==NULL) {
    if (ifsexp && (ifs!=NULL) && (ifs[0]!='\0') && (strchr(ifs,ch)!=NULL)) {
      if ((cx->s=nstrccat(cx->s,(char )0x01))==NULL) return -1;
    } else {
      if ((cx->s=nstrccat(cx->s,ch))==NULL) return -1;
    }
    return 1;
  }
  return 0;
}

static char *
expand(struct nshell *nshell,char *str,int *quote,int *bquote, int ifsexp)
{
  char *po;
  int i;
  struct expand_context cx;

  if ((po=str)==NULL) return NULL;
  if ((cx.s=nstrnew())==NULL) return NULL;

  cx.quote = quote;
  cx.bquote = bquote;
  cx.name = NULL;
  cx.cmd_buf = cx.param_buf = NULL;
  cx.escape = FALSE;
  char *ifs = getval(nshell,"IFS");

  for (i=0;po[i]!='\0';i++) {
    /* check escapse */
    int res;
    res = expand_escape(&cx, po[i], po[i + 1]);
    if (res < 0) goto errexit;
    if (res > 0) goto common_se; // 特殊文字として処理済みなら se蓄積へ

    /* check back quote */
    res = expand_bquote (nshell, &cx, po[i]);
    if (res < 0) goto errexit;
    if (res > 0) goto common_se; // 特殊文字として処理済みなら se蓄積へ

    /* check constant variable */
    res = expand_variable(nshell, &cx, po, &i);
    if (res < 0) goto errexit;
    if (res > 0) goto common_se; // 特殊文字として処理済みなら se蓄積へ

    /* check quotation */
    res = expand_quotation(&cx, po[i]);
    if (res < 0) goto errexit;
    if (res > 0) goto common_se; // 特殊文字として処理済みなら se蓄積へ

    expand_ifs (&cx, po[i], ifs, ifsexp);

  common_se:
    if (cx.param_buf!=NULL) {
      if ((cx.param_buf=nstrccat(cx.param_buf,po[i]))==NULL) goto errexit;
    }
  }

  if ((cx.param_buf!=NULL) || (cx.cmd_buf!=NULL)) {
    sherror(ERRBADSUB);
    goto errexit;
  }

  g_free(cx.name);
  g_free(cx.cmd_buf);
  g_free(cx.param_buf);
  return cx.s;

errexit:
  g_free(cx.name);
  g_free(cx.cmd_buf);
  g_free(cx.param_buf);
  g_free(cx.s);
  return NULL;
}

static int
checkcmd(struct nshell *nshell,struct cmdlist **cmdroot)
/* checkcmd() returns
     -1: fatal error
      0: noerror
      2: error */
{
  struct cmdlist *cmdcur,*cmdprev,*cmddel,*cmdnew;
  struct prmlist *prmcur,*prmprev;
  int cmd,i;
  char *po;
  char *s,*eof, *prompt;
  int quoted,ch;
  int eoflen,match,len;
  char *tmpfil;
  int sout;
  int ignore_indent, remove_tab;

  cmdcur=*cmdroot;
  cmdprev=NULL;
  while (cmdcur!=NULL) {
    if (cmdcur->prm==NULL) {
      if (cmdprev==NULL) *cmdroot=cmdcur->next;
      else cmdprev->next=cmdcur->next;
      cmddel=cmdcur;
      cmdcur=cmdcur->next;
      g_free(cmddel);
    } else {
      /* remove zero length parameter */
      prmcur=cmdcur->prm;
      cmd=CPNO;
      while (prmcur->next!=NULL) prmcur=prmcur->next;
      if (prmcur->str[0]==')') cmd=CPPATI;
      prmcur=cmdcur->prm;
      if ((prmcur->str[0]!='\0')
      && (strchr(cpcmdtable[0],prmcur->str[0])!=NULL)) cmd=CPNULL;

      i = check_cpcmd(prmcur->str);
      if (i >= 0) {
	cmd = i + 1;
      } else {
        prmcur=prmcur->next;
        if ((prmcur!=NULL) && (prmcur->str[0]=='(')) {
          prmcur=prmcur->next;
          if ((prmcur!=NULL) && (prmcur->str[0]==')')) cmd=CPFN;
        }
      }
      cmdcur->cmdno=cmd;
      if ((cmd!=CPNO) && (cmd!=CPCASE) && (cmd!=CPPATI) && (cmd!=CPPATO)
      && (cmd!=CPFOR) && (cmd!=CPFN) && ((cmdcur->prm)->next!=NULL)) {
        if ((cmdnew=g_malloc(sizeof(struct cmdlist)))==NULL) return -1;
        cmdnew->next=cmdcur->next;
        cmdcur->next=cmdnew;
        cmdnew->prm=(cmdcur->prm)->next;
        (cmdcur->prm)->next=NULL;
        cmdnew->cmdno=CPNO;
      }

      /* check pattern */
      if (cmdcur->cmdno==CPCASE) {
        prmcur=cmdcur->prm; /* case */
        if (prmcur->next!=NULL) {
          prmcur=prmcur->next; /* word */
          if (prmcur->next!=NULL) {
            prmprev=prmcur->next; /* in */
            if (prmprev->next!=NULL) {
              prmcur=prmprev->next;
              prmprev->next=NULL;
              if ((cmdnew=g_malloc(sizeof(struct cmdlist)))==NULL) return -1;
              cmdnew->next=cmdcur->next;
              cmdcur->next=cmdnew;
              cmdnew->prm=prmcur;
            }
          }
        }
      }

      /* check redirect and pipe */
      cmdcur->cmdend=PPEND;
      cmdcur->pipefile=NULL;
      prmcur=cmdcur->prm;
      prmprev=NULL;
      while (prmcur!=NULL) {
        if (prmcur->str!=NULL) {
          if ((prmcur->str[0]=='>') || (prmcur->str[0]=='<')) {
            if ((prmcur->next==NULL) || ((prmcur->next)->str==NULL)
             || (strchr(";&|",(prmcur->next)->str[0])!=NULL)) {
              sherror2(ERRUEXPTOK,prmprev->str);
              return 2;
            }
            if (prmcur->str[0]==prmcur->str[1]) {
              if (prmcur->str[0]=='>')
                prmcur->prmno=PPSO2;
              else {
                prmcur->prmno=PPSI2;
                if ((eof=unquotation((prmcur->next)->str,&quoted))==NULL)
                  return -1;
                g_free((prmcur->next)->str);
                (prmcur->next)->str=eof;
                (prmcur->next)->quoted=quoted;
		if (prmcur->str[2] == '-') {
		  ignore_indent = TRUE;
		  remove_tab = TRUE;
		} else {
		  ignore_indent = FALSE;
		  remove_tab = FALSE;
		}

                /* get << contents */

                // if ((sys=getobject("system"))==NULL) return -1; /* fix-me: assigned value of the variable "sys" is never used */
		sout = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &tmpfil);
		if (sout < 0)
		  return -1;
                if ((quoted) && !nisatty(nshell->fd)) {
                  eoflen=strlen(eof);
                  match=0;
                  writepo=0;
                  while (TRUE) {
                    ch=shget(nshell);
                    if ((ch==EOF) || (ch=='\0')) break;
		    if (remove_tab && ch == '\t') continue;
		    remove_tab = FALSE;
                    if (ch=='\n') {
                      if (match==eoflen) break;
                      writebuf[writepo]=ch;
                      writepo++;
                      if (writepo==WRITEBUFSIZE) {
                        nwrite(sout,writebuf,WRITEBUFSIZE);
                        writepo=0;
                      }
                      match=0;
		      remove_tab = ignore_indent;
                    } else if (match==-1) {
                      writebuf[writepo]=ch;
                      writepo++;
                      if (writepo==WRITEBUFSIZE) {
                        nwrite(sout,writebuf,WRITEBUFSIZE);
                        writepo=0;
                      }
                    } else if ((match==eoflen) || (ch!=eof[match])) {
                      for (i=0;i<match;i++) {
                        writebuf[writepo]=eof[i];
                        writepo++;
                        if (writepo==WRITEBUFSIZE) {
                          nwrite(sout,writebuf,WRITEBUFSIZE);
                          writepo=0;
                        }
                      }
                      writebuf[writepo]=ch;
                      writepo++;
                      if (writepo==WRITEBUFSIZE) {
                        nwrite(sout,writebuf,WRITEBUFSIZE);
                        writepo=0;
                      }
                      match=-1;
                    } else match++;
                  }
                  if (writepo!=0) nwrite(sout,writebuf,writepo);
                } else {
                  do {
		    remove_tab = ignore_indent;
                    if ((s=nstrnew())==NULL) {
                      nclose(sout);
                      unlinkfile(&tmpfil);
                      return -1;
                    }
		    if (nisatty(nshell->fd)) {
		      prompt = getval(nshell,"PS2");
#ifdef HAVE_READLINE_READLINE_H
		      Prompt = prompt;
		      MultiLine = TRUE;
#else
		      if (prompt) printfconsole("%.256s", prompt);
#endif
		    }
                    do {
                      ch=shget(nshell);
                      if (ch==EOF) {
                        if (strlen(s)!=0) {
                          if (!nisatty(nshell->fd)) break;
                          else printfconsole("%c",(char )0x07);
                        } else break;
                      } else {
                        if (ch=='\n') ch='\0';
			if (remove_tab && ch == '\t') continue;
			remove_tab = FALSE;
                        if ((s=nstrccat(s,ch))==NULL) {
                          nclose(sout);
                          unlinkfile(&tmpfil);
                          return -1;
                        }
                      }
                    } while (ch!='\0');
                    if (strcmp0(eof,s)!=0) {
                      len=strlen(s);
                      s[len]='\n';
                      nwrite(sout,s,len+1);
                      g_free(s);
                    } else {
                      g_free(s);
                      break;
                    }
                  } while (ch!=EOF);
                }
                nclose(sout);
                g_free((prmcur->next)->str);
                (prmcur->next)->str=tmpfil;
              }
            } else {
              if (prmcur->str[0]=='>')
                prmcur->prmno=PPSO1;
              else
                prmcur->prmno=PPSI1;
            }

            (prmcur->next)->prmno=prmcur->prmno;
            prmcur=prmcur->next;
          } else if (prmcur->str[0]=='|') {
            if (cmdcur->cmdno==CPPATI) {
	      prmcur->prmno=PPPATOR;
            } else {
              cmdcur->cmdend=PPPIPE;
              prmcur->prmno=PPPIPE;
              if ((prmcur->next==NULL) || ((prmcur->next)->str==NULL)
               || (strchr(";&|",(prmcur->next)->str[0])!=NULL)) {
                sherror2(ERRUEXPTOK,prmcur->str);
                return 2;
              }
              if ((cmdnew=g_malloc(sizeof(struct cmdlist)))==NULL) return -1;
              cmdnew->next=cmdcur->next;
              cmdcur->next=cmdnew;
              cmdnew->prm=prmcur->next;
              prmcur->next=NULL;
              cmdnew->cmdno=CPNO;
            }
          } else if (prmcur->str[0]=='&') {
            prmcur->prmno=PPAND;
            cmdcur->cmdend=PPAND;
          } else if (prmcur->str[0]==';') {
            if (prmcur->str[1]==';') {
	      cmdcur->cmdno=CPPATO;
	      /* how is prmcur->prmno ? */
	      prmcur->prmno=PPNULL; /* is it right? */
	    } else {
	      prmcur->prmno=PPEND;
	    }
          } else {
	    prmcur->prmno=PPNO;
	  }
        } else {
	  prmcur->prmno=PPNULL;
	}
        prmprev=prmcur;
        prmcur=prmcur->next;
      }

      /* check variable setting command */
      prmcur=cmdcur->prm;
      while (prmcur!=NULL) {
        if (prmcur->prmno == PPNO) {
          po=prmcur->str;
          if (isobject(&po) && (po[0]=='=')) {
            prmcur->prmno=PPSETO;
          } else {
            po=prmcur->str;
	    i = find_varname_end(po, 0);
            if ((i!=0) && (po[i]=='=')) prmcur->prmno=PPSETV;
          }
          if (prmcur->prmno==PPNO) break;
        }
        prmcur=prmcur->next;
      }

      cmdprev=cmdcur;
      cmdcur=cmdcur->next;
    }
  }
  return 0;
}

static int
syntax(struct nshell *nshell,
       struct cmdlist *cmdroot,int *needcmd,struct cmdstack **sx)
/* syntax() returns
     -1: fatal error
      0: noerror
      2: syntax error */
{
  struct cmdlist *cmdcur;
  struct prmlist *prmcur;
  int i;
  char *s;
  struct cmdstack *st;

  cmdcur=cmdroot;
  while (cmdcur!=NULL) {
    int cmd, c;
    cmd=cmdcur->cmdno;
    c=cmdstackgetlast(sx);
    if ((c==CPFOR) && (cmd!=CPDO)) {
      sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
      return 2;
    }
    if ((c==CPCASE) && (cmd!=CPPATI) && (cmd!=CPESAC)) {
      sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
      return 2;
    }
    if ((c==CPFN) && (cmd!=CPBI)) {
      sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
      return 2;
    }
    if ((*needcmd) &&
       !((cmd==CPIF) || (cmd==CPCASE) || (cmd==CPFOR) || (cmd==CPWHILE)
     || (cmd==CPUNTIL) || (cmd==CPFN) || (cmd==CPNO))) {
      sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
      return 2;
    }
    if ((cmd==CPIF) || (cmd==CPTHEN) || (cmd==CPELSE) || (cmd==CPELIF)
     || (cmd==CPWHILE) || (cmd==CPUNTIL) || (cmd==CPDO) || (cmd==CPBI))
      *needcmd=TRUE;
    else if (cmdcur->cmdend==PPPIPE) *needcmd=TRUE;
    else *needcmd=FALSE;
    switch (cmd) {
    case CPNULL:
      sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
      return 2;
    case CPIF:
      if ((st=cmdstackcat(sx,CPIF))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPTHEN:
      if (cmdstackgetlast(sx)!=CPIF) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      if ((st=cmdstackcat(sx,CPTHEN))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPELIF:
      if (cmdstackgetlast(sx)!=CPTHEN) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      if ((st=cmdstackcat(sx,CPIF))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPELSE:
      if (cmdstackgetlast(sx)!=CPTHEN) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      if ((st=cmdstackcat(sx,CPELSE))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPFI:
      c=cmdstackgetlast(sx);
      if ((c!=CPTHEN) && (c!=CPELSE)) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      break;
    case CPCASE:
      if ((st=cmdstackcat(sx,CPCASE))==NULL) return -1;
      st->cmd=cmdcur;
      prmcur=(cmdcur->prm)->next;
      if ((prmcur==NULL) || (prmcur->next==NULL)) {
        sherror2(ERRSYNTAX,(cmdcur->prm)->str);
        return 2;
      }
      prmcur=prmcur->next;
      if ((prmcur->str==NULL) || (strcmp0("in",prmcur->str)!=0)) {
        sherror2(ERRSYNTAX,prmcur->str);
        return 2;
      }
      if (prmcur->next!=NULL) {
        sherror2(ERRUEXPTOK,(prmcur->next)->str);
        return 2;
      }
      break;
    case CPPATI:
      c=cmdstackgetlast(sx);
      if ((c!=CPCASE) && (c!=CPPATO)) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      prmcur=cmdcur->prm;
      if (prmcur->str[0]==')') {
        sherror2(ERRUEXPTOK,prmcur->str);
        return 2;
      }
      prmcur=prmcur->next;
      while (TRUE) {
        if (prmcur->str[0]==')') break;
        if (prmcur->prmno!=PPPATOR) {
          sherror2(ERRUEXPTOK,prmcur->str);
          return 2;
        }
        if ((prmcur->next==NULL)
        || ((prmcur->next)->next==NULL)) {
          sherror2(ERRUEXPTOK,prmcur->str);
          return 2;
        }
        prmcur=(prmcur->next)->next;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      if ((st=cmdstackcat(sx,CPPATI))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPPATO:
      if (cmdstackgetlast(sx)!=CPPATI) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      if ((st=cmdstackcat(sx,CPPATO))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPESAC:
      c=cmdstackgetlast(sx);
      if ((c!=CPCASE) && (c!=CPPATI) && (c!=CPPATO)) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      break;
    case CPFOR:
      if (cmdstackcat(sx,CPFOR)==NULL) return -1;
      prmcur=(cmdcur->prm)->next;
      if (prmcur==NULL) {
        sherror2(ERRSYNTAX,(cmdcur->prm)->str);
        return 2;
      }
      prmcur=prmcur->next;
      if (prmcur!=NULL) {
        if (strcmp0("in",prmcur->str)!=0) {
          sherror2(ERRSYNTAX,prmcur->str);
          return 2;
        }
        prmcur=prmcur->next;
        while (prmcur!=NULL) {
          if ((prmcur->prmno!=PPNO) && (prmcur->prmno!=PPEND)) {
            sherror2(ERRUEXPTOK,prmcur->str);
            return 2;
          }
          prmcur=prmcur->next;
        }
      }
      break;
    case CPWHILE:
      if (cmdstackcat(sx,CPWHILE)==NULL) return -1;
      break;
    case CPUNTIL:
      if (cmdstackcat(sx,CPUNTIL)==NULL) return -1;
      break;
    case CPDO:
      c=cmdstackgetlast(sx);
      if ((c!=CPFOR) && (c!=CPWHILE) && (c!=CPUNTIL)) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      cmdstackrmlast(sx);
      if ((st=cmdstackcat(sx,CPDO))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPDONE:
      if (cmdstackgetlast(sx)!=CPDO) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      break;
    case CPFN:
      s=(cmdcur->prm)->str;
      for (i=0;s[i]!='\0';i++) if (!isalnum(s[i]) && (s[i]!='_')) {
        sherror2(ERRIDENT,s);
        return 2;
      }
      if ((st=cmdstackcat(sx,CPFN))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPBI:
      if (cmdstackgetlast(sx)!=CPFN) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackcat(sx,CPBI))==NULL) return -1;
      st->cmd=cmdcur;
      break;
    case CPBO:
      if (cmdstackgetlast(sx)!=CPBI) {
        sherror2(ERRUEXPTOK,(cmdcur->prm)->str);
        return 2;
      }
      if ((st=cmdstackgetpo(sx))==NULL) return -1;
      (st->cmd)->done=cmdcur;
      cmdstackrmlast(sx);
      cmdstackrmlast(sx);
      break;
    default:
      break;
    }
    cmdcur=cmdcur->next;
  }
  return 0;
}

struct set_env_arg {
  struct nshell *nshell;
  char ***newenviron;
};

static int
set_env_val(struct nhash *h, void *data)
{
  struct vallist *valcur;
  struct set_env_arg *arg;
  const char *val, *env;
  char *s;
  int r;

  valcur = (struct vallist *) h->val.p;
  arg = (struct set_env_arg *) data;

  if (valcur->func)
    return 0;

  r = getexp(arg->nshell, valcur->name);
  if (r || valcur->arg) {
    val = valcur->val;
  } else if ((env = g_getenv(valcur->name))) {
    val = env;
  } else {
    return 0;
  }

  s = g_strdup_printf("%s=%s", valcur->name, val);
  if (s == NULL)
    return 1;

  if (arg_add(arg->newenviron, s) == NULL) {
    g_free(s);
    return 1;
  }

  return 0;
}

int
msleep(int ms)
{
#ifdef HAVE_NANOSLEEP
  struct timespec ts;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  return nanosleep(&ts, NULL);
#else
  return usleep(ms * 1000);
#endif
}

#if WINDOWS
static int WaitProc;

static void *
proc_in_thread(void *ptr)
{
  char *cmd;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  DWORD exit_code;
  int r;

  if (ptr == NULL) {
    return NULL;
  }

  while (! WaitProc) {
    msleep(1);
  }

  cmd = (char *) ptr;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  r = CreateProcess(NULL, cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
  if (r == 0) {
    show_system_error();
    WaitProc = 0;
    return NULL;
  }

  CloseHandle(pi.hThread);
  WaitForSingleObject(pi.hProcess, INFINITE);
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);

  WaitProc = 0;

  return GINT_TO_POINTER(exit_code);
}

static char *
quote_args(char **args)
{
  int len, i;
  char *cmd, *ptr;

  if (args == NULL) {
    return NULL;
  }

  for (len = i = 0; args[i]; i++) {
    len += strlen(args[i]) * 2 + 3;
  }

  cmd = g_malloc(len + 1);
  if (cmd == NULL) {
    return NULL;
  }

  ptr = cmd;
  for (i = 0; args[i]; i++) {
    char *arg;

    *ptr = '"';
    ptr++;
    arg = args[i];
    while (*arg) {
      char ch;
      ch = *arg;
      if (ch == '"') {
	*ptr = '\\';
	ptr++;
      }
      *ptr = ch;
      ptr++;
      arg++;
    }
    *ptr = '"';
    ptr++;
    *ptr = ' ';
    ptr++;
  }
  *ptr = '\0';

  return cmd;
}
#endif	/* WINDOWS */

void
set_interrupt(void)
{
  Interrupted = TRUE;
}

void
reset_interrupt(void)
{
  Interrupted = FALSE;
}

int
check_interrupt(void)
{
  int state;
  state = Interrupted;
  Interrupted = FALSE;
  return state;
}

struct cmdexec_fd {
  int sout, sout2, sin, sin2;
  char *fstdout, *fstdin;
  struct vallist *newvalroot;
  struct cmdlist *cmdcur;
  char **newenviron, *tmpfil2;
  struct prmlist *prmnewroot;
  char **env;
};

struct cmdexec_pipe {
  int istdout, istdin, pipef, pnum;
};

static int
cmdparse_redirect(struct cmdexec_pipe *pipe, struct cmdexec_fd *cmdfd)
{
  int quoted = FALSE;
  struct prmlist *prmcur, *prmprev, *prm;
  pipe->istdout=pipe->istdin=PPNO;
  pipe->pipef=FALSE;
  prmcur=cmdfd->prmnewroot;
  prmprev=NULL;
  pipe->pnum=0;
  while (prmcur!=NULL) {
    if (prmcur->str==NULL) {
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm->str);
      g_free(prm);
    } else if ((prmcur->prmno==PPSI1) || (prmcur->prmno==PPSI2)
	       || (prmcur->prmno==PPSO1) || (prmcur->prmno==PPSO2)) {
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm->str);
      g_free(prm);
      switch (prmcur->prmno) {
      case PPSI1:
	g_free(cmdfd->fstdin);
	cmdfd->fstdin=prmcur->str;
	pipe->istdin=PPSI1;
	break;
      case PPSI2:
	g_free(cmdfd->fstdin);
	cmdfd->fstdin=prmcur->str;
	pipe->istdin=PPSI2;
	quoted=prmcur->quoted;
	break;
      case PPSO1:
	g_free(cmdfd->fstdout);
	cmdfd->fstdout=prmcur->str;
	pipe->istdout=PPSO1;
	break;
      case PPSO2:
	g_free(cmdfd->fstdout);
	cmdfd->fstdout=prmcur->str;
	pipe->istdout=PPSO2;
	break;
      }
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm);
    } else if (prmcur->prmno==PPPIPE) {
      pipe->pipef=TRUE;
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm->str);
      g_free(prm);
    } else if (prmcur->prmno==PPEND) {
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm->str);
      g_free(prm);
    } else if ((prmcur->prmno!=PPSETV) && (prmcur->prmno!=PPSETO)) {
      pipe->pnum++;
      prmprev=prmcur;
      prmcur=prmcur->next;
    } else {
      prmprev=prmcur;
      prmcur=prmcur->next;
    }
  }
  return quoted;
}

static int
cmdexpand(struct nshell *nshell, struct cmdexec_fd *cmdfd)
{
  struct prmlist *prmcur, *prmprev, *prm;
  char *str;
  prmprev=NULL;
  prmcur=cmdfd->cmdcur->prm;
  while (prmcur!=NULL) {
    if (prmcur->prmno!=PPSI2) {
      int quote, bquote;
      quote='\0';
      bquote='\0';
      if ((str=expand(nshell,prmcur->str,&quote,&bquote,FALSE))==NULL)
	return 1;
      if (quote || bquote) {
	g_free(str);
	sherror(ERRUEXPEOF);
	return 1;
      }
      prmcur->quoted=FALSE;
    } else {
      if ((str=g_malloc(strlen(prmcur->str)+1))==NULL) return 1;
      strcpy(str,prmcur->str);
    }
    if ((prmcur->prmno!=PPNO) || (str[0]!='\0')) {
      if ((prm=g_malloc(sizeof(struct prmlist)))==NULL) return 1;
      if (prmprev==NULL) cmdfd->prmnewroot=prm;
      else prmprev->next=prm;
      prm->next=NULL;
      prm->prmno=prmcur->prmno;
      prm->quoted=prmcur->quoted;
      if (prmcur->prmno!=PPSI2) {
	if ((prm->str=fnexpand(nshell,str))==NULL) return 1;
	if (prm->prmno==PPNO) {
	  int num=wordsplit(prm);
	  if (num==-1) return 1;
	  for (int j=0;j<num;j++) {
	    prmprev=prm;
	    prm->prmno=PPNO;
	    prm=prm->next;
	  }
	} else {
	  wordunsplit(prm->str);
	  prmprev=prm;
	}
      } else {
	prm->str=str;
	prmprev=prm;
      }
    } else g_free(str);
    prmcur=prmcur->next;
  }
  return 0;
}

static int
cmdsetvar(struct nshell *nshell, struct cmdexec_fd *cmdfd, const struct cmdexec_pipe *pipe)
{
  struct prmlist *prmcur,*prmprev,*prm;
  char *name,*val;
  prmcur=cmdfd->prmnewroot;
  prmprev=NULL;
  while (prmcur!=NULL) {
    if (prmcur->prmno==PPSETV) {
      const char *po;
      po=strchr(prmcur->str,'=');
      if ((name=g_malloc(po-prmcur->str+1))==NULL) return 1;
      strncpy(name,prmcur->str,po-prmcur->str);
      name[po-prmcur->str]='\0';
      if ((val=g_malloc(strlen(prmcur->str)-(po-prmcur->str)))==NULL) {
	g_free(name);
	return 1;
      }
      strcpy(val,po+1);
      if (pipe->pnum==0) po=addval(nshell,name,val);
      else po=saveval(nshell,name,val,&cmdfd->newvalroot);
      g_free(name);
      g_free(val);
      if (po==NULL) return 1;
      g_free(prmcur->str);
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm);
    } else if (prmcur->prmno==PPSETO) {
      /* set object */
      if (sputobj(prmcur->str)==-1) return 1;
      g_free(prmcur->str);
      if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
      else prmprev->next=prmcur->next;
      prm=prmcur;
      prmcur=prmcur->next;
      g_free(prm);
    } else {
      prmprev=prmcur;
      prmcur=prmcur->next;
    }
  }
  return 0;
}

static int
cmdrun (struct nshell *nshell, int namedfunc, struct cmdexec_fd *cmdfd, struct cmdexec_pipe *pipe, int quoted, struct cmdstack **stroot, int *err)
{
  struct prmlist *prmcur,*prm;
  char *tmpfil, *str;
  struct cmdlist *cmdnew;
  char **argvsave,**argvnew,*s;
  int argcsave,argcnew,a,looplevel,errlevel, lastc;
  char *arg,*endptr;
  struct cmdstack *stcur,*st;

  prmcur=cmdfd->prmnewroot;
  if ((prmcur!=NULL) && (prmcur->prmno==PPNO)) {

    /* set environment variable */
#if USE_HASH
    {
      struct set_env_arg se_arg;

      se_arg.nshell = nshell;
      se_arg.newenviron = &cmdfd->newenviron;
      if (nhash_each(nshell->valroot, set_env_val, &se_arg))
	return 1;
    }
#else
    struct vallist *valcur = nshell->valroot;
    while (valcur!=NULL) {
      if (!valcur->func
	  && (getexp(nshell,valcur->name) || valcur->arg ||
	      (g_getenv(valcur->name)!=NULL))) {
	len=strlen(valcur->name);
	if (getexp(nshell,valcur->name) || valcur->arg) val=valcur->val;
	else val=g_getenv(valcur->name);
	if ((s=g_malloc(len+strlen(val)+2))==NULL) return 1;
	strcpy(s,valcur->name);
	s[len]='=';
	strcpy(s+len+1,val);
	if (arg_add(&cmdfd->newenviron,s)==NULL) {
	  g_free(s);
	  return 1;
	}
      }
      valcur=valcur->next;
    }
#endif

    cmdfd->env=MainEnviron;
    MainEnviron=cmdfd->newenviron;
    cmdfd->newenviron=NULL;
    cmdfd->sout=cmdfd->sin=NOHANDLE;
    if (pipe->istdin!=PPNO) {
      if (pipe->istdin==PPSI1) cmdfd->sin=nopen(cmdfd->fstdin,O_RDONLY,NFMODE);
      else if (pipe->istdin==PPSI2) {
	cmdfd->sin=nopen(cmdfd->fstdin,O_RDONLY,NFMODE);
	if (!quoted) {
	  char ch;
	  int fd = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &cmdfd->tmpfil2);
	  if (fd < 0) {
	    nclose(cmdfd->sin);
	    return 1;
	  }
	  do {
	    s = nstrnew();
	    if (s == NULL) {
	      nclose(cmdfd->sin);
	      nclose(fd);
	      return 1;
	    }
	    do {
	      char buf[2];
	      if (nread(cmdfd->sin,buf,1)>0) ch=buf[0];
	      else ch=EOF;
	      if (ch==EOF) break;
	      else {
		if (ch=='\n') ch='\0';
		if ((s=nstrccat(s,ch))==NULL) {
		  nclose(cmdfd->sin);
		  nclose(fd);
		  return 1;
		}
	      }
	    } while (ch!='\0');
	    int quote, bquote;
	    quote=bquote='\0';
	    str=s;
	    s=expand(nshell,str,&quote,&bquote,FALSE);
	    g_free(str);
	    if (s==NULL) {
	      nclose(cmdfd->sin);
	      nclose(fd);
	      return 1;
	    }
	    wordunsplit(s);
	    int len=strlen(s);
	    if (ch!=EOF) {
	      s[len]='\n';
	      len++;
	    }
	    nwrite(fd,s,len);
	    g_free(s);
	  } while (ch!=EOF);
	  nclose(cmdfd->sin);
	  nclose(fd);
	  cmdfd->sin=nopen(cmdfd->tmpfil2,O_RDONLY,NFMODE);
	}
      }
    } else if (cmdfd->cmdcur->pipefile!=NULL) {
      cmdfd->sin=nopen(cmdfd->cmdcur->pipefile,O_RDONLY,NFMODE);
    }
    if (cmdfd->sin!=NOHANDLE) cmdfd->sin2=nredirect(0,cmdfd->sin);

    /* redirect : stdout */
    if (pipe->istdout!=PPNO) {
      if (Security) {
	sherror(ERRSECURITY);
	return 1;
      }
      if (pipe->istdout==PPSO2)
	cmdfd->sout=nopen(cmdfd->fstdout,O_APPEND|O_CREAT|O_WRONLY,NFMODE);
      else cmdfd->sout=nopen(cmdfd->fstdout,O_CREAT|O_WRONLY|O_TRUNC,NFMODE);
      /* pipe */
    } else if (pipe->pipef) {
      // struct objlist *sys;
      // if ((sys=getobject("system"))==NULL) return 1; /* fix-me: assigned value of the variable "sys" is never used */
      cmdfd->sout = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &tmpfil);
      if (cmdfd->sout < 0)
	return 1;
      unlinkfile(&((cmdfd->cmdcur->next)->pipefile));
      (cmdfd->cmdcur->next)->pipefile=tmpfil;
    }
    if (cmdfd->sout!=NOHANDLE) cmdfd->sout2=nredirect(1,cmdfd->sout);

    if (nshell->optionx) {
      for (int i=0;i<nshell->cmdexec;i++) printfconsole("+");
      printfconsole(" ");
      prm=prmcur;
      while (prm!=NULL) {
	printfconsole("%.256s ",prm->str);
	prm=prm->next;
      }
      printfconsole("\n");
    }

    char *cmds=prmcur->str;
    /* exec named function */
    if ((cmdnew=getfunc(nshell,cmds))!=NULL) {
      int rcode,needcmd;

      needcmd=FALSE;
      st=NULL;
      rcode=syntax(nshell,cmdnew,&needcmd,&st);
      cmdstackfree(st);
      if ((rcode!=0) || (st!=NULL) || (needcmd)) return 1;
      argvnew=NULL;
      if ((s=g_malloc(strlen((nshell->argv)[0])+1))==NULL)
	return 1;
      strcpy(s,(nshell->argv)[0]);
      if (arg_add(&argvnew,s)==NULL) {
	g_free(s);
	arg_del(argvnew);
	return 1;
      }
      prmcur=prmcur->next;
      while (prmcur!=NULL) {
	if ((s=g_malloc(strlen(prmcur->str)+1))==NULL) {
	  g_free(s);
	  arg_del(argvnew);
	  return 1;
	}
	strcpy(s,prmcur->str);
	if (arg_add(&argvnew,s)==NULL) {
	  g_free(s);
	  arg_del(argvnew);
	  return 1;
	}
	prmcur=prmcur->next;
      }
      argcnew=getargc(argvnew);
      argcsave=nshell->argc;
      argvsave=nshell->argv;
      nshell->argc=argcnew;
      nshell->argv=argvnew;
      rcode=cmdexec(nshell,cmdnew,TRUE);
      arg_del(nshell->argv);
      nshell->argc=argcsave;
      nshell->argv=argvsave;
      if ((rcode==-1) || (rcode==1)) return 1;
      /* exec special command */
      /* . */
    } else if (strcmp0(".",cmds)==0) {
      prmcur=prmcur->next;
      if (prmcur!=NULL) {
	char *cmdname;
	cmdname=nsearchpath(getval(nshell,"PATH"),prmcur->str,TRUE);
	if (cmdname!=NULL) {
	  char *readbuf;
	  int readbyte;
	  int readpo;

	  argvnew=NULL;
	  if ((s=g_malloc(strlen((nshell->argv)[0])+1))==NULL)
	    return 1;
	  strcpy(s,(nshell->argv)[0]);
	  if (arg_add(&argvnew,s)==NULL) {
	    g_free(s);
	    arg_del(argvnew);
	    return 1;
	  }
	  prmcur=prmcur->next;
	  while (prmcur!=NULL) {
	    if ((s=g_malloc(strlen(prmcur->str)+1))==NULL) {
	      g_free(s);
	      arg_del(argvnew);
	      return 1;
	    }
	    strcpy(s,prmcur->str);
	    if (arg_add(&argvnew,s)==NULL) {
	      g_free(s);
	      arg_del(argvnew);
	      return 1;
	    }
	    prmcur=prmcur->next;
	  }
	  argcnew=getargc(argvnew);

	  cmdfd->sin=nopen(cmdname,O_RDONLY,NFMODE);
	  g_free(cmdname);
	  cmdname=NULL;
	  if (cmdfd->sin==NOHANDLE) {
	    sherror2(ERROPEN,cmdname);
	    arg_del(argvnew);
	    return 1;
	  }
	  argcsave=nshell->argc;
	  argvsave=nshell->argv;
	  nshell->argc=argcnew;
	  nshell->argv=argvnew;
	  int sin3=storeshhandle(nshell,cmdfd->sin,&readbuf,&readbyte,&readpo);
	  int rcode=cmdexecute(nshell,NULL);
	  restoreshhandle(nshell,sin3,readbuf,readbyte,readpo);
	  nclose(cmdfd->sin);
	  arg_del(nshell->argv);
	  nshell->argc=argcsave;
	  nshell->argv=argvsave;
	  if ((rcode!=0) && (rcode!=1)) return 1;
	} else {
	  sherror2(ERRNOFIL,prmcur->str);
	  return 1;
	}
      }
      /* break */
    } else if (strcmp0("break",cmds)==0) {
      if (pipe->pnum>2) {
	sherror(ERREXTARG);
	return 1;
      } else if (pipe->pnum==2) {
	arg=(prmcur->next)->str;
	a=strtol(arg,&endptr,10);
	if (endptr[0]!='\0') {
	  sherror2(ERRNUMERIC,arg);
	  return 1;
	}
      } else a=1;
      looplevel=0;
      stcur=*stroot;
      while (stcur!=NULL) {
	if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
	    || (stcur->cmdno==CPUNTIL)) looplevel++;
	stcur=stcur->next;
      }
      if (looplevel!=0) {
	while (a>0) {
	  if ((stcur=cmdstackgetpo(stroot))==NULL) break;
	  if ((stcur->cmdno==CPFOR) && (stcur->ival==-1)) {
	    prmfree(stcur->prm);
	    stcur->prm=NULL;
	  }
	  if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
	      || (stcur->cmdno==CPUNTIL)) {
	    cmdfd->cmdcur=stcur->cmd;
	    while (cmdfd->cmdcur->cmdno!=CPDO) cmdfd->cmdcur=cmdfd->cmdcur->next;
	    cmdfd->cmdcur=cmdfd->cmdcur->done;
	    cmdstackrmlast(stroot);
	    a--;
	  } else cmdstackrmlast(stroot);
	}
      }
      /* continue */
    } else if (strcmp0("continue",cmds)==0) {
      if (pipe->pnum>2) {
	sherror(ERREXTARG);
	return 1;
      } else if (pipe->pnum==2) {
	arg=(prmcur->next)->str;
	a=strtol(arg,&endptr,10);
	if (endptr[0]!='\0') {
	  sherror2(ERRNUMERIC,arg);
	  return 1;
	}
      } else a=1;
      looplevel=0;
      stcur=*stroot;
      while (stcur!=NULL) {
	if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
	    || (stcur->cmdno==CPUNTIL)) looplevel++;
	stcur=stcur->next;
      }
      if (looplevel!=0) {
	while (a>0) {
	  if ((stcur=cmdstackgetpo(stroot))==NULL) break;
	  if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
	      || (stcur->cmdno==CPUNTIL)) {
	    cmdfd->cmdcur=stcur->cmd;
	    a--;
	    if (a>0) {
	      if ((stcur->cmdno==CPFOR) && (stcur->ival==-1)) {
		prmfree(stcur->prm);
		stcur->prm=NULL;
	      }
	      cmdstackrmlast(stroot);
	    }
	  } else cmdstackrmlast(stroot);
	}
      }
      /* return */
    } else if (strcmp0("return",cmds)==0) {
      if (pipe->pnum>2) {
	sherror(ERREXTARG);
	return 1;
      } else if (pipe->pnum==2) {
	arg=(prmcur->next)->str;
	a=strtol(arg,&endptr,10);
	if (endptr[0]!='\0') {
	  sherror2(ERRNUMERIC,arg);
	  return 1;
	}
	nshell->status=a;
      }
      if (namedfunc) {
	*err=0;
	return 1;
      }
      /* execute object */
    } else if (isobject(&cmds)) {
      int len=0;
      prm=prmcur;
      while (prm!=NULL) {
	len+=strlen(prm->str)+1;
	prm=prm->next;
      }
      if ((str=g_malloc(len))==NULL) return 1;
      str[0]='\0';
      prm=prmcur;
      while (prm!=NULL) {
	strcat(str,prm->str);
	if (prm->next!=NULL) strcat(str," ");
	prm=prm->next;
      }
      errlevel=sexeobj(str);
      g_free(str);
      if (errlevel==-1) return 1;
      nshell->status=errlevel;
      lastc=cmdstackgetlast(stroot);
      if ((lastc!=CPIF) && (lastc!=CPELIF)
	  && (lastc!=CPWHILE) && (lastc!=CPUNTIL)
	  && (nshell->optione) && (errlevel!=0)) {
	*err=3;
	return 1;
      }
      nshell->status=errlevel;
      lastc=cmdstackgetlast(stroot);
      if ((lastc!=CPIF) && (lastc!=CPELIF)
	  && (lastc!=CPWHILE) && (lastc!=CPUNTIL)
	  && (nshell->optione) && (errlevel!=0)) {
	*err=3;
	return 1;
      }
      if (nshell->quit) return 0;
      /* exec inner command */
    } else {
      shell_proc proc;
      char **argv;
      proc = check_cmd(prmcur->str);
      if (proc) {
	argv = NULL;
	if (arg_add(&argv, NULL) == NULL)
	  return 1;
	prm = cmdfd->prmnewroot;
	while (prm) {
	  if (arg_add(&argv, prm->str) == NULL) {
	    g_free(argv);
	    return 1;
	  }
	  prm = prm->next;
	}
	errlevel = proc(nshell, pipe->pnum, (char **)argv);
	g_free(argv);
      } else {
	char *cmdname;
	cmdname=nsearchpath(getval(nshell,"PATH"),prmcur->str,FALSE);
	if (cmdname==NULL) {
	  sherror2(ERRCFOUND,prmcur->str);
	  return 1;
	} else {
	  if (Security) {
	    sherror(ERRSECURITY);
	    return 1;
	  }
	  argv=NULL;
	  if (arg_add(&argv,NULL)==NULL) return 1;
	  prm=cmdfd->prmnewroot;
	  while (prm!=NULL) {
	    if (arg_add(&argv,prm->str)==NULL) {
	      g_free(argv);
	      return 1;
	    }
	    prm=prm->next;
	  }
#if WINDOWS
	  {
	    GThread *thread;
	    char *ptr, *win_cmd;

	    ptr = quote_args(argv);
	    if (ptr == NULL) {
	      sherror(ERRMEMORY);
	      return 1;
	    }

	    win_cmd = g_locale_from_utf8(ptr, -1, NULL, NULL, NULL);
	    g_free(ptr);
	    if (win_cmd == NULL) {
	      sherror(ERRMEMORY);
	      return 1;
	    }

	    errlevel = 0;
	    thread= g_thread_new("process", proc_in_thread, win_cmd);
	    if (thread) {
	      WaitProc = 1;
#if USE_EVENT_LOOP
	      while (WaitProc) {
		msleep(1);
		eventloop();
	      }
#endif
	      errlevel = GPOINTER_TO_INT(g_thread_join(thread));
	    }
	    g_free(win_cmd);
	  }
#else	/* WINDOWS */
	  unset_childhandler();
	  pid_t pid = fork();
	  if (pid < 0) {
	    sherror2(ERRSYSTEM,"fork");
	    set_childhandler();
	    return 1;
	  } else if (pid == 0) {
	    errlevel=execve(cmdname,(char **)argv,MainEnviron);
	    printfstderr("shell: %.64s: %.64s",
			 argv[0],g_strerror(errno));
	    exit(errlevel);
	  } else {
#if USE_EVENT_LOOP
	    if (has_eventloop()) {
	      while (waitpid(pid,&errlevel,WNOHANG)==0) {
		eventloop();
		msleep(10);
	      }
	    } else {
	      waitpid(pid, &errlevel, 0);
	    }
#else
	    waitpid(pid, &errlevel, 0);
#endif
	    errlevel = WIFEXITED(errlevel) ? WEXITSTATUS(errlevel) : 1;
	  }
	  set_childhandler();
#endif	/* WINDOWS */
	  g_free(argv);
	  g_free(cmdname);
	  cmdname=NULL;
	}
      }
      nshell->status = errlevel;
      lastc=cmdstackgetlast(stroot);
      if ((lastc!=CPIF) && (lastc!=CPELIF)
	  && (lastc!=CPWHILE) && (lastc!=CPUNTIL)
	  && (nshell->optione) && (errlevel!=0)) {
	*err=3;
	return 1;
      }
      if (nshell->quit) return 1;
    }

    if (cmdfd->sout2!=NOHANDLE) {
      nredirect2(1,cmdfd->sout2);
      cmdfd->sout2=NOHANDLE;
    }
    if (cmdfd->sin2!=NOHANDLE) {
      nredirect2(0,cmdfd->sin2);
      cmdfd->sin2=NOHANDLE;
    }
    unlinkfile(&cmdfd->tmpfil2);
    arg_del(MainEnviron);
    MainEnviron=cmdfd->env;
    cmdfd->env=NULL;
  }
  restoreval(nshell,cmdfd->newvalroot);
  cmdfd->newvalroot=NULL;

  return 0;
}

static int
cmdexec_cpno(struct nshell *nshell, int namedfunc, struct cmdexec_fd *cmdfd, struct cmdstack **stroot, int *err)
{
  struct cmdexec_pipe pipe;
  int quoted;

  if (cmdexpand(nshell, cmdfd)) {
    return 1;
  }

  /* remove null parameter & check redirect and pipe */
  quoted = cmdparse_redirect(&pipe, cmdfd);

  /* set variable */
  cmdsetvar(nshell,cmdfd, &pipe);

  if (cmdrun (nshell, namedfunc, cmdfd, &pipe, quoted, stroot, err)) {
    return 1;
  }

  prmfree(cmdfd->prmnewroot);
  cmdfd->prmnewroot=NULL;
  g_free(cmdfd->fstdout);
  g_free(cmdfd->fstdin);
  cmdfd->fstdout=cmdfd->fstdin=NULL;
  unlinkfile(&(cmdfd->cmdcur->pipefile));
  cmdfd->cmdcur=cmdfd->cmdcur->next;

  return 0;
}

static int
cmdexec_cpdo(struct nshell *nshell, struct cmdexec_fd *cmdfd, struct cmdstack **stroot)
{
  struct cmdstack *stcur;
  if ((stcur=cmdstackgetpo(stroot))==NULL) return 1;
  if (stcur->cmdno==CPFOR) {
    if (stcur->ival==-1) {
      struct prmlist *prmcur;
      prmcur=stcur->prm;
      if (prmcur==NULL) {
	cmdfd->cmdcur=cmdfd->cmdcur->done;
	cmdstackrmlast(stroot);
	cmdfd->cmdcur=cmdfd->cmdcur->next;
	return 0;
      }
      if (addval(nshell,stcur->val,prmcur->str)==NULL) return 1;
      stcur->prm=prmcur->next;
      g_free(prmcur->str);
      g_free(prmcur);
    } else {
      stcur->ival++;
      if (stcur->ival<nshell->argc) {
	if (addval(nshell,stcur->val,nshell->argv[stcur->ival])==NULL)
	  return 1;
      } else {
	cmdfd->cmdcur=cmdfd->cmdcur->done;
	cmdstackrmlast(stroot);
	cmdfd->cmdcur=cmdfd->cmdcur->next;
	return 0;
      }
    }
  } else if (stcur->cmdno==CPWHILE) {
    if (nshell->status) {
      cmdfd->cmdcur=cmdfd->cmdcur->done;
      cmdstackrmlast(stroot);
      cmdfd->cmdcur=cmdfd->cmdcur->next;
      return 0;
    }
  } else if (stcur->cmdno==CPUNTIL) {
    if (!nshell->status) {
      cmdfd->cmdcur=cmdfd->cmdcur->done;
      cmdstackrmlast(stroot);
      cmdfd->cmdcur=cmdfd->cmdcur->next;
      return 0;
    }
  }
  cmdfd->cmdcur=cmdfd->cmdcur->next;
  return 0;
}

static int
cmdexec_cpfor (struct nshell *nshell, struct cmdexec_fd *cmdfd, struct cmdstack **stroot)
{
  struct cmdstack *stcur;
  struct prmlist *prmcur,*prm;
  int quote,bquote;

  if ((stcur=cmdstackcat(stroot,CPFOR))==NULL) return 1;
  stcur->cmd=cmdfd->cmdcur;
  prmcur=(cmdfd->cmdcur->prm)->next;
  stcur->val=prmcur->str;
  prmcur=prmcur->next;
  if (prmcur==NULL) stcur->ival=0;
  else {
    struct prmlist *prmprev;
    stcur->ival=-1;
    cmdfd->prmnewroot=NULL;
    prmprev=NULL;
    prmcur=prmcur->next;
    while (prmcur!=NULL) {
      char *str;
      if ((prm=g_malloc(sizeof(struct prmlist)))==NULL) return 1;
      if (prmprev==NULL) cmdfd->prmnewroot=prm;
      else prmprev->next=prm;
      prm->next=NULL;
      prm->str=NULL;
      prm->prmno=prmcur->prmno;
      quote='\0';
      bquote='\0';
      if ((str=expand(nshell,prmcur->str,&quote,&bquote,FALSE))==NULL)
	return 1;
      if (quote || bquote) {
	sherror(ERRUEXPEOF);
	return 1;
      }
      if (str[0]!='\0') {
	if ((prm->str=fnexpand(nshell,str))==NULL) return 1;
      } else {
	g_free(str);
	prm->str=NULL;
      }
      int num;
      if ((num=wordsplit(prm))==-1) return 1;
      for (int j=0;j<num;j++) {
	prmprev=prm;
	prm=prm->next;
      }
      prmcur=prmcur->next;
    }
    /* remove null parameter and command end */
    prmcur=cmdfd->prmnewroot;
    prmprev=NULL;
    while (prmcur!=NULL) {
      if ((prmcur->str==NULL) || (prmcur->prmno==PPEND)) {
	if (prmprev==NULL) cmdfd->prmnewroot=prmcur->next;
	else prmprev->next=prmcur->next;
	prm=prmcur;
	prmcur=prmcur->next;
	g_free(prm->str);
	g_free(prm);
      } else {
	prmprev=prmcur;
	prmcur=prmcur->next;
      }
    }
    prmfree(stcur->prm);
    stcur->prm=cmdfd->prmnewroot;
    cmdfd->prmnewroot=NULL;
  }
  cmdfd->cmdcur=cmdfd->cmdcur->next;
  return 0;
}

static int
cmdexec(struct nshell *nshell,struct cmdlist *cmdroot,int namedfunc)
{
  struct cmdlist *cmd;
  struct prmlist *prmcur,*prm;
  int err,quote,bquote;
  char *str,*cmdname;
  struct cmdstack *stroot,*stcur,*st;
  struct cmdexec_fd cmdfd;
  int iftrue,casetrue;
  char *pat;

  reset_interrupt();
  nshell->cmdexec++;
  err=-1;
  stroot=NULL;
  cmdfd.prmnewroot=NULL;
  cmdfd.fstdout=cmdfd.fstdin=NULL;
  cmdfd.env=NULL;
  cmdfd.newenviron=NULL;
  cmdfd.newvalroot=NULL;
  cmdname=NULL;
  cmdfd.sout=cmdfd.sout2=cmdfd.sin=cmdfd.sin2=NOHANDLE;
  cmdfd.tmpfil2=NULL;
  if (nshell->optionv) {
    cmd=cmdroot;
    while (cmd!=NULL) {
      prm=cmd->prm;
      while (prm!=NULL) {
	printfconsole("%.256s ",prm->str);
	prm=prm->next;
      }
      printfconsole("\n");
      cmd=cmd->next;
    }
  }
  cmdfd.cmdcur=cmdroot;
  while ((cmdfd.cmdcur!=NULL) && (!(nshell->quit))) {
    if (ninterrupt()) goto errexit;
    stcur=cmdstackgetpo(&stroot);
    if ((stcur!=NULL) && (stcur->cmdno==CPBI) && (stcur->cmd!=cmdfd.cmdcur)) {
      if (addfunc(nshell,stcur->val,cmdfd.cmdcur)==NULL) goto errexit;
      cmdfd.cmdcur=cmdfd.cmdcur->next;
    } else {
      switch (cmdfd.cmdcur->cmdno) {
      case CPNULL:
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPIF:
	if ((st=cmdstackcat(&stroot,CPIF))==NULL) goto errexit;
	st->iftrue=FALSE;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPTHEN:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	iftrue=st->iftrue;
	if (iftrue || nshell->status) cmdfd.cmdcur=cmdfd.cmdcur->done;
	else {
	  iftrue=TRUE;
	  cmdfd.cmdcur=cmdfd.cmdcur->next;
	}
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPTHEN))==NULL) goto errexit;
	st->iftrue=iftrue;
	break;
      case CPELIF:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	iftrue=st->iftrue;
	if (iftrue) cmdfd.cmdcur=cmdfd.cmdcur->done;
	else cmdfd.cmdcur=cmdfd.cmdcur->next;
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPIF))==NULL) goto errexit;
	st->iftrue=iftrue;
	break;
      case CPELSE:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	iftrue=st->iftrue;
	if (iftrue) cmdfd.cmdcur=cmdfd.cmdcur->done;
	else cmdfd.cmdcur=cmdfd.cmdcur->next;
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPELSE))==NULL) goto errexit;
	st->iftrue=iftrue;
	break;
      case CPFI:
	cmdstackrmlast(&stroot);
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPCASE:
	if ((st=cmdstackcat(&stroot,CPCASE))==NULL) return -1;
	st->casetrue=FALSE;
	prmcur=(cmdfd.cmdcur->prm)->next;
	quote='\0';
	bquote='\0';
	if ((str=expand(nshell,prmcur->str,&quote,&bquote,FALSE))==NULL)
	  goto errexit;
	if (quote || bquote) {
	  sherror(ERRUEXPEOF);
	  goto errexit;
	}
	if ((str=fnexpand(nshell,str))==NULL) goto errexit;
	wordunsplit(str);
	st->pat=str;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPPATI:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	casetrue=st->casetrue;
	pat=st->pat;
	st->pat = NULL;		/* hito (mem leak of pat) */
	if (casetrue) cmdfd.cmdcur=cmdfd.cmdcur->done;
	else {
	  prm=cmdfd.cmdcur->prm;
	  while (prm!=NULL) {
	    if (wildmatch(prm->str,pat,0)) break;
	    prm=prm->next;
	    prm=prm->next;
	  }
	  if (prm!=NULL) {
	    casetrue=TRUE;
	    cmdfd.cmdcur=cmdfd.cmdcur->next;
	  } else cmdfd.cmdcur=cmdfd.cmdcur->done;
	}
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPPATI))==NULL) goto errexit;
	st->casetrue=casetrue;
	st->pat=pat;
	break;
      case CPPATO:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	casetrue=st->casetrue;
	pat=st->pat;
	st->pat = NULL;		/* hito (mem leak of pat) */
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPPATO))==NULL) goto errexit;
	st->casetrue=casetrue;
	st->pat=pat;
	cmdfd.cmdcur=cmdfd.cmdcur->done;
	break;
      case CPESAC:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	casetrue=st->casetrue;
	pat=st->pat;
	st->pat = NULL;		/* hito (mem leak of pat) */
	g_free(pat);
	cmdstackrmlast(&stroot);
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPFOR:
	if (cmdexec_cpfor(nshell, &cmdfd, &stroot)) {
	  goto errexit;
	}
	break;
      case CPWHILE:
	if ((stcur=cmdstackcat(&stroot,CPWHILE))==NULL) goto errexit;
	stcur->cmd=cmdfd.cmdcur;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPUNTIL:
	if ((stcur=cmdstackcat(&stroot,CPUNTIL))==NULL) goto errexit;
	stcur->cmd=cmdfd.cmdcur;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPDO:
	if (cmdexec_cpdo(nshell, &cmdfd, &stroot)) {
	  goto errexit;
	}
	break;
      case CPDONE:
	if ((stcur=cmdstackgetpo(&stroot))==NULL) goto errexit;
	cmdfd.cmdcur=stcur->cmd;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPFN:
	if ((stcur=cmdstackcat(&stroot,CPFN))==NULL) goto errexit;
	prmcur=cmdfd.cmdcur->prm;
	stcur->val=prmcur->str;
	if (newfunc(nshell,prmcur->str)==NULL) goto errexit;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPBI:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	if ((stcur=cmdstackcat(&stroot,CPBI))==NULL) goto errexit;
	stcur->val=st->val;
	stcur->cmd=cmdfd.cmdcur->done;
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPBO:
	cmdstackrmlast(&stroot);
	cmdstackrmlast(&stroot);
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      case CPNO:
	if (cmdexec_cpno(nshell, namedfunc, &cmdfd, &stroot, &err)) {
	  goto errexit;
	}
	break;
      default:
	unlinkfile(&(cmdfd.cmdcur->pipefile));
	cmdfd.cmdcur=cmdfd.cmdcur->next;
	break;
      }
    }
  }
  err=0;

 errexit:
  cmdstackfree(stroot);
  prmfree(cmdfd.prmnewroot);
  g_free(cmdfd.fstdout);
  g_free(cmdfd.fstdin);
  g_free(cmdname);
  arg_del(cmdfd.newenviron);
  if (cmdfd.env!=NULL) {
    arg_del(MainEnviron);
    MainEnviron=cmdfd.env;
  }
  if (cmdfd.newvalroot!=NULL) restoreval(nshell,cmdfd.newvalroot);
  if (cmdfd.sout2!=NOHANDLE) nredirect2(1,cmdfd.sout2);
  if (cmdfd.sin2!=NOHANDLE) nredirect2(0,cmdfd.sin2);
  unlinkfile(&cmdfd.tmpfil2);
  cmdfd.cmdcur=cmdroot;
  while (cmdfd.cmdcur!=NULL) {
    prmcur=cmdfd.cmdcur->prm;
    while (prmcur!=NULL) {
      if (prmcur->prmno == PPSI2 &&
	  prmcur->next!=NULL &&
	  prmcur->next->str != NULL) {
        g_unlink((prmcur->next)->str);
        prmcur=prmcur->next;
      }
      prmcur=prmcur->next;
    }
    unlinkfile(&(cmdfd.cmdcur->pipefile));
    cmdfd.cmdcur=cmdfd.cmdcur->next;
  }
  nshell->cmdexec--;
  return err;
}

int
cmdexecute(struct nshell *nshell, const char *cline)
/* return
     -2: unexpected eof detected
     -1: fatal error
      0: no error
      1: eof detected
      2: syntax error
      3: runtime error  */
{
  struct cmdlist *cmdroot,*cmdcur,*cmdnew;
  struct cmdstack *stroot;
  int needcmd,rcode;
  int istr;

  istr=0;
  nshell->quit=FALSE;
  while (TRUE) {
    needcmd=FALSE;
    cmdroot=NULL;
    cmdcur=cmdroot;
    cmdnew=NULL;
    stroot=NULL;

    if (nshell->deleted) {
      rcode = -1;
      break;
    }

    do {
      if ((rcode=getcmdline(nshell,&cmdnew,cmdroot,cline,&istr))!=0) break;
      if ((rcode=checkcmd(nshell,&cmdnew))!=0) break;
      if ((rcode=syntax(nshell,cmdnew,&needcmd,&stroot))!=0) break;
      if (cmdnew!=NULL) {
        if (cmdcur==NULL) cmdroot=cmdnew;
        else cmdcur->next=cmdnew;
        cmdcur=cmdnew;
        while (cmdcur->next!=NULL) cmdcur=cmdcur->next;
        cmdnew=NULL;
      }
    } while ((stroot!=NULL) || (needcmd));
    if (rcode!=0) {
      cmdfree(cmdnew);
      cmdfree(cmdroot);
      cmdstackfree(stroot);
      if (nshell->quit) rcode=0;
      break;
    } else if (cmdroot!=NULL) {
      rcode=cmdexec(nshell,cmdroot,FALSE);
      cmdfree(cmdroot);
      if (nshell->quit) {
        rcode=0;
        break;
      }
      if (rcode!=0) break;
    }
  }
  nshell->quit=FALSE;
  return rcode;
}

void
setshhandle(struct nshell *nshell,int fd)
{
  nshell->fd=fd;
  nshell->readbyte=0;
  nshell->readpo=0;
}

static int
storeshhandle(struct nshell *nshell,int fd,
                     char **readbuf,int *readbyte,int *readpo)
{
  int sfd;

  sfd=nshell->fd;
  *readbuf=nshell->readbuf;
  *readbyte=nshell->readbyte;
  *readpo=nshell->readpo;
  nshell->readbuf=g_malloc(SHELLBUFSIZE);
  nshell->readbyte=0;
  nshell->readpo=0;
  nshell->fd=fd;
  return sfd;
}

static void
restoreshhandle(struct nshell *nshell,int fd,
                     char *readbuf,int readbyte,int readpo)
{
  g_free(nshell->readbuf);
  nshell->fd=fd;
  nshell->readbuf=readbuf;
  nshell->readbyte=readbyte;
  nshell->readpo=readpo;
}


int
getshhandle(struct nshell *nshell)
{
  return nshell->fd;
}

struct nshell *
newshell(void)
{
  struct nshell *nshell;
  char **env,*tok;
  int i,len;

  nshell = g_malloc(sizeof(struct nshell));
  if (nshell == NULL)
    return NULL;

#if USE_HASH
  nshell->valroot = nhash_new();
  nshell->exproot = nhash_new();
#else
  nshell->valroot = NULL;
  nshell->exproot = NULL;
#endif

  env = MainEnviron;

  i=0;
  while (env && env[i]) {
    char *name;
    tok=env[i];
    name=getitok2(&tok,&len,"=");
    if (tok[0]=='=') tok++;
    if (addval(nshell,name,tok)==NULL) {
      g_free(name);
      delshell(nshell);
      return NULL;
    }
    g_free(name);
    i++;
  }

  if ((getval(nshell,"PATH")==NULL) && (g_getenv("Path")!=NULL)) {
    const char *path;
    char *tmp;

    path = g_getenv("Path");
    tmp = g_strdup(path);

    if (tmp) {
      addval(nshell,"PATH", tmp);
      g_free(tmp);
    }
  }
  nshell->argc=0;
  nshell->argv=NULL;
  nshell->cmdexec=0;
  nshell->status=0;
  nshell->quit=0;
  nshell->fd=stdinfd();
  nshell->options=TRUE;
  nshell->optionf=FALSE;
  nshell->optione=TRUE;
  nshell->optionv=FALSE;
  nshell->optionx=FALSE;
  nshell->readbuf=g_malloc(SHELLBUFSIZE);
  nshell->readbyte=0;
  nshell->readpo=0;
  nshell->deleted = 0;

  return nshell;
}

static int
del_vallist(struct nhash *h, void *data)
{
  struct vallist *val;

  val = (struct vallist *) h->val.p;
  free_vallist(val);

  return 0;
}

void
delshell(struct nshell *nshell)
{
#if ! USE_HASH
  struct vallist *valcur, *valdel;
  struct explist *expcur,*expdel;
#endif

  if (nshell==NULL) return;

#if USE_HASH
  nhash_each(nshell->valroot, del_vallist, NULL);
  nhash_free(nshell->valroot);
#else
  valcur=nshell->valroot;
  while (valcur!=NULL) {
    g_free(valcur->name);
    if (valcur->func) cmdfree(valcur->val);
    else g_free(valcur->val);
    valdel=valcur;
    valcur=valcur->next;
    g_free(valdel);
  }
#endif

#if USE_HASH
  nhash_free(nshell->exproot);
#else
  expcur=nshell->exproot;
  while (expcur!=NULL) {
    g_free(expcur->val);
    expdel=expcur;
    expcur=expcur->next;
    g_free(expdel);
  }
#endif
  arg_del(nshell->argv);
  g_free(nshell->readbuf);
  g_free(nshell);
  return;
}

void
sherror(int code)
{
  printfstderr("shell: %.64s\n",cmderrorlist[code-100]);
}

void
sherror2(int code,char *mes)
{
  if (mes!=NULL) {
    printfstderr("shell: %.64s `%.64s'.\n",cmderrorlist[code-100],mes);
  } else {
    printfstderr("shell: %.64s.\n",cmderrorlist[code-100]);
  }
}

void
sherror3(char *cmd,int code,char *mes)
{
  cmd = CHK_STR(cmd);
  if (mes!=NULL) {
    printfstderr("shell: %.64s: %.64s `%.64s'.\n",
                   cmd,cmderrorlist[code-100],mes);
  } else {
    printfstderr("shell: %.64s: %.64s.\n",cmd,cmderrorlist[code-100]);
  }
}

void
sherror4(char *cmd,int code)
{
  cmd = CHK_STR(cmd);
  printfstderr("shell: %.64s: %.64s\n",cmd,cmderrorlist[code-100]);
}

void
shellsavestdio(struct nshell *nshell)
{
  nshell->sgetstdin=getstdin;
  nshell->sputstdout=putstdout;
  nshell->sprintfstdout=printfstdout;
  getstdin=shgetstdin;
  putstdout=shputstdout;
  printfstdout=shprintfstdout;
}

void
shellrestorestdio(const struct nshell *nshell)
{
  getstdin=nshell->sgetstdin;
  putstdout=nshell->sputstdout;
  printfstdout=nshell->sprintfstdout;
}

int
setshelloption(struct nshell *nshell,char *opt)
{
  int flag;

  if ((opt[0]!='-') && (opt[0]!='+')) return 1;
  if (opt[0]=='-') flag=TRUE;
  else flag=FALSE;
  switch (opt[1]) {
  case 's':
    nshell->options=flag;
    break;
  case 'e':
    nshell->optione=flag;
    break;
  case 'v':
    nshell->optionv=flag;
    break;
  case 'x':
    nshell->optionx=flag;
    break;
  case 'f':
    nshell->optionf=flag;
    break;
  default:
    sherror2(ERRILOPS,opt);
    return -1;
  }
  return 0;
}

int
getshelloption(struct nshell *nshell,char opt)
{
  switch (opt) {
  case 's':
    return nshell->options;
  case 'e':
    return nshell->optione;
  case 'v':
    return nshell->optionv;
  case 'x':
    return nshell->optionx;
  case 'f':
    return nshell->optionf;
  default:
    break;
  }
  return 0;
}

int
set_shell_args(struct nshell *nshell, int j, const char *argv0, int argc, char **argv)
{
  char *s, **argv2;
  int argc2;

  argv2 = NULL;
  s = g_strdup(argv0);
  if (s == NULL) {
    return ERRMEMORY;
  }

  if (arg_add(&argv2, s) == NULL) {
    g_free(s);
    arg_del(argv2);
    return ERRMEMORY;
  }

  for (; j < argc; j++) {
    if (argv[j]) {
      s = g_strdup(argv[j]);
      if (s == NULL) {
	return ERRMEMORY;
      }

      if (arg_add(&argv2, s) == NULL) {
	g_free(s);
	arg_del(argv2);
	return ERRMEMORY;
      }
    }
  }
  argc2 = getargc(argv2);
  arg_del(nshell->argv);
  nshell->argv = argv2;
  nshell->argc = argc2;

  return 0;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
void
setshellargument(struct nshell *nshell,int argc,char **argv)
{
  arg_del(nshell->argv);
  nshell->argc=argc;
  nshell->argv=argv;
}
#endif  /* COMPILE_UNUSED_FUNCTIONS */

void
ngraphenvironment(struct nshell *nshell)
{
  char *sver,*lib,*home,*conf,*data,*addin;
  struct objlist *sobj;
  char *systemname;

  sobj=chkobject("system");
  getobj(sobj,"name",0,0,NULL,&systemname);
  getobj(sobj,"conf_dir",0,0,NULL,&conf);
  getobj(sobj,"data_dir",0,0,NULL,&data);
  getobj(sobj,"lib_dir",0,0,NULL,&lib);
  getobj(sobj,"home_dir",0,0,NULL,&home);
  sver=getobjver("system");
  addval(nshell,"NGRAPH",systemname);
  addval(nshell,"VERSION",sver);
  addval(nshell,"NGRAPHCONF",conf);
  addval(nshell,"NGRAPHLIB",lib);
  addval(nshell,"NGRAPHHOME",home);
  if (getval(nshell,"PS1")==NULL) addval(nshell,"PS1","Ngraph$ ");
  if (getval(nshell,"PS2")==NULL) addval(nshell,"PS2",">");
  if (getval(nshell,"IFS")==NULL) addval(nshell,"IFS"," \t\n");
  if (getval(nshell,"IGNOREEOF")==NULL) addval(nshell,"IGNOREEOF","10");

  addin = g_strdup_printf("%s/addin", data);
  if (addin) {
    char *pathset;
    pathset = g_strdup_printf("PATH='%s%s%s%s%s%s%s%s'$PATH", home, PATHSEP, addin, PATHSEP, lib, PATHSEP, ".", PATHSEP);
    if (pathset) {
#if WINDOWS
      path_to_win(pathset);
#endif	/* WINDOWS */
      cmdexecute(nshell,pathset);
      g_free(pathset);
    }
    g_free(addin);
  }
}

int
str_calc(const char *str, double *val, int *r, char **err_msg)
{
  int ecode, rcode;
  static MathEquation *eq = NULL;
  MathValue value = {0, 0};

  if (r) {
    *r = MATH_VALUE_ERROR;
  }

  if (err_msg) {
    *err_msg = NULL;
  }

  if (str == NULL || val == NULL) {
    return ERRMILLEGAL;
  }

  *val = 0;

  if (eq == NULL) {
    eq = math_equation_basic_new();
    if (eq == NULL) {
      return ERRMEMORY;
    }
  }

  ecode = math_equation_parse(eq, str);
  if (ecode) {
    if (err_msg) {
      *err_msg = math_err_get_error_message(eq, str, ecode);
    }
    return ERRMSYNTAX;
  }

  rcode = math_equation_calculate(eq, &value);
  ecode = (rcode) ? ERRMFAT : 0;

  *val = value.val;

  if (value.type == MATH_VALUE_NAN || value.type == MATH_VALUE_UNDEF) {
    rcode = value.type;
  }

  if (ecode) {
    return ecode;
  }

  if (r) {
    *r = rcode;
  }

  return 0;
}

#if WINDOWS
void
show_system_error(void)
{
  LPVOID *msg;
  int r;

  r = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR) &msg,
                    0,
                    NULL);

  if (r) {
    shputstderr((char *)msg);
    LocalFree(msg);
  }
}

static void *
system_in_thread(void *ptr)
{
  char *cmd;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  int r;

  if (ptr == NULL) {
    return NULL;
  }

  cmd = (char *) ptr;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  r = CreateProcess(NULL, cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
  if (r == 0) {
    show_system_error();
    return NULL;
  }

  CloseHandle(pi.hThread);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  g_free(cmd);

  return NULL;
}
#endif	/* WINDOWS */

int
system_bg(char *cmd)
{
#if WINDOWS
  GThread *thread;
  char *ptr;

  if (cmd == NULL)
    return 1;

  ptr = g_locale_from_utf8(cmd, -1, NULL, NULL, NULL);

  thread= g_thread_new("process", system_in_thread, ptr);
  if (thread == NULL) {
    return 1;
  }
  g_thread_unref(thread);

  return 0;
#else  /* WINDOWS */
  if (cmd == NULL)
    return 1;

  return ! g_spawn_command_line_async(cmd, NULL);
#endif	/* WINDOWS */
}
