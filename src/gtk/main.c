/* 
 * $Id: main.c,v 1.46 2009/11/25 14:36:02 hito Exp $
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

#include "gtk_common.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <locale.h>
#include <signal.h>

#include "dir_defs.h"
#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "nconfig.h"
#include "shell.h"

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#include "ox11menu.h"
#include "x11menu.h"
#include "ogra2x11.h"
#include "ogra2cairo.h"
#include <assert.h>
static char **attempt_shell_completion(char *text, int start, int end);
#define HIST_SIZE 100
#define HIST_FILE "shell_history"
#endif

#define SYSCONF "[Ngraph]"
#ifndef LIBDIR
#define LIBDIR "/usr/local/lib/Ngraph"
#endif

#define INIT_SCRIPT "Ngraph.nsc"

static char *systemname;
static int consolefdout, consolefdin, consoleac = FALSE;
static int consolecol = 80, consolerow = 25;

void *addobjectroot(void);
void *addint(void);
void *adddouble(void);
void *addio(void);
void *addstring(void);
void *addiarray(void);
void *adddarray(void);
void *addsarray(void);
void *addsystem(void);
void *addshell(void);
void *adddraw(void);
void *addfile(void);
void *addmath(void);
void *addfit(void);
void *addgra(void);
void *addgra2(void);
void *addgra2null(void);
void *addgra2file(void);
void *addgra2prn(void);
void *addgra2cairo(void);
void *addgra2cairofile(void);
void *addgra2gtkprint(void);
void *addgra2gdk(void);
void *addmerge(void);
void *addlegend(void);
void *addline(void);
void *addcurve(void);
void *addrectangle(void);
void *addarc(void);
void *addpolygon(void);
void *addmark(void);
void *addtext(void);
void *addaxis(void);
void *addagrid(void);
void *addprm(void);

void *addgra2gtk(void);
void *addmenu(void);
void *adddialog(void);

void resizeconsole(int col, int row);

// XtAppContext Application=NULL;
char *AppName = "Ngraph", *AppClass = "Ngraph", *Home;
char *License = "\
This program is free software; you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation; either version 2 of the License, or \
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License \
along with this program; if not, write to the Free Software \
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\
";

char *Auther[] = {
  "Satoshi ISHIZAKA",
  "Ito Hiroyuki",
  NULL
};

char *Translator = 
  "Satoshi ISHIZAKA\n" \
  "Ito Hiroyuki" \
  ;

char *Documenter[] = {
  "Satoshi ISHIZAKA",
  "Ito Hiroyuki",
  NULL
};


static int OpenDisplay = FALSE;

static void * ( * obj_add_func_ary[]) (void) = {
  addshell,
  addgra,
  addgra2,
  addgra2null,
  addgra2file,
  addgra2prn,
  addgra2cairo,
  addgra2cairofile,
  addgra2gtkprint,
  addgra2gtk,
  addgra2gdk,
  addio,
  addint,
  adddouble,
  addstring,
  addiarray,
  adddarray,
  addsarray,
  addmath,
  addfit,
  addprm,
  adddraw,
  addagrid,
  addaxis,
  addfile,
  addmerge,
  addlegend,
  addrectangle,
  addline,
  addcurve,
  addarc,
  addpolygon,
  addmark,
  addtext,
  addmenu,
  adddialog,
};

int
OpenApplication(void)
{
  return OpenDisplay;
}

int
putconsole(char *s)
{
  int len;

  len = strlen(s);
  if (write(consolefdout, s, len) < 0)
    return 0;

  if (write(consolefdout, "\n", 1) < 0)
    return 0;

  return len + 1;
}

int
printfconsole(char *fmt, ...)
{
  int len;
  char buf[1024];
  va_list ap;

  va_start(ap, fmt);
  len = vsnprintf(buf, sizeof(buf),  fmt, ap);
  va_end(ap);
  if (write(consolefdout, buf, len) < 0)
    return 0;

  return len;
}

static int
interruptconsole(void)
{
  return FALSE;
}

static int
inputynconsole(char *mes)
{
  int len, r;
  char buf[10], yn[] = " [yn] ";

  len = strlen(mes);
  if (write(consolefdout, mes, len))
    return FALSE;

  if (write(consolefdout, yn, sizeof(yn) - 1))
    return FALSE;

  do {
    r = read(consolefdin, buf, 1);
  } while (strchr("yYnN", buf[0]) == NULL && r >= 0);

  if (r < 0)
    return FALSE;

  do {
    r = read(consolefdin, buf, 1);
  } while(buf[0] != '\n' && r >= 0);

  if ((buf[0] == 'y') || (buf[0] == 'Y'))
    return TRUE;
  return FALSE;
}

static void
displaydialogconsole(char *str)
{
  putconsole(str);
}

static void
displaystatusconsole(char *str)
{
}

static char *terminal = NULL;
static struct savedstdio consolesave;
static int consolefd[3];
static pid_t consolepid = -1;

void
resizeconsole(int col, int row)
{
}

static void
reset_fifo(char *fifo_in, char *fifo_out)
{
  int fdi, fdo;

  fdi = open(fifo_in, O_WRONLY);
  fdo = open(fifo_out, O_RDONLY);
  if (fdi >= 0)
    close(fdi);

  if (fdo >= 0)
    close(fdo);
}

static int
exec_console(char *fifo_in, char *fifo_out)
{
  char **argv;
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    unlink(fifo_in);
    unlink(fifo_out);
    return 1;
  } else if (pid == 0) {
    pid = fork();
    if (pid == 0) {
      char buf[256], *s, *s2;
      int len;

      snprintf(buf, sizeof(buf), "%s %s %s", terminal, fifo_in, fifo_out);
      argv = NULL;
      s = buf;
      while ((s2 = getitok2(&s, &len, " \t")) != NULL) {
	arg_add(&argv, s2);
      }
      execvp(argv[0], argv);
    } else if (pid != -1) {
      int status;

      waitpid(pid, &status, 0);
    }
    reset_fifo(fifo_in, fifo_out);
    exit(0);
  }
  return 0;
}

int
nallocconsole(void)
{
  int fd[3] = {-1, -1, -1}, fdi, fdo;
  unsigned int i;
  char buf[256], ttyname[256], fifo_in[1024], fifo_out[1024];
  struct objlist *sys;
  char *sysname;
  char *version;

  if (consoleac)
    return FALSE;

  if (terminal == NULL)
    return FALSE;

  snprintf(fifo_in, sizeof(fifo_in) - 1, "/tmp/nterm1_%d", getpid());
  snprintf(fifo_out, sizeof(fifo_out) - 1, "/tmp/nterm2_%d", getpid());

  if (mkfifo(fifo_in, 0600)) {
    return FALSE;
  }

  if (mkfifo(fifo_out, 0600)) {
    unlink(fifo_in);
    return FALSE;
  }

  if (exec_console(fifo_in, fifo_out))
    return FALSE;

  fdi = open(fifo_in, O_RDONLY);
  fdo = open(fifo_out, O_WRONLY);
  unlink(fifo_in);
  unlink(fifo_out);

  sys = chkobject("system");
  getobj(sys, "name", 0, 0, NULL, &sysname);
  getobj(sys, "version", 0, 0, NULL, &version);
  snprintf(buf, sizeof(buf), "%c]2;Ngraph shell%c%s version %s. Script interpreter.\n",
	   0x1b, 0x07, sysname, version);

  if (write(fdo, buf, strlen(buf) + 1) < 0) {
    close(fdi);
    close(fdo);
    return FALSE;
  }

  close(fdo);

  for (i = 0; i < sizeof(ttyname) - 1; i++) {
    if (read(fdi, ttyname + i, 1) != 1)
      break;

    if (ttyname[i] == '\0')
      break;
  }
  ttyname[i] = '\0';

  if (i == 0) {
    close(fdi);
    return FALSE;
  }

  for (i = 0; i < sizeof(buf) - 1; i++) {
    if (read(fdi, buf + i, 1) != 1)
      break;

    if (buf[0] == '\0')
      break;
  }
  close(fdi);

  if (i == 0)
    return FALSE;

  buf[i] = '\0';

  fd[0] = open(ttyname, O_RDONLY);
  if (fd[0] < 0) {
    goto ErrEnd;
  }

  fd[1] = open(ttyname, O_WRONLY);
  if (fd[1] < 0) {
    goto ErrEnd;
  }

  fd[2] = open(ttyname, O_WRONLY);
  if (fd[2] < 0) {
    goto ErrEnd;
  }

  consolepid = atoi(buf);

  consolefd[0] = dup(0);
  close(0);
  dup2(fd[0], 0);

  consolefd[1] = dup(1);
  close(1);
  dup2(fd[1], 1);

  consolefd[2] = dup(2);
  close(2);
  dup2(fd[2], 2);

  close(fd[0]);
  close(fd[1]);
  close(fd[2]);
  consolefdin = dup(0);
  consolefdout = dup(2);
  consoleac = TRUE;
  savestdio(&consolesave);
  putstderr = putconsole;
  printfstderr = printfconsole;
  ninterrupt = interruptconsole;
  inputyn = inputynconsole;
  ndisplaydialog = displaydialogconsole;
  ndisplaystatus = displaystatusconsole;

  return TRUE;

 ErrEnd:
  if (fd[0] < 0) {
    close(fd[0]);
  }

  if (fd[1] < 0) {
    close(fd[1]);
  }

  if (fd[2] < 0) {
    close(fd[2]);
  }

  return FALSE;
}

void
nfreeconsole(void)
{
  if (consoleac) {
    close(0);
    if (consolefd[0] != -1) {
      dup2(consolefd[0], 0);
      close(consolefd[0]);
    }
    close(1);
    if (consolefd[1] != -1) {
      dup2(consolefd[1], 1);
      close(consolefd[1]);
    }
    close(2);
    if (consolefd[2] != -1) {
      dup2(consolefd[2], 2);
      close(consolefd[2]);
    }

    kill(consolepid, SIGTERM);
    consolepid = -1;

    close(consolefdin);
    close(consolefdout);

    consolefdin = 0;
    consolefdout = 2;
    consoleac = FALSE;
    loadstdio(&consolesave);
  }
}

void
nforegroundconsole()
{
}

static void
#ifdef HAVE_LIBREADLINE
load_config(struct objlist *sys, char *inst, int *allocconsole, int *history_size)
#else
load_config(struct objlist *sys, char *inst, int *allocconsole)
#endif
{
  FILE *fp;
  char *tok, *str, *s2, *f1, *endptr;
  int len;
  long val;

  if ((fp = openconfig(SYSCONF)) != NULL) {
    while ((tok = getconfig(fp, &str)) != NULL) {
      s2 = str;
      if (strcmp(tok, "login_shell") == 0) {
	f1 = getitok2(&s2, &len, " \t,");
	if (_putobj(sys, "login_shell", inst, f1))
	  exit(1);
      } else if (strcmp(tok, "create_object") == 0) {
	while ((f1 = getitok2(&s2, &len, " \t,")) != NULL) {
	  struct objlist *obj;
	  obj = getobject(f1);
	  if (obj) {
	    newobj(obj);
	  }
	  g_free(f1);
	}
      } else if (strcmp(tok, "alloc_console") == 0) {
	f1 = getitok2(&s2, &len, " \t,");
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0') {
	  if (val == 0)
	    *allocconsole = FALSE;
	  else
	    *allocconsole = TRUE;
	}
	g_free(f1);
      } else if (strcmp(tok, "console_size") == 0) {
	f1 = getitok2(&s2, &len, " \x09,");
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  consolecol = val;
	g_free(f1);
	f1 = getitok2(&s2, &len, " \x09,");
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  consolerow = val;
	g_free(f1);
#ifdef HAVE_LIBREADLINE
      } else if (strcmp(tok, "history_size") == 0) {
	f1 = getitok2(&s2, &len, " \t,");
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0' && val > 0) {
	  *history_size = val;
	}
	g_free(f1);
#endif
      } else if (strcmp(tok, "terminal") == 0) {
	terminal = getitok2(&s2, &len, "");
      } else {
	fprintf(stderr, "configuration '%s' in section %s is not used.\n", tok, SYSCONF);
      }
      g_free(tok);
      g_free(str);
    }
    closeconfig(fp);
  }
}

int
main(int argc, char **argv, char **environ)
{
  char *homedir, *libdir, *confdir, *home, *inifile, *loginshell;
  char *inst;
  struct objlist *sys, *obj, *lobj;
  unsigned int j;
  int i;
  char *sarg[2];
  struct narray sarray;
  int id;
  int allocnow, allocconsole = FALSE;
  struct narray iarray;
  char *arg;
#ifdef HAVE_LIBREADLINE
  int history_size = HIST_SIZE;
  char *history_file = NULL;
#endif

#if EOF == -1
  char_type_buf_init();
#endif

  set_childhandler();

  gtk_set_locale();
  OpenDisplay = gtk_init_check(&argc, &argv);
  g_set_application_name(AppName);

  set_environ(environ);

  if (init_cmd_tbl()) {
    exit(1);
  }

  ignorestdio(NULL);
  inputyn = vinputyn;
  ninterrupt = vinterrupt;
  printfstderr = seprintf;
  putstderr = seputs;
  consolefdin = 0;
  consolefdout = 2;

#ifdef HAVE_GETTEXT
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);
#endif

#if 0
  if ((lib = getenv("NGRAPHLIB")) != NULL) {
    libdir = g_strdup(lib);
  } else {
    libdir = g_strdup(LIBDIR);
  }
#else
  libdir = g_strdup(LIBDIR);
#endif
  if (libdir == NULL)
    exit(1);

  confdir = g_strdup(CONFDIR);
  if (confdir == NULL)
    exit(1);

  /*
  if ((home = getenv("NGRAPHHOME")) != NULL) {
    if ((homedir = (char *) g_malloc(strlen(home) + 1)) == NULL)
      exit(1);
    strcpy(homedir, home);
  } else 
  */
  if ((home = getenv("HOME")) != NULL) {
    char *ptr;

    ptr = g_strdup_printf("%s/%s", home, HOME_DIR);
    homedir = g_strdup(ptr);
    g_free(ptr);
    if (homedir == NULL)
      exit(1);
  } else {
    homedir = g_strdup(confdir);
    if (homedir == NULL)
      exit(1);
  }

  if (addobjectroot() == NULL)
    exit(1);

  if (addsystem() == NULL)
    exit(1);

  sys = getobject("system");
  if (sys == NULL)
    exit(1);

  if (newobj(sys) < 0)
    exit(1);

  inst = chkobjinst(sys, 0);
  if (inst == NULL)
    exit(1);

  if (_putobj(sys, "conf_dir", inst, confdir))
    exit(1);
  if (_putobj(sys, "lib_dir", inst, libdir))
    exit(1);
  if (_putobj(sys, "home_dir", inst, homedir))
    exit(1);
  if (_getobj(sys, "conf_dir", inst, &confdir) == -1)
    exit(1);
  if (_getobj(sys, "lib_dir", inst, &libdir) == -1)
    exit(1);
  if (_getobj(sys, "home_dir", inst, &homedir) == -1)
    exit(1);
  if (_getobj(sys, "name", inst, &systemname) == -1)
    exit(1);

  for (j = 0; j < sizeof(obj_add_func_ary) / sizeof(*obj_add_func_ary); j++) {
    if (obj_add_func_ary[j]() == NULL)
      exit(1);
  }

  loginshell = NULL;
