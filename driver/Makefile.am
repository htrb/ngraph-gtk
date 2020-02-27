ngraphconfdir = $(sysconfdir)/$(PACKAGE)

ngraphconf_DATA = # gra2ps.ini Ngraph.ps gra2wmf.ini

EXTRA_DIST = dir_defs.h.in ngp2.in ${ngraphconf_DATA}

#bin_PROGRAMS = gra2ps gra2wmf

bin_SCRIPTS = ngp2

# dir_defs.h: dir_defs.h.in Makefile
# 	$(SHELL) ../pathconv.sh "$(docdir)" "$(pkgdatadir)" "$(ngraphconfdir)" dir_defs.h.in

ngp2: ngp2.in $(top_srcdir)/pathconv.sh
	$(SHELL) $(top_srcdir)/pathconv.sh $(srcdir)/ngp2.in

# gra2ps_SOURCES = gra2ps.c
# nodist_gra2ps_SOURCES = dir_defs.h
# gra2ps_LDADD = $(GLIB_LIBS)
# gra2ps_CFLAGS = $(GLIB_CFLAGS)

# gra2wmf_SOURCES = gra2wmf.c wmfapi.c wmfapi.h
# nodist_gra2wmf_SOURCES = dir_defs.h
# gra2wmf_LDADD = $(GLIB_LIBS)
# gra2wmf_CFLAGS = $(GLIB_CFLAGS)

#BUILT_SOURCES = dir_defs.h

CLEANFILES = ngp2 dir_defs.h
