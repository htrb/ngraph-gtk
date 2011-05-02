ngraphconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = config.rpath pathconv.sh

doc_DATA = README AUTHORS ChangeLog NEWS COPYING

SUBDIRS = po m4 src initfile driver addin demo doc debian zsh test mingw pixmap

ACLOCAL_AMFLAGS = -I m4

LIBTOOL_DEPS = @LIBTOOL_DEPS@

test_all:
	(cd test; $(MAKE) test)
