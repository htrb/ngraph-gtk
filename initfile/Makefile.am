pkgdata_DATA = .Ngraph .Ngraph.ngp .fit.ngp Ngraph.ini accel_map

EXTRA_DIST = ${pkgdata_DATA} Ngraph.ini.in

Ngraph.ini: Ngraph.ini.in Makefile
	sh ../pathconv.sh $(docdir) $(pkgdatadir) $(demodir) Ngraph.ini.in
