SUBDIRS = ruby

extra_dist_common = libruby.def libruby_resource.rc

if HAVE_RUBY_DEV
  ngraphplugindir = $(pkglibdir)/plugins
  RUBY_INCLUDEDIR = `ruby -e 'puts(RbConfig::CONFIG["rubyhdrdir"])'`
  RUBY_ARCH_INCLUDEDIR = `ruby -e 'c=RbConfig::CONFIG; s="rubyarchhdrdir"; puts(if (c[s]) then c[s]; else c["rubyhdrdir"] + "/" + c["arch"] + "/"; end)'`
  built_sources =
  ngraphplugin_LTLIBRARIES = libruby.la
  libruby_la_SOURCES = ruby.c
  libruby_la_CFLAGS = -I.. -I $(RUBY_INCLUDEDIR) -I $(RUBY_ARCH_INCLUDEDIR)
  libruby_ldflags = -L../src -avoid-version -module `ruby -e 'puts(RbConfig::CONFIG["LIBRUBYARG_SHARED"])'`
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

BUILT_SOURCES = $(built_sources)
CLEANFILES = $(BUILT_SOURCES)