#ifdef HAVE_LIBREADLINE
  load_config(sys, inst, &allocconsole, &history_size);
  rl_readline_name = "ngraph";
  rl_completer_word_break_characters = " \t\n\"'@><;|&({}`";
  rl_attempted_completion_function = (CPPFunction *) attempt_shell_completion;
  rl_completion_entry_function = NULL;

  history_file = g_strdup_printf("%s/%s", homedir, HIST_FILE);
  if (history_file) {
    read_history(history_file);
  }
  using_history();
  stifle_history(history_size);
#else
  load_config(sys, inst, &allocconsole);
#endif

  putstderr = putconsole;
  printfstderr = printfconsole;
  inputyn = inputynconsole;
  ndisplaydialog = displaydialogconsole;
  ndisplaystatus = displaystatusconsole;

  if (allocconsole) {
    nallocconsole();
  }

  if (isatty(0) && isatty(1) && isatty(2)) {
    consoleac = TRUE;
    if (!allocconsole) {
      consolefdin = dup(0);
      consolefdout = dup(2);
    }
  } else {
    consoleac = FALSE;
  }

  inifile = NULL;
  obj = getobject("shell");
  if (obj == NULL)
    exit(1);

  id = newobj(obj);
  if (id < 0)
    exit(1);

  for (i = 1; i < argc; i++) {
    if (argv[i][0] != '-' || argv[i][1] != 'i' || i >= argc - 1) {
      break;
    }
    i++;
    inifile = g_strdup(argv[i]);
    if (inifile == NULL) {
      exit(1);
    }
    changefilename(inifile);
  }
  if (inifile == NULL) {
    if (findfilename(homedir, CONFTOP, INIT_SCRIPT))
      inifile = getfilename(homedir, CONFTOP, INIT_SCRIPT);
    else if (findfilename(confdir, CONFTOP, INIT_SCRIPT))
      inifile = getfilename(confdir, CONFTOP, INIT_SCRIPT);
  }
  if (inifile) {
    arrayinit(&sarray, sizeof(char *));
    if (arrayadd(&sarray, &inifile) == NULL)
      exit(1);
    for (; i < argc; i++)
      if (arrayadd(&sarray, &(argv[i])) == NULL)
	exit(1);
    sarg[0] = (char *) &sarray;
    sarg[1] = NULL;
    exeobj(obj, "shell", id, 1, sarg);
    arraydel(&sarray);
    g_free(inifile);
  }
  if (getobj(sys, "login_shell", 0, 0, NULL, &loginshell))
    exit(1);
  do {
    if (_putobj(sys, "login_shell", inst, NULL))
      exit(1);
    if (loginshell == NULL) {
      allocnow = nallocconsole();
      exeobj(obj, "shell", id, 0, NULL);
      if (allocnow)
	nfreeconsole();
    } else {
      arrayinit(&iarray, sizeof(int));
      arg = loginshell;
      if (getobjilist2(&arg, &lobj, &iarray, TRUE)) {
	return -1;
      }
      arraydel(&iarray);
      if (lobj == obj) {
	allocnow = nallocconsole();
      } else {
	allocnow = FALSE;
      }
      sexeobj(loginshell);
      if (allocnow) {
	nfreeconsole();
      }
    }
    g_free(loginshell);
    if (getobj(sys, "login_shell", 0, 0, NULL, &loginshell)) {
      exit(1);
    }
  } while (loginshell != NULL);
