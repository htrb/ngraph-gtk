NGRAPH_ICONS = data/pixmaps/ngraph_align_b.png				\
	data/pixmaps/ngraph_align_hc.png				\
	data/pixmaps/ngraph_align_l.png					\
	data/pixmaps/ngraph_align_r.png					\
	data/pixmaps/ngraph_align_t.png					\
	data/pixmaps/ngraph_align_vc.png data/pixmaps/ngraph_icon.png	\
	data/pixmaps/ngraph_icon64.png data/pixmaps/ngraph_math.png	\
	data/pixmaps/none_begin-symbolic.svg				\
	data/pixmaps/none_end-symbolic.svg				\
	data/pixmaps/arrow_begin-symbolic.svg				\
	data/pixmaps/arrow_end-symbolic.svg				\
	data/pixmaps/wave_begin-symbolic.svg				\
	data/pixmaps/wave_end-symbolic.svg				\
	data/pixmaps/mark_begin-symbolic.svg				\
	data/pixmaps/mark_end-symbolic.svg				\
	data/pixmaps/bar_begin-symbolic.svg				\
	data/pixmaps/bar_end-symbolic.svg				\
	data/pixmaps/stroke_fill_0-symbolic.svg				\
	data/pixmaps/stroke_fill_1-symbolic.svg				\
	data/pixmaps/stroke_fill_2-symbolic.svg				\
	data/pixmaps/stroke_fill_3-symbolic.svg				\
	data/pixmaps/stroke_fill_4-symbolic.svg				\
	data/pixmaps/stroke_fill_5-symbolic.svg				\
	data/pixmaps/stroke_fill_6-symbolic.svg				\
	data/pixmaps/stroke_fill_7-symbolic.svg				\
	data/pixmaps/join_bevel-symbolic.svg				\
	data/pixmaps/join_miter-symbolic.svg				\
	data/pixmaps/join_round-symbolic.svg data/pixmaps/ngraph.svg	\
	data/pixmaps/ngraph_draw-symbolic.svg				\
	data/pixmaps/ngraph_draw-attention-symbolic.svg			\
	data/pixmaps/ngraph_eval-symbolic.svg				\
	data/pixmaps/ngraph_line-symbolic.svg				\
	data/pixmaps/ngraph_single-symbolic.svg				\
	data/pixmaps/ngraph_arc-symbolic.svg				\
	data/pixmaps/ngraph_filewin-symbolic.svg			\
	data/pixmaps/ngraph_mark-symbolic.svg				\
	data/pixmaps/ngraph_text-symbolic.svg				\
	data/pixmaps/ngraph_axispoint-symbolic.svg			\
	data/pixmaps/ngraph_frame-symbolic.svg				\
	data/pixmaps/ngraph_mergewin-symbolic.svg			\
	data/pixmaps/ngraph_trimming-symbolic.svg			\
	data/pixmaps/ngraph_axiswin-symbolic.svg			\
	data/pixmaps/ngraph_gauss-symbolic.svg				\
	data/pixmaps/ngraph_point-symbolic.svg				\
	data/pixmaps/ngraph_zoom-symbolic.svg				\
	data/pixmaps/ngraph_rect-symbolic.svg				\
	data/pixmaps/ngraph_cross-symbolic.svg				\
	data/pixmaps/ngraph_legendpoint-symbolic.svg			\
	data/pixmaps/ngraph_scale-symbolic.svg				\
	data/pixmaps/ngraph_datapoint-symbolic.svg			\
	data/pixmaps/ngraph_section-symbolic.svg			\
	data/pixmaps/ngraph_exchange-symbolic.svg			\
	data/pixmaps/ngraph_link-symbolic.svg				\
	data/pixmaps/linewidth_002-symbolic.svg				\
	data/pixmaps/linewidth_004-symbolic.svg				\
	data/pixmaps/linewidth_008-symbolic.svg				\
	data/pixmaps/linewidth_016-symbolic.svg				\
	data/pixmaps/linewidth_032-symbolic.svg				\
	data/pixmaps/linewidth_064-symbolic.svg				\
	data/pixmaps/linewidth_128-symbolic.svg				\
	data/pixmaps/ngraph_parameter-symbolic.svg

