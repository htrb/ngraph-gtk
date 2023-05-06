/*
 * $id: main.c,v 1.47 2010-03-04 08:30:17 hito Exp $
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
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif	/* HAVE_SYS_WAIT_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <locale.h>
#include <signal.h>

#if OSX
#if ! GTK_CHECK_VERSION(4, 0, 0)
#include <gtkosxapplication.h>
static GtkosxApplication *GtkMacIntegration = NULL;
#endif
#endif

char *DOCDIR, *NDATADIR, *ADDINDIR, *LIBDIR, *PLUGINDIR, *CONFDIR, *NLOCALEDIR, *BINDIR;

#include "dir_defs.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "nconfig.h"
#include "shell.h"
#include "gtk_action.h"
#include "init.h"

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#include "ox11menu.h"
#include "x11menu.h"
#include "ogra2x11.h"
#include "ogra2cairo.h"
#include "x11commn.h"
#include <assert.h>
static char **attempt_shell_completion(char *text, int start, int end);
char *HistoryFile = NULL;
#define HIST_SIZE 100
#define HIST_FILE "shell_history"
#endif	/* HAVE_READLINE_READLINE_H */

GtkApplication *GtkApp = NULL;

#define CSS_PATH RESOURCE_PATH "/css/ngraph.css"
#define SYSCONF "[Ngraph]"

static char *systemname, *locale;
static int consolefdout, consolefdin, ConsoleAc = FALSE;
static int consolecol = 80, consolerow = 25;
static int Initialized = FALSE;
static GThread *MainThread;

void *addobjectroot(void);
void *addint(void);
void *adddouble(void);
void *addio(void);
void *addstring(void);
void *addregexp(void);
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
void *addrectangle(void);
void *addarc(void);
void *addmark(void);
void *addtext(void);
void *addaxis(void);
void *addagrid(void);
void *addprm(void);
void *addpath(void);
void *addparameter(void);

void *addgra2gtk(void);
void *addmenu(void);
void *adddialog(void);
#if WINDOWS
void *addgra2emf(void);
#endif	/* WINDOWS */

void resizeconsole(int col, int row);

// XtAppContext Application=NULL;
char *AppName = "Ngraph", *AppClass = "Ngraph", *Home;
#if ! OSX
static const char *Snap = NULL;
#endif

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
static GMainLoop *MainLoop = NULL;

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
  addregexp,
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
  addarc,
  addpath,
  addmark,
  addtext,
  addparameter,
  addmenu,
  adddialog,
#if WINDOWS
  addgra2emf,
#endif
};

int
OpenApplication(void)
{
  return OpenDisplay;
}

GMainLoop *
main_loop(void)
{
  return MainLoop;
}

int
main_loop_is_running(void)
{
  return g_main_loop_is_running(MainLoop);
}

void
main_loop_run(void)
{
  g_main_loop_run(main_loop());
}

int
putconsole(const char *s)
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
printfconsole(const char *fmt, ...)
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
inputynconsole(const char *mes)
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
displaydialogconsole(const char *str)
{
  putconsole(str);
}

static void
displaystatusconsole(const char *str)
{
}

void
resizeconsole(int col, int row)
{
}

static char *Terminal = NULL;

#if WINDOWS
static HWND ConsoleHandle = NULL;

static int
check_console(int allocconsole)
{
  DWORD console_pid;
  pid_t pid;

  if (allocconsole) {
    return TRUE;
  }

  ConsoleHandle = GetConsoleWindow();
  if (ConsoleHandle == NULL) {
    return TRUE;
  }

  GetWindowThreadProcessId(ConsoleHandle, &console_pid);

  pid = getpid();

  return (pid != console_pid);
}

int
nallocconsole(void)
{
  if (ConsoleAc) {
    return TRUE;
  }

  if (ConsoleHandle) {
    ShowWindow(ConsoleHandle, SW_RESTORE);
    return TRUE;
  }

  return FALSE;
}

void
nfreeconsole(void)
{
  if (ConsoleAc) {
    return;
  }

  if (ConsoleHandle) {
    ShowWindow(ConsoleHandle, SW_HIDE);
  }
}

void
hide_console(void)
{
  HMENU menu;

  if (ConsoleHandle == NULL || ConsoleAc) {
    return;
  }

  menu = GetSystemMenu(ConsoleHandle, FALSE);
  RemoveMenu(menu, SC_CLOSE, MF_BYCOMMAND);

  nfreeconsole();
}