#ifdef HAVE_LIBREADLINE
  if (history_file != NULL) {
    write_history(history_file);
    g_free(history_file);
  }
#endif
  if (consoleac && (consolepid != -1))
    nfreeconsole();
  g_free(terminal);
  delobj(getobject("system"), 0);
  return 0;
}

#ifdef HAVE_LIBREADLINE
struct mylist
{
  struct mylist *next;
  int len;
  char str[1];
};

static char **obj_name_matching(const char *text);
static char *obj_member_completion_function(const char *text, int state);
static char *obj_name_completion_function(const char *text, int state);
static char *my_completion_function(const char *text, int state,
				    char **func(const char *));
static char *command_word_completion_function(const char *hint_text, int state);
static char **cmd_name_matching(const char *text);
static char **get_obj_member_list(struct objlist *objcur, char *member);
static char **get_obj_enum_list(struct objlist *objcur, char *member,
				char *val);
static char **get_obj_bool_list(struct objlist *objcur, char *member,
				char *val);
static char **get_obj_font_list(struct objlist *objcur, char *member,
				char *val);
static int get_obj_num(void);
static struct mylist *mylist_add(struct mylist *parent, const char *text);
static struct mylist *mylist_cat(struct mylist *list_top,
				 struct mylist *list);
static void mylist_free(struct mylist *list);
static int mylist_num(const struct mylist *list);
static struct mylist *get_file_list(const char *path, int type, int mode);
static struct mylist *get_exec_file_list(void);
static int my_sprintf(char **str, char *format, ...);

