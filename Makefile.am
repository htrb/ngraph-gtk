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
ngraph_app_bin = $(ngraph_app)/Contents/MacOS
ngraph_package = $(PACKAGE)-$(PACKAGE_VERSION)-$(build_cpu).dmg

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
