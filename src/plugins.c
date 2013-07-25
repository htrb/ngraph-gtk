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
#define OVERSION   "1.00.00"
#define MAXCLINE 256

#define ERRRUN 100
#define ERRNOCL 101
#define ERRFILEFIND 102

static char *sherrorlist[]={
  "already running.",
  "no command string is specified.",
  "no such file",
};

struct plugin_shell {
  GModule *module;
  char *name;
  plugin_shell_init shell_init;
  plugin_shell_done shell_done;
  plugin_shell_shell shell_shell;
  int lock;
  void *user_data;
};

static int
get_symbol(GModule *module, const char *format, const char *name, gpointer *symbol)
{
  char *func;
  int r;

  func = g_strdup_printf("ngraph_plugin_init_%s", name);
  r = g_module_symbol(module, func, symbol);
  g_free(func);
  if (! r || symbol == NULL) {
    return 1;
  }

  return 0;
}

static GModule *
load_plugin(char *name, struct plugin_shell *shlocal)
{
  GModule *module;
  plugin_shell_init shell_init;
  plugin_shell_done shell_done;
  plugin_shell_shell shell_shell;
  int r;

  module = g_module_open(name, 0);
  if (module == NULL) {
    return NULL;
  }

  r = get_symbol(module, "ngraph_plugin_shell_init_%s", name, (gpointer *) &shell_init);
  if (! r || shell_init == NULL) {
    g_module_close(module);
    return NULL;
  }

  r = get_symbol(module, "ngraph_plugin_shell_done_%s", name, (gpointer *) &shell_done);
  if (! r || shell_init == NULL) {
    g_module_close(module);
    return NULL;
  }

  r = get_symbol(module, "ngraph_plugin_shell_shell_%s", name, (gpointer *) &shell_shell);
  if (! r || shell_init == NULL) {
    g_module_close(module);
    return NULL;
  }

  shlocal->name = name;
  shlocal->module = module;
  shlocal->shell_shell = shell_shell;
  shlocal->shell_init = shell_init;
  shlocal->shell_done = shell_done;

  return module;
}


static int 
plugin_shell_open(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *name;
  GModule *module;
  struct plugin_shell *shlocal;

  if (argv[2] == NULL) {
    return 1;
  }

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  name = g_strdup(argv[2]);
  if (name == NULL) {
    return 1;
  }

  module = load_plugin(name, shlocal);
  if (module == NULL) {
    g_free(name);
    return 1;
  }

  return 0;
}

static int 
plugin_shell_close(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct plugin_shell *shlocal;

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  if (shlocal->module) {
    g_module_close(shlocal->module);
    shlocal->module = NULL;
    shlocal->shell_init = NULL;
    shlocal->shell_done = NULL;
    shlocal->shell_shell = NULL;
  }

  return 0;
}

static int 
plugin_init(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct plugin_shell *shlocal;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  shlocal = g_malloc0(sizeof(struct plugin_shell));
  if (shlocal == NULL) return 1;
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
  struct plugin_shell *shlocal;

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  plugin_shell_close(obj, inst, rval, argc, argv);

  g_free(shlocal);

  return 0;
}

static int 
plugin_shell_exec(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct plugin_shell *shlocal;

  _getobj(obj, "_local", inst, &shlocal);
  if (shlocal == NULL) {
    return 1;
  }

  if (shlocal->module == NULL ||
      shlocal->shell_init == NULL ||
      shlocal->shell_done == NULL ||
      shlocal->shell_shell == NULL) {
    return 1;
  }

  if (shlocal->lock) {
    return 1;
  }

  shlocal->lock = TRUE;

  shlocal->shell_init(shlocal);
  shlocal->shell_done(shlocal);

  shlocal->lock = FALSE;

  return 0;
}

#define ERRNUM (sizeof(sherrorlist) / sizeof(*sherrorlist))

static struct objtable PluginShell[] = {
  {"init", NVFUNC, NEXEC, plugin_init, NULL, 0},
  {"done", NVFUNC, NEXEC, plugin_done, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"shell", NVFUNC, NREAD|NEXEC, plugin_shell_exec, "sa", 0},
  {"security", NBOOL, 0, NULL, "b", 0},
  {"open", NVFUNC, NREAD|NEXEC, plugin_shell_open, "s", 0},
  {"close", NVFUNC, NREAD|NEXEC, plugin_shell_close, "", 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(PluginShell) / sizeof(*PluginShell))

void *
add_plugin_shell(const char *name)
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

int
ngraph_plugin_shell_putobj(struct objlist *obj, const char *vname, int id, void *val)
{
  return 0;
}

int 
ngraph_plugin_shell_getobj(struct objlist *obj, const char *vname, int id, int argc, char **argv, void *val)
{
  return 0;
}

int 
ngraph_plugin_shell_exeobj(struct objlist *obj, const char *vname, int id, int argc, char **argv)
{
  return 0;
}

struct objlist *
ngraph_plugin_shell_getobject(const char *name)
{
  return NULL;
}

int 
ngraph_plugin_shell_getid_by_name(struct objlist *obj, const char *name)
{
  return 0;
}

int 
ngraph_plugin_shell_getid_by_oid(struct objlist *obj, int oid)
{
  return 0;
}

int 
ngraph_plugin_shell_move_top(struct objlist *obj, int id)
{
  return 0;
}

int 
ngraph_plugin_shell_move_last(struct objlist *obj, int id)
{
  return 0;
}

int 
ngraph_plugin_shell_move_up(struct objlist *obj, int id)
{
  return 0;
}

int 
ngraph_plugin_shell_move_down(struct objlist *obj, int id)
{
  return 0;
}

int 
ngraph_plugin_shell_exchange(struct objlist *obj, int id1, int id2)
{
  return 0;
}

int 
ngraph_plugin_shell_copy(struct objlist *obj, int id_dest, int id_src)
{
  return 0;
}

int 
ngraph_plugin_shell_new(struct objlist *obj)
{
  return 0;
}

int 
ngraph_plugin_shell_del(struct objlist *obj, int id)
{
  return 0;
}

int 
ngraph_plugin_shell_exist(struct objlist *obj, int id)
{
  return 0;
}

const char *
ngraph_plugin_shell_get_obj_name(const char *name)
{
  return NULL;
}

char *
ngraph_plugin_shell_get_obj_fields(const char *name)
{
  return 0;
}

const char *
ngraph_plugin_shell_get_obj_parent(const char *name)
{
  return NULL;
}

const char *
ngraph_plugin_shell_get_obj_version(const char *name)
{
  return NULL;
}

int
ngraph_plugin_shell_get_obj_id(const char *name)
{
  return 0;
}

int
ngraph_plugin_shell_get_obj_size(const char *name)
{
  return 0;
}

int
ngraph_plugin_shell_get_obj_current(const char *name)
{
  return 0;
}

int
ngraph_plugin_shell_get_obj_last(const char *name)
{
  return 0;
}

int
ngraph_plugin_shell_get_obj_inst_num(const char *name)
{
  return 0;
}

char *
ngraph_plugin_shell_derive(const char *name)
{
  return NULL;
}

char *
ngraph_plugin_shell_derive_inst(const char *name)
{
  return NULL;
}

/*

derive [-instance] object

 */
