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

#include "ngraph.h"

#define INIT_SCRIPT "Ngraph.nsc"

static void
exec_init_script(int argc, char **argv, struct objlist *obj, int id)
{
  char *inifile;
  ngraph_arg *sarray, sarg;
  int i, j, n, r;

  i = 1;

  if (argc > 1 && strcmp(argv[1], "-i") == 0) {
    i++;
    if (argc > 2) {
      inifile = ngraph_strdup(argv[2]);
      if (inifile == NULL) {
	exit(1);
      }
#ifdef WINDOWS
      for (j = 0; inifile[i] != '\0'; i++) {
	if (inifile[i] == '\\') {
	  name[i] = '/';
	}
      }
#endif  /* WINDOWS */
      i++;
    } else {
      inifile = NULL;
    }
  } else {
    inifile = ngraph_get_init_file(INIT_SCRIPT);
  }

  if (inifile) {
    n = argc - i + 1;
    sarray = ngraph_malloc(sizeof(*sarray) + sizeof(ngraph_value) * n);
    if (sarray == NULL) {
      exit(1);
    }

    sarray->num = n;
    sarray->ary[0].str = inifile;
    for (j = 1; j < n; j++) {
      sarray->ary[j].str = argv[j - 1 + i];
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

int
main(int argc, char **argv)
{
  char *loginshell;
  struct objlist *sys, *obj;
  int id;

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