static char **
attempt_shell_completion(char *text, int start, int end)
{
  char **matches = NULL;
  int in_command_position, ti;
  char *command_separator_chars = ";|(`";

  ti = start - 1;
  while ((ti > -1) && (whitespace(rl_line_buffer[ti])))
    ti--;

  in_command_position = 0;
  if (ti < 0) {
    in_command_position++;
  } else if (strchr(command_separator_chars, rl_line_buffer[ti])) {
    in_command_position++;
  }

  if (!matches && in_command_position)
    matches = rl_completion_matches(text, command_word_completion_function);

  if (!matches)
    matches = rl_completion_matches(text, obj_name_completion_function);

  if (!matches)
    matches = rl_completion_matches(text, obj_member_completion_function);

  return matches;
}

static char *
obj_member_completion_function(const char *text, int state)
{
  static char **list = (char **) NULL;
  static int list_index = 0, first_char_loc;
  struct objlist *objcur;

  /* If we don't have any state, make some. */
  if (!state) {
    static char *obj, *instances, *member, *val;

    if (list)
      g_free(list);

    list = (char **) NULL;

    first_char_loc = 0;

    obj = g_strdup(text);
    if (obj == NULL)
      return NULL;

    if ((instances = strchr(obj, ':'))
	&& (member = strchr(instances + 1, ':'))) {
      *instances = *member = '\0';
      instances++;
      member++;
    } else {
      g_free(obj);
      return NULL;
    }

    objcur = getobject(obj);
    if (objcur == NULL) {
      g_free(obj);
      return NULL;
    }

    if ((val = strchr(member, '=')) != NULL) {
      *val = '\0';
      val++;
      first_char_loc = val - obj;
      list = get_obj_enum_list(objcur, member, val);

      if (list == NULL)
	list = get_obj_bool_list(objcur, member, val);

      if (list == NULL)
	list = get_obj_font_list(objcur, member, val);
    } else {
      first_char_loc = member - obj;
      list = get_obj_member_list(objcur, member);
    }

    list_index = 0;
    g_free(obj);
  }

  if (list && list[list_index]) {
    char *t;

    t = g_strdup_printf("%.*s%s", first_char_loc, text, list[list_index]);
    list_index++;
    return (t);
  } else {
    return ((char *) NULL);
  }
}

