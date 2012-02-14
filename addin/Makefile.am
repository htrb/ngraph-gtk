addindir = $(pkgdatadir)/addin

dist_addin_DATA = append.nsc calc.nsc fft.nsc fitrslt.nsc legend.nsc	\
	math.nsc text-in.nsc cal.nsc tex_equation.nsc ps2gra.nsc	\
	fig2gra.nsc timer.nsc fig2gra.rb

APP_CFLAGS = 

pkglibexec_PROGRAMS = fft calc fitrslt legend

calc_SOURCES = calc.c addin_common.c addin_common.h
calc_CFLAGS = $(GTK_CFLAGS) $(APP_CFLAGS)
calc_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

fitrslt_SOURCES = fitrslt.c addin_common.c addin_common.h
fitrslt_CFLAGS = $(GTK_CFLAGS) $(APP_CFLAGS)
fitrslt_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

legend_SOURCES = legend.c addin_common.c addin_common.h
legend_CFLAGS = $(GTK_CFLAGS) $(APP_CFLAGS)
legend_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

fft_SOURCES = fft.c
