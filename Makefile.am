ngraphconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = config.rpath pathconv.sh

doc_DATA = README AUTHORS ChangeLog NEWS COPYING

SUBDIRS = po m4 src initfile driver addin demo doc zsh test mingw pixmap icons misc plugins

ACLOCAL_AMFLAGS = -I m4

if OS_WIN32
AM_CFLAGS = -mms-bitfields
endif

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

test_all:
	(cd test; $(MAKE) test)
