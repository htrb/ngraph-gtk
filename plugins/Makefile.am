extra_dist_common = getobj.nsc create_objs.rb libruby.def libruby_resource.rc

if HAVE_RUBY_DEV
  ngraphplugindir = $(pkglibdir)/plugins
  RUBY_INCRUDEDIR = `ruby -e 'puts(RbConfig::CONFIG["rubyhdrdir"])'`
  RUBY_ARCH = `ruby -e 'puts(RbConfig::CONFIG["arch"])'`
  built_sources = ruby_ngraph.c
  ngraphplugin_LTLIBRARIES = libruby.la
  libruby_la_SOURCES = ruby.c
  libruby_la_CFLAGS = -I.. -I $(RUBY_INCRUDEDIR) -I $(RUBY_INCRUDEDIR)/$(RUBY_ARCH)
  libruby_ldflags = -L../src -avoid-version -module -lngraph `ruby -e 'puts(RbConfig::CONFIG["LIBRUBYARG_SHARED"])'`
if OS_WIN32
  libruby_ldflags += -no-undefined -L`ruby -e 'puts(RbConfig::CONFIG["libdir"])'` -export-symbols libruby.def -Wl,libruby_resource.o
  built_sources += libruby_resource.o

.rc.o:
	windres -o $@ $<
endif
  libruby_la_LDFLAGS = $(libruby_ldflags)
  extra_dist = $(extra_dist_common)
else
  extra_dist = $(extra_dist_common) ruby.c
endif

EXTRA_DIST = $(extra_dist)

obj.txt: ../src/ngraph
	../src/ngraph -i getobj.nsc > obj.txt

ruby_ngraph.c: obj.txt $(EXTRA_DIST)
	ruby create_objs.rb obj.txt ruby_ngraph.c

BUILT_SOURCES = $(built_sources)
CLEANFILES = obj.txt $(BUILT_SOURCES)
