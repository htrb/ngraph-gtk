EXTRA_DIST = create_objs.rb extconf.rb getobj.nsc ngraph.c lib/ngraph.rb lib/ngraph.rb.win lib/ngraph.rb.osx lib/ngraph/ngp2.rb

if HAVE_RUBY_DEV
  ruby_ext = ngraph.$(RUBY_DLEXT)
$(ruby_ext): ngraph.c ruby_ngraph.h
	mv Makefile Makefile.org
	ruby $(srcdir)/extconf.rb $(prefix) --with-ngraph-include=$(top_srcdir)/src --with-ngraph-lib=$(top_builddir)/src/.libs
	$(MAKE) $(AM_MAKEFLAGS)
	mv Makefile Makefile.ruby
	mv Makefile.org Makefile

if OS_WIN32
install-data-local: $(ruby_ext)
	$(INSTALL) $(builddir)/$(ruby_ext) $(pkglibdir)/ruby/$(ruby_ext)
	$(INSTALL_DATA) $(srcdir)/lib/ngraph.rb.win  $(pkglibdir)/ruby/ngraph.rb
	$(INSTALL_DATA) $(srcdir)/lib/ngraph/ngp2.rb $(pkglibdir)/ruby/ngraph/ngp2.rb
else
if OS_OSX
	$(INSTALL) $(builddir)/$(ruby_ext) $(pkglibdir)/ruby/$(ruby_ext)
	$(INSTALL_DATA) $(srcdir)/lib/ngraph.rb.osx  $(pkglibdir)/ruby/ngraph.rb
	$(INSTALL_DATA) $(srcdir)/lib/ngraph/ngp2.rb $(pkglibdir)/ruby/ngraph/ngp2.rb
else
install-data-local: $(ruby_ext)
	$(MAKE) -f Makefile.ruby install
endif
endif

obj.txt: getobj.nsc $(top_builddir)/src/libngraph.la
	$(top_builddir)/src/ngraph -i $(srcdir)/getobj.nsc > $(srcdir)/obj.txt

ruby_ngraph.h: obj.txt
	ruby $(srcdir)/create_objs.rb $(srcdir)/obj.txt $(srcdir)/ruby_ngraph.h

  BUILT_SOURCES = $(ruby_ext)
  CLEANFILES = mkmf.log obj.txt ruby_ngraph.h $(ruby_ext) Makefile.ruby ngraph.o .*.time
endif
