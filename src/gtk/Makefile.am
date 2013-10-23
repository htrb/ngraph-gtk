EXTRA_DIST = dir_defs.h.in

AM_CPPFLAGS = -DLOCALEDIR=\""@localedir@"\"

noinst_LTLIBRARIES = libngraphui.la

nodist_libngraphui_la_SOURCES = dir_defs.h

libngraphui_la_SOURCES = gtk_common.h gtk_combo.c gtk_combo.h		\
	gtk_entry_completion.c gtk_entry_completion.h gtk_liststore.c	\
	gtk_liststore.h gtk_subwin.c gtk_subwin.h main.c main.h		\
	ogra2x11.c ogra2cairo.c ogra2emf.c ogra2cairo.h			\
	ogra2cairofile.c ogra2gtkprint.c ogra2cairofile.h ogra2x11.h	\
	ogra2gdk.c ogra2gdk.h ox11dlg.c ox11menu.c ox11menu.h		\
	x11axis.c x11axis.h x11bitmp.c x11bitmp.h x11commn.c		\
	x11commn.h x11cood.c x11cood.h x11dialg.c x11dialg.h		\
	x11file.c x11file.h x11graph.c x11graph.h x11gui.c x11gui.h	\
	x11info.c x11info.h x11lgnd.c x11lgnd.h x11lgndx.c x11menu.c	\
	x11menu.h x11merge.c x11merge.h x11opt.c x11opt.h		\
	x11opt_proto.h x11print.c x11print.h x11view.c x11view.h	\
	gtk_widget.c gtk_widget.h gtk_ruler.c gtk_ruler.h

BUILT_SOURCES = ${nodist_libngraphui_la_SOURCES}

ngraphconfdir = $(sysconfdir)/$(PACKAGE)
pixmapdir = $(datadir)/pixmaps/$(PACKAGE)

dir_defs.h: dir_defs.h.in Makefile
	sh ../../pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" "$(pixmapdir)" dir_defs.h.in

libngraphui_la_LDFLAGS = -static

libngraphui_la_CFLAGS = $(GTK_CFLAGS) -I.. -I../math

CLEANFILES = dir_defs.h
