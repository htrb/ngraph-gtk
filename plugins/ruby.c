#include <ruby.h>
#include <ruby/encoding.h>

#include "config.h"
#include "../src/ngraph.h"

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

static int Initialized = FALSE;
static char *DummyArgv[] = {"ngraph_ruby", NULL};
static char **DummyArgvPtr = DummyArgv;
static int DummyArgc = 1;

#include "ruby_common.h"

#ifdef __MINGW32__
static char *
get_ext_name(void)
{
  int n, i;
  struct objlist *sys;
  char *ext_name, *plugin_path, ext_basename[] = "ruby/ngraph.so";
  ngraph_returned_value val;
  ngraph_arg arg;

  sys = ngraph_get_object("system");
  if (sys == NULL) {
    return NULL;
  }

  arg.num = 0;
  ngraph_getobj(sys, "plugin_dir", 0, &arg, &val);
  if (val.str == NULL) {
    return NULL;
  }

  plugin_path = strdup(val.str);
  if (plugin_path == NULL) {
    return NULL;
  }

  n = strlen(plugin_path);
  for (i = n - 1; i >= 0; i--) {
    if (plugin_path[i] == '/') {
      plugin_path[i] = '\0';
      break;
    }
  }

  ext_name = malloc(n + sizeof(ext_basename) + 1);
  if (ext_name == NULL) {
    free(plugin_path);
    return NULL;
  }
  sprintf(ext_name, "%s/%s", plugin_path, ext_basename);

  free(plugin_path);

  return ext_name;
}
#endif

int
ngraph_plugin_open_ruby(struct ngraph_plugin *plugin)
{
  rb_encoding *enc;
  VALUE ngraph_module;
#ifdef __MINGW32__
  char *ext_name;
#endif

  if (Initialized) {
    return 0;
  }

#ifdef __MINGW32__
  ext_name = get_ext_name();
  if (ext_name == NULL) {
    return 1;
  }
#endif

  ruby_sysinit(&DummyArgc, &DummyArgvPtr);
  ruby_init();
  ruby_script("Embedded Ruby on Ngraph");
  ruby_init_loadpath();
  rb_enc_find_index("encdb");	/* http://www.artonx.org/diary/20090206.html */
  enc = rb_locale_encoding();
  if (enc) {
    rb_enc_set_default_external(rb_enc_from_encoding(enc));
  }
  rb_enc_set_default_internal(rb_enc_from_encoding(rb_utf8_encoding()));
  rb_require("enc/encdb");
  rb_require("enc/trans/transdb");
  rb_require("rubygems");
  Initialized = TRUE;

#ifdef __MINGW32__
  rb_require(ext_name);
  free(ext_name);
#else
  rb_require("ngraph.so");
#endif

  ngraph_module = rb_const_get(rb_mKernel, rb_intern("Ngraph"));
  rb_funcall(ngraph_module, rb_intern("ngraph_initialize"), 2, Qnil, Qfalse);

  return 0;
}

int
ngraph_plugin_exec_ruby(struct ngraph_plugin *plugin, int argc, char *argv[])
{
  int state, i;
  VALUE r_argv;

  if (! Initialized) {
    return 1;
  }

  if (argc < 2) {
    return 0;
  }

  ruby_script(argv[1]);
  load_script(argc - 1, argv + 1);

  return 0;
}

#ifndef __MINGW32__
void
ngraph_plugin_close_ruby(struct ngraph_plugin *plugin)
{
  if (Initialized) {
    ruby_finalize();
    Initialized = FALSE;
  }
}
#endif
