#include "common.h"

#include <glib.h>
#include <gmodule.h>

#define USE_HASH 1

#include "ngraph.h"
#include "object.h"
#include "mathfn.h"
#include "shell.h"

#define NAME "plugin"
#define PARENT "object"
#define OVERSION "1.00.00"
#define MAXCLINE 256

#define ERRMEM 100
#define ERRRUN 101
#define ERRNOCL 102
#define ERRFILEFIND 103
#define ERRLOAD 104
#define ERRLOADED 105
#define ERRNOLOAD 106
#define ERRINIT 107
#define ERRSECURTY 108
#define ERRINVALID 109

static char *sherrorlist[] = {
  "cannot allocate enough memory",
  "already running.",
  "no command string is specified.",
  "no such file",
  "cannot load module",
  "the module is already loaded",
  "a module is not loaded",
  "cannot initialize the plugin",
  "the method is forbidden for the security",
  "invalid module",
};

struct ngraph_plugin {
  GModule *module;
  ngraph_plugin_exec exec;
  ngraph_plugin_open open;
  ngraph_plugin_close close, interrupt;
  int deleted, id;
  void *user_data;
  char *name, *file;
};

struct shlocal {
  struct ngraph_plugin *plugin;
  int lock;
};
static NHASH Plugins = NULL;

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

static int
load_plugin(struct objlist *obj, N_VALUE *inst, const char *name, struct ngraph_plugin *plugin)
{
  GModule *module;
  void *loaded;
  ngraph_plugin_exec np_exec;
  ngraph_plugin_open np_open;
  ngraph_plugin_close np_close, np_interrupt;
  int r, i, id;
  char *basename, *module_file, *plugin_path;
  struct objlist *sysobj;

  module = NULL;
  module_file = NULL;
  basename = NULL;

  basename = getbasename(name);
  for (i = 0; basename[i]; i++) {
    if (basename[i] == '.') {
      basename[i] = '\0';
      break;
    }
  }

  r = nhash_get_ptr(Plugins, basename, &loaded);
  if (r == 0 && loaded) {
    error2(obj, ERRLOADED, basename);
    goto ErrorExit;
  }

  plugin_path = NULL;
  sysobj = getobject("system");
  getobj(sysobj, "plugin_dir", 0, 0, NULL, &plugin_path);

  module_file = g_module_build_path(plugin_path, basename);
  module = g_module_open(module_file, 0);
  if (module == NULL) {
    error2(obj, ERRLOAD, name);
    putstderr(g_module_error());
    goto ErrorExit;
  }

  np_exec = NULL;
  r = get_symbol(module, "ngraph_plugin_exec_%s", basename, (gpointer *) &np_exec);
  if (r) {
    error2(obj, ERRINVALID, name);
    goto ErrorExit;
  }

  np_open = NULL;
  get_symbol(module, "ngraph_plugin_open_%s", basename, (gpointer *) &np_open);

  np_close = NULL;
  get_symbol(module, "ngraph_plugin_close_%s", basename, (gpointer *) &np_close);

  np_interrupt = NULL;
  get_symbol(module, "ngraph_plugin_interrupt_%s", basename, (gpointer *) &np_interrupt);


  r = nhash_set_ptr(Plugins, basename, plugin);
  if (r) {
    error(obj, ERRMEM);
    goto ErrorExit;
  }

  _getobj(obj, "id", inst, &id);

  plugin->name = basename;
  plugin->file = module_file;
  plugin->module = module;
  plugin->exec = np_exec;
  plugin->open = np_open;
  plugin->close = np_close;
  plugin->interrupt = np_interrupt;
  plugin->id = id;

  return 0;

 ErrorExit:
  if (module) {
    g_module_close(module);
  }

  g_free(module_file);
  g_free(basename);

  return 1;
}

static struct ngraph_plugin *
get_plugin(struct objlist *obj, N_VALUE *inst)
{
  const char *name;
  void *ptr;
  int r;

  _getobj(obj, "module_name", inst, &name);
  if (name == NULL) {
    return NULL;
  }

  r = nhash_get_ptr(Plugins, name, &ptr);
  if (r) {
    return NULL;
  }

  return ptr;
}

