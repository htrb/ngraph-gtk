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

Ngraph.ini: Ngraph.ini.in Makefile
	sh ../pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" "$(pixmapdir)" Ngraph.ini.in

update_ui_file:
	../src/ngraph -i save_ui_file.nsc $(ACCEL_FILE)

CLEANFILES = Ngraph.ini