static char **
get_obj_member_list(struct objlist *objcur, char *member)
{
  char **list = (char **) NULL;
  int i, j, len;

  if ((list = g_malloc(sizeof(*list) * (chkobjfieldnum(objcur) + 1))) == NULL)
    return NULL;

  len = strlen(member);
  j = 0;
  for (i = 0; i < chkobjfieldnum(objcur); i++) {
    if (strncmp(chkobjfieldname(objcur, i), member, len) == 0) {
      list[j++] = chkobjfieldname(objcur, i);
    }
  }
  list[j] = NULL;
  return list;
}

static char **
get_obj_enum_list(struct objlist *objcur, char *member, char *val)
{
  char **list = (char **) NULL, **enumlist;
  int i, j, len;

  if (chkobjfieldtype(objcur, member) != NENUM)
    return NULL;

  enumlist = (char **) chkobjarglist(objcur, member);
  for (i = 0; enumlist[i] != NULL; i++);

  list = g_malloc((sizeof(*list)) * (i + 1));
  if (list == NULL)
    return NULL;

  len = strlen(val);
  j = 0;
  for (i = 0; enumlist[i] != NULL; i++) {
    if (strncmp(enumlist[i], val, len) == 0) {
      list[j++] = enumlist[i];
    }
  }
  list[j] = NULL;

  return list;
}

