ngraphconfdir = $(sysconfdir)/$(PACKAGE)

ngraphconf_DATA = Ngraph.nsc Ngraph.ngp fit.ngp Ngraph.ini accel_map NgraphUI.xml

EXTRA_DIST = ${ngraphconf_DATA} Ngraph.ini.in Ngraph.ini.win

Ngraph.ini: Ngraph.ini.in Makefile
	sh ../pathconv.sh "$(docdir)" "$(pkglibdir)" "$(ngraphconfdir)" Ngraph.ini.in
