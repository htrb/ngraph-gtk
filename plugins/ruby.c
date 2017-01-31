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

#ifdef __MINGW32__
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

int
ngraph_plugin_open_ruby(void)
{
  rb_encoding *enc;
#ifdef __MINGW32__
  char *ext_name;
#endif
  VALUE result;
  int status;

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
  rb_protect(RUBY_METHOD_FUNC(rb_require), (VALUE) "enc/encdb", &status);
  if (status) {
#ifdef __MINGW32__
    free(ext_name);
#endif
    return 1;
  }
  rb_protect(RUBY_METHOD_FUNC(rb_require), (VALUE)"enc/trans/transdb", &status);
  if (status) {
#ifdef __MINGW32__
    free(ext_name);
#endif
    return 1;
  }
  rb_protect(RUBY_METHOD_FUNC(rb_require), (VALUE) "rubygems", &status);
  if (status) {
#ifdef __MINGW32__
    free(ext_name);
#endif
    return 1;
  }

#ifdef __MINGW32__
  result = rb_protect(RUBY_METHOD_FUNC(rb_require), (VALUE) ext_name, &status);
  free(ext_name);
  if (status) {
    return 1;
  }
#else
  result = rb_protect(RUBY_METHOD_FUNC(rb_require), (VALUE) "ngraph.rb", &status);
  if (status) {
    return 1;
  }
#endif

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
