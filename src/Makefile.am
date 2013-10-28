SUBDIRS = gtk math

noinst_LTLIBRARIES = libngraph.la

libngraph_la_SOURCES = axis.c axis.h nconfig.c nconfig.h gra.c gra.h	\
	ioutil.c ioutil.h mathcode.c mathcode.h mathfn.c mathfn.h	\
	nstring.c nstring.h ntime.c ntime.h oagrid.c oarc.c oarc.h	\
	oaxis.c object.c object.h odarray.c odouble.c odraw.c odraw.h	\
	ofile.c ofile.h ofit.c ofit.h ogra.c ogra.h ogra2.c		\
	ogra2fil.c ogra2nul.c ogra2prn.c oiarray.c oiarray.h oint.c	\
	olegend.c olegend.h omark.c omath.c omerge.c oprm.c orect.c	\
	oroot.c oroot.h osarray.c oshell.c oregexp.c ostring.c		\
	osystem.c otext.c otext.h opath.c opath.h shell.c shell.h	\
	shellcm.c shellcm.h shellux.c shellux.h spline.c spline.h	\
	common.h nhash.h nhash.c strconv.c strconv.h oio.c oplugin.c	\
	gettext.h ngraph_export.map

libngraph_la_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) -I.. -DCCNAME='"'$(CC)'"'
libngraph_la_LIBADD = gtk/libngraphui.la math/libmath.la $(M_LIBS) $(READLINE_LIBS) $(GTK_LIBS) $(GLIB_LIBS) $(GSL_LIBS)

bin_PROGRAMS = ngraph
ngraph_SOURCES =
ngraph_LDADD = libngraph.la
ngraph_LDFLAGS = $(RDYNAMIC) $(VER_SCRIPT)

pkglibexec_PROGRAMS = terminal
terminal_SOURCES = terminal.c
terminal_CFLAGS = $(GTK_CFLAGS)

include_HEADERS = ngraph.h