static int 
plugin_open(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *name;
  struct ngraph_plugin *plugin;
  int r;

  rval->i = 0;

  if (argv[2] == NULL) {
    error(obj, ERRNOCL);
    return 1;
  }

  name = argv[2];

  plugin = get_plugin(obj, inst);
#ifdef WINDOWS
  if (plugin) {
    if (plugin->id >= 0) {
      error2(obj, ERRLOADED, plugin->name);
      return 1;
    }
  } else {
    plugin = g_malloc0(sizeof(struct ngraph_plugin));
    if (plugin == NULL) {
      error(obj, ERRMEM);
      return 1;
    }

    if (load_plugin(obj, inst, name, plugin)) {
      g_free(plugin);
      return 1;
    }
  }
#else
  if (plugin) {
    error2(obj, ERRLOADED, plugin->name);
    return 1;
  }

  plugin = g_malloc0(sizeof(struct ngraph_plugin));
  if (plugin == NULL) {
    error(obj, ERRMEM);
    return 1;
  }

  if (load_plugin(obj, inst, name, plugin)) {
    g_free(plugin);
    return 1;
  }
#endif

  _putobj(obj, "module_name", inst, plugin->name);
  _putobj(obj, "module_file", inst, plugin->file);

  if (plugin->open) {
    r = plugin->open(plugin);
    rval->i = r;
    if (r) {
      error2(obj, ERRINIT, name);
      g_free(plugin);
      return 1;
    }
  }

  return 0;
}

static int
close_plugin(struct objlist *obj, N_VALUE *inst, struct ngraph_plugin *plugin)
{
  if (plugin->module == NULL) {
    error(obj, ERRNOLOAD);
    return 1;
  }

#ifdef WINDOWS
  plugin->id = -1;
#else
  if (plugin->close) {
    plugin->close(plugin);
  }
  g_module_close(plugin->module);
  plugin->module = NULL;
  plugin->exec = NULL;
  plugin->open = NULL;
  plugin->close = NULL;
#endif

  if (inst) {
    _putobj(obj, "module_name", inst, NULL);
    _putobj(obj, "module_file", inst, NULL);
  }

#ifndef WINDOWS
  if (plugin->name) {
    g_free(plugin->name);
    plugin->name = NULL;
  }

  if (plugin->file) {
    g_free(plugin->file);
    plugin->name = NULL;
  }

  g_free(plugin);
  nhash_set_ptr(Plugins, plugin->name, NULL);
#endif

  return 0;
}

static int
plugin_close(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;
  int lock;

  plugin = get_plugin(obj, inst);
  if (plugin == NULL) {
    error(obj, ERRNOLOAD);
    return 1;
  }

  _getobj(obj, "lock", inst, &lock);
  if (lock) {
    error(obj, ERRRUN);
    return 1;
  }

  close_plugin(obj, inst, plugin);

  return 0;
}

static int
plugin_lock(struct objlist *obj, N_VALUE *inst, int state)
{
  int lock;

  lock = state;
  return _putobj(obj, "lock", inst, &lock);
}

static int
plugin_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  if (plugin_lock(obj, inst, FALSE)) {
    return 1;
  }

  return 0;
}

static int
plugin_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;
  int lock;

  plugin = get_plugin(obj, inst);

  if (plugin) {
    _getobj(obj, "lock", inst, &lock);
    if (lock) {
      if (plugin->interrupt) {
	plugin->interrupt(plugin);
      }
      plugin->deleted = TRUE;
    } else {
      close_plugin(obj, inst, plugin);
    }
  }

  _putobj(obj, "module_name", inst, NULL);
  _putobj(obj, "module_file", inst, NULL);

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
allocate_argv(const char *name, int argc, char * const *argv)
{
  int i, new_argc;
  char **new_argv;

  new_argc = argc + 2;
  new_argv = g_malloc0(sizeof(*new_argv) * new_argc);
  if (new_argv == NULL) {
    return NULL;
  }

  new_argv[0] = g_strdup(name);
  if (new_argv[0] == NULL) {
    free_argv(new_argc, new_argv);
    return NULL;
  }

  for (i = 0; i < argc; i++) {
    new_argv[i + 1] = g_strdup(argv[i]);
    if (new_argv[i + 1] == NULL) {
      free_argv(new_argc, new_argv);
      return NULL;
    }
  }

  new_argv[i + 1] = NULL;

  return new_argv;
}


static int
plugin_exec(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;
  char **new_argv;
  int r, lock;

  rval->i = 0;

  plugin = get_plugin(obj, inst);
  if (plugin == NULL) {
    error(obj, ERRNOLOAD);
    return 1;
  }

  if (get_security()) {
    error2(obj, ERRSECURTY, plugin->name);
    return 1;
  }

  if (plugin->module == NULL ||
      plugin->exec == NULL) {
    error(obj, ERRNOLOAD);
    return 1;
  }

  _getobj(obj, "lock", inst, &lock);
  if (lock) {
    error(obj, ERRRUN);
    return 1;
  }

  new_argv = allocate_argv(plugin->name, argc - 2, argv + 2);
  if (new_argv == NULL) {
    error(obj, ERRMEM);
    return 1;
  }

  plugin_lock(obj, inst, TRUE);

  r = plugin->exec(plugin, argc - 1, new_argv);
  rval->i = r;

  if (plugin->deleted) {
    close_plugin(obj, NULL, plugin);
  } else { 
    plugin_lock(obj, inst, FALSE);
  }

  free_argv(argc - 1, new_argv);

  return r;
}

