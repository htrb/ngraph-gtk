#include <ruby.h>
#include <ruby/encoding.h>
#include <ruby/version.h>

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

#ifdef __MINGW32__
#define WINDOWS 1
#else
#define WINDOWS 0
#endif

#ifdef __APPLE__
#define MAC_OS 1
#else
#define MAC_OS 0
#endif

#if WINDOWS || MAC_OS
static char *
get_ext_name(void)
{
  int n, i;
  struct objlist *sys;
  char *ext_name, *plugin_path, ext_basename[] = "ruby/ngraph.rb";
  ngraph_returned_value val;
  ngraph_arg arg;

  sys = ngraph_get_object("system");
  if (sys == NULL) {
    return NULL;
  }

  arg.num = 0;
  ngraph_object_get(sys, "plugin_dir", 0, &arg, &val);
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

static VALUE
require_files(VALUE data)
{
  VALUE result;

  rb_require("enc/encdb");
  rb_require("enc/trans/transdb");
  rb_require("rbconfig");
#if RUBY_API_VERSION_MAJOR < 3
  rb_require("rubygems");
#endif

  result = rb_require((char *) data);

  return result;
}

int
ngraph_plugin_open_ruby(void)
{
  rb_encoding *enc;
#if WINDOWS || MAC_OS
  char *ext_name;
#endif
  VALUE result, arg;
  int status;

  if (Initialized) {
    return 0;
  }

#if WINDOWS || MAC_OS
  ext_name = get_ext_name();
  if (ext_name == NULL) {
    return 1;
  }
#endif

  ruby_sysinit(&DummyArgc, &DummyArgvPtr);
  {
    RUBY_INIT_STACK;
    ruby_init();
    ruby_script("Embedded Ruby on Ngraph");
    ruby_init_loadpath();
    rb_enc_find_index("encdb");	/* http://www.artonx.org/diary/20090206.html */
    enc = rb_locale_encoding();
    if (enc) {
      rb_enc_set_default_external(rb_enc_from_encoding(enc));
    }
    rb_enc_set_default_internal(rb_enc_from_encoding(rb_utf8_encoding()));
#if WINDOWS || MAC_OS
    arg = (VALUE) ext_name;
#else
    arg = (VALUE) "ngraph.rb";
#endif
    result = rb_protect(require_files, arg, &status);
  }
#if WINDOWS || MAC_OS
  free(ext_name);
#endif
  if (status) {
    return 1;
  }

  Initialized = TRUE;

  return ! RTEST(result);
}

void
ngraph_plugin_close_ruby(void)
{
  if (Initialized) {
    ruby_finalize();
  }
}
