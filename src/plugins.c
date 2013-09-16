#include "common.h"

#include <glib.h>
#include <gmodule.h>

#define USE_HASH 1

#include "ngraph.h"
#include "ngraph_plugin_shell.h"
#include "object.h"
#include "shell.h"

#define NAME "plugin_shell"
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

static char *sherrorlist[] = {
  "cannot allocate enough memory",
  "already running.",
  "no command string is specified.",
  "no such file",
  "connnot load module",
  "the module is already loaded",
  "a module is not loaded",
  "connnot initialize the shell",
};

struct plugin_shell {
  GModule *module;
  char *name;
  plugin_shell_shell shell_shell;
  int deleted;
  void *user_data;
};

struct shlocal {
  struct plugin_shell *shell;
  int lock;
};

static int
get_symbol(GModule *module, const char *format, const char *name, gpointer *symbol)
{
  char *func;
  int r;

  func = g_strdup_printf(format, name);
  r = g_module_symbol(module, func, symbol);
  g_free(func);
  if (! r || symbol == NULL) {
    return 1;
  }

  return 0;
}

static int
load_plugin(char *name, struct plugin_shell *shell)
{
  GModule *module;
  plugin_shell_shell shell_shell;
  int r, i;
  char *basename;

  module = g_module_open(name, 0);
  if (module == NULL) {
    return 1;
  }

  basename = getbasename(name);
  for (i = 0; basename[i]; i++) {
    if (basename[i] == '.') {
      basename[i] = '\0';
      break;
    }
  }

  shell_shell = NULL;
  r = get_symbol(module, "ngraph_plugin_shell_shell_%s", basename, (gpointer *) &shell_shell);
  g_free(basename);
  if (r) {
    g_module_close(module);
    return 1;
  }

  shell->name = name;
  shell->module = module;
  shell->shell_shell = shell_shell;

  return 0;
}


static int 
plugin_shell_open(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *name;
  struct shlocal *shlocal;
  struct plugin_shell *shell;

  if (argv[2] == NULL) {
    error(obj, ERRNOCL);
    return 1;
  }

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  if (shlocal->shell) {
    error2(obj, ERRLOADED, shlocal->shell->name);
    return 1;
  }

  shell = g_malloc0(sizeof(struct plugin_shell));
  if (shell == NULL) {
    error(obj, ERRMEM);
    return 1;
  }

  name = g_strdup(argv[2]);
  if (name == NULL) {
    g_free(shell);
    error(obj, ERRMEM);
    return 1;
  }

  if (load_plugin(name, shell)) {
    error2(obj, ERRLOAD, name);
    g_free(name);
    g_free(shell);
    return 1;
  }

  shlocal->shell = shell;

  return 0;
}

static int
close_shell(struct objlist *obj, struct plugin_shell *shell)
{
  if (shell == NULL) {
    return 0;
  }

  if (shell->module == NULL) {
    error(obj, ERRNOLOAD);
    return 1;
  }

  g_module_close(shell->module);
  shell->module = NULL;
  shell->shell_shell = NULL;

  if (shell->name) {
    g_free(shell->name);
    shell->name = NULL;
  }

  g_free(shell);

  return 0;
}

static int
plugin_shell_close(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct shlocal *shlocal;

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  if (shlocal->lock) {
    error(obj, ERRRUN);
    return 1;
  }

  close_shell(obj, shlocal->shell);
  shlocal->shell = NULL;

  return 0;
}

static int
plugin_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct shlocal *shlocal;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  shlocal = g_malloc0(sizeof(struct shlocal));
  if (shlocal == NULL) {
    error(obj, ERRMEM);
    return 1;
  }

  if (_putobj(obj, "_local", inst, shlocal)) {
    g_free(shlocal);
    return 1;
  }

  shlocal->lock = 0;

  return 0;
}