void
resotre_console(void)
{
  if (ConsoleHandle == NULL || ConsoleAc) {
    return;
  }

  GetSystemMenu(ConsoleHandle, TRUE);
  nallocconsole();
}
#else  /* WINDOWS */
static pid_t consolepid = -1;
static struct savedstdio consolesave;
static int consolefd[3];

static int
interruptconsole(void)
{
  return FALSE;
}

static void
reset_fifo(char *fifo_in, char *fifo_out)
{
  int fdi, fdo;

  fdi = nopen(fifo_in, O_WRONLY, 0);
  fdo = nopen(fifo_out, O_RDONLY, 0);
  if (fdi >= 0)
    nclose(fdi);

  if (fdo >= 0)
    nclose(fdo);
}


void
hide_console(void)
{
  /* do nothing */
}

void
resotre_console(void)
{
  /* do nothing */
}

static int
exec_console(char *fifo_in, char *fifo_out)
{
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    g_unlink(fifo_in);
    g_unlink(fifo_out);
    return 1;
  } else if (pid == 0) {
    pid = fork();
    if (pid == 0) {
      char buf[2049];
#if OSX
      snprintf(buf, sizeof(buf), "%s -e %s/terminal %s %s", Terminal, LIBDIR, fifo_in, fifo_out);
      system(buf);
      exit(0);
#else
      char **argv, *s2, *s;
      int len;
      snprintf(buf, sizeof(buf), "%s %s %s", Terminal, fifo_in, fifo_out);
      argv = NULL;
      s = buf;
      while ((s2 = getitok2(&s, &len, " \t")) != NULL) {
	if (Snap && g_str_has_suffix(s2, "/terminal")) {
	  g_free(s2);
	  s2 = g_strdup_printf("%s/terminal", LIBDIR);
	}
	arg_add(&argv, s2);
      }
      execvp(argv[0], argv);
#endif
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

  if (ConsoleAc)
    return FALSE;

  if (Terminal == NULL)
    return FALSE;

#if OSX
  {
    char *path;

    path = g_find_program_in_path(Terminal);
    if (path == NULL) {
      PutStderr("To use this function, please install XQuartz.");
      return FALSE;
    }
    g_free(path);
  }
#endif

  snprintf(fifo_in, sizeof(fifo_in) - 1, "/tmp/nterm1_%d", getpid());
  snprintf(fifo_out, sizeof(fifo_out) - 1, "/tmp/nterm2_%d", getpid());

  if (mkfifo(fifo_in, 0600)) {
    return FALSE;
  }

  if (mkfifo(fifo_out, 0600)) {
    g_unlink(fifo_in);
    return FALSE;
  }

  if (exec_console(fifo_in, fifo_out))
    return FALSE;

  fdi = nopen(fifo_in, O_RDONLY, 0);
  fdo = nopen(fifo_out, O_WRONLY, 0);
  g_unlink(fifo_in);
  g_unlink(fifo_out);

  sys = chkobject("system");
  getobj(sys, "name", 0, 0, NULL, &sysname);
  getobj(sys, "version", 0, 0, NULL, &version);
  snprintf(buf, sizeof(buf), "%c]2;Ngraph shell%c%s version %s. Script interpreter.\n",
	   0x1b, 0x07, sysname, version);

  if (write(fdo, buf, strlen(buf) + 1) < 0) {
    nclose(fdi);
    nclose(fdo);
    return FALSE;
  }

  nclose(fdo);

  for (i = 0; i < sizeof(ttyname) - 1; i++) {
    if (read(fdi, ttyname + i, 1) != 1)
      break;

    if (ttyname[i] == '\0')
      break;
  }
  ttyname[i] = '\0';

  if (i == 0) {
    nclose(fdi);
    return FALSE;
  }

  for (i = 0; i < sizeof(buf) - 1; i++) {
    if (read(fdi, buf + i, 1) != 1)
      break;

    if (buf[0] == '\0')
      break;
  }
  nclose(fdi);

  if (i == 0)
    return FALSE;

  buf[i] = '\0';

  fd[0] = nopen(ttyname, O_RDONLY, 0);
  if (fd[0] < 0) {
    goto ErrEnd;
  }

  fd[1] = nopen(ttyname, O_WRONLY, 0);
  if (fd[1] < 0) {
    goto ErrEnd;
  }

  fd[2] = nopen(ttyname, O_WRONLY, 0);
  if (fd[2] < 0) {
    goto ErrEnd;
  }

  consolepid = atoi(buf);

  consolefd[0] = dup(0);
  nclose(0);
  dup2(fd[0], 0);

  consolefd[1] = dup(1);
  nclose(1);
  dup2(fd[1], 1);

  consolefd[2] = dup(2);
  nclose(2);
  dup2(fd[2], 2);

  nclose(fd[0]);
  nclose(fd[1]);
  nclose(fd[2]);
  consolefdin = dup(0);
  consolefdout = dup(2);
  ConsoleAc = TRUE;
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
    nclose(fd[0]);
  }

  if (fd[1] < 0) {
    nclose(fd[1]);
  }

  if (fd[2] < 0) {
    nclose(fd[2]);
  }

  return FALSE;
}

