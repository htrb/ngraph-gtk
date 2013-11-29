SUBDIRS = gtk math

EXTRA_DIST = ngraph_export.map ngraph_resource.rc libngraph_resource.rc ngraph.ico

libngraph_ldflags = -version-info $(LIBNGRAPH_VERSION) $(M_LIBS) $(READLINE_LIBS) $(GTK_LIBS) $(GLIB_LIBS) $(GSL_LIBS)
ngraph_ldflags = -lngraph

if OS_WIN32
libngraph_ldflags += -no-undefined -export-symbols ngraph.def -Wl,libngraph_resource.o
ngraph_ldflags += -Wl,ngraph_resource.o

BUILT_SOURCES = ngraph.def libngraph_resource.o ngraph_resource.o
CLEANFILES = $(BUILT_SOURCES)

.rc.o:
	windres -o $@ $<

ngraph.def: ngraph_api.c
	echo "EXPORTS" > $@
	cat $^ | grep '^ngraph_' | sed 's/[^a-z0-9_].*//' >> $@
else
libngraph_ldflags += -Wl,--version-script,ngraph_export.map

pkglibexec_PROGRAMS = terminal
terminal_SOURCES = terminal.c
terminal_CFLAGS = $(GTK_CFLAGS)
endif

lib_LTLIBRARIES = libngraph.la

libngraph_la_SOURCES = axis.c axis.h nconfig.c nconfig.h gra.c gra.h	\
	ioutil.c ioutil.h mathcode.c mathcode.h mathfn.c mathfn.h	\
	nstring.c nstring.h ntime.c ntime.h oagrid.c oarc.c oarc.h	\
	oaxis.c object.c object.h odarray.c odouble.c odraw.c odraw.h	\
	ofile.c ofile.h ofit.c ofit.h ogra.c ogra.h ogra2.c		\
	ogra2fil.c ogra2nul.c ogra2prn.c oiarray.c oiarray.h oint.c	\
	olegend.c olegend.h omark.c omath.c omerge.c oprm.c orect.c	\
	oroot.c oroot.h osarray.c oshell.c oregexp.c ostring.c		\
	osystem.c osystem.h otext.c otext.h opath.c opath.h shell.c	\
	shell.h shellcm.c shellcm.h shellux.c shellux.h spline.c	\
	spline.h common.h nhash.h nhash.c strconv.c strconv.h oio.c	\
	gettext.h ngraph_api.c

libngraph_la_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) -I.. -DCCNAME='"'$(CC)'"'
libngraph_la_LIBADD = gtk/libngraphui.la math/libmath.la
libngraph_la_LDFLAGS = $(libngraph_ldflags)

bin_PROGRAMS = ngraph
ngraph_SOURCES = main.c
ngraph_LDFLAGS = $(ngraph_ldflags)

include_HEADERS = ngraph.h