static int
plugin_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct plugin_shell *shell;
  struct shlocal *shlocal;


  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  shell = shlocal->shell;
  if (shell == NULL) {
    return 0;
  }

  if (shlocal->lock) {
    shell->deleted = TRUE;
  } else {
    close_shell(obj, shell);
  }

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
plugin_shell_exec(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct plugin_shell *shell;
  struct shlocal *shlocal;
  char **new_argv;
  int r;

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL || shlocal->shell == NULL) {
    return 1;
  }

  shell = shlocal->shell;

  if (shell->module == NULL ||
      shell->shell_shell == NULL) {
    error(obj, ERRNOLOAD);
    return 1;
  }

  if (shlocal->lock) {
    error(obj, ERRRUN);
    return 1;
  }

  new_argv = allocate_argv(shell->name, argc - 2, argv + 2);
  if (new_argv == NULL) {
    error(obj, ERRMEM);
    return 1;
  }

  shlocal->lock = TRUE;

  r = shell->shell_shell(shell, argc - 1, new_argv);

  if (shell->deleted) {
    close_shell(obj, shell);
  } else {
    shlocal->lock = FALSE;
  }

  free_argv(argc - 1, new_argv);

  return r;
}

#define ERRNUM (sizeof(sherrorlist) / sizeof(*sherrorlist))

static struct objtable PluginShell[] = {
  {"init", NVFUNC, NEXEC, plugin_init, NULL, 0},
  {"done", NVFUNC, NEXEC, plugin_done, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"shell", NVFUNC, NREAD|NEXEC, plugin_shell_exec, NULL, 0},
  {"security", NBOOL, 0, NULL, "b", 0},
  {"open", NVFUNC, NREAD|NEXEC, plugin_shell_open, "s", 0},
  {"close", NVFUNC, NREAD|NEXEC, plugin_shell_close, "", 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(PluginShell) / sizeof(*PluginShell))

void *
add_plugin_shell(void)
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, PluginShell, ERRNUM, sherrorlist, NULL, NULL);
}

/*****************************************************/

void
ngraph_plugin_shell_set_user_data(struct plugin_shell *shlocal, void *user_data)
{
  if (shlocal == NULL) {
    return;
  }
  shlocal->user_data = user_data;
}

void *
ngraph_plugin_shell_get_user_data(struct plugin_shell *shlocal)
{
  if (shlocal == NULL) {
    return NULL;
  }
  return shlocal->user_data;
}

union ngraph_val {
  int i;
  double d;
  char *str;
  struct narray *ary;;
};

static struct narray *
allocate_iarray(ngraph_arg *arg)
{
  struct narray *array, *ptr;
  int i;

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

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(char *));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    ptr = arrayadd2(array, &arg->ary[i].str);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

int
ngraph_plugin_shell_putobj(struct objlist *obj, const char *vname, int id, ngraph_value *val)
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

