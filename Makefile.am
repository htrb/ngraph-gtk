ngraphconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = config.rpath pathconv.sh

doc_DATA = README AUTHORS ChangeLog NEWS COPYING

SUBDIRS = m4 src initfile driver addin demo doc zsh test mingw icons misc plugins osx gtksourceview po

ACLOCAL_AMFLAGS = -I m4

if OS_WIN32
AM_CFLAGS = -mms-bitfields
endif

if OS_OSX

ngraph_app = ~/Desktop/$(PACKAGE).app
ngraph_arch = $(shell uname -m)
ngraph_app_bin = $(ngraph_app)/Contents/MacOS
ngraph_package = $(PACKAGE)-$(PACKAGE_VERSION)-$(ngraph_arch).dmg

bundler: $(ngraph_app)

$(ngraph_app): osx/ngraph-gtk.bundle $(bindir)/ngraph
	rm -rf $(ngraph_app)
	gtk-mac-bundler $<
	mv $(ngraph_app_bin)/ngraph-gtk-bin $(ngraph_app_bin)/ngraph-gtk

dmg: $(ngraph_package)

$(ngraph_package): $(ngraph_app)
	$(RM) $(ngraph_package)
	hdiutil create -srcfolder $(ngraph_app) -volname "$(PACKAGE) $(PACKAGE_VERSION)" -format UDBZ -o $(ngraph_package)

CLEANFILES = $(ngraph_package)
endif

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) $(top_srcdir)/config.status --recheck

test_all:
	(cd test; $(MAKE) test)

OBJECT_DEFINITION = $(top_builddir)/src/oagrid.c			\
	$(top_builddir)/src/oarc.c $(top_builddir)/src/odarray.c	\
	$(top_builddir)/src/odata.c $(top_builddir)/src/odouble.c	\
	$(top_builddir)/src/odraw.c $(top_builddir)/src/ofit.c		\
	$(top_builddir)/src/ogra.c $(top_builddir)/src/ogra2.c		\
	$(top_builddir)/src/ogra2fil.c $(top_builddir)/src/ogra2nul.c	\
	$(top_builddir)/src/ogra2prn.c $(top_builddir)/src/oiarray.c	\
	$(top_builddir)/src/oint.c $(top_builddir)/src/oio.c		\
	$(top_builddir)/src/olegend.c $(top_builddir)/src/omark.c	\
	$(top_builddir)/src/omath.c $(top_builddir)/src/omerge.c	\
	$(top_builddir)/src/oparameter.c $(top_builddir)/src/opath.c	\
	$(top_builddir)/src/oprm.c $(top_builddir)/src/orect.c		\
	$(top_builddir)/src/oregexp.c $(top_builddir)/src/oroot.c	\
	$(top_builddir)/src/osarray.c $(top_builddir)/src/oshell.c	\
	$(top_builddir)/src/ostring.c $(top_builddir)/src/osystem.c	\
	$(top_builddir)/src/otext.c					\
	$(top_builddir)/src/gtk/ogra2cairo.c				\
	$(top_builddir)/src/gtk/ogra2cairofile.c			\
	$(top_builddir)/src/gtk/ogra2emf.c				\
	$(top_builddir)/src/gtk/ogra2gdk.c				\
	$(top_builddir)/src/gtk/ogra2gtkprint.c				\
	$(top_builddir)/src/gtk/ogra2x11.c				\
	$(top_builddir)/src/gtk/ox11dlg.c				\
	$(top_builddir)/src/gtk/ox11menu.c
