#include "common.h"

#include <stdlib.h>
#include <gmodule.h>

#define USE_HASH 1

#include "ngraph.h"
#include "object.h"
#include "shell.h"

#define NAME "plugin"
#define PARENT "object"
#define OVERSION "1.00.00"
#define MAXCLINE 256

#define ERRMEM 100
#define ERRRUN 101
#define ERRNOMODULE 102
#define ERRFILEFIND 103
#define ERRLOAD 104
#define ERRLOADED 105
#define ERRNOLOAD 106
#define ERRINIT 107
#define ERRSECURTY 108
#define ERRINVALID 109
#define ERRPROHIBITED 110

static char *sherrorlist[] = {
  "cannot allocate enough memory",
  "already running.",
  "no module name is specified.",
  "no such file",
  "cannot load module",
  "the module is already loaded",
  "a module is not loaded",
  "cannot initialize the plugin",
  "the method is forbidden for the security",
  "invalid module",
  "the module is prohibited",
};

struct ngraph_plugin {
  GModule *module;
  ngraph_plugin_exec exec;
  ngraph_plugin_open open;
  ngraph_plugin_close close, interrupt;
  int deleted, id;
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

static char *
get_plugin_name(const char *str)
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

static int
load_plugin(struct objlist *obj, N_VALUE *inst, const char *name, struct ngraph_plugin *plugin)
{
  GModule *module;
  void *loaded;
  ngraph_plugin_exec np_exec;
  ngraph_plugin_open np_open;
  ngraph_plugin_close np_close, np_interrupt;
  int r, id;
  char *basename, *module_file, *plugin_path, *argv[2];
  struct objlist *sysobj;

  module = NULL;
  module_file = NULL;
  basename = NULL;

  basename = get_plugin_name(name);
  if (basename == NULL) {
    error(obj, ERRLOAD);
    return 1;
  }

  r = nhash_get_ptr(Plugins, basename, &loaded);
  if (r == 0 && loaded) {
    error2(obj, ERRLOADED, basename);
    goto ErrorExit;
  }

  plugin_path = NULL;
  sysobj = getobject("system");

  argv[0] = basename;
  argv[1] = NULL;
  getobj(sysobj, "prohibited_plugin", 0, 1, argv, &r);
  if (r) {
    error2(obj, ERRPROHIBITED, basename);
    goto ErrorExit;
  }

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

static struct ngraph_plugin *
get_plugin(struct objlist *obj, N_VALUE *inst)
{
  const char *name;

  _getobj(obj, "module_name", inst, &name);
  if (name == NULL) {
    return NULL;
  }

  return get_plugin_from_name(name);
}

static int 
plugin_open(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *name;
  struct ngraph_plugin *plugin;
  int r;

  rval->i = 0;

  if (argv[2] == NULL) {
    error(obj, ERRNOMODULE);
    return 1;
  }

  name = get_plugin_name(argv[2]);
  if (name == NULL) {
    error(obj, ERRNOMODULE);
    return 1;
  }
  plugin = get_plugin_from_name(name);
  g_free(name);

#ifdef WINDOWS
  if (plugin) {
    if (plugin->id >= 0) {
      error2(obj, ERRLOADED, plugin->name);
      return 1;
    }
    _getobj(obj, "id", inst, &plugin->id);
  } else {
    plugin = g_malloc0(sizeof(struct ngraph_plugin));
    if (plugin == NULL) {
      error(obj, ERRMEM);
      return 1;
    }

    if (load_plugin(obj, inst, argv[2], plugin)) {
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

  if (load_plugin(obj, inst, argv[2], plugin)) {
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
      error2(obj, ERRINIT, argv[2]);
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

static int
plugin_exec(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct ngraph_plugin *plugin;
  char **new_argv, *tmp;
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

  tmp = argv[1];
  argv[1] = plugin->name;
  new_argv = allocate_argv(argc - 1, argv + 1);
  argv[1] = tmp;
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
