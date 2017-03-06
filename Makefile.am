ngraphconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = config.rpath pathconv.sh winrc_version.awk

doc_DATA = README AUTHORS ChangeLog NEWS COPYING

SUBDIRS = po m4 src initfile driver addin demo doc zsh test mingw icons misc plugins osx

ACLOCAL_AMFLAGS = -I m4

if OS_WIN32
AM_CFLAGS = -mms-bitfields
endif

if OS_OSX

ngraph_app = ~/Desktop/$(PACKAGE).app
ngraph_package = $(PACKAGE)-$(PACKAGE_VERSION)-x86_64.dmg

bundler: $(ngraph_app)

$(ngraph_app): osx/ngraph-gtk.bundle $(bindir)/ngraph
	rm -rf $(ngraph_app)
	gtk-mac-bundler $<

dmg: $(ngraph_package)

$(ngraph_package): $(ngraph_app)
	$(RM) $(ngraph_package)
	hdiutil create -srcfolder $(ngraph_app) -volname "$PACKAGE $PACKAGE_VERSION" -format UDBZ -o $(ngraph_package)

CLEANFILES = $(ngraph_package)
endif

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) $(top_srcdir)/config.status --recheck

test_all:
	(cd test; $(MAKE) test)
