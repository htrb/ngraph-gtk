EXTRA_DIST = dir_defs.h.in data/css/ngraph.css				\
	data/gtk/menus-common.ui data/gtk/menus-appmenu.ui.in		\
	data/gtk/help-overlay.ui data/gtk/menus-appmenu.ui.osx		\
	data/pixmaps/ngraph_align_b.png					\
	data/pixmaps/ngraph_align_hc.png				\
	data/pixmaps/ngraph_align_l.png					\
	data/pixmaps/ngraph_align_r.png					\
	data/pixmaps/ngraph_align_t.png					\
	data/pixmaps/ngraph_align_vc.png data/pixmaps/ngraph_arc.png	\
	data/pixmaps/ngraph_axispoint.png				\
	data/pixmaps/ngraph_axiswin.png					\
	data/pixmaps/ngraph_axiswin48.png				\
	data/pixmaps/ngraph_coordwin.png				\
	data/pixmaps/ngraph_coordwin48.png				\
	data/pixmaps/ngraph_cross.png					\
	data/pixmaps/ngraph_datapoint.png				\
	data/pixmaps/ngraph_draw.png data/pixmaps/ngraph_eval.png	\
	data/pixmaps/ngraph_filewin.png					\
	data/pixmaps/ngraph_filewin48.png				\
	data/pixmaps/ngraph_frame.png data/pixmaps/ngraph_gauss.png	\
	data/pixmaps/ngraph_icon.png data/pixmaps/ngraph_icon64.png	\
	data/pixmaps/ngraph_infowin.png					\
	data/pixmaps/ngraph_infowin48.png				\
	data/pixmaps/ngraph_legendpoint.png				\
	data/pixmaps/ngraph_legendwin.png				\
	data/pixmaps/ngraph_legendwin48.png				\
	data/pixmaps/ngraph_line.png data/pixmaps/ngraph_mark.png	\
	data/pixmaps/ngraph_math.png data/pixmaps/ngraph_mergewin.png	\
	data/pixmaps/ngraph_mergewin48.png				\
	data/pixmaps/ngraph_point.png data/pixmaps/ngraph_rect.png	\
	data/pixmaps/ngraph_scale.png data/pixmaps/ngraph_section.png	\
	data/pixmaps/ngraph_single.png data/pixmaps/ngraph_text.png	\
	data/pixmaps/ngraph_trimming.png data/pixmaps/ngraph_undo.png	\
	data/pixmaps/ngraph_zoom.png data/ngraph.resources.xml

AM_CPPFLAGS = -DLOCALEDIR=\""@localedir@"\"

noinst_LTLIBRARIES = libngraphui.la

nodist_libngraphui_la_SOURCES = dir_defs.h ngraph.resources.c

libngraphui_la_SOURCES = gtk_common.h gtk_combo.c gtk_combo.h		\
	gtk_entry_completion.c gtk_entry_completion.h gtk_liststore.c	\
	gtk_liststore.h gtk_subwin.c gtk_subwin.h ogra2x11.c		\
	ogra2cairo.c ogra2emf.c ogra2cairo.h ogra2cairofile.c		\
	ogra2gtkprint.c ogra2cairofile.h ogra2x11.h ogra2gdk.c		\
	ogra2gdk.h ox11dlg.c ox11menu.c ox11menu.h x11axis.c		\
	x11axis.h x11bitmp.h x11commn.c x11commn.h x11cood.c		\
	x11cood.h x11dialg.c x11dialg.h x11file.c x11file.h		\
	x11graph.c x11graph.h x11gui.c x11gui.h x11info.c x11info.h	\
	x11lgnd.c x11lgnd.h x11lgndx.c x11menu.c x11menu.h x11merge.c	\
	x11merge.h x11opt.c x11opt.h x11opt_proto.h x11print.c		\
	x11print.h x11view.c x11view.h gtk_widget.c gtk_widget.h	\
	gtk_ruler.c gtk_ruler.h init.c init.h gtk_action.c		\
	gtk_action.h

if OS_WIN32
APPMENU_UI_SOURCE = $(srcdir)/data/gtk/menus-appmenu.ui.in
else
if OS_OSX
APPMENU_UI_SOURCE = $(srcdir)/data/gtk/menus-appmenu.ui.osx
else
APPMENU_UI_SOURCE = $(srcdir)/data/gtk/menus-appmenu.ui.in
endif
endif

data/gtk/menus-appmenu.ui: $(APPMENU_UI_SOURCE)
	cp $< $@

ngraph.resources.c: $(srcdir)/data/ngraph.resources.xml $(srcdir)/data/gtk/menus-common.ui $(srcdir)/data/css/ngraph.css $(srcdir)/data/gtk/help-overlay.ui data/gtk/menus-appmenu.ui
	$(GLIB_COMPILE_RESOURCES) --target=$@ --generate-source --sourcedir $(srcdir)/data --sourcedir $(builddir)/data $<

BUILT_SOURCES = ${nodist_libngraphui_la_SOURCES}

ngraphconfdir = $(sysconfdir)/$(PACKAGE)
pixmapdir = $(datadir)/pixmaps/$(PACKAGE)

dir_defs.h: dir_defs.h.in Makefile
	sh $(top_srcdir)/pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" $(srcdir)/dir_defs.h.in

libngraphui_la_CFLAGS = $(GTK_CFLAGS) -I$(top_builddir) -I$(builddir) -I$(srcdir)/.. -I$(srcdir)/../math

CLEANFILES = dir_defs.h $(builddir)/data/gtk/menus-appmenu.ui $(BUILT_SOURCES)
