SUBDIRS = gtk math

bin_PROGRAMS = ngraph

ngraph_SOURCES = axis.c axis.h nconfig.c nconfig.h gra.c gra.h		\
	ioutil.c ioutil.h mathcode.c mathcode.h mathfn.c mathfn.h	\
	ngraph.h nstring.c nstring.h ntime.c ntime.h oagrid.c oarc.c	\
	oarc.h oaxis.c object.c object.h odarray.c odouble.c odraw.c	\
	odraw.h ofile.c ofile.h ofit.c ofit.h ogra.c ogra.h ogra2.c	\
	ogra2fil.c ogra2nul.c ogra2prn.c oiarray.c oiarray.h oint.c	\
	olegend.c olegend.h omark.c omath.c omerge.c oprm.c orect.c	\
	oroot.c oroot.h osarray.c oshell.c oregexp.c ostring.c		\
	osystem.c otext.c otext.h opath.c opath.h shell.c shell.h	\
	shellcm.c shellcm.h shellux.c shellux.h spline.c spline.h	\
	common.h nhash.h nhash.c strconv.c strconv.h oio.c plugins.c	\
	plugins.h gettext.h

pkglibexec_PROGRAMS = terminal
terminal_SOURCES = terminal.c

ngraph_LDADD = gtk/libngraph.la math/libmath.la $(READLINE_LIBS) $(GTK_LIBS) $(GLIB_LIBS) $(GSL_LIBS)

ngraph_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) -I.. -DCCNAME='"'$(CC)'"'

terminal_CFLAGS = $(GTK_CFLAGS)
