ngraphconfdir = $(sysconfdir)/$(PACKAGE)

UI_FILE = NgraphUI.xml
ACCEL_FILE = accel_map 

ngraphconf_DATA = Ngraph.nsc Ngraph.ngp fit.ngp $(UI_FILE) $(ACCEL_FILE)

nodist_ngraphconf_DATA = Ngraph.ini

EXTRA_DIST = ${ngraphconf_DATA} Ngraph.ini.in Ngraph.ini.win NgraphUI.xml.win save_ui_file.nsc

Ngraph.ini: Ngraph.ini.in Makefile
	sh ../pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkgdatadir)" "$(ngraphconfdir)" "$(pixmapdir)" Ngraph.ini.in

update_ui_file:
	../src/ngraph -i save_ui_file.nsc $(UI_FILE) $(ACCEL_FILE)

CLEANFILES = Ngraph.ini