#define ERRNUM (sizeof(sherrorlist) / sizeof(*sherrorlist))

static struct objtable Plugin[] = {
  {"init", NVFUNC, NEXEC, plugin_init, NULL, 0},
  {"done", NVFUNC, NEXEC, plugin_done, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"exec", NIFUNC, NREAD|NEXEC, plugin_exec, NULL, 0},
  {"open", NIFUNC, NREAD|NEXEC, plugin_open, "s", 0},
  {"close", NVFUNC, NREAD|NEXEC, plugin_close, "", 0},
  {"module_name", NSTR, NREAD, NULL, NULL, 0},
  {"module_file", NSTR, NREAD, NULL, NULL, 0},
  {"lock", NBOOL, NREAD, NULL, NULL, 0},
};

#define TBLNUM (sizeof(Plugin) / sizeof(*Plugin))

void *
addplugin(void)
{
  if (Plugins == NULL) {
    Plugins = nhash_new();
    if (Plugins == NULL) {
      return NULL;
    }
  }

  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, Plugin, ERRNUM, sherrorlist, NULL, NULL);
}

/*****************************************************/

void
ngraph_plugin_set_user_data(struct ngraph_plugin *plugin, void *user_data)
{
  if (plugin == NULL) {
    return;
  }
  plugin->user_data = user_data;
}

void *
ngraph_plugin_get_user_data(struct ngraph_plugin *plugin)
{
  if (plugin == NULL) {
    return NULL;
  }
  return plugin->user_data;
}

union ngraph_val {
  int i;
  double d;
  char *str;
  struct narray *ary;
};

static struct narray *
allocate_iarray(ngraph_arg *arg)
{
  struct narray *array, *ptr;
  int i;

  if (arg == NULL) {
    return NULL;
  }

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(int));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    ptr = arrayadd(array, &arg->ary[i].i);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

static struct narray *
allocate_darray(ngraph_arg *arg)
{
  struct narray *array, *ptr;
  int i;

  if (arg == NULL) {
    return NULL;
  }

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(double));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    ptr = arrayadd(array, &arg->ary[i].d);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

static struct narray *
allocate_sarray(ngraph_arg *arg)
{
  struct narray *array, *ptr;
  int i;

  if (arg == NULL) {
    return NULL;
  }

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(char *));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    ptr = arrayadd2(array, arg->ary[i].str);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

int
ngraph_putobj(struct objlist *obj, const char *vname, int id, ngraph_value *val)
{
  int r, type;
  void *valp;
  struct narray *array;

  r = -1;

  type = chkobjfieldtype(obj, vname);
  switch (type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    valp = NULL;
    r = putobj(obj, vname, id, valp);
    break;
  case NSFUNC:
  case NSTR:
  case NOBJ:
    if (val->str) {
      valp = g_strdup(val->str);
    } else {
      valp = NULL;
    }
    r = putobj(obj, vname, id, valp);
    break;
  case NPOINTER:		/* these fields may not be writable */
  case NBFUNC:
  case NIFUNC:
  case NDFUNC:
  case NIAFUNC:
  case NDAFUNC:
  case NSAFUNC:
    valp = NULL;
    r = putobj(obj, vname, id, valp);
    break;
  case NBOOL:
  case NINT:
  case NENUM:
    valp = &val->i;
    r = putobj(obj, vname, id, valp);
    break;
  case NDOUBLE:
    valp = &val->d;
    r = putobj(obj, vname, id, valp);
    break;
  case NIARRAY:
    array = allocate_iarray(val->ary);
    r = putobj(obj, vname, id, array);
    if (r < 0) {
      arrayfree(array);
    }
    break;
  case NDARRAY:
    array = allocate_darray(val->ary);
    r = putobj(obj, vname, id, array);
    if (r < 0) {
      arrayfree(array);
    }
    break;
  case NSARRAY:
    array = allocate_sarray(val->ary);
    r = putobj(obj, vname, id, array);
    if (r < 0) {
      arrayfree2(array);
    }
    break;
  }

  return r;
}

