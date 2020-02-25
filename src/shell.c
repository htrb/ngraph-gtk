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
  while (th.sleep) {
    eventloop();
    msleep(1);
  }
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

static int
shgetstdin(void)
{
  char buf[2];
  int byte;

  if (nisatty(stdinfd())) {
    set_shellevloop(0);
    do {
      byte=read(stdinfd(),buf,1);
    } while (byte<0);
    reset_shellevloop();
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
    set_shellevloop(0);
    do {
      byte=read(nshell->fd,buf,1);
    } while (byte<0);
    reset_shellevloop();
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

  nshell = newshell();
  if (nshell == NULL)
    return 1;

  set_security(security);
  ngraphenvironment(nshell);
  cmdexecute(nshell, script);
  set_security(FALSE);
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
      if (!*quote
      || ((*quote=='"') && (strchr("\"\\'$",po[i+1])!=NULL))
      || *bquote ) *escape=TRUE;
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
      if (!quote
      || ((quote=='"') && (strchr("\"\\'$",po[i+1])!=NULL))) escape=TRUE;
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

static char *
expand(struct nshell *nshell,char *str,int *quote,int *bquote, int ifsexp)
{
  char *po;
  int quote2,bquote2,escape;
  int i,j,k,num;
  unsigned int u, n;
  char *c1,*c2,*name,*val,ch,valf,valf2,*ifs;
  char *s,*sb,*se;
  int sout,sout2;
  int rcode,dummy;
  int byte;
  char *tmpfil;
  struct objlist *sys;
  struct narray *sarray;
  char **sdata;
  int snum;
  char *tmp;

  if ((po=str)==NULL) return NULL;
  if ((s=nstrnew())==NULL) return NULL;
  name=NULL;
  sb=se=NULL;
  escape=FALSE;
  ifs=getval(nshell,"IFS");

  for (i=0;po[i]!='\0';i++) {
    /* check escapse */
    if (escape) {
      if (se==NULL) {
        if (sb!=NULL) {
          if ((sb=nstrccat(sb,po[i]))==NULL) goto errexit;
        } else {
          if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
        }
      }
      escape=FALSE;
    } else if (po[i]=='\\') {
      if (!*quote
      || ((*quote=='"') && (strchr("\"\\'$",po[i+1])!=NULL))
      || *bquote ) {
        escape=TRUE;
        if ((se==NULL) && (sb==NULL)) {
          if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
        }
      } else if (se==NULL) {
        if (sb!=NULL) {
          if ((sb=nstrccat(sb,po[i]))==NULL) goto errexit;
        } else {
          if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
        }
      }
    } else if (po[i]==(char )0x02) {
      escape=TRUE;
      if ((se==NULL) && (sb==NULL)) {
        if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
      }
    /* check back quote */
    } else if (*bquote) {
      if (po[i]=='`') {
        *bquote='\0';
        if (se==NULL) {
          /* command substitution */
          if ((sys=getobject("system"))==NULL) goto errexit; /* fix-me: assigned value of the variable "sys" is never used */
	  sout = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &tmpfil);
	  if (sout < 0)
	    goto errexit;
          sout2=nredirect(1,sout);
          rcode=cmdexecute(nshell,sb);
          if ((rcode!=0) && (rcode!=1)) {
            nredirect2(1,sout2);
            g_unlink(tmpfil);
            g_free(tmpfil);
            goto errexit;
          }
          g_free(sb);
          if ((sb=nstrnew())==NULL) {
            nredirect2(1,sout2);
            g_unlink(tmpfil);
            g_free(tmpfil);
            goto errexit;
          }
          nlseek(stdoutfd(),0L,SEEK_SET);
          while ((byte=nread(stdoutfd(),writebuf,WRITEBUFSIZE-1))>0) {
            writebuf[byte]='\0';
            if ((sb=nstrcat(sb,writebuf))==NULL) {
              nredirect2(1,sout2);
              g_unlink(tmpfil);
              g_free(tmpfil);
              goto errexit;
            }
          }
          nredirect2(1,sout2);
          g_unlink(tmpfil);
          if (byte==-1) {
            sherror2(ERRREAD,tmpfil);
            g_free(tmpfil);
            goto errexit;
          }
          g_free(tmpfil);
	  n = strlen(sb);
          for (u = 0; u < n; u++) {
	    if (sb[u] == '\n') {
	      sb[u] = ' ';
	    }
	  }
          for (j=strlen(sb)-1;(j>=0) && (sb[j]==' ');j--);
          sb[j+1]='\0';
          if ((c1=quotation(nshell,sb,*quote))==NULL) goto errexit;
          s=nstrcat(s,c1);
          g_free(c1);
          if (s==NULL) goto errexit;
          g_free(sb);
          sb=NULL;
        }
      } else if (se==NULL) {
        if (sb!=NULL) {
          if ((sb=nstrccat(sb,po[i]))==NULL) goto errexit;
        } else {
          if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
        }
      }
    } else if ((po[i]=='`') && (*quote!='\'')) {
      *bquote='`';
      if (se==NULL) {
        if ((sb=nstrnew())==NULL) goto errexit;
      }
    /* check constant variable */
    } else if ((po[i]=='$') && (po[i+1]!='\0')
    && (*quote!='\'') && (se==NULL)) {
      if (isdigit(po[i+1])) {
        for (j=i+1;(po[j]!='\0') && isdigit(po[j]);j++);
        g_free(name);
        if ((name=g_malloc(j-i))==NULL) goto errexit;
        strncpy(name,po+i+1,j-i-1);
        name[j-i-1]='\0';
        num=atoi(name);
        if (num<nshell->argc) {
          if ((c1=quotation(nshell,(nshell->argv)[num],*quote))==NULL)
            goto errexit;
          s=nstrcat(s,c1);
          g_free(c1);
          if (s==NULL) goto errexit;
        }
        i=j-1;
      } else if (strchr("#?",po[i+1])!=NULL) {
        switch (po[i+1]) {
        case '#': g_free(name);
                  if ((name=g_malloc(12))==NULL) goto errexit;
                  sprintf(name,"%d",nshell->argc-1);
                  break;
        case '?': g_free(name);
                  if ((name=g_malloc(12))==NULL) goto errexit;
                  sprintf(name,"%d",nshell->status);
                  break;
        }
        if ((c1=quotation(nshell,name,*quote))==NULL) goto errexit;
        s=nstrcat(s,c1);
        g_free(c1);
        if (s==NULL) goto errexit;
        i++;
      } else if (strchr("*@",po[i+1])!=NULL) {
        val=getval(nshell,"IFS");
        if (val==NULL) ch='\0';
        else ch=val[0];
        for (j=1;j<nshell->argc;j++) {
          if ((c1=quotation(nshell,(nshell->argv)[j],*quote))==NULL)
            goto errexit;
          s=nstrcat(s,c1);
          g_free(c1);
          if (s==NULL) goto errexit;
          if (j!=(nshell->argc-1)) {
            if ((*quote=='"') && (po[i+1]=='*')) {
              if (ch!='\0') {
                if (strchr("\"\\'$",ch)!=NULL) {
                  if ((s=nstrccat(s,'\\'))==NULL) goto errexit;
                }
                if ((s=nstrccat(s,ch))==NULL) goto errexit;
              }
            } else {
              if ((s=nstrccat(s,(char )0x01))==NULL) goto errexit;
            }
          }
        }
        i++;
      /* check variable */
      } else if (po[i+1]=='{') {
        for (j=i+2;(po[j]!='\0') && (isalnum(po[j]) || (po[j]=='_'));j++);
        if ((po[j]=='\0') || (strchr(":-=?+#%}",po[j])==NULL)
                          || (po[j-1]=='{')) {
          sherror(ERRBADSUB);
          goto errexit;
        }
        if (po[j]=='}') {
          g_free(name);
          if ((name=g_malloc(j-i-1))==NULL) goto errexit;
          strncpy(name,po+i+2,j-i-2);
          name[j-i-2]='\0';
          val=getval(nshell,name);
          if (val!=NULL) {
            if ((c1=quotation(nshell,val,*quote))==NULL) goto errexit;
            s=nstrcat(s,c1);
            g_free(c1);
            if (s==NULL) goto errexit;
          }
          i=j;
        } else {
          if ((po[j]==':')
          && (po[j+1]!='\0') && (strchr("-=?+",po[j+1])==NULL)) {
            valf='o';
            if ((se=nstrnew())==NULL) goto errexit;
            i+=2;
          } else {
            g_free(name);
            if ((name=g_malloc(j-i-1))==NULL) goto errexit;
            strncpy(name,po+i+2,j-i-2);
            name[j-i-2]='\0';
            val=getval(nshell,name);
            quote2=*quote;
            bquote2=*bquote;
            valf2=' ';
            if (po[j]==':') {
              valf2=':';
              j++;
            }
	    if (strchr("-=?+",po[j])!=NULL) {
              valf=po[j];
              j++;
            } else if (strchr("#%",po[j])!=NULL) {
              if (po[j]==po[j+1]) {
                valf2=po[j];
                j++;
              }
              valf=po[j];
              j++;
            } else {
              sherror(ERRBADSUB);
              goto errexit;
            }
            if ((se=nstrnew())==NULL) goto errexit;
            i=j;
          }
        }
      } else {
        /* simple variable substitution */
        for (j=i+1;(po[j]!='\0') && (isalnum(po[j]) || (po[j]=='_'));j++);
        g_free(name);
        if ((name=g_malloc(j-i))==NULL) goto errexit;
        strncpy(name,po+i+1,j-i-1);
        name[j-i-1]='\0';
        if (name[0]=='\0') val="$";
        else val=getval(nshell,name);
        if (val!=NULL) {
          if ((c1=quotation(nshell,val,*quote))==NULL) goto errexit;
          s=nstrcat(s,c1);
          g_free(c1);
          if (s==NULL) goto errexit;
        }
        i=j-1;
      }
    /* check variable end */
    } else if ((po[i]=='}') && (*quote!='\'') && (se!=NULL)) {
      if (valf=='o') {
      /* object replacement */
        if ((tmp=unquotation(se,&dummy))==NULL) goto errexit;
        g_free(se);
        se=NULL;
        sarray=sgetobj(tmp,FALSE,FALSE,FALSE);
        g_free(tmp);
        sdata=arraydata(sarray);
        snum=arraynum(sarray);
        for (j=0;j<snum;j++) {
          if ((c1=quotation(nshell,sdata[j],*quote))==NULL) {
            arrayfree2(sarray);
            goto errexit;
          }
          s=nstrcat(s,c1);
          g_free(c1);
          if (s==NULL) {
            arrayfree2(sarray);
            goto errexit;
          }
        }
        arrayfree2(sarray);
      /* variable substitution */
      } else {
        c1=NULL;
        switch (valf) {
        case '-':
          if ((val!=NULL) && ((valf2!=':') || (val[0]!='\0'))) {
            if ((c1=quotation(nshell,val,quote2))==NULL) goto errexit;
	  } else {
            if ((c1=expand(nshell,se,&quote2,&bquote2,TRUE))==NULL)
              goto errexit;
          }
          break;
        case '=':
          if ((val!=NULL) && ((valf2!=':') || (val[0]!='\0'))) {
            if ((c1=quotation(nshell,val,quote2))==NULL) goto errexit;
          } else {
            if ((c1=expand(nshell,se,&quote2,&bquote2,TRUE))==NULL)
              goto errexit;
            g_free(se);
            if ((se=unquotation(c1,&dummy))==NULL) goto errexit;
            if (addval(nshell,name,se)==NULL) goto errexit;
          }
          break;
        case '?':
          if ((val!=NULL) && ((valf2!=':') || (val[0]!='\0'))) {
            if ((c1=quotation(nshell,val,quote2))==NULL) goto errexit;
          } else {
            if ((c1=expand(nshell,se,&quote2,&bquote2,TRUE))==NULL)
              goto errexit;
            printfstdout("%.256s: %.256s\n",name,c1);
            goto errexit;
          }
          break;
        case '+':
          if ((val!=NULL) && ((valf2!=':') || (val[0]!='\0'))) {
            if ((c1=expand(nshell,se,&quote2,&bquote2,TRUE))==NULL)
              goto errexit;
          }
          break;
        case '#':
          if ((val!=NULL) && (val[0]!='\0')) {
            if ((c2=nstrnew())==NULL) goto errexit;
            if ((c2=nstrcat(c2,val))==NULL) goto errexit;
            if ((c1=expand(nshell,se,&quote2,&bquote2,TRUE))==NULL) {
              g_free(c2);
              goto errexit;
            }
            if (valf2=='#') {
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
	    }
            g_free(c1);
            c1=quotation(nshell,c2+k,quote2);
            g_free(c2);
            if (c1==NULL) goto errexit;
          }
          break;
        case '%':
          if ((val!=NULL) && (val[0]!='\0')) {
            if ((c2=nstrnew())==NULL) goto errexit;
            if ((c2=nstrcat(c2,val))==NULL) goto errexit;
            if ((c1=expand(nshell,se,&quote2,&bquote2,TRUE))==NULL) {
              g_free(c2);
              goto errexit;
            }
            if (valf2=='%') {
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
            c1=quotation(nshell,c2,quote2);
            g_free(c2);
            if (c1==NULL) goto errexit;
          }
          break;
        }
        if (c1!=NULL) {
          s=nstrcat(s,c1);
          g_free(c1);
          if (s==NULL) goto errexit;
        }
        *quote=quote2;
        *bquote=bquote2;
        g_free(se);
        se=NULL;
      }
    /* check quotation */
    } else if ((*quote=='"') || (*quote=='\'')) {
      if (po[i]==*quote) *quote='\0';
      if (se==NULL) {
        if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
      }
    } else if (po[i]=='\'' || po[i]=='"') {
      *quote=po[i];
      if (se==NULL) {
        if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
      }
    } else {
      if (se==NULL) {
        if (ifsexp && (ifs!=NULL) && (ifs[0]!='\0')
        && (strchr(ifs,po[i])!=NULL)) {
          if ((s=nstrccat(s,(char )0x01))==NULL) goto errexit;
        } else {
          if ((s=nstrccat(s,po[i]))==NULL) goto errexit;
        }
      }
    }
    if (se!=NULL) {
      if ((se=nstrccat(se,po[i]))==NULL) goto errexit;
    }
  }

  if ((se!=NULL) || (sb!=NULL)) {
    sherror(ERRBADSUB);
    goto errexit;
  }

  g_free(name);
  g_free(sb);
  g_free(se);
  return s;

errexit:
  g_free(name);
  g_free(sb);
  g_free(se);
  g_free(s);
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
                struct objlist *sys;
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

                if ((sys=getobject("system"))==NULL) return -1; /* fix-me: assigned value of the variable "sys" is never used */
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
            for (i=0;(po[i]!='\0') && (isalnum(po[i]) || (po[i]=='_'));i++);
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

int
cmdexec(struct nshell *nshell,struct cmdlist *cmdroot,int namedfunc)
{
  struct cmdlist *cmdcur,*cmdnew,*cmd;
  struct prmlist *prmcur,*prmprev,*prm,*prmnewroot;
  struct vallist *newvalroot;
  int err,quote,bquote,rcode,needcmd;
  char *str,*name,*val,*po,*cmdname;
  int i,j,num,pnum,errlevel,a,looplevel,len;
  char *arg,*endptr,**env;
  struct cmdstack *stroot,*stcur,*st;
  char *fstdout,*fstdin;
  int istdout,istdin;
  int sout,sout2,sin,sin2,sin3,fd;
  int lastc;
  int pipef;
  struct objlist *sys;
  char *tmpfil,*tmpfil2;
  char *cmds;
  char *s;
  char **argv,**newenviron;
  char **argvsave,**argvnew;
  int quoted;
  int argcsave,argcnew;
  int iftrue,casetrue;
  char *pat;
  char *readbuf;
  int readpo;
  int readbyte;
  int ch;
  char buf[2];
  shell_proc proc;

#if ! WINDOWS
  pid_t pid;
#endif	/* WINDOWS */

  reset_interrupt();
  nshell->cmdexec++;
  err=-1;
  stroot=NULL;
  prmnewroot=NULL;
  fstdout=fstdin=NULL;
  env=NULL;
  newenviron=NULL;
  newvalroot=NULL;
  cmdname=NULL;
  sout=sout2=sin=sin2=NOHANDLE;
  tmpfil2=NULL;
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
  cmdcur=cmdroot;
  while ((cmdcur!=NULL) && (!(nshell->quit))) {
    if (ninterrupt()) goto errexit;
    stcur=cmdstackgetpo(&stroot);
    if ((stcur!=NULL) && (stcur->cmdno==CPBI) && (stcur->cmd!=cmdcur)) {
      if (addfunc(nshell,stcur->val,cmdcur)==NULL) goto errexit;
      cmdcur=cmdcur->next;
    } else {
      switch (cmdcur->cmdno) {
      case CPNULL:
	cmdcur=cmdcur->next;
	break;
      case CPIF:
	if ((st=cmdstackcat(&stroot,CPIF))==NULL) goto errexit;
	st->iftrue=FALSE;
	cmdcur=cmdcur->next;
	break;
      case CPTHEN:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	iftrue=st->iftrue;
	if (iftrue || nshell->status) cmdcur=cmdcur->done;
	else {
	  iftrue=TRUE;
	  cmdcur=cmdcur->next;
	}
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPTHEN))==NULL) goto errexit;
	st->iftrue=iftrue;
	break;
      case CPELIF:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	iftrue=st->iftrue;
	if (iftrue) cmdcur=cmdcur->done;
	else cmdcur=cmdcur->next;
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPIF))==NULL) goto errexit;
	st->iftrue=iftrue;
	break;
      case CPELSE:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	iftrue=st->iftrue;
	if (iftrue) cmdcur=cmdcur->done;
	else cmdcur=cmdcur->next;
	cmdstackrmlast(&stroot);
	if ((st=cmdstackcat(&stroot,CPELSE))==NULL) goto errexit;
	st->iftrue=iftrue;
	break;
      case CPFI:
	cmdstackrmlast(&stroot);
	cmdcur=cmdcur->next;
	break;
      case CPCASE:
	if ((st=cmdstackcat(&stroot,CPCASE))==NULL) return -1;
	st->casetrue=FALSE;
	prmcur=(cmdcur->prm)->next;
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
	cmdcur=cmdcur->next;
	break;
      case CPPATI:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	casetrue=st->casetrue;
	pat=st->pat;
	st->pat = NULL;		/* hito (mem leak of pat) */
	if (casetrue) cmdcur=cmdcur->done;
	else {
	  prm=cmdcur->prm;
	  while (prm!=NULL) {
	    if (wildmatch(prm->str,pat,0)) break;
	    prm=prm->next;
	    prm=prm->next;
	  }
	  if (prm!=NULL) {
	    casetrue=TRUE;
	    cmdcur=cmdcur->next;
	  } else cmdcur=cmdcur->done;
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
	cmdcur=cmdcur->done;
	break;
      case CPESAC:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	casetrue=st->casetrue;
	pat=st->pat;
	st->pat = NULL;		/* hito (mem leak of pat) */
	g_free(pat);
	cmdstackrmlast(&stroot);
	cmdcur=cmdcur->next;
	break;
      case CPFOR:
	if ((stcur=cmdstackcat(&stroot,CPFOR))==NULL) goto errexit;
	stcur->cmd=cmdcur;
	prmcur=(cmdcur->prm)->next;
	stcur->val=prmcur->str;
	prmcur=prmcur->next;
	if (prmcur==NULL) stcur->ival=0;
	else {
	  stcur->ival=-1;
	  prmnewroot=NULL;
	  prmprev=NULL;
	  prmcur=prmcur->next;
	  while (prmcur!=NULL) {
	    if ((prm=g_malloc(sizeof(struct prmlist)))==NULL) goto errexit;
	    if (prmprev==NULL) prmnewroot=prm;
	    else prmprev->next=prm;
	    prm->next=NULL;
	    prm->str=NULL;
	    prm->prmno=prmcur->prmno;
	    quote='\0';
	    bquote='\0';
	    if ((str=expand(nshell,prmcur->str,&quote,&bquote,FALSE))==NULL)
	      goto errexit;
	    if (quote || bquote) {
	      sherror(ERRUEXPEOF);
	      goto errexit;
	    }
	    if (str[0]!='\0') {
	      if ((prm->str=fnexpand(nshell,str))==NULL) goto errexit;
	    } else {
	      g_free(str);
	      prm->str=NULL;
	    }
	    if ((num=wordsplit(prm))==-1) goto errexit;
	    for (j=0;j<num;j++) {
	      prmprev=prm;
	      prm=prm->next;
	    }
	    prmcur=prmcur->next;
	  }
	  /* remove null parameter and command end */
	  prmcur=prmnewroot;
	  prmprev=NULL;
	  while (prmcur!=NULL) {
	    if ((prmcur->str==NULL) || (prmcur->prmno==PPEND)) {
	      if (prmprev==NULL) prmnewroot=prmcur->next;
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
	  stcur->prm=prmnewroot;
	  prmnewroot=NULL;
	}
	cmdcur=cmdcur->next;
	break;
      case CPWHILE:
	if ((stcur=cmdstackcat(&stroot,CPWHILE))==NULL) goto errexit;
	stcur->cmd=cmdcur;
	cmdcur=cmdcur->next;
	break;
      case CPUNTIL:
	if ((stcur=cmdstackcat(&stroot,CPUNTIL))==NULL) goto errexit;
	stcur->cmd=cmdcur;
	cmdcur=cmdcur->next;
	break;
      case CPDO:
	if ((stcur=cmdstackgetpo(&stroot))==NULL) goto errexit;
	if (stcur->cmdno==CPFOR) {
	  if (stcur->ival==-1) {
	    prmcur=stcur->prm;
	    if (prmcur==NULL) {
	      cmdcur=cmdcur->done;
	      cmdstackrmlast(&stroot);
	      cmdcur=cmdcur->next;
	      break;
	    }
	    if (addval(nshell,stcur->val,prmcur->str)==NULL) goto errexit;
	    stcur->prm=prmcur->next;
	    g_free(prmcur->str);
	    g_free(prmcur);
	  } else {
	    stcur->ival++;
	    if (stcur->ival<nshell->argc) {
	      if (addval(nshell,stcur->val,nshell->argv[stcur->ival])==NULL)
                goto errexit;
	    } else {
	      cmdcur=cmdcur->done;
	      cmdstackrmlast(&stroot);
	      cmdcur=cmdcur->next;
	      break;
	    }
	  }
	} else if (stcur->cmdno==CPWHILE) {
	  if (nshell->status) {
	    cmdcur=cmdcur->done;
	    cmdstackrmlast(&stroot);
	    cmdcur=cmdcur->next;
	    break;
	  }
	} else if (stcur->cmdno==CPUNTIL) {
	  if (!nshell->status) {
	    cmdcur=cmdcur->done;
	    cmdstackrmlast(&stroot);
	    cmdcur=cmdcur->next;
	    break;
	  }
	}
	cmdcur=cmdcur->next;
	break;
      case CPDONE:
	if ((stcur=cmdstackgetpo(&stroot))==NULL) goto errexit;
	cmdcur=stcur->cmd;
	cmdcur=cmdcur->next;
	break;
      case CPFN:
	if ((stcur=cmdstackcat(&stroot,CPFN))==NULL) goto errexit;
	prmcur=cmdcur->prm;
	stcur->val=prmcur->str;
	if (newfunc(nshell,prmcur->str)==NULL) goto errexit;
	cmdcur=cmdcur->next;
	break;
      case CPBI:
	if ((st=cmdstackgetpo(&stroot))==NULL) goto errexit;
	if ((stcur=cmdstackcat(&stroot,CPBI))==NULL) goto errexit;
	stcur->val=st->val;
	stcur->cmd=cmdcur->done;
	cmdcur=cmdcur->next;
	break;
      case CPBO:
	cmdstackrmlast(&stroot);
	cmdstackrmlast(&stroot);
	cmdcur=cmdcur->next;
	break;

      case CPNO:

	prmnewroot=NULL;
	prmprev=NULL;
	prmcur=cmdcur->prm;
	while (prmcur!=NULL) {
	  if (prmcur->prmno!=PPSI2) {
	    quote='\0';
	    bquote='\0';
	    if ((str=expand(nshell,prmcur->str,&quote,&bquote,FALSE))==NULL)
	      goto errexit;
	    if (quote || bquote) {
	      g_free(str);
	      sherror(ERRUEXPEOF);
	      goto errexit;
	    }
	    prmcur->quoted=FALSE;
	  } else {
	    if ((str=g_malloc(strlen(prmcur->str)+1))==NULL) goto errexit;
	    strcpy(str,prmcur->str);
	  }
	  if ((prmcur->prmno!=PPNO) || (str[0]!='\0')) {
	    if ((prm=g_malloc(sizeof(struct prmlist)))==NULL) goto errexit;
	    if (prmprev==NULL) prmnewroot=prm;
	    else prmprev->next=prm;
	    prm->next=NULL;
	    prm->prmno=prmcur->prmno;
	    prm->quoted=prmcur->quoted;
	    if (prmcur->prmno!=PPSI2) {
	      if ((prm->str=fnexpand(nshell,str))==NULL) goto errexit;
	      if (prm->prmno==PPNO) {
		if ((num=wordsplit(prm))==-1) goto errexit;
		for (j=0;j<num;j++) {
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

	/* remove null parameter & check redirect and pipe */
	istdout=istdin=PPNO;
	pipef=FALSE;
	prmcur=prmnewroot;
	prmprev=NULL;
	pnum=0;
	while (prmcur!=NULL) {
	  if (prmcur->str==NULL) {
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm->str);
	    g_free(prm);
	  } else if ((prmcur->prmno==PPSI1) || (prmcur->prmno==PPSI2)
		     || (prmcur->prmno==PPSO1) || (prmcur->prmno==PPSO2)) {
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm->str);
	    g_free(prm);
	    switch (prmcur->prmno) {
	    case PPSI1:
	      g_free(fstdin);
	      fstdin=prmcur->str;
	      istdin=PPSI1;
	      break;
	    case PPSI2:
	      g_free(fstdin);
	      fstdin=prmcur->str;
	      istdin=PPSI2;
	      quoted=prmcur->quoted;
	      break;
	    case PPSO1:
	      g_free(fstdout);
	      fstdout=prmcur->str;
	      istdout=PPSO1;
	      break;
	    case PPSO2:
	      g_free(fstdout);
	      fstdout=prmcur->str;
	      istdout=PPSO2;
	      break;
	    }
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm);
	  } else if (prmcur->prmno==PPPIPE) {
	    pipef=TRUE;
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm->str);
	    g_free(prm);
	  } else if (prmcur->prmno==PPEND) {
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm->str);
	    g_free(prm);
	  } else if ((prmcur->prmno!=PPSETV) && (prmcur->prmno!=PPSETO)) {
	    pnum++;
	    prmprev=prmcur;
	    prmcur=prmcur->next;
	  } else {
	    prmprev=prmcur;
	    prmcur=prmcur->next;
	  }
	}

	/* set variable */
	prmcur=prmnewroot;
	prmprev=NULL;
	while (prmcur!=NULL) {
	  if (prmcur->prmno==PPSETV) {
	    po=strchr(prmcur->str,'=');
	    if ((name=g_malloc(po-prmcur->str+1))==NULL) goto errexit;
	    strncpy(name,prmcur->str,po-prmcur->str);
	    name[po-prmcur->str]='\0';
	    if ((val=g_malloc(strlen(prmcur->str)-(po-prmcur->str)))==NULL) {
	      g_free(name);
	      goto errexit;
	    }
	    strcpy(val,po+1);
	    if (pnum==0) po=addval(nshell,name,val);
	    else po=saveval(nshell,name,val,&newvalroot);
	    g_free(name);
	    g_free(val);
	    if (po==NULL) goto errexit;
	    g_free(prmcur->str);
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm);
	  } else if (prmcur->prmno==PPSETO) {
	    /* set object */
	    if (sputobj(prmcur->str)==-1) goto errexit;
	    g_free(prmcur->str);
	    if (prmprev==NULL) prmnewroot=prmcur->next;
	    else prmprev->next=prmcur->next;
	    prm=prmcur;
	    prmcur=prmcur->next;
	    g_free(prm);
	  } else {
	    prmprev=prmcur;
	    prmcur=prmcur->next;
	  }
	}

	prmcur=prmnewroot;
	if ((prmcur!=NULL) && (prmcur->prmno==PPNO)) {

	  /* set environment variable */
#if USE_HASH
	  {
	    struct set_env_arg se_arg;

	    se_arg.nshell = nshell;
	    se_arg.newenviron = &newenviron;
	    if (nhash_each(nshell->valroot, set_env_val, &se_arg))
	      goto errexit;
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
	      if ((s=g_malloc(len+strlen(val)+2))==NULL) goto errexit;
	      strcpy(s,valcur->name);
	      s[len]='=';
	      strcpy(s+len+1,val);
	      if (arg_add(&newenviron,s)==NULL) {
		g_free(s);
		goto errexit;
	      }
	    }
	    valcur=valcur->next;
	  }
#endif

	  env=MainEnviron;
	  MainEnviron=newenviron;
	  newenviron=NULL;
	  sout=sin=NOHANDLE;
	  if (istdin!=PPNO) {
	    if (istdin==PPSI1) sin=nopen(fstdin,O_RDONLY,NFMODE);
	    else if (istdin==PPSI2) {
	      sin=nopen(fstdin,O_RDONLY,NFMODE);
	      if (!quoted) {
		fd = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &tmpfil2);
		if (fd < 0) {
		  nclose(sin);
		  goto errexit;
		}
		do {
		  if ((s=nstrnew())==NULL) {
		    nclose(sin);
		    nclose(fd);
		    goto errexit;
		  }
		  do {
		    if (nread(sin,buf,1)>0) ch=buf[0];
		    else ch=EOF;
		    if (ch==EOF) break;
		    else {
		      if (ch=='\n') ch='\0';
		      if ((s=nstrccat(s,ch))==NULL) {
			nclose(sin);
			nclose(fd);
			goto errexit;
		      }
		    }
		  } while (ch!='\0');
		  quote=bquote='\0';
		  str=s;
		  s=expand(nshell,str,&quote,&bquote,FALSE);
		  g_free(str);
		  if (s==NULL) {
		    nclose(sin);
		    nclose(fd);
		    goto errexit;
		  }
		  wordunsplit(s);
		  len=strlen(s);
		  if (ch!=EOF) {
		    s[len]='\n';
		    len++;
		  }
		  nwrite(fd,s,len);
		  g_free(s);
		} while (ch!=EOF);
		nclose(sin);
		nclose(fd);
		sin=nopen(tmpfil2,O_RDONLY,NFMODE);
	      }
	    }
	  } else if (cmdcur->pipefile!=NULL) {
	    sin=nopen(cmdcur->pipefile,O_RDONLY,NFMODE);
	  }
	  if (sin!=NOHANDLE) sin2=nredirect(0,sin);

	  /* redirect : stdout */
	  if (istdout!=PPNO) {
	    if (Security) {
	      sherror(ERRSECURITY);
	      goto errexit;
	    }
	    if (istdout==PPSO2)
	      sout=nopen(fstdout,O_APPEND|O_CREAT|O_WRONLY,NFMODE);
	    else sout=nopen(fstdout,O_CREAT|O_WRONLY|O_TRUNC,NFMODE);
	    /* pipe */
	  } else if (pipef) {
	    if ((sys=getobject("system"))==NULL) goto errexit;
	    sout = n_mkstemp(getval(nshell,"TMPDIR"), TEMPPFX, &tmpfil);
	    if (sout < 0)
	      goto errexit;
	    unlinkfile(&((cmdcur->next)->pipefile));
	    (cmdcur->next)->pipefile=tmpfil;
	  }
	  if (sout!=NOHANDLE) sout2=nredirect(1,sout);

	  if (nshell->optionx) {
	    for (i=0;i<nshell->cmdexec;i++) printfconsole("+");
	    printfconsole(" ");
	    prm=prmcur;
	    while (prm!=NULL) {
	      printfconsole("%.256s ",prm->str);
	      prm=prm->next;
	    }
	    printfconsole("\n");
	  }

	  cmds=prmcur->str;
	  /* exec named function */
	  if ((cmdnew=getfunc(nshell,cmds))!=NULL) {
	    needcmd=FALSE;
	    st=NULL;
	    rcode=syntax(nshell,cmdnew,&needcmd,&st);
	    cmdstackfree(st);
	    if ((rcode!=0) || (st!=NULL) || (needcmd)) goto errexit;
	    argvnew=NULL;
	    if ((s=g_malloc(strlen((nshell->argv)[0])+1))==NULL)
	      goto errexit;
	    strcpy(s,(nshell->argv)[0]);
	    if (arg_add(&argvnew,s)==NULL) {
	      g_free(s);
	      arg_del(argvnew);
	      goto errexit;
	    }
	    prmcur=prmcur->next;
	    while (prmcur!=NULL) {
	      if ((s=g_malloc(strlen(prmcur->str)+1))==NULL) {
		g_free(s);
		arg_del(argvnew);
		goto errexit;
	      }
	      strcpy(s,prmcur->str);
	      if (arg_add(&argvnew,s)==NULL) {
		g_free(s);
		arg_del(argvnew);
		goto errexit;
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
	    if ((rcode==-1) || (rcode==1)) goto errexit;
	    /* exec special command */
	    /* . */
	  } else if (strcmp0(".",cmds)==0) {
	    prmcur=prmcur->next;
	    if (prmcur!=NULL) {
	      cmdname=nsearchpath(getval(nshell,"PATH"),prmcur->str,TRUE);
	      if (cmdname!=NULL) {
		argvnew=NULL;
		if ((s=g_malloc(strlen((nshell->argv)[0])+1))==NULL)
		  goto errexit;
		strcpy(s,(nshell->argv)[0]);
		if (arg_add(&argvnew,s)==NULL) {
		  g_free(s);
		  arg_del(argvnew);
		  goto errexit;
		}
		prmcur=prmcur->next;
		while (prmcur!=NULL) {
		  if ((s=g_malloc(strlen(prmcur->str)+1))==NULL) {
		    g_free(s);
		    arg_del(argvnew);
		    goto errexit;
		  }
		  strcpy(s,prmcur->str);
		  if (arg_add(&argvnew,s)==NULL) {
		    g_free(s);
		    arg_del(argvnew);
		    goto errexit;
		  }
		  prmcur=prmcur->next;
		}
		argcnew=getargc(argvnew);

		sin=nopen(cmdname,O_RDONLY,NFMODE);
		g_free(cmdname);
		cmdname=NULL;
		if (sin==NOHANDLE) {
		  sherror2(ERROPEN,cmdname);
		  arg_del(argvnew);
		  goto errexit;
		}
		argcsave=nshell->argc;
		argvsave=nshell->argv;
		nshell->argc=argcnew;
		nshell->argv=argvnew;
		sin3=storeshhandle(nshell,sin,&readbuf,&readbyte,&readpo);
		rcode=cmdexecute(nshell,NULL);
		restoreshhandle(nshell,sin3,readbuf,readbyte,readpo);
		nclose(sin);
		arg_del(nshell->argv);
		nshell->argc=argcsave;
		nshell->argv=argvsave;
		if ((rcode!=0) && (rcode!=1)) goto errexit;
	      } else {
		sherror2(ERRNOFIL,prmcur->str);
		goto errexit;
	      }
	    }
	    /* break */
	  } else if (strcmp0("break",cmds)==0) {
	    if (pnum>2) {
	      sherror(ERREXTARG);
	      goto errexit;
	    } else if (pnum==2) {
	      arg=(prmcur->next)->str;
	      a=strtol(arg,&endptr,10);
	      if (endptr[0]!='\0') {
		sherror2(ERRNUMERIC,arg);
		goto errexit;
	      }
	    } else a=1;
	    looplevel=0;
	    stcur=stroot;
	    while (stcur!=NULL) {
	      if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
		  || (stcur->cmdno==CPUNTIL)) looplevel++;
	      stcur=stcur->next;
	    }
	    if (looplevel!=0) {
	      while (a>0) {
		if ((stcur=cmdstackgetpo(&stroot))==NULL) break;
		if ((stcur->cmdno==CPFOR) && (stcur->ival==-1)) {
		  prmfree(stcur->prm);
		  stcur->prm=NULL;
		}
		if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
		    || (stcur->cmdno==CPUNTIL)) {
		  cmdcur=stcur->cmd;
		  while (cmdcur->cmdno!=CPDO) cmdcur=cmdcur->next;
		  cmdcur=cmdcur->done;
		  cmdstackrmlast(&stroot);
		  a--;
		} else cmdstackrmlast(&stroot);
	      }
	    }
	    /* continue */
	  } else if (strcmp0("continue",cmds)==0) {
	    if (pnum>2) {
	      sherror(ERREXTARG);
	      goto errexit;
	    } else if (pnum==2) {
	      arg=(prmcur->next)->str;
	      a=strtol(arg,&endptr,10);
	      if (endptr[0]!='\0') {
		sherror2(ERRNUMERIC,arg);
		goto errexit;
	      }
	    } else a=1;
	    looplevel=0;
	    stcur=stroot;
	    while (stcur!=NULL) {
	      if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
		  || (stcur->cmdno==CPUNTIL)) looplevel++;
	      stcur=stcur->next;
	    }
	    if (looplevel!=0) {
	      while (a>0) {
		if ((stcur=cmdstackgetpo(&stroot))==NULL) break;
		if ((stcur->cmdno==CPFOR) || (stcur->cmdno==CPWHILE)
		    || (stcur->cmdno==CPUNTIL)) {
		  cmdcur=stcur->cmd;
		  a--;
		  if (a>0) {
		    if ((stcur->cmdno==CPFOR) && (stcur->ival==-1)) {
		      prmfree(stcur->prm);
		      stcur->prm=NULL;
		    }
		    cmdstackrmlast(&stroot);
		  }
		} else cmdstackrmlast(&stroot);
	      }
	    }
	    /* return */
	  } else if (strcmp0("return",cmds)==0) {
	    if (pnum>2) {
	      sherror(ERREXTARG);
	      goto errexit;
	    } else if (pnum==2) {
	      arg=(prmcur->next)->str;
	      a=strtol(arg,&endptr,10);
	      if (endptr[0]!='\0') {
		sherror2(ERRNUMERIC,arg);
		goto errexit;
	      }
	      nshell->status=a;
	    }
	    if (namedfunc) {
	      err=0;
	      goto errexit;
	    }
	    /* execute object */
	  } else if (isobject(&cmds)) {
	    len=0;
	    prm=prmcur;
	    while (prm!=NULL) {
	      len+=strlen(prm->str)+1;
	      prm=prm->next;
	    }
	    if ((str=g_malloc(len))==NULL) goto errexit;
	    str[0]='\0';
	    prm=prmcur;
	    while (prm!=NULL) {
	      strcat(str,prm->str);
	      if (prm->next!=NULL) strcat(str," ");
	      prm=prm->next;
	    }
	    errlevel=sexeobj(str);
	    g_free(str);
	    if (errlevel==-1) goto errexit;
	    nshell->status=errlevel;
	    lastc=cmdstackgetlast(&stroot);
	    if ((lastc!=CPIF) && (lastc!=CPELIF)
		&& (lastc!=CPWHILE) && (lastc!=CPUNTIL)
		&& (nshell->optione) && (errlevel!=0)) {
	      err=3;
	      goto errexit;
	    }
	    nshell->status=errlevel;
	    lastc=cmdstackgetlast(&stroot);
	    if ((lastc!=CPIF) && (lastc!=CPELIF)
		&& (lastc!=CPWHILE) && (lastc!=CPUNTIL)
		&& (nshell->optione) && (errlevel!=0)) {
	      err=3;
	      goto errexit;
	    }
	    if (nshell->quit) break;
	    /* exec inner command */
	  } else {
	    proc = check_cmd(prmcur->str);
	    if (proc) {
	      argv = NULL;
	      if (arg_add(&argv, NULL) == NULL)
		goto errexit;
	      prm = prmnewroot;
	      while (prm) {
		if (arg_add(&argv, prm->str) == NULL) {
		  g_free(argv);
		  goto errexit;
		}
		prm = prm->next;
	      }
	      errlevel = proc(nshell, pnum, (char **)argv);
	      g_free(argv);
	    } else {
	      cmdname=nsearchpath(getval(nshell,"PATH"),prmcur->str,FALSE);
	      if (cmdname==NULL) {
		sherror2(ERRCFOUND,prmcur->str);
		goto errexit;
	      } else {
		if (Security) {
		  sherror(ERRSECURITY);
		  goto errexit;
		}
		argv=NULL;
		if (arg_add(&argv,NULL)==NULL) goto errexit;
		prm=prmnewroot;
		while (prm!=NULL) {
		  if (arg_add(&argv,prm->str)==NULL) {
		    g_free(argv);
		    goto errexit;
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
		    goto errexit;
		  }

		  win_cmd = g_locale_from_utf8(ptr, -1, NULL, NULL, NULL);
		  g_free(ptr);
		  if (win_cmd == NULL) {
		    sherror(ERRMEMORY);
		    goto errexit;
		  }

		  errlevel = 0;
		  thread= g_thread_new("process", proc_in_thread, win_cmd);
		  if (thread) {
		    WaitProc = 1;
		    while (WaitProc) {
		      msleep(1);
		      eventloop();
		    }
		    errlevel = GPOINTER_TO_INT(g_thread_join(thread));
		  }
		  g_free(win_cmd);
		}
#else	/* WINDOWS */
		unset_childhandler();
		pid = fork();
		if (pid < 0) {
		  sherror2(ERRSYSTEM,"fork");
		  set_childhandler();
		  goto errexit;
		} else if (pid == 0) {
		  errlevel=execve(cmdname,(char **)argv,MainEnviron);
		  printfstderr("shell: %.64s: %.64s",
			       argv[0],g_strerror(errno));
		  exit(errlevel);
		} else {
		  if (has_eventloop()) {
		    while (waitpid(pid,&errlevel,WNOHANG)==0) {
		      eventloop();
		      msleep(10);
		    }
		  } else {
		    waitpid(pid, &errlevel, 0);
		  }
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
	    lastc=cmdstackgetlast(&stroot);
	    if ((lastc!=CPIF) && (lastc!=CPELIF)
		&& (lastc!=CPWHILE) && (lastc!=CPUNTIL)
		&& (nshell->optione) && (errlevel!=0)) {
	      err=3;
	      goto errexit;
	    }
	    if (nshell->quit) break;
	  }

	  if (sout2!=NOHANDLE) {
	    nredirect2(1,sout2);
	    sout2=NOHANDLE;
	  }
	  if (sin2!=NOHANDLE) {
	    nredirect2(0,sin2);
	    sin2=NOHANDLE;
	  }
	  unlinkfile(&tmpfil2);
	  arg_del(MainEnviron);
	  MainEnviron=env;
	  env=NULL;
	}
	restoreval(nshell,newvalroot);
	newvalroot=NULL;

	prmfree(prmnewroot);
	prmnewroot=NULL;
	g_free(fstdout);
	g_free(fstdin);
	fstdout=fstdin=NULL;
	unlinkfile(&(cmdcur->pipefile));
	cmdcur=cmdcur->next;
	break;
      default:

	unlinkfile(&(cmdcur->pipefile));
	cmdcur=cmdcur->next;
	break;
      }
    }
  }
  err=0;

 errexit:
  cmdstackfree(stroot);
  prmfree(prmnewroot);
  g_free(fstdout);
  g_free(fstdin);
  g_free(cmdname);
  arg_del(newenviron);
  if (env!=NULL) {
    arg_del(MainEnviron);
    MainEnviron=env;
  }
  if (newvalroot!=NULL) restoreval(nshell,newvalroot);
  if (sout2!=NOHANDLE) nredirect2(1,sout2);
  if (sin2!=NOHANDLE) nredirect2(0,sin2);
  unlinkfile(&tmpfil2);
  cmdcur=cmdroot;
  while (cmdcur!=NULL) {
    prmcur=cmdcur->prm;
    while (prmcur!=NULL) {
      if (prmcur->prmno == PPSI2 &&
	  prmcur->next!=NULL &&
	  prmcur->next->str != NULL) {
        g_unlink((prmcur->next)->str);
        prmcur=prmcur->next;
      }
      prmcur=prmcur->next;
    }
    unlinkfile(&(cmdcur->pipefile));
    cmdcur=cmdcur->next;
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
shellrestorestdio(struct nshell *nshell)
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

void
setshellargument(struct nshell *nshell,int argc,char **argv)
{
  arg_del(nshell->argv);
  nshell->argc=argc;
  nshell->argv=argv;
}

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
