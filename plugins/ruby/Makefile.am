EXTRA_DIST = create_objs.rb extconf.rb getobj.nsc ngraph.c lib/ngraph.rb lib/ngraph.rb.win lib/ngraph.rb.osx lib/ngraph/ngp2.rb obj.txt

if HAVE_RUBY_DEV
  ruby_ext = $(builddir)/ngraph.$(RUBY_DLEXT)
if OS_OSX
$(ruby_ext): ngraph.c $(builddir)/ruby_ngraph.h
	mv Makefile Makefile.org
	ARCHFLAGS="-arch x86_64" ruby $(srcdir)/extconf.rb $(prefix) --with-ngraph-include=$(top_srcdir)/src --with-ngraph-lib=$(top_builddir)/src/.libs
	mv Makefile Makefile.ruby
	mv Makefile.org Makefile
	$(MAKE) -f Makefile.ruby $(AM_MAKEFLAGS)
else
$(ruby_ext): ngraph.c $(builddir)/ruby_ngraph.h
	mv Makefile Makefile.org
	ruby $(srcdir)/extconf.rb $(prefix) --with-ngraph-include=$(top_srcdir)/src --with-ngraph-lib=$(top_builddir)/src/.libs
	mv Makefile Makefile.ruby
	mv Makefile.org Makefile
	$(MAKE) -f Makefile.ruby $(AM_MAKEFLAGS)
endif


if OS_WIN32
install-data-local: $(ruby_ext)
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/ruby/ngraph
	$(INSTALL) $(builddir)/$(ruby_ext) $(DESTDIR)$(pkglibdir)/ruby/$(ruby_ext)
	$(INSTALL_DATA) $(srcdir)/lib/ngraph.rb.win  $(DESTDIR)$(pkglibdir)/ruby/ngraph.rb
	$(INSTALL_DATA) $(srcdir)/lib/ngraph/ngp2.rb $(DESTDIR)$(pkglibdir)/ruby/ngraph/ngp2.rb
else
if OS_OSX
install-data-local: $(ruby_ext)
	$(MKDIR_P) $(DESTDIR)$(pkglibdir)/ruby/ngraph
	$(INSTALL) $(builddir)/$(ruby_ext) $(DESTDIR)$(pkglibdir)/ruby/$(ruby_ext)
	$(INSTALL_DATA) $(srcdir)/lib/ngraph.rb.osx  $(DESTDIR)$(pkglibdir)/ruby/ngraph.rb
	$(INSTALL_DATA) $(srcdir)/lib/ngraph/ngp2.rb $(DESTDIR)$(pkglibdir)/ruby/ngraph/ngp2.rb
else
install-data-local: $(ruby_ext)
	$(MAKE) -f Makefile.ruby install
endif
endif

obj.txt: getobj.nsc $(OBJECT_DEFINITION)
	$(top_builddir)/src/ngraph -i $(srcdir)/getobj.nsc > $(srcdir)/obj.txt

$(builddir)/ruby_ngraph.h: obj.txt create_objs.rb
	ruby $(srcdir)/create_objs.rb $(srcdir)/obj.txt $(builddir)/ruby_ngraph.h

  BUILT_SOURCES = $(ruby_ext)
  CLEANFILES = mkmf.log $(builddir)/ruby_ngraph.h $(ruby_ext) Makefile.ruby ngraph.o .*.time
endif
