/*
 * $Id: main.c,v 1.47 2010-03-04 08:30:17 hito Exp $
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

#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <limits.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "ngraph.h"

#define INIT_SCRIPT "Ngraph.nsc"

static void
exec_init_script(int argc, char **argv, struct objlist *obj, int id)
{
  char *inifile;
  ngraph_arg *sarray, sarg;
  int ofst, r;

  ofst = 1;

  if (argc > 1 && strcmp(argv[1], "-i") == 0) {
    ofst++;
    if (argc > 2) {
#if WINDOWS
      int i;
#endif  /* WINDOWS */
      inifile = ngraph_strdup(argv[2]);
      if (inifile == NULL) {
	exit(1);
      }
#if WINDOWS
      for (i = 0; inifile[i] != '\0'; i++) {
	if (inifile[i] == '\\') {
	  inifile[i] = '/';
	}
      }
#endif  /* WINDOWS */
      ofst++;
    } else {
      inifile = NULL;
    }
  } else {
    inifile = ngraph_get_init_file(INIT_SCRIPT);
  }

  if (inifile) {
    int i, n;
    n = argc - ofst + 1;
    sarray = ngraph_malloc(sizeof(*sarray) + sizeof(ngraph_value) * n);
    if (sarray == NULL) {
      exit(1);
    }

    sarray->num = n;
    sarray->ary[0].str = inifile;
    for (i = 1; i < n; i++) {
      sarray->ary[i].str = argv[i - 1 + ofst];
    }
  } else {
    sarray = ngraph_malloc(sizeof(*sarray));
    sarray->num = 0;
  }

  sarg.num = 1;
  sarg.ary[0].ary = sarray;

  r = ngraph_object_exe(obj, "shell", id, &sarg);

  ngraph_free(sarray);
  ngraph_free(inifile);

  if (r) {
    exit(1);
  }
}

static char*
get_login_shell(struct objlist *sys)
{
  char *loginshell;
  ngraph_arg arg;
  ngraph_returned_value rval;

  arg.num = 0;
  arg.ary[0].str = NULL;

  if (ngraph_object_get(sys, "login_shell", 0, &arg, &rval) < 0) {
    exit(1);
  }

  if (rval.str) {
    loginshell = ngraph_strdup(rval.str);
  } else {
    loginshell = NULL;
  }

  return loginshell;
}

#ifdef __APPLE__
static void
osx_set_env(const char *app)
{
  const char *bundle;
  char rpath[PATH_MAX];
  char bundle_contents[PATH_MAX], bundle_res[PATH_MAX], bundle_lib[PATH_MAX], bundle_bin[PATH_MAX], bundle_dat[PATH_MAX], bundle_etc[PATH_MAX];
  char xdg[PATH_MAX], ruby[PATH_MAX], pango[PATH_MAX], pixbuf[PATH_MAX], immodule[PATH_MAX];
  const char *home;
  int r;
  struct stat stat_buf;

  realpath(app, rpath);
  bundle = dirname(dirname(dirname(rpath)));
  snprintf(bundle_contents, PATH_MAX, "%s/%s", bundle, "Contents");

  r = stat(bundle_contents, &stat_buf);
  if (r || ! S_ISDIR(stat_buf.st_mode)) {
    return;
  }

  snprintf(bundle_res, PATH_MAX, "%s/%s", bundle_contents, "Resources");
  snprintf(bundle_lib, PATH_MAX, "%s/%s", bundle_res, "lib");
  snprintf(bundle_bin, PATH_MAX, "%s/%s", bundle_res, "bin");
  snprintf(bundle_dat, PATH_MAX, "%s/%s", bundle_res, "share");
  snprintf(bundle_etc, PATH_MAX, "%s/%s", bundle_res, "etc");

  setenv("DYLD_LIBRARY_PATH", bundle_lib, 1);
  snprintf(xdg, PATH_MAX, "%s/%s", bundle_etc, "xdg");
  setenv("XDG_CONFIG_DIRS", xdg, 1);
  setenv("XDG_DATA_DIRS", bundle_dat, 1);
  setenv("GTK_DATA_PREFIX", bundle_res, 1);
  setenv("GTK_EXE_PREFIX", bundle_res, 1);
  setenv("GTK_PATH", bundle_res, 1);
  setenv("NGRAPH_APP_CONTENTS", bundle_contents, 1);
  snprintf(ruby, PATH_MAX, "%s/%s", bundle_lib, "ngraph-gtk/ruby");
  setenv("RUBYLIB", ruby, 1);
  snprintf(pango, PATH_MAX, "%s/%s", bundle_etc, "pango/pangorc");
  setenv("PANGO_RC_FILE", pango, 1);
  setenv("PANGO_SYSCONFDIR", bundle_etc, 1);
  setenv("PANGO_LIBDIR", bundle_lib, 1);
  snprintf(pixbuf, PATH_MAX, "%s/%s", bundle_lib, "gdk-pixbuf-2.0/2.10.0/loaders.cache");
  setenv("GDK_PIXBUF_MODULE_FILE", pixbuf, 1);
  snprintf(immodule, PATH_MAX, "%s/%s", bundle_etc, "gtk-3.0/gtk.immodules");
  setenv("GTK_IM_MODULE_FILE", immodule, 1);
  home = getenv("HOME");
  if (home) {
    chdir(home);
  }
}
#endif

int
main(int argc, char **argv)
{
  char *loginshell;
  struct objlist *sys, *obj;
  int id;

#ifdef __APPLE__
  /* remove MacOS session identifier from the command line args */
  int i, newargc = 0;
  for (i = 0; i < argc; i++) {
    if (strncmp (argv[i], "-psn_", 5)){
      argv[newargc] = argv[i];
      newargc++;
    }
  }
  if (argc > newargc) {
    argv[newargc] = NULL; /* glib expects NULL terminated array */
    argc = newargc;
  }
  osx_set_env(argv[0]);
#endif
  if (ngraph_initialize(&argc, &argv)) {
    exit(1);
  }

  sys = ngraph_get_object("system");
  obj = ngraph_get_object("shell");
  if (obj == NULL || sys == NULL) {
    exit(1);
  }

  id = ngraph_object_new(obj);
  if (id < 0) {
    exit(1);
  }

  exec_init_script(argc, argv, obj, id);

  loginshell = get_login_shell(sys);

#if 1
  if (loginshell) {
    ngraph_exec_loginshell(loginshell, obj, id);
    ngraph_free(loginshell);
  }
#else
  do {
    ngraph_value val;

    val.str = NULL;
    if (ngraph_object_put(sys, "login_shell", 0, &val) < 0) {
      exit(1);
    }

    ngraph_exec_loginshell(loginshell, obj, id);

    if (loginshell) {
      ngraph_free(loginshell);
    }

    loginshell = get_login_shell(sys);
  } while (loginshell);
#endif
  ngraph_finalize();

  ngraph_object_del(sys, 0);
  return 0;
}
