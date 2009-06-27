ngraphconfdir = $(sysconfdir)/$(PACKAGE)

ngraphconf_DATA = gra2ps.ini Ngraph.ps gra2wmf.ini

EXTRA_DIST = dir_defs.h.in ngp2.in ${ngraphconf_DATA}

bin_PROGRAMS = gra2ps gra2wmf

bin_SCRIPTS = ngp2

gra2ps_SOURCES = gra2ps.c dir_defs.h

gra2wmf_SOURCES = gra2wmf.c wmfapi.c wmfapi.h dir_defs.h

dir_defs.h: dir_defs.h.in Makefile
	sh ../pathconv.sh $(docdir) $(pkgdatadir) $(ngraphconfdir) dir_defs.h.in

ngp2: ngp2.in Makefile
	sh ../pathconv.sh $(docdir) $(pkgdatadir) $(bindir) ngp2.in

gra2ps_CFLAGS = -D_FORTIFY_SOURCE
gra2wmf_CFLAGS = -D_FORTIFY_SOURCE
