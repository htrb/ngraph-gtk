ngraphconfdir = $(sysconfdir)/$(PACKAGE)
ngraphinitdir = $(sysconfdir)/$(PACKAGE)/init.d

UI_FILE = NgraphUI.xml
ACCEL_FILE = accel_map 

dist_ngraphinit_DATA = 10append_addins.nsc
dist_ngraphconf_DATA = Ngraph.nsc Ngraph.ngp fit.ngp $(UI_FILE) $(ACCEL_FILE)
nodist_ngraphconf_DATA = Ngraph.ini

EXTRA_DIST = Ngraph.ini.in Ngraph.ini.win NgraphUI.xml.win save_ui_file.nsc

Ngraph.ini: Ngraph.ini.in Makefile
	sh ../pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" "$(pixmapdir)" Ngraph.ini.in

update_ui_file:
	../src/ngraph -i save_ui_file.nsc $(UI_FILE) $(ACCEL_FILE)

CLEANFILES = Ngraph.ini