static char **
allocate_obj_arg(struct objlist *obj, const char *vname, ngraph_arg *arg)
{
  int i, n, num, is_a;
  const char *arglist;
  char **ary;

  num = arg->num;
  if (num < 1) {
    /* If the type of the field is NENUM the number of the argument is 0. */
    return NULL;
  }

  ary = g_malloc0(sizeof(*ary) * (num + 1));
  if (ary == NULL) {
    return NULL;
  }

  arglist = chkobjarglist(obj, vname);
  if (arglist == NULL) {
    return NULL;
  }

  if (arglist[0] == '\0') {
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
free_obj_arg(char **ary, struct objlist *obj, const char *vname, ngraph_arg *arg)
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
ngraph_plugin_shell_getobj(struct objlist *obj, const char *vname, int id, ngraph_arg *arg, ngraph_returned_value *val)
{
  int r, type, argc;
  char **argv;
  union ngraph_val nval;

  type = chkobjfieldtype(obj, vname);

  argc = arg->num;
  argv = allocate_obj_arg(obj, vname, arg);

  r = getobj(obj, vname, id, argc, argv, &nval);

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
ngraph_plugin_shell_exeobj(struct objlist *obj, const char *vname, int id, ngraph_arg *arg)
{
  int r, argc, type;
  char **argv;

  type = chkobjfieldtype(obj, vname);
  if (type < NVFUNC) {
    return -1;
  }

  argc = arg->num;
  argv = allocate_obj_arg(obj, vname, arg);

  r = exeobj(obj, vname, id, argc, argv);

  free_obj_arg(argv, obj, vname, arg);

  return r;
}

struct objlist *
ngraph_plugin_shell_get_object(const char *name)
{
  return getobject(name);
}

int *
ngraph_plugin_shell_get_id_by_str(struct objlist *obj, const char *name)
{
  struct narray iarray;
  struct objlist *obj2;
  int *id_ary, *adata, anum, i, r;
  char *tmp, *objname;

  if (obj == NULL) {
    return NULL;
  }

  objname = chkobjectname(obj);
  if (objname == NULL) {
    return NULL;
  }

  tmp = g_strdup_printf("%s:%s", objname, name);

  arrayinit(&iarray,sizeof(int));
  r = chkobjilist(tmp, &obj2, &iarray, TRUE, NULL);
  g_free(tmp);
  if (r) {
    arraydel(&iarray);
    return NULL;
  }

  anum = arraynum(&iarray);
  adata = arraydata(&iarray);

  id_ary = g_malloc(sizeof(*id_ary) * (anum + 1));
  if (id_ary == NULL) {
    arraydel(&iarray);
    return NULL;
  }

  for (i = 0; i < anum; i++) {
    id_ary[i] = adata[i];
  }
  id_ary[i] = -1;
  arraydel(&iarray);

  return id_ary;
}

int
ngraph_plugin_shell_get_id_by_oid(struct objlist *obj, int oid)
{
  return chkobjoid(obj, oid);
}

int
ngraph_plugin_shell_move_top(struct objlist *obj, int id)
{
  return movetopobj(obj, id);
}

int
ngraph_plugin_shell_move_last(struct objlist *obj, int id)
{
  return movelastobj(obj, id);
}

int
ngraph_plugin_shell_move_up(struct objlist *obj, int id)
{
  return moveupobj(obj, id);
}

int
ngraph_plugin_shell_move_down(struct objlist *obj, int id)
{
  return movedownobj(obj, id);
}

int
ngraph_plugin_shell_exchange(struct objlist *obj, int id1, int id2)
{
  return exchobj(obj, id1, id2);
}

int
ngraph_plugin_shell_new(struct objlist *obj)
{
  return newobj(obj);
}

int
ngraph_plugin_shell_del(struct objlist *obj, int id)
{
  return delobj(obj, id);
}

int
ngraph_plugin_shell_exist(struct objlist *obj, int id)
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
ngraph_plugin_shell_get_obj_name(struct objlist *obj)
{
  return chkobjectname(obj);
}

int
ngraph_plugin_shell_get_obj_field_num(struct objlist *obj)
{
  return chkobjfieldnum(obj);
}

const char *
ngraph_plugin_shell_get_obj_field(struct objlist *obj, int i)
{
  return chkobjfieldname(obj, i);;
}

int
ngraph_plugin_shell_get_obj_field_permission(struct objlist *obj, const char *field)
{
  return chkobjperm(obj, field);
}

int
ngraph_plugin_shell_get_obj_field_type(struct objlist *obj, const char *field)
{
  return chkobjfieldtype(obj, field);
}

const char *
ngraph_plugin_shell_get_obj_field_args(struct objlist *obj, const char *field)
{
  return chkobjarglist(obj, field);
}

struct objlist *
ngraph_plugin_shell_get_obj_parent(struct objlist *obj)
{
  return chkobjparent(obj);
}

const char *
ngraph_plugin_shell_get_obj_version(struct objlist *obj)
{
  return chkobjver(obj);
}

int
ngraph_plugin_shell_get_obj_id(struct objlist *obj)
{
  return chkobjectid(obj);;
}

int
ngraph_plugin_shell_get_obj_size(struct objlist *obj)
{
  return chkobjsize(obj);
}

int
ngraph_plugin_shell_get_obj_current_id(struct objlist *obj)
{
  return chkobjcurinst(obj);
}

int
ngraph_plugin_shell_get_obj_last_id(struct objlist *obj)
{
  return chkobjlastinst(obj);
}

struct objlist *
ngraph_plugin_shell_get_obj_next(struct objlist *obj)
{
  return obj->next;
}

struct objlist *
ngraph_plugin_shell_get_obj_child(struct objlist *obj)
{
  return obj->child;
}

