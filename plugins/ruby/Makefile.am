SUBDIRS = lib

EXTRA_DIST = create_objs.rb extconf.rb getobj.nsc ngraph.c

if HAVE_RUBY_DEV
  ruby_ext = ngraph.$(RUBY_DLEXT)
$(ruby_ext): ngraph.c ruby_ngraph.h
	mv Makefile Makefile.org
	ruby extconf.rb $(prefix) --with-ngraph-include="../../src" --with-ngraph-lib="../../src/.libs"
	make
	mv Makefile Makefile.ruby
	mv Makefile.org Makefile

if OS_WIN32
else
install-data-local: $(ruby_ext)
	make -f Makefile.ruby install
endif

obj.txt: getobj.nsc ../../src/libngraph.la
	../../src/ngraph -i getobj.nsc > obj.txt

ruby_ngraph.h: obj.txt
	ruby create_objs.rb obj.txt ruby_ngraph.h

  BUILT_SOURCES = $(ruby_ext)
  CLEANFILES = mkmf.log obj.txt ruby_ngraph.h $(ruby_ext) Makefile.ruby ngraph.o
endif
