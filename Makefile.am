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

OBJECT_DEFINITION = $(abs_top_srcdir)/src/oagrid.c			\
	$(abs_top_srcdir)/src/oarc.c $(abs_top_srcdir)/src/odarray.c	\
	$(abs_top_srcdir)/src/odata.c $(abs_top_srcdir)/src/odouble.c	\
	$(abs_top_srcdir)/src/odraw.c $(abs_top_srcdir)/src/ofit.c	\
	$(abs_top_srcdir)/src/ogra.c $(abs_top_srcdir)/src/ogra2.c	\
	$(abs_top_srcdir)/src/ogra2fil.c				\
	$(abs_top_srcdir)/src/ogra2nul.c				\
	$(abs_top_srcdir)/src/ogra2prn.c				\
	$(abs_top_srcdir)/src/oiarray.c $(abs_top_srcdir)/src/oint.c	\
	$(abs_top_srcdir)/src/oio.c $(abs_top_srcdir)/src/olegend.c	\
	$(abs_top_srcdir)/src/omark.c $(abs_top_srcdir)/src/omath.c	\
	$(abs_top_srcdir)/src/omerge.c					\
	$(abs_top_srcdir)/src/oparameter.c				\
	$(abs_top_srcdir)/src/opath.c $(abs_top_srcdir)/src/oprm.c	\
	$(abs_top_srcdir)/src/orect.c $(abs_top_srcdir)/src/oregexp.c	\
	$(abs_top_srcdir)/src/oroot.c $(abs_top_srcdir)/src/osarray.c	\
	$(abs_top_srcdir)/src/oshell.c					\
	$(abs_top_srcdir)/src/ostring.c					\
	$(abs_top_srcdir)/src/osystem.c $(abs_top_srcdir)/src/otext.c	\
	$(abs_top_srcdir)/src/gtk/ogra2cairo.c				\
	$(abs_top_srcdir)/src/gtk/ogra2cairofile.c			\
	$(abs_top_srcdir)/src/gtk/ogra2emf.c				\
	$(abs_top_srcdir)/src/gtk/ogra2gdk.c				\
	$(abs_top_srcdir)/src/gtk/ogra2gtkprint.c			\
	$(abs_top_srcdir)/src/gtk/ogra2x11.c				\
	$(abs_top_srcdir)/src/gtk/ox11dlg.c				\
	$(abs_top_srcdir)/src/gtk/ox11menu.c
export OBJECT_DEFINITION
