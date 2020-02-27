ngraphconfdir = $(sysconfdir)/$(PACKAGE)
ngraphinitdir = $(sysconfdir)/$(PACKAGE)/init.d

ACCEL_FILE = accel_map

init_scripts = 10append_addins.nsc
if HAVE_RUBY_DEV
  init_scripts += 20append_addins_ruby.nsc
endif
dist_ngraphinit_DATA = $(init_scripts)
dist_ngraphconf_DATA = Ngraph.nsc Ngraph.ngp fit.ngp $(ACCEL_FILE)
nodist_ngraphconf_DATA = Ngraph.ini

EXTRA_DIST = Ngraph.ini.in Ngraph.ini.win save_ui_file.nsc

if OS_WIN32
Ngraph.ini: Ngraph.ini.win
	cp $< $@
else
if OS_OSX
Ngraph.ini: Ngraph.ini.osx
	cp $< $@
else
Ngraph.ini: $(srcdir)/Ngraph.ini.in $(top_srcdir)/pathconv.sh
	$(SHELL) $(top_srcdir)/pathconv.sh $<
endif
endif

update_ui_file:
	$(top_srcdir)/src/ngraph -i save_ui_file.nsc $(ACCEL_FILE)

CLEANFILES = Ngraph.ini
