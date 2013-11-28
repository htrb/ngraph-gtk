#ifndef RUBY_COMMON_HEADER
#define RUBY_COMMON_HEADER

static int
load_script(int argc, char **argv)
{
  VALUE r_argv;
  int state, i;

  if (argc < 1) {
    return 0;
  }

  r_argv = rb_const_get(rb_mKernel, rb_intern("ARGV"));
  rb_ary_clear(r_argv);
  for (i = 1; i < argc; i++) {
    rb_ary_push(r_argv, rb_tainted_str_new2(argv[i]));
  }

  rb_load_protect(rb_str_new2(argv[0]), 1, &state);
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
#endif