static const char **
allocate_obj_arg(struct objlist *obj, const char *vname, ngraph_arg *arg)
{
  int i, n, num, is_a;
  const char *arglist;
  const char **ary;

  num = arg->num;
  if (num < 1) {
    /* If the type of the field is NENUM the number of the argument is 0. */
    return NULL;
  }

  arglist = chkobjarglist(obj, vname);
  if (arglist && arglist[0] == '\0') {
    return NULL;
  }

  ary = g_malloc0(sizeof(*ary) * (num + 1));
  if (ary == NULL) {
    return NULL;
  }

  if (arglist == NULL) {
    for (i = 0; i < num; i++) {
      ary[i] = arg->ary[i].str;
    }
  } else {
    n = 0;
    for (i = 0; arglist[i]; i++) {
      if (n >= num) {
	break;
      }

      is_a = (arglist[i + 1]== 'a');

      switch (arglist[i]) {
      case 'b':
      case 'c':
	ary[n] = (char *) &arg->ary[n].i;
	break;
      case 'i':
	if (is_a) {
	  ary[n] = (char *) allocate_iarray(arg->ary[n].ary);
	} else {
	  ary[n] = (char *) &arg->ary[n].i;
	}
	break;
      case 'd':
	if (is_a) {
	  ary[n] = (char *) allocate_darray(arg->ary[n].ary);
	} else {
	  ary[n] = (char *) &arg->ary[n].d;
	}
	break;
      case 's':
	if (is_a) {
	  ary[n] = (char *) allocate_sarray(arg->ary[n].ary);
	} else {
	  ary[n] = arg->ary[n].str;
	}
	break;
      case 'p':
	ary[n] = NULL;
	break;
      case 'o':
	ary[n] = arg->ary[n].str;
	break;
      }

      if (is_a) {
	i++;
      }
      n++;
    }
  }

  return ary;
}

static void
free_obj_arg(const char **ary, struct objlist *obj, const char *vname, ngraph_arg *arg)
{
  int i, n, num, is_a;
  const char *arglist;

  num = arg->num;
  if (num < 1) {
    return;
  }

  arglist = chkobjarglist(obj, vname);
  if (arglist == NULL) {
    return;
  }

  n = 0;
  for (i = 0; arglist[i]; i++) {
    if (n >= num) {
      break;
    }

    is_a = (arglist[i + 1]== 'a');

    switch (arglist[i]) {
    case 'b':
    case 'c':
      break;
    case 'i':
    case 'd':
      if (is_a) {
	arrayfree((struct narray *) ary[n]);
      }
      break;
    case 's':
      if (is_a) {
	arrayfree2((struct narray *) ary[n]);
      }
      break;
    case 'p':
      break;
    case 'o':
      break;
    }

    if (is_a) {
      i++;
    }
    n++;
  }

  g_free(ary);

  return;
}

int
ngraph_getobj(struct objlist *obj, const char *vname, int id, ngraph_arg *arg, ngraph_returned_value *val)
{
  int r, type, argc;
  const char **argv;
  union ngraph_val nval;

  type = chkobjfieldtype(obj, vname);

  argc = arg->num;
  argv = allocate_obj_arg(obj, vname, arg);

  r = getobj(obj, vname, id, argc, (char **) argv, &nval);

  free_obj_arg(argv, obj, vname, arg);
  if (r < 0) {
    return r;
  }

  switch (type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    break;
  case NSFUNC:
  case NSTR:
  case NOBJ:
    val->str = nval.str;
    break;
  case NPOINTER:		/* these fields may not be writable */
    break;
  case NBFUNC:
  case NIFUNC:
  case NBOOL:
  case NINT:
  case NENUM:
    val->i = nval.i;
    break;
  case NDFUNC:
  case NDOUBLE:
    val->d = nval.d;
    break;
  case NIAFUNC:
  case NIARRAY:
    val->ary.num = arraynum(nval.ary);
    val->ary.data.ia = arraydata(nval.ary);
    break;
  case NDAFUNC:
  case NDARRAY:
    val->ary.num = arraynum(nval.ary);
    val->ary.data.da = arraydata(nval.ary);
    break;
  case NSAFUNC:
  case NSARRAY:
    val->ary.num = arraynum(nval.ary);
    val->ary.data.sa = arraydata(nval.ary);
    break;
  }

  return r;
}

