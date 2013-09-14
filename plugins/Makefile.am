EXTRA_DIST = getobj.nsc create_objs.rb

pkglib_LTLIBRARIES = libruby.la

libruby_la_SOURCES = ruby.c

obj.txt: ../src/ngraph
	../src/ngraph -i getobj.nsc > obj.txt

ruby_ngraph.c: obj.txt $(EXTRA_DIST)
	ruby create_objs.rb obj.txt ruby_ngraph.c

BUILT_SOURCES = ruby_ngraph.c

libruby_la_CFLAGS = $(GTK_CFLAGS) -I.. -I /usr/include/ruby-1.9.1/ -I /usr/include/ruby-1.9.1/x86_64-linux -lruby-1.9.1

CLEANFILES = obj.txt ruby_ngraph.c
