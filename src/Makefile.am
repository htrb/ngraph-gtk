SUBDIRS = math gtk

EXTRA_DIST = ngraph_export.map ngraph_resource.rc.in libngraph_resource.rc.in ngraph.ico

libngraph_ldflags = -version-info $(LIBNGRAPH_VERSION) $(READLINE_LIBS) $(GTK_LIBS) $(GLIB_LIBS) $(GSL_LIBS)
libngraph_cflags =
ngraph_ldflags = -lngraph
ngraph_lib = libngraph.la

if OS_WIN32
libngraph_ldflags += -no-undefined -export-symbols ngraph.def -Wl,libngraph_resource.o
ngraph_ldflags += -Wl,ngraph_resource.o

MINGW_DIR = $(top_srcdir)/mingw
BUILT_SOURCES = ngraph.def libngraph_resource.o ngraph_resource.o
CLEANFILES = $(BUILT_SOURCES) libngraph_resource.rc ngraph_resource.rc

ngraph_resource.rc: ngraph_resource.rc.in
	$(AWK) -f $(MINGW_DIR)/winrc_version.awk -v VERSION=$(VERSION) $^ > $@

libngraph_resource.rc: libngraph_resource.rc.in
	$(AWK) -f $(MINGW_DIR)/winrc_version.awk -v VERSION=$(VERSION) $^ > $@

.rc.o:
	$(WINDRES) -I $(top_srcdir)/src -o $@ $<

ngraph.def: ngraph_api.c
	echo "EXPORTS" > $@
	cat $^ | grep '^ngraph_' | sed 's/[^a-z0-9_].*//' >> $@
else
libngraph_ldflags += @vsymldflags@

pkglibexec_PROGRAMS = terminal
terminal_SOURCES = terminal.c
terminal_CFLAGS = $(GTK_CFLAGS)
endif

if OS_OSX
  libngraph_ldflags += $(MAC_INTEGRATION_LIBS)
  libngraph_cflags += $(MAC_INTEGRATION_CFLAGS)
endif

lib_LTLIBRARIES = $(ngraph_lib)

libngraph_la_SOURCES = axis.c axis.h nconfig.c nconfig.h gra.c gra.h	\
	ioutil.c ioutil.h mathcode.c mathcode.h mathfn.c mathfn.h	\
	nstring.c nstring.h ntime.c ntime.h oagrid.c oarc.c oarc.h	\
	oaxis.h oaxis.c object.c object.h odarray.c odouble.c odraw.c	\
	odraw.h odata.c odata.h ofit.c ofit.h ogra.c ogra.h ogra2.c	\
	ogra2fil.c ogra2nul.c ogra2prn.c oiarray.c oiarray.h oint.c	\
	olegend.c olegend.h omark.c omath.c omerge.c omerge.h oprm.c	\
	orect.c oroot.c oroot.h osarray.c oshell.c oregexp.c		\
	ostring.c osystem.c osystem.h otext.c opath.c opath.h shell.c	\
	shell.h shell_error.h shellcm.c shellcm.h shellux.c shellux.h	\
	spline.c spline.h common.h nhash.h nhash.c strconv.c		\
	strconv.h oio.c gettext.h ngraph_api.c

libngraph_la_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) $(libngraph_cflags) -I$(top_builddir) -DCCNAME='"'$(CC)'"'
libngraph_la_LIBADD = gtk/libngraphui.la math/libmath.la
libngraph_la_LDFLAGS = $(libngraph_ldflags)

bin_PROGRAMS = ngraph
ngraph_SOURCES = main.c
ngraph_DEPENDENCIES = $(ngraph_lib)
ngraph_LDFLAGS = $(ngraph_ldflags)

include_HEADERS = ngraph.h