static char **
get_obj_bool_list(struct objlist *objcur, char *member, char *val)
{
  char **list = (char **) NULL;
  static char *boollist[] = { "true", "false", NULL };
  int i, j, len;

  if (chkobjfieldtype(objcur, member) != NBOOL)
    return NULL;

  list = g_malloc(sizeof(boollist));
  if (list == NULL)
    return NULL;

  len = strlen(val);
  j = 0;
  for (i = 0; boollist[i] != NULL; i++) {
    if (strncmp(boollist[i], val, len) == 0) {
      list[j++] = boollist[i];
    }
  }
  list[j] = NULL;

  return list;
}

static char **
get_obj_font_list(struct objlist *objcur, char *member, char *val)
{
  char **list = (char **) NULL;
  struct fontmap *fontmap, *fontmaproot;
  int j, len, twobyte;

  if (Gra2cairoConf == NULL)
    return NULL;

  fontmaproot = Gra2cairoConf->fontmap_list_root;

  if (fontmaproot == NULL)
    return NULL;

  if (chkobjfieldtype(objcur, member) != NSTR)
    return NULL;

  if (strstr(member, "font") == NULL)
    return NULL;

  list = g_malloc((sizeof(*list)) * (Gra2cairoConf->font_num + 1));
  if (list == NULL)
    return NULL;

  twobyte = (strstr(member, "jfont") != NULL);

  len = strlen(val);
  j = 0;
  for (fontmap = fontmaproot; fontmap != NULL;
       fontmap = fontmap->next) {
    if ((fontmap->twobyte == twobyte)
	&& (strncmp(fontmap->fontalias, val, len) == 0)) {
      list[j++] = fontmap->fontalias;
    }
  }
  list[j] = NULL;
  return list;
}

