/*
 * $Id: osystem.c,v 1.17 2010-03-04 08:30:16 hito Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <sys/types.h>
#include <unistd.h>
#include <gmodule.h>

#include "nstring.h"
#include "object.h"
#include "shell.h"
#include "ioutil.h"
#include "ntime.h"
#include "osystem.h"

#ifdef HAVE_LIBGSL
#include <gsl/gsl_errno.h>
#endif

#define NAME      "system"
#define PARENT    "object"
#define SYSNAME   "Ngraph"
#define TEMPN     "NGP"
#define COPYRIGHT "Copyright (C) 2003, Satoshi ISHIZAKA."
#define EMAIL     "ZXB01226@nifty.com"
#define WEB       "https://github.com/htrb/ngraph-gtk/"

#define ERRSYSNODIR    100
#define ERRSYSTMPFILE  101
#define ERRSYSSECURTY  102
#define ERRSYSMEM      103
#define ERRSYSNOMODULE 104
#define ERRSYSLOAD     105
#define ERRSYSLOADED   106
#define ERRSYSNOLOAD   107
#define ERRSYSINIT     108
#define ERRSYSINVALID  109
#define ERRSYSNOTEXECUTABLE 110
#define ERRSYSLOCKED   111
#define ERRSYSSMALLARGS 112

void resizeconsole(int col,int row);
extern int consolecol,consolerow;

static char *syserrorlist[]={
  "no such directory",
  "can't create temporary file",
  "the method is forbidden for the security",
  "cannot allocate enough memory",
  "no module name is specified.",
  "cannot load module",
  "the module is already loaded",
  "a module is not loaded",
  "cannot initialize the plugin",
  "invalid module",
  "not executable",
  "the module is locked",
  "too small number of arguments.",
};

#define ERRNUM (sizeof(syserrorlist) / sizeof(*syserrorlist))

struct ngraph_plugin {
  GModule *module;
  ngraph_plugin_exec exec;
  ngraph_plugin_open open;
  ngraph_plugin_close close;
  char *file;
  int lock;
};

static NHASH Plugins = NULL;

static DRAW_NOTIFY_FUNC DrawNotify = NULL;

void
system_set_draw_notify_func(DRAW_NOTIFY_FUNC func)
{
  DrawNotify = func;
}

void
system_draw_notify(void)
{
  if (DrawNotify) {
    DrawNotify(TRUE);
  }
}

static int
sysinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *wd;
  int expand, pid, status;
  char *exdir;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  expand=TRUE;
  if (_putobj(obj,"expand_file",inst,&expand)) return 1;
  exdir = g_strdup("./");
  if (exdir == NULL) return 1;
  if (_putobj(obj,"expand_dir",inst,exdir)) return 1;
  if (_putobj(obj,"name",inst,SYSNAME)) return 1;
  if (_putobj(obj,"version",inst,VERSION)) return 1;
  if (_putobj(obj,"copyright",inst,COPYRIGHT)) return 1;
  if (_putobj(obj,"e-mail",inst,EMAIL)) return 1;
  if (_putobj(obj,"web",inst,WEB)) return 1;
  if (_putobj(obj,"compiler",inst, COMPILER_NAME)) return 1;
  if (_putobj(obj,"GRAF",inst,"%Ngraph GRAF")) return 1;
  if (_putobj(obj,"temp_prefix",inst,TEMPN)) return 1;
  if ((wd=ngetcwd())==NULL) return 1;
  pid = getpid();
  if (_putobj(obj,"pid",inst,&pid)) return 1;
  status = 0;
  if (_putobj(obj, "shell_status", inst, &status)) return 1;
  if (_putobj(obj,"cwd",inst,wd)) {
    g_free(wd);
    return 1;
  }

#ifdef HAVE_LIBGSL
  gsl_set_error_handler_off();
#endif

  return 0;
}

static int
close_module(struct nhash *hash, void *data)
{
  struct ngraph_plugin *plugin;

  plugin = (struct ngraph_plugin *) hash->val.p;
  if (plugin->close) {
    plugin->close();
  }
  if (plugin->file) {
    g_free(plugin->file);
  }
  if (plugin->module) {
    g_module_close(plugin->module);
  }
  g_free(plugin);
  hash->val.p = NULL;

  return 0;
}

static int
sysdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct objlist *objcur;
  int i, n, status;
  char *s;
  struct narray *array;
  struct objlist *objectcur;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  nhash_each(Plugins, close_module, NULL);
  nhash_clear(Plugins);

  objcur=chkobjroot();
  while (objcur!=NULL) {
    if (objcur!=obj) {
      recoverinstance(objcur);
      for (i=chkobjlastinst(objcur);i>=0;i--) delobj(objcur,i);
    }
    objcur=objcur->next;
  }
  status = 0;
  _getobj(obj, "shell_status", inst, &status);

  _getobj(obj,"conf_dir",inst,&s);
  g_free(s);
  _getobj(obj,"data_dir",inst,&s);
  g_free(s);
  _getobj(obj,"doc_dir",inst,&s);
  g_free(s);
  _getobj(obj,"lib_dir",inst,&s);
  g_free(s);
  _getobj(obj,"plugin_dir",inst,&s);
  g_free(s);
  _getobj(obj,"home_dir",inst,&s);
  g_free(s);
  _getobj(obj,"cwd",inst,&s);
  g_free(s);
  _getobj(obj,"login_shell",inst,&s);
  g_free(s);
  _getobj(obj,"time",inst,&s);
  g_free(s);
  _getobj(obj,"date",inst,&s);
  g_free(s);
  _getobj(obj,"expand_dir",inst,&s);
  g_free(s);
  _getobj(obj,"temp_file",inst,&s);
  g_free(s);
  _getobj(obj,"temp_list",inst,&array);
  n = arraynum(array);
  if (n > 0) {
    char **data;

    data = arraydata(array);
    for (i = 0; i < n; i++) {
      g_unlink(data[i]);
    }
  }
  arrayfree2(array);

  objectcur=chkobjroot();
  while (objectcur!=NULL) {
    struct objlist *objectdel;
    objectdel=objectcur;
    objectcur=objectcur->next;

    if (objectdel->doneproc)
      objectdel->doneproc(objectdel,objectdel->local);

    if (objectdel->table_hash)
      nhash_free(objectdel->table_hash);

    g_free(objectdel);
  }
  g_free(inst);
  g_free(argv);

  nhash_free(Plugins);

  exit(status);
  return 0;
}

static int
syscwd(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *wd;
  const char *home;

  wd=argv[2];
  if (wd!=NULL) {
    if (nchdir(wd)!=0) {
      error2(obj,ERRSYSNODIR,wd);
      return 1;
    }
  } else {
    if ((home=g_getenv("HOME"))!=NULL) {
      if (nchdir(home)!=0) {
        error2(obj,ERRSYSNODIR,home);
        return 1;
      }
    }
  }
  if ((wd=ngetcwd())==NULL) return 1;
  g_free(argv[2]);
  argv[2]=wd;
  return 0;
}

static int
systime(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  time_t t;
  int style;

  t=time(NULL);
  style=*(int *)(argv[2]);
  g_free(rval->str);
  rval->str=ntime(&t,style);
  return 0;
}

static int
sysdate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  time_t t;
  int style;

  t=time(NULL);
  style=*(int *)(argv[2]);
  g_free(rval->str);
  rval->str=ndate(&t,style);
  return 0;
}

static int
systemp(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *pfx, *tmpfil;
  struct narray *array;
  int fd;

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"temp_prefix",inst,&pfx);
  _getobj(obj,"temp_list",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(char *)))==NULL) return 1;
    if (_putobj(obj,"temp_list",inst,array)) {
      arrayfree2(array);
      return 1;
    }
  }

  fd = n_mkstemp(NULL, pfx, &tmpfil);
  if (fd < 0) {
    error(obj, ERRSYSTMPFILE);
    return 1;
  }
  close(fd);

  arrayadd2(array,tmpfil);
  rval->str=tmpfil;
  return 0;
}

static int
sysunlink(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *tmpfil;
  struct narray *array;
  int num;
  char **data;

  _getobj(obj,"temp_list",inst,&array);
  if (array==NULL) return 0;
  num=arraynum(array);
  data=arraydata(array);
  if (num>0) {
    int i;
    tmpfil=(char *)argv[2];
    if (tmpfil==NULL) tmpfil=data[num-1];
    for (i=num-1;i>=0;i--)
      if (strcmp(tmpfil,data[i])==0) break;
    if (i>=0) {
      g_unlink(data[i]);
      arrayndel2(array,i);
    }
  }
  if (arraynum(array)==0) {
    arrayfree2(array);
    _putobj(obj,"temp_list",inst,NULL);
  }
  _getobj(obj,"temp_file",inst,&tmpfil);
  g_free(tmpfil);
  _putobj(obj,"temp_file",inst,NULL);
  return 0;
}

static int
syshideinstance(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char **objdata;
  int j,anum;

  array=(struct narray *)argv[2];
  if (array == NULL) {
    return 0;
  }
  anum=arraynum(array);
  objdata=arraydata(array);
  for (j=0;j<anum;j++) {
    struct objlist *obj2;
    obj2=getobject(objdata[j]);
    if ((obj2!=obj) && (obj2!=NULL)) hideinstance(obj2);
  }
  return 0;
}

static int
sysrecoverinstance(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char **objdata;
  int j,anum;

  array=(struct narray *)argv[2];
  if (array==NULL) {
    return 0;
  }
  anum=arraynum(array);
  objdata=arraydata(array);
  for (j=0;j<anum;j++) {
    struct objlist *obj2;
    obj2=getobject(objdata[j]);
    recoverinstance(obj2);
  }
  return 0;
}

static int
systemresize(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *iarray;
  int num;
  int *data;

  iarray=(struct narray *)argv[2];
  num=arraynum(iarray);
  if (num < 2) {
    return 0;
  }
  data=arraydata(iarray);
  resizeconsole(data[0], data[1]);
  return 0;
}

static int
get_symbol(GModule *module, const char *format, const char *name, gpointer *symbol)
{
  char *func;
  int r;

  func = g_strdup_printf(format, name);
  r = g_module_symbol(module, func, symbol);
  g_free(func);

  return ! r;
}

static char *
get_basename(const char *str)
{
  char *basename;
  int i;

  basename = getbasename(str);
  if (basename == NULL) {
    return NULL;
  }
  for (i = 0; basename[i]; i++) {
    if (basename[i] == '.') {
      basename[i] = '\0';
      break;
    }
  }

  return basename;
}

static char *
get_plugin_name(const char *name)
{
  struct objlist *sysobj;
  char *module_file, *plugin_path;

  plugin_path = NULL;
  sysobj = getobject("system");
  getobj(sysobj, "plugin_dir", 0, 0, NULL, &plugin_path);

  module_file = g_module_build_path(plugin_path, name);

  return module_file;
}

static int
load_plugin_sub(struct objlist *obj, const char *name, struct ngraph_plugin *plugin)
{
  GModule *module;
  void *loaded;
  ngraph_plugin_open np_open;
  ngraph_plugin_close np_close;
  int r;
  char *module_file;

  module = NULL;
  module_file = NULL;

  r = nhash_get_ptr(Plugins, name, &loaded);
  if (r == 0 && loaded) {
    error2(obj, ERRSYSLOADED, name);
    goto ErrorExit;
  }

  module_file = get_plugin_name(name);
  module = g_module_open(module_file, 0);
  if (module == NULL) {
    putstderr(g_module_error());
    error2(obj, ERRSYSLOAD, name);
    goto ErrorExit;
  }

  np_open = NULL;
  get_symbol(module, "ngraph_plugin_open_%s", name, (gpointer *) &np_open);

  np_close = NULL;
  get_symbol(module, "ngraph_plugin_close_%s", name, (gpointer *) &np_close);

  r = nhash_set_ptr(Plugins, name, plugin);
  if (r) {
    error(obj, ERRSYSMEM);
    goto ErrorExit;
  }


  plugin->file = module_file;
  plugin->module = module;
  plugin->exec = NULL;
  plugin->open = np_open;
  plugin->close = np_close;

  return 0;

 ErrorExit:
  if (module) {
    g_module_close(module);
  }

  g_free(module_file);

  return 1;
}

static struct ngraph_plugin *
get_plugin_from_name(const char *name)
{
  void *ptr;
  int r;

  r = nhash_get_ptr(Plugins, name, &ptr);
  if (r) {
    return NULL;
  }

  return ptr;
}

struct ngraph_plugin *
load_plugin(struct objlist *obj, const char *arg, int *rval)
{
  char *name;
  struct ngraph_plugin *plugin;

  if (arg == NULL) {
    error(obj, ERRSYSNOMODULE);
    return NULL;
  }

  name = get_basename(arg);
  if (name == NULL) {
    error(obj, ERRSYSNOMODULE);
    return NULL;
  }
  plugin = get_plugin_from_name(name);

  if (plugin) {
    error2(obj, ERRSYSLOADED, name);
    g_free(name);
    return NULL;
  }

  plugin = g_malloc0(sizeof(struct ngraph_plugin));
  if (plugin == NULL) {
    g_free(name);
    error(obj, ERRSYSMEM);
    return NULL;
  }

  if (load_plugin_sub(obj, name, plugin)) {
    g_free(name);
    g_free(plugin);
    return NULL;
  }
  g_free(name);

  if (plugin->open) {
    int r;
    r = plugin->open();
    if (rval) {
      *rval = r;
    }
    if (r) {
      error2(obj, ERRSYSINIT, arg);
      return NULL;
    }
  }

  return plugin;
}

static int
system_plugin_load(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;

  rval->i = 0;
  plugin = load_plugin(obj, argv[2], &rval->i);

  return (plugin) ? 0 : 1;
}

static int
system_plugin_check(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *name, *module_file;

  name = get_basename(argv[2]);
  if (name == NULL) {
    return 1;
  }

  module_file = get_plugin_name(name);
  g_free(name);

  if (module_file == NULL) {
    return 1;
  }

  if (naccess(module_file, R_OK)) {
    g_free(module_file);
    return 1;
  }

  g_free(module_file);

  return 0;
}

static void
free_argv(int argc, char **argv)
{
  int i;

  for (i = 0; i < argc; i ++) {
    if (argv[i]) {
      g_free(argv[i]);
      argv[i] = NULL;
    }
  }
  g_free(argv);
}

static char **
allocate_argv(int argc, char * const *argv)
{
  int i, new_argc;
  char **new_argv;

  new_argc = argc + 1;
  new_argv = g_malloc0(sizeof(*new_argv) * new_argc);
  if (new_argv == NULL) {
    return NULL;
  }

  for (i = 0; i < argc; i++) {
    new_argv[i] = g_strdup(argv[i]);
    if (new_argv[i] == NULL) {
      free_argv(new_argc, new_argv);
      return NULL;
    }
  }

  new_argv[i] = NULL;

  return new_argv;
}

static int
system_plugin_exec(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;
  char **new_argv;
  int r;

  rval->i = 0;

  if (argc < 4) {
    error2(obj, ERRSYSSMALLARGS, argv[1]);
    return 0;
  }

  if (get_security()) {
    error(obj, ERRSYSSECURTY);
    return 1;
  }

  plugin = get_plugin_from_name(argv[2]);
  if (plugin == NULL) {
    plugin = load_plugin(obj, argv[2], NULL);
    if (plugin == NULL) {
      return 1;
    }
  }

  if (plugin->exec == NULL) {
    error2(obj, ERRSYSNOTEXECUTABLE, argv[2]);
    return 1;
  }

  if (plugin->lock) {
    error2(obj, ERRSYSLOCKED, argv[2]);
    return 1;
  }

  new_argv = allocate_argv(argc - 3, argv + 3);
  if (new_argv == NULL) {
    error(obj, ERRSYSMEM);
    return 1;
  }

  plugin->lock = TRUE;
  r = plugin->exec(argc - 3, new_argv);
  plugin->lock = FALSE;
  rval->i = r;

  free_argv(argc - 3, new_argv);

  return r;
}

static int
system_plugin_get_module(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;

  g_free(rval->str);
  rval->str = NULL;

  if (argv[2] == NULL) {
    return 1;
  }

  plugin = get_plugin_from_name(argv[2]);
  if (plugin == NULL) {
    error(obj, ERRSYSNOLOAD);
    return 1;
  }

  if (plugin->file) {
    rval->str = g_strdup(plugin->file);
  } else {
    rval->str = g_strdup("internal");
  }

  return 0;
}

static int
list_module(struct nhash *hash, void *data)
{
  struct narray *array;
  const char *name;

  array = (struct narray *) data;
  name = hash->key;

  arrayadd2(array, name);

  return 0;
}

static int
system_plugin_modules(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;

  arrayfree2(rval->array);
  rval->array = NULL;

  array = arraynew(sizeof(char *));
  if (array == NULL) {
    error(obj, ERRSYSMEM);
    return 1;
  }
  rval->array = array;

  nhash_each(Plugins, list_module, array);

  return 0;
}

int
system_set_exec_func(const char *name, ngraph_plugin_exec func)
{
  struct ngraph_plugin *plugin;

  if (name == NULL || func == NULL) {
    return 1;
  }

  plugin = get_plugin_from_name(name);
  if (plugin == NULL) {
    int r;
    plugin = g_malloc0(sizeof(struct ngraph_plugin));
    if (plugin == NULL) {
      return 1;
    }
    r = nhash_set_ptr(Plugins, name, plugin);
    if (r) {
      return 1;
    }
  }

  plugin->exec = func;
  return 0;
}

static struct objtable nsystem[] = {
  {"init",NVFUNC,NEXEC,sysinit,NULL,0},
  {"done",NVFUNC,NEXEC,sysdone,NULL,0},
  {"name",NSTR,NREAD,NULL,NULL,0},
  {"version",NSTR,NREAD,NULL,NULL,0},
  {"copyright",NSTR,NREAD,NULL,NULL,0},
  {"e-mail",NSTR,NREAD,NULL,NULL,0},
  {"web",NSTR,NREAD,NULL,NULL,0},
  {"compiler",NSTR,NREAD,NULL,NULL,0},
  {"login_shell",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"cwd",NSTR,NREAD|NWRITE,syscwd,NULL,0},
  {"ignore_path",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"expand_file",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"expand_dir",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"GRAF",NSTR,NREAD,NULL,NULL,0},
  {"temp_prefix",NSTR,NREAD,NULL,NULL,0},
  {"conf_dir",NSTR,NREAD,NULL,NULL,0},
  {"data_dir",NSTR,NREAD,NULL,NULL,0},
  {"doc_dir",NSTR,NREAD,NULL,NULL,0},
  {"lib_dir",NSTR,NREAD,NULL,NULL,0},
  {"plugin_dir",NSTR,NREAD,NULL,NULL,0},
  {"home_dir",NSTR,NREAD,NULL,NULL,0},
  {"pid",NINT,NREAD,NULL,NULL,0},
  {"shell_status",NINT,NREAD,NULL,NULL,0},
  {"time",NSFUNC,NREAD|NEXEC,systime,"i",0},
  {"date",NSFUNC,NREAD|NEXEC,sysdate,"i",0},
  {"temp_file",NSFUNC,NREAD|NEXEC,systemp,"",0},
  {"temp_list",NSARRAY,NREAD,NULL,NULL,0},
  {"unlink_temp_file",NVFUNC,NREAD|NEXEC,sysunlink,NULL,0},
  {"hide_instance",NVFUNC,NREAD|NEXEC,syshideinstance,"sa",0},
  {"recover_instance",NVFUNC,NREAD|NEXEC,sysrecoverinstance,"sa",0},
  {"resize",NVFUNC,NREAD|NEXEC,systemresize,"ia",0},
  {"plugin_check", NIFUNC, NREAD|NEXEC, system_plugin_check, "s", 0},
  {"plugin_load", NIFUNC, NREAD|NEXEC, system_plugin_load, "s", 0},
  {"plugin_exec",NIFUNC, NREAD|NEXEC, system_plugin_exec, NULL, 0},
  {"plugin_module",NSFUNC, NREAD|NEXEC, system_plugin_get_module, "s", 0},
  {"plugins",NSAFUNC, NREAD|NEXEC, system_plugin_modules, "", 0},
};

#define TBLNUM (sizeof(nsystem) / sizeof(*nsystem))

void *
addsystem()
/* addsystem() returns NULL on error */
{
  Plugins = nhash_new();
  if (Plugins == NULL) {
    return NULL;
  }
  return addobject(NAME,NULL,PARENT,VERSION,TBLNUM,nsystem,ERRNUM,syserrorlist,NULL,NULL);
}