EXTRA_DIST = dir_defs.h.in data/css/ngraph.css				\
	data/gtk/menus-common.ui.in data/gtk/menus-common-win.ui.in	\
	data/gtk/menus-common.ui.osx data/gtk/menus-appmenu.ui		\
	data/gtk/help-overlay.ui data/gtk/menus-tool.ui			\
	create_completion_info.rb completion_info_functions.txt		\
	completion_info_constants.txt data/ngraphui.resources.xml.in	\
	data/ngraphui.resources.xml.osx $(NGRAPH_ICONS)

AM_CPPFLAGS = -DLOCALEDIR=\""@localedir@"\"

noinst_LTLIBRARIES = libngraphui.la

nodist_libngraphui_la_SOURCES = dir_defs.h ngraphui.resources.c

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
	gtk_action.h sourcecompletionwords.c sourcecompletionwords.h	\
	completion_info.c completion_info.h completion_info_data.c	\
	gtk_presettings.c gtk_presettings.h x11parameter.c		\
	x11parameter.h

libngraph_cflags =

if OS_WIN32
UI_RESOURCES_SOURCE = $(srcdir)/data/ngraphui.resources.xml.in
MENU_UI_SOURCE = $(srcdir)/data/gtk/menus-common-win.ui.in
else
if OS_OSX
UI_RESOURCES_SOURCE = $(srcdir)/data/ngraphui.resources.xml.osx
MENU_UI_SOURCE = $(srcdir)/data/gtk/menus-common.ui.osx
libngraph_cflags += $(MAC_INTEGRATION_CFLAGS)
else
UI_RESOURCES_SOURCE = $(srcdir)/data/ngraphui.resources.xml.in
MENU_UI_SOURCE = $(srcdir)/data/gtk/menus-common.ui.in
endif
endif

uidir = data/gtk
data/ngraphui.resources.xml: $(UI_RESOURCES_SOURCE)
	$(MKDIR_P) $(uidir)
	cp $< $@

$(uidir)/menus-common.ui: $(MENU_UI_SOURCE)
	$(MKDIR_P) $(uidir)
	cp $< $@

ngraphui.resources.c: data/ngraphui.resources.xml $(uidir)/menus-common.ui $(srcdir)/data/gtk/menus-tool.ui $(srcdir)/data/css/ngraph.css $(srcdir)/data/gtk/help-overlay.ui $(uidir)/menus-appmenu.ui $(NGRAPH_ICONS)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --generate-source --sourcedir $(srcdir)/data --sourcedir $(builddir)/data $<

BUILT_SOURCES = ${nodist_libngraphui_la_SOURCES}

ngraphconfdir = $(sysconfdir)/$(PACKAGE)
pixmapdir = $(datadir)/pixmaps/$(PACKAGE)

dir_defs.h: dir_defs.h.in $(top_srcdir)/pathconv.sh
	$(SHELL) $(top_srcdir)/pathconv.sh $(srcdir)/dir_defs.h.in

completion_info_data.c: $(srcdir)/completion_info_functions.txt $(srcdir)/completion_info_constants.txt $(srcdir)/create_completion_info.rb
	ruby $(srcdir)/create_completion_info.rb $(srcdir)/completion_info_functions.txt $(srcdir)/completion_info_constants.txt completion_info_data.c

libngraphui_la_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) $(libngraph_cflags) -I$(top_builddir) -I$(builddir) -I$(srcdir)/.. -I$(srcdir)/../math $(GTKSOURCEVIEW_CFLAGS)
libngraphui_la_LDFLAGS = $(GTKSOURCEVIEW_LIBS)

CLEANFILES = dir_defs.h $(builddir)/data/gtk/menus-common.ui $(builddir)/data/ngraphui.resources.xml $(BUILT_SOURCES)