static char *
command_word_completion_function(const char *text, int state)
{
  return my_completion_function(text, state, cmd_name_matching);
}

static char *
obj_name_completion_function(const char *text, int state)
{
  return my_completion_function(text, state, obj_name_matching);
}

static char *
my_completion_function(const char *text, int state, char **func(const char *))
{
  static char **list = (char **) NULL;
  static int list_index = 0;
  static int first_char_loc;

  /* If we don't have any state, make some. */
  if (!state) {
    if (list)
      g_free(list);

    list = (char **) NULL;

    first_char_loc = 0;

    list = func(&text[first_char_loc]);
    list_index = 0;
  }

  if (list && list[list_index]) {
    char *t = g_strdup(list[list_index]);
    if (t == NULL)
      return NULL;

    list_index++;
    return t;
  }

  return NULL;
}

static int
get_obj_num(void)
{
  static int num = 0;
  if (num == 0) {
    struct objlist *objcur;
    for (objcur = chkobjroot(); objcur != NULL; objcur = objcur->next)
      num++;
  }

  return num;
}

static char **
obj_name_matching(const char *text)
{
  int j, text_len;
  struct objlist *objcur;
  char **list;

  list = g_malloc((get_obj_num() + 1) * sizeof(*list));

  if (list == NULL)
    return NULL;

  text_len = strlen(text);
  j = 0;

  for (objcur = chkobjroot(); objcur != NULL; objcur = objcur->next) {
    if (strncmp(objcur->name, text, text_len) == 0)
      list[j++] = objcur->name;
  }

  if (j == 0) {
    g_free(list);
    list = NULL;
  } else {
    list[j] = NULL;
  }

  assert(j < get_obj_num() + 1);
  return list;
}

static char **
cmd_name_matching(const char *text)
{
  int i, j, text_len, file_len;
  struct objlist *objcur;
  char **list;
  struct mylist *file_list = NULL, *file_list_cur = NULL;

  file_list = get_exec_file_list();
  file_len = mylist_num(file_list);

  list = g_malloc((CMDNUM + CPCMDNUM + get_obj_num() + file_len + 1) * sizeof(*list));
  if (list == NULL)
    return NULL;

  text_len = strlen(text);
  j = 0;

  for (file_list_cur = file_list; file_list_cur != NULL;
       file_list_cur = file_list_cur->next) {
    if (strncmp(file_list_cur->str, text, text_len) == 0) {
      list[j++] = file_list_cur->str;
    }
  }

  for (i = 0; i < CMDNUM; i++) {
    if (strncmp(cmdtable[i].name, text, text_len) == 0) {
      list[j++] = cmdtable[i].name;
    }
  }

  for (i = 0; i < CPCMDNUM; i++) {
    if (strncmp(cpcmdtable[i], text, text_len) == 0) {
      list[j++] = cpcmdtable[i];
    }
  }

  for (objcur = chkobjroot(); objcur != NULL; objcur = objcur->next) {
    if (strncmp(objcur->name, text, text_len) == 0)
      list[j++] = objcur->name;
  }

  if (j == 0) {
    g_free(list);
    list = NULL;
  } else {
    list[j] = NULL;
  }

  assert(j < CMDNUM + CPCMDNUM + get_obj_num() + file_len + 1);
  return list;
}