void
nfreeconsole(void)
{
  if (ConsoleAc) {
    nclose(0);
    if (consolefd[0] != -1) {
      dup2(consolefd[0], 0);
      nclose(consolefd[0]);
    }
    nclose(1);
    if (consolefd[1] != -1) {
      dup2(consolefd[1], 1);
      nclose(consolefd[1]);
    }
    nclose(2);
    if (consolefd[2] != -1) {
      dup2(consolefd[2], 2);
      nclose(consolefd[2]);
    }

    kill(consolepid, SIGTERM);
    consolepid = -1;

    nclose(consolefdin);
    nclose(consolefdout);

    consolefdin = 0;
    consolefdout = 2;
    ConsoleAc = FALSE;
    loadstdio(&consolesave);
  }
}
#endif	/* WINDOWS */

void
nforegroundconsole()
{
}

static void
#ifdef HAVE_READLINE_READLINE_H
load_config(struct objlist *sys, N_VALUE *inst, int *allocconsole, int *history_size)
#else
load_config(struct objlist *sys, N_VALUE *inst, int *allocconsole)
#endif
{
  FILE *fp;
  char *tok, *str, *f1, *endptr, *s2;
  int len;
  long val;

  fp = openconfig(SYSCONF);
  if (fp == NULL) {
    return;
  }
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
#ifdef HAVE_READLINE_READLINE_H
    } else if (strcmp(tok, "history_size") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0' && val > 0) {
        *history_size = val;
      }
      g_free(f1);
#endif
    } else if (strcmp(tok, "terminal") == 0) {
      Terminal = getitok2(&s2, &len, "");
    } else {
      fprintf(stderr, "(%s): configuration '%s' in section %s is not used.\n", AppName, tok, SYSCONF);
    }
    g_free(tok);
    g_free(str);
  }
  closeconfig(fp);
}

#if WINDOWS
static int
set_dir_defs(char *app)
{
  char *utf8_name, *app_path, *tmp;

  utf8_name = n_locale_to_utf8(app);
  if (utf8_name == NULL) {
    return 1;
  }

  changefilename(utf8_name);
  tmp = g_canonicalize_filename(utf8_name, NULL);
  g_free(utf8_name);
  utf8_name = tmp;

  tmp = g_path_get_dirname(utf8_name);
  g_free(utf8_name);
  if (tmp == NULL) {
    return 1;
  }

  app_path = g_path_get_dirname(tmp);
  g_free(tmp);
  if (app_path == NULL) {
    return 1;
  }

  BINDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "bin");
  DOCDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/doc/ngraph-gtk");
  LIBDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "lib/ngraph-gtk");
  PLUGINDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "lib/ngraph-gtk/plugins");
  NDATADIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/ngraph-gtk");
  ADDINDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/ngraph-gtk/addin");
  CONFDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "etc/ngraph-gtk");
  NLOCALEDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/locale");

  g_free(app_path);

  return 0;
}
#elif OSX
static char *
get_libexec_dir(const char *app_path)
{
  char *libdir;
  GStatBuf buf;
  libdir = g_strdup_printf("%s%c%s", app_path, DIRSEP, "libexec/ngraph-gtk");
  g_stat(libdir, &buf);
  if (S_ISDIR(buf.st_mode)) {
    return libdir;
  }
  g_free(libdir);
  libdir = g_strdup_printf("%s%c%s", app_path, DIRSEP, "opt/ngraph-gtk/libexec/ngraph-gtk");
  return libdir;
}

