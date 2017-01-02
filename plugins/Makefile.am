SUBDIRS = ruby

extra_dist_common = libruby.def libruby_resource.rc.in

if HAVE_RUBY_DEV
  ngraphplugindir = $(pkglibdir)/plugins
  RUBY_INCRUDEDIR = `ruby -r rbconfig -e "print RbConfig::CONFIG['rubyhdrdir']"`
  RUBY_ARCH = `ruby -r rbconfig -e "print RbConfig::CONFIG['rubyarchhdrdir']"`
  built_sources =
  clean_files =
  ngraphplugin_LTLIBRARIES = libruby.la
  libruby_la_SOURCES = ruby.c
  libruby_la_CFLAGS = -I$(top_srcdir) -I $(RUBY_INCRUDEDIR) -I $(RUBY_ARCH)
  libruby_ldflags = -L$(top_builddir)/src -avoid-version -module `ruby -e 'puts(RbConfig::CONFIG["LIBRUBYARG_SHARED"])'`
if OS_WIN32
  libruby_ldflags += -no-undefined -L`ruby -e 'puts(RbConfig::CONFIG["libdir"])'` -export-symbols $(srcdir)/libruby.def -Wl,$(builddir)/libruby_resource.o
  built_sources += libruby_resource.o
  libruby_la_LIBADD = -lngraph
libruby_resource.rc: libruby_resource.rc.in
	$(AWK) -f $(top_srcdir)/winrc_version.awk -v VERSION=$(VERSION) $^ > $@

.rc.o:
	windres -o $@ $<
  clean_files += ngraph_resource.rc
endif
  libruby_la_LDFLAGS = $(libruby_ldflags)
  extra_dist = $(extra_dist_common)
else
  extra_dist = $(extra_dist_common) ruby.c
endif

EXTRA_DIST = $(extra_dist)

BUILT_SOURCES = $(built_sources)
CLEANFILES = $(BUILT_SOURCES) $(clean_files)
