SUBDIRS = gtk math

bin_PROGRAMS = ngraph

ngraph_SOURCES = axis.c axis.h nconfig.c nconfig.h gra.c gra.h		\
	ioutil.c ioutil.h jnstring.c jnstring.h mathcode.c mathcode.h	\
	mathfn.c mathfn.h ngraph.h nstring.c nstring.h ntime.c ntime.h	\
	oagrid.c oarc.c oarc.h oaxis.c object.c object.h ocurve.c	\
	odarray.c odouble.c odraw.c odraw.h ofile.c ofile.h ofit.c	\
	ofit.h ogra.c ogra.h ogra2.c ogra2fil.c ogra2nul.c ogra2prn.c	\
	oiarray.c oint.c olegend.c olegend.h oline.c omark.c omath.c	\
	omerge.c opolygon.c oprm.c orect.c oroot.c oroot.h osarray.c	\
	oshell.c ostring.c osystem.c otext.c otext.h shell.c shell.h	\
	shellcm.c shellcm.h shellux.c shellux.h spline.c spline.h	\
	gettext.h common.h nhash.h nhash.c strconv.c strconv.h oio.c

pkglib_PROGRAMS = terminal
terminal_SOURCES = terminal.c

ngraph_LDADD = $(GTK_LIBS) $(GSL_LIBS) gtk/libngraph.la math/libmath.la

ngraph_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) -I.. -D_FORTIFY_SOURCE