int
ngraph_exeobj(struct objlist *obj, const char *vname, int id, ngraph_arg *arg)
{
  int r, argc, type;
  const char **argv;

  type = chkobjfieldtype(obj, vname);
  if (type < NVFUNC) {
    return -1;
  }

  argc = arg->num;
  argv = allocate_obj_arg(obj, vname, arg);

  r = exeobj(obj, vname, id, argc, (char **) argv);

  free_obj_arg(argv, obj, vname, arg);

  return r;
}

struct objlist *
ngraph_get_object(const char *name)
{
  return getobject(name);
}

struct objlist *
ngraph_get_instances_by_str(const char *str, int *n, int **ids)
{
  struct narray iarray;
  int *id_ary, *adata, anum, i, r;
  struct objlist *obj;

  if (n) {
    *n = 0;
  }

  if (ids == NULL) {
    return NULL;
  }

  *ids = NULL;

  if (str == NULL) {
    return NULL;
  }

  arrayinit(&iarray,sizeof(int));
  r = chkobjilist((char *) str, &obj, &iarray, TRUE, NULL);
  if (r) {
    arraydel(&iarray);
    return NULL;
  }

  anum = arraynum(&iarray);
  adata = arraydata(&iarray);

  id_ary = malloc(sizeof(*id_ary) * (anum + 1));
  if (id_ary == NULL) {
    arraydel(&iarray);
    return NULL;
  }

  for (i = 0; i < anum; i++) {
    id_ary[i] = adata[i];
  }
  id_ary[i] = -1;
  arraydel(&iarray);

  if (n) {
    *n = anum;
  }

  *ids = id_ary;

  return obj;
}

int
ngraph_get_id_by_oid(struct objlist *obj, int oid)
{
  return chkobjoid(obj, oid);
}

int
ngraph_move_top(struct objlist *obj, int id)
{
  return movetopobj(obj, id);
}

int
ngraph_move_last(struct objlist *obj, int id)
{
  return movelastobj(obj, id);
}

int
ngraph_move_up(struct objlist *obj, int id)
{
  return moveupobj(obj, id);
}

int
ngraph_move_down(struct objlist *obj, int id)
{
  return movedownobj(obj, id);
}

int
ngraph_exchange(struct objlist *obj, int id1, int id2)
{
  return exchobj(obj, id1, id2);
}

int
ngraph_copy(struct objlist *obj, int dist, int src)
{
  return copy_obj_field(obj, dist, src, NULL);
}

int
ngraph_new(struct objlist *obj)
{
  return newobj(obj);
}

int
ngraph_del(struct objlist *obj, int id)
{
  return delobj(obj, id);
}

int
ngraph_exist(struct objlist *obj, int id)
{
  int last;

  if (obj == NULL) {
    return -1;
  }

  last = chkobjlastinst(obj);
  if (id < 0 || id > last) {
    return -1;
  }

  return id;
}

const char *
ngraph_get_obj_name(struct objlist *obj)
{
  return chkobjectname(obj);
}

int
ngraph_get_obj_field_num(struct objlist *obj)
{
  return chkobjfieldnum(obj);
}

const char *
ngraph_get_obj_field(struct objlist *obj, int i)
{
  return chkobjfieldname(obj, i);
}

int
ngraph_get_obj_field_permission(struct objlist *obj, const char *field)
{
  return chkobjperm(obj, field);
}

int
ngraph_get_obj_field_type(struct objlist *obj, const char *field)
{
  return chkobjfieldtype(obj, field);
}

const char *
ngraph_get_obj_field_args(struct objlist *obj, const char *field)
{
  return chkobjarglist(obj, field);
}

struct objlist *
ngraph_get_obj_parent(struct objlist *obj)
{
  return chkobjparent(obj);
}

struct objlist *
ngraph_get_obj_root(void)
{
  return chkobjroot();
}

const char *
ngraph_get_obj_version(struct objlist *obj)
{
  return chkobjver(obj);
}

int
ngraph_get_obj_id(struct objlist *obj)
{
  return chkobjectid(obj);
}

int
ngraph_get_obj_size(struct objlist *obj)
{
  return chkobjsize(obj);
}

int
ngraph_get_obj_current_id(struct objlist *obj)
{
  return chkobjcurinst(obj);
}

int
ngraph_get_obj_last_id(struct objlist *obj)
{
  return chkobjlastinst(obj);
}

struct objlist *
ngraph_get_obj_next(struct objlist *obj)
{
  return obj->next;
}

struct objlist *
ngraph_get_obj_child(struct objlist *obj)
{
  return obj->child;
}

int
ngraph_puts(const char *s)
{
  return putstdout(s);
}

int
ngraph_err_puts(const char *s)
{
  return putstderr(s);
}

void
ngraph_sleep(int t)
{
  nsleep(t);
}