static int
set_dir_defs(char *app)
{
  const char *app_contents;
  char *app_path, *bin_path;

  app_contents = g_getenv("NGRAPH_APP_CONTENTS");
  app_path = gtkosx_application_get_resource_path();
  if (app_contents) {
    bin_path = gtkosx_application_get_executable_path();
    LIBDIR = g_path_get_dirname(bin_path);
    g_free(bin_path);
  } else {
    bin_path = g_path_get_dirname(app_path);
    g_free(app_path);
    app_path = bin_path;
    LIBDIR = get_libexec_dir(app_path);
  }

  if (app_path == NULL) {
    return 1;
  }

  BINDIR = NULL;
  DOCDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/doc/ngraph-gtk");
  PLUGINDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "lib/ngraph-gtk/plugins");
  NDATADIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/ngraph-gtk");
  ADDINDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/ngraph-gtk/addin");
  CONFDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "etc/ngraph-gtk");
  NLOCALEDIR = g_strdup_printf("%s%c%s", app_path, DIRSEP, "share/locale");

  g_free(app_path);

  return 0;
}
#else
static int
set_dir_defs(char *app)
{
  const char *snap;

  snap = g_getenv("SNAP");
  if (snap) {
    BINDIR = g_strdup_printf("%s%c%s", snap, DIRSEP, "bin");
    Snap = snap;
  } else {
    BINDIR = NULL;
    snap = "";
  }

  DOCDIR = g_strdup_printf("%s%s", snap, _DOCDIR);
  LIBDIR = g_strdup_printf("%s%s", snap, _LIBDIR);
  PLUGINDIR = g_strdup_printf("%s%s", snap, _PLUGINDIR);
  NDATADIR = g_strdup_printf("%s%s", snap, _NDATADIR);
  ADDINDIR = g_strdup_printf("%s%s", snap, _ADDINDIR);
  CONFDIR = g_strdup_printf("%s%s", snap, _CONFDIR);
  NLOCALEDIR = g_strdup_printf("%s%s", snap, LOCALEDIR);

  return 0;
}
#endif

static void
set_path_env(char *homedir)
{
  const char *path;
  char *pathset;

  if (BINDIR == NULL) {
    return;
  }
  path = g_getenv("PATH");
  pathset = g_strdup_printf("%s%s%s%s%s%s%s%s%s%s%s",
			    homedir, PATHSEP,
			    ADDINDIR, PATHSEP,
			    LIBDIR, PATHSEP,
			    BINDIR, PATHSEP,
			    ".", PATHSEP,
			    CHK_STR(path));
  path_to_win(pathset);
  g_setenv("PATH", pathset, TRUE);
  g_setenv("NGRAPHLIB",  LIBDIR, TRUE);
  g_setenv("NGRAPHCONF", CONFDIR, TRUE);
  g_free(pathset);
}

const char *
n_getlocale(void)
{
  return locale;
}

static void
load_css(void)
{
  GtkCssProvider *css_provider;
  GdkDisplay *display;

  css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource(css_provider, CSS_PATH);
  display = gdk_display_get_default();
  if (display) {
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  }
}

void
n_application_ready(void)
{
#if OSX
#if ! GTK_CHECK_VERSION(4, 0, 0)
  if (GtkMacIntegration) {
    gtkosx_application_ready(GtkMacIntegration);
  }
#endif
#endif
}