struct mylist *
get_exec_file_list(void)
{
  char *path, *path_env, *next_ptr, *path_ptr;
  static struct mylist *list = NULL, *list_next = NULL;

  if (list != NULL) {
    return list;
    /*
       mylist_free(list);
       list = NULL;
     */
  }

  if ((path_env = getenv("PATH")) == NULL)
    return NULL;

  if ((path = path_ptr = g_strdup(path_env)) == NULL)
    return NULL;

  while ((next_ptr = strchr(path_ptr, ':')) != NULL) {
    *next_ptr = '\0';
    next_ptr++;

    list_next = get_file_list(path_ptr, S_IFREG, S_IXUSR);
    list = mylist_cat(list, list_next);
    path_ptr = next_ptr;
  }
  g_free(path);
  return list;
}

static struct mylist *
get_file_list(const char *path, int type, int mode)
{
  DIR *dir;
  struct dirent *ent;
  struct stat statbuf;
  struct mylist *list = NULL, *list_next = list;
  char *full_path_name;

  if ((dir = opendir(path)) == NULL) {
    return NULL;
  }
  while ((ent = readdir(dir)) != NULL) {
    if (my_sprintf(&full_path_name, "%s/%s", path, ent->d_name) < 0) {
      if (list != NULL)
	mylist_free(list);
      list = NULL;
      break;
    }
    stat(full_path_name, &statbuf);
    if ((statbuf.st_mode & type) && (statbuf.st_mode & mode)) {
      list_next = mylist_add(list_next, ent->d_name);
      if (list == NULL)
	list = list_next;
    }
  }

  closedir(dir);

  return list;
}

#define BUF_UNIT   256

static int
my_sprintf(char **str, char *format, ...)
{
  va_list arg;
  static int buf_size = BUF_UNIT;
  static char *buf = NULL;
  int len;

  if (buf == NULL && (buf = g_malloc(buf_size)) == NULL) {
    return -1;
  }

  va_start(arg, format);
  len = vsnprintf(buf, buf_size, format, arg) + 1;
  va_end(arg);
  if (len > buf_size) {
    char *tmp;
    int size;
    size = (len / BUF_UNIT + 1) * BUF_UNIT;
    tmp = g_realloc(buf, size);
    if (tmp == NULL) {
      return -1;
    }
    buf = tmp;
    buf_size = size;
    va_start(arg, format);
    len = vsnprintf(buf, buf_size, format, arg) + 1;
    va_end(arg);
  }
  *str = buf;
  return len;
}

static struct mylist *
mylist_add(struct mylist *parent, const char *text)
{
  struct mylist *list;
  int len;

  len = strlen(text) + 1;
  if ((list = g_malloc(sizeof(struct mylist) + len)) == NULL)
    return NULL;

  memcpy(list->str, text, len);
  list->next = NULL;
  list->len = len;
  if (parent != NULL)
    parent->next = list;

  return list;
}

static void
mylist_free(struct mylist *list)
{
  struct mylist *tmp;
  while (list != NULL) {
    tmp = list->next;
    g_free(list);
    list = tmp;
  }
}

static int
mylist_num(const struct mylist *list)
{
  int num = 0;
  while (list != NULL) {
    num++;
    list = list->next;
  }
  return num;
}

static struct mylist *
mylist_cat(struct mylist *list_top, struct mylist *list)
{
  struct mylist *list_ptr;

  if (list_top == NULL)
    return list;

  list_ptr = list_top;
  while (list_ptr->next != NULL)
    list_ptr = list_ptr->next;

  list_ptr->next = list;
  return list_top;
}

#endif
