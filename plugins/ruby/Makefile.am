EXTRA_DIST = create_objs.rb extconf.rb getobj.nsc ngraph.c

if HAVE_RUBY_DEV
ngraph.so: ngraph.c ruby_ngraph.h
	mv Makefile Makefile.org
	ruby extconf.rb
	make
	mv Makefile Makefile.ruby
	mv Makefile.org Makefile

if OS_WIN32
else
install-data-local: ngraph.so
	make -f Makefile.ruby install
endif

obj.txt: getobj.nsc
	../../src/ngraph -i getobj.nsc > obj.txt

ruby_ngraph.h: obj.txt
	ruby create_objs.rb obj.txt ruby_ngraph.h

  BUILT_SOURCES = ngraph.so
  CLEANFILES = mkmf.log obj.txt ruby_ngraph.h ngraph.so Makefile.ruby
endif