#if OSX
#if ! GTK_CHECK_VERSION(4, 0, 0)
static gboolean
osx_open_file(GtkosxApplication *app, gchar *path, gpointer user_data)
{
  char *dir;
  if (path == NULL) {
    return TRUE;
  }
  dir = getdirname(path);
  if (dir) {
    nchdir(dir);
    g_free(dir);
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  LoadNgpFile(path, FALSE, "-f", NULL);
#else
  LoadNgpFile(path, FALSE, "-f");
#endif
  return TRUE;
}

static void
create_app_menu(GtkApplication *app)
{
  GtkBuilder *builder;
  GObject *app_menu;
  /* only for remove "Settings" menu item */
  builder = gtk_builder_new_from_resource(RESOURCE_PATH "/gtk/menus-appmenu.ui");
  app_menu = gtk_builder_get_object(builder, "app-menu");
  gtk_application_set_app_menu(app, G_MENU_MODEL(app_menu));
  g_object_unref(builder);
}
#endif
#endif

GtkApplication *
n_get_gtk_application(void)
{
  return GtkApp;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
set_prgname(int argc, char **argv)
{
  if (g_get_prgname() == NULL && argc > 0) {
    gchar *prgname;
    prgname = g_path_get_basename(argv[0]);
    g_set_prgname(prgname);
    g_free (prgname);
  }
}
#endif

int
is_main_thread(void)
{
  return MainThread == g_thread_self();
}

int
n_initialize(int *argc, char ***argv)
{
  char *homedir, *datadir, *docdir, *libdir, *plugindir, *confdir;
  const char *home;
  N_VALUE *inst;
  struct objlist *sys;
  unsigned int j;
  int allocconsole = FALSE;
#ifdef HAVE_READLINE_READLINE_H
  int history_size = HIST_SIZE;
#endif
  struct lconv *lconv;

  if (Initialized) {
    return 0;
  }

  MainThread = g_thread_self();

#if EOF == -1
  char_type_buf_init();
#endif

#if ! WINDOWS
  set_childhandler();
#endif	/* WINDOWS */

  set_dir_defs((*argv)[0]);

#ifdef HAVE_GETTEXT
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, NLOCALEDIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);
#endif	/* HAVE_GETTEXT */

#if GTK_CHECK_VERSION(4, 0, 0)
  MainLoop = g_main_loop_new (NULL, FALSE);
  OpenDisplay = gtk_init_check();
#else
  OpenDisplay = gtk_init_check(argc, argv);
#endif
  if (OpenDisplay) {
    GtkApp = gtk_application_new(APPLICATION_ID, G_APPLICATION_NON_UNIQUE);
    g_application_register(G_APPLICATION(GtkApp), NULL, NULL);
    setup_actions(GtkApp);
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  set_prgname(*argc, *argv);  /* this is necessary to use GtkRecentManager */
#endif
  g_set_application_name(AppName);
#if OSX
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GtkMacIntegration = gtkosx_application_get();
  g_signal_connect(GtkMacIntegration, "NSApplicationOpenFile", G_CALLBACK(osx_open_file), NULL);
  if (OpenDisplay) {
    create_app_menu(GtkApp);
  }
#endif
#endif

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

  if (OpenDisplay) {
    load_css();
  }

#if WINDOWS
  locale = g_win32_getlocale();
#else  /* WINDOWS */
  locale = setlocale(LC_ALL, NULL);
  locale = g_strdup(CHK_STR(locale));
#endif /* WINDOWS */
  /* to use a period as a decimal separator */
  lconv = localeconv();
  set_system_decimalsign(lconv->decimal_point);
  setlocale(LC_NUMERIC, "C");

  libdir = g_strdup(LIBDIR);
  if (libdir == NULL)
    exit(1);

  plugindir = g_strdup(PLUGINDIR);
  if (plugindir == NULL)
    exit(1);

  docdir = g_strdup(DOCDIR);
  if (docdir == NULL)
    exit(1);

  datadir = g_strdup(NDATADIR);
  if (datadir == NULL)
    exit(1);

  confdir = g_strdup(CONFDIR);
  if (confdir == NULL) {
    exit(1);
  }

  if ((home = g_getenv("HOME")) != NULL) {
    homedir = g_strdup_printf("%s/%s", home, HOME_DIR);
    if (homedir == NULL) {
      exit(1);
    }
    changefilename(homedir);
  } else {
    if ((home = g_get_user_config_dir()) != NULL) {
      homedir = g_strdup_printf("%s/%s", home, HOME_DIR);
      if (homedir == NULL) {
	exit(1);
      }
      changefilename(homedir);
    } else {
      homedir = g_strdup(confdir);
      if (homedir == NULL) {
	exit(1);
      }
    }
  }

  set_path_env(homedir);
  set_environ();

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
  if (_putobj(sys, "data_dir", inst, datadir))
    exit(1);
  if (_putobj(sys, "doc_dir", inst, docdir))
    exit(1);
  if (_putobj(sys, "lib_dir", inst, libdir))
    exit(1);
  if (_putobj(sys, "plugin_dir", inst, plugindir))
    exit(1);
  if (_putobj(sys, "home_dir", inst, homedir))
    exit(1);

  if (_getobj(sys, "conf_dir", inst, &confdir) == -1)
    exit(1);
  if (_getobj(sys, "data_dir", inst, &datadir) == -1)
    exit(1);
  if (_getobj(sys, "doc_dir", inst, &docdir) == -1)
    exit(1);
  if (_getobj(sys, "lib_dir", inst, &libdir) == -1)
    exit(1);
  if (_getobj(sys, "plugin_dir", inst, &plugindir) == -1)
    exit(1);
  if (_getobj(sys, "home_dir", inst, &homedir) == -1)
    exit(1);

  if (_getobj(sys, "name", inst, &systemname) == -1)
    exit(1);

  for (j = 0; j < sizeof(obj_add_func_ary) / sizeof(*obj_add_func_ary); j++) {
    if (obj_add_func_ary[j]() == NULL)
      exit(1);
  }

#ifdef HAVE_READLINE_READLINE_H
  load_config(sys, inst, &allocconsole, &history_size);
  rl_readline_name = "ngraph";
  rl_completer_word_break_characters = " \t\n\"'@><;|&({}`";
  rl_attempted_completion_function = (rl_completion_func_t *) attempt_shell_completion;
  rl_completion_entry_function = NULL;

  HistoryFile = g_strdup_printf("%s/%s", homedir, HIST_FILE);
  if (HistoryFile) {
    read_history(HistoryFile);
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

#if WINDOWS
  ConsoleAc = check_console(allocconsole);
  if (isatty(0) && isatty(1) && isatty(2)) {
    if (! allocconsole) {
      consolefdin = dup(0);
      consolefdout = dup(2);
    }
  }
#else
  if (isatty(0) && isatty(1) && isatty(2)) {
    ConsoleAc = TRUE;
    if (! allocconsole) {
      consolefdin = dup(0);
      consolefdout = dup(2);
    }
  } else {
    ConsoleAc = FALSE;
  }
#endif

  Initialized = TRUE;

  return 0;
}

void
n_save_shell_history(void)
{
#ifdef HAVE_READLINE_READLINE_H
  if (HistoryFile) {
    write_history(HistoryFile);
  }
#endif
}

void
n_finalize(void)
{
#ifdef HAVE_READLINE_READLINE_H
  if (HistoryFile != NULL) {
    n_save_shell_history();
    g_free(HistoryFile);
    HistoryFile = NULL;
  }
#endif

#if ! WINDOWS
  if (ConsoleAc && (consolepid != -1)) {
    nfreeconsole();
  }
#endif

  if (Terminal) {
    g_free(Terminal);
    Terminal = NULL;
  }
}

#ifdef HAVE_READLINE_READLINE_H
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

  if (in_command_position)
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

  /* If we don't have any state, make some. */
  if (!state) {
    static char *obj, *instances, *member, *val;
    struct objlist *objcur;

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
    char *t, *tt;

    t = g_strdup_printf("%.*s%s", first_char_loc, text, list[list_index]);
    tt = strdup(t);
    g_free(t);

    list_index++;
    return tt;
  }

  return NULL;
}

static char **
get_obj_member_list(struct objlist *objcur, char *member)
{
  char **list = (char **) NULL, *field;
  int i, j, len, n;

  n = chkobjfieldnum(objcur);
  list = g_malloc(sizeof(*list) * (n + 1));
  if (list == NULL) {
    return NULL;
  }

  len = strlen(member);
  j = 0;
  for (i = 0; i < n; i++) {
    field = chkobjfieldname(objcur, i);
    if (field == NULL) {
      continue;
    }
    if (strncmp(field, member, len) == 0) {
      list[j++] = field;
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
  for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
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
  int j, len;

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

  len = strlen(val);
  j = 0;
  for (fontmap = fontmaproot; fontmap != NULL; fontmap = fontmap->next) {
    if (strncmp(fontmap->fontalias, val, len) == 0) {
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
    char *t = strdup(list[list_index]);
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
  char *path, *next_ptr, *path_ptr;
  const char *path_env;
  static struct mylist *list = NULL, *list_next = NULL;

  if (list != NULL) {
    return list;
    /*
       mylist_free(list);
       list = NULL;
     */
  }

  if ((path_env = g_getenv("PATH")) == NULL)
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
  GDir *dir;
  const char *ent;
  GStatBuf statbuf;
  struct mylist *list = NULL, *list_next = list;
  char *full_path_name;

  if ((dir = g_dir_open(path, 0, NULL)) == NULL) {
    return NULL;
  }
  while ((ent = g_dir_read_name(dir)) != NULL) {
    full_path_name = g_strdup_printf("%s/%s", path, ent);
    if (full_path_name == NULL) {
      if (list != NULL) {
	mylist_free(list);
      }
      list = NULL;
      break;
    }
    nstat(full_path_name, &statbuf);
    if ((statbuf.st_mode & type) && (statbuf.st_mode & mode)) {
      list_next = mylist_add(list_next, ent);
      if (list == NULL) {
	list = list_next;
      }
    }
  }

  g_dir_close(dir);

  return list;
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
