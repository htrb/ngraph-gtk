ngraphconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = config.rpath pathconv.sh winrc_version.awk

doc_DATA = README AUTHORS ChangeLog NEWS COPYING

SUBDIRS = po m4 src initfile driver addin demo doc zsh test mingw icons misc plugins osx

ACLOCAL_AMFLAGS = -I m4

if OS_WIN32
AM_CFLAGS = -mms-bitfields
endif

if OS_OSX
bundler: osx/ngraph-gtk.bundle
	rm -rf ~/Desktop/Ngraph-GTK.app
	gtk-mac-bundler $<
endif

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) $(top_srcdir)/config.status --recheck

test_all:
	(cd test; $(MAKE) test)
