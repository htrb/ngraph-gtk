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
static ID Argv;

static char *DummyArgv[] = {"ngraph_ruby", NULL};
static char **DummyArgvPtr = DummyArgv;
static int DummyArgc = 1;

int
ngraph_plugin_open_ruby(struct ngraph_plugin *plugin)
{
  rb_encoding *enc;

  if (Initialized) {
    return 0;
  }

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

  rb_require("ngraph");

  Argv = rb_intern("ARGV");

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

  r_argv = rb_const_get(rb_mKernel, Argv);
  rb_ary_clear(r_argv);
  for (i = 2; i < argc; i++) {
    rb_ary_push(r_argv, rb_tainted_str_new2(argv[i]));
  }

  ruby_script(argv[1]);
  rb_load_protect(rb_str_new2(argv[1]), 1, &state);
  if (state) {
    VALUE errinfo, errstr, errat;
    int n, i;
    const char *cstr;

    errinfo = rb_errinfo();
    errstr = rb_obj_as_string(errinfo);
    cstr = StringValueCStr(errstr);
    if (strcmp(cstr, "exit")) {
      ngraph_err_puts(cstr);
      errat = rb_funcall(errinfo, rb_intern("backtrace"), 0);
      if (! NIL_P(errat)) {
	n = RARRAY_LEN(errat);
	for (i = 0; i < n; i ++) {
	  errstr = rb_str_new2("\tfrom ");
	  rb_str_append(errstr, rb_ary_entry(errat, i));
	  ngraph_err_puts(StringValueCStr(errstr));
	}
      }
    }
  }
  rb_gc_start();

  return 0;
}

#ifndef WINDOWS
void
ngraph_plugin_close_ruby(struct ngraph_plugin *plugin)
{
  if (Initialized) {
    ruby_finalize();
    Initialized = FALSE;
  }
}
#endif
