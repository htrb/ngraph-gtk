pkgdata_DATA = gra2ps.ini Ngraph.ps gra2wmf.ini

EXTRA_DIST = dir_defs.h.in ${pkgdata_DATA}

bin_PROGRAMS = gra2ps gra2wmf

gra2ps_SOURCES = gra2ps.c

gra2wmf_SOURCES = gra2wmf.c wmfapi.c wmfapi.h

gra2wmf.c: dir_defs.h
gra2ps.c: dir_defs.h

dir_defs.h: dir_defs.h.in
	sh ../pathconv.sh $(docdir) $(pkgdatadir) $(demodir) dir_defs.h.in
