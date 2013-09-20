extra_dist_common = getobj.nsc create_objs.rb

if HAVE_RUBY_DEV
  RUBY_INCRUDEDIR = `ruby -e 'puts(RbConfig::CONFIG["rubyhdrdir"])'`
  RUBY_ARCH = `ruby -e 'puts(RbConfig::CONFIG["arch"])'`
  BUILT_SOURCES = ruby_ngraph.c
  pkglib_LTLIBRARIES = libruby.la
  libruby_la_SOURCES = ruby.c
  libruby_la_CFLAGS = $(GTK_CFLAGS) -I.. -I $(RUBY_INCRUDEDIR) -I $(RUBY_INCRUDEDIR)/$(RUBY_ARCH)
  libruby_la_LDFLAGS = -avoid-version -disable-static --mode=link -module `ruby -e 'puts(RbConfig::CONFIG["LIBRUBYARG_SHARED"])'`
  extra_dist = $(extra_dist_common)
else
  extra_dist = $(extra_dist_common) ruby.c
endif

EXTRA_DIST = $(extra_dist)

obj.txt: ../src/ngraph
	../src/ngraph -i getobj.nsc > obj.txt

ruby_ngraph.c: obj.txt $(EXTRA_DIST)
	ruby create_objs.rb obj.txt ruby_ngraph.c

CLEANFILES = obj.txt ruby_ngraph.c
