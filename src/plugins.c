#include "common.h"

#include <glib.h>
#include <gmodule.h>

#define USE_HASH 1

#include "ngraph.h"
#include "object.h"
#include "shell.h"

typedef int (* plugin_init_func) (const char *message);
typedef int (* plugin_shell_init) (int argc, char *argv[]);
typedef int (* plugin_shell_done) (int argc, char *argv[]);
typedef int (* plugin_shell_shell) (int argc, char *argv[]);

struct plugin {
  GModule *module;
  char *name;
  plugin_shell_init shell_init;
  plugin_shell_done shell_done;
  plugin_shell_shell shell_shell;
  struct plugin *next;
};

static int plugin_init(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
static int plugin_init(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
static int plugin_done(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
static int plugin_shell(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);

#define NAME "shell"
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

#define ERRNUM (sizeof(sherrorlist) / sizeof(*sherrorlist))

static struct objtable PluginShellPrototype[] = {
  {"init", NVFUNC, NEXEC, plugin_init, NULL, 0},
  {"done", NVFUNC, NEXEC, plugin_done, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"shell", NVFUNC, NREAD|NEXEC, plugin_shell, "sa", 0},
  {"security", NVFUNC, 0, NULL, "b", 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(PluginShellPrototype) / sizeof(*PluginShellPrototype))

static struct plugin *PluginList = NULL;

static int
check_loaded(const char *name)
{
  struct plugin *cur;

  for (cur = PluginList; cur; cur = cur->next) {
    if (g_strcmp0(name, cur->name) == 0) {
      return 1;
    }
  }

  return 0;
}

static int
add_plugin(GModule *module, const char *name, plugin_shell_init shell_init, plugin_shell_done shell_done, plugin_shell_shell shell_shell)
{
  struct plugin *cur, *next;
  char *copy_name;

  copy_name = g_strdup(name);
  if (copy_name == NULL) {
    return 1;
  }

  if (PluginList == NULL) {
    PluginList = g_malloc0(sizeof(struct plugin));
    if (PluginList == NULL) {
      g_free(copy_name);
      return 1;
    }
    cur = PluginList;
  } else {
    for (cur = PluginList; cur->next; cur = cur->next) {
      ;
    }
    next = g_malloc0(sizeof(struct plugin));
    if (next == NULL) {
      g_free(copy_name);
      return 1;
    }
    cur->next = next;
    cur = next;
  }

  cur->name = copy_name;
  cur->module = module;
  cur->shell_init = shell_init;
  cur->shell_done = shell_done;
  cur->shell_shell = shell_shell;
  cur->next = NULL;

  return 0;
}

static void *
add_plugin_shell(const char *name)
{
  static struct objtable *plugin_shell;
  unsigned int i;
  char *copy_name;

  copy_name = g_strdup(name);
  if (copy_name == NULL) {
    return NULL;
  }

  plugin_shell = g_malloc(sizeof(PluginShellPrototype));
  if (plugin_shell == NULL) {
    g_free(copy_name);
    return NULL;
  }

  for (i = 0; i < TBLNUM; i++) {
    plugin_shell[i] = PluginShellPrototype[i];
  }
  
  return addobject(copy_name, NULL, PARENT, OVERSION, TBLNUM, plugin_shell, ERRNUM, sherrorlist, NULL, NULL);
}

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

static int
load_plugin(const char *name)
{
  GModule *module;
  plugin_init_func plugin_init;
  plugin_shell_init shell_init;
  plugin_shell_done shell_done;
  plugin_shell_shell shell_shell;
  int r;

  module = g_module_open(name, 0);
  if (module == NULL) {
    return 1;
  }

  r = get_symbol(module, "ngraph_plugin_init_%s", name, (gpointer *) &plugin_init);
  if (! r || plugin_init == NULL) {
    g_module_close(module);
    return 1;
  }

  r = get_symbol(module, "ngraph_plugin_shell_init_%s", name, (gpointer *) &shell_init);
  if (! r || shell_init == NULL) {
    g_module_close(module);
    return 1;
  }

  r = get_symbol(module, "ngraph_plugin_shell_done_%s", name, (gpointer *) &shell_done);
  if (! r || shell_init == NULL) {
    g_module_close(module);
    return 1;
  }

  r = get_symbol(module, "ngraph_plugin_shell_shell_%s", name, (gpointer *) &shell_shell);
  if (! r || shell_init == NULL) {
    g_module_close(module);
    return 1;
  }

  if (plugin_init("test")) {
    g_module_close(module);
    return 1;
  }

  if (add_plugin(module, name, shell_init, shell_done, shell_shell)) {
    g_module_close(module);
    return 1;
  }

  if (add_plugin_shell(name) == NULL) {
    return 1;
  }

  return 0;
}

static int 
plugin_init(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  return 0;
}

static int 
plugin_done(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  return 0;
}

static int 
plugin_shell(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  return 0;
}

int
cmload(struct nshell *nshell,int argc,char **argv)
{
  if (get_security()) {
    sherror4(argv[0], ERRSECURITY);
    return 1;
  }

  if (argc < 2) {
    sherror4(argv[0], ERRSMLARG);
    return 1;
  }

  if (check_loaded(argv[1])) {
    printfstdout("'%s' is already loaded.\n", argv[1]);
    return 1;
  }

  if (load_plugin(argv[1])) {
    printfstdout("cannot load module '%s'.\n", argv[1]);
    return 1;
  }

  return 0;
}
